
#include "threadpool.h"
#include "debug.h"
int threadpool_init(threadpool_t* pool, threadpool_attr_t* attr)
{
    // 初始化空闲线程池的链表
    list_init(&pool->list);
    
    // 为线程池分配内存
    pool->threads_buf = (thread_t*)malloc(sizeof(thread_t) * attr->threads_nr);
    if (pool->threads_buf == NULL) {
        dbg_error("Failed to allocate memory for threads\n");
        return -1;
    }
    
    memset(pool->threads_buf, 0, sizeof(thread_t) * attr->threads_nr);
    
    // 初始化每个线程节点并将其加入到线程池的空闲线程列表中
    thread_t* cur = pool->threads_buf;
    for (int i = 0; i < attr->threads_nr; ++i) {
        list_insert_last(&pool->list, &cur->node);
        cur++;
    }
    
    return 0;
}
thread_t* thread_create(threadpool_t* pool, thread_func_t func, void* func_arg)
{
    // 如果线程池没有可用的线程，返回 NULL
    if (list_count(&pool->list) == 0) {
        dbg_error("There is no more available thread\n");
        return NULL;
    }

    // 从线程池的空闲线程列表中移除一个线程节点
    list_node_t* tnode = list_remove_first(&pool->list);
    // 获取线程节点对应的线程结构体
    thread_t* thread = list_node_parent(tnode, thread_t, node);

    // 创建线程
    if (pthread_create(&thread->thread, NULL, func, func_arg) != 0) {
        dbg_error("Thread creation failed\n");
        
        // 线程创建失败，释放线程资源并将节点重新插入到空闲列表
        list_insert_last(&pool->list, &thread->node);
        return NULL;
    }

    // 线程创建成功，返回线程结构体
    return thread;
}
int thread_join(threadpool_t* pool, thread_t* thread, void** ret)
{
    // 等待线程结束并收集返回值
    if (pthread_join(thread->thread, ret) != 0) {
        dbg_error("Failed to join thread\n");
        return -1;
    }

    // 线程已经结束，将其重新加入到线程池的空闲线程列表
    list_insert_last(&pool->list, &thread->node);
    
    return 0;
}


int threads_list_count(threadpool_t* pool)
{
    return list_count(&pool->list);
}

int lock(lock_t* lk)
{
    int ret;
    ret =  pthread_mutex_lock(lk);
    return ret==0?0:-1;
}
int unlock(lock_t* lk)
{
    int ret;
    ret =  pthread_mutex_unlock(lk);
    return ret==0?0:-1;
}

int lock_init(lock_t* lk)
{
    int ret;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

    ret = pthread_mutex_init(lk, &attr);
    pthread_mutexattr_destroy(&attr);
    return ret==0?0:-1;
}

int lock_destory(lock_t* lk)
{
    int ret;
    ret = pthread_mutex_destroy(lk);
    return ret==0?0:-1;
}

int semaphore_init(semaphore_t* sem,uint32_t count)
{
    int ret;
    ret = sem_init(sem,0,count);
    return ret==0?0:-1;
}
int semaphore_destory(semaphore_t* sem)
{
    int ret;
    ret = sem_destroy(sem);
    return ret==0?0:-1;
}

int wait(semaphore_t* sem)
{
    int ret;
    ret = sem_wait(sem);
    return ret==0?0:-1;
}


/*毫秒*/
int time_wait(semaphore_t *sem, int timeout) {
    if (timeout < 0) {
        // timeout < 0 时，尝试非阻塞地获取信号量
        int ret = sem_trywait(sem);
        if (ret == 0) {
            return 0;  // 成功获取信号量
        } else {
            return -1;  // 没有资源，立即返回
        }
    } else if (timeout == 0) {
        // timeout == 0 时，一直阻塞等待直到获取到信号量
        int ret = sem_wait(sem);
        return ret == 0 ? 0 : -2;
    } else {
        // timeout > 0 时，设置超时时间
        struct timespec ts;

        // 获取当前时间
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            return -1;
        }

        // 设置绝对超时时间
        ts.tv_sec += timeout / 1000;              // 秒部分
        ts.tv_nsec += (timeout % 1000) * 1000000; // 毫秒部分转为纳秒
        if (ts.tv_nsec >= 1000000000) {           // 如果纳秒超过1秒，进位到秒
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        // 等待信号量，超时返回
        int ret = sem_timedwait(sem, &ts);
        if (ret == 0) {
            return 0;  // 成功获取信号量
        } else if (errno == ETIMEDOUT) {
            return -3;  // 超时返回
        } else {
            return -1;  // 其他错误
        }
    }
}

int post(semaphore_t* sem)
{
    int ret;
    ret = sem_post(sem);
    return ret==0?0:-1;
    
}

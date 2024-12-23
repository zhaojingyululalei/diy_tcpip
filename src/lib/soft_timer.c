#include "soft_timer.h"
#include "debug.h"
//安时间有序插入
static list_t soft_timer_list;

void soft_timer_dbg_print(soft_timer_t* timer,int id)
{
#ifdef DBG_SOFT_TIMER_PRINT
    dbg_info("----\r\n");
    dbg_info("id:%d\r\n",id);
    dbg_info("name:%s\r\n",timer->dbg_name);
    if(timer->type == SOFT_TIMER_TYPE_NONE)
    {
        dbg_info("type:none\r\n");
    }
    else if(timer->type == SOFT_TIMER_TYPE_PERIOD)
    {
        dbg_info("type:period\r\n");
    }
    else
    {
        dbg_info("type:once\r\n");
    }
    dbg_info("cur:%d\r\n",timer->cur);
    dbg_info("reload:%d\r\n",timer->reload);
    dbg_info("----\r\n");
    dbg_info("\r\n");
#endif
}

void soft_timer_list_print(void)
{
#ifdef DBG_SOFT_TIMER_PRINT
    list_node_t* cur = soft_timer_list.first;
    int count = 0;
    dbg_info("********************************************************\r\n");
    dbg_info("timer list :\r\n");
    while (cur)
    {
        soft_timer_t* timer = list_node_parent(cur,soft_timer_t,node);
        soft_timer_dbg_print(timer,count);
        cur = cur->next;
        count++;
    }
    dbg_info("********************************************************\r\n");
    dbg_info("\r\n");
#endif
}
void soft_timer_init(void)
{
    list_init(&soft_timer_list);
}


void insert_soft_timer(soft_timer_t* timer)
{
    list_node_t* cur = soft_timer_list.first;
    int sum = 0;
    while(cur)
    {
        soft_timer_t* cur_timer = list_node_parent(cur,soft_timer_t,node);
        sum+=cur_timer->cur;
        if(timer->reload > sum)
        {
            cur = cur->next;

        }
        else if(timer->reload == sum)
        {
            timer->cur = 0;
            list_insert_behind(&soft_timer_list,cur,&timer->node);
            break;
        }
        else
        {
            //遇见了第一个<的
            sum -= cur_timer->cur;
            list_insert_front(&soft_timer_list,&cur_timer->node,&timer->node);
            timer->cur -= sum;
            cur_timer->cur -= timer->cur;
            break;
            
        }
        
    }

    if(!cur)
    {
        list_insert_last(&soft_timer_list,&timer->node);
        timer->cur -= sum;
    }
}
int soft_timer_add(soft_timer_t *timer, soft_timer_type_t type, int ms,const char *name, soft_timer_handler handle_func, void *arg,void** ret)
{
    timer->type = type;
    timer->reload = timer->cur = ms;
    memset(timer->dbg_name,0,SOFT_TIMER_NAME_SIZE);
    strncpy(timer->dbg_name,name,strlen(name));
    timer->handler = handle_func;
    timer->arg = arg;
    timer->ret = ret;

    insert_soft_timer(timer);
    return 0;
}

int soft_timer_remove(soft_timer_t* timer)
{
    list_node_t* node = &timer->node;
    
    if(node->next)
    {
        soft_timer_t* next_timer = list_node_parent(node->next,soft_timer_t,node);
        next_timer->cur += timer->cur;
    }
    list_remove(&soft_timer_list,node);
    return 0;
}

int soft_timer_scan_list(int diff_ms)
{
    list_t done_list;
    list_init(&done_list);

    list_node_t* cur = soft_timer_list.first;
    while(cur)
    {
        list_node_t* next = cur->next;
        soft_timer_t* cur_timer = list_node_parent(cur,soft_timer_t,node);
        if(cur_timer->cur >diff_ms)
        {
            cur_timer->cur -= diff_ms;
            break;
        }
        diff_ms -= cur_timer->cur;
        cur_timer->cur = 0;
        list_remove(&soft_timer_list,&cur_timer->node);
        list_insert_last(&done_list,&cur_timer->node);

        cur = next;
    }
    cur = done_list.first;
    while(cur)
    {
        list_node_t* next = cur->next;
        list_remove(&done_list,cur);
        soft_timer_t* timer = list_node_parent(cur,soft_timer_t,node);

         
        void* x = timer->handler(timer->arg);
        if(timer->ret)
        {
            *timer->ret = x;
        }
        if(timer->type == SOFT_TIMER_TYPE_PERIOD)
        {
            timer->cur = timer->reload;
            insert_soft_timer(timer);
            //soft_timer_list_print();
        }
        cur = next;
    }
    //soft_timer_list_print();
    return 0;
}


int soft_timer_get_first_time(void)
{
    list_node_t* fnode = soft_timer_list.first;
    if(fnode)
    {
        soft_timer_t* timer = list_node_parent(fnode,soft_timer_t,node);
        return timer->cur;
    }
    else{
        return 0;
    }
}
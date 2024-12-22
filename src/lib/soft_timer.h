#ifndef __SOFT_TIMER_H
#define __SOFT_TIMER_H
#include "list.h" 
#include "types.h"
#include "net_cfg.h"
#define SOFT_TIMER_NAME_SIZE 32
typedef enum _soft_timer_type_t
{
    SOFT_TIMER_TYPE_NONE,
    SOFT_TIMER_TYPE_PERIOD,
    SOFT_TIMER_TYPE_ONCE
}soft_timer_type_t;

struct _soft_timer_t;
typedef void* (*soft_timer_handler)(void* arg);
#define DEFINE_TIMER_FUNC(name)            void* name(void* arg)
typedef struct _soft_timer_t
{
    char dbg_name[SOFT_TIMER_NAME_SIZE];
    soft_timer_type_t type;
    int cur; //剩余时间
    int reload;
    void** ret;
    void* arg;
    soft_timer_handler handler;

    list_node_t node;
}soft_timer_t;

void soft_timer_dbg_print(soft_timer_t* timer,int id);
void soft_timer_list_print(void);
void soft_timer_init(void);

static inline soft_timer_t* soft_timer_alloc(void)
{
    return (soft_timer_t*)malloc(sizeof(soft_timer_t));
}
static inline void soft_timer_free(soft_timer_t* timer)
{
    free(timer);
}
int soft_timer_add(soft_timer_t* timer,soft_timer_type_t type,int ms,const char* name,soft_timer_handler handle_func,void* arg,void** ret);
int soft_timer_remove(soft_timer_t* timer);
int soft_timer_scan_list(int diff_ms);
int soft_timer_get_first_time(void);
#endif


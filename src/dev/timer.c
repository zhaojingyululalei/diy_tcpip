#include "timer.h"

uint32_t get_cur_time_ms(void)
{
     struct timespec ts;
    // CLOCK_REALTIME: 获取当前时间（墙上时钟时间）
    // CLOCK_MONOTONIC: 获取从某个固定时间点开始的时间（不会被系统时间调整影响）
    clock_gettime(CLOCK_REALTIME, &ts);

    // 转换为毫秒
    uint32_t time_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return time_ms;
}
//
// Created by tyy on 2017/1/22.
//

#ifndef MAFIA_UTILS_H
#define MAFIA_UTILS_H

#include <unistd.h>
#include <string.h>
#include <sys/time.h>


static inline long time_current_usec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long time = (tv.tv_sec * 1e6) + tv.tv_usec;
    return time;
}


static inline char *remove_quotes(char *buf) {
    int len = strlen(buf);
    char *p = buf;

    if (p[0] == '\"' && p[len - 1] == '\"') {
        p[len - 1] = '\0';
        p++;
    }
    return p;
}



static inline void spinlock(volatile int *lock_ptr)
{
    while (!__sync_bool_compare_and_swap(lock_ptr, 0, 1))
        ; // Do nothing.
}


static inline void spinunlock(volatile int *lock_ptr)
{
    //asm volatile ("" ::: "memory");
    __sync_synchronize(); // Memory barrier
    *lock_ptr = 0;
}

#endif //MAFIA_UTILS_H

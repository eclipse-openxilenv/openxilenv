#ifndef GET_RDTSC_H
#define GET_RDTSC_H

#include <stdint.h>

static inline uint64_t Get_RDTSC(void)
{
#if defined(__x86_64__)
    unsigned int hi, lo;
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t tsc;
    asm volatile("mrs %0, cntvct_el0" : "=r" (tsc));
    return tsc;
#else
#error "not supported cpu architecture"
#endif    
}
#endif
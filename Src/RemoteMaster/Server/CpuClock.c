/*
 * Copyright 2023 ZF Friedrichshafen AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdint.h>
#include <stdio.h>
#include <time.h>

uint64_t Get_RDTSC(void)
{
    unsigned int hi, lo;
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}


uint64_t GetSystemTime(void)
{
    uint64_t ret;
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);
    ret = time.tv_sec * 1000000000ULL + time.tv_nsec;

    return ret;
}

static double TicksPerNanoSec;
static uint64_t TicksPerMicroSec;
static uint64_t TicksPerSec;
void CalibrateCpuTicks(void)
{
    uint64_t SystemBegin, SystemEnd;
    uint64_t ClockBegin, ClockEnd;

    SystemBegin = GetSystemTime();
    ClockBegin = Get_RDTSC();

    // Wait 100ms, with full cpu load
    while ((GetSystemTime() - SystemBegin) < 100000000ULL);

    SystemEnd = GetSystemTime();
    ClockEnd = Get_RDTSC();

    TicksPerNanoSec = (double)(ClockEnd - ClockBegin) / (double)(SystemEnd - SystemBegin);
    TicksPerMicroSec = (uint64_t)(TicksPerNanoSec * 1000.0);
    TicksPerSec = (uint64_t)(TicksPerNanoSec * 1000000000.0);

    printf("\nCPU Clock = %f GHz (%u)\n", TicksPerNanoSec, TicksPerSec);
}

uint64_t GetCpuFrquency(void)
{
    return TicksPerSec;
}

uint64_t GetCpuTicksPerMicroSec(void)
{
    return TicksPerMicroSec;
}

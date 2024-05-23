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


#pragma once

#include <stdint.h>

#include "tcb.h"

#define MAX_SCHEDULERS  1
#define MAX_REALTIME_PROCESSES 32

int AddRealtimeScheduler(char *par_Name, double par_CyclePeriod, int par_SyncWithFlexray, uint64_t *ret_CyclePeriodInNanoSecond);

int StopRealtimeScheduler(char *par_Name);
int StopAllRealtimeScheduler(void);

int StartRealtimeProcess(int par_Pid, char *par_Name, char *par_Scheduler, int par_Prio, int par_Cycles, int par_Delay);

int StopRealtimeProcess(int par_Pid);

int GetNextRealtimeProcess(int par_Index, int par_Flags, char *ret_Buffer, int par_maxc);

int IsRealtimeProcess(char *par_Name);
int IsRealtimeProcessPid(int par_Pid);

int AddProcessToPidNameArray(int par_Pid, char *par_Name, int MessageQueuSize);

int RemoveProcessFromPidNameArray(int par_Pid);

int GET_PID();
int get_pid_by_name(char *Name);

MESSAGE_BUFFER *GetMessageQueueForProcess(int par_Pid);

uint64_t get_timestamp_counter(void);

uint64_t get_rt_cycle_counter(void);
uint64_t GetCycleCounter64(void);

uint64_t get_sched_periode_timer_clocks(void);
uint64_t get_sched_periode_timer_clocks64(void);

void InitSchedulers(int par_PidForSchedulersHelperProcess);

int get_process_info_ex(int pid, char *ret_Name, int maxc_Name, int *ret_Type, int *ret_Prio, int *ret_Cycles, int *ret_Delay, int *ret_MessageSize,
                        char *ret_AssignedScheduler, int maxc_AssignedScheduler, uint64_t *ret_bb_access_mask,
                        int *ret_State, uint64_t *ret_CyclesCounter, uint64_t *ret_CyclesTime, uint64_t *ret_CyclesTimeMax, uint64_t *ret_CyclesTimeMin);

int ResetProcessMinMaxRuntimeMeasurement(int par_Pid);

int MyTimeout(int us);

#define MAX_PIDS 64

#define GENERATE_UNIQUE_PID_COMMAND       0
#define INVALIDATE_UNIQUE_PID_COMMAND     1
#define FREE_UNIQUE_PID_COMMAND           2
#define GET_UNIQUE_PID_BY_NAME_COMMAND    3
#define GENERATE_UNIQUE_RT_PID_COMMAND    4

#define RT_PID_BIT_MASK          0x10000000

int GetOrFreeUniquePid(int par_Command, int par_Pid, char *par_Name);

TASK_CONTROL_BLOCK * GetPointerToTaskControlBlock(int pid);

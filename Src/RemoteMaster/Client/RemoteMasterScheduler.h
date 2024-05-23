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

#include "StructsRM_Scheduler.h"

int rm_AddRealtimeScheduler(char *par_Name, double par_CyclePeriod, int par_SyncWithFlexray);

int rm_StopRealtimeScheduler(char *par_Name);

int rm_StartRealtimeProcess(int par_Pid, const char *par_Name, const char *par_Scheduler, int par_Prio, int par_Cycles, int par_Delay);

int rm_StopRealtimeProcess(int par_Pid);

int rm_GetNextRealtimeProcess(int par_Index, int par_Flags, char *ret_Buffer, int par_maxc);

int rm_IsRealtimeProcess(const char *par_Name);

int rm_AddProcessToPidNameArray(int par_Pid, char *par_Name, int par_MessageQueueSize);

int rm_RemoveProcessFromPidNameArray(int par_Pid);

int rm_get_pid_by_name(char *par_Name);

unsigned long rm_get_timestamp_counter(void);

int rm_get_process_info_ex (PID pid, char *ret_Name, int maxc_Name, int *ret_Type, int *ret_Prio, int *ret_Cycles, int *ret_Delay, int *ret_MessageSize,
                            char *ret_AssignedScheduler, int maxc_AssignedScheduler, uint64_t *ret_bb_access_mask,
                            int *ret_State, uint64_t *ret_CyclesCounter, uint64_t *ret_CyclesTime, uint64_t *ret_CyclesTimeMax, uint64_t *ret_CyclesTimeMin);


int rm_GetOrFreeUniquePid (int par_Command, int par_Pid, const char *par_Name);

int rm_AddBeforeProcessEquation (int par_Nr, int par_Pid, void *ExecStack);
int rm_AddBehindProcessEquation (int par_Nr, int par_Pid, void *ExecStack);
int rm_DelBeforeProcessEquations (int par_Nr, int par_Pid);
int rm_DelBehindProcessEquations (int par_Nr, int par_Pid);

int rm_IsRealtimeProcessPid (int par_Pid);

int rm_ResetProcessMinMaxRuntimeMeasurement (int par_Pid);

int rm_SchedulerCycleEvent (RM_SCHEDULER_CYCLIC_EVENT_REQ *Req);
                           //RM_SCHEDULER_CYCLIC_EVENT_ACK *Ack);

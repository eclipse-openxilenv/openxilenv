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

#include "StructRM.h"

#define RM_SCHEDULER_OFFSET  160


#define RM_SCHEDULER_ADD_REALTIME_SCHEDULER_CMD  (RM_SCHEDULER_OFFSET+0)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Offset_Name;
    int32_t SyncWithFlexray;
    double CyclePeriod;
    // ... followed by name
}  RM_SCHEDULER_ADD_REALTIME_SCHEDULER_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t CyclePeriodInNanoSecond;
    int32_t Ret;
}  RM_SCHEDULER_ADD_REALTIME_SCHEDULER_ACK;

#define RM_SCHEDULER_STOP_REALTIME_SCHEDULER_CMD  (RM_SCHEDULER_OFFSET+1)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Offset_Name;
    // ... followed by name
}  RM_SCHEDULER_STOP_REALTIME_SCHEDULER_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_STOP_REALTIME_SCHEDULER_ACK;


#define RM_SCHEDULER_START_REALTIME_PROCESS_CMD  (RM_SCHEDULER_OFFSET+2)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint32_t Offset_Name;
    uint32_t Offset_Scheduler;
    int32_t Prio;
    int32_t Cycles;
    int32_t Delay;
    // ... followed by name of th process and the scheduler
}  RM_SCHEDULER_START_REALTIME_PROCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_START_REALTIME_PROCESS_ACK;

int StopRealtimeProcess(int par_Pid);
#define RM_SCHEDULER_STOP_REALTIME_PROCESS_CMD  (RM_SCHEDULER_OFFSET+3)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
}  RM_SCHEDULER_STOP_REALTIME_PROCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_STOP_REALTIME_PROCESS_ACK;


#define RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_CMD  (RM_SCHEDULER_OFFSET+4)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Index;
    int32_t Flags;
    int32_t maxc;
}  RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
    uint32_t Offset_Name;
    // ... followed by name
}  RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK;


#define RM_SCHEDULER_IS_REALTIME_PROCESS_CMD  (RM_SCHEDULER_OFFSET+5)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Offset_Name;
    // ... followed by name
}  RM_SCHEDULER_IS_REALTIME_PROCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_IS_REALTIME_PROCESS_ACK;

#define RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_CMD  (RM_SCHEDULER_OFFSET+6)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint32_t Offset_Name;
    int32_t MessageQueueSize;
    // ... followed by name
}  RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_ACK;

#define RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_CMD  (RM_SCHEDULER_OFFSET+7)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
}  RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_ACK;

#define RM_SCHEDULER_GET_PID_BY_NAME_CMD  (RM_SCHEDULER_OFFSET+8)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Offset_Name;
    // ... followed by name
}  RM_SCHEDULER_GET_PID_BY_NAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_GET_PID_BY_NAME_ACK;

#define RM_SCHEDULER_GET_TIMESTAMPCOUNTER_CMD  (RM_SCHEDULER_OFFSET+9)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
}  RM_SCHEDULER_GET_TIMESTAMPCOUNTER_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Ret;
}  RM_SCHEDULER_GET_TIMESTAMPCOUNTER_ACK;

#define RM_SCHEDULER_GET_PROCESS_INFO_EX_CMD  (RM_SCHEDULER_OFFSET+10)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t pid;
    int32_t maxc_Name;
    int32_t maxc_AssignedScheduler;
}  RM_SCHEDULER_GET_PROCESS_INFO_EX_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Offset_Name;
    int32_t ret_Type;
    int32_t ret_Prio;
    int32_t ret_Cycles;
    int32_t ret_Delay;
    int32_t ret_MessageSize;
    uint32_t Offset_AssignedScheduler;
    int32_t ret_State;
    uint64_t ret_bb_access_mask;
    uint64_t ret_CyclesCounter;
    uint64_t ret_CyclesTime;
    uint64_t ret_CyclesTimeMax;
    uint64_t ret_CyclesTimeMin;
    int32_t Ret;
    // ... followed by Name
}  RM_SCHEDULER_GET_PROCESS_INFO_EX_ACK;

#define RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_CMD  (RM_SCHEDULER_OFFSET+11)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Command;
    int32_t Pid;
    uint32_t Offset_Name;
    // ... followed by name
}  RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_ACK;

#define RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_CMD  (RM_SCHEDULER_OFFSET+12)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Nr;
    int32_t Pid;
    int32_t SizeOfExecStack;
    uint32_t Offset_ExecStack;
    // ... followed by ExecSack
}  RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_ACK;

#define RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_CMD  (RM_SCHEDULER_OFFSET+13)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Nr;
    int32_t Pid;
    int32_t SizeOfExecStack;
    uint32_t Offset_ExecStack;
    // ... followed by ExecSack
}  RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_ACK;

#define RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_CMD  (RM_SCHEDULER_OFFSET+14)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Nr;
    int32_t Pid;
}  RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_ACK;

#define RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_CMD  (RM_SCHEDULER_OFFSET+15)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Nr;
    int32_t Pid;
}  RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_ACK;

#define RM_SCHEDULER_IS_REALTIME_PROCESS_PID_CMD  (RM_SCHEDULER_OFFSET+16)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
}  RM_SCHEDULER_IS_REALTIME_PROCESS_PID_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_SCHEDULER_IS_REALTIME_PROCESS_PID_ACK;

#define RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_CMD  (RM_SCHEDULER_OFFSET+17)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
}  RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_ACK;


// From remote master to client
#define RM_SCHEDULER_CYCLIC_EVENT_CMD  (RM_SCHEDULER_OFFSET+200)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t SchedulerNr;
    uint32_t Fill1;
    uint64_t CycleCounter;
    // The simulated time since XilEnv is started
    uint64_t SimulatedTimeSinceStartedInNanoSecond;
    uint32_t OffsetXX;
    // ... followed by the data
}  RM_SCHEDULER_CYCLIC_EVENT_REQ;

/* no ACK
typedef struct {
RM_PACKAGE_HEADER PackageHeader;
int32_t Ret;
}  RM_SCHEDULER_CYCLIC_EVENT_ACK;
*/

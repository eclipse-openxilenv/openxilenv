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


#ifndef RPCFUNCSCHED_H
#define RPCFUNCSCHED_H

#include <stdint.h>

#include "RpcFuncBase.h"

#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct on
#endif
#endif
#pragma pack(push,1)

#define SCHED_CMD_OFFSET  20

#define RPC_API_STOP_SCHEDULER_CMD         (SCHED_CMD_OFFSET+0)
typedef struct {
#define RPC_API_STOP_SCHEDULER_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_STOP_SCHEDULER_MESSAGE_MEMBERS
} RPC_API_STOP_SCHEDULER_MESSAGE;

typedef struct {
#define RPC_API_STOP_SCHEDULER_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_STOP_SCHEDULER_MESSAGE_ACK_MEMBERS
} RPC_API_STOP_SCHEDULER_MESSAGE_ACK;

#define RPC_API_CONTINUE_SCHEDULER_CMD     (SCHED_CMD_OFFSET+1)
typedef struct {
#define RPC_API_CONTINUE_SCHEDULER_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_CONTINUE_SCHEDULER_MESSAGE_MEMBERS
} RPC_API_CONTINUE_SCHEDULER_MESSAGE;

typedef struct {
#define RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK_MEMBERS
} RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK;

#define RPC_API_IS_SCHEDULER_RUNNING_CMD   (SCHED_CMD_OFFSET+2)
typedef struct {
#define RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_MEMBERS
} RPC_API_IS_SCHEDULER_RUNNING_MESSAGE;

typedef struct {
#define RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK_MEMBERS
} RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK;

#define RPC_API_START_PROCESS_CMD          (SCHED_CMD_OFFSET+3)
typedef struct {
#define RPC_API_START_PROCESS_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetProcessName;\
    char Data[1];  // > 1Byte!
    RPC_API_START_PROCESS_MESSAGE_MEMBERS
} RPC_API_START_PROCESS_MESSAGE;

typedef struct {
#define RPC_API_START_PROCESS_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_PROCESS_MESSAGE_ACK_MEMBERS
} RPC_API_START_PROCESS_MESSAGE_ACK;

#define RPC_API_START_PROCESS_AND_LOAD_SVL_CMD          (SCHED_CMD_OFFSET+4)
typedef struct {
#define RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetProcessName;\
    int32_t OffsetSvlName;\
    char Data[1];  // > 1Byte!
    RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_MEMBERS
} RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE;

typedef struct {
#define RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK_MEMBERS
} RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK;

#define RPC_API_START_PROCESS_EX_CMD          (SCHED_CMD_OFFSET+5)
typedef struct {
#define RPC_API_START_PROCESS_EX_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetProcessName;\
    int32_t Prio;\
    int32_t Cycle;\
    int32_t Delay;\
    int32_t Timeout;\
    int32_t OffsetSvlName;\
    int32_t OffsetBBPrefix;\
    int32_t UseRangeControl;  /* wenn 0, dann ignoriere alle folgenden Parameter */\
    int32_t RangeControlBeforeActiveFlags;\
    int32_t RangeControlBehindActiveFlags;\
    int32_t RangeControlStopSchedFlag;\
    int32_t RangeControlOutput;\
    uint32_t RangeErrorCounterFlag;\
    int32_t OffsetRangeErrorCounter;\
    uint32_t RangeControlVarFlag;\
    int32_t OffsetRangeControl;\
    int32_t RangeControlPhysFlag;\
    int32_t RangeControlLimitValues;\
    char Data[1];  // > 1Byte!
    RPC_API_START_PROCESS_EX_MESSAGE_MEMBERS
} RPC_API_START_PROCESS_EX_MESSAGE;

typedef struct {
#define RPC_API_START_PROCESS_EX_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_PROCESS_EX_MESSAGE_ACK_MEMBERS
} RPC_API_START_PROCESS_EX_MESSAGE_ACK;

#define RPC_API_STOP_PROCESS_CMD          (SCHED_CMD_OFFSET+6)
typedef struct {
#define RPC_API_STOP_PROCESS_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetProcessName;\
    char Data[1];  // > 1Byte!
    RPC_API_STOP_PROCESS_MESSAGE_MEMBERS
} RPC_API_STOP_PROCESS_MESSAGE;

typedef struct {
#define RPC_API_STOP_PROCESS_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_STOP_PROCESS_MESSAGE_ACK_MEMBERS
} RPC_API_STOP_PROCESS_MESSAGE_ACK;

#define RPC_API_GET_NEXT_PROCESS_CMD         (SCHED_CMD_OFFSET+7)
typedef struct {
#define RPC_API_GET_NEXT_PROCESS_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Flag;\
    int32_t OffsetFilter;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_NEXT_PROCESS_MESSAGE_MEMBERS
} RPC_API_GET_NEXT_PROCESS_MESSAGE;

typedef struct {
#define RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    int32_t OffsetReturnValue;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK_MEMBERS
} RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK;

#define RPC_API_GET_PROCESS_STATE_CMD        (SCHED_CMD_OFFSET+8)
typedef struct {
#define RPC_API_GET_PROCESS_STATE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetProcessName;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_PROCESS_STATE_MESSAGE_MEMBERS
} RPC_API_GET_PROCESS_STATE_MESSAGE;

typedef struct {
#define RPC_API_GET_PROCESS_STATE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_GET_PROCESS_STATE_MESSAGE_ACK_MEMBERS
} RPC_API_GET_PROCESS_STATE_MESSAGE_ACK;

#define RPC_API_DO_NEXT_CYCLES_CMD           (SCHED_CMD_OFFSET+9)
typedef struct {
#define RPC_API_DO_NEXT_CYCLES_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    uint32_t Cycles;
    RPC_API_DO_NEXT_CYCLES_MESSAGE_MEMBERS
} RPC_API_DO_NEXT_CYCLES_MESSAGE;

typedef struct {
#define RPC_API_DO_NEXT_CYCLES_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_DO_NEXT_CYCLES_MESSAGE_ACK_MEMBERS
} RPC_API_DO_NEXT_CYCLES_MESSAGE_ACK;

#define RPC_API_DO_NEXT_CYCLES_AND_WAIT_CMD           (SCHED_CMD_OFFSET+10)
typedef struct {
#define RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Cycles;
    RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_MEMBERS
} RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE;

typedef struct {
#define RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK_MEMBERS
} RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK;

#define RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_CMD        (SCHED_CMD_OFFSET+11)
typedef struct {
#define RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Nr;\
    int32_t OffsetProcessName;\
    int32_t OffsetEquationFile;\
    char Data[1];  // > 1Byte!
    RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_MEMBERS
} RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE;

typedef struct {
#define RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK_MEMBERS
} RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK;

#define RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_CMD        (SCHED_CMD_OFFSET+12)
typedef struct {
#define RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Nr;\
    int32_t OffsetProcessName;\
    int32_t OffsetEquationFile;\
    char Data[1];  // > 1Byte!
    RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_MEMBERS
} RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE;

typedef struct {
#define RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK_MEMBERS
} RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK;

#define RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_CMD        (SCHED_CMD_OFFSET+13)
typedef struct {
#define RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Nr;\
    int32_t OffsetProcessName;\
    char Data[1];  // > 1Byte!
    RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_MEMBERS
} RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE;

typedef struct {
#define RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK_MEMBERS
} RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK;

#define RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_CMD        (SCHED_CMD_OFFSET+14)
typedef struct {
#define RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Nr;\
    int32_t OffsetProcessName;\
    char Data[1];  // > 1Byte!
    RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_MEMBERS
} RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE;

typedef struct {
#define RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK_MEMBERS
} RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK;

#define RPC_API_WAIT_UNTIL_CMD        (SCHED_CMD_OFFSET+15)
typedef struct {
#define RPC_API_WAIT_UNTIL_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetEquation;\
    int32_t Cycles;\
    char Data[1];  // > 1Byte!
    RPC_API_WAIT_UNTIL_MESSAGE_MEMBERS
} RPC_API_WAIT_UNTIL_MESSAGE;

typedef struct {
#define RPC_API_WAIT_UNTIL_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_WAIT_UNTIL_MESSAGE_ACK_MEMBERS
} RPC_API_WAIT_UNTIL_MESSAGE_ACK;

#define RPC_API_START_PROCESS_EX2_CMD          (SCHED_CMD_OFFSET+16)
typedef struct {
#define RPC_API_START_PROCESS_EX2_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetProcessName;\
    int32_t Prio;\
    int32_t Cycle;\
    int32_t Delay;\
    int32_t Timeout;\
    int32_t OffsetSvlName;\
    int32_t OffsetA2LName;\
    uint32_t A2LFlags;\
    int32_t OffsetBBPrefix;\
    int32_t UseRangeControl;  /* wenn 0, dann ignoriere alle folgenden Parameter */\
    int32_t RangeControlBeforeActiveFlags;\
    int32_t RangeControlBehindActiveFlags;\
    int32_t RangeControlStopSchedFlag;\
    int32_t RangeControlOutput;\
    uint32_t RangeErrorCounterFlag;\
    int32_t OffsetRangeErrorCounter;\
    uint32_t RangeControlVarFlag;\
    int32_t OffsetRangeControl;\
    int32_t RangeControlPhysFlag;\
    int32_t RangeControlLimitValues;\
    char Data[1];  // > 1Byte!
    RPC_API_START_PROCESS_EX2_MESSAGE_MEMBERS
} RPC_API_START_PROCESS_EX2_MESSAGE;

typedef struct {
#define RPC_API_START_PROCESS_EX2_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_PROCESS_EX2_MESSAGE_ACK_MEMBERS
} RPC_API_START_PROCESS_EX2_MESSAGE_ACK;

#pragma pack(pop)
#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct reset
#endif
#endif

int AddSchedFunctionToTable(void);

#endif // RPCFUNCSCHED_H

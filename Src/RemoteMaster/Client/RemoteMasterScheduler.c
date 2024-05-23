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


#include<stdio.h>
#ifdef _WIN32
#include<WinSock2.h>
#else
#endif
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Scheduler.h"

#include "Message.h"
#include "ExecutionStack.h"
#include "StructsRM_Scheduler.h"
#include "RemoteMasterNet.h"
#include "RemoteMasterFiFo.h"
#include "RemoteMasterScheduler.h"

#define CHECK_ANSWER(Req,Ack)

int rm_AddRealtimeScheduler(char *par_Name, double par_CyclePeriod, int par_SyncWithFlexray)
{
    RM_SCHEDULER_ADD_REALTIME_SCHEDULER_REQ *Req;
    RM_SCHEDULER_ADD_REALTIME_SCHEDULER_ACK Ack;
    int SizeOfStruct;
    int NameLen = (int)strlen (par_Name) + 1;

    SizeOfStruct = (int)sizeof(RM_SCHEDULER_ADD_REALTIME_SCHEDULER_REQ) + NameLen;
    Req = (RM_SCHEDULER_ADD_REALTIME_SCHEDULER_REQ*)_alloca((size_t)SizeOfStruct);
    Req->CyclePeriod = par_CyclePeriod;
    Req->SyncWithFlexray = par_SyncWithFlexray;
    Req->Offset_Name = sizeof (RM_SCHEDULER_ADD_REALTIME_SCHEDULER_REQ);
    MEMCPY (Req + 1, par_Name, (size_t)NameLen);
    TransactRemoteMaster (RM_SCHEDULER_ADD_REALTIME_SCHEDULER_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    if (Ack.Ret  == 0) {
        SCHEDULER_DATA *pScheduler;
        pScheduler = RegisterScheduler  (par_Name);
        if (pScheduler == NULL) return -1;
        pScheduler->State = SCHED_EXTERNAL_ONLY_INFO;
        pScheduler->SchedPeriodInNanoSecond = (int64_t)Ack.CyclePeriodInNanoSecond;
        pScheduler->SchedulerHaveRecogizedTerminationRequeFlag = 1;  // not a real scheduler dont wait for him!
    }
    return Ack.Ret;
}


int rm_StopRealtimeScheduler(char *par_Name)
{
    RM_SCHEDULER_STOP_REALTIME_SCHEDULER_REQ *Req;
    RM_SCHEDULER_STOP_REALTIME_SCHEDULER_ACK Ack;
    int SizeOfStruct;
    int NameLen = (int)strlen (par_Name) + 1;

    SizeOfStruct = (int)sizeof(RM_SCHEDULER_STOP_REALTIME_SCHEDULER_REQ) + NameLen;
    Req = (RM_SCHEDULER_STOP_REALTIME_SCHEDULER_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Offset_Name = sizeof (RM_SCHEDULER_STOP_REALTIME_SCHEDULER_REQ);
    MEMCPY (Req + 1, par_Name, (size_t)NameLen);
    TransactRemoteMaster (RM_SCHEDULER_STOP_REALTIME_SCHEDULER_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_StartRealtimeProcess(int par_Pid, const char *par_Name, const char *par_Scheduler, int par_Prio, int par_Cycles, int par_Delay)
{
    RM_SCHEDULER_START_REALTIME_PROCESS_REQ *Req;
    RM_SCHEDULER_START_REALTIME_PROCESS_ACK Ack;
    int SizeOfStruct;
    int NameLen;
    int SchedulerLen;

    NameLen = (int)strlen (par_Name) + 1;
    SchedulerLen = (int)strlen (par_Scheduler) + 1;
    SizeOfStruct = (int)sizeof(RM_SCHEDULER_START_REALTIME_PROCESS_REQ) + NameLen + SchedulerLen;
    Req = (RM_SCHEDULER_START_REALTIME_PROCESS_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Pid = par_Pid;
    Req->Prio = par_Prio;
    Req->Cycles = par_Cycles;
    Req->Delay = par_Delay;
    Req->Offset_Name = sizeof (RM_SCHEDULER_START_REALTIME_PROCESS_REQ);
    MEMCPY ((char*)(Req + 1), par_Name, (size_t)NameLen);
    Req->Offset_Scheduler = Req->Offset_Name + (uint32_t)NameLen;
    MEMCPY ((char*)(Req + 1) + NameLen, par_Scheduler, (size_t)SchedulerLen);
    TransactRemoteMaster (RM_SCHEDULER_START_REALTIME_PROCESS_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}


int rm_StopRealtimeProcess(int par_Pid)
{
    RM_SCHEDULER_STOP_REALTIME_PROCESS_REQ Req;
    RM_SCHEDULER_STOP_REALTIME_PROCESS_ACK Ack;

    Req.Pid = par_Pid;
    TransactRemoteMaster (RM_SCHEDULER_STOP_REALTIME_PROCESS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_GetNextRealtimeProcess(int par_Index, int par_Flags, char *ret_Buffer, int par_maxc)
{
    RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_REQ Req;
    RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK *Ack;
    int SizeOfStruct;

    SizeOfStruct = (int)sizeof(RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK) + par_maxc;
    Ack = (RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK*)_alloca((size_t)SizeOfStruct);
    Req.Index = par_Index;
    Req.Flags = par_Flags;
    Req.maxc = par_maxc;
    TransactRemoteMaster (RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_CMD, &Req, sizeof(Req), Ack, SizeOfStruct);
    CHECK_ANSWER(Req, Ack);
    MEMCPY (ret_Buffer, Ack + 1, Ack->PackageHeader.SizeOf - sizeof (RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK));
    return Ack->Ret;
}

int rm_IsRealtimeProcess(const char *par_Name)
{
    RM_SCHEDULER_IS_REALTIME_PROCESS_REQ *Req;
    RM_SCHEDULER_IS_REALTIME_PROCESS_ACK Ack;
    int SizeOfStruct;
    int NameLen = (int)strlen (par_Name) + 1;

    SizeOfStruct = (int)sizeof(RM_SCHEDULER_IS_REALTIME_PROCESS_REQ) + NameLen;
    Req = (RM_SCHEDULER_IS_REALTIME_PROCESS_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Offset_Name = sizeof (RM_SCHEDULER_IS_REALTIME_PROCESS_REQ);
    MEMCPY (Req + 1, par_Name, (size_t)NameLen);
    TransactRemoteMaster (RM_SCHEDULER_IS_REALTIME_PROCESS_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_AddProcessToPidNameArray(int par_Pid, char *par_Name, int par_MessageQueueSize)
{
    RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_REQ *Req;
    RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_ACK Ack;
    int SizeOfStruct;
    int NameLen = (int)strlen (par_Name) + 1;

    SizeOfStruct = (int)sizeof(RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_REQ) + NameLen;
    Req = (RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Pid = par_Pid;
    Req->MessageQueueSize = par_MessageQueueSize;
    Req->Offset_Name = sizeof (RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_REQ);
    MEMCPY (Req + 1, par_Name, (size_t)NameLen);
    TransactRemoteMaster (RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_RemoveProcessFromPidNameArray(int par_Pid)
{
    RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_REQ Req;
    RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_ACK Ack;

    Req.Pid = par_Pid;
    TransactRemoteMaster (RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_pid_by_name(char *par_Name)
{
    RM_SCHEDULER_GET_PID_BY_NAME_REQ *Req;
    RM_SCHEDULER_GET_PID_BY_NAME_ACK Ack;
    int SizeOfStruct;
    int NameLen = (int)strlen (par_Name) + 1;

    SizeOfStruct = (int)sizeof(RM_SCHEDULER_GET_PID_BY_NAME_REQ) + NameLen;
    Req = (RM_SCHEDULER_GET_PID_BY_NAME_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Offset_Name = sizeof (RM_SCHEDULER_GET_PID_BY_NAME_REQ);
    MEMCPY (Req + 1, par_Name, (size_t)NameLen);
    TransactRemoteMaster (RM_SCHEDULER_GET_PID_BY_NAME_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

static void strcpy_maxc (char *dst, char *src, int max_c)
{
    if (max_c > 0) {
        size_t len = strlen (src) + 1;
        if (len <= (size_t)max_c) {
            MEMCPY (dst, src, len);
        } else {
            MEMCPY (dst, src, (size_t)max_c - 1);
            dst[max_c - 1] = 0;
        }
    }
}

int rm_get_process_info_ex (PID pid, char *ret_Name, int maxc_Name, int *ret_Type, int *ret_Prio, int *ret_Cycles, int *ret_Delay, int *ret_MessageSize,
                            char *ret_AssignedScheduler, int maxc_AssignedScheduler, uint64_t *ret_bb_access_mask,
                            int *ret_State, uint64_t *ret_CyclesCounter, uint64_t *ret_CyclesTime, uint64_t *ret_CyclesTimeMax, uint64_t *ret_CyclesTimeMin)
{
    RM_SCHEDULER_GET_PROCESS_INFO_EX_REQ Req;
    RM_SCHEDULER_GET_PROCESS_INFO_EX_ACK *Ack;
    size_t SizeOfStruct;

    SizeOfStruct = sizeof(RM_SCHEDULER_GET_PROCESS_INFO_EX_ACK) + (size_t)(maxc_Name + maxc_AssignedScheduler);
    Ack = (RM_SCHEDULER_GET_PROCESS_INFO_EX_ACK*)_alloca(SizeOfStruct);
    Req.pid = pid;
    Req.maxc_Name = maxc_Name;
    Req.maxc_AssignedScheduler = maxc_AssignedScheduler;
    TransactRemoteMaster (RM_SCHEDULER_GET_PROCESS_INFO_EX_CMD, &Req, sizeof(Req), Ack, (int)SizeOfStruct);
    CHECK_ANSWER(Req, Ack);
    if (Ack->Ret == 0) {
        if ((ret_Name != NULL) && (maxc_Name > 0) && (Ack->Offset_Name > 0))
            strcpy_maxc (ret_Name, (char*)Ack + Ack->Offset_Name, maxc_Name);
        if (ret_Type != NULL) *ret_Type = Ack->ret_Type;
        if (ret_Prio != NULL) *ret_Prio = Ack->ret_Prio;
        if (ret_Cycles != NULL) *ret_Cycles = Ack->ret_Cycles;
        if (ret_Delay != NULL) *ret_Delay = Ack->ret_Delay;
        if (ret_MessageSize != NULL) *ret_MessageSize = Ack->ret_MessageSize;
        if ((ret_AssignedScheduler != NULL) && (maxc_AssignedScheduler > 0) && (Ack->Offset_AssignedScheduler > 0))
            strcpy_maxc (ret_AssignedScheduler, (char*)Ack + Ack->Offset_AssignedScheduler,maxc_AssignedScheduler);
        if (ret_bb_access_mask != NULL) *ret_bb_access_mask = Ack->ret_bb_access_mask;
        if (ret_State != NULL) *ret_State = Ack->ret_State;
        if (ret_CyclesCounter != NULL) *ret_CyclesCounter = Ack->ret_CyclesCounter;
        if (ret_CyclesTime != NULL) *ret_CyclesTime = Ack->ret_CyclesTime;
        if (ret_CyclesTimeMax != NULL) *ret_CyclesTimeMax = Ack->ret_CyclesTimeMax;
        if (ret_CyclesTimeMin != NULL) *ret_CyclesTimeMin = Ack->ret_CyclesTimeMin;
    }
    return Ack->Ret;
}


int rm_GetOrFreeUniquePid (int par_Command, int par_Pid, const char *par_Name)
{
    RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_REQ *Req;
    RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_ACK Ack;
    size_t SizeOfStruct;
    size_t NameLen = strlen (par_Name) + 1;

    SizeOfStruct = sizeof(RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_REQ) + NameLen;
    Req = (RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_REQ*)_alloca(SizeOfStruct);
    Req->Command = par_Command;
    Req->Pid = par_Pid;
    Req->Offset_Name = sizeof (RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_REQ);
    MEMCPY (Req + 1, par_Name, NameLen);
    TransactRemoteMaster (RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_CMD, Req, (int)SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;

}

int rm_AddBeforeProcessEquation (int par_Nr, int par_Pid, void *ExecStack)
{
    RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_REQ *Req;
    RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_ACK Ack;
    size_t SizeOfStruct;
    size_t SizeofStack = (size_t)sizeof_exec_stack ((struct EXEC_STACK_ELEM*)ExecStack);

    SizeOfStruct = sizeof(RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_REQ) + SizeofStack;
    Req = (RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_REQ*)_alloca(SizeOfStruct);
    Req->Nr = par_Nr;
    Req->Pid = par_Pid;
    Req->Offset_ExecStack = sizeof (RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_REQ);
    MEMCPY (Req + 1, ExecStack, SizeofStack);
    TransactRemoteMaster (RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_CMD, Req, (int)SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_AddBehindProcessEquation (int par_Nr, int par_Pid, void *ExecStack)
{
    RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_REQ *Req;
    RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_ACK Ack;
    size_t SizeOfStruct;
    size_t SizeofStack = (size_t)sizeof_exec_stack ((struct EXEC_STACK_ELEM*)ExecStack);

    SizeOfStruct = sizeof(RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_REQ) + SizeofStack;
    Req = (RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_REQ*)_alloca(SizeOfStruct);
    Req->Nr = par_Nr;
    Req->Pid = par_Pid;
    Req->Offset_ExecStack = sizeof (RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_REQ);
    MEMCPY (Req + 1, ExecStack, SizeofStack);
    TransactRemoteMaster (RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_CMD, Req, (int)SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_DelBeforeProcessEquations (int par_Nr, int par_Pid)
{
    RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_REQ Req;
    RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_ACK Ack;

    Req.Nr = par_Nr;
    Req.Pid = par_Pid;
    TransactRemoteMaster (RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_DelBehindProcessEquations (int par_Nr, int par_Pid)
{
    RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_REQ Req;
    RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_ACK Ack;

    Req.Nr = par_Nr;
    Req.Pid = par_Pid;
    TransactRemoteMaster (RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_IsRealtimeProcessPid (int par_Pid)
{
    RM_SCHEDULER_IS_REALTIME_PROCESS_PID_REQ Req;
    RM_SCHEDULER_IS_REALTIME_PROCESS_PID_ACK Ack;

    Req.Pid = par_Pid;
    TransactRemoteMaster (RM_SCHEDULER_IS_REALTIME_PROCESS_PID_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_ResetProcessMinMaxRuntimeMeasurement (int par_Pid)
{
    RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_REQ Req;
    RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_ACK Ack;

    Req.Pid = par_Pid;
    TransactRemoteMaster (RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_SchedulerCycleEvent (RM_SCHEDULER_CYCLIC_EVENT_REQ *Req)
{
    if (Req->PackageHeader.Command == RM_SCHEDULER_CYCLIC_EVENT_CMD) {
        SCHEDULER_DATA *Scheduler = GetSchedulerInfos ((int)Req->SchedulerNr);
        if (Scheduler != NULL) {
            Scheduler->Cycle = Req->CycleCounter;
            Scheduler->SimulatedTimeSinceStartedInNanoSecond = Req->SimulatedTimeSinceStartedInNanoSecond;
            Scheduler->SimulatedTimeInNanoSecond = Scheduler->SimulatedTimeSinceStartedInNanoSecond;

            rm_SyncFiFos();
        }
        return 1;
    } else {
        return 0;
    }
}

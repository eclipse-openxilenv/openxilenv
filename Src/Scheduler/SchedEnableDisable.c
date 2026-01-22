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


#include "ThrowError.h"
#include "MainWinowSyncWithOtherThreads.h"
#include "MainValues.h"
#include "MemZeroAndCopy.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#include "Scheduler.h"
#include "SchedEnableDisable.h"

//#define DEBUG_ENABLE_DISABLE_SCHED_LOG

#define UNUSED(x) (void)(x)

#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
#define ENTER_CRITICAL_SECTION_LOG(a) \
    EnterCriticalSection (&par_Requests->StopStartRequestCriticalSection); \
    if (par_Requests->WaitForSchedulerContinueCriticalSectionLineNr) { \
        fprintf (DisableEnableLog, "Internal error: %i\n", par_Requests->WaitForSchedulerContinueCriticalSectionLineNr); \
        fflush (DisableEnableLog);\
    } else par_Requests->WaitForSchedulerContinueCriticalSectionLineNr = __LINE__;

#define LEAVE_CRITICAL_SECTION_LOG(a) \
    par_Requests->WaitForSchedulerContinueCriticalSectionLineNr = 0; \
    LeaveCriticalSection (&par_Requests->StopStartRequestCriticalSection);
#else
#define ENTER_CRITICAL_SECTION_LOG(a) \
    EnterCriticalSection (&par_Requests->StopStartRequestCriticalSection); \

#define LEAVE_CRITICAL_SECTION_LOG(a) \
    LeaveCriticalSection (&par_Requests->StopStartRequestCriticalSection);
#endif

#define UNUSED(x) (void)(x)

#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
#include "Scheduler.h"
static FILE *DisableEnableLog;
#endif

int InitStopRequest(SCHEDULER_STOP_REQ *par_Requests)
{
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    DisableEnableLog = fopen("c:\\temp\\sched_enable_disable_log.txt", "wt");
    fprintf (DisableEnableLog, "%06llu (%i) InitStopRequest()\n", GetSchedulerInfos(0)->Cycle, __LINE__);
    fflush (DisableEnableLog);
#endif
    MEMSET(par_Requests, 0, sizeof(*par_Requests));
    InitializeCriticalSection (&(par_Requests->StopStartRequestCriticalSection));
    InitializeConditionVariable(&(par_Requests->WaitForSchedulerContinueConditionVariable));
    par_Requests->WaitForSchedulerContinueCriticalSectionLineNr = 0;
    par_Requests->WaitForSchedulerContinueWakeFlag = 0;
    par_Requests->EnableDisableCounter = 0;
    par_Requests->UserEnableDisableCounter = 0;
    par_Requests->RpcEnableDisableCounter = 0;
    par_Requests->StopCallbackCount = 0;
    par_Requests->ContinueCallbackCount = 0;
    par_Requests->OpenStopRequestCount = 0;

    return 0;
}

int StopRequest(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser)
{
    UNUSED(par_FromThreadId);

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return 0;
    }
    ENTER_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "%06llu (%i) StopRequest()\n", GetSchedulerInfos(0)->Cycle, __LINE__);
    fflush (DisableEnableLog);
#endif
    switch (par_FromUser) {
    case SCHEDULER_CONTROLED_BY_SYSTEM:
        par_Requests->EnableDisableCounter++;
        break;
    case SCHEDULER_CONTROLED_BY_USER:
        if (par_Requests->UserEnableDisableCounter == 0) {  // user disable scheduler counter should be only 0 or 1
            par_Requests->UserEnableDisableCounter++;
        }
        break;
    case SCHEDULER_CONTROLED_BY_RPC:
        par_Requests->RpcEnableDisableCounter++;
        break;
    default:  // all other would be ignored
        break;
    }
    LEAVE_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
    return 0;
}

int ContinueRequest(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser)
{
    int Ret = 0;
    int SignalToUser = 0;
    UNUSED(par_FromThreadId);

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return Ret;
    }

    ENTER_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "%06llu (%i) ContinueRequest()\n", GetSchedulerInfos(0)->Cycle, __LINE__);
    fflush (DisableEnableLog);
#endif
    switch (par_FromUser) {
    case SCHEDULER_CONTROLED_BY_SYSTEM:
        if (par_Requests->EnableDisableCounter > 0) {
            par_Requests->EnableDisableCounter--;
        } else {
            Ret = -1; // negativ response -> system error
        }
        break;
    case SCHEDULER_CONTROLED_BY_USER:
        if (par_Requests->UserEnableDisableCounter > 0) {
            par_Requests->UserEnableDisableCounter--;
            if (par_Requests->UserEnableDisableCounter == 0) {
                SignalToUser = 1;
            }
        }
        break;
    case SCHEDULER_CONTROLED_BY_RPC:
        if (par_Requests->RpcEnableDisableCounter > 0) {
            par_Requests->RpcEnableDisableCounter--;
        }
        break;
    default:  // all other would be ignored
        break;
    }

    if (Ret == 0) {
        if ((par_Requests->RpcEnableDisableCounter == 0) && (par_Requests->UserEnableDisableCounter == 0) && (par_Requests->EnableDisableCounter == 0)) {
            Ret = 1;
        }
    }
    if (Ret == 1) {
        SchedulerWakeupNoLock(par_Requests, __LINE__);  // wakeup scheduler outside the critical cection
    }
    LEAVE_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
    if ((Ret == 1) && SignalToUser) {
        SchedulerStateChanged(1);  // change the control panel buttons
    }
    return Ret;
}

int ContinueRequestWithCallback(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser,
                                void (*par_Callback)(void* par_Parameter), void *par_Parameter)
{
    int Ret = 0;
    int SignalToUser = 0;
    int Err = 0;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return Ret;
    }

    ENTER_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "%06llu (%i) ContinueRequestWithCallback()\n", GetSchedulerInfos(0)->Cycle, __LINE__);
    fflush (DisableEnableLog);
#endif
    if (par_Requests->ContinueCallbackCount < (MAX_OPEN_REQUESTS - 1)) {
        par_Requests->ContinueCallbackElems[par_Requests->ContinueCallbackCount].FromThreadId = par_FromThreadId;
        par_Requests->ContinueCallbackElems[par_Requests->ContinueCallbackCount].Callback = par_Callback;
        par_Requests->ContinueCallbackElems[par_Requests->ContinueCallbackCount].Parameter = par_Parameter;
        par_Requests->ContinueCallbackElems[par_Requests->ContinueCallbackCount].FromUser = par_FromUser;
        par_Requests->ContinueCallbackCount++;
    } else {
        Err = -1;
    }

    switch (par_FromUser) {
    case SCHEDULER_CONTROLED_BY_SYSTEM:
        if (par_Requests->EnableDisableCounter > 0) {
            par_Requests->EnableDisableCounter--;
        } else {
            Ret = -1; // negativ response -> system error
        }
        break;
    case SCHEDULER_CONTROLED_BY_USER:
        if (par_Requests->UserEnableDisableCounter > 0) {
            par_Requests->UserEnableDisableCounter--;
            if (par_Requests->UserEnableDisableCounter == 0) {
                SignalToUser = 1;
            }
        }
        break;
    case SCHEDULER_CONTROLED_BY_RPC:
        if (par_Requests->RpcEnableDisableCounter > 0) {
            par_Requests->RpcEnableDisableCounter--;
        }
        break;
    default:  // all other would be ignored
        break;
    }

    if (Ret == 0) {
        if ((par_Requests->RpcEnableDisableCounter == 0) && (par_Requests->UserEnableDisableCounter == 0) && (par_Requests->EnableDisableCounter == 0)) {
            Ret = 1;
        }
    }
    if (Ret == 1) {
        SchedulerWakeupNoLock(par_Requests, __LINE__);
    }
    LEAVE_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
    if (Err) ThrowError (1, "cannot add continue scheduler request callback not more than %i allowed", MAX_OPEN_REQUESTS);
    if ((Ret == 1) && SignalToUser) {
        SchedulerStateChanged(1);  // change the button inside the control panel
    }
    return Ret;
}


int StopRequestWithCallback(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser,
                            void (*par_Callback)(void* par_Parameter), void *par_Parameter)
{
    int Ret = 0;
    int CallbackImmediately = 0;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return Ret;
    }

    ENTER_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "%06llu (%i) StopRequestWithCallback()\n", GetSchedulerInfos(0)->Cycle, __LINE__);
    fflush (DisableEnableLog);
#endif

    switch (par_FromUser) {
    case SCHEDULER_CONTROLED_BY_SYSTEM:
        par_Requests->EnableDisableCounter++;
        break;
    case SCHEDULER_CONTROLED_BY_USER:
        par_Requests->UserEnableDisableCounter++;
        break;
    case SCHEDULER_CONTROLED_BY_RPC:
        par_Requests->RpcEnableDisableCounter++;
        break;
    default:  // all other would be ignored
        break;
    }

    if (par_Requests->SchedulerDisabledFlag) {
        CallbackImmediately = 1;
    } else {
        if (par_Requests->StopCallbackCount < (MAX_OPEN_REQUESTS - 1)) {
            par_Requests->StopCallbackElems[par_Requests->StopCallbackCount].FromThreadId = par_FromThreadId;
            par_Requests->StopCallbackElems[par_Requests->StopCallbackCount].Callback = par_Callback;
            par_Requests->StopCallbackElems[par_Requests->StopCallbackCount].Parameter = par_Parameter;
            par_Requests->StopCallbackElems[par_Requests->StopCallbackCount].FromUser = par_FromUser;
            par_Requests->StopCallbackCount++;
        } else {
            Ret = -1;
        }
    }
    LEAVE_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
    if (Ret) ThrowError (1, "cannot add stop scheduler request callback not more than %i allowed", MAX_OPEN_REQUESTS);
    if (CallbackImmediately) {
        if (par_Callback != NULL) {
            par_Callback(par_Parameter);
        }
    }
    return Ret;
}


// Return -1 if an error occur and the stop scheduler request are not added.
// Or 0 stop scheduler request are added during the scheduler is running.
// Or 1 if stop scheduler request are added but the scheduler was already stopped.
// Or 2 the stop scheduler request are not added because the condition are already true.
int AddTimedStopRequest(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser,
                        uint64_t par_AtCycle, const char *par_Equation,
                        void (*par_Callback)(void* par_Parameter), void *par_Parameter,
                        int par_ContinueFlag)
{
    int Ret = 0;
    struct EXEC_STACK_ELEM *ExecStack;
    char *ErrString;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return Ret;
    }

    if (par_Equation != NULL) {
        ExecStack = solve_equation_err_string ((char*)par_Equation, &ErrString);
        if (ExecStack == NULL) {
            ThrowError(1, "cannot set stop request condition because \"%s\"", (ErrString == NULL) ? "unknown" : ErrString);
        } else {
            // Don't stop the scheduler if conditons is already true
            double Value = execute_stack(ExecStack);
            if (Value >= 0.5) {
                remove_exec_stack(ExecStack);
                return 2;
            }
        }
    } else {
        ExecStack = NULL;
    }

    ENTER_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);

#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "%06llu (%i) AddTimedStopRequest(%p, %i, %i, %llu, %p, %p, %i)\n", GetSchedulerInfos(0)->Cycle, __LINE__,
            par_Requests,  par_FromThreadId, par_FromUser,
            par_AtCycle,
            par_Callback, par_Parameter,
            par_ContinueFlag);
    fflush (DisableEnableLog);
#endif

    if (par_Requests->OpenStopRequestCount < (MAX_OPEN_REQUESTS - 1)) {
        par_Requests->StopRequestElems[par_Requests->OpenStopRequestCount].FromThreadId = par_FromThreadId;
        par_Requests->StopRequestElems[par_Requests->OpenStopRequestCount].Callback = par_Callback;
        par_Requests->StopRequestElems[par_Requests->OpenStopRequestCount].Parameter = par_Parameter;
        par_Requests->StopRequestElems[par_Requests->OpenStopRequestCount].FromUser = par_FromUser;
        par_Requests->StopRequestElems[par_Requests->OpenStopRequestCount].AtCycle = par_AtCycle;
        par_Requests->StopRequestElems[par_Requests->OpenStopRequestCount].ExecStack = ExecStack;
        par_Requests->OpenStopRequestCount++;
    } else {
        Ret = -1;
    }

    if (par_ContinueFlag && (Ret == 0)) {

        switch (par_FromUser) {
        case SCHEDULER_CONTROLED_BY_SYSTEM:
            if (par_Requests->EnableDisableCounter > 0) {
                par_Requests->EnableDisableCounter--;
            } else {
                Ret = -1;  // negativ response -> system error
            }
            break;
        case SCHEDULER_CONTROLED_BY_USER:
            if (par_Requests->UserEnableDisableCounter > 0) {
                par_Requests->UserEnableDisableCounter--;
            }
            break;
        case SCHEDULER_CONTROLED_BY_RPC:
            if (par_Requests->RpcEnableDisableCounter > 0) {
                par_Requests->RpcEnableDisableCounter--;
            }
            break;
        default:  // all other would be ignored
            break;
        }
        if (Ret == 0) {
            if ((par_Requests->RpcEnableDisableCounter == 0) && (par_Requests->UserEnableDisableCounter == 0) && (par_Requests->EnableDisableCounter == 0)) {
                Ret = 1;
            }
        }
    }
    if (Ret == 1) {
        SchedulerWakeupNoLock(par_Requests, __LINE__);  // oder ausserhalb der Critical Section?
    }

    LEAVE_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
    if (Ret == -1) ThrowError (1, "cannot add timed stop scheduler request not more than %i allowed", MAX_OPEN_REQUESTS);
    return Ret;
}

int RemoveAllStopRequest(SCHEDULER_STOP_REQ *par_Requests)
{
    int x;
    int Ret;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return 0;
    }
    ENTER_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
    for (x = 0; x < par_Requests->OpenStopRequestCount; x++) {
        int y;
        par_Requests->OpenStopRequestCount--;
        for (y = x; y < par_Requests->OpenStopRequestCount; y++) {
            par_Requests->StopRequestElems[y] = par_Requests->StopRequestElems[y+1];
        }
    }
    Ret = par_Requests->OpenStopRequestCount;
    LEAVE_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
    return Ret;
}

static int ShouldSchedulerStoppedAndAckAllStopRequest(SCHEDULER_STOP_REQ *par_Requests, uint64_t par_CurrentCycle)
{
    int x;
    void (*Callbacks[2*MAX_OPEN_REQUESTS])(void*);
    void *CallbackParameters[2*MAX_OPEN_REQUESTS];
    int CallbackCount = 0;
    int Ret = 0;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return Ret;
    }
    //ENTER_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "%06llu (%i) ShouldSchedulerStoppedAndAckAllStopRequest()\n", GetSchedulerInfos(0)->Cycle, __LINE__);
    fflush (DisableEnableLog);
#endif
    for (x = 0; x < par_Requests->OpenStopRequestCount; x++) {
        int EquationHit = 0;
        if (par_Requests->StopRequestElems[x].ExecStack != NULL) {
            double Value = execute_stack(par_Requests->StopRequestElems[x].ExecStack);
            if (Value >= 0.5) {
                EquationHit = 1;
            }
        }
        if (EquationHit || (par_Requests->StopRequestElems[x].AtCycle <= par_CurrentCycle)) {
            int y;
            if (par_Requests->StopRequestElems[x].Callback != NULL) {
                Callbacks[CallbackCount] = par_Requests->StopRequestElems[x].Callback;
                CallbackParameters[CallbackCount] = par_Requests->StopRequestElems[x].Parameter;
                CallbackCount++;
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
                fprintf(DisableEnableLog, "  find stop req callback (%p)  %i\n", par_Requests->StopRequestElems[x].Callback, __LINE__);
                fflush(DisableEnableLog);
#endif
            }
            switch (par_Requests->StopRequestElems[x].FromUser) {
            case SCHEDULER_CONTROLED_BY_SYSTEM:
                par_Requests->EnableDisableCounter++;
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
                fprintf (DisableEnableLog, "  (%i) there is a timed stop request EnableDisableCounter incremented to %i\n", __LINE__, par_Requests->EnableDisableCounter);
                fflush (DisableEnableLog);
#endif
                break;
            case SCHEDULER_CONTROLED_BY_USER:
                if (par_Requests->UserEnableDisableCounter == 0) {  // user disable scheduler counter should be only 0 or 1
                    par_Requests->UserEnableDisableCounter++;
                }
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
                fprintf (DisableEnableLog, "  (%i) there is a timed stop request UserEnableDisableCounter incremented to %i\n", __LINE__, par_Requests->UserEnableDisableCounter);
                fflush (DisableEnableLog);
#endif
                break;
            case SCHEDULER_CONTROLED_BY_RPC:
                par_Requests->RpcEnableDisableCounter++;
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
                fprintf (DisableEnableLog, "  (%i) there is a timed stop request RpcEnableDisableCounter incremented to %i\n", __LINE__, par_Requests->RpcEnableDisableCounter);
                fflush (DisableEnableLog);
#endif
                break;
            }
            par_Requests->OpenStopRequestCount--;
            for (y = x; y < par_Requests->OpenStopRequestCount; y++) {
                par_Requests->StopRequestElems[y] = par_Requests->StopRequestElems[y+1];
            }
            x--;
        }
    }
    if ((par_Requests->EnableDisableCounter) ||
        (par_Requests->UserEnableDisableCounter) ||
        (par_Requests->RpcEnableDisableCounter)) {
        Ret = 1;  // Should be stopped
        par_Requests->SchedulerDisabledFlag = 1;
        if (par_Requests->StopRequestElems[x].ExecStack != NULL) {
            remove_exec_stack(par_Requests->StopRequestElems[x].ExecStack);
            par_Requests->StopRequestElems[x].ExecStack = NULL;
        }
        for (x = 0; x < par_Requests->StopCallbackCount; x++) {
            Callbacks[CallbackCount] = par_Requests->StopCallbackElems[x].Callback;
            CallbackParameters[CallbackCount] = par_Requests->StopCallbackElems[x].Parameter;
            CallbackCount++;
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
            fprintf(DisableEnableLog, "  find callback (%p)  %i\n", par_Requests->StopCallbackElems[x].Callback, __LINE__);
            fflush(DisableEnableLog);
#endif
        }
        par_Requests->StopCallbackCount = 0;
    }
    //LEAVE_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);

    for (x = 0; x < CallbackCount; x++) {
        if (Callbacks[x] != NULL) {
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
            fprintf(DisableEnableLog, "  callback (%p)  %i\n", Callbacks[x], __LINE__);
            fflush(DisableEnableLog);
#endif
            Callbacks[x](CallbackParameters[x]);
        }
    }
    if (par_Requests->UserEnableDisableCounter) {
        SchedulerStateChanged(0);
    }
    return Ret;
}

static int SchedulerGoToSleep(SCHEDULER_STOP_REQ *par_Requests)
{
    if (s_main_ini_val.ConnectToRemoteMaster) {
        return 0;
    }
    //ENTER_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "Scheduler go to sleep %i\n", par_Requests->WaitForSchedulerContinueWakeFlag);
    fflush (DisableEnableLog);
#endif
    par_Requests->WaitForSchedulerContinueWakeFlag = 0;
    while (!par_Requests->WaitForSchedulerContinueWakeFlag) {
        SleepConditionVariableCS_INFINITE(&(par_Requests->WaitForSchedulerContinueConditionVariable), &(par_Requests->WaitForSchedulerContinueCriticalSection));
    }
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf(DisableEnableLog, "Scheduler wake up\n");
    fflush(DisableEnableLog);
#endif
    par_Requests->SchedulerDisabledFlag = 0;
    //LEAVE_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
    return 0;
}

static int SchedulerAreWakeupedNoLock (SCHEDULER_STOP_REQ *par_Requests)
{
    void (*Callbacks[2*MAX_OPEN_REQUESTS])(void*);
    void *CallbackParameters[2*MAX_OPEN_REQUESTS];
    int CallbackCount = 0;
    int x;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return 0;
    }
    //ENTER_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "SchedulerAreWakeuped()\n");
    fflush (DisableEnableLog);
#endif
    for (x = 0; x < par_Requests->ContinueCallbackCount; x++) {
        Callbacks[CallbackCount] = par_Requests->ContinueCallbackElems[x].Callback;
        CallbackParameters[CallbackCount] = par_Requests->ContinueCallbackElems[x].Parameter;
        CallbackCount++;
    }
    par_Requests->ContinueCallbackCount = 0;
    //LEAVE_CRITICAL_SECTION_LOG (&par_Requests->StopStartRequestCriticalSection);
    for (x = 0; x < CallbackCount; x++) {
        if (Callbacks[x] != NULL) {
            Callbacks[x](CallbackParameters[x]);
        }
    }
    return 0;
}

void ShouldSchedulerSleep(struct SCHEDULER_DATA_STRUCT *pSchedulerData)
{
    SCHEDULER_STOP_REQ *par_Requests = &(pSchedulerData->StopReq);
    ENTER_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
    if (ShouldSchedulerStoppedAndAckAllStopRequest(&(pSchedulerData->StopReq), pSchedulerData->Cycle)) {
        pSchedulerData->State = SCHED_IS_STOPPED_STATE;
        SchedulerGoToSleep(&(pSchedulerData->StopReq));  // This will wait till enable scheduler was called
        pSchedulerData->State = SCHED_RUNNING_STATE;
        SchedulerAreWakeupedNoLock (&(pSchedulerData->StopReq));
    }
    LEAVE_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
}

int SchedulerWakeupNoLock (SCHEDULER_STOP_REQ *par_Requests, int par_LineNr)
{
    if (s_main_ini_val.ConnectToRemoteMaster) {
        return 0;
    }
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "SchedulerWakeupNoLock() %i\n", par_LineNr);
    fflush (DisableEnableLog);
#else
    UNUSED(par_LineNr);
#endif
    par_Requests->WaitForSchedulerContinueWakeFlag = 1;
    WakeAllConditionVariable(&(par_Requests->WaitForSchedulerContinueConditionVariable));
    return 0;
}

int SchedulerWakeup (SCHEDULER_STOP_REQ *par_Requests)
{
    if (s_main_ini_val.ConnectToRemoteMaster) {
        return 0;
    }
    ENTER_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "SchedulerWakeup()\n");
    fflush (DisableEnableLog);
#endif
    par_Requests->WaitForSchedulerContinueWakeFlag = 1;
    WakeAllConditionVariable(&(par_Requests->WaitForSchedulerContinueConditionVariable));
    LEAVE_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
    return 0;
}


int IsSchedulerSleeping (SCHEDULER_STOP_REQ *par_Requests)
{
    volatile int Ret;
    if (s_main_ini_val.ConnectToRemoteMaster) {
        return 0;
    }
    ENTER_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "IsSchedulerAreSleeping()\n");
    fflush (DisableEnableLog);
#endif
    Ret = par_Requests->SchedulerDisabledFlag;
    LEAVE_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
    return Ret;
}

void RemoveAllPendingRequestsOfThisThread(SCHEDULER_STOP_REQ *par_Requests, int par_DisableCounter)
{
    int x, y;
    int ThrowErrorFlag;
    int ThreadId = (int)GetCurrentThreadId();
    if (s_main_ini_val.ConnectToRemoteMaster) {
        return;
    }
    ENTER_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
#ifdef DEBUG_ENABLE_DISABLE_SCHED_LOG
    fprintf (DisableEnableLog, "RemoveAllPendingRequestsOfThisThread()\n");
    fflush (DisableEnableLog);
#endif

    for(x = 0; x < par_Requests->OpenStopRequestCount; x++) {
        if (par_Requests->StopRequestElems[x].FromThreadId == ThreadId) {
            for(y = x+1; y < par_Requests->OpenStopRequestCount; y++) {
                par_Requests->StopRequestElems[y-1] = par_Requests->StopRequestElems[y];
            }
            par_Requests->OpenStopRequestCount--;
        }
    }
    for(x = 0; x < par_Requests->StopCallbackCount; x++) {
        if (par_Requests->StopCallbackElems[x].FromThreadId == ThreadId) {
            for(y = x+1; y < par_Requests->StopCallbackCount; y++) {
                par_Requests->StopCallbackElems[y-1] = par_Requests->StopCallbackElems[y];
            }
            par_Requests->StopCallbackCount--;
        }
    }
    for(x = 0; x < par_Requests->ContinueCallbackCount; x++) {
        if (par_Requests->ContinueCallbackElems[x].FromThreadId == ThreadId) {
            for(y = x+1; y < par_Requests->ContinueCallbackCount; y++) {
                par_Requests->ContinueCallbackElems[y-1] = par_Requests->ContinueCallbackElems[y];
            }
            par_Requests->ContinueCallbackCount--;
        }
    }

    if (par_Requests->RpcEnableDisableCounter < par_DisableCounter) {
        ThrowErrorFlag = 1;
        par_Requests->RpcEnableDisableCounter = 0;
    } else {
        ThrowErrorFlag = 0;
        par_Requests->RpcEnableDisableCounter -= par_DisableCounter;
    }

    if ((par_Requests->EnableDisableCounter == 0) && (par_Requests->UserEnableDisableCounter == 0) && (par_Requests->RpcEnableDisableCounter == 0)) {
        SchedulerWakeupNoLock(par_Requests, __LINE__);
    }
    LEAVE_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
    if (ThrowErrorFlag){
        ThrowError(1, "There should be remove more scheduler disable request than existing");
    }
}


void StopRequestGetCounters (SCHEDULER_STOP_REQ *par_Requests, int *ret_System, int *ret_User, int *ret_Rpc)
{
    ENTER_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
    *ret_System = par_Requests->EnableDisableCounter;
    *ret_User = par_Requests->UserEnableDisableCounter;
    *ret_Rpc = par_Requests->RpcEnableDisableCounter;
    LEAVE_CRITICAL_SECTION_LOG(&(par_Requests->WaitForSchedulerContinueCriticalSection));
}

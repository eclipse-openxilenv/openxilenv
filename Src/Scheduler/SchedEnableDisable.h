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


#ifndef SCHEDENABLEDISABLE_H
#define SCHEDENABLEDISABLE_H

#include <stdint.h>
#include "Platform.h"

#define MAX_OPEN_REQUESTS 8

typedef struct {
    int FromThreadId;
    int FromUser;
    uint64_t AtCycle;
    struct EXEC_STACK_ELEM *ExecStack;
    void (*Callback)(void* par_Parameter);
    void *Parameter;
} SCHEDULER_STOP_TIMED_REQ_ELEM;

typedef struct {
    int FromThreadId;
    int FromUser;
    void (*Callback)(void* par_Parameter);
    void *Parameter;
} SCHEDULER_CALLBACK_ELEM;

typedef struct {
    CRITICAL_SECTION StopStartRequestCriticalSection;
    int OpenStopRequestCount;
    SCHEDULER_STOP_TIMED_REQ_ELEM StopRequestElems[MAX_OPEN_REQUESTS];
    int StopCallbackCount;
    SCHEDULER_CALLBACK_ELEM StopCallbackElems[MAX_OPEN_REQUESTS];
    int ContinueCallbackCount;
    SCHEDULER_CALLBACK_ELEM ContinueCallbackElems[MAX_OPEN_REQUESTS];
    volatile int SchedulerDisabledFlag;
    int EnableDisableCounter;      // Counts the scheduler disable request from system, independed if the scheduler already stopped
    int UserEnableDisableCounter;  // Counts the scheduler disable request from the user, independed if the scheduler already stopped
    int RpcEnableDisableCounter;   // Counts the scheduler disable request through the RPC API, independed if the scheduler already stopped

    //CRITICAL_SECTION WaitForSchedulerContinueCriticalSection;
#define WaitForSchedulerContinueCriticalSection StopStartRequestCriticalSection
    CONDITION_VARIABLE WaitForSchedulerContinueConditionVariable;
    int WaitForSchedulerContinueWakeFlag;
    int WaitForSchedulerContinueCriticalSectionLineNr;

} SCHEDULER_STOP_REQ;

int InitStopRequest(SCHEDULER_STOP_REQ *par_Requests);

int StopRequest(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser);

int ContinueRequest(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser);

int ContinueRequestWithCallback(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser,
                                void (*par_Callback)(void* par_Parameter), void *par_Parameter);

int StopRequestWithCallback(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser,
                            void (*par_Callback)(void* par_Parameter), void *par_Parameter);

int AddStopRequest(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser,
                   uint64_t par_Cycles, uint64_t par_CurrentCycle,
                   void (*par_Callback)(void* par_Parameter), void *par_Parameter);

int AddTimedStopRequest(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser,
                        uint64_t par_AtCycle, const char *par_Equation,
                        void (*par_Callback)(void* par_Parameter), void *par_Parameter,
                        int par_ContinueFlag);

//int RemoveStopRequest(SCHEDULER_STOP_REQ *par_Requests, int par_FromThreadId, int par_FromUser);

int RemoveAllStopRequest(SCHEDULER_STOP_REQ *par_Requests);

struct SCHEDULER_DATA_STRUCT;
void ShouldSchedulerSleep(struct SCHEDULER_DATA_STRUCT *pSchedulerData);
//int ShouldSchedulerStoppedAndAckAllStopRequest(SCHEDULER_STOP_REQ *par_Requests, uint64_t par_CurrentCycle);
//int SchedulerGoToSleep(SCHEDULER_STOP_REQ *par_Requests);

int SchedulerWakeupNoLock (SCHEDULER_STOP_REQ *par_Requests, int par_LineNr);
int SchedulerWakeup (SCHEDULER_STOP_REQ *par_Requests);

int IsSchedulerSleeping (SCHEDULER_STOP_REQ *par_Requests);

void RemoveAllPendingRequestsOfThisThread(SCHEDULER_STOP_REQ *par_Requests, int par_DisableCounter);

void StopRequestGetCounters (SCHEDULER_STOP_REQ *par_Requests, int *ret_System, int *ret_User, int *ret_Rpc);

#endif // SCHEDENABLEDISABLE_H

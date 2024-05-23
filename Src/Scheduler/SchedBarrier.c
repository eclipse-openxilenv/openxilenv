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


#include "Platform.h"
#include <stdio.h>
#include <stdlib.h>


#include "tcb.h"
#include "IniDataBase.h"
#include "Files.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Wildcards.h"
#include "Scheduler.h"
#include "UniqueNumber.h"

#include "SchedBarrier.h"

#define MAX_BARRIERS 8
#define MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS 64
#define BARRIER_LOGGING_DEPTH  (8*64)


typedef struct {
    CRITICAL_SECTION CriticalSection;
    CONDITION_VARIABLE ConditionVariable;
    volatile unsigned int SchedulerFlags;
    volatile unsigned int ProcessFlags;
} INNER_BARRIER;


typedef struct {
    int State;
#define BARRIER_STATE_EMPTY   0
#define BARRIER_STATE_AKTIVE  1
#define BARRIER_STATE_MARKED_FOR_REMOVING  2

    CRITICAL_SECTION CriticalSection;
    volatile unsigned int SchedulerMask;
    volatile unsigned int ProcessMask;
    volatile int Switch;  // 0 or 1
    INNER_BARRIER Ib[2];
    char Name[MAX_BARRIER_NAME_LENGTH];
    int UniqueBefore;
    char *AssociatedSchedulerOrProcesses[MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS];
    int WaitFlags[MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS];
    int Types[MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS];
#define BARRIER_FOR_SCHEDULER        1
#define BARRIER_FOR_PROCESS          2
    int ProcesseOrSchedulerCount;
    union {
        struct TCB *Tcb;
        SCHEDULER_DATA *Schedulers;
    } Ref[MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS];

    int WaitOnCounter;
    volatile unsigned int AddNewProcessMasksBefore;
    volatile unsigned int AddNewProcessMasksBehind;
    volatile unsigned int AddNewSchedulerMasks;
    volatile unsigned int RemoveProcessMasks;
    volatile unsigned int RemoveSchedulerMasks;

    int WaitIfAlone;   // If this flag is 1 than the process/scheduler is waiting alone at this barrier.
}  SCHED_BARRIER;

static SCHED_BARRIER Barriers[MAX_BARRIERS];


typedef struct {
    int BarrierNumber;   // Index into the barriers array
    BARRIER_LOGGING_ACTION Action;
    BARRIER_LOGGING_TYPE ProcessOrSched;
    int PidOrSchedNumber;
    int LineNr;

    // Snapshot data
    unsigned int SchedulerMask;
    unsigned int ProcessMask;
    int Switch;  // 0 or 1
    unsigned int Ib_0_SchedulerFlags;
    unsigned int Ib_0_ProcessFlags;
    unsigned int Ib_1_SchedulerFlags;
    unsigned int Ib_1_ProcessFlags;

    int ProcesseOrSchedulerCount;
    unsigned int AddNewProcessMasksBefore;
    unsigned int AddNewProcessMasksBehind;
    unsigned int AddNewSchedulerMasks;
    unsigned int RemoveProcessMasks;
    unsigned int RemoveSchedulerMasks;

    unsigned int Counter;
} BARRIER_LOGGING_ELEM;

#ifdef BARRIER_LOGGING_DEPTH
static BARRIER_LOGGING_ELEM BarriersLogging[BARRIER_LOGGING_DEPTH];
static int BarriersLoggingPos;
static CRITICAL_SECTION BarriersLoggingCriticalSection;
static unsigned int BarriersLoggingCounter;

static void AddBarrierLogging (int par_Lock, int par_BarrierNumber, BARRIER_LOGGING_ACTION par_Action,
                              BARRIER_LOGGING_TYPE par_ProcessOrSched, int par_PidOrSchedNumber,
                              int par_LineNr
                              )
{
    if (par_Lock) EnterCriticalSection (&BarriersLoggingCriticalSection);
    BarriersLogging[BarriersLoggingPos].BarrierNumber = par_BarrierNumber;
    BarriersLogging[BarriersLoggingPos].Action = par_Action;
    BarriersLogging[BarriersLoggingPos].ProcessOrSched = par_ProcessOrSched;
    BarriersLogging[BarriersLoggingPos].LineNr = par_LineNr;
    BarriersLogging[BarriersLoggingPos].PidOrSchedNumber = par_PidOrSchedNumber;

    // Spapshot
    if ((par_BarrierNumber >= 0) && (par_BarrierNumber < MAX_BARRIERS)) {
        BarriersLogging[BarriersLoggingPos].SchedulerMask = Barriers[par_BarrierNumber].SchedulerMask;
        BarriersLogging[BarriersLoggingPos].ProcessMask = Barriers[par_BarrierNumber].ProcessMask;
        BarriersLogging[BarriersLoggingPos].Switch = Barriers[par_BarrierNumber].Switch;
        BarriersLogging[BarriersLoggingPos].Ib_0_SchedulerFlags = Barriers[par_BarrierNumber].Ib[0].SchedulerFlags;
        BarriersLogging[BarriersLoggingPos].Ib_0_ProcessFlags = Barriers[par_BarrierNumber].Ib[0].ProcessFlags;
        BarriersLogging[BarriersLoggingPos].Ib_1_SchedulerFlags = Barriers[par_BarrierNumber].Ib[1].SchedulerFlags;
        BarriersLogging[BarriersLoggingPos].Ib_1_ProcessFlags = Barriers[par_BarrierNumber].Ib[1].ProcessFlags;
        BarriersLogging[BarriersLoggingPos].ProcesseOrSchedulerCount = Barriers[par_BarrierNumber].ProcesseOrSchedulerCount;
        BarriersLogging[BarriersLoggingPos].AddNewProcessMasksBefore = Barriers[par_BarrierNumber].AddNewProcessMasksBefore;
        BarriersLogging[BarriersLoggingPos].AddNewProcessMasksBehind = Barriers[par_BarrierNumber].AddNewProcessMasksBehind;
        BarriersLogging[BarriersLoggingPos].AddNewSchedulerMasks = Barriers[par_BarrierNumber].AddNewSchedulerMasks;
        BarriersLogging[BarriersLoggingPos].RemoveProcessMasks = Barriers[par_BarrierNumber].RemoveProcessMasks;
        BarriersLogging[BarriersLoggingPos].RemoveSchedulerMasks = Barriers[par_BarrierNumber].RemoveSchedulerMasks;
    }

    BarriersLogging[BarriersLoggingPos].Counter = BarriersLoggingCounter;

    if (BarriersLoggingPos >= (BARRIER_LOGGING_DEPTH-1)) BarriersLoggingPos = 0;
    else BarriersLoggingPos++;

    BarriersLoggingCounter++;
    if (par_Lock) LeaveCriticalSection (&BarriersLoggingCriticalSection);
}

#define ADD_BARRIER_LOGGING(par_Lock, par_BarrierNumber, par_Action, par_ProcessOrSched, par_PidOrSchedNumber, par_LineNr)\
AddBarrierLogging ((par_Lock), (par_BarrierNumber), (par_Action), (par_ProcessOrSched), (par_PidOrSchedNumber), (par_LineNr))
#else
#define ADD_BARRIER_LOGGING(par_Lock, par_BarrierNumber, par_Action, par_ProcessOrSched, par_PidOrSchedNumber, par_LineNr)
#endif

    int GetBarrierLoggingSize (void)
{
#ifdef BARRIER_LOGGING_DEPTH
    return BARRIER_LOGGING_DEPTH;
#else
    return 0;
#endif
}

int GetNextBarrierLoggingEntry (int par_Index, char *ret_BarrierName,
                                BARRIER_LOGGING_ACTION *ret_Action,
                                BARRIER_LOGGING_TYPE *ret_ProcessOrSched, char *ret_ProcOrSchedName,
                                int *ret_LineNr,

                                // Snapshot
                                unsigned int *ret_SchedulerMask,
                                unsigned int *ret_ProcessMask,
                                int *ret_Switch,  // 0 or 1
                                unsigned int *ret_Ib_0_SchedulerFlags,
                                unsigned int *ret_Ib_0_ProcessFlags,
                                unsigned int *ret_Ib_1_SchedulerFlags,
                                unsigned int *ret_Ib_1_ProcessFlags,

                                int *ret_ProcesseOrSchedulerCount,
                                unsigned int *ret_AddNewProcessMasksBefore,
                                unsigned int *ret_AddNewProcessMasksBehind,
                                unsigned int *ret_AddNewSchedulerMasks,
                                unsigned int *ret_RemoveProcessMasks,
                                unsigned int *ret_RemoveSchedulerMasks,

                                unsigned int *ret_Counter
                                )
{
#ifdef BARRIER_LOGGING_DEPTH
    int Index;

    if (par_Index < 0) {
        Index = BarriersLoggingPos;
        EnterCriticalSection (&BarriersLoggingCriticalSection);
        while (BarriersLogging[Index].LineNr == 0) {
            if (Index >= (BARRIER_LOGGING_DEPTH-1)) Index = 0;
            else Index++;
            if (Index == BarriersLoggingPos) {
                LeaveCriticalSection (&BarriersLoggingCriticalSection);
                return -1;  // There is no element
            }
        }
    } else {
        Index = par_Index;
        if (Index == BarriersLoggingPos) {
            LeaveCriticalSection (&BarriersLoggingCriticalSection);
            return -1;  // End of the list
        }
    }
    strcpy (ret_BarrierName, Barriers[BarriersLogging[Index].BarrierNumber].Name);

    *ret_Action = BarriersLogging[Index].Action;
    *ret_ProcessOrSched = BarriersLogging[Index].ProcessOrSched;
    switch (BarriersLogging[Index].ProcessOrSched) {
    //default:
    case BARRIER_LOGGING_UNDEF_TYPE:
        strcpy (ret_ProcOrSchedName, "undef");
        break;
    case BARRIER_LOGGING_PROCESS_TYPE:
        if (GetProcessShortName(BarriersLogging[Index].PidOrSchedNumber, ret_ProcOrSchedName)) {
            sprintf (ret_ProcOrSchedName, "unknown pid = %i", BarriersLogging[Index].PidOrSchedNumber);
        }
        break;
    case BARRIER_LOGGING_SCHEDULER_TYPE:
        strcpy (ret_ProcOrSchedName,  GetSchedulerName (BarriersLogging[Index].PidOrSchedNumber));
        break;
    }
    *ret_LineNr = BarriersLogging[Index].LineNr;

    // Snapshot
    *ret_SchedulerMask = BarriersLogging[Index].SchedulerMask;
    *ret_ProcessMask = BarriersLogging[Index].ProcessMask;
    *ret_Switch = BarriersLogging[Index].Switch;
    *ret_Ib_0_SchedulerFlags = BarriersLogging[Index].Ib_0_SchedulerFlags;
    *ret_Ib_0_ProcessFlags = BarriersLogging[Index].Ib_0_ProcessFlags;
    *ret_Ib_1_SchedulerFlags = BarriersLogging[Index].Ib_1_SchedulerFlags;
    *ret_Ib_1_ProcessFlags = BarriersLogging[Index].Ib_1_ProcessFlags;

    *ret_ProcesseOrSchedulerCount = BarriersLogging[Index].ProcesseOrSchedulerCount;
    *ret_AddNewProcessMasksBefore = BarriersLogging[Index].AddNewProcessMasksBefore;
    *ret_AddNewProcessMasksBehind = BarriersLogging[Index].AddNewProcessMasksBehind;
    *ret_AddNewSchedulerMasks = BarriersLogging[Index].AddNewSchedulerMasks;
    *ret_RemoveProcessMasks = BarriersLogging[Index].RemoveProcessMasks;
    *ret_RemoveSchedulerMasks = BarriersLogging[Index].RemoveSchedulerMasks;

    *ret_Counter = BarriersLogging[Index].Counter;

    if (Index >= (BARRIER_LOGGING_DEPTH-1)) Index = 0;
    else Index++;
    return Index;
#endif
}

static CRITICAL_SECTION GlobalBarrierCriticalSection;


int ConnectProcessToBarrier (struct TCB *pTcb, char *par_BarrierName, int par_WaitFlags)
{
    int x;

    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        if (Barriers[x].State == BARRIER_STATE_AKTIVE) {
            if (!strcmp (Barriers[x].Name, par_BarrierName)) {
                if (Barriers[x].ProcesseOrSchedulerCount >= MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS) {
                    LeaveCriticalSection (&GlobalBarrierCriticalSection);
                    ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" because max. %i processes/schedulers allowed",
                           pTcb->name, par_BarrierName, MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS);
                    return -1;
                }
                // Only for information store the name
                Barriers[x].AssociatedSchedulerOrProcesses[Barriers[x].ProcesseOrSchedulerCount] = my_malloc (strlen (pTcb->name) + 1);
                strcpy (Barriers[x].AssociatedSchedulerOrProcesses[Barriers[x].ProcesseOrSchedulerCount], pTcb->name);
                Barriers[x].Types[Barriers[x].ProcesseOrSchedulerCount] = BARRIER_FOR_PROCESS;
                Barriers[x].WaitFlags[Barriers[x].ProcesseOrSchedulerCount] = par_WaitFlags;
                Barriers[x].Ref[Barriers[x].ProcesseOrSchedulerCount].Tcb = pTcb;

                Barriers[x].ProcesseOrSchedulerCount++;
                LeaveCriticalSection (&GlobalBarrierCriticalSection);
                return x;
            }
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
    ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" because barrier doesn't exists",
           pTcb->name, par_BarrierName);

    return -1;
}


void ActivateProcessBarriers (struct TCB *pTcb)
{
    int x;
    SCHED_BARRIER *pBarrier;

    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < pTcb->BarrierBeforeCount; x++) {
        pBarrier = &(Barriers[pTcb->BarrierBeforeNr[x]]);
        pBarrier->AddNewProcessMasksBefore |= pTcb->bb_access_mask;
        ADD_BARRIER_LOGGING (0, pTcb->BarrierBeforeNr[x], BARRIER_LOGGING_ADD_BEFORE_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
    }
    for (x = 0; x < pTcb->BarrierBehindCount; x++) {
        pBarrier = &(Barriers[pTcb->BarrierBehindNr[x]]);
        pBarrier->AddNewProcessMasksBehind |= pTcb->bb_access_mask;
        ADD_BARRIER_LOGGING (0, pTcb->BarrierBehindNr[x], BARRIER_LOGGING_ADD_BEHIND_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
    }
    for (x = 0; x < pTcb->BarrierLoopOutBeforeCount; x++) {
        pBarrier = &(Barriers[pTcb->BarrierLoopOutBeforeNr[x]]);
        pBarrier->AddNewProcessMasksBefore |= pTcb->bb_access_mask;
        ADD_BARRIER_LOGGING (0, pTcb->BarrierLoopOutBeforeNr[x], BARRIER_LOGGING_ADD_BEFORE_LOOPOUT_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
    }
    for (x = 0; x < pTcb->BarrierLoopOutBehindCount; x++) {
        pBarrier = &(Barriers[pTcb->BarrierLoopOutBehindNr[x]]);
        pBarrier->AddNewProcessMasksBehind |= pTcb->bb_access_mask;
        ADD_BARRIER_LOGGING (0, pTcb->BarrierLoopOutBehindNr[x], BARRIER_LOGGING_ADD_BEHIND_LOOPOUT_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
}


int DisconnectProcessFromBarrier (struct TCB *pTcb, char *par_BarrierName, int par_EmergencyCleanup)
{
    int x, i;
    int RemoveCounter = 0;
    SCHED_BARRIER *pBarrier;

    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        pBarrier = &(Barriers[x]);
        if (pBarrier->State == BARRIER_STATE_AKTIVE) {
            if ((par_BarrierName == NULL) ||
                !strcmp (pBarrier->Name, par_BarrierName)) {
                for (i = 0; i < pBarrier->ProcesseOrSchedulerCount; i++) {
                    if (pBarrier->Types[i] == BARRIER_FOR_PROCESS) {
                        if (pBarrier->Ref[i].Tcb == pTcb) {
                            RemoveCounter++;
                            my_free(pBarrier->AssociatedSchedulerOrProcesses[i]);
                            for (i++; i < pBarrier->ProcesseOrSchedulerCount; i++) {
                                pBarrier->AssociatedSchedulerOrProcesses[i-1] = pBarrier->AssociatedSchedulerOrProcesses[i];
                                pBarrier->WaitFlags[i-1] = pBarrier->WaitFlags[i];
                                pBarrier->Types[i-1] = pBarrier->Types[i];
                                pBarrier->Ref[i-1] = pBarrier->Ref[i];
                            }
                            pBarrier->ProcesseOrSchedulerCount--;
                            if (par_EmergencyCleanup) {
                                if ((pBarrier->ProcessMask & pTcb->bb_access_mask) == pTcb->bb_access_mask) {
                                    if (((pBarrier->ProcessMask & ~pBarrier->RemoveProcessMasks) == pTcb->bb_access_mask) &&
                                        ((pBarrier->SchedulerMask & ~pBarrier->RemoveSchedulerMasks) == 0)) {
                                        pBarrier->ProcessMask = 0;
                                        pBarrier->SchedulerMask = 0;

                                        pBarrier->WaitOnCounter++;
                                        ADD_BARRIER_LOGGING (0, x, BARRIER_LOGGING_EMERGENCY_CLEANUP_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);

                                        // Last process and scheduler than immediately delete here and if somebody is waiting for signal it
                                        EnterCriticalSection (&(pBarrier->Ib[0].CriticalSection));
                                        pBarrier->Ib[0].ProcessFlags = 0;
                                        pBarrier->Ib[0].SchedulerFlags = 0;
                                        WakeAllConditionVariable (&(pBarrier->Ib[0].ConditionVariable));
                                        LeaveCriticalSection (&(pBarrier->Ib[0].CriticalSection));

                                        EnterCriticalSection (&(pBarrier->Ib[1].CriticalSection));
                                        pBarrier->Ib[1].ProcessFlags = 0;
                                        pBarrier->Ib[1].SchedulerFlags = 0;
                                        WakeAllConditionVariable (&(pBarrier->Ib[1].ConditionVariable));
                                        LeaveCriticalSection (&(pBarrier->Ib[1].CriticalSection));

                                        pBarrier->RemoveProcessMasks = 0;
                                        pBarrier->RemoveSchedulerMasks = 0;
                                    } else {
                                        // If not the last one look if there is waiting someone
                                        ADD_BARRIER_LOGGING (0, x, BARRIER_LOGGING_EMERGENCY_CLEANUP_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                                        pBarrier->RemoveProcessMasks |= pTcb->bb_access_mask;

                                        LeaveCriticalSection (&GlobalBarrierCriticalSection);
                                        EnterCriticalSection (&(pBarrier->Ib[0].CriticalSection));
                                        pBarrier->Ib[0].ProcessFlags |= pTcb->bb_access_mask;
                                        if (((pBarrier->Ib[0].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
                                            ((pBarrier->Ib[0].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
                                            pBarrier->Ib[0].ProcessFlags = 0;
                                            pBarrier->Ib[0].SchedulerFlags = 0;
                                            ADD_BARRIER_LOGGING (0, x, BARRIER_LOGGING_EMERGENCY_CLEANUP_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                                            WakeAllConditionVariable (&(pBarrier->Ib[0].ConditionVariable));
                                        }
                                        LeaveCriticalSection (&(pBarrier->Ib[0].CriticalSection));
                                        EnterCriticalSection (&(pBarrier->Ib[1].CriticalSection));
                                        pBarrier->Ib[1].ProcessFlags |= pTcb->bb_access_mask;
                                        if (((pBarrier->Ib[1].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
                                            ((pBarrier->Ib[1].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
                                            pBarrier->Ib[1].ProcessFlags = 0;
                                            pBarrier->Ib[1].SchedulerFlags = 0;
                                            ADD_BARRIER_LOGGING (0, x, BARRIER_LOGGING_EMERGENCY_CLEANUP_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                                            WakeAllConditionVariable (&(pBarrier->Ib[1].ConditionVariable));
                                        }
                                        LeaveCriticalSection (&(pBarrier->Ib[1].CriticalSection));
                                        EnterCriticalSection (&GlobalBarrierCriticalSection);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
    if ((par_BarrierName != NULL) && (RemoveCounter == 0)) {
        ThrowError (1, "cannot remove process \"%s\" from barrier \"%s\"", pTcb->name, par_BarrierName);
        return -1;
    }
    return 0;
}

void DisconnectProcessFromAllBarriers (struct TCB *pTcb, int par_EmergencyCleanup)
{
    DisconnectProcessFromBarrier (pTcb, NULL, par_EmergencyCleanup);
}



int ConnectSchedulerToBarrier (SCHEDULER_DATA *Scheduler, char *par_BarrierName, int par_WaitFlags)
{
    int x;

    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        if (Barriers[x].State == BARRIER_STATE_AKTIVE) {
            if (!strcmp (Barriers[x].Name, par_BarrierName)) {
                if (Barriers[x].ProcesseOrSchedulerCount >= MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS) {
                    LeaveCriticalSection (&GlobalBarrierCriticalSection);
                    ThrowError (1, "cannot associate scheduler \"%s\" to barrier \"%s\" because max. %i processes/schedulers allowed",
                           Scheduler->SchedulerName, par_BarrierName, MAX_SCHEDULER_AND_PROCESS_PER_BARRIERS);
                    return -1;
                }
                // Only for information store the name
                Barriers[x].AssociatedSchedulerOrProcesses[Barriers[x].ProcesseOrSchedulerCount] = my_malloc (strlen (Scheduler->SchedulerName) + 1);
                strcpy (Barriers[x].AssociatedSchedulerOrProcesses[Barriers[x].ProcesseOrSchedulerCount], Scheduler->SchedulerName);
                Barriers[x].Types[Barriers[x].ProcesseOrSchedulerCount] = BARRIER_FOR_SCHEDULER;
                Barriers[x].WaitFlags[Barriers[x].ProcesseOrSchedulerCount] = par_WaitFlags;
                Barriers[x].Ref[Barriers[x].ProcesseOrSchedulerCount].Schedulers = Scheduler;

                Barriers[x].AddNewSchedulerMasks |= Scheduler->BarrierMask;

                Barriers[x].ProcesseOrSchedulerCount++;

                LeaveCriticalSection (&GlobalBarrierCriticalSection);
                return x;
            }
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
    ThrowError (1, "cannot associate scheduler \"%s\" to barrier \"%s\" because barrier doesn't exists",
           Scheduler->SchedulerName, par_BarrierName);

    return -1;
}

int DisconnectSchedulerFromBarrier (SCHEDULER_DATA *Scheduler, char *par_BarrierName)
{
    int x, i;
    int RemoveCounter = 0;
    SCHED_BARRIER *pBarrier;

    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        pBarrier = &(Barriers[x]);
        if (pBarrier->State == BARRIER_STATE_AKTIVE) {
            if ((par_BarrierName == NULL) ||
                !strcmp (pBarrier->Name, par_BarrierName)) {
                for (i = 0; i < pBarrier->ProcesseOrSchedulerCount; i++) {
                    if (pBarrier->Types[i] == BARRIER_FOR_SCHEDULER) {
                        if (pBarrier->Ref[i].Schedulers == Scheduler) {
                            RemoveCounter++;
                            for (i++; i < pBarrier->ProcesseOrSchedulerCount; i++) {
                                pBarrier->AssociatedSchedulerOrProcesses[i-1] = pBarrier->AssociatedSchedulerOrProcesses[i];
                                pBarrier->WaitFlags[i-1] = pBarrier->WaitFlags[i];
                                pBarrier->Types[i-1] = pBarrier->Types[i];
                                pBarrier->Ref[i-1] = pBarrier->Ref[i];
                            }
                            pBarrier->ProcesseOrSchedulerCount--;
                            if ((pBarrier->SchedulerMask & Scheduler->BarrierMask) == Scheduler->BarrierMask) {
                                if (((pBarrier->SchedulerMask & ~pBarrier->RemoveSchedulerMasks) == Scheduler->BarrierMask) &&
                                    ((pBarrier->ProcessMask & ~pBarrier->RemoveProcessMasks) == 0)) {
                                    // Last process and scheduler than immediately delete here and if somebody is waiting for signal it
                                    pBarrier->SchedulerMask = 0;
                                    pBarrier->ProcessMask = 0;
                                    pBarrier->WaitOnCounter++;
                                    ADD_BARRIER_LOGGING (0, x, BARRIER_LOGGING_UNDEF_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex (Scheduler), __LINE__);
                                    EnterCriticalSection (&(pBarrier->Ib[0].CriticalSection));
                                    pBarrier->Ib[0].SchedulerFlags = 0;
                                    pBarrier->Ib[0].ProcessFlags = 0;
                                    WakeAllConditionVariable (&(pBarrier->Ib[0].ConditionVariable));
                                    LeaveCriticalSection (&(pBarrier->Ib[1].CriticalSection));

                                    EnterCriticalSection (&(pBarrier->Ib[1].CriticalSection));
                                    pBarrier->Ib[1].SchedulerFlags = 0;
                                    pBarrier->Ib[1].ProcessFlags = 0;
                                    WakeAllConditionVariable (&(pBarrier->Ib[1].ConditionVariable));
                                    LeaveCriticalSection (&(pBarrier->Ib[0].CriticalSection));
                                    pBarrier->RemoveSchedulerMasks = 0;
                                    pBarrier->RemoveProcessMasks = 0;
                                } else {
                                    // If not the last one look if there is waiting someone
                                    ADD_BARRIER_LOGGING (0, x, BARRIER_LOGGING_UNDEF_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex (Scheduler), __LINE__);
                                    pBarrier->RemoveSchedulerMasks |= Scheduler->BarrierMask;
                                    LeaveCriticalSection (&GlobalBarrierCriticalSection);
                                    EnterCriticalSection (&(pBarrier->Ib[0].CriticalSection));
                                    pBarrier->Ib[0].SchedulerFlags |= Scheduler->BarrierMask;
                                    if (((pBarrier->Ib[0].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
                                        ((pBarrier->Ib[0].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
                                        pBarrier->Ib[0].ProcessFlags = 0;
                                        pBarrier->Ib[0].SchedulerFlags = 0;
                                        ADD_BARRIER_LOGGING (0, x, BARRIER_LOGGING_UNDEF_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex (Scheduler), __LINE__);
                                        WakeAllConditionVariable (&(pBarrier->Ib[0].ConditionVariable));
                                    }
                                    LeaveCriticalSection (&(pBarrier->Ib[0].CriticalSection));
                                    EnterCriticalSection (&(pBarrier->Ib[1].CriticalSection));
                                    pBarrier->Ib[1].SchedulerFlags |= Scheduler->BarrierMask;
                                    if (((pBarrier->Ib[1].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
                                        ((pBarrier->Ib[1].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
                                        pBarrier->Ib[1].ProcessFlags = 0;
                                        pBarrier->Ib[1].SchedulerFlags = 0;
                                        ADD_BARRIER_LOGGING (0, x, BARRIER_LOGGING_UNDEF_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex (Scheduler), __LINE__);
                                        WakeAllConditionVariable (&(pBarrier->Ib[1].ConditionVariable));
                                    }
                                    LeaveCriticalSection (&(pBarrier->Ib[1].CriticalSection));
                                    EnterCriticalSection (&GlobalBarrierCriticalSection);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
    if ((par_BarrierName != NULL) && (RemoveCounter == 0)) {
        ThrowError (1, "cannot remove scheduler \"%s\" from barrier \"%s\"", Scheduler->SchedulerName, par_BarrierName);
        return -1;
    }
    return 0;
}

void DisconnectSchedulerFromAllBarriers (SCHEDULER_DATA *Scheduler)
{
    DisconnectSchedulerFromBarrier (Scheduler, NULL);
}


static void InitBarrier (SCHED_BARRIER *pBarrier, char *Name, int WaitIfAlone)
{
    int x;
    memset (pBarrier, 0, sizeof (SCHED_BARRIER));
    InitializeCriticalSection (&(pBarrier->CriticalSection));
    for (x = 0; x < 2; x++) {
        InitializeCriticalSection (&(pBarrier->Ib[x].CriticalSection));
        InitializeConditionVariable (&(pBarrier->Ib[x].ConditionVariable));
        pBarrier->Ib[x].SchedulerFlags = 0;
        pBarrier->Ib[x].ProcessFlags = 0;
        pBarrier->UniqueBefore = AquireUniqueNumber();
    }
    // todo: should be fetch from INI
    pBarrier->WaitIfAlone = WaitIfAlone;

    pBarrier->SchedulerMask = 0;
    pBarrier->ProcessMask = 0;
    strncpy (pBarrier->Name, Name, sizeof (pBarrier->Name));
    pBarrier->Name[sizeof (pBarrier->Name) - 1] = 0;
    pBarrier->State = BARRIER_STATE_AKTIVE;
}

static void DestroyBarrier (SCHED_BARRIER *pBarrier)
{
    int x;

    FreeUniqueNumber (pBarrier->UniqueBefore);
    for (x = 0; x < 2; x++) {
        DeleteCriticalSection (&(pBarrier->Ib[x].CriticalSection));
    }
    memset (pBarrier, 0, sizeof (SCHED_BARRIER));
}


int AddBarrier (char *par_BarrierName ,int par_WaitIfAlone)
{
    int x;

    if (strlen (par_BarrierName) > (sizeof (Barriers[0].Name) - 1)) {
        ThrowError (1, "barrier name \"%s\" to long", par_BarrierName);
        return -1;
    }
    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        if (Barriers[x].State != BARRIER_STATE_EMPTY) {
            if (!strcmp (par_BarrierName, Barriers[x].Name)) {
                LeaveCriticalSection (&GlobalBarrierCriticalSection);
                ThrowError (1, "barrier \"%s\" already exists", par_BarrierName);
                return -1;
            }
        }
    }
    for (x = 0; x < MAX_BARRIERS; x++) {
        if (Barriers[x].State == BARRIER_STATE_EMPTY) {
            InitBarrier (&Barriers[x], par_BarrierName, par_WaitIfAlone);
            strcpy (Barriers[x].Name, par_BarrierName);
            LeaveCriticalSection (&GlobalBarrierCriticalSection);
            return x;
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
    return -1;
}

int DeleteBarrier (char *par_BarrierName)
{
    int x;

    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        if (Barriers[x].State == BARRIER_STATE_AKTIVE) {
            if (!strcmp (par_BarrierName, Barriers[x].Name)) {
                if (Barriers[x].ProcesseOrSchedulerCount == 0) {
                    // Only mark as deleted
                    Barriers[x].State = BARRIER_STATE_MARKED_FOR_REMOVING;
                    DestroyBarrier (&Barriers[x]); // This can be deleted immediately there should no  keine connection
                    LeaveCriticalSection (&GlobalBarrierCriticalSection);
                    return 0;
                } else {
                    LeaveCriticalSection (&GlobalBarrierCriticalSection);
                    ThrowError (1, "cannot remove barrier \"%s\" because there are %i processes or/and schedulers  associated",
                           Barriers[x].ProcesseOrSchedulerCount);
                    return -1;
                }
            }
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
    ThrowError (1, "barrier \"%s\" already exists", par_BarrierName);
    return -1;
}


static int InitAndReadBarrierConfigFromIni (SCHED_BARRIER *pBarrier, int Index)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[INI_MAX_LINE_LENGTH];
    char *Name, *WaitIfAloneString;
    int WaitIfAloneFlag;
    int Fd = GetMainFileDescriptor();

    sprintf (Entry, "Barrier_%i", Index);
    if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", Line, sizeof (Line), Fd) <= 0) {
        return -1;
    }
    WaitIfAloneFlag = 0;
    if (StringCommaSeparate (Line, &Name, &WaitIfAloneString, NULL) >= 2) {
        if (!strcmpi (WaitIfAloneString, "WaitIfAlone")) {
            WaitIfAloneFlag = 1;
        }
    } else {
        Name = Line;
    }

    InitBarrier (pBarrier, Name, WaitIfAloneFlag);
    return 0;
}


void InitAllBarriers (void)
{
    int x;

#ifdef BARRIER_LOGGING_DEPTH
    InitializeCriticalSection (&BarriersLoggingCriticalSection);
#endif
    InitializeCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        if (InitAndReadBarrierConfigFromIni (&Barriers[x], x)) break;
    }
}


int AddNewBarrierToIni (const char *par_BarrierName)
{
    int b;
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char BarrierName[MAX_BARRIER_NAME_LENGTH];
    int Fd = GetMainFileDescriptor();

    for (b = 0; ; b++) {
        sprintf (Entry, "Barrier_%i", b);
        if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", BarrierName, sizeof (BarrierName), Fd) <= 0) {
            break;
        }
    }
    IniFileDataBaseWriteString ("SchedulerBarriersConfiguration", Entry, par_BarrierName, Fd);
    return 0;
}

int DeleteBarrierFromIni (const char *par_BarrierName)
{
    int b;
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char BarrierName[MAX_BARRIER_NAME_LENGTH];
    int Fd = GetMainFileDescriptor();

    for (b = 0; ; b++) {
        sprintf (Entry, "Barrier_%i", b);
        if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", BarrierName, sizeof (BarrierName), Fd) <= 0) {
            break;
        }
        if (!strcmp (par_BarrierName, BarrierName)) {
            IniFileDataBaseWriteString ("SchedulerBarriersConfiguration", Entry, NULL, Fd);
            // Now move all 1 up
            for (b++; ; b++) {
                sprintf (Entry, "Barrier_%i", b);
                if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", BarrierName, sizeof (BarrierName), Fd) <= 0) {
                    break;
                }
                // remove old entry
                IniFileDataBaseWriteString ("SchedulerBarriersConfiguration", Entry, NULL, Fd);
                sprintf (Entry, "Barrier_%i", b - 1);
                // add new entry
                IniFileDataBaseWriteString ("SchedulerBarriersConfiguration", Entry, BarrierName, Fd);
            }
            return 0;
        }
    }
    return -1;
}

static void AddProcessConnectionsToFromBarrierBefore (SCHED_BARRIER *pBarrier, struct TCB *pTcb)
{
    if (pBarrier->AddNewProcessMasksBefore != 0) {
        EnterCriticalSection (&GlobalBarrierCriticalSection);
        ADD_BARRIER_LOGGING (0, (int)(pBarrier - Barriers), BARRIER_LOGGING_ADD_BEFORE_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
        pBarrier->ProcessMask |= pBarrier->AddNewProcessMasksBefore;
        pBarrier->AddNewProcessMasksBefore = 0;
        LeaveCriticalSection (&GlobalBarrierCriticalSection);
    }
}

static void RemoveProcessConnectionsToFromBarrierBefore (SCHED_BARRIER *pBarrier, struct TCB *pTcb)
{
    if (pBarrier->RemoveProcessMasks != 0) {
        EnterCriticalSection (&GlobalBarrierCriticalSection);
        ADD_BARRIER_LOGGING (0, (int)(pBarrier - Barriers), BARRIER_LOGGING_REMOVE_BEFORE_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
        pBarrier->ProcessMask &= ~pBarrier->RemoveProcessMasks;
        pBarrier->RemoveProcessMasks = 0;
        LeaveCriticalSection (&GlobalBarrierCriticalSection);
    }
}

static void AddSchedulerConnectionsToFromBarrierBefore (SCHED_BARRIER *pBarrier, SCHEDULER_DATA *pSchedulerData)
{
    if (pBarrier->AddNewSchedulerMasks != 0) {
        EnterCriticalSection (&GlobalBarrierCriticalSection);
        ADD_BARRIER_LOGGING (0, (int)(pBarrier - Barriers), BARRIER_LOGGING_ADD_BEFORE_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);
        pBarrier->SchedulerMask |= pBarrier->AddNewSchedulerMasks;
        pBarrier->AddNewSchedulerMasks = 0;
        LeaveCriticalSection (&GlobalBarrierCriticalSection);
    }
}

static void RemoveSchedulerConnectionsToFromBarrierBefore (SCHED_BARRIER *pBarrier, SCHEDULER_DATA *pSchedulerData)
{
    if (pBarrier->RemoveSchedulerMasks != 0) {
        EnterCriticalSection (&GlobalBarrierCriticalSection);
        ADD_BARRIER_LOGGING (0, (int)(pBarrier - Barriers), BARRIER_LOGGING_REMOVE_BEFORE_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);
        pBarrier->SchedulerMask &= ~pBarrier->RemoveSchedulerMasks;
        pBarrier->RemoveSchedulerMasks = 0;
        LeaveCriticalSection (&GlobalBarrierCriticalSection);
    }
}


static void AddProcessConnectionsToFromBarrierBehind (SCHED_BARRIER *pBarrier, struct TCB *pTcb)
{
    if (pBarrier->AddNewProcessMasksBehind != 0) {
        EnterCriticalSection (&GlobalBarrierCriticalSection);
        ADD_BARRIER_LOGGING (0, (int)(pBarrier - Barriers), BARRIER_LOGGING_ADD_BEHIND_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
        pBarrier->ProcessMask |= pBarrier->AddNewProcessMasksBehind;
        pBarrier->AddNewProcessMasksBehind = 0;
        LeaveCriticalSection (&GlobalBarrierCriticalSection);
    }
}

static void RemoveProcessConnectionsToFromBarrierBehind (SCHED_BARRIER *pBarrier, struct TCB *pTcb)
{
    if (pBarrier->RemoveProcessMasks != 0) {
        EnterCriticalSection (&GlobalBarrierCriticalSection);
        ADD_BARRIER_LOGGING (0, (int)(pBarrier - Barriers), BARRIER_LOGGING_REMOVE_BEHIND_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
        pBarrier->ProcessMask &= ~pBarrier->RemoveProcessMasks;
        pBarrier->RemoveProcessMasks = 0;
        LeaveCriticalSection (&GlobalBarrierCriticalSection);
    }
}


static void AddSchedulerConnectionsToFromBarrierBehind (SCHED_BARRIER *pBarrier, SCHEDULER_DATA *pSchedulerData)
{
    if (pBarrier->AddNewSchedulerMasks != 0) {
        EnterCriticalSection (&GlobalBarrierCriticalSection);
        ADD_BARRIER_LOGGING (0, (int)(pBarrier - Barriers), BARRIER_LOGGING_ADD_BEHIND_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);
        pBarrier->SchedulerMask |= pBarrier->AddNewSchedulerMasks;
        pBarrier->AddNewSchedulerMasks = 0;
        LeaveCriticalSection (&GlobalBarrierCriticalSection);
    }
}

static void RemoveSchedulerConnectionsToFromBarrierBehind (SCHED_BARRIER *pBarrier, SCHEDULER_DATA *pSchedulerData)
{
    if (pBarrier->RemoveSchedulerMasks != 0) {
        EnterCriticalSection (&GlobalBarrierCriticalSection);
        ADD_BARRIER_LOGGING (0, (int)(pBarrier - Barriers), BARRIER_LOGGING_REMOVE_BEHIND_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);
        pBarrier->SchedulerMask &= ~pBarrier->RemoveSchedulerMasks;
        pBarrier->RemoveSchedulerMasks = 0;
        LeaveCriticalSection (&GlobalBarrierCriticalSection);
    }
}


void WalkThroughBarrierBeforeLoopOut (struct TCB *pTcb, char *par_BarrierName, int FirstOrSecond, int Disconnect)
{
    int WaitFlag;
    int x;
    SCHED_BARRIER *pBarrier;

    for (x = 0; x < pTcb->BarrierLoopOutBeforeCount; x++) {
        pBarrier = &(Barriers[pTcb->BarrierLoopOutBeforeNr[x]]);
        if ((par_BarrierName == NULL) || !strcmp (pBarrier->Name, par_BarrierName)) {
            EnterCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
            pBarrier->Switch = FirstOrSecond; // only for logging

            if (Disconnect) {
                pBarrier->RemoveProcessMasks |= pTcb->bb_access_mask;
            }

            if (FirstOrSecond == 0) {
                AddProcessConnectionsToFromBarrierBefore (pBarrier, pTcb);
            }

            pBarrier->Ib[FirstOrSecond].ProcessFlags = pBarrier->Ib[FirstOrSecond].ProcessFlags | (pTcb->bb_access_mask & pBarrier->ProcessMask);

            if (pBarrier->WaitIfAlone) {
                if ((pBarrier->Ib[FirstOrSecond].SchedulerFlags == 0) &&
                    (pBarrier->Ib[FirstOrSecond].ProcessFlags == pTcb->bb_access_mask)) {
                    goto SLEEP;
                }
            }

            if (((pBarrier->Ib[FirstOrSecond].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
                ((pBarrier->Ib[FirstOrSecond].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
                pBarrier->Ib[FirstOrSecond].ProcessFlags = 0;
                pBarrier->Ib[FirstOrSecond].SchedulerFlags = 0;
                ADD_BARRIER_LOGGING (1, pTcb->BarrierLoopOutBeforeNr[x], BARRIER_LOGGING_WAKE_ALL_BEFORE_LOOPOUT_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);

                RemoveProcessConnectionsToFromBarrierBefore (pBarrier, pTcb);

                WakeAllConditionVariable (&(pBarrier->Ib[FirstOrSecond].ConditionVariable));
                LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
            } else {
SLEEP:
                WaitFlag = pBarrier->WaitFlags[pTcb->PosInsideBarrierLoopOutBefore[x]];
                if (WaitFlag == BARRIER_SIGNAL_AND_WAIT) {
                    ADD_BARRIER_LOGGING (1, pTcb->BarrierLoopOutBeforeNr[x], BARRIER_LOGGING_GOTO_SLEEP_BEFORE_LOOPOUT_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                    // Condition variables are subject to spurious wakeups
                    while ((pBarrier->Ib[FirstOrSecond].ProcessFlags != 0) || (pBarrier->Ib[FirstOrSecond].SchedulerFlags != 0)) {
                        SleepConditionVariableCS_INFINITE(&(pBarrier->Ib[FirstOrSecond].ConditionVariable), &(pBarrier->Ib[FirstOrSecond].CriticalSection));
                    }
                } else {
                    ADD_BARRIER_LOGGING (1, pTcb->BarrierLoopOutBeforeNr[x], BARRIER_LOGGING_WALK_THROUGH_BEFORE_LOOPOUT_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                                    }
                LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
            }
        }
    }
}

void WalkThroughBarrierBehindLoopOut (struct TCB *pTcb, char *par_BarrierName, int FirstOrSecond, int Disconnect)
{
    int WaitFlag;
    int x;
    SCHED_BARRIER *pBarrier;

    for (x = 0; x < pTcb->BarrierLoopOutBehindCount; x++) {
        pBarrier = &(Barriers[pTcb->BarrierLoopOutBehindNr[x]]);
        if ((par_BarrierName == NULL) || !strcmp (pBarrier->Name, par_BarrierName)) {
            EnterCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
            pBarrier->Switch = FirstOrSecond; // only for logging

            if (Disconnect) {
                pBarrier->RemoveProcessMasks |= pTcb->bb_access_mask;
            }

            if (FirstOrSecond == 0) AddProcessConnectionsToFromBarrierBehind (pBarrier, pTcb);

            pBarrier->Ib[FirstOrSecond].ProcessFlags = pBarrier->Ib[FirstOrSecond].ProcessFlags | (pTcb->bb_access_mask & pBarrier->ProcessMask);

            if (pBarrier->WaitIfAlone) {
                if ((pBarrier->Ib[FirstOrSecond].SchedulerFlags == 0) &&
                    (pBarrier->Ib[FirstOrSecond].ProcessFlags == pTcb->bb_access_mask)) {
                    goto SLEEP;
                }
            }

            if (((pBarrier->Ib[FirstOrSecond].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
                ((pBarrier->Ib[FirstOrSecond].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
                pBarrier->Ib[FirstOrSecond].ProcessFlags = 0;
                pBarrier->Ib[FirstOrSecond].SchedulerFlags = 0;
                ADD_BARRIER_LOGGING (1, pTcb->BarrierLoopOutBehindNr[x], BARRIER_LOGGING_WAKE_ALL_BEHIND_LOOPOUT_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);

                RemoveProcessConnectionsToFromBarrierBehind (pBarrier, pTcb);

                WakeAllConditionVariable (&(pBarrier->Ib[FirstOrSecond].ConditionVariable));
                LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
            } else {
SLEEP:
                WaitFlag = pBarrier->WaitFlags[pTcb->PosInsideBarrierLoopOutBehind[x]];
                if (WaitFlag == BARRIER_SIGNAL_AND_WAIT) {
                    ADD_BARRIER_LOGGING (1, pTcb->BarrierLoopOutBehindNr[x], BARRIER_LOGGING_GOTO_SLEEP_BEHIND_LOOPOUT_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                    // Condition variables are subject to spurious wakeups
                    while ((pBarrier->Ib[FirstOrSecond].ProcessFlags != 0) || (pBarrier->Ib[FirstOrSecond].SchedulerFlags != 0)) {
                        SleepConditionVariableCS_INFINITE(&(pBarrier->Ib[FirstOrSecond].ConditionVariable), &(pBarrier->Ib[FirstOrSecond].CriticalSection));
                    }
                } else {
                    ADD_BARRIER_LOGGING (1, pTcb->BarrierLoopOutBehindNr[x], BARRIER_LOGGING_WALK_THROUGH_BEHIND_LOOPOUT_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                }
                LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
            }
        }
    }
}



void WalkThroughBarrierBeforeProcess (struct TCB *pTcb, int FirstOrSecond, int Disconnect)
{
    int WaitFlag;
    int x;
    SCHED_BARRIER *pBarrier;

    for (x = 0; x < pTcb->BarrierBeforeCount; x++) {
        pBarrier = &(Barriers[pTcb->BarrierBeforeNr[x]]);
        EnterCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        pBarrier->Switch = FirstOrSecond; // only for logging

        if (Disconnect) {
            pBarrier->RemoveProcessMasks |= pTcb->bb_access_mask;
        }

        if (FirstOrSecond == 0) AddProcessConnectionsToFromBarrierBefore (pBarrier, pTcb);

        pBarrier->Ib[FirstOrSecond].ProcessFlags = pBarrier->Ib[FirstOrSecond].ProcessFlags | (pTcb->bb_access_mask & pBarrier->ProcessMask);

        if (pBarrier->WaitIfAlone) {
            if ((pBarrier->Ib[FirstOrSecond].SchedulerFlags == 0) &&
                (pBarrier->Ib[FirstOrSecond].ProcessFlags == pTcb->bb_access_mask)) {
                goto SLEEP;
            }
        }

        if (((pBarrier->Ib[FirstOrSecond].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
            ((pBarrier->Ib[FirstOrSecond].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
            pBarrier->Ib[FirstOrSecond].ProcessFlags = 0;
            pBarrier->Ib[FirstOrSecond].SchedulerFlags = 0;
            ADD_BARRIER_LOGGING (1, pTcb->BarrierBeforeNr[x], BARRIER_LOGGING_WAKE_ALL_BEFORE_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);

            RemoveProcessConnectionsToFromBarrierBefore (pBarrier, pTcb);

            WakeAllConditionVariable (&(pBarrier->Ib[FirstOrSecond].ConditionVariable));
            LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        } else {
SLEEP:
            WaitFlag = pBarrier->WaitFlags[pTcb->PosInsideBarrierBefore[x]];
            if (WaitFlag == BARRIER_SIGNAL_AND_WAIT) {
                ADD_BARRIER_LOGGING (1, pTcb->BarrierBeforeNr[x], BARRIER_LOGGING_GOTO_SLEEP_BEFORE_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                // Condition variables are subject to spurious wakeups
                while ((pBarrier->Ib[FirstOrSecond].ProcessFlags != 0) || (pBarrier->Ib[FirstOrSecond].SchedulerFlags != 0)) {
                    SleepConditionVariableCS_INFINITE(&(pBarrier->Ib[FirstOrSecond].ConditionVariable), &(pBarrier->Ib[FirstOrSecond].CriticalSection));
                }
            } else {
                ADD_BARRIER_LOGGING (1, pTcb->BarrierBeforeNr[x], BARRIER_LOGGING_WALK_THROUGH_BEFORE_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
            }
            LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        }
    }
}


void WalkThroughBarrierBehindProcess (struct TCB *pTcb, int FirstOrSecond, int Disconnect)
{
    int WaitFlag;
    int x;
    SCHED_BARRIER *pBarrier;

    for (x = 0; x < pTcb->BarrierBehindCount; x++) {
        pBarrier = &(Barriers[pTcb->BarrierBehindNr[x]]);
        EnterCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        pBarrier->Switch = FirstOrSecond; // only for logging

        if (Disconnect) {
            pBarrier->RemoveProcessMasks |= pTcb->bb_access_mask;
        }

        if (FirstOrSecond == 0) AddProcessConnectionsToFromBarrierBehind (pBarrier, pTcb);

        pBarrier->Ib[FirstOrSecond].ProcessFlags = pBarrier->Ib[FirstOrSecond].ProcessFlags | (pTcb->bb_access_mask & pBarrier->ProcessMask);

        if (pBarrier->WaitIfAlone) {
            if ((pBarrier->Ib[FirstOrSecond].SchedulerFlags == 0) &&
                (pBarrier->Ib[FirstOrSecond].ProcessFlags == pTcb->bb_access_mask)) {
                goto SLEEP;
            }
        }

        if (((pBarrier->Ib[FirstOrSecond].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
            ((pBarrier->Ib[FirstOrSecond].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
            pBarrier->Ib[FirstOrSecond].ProcessFlags = 0;
            pBarrier->Ib[FirstOrSecond].SchedulerFlags = 0;
            ADD_BARRIER_LOGGING (1, pTcb->BarrierBehindNr[x], BARRIER_LOGGING_WAKE_ALL_BEHIND_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);

            RemoveProcessConnectionsToFromBarrierBehind (pBarrier, pTcb);

            WakeAllConditionVariable (&(pBarrier->Ib[FirstOrSecond].ConditionVariable));
            LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        } else {
SLEEP:
            WaitFlag = pBarrier->WaitFlags[pTcb->PosInsideBarrierBehind[x]];
            if (WaitFlag == BARRIER_SIGNAL_AND_WAIT) {
                ADD_BARRIER_LOGGING (1, pTcb->BarrierBehindNr[x], BARRIER_LOGGING_GOTO_SLEEP_BEHIND_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
                // Condition variables are subject to spurious wakeups
                while ((pBarrier->Ib[FirstOrSecond].ProcessFlags != 0) || (pBarrier->Ib[FirstOrSecond].SchedulerFlags != 0)) {
                    SleepConditionVariableCS_INFINITE(&(pBarrier->Ib[FirstOrSecond].ConditionVariable), &(pBarrier->Ib[FirstOrSecond].CriticalSection));
                }
            } else {
                ADD_BARRIER_LOGGING (1, pTcb->BarrierBehindNr[x], BARRIER_LOGGING_WALK_THROUGH_BEHIND_PROCESS_ACTION, BARRIER_LOGGING_PROCESS_TYPE, pTcb->pid, __LINE__);
            }
            LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        }
    }
}

void WalkThroughBarrierBeforeStartSchedulerCycle (SCHEDULER_DATA *pSchedulerData,
                                                 int FirstOrSecond, int Disconnect,
                                                 void (*Callback)(SCHEDULER_DATA *))
{
    int WaitFlag;
    int x;
    SCHED_BARRIER *pBarrier;

    for (x = 0; x < pSchedulerData->BarrierBeforeCount; x++) {
        pBarrier = &(Barriers[pSchedulerData->BarrierBeforeNr[x]]);
        EnterCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        pBarrier->Switch = FirstOrSecond; // only for logging

        if (Disconnect) {
            pBarrier->RemoveSchedulerMasks |= pSchedulerData->BarrierMask;
        }

        if (FirstOrSecond == 0) AddSchedulerConnectionsToFromBarrierBefore (pBarrier, pSchedulerData);

        pBarrier->Ib[FirstOrSecond].SchedulerFlags = pBarrier->Ib[FirstOrSecond].SchedulerFlags | (pSchedulerData->BarrierMask & pBarrier->SchedulerMask);

        if (pBarrier->WaitIfAlone) {
            if ((pBarrier->Ib[FirstOrSecond].SchedulerFlags == pSchedulerData->BarrierMask) &&
                (pBarrier->Ib[FirstOrSecond].ProcessFlags == 0)) {
                goto SLEEP;
            }
        }

        if (((pBarrier->Ib[FirstOrSecond].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
            ((pBarrier->Ib[FirstOrSecond].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
            pBarrier->Ib[FirstOrSecond].ProcessFlags = 0;
            pBarrier->Ib[FirstOrSecond].SchedulerFlags = 0;
            ADD_BARRIER_LOGGING (1, pSchedulerData->BarrierBeforeNr[x], BARRIER_LOGGING_WAKE_ALL_BEFORE_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);
            WakeAllConditionVariable (&(pBarrier->Ib[FirstOrSecond].ConditionVariable));
            LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        } else {
        SLEEP:
            WaitFlag = pBarrier->WaitFlags[pSchedulerData->PosInsideBarrierBefore[x]];
            if (WaitFlag == BARRIER_SIGNAL_AND_WAIT) {
                ADD_BARRIER_LOGGING (1, pSchedulerData->BarrierBeforeNr[x], BARRIER_LOGGING_GOTO_SLEEP_BEFORE_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);

                RemoveSchedulerConnectionsToFromBarrierBefore (pBarrier, pSchedulerData);

                // Condition variables are subject to spurious wakeups or if a new process logged in
                if ((pBarrier->Ib[FirstOrSecond].ProcessFlags != 0) || (pBarrier->Ib[FirstOrSecond].SchedulerFlags != 0)) {
                    while(1) {
                        SleepConditionVariableCS_INFINITE(&(pBarrier->Ib[FirstOrSecond].ConditionVariable), &(pBarrier->Ib[FirstOrSecond].CriticalSection));
                        if ((pBarrier->Ib[FirstOrSecond].ProcessFlags == 0) && (pBarrier->Ib[FirstOrSecond].SchedulerFlags == 0)) {
                            break;
                        }
                        if (FirstOrSecond == 0) {
                            if (Callback != NULL) {
                                Callback(pSchedulerData);
                            }
                        }
                    }
                }
            } else {
                ADD_BARRIER_LOGGING (1, pSchedulerData->BarrierBeforeNr[x], BARRIER_LOGGING_WALK_THROUGH_BEFORE_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);
            }
            LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        }
    }
}

void WalkThroughBarrierBehindEndOfSchedulerCycle (SCHEDULER_DATA *pSchedulerData,
                                                 int FirstOrSecond, int Disconnect,
                                                 void (*Callback)(SCHEDULER_DATA *))
{
    int WaitFlag;
    int x;
    SCHED_BARRIER *pBarrier;

    for (x = 0; x < pSchedulerData->BarrierBehindCount; x++) {
        pBarrier = &(Barriers[pSchedulerData->BarrierBehindNr[x]]);
        EnterCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        pBarrier->Switch = FirstOrSecond; // only for logging

        if (Disconnect) {
            pBarrier->RemoveSchedulerMasks |= pSchedulerData->BarrierMask;
        }

        if (FirstOrSecond == 0) AddSchedulerConnectionsToFromBarrierBehind (pBarrier, pSchedulerData);

        pBarrier->Ib[FirstOrSecond].SchedulerFlags = pBarrier->Ib[FirstOrSecond].SchedulerFlags | (pSchedulerData->BarrierMask & pBarrier->SchedulerMask);

        if (pBarrier->WaitIfAlone) {
            if ((pBarrier->Ib[FirstOrSecond].SchedulerFlags == pSchedulerData->BarrierMask) &&
                (pBarrier->Ib[FirstOrSecond].ProcessFlags == 0)) {
                goto SLEEP;
            }
        }

        if (((pBarrier->Ib[FirstOrSecond].ProcessFlags & pBarrier->ProcessMask) == pBarrier->ProcessMask) &&
            ((pBarrier->Ib[FirstOrSecond].SchedulerFlags & pBarrier->SchedulerMask) == pBarrier->SchedulerMask)) {
            pBarrier->Ib[FirstOrSecond].ProcessFlags = 0;
            pBarrier->Ib[FirstOrSecond].SchedulerFlags = 0;
            ADD_BARRIER_LOGGING (1, pSchedulerData->BarrierBehindNr[x], BARRIER_LOGGING_WAKE_ALL_BEHIND_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);

            RemoveSchedulerConnectionsToFromBarrierBehind (pBarrier, pSchedulerData);

            WakeAllConditionVariable (&(pBarrier->Ib[FirstOrSecond].ConditionVariable));
            LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        } else {
        SLEEP:
            WaitFlag = pBarrier->WaitFlags[pSchedulerData->PosInsideBarrierBehind[x]];
            if (WaitFlag == BARRIER_SIGNAL_AND_WAIT) {
                ADD_BARRIER_LOGGING (1, pSchedulerData->BarrierBehindNr[x], BARRIER_LOGGING_GOTO_SLEEP_BEHIND_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);
                // Condition variables are subject to spurious wakeups
                if ((pBarrier->Ib[FirstOrSecond].ProcessFlags != 0) || (pBarrier->Ib[FirstOrSecond].SchedulerFlags != 0)) {
                    while(1) {
                        SleepConditionVariableCS_INFINITE(&(pBarrier->Ib[FirstOrSecond].ConditionVariable), &(pBarrier->Ib[FirstOrSecond].CriticalSection));
                        if ((pBarrier->Ib[FirstOrSecond].ProcessFlags == 0) || (pBarrier->Ib[FirstOrSecond].SchedulerFlags == 0)) {
                            break;
                        }
                        if (FirstOrSecond == 0) {
                            if (Callback != NULL) {
                                Callback(pSchedulerData);
                            }
                        }
                    }
                }
            } else {
                ADD_BARRIER_LOGGING (1, pSchedulerData->BarrierBehindNr[x], BARRIER_LOGGING_WALK_THROUGH_BEHIND_SCHEDULER_ACTION, BARRIER_LOGGING_SCHEDULER_TYPE, GetSchedulerIndex(pSchedulerData), __LINE__);
            }
            LeaveCriticalSection (&(pBarrier->Ib[FirstOrSecond].CriticalSection));
        }
    }
}


void ConnectProcessToAllAssociatedBarriers (TASK_CONTROL_BLOCK  *pTcb,
                                            char *BarriersBeforeOnlySignal,
                                            char *BarriersBeforeSignalAndWait,
                                            char *BarriersBehindOnlySignal,
                                            char *BarriersBehindSignalAndWait,
                                            char *BarriersLoopOutBeforeOnlySignal,
                                            char *BarriersLoopOutBeforeSignalAndWait,
                                            char *BarriersLoopOutBehindOnlySignal,
                                            char *BarriersLoopOutBehindSignalAndWait)
{
    char *p, *pp;
    int BarrierNr;
    int End;

    pp = p = BarriersBeforeOnlySignal;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (pTcb->BarrierBeforeCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                               pTcb->name, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectProcessToBarrier (pTcb, pp, BARRIER_ONLY_SIGNAL);
                    if (BarrierNr >= 0) {
                        pTcb->BarrierBeforeNr[pTcb->BarrierBeforeCount] = BarrierNr;
                        pTcb->BarrierBeforeCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersBeforeSignalAndWait;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (pTcb->BarrierBeforeCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                               pTcb->name, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectProcessToBarrier (pTcb, pp, BARRIER_SIGNAL_AND_WAIT);
                    if (BarrierNr >= 0) {
                        pTcb->BarrierBeforeNr[pTcb->BarrierBeforeCount] = BarrierNr;
                        pTcb->BarrierBeforeCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersBehindOnlySignal;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (pTcb->BarrierBehindCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                               pTcb->name, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectProcessToBarrier (pTcb, pp, BARRIER_ONLY_SIGNAL);
                    if (BarrierNr >= 0) {
                        pTcb->BarrierBehindNr[pTcb->BarrierBehindCount] = BarrierNr;
                        pTcb->BarrierBehindCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersBehindSignalAndWait;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (pTcb->BarrierBehindCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                               pTcb->name, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectProcessToBarrier (pTcb, pp, BARRIER_SIGNAL_AND_WAIT);
                    if (BarrierNr >= 0) {
                        pTcb->BarrierBehindNr[pTcb->BarrierBehindCount] = BarrierNr;
                        pTcb->BarrierBehindCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
// Loop out
    pp = p = BarriersLoopOutBeforeOnlySignal;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (pTcb->BarrierLoopOutBeforeCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                               pTcb->name, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectProcessToBarrier (pTcb, pp, BARRIER_ONLY_SIGNAL);
                    if (BarrierNr >= 0) {
                        pTcb->BarrierLoopOutBeforeNr[pTcb->BarrierLoopOutBeforeCount] = BarrierNr;
                        pTcb->BarrierLoopOutBeforeCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersLoopOutBeforeSignalAndWait;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (pTcb->BarrierLoopOutBeforeCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                               pTcb->name, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectProcessToBarrier (pTcb, pp, BARRIER_SIGNAL_AND_WAIT);
                    if (BarrierNr >= 0) {
                        pTcb->BarrierLoopOutBeforeNr[pTcb->BarrierLoopOutBeforeCount] = BarrierNr;
                        pTcb->BarrierLoopOutBeforeCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersLoopOutBehindOnlySignal;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (pTcb->BarrierLoopOutBehindCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                               pTcb->name, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectProcessToBarrier (pTcb, pp, BARRIER_ONLY_SIGNAL);
                    if (BarrierNr >= 0) {
                        pTcb->BarrierLoopOutBehindNr[pTcb->BarrierLoopOutBehindCount] = BarrierNr;
                        pTcb->BarrierLoopOutBehindCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersLoopOutBehindSignalAndWait;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (pTcb->BarrierLoopOutBehindCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate process \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                               pTcb->name, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectProcessToBarrier (pTcb, pp, BARRIER_SIGNAL_AND_WAIT);
                    if (BarrierNr >= 0) {
                        pTcb->BarrierLoopOutBehindNr[pTcb->BarrierLoopOutBehindCount] = BarrierNr;
                        pTcb->BarrierLoopOutBehindCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
}

void ConnectSchedulerToAllAssociatedBarriers (SCHEDULER_DATA *Scheduler,
                                              char *BarriersBeforeOnlySignal,
                                              char *BarriersBeforeSignalAndWait,
                                              char *BarriersBehindOnlySignal,
                                              char *BarriersBehindSignalAndWait)
{
    char *p, *pp;
    int BarrierNr;
    int End;

    pp = p = BarriersBeforeOnlySignal;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (Scheduler->BarrierBeforeCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate scheduler \"%s\" to barrier \"%s\" max. %i barriers per scheduler allowed",
                               Scheduler->SchedulerName, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectSchedulerToBarrier (Scheduler, pp, BARRIER_ONLY_SIGNAL);
                    if (BarrierNr >= 0) {
                        Scheduler->BarrierBeforeNr[Scheduler->BarrierBeforeCount] = BarrierNr;
                        Scheduler->BarrierBeforeCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersBeforeSignalAndWait;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (Scheduler->BarrierBeforeCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate scheduler \"%s\" to barrier \"%s\" max. %i barriers per scheduler allowed",
                               Scheduler->SchedulerName, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectSchedulerToBarrier (Scheduler, pp, BARRIER_SIGNAL_AND_WAIT);
                    if (BarrierNr >= 0) {
                        Scheduler->BarrierBeforeNr[Scheduler->BarrierBeforeCount] = BarrierNr;
                        Scheduler->BarrierBeforeCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersBehindOnlySignal;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (Scheduler->BarrierBehindCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate scheduler \"%s\" to barrier \"%s\" max. %i barriers per scheduler allowed",
                               Scheduler->SchedulerName, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectSchedulerToBarrier (Scheduler, pp, BARRIER_ONLY_SIGNAL);
                    if (BarrierNr >= 0) {
                        Scheduler->BarrierBehindNr[Scheduler->BarrierBehindCount] = BarrierNr;
                        Scheduler->BarrierBehindCount++;
                    }
                    pp = p;
                }
            }
            p++;
        } while (!End);
    }
    pp = p = BarriersBehindSignalAndWait;
    if ((p != NULL) && (*p != 0)) {
        End = 0;
        do {
            if ((*p == 0) || (*p == ',')) {
                if (*p == 0) {
                    End = 1;
                } else {
                    p = 0;
                    p++;
                }
                if (strlen (pp)) {
                    if (Scheduler->BarrierBehindCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                        ThrowError (1, "cannot associate scheduler \"%s\" to barrier \"%s\" max. %i barriers per scheduler allowed",
                               Scheduler->SchedulerName, pp, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    }
                    BarrierNr = ConnectSchedulerToBarrier (Scheduler, pp, BARRIER_SIGNAL_AND_WAIT);
                    if (BarrierNr >= 0) {
                        Scheduler->BarrierBehindNr[Scheduler->BarrierBehindCount] = BarrierNr;
                        Scheduler->BarrierBehindCount++;
                    }
                }
                pp = p;
            }
            p++;
        } while (!End);
    }
}


int AddProcessOrSchdulerToBarrierIniConfig (char *par_Name, int par_ProcessOrSchedulerFlag, int par_WaitFlag, int par_BeforeOrBehindFlag, char *par_Barrier)
{
    int b;
    char Line[INI_MAX_LINE_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char BarrierName[MAX_BARRIER_NAME_LENGTH];
    char *p, *pp;
    int BarrierCount;
    int AlreadAssociated;
    char *Type, *BeforeBehind, *Wait;
    int Fd = GetMainFileDescriptor();

    for (b = 0; ; b++) {
        sprintf (Entry, "Barrier_%i", b);
        if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", BarrierName, sizeof (BarrierName), Fd) <= 0) {
            break;
        }
        if (!strcmp (par_Barrier, BarrierName)) {
            if (par_ProcessOrSchedulerFlag) {
                Type = "Process";
            } else {
                Type = "Scheduler";
            }
            if (par_BeforeOrBehindFlag) {
                BeforeBehind = "Before";
            } else {
                BeforeBehind = "Behind";
            }
            if (par_WaitFlag) {
                Wait = "SignalAndWait";
            } else {
                Wait = "OnlySignal";
            }
            sprintf (Entry, "Barriers%s%sFor%s %s", BeforeBehind, Wait, Type, par_Name);
            IniFileDataBaseReadString (SCHED_INI_SECTION, Entry, "", Line, INI_MAX_LINE_LENGTH, Fd);

            BarrierCount = 0;
            AlreadAssociated = 0;
            pp = p = Line;
            if ((p != NULL) && (*p != 0)) {
                do {
                    if ((*p == 0) || (*p == ',')) {
                        if (*p == ';') {
                            p = 0;
                            p++;
                        }
                        if (strlen (pp)) {
                            if (strcmp (pp, par_Barrier)) {
                                ThrowError (1, "cannot associate %s \"%s\" to barrier \"%s\" (process is already associated",
                                       Type, par_Name, par_Barrier);
                                AlreadAssociated = 1;
                            }
                            BarrierCount++;
                        }
                    }
                    p++;
                } while (*p != 0);
            }
            if (!AlreadAssociated) {
                if (BarrierCount >= MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS) {
                    ThrowError (1, "cannot associate %s \"%s\" to barrier \"%s\" max. %i barriers per process allowed",
                           Type, par_Name, par_Barrier, MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS);
                    return -1;
                } else {
                    // Read the line again because it was destroyed
                    IniFileDataBaseReadString (SCHED_INI_SECTION, Entry, "", Line, INI_MAX_LINE_LENGTH, Fd);
                    strcat (Line, ";");
                    strcat (Line, par_Name);
                    IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, Line, Fd);
                    return 0;
                }
            }
        }
    }
    ThrowError (1, "try to add a %s \"%s\" from unknown barrier \"%s\"",
           (par_ProcessOrSchedulerFlag) ? "Process" : "Scheduler", par_Name, par_Barrier);
    return -1;
}


int RemoveProcessOrSchedulerFromBarrierIniConfig (char *par_Name, int par_ProcessOrSchedulerFlag, int par_WaitFlag, int par_BeforeOrBehindFlag, char *par_Barrier)
{
    int b;
    char Line[INI_MAX_LINE_LENGTH];
    char NewLine[INI_MAX_LINE_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char BarrierName[MAX_BARRIER_NAME_LENGTH];
    char *p, *pp;
    int BarrierCount;
    int AlreadAssociated;
    char *Type, *BeforeBehind, *Wait;
    int Fd = GetMainFileDescriptor();

    NewLine[0] = 0;
    for (b = 0; ; b++) {
        sprintf (Entry, "Barrier_%i", b);
        if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", BarrierName, sizeof (BarrierName), Fd) <= 0) {
            break;
        }
        if (!strcmp (par_Barrier, BarrierName)) {
            if (par_ProcessOrSchedulerFlag) {
                Type = "Process";
            } else {
                Type = "Scheduler";
            }
            if (par_BeforeOrBehindFlag) {
                BeforeBehind = "Before";
            } else {
                BeforeBehind = "Behind";
            }
            if (par_WaitFlag) {
                Wait = "SignalAndWait";
            } else {
                Wait = "OnlySignal";
            }
            sprintf (Entry, "Barriers%s%sFor%s %s", BeforeBehind, Wait, Type, par_Name);
            IniFileDataBaseReadString (SCHED_INI_SECTION, Entry, "", Line, INI_MAX_LINE_LENGTH, Fd);

            BarrierCount = 0;
            AlreadAssociated = 0;
            pp = p = Line;
            if ((p != NULL) && (*p != 0)) {
                do {
                    if ((*p == 0) || (*p == ',')) {
                        if (*p == ';') {
                            p = 0;
                            p++;
                        }
                        if (strlen (pp)) {
                            if (strcmp (pp, par_Barrier)) {  // cutout the barrier
                            } else {
                                if (BarrierCount) strcat (NewLine, ";");
                                strcat (NewLine, pp);
                                BarrierCount++;
                            }
                        }
                    }
                    p++;
                } while (*p != 0);
            }
            if (BarrierCount) {
                IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, NewLine, Fd);
            } else {
                IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, NULL, Fd);
            }
        }
    }
    ThrowError (1, "try to delet a %s \"%s\" from unknown barrier \"%s\"",
           (par_ProcessOrSchedulerFlag) ? "Process" : "Scheduler", par_Name, par_Barrier);
    return -1;
}

void RemoveProcessOrSchedulerFromAllBarrierIniConfig (const char *par_Name, int par_ProcessOrSchedulerFlag)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char *Type;
    int Fd = GetMainFileDescriptor();

    if (par_ProcessOrSchedulerFlag) {
        Type = "Process";
    } else {
        Type = "Scheduler";
    }
    sprintf (Entry, "BarriersBeforeOnlySignalFor%s %s", Type, par_Name);
    IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, NULL, Fd);
    sprintf (Entry, "BarriersBeforeSignalAndWaitFor%s %s", Type, par_Name);
    IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, NULL, Fd);
    sprintf (Entry, "BarriersBehindOnlySignalFor%s %s", Type, par_Name);
    IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, NULL, Fd);
    sprintf (Entry, "BarriersBehindSignalAndWaitFor%s %s", Type, par_Name);
    IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, NULL, Fd);
}

char *GetAllBarrierString (void)
{
    char *Ret;
    int x;
    int Len = 1;
    int Pos = 0;

    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        if (Barriers[x].State == BARRIER_STATE_AKTIVE) {
            Len += (int)strlen (Barriers[x].Name) + 1;
        }
    }
    Ret = my_malloc (Len);
    if (Ret == NULL) return NULL;
    Ret[0] = 0;
    for (x = 0; x < MAX_BARRIERS; x++) {
        if (Barriers[x].State == BARRIER_STATE_AKTIVE) {
            int BarrierLen = (int)strlen (Barriers[x].Name) + 1;
            if (Pos > 0) {
                Ret[Pos] = ';';
                Pos++;
            }
            MEMCPY (Ret + Pos, Barriers[x].Name, (size_t)BarrierLen);
            Pos += BarrierLen - 1;
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
    return Ret;
}

void FreeAllBarrierString (char *String)
{
    my_free (String);
}

int GetBarrierState (const char *par_BarrierName, const char *par_SchedulerOrProcessName)
{
    int x, p;
    int Ret = -1;

    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        SCHED_BARRIER *b = &(Barriers[x]);
        if (b->State == BARRIER_STATE_AKTIVE) {
            if (!strcmp (b->Name, par_BarrierName)) {
                for (p = 0; p < b->ProcesseOrSchedulerCount; p++) {
                    switch (b->Types[p]) {
                    case BARRIER_FOR_SCHEDULER:
                        if (!strcmp (b->Ref[p].Schedulers->SchedulerName, par_SchedulerOrProcessName)) {
                            if (b->Ib[b->Switch].SchedulerFlags & b->Ref[p].Schedulers->BarrierMask) {
                                Ret = 1;
                            } else {
                                Ret = 0;
                            }
                            goto __OUT;
                        }
                        break;
                    case BARRIER_FOR_PROCESS:
                        if (!Compare2ProcessNames(b->Ref[p].Tcb->name, par_SchedulerOrProcessName)) {
                            if (b->Ib[b->Switch].ProcessFlags & b->Ref[p].Tcb->bb_access_mask) {
                                Ret = 1;
                            } else {
                                Ret = 0;
                            }
                            goto __OUT;
                        }
                        break;
                    }
                }
            }
        }
    }
__OUT:
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
    return Ret;
}


int GetBarierInfos (int par_BarrierNr,
                    int *ret_State, unsigned int *ret_SchedulerMask, unsigned int *ret_ProcessMask, int *ret_Switch,
                    unsigned int *ret_SchedulerFlags_0, unsigned int *ret_ProcessFlags_0,
                    unsigned int *ret_SchedulerFlags_1, unsigned int *ret_ProcessFlags_1,
                    int *ret_ProcesseOrSchedulerCount,
                    int *ret_WaitOnCounter,
                    unsigned int *ret_AddNewProcessMasksBefore,
                    unsigned int *ret_AddNewProcessMasksBehind,
                    unsigned int *ret_AddNewSchedulerMasks,
                    unsigned int *ret_RemoveProcessMasks,
                    unsigned int *ret_RemoveSchedulerMasks,
                    unsigned int *ret_BarriersLoggingCounter)
{
    if ((par_BarrierNr >= 0) && (par_BarrierNr < MAX_BARRIERS)) {
        SCHED_BARRIER *b = &(Barriers[par_BarrierNr]);
        *ret_State = b->State;
        *ret_SchedulerMask = b->SchedulerMask;
        *ret_ProcessMask = b->ProcessMask;
        *ret_Switch = b->Switch;
        *ret_SchedulerFlags_0 = b->Ib[0].SchedulerFlags;
        *ret_ProcessFlags_0 = b->Ib[0].ProcessFlags;
        *ret_SchedulerFlags_1 = b->Ib[1].SchedulerFlags;
        *ret_ProcessFlags_1 = b->Ib[1].ProcessFlags;
        *ret_ProcesseOrSchedulerCount = b->ProcesseOrSchedulerCount;
        *ret_WaitOnCounter = b->WaitOnCounter;
        *ret_AddNewProcessMasksBefore = b->AddNewProcessMasksBefore;
        *ret_AddNewProcessMasksBehind = b->AddNewProcessMasksBehind;
        *ret_AddNewSchedulerMasks = b->AddNewSchedulerMasks;
        *ret_RemoveProcessMasks = b->RemoveProcessMasks;
        *ret_RemoveSchedulerMasks = b->RemoveSchedulerMasks;
#ifdef BARRIER_LOGGING_DEPTH
        *ret_BarriersLoggingCounter = BarriersLoggingCounter;
#else
        *ret_BarriersLoggingCounter = 0;
#endif
        return 0;
    }
    return -1;
}

#if 0
void WriteBarrierLoggingToFile (void)
{
    FILE *fh;
    char BarrierName[MAX_PATH];
    char ProcOrSchedName[MAX_PATH];
    int BeforeOrBehind;
    int ProcOrSched;
    int LineNr;
    int SleepFlag;
    int State;
    int FirstOrSecond;
    unsigned int Mask;
    unsigned int Flags;
    int Switch;
    unsigned int BarriersLoggingCounter;

    unsigned int AddNewProcessMasksBefore;
    unsigned int AddNewProcessMasksBehind;
    unsigned int AddNewSchedulerMasks;
    unsigned int RemoveProcessMasks;
    unsigned int RemoveSchedulerMasks;

    char *SleepFlagString;
    int Index;

    fh = fopen ("c:\\tmp\\barier_history_log.txt", "wt");
    if (fh == NULL) return;
    fprintf (fh, "Barrier\t"
                 "X\t"
                 "Process/Scheduler name\t"
                 "Line\t"
                 "Sleep flag\t"
                 "State\t"
                 "FirstOrSecond\t"
                 "Mask\t"
                 "Flags\t"
                 "Switch\t"
                 "Count\n");
    Index = -1;
    while ((Index = GetNextBarrierLoggingEntry (Index, BarrierName, &BeforeOrBehind,
                                                &ProcOrSched, ProcOrSchedName, &LineNr, &SleepFlag, &State, &FirstOrSecond, &Mask, &Flags, &Switch, &BarriersLoggingCounter,
                                                &AddNewProcessMasksBefore,
                                                &AddNewProcessMasksBehind,
                                                &AddNewSchedulerMasks,
                                                &RemoveProcessMasks,
                                                &RemoveSchedulerMasks)) >= 0) {
        fprintf (fh, "%s:\t", BarrierName);
        fprintf (fh, "%i\t", BeforeOrBehind);
        fprintf (fh, "%s\t", ProcOrSchedName);
        fprintf (fh, "%i\t", LineNr);
        switch (SleepFlag) {
        case 0:
            SleepFlagString = "no sleep";
            break;
        case 1:
            SleepFlagString = "sleep";
            break;
        case 2:
            SleepFlagString = "wake all";
            break;
        case 3:
            SleepFlagString = "add";
            break;
        case 4:
            SleepFlagString = "remove last signal";
            break;
        case 5:
            SleepFlagString = "remove";
            break;
        }
        fprintf (fh, "%s\t", SleepFlagString);
        fprintf (fh, "%i\t", State);
        fprintf (fh, "%i\t", FirstOrSecond);
        fprintf (fh, "%X\t", Mask);
        fprintf (fh, "%X\t", Flags);
        fprintf (fh, "%i\t", Switch);
        fprintf (fh, "%i\n", BarriersLoggingCounter);
    }
    fclose (fh);
}

void WriteBarrierStateToFile (void)
{
    FILE *fh;
    char *AllBarrierString;

    int State;
    unsigned int SchedulerMask;
    unsigned int ProcessMask;
    int Switch;
    unsigned int SchedulerFlags_0;
    unsigned int ProcessFlags_0;
    unsigned int SchedulerFlags_1;
    unsigned int ProcessFlags_1;
    int ProcesseOrSchedulerCount;
    int WaitOnCounter;
    unsigned int AddNewProcessMasksBefore;
    unsigned int AddNewProcessMasksBehind;
    unsigned int AddNewSchedulerMasks;
    unsigned int RemoveProcessMasks;
    unsigned int RemoveSchedulerMasks;
    unsigned int BarriersLoggingCounter;
    int x;

    fh = fopen ("c:\\tmp\\barier_state.txt", "wt");
    if (fh == NULL) return;

    AllBarrierString = GetAllBarrierString();
    fprintf (fh, "%s\n", AllBarrierString);
    my_free (AllBarrierString);
    for (x = 0; ; x++) {
        if (GetBarierInfos (x,
                            &State, &SchedulerMask, &ProcessMask, &Switch,
                            &SchedulerFlags_0, &ProcessFlags_0,
                            &SchedulerFlags_1, &ProcessFlags_1,
                            &ProcesseOrSchedulerCount,
                            &WaitOnCounter,
                            &AddNewProcessMasksBefore,
                            &AddNewProcessMasksBehind,
                            &AddNewSchedulerMasks,
                            &RemoveProcessMasks,
                            &RemoveSchedulerMasks,
                            &BarriersLoggingCounter) == 0) {
            fprintf (fh, "State = %i\n", State);
            fprintf (fh, "SchedulerMask = %X\n", SchedulerMask);
            fprintf (fh, "ProcessMask = %X\n", ProcessMask);
            fprintf (fh, "Switch = %i\n", Switch);
            fprintf (fh, "SchedulerFlags_0 = %X\n", SchedulerFlags_0);
            fprintf (fh, "ProcessFlags_0 = %X\n", ProcessFlags_0);
            fprintf (fh, "SchedulerFlags_1 = %X\n", SchedulerFlags_1);
            fprintf (fh, "ProcessFlags_1 = %X\n", ProcessFlags_1);
            fprintf (fh, "ProcesseOrSchedulerCount = %i\n", ProcesseOrSchedulerCount);
            fprintf (fh, "WaitOnCounter = %i\n", WaitOnCounter);
            fprintf (fh, "AddNewProcessMasksBefore = %X\n", AddNewProcessMasksBefore);
            fprintf (fh, "AddNewProcessMasksBehind = %X\n", AddNewProcessMasksBehind);
            fprintf (fh, "AddNewSchedulerMasks = %X\n", AddNewSchedulerMasks);
            fprintf (fh, "RemoveProcessMasks = %X\n", RemoveProcessMasks);
            fprintf (fh, "RemoveSchedulerMasks = %X\n", RemoveSchedulerMasks);
            fprintf (fh, "BarriersLoggingCounter = %i\n\n", BarriersLoggingCounter);
        } else {
            break;
        }
    }
    fclose (fh);
}
#endif


void RemoveWaitIfAloneForAllBarriers (void)
{
    int x;
    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        SCHED_BARRIER *pBarrier = &(Barriers[x]);
        if (pBarrier->State == BARRIER_STATE_AKTIVE) {
            pBarrier->WaitIfAlone = 0;
            if (pBarrier->ProcesseOrSchedulerCount) {
                LeaveCriticalSection (&GlobalBarrierCriticalSection);
                pBarrier->Ib[0].ProcessFlags = 0;
                pBarrier->Ib[0].SchedulerFlags = 0;
                WakeAllConditionVariable (&(pBarrier->Ib[0].ConditionVariable));
                pBarrier->Ib[1].ProcessFlags = 0;
                pBarrier->Ib[1].SchedulerFlags = 0;
                WakeAllConditionVariable (&(pBarrier->Ib[1].ConditionVariable));
                EnterCriticalSection (&GlobalBarrierCriticalSection);
            }
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
}

void WakeupAllBarriersForProcessLogin (void)
{
    int x;
    EnterCriticalSection (&GlobalBarrierCriticalSection);
    for (x = 0; x < MAX_BARRIERS; x++) {
        SCHED_BARRIER *pBarrier = &(Barriers[x]);
        if (pBarrier->State == BARRIER_STATE_AKTIVE) {
            if (pBarrier->ProcesseOrSchedulerCount) {
                LeaveCriticalSection (&GlobalBarrierCriticalSection);
                WakeAllConditionVariable (&(pBarrier->Ib[0].ConditionVariable));
                WakeAllConditionVariable (&(pBarrier->Ib[1].ConditionVariable));
                EnterCriticalSection (&GlobalBarrierCriticalSection);
            }
        }
    }
    LeaveCriticalSection (&GlobalBarrierCriticalSection);
}

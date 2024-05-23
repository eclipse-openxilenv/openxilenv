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


#ifndef SCHEDBARRIER_H
#define SCHEDBARRIER_H

#include "tcb.h"
#include "Scheduler.h"

#define MAX_BARRIER_NAME_LENGTH   64
#define MAX_BARRIERS_ASSIGNED_TO_ONE_PROCESS 8

#define BARRIER_SIGNAL_AND_WAIT      3
#define BARRIER_ONLY_SIGNAL          1


typedef enum {BARRIER_LOGGING_UNDEF_ACTION                          = 0,
              BARRIER_LOGGING_EMERGENCY_CLEANUP_ACTION              = 1,

              BARRIER_LOGGING_ADD_BEFORE_PROCESS_ACTION             = 1101,
              BARRIER_LOGGING_REMOVE_BEFORE_PROCESS_ACTION          = 1102,
              BARRIER_LOGGING_GOTO_SLEEP_BEFORE_PROCESS_ACTION      = 1103,
              BARRIER_LOGGING_WAKE_ALL_BEFORE_PROCESS_ACTION        = 1104,
              BARRIER_LOGGING_WALK_THROUGH_BEFORE_PROCESS_ACTION    = 1105,

              BARRIER_LOGGING_ADD_BEHIND_PROCESS_ACTION             = 1201,
              BARRIER_LOGGING_REMOVE_BEHIND_PROCESS_ACTION          = 1202,
              BARRIER_LOGGING_GOTO_SLEEP_BEHIND_PROCESS_ACTION      = 1203,
              BARRIER_LOGGING_WAKE_ALL_BEHIND_PROCESS_ACTION        = 1204,
              BARRIER_LOGGING_WALK_THROUGH_BEHIND_PROCESS_ACTION    = 1205,

              BARRIER_LOGGING_ADD_BEFORE_SCHEDULER_ACTION           = 2101,
              BARRIER_LOGGING_REMOVE_BEFORE_SCHEDULER_ACTION        = 2102,
              BARRIER_LOGGING_GOTO_SLEEP_BEFORE_SCHEDULER_ACTION    = 2103,
              BARRIER_LOGGING_WAKE_ALL_BEFORE_SCHEDULER_ACTION      = 2104,
              BARRIER_LOGGING_WALK_THROUGH_BEFORE_SCHEDULER_ACTION  = 2105,

              BARRIER_LOGGING_ADD_BEHIND_SCHEDULER_ACTION           = 2201,
              BARRIER_LOGGING_REMOVE_BEHIND_SCHEDULER_ACTION        = 2202,
              BARRIER_LOGGING_GOTO_SLEEP_BEHIND_SCHEDULER_ACTION    = 2203,
              BARRIER_LOGGING_WAKE_ALL_BEHIND_SCHEDULER_ACTION      = 2204,
              BARRIER_LOGGING_WALK_THROUGH_BEHIND_SCHEDULER_ACTION  = 2205,

              BARRIER_LOGGING_ADD_BEFORE_LOOPOUT_ACTION             = 3101,
              BARRIER_LOGGING_REMOVE_BEFORE_LOOPOUT_ACTION          = 3102,
              BARRIER_LOGGING_GOTO_SLEEP_BEFORE_LOOPOUT_ACTION      = 3103,
              BARRIER_LOGGING_WAKE_ALL_BEFORE_LOOPOUT_ACTION        = 3104,
              BARRIER_LOGGING_WALK_THROUGH_BEFORE_LOOPOUT_ACTION    = 3105,

              BARRIER_LOGGING_ADD_BEHIND_LOOPOUT_ACTION             = 3201,
              BARRIER_LOGGING_REMOVE_BEHIND_LOOPOUT_ACTION          = 3202,
              BARRIER_LOGGING_GOTO_SLEEP_BEHIND_LOOPOUT_ACTION      = 3203,
              BARRIER_LOGGING_WAKE_ALL_BEHIND_LOOPOUT_ACTION        = 3204,
              BARRIER_LOGGING_WALK_THROUGH_BEHIND_LOOPOUT_ACTION    = 3205

             } BARRIER_LOGGING_ACTION;

typedef enum { BARRIER_LOGGING_UNDEF_TYPE, BARRIER_LOGGING_PROCESS_TYPE, BARRIER_LOGGING_SCHEDULER_TYPE}  BARRIER_LOGGING_TYPE;


void ConnectProcessToAllAssociatedBarriers(TASK_CONTROL_BLOCK  *pTcb,
                                           char *BarriersBeforeOnlySignal,
                                           char *BarriersBeforeSignalAndWait,
                                           char *BarriersBehindOnlySignal,
                                           char *BarriersBehindSignalAndWait,
                                           char *BarriersLoopOutBeforeOnlySignal,
                                           char *BarriersLoopOutBeforeSignalAndWait,
                                           char *BarriersLoopOutBehindOnlySignal,
                                           char *BarriersLoopOutBehindSignalAndWait);
int ConnectProcessToBarrier (struct TCB *pTcb, char *par_BarrierName, int par_WaitFlags);
void ActivateProcessBarriers(struct TCB *pTcb);
int DisconnectProcessFromBarrier (struct TCB *pTcb, char *par_BarrierName, int par_EmergencyCleanup);
void DisconnectProcessFromAllBarriers (struct TCB *pTcb, int par_EmergencyCleanup);


void ConnectSchedulerToAllAssociatedBarriers (SCHEDULER_DATA *Scheduler,
                                              char *BarriersBeforeOnlySignal,
                                              char *BarriersBeforeSignalAndWait,
                                              char *BarriersBehindOnlySignal,
                                              char *BarriersBehindSignalAndWait);
int ConnectSchedulerToBarrier (SCHEDULER_DATA *Scheduler, char *par_BarrierName, int par_WaitFlags);
int DisconnectSchedulerFromBarrier (SCHEDULER_DATA *Scheduler, char *par_BarrierName);
void DisconnectSchedulerFromAllBarriers (SCHEDULER_DATA *Scheduler);

int AddBarrier (char *par_BarrierName, int par_WaitIfAlone);
int DeleteBarrier (char *par_BarrierName);

void InitAllBarriers (void);

void CloseAllBarriers (void);

int AddNewBarrierToIni (const char *par_BarrierName);
int DeleteBarrierFromIni (const char *par_BarrierName);

void WalkThroughBarrierBeforeLoopOut (struct TCB *pTcb, char *par_BarrierName, int FirstOrSecond, int Disconnect);
void WalkThroughBarrierBehindLoopOut (struct TCB *pTcb, char *par_BarrierName, int FirstOrSecond, int Disconnect);

void WalkThroughBarrierBeforeProcess (struct TCB *pTcb, int FirstOrSecond, int Disconnect);
void WalkThroughBarrierBehindProcess (struct TCB *pTcb, int FirstOrSecond, int Disconnect);

void WalkThroughBarrierBeforeStartSchedulerCycle (SCHEDULER_DATA *pSchedulerData,
                                                 int FirstOrSecond, int Disconnect,
                                                 void (*Callback)(SCHEDULER_DATA *));
void WalkThroughBarrierBehindEndOfSchedulerCycle (SCHEDULER_DATA *pSchedulerData,
                                                 int FirstOrSecond, int Disconnect,
                                                 void (*Callback)(SCHEDULER_DATA *));

int AddProcessOrSchdulerToBarrierIniConfig (char *par_Name, int par_ProcessOrSchedulerFlag, int par_WaitFlag, int par_BeforeOrBehindFlag, char *par_Barrier);
int RemoveProcessOrSchedulerFromBarrierIniConfig (char *par_Name, int par_ProcessOrSchedulerFlag, int par_WaitFlag, int par_BeforeOrBehindFlag, char *par_Barrier);

void RemoveProcessOrSchedulerFromAllBarrierIniConfig (const char *par_Name, int par_ProcessOrSchedulerFlag);

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
                                );

char *GetAllBarrierString(void);
void FreeAllBarrierString (char *String);

int GetBarrierState (const char *par_BarrierName, const char *par_SchedulerOrProcessName);

int GetBarrierLoggingSize (void);

int GetBarierInfos (int par_BarrierNr,
                    int *ret_State, unsigned int *ret_SchedulerMask, unsigned int *ret_ProcessMask, int *ret_Switch,
                    unsigned int *ret_SchedulerFlags_0, unsigned int *ret_ProcessFlags_0,
                    unsigned int *ret_SchedulerFlags_1, unsigned int *ret_ProcessFlags_1,
                    int *ret_ProcesseOrSchedulerCount,
                    int *ret_WaitOnCounter,
                    unsigned int *ret_AddNewProcessMasksBefore, unsigned int *ret_AddNewProcessMasksBehind,
                    unsigned int *ret_AddNewSchedulerMasks, unsigned int *ret_RemoveProcessMasks, unsigned int *ret_RemoveSchedulerMasks, unsigned int *ret_BarriersLoggingCounter);


void WriteBarrierLoggingToFile (void);
void WriteBarrierStateToFile (void);

void RemoveWaitIfAloneForAllBarriers (void);
void WakeupAllBarriersForProcessLogin (void);

#endif

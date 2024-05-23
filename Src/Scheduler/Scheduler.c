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
#ifdef _WIN32
#include <Psapi.h>
#else
#include <ctype.h>
#include <string.h>
//#include <sys/syscall.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "RunTimeMeasurement.h"

#include "Config.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Fifos.h"
#include "Message.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "IniDataBase.h"
#include "Files.h"
#include "EnvironmentVariables.h"
#include "MainValues.h"
#include "Wildcards.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "tcb.h"
#define SCHEDULER_C
#include "InternalProcessList.h"
#undef SCHEDULER_C
#include "PipeMessages.h"
#include "SchedBarrier.h"
#include "ExternLoginTimeoutControl.h"
#include "DebugInfoDB.h"
#include "LoadSaveToFile.h"
#include "EquationParser.h"
#include "ScBbCopyLists.h"
#include "VirtualNetwork.h"
#include "IniSectionEntryDefines.h"
#include "InitProcess.h"
#include "RemoteMasterScheduler.h"
#include "RemoteMasterNet.h"
#include "ExtProcessReferences.h"
#include "ExtProcessRefFilter.h"
#include "my_udiv128.h"
#include "ProcessEquations.h"
#include "VirtualCanDriver.h"
#include "A2LLink.h"
#include "ParseCommandLine.h"
#include "MainWinowSyncWithOtherThreads.h"

#include "DelayedFetchData.h"

#include "Scheduler.h"

#define UNUSED(x) (void)(x)

static int SyncStartSchedulerFlag;

static CRITICAL_SECTION SyncStartSchedulerCriticalSection;
static CONDITION_VARIABLE SyncStartSchedulerConditionVariable;
static int SyncStartSchedulerWakeFlag;

static CRITICAL_SECTION PipeSchedCriticalSection;
static CRITICAL_SECTION FreeTcbCriticalSection;

static DWORD GuiThreadid;

void InitPipeSchedulerCriticalSections (void)
{
    InitializeCriticalSection (&PipeSchedCriticalSection);
    InitializeCriticalSection (&FreeTcbCriticalSection);

    GuiThreadid = GetCurrentThreadId();
}

int IsGuiThread(void)
{
    if (GuiThreadid == GetCurrentThreadId()) {
        return 1;
    } else {
        return 0;
    }
}

static int init_gui (void)
{
    return 0;
}

static void terminate_gui (void)
{
}

static void cyclic_gui (void)
{
}


TASK_CONTROL_BLOCK GuiTcb
    = INIT_TASK_COTROL_BLOCK("GUIProcess", INTERN_ASYNC, 10000, cyclic_gui, init_gui, terminate_gui, 1024*1024);

#define MAX_PIDS 64

#define GENERATE_UNIQUE_PID_COMMAND       0
#define INVALIDATE_UNIQUE_PID_COMMAND     1
#define FREE_UNIQUE_PID_COMMAND           2
#define GET_UNIQUE_PID_BY_NAME_COMMAND    3
#define GENERATE_UNIQUE_RT_PID_COMMAND    4

#define RT_PID_BIT_MASK          0x10000000

//#define GET_FREE_UNIQUE_PID_DEBUGGING

int GetOrFreeUniquePid (int par_Command, int par_Pid, const char *par_Name)
{
    // Process Ids are 16Bit value (1...32767) and must be unique
    // The last 6 significant bits are  6Bit correspond with the bit position inside the WrMaske of the blackboard
#ifdef GET_FREE_UNIQUE_PID_DEBUGGING
    static FILE *fh = NULL;
#endif
    static int Pid[MAX_PIDS];
    static char *Name[MAX_PIDS];
    static char ValidFlag[MAX_PIDS];
    static int PidCounter;
    int Ret;
    int x;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return rm_GetOrFreeUniquePid (par_Command, par_Pid, par_Name);
    }

    Ret = UNKNOWN_PROCESS;
    EnterCriticalSection (&PipeSchedCriticalSection);

#ifdef GET_FREE_UNIQUE_PID_DEBUGGING
    if (fh == NULL) {
        fh = fopen ("c:\\temp\\pid_logging.txt", "wt");
    }
    if (fh != NULL) {
        fprintf (fh, "%u:\n", GetCycleCounter());
        fprintf (fh, "GetOrFreeUniquePid (par_Command=%i, par_Pid=%i, par_Name=%s)\n", par_Command, par_Pid, par_Name);
        for (x = 0; x < MAX_PIDS; x++) {
            fprintf (fh, " %i", Pid[x]);
        }
        fprintf (fh, "\n");
    }
#endif
    switch (par_Command) {
    case GENERATE_UNIQUE_RT_PID_COMMAND:
    case GENERATE_UNIQUE_PID_COMMAND:
        Ret = NO_FREE_PID;
        for (x = 0; x < MAX_PIDS; x++) {
            if (Pid[x] == 0) {
                PidCounter++;
                if (PidCounter > (0x7FFF / MAX_PIDS)) {
                    PidCounter = 1;
                }
                if (par_Command == GENERATE_UNIQUE_RT_PID_COMMAND) {
                    Ret = Pid[x] = (PidCounter * MAX_PIDS + x) | RT_PID_BIT_MASK;
                } else {
                    Ret = Pid[x] = PidCounter * MAX_PIDS + x;
                }
                if (par_Name != NULL) {
                    int len = (int)strlen (par_Name) + 1;
                    Name[x] = (char*)my_malloc (len);
                    if (Name[x] != NULL) MEMCPY (Name[x], par_Name, (size_t)len);
                }
                ValidFlag[x] = 1;
                break;
            }
        }
        break;
    case INVALIDATE_UNIQUE_PID_COMMAND:
        Ret = UNKNOWN_PROCESS;
        for (x = 0; x < MAX_PIDS; x++) {
            if (Pid[x] == par_Pid) {
                ValidFlag[x] = 0;
                Ret = Pid[x];
                break;
            }
        }
        break;
    case FREE_UNIQUE_PID_COMMAND:
        Ret = UNKNOWN_PROCESS;
        for (x = 0; x < MAX_PIDS; x++) {
            if (par_Pid == Pid[x]) {
                ValidFlag[x] = 0;
                Ret = Pid[x];
                Pid[x] = 0;
                if (Name[x] != NULL) my_free (Name[x]);
                Name[x] = NULL;
                break;
            }
        }
        break;
    case GET_UNIQUE_PID_BY_NAME_COMMAND:
        Ret = UNKNOWN_PROCESS;
        if (par_Name != NULL) {
            for (x = 0; x < MAX_PIDS; x++) {
                if (ValidFlag[x] && (Pid[x] > 0) && (Name[x] != NULL)) {
                    if (!Compare2ProcessNames (Name[x], par_Name)) {
                        Ret = Pid[x];
                        break;
                    }
                }
            }
        }
        break;
    default:
        break;
    }
#ifdef GET_FREE_UNIQUE_PID_DEBUGGING
    if (fh != NULL) {
        fprintf (fh, "Ret = %i\n", Ret);
        fflush(fh);
    }
#endif
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return Ret;
}

#define FREE_TCB_DELAY_TIME_MS  100000

#define FREE_TCB_CMD_MARK_FOR_DELETION   0
#define FREE_TCB_CMD_DELETE_ALL          1
#define FREE_TCB_CMD_DELETE_OLDER_AS_1S  2
#define FREE_TCB_CMD_DELETE_THE_OLDEST   3

static void FreeTcbResources(TASK_CONTROL_BLOCK *pTcb)
{
    // Free the message buffer only for internal Processes
    if (pTcb->Type != EXTERN_ASYNC) {
        remove_message_queue (pTcb->message_buffer);
        pTcb->message_buffer = NULL;
    }
    if (pTcb->Type == EXTERN_ASYNC) {
        if (pTcb->TransmitMessageBuffer != NULL) my_free (pTcb->TransmitMessageBuffer);
        if (pTcb->ReceiveMessageBuffer != NULL) my_free (pTcb->ReceiveMessageBuffer);
        FreeCopyLists (&pTcb->CopyLists);
        if (pTcb->InsideExecutableName != NULL) my_free(pTcb->InsideExecutableName);
        if (pTcb->DllName != NULL) my_free(pTcb->DllName);
    }
    DeleteCriticalSection(&pTcb->CriticalSection);

    GetOrFreeUniquePid (FREE_UNIQUE_PID_COMMAND, pTcb->pid, pTcb->name);
    my_free (pTcb);
}

static void FreeTcb (TASK_CONTROL_BLOCK *pFreeTcb, int par_Cmd)
{
    static TASK_CONTROL_BLOCK *pFreeTcbListBase;
    static TASK_CONTROL_BLOCK *pFreeTcbListEnd;
    TASK_CONTROL_BLOCK *pFreeTcbOldest;
    TASK_CONTROL_BLOCK *pTcb, *pNextTcb;
    uint64_t TickCount64;
    uint64_t TickCount64Oldest = 0xFFFFFFFFFFFFFFFFULL;

    TickCount64 = GetTickCount64();

    EnterCriticalSection (&FreeTcbCriticalSection);
    switch(par_Cmd) {
    case FREE_TCB_CMD_MARK_FOR_DELETION:
        if (pFreeTcb != NULL) {
            GetOrFreeUniquePid (INVALIDATE_UNIQUE_PID_COMMAND, pFreeTcb->pid, pFreeTcb->name);
            pFreeTcb->min_time = TickCount64 + FREE_TCB_DELAY_TIME_MS;  // Wait 1 second till the TCB can be really delete
            // The new element should be added to the end of the list
            pFreeTcb->next = NULL;
            pFreeTcb->prev = pFreeTcbListEnd;
            if (pFreeTcbListBase == NULL) {
                pFreeTcbListBase = pFreeTcb;
            }
            if (pFreeTcbListEnd != NULL) {
                pFreeTcbListEnd->next = pFreeTcb;
            }
            pFreeTcbListEnd = pFreeTcb;
        }
        break;
    case FREE_TCB_CMD_DELETE_OLDER_AS_1S:
        // Walk through the process list and delete that one which are older than 1 second
        for (pTcb = pFreeTcbListBase; pTcb != NULL; pTcb = pNextTcb) {
            pNextTcb = pTcb->next;  // Save wbecause pTcb can be give free
            if (TickCount64 > pTcb->min_time) {
                // Remove process from the list
                if (pTcb->next != NULL) {
                    pTcb->next->prev = pTcb->prev;
                }
                if (pTcb->prev != NULL) {
                    pTcb->prev->next = pTcb->next;
                }
                // If first process erster Prozess, change list root
                if (pFreeTcbListBase == pTcb) {
                    pFreeTcbListBase = pTcb->next;
                }
                // If first process erster Prozess, change list end
                if (pFreeTcbListEnd == pTcb) {
                    pFreeTcbListEnd = pTcb->prev;
                }
                FreeTcbResources(pTcb);
            }
        }
        break;
    case FREE_TCB_CMD_DELETE_ALL:
        for (pTcb = pFreeTcbListBase; pTcb != NULL; pTcb = pNextTcb) {
            pNextTcb = pTcb->next;  // save because pTcb can be changed
            FreeTcbResources(pTcb);
        }
        pFreeTcbListBase = NULL;
        pFreeTcbListEnd = NULL;
        break;
    case FREE_TCB_CMD_DELETE_THE_OLDEST:
        pFreeTcbOldest = NULL;
        for (pTcb = pFreeTcbListBase; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->min_time < TickCount64Oldest) {
                // but it must be older than 0.1s
                if (TickCount64 > ((pTcb->min_time - FREE_TCB_DELAY_TIME_MS) + 100)) {
                    TickCount64Oldest = pTcb->min_time;
                    pFreeTcbOldest = pTcb;
                }
            }
        }
        if (pFreeTcbOldest != NULL) {
            // Remove process from the process list
            if (pFreeTcbOldest->next != NULL) {
                pFreeTcbOldest->next->prev = pFreeTcbOldest->prev;
            }
            if (pFreeTcbOldest->prev != NULL) {
                pFreeTcbOldest->prev->next = pFreeTcbOldest->next;
            }
            // If first process erster Prozess, change list root
            if (pFreeTcbListBase == pFreeTcbOldest) {
                pFreeTcbListBase = pFreeTcbOldest->next;
            }
            // If first process erster Prozess, change list end
            if (pFreeTcbListEnd == pFreeTcbOldest) {
                pFreeTcbListEnd = pFreeTcbOldest->prev;
            }
            FreeTcbResources(pFreeTcbOldest);
        }
        break;
    }
    LeaveCriticalSection (&FreeTcbCriticalSection);
}

static TASK_CONTROL_BLOCK *AllocAndInitTaskControlBlock (const char *ProcessName, HANDLE hPipe)
{
    TASK_CONTROL_BLOCK *RetTcb;

    RetTcb = (TASK_CONTROL_BLOCK*)my_calloc (1, sizeof (TASK_CONTROL_BLOCK));
    if (RetTcb == NULL) {
        ThrowError (1, "out of memory!");
    } else {
        int Pid;
        FreeTcb(NULL, FREE_TCB_CMD_DELETE_OLDER_AS_1S);  // cleanup all stored old TCBs
        Pid = GetOrFreeUniquePid (GENERATE_UNIQUE_PID_COMMAND, 0, ProcessName);
        if (Pid <= 0) {
            FreeTcb(NULL, FREE_TCB_CMD_DELETE_THE_OLDEST);  // cleanup all stored old TCBs
            // and try again
            Pid = GetOrFreeUniquePid (GENERATE_UNIQUE_PID_COMMAND, 0, ProcessName);
            if (Pid <= 0) {
                my_free (RetTcb);
                RetTcb = NULL;
            }
        }
        if (Pid > 0) {
            RetTcb->pid = Pid;
            strcpy (RetTcb->name, ProcessName);
            RetTcb->Type = EXTERN_ASYNC;
            RetTcb->state = EX_PROCSS_LOGIN;
            RetTcb->hPipe = hPipe;
            RetTcb->Active = 1;
        }
    }
    return RetTcb;
}

int AddProcessToScheduler (char *SchedulerName, TASK_CONTROL_BLOCK *pTcb);

PID start_process_timeout (const char *par_ProcessName, int par_Timeout, char **ret_NoErrMsg)
{
    int i;
    TASK_CONTROL_BLOCK  *pTcb;
    int prio;
    int time_steps;
    int delay;
    int PidSave;
    int timeout;
    char BBPrefix[BBVARI_PREFIX_MAX_LEN];
    char Scheduler[MAX_SCHEDULER_NAME_LENGTH];
    char BarriersBeforeOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersBeforeSignalAndWait[INI_MAX_LINE_LENGTH];
    char BarriersBehindOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersBehindSignalAndWait[INI_MAX_LINE_LENGTH];
    char BarriersLoopOutBeforeOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersLoopOutBeforeSignalAndWait[INI_MAX_LINE_LENGTH];
    char BarriersLoopOutBehindOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersLoopOutBehindSignalAndWait[INI_MAX_LINE_LENGTH];

    timeout = par_Timeout;
    // Is the process running
    if (get_pid_by_name (par_ProcessName) != UNKNOWN_PROCESS) {
        return PROCESS_RUNNING;
    }

    if (s_main_ini_val.ConnectToRemoteMaster) {
        if (rm_IsRealtimeProcess(par_ProcessName)) {
            int Pid;
            read_extprocinfos_from_ini (GetMainFileDescriptor(),
                                        par_ProcessName,
                                        &prio,
                                        &time_steps,
                                        &delay,
                                        &timeout,
                                        NULL,
                                        NULL,
                                        NULL,
                                        BBPrefix,
                                        Scheduler,
                                        BarriersBeforeOnlySignal,
                                        BarriersBeforeSignalAndWait,
                                        BarriersBehindOnlySignal,
                                        BarriersBehindSignalAndWait,
                                        BarriersLoopOutBeforeOnlySignal,
                                        BarriersLoopOutBeforeSignalAndWait,
                                        BarriersLoopOutBehindOnlySignal,
                                        BarriersLoopOutBehindSignalAndWait);

            Pid = GetOrFreeUniquePid(GENERATE_UNIQUE_RT_PID_COMMAND, 0, par_ProcessName);
            if (Pid < 0) {
                return NO_FREE_PID;
            }
            return rm_StartRealtimeProcess (Pid, par_ProcessName, "RealtimeScheduler", prio, time_steps, delay);
        }
    }

    // Is the process an internal process
    for (i = 0; all_known_tasks[i] != NULL; i++) {
        if (strcmpi (all_known_tasks[i]->name, par_ProcessName) == 0) {
            break;
        }
    }

    // If process not known than try to start as external process
    if (all_known_tasks[i] == NULL) {
        return activate_extern_process (par_ProcessName, timeout, ret_NoErrMsg);
    }

    pTcb = AllocAndInitTaskControlBlock (par_ProcessName, INVALID_HANDLE_VALUE);
    if (pTcb == NULL) {
        return NO_FREE_MEMORY;
    }
    PidSave = pTcb->pid;

    MEMCPY (pTcb, all_known_tasks[i], sizeof(TASK_CONTROL_BLOCK));
    pTcb->pid = PidSave;

#ifndef REMOTE_MASTER
    if (s_main_ini_val.ConnectToRemoteMaster) {
        rm_AddProcessToPidNameArray(pTcb->pid, pTcb->name, pTcb->message_size);
    }// else {
        pTcb->message_buffer = build_message_queue (pTcb->message_size);
    //}
#else
    pTcb->message_buffer = build_message_queue (pTcb->message_size);
#endif

    // Read process informations from INI file
    if (read_extprocinfos_from_ini (GetMainFileDescriptor(),
                                    par_ProcessName,
                                    &prio,
                                    &time_steps,
                                    &delay,
                                    &timeout,
                                    NULL,
                                    NULL,
                                    NULL,  
                                    BBPrefix,
                                    Scheduler,
                                    BarriersBeforeOnlySignal,
                                    BarriersBeforeSignalAndWait,
                                    BarriersBehindOnlySignal,
                                    BarriersBehindSignalAndWait,
                                    BarriersLoopOutBeforeOnlySignal,
                                    BarriersLoopOutBeforeSignalAndWait,
                                    BarriersLoopOutBehindOnlySignal,
                                    BarriersLoopOutBehindSignalAndWait) == 0) {
        pTcb->prio = prio;
        pTcb->time_steps = time_steps;
        pTcb->delay = delay;
        if (par_Timeout < 0) {
            pTcb->timeout = timeout;
        }
    }

    if (pTcb->delay >= pTcb->time_steps) {
        pTcb->delay = pTcb->time_steps - 1;
    }

    pTcb->time_counter = 0;

    // Get a blackboard access mask
    if (get_bb_accessmask (pTcb->pid, &(pTcb->bb_access_mask), BBPrefix) != 0) {
        FreeTcb (pTcb, FREE_TCB_CMD_MARK_FOR_DELETION);
        return NO_FREE_ACCESS_MASK;
    }
    get_free_wrflag (pTcb->pid, &pTcb->wr_flag_mask);

    ConnectProcessToAllAssociatedBarriers (pTcb,
                                           BarriersBeforeOnlySignal,
                                           BarriersBeforeSignalAndWait,
                                           BarriersBehindOnlySignal,
                                           BarriersBehindSignalAndWait,
                                           BarriersLoopOutBeforeOnlySignal,
                                           BarriersLoopOutBeforeSignalAndWait,
                                           BarriersLoopOutBehindOnlySignal,
                                           BarriersLoopOutBehindSignalAndWait);

    pTcb->Active = 1;
    pTcb->state = EX_PROCESS_INIT;

    InitializeCriticalSection(&pTcb->CriticalSection);

    AddProcessToScheduler (Scheduler, pTcb);

    // returns the process Id
    return pTcb->pid;
}


PID start_process (const char *ProcessName)
{
    return start_process_timeout (ProcessName, -1, NULL);
}

PID start_process_err_msg (char *ProcessName, char **ret_ErrMsg)
{
    return start_process_timeout (ProcessName, -1, ret_ErrMsg);
}

PID start_process_env_var (char *ProcessName)
{
    char Path[MAX_PATH];
    SearchAndReplaceEnvironmentStrings(ProcessName, Path, sizeof(Path));
    return start_process_timeout (Path, -1, NULL);
}

// Starts an internal or external process
// Return value can be the process Id on success
// Or if an error occur:
// PROCESS_RUNNING -> Prozess is already running
// UNKNOWN_PROCESS -> Prozess not found
// NO_FREE_MEMORY -> not enough memory
PID start_process_ex (const char *name, int Prio, int Cycle, int Delay, int Timeout,
                      const char *SVLFile,
                      const char *A2LFile, int A2LFlags,
                      const char *BBPrefix,
                      int UseRangeControl,  // If this is 0 ignore all following "RangeControl*" parameters
                      int RangeControlBeforeActiveFlags,
                      int RangeControlBehindActiveFlags,
                      int RangeControlStopSchedFlag,
                      int RangeControlOutput,
                      int RangeErrorCounterFlag,
                      const char *RangeErrorCounter,
                      int RangeControlVarFlag,
                      const char *RangeControl,
                      int RangeControlPhysFlag,
                      int RangeControlLimitValues,
                      char **ret_ErrMsg)
{
    int Prio_tmp;
    int Cycle_tmp;
    int Timeout_tmp;
    int Delay_tmp;
    char RangeErrorCounter_tmp[BBVARI_NAME_SIZE];
    char RangeControl_tmp[BBVARI_NAME_SIZE];
    unsigned int RangeControlFlags;
    char BBPrefix_tmp[BBVARI_NAME_SIZE];
    char name_up[MAX_PATH];

    // The section and entrys inside the INI files are case sensitive
    // This will used as entrys inside the scheduler section
    // To avoid problems with diferent uppercase or lowercase leters all externam process names should e uppercase
    // This is only necessary on Windows
    strcpy (name_up, name);
#ifdef _WIN32
    if (!IsInternalProcess (name_up)) {
        strupr (name_up);
    }
#endif
    read_extprocinfos_from_ini (GetMainFileDescriptor(),
                                name_up,
                                &Prio_tmp,
                                &Cycle_tmp,
                                &Delay_tmp,
                                &Timeout_tmp,
                                RangeErrorCounter_tmp,
                                RangeControl_tmp,
                                &RangeControlFlags,
                                BBPrefix_tmp,
                                NULL, // char *Scheduler,
                                NULL, // char *BarriersBeforeOnlySignal,
                                NULL, // char *BarriersBeforeSignalAndWait,
                                NULL, // char *BarriersBehindOnlySignal,
                                NULL,  // char *BarriersBehindSignalAndWait
                                NULL, // char *BarriersLoopOutBeforeOnlySignal,
                                NULL, // char *BarriersLoopOutBeforeSignalAndWait,
                                NULL, // char *BarriersLoopOutBehindOnlySignal,
                                NULL  // char *BarriersLoopOutBehindSignalAndWait
                                );

    if (Prio < 0) Prio = Prio_tmp;
    if (Cycle < 0) Cycle = Cycle_tmp;
    if (Delay < 0) Delay = Delay_tmp;
    if (Timeout < 0) Timeout = Timeout_tmp;

    if (Delay >= Cycle) {
        Delay = Cycle - 1;
    }

    if (!UseRangeControl) {  // Use the INI file information
        #define CHECK_RANGE_CONTROL_FLAG(f) ((RangeControlFlags & (f)) == (f))
        RangeControlBeforeActiveFlags = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_BEFORE_ACTIVE_FLAG) +
                                        (CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_BEFORE_INIT_AS_DEACTIVE) << 1);
        RangeControlBehindActiveFlags = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_BEHIND_ACTIVE_FLAG) +
                                        (CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE) << 1);
        RangeControlStopSchedFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_STOP_SCHEDULER_FLAG);
        RangeControlOutput = RangeControlFlags & 0xF;              // Messagebox, Write to File, ...
        RangeErrorCounterFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_COUNTER_VARIABLE_FLAG);
        RangeControlVarFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_CONTROL_VARIABLE_FLAG);
        RangeControlPhysFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_PHYSICAL_FLAG);
        RangeControlLimitValues = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_LIMIT_VALUES_FLAG);
    } else {
        StringCopyMaxCharTruncate (RangeErrorCounter_tmp, RangeErrorCounter, sizeof (RangeErrorCounter_tmp));
        StringCopyMaxCharTruncate (RangeControl_tmp,RangeControl, sizeof (RangeControl_tmp));
    }

    write_extprocinfos_to_ini (GetMainFileDescriptor(), name_up,
                               Prio,
                               Cycle,
                               Delay,
                               Timeout,
                               RangeControlBeforeActiveFlags,
                               RangeControlBehindActiveFlags,
                               RangeControlStopSchedFlag,
                               RangeControlOutput,
                               RangeErrorCounterFlag,
                               RangeErrorCounter_tmp,
                               RangeControlVarFlag,
                               RangeControl_tmp,
                               RangeControlPhysFlag,
                               RangeControlLimitValues,
                               BBPrefix,
                               NULL, //char *Scheduler,
                               NULL, //char *BarriersBeforeOnlySignal,
                               NULL, //char *BarriersBeforeSignalAndWait,
                               NULL, //char *BarriersBehindOnlySignal,
                               NULL, //char *BarriersBehindSignalAndWait,
                               NULL, //char *BarriersLoopOutBeforeOnlySignal,
                               NULL, //char *BarriersLoopOutBeforeSignalAndWait,
                               NULL, //char *BarriersLoopOutBehindOnlySignal,
                               NULL  //char *BarriersLoopOutBehindSignalAndWait
                               );

    if (strlen (SVLFile)) {
        SetSVLFileLoadedBeforeInitProcessFileName (name_up, SVLFile, 0);
    }
    if (strlen (A2LFile)) {
        SetA2LFileAssociatedToProcessFileName (name_up, A2LFile, A2LFlags, 0);
    }
    return start_process_timeout (name, Timeout, ret_ErrMsg);
}


void RemoveProcessFromSchedulerList (SCHEDULER_DATA *pSchedulerData, TASK_CONTROL_BLOCK *pTcb)
{
    EnterCriticalSection (&PipeSchedCriticalSection);

    // It this is the current active process
    if (pSchedulerData->CurrentTcb == pTcb) {
        pSchedulerData->CurrentTcb = pTcb->next;
    }

    // Remove process from the process list
    if (pTcb->next != NULL) {
        pTcb->next->prev = pTcb->prev;
    }
    if (pTcb->prev != NULL) {
        pTcb->prev->next = pTcb->next;
    }

    // If first process erster Prozess, change list root
    if (pSchedulerData->FirstTcb == pTcb) {
        pSchedulerData->FirstTcb = pTcb->next;
    }

    // If first process erster Prozess, change list end
    if (pSchedulerData->LastTcb == pTcb) {
        pSchedulerData->LastTcb = pTcb->prev;
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
}


static void KillProcess (SCHEDULER_DATA *pSchedulerData, TASK_CONTROL_BLOCK *pTcb, int SendKillSignal, int par_EmergencyCleanup)
{
    if (pTcb->Type == EXTERN_ASYNC) {
        FreeExternProcessReferenceFilter (pTcb->pid);
    }

    if (pTcb->BeforeProcessEquations != NULL) {
        DelBeforeProcessEquations (-1, pTcb->name);
        pTcb->BeforeProcessEquationCounter = 0;
        my_free (pTcb->BeforeProcessEquations);
        pTcb->BeforeProcessEquations = NULL;
    }
    if (pTcb->BehindProcessEquations != NULL) {
        DelBehindProcessEquations (-1, pTcb->name);
        pTcb->BehindProcessEquationCounter = 0;
        my_free (pTcb->BehindProcessEquations);
        pTcb->BehindProcessEquations = NULL;
    }

    if (SendKillSignal) {
        if (pTcb->Type == EXTERN_ASYNC) {
            SendKillExternalProcessMessage (pTcb);
        }
    }
    if (pTcb->Type == EXTERN_ASYNC) {
        if (pTcb->A2LLinkNr > 0) {
            A2LCloseLink(pTcb->A2LLinkNr, pTcb->name);
        }
        ClosePipeToExternProcess (pTcb);
        CleanVirtualCanExternalProcess(pTcb);
    }

    DelProcessFromArray (pTcb->pid);
    // Remove all blackboard accesses
    remove_all_bbvari (pTcb->pid);

    // Free the message buffer only for internal processes
    if (pTcb->Type == INTERN_ASYNC) {
#ifndef REMOTE_MASTER
    if (s_main_ini_val.ConnectToRemoteMaster) {
        rm_RemoveProcessFromPidNameArray (pTcb->pid);
    } else {
        remove_message_queue (pTcb->message_buffer);
    }
#else
        remove_message_queue (pTcb->message_buffer);
#endif
        pTcb->message_buffer = NULL;
    }
    if (pTcb->Type == EXTERN_ASYNC) {
        // Update all windows using debug informations of this process
        application_update_terminate_process (pTcb->name, pTcb->DllName);
        CleanVirtualCanExternalProcess (pTcb);
    }
    RemoveProcessFromSchedulerList (pSchedulerData, pTcb);

    if (pTcb->Type == EXTERN_ASYNC) {
#ifdef _WIN32
        if (pTcb->hAsyncAlivePingReqEvent != NULL) CloseHandle (pTcb->hAsyncAlivePingReqEvent);
        if (pTcb->hAsyncAlivePingAckEvent != NULL) CloseHandle (pTcb->hAsyncAlivePingAckEvent);
#endif
        TerminatWaitUntilProcessIsNotActive (pTcb);
        if (pTcb->ProcHandle != NULL_INT_OR_PTR) {
#ifdef _WIN32
            CloseHandle(pTcb->ProcHandle);
#endif
        }
    }

    DisconnectProcessFromAllBarriers (pTcb, par_EmergencyCleanup);

    // Free blackboard maske
    free_bb_accessmask (pTcb->pid, pTcb->bb_access_mask);

    DeleteCriticalSection(&pTcb->CriticalSection);

    // The Task Control Block will be removed delayed (1s) also the Pid will be invalid delayed
    FreeTcb (pTcb, FREE_TCB_CMD_MARK_FOR_DELETION);
}

#ifdef _WIN32
static DWORD64 GetTimeOfSystemIsRunning64 (void)
{
    LARGE_INTEGER Frequency;
    LARGE_INTEGER Time;
    DWORD64 Ret;

    QueryPerformanceFrequency (&Frequency);
    QueryPerformanceCounter(&Time);

#ifdef __GNUC__
    Ret = (DWORD64)(((unsigned __int128)Time.QuadPart * (unsigned __int128)TIMERCLKFRQ) / Frequency.QuadPart);
#else
    DWORD64 Low, High, Remainder;
    Low = _umul128((uint64_t)Time.QuadPart, (uint64_t)TIMERCLKFRQ, &High);
    Ret = _udiv128(High, Low, (uint64_t)Frequency.QuadPart, &Remainder);
#endif
    return Ret;
}

static double GetTimeOfSystemIsRunning (void)
{
    LARGE_INTEGER frequency; 
    LARGE_INTEGER t;
    double time;

    QueryPerformanceFrequency (&frequency);
    QueryPerformanceCounter(&t); 
    time =(double)t.QuadPart / (double)frequency.QuadPart;
    return time;
}
#else

static uint64_t GetTimeOfSystemIsRunning64 (void)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
}

static double GetTimeOfSystemIsRunning (void)
{
    return GetTimeOfSystemIsRunning64() * 0.000000001;
}

#endif


#define T 0.05

void CalculateRealtimeFactor (SCHEDULER_DATA *pSchedulerData)
{
    double Time;
    
    Time = GetTimeOfSystemIsRunning ();
    pSchedulerData->SchedPeriodNotFiltered = Time - pSchedulerData->LastSchedCallTime;

    pSchedulerData->LastSchedCallTime = Time;

    pSchedulerData->SchedPeriodFiltered = pSchedulerData->SchedPeriodFiltered * (1.0 - T) + pSchedulerData->SchedPeriodNotFiltered * T;

    // Realtime factor is the ratio of configured and measured sample time
    pSchedulerData->RealtimeFactorFiltered = pSchedulerData->SchedPeriod / pSchedulerData->SchedPeriodFiltered; 
}

// Scheduler data are static
static SCHEDULER_DATA SchedulerData[MAX_SCHEDULERS];
static int SchedulerCount;

// This funkcion will be called only by scheduler 0
static int WaitForNextSchedulerCycle (SCHEDULER_DATA *pSchedulerData)
{
    if (s_main_ini_val.NotFasterThanRealTime) {
        DWORD Remains;
        if (!pSchedulerData->not_faster_than_real_time_old) {   // currently switched on
            pSchedulerData->TimeOffsetBetweenSimualtedAndRealTimeInNanoSecond = pSchedulerData->SimulatedTimeInNanoSecond  - GetTimeOfSystemIsRunning64 ();
        }
WAIT_LOOP:
        pSchedulerData->SystemTimeInNanoSecond = GetTimeOfSystemIsRunning64 ();
        pSchedulerData->TimeDiffBetweenSymAndSystemInNanoSecond = (int64_t)(pSchedulerData->SimulatedTimeInNanoSecond - (pSchedulerData->TimeOffsetBetweenSimualtedAndRealTimeInNanoSecond + pSchedulerData->SystemTimeInNanoSecond));
        if (pSchedulerData->TimeDiffBetweenSymAndSystemInNanoSecond > pSchedulerData->SchedPeriodInNanoSecond) {  // The simulation was to fast wait a little bit
            if (s_main_ini_val.DontCallSleep) {
                // dont call sleep insead burn down cpu time (better realtime behaviour)
                goto WAIT_LOOP;
            }

            Remains = (DWORD)(pSchedulerData->TimeDiffBetweenSymAndSystemInNanoSecond / (int64_t)(TIMERCLKFRQ / 1000));
#ifdef _WIN32
            Sleep (Remains);
#else
            usleep(Remains*1000);
#endif
        } else if (pSchedulerData->TimeDiffBetweenSymAndSystemInNanoSecond < -pSchedulerData->SchedPeriodInNanoSecond) { // The simulation was to slow (slower than realtime)
            pSchedulerData->TimeOffsetBetweenSimualtedAndRealTimeInNanoSecond = pSchedulerData->SimulatedTimeInNanoSecond - pSchedulerData->SystemTimeInNanoSecond;
        }
    }
    pSchedulerData->not_faster_than_real_time_old = s_main_ini_val.NotFasterThanRealTime;
    CalculateRealtimeFactor (pSchedulerData);
    return 1;
}

// This are arrays for each scheduler. Here will be stored all to start process till the scheduler are able to start them
// Schedulers should only start process at the beginning or the end of a cycle.
static TASK_CONTROL_BLOCK *AddProcessesToSchedPtrs[MAX_SCHEDULERS][MAX_PIDS];
static int AddProcessesToSchedWrPos[MAX_SCHEDULERS];
static int AddProcessesToSchedRdPos[MAX_SCHEDULERS];

int GetSchedulerCount (void)
{
    return SchedulerCount;
}

SCHEDULER_DATA *GetSchedulerInfos (int par_SchedulerNr)
{
    if (par_SchedulerNr >= SchedulerCount) {
        return NULL;
    }
    return &(SchedulerData[par_SchedulerNr]);
}

static int GetSchedulerNr (char *SchedulerName)
{
    int x;
    for (x = 0; x < MAX_SCHEDULERS; x++) {
        if (!strcmp (SchedulerData[x].SchedulerName, SchedulerName)) {
            return x;
        }
    }
    return -1;
}

SCHEDULER_DATA *GetCurrentScheduler (void)
{
    DWORD CurrentThreadId;
    int x;

    CurrentThreadId = GetCurrentThreadId ();
    for (x = 0; x < SchedulerCount; x++) {
        if (SchedulerData[x].SchedulerThreadId == CurrentThreadId) {
            return &(SchedulerData[x]);
        }
    }
    return NULL;
}

SCHEDULER_DATA *GetSchedulerProcessIsRunning (int Pid)
{
    int x;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == Pid) {
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return &(SchedulerData[x]);
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return NULL;
}

char *GetSchedulerName (int par_Index)
{
    if ((par_Index >= 0) && (par_Index < SchedulerCount)) {
        return SchedulerData[par_Index].SchedulerName;
    } else {
        return "unknown";
    }
}

int GetSchedulerIndex (SCHEDULER_DATA *par_Scheduler)
{
    return (int)(par_Scheduler - SchedulerData);
}


int GetSchedulerNameProcessIsRunning (char *ProcessName, char *ret_SchedulerName, int par_MaxChars)
{
    int x;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (!Compare2ProcessNames (pTcb->name, ProcessName)) {
                int Len = (int)strlen (SchedulerData[x].SchedulerName);
                if (Len > (par_MaxChars-1)) Len = par_MaxChars-1;
                MEMCPY (ret_SchedulerName, SchedulerData[x].SchedulerName, (size_t)Len);
                ret_SchedulerName[Len] = 0;
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return pTcb->pid;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;
}


SCHEDULER_DATA *RegisterScheduler  (char *SchedulerName)
{
    SCHEDULER_DATA *Ret;
    if (SchedulerCount < MAX_SCHEDULERS) {
        memset (&(SchedulerData[SchedulerCount]), 0, sizeof (SchedulerData[SchedulerCount]));
        SchedulerData[SchedulerCount].Number = SchedulerCount;
        strcpy (SchedulerData[SchedulerCount].SchedulerName, SchedulerName);
        SchedulerData[SchedulerCount].SchedulerNr = SchedulerCount;
        SchedulerData[SchedulerCount].BarrierMask = 1UL << SchedulerCount;
        SchedulerData[SchedulerCount].State = SCHED_RUNNING_STATE;
        Ret = &(SchedulerData[SchedulerCount]);
        SchedulerCount++;
    } else {
        Ret = NULL;
    }
    return Ret;
}

TASK_CONTROL_BLOCK *GetCurrentTcb (void)
{
    DWORD CurrentThreadId;
    int x;

    CurrentThreadId = GetCurrentThreadId ();
    for (x = 0; x < SchedulerCount; x++) {
        if (SchedulerData[x].SchedulerThreadId == CurrentThreadId) {
            return SchedulerData[x].CurrentTcb;
        }
    }
    return &GuiTcb;
}

int GetCurrentPid (void)
{
    DWORD CurrentThreadId;
    int x;

    CurrentThreadId = GetCurrentThreadId ();
    for (x = 0; x < SchedulerCount; x++) {
        if (SchedulerData[x].SchedulerThreadId == CurrentThreadId) {
            if (SchedulerData[x].CurrentTcb != NULL) {
                return SchedulerData[x].CurrentTcb->pid;
            }
        }
    }
    return GuiTcb.pid;
}

int GetGuiPid(void)
{
    return GuiTcb.pid;
}

int get_pid_by_name (const char *ProcessName)
{
    // This function will return a valid pid even if it is not sorted into a scheduller
    // And will be return a valid pid even the process is terminated
    return GetOrFreeUniquePid(GET_UNIQUE_PID_BY_NAME_COMMAND, 0, ProcessName);
}


int lives_process_inside_dll (const char *ProcessName, char *ret_DllName)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (Compare2ProcessNames (pTcb->name, ProcessName) == 0) {
                int Ret = 0;
                if (pTcb->DllName != NULL) {
                    if (strlen(pTcb->DllName) > 0) {
                        if (ret_DllName != NULL) strcpy (ret_DllName, pTcb->DllName);
                        Ret = 1;
                    }
                }
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return Ret;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;
}

HANDLE get_extern_process_handle_by_pid (int pid)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->Type == EXTERN_ASYNC) {
                if (pTcb->pid == pid) {
                    LeaveCriticalSection (&PipeSchedCriticalSection);
                    return pTcb->ProcHandle;
                }
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return INVALID_HANDLE_VALUE;
}

DWORD get_extern_process_windows_id_by_pid (int pid)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->Type == EXTERN_ASYNC) {
                if (pTcb->pid == pid) {
                    LeaveCriticalSection (&PipeSchedCriticalSection);
                    return pTcb->ProcessId;
                }
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return (DWORD)-1;
}



TASK_CONTROL_BLOCK * GetPointerToTaskControlBlock (PID pid)
{
    TASK_CONTROL_BLOCK *RetTcb = NULL;
    int x;
    
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        // Walk through all processes of this scheduler
        for (RetTcb = SchedulerData[x].FirstTcb; RetTcb != NULL; RetTcb = RetTcb->next) {
            if (RetTcb->pid == pid) {
                goto BREAK_OUT;
            }
        }
    }
    BREAK_OUT:
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return RetTcb;
}

// This function checks it it is an internal process name
// It will return 1 for an internal running process
// It will return 2 for an internal not running process
// If it is not an internal process the return value is 0
int IsInternalProcess (char *ProcessName)
{
    int i;
    for (i = 0; all_known_tasks[i] != NULL; i++) {
        if (!stricmp (all_known_tasks[i]->name, ProcessName)) {
            if (get_pid_by_name(all_known_tasks[i]->name) == UNKNOWN_PROCESS) return 1;
            else return 2;
        }
    }
    return 0;
}


// With this function new processes will be started
// It will be add the processes only to a buffer
// The scheduler should look at the beginning or/and end of its cycle into this buffer.
// This function can be called from outside the scheduler threads
int AddProcessToScheduler (char *SchedulerName, TASK_CONTROL_BLOCK *pTcb)
{
    int SchedulerNr;

    SchedulerNr = GetSchedulerNr (SchedulerName);

    if ((SchedulerNr < 0) || (SchedulerNr >= MAX_SCHEDULERS)) {
        return -1;
    }
    if (AddProcessesToSchedPtrs[SchedulerNr][AddProcessesToSchedWrPos[SchedulerNr]] == NULL) {
        AddProcessesToSchedPtrs[SchedulerNr][AddProcessesToSchedWrPos[SchedulerNr]] = pTcb;
        if (AddProcessesToSchedWrPos[SchedulerNr] >= (MAX_PIDS - 1)) {
            AddProcessesToSchedWrPos[SchedulerNr] = 0;
        } else {
            AddProcessesToSchedWrPos[SchedulerNr]++;
        }

        // Wakup all barrirs for login check
        WakeupAllBarriersForProcessLogin();

        return 0;
    } else {
        return -1;
    }
}


static void InsertProcessToSchedulerList (SCHEDULER_DATA *pSchedulerData, TASK_CONTROL_BLOCK *pNewTcb)
{
    TASK_CONTROL_BLOCK *pTcb;

    EnterCriticalSection (&PipeSchedCriticalSection);
    // There is no process inside the list
    if (pSchedulerData->FirstTcb == NULL) {
        pSchedulerData->FirstTcb = pSchedulerData->LastTcb = pNewTcb;
    } else {
        // Search inside process list (belong the priority)
        for (pTcb = pSchedulerData->FirstTcb; pTcb != NULL;
             pTcb = pTcb->next) {
            if (pNewTcb->prio < pTcb->prio) {
                break;
            }
        }

        // Insert process accordingly
        if (pTcb == NULL) {   // Add to the end
            pNewTcb->prev = pSchedulerData->LastTcb;
            pNewTcb->next = NULL;
            pSchedulerData->LastTcb = pNewTcb->prev->next = pNewTcb;
        } else if (pTcb->prev == NULL) {  // Add to the beginning
            pNewTcb->prev = NULL;
            pNewTcb->next = pTcb;
            pSchedulerData->FirstTcb = pTcb->prev = pNewTcb;
        } else {                       // Insert somewhere in the middle
            pNewTcb->next = pTcb;
            pNewTcb->prev = pTcb->prev;
            pNewTcb->prev->next = pNewTcb->next->prev = pNewTcb;
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);

}

int AddExternPorcessToScheduler (char *ProcessName, char *ExecutableName, int ProcessNumber, int NumberOfProcesses,
                                 DWORD ProcessId, HANDLE hPipe, int SocketOrPipe,
                                 int Priority, int CycleDivider, int Delay,
                                 int IsStartedInDebugger, int MachineType, HANDLE hAsyncAlivePingReqEvent, HANDLE hAsyncAlivePingAckEvent, char *DllName,
                                 DWORD (*ReadFromConnection) (HANDLE Handle, void *Buffer, DWORD BufferSize, LPDWORD BytesRead),
                                 DWORD (*WriteToConnection) (HANDLE Handle, void *Buffer, int nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten),
                                 void  (*CloseConnectionToExternProcess) (HANDLE Handle),
                                 FILE *ConnectionLoggingFile,
                                 uint64_t ExecutableBaseAddress, uint64_t DllBaseAddress, int ExternProcessComunicationProtocolVersion)
{
#ifndef _WIN32
    UNUSED(hAsyncAlivePingReqEvent);
    UNUSED(hAsyncAlivePingAckEvent);
#endif
    int Ret;
    TASK_CONTROL_BLOCK *Tcb;
    char BBPrefix[BBVARI_PREFIX_MAX_LEN];
    char Scheduler[MAX_SCHEDULER_NAME_LENGTH];
    char BarriersBeforeOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersBeforeSignalAndWait[INI_MAX_LINE_LENGTH];
    char BarriersBehindOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersBehindSignalAndWait[INI_MAX_LINE_LENGTH];
    char BarriersLoopOutBeforeOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersLoopOutBeforeSignalAndWait[INI_MAX_LINE_LENGTH];
    char BarriersLoopOutBehindOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersLoopOutBehindSignalAndWait[INI_MAX_LINE_LENGTH];
    char RangeControlCounterVariable[BBVARI_NAME_SIZE];
    char RangeControlControlVariable[BBVARI_NAME_SIZE];

    LoginProcess (ExecutableName, ProcessNumber, NumberOfProcesses);

    Tcb = AllocAndInitTaskControlBlock (ProcessName, hPipe);
    if (Tcb == NULL) {
        ThrowError (1, "cannot alloc task control block");
        Ret = -2;
    } else {
        Tcb->SocketOrPipe = SocketOrPipe;
        Tcb->IsStartedInDebugger = IsStartedInDebugger;
        Tcb->MachineType = MachineType;
        Tcb->ReadFromConnection = ReadFromConnection;
        Tcb->WriteToConnection = WriteToConnection;
        Tcb->CloseConnectionToExternProcess = CloseConnectionToExternProcess;
        Tcb->ConnectionLoggingFile = ConnectionLoggingFile;
        Tcb->ExecutableBaseAddress = ExecutableBaseAddress;
        Tcb->DllBaseAddress = DllBaseAddress;
        Tcb->ExternProcessComunicationProtocolVersion = ExternProcessComunicationProtocolVersion;
#ifdef _WIN32
        if ((hAsyncAlivePingReqEvent != NULL) && (hAsyncAlivePingAckEvent != NULL)) {
            HANDLE hProcess;
            hProcess = OpenProcess (PROCESS_DUP_HANDLE, FALSE, ProcessId);
            if (hProcess != NULL) {
                Ret = DuplicateHandle (hProcess, hAsyncAlivePingReqEvent, GetCurrentProcess (), &(Tcb->hAsyncAlivePingReqEvent), 0, FALSE, DUPLICATE_SAME_ACCESS);
                Ret = Ret && DuplicateHandle (hProcess, hAsyncAlivePingAckEvent, GetCurrentProcess (), &(Tcb->hAsyncAlivePingAckEvent), 0, FALSE, DUPLICATE_SAME_ACCESS);
                if (Ret == 0) {
                    char *lpMsgBuf;
                    DWORD dw = GetLastError();
                    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                   FORMAT_MESSAGE_FROM_SYSTEM |
                                   FORMAT_MESSAGE_IGNORE_INSERTS,
                                   NULL,
                                   dw,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   (LPTSTR) &lpMsgBuf,
                                   0, NULL );
                    ThrowError (1, "cannot DuplicateHandle (%s)\n", lpMsgBuf);
                    LocalFree (lpMsgBuf);
                }
                CloseHandle (hProcess);
            } else {
                char *lpMsgBuf;
                DWORD dw = GetLastError();
                FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL,
                                dw,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR) &lpMsgBuf,
                                0, NULL );
                ThrowError (1, "cannot OpenProcess (%s)\n", lpMsgBuf);
                LocalFree (lpMsgBuf);
                Tcb->hAsyncAlivePingReqEvent = NULL;
                Tcb->hAsyncAlivePingAckEvent = NULL;
            }
        } else {
#else
        if (1) {
#endif
            Tcb->hAsyncAlivePingReqEvent = NULL_INT_OR_PTR;
            Tcb->hAsyncAlivePingAckEvent = NULL_INT_OR_PTR;
        }

        if (read_extprocinfos_from_ini (GetMainFileDescriptor(), ProcessName,
                                        &(Tcb->prio),
                                        &(Tcb->time_steps),
                                        &(Tcb->delay),
                                        &(Tcb->timeout),
                                        RangeControlCounterVariable,
                                        RangeControlControlVariable,
                                        &(Tcb->RangeControlFlags),
                                        BBPrefix,
                                        Scheduler,
                                        BarriersBeforeOnlySignal,
                                        BarriersBeforeSignalAndWait,
                                        BarriersBehindOnlySignal,
                                        BarriersBehindSignalAndWait,
                                        BarriersLoopOutBeforeOnlySignal,
                                        BarriersLoopOutBeforeSignalAndWait,
                                        BarriersLoopOutBehindOnlySignal,
                                        BarriersLoopOutBehindSignalAndWait) != 0) {
            Tcb->prio = 150;
            Tcb->time_steps = 1;
            Tcb->delay = 0;
            Tcb->timeout = s_main_ini_val.ExternProcessLoginTimeout;
        }

        if (Priority > 0) {
            // Prio will be set by the external process, don't use the value frome the INI file
            Tcb->prio = Priority;
        }
        if (CycleDivider >= 0) {
            // CycleDivider will be set by the external process, don't use the value frome the INI file
            Tcb->time_steps = CycleDivider;
        }
        if (Delay >= 0) {
            // Delay will be set by the external process, don't use the value frome the INI file
            Tcb->delay = Delay;
        }

        if (Tcb->delay >= Tcb->time_steps) {
            Tcb->delay = Tcb->time_steps - 1;
        }

        Tcb->time_counter = 0;

        // Blackboard access mask
        if (get_bb_accessmask (Tcb->pid, &(Tcb->bb_access_mask), BBPrefix) != 0) {
            ThrowError (1, "no free blackboard access mask");
            FreeTcb (Tcb, FREE_TCB_CMD_MARK_FOR_DELETION);
            return -1;
        }
        get_free_wrflag (Tcb->pid, &Tcb->wr_flag_mask);
        Tcb->ProcessId = ProcessId;

        InitWaitUntilProcessIsNotActive (Tcb);

        Tcb->TransmitMessageBuffer = (PIPE_API_BASE_CMD_MESSAGE*)my_calloc (PIPE_MESSAGE_BUFSIZE, 1);
        Tcb->ReceiveMessageBuffer = (PIPE_API_BASE_CMD_MESSAGE_ACK*)my_calloc (PIPE_MESSAGE_BUFSIZE, 1);
        if ((Tcb->ReceiveMessageBuffer == NULL) || (Tcb->ReceiveMessageBuffer  == NULL)) {
            ThrowError (1, "out of memory!");
            FreeTcb (Tcb, FREE_TCB_CMD_MARK_FOR_DELETION);
            return -1;
        }

        ConnectProcessToAllAssociatedBarriers (Tcb,
                                               BarriersBeforeOnlySignal,
                                               BarriersBeforeSignalAndWait,
                                               BarriersBehindOnlySignal,
                                               BarriersBehindSignalAndWait,
                                               BarriersLoopOutBeforeOnlySignal,
                                               BarriersLoopOutBeforeSignalAndWait,
                                               BarriersLoopOutBehindOnlySignal,
                                               BarriersLoopOutBehindSignalAndWait);
        if (((Tcb->RangeControlFlags & RANGE_CONTROL_BEFORE_ACTIVE_FLAG ) == RANGE_CONTROL_BEFORE_ACTIVE_FLAG) ||
            ((Tcb->RangeControlFlags & RANGE_CONTROL_BEHIND_ACTIVE_FLAG ) == RANGE_CONTROL_BEHIND_ACTIVE_FLAG)) {
            if ((Tcb->RangeControlFlags & RANGE_CONTROL_COUNTER_VARIABLE_FLAG) == RANGE_CONTROL_COUNTER_VARIABLE_FLAG) {
                Tcb->RangeCounterVid = add_bbvari (RangeControlCounterVariable, BB_UQWORD, "");
                if (Tcb->RangeCounterVid < 0) {
                    ThrowError (1, "cannot add range control counter \"%s\" to blackboard", RangeControlCounterVariable);
                    Tcb->RangeControlFlags = 0;
                }
            }
            if ((Tcb->RangeControlFlags & RANGE_CONTROL_CONTROL_VARIABLE_FLAG) == RANGE_CONTROL_CONTROL_VARIABLE_FLAG) {
                Tcb->RangeControlVid = add_bbvari (RangeControlControlVariable, BB_UDWORD, "");
                if (Tcb->RangeControlVid < 0) {
                    ThrowError (1, "cannot add range control variable \"%s\" to blackboard", RangeControlControlVariable);
                    Tcb->RangeControlFlags = 0;
                } else {
                    set_bbvari_conversion(Tcb->RangeControlVid, 2, "0 0 \"RANGE_CONTROL_OFF\";"
                                                                   "1 1 \"RANGE_CONTROL_READ_AND_WRITE\";"
                                                                   "2 2 \"RANGE_CONTROL_ONLY_WRITE\";"
                                                                   "3 3 \"RANGE_CONTROL_ONLY_READ\";");
                }
            }
        }     
        if (DllName != NULL) {
            int Len = (int)strlen (DllName)+1;
            if (Len > 0) {
                char *Save = Tcb->DllName;  // If TCB was in use before store to be able to free memory
                Tcb->DllName = my_malloc (Len);
                if (Tcb->DllName != NULL) {
                    MEMCPY (Tcb->DllName, DllName, (size_t)Len);
                }
                if (Save != NULL) my_free (Save);
            } else {
                Tcb->DllName = NULL;
            }
        } else {
            Tcb->DllName = NULL;
        }
        if (ExecutableName != NULL) {
            int Len = (int)strlen (ExecutableName)+1;
            if (Len > 0) {
                char *Save = Tcb->InsideExecutableName;   // If TCB was in use before store to be able to free memory
                Tcb->InsideExecutableName = my_malloc (Len);
                if (Tcb->InsideExecutableName != NULL) {
                    MEMCPY (Tcb->InsideExecutableName, ExecutableName, (size_t)Len);
                }
                if (Save != NULL) my_free (Save);
            } else {
                Tcb->InsideExecutableName = NULL;
            }
        } else {
            Tcb->InsideExecutableName = NULL;
        }
        Tcb->NumberInsideExecutable = ProcessNumber;
        Tcb->NumberOfProcessesInsideExecutable = NumberOfProcesses;

        InitializeCriticalSection(&Tcb->CriticalSection);

        if (AddProcessToScheduler (Scheduler, Tcb)) {
            ThrowError (1, "cannot add process \"%s\" to unknown scheduler \"%s\"", ProcessName, Scheduler);
            Ret = -1;
        } else {
            Ret = Tcb->pid;
        }
    }
    return Ret;
}


static void CallExternProcessReferenceFunction(SCHEDULER_DATA *pSchedulerData,
                                               TASK_CONTROL_BLOCK *CurrentTcb,
                                               int CallFromCycic)
{
    if (CurrentTcb->Type == INTERN_ASYNC) {
        ThrowError (1, "refernce function dosn't exist on internel processes");
    } else if (CurrentTcb->Type == EXTERN_ASYNC) {
        int Ret;
        int SnapShotSize;
        int VirtualNetworkSnapShotSize;
        PIPE_API_CALL_FUNC_CMD_MESSAGE* pReq = (PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer;
        // Externer Prozess Appl.-Fenster updaten
        application_update_start_process (CurrentTcb->pid, CurrentTcb->name, CurrentTcb->DllName);

        // ist ein SVL-File angegeben welches vor dem Prozess-Start geladen werden soll
        {
            char FileName[MAX_PATH];
            int UpdateFlag;
            if (GetSVLFileLoadedBeforeInitProcessFileName (CurrentTcb->name, FileName) > 0) {
                SearchAndReplaceEnvironmentStrings (FileName, FileName, sizeof(FileName));
                WriteSVLToProcess(FileName, CurrentTcb->name);
            }
            if (GetA2LFileAssociatedProcessFileName (CurrentTcb->name, FileName, &UpdateFlag) > 0) {
                SearchAndReplaceEnvironmentStrings (FileName, FileName, sizeof(FileName));
                //CurrentTcb->A2LLinkNr = A2LLinkToExternProcess("C:\\UserData\\f23576\\A2L\\Samples\\Jan\\tesa_eva_tc38x.A2L", CurrentTcb->pid, UpdateFlag);
                CurrentTcb->A2LLinkNr = A2LLinkToExternProcess(FileName, CurrentTcb->pid, UpdateFlag);
            }
        }
        // der externe Prozessmuss auch Zugriff auf die RabgeConrol Variable haben
        if (CurrentTcb->RangeCounterVid > 0) {
            attach_bbvari(CurrentTcb->RangeCounterVid);
            write_bbvari_uqword(CurrentTcb->RangeCounterVid, 0);
        }
        if (CurrentTcb->RangeControlVid > 0) {
            uint32_t v = 1;
            if ((CurrentTcb->RangeControlFlags & RANGE_CONTROL_BEFORE_INIT_AS_DEACTIVE) == RANGE_CONTROL_BEFORE_INIT_AS_DEACTIVE) {
                if ((CurrentTcb->RangeControlFlags & RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE) == RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE) {
                    v = 0; // no range control are active
                } else {
                    v = 2; // behind range control are active
                }
            } else if ((CurrentTcb->RangeControlFlags & RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE) == RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE) {
                v = 3;  // before ist range control are active
            } else {
                v = 1;  // before and behind range control are active
            }
            attach_bbvari(CurrentTcb->RangeControlVid);
            write_bbvari_udword(CurrentTcb->RangeControlVid, v);
        }
        AddExternProcessTerminateOrResetBBVariable (CurrentTcb->pid);

        BuildExternProcessReferenceFilter (CurrentTcb->pid, CurrentTcb->name);

        if (CallFromCycic) {
            WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
        }
        SnapShotSize = CopyFromBlackbardToPipe (CurrentTcb, &(((PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer)->SnapShotData[0]));
        if (CurrentTcb->VirtualNetworkHandleCount > 0) {
            int MaxSize = (PIPE_MESSAGE_BUFSIZE - sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK) - 1) - SnapShotSize;
            VirtualNetworkSnapShotSize = VirtualNetworkCopyFromFifosToPipe(CurrentTcb, &pReq->SnapShotData[SnapShotSize], MaxSize);
        } else {
            VirtualNetworkSnapShotSize = 0;
        }
        WaitUntilProcessIsNotLocked (CurrentTcb, __FILE__, __LINE__);
        if (CallFromCycic) {
            WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 0);
        }
        Ret = CallExternProcessFunction (PIPE_API_CALL_REFERENCE_FUNC_CMD, pSchedulerData->Cycle, CurrentTcb,
                                        pSchedulerData->pTransmitMessageBuffer, SnapShotSize,
                                        (int32_t)sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE) + SnapShotSize + VirtualNetworkSnapShotSize,
                                        pSchedulerData->pReceiveMessageBuffer);
        if (CallFromCycic) {
            WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
        }
        if (Ret == 0) {
            CopyFromPipeToBlackboard (CurrentTcb, &(((PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer)->SnapShotData[0]),
                                     ((PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer)->SnapShotSize);
        }
        EndOfProcessCycle (CurrentTcb);
        if (CallFromCycic) {
            WalkThroughBarrierBehindProcess (CurrentTcb, 1, (Ret != 0));
        }
        if (Ret) {
            RemoveExternProcessTerminateOrResetBBVariable (CurrentTcb->pid);
            // Ret == -2 if disconnect without terminate signal
            KillProcess (pSchedulerData, CurrentTcb, (Ret == -2) ? 0 : 1, 1);
        } else {
            rereference_all_vari_from_ini (CurrentTcb->pid, 0, 0);
            CurrentTcb->state = EX_PROCESS_INIT;
        }
    } else {
        ThrowError (1, "internal processes have no reference function");
    }
}

void CheckEquationBeforeBehindProcess (char *name);

static void CallProcessInitFunction(SCHEDULER_DATA *pSchedulerData,
                                    TASK_CONTROL_BLOCK *CurrentTcb,
                                    int CallFromCycic)
{
    if (CurrentTcb->Type == INTERN_ASYNC) {
        if (CallFromCycic) {
            WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
            CheckEquationBeforeBehindProcess (CurrentTcb->name);
            ClacBeforeProcessEquations (CurrentTcb);
            WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 0);
        }
        CurrentTcb->init ();
        if (CallFromCycic) {
            WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
            ClacBehindProcessEquations (CurrentTcb);
            WalkThroughBarrierBehindProcess (CurrentTcb, 1, 0);
        }
    } else if (CurrentTcb->Type == EXTERN_ASYNC) {
        int Ret;
        int SnapShotSize;
        int VirtualNetworkSnapShotSize;
        PIPE_API_CALL_FUNC_CMD_MESSAGE* pReq = (PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer;
        if (CallFromCycic) {
            WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
            CheckEquationBeforeBehindProcess (CurrentTcb->name);
            ClacBeforeProcessEquations (CurrentTcb);
        }
        SnapShotSize = CopyFromBlackbardToPipe (CurrentTcb, &(((PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer)->SnapShotData[0]));
        if (CurrentTcb->VirtualNetworkHandleCount > 0) {
            int MaxSize = (PIPE_MESSAGE_BUFSIZE - sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK) - 1) - SnapShotSize;
            VirtualNetworkSnapShotSize = VirtualNetworkCopyFromFifosToPipe(CurrentTcb, &pReq->SnapShotData[SnapShotSize], MaxSize);
        } else {
            VirtualNetworkSnapShotSize = 0;
        }
        WaitUntilProcessIsNotLocked (CurrentTcb, __FILE__, __LINE__);
        if (CallFromCycic) {
            WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 0);
        }
        Ret = CallExternProcessFunction (PIPE_API_CALL_INIT_FUNC_CMD, pSchedulerData->Cycle, CurrentTcb,
                                        pSchedulerData->pTransmitMessageBuffer, SnapShotSize,
                                        (int32_t)sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE) + SnapShotSize + VirtualNetworkSnapShotSize,
                                        pSchedulerData->pReceiveMessageBuffer);
        if (CallFromCycic) {
            WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
        }
        if (Ret == 0) {
            CopyFromPipeToBlackboard (CurrentTcb, &(((PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer)->SnapShotData[0]),
                                     ((PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer)->SnapShotSize);
        }
        ClacBehindProcessEquations (CurrentTcb);
        EndOfProcessCycle (CurrentTcb);
        if (CallFromCycic) {
            WalkThroughBarrierBehindProcess (CurrentTcb, 1, (Ret != 0));
        }
        if (Ret) {
            RemoveExternProcessTerminateOrResetBBVariable (CurrentTcb->pid);
            // Ret == -2 if disconnect without terminate signal
            KillProcess (pSchedulerData, CurrentTcb, (Ret == -2) ? 0 : 1, 1);
        }
    }
    CurrentTcb->state = PROCESS_AKTIV;
}

static void CheckProcessLoginToScheduler (SCHEDULER_DATA *pSchedulerData)
{
    TASK_CONTROL_BLOCK *pTcb;
    TASK_CONTROL_BLOCK *pCurrntTcbSave;

    while ((pTcb = AddProcessesToSchedPtrs[pSchedulerData->SchedulerNr][AddProcessesToSchedRdPos[pSchedulerData->SchedulerNr]]) != NULL) {
        AddProcessesToSchedPtrs[pSchedulerData->SchedulerNr][AddProcessesToSchedRdPos[pSchedulerData->SchedulerNr]] = NULL;
        ActivateProcessBarriers (pTcb);
        InsertProcessToSchedulerList (pSchedulerData, pTcb);

        pCurrntTcbSave = pSchedulerData->CurrentTcb;
        pSchedulerData->CurrentTcb = pTcb;
        if (pTcb->Type == EXTERN_ASYNC) {
            pTcb->state = EX_PROCESS_REFERENCE;
            if (!s_main_ini_val.SeparateCyclesForRefAndInitFunction) {
                // we have to do it here and not inside the scheduler function
                CallExternProcessReferenceFunction(pSchedulerData,
                                                   pTcb,
                                                   0);
                CallProcessInitFunction(pSchedulerData,
                                        pTcb,
                                        0);
            }
        } else {
            pTcb->state = EX_PROCESS_INIT;
            if (!s_main_ini_val.SeparateCyclesForRefAndInitFunction) {
                // we have to do it here and not inside the scheduler function
                CallProcessInitFunction(pSchedulerData,
                                        pTcb,
                                        0);
            }
        }
        pSchedulerData->CurrentTcb = pCurrntTcbSave;

        if (AddProcessesToSchedRdPos[pSchedulerData->SchedulerNr] >= (MAX_PIDS - 1)) {
            AddProcessesToSchedRdPos[pSchedulerData->SchedulerNr] = 0;
        } else {
            AddProcessesToSchedRdPos[pSchedulerData->SchedulerNr]++;
        }
    }
}

static void inc_timestamp_counter (SCHEDULER_DATA *pSchedulerData)
{
    pSchedulerData->Cycle++;
    pSchedulerData->CurrentTcb = &GuiTcb;
    write_bbvari_udword (pSchedulerData->CycleCounterVid, read_bbvari_udword (pSchedulerData->CycleCounterVid) + 1);
    pSchedulerData->CurrentTcb = NULL;

    pSchedulerData->SimulatedTimeSinceStartedInNanoSecond += (uint64_t)pSchedulerData->SchedPeriodInNanoSecond;
    pSchedulerData->SimulatedTimeInNanoSecond += (uint64_t)pSchedulerData->SchedPeriodInNanoSecond;
}

void CheckEquationBeforeBehindProcess (char *name)
{
    char EquFile[MAX_PATH];

    if (GetBeforeProcessEquationFileName (name, EquFile)) {
        char *ErrString = NULL;
        if (AddBeforeProcessEquationFromFile (0, name, EquFile, 1, &ErrString)) {
            if (ErrString != NULL) FreeErrStringBuffer (&ErrString);
            if (ThrowError (ERROR_OKCANCEL, "cannot read equation file %s witch should execute before process %s\n press OK to delete it", EquFile, name) == IDOK) {
                DelBeforeProcessEquationFile (name);
            }
        }
    }
    if (GetBehindProcessEquationFileName (name, EquFile)) {
        char *ErrString = NULL;
        if (AddBehindProcessEquationFromFile (0, name, EquFile, 1, &ErrString)) {
            if (ErrString != NULL) FreeErrStringBuffer (&ErrString);
            if (ThrowError (ERROR_OKCANCEL, "cannot read equation file %s witch should execute behind process %s\n press OK to delete it", EquFile, name) == IDOK) {
                DelBehindProcessEquationFile (name);
            }
        }
    }
}


FILE *DebugTermSchedFh;
static int SyncWaitAllSchedulerTerminateCounter;
static int AllSchedulerShouldBeTerminatedFlag;
static int AllSchedulerShouldBeTerminatedAcceptedFlag;

void SetToTerminateSchedulerCount(void)
{
    int x;
    // if this file is created a scheduler logging is created
    //DebugTermSchedFh = fopen("c:\\temp\\term_sched.txt", "at");
    if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "\n\nTerminate schedulers log:\n");
    EnterCriticalSection(&SyncStartSchedulerCriticalSection);
    SyncWaitAllSchedulerTerminateCounter = 0;
    for (x = 0; x < SchedulerCount; x++) {
        if (SchedulerData[x].State != SCHED_EXTERNAL_ONLY_INFO) {
            SyncWaitAllSchedulerTerminateCounter++;
        }
    }
    LeaveCriticalSection(&SyncStartSchedulerCriticalSection);
}

static int CheckIfAllSchedulerHaveRecogizedTerminationRequest(SCHEDULER_DATA *pSchedulerData)
{
    int x;
    int Ret = 1;
    EnterCriticalSection(&SyncStartSchedulerCriticalSection);
    if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "TS %u\n", (unsigned int)GetTickCount());
    if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "scheduler \"%s\" check if all other schedulers have recogized term req\n", pSchedulerData->SchedulerName);

    for (x = 0; x < SchedulerCount; x++) {
        if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "   scheduler \"%s\" = %i, %i\n", SchedulerData[x].SchedulerName, SchedulerData[x].SchedulerHaveRecogizedTerminationRequeFlag, (int)SchedulerData[x].Cycle);
        if ((SchedulerData[x].State != SCHED_EXTERNAL_ONLY_INFO) &&
            !SchedulerData[x].SchedulerHaveRecogizedTerminationRequeFlag) Ret = 0;
    }
    pSchedulerData->SchedulerHaveRecogizedTerminationRequeFlag++;

    if (Ret) {
        if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "scheduler \"%s\" all other schedulers have recogized term req\n", pSchedulerData->SchedulerName);
        AllSchedulerShouldBeTerminatedAcceptedFlag = 1;
    }
    LeaveCriticalSection(&SyncStartSchedulerCriticalSection);
    return Ret;
}


#ifdef _WIN32
static DWORD WINAPI SchedulerThreadProc (LPVOID lpParam);
static DWORD WINAPI SchedulerThreadProcTryCatch (LPVOID lpParam)
{
    SCHEDULER_DATA *pSchedulerData;
    pSchedulerData = (SCHEDULER_DATA*)lpParam;
#if defined(_WIN32) && !defined(__GNUC__)
    __try {
#endif
        return SchedulerThreadProc(lpParam);
#if defined(_WIN32) && !defined(__GNUC__)
    } __except(ExceptionFilter(GetExceptionInformation(), pSchedulerData->SchedulerName)) {
        return -1;
    }
#endif
}

static DWORD WINAPI SchedulerThreadProc (LPVOID lpParam)
#else
static void *SchedulerThreadProc(void* lpParam)
#endif
{
    SCHEDULER_DATA *pSchedulerData;
    TASK_CONTROL_BLOCK *CurrentTcb;
    TASK_CONTROL_BLOCK *NextTcb;

    pSchedulerData = (SCHEDULER_DATA*)lpParam;
    pSchedulerData->SchedulerThreadId = GetCurrentThreadId ();
#ifdef _WIN32
    // Scheduler threads should have lower priority. so the GUI will get more CPU power if needed
    if (!SetThreadPriority (GetCurrentThread(), pSchedulerData->Prio)) {
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "cannot change scheduler \"%s\" priority to %i: (%s)\n", pSchedulerData->SchedulerName, pSchedulerData->Prio, lpMsgBuf);
        LocalFree (lpMsgBuf);
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_IDLE);
    }
#else
    // Scheduler-Threads haben niedere Prio. Damit GUI immer bedienbar bleibt!
    pthread_setschedprio(pthread_self(), -1 * pSchedulerData->Prio);   // Prioritaet negieren (Windows ist hier genau anderherum!)
#endif

    pSchedulerData->CurrentTcb = &GuiTcb;
    {
        char Help[MAX_SCHEDULER_NAME_LENGTH + 64];
        if (pSchedulerData->SchedulerNr == 0) {
            sprintf (Help, "%sCycleCounter", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD));
        } else {
            sprintf (Help, "%sCycleCounter_%s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD), pSchedulerData->SchedulerName);
        }
        pSchedulerData->CycleCounterVid = add_bbvari (Help, BB_UDWORD, "");
    }
    if (pSchedulerData->CycleCounterVid < 0) {
        ThrowError (1, "cannot add blackboard variable %sCycleCounter %i", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD), pSchedulerData->CycleCounterVid);
    }
    write_bbvari_udword (pSchedulerData->CycleCounterVid, 0);
    pSchedulerData->CurrentTcb = NULL;
    if (pSchedulerData->SchedulerNr == 0) {
        CycleCounterVid = pSchedulerData->CycleCounterVid; // The control panel will use this blackboard variable
    }

    // Wait till all other schedulers and all startup processes are active
    while (!SyncStartSchedulerFlag) {
        // Look if there are new processes
        CheckProcessLoginToScheduler (pSchedulerData);

        EnterCriticalSection(&SyncStartSchedulerCriticalSection);
        while (!SyncStartSchedulerWakeFlag) {
            SleepConditionVariableCS_INFINITE(&SyncStartSchedulerConditionVariable, &SyncStartSchedulerCriticalSection);
        }
        LeaveCriticalSection(&SyncStartSchedulerCriticalSection);
    }

    while (pSchedulerData->State != SCHED_TERMINATING_STATE) {  // Till the scheduler should be termnated
        while (pSchedulerData->State == SCHED_RUNNING_STATE) {  // Till the scheduler should not be stopped
            if (pSchedulerData->IsMainScheduler) {   // Only the first scheduler can be wait for a real time and can be stopped

                WaitForNextSchedulerCycle (pSchedulerData);

                if (pSchedulerData->BreakPointActive) {
                    if (CheckBreakPoint()) {
                        disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_USER, NULL, NULL);
                    }
                }

                ShouldSchedulerSleep(pSchedulerData);

            }
            if (1) {
                WalkThroughBarrierBeforeStartSchedulerCycle (pSchedulerData, 0, 0, CheckProcessLoginToScheduler);

                if (pSchedulerData->BarrierBeforeCount > 0) { // if the scheduler must walk through barrieren "before"
                    // look if scheduler should be terminate
                    if (AllSchedulerShouldBeTerminatedFlag) {
                        CheckIfAllSchedulerHaveRecogizedTerminationRequest(pSchedulerData);
                    } else {
                        WAIT_FOR_ALL_LOGIN_COMPLETE();
                        CheckProcessLoginToScheduler (pSchedulerData);
                    }
                }
                // increment Timestampcounter and cycle counter inside blackboard
                inc_timestamp_counter (pSchedulerData);
                if (pSchedulerData->SchedulerNr == 0) blackboard_infos.ActualCycleNumber++;

                WalkThroughBarrierBeforeStartSchedulerCycle (pSchedulerData, 1, 0, CheckProcessLoginToScheduler);

                // This will be used by Script for the WAIT_EXE command
                if (pSchedulerData->user_sched_func_flag) {
                    while (pSchedulerData->user_sched_func_flag) {
                        pSchedulerData->user_sched_func();
                    }
                } else {

                    pSchedulerData->CurrentTcb = CurrentTcb = pSchedulerData->FirstTcb;
                    // Loop through all process of this scheduler
                    while (CurrentTcb != NULL) {
                        NextTcb = pSchedulerData->CurrentTcb->next;
                        if (CurrentTcb->Active) {
                            switch (CurrentTcb->state) {
                            case PROCESS_AKTIV:
                                CurrentTcb->time_counter++;
                                if (CurrentTcb->time_counter >= CurrentTcb->time_steps) {
                                    CurrentTcb->time_counter = 0;
                                    CurrentTcb->call_count++;
                                    if (CurrentTcb->Type == INTERN_ASYNC) {
                                        WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
                                        ClacBeforeProcessEquations (CurrentTcb);
                                        WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 0);
                                        CurrentTcb->cyclic ();
                                        WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
                                        ClacBehindProcessEquations (CurrentTcb);
                                        WalkThroughBarrierBehindProcess (CurrentTcb, 1, 0);
                                    } else if (CurrentTcb->Type == EXTERN_ASYNC) {
                                        int Ret;
                                        int SnapShotSize;
                                        int VirtualNetworkSnapShotSize;
                                        CheckFetchDataAfterProcess(CurrentTcb->pid);
                                        PIPE_API_CALL_FUNC_CMD_MESSAGE* pReq = (PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer;
                                        WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
                                        WaitUntilProcessIsNotLocked (CurrentTcb, __FILE__, __LINE__);
                                        ClacBeforeProcessEquations (CurrentTcb);
                                        SnapShotSize = CopyFromBlackbardToPipe (CurrentTcb, &pReq->SnapShotData[0]);
                                        if (CurrentTcb->VirtualNetworkHandleCount > 0) {
                                            int MaxSize = (PIPE_MESSAGE_BUFSIZE - sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK) - 1) - SnapShotSize;

                                            VirtualNetworkSnapShotSize = VirtualNetworkCopyFromFifosToPipe(CurrentTcb, &pReq->SnapShotData[SnapShotSize], MaxSize);
                                        } else {
                                            VirtualNetworkSnapShotSize = 0;
                                        }
                                        WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 0);
                                        Ret = CallExternProcessFunction (PIPE_API_CALL_CYCLIC_FUNC_CMD, pSchedulerData->Cycle, CurrentTcb,
                                                                         pSchedulerData->pTransmitMessageBuffer,
                                                                         SnapShotSize,
                                                                         (int32_t)sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE) + SnapShotSize + VirtualNetworkSnapShotSize,
                                                                         pSchedulerData->pReceiveMessageBuffer);
                                        WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
                                        if (Ret == 0) {
                                            int VirtualNetworkSnapShotSize;
                                            PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK* pAck = (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer;
                                            CopyFromPipeToBlackboard (CurrentTcb, &pAck->SnapShotData[0],
                                                                      pAck->SnapShotSize);

                                            VirtualNetworkSnapShotSize = pAck->StructSize - pAck->SnapShotSize - (int32_t)sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK);
                                            if (VirtualNetworkSnapShotSize > 0) {
                                                VirtualNetworkCopyFromPipeToFifos (CurrentTcb,
                                                                                   GetSimulatedTimeInNanoSecond(),
                                                                                   &(pAck->SnapShotData[pAck->SnapShotSize]), VirtualNetworkSnapShotSize);
                                            }
                                        }
                                        ClacBehindProcessEquations (CurrentTcb);
                                        EndOfProcessCycle (CurrentTcb);
                                        WalkThroughBarrierBehindProcess (CurrentTcb, 1, (Ret != 0));
                                        CheckFetchDataAfterProcess(CurrentTcb->pid);
                                        if (Ret) {
                                            RemoveExternProcessTerminateOrResetBBVariable (CurrentTcb->pid);
                                            // Ret == -2 if disconnect without terminate signal
                                            KillProcess (pSchedulerData, CurrentTcb, (Ret == -2) ? 0 : 1, 1);
                                        }
                                    }
                                } else {
                                    // Even the process should not run this cycle go through the barriers
                                    WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
                                    WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 0);
                                    WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
                                    WalkThroughBarrierBehindProcess (CurrentTcb, 1, 0);
                                }
                                break;
                            case EX_PROCESS_REFERENCE:
                                CallExternProcessReferenceFunction(pSchedulerData,
                                                                   CurrentTcb,
                                                                   1);

                                break;
                            case EX_PROCESS_INIT: // this should be rename to PROCESS_INIT
                                CallProcessInitFunction(pSchedulerData,
                                                        CurrentTcb,
                                                        1);

                                break;
                            case EX_PROCESS_TERMINATE: // this should be rename to PROCESS_TERMINATE
                                if (CurrentTcb->Type == INTERN_ASYNC) {
                                    WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
                                    ClacBeforeProcessEquations (CurrentTcb);
                                    WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 0);
                                    CurrentTcb->terminat ();
                                    WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
                                    ClacBehindProcessEquations (CurrentTcb);
                                    WalkThroughBarrierBehindProcess (CurrentTcb, 1, 1);
                                } else if (CurrentTcb->Type == EXTERN_ASYNC) {
                                    int Ret;
                                    int SnapShotSize;
                                    int VirtualNetworkSnapShotSize;
                                    PIPE_API_CALL_FUNC_CMD_MESSAGE* pReq = (PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer;
                                    WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
                                    ClacBeforeProcessEquations (CurrentTcb);
                                    SnapShotSize = CopyFromBlackbardToPipe (CurrentTcb, &(((PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer)->SnapShotData[0]));
                                    if (CurrentTcb->VirtualNetworkHandleCount > 0) {
                                        int MaxSize = (PIPE_MESSAGE_BUFSIZE - sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK) - 1) - SnapShotSize;
                                        VirtualNetworkSnapShotSize = VirtualNetworkCopyFromFifosToPipe(CurrentTcb, &pReq->SnapShotData[SnapShotSize], MaxSize);
                                    } else {
                                        VirtualNetworkSnapShotSize = 0;
                                    }
                                    WaitUntilProcessIsNotLocked (CurrentTcb, __FILE__, __LINE__);
                                    WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 0);
                                    Ret = CallExternProcessFunction (PIPE_API_CALL_TERMINATE_FUNC_CMD, pSchedulerData->Cycle, CurrentTcb,
                                                                     pSchedulerData->pTransmitMessageBuffer, SnapShotSize,
                                                                     (int32_t)sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE) + SnapShotSize + VirtualNetworkSnapShotSize,
                                                                     pSchedulerData->pReceiveMessageBuffer);
                                    WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
                                    if (Ret == 0) {
                                        CopyFromPipeToBlackboard (CurrentTcb, &(((PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer)->SnapShotData[0]),
                                                                  ((PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer)->SnapShotSize);
                                    }
                                    ClacBehindProcessEquations (CurrentTcb);

                                    WalkThroughBarrierBehindProcess (CurrentTcb, 1, Ret != 0);
                                    RemoveExternProcessTerminateOrResetBBVariable (CurrentTcb->pid);

                                    if (CurrentTcb->RangeCounterVid > 0) remove_bbvari(CurrentTcb->RangeCounterVid);
                                    if (CurrentTcb->RangeControlVid > 0) remove_bbvari(CurrentTcb->RangeControlVid);

                                    dereference_all_blackboard_variables (CurrentTcb->pid);
                                } 
                                KillProcess (pSchedulerData, CurrentTcb, 1, 0);  // no emergency cleanup barriers
                                break;
                            case EX_PROCESS_WAIT_FOR_END_OF_CYCLE_TO_GO_TO_TERM:
                                 WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
                                 WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 1);
                                 CurrentTcb->state = EX_PROCESS_TERMINATE;
                                 WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
                                 WalkThroughBarrierBehindProcess (CurrentTcb, 1, 1);
                                 break;
                            default:
                                ThrowError (1, "process %s are in unknown state %i",  CurrentTcb->name, CurrentTcb->state);
                                break;
                            }
                        }
                         pSchedulerData->CurrentTcb = CurrentTcb = NextTcb;
                    }
                }
                WalkThroughBarrierBehindEndOfSchedulerCycle (pSchedulerData, 0, 0, CheckProcessLoginToScheduler);

                if (pSchedulerData->BarrierBeforeCount == 0) { // if the scheduler are not walked trow the barriers "before"
                    // look if scheduler should be terminate
                    if (AllSchedulerShouldBeTerminatedFlag) {
                        CheckIfAllSchedulerHaveRecogizedTerminationRequest(pSchedulerData);
                    } else {
                        WAIT_FOR_ALL_LOGIN_COMPLETE();
                        CheckProcessLoginToScheduler (pSchedulerData);
                    }
                }

                WalkThroughBarrierBehindEndOfSchedulerCycle (pSchedulerData, 1, 0, CheckProcessLoginToScheduler);

                if (AllSchedulerShouldBeTerminatedAcceptedFlag) {
                    if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "scheduler \"%s\" terminate now\n", pSchedulerData->SchedulerName);
                    if (pSchedulerData->State != SCHED_EXTERNAL_ONLY_INFO) {
                        pSchedulerData->State = SCHED_TERMINATING_STATE;
                    }
                }
            }
        }
    }

    // Wait here till all other schedulers reach this point.
    EnterCriticalSection(&SyncStartSchedulerCriticalSection);
    SyncWaitAllSchedulerTerminateCounter--;
    if (SyncWaitAllSchedulerTerminateCounter > 0) {
        while (SyncWaitAllSchedulerTerminateCounter > 0) {
            if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "scheduler \"%s\" 0x%X went to sleep: %i\n", pSchedulerData->SchedulerName, (unsigned int)GetCurrentThreadId(), SyncWaitAllSchedulerTerminateCounter);
            SleepConditionVariableCS_INFINITE(&SyncStartSchedulerConditionVariable, &SyncStartSchedulerCriticalSection);
        }
    } else {
        if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "scheduler \"%s\" 0x%X wakeup all\n", pSchedulerData->SchedulerName, (unsigned int)GetCurrentThreadId());
        WakeAllConditionVariable(&SyncStartSchedulerConditionVariable);
    }
    if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "scheduler \"%s\" 0x%X are wakeup\n", pSchedulerData->SchedulerName, (unsigned int)GetCurrentThreadId());
    LeaveCriticalSection(&SyncStartSchedulerCriticalSection);

    // Now we are sure all scheduler are in terminate state

    WalkThroughBarrierBeforeStartSchedulerCycle (pSchedulerData, 0, 0, NULL);

    // increment Timestampcounter and cycle counter inside blackboard
    inc_timestamp_counter (pSchedulerData);
    if (pSchedulerData->SchedulerNr == 0) blackboard_infos.ActualCycleNumber++;

    WalkThroughBarrierBeforeStartSchedulerCycle (pSchedulerData, 1, 1, NULL);

    pSchedulerData->CurrentTcb = CurrentTcb = pSchedulerData->FirstTcb;

    // Walk through all processes of this scheduler
    while (CurrentTcb != NULL) {
        NextTcb = pSchedulerData->CurrentTcb->next;
        if (CurrentTcb->Type == INTERN_ASYNC) {
            WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
            WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 1);
            CurrentTcb->terminat ();
            WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
            WalkThroughBarrierBehindProcess (CurrentTcb, 1, 1);
        } else if (CurrentTcb->Type == EXTERN_ASYNC) {
            int Ret;
            int SnapShotSize;
            int VirtualNetworkSnapShotSize;
            PIPE_API_CALL_FUNC_CMD_MESSAGE* pReq = (PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer;
            WalkThroughBarrierBeforeProcess (CurrentTcb, 0, 0);
            ClacBeforeProcessEquations (CurrentTcb);
            SnapShotSize = CopyFromBlackbardToPipe (CurrentTcb, &(((PIPE_API_CALL_FUNC_CMD_MESSAGE*)pSchedulerData->pTransmitMessageBuffer)->SnapShotData[0]));
            if (CurrentTcb->VirtualNetworkHandleCount > 0) {
                int MaxSize = (PIPE_MESSAGE_BUFSIZE - sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK) - 1) - SnapShotSize;
                VirtualNetworkSnapShotSize = VirtualNetworkCopyFromFifosToPipe(CurrentTcb, &pReq->SnapShotData[SnapShotSize], MaxSize);
            } else {
                VirtualNetworkSnapShotSize = 0;
            }
            WaitUntilProcessIsNotLocked (CurrentTcb, __FILE__, __LINE__);
            WalkThroughBarrierBeforeProcess (CurrentTcb, 1, 1);
            Ret = CallExternProcessFunction (PIPE_API_CALL_TERMINATE_FUNC_CMD, pSchedulerData->Cycle, CurrentTcb,
                                             pSchedulerData->pTransmitMessageBuffer, SnapShotSize,
                                             (int32_t)sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE) + SnapShotSize + VirtualNetworkSnapShotSize,
                                             pSchedulerData->pReceiveMessageBuffer);

            WalkThroughBarrierBeforeLoopOut(CurrentTcb, NULL, 0, 0);
            WalkThroughBarrierBeforeLoopOut(CurrentTcb, NULL, 1, 0);
            WalkThroughBarrierBehindLoopOut(CurrentTcb, NULL, 0, 0);
            WalkThroughBarrierBehindLoopOut(CurrentTcb, NULL, 1, 0);

            WalkThroughBarrierBehindProcess (CurrentTcb, 0, 0);
            if (Ret == 0) {
                CopyFromPipeToBlackboard (CurrentTcb, &(((PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer)->SnapShotData[0]),
                                          ((PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)pSchedulerData->pReceiveMessageBuffer)->SnapShotSize);
            }
            ClacBehindProcessEquations (CurrentTcb);
            EndOfProcessCycle (CurrentTcb);

            WalkThroughBarrierBehindProcess (CurrentTcb, 1, 1);
            RemoveExternProcessTerminateOrResetBBVariable (CurrentTcb->pid);
            dereference_all_blackboard_variables (CurrentTcb->pid);
        }
        KillProcess (pSchedulerData, CurrentTcb, 1, 0);  // no emergency cleanup barriers
        pSchedulerData->CurrentTcb = CurrentTcb = NextTcb;
    }
    WalkThroughBarrierBehindEndOfSchedulerCycle (pSchedulerData, 0, 0, NULL);
    WalkThroughBarrierBehindEndOfSchedulerCycle (pSchedulerData, 1, 1, NULL);  // Additional remove the connections to the barriers

    // Now remove all connections to the barriers
    DisconnectSchedulerFromAllBarriers (pSchedulerData);

    pSchedulerData->State = SCHED_IS_TERMINATED_STATE;

    if (pSchedulerData->pTransmitMessageBuffer != NULL) my_free(pSchedulerData->pTransmitMessageBuffer);
    if (pSchedulerData->pReceiveMessageBuffer != NULL) my_free(pSchedulerData->pReceiveMessageBuffer);

    CloseSocketToRemoteMasterForThisThead();

    return 0;
}


static int CreateSchedulerThread (SCHEDULER_DATA *par_Scheduler)
{
#ifdef _WIN32
    HANDLE hThread;
    DWORD dwThreadId; 

    hThread = CreateThread (NULL,              // No security attribute
                            256*1024,          // Stack size
                            SchedulerThreadProcTryCatch,
                            (void*)par_Scheduler,
                            0,
                            &dwThreadId);
    if (hThread == NULL)  {
        char *lpMsgBuf;
        DWORD dw = GetLastError(); 
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "cannot create scheduler thread (%s)\n", lpMsgBuf);
        LocalFree (lpMsgBuf);
        return -1;
    }
#else
    pthread_attr_t attr;
    pthread_t ThreadId;
    int ret;

    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        return -1;
    }

    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, 256*1024);
    if (ret) {
        printf("pthread setstacksize failed\n");
        return -1;
    }

    if (pthread_create(&ThreadId, &attr, SchedulerThreadProc, (void*)par_Scheduler) < 0) {
        perror("could not create scheduler thread");
        return -1;
    }
    pthread_attr_destroy(&attr);
#endif
    return 0;
}

static char *CallFromProcess;

void SetCallFromProcess (char *par_CallFromProcess)
{
    CallFromProcess = my_malloc (strlen (par_CallFromProcess)+ 1);
    strcpy (CallFromProcess, par_CallFromProcess);
}

static int ToHostPCFifoHandle;

int GetToHostPCFifoHandle(void)
{
    return ToHostPCFifoHandle;
}

int InitPipeSchedulers (char *par_Prefix)
{
    int p, p2, s;
    char Entry[64];
    char SchedulerName[MAX_PATH];
    char Name[BBVARI_NAME_SIZE];
    char ProcessName[MAX_PATH];
    int abt_frq_vid;
    int abt_per_vid;
    int VersionVid;
    int realtime_vid;
    double period;
    char *StartedProcesses[MAX_EXTERN_PROCESSES];
    int Ignore;
    char *WaitForProcessesBuffer = NULL;
    char *WaitForProcesses[64];
    int WaitForProcessesCount = 0;

    // Try to set the Windows Scheduler to 1ms time slices
#ifdef _WIN32
    timeBeginPeriod(1);
#endif

#ifndef REMOTE_MASTER
    if (s_main_ini_val.ConnectToRemoteMaster) {
        int rp, r;
        // Remove the realtime processes from the list
        char *RealtimeProcesses[] = {TRACE_QUEUE, STIMULI_QUEUE, "CANServer", SIGNAL_GENERATOR_CALCULATOR, EQUATION_CALCULATOR, EQUATION_COMPILER, SIGNAL_GENERATOR_COMPILER, NULL};
        for (rp = 0; RealtimeProcesses[rp] != NULL; rp++) {
            for (r = 0; all_known_tasks[r] != NULL; r++) {
                if (!strcmp (all_known_tasks[r]->name, RealtimeProcesses[rp])) {
                    for (; all_known_tasks[r] != NULL; r++) {
                        all_known_tasks[r] = all_known_tasks[r + 1];
                    }
                    break;
                }
            }
        }
        if (1) {
            char *RenameProcessesFrom[] = {"rm_" EQUATION_COMPILER, "rm_" SIGNAL_GENERATOR_COMPILER, NULL};
            char *RenameProcessesTo[] = {EQUATION_COMPILER, SIGNAL_GENERATOR_COMPILER, NULL};
            for (rp = 0; RenameProcessesFrom[rp] != NULL; rp++) {
                for (r = 0; all_known_tasks[r] != NULL; r++) {
                    if (!strcmp (all_known_tasks[r]->name, RenameProcessesFrom[rp])) {
                        strcpy (all_known_tasks[r]->name, RenameProcessesTo[rp]);
                        break;
                    }
                }
            }
        }

        for (r = 0; all_known_tasks[r] != NULL; r++) {
            TASK_CONTROL_BLOCK *tcb = all_known_tasks[r];
            printf ("name = %s", tcb->name);
        }

        // The "ToHostPCFifo" fifo have to be constructed before the realtime scheduler will be setup
        // The realtime scheduler will be added blackboard variable and this will write to this fifo
        ToHostPCFifoHandle = CreateNewRxFifo(1/*will not used*/, 32*1024*1024, "ToHostPCFifo");
        if (ToHostPCFifoHandle < 0) {
            ThrowError (1, "cannot create \"ToHostPCFifo\"");
        }

        if (rm_AddRealtimeScheduler("RealtimeScheduler", s_main_ini_val.SchedulerPeriode, s_main_ini_val.SyncWithFlexray)) {
            ThrowError (1, "cannot start \"RealtimeScheduler\"");
        }
    } else {
        // GUIProcess exists only with remote master
        int rp, r;
        char *RealtimeProcesses[] = {"GUIProcess", "rm_" EQUATION_COMPILER, "rm_" SIGNAL_GENERATOR_COMPILER, NULL};
        for (rp = 0; RealtimeProcesses[rp] != NULL; rp++) {
            for (r = 0; all_known_tasks[r] != NULL; r++) {
                if (!strcmp (all_known_tasks[r]->name, RealtimeProcesses[rp])) {
                    for (; all_known_tasks[r] != NULL; r++) {
                        all_known_tasks[r] = all_known_tasks[r + 1];
                    }
                    break;
                }
            }
        }
    }
#endif

    // First reserve the Pid for the GUI process
    GuiTcb.pid = GetOrFreeUniquePid (GENERATE_UNIQUE_PID_COMMAND, 0, GuiTcb.name);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        rm_AddProcessToPidNameArray(GuiTcb.pid, GuiTcb.name, GuiTcb.message_size);
    }
    get_bb_accessmask (GuiTcb.pid, &(GuiTcb.bb_access_mask), NULL);
    InitExternProcessMessages (par_Prefix,
                               IniFileDataBaseReadYesNo(OPT_SECTION, "WritePipeCommunicationLogFile", 0, GetMainFileDescriptor()),
                               GetExternProcessLoginSocketPort());

    InitAllBarriers ();

    InitExternProcessTimeouts();

    // Start all schedulers simultaneous
    SyncStartSchedulerFlag = 0;
    InitializeCriticalSection(&SyncStartSchedulerCriticalSection);
    InitializeConditionVariable(&SyncStartSchedulerConditionVariable);
    SyncStartSchedulerWakeFlag = 0;


    // The main scheduler will be started always
    AddPipeSchedulers ("Scheduler", 1);

    for (s = 1; ; s++) {
        sprintf (Entry, "Scheduler_%i", s);
        if (IniFileDataBaseReadString (SCHED_INI_SECTION, Entry, "", SchedulerName, sizeof (SchedulerName), GetMainFileDescriptor()) <= 0) {
            break;
        }
        AddPipeSchedulers (SchedulerName, 0);
    }

    // Wait till all scheduler threads active
    while (1) {
        int s;
        for (s = 0; s < SchedulerCount; s++) {
            if (SchedulerData[s].State == SCHED_OFF_STATE) {
#ifdef _WIN32
                Sleep (10);   // wait 10ms
#else
                usleep(10*1000);
#endif
                break;
            }
        }
        if (s == SchedulerCount) break;
    }

    if (s_main_ini_val.ConnectToRemoteMaster) {
        start_process("RemoteMasterControl");
    }
    // Add all importand blackboard variable before the init process would be started
    abt_frq_vid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_FREQUENCY),
                              BB_DOUBLE, "Hz");
    abt_per_vid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME),
                              BB_DOUBLE, "s");
    VersionVid = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD, "Version", Name, sizeof(Name)),
                             BB_UWORD, "");
    realtime_vid = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD, "Realtime", Name, sizeof(Name)),
                               BB_UWORD, "");
    add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD, "exit", Name, sizeof(Name)),
                BB_UWORD, "");
    add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD, "ExitCode", Name, sizeof(Name)),
                BB_DWORD, "");
    add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".RealtimeFactor", Name, sizeof(Name)),
                BB_DOUBLE, "-");

    period = IniFileDataBaseReadFloat ("Scheduler", "Period", 0.01, GetMainFileDescriptor());
    if (period > 10.0) period = 10.0;
    if (period < 0.000001) period = 0.000001;
    write_bbvari_double (abt_frq_vid, 1.0/period);
    write_bbvari_double (abt_per_vid, period);
    write_bbvari_uword (VersionVid, XILENV_VERSION);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        write_bbvari_uword (realtime_vid, 1);
        start_process("GUIProcess");
    } else {
        write_bbvari_uword (realtime_vid, 0);
    }

    if (CallFromProcess != NULL) {
        char *p, *e;
        int End = 0;

        WaitForProcessesBuffer = my_malloc (strlen(CallFromProcess)+1);
        strcpy (WaitForProcessesBuffer, CallFromProcess);
        // This can be a liste of ',' separated process namens
        for (WaitForProcessesCount = 0, p = WaitForProcessesBuffer; !End && (WaitForProcessesCount < 64); WaitForProcessesCount++) {
            WaitForProcesses[WaitForProcessesCount] = p;
            e = p;
            while ((*e != 0) && (*e != ',')) e++;
            End = (*e == 0);
            *e = 0;

            AddExecutableToTimeoutControl (WaitForProcesses[WaitForProcessesCount], s_main_ini_val.ExternProcessLoginTimeout);

            p = e + 1;
        }
    }
    // Activate all startup processes
    for (p = 0; p < MAX_EXTERN_PROCESSES; p++) {
        sprintf (Entry, "P%i", p);
        IniFileDataBaseReadString ("InitStartProcesses", Entry, "",
                                   ProcessName,
                                   MAX_PATH, GetMainFileDescriptor());
        SearchAndReplaceEnvironmentStrings (ProcessName, ProcessName, sizeof (ProcessName));
        StartedProcesses[p] = (char*)my_malloc (strlen (ProcessName) + 1);
        strcpy (StartedProcesses[p], ProcessName);
        if (strlen (ProcessName)) {
            Ignore = 0;
            for (p2 = 0; p2 < p; p2++) {
                if (!Compare2ExecutableNames (ProcessName, StartedProcesses[p2])) {
                    ThrowError (1, "process \"%s\" already started from INI it lives there twice, this will be ignored", ProcessName);
                    Ignore = 1;
                }
            }
            if (!Ignore) {
                int StartProcessFlag = 1;
                // If XilEnv was started from an external process this process should not started from this list
                if (WaitForProcessesCount > 0) {
                    int x;
                    for (x = 0; x < WaitForProcessesCount; x++) {
                        if (!Compare2ExecutableNames (ProcessName, WaitForProcesses[x])) {
                            StartProcessFlag = 0;   // Don't start process it is already running
                        }
                    }
                }
                if (StartProcessFlag) {
                    if (start_process (ProcessName) < 0) {
                        ThrowError (1, "cannot start prozess %s",ProcessName);
                    }
                }
            }
        }
    }
    for (p = 0; p < MAX_EXTERN_PROCESSES; p++) {
        if (StartedProcesses[p] != NULL) my_free(StartedProcesses[p]);
    }
    if (WaitForProcessesBuffer != NULL) my_free (WaitForProcessesBuffer);

    return 0;
}


void SchedulersStartingShot (void)
{
    SyncStartSchedulerFlag = 1;
    EnterCriticalSection(&SyncStartSchedulerCriticalSection);
    SyncStartSchedulerWakeFlag = 1;
    LeaveCriticalSection(&SyncStartSchedulerCriticalSection);
    WakeAllConditionVariable(&SyncStartSchedulerConditionVariable);
}

static int read_schedulerinfos_from_ini (double *schedperiode)
{
    double Period;

    // Read period from the INI file
    Period = IniFileDataBaseReadFloat (SCHED_INI_SECTION, PERIODE_INI_ENTRY, 0.01, GetMainFileDescriptor());

    *schedperiode = Period;

    return 0;
}


int AddPipeSchedulers (char *par_SchedulerName, int par_IsMainScheduler)
{
    SCHEDULER_DATA *pScheduler;
#ifdef _WIN32
    SYSTEMTIME LocalTime;
#endif
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char BarriersBeforeOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersBeforeSignalAndWait[INI_MAX_LINE_LENGTH];
    char BarriersBehindOnlySignal[INI_MAX_LINE_LENGTH];
    char BarriersBehindSignalAndWait[INI_MAX_LINE_LENGTH];
    uint64_t SystemTypeIn100NanoSecond;
    int Fd = GetMainFileDescriptor();

    pScheduler = RegisterScheduler  (par_SchedulerName);
    if (pScheduler == NULL) return -1;
    InitStopRequest(&(pScheduler->StopReq));

    pScheduler->IsMainScheduler = par_IsMainScheduler;

    sprintf (Entry, "PriorityForScheduler %s", par_SchedulerName);
    pScheduler->Prio = IniFileDataBaseReadInt (SCHED_INI_SECTION, Entry, THREAD_PRIORITY_IDLE,  Fd);
    if (pScheduler->Prio < THREAD_PRIORITY_IDLE) pScheduler->Prio = THREAD_PRIORITY_IDLE;
    if (pScheduler->Prio > THREAD_PRIORITY_TIME_CRITICAL) pScheduler->Prio = THREAD_PRIORITY_TIME_CRITICAL;

    sprintf (Entry, "BarriersBeforeOnlySignalForScheduler %s", par_SchedulerName);
    IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersBeforeOnlySignal, INI_MAX_LINE_LENGTH, Fd);
    sprintf (Entry, "BarriersBeforeSignalAndWaitForScheduler %s", par_SchedulerName);
    IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersBeforeSignalAndWait, INI_MAX_LINE_LENGTH, Fd);
    sprintf (Entry, "BarriersBehindOnlySignalForScheduler %s", par_SchedulerName);
    IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersBehindOnlySignal, INI_MAX_LINE_LENGTH, Fd);
    sprintf (Entry, "BarriersBehindSignalAndWaitForScheduler %s", par_SchedulerName);
    IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "",  BarriersBehindSignalAndWait, INI_MAX_LINE_LENGTH, Fd);
    ConnectSchedulerToAllAssociatedBarriers (pScheduler,
                                             BarriersBeforeOnlySignal,
                                             BarriersBeforeSignalAndWait,
                                             BarriersBehindOnlySignal,
                                             BarriersBehindSignalAndWait);


    // Read scheduler parameter from the INI file
    if (read_schedulerinfos_from_ini (&pScheduler->SchedPeriod) != 0) {
        return -1;
    }
    pScheduler->SchedPeriodInNanoSecond = (int64_t)((pScheduler->SchedPeriod * TIMERCLKFRQ) + 0.5);

    // Pipe read write buffer
    pScheduler->pTransmitMessageBuffer = (PIPE_API_BASE_CMD_MESSAGE*)my_calloc (PIPE_MESSAGE_BUFSIZE, 1);
    pScheduler->pReceiveMessageBuffer = (PIPE_API_BASE_CMD_MESSAGE_ACK*)my_calloc (PIPE_MESSAGE_BUFSIZE, 1);

    // Init. time variable
#ifdef _WIN32
    GetLocalTime (&LocalTime);
    SystemTimeToFileTime (&LocalTime, (LPFILETIME)&SystemTypeIn100NanoSecond);

    pScheduler->SimulatedStartTimeInNanoSecond = SystemTypeIn100NanoSecond * 100;
#else
    pScheduler->SimulatedStartTimeInNanoSecond = GetTimeOfSystemIsRunning64();
#endif

    SystemTypeIn100NanoSecond = 0;

    pScheduler->SimulatedTimeInNanoSecond = 0;

    // Before first scheduler call
    pScheduler->LastSchedCallTime = GetTickCount();
    pScheduler->SchedPeriodFiltered = pScheduler->SchedPeriod;

    CreateSchedulerThread (pScheduler);

    return 0;
}

// this have to call inside the PipeSchedCriticalSection
static char *GetProcessNamesAllScheduler (int RunOrSleep)
{
    char *Ret;
    int x;
    int Len, Pos;

    int TruncatePathFlag = (RunOrSleep & 0x100) == 0x100;
    int NotYetAddToSchedulerFlag = (RunOrSleep & 0x200) == 0x200;
    RunOrSleep = RunOrSleep & 0xFF;
    
    Ret = NULL;
    Pos = 0;    
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (((RunOrSleep == 0) && (pTcb->state == PROCESS_SLEEP)) || 
                ((RunOrSleep == 1) && (pTcb->state != PROCESS_SLEEP)) || 
                    (RunOrSleep >= 2)) {
                char *p;
                p = pTcb->name;
                if (TruncatePathFlag && (pTcb->Type == EXTERN_ASYNC)) {
                    char *plbs = NULL;
                    while (*p != 0) {
                        if ((*p == '\\') || (*p == '/')) plbs = p;
                        p++;
                    }
                    if (plbs != NULL) {
                        p = plbs + 1;
                    } else {
                        p = pTcb->name;  // Process name without path
                    }
                }
                Len = (int)strlen (p) + 1;
                Ret = my_realloc (Ret, Pos + Len + 2);
                if (Ret != NULL) {
                    MEMCPY (Ret + Pos, p, (size_t)Len);
                    Pos += Len;
                }
            }
        }
    }

    if (NotYetAddToSchedulerFlag) {
        for (x = 0; x < SchedulerCount; x++) {
            TASK_CONTROL_BLOCK *pTcb;
            int RdPos = AddProcessesToSchedRdPos[SchedulerData[x].SchedulerNr];
            while ((pTcb = AddProcessesToSchedPtrs[SchedulerData[x].SchedulerNr][RdPos]) != NULL) {
                char *p;
                p = pTcb->name;
                if (TruncatePathFlag && (pTcb->Type == EXTERN_ASYNC)) {
                    char *plbs = NULL;
                    while (*p != 0) {
                        if ((*p == '\\') || (*p == '/')) plbs = p;
                        p++;
                    }
                    if (plbs != NULL) {
                        p = plbs + 1;
                    } else {
                        p = pTcb->name;  // this should never happen (name should include always the path)
                    }
                }
                Len = (int)strlen (p) + 1;
                Ret = my_realloc (Ret, Pos + Len + 2);
                if (Ret != NULL) {
                    memcpy (Ret + Pos, p, (size_t)Len);
                    Pos += Len;
                }

                if (RdPos >= (MAX_PIDS - 1)) {
                    RdPos = 0;
                } else {
                    RdPos++;
                }
            }
        }
    }

    if (s_main_ini_val.ConnectToRemoteMaster) {
        char ProcessName[512];
        int Index = 0;
        while ((Index = rm_GetNextRealtimeProcess(Index, RunOrSleep, ProcessName, sizeof (ProcessName))) > 0) {
            Len = (int)strlen(ProcessName) + 1;
            Ret = my_realloc (Ret, Pos + Len + 2);
            if (Ret != NULL) {
                MEMCPY (Ret + Pos, ProcessName, (size_t)Len);
                Pos += Len;
            }

        }
    }

    if (Ret != NULL) {
        Ret[Pos] = 0;     // End of the list
    }

    return Ret;
}

static char *FreeProcessNames (char *ProcessNames)
{
    if (ProcessNames != NULL) my_free (ProcessNames);
    return NULL;
}

READ_NEXT_PROCESS_NAME *init_read_next_process_name (int RunOrSleep)
{
    READ_NEXT_PROCESS_NAME *Ret;

    Ret = my_calloc(1, sizeof(READ_NEXT_PROCESS_NAME));

    Ret->RunOrSleep = RunOrSleep;

    EnterCriticalSection (&PipeSchedCriticalSection);
    Ret->Ptr = Ret->BasePtr = GetProcessNamesAllScheduler (RunOrSleep);
    LeaveCriticalSection (&PipeSchedCriticalSection);

    return Ret;
}

char *read_next_process_name (READ_NEXT_PROCESS_NAME *Buffer)
{
    char *Ret;
    if (Buffer->First == 1) {
        Buffer->First = 0;  // will not be used
    }
    if (Buffer->Ptr != NULL) {
        Buffer->Ptr += strlen (Buffer->Ptr) + 1;
        if (*(Buffer->Ptr) == 0) {
            Buffer->BasePtr = FreeProcessNames (Buffer->BasePtr);
            Buffer->Ptr = NULL;
            Ret = NULL;
        } else {
            Ret = Buffer->Ptr;
        }
    } else {
        Ret = NULL;
    }
    return Ret;
}

void close_read_next_process_name(READ_NEXT_PROCESS_NAME *Buffer)
{
    if (Buffer != NULL) {
        if (Buffer->BasePtr != NULL) my_free(Buffer->BasePtr);
        my_free(Buffer);
    }
}

static char *PtrToProcessNameWithoutPath (char *par_ProcessName)
{
    char *p;
    char *Ret;
    p = Ret = par_ProcessName;

    while (*p != 0) {
        if ((*p == '\\') || (*p == '/')) Ret = p + 1;  // Store position of the last backslashes
        p++;
    }
    return Ret;
}

char *GetAllProcessNamesSemicolonSeparated (void)
{
    char *Ret;
    int x;
    int Len = 0;
    int Count = 0;
    int Pos = 0;

    Ret = NULL;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            Len += (int)strlen (PtrToProcessNameWithoutPath (pTcb->name)) + 1;
        }
    }
    Ret = my_malloc (Len);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            char *ShortName;
            int NameLen;
            ShortName = PtrToProcessNameWithoutPath (pTcb->name);
            NameLen = (int)strlen (ShortName) + 1;
            if (Count > 0) {
                Ret[Pos] = ';';
                Pos++;
            }
            MEMCPY (Ret + Pos, ShortName, (size_t)NameLen);
            Pos += NameLen - 1;
            Count++;
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return Ret;
}

char *GetProcessInsideExecutionFunctionSemicolonSeparated (void)
{
    char *Ret;
    int x;
    int Len = 0;
    int Count = 0;
    int Pos = 0;

    Ret = NULL;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb = SchedulerData[x].CurrentTcb;
        if (pTcb != NULL) {
            Len += (int)strlen (PtrToProcessNameWithoutPath (pTcb->name)) + 1;
        }
    }
    if (Len == 0) {
         Ret = my_malloc (1);
         Ret[0] = 0;
         goto __OUT;
    }
    Ret = my_malloc (Len);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb = SchedulerData[x].CurrentTcb;
        if (pTcb != NULL) {
            char *ShortName;
            int NameLen;
            ShortName = PtrToProcessNameWithoutPath (pTcb->name);
            NameLen = (int)strlen (ShortName) + 1;
            if (Count > 0) {
                Ret[Pos] = ';';
                Pos++;
            }
            MEMCPY (Ret + Pos, ShortName, (size_t)NameLen);
            Pos += NameLen - 1;
            Count++;
        }
    }
__OUT:
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return Ret;
}

int get_process_timeout (PID par_Pid)
{
    int x;
    int Ret;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {
                Ret = pTcb->timeout;
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return Ret;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;
}

int GetProcessLongName (PID par_Pid, char *ret_Name)
{
    int x;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        if (rm_IsRealtimeProcessPid(par_Pid)) {
            int Type;
            int Prio;
            int Cycles;
            int Delay;
            int MessageSize;
            char AssignedScheduler[MAX_PATH];
            uint64_t bb_access_mask;
            int State;
            uint64_t CyclesCounter;
            uint64_t CyclesTime;
            uint64_t CyclesTimeMax;
            uint64_t CyclesTimeMin;
            return rm_get_process_info_ex (par_Pid, ret_Name, 512,
                                           &Type, &Prio, &Cycles, &Delay, &MessageSize,
                                           AssignedScheduler, sizeof(AssignedScheduler), &bb_access_mask,
                                           &State, &CyclesCounter, &CyclesTime, &CyclesTimeMax, &CyclesTimeMin);
        }
    }
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {
                strcpy (ret_Name, pTcb->name);
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return 0;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;     
}

int get_name_by_pid ( PID pid, char *name)
{
    return GetProcessLongName (pid, name);
}

int GetProcessShortName (PID par_Pid, char *ret_Name)
{
    int x;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        if (rm_IsRealtimeProcessPid(par_Pid)) {
            int Type;
            int Prio;
            int Cycles;
            int Delay;
            int MessageSize;
            char AssignedScheduler[MAX_PATH];
            uint64_t bb_access_mask;
            int State;
            uint64_t CyclesCounter;
            uint64_t CyclesTime;
            uint64_t CyclesTimeMax;
            uint64_t CyclesTimeMin;
            return rm_get_process_info_ex (par_Pid, ret_Name, 512,
                                           &Type, &Prio, &Cycles, &Delay, &MessageSize,
                                           AssignedScheduler, sizeof(AssignedScheduler), &bb_access_mask,
                                           &State, &CyclesCounter, &CyclesTime, &CyclesTimeMax, &CyclesTimeMin);
        }
    }
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {
                char *p, *pbs;
                p = pbs = pTcb->name;
                while (*p != 0) {
                    if ((*p == '\\') || (*p == '/')) pbs = p + 1;
                    p++;
                }
                strcpy (ret_Name, pbs);
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return 0;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;
}

int GetProcessExecutableName (PID par_Pid, char *ret_Name)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {
                if (pTcb->Type == EXTERN_ASYNC) {
                    if ((pTcb->DllName == NULL) || (strlen (pTcb->DllName) == 0)) {
                        char *p, *pka;
                        p = pTcb->name;
                        pka = NULL;
                        while (*p != 0) {
                            if (*p == '@') pka = p;
                            p++;
                        }
                        if (pka != NULL) {
                            p = pka;
                        }
                        MEMCPY (ret_Name, pTcb->name, (size_t)(p - pTcb->name));
                        ret_Name[p - pTcb->name] = 0;
                    } else {
                        strcpy (ret_Name, pTcb->DllName);
                    }
                    LeaveCriticalSection (&PipeSchedCriticalSection);
                    return 0;
                }
            }
        }
    }
    *ret_Name = 0;
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;
}

int GetProcessInsideExecutableName (PID par_Pid, char *ret_Name)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {
                if (pTcb->Type == EXTERN_ASYNC) {
                    strcpy (ret_Name, pTcb->InsideExecutableName);
                    LeaveCriticalSection (&PipeSchedCriticalSection);
                    return 0;
                }
            }
        }
    }
    *ret_Name = 0;
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;
}


int GetProcessPidAndExecutableAndDllName (char *par_Name, char *ret_ExecutableName, char *ret_DllName, int *ret_ProcessInsideExecutableNumber)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (!Compare2ProcessNames(par_Name, pTcb->name)) {
                if (pTcb->Type == EXTERN_ASYNC) {
                    int Ret;
                    if (ret_ExecutableName != NULL) {
                        strcpy (ret_ExecutableName, pTcb->InsideExecutableName);
                    }
                    if (ret_DllName != NULL) {
                        if ((pTcb->DllName == NULL) || (strlen (pTcb->DllName) == 0)) {
                            strcpy (ret_DllName, "");
                        } else {
                            strcpy (ret_DllName, pTcb->DllName);
                        }
                    }
                    if (ret_ProcessInsideExecutableNumber != NULL) {
                        *ret_ProcessInsideExecutableNumber = pTcb->NumberInsideExecutable;
                    }
                    Ret = pTcb->pid;
                    LeaveCriticalSection (&PipeSchedCriticalSection);
                    return Ret;
                }
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;
}


int Compare2ProcessNames (const char *par_Name1, const char *par_Name2)
{
    const char *p1, *p2;
    const char *po1, *po2;   // last '.' char
    const char *bs1, *bs2;   // last '\\' char

    BEGIN_RUNTIME_MEASSUREMENT ("Compare2ProcessNames");
    p1 = par_Name1;
    p2 = par_Name2;
    bs1 = po1 = NULL;
    bs2 = po2 = NULL;
    while (*p1 != 0) {
        if (*p1 == '.') po1 = p1 + 1;
        else if ((*p1 == '\\') || (*p1 == '/')) bs1 = p1 + 1;
        p1++;
    }
    while (*p2 != 0) {
        if (*p2 == '.') po2 = p2 + 1;
        else if ((*p2 == '\\') || (*p2 == '/')) bs2 = p2 + 1;
        p2++;
    }

    if ((po1 != NULL) && (po2 != NULL)) {  // Compare two EXE-Prozesse
        int Len1, Len2;
        const char *pp1, *pp2;
        if (bs1 != NULL)  {  // If name this path ignore the path
            pp1 = bs1;
        } else {
            pp1 = par_Name1;
        }
        if (bs2 != NULL)  {  // If name this path ignore the path
            pp2 = bs2;
        } else {
            pp2 = par_Name2;
        }
        Len1 = (int)(p1 - pp1);
        Len2 = (int)(p2 - pp2);
        if ((Len1 > 0) && (Len1 == Len2)) {
#ifdef _WIN32
            return _memicmp (pp1, pp2, (size_t)Len1);  // different name?
#else
            return strncasecmp(pp1, pp2, (size_t)Len1);
#endif
        }
        return 1;
    } else if ((po1 != NULL) || (po2 != NULL)) { // One EXE and one internal process
        return 1;

    } else {   // Otherwise they are 2 internal processes

        int Len1, Len2;
        Len1 = (int)(p1 - par_Name1);
        Len2 = (int)(p2 - par_Name2);
        if ((Len1 > 0) && (Len1 == Len2)) {
#ifdef _WIN32
            return _memicmp (par_Name1, par_Name2, (size_t)Len1);  // different name?
#else
            return strncasecmp(par_Name1, par_Name2, (size_t)Len1);
#endif
        }
        return 1;
    }
    END_RUNTIME_MEASSUREMENT
}

int Compare2ExecutableNames (const char *par_Name1, const char *par_Name2)
{
    const char *p1, *p2;
    const char *po1, *po2;   // last '.' char
    const char *ka1, *ka2;   // last '@' char
    const char *bs1, *bs2;   // last '\\' or '/' char

    BEGIN_RUNTIME_MEASSUREMENT ("Compare2ExecutableNames");
    p1 = par_Name1;
    p2 = par_Name2;
    bs1 = po1 = NULL;
    bs2 = po2 = NULL;
    ka1 = ka2 = NULL;
    while (*p1 != 0) {
        if (*p1 == '.') po1 = p1 + 1;
        else if ((*p1 == '\\') || (*p1 == '/')) bs1 = p1 + 1;
        else if (*p1 == '@') ka1 = p1;
        p1++;
    }
    while (*p2 != 0) {
        if (*p2 == '.') po2 = p2 + 1;
        else if ((*p2 == '\\') || (*p2 == '/')) bs2 = p2 + 1;
        else if (*p2 == '@') ka2 = p2;
        p2++;
    }

    if ((po1 != NULL) && (po2 != NULL)) {  // Compare two EXE-Prozesse
        int Len1, Len2;
        const char *pp1, *pp2;
        if (bs1 != NULL)  {  // If name this path ignore the path
            pp1 = bs1;
        } else {
            pp1 = par_Name1;
        }
        if (bs2 != NULL)  {  // If name this path ignore the path
            pp2 = bs2;
        } else {
            pp2 = par_Name2;
        }
        if (ka1 != NULL)  {  // If process postfix '@' exist ignore
            p1 = ka1;
        }
        if (ka2 != NULL)  {  // If process postfix '@' exist ignore
            p2 = ka2;
        }
        Len1 = (int)(p1 - pp1);
        Len2 = (int)(p2 - pp2);
        if ((Len1 > 0) && (Len1 == Len2)) {
#ifdef _WIN32
            return _memicmp (pp1, pp2, (size_t)Len1);  // different name?
#else
            return strncasecmp(pp1, pp2, Len1);
#endif

        }
        return 1;
    } else if ((po1 != NULL) || (po2 != NULL)) { // One EXE and one internal process
        return 1;

    } else {   // Otherwise they are 2 internal processes
        int Len1, Len2;
        Len1 = (int)(p1 - par_Name1);
        Len2 = (int)(p2 - par_Name2);
        if ((Len1 > 0) && (Len1 == Len2)) {
#ifdef _WIN32
            return _memicmp (par_Name1, par_Name2, (size_t)(Len1));  // different name?
#else
            return strncasecmp(par_Name1, par_Name2, Len1);
#endif
        }
        return 1;
    }
    END_RUNTIME_MEASSUREMENT
}


int GetProcessNameWithoutPath (int pid, char *pname)
{
    char *p, *ka, name[MAX_PATH];

    if (!get_name_by_pid (pid, name)) {
        p = name;
        ka = NULL;
        while (*p != 0) {
            if (*p == '@') ka = p;
            p++;
        }
        if (ka != NULL) {
            p = ka;
        }
        if ((*(p-4) == '.') && ((((*(p-3) == 'E') || (*(p-3) == 'e')) &&
                                 ((*(p-2) == 'X') || (*(p-2) == 'x')) &&
                                 ((*(p-1) == 'E') || (*(p-1) == 'e'))) ||
                                (((*(p-3) == 'F') || (*(p-3) == 'f')) &&
                                 ((*(p-2) == 'M') || (*(p-2) == 'm')) &&
                                 ((*(p-1) == 'U') || (*(p-1) == 'u'))))) {
            while (((*p != '\\') && (*p != '/')) && (p > name)) p--;
            strcpy (pname, p+1);
        } else {
            strcpy (pname, name);
        }
        return 0;
    }
    return UNKNOWN_PROCESS;
}


int TruncatePathFromProcessName (char *DestProcessName, const char *SourceProcessName)
{
    const char *p, *px;

    px = NULL;
    p = SourceProcessName;
    while (*p != 0) {
        if ((*p == '\\') || (*p == '/')) px = p;  // Store the position of the last Backslashes
        p++;
    }
    if (px == NULL) {
        strcpy (DestProcessName, SourceProcessName);
    } else {
        strcpy (DestProcessName, px + 1);
    }
    return 0;
}

int TruncatePathAndTaskNameFromProcessName (char *DestProcessName, char *SourceProcessName)
{
    char *d, *p, *px;

    px = NULL;
    p = SourceProcessName;
    while (*p != 0) {
        if ((*p == '\\') || (*p == '/')) px = p;  // Store the position of the last Backslashes
        p++;
    }
    if (px == NULL) {
        p = SourceProcessName;
    } else {
        p = px + 1;
    }
    d = DestProcessName;
    while ((*p != '\0') && (*p != '@')) {
        *d++ = *p++;
    }
    *p = 0;
    return 0;
}

int TruncateTaskNameFromProcessName (char *DestProcessName, char *SourceProcessName)
{
    char *d, *s;

    s = SourceProcessName;
    d = DestProcessName;
    while ((*s != '\0') && (*s != '@')) {
        *d++ = *s++;
    }
    *d = 0;
    return 0;
}

int terminate_process (PID pid)
{
    int x;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        if (rm_StopRealtimeProcess(pid) == 0) {
            return 0;  // Was a realtime process
        }
    }
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == pid) {
                pTcb->state = EX_PROCESS_WAIT_FOR_END_OF_CYCLE_TO_GO_TO_TERM;
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return 0;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;     
}

int get_process_state (PID pid)
{
    int x;
    int State;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == pid) {
                State = pTcb->state;
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return State;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;     
}

int is_pid_valid (PID pid)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == pid) {
                return 0;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;     
}


TASK_CONTROL_BLOCK *get_process_info (PID pid)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == pid) {
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return pTcb;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return NULL;     
}


static void strcpy_maxc (char *dst, const char *src, int max_c)
{
    if (src == NULL) {
        dst[0] = 0;
    } else {
        if (max_c > 0) {
            int len = (int)strlen (src) + 1;
            if (len <= max_c) {
                MEMCPY (dst, src, (size_t)len);
            } else {
                MEMCPY (dst, src, (size_t)max_c - 1);
                dst[max_c - 1] = 0;
            }
        }
    }
}

int get_process_info_ex (PID pid, char *ret_Name, int maxc_Name, int *ret_Type, int *ret_Prio, int *ret_Cycles, int *ret_Delay, int *ret_MessageSize,
                         char *ret_AssignedScheduler, int maxc_AssignedScheduler, uint64_t *ret_bb_access_mask,
                         int *ret_State, uint64_t *ret_CyclesCounter, uint64_t *ret_CyclesTime, uint64_t *ret_CyclesTimeMax, uint64_t *ret_CyclesTimeMin)
{
    int x;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        int Ret = rm_get_process_info_ex (pid, ret_Name, maxc_Name, ret_Type, ret_Prio, ret_Cycles, ret_Delay, ret_MessageSize,
                                          ret_AssignedScheduler, maxc_AssignedScheduler, ret_bb_access_mask,
                                          ret_State, ret_CyclesCounter, ret_CyclesTime, ret_CyclesTimeMax, ret_CyclesTimeMin);
        if (Ret == 0) return 0;
    }

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == pid) {
                if ((ret_Name != NULL) && (maxc_Name > 0)) {
                    strcpy_maxc (ret_Name, pTcb->name, maxc_Name);
                }
                if (ret_Type != NULL) *ret_Type = pTcb->Type;
                if (ret_Prio != NULL) *ret_Prio = pTcb->prio;
                if (ret_Cycles != NULL) *ret_Cycles = pTcb->time_steps;
                if (ret_Delay != NULL) *ret_Delay = pTcb->delay;
                if (ret_MessageSize != NULL) *ret_MessageSize = pTcb->message_size;
                if ((ret_AssignedScheduler != NULL) && (maxc_AssignedScheduler > 0)) {
                    strcpy_maxc (ret_AssignedScheduler, SchedulerData[x].SchedulerName, maxc_AssignedScheduler);
                }
                if (ret_bb_access_mask != NULL) *ret_bb_access_mask = pTcb->bb_access_mask;
                if (ret_State != NULL) *ret_State = pTcb->state;
                if (ret_CyclesCounter != NULL) *ret_CyclesCounter = pTcb->call_count;
                if (ret_CyclesTime != NULL) *ret_CyclesTime = 0;
                if (ret_CyclesTimeMax != NULL) *ret_CyclesTimeMax = 0;
                if (ret_CyclesTimeMin != NULL) *ret_CyclesTimeMin = 0;

                LeaveCriticalSection (&PipeSchedCriticalSection);
                return 0;
            }
        }
    }
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        int RdPos = AddProcessesToSchedRdPos[SchedulerData[x].SchedulerNr];
        while ((pTcb = AddProcessesToSchedPtrs[SchedulerData[x].SchedulerNr][RdPos]) != NULL) {
            if (pTcb->pid == pid) {
                if ((ret_Name != NULL) && (maxc_Name > 0)) {
                    strcpy_maxc (ret_Name, pTcb->name, maxc_Name);
                }
                if (ret_Type != NULL) *ret_Type = pTcb->Type;
                if (ret_Prio != NULL) *ret_Prio = pTcb->prio;
                if (ret_Cycles != NULL) *ret_Cycles = pTcb->time_steps;
                if (ret_Delay != NULL) *ret_Delay = pTcb->delay;
                if (ret_MessageSize != NULL) *ret_MessageSize = pTcb->message_size;
                if ((ret_AssignedScheduler != NULL) && (maxc_AssignedScheduler > 0)) {
                    strcpy_maxc (ret_AssignedScheduler, SchedulerData[x].SchedulerName, maxc_AssignedScheduler);
                }
                if (ret_bb_access_mask != NULL) *ret_bb_access_mask = pTcb->bb_access_mask;
                if (ret_State != NULL) *ret_State = pTcb->state;
                if (ret_CyclesCounter != NULL) *ret_CyclesCounter = pTcb->call_count;
                if (ret_CyclesTime != NULL) *ret_CyclesTime = 0;
                if (ret_CyclesTimeMax != NULL) *ret_CyclesTimeMax = 0;
                if (ret_CyclesTimeMin != NULL) *ret_CyclesTimeMin = 0;

                LeaveCriticalSection (&PipeSchedCriticalSection);
                return 0;
            }

            if (RdPos >= (MAX_PIDS - 1)) {
                RdPos = 0;
            } else {
                RdPos++;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return -1;
}


int get_process_info_internal_debug(PID pid,
                                    HANDLE *ret_hPipe,

                                    int *ret_SocketOrPipe,

                                    int *ret_IsStartedInDebugger,  // This is 1 if the process are started inside a debugger (remark the process can be attached afterwards, this will not recognized)
                                    int *ret_MachineType,          // If 0 -> 32Bit Windows 1 -> 64Bit Windows, 2 -> 32Bit Linux, 3 -> 64Bit Linux

                                    int *ret_UseDebugInterfaceForWrite,
                                    int *ret_UseDebugInterfaceForRead,

                                    int *ret_wait_on,             // This is 1 if the scheduler waits on this process


                                    int *ret_WorkingFlag,   // Process is inside cyclic/init/terminate/reference Function
                                    int *ret_LockedFlag,    // Process cannot enter its cyclic/init/terminate/reference Function the scheduler have to wait
                                    int *ret_SchedWaitForUnlocking,  // Scheduler waits that he can call cyclic/init/terminate/reference function of the process
                                    int *ret_WaitForEndOfCycleFlag,  // Somebody is waiting that cyclic/init/terminate/reference Function will be returned

                                    DWORD *ret_LockedByThreadId,
                                    char *ret_SrcFileName,
                                    int maxc_SrcFileName,
                                    int *ret_SrcLineNr,
                                    char *ret_DllName,     // If the process is a *.DLL/*.so this will be the file name otherwise this is NULL
                                    int maxc_DllName,
                                    char *ret_InsideExecutableName,
                                    int maxc_InsideExecutableName,
                                    int *ret_NumberInsideExecutable,

                                     uint64_t *ret_ExecutableBaseAddress,
                                     uint64_t *ret_DllBaseAddress,

                                     int *ret_ExternProcessComunicationProtocolVersion)
{
    int x;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        // This is not available if remote master is activated
    }

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == pid) {
                if (ret_hPipe != NULL) *ret_hPipe = pTcb->hPipe;
                if (ret_SocketOrPipe != NULL) *ret_SocketOrPipe = pTcb->SocketOrPipe;
                if (ret_IsStartedInDebugger != NULL) *ret_IsStartedInDebugger = pTcb->IsStartedInDebugger;
                if (ret_MachineType != NULL) *ret_MachineType = pTcb->MachineType;
                if (ret_UseDebugInterfaceForWrite != NULL) *ret_UseDebugInterfaceForWrite = pTcb->UseDebugInterfaceForWrite;
                if (ret_UseDebugInterfaceForRead != NULL) *ret_UseDebugInterfaceForRead = pTcb->UseDebugInterfaceForRead;
                if (ret_wait_on != NULL) *ret_wait_on = pTcb->wait_on;
                if (ret_WorkingFlag != NULL) *ret_WorkingFlag = pTcb->WorkingFlag;
                if (ret_LockedFlag != NULL) *ret_LockedFlag = pTcb->LockedFlag;
                if (ret_SchedWaitForUnlocking != NULL) *ret_SchedWaitForUnlocking = pTcb->SchedWaitForUnlocking;
                if (ret_WaitForEndOfCycleFlag != NULL) *ret_WaitForEndOfCycleFlag = pTcb->WaitForEndOfCycleFlag;
                if (ret_LockedByThreadId != NULL) *ret_LockedByThreadId = pTcb->LockedByThreadId;
                if ((ret_SrcFileName != NULL) && (maxc_SrcFileName > 0)) {
                    strcpy_maxc (ret_SrcFileName, pTcb->SrcFileName, maxc_SrcFileName);
                }
                if (ret_SrcLineNr != NULL) *ret_SrcLineNr = pTcb->SrcLineNr;
                if (ret_LockedFlag != NULL) *ret_LockedFlag = pTcb->LockedFlag;
                if (ret_LockedFlag != NULL) *ret_LockedFlag = pTcb->LockedFlag;
                if ((ret_DllName != NULL) && (maxc_DllName > 0)) {
                    strcpy_maxc (ret_DllName, pTcb->DllName, maxc_DllName);
                }
                if ((ret_InsideExecutableName != NULL) && (maxc_InsideExecutableName > 0)) {
                    strcpy_maxc (ret_InsideExecutableName, pTcb->InsideExecutableName, maxc_InsideExecutableName);
                }
                if (ret_NumberInsideExecutable != NULL) *ret_NumberInsideExecutable = pTcb->NumberInsideExecutable;
                if (ret_ExecutableBaseAddress != NULL) *ret_ExecutableBaseAddress = pTcb->ExecutableBaseAddress;
                if (ret_DllBaseAddress != NULL) *ret_DllBaseAddress = pTcb->DllBaseAddress;
                if (ret_ExternProcessComunicationProtocolVersion != NULL) *ret_ExternProcessComunicationProtocolVersion = pTcb->ExternProcessComunicationProtocolVersion;

                LeaveCriticalSection (&PipeSchedCriticalSection);
                return 0;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return -1;
}


int GetExternProcessBaseAddress (int Pid, uint64_t *ret_Address, char *ExecutableName)
{
    int Ret, x;
    Ret = -1;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == Pid) {
                if (strlen(pTcb->DllName) == 0) {
                    *ret_Address = pTcb->ExecutableBaseAddress;
                } else {
                    *ret_Address = pTcb->DllBaseAddress;
                }
                Ret = 0;
                break;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return Ret;
}


int GetExternProcessIndexInsideExecutable (int Pid)
{
    TASK_CONTROL_BLOCK *Tcb;

    Tcb = get_process_info (Pid);
    if (Tcb == NULL) return -2;
    if (Tcb->Type != EXTERN_ASYNC) return -3;
    return Tcb->NumberInsideExecutable;
}

int IsExternProcess64BitExecutable (int Pid)
{
    TASK_CONTROL_BLOCK *Tcb;

    Tcb = get_process_info (Pid);
    if (Tcb == NULL) return -2;
    if (Tcb->Type != EXTERN_ASYNC) return -3;
    return Tcb->MachineType & 0x1;   // The last bit is set if the external process is a 64bit executable
}

int IsExternProcessLinuxExecutable (int Pid)
{
    TASK_CONTROL_BLOCK *Tcb;

    Tcb = get_process_info (Pid);
    if (Tcb == NULL) return -2;
    if (Tcb->Type != EXTERN_ASYNC) return -3;
    return (Tcb->MachineType & 0x2) == 0x2;   // The second last Bit is set if the external process is a linux executable
}

int GetExternProcessMachineType (int Pid)
{
    TASK_CONTROL_BLOCK *Tcb;

    Tcb = get_process_info (Pid);
    if (Tcb == NULL) return -2;
    if (Tcb->Type != EXTERN_ASYNC) return -3;
    return Tcb->MachineType;
}

uint32_t get_sched_periode_timer_clocks()
{
    // Use always scheduler 0
    return (uint32_t)SchedulerData[0].SchedPeriodInNanoSecond;
}

uint64_t get_sched_periode_timer_clocks64()
{
    // Use always scheduler 0
    return (uint64_t)SchedulerData[0].SchedPeriodInNanoSecond;
}

uint64_t get_timestamp_counter (void)
{
    // Use always scheduler 0
    return  SchedulerData[0].SimulatedTimeInNanoSecond;
}

uint64_t GetSimulatedTimeInNanoSecond(void)
{
    // Always from scheduler 0
    return SchedulerData[0].SimulatedTimeInNanoSecond;
}

uint64_t GetSimulatedTimeSinceStartedInNanoSecond (void)
{
    return SchedulerData[0].SimulatedTimeSinceStartedInNanoSecond;
}

double GetSimulatedTimeSinceStartedInSecond (void)
{
    return (double)SchedulerData[0].SimulatedTimeSinceStartedInNanoSecond / TIMERCLKFRQ;
}

uint64_t GetSimulatedStartTimeInNanoSecond (void)
{
    return SchedulerData[0].SimulatedStartTimeInNanoSecond;
}

int get_scheduler_state (void)
{
    return !IsSchedulerSleeping (&(SchedulerData[0].StopReq));
}

void enable_scheduler (int FromUser)
{
    int ThreadId = (int)GetCurrentThreadId();

    if (s_main_ini_val.ConnectToRemoteMaster) {   // User and RPC stops requests would be ignored if remote master (realtime) are activated
        if (FromUser != SCHEDULER_CONTROLED_BY_SYSTEM) {
            return;
        }
    }
    ContinueRequest(&(SchedulerData[0].StopReq), ThreadId, FromUser);
}

void enable_scheduler_with_callback (int FromUser, void (*Callback)(void*), void *CallbackParameter)
{
    int ThreadId = (int)GetCurrentThreadId();

    if (s_main_ini_val.ConnectToRemoteMaster) {   // User and RPC stops requests would be ignored if remote master (realtime) are activated
        if (FromUser != SCHEDULER_CONTROLED_BY_SYSTEM) {
            return;
        }
    }
    ContinueRequestWithCallback(&(SchedulerData[0].StopReq), ThreadId, FromUser, Callback, CallbackParameter);
}

int disable_scheduler_at_end_of_cycle (int FromUser, void (*Callback)(void*), void *CallbackParameter)
{
    int Err;
    int ThreadId = (int)GetCurrentThreadId();

    if (s_main_ini_val.ConnectToRemoteMaster) {   // User and RPC stops requests would be ignored if remote master (realtime) are activated
        if (FromUser != SCHEDULER_CONTROLED_BY_SYSTEM) {
            return 0;
        }
    }

    if (SchedulerData[0].State == SCHED_OFF_STATE) return -1;

    if (Callback == NULL) {
        StopRequest(&(SchedulerData[0].StopReq), ThreadId, FromUser);
        Err = 0;
    } else {
        Err = StopRequestWithCallback(&(SchedulerData[0].StopReq), ThreadId, FromUser, Callback, CallbackParameter);
        if (Err == -1) {
            ThrowError (1, "cannot add callback function in disable_scheduler_at_end_of_cycle() there already more than 8 callbacks");
        }
    }
    return Err;
}

int make_n_next_cycles (int FromUser, uint64_t NextCycleCount,
                        void (*Callback)(void*), void *CallbackParameter)
{
    int ThreadId = (int)GetCurrentThreadId();

    AddTimedStopRequest(&(SchedulerData[0].StopReq), ThreadId, FromUser,
                        SchedulerData[0].Cycle + NextCycleCount,
                        Callback, CallbackParameter,
                        1);
    return 0;
}


static int TerminateSchedulerState;
#define NO_TERMINATION_REQUEST_ACTIVE      0
#define WAIT_TILL_INIT_PROCESS_TERMINATED  1
#define WAIT_TILL_ALL_SCHEDULER_TERMINATED 2
#define ALL_SCHEDULER_TERMINATED           3

static void TerminateAllSchedulerSetRequest (void)
{
    SetToTerminateSchedulerCount();

    AllSchedulerShouldBeTerminatedFlag = 1;
    if (!s_main_ini_val.ConnectToRemoteMaster) {
        RemoveAllStopRequest(&(SchedulerData[0].StopReq));
        SchedulerWakeup(&(SchedulerData[0].StopReq));
    }
}

static void UnRegisterAllSchedFunc (void)
{
    int x;
    for (x = 0; x < SchedulerCount; x++) {
        SchedulerData[x].user_sched_func_flag = 0;
        SchedulerData[x].user_sched_func = NULL;
    }
}

static int BreakAllBarriersFlag;

// This will will be called cyclical during termination to test if all scheduler are terminated
int CheckIfAllSchedulerAreTerminated (void)
{
    int x;
    int AllTerminatedFlag = 1;

    switch (TerminateSchedulerState) {
    case WAIT_TILL_INIT_PROCESS_TERMINATED:
        if (get_pid_by_name("init") < 0) {
            TerminateAllSchedulerSetRequest();
            TerminateSchedulerState = WAIT_TILL_ALL_SCHEDULER_TERMINATED;
        }
        AllTerminatedFlag = 0;
        break;
    default:
    case WAIT_TILL_ALL_SCHEDULER_TERMINATED:
        if (BreakAllBarriersFlag) RemoveWaitIfAloneForAllBarriers ();
        for (x = 0; x < SchedulerCount; x++) {
            if (SchedulerData[x].State == SCHED_EXTERNAL_ONLY_INFO) continue;
            if (SchedulerData[x].State != SCHED_IS_TERMINATED_STATE) {
                AllTerminatedFlag = 0;
            }
        }
        break;
    }
    if (AllTerminatedFlag) {
        FreeTcb(NULL, FREE_TCB_CMD_DELETE_ALL);   // free all finally
        GetOrFreeUniquePid (FREE_UNIQUE_PID_COMMAND, GuiTcb.pid, GuiTcb.name);
    }
    return AllTerminatedFlag;
}

void TerminateAllSchedulerRequest (void)
{
    int init_pid;

    SetSchedulerBreakPoint (NULL);

    UnRegisterAllSchedFunc();  // If a WAIT_EXE script command are running

    // If scheduler is stopped gestoppt
    // enable scheduler to be on the safe side
    if (get_scheduler_state() == 0) {
        enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
        enable_scheduler(SCHEDULER_CONTROLED_BY_RPC);
    }

    init_pid = get_pid_by_name ("init");

    if (init_pid > 0) {
        // Terminate Init process
        terminate_process (init_pid); // This will done after all windows was closed
        TerminateSchedulerState = WAIT_TILL_INIT_PROCESS_TERMINATED;
    } else {
        TerminateAllSchedulerSetRequest();
        TerminateSchedulerState = WAIT_TILL_ALL_SCHEDULER_TERMINATED;
    }
}

static uint64_t SchedulerTerminationTimeOut;
// If this timeout reached all barriers will be break
static uint64_t SchedulerTerminationTimeOutBreakBarriers;

void SetTerminateAllSchedulerTimeout (int par_Timeout)
{
    uint64_t TickCount = GetTickCount64();
    SchedulerTerminationTimeOut = TickCount + (uint64_t)par_Timeout * 1000;
    SchedulerTerminationTimeOutBreakBarriers = TickCount + (uint64_t)par_Timeout * 500;
}

int CheckTerminateAllSchedulerTimeout (void)
{
    if (SchedulerTerminationTimeOut == 0) return 0;  // If timeout was not set before
    uint64_t TickCount = GetTickCount64();
    if (SchedulerTerminationTimeOutBreakBarriers <= TickCount) {
        BreakAllBarriersFlag = 1;
        if (DebugTermSchedFh != NULL) fprintf (DebugTermSchedFh, "TS %u BreakAllBarriersFlag!!!!!!!!\n", (unsigned int)GetTickCount());
    }
    if (SchedulerTerminationTimeOut <= TickCount) return 1;
    else return 0;
}



void RegisterSchedFunc (void (*user_sched_func) (void))
{
    SCHEDULER_DATA *Sched = GetCurrentScheduler ();
    if (Sched != NULL) {
        Sched->user_sched_func = user_sched_func;
        Sched->user_sched_func_flag = 1;
    }
}

void UnRegisterSchedFunc (void)
{
    SCHEDULER_DATA *Sched = GetCurrentScheduler ();
    if (Sched != NULL) {
        Sched->user_sched_func_flag = 0;
        Sched->user_sched_func = NULL;
    }
}

int write_extprocinfos_to_ini (int par_Fd,
                               const char *ext_proc_name,
                               int priority, 
                               int time_steps,
                               int delay,
                               int timeout,
                               int RangeControlBeforeActiveFlags,
                               int RangeControlBehindActiveFlags,
                               int RangeControlStopSchedFlag,
                               int RangeControlOutput,
                               int RangeErrorCounterFlag,
                               const char *RangeErrorCounter,
                               int RangeControlFlag,
                               const char *RangeControl,
                               int RangeControlPhysFlag,
                               int RangeControlLimitValues,
                               const char *BBPrefix,
                               const char *Scheduler,
                               const char *BarriersBeforeOnlySignal,
                               const char *BarriersBeforeSignalAndWait,
                               const char *BarriersBehindOnlySignal,
                               const char *BarriersBehindSignalAndWait,
                               const char *BarriersLoopOutBeforeOnlySignal,
                               const char *BarriersLoopOutBeforeSignalAndWait,
                               const char *BarriersLoopOutBehindOnlySignal,
                               const char *BarriersLoopOutBehindSignalAndWait)
{
    char  tmp_str[INI_MAX_LINE_LENGTH];
    char Entry[MAX_PATH];
    const char *s;

    s = GetProcessNameFromPath (ext_proc_name);

    sprintf (Entry, "BBPrefixForProcess %s", s);
    if (BBPrefix != NULL) {
        if (strlen (BBPrefix) == 0) {
            BBPrefix = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BBPrefix, par_Fd);
    }

    sprintf (Entry, "SchedulerForProcess %s", s);
    if (Scheduler != NULL) {
        if (strlen (Scheduler) == 0) {
            Scheduler = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, Scheduler, par_Fd);
    }

    sprintf (Entry, "BarriersBeforeOnlySignalForProcess %s", s);
    if (BarriersBeforeOnlySignal != NULL) {
        if (strlen (BarriersBeforeOnlySignal) == 0) {
            BarriersBeforeOnlySignal = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BarriersBeforeOnlySignal, par_Fd);
    }

    sprintf (Entry, "BarriersBeforeSignalAndWaitForProcess %s", s);
    if (BarriersBeforeSignalAndWait != NULL) {
        if (strlen (BarriersBeforeSignalAndWait) == 0) {
            BarriersBeforeSignalAndWait = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BarriersBeforeSignalAndWait, par_Fd);
    }

    sprintf (Entry, "BarriersBehindOnlySignalForProcess %s", s);
    if (BarriersBehindOnlySignal != NULL) {
        if (strlen (BarriersBehindOnlySignal) == 0) {
            BarriersBehindOnlySignal = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BarriersBehindOnlySignal, par_Fd);
    }

    sprintf (Entry, "BarriersBehindSignalAndWaitForProcess %s", s);
    if (BarriersBehindSignalAndWait != NULL) {
        if (strlen (BarriersBehindSignalAndWait) == 0) {
            BarriersBehindSignalAndWait = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BarriersBehindSignalAndWait, par_Fd);
    }

    // Loop out
    sprintf (Entry, "BarriersLoopOutBeforeOnlySignalForProcess %s", s);
    if (BarriersLoopOutBeforeOnlySignal != NULL) {
        if (strlen (BarriersLoopOutBeforeOnlySignal) == 0) {
            BarriersLoopOutBeforeOnlySignal = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BarriersLoopOutBeforeOnlySignal, par_Fd);
    }

    sprintf (Entry, "BarriersLoopOutBeforeSignalAndWaitForProcess %s", s);
    if (BarriersLoopOutBeforeSignalAndWait != NULL) {
        if (strlen (BarriersLoopOutBeforeSignalAndWait) == 0) {
            BarriersLoopOutBeforeSignalAndWait = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BarriersLoopOutBeforeSignalAndWait, par_Fd);
    }

    sprintf (Entry, "BarriersLoopOutBehindOnlySignalForProcess %s", s);
    if (BarriersLoopOutBehindOnlySignal != NULL) {
        if (strlen (BarriersLoopOutBehindOnlySignal) == 0) {
            BarriersLoopOutBehindOnlySignal = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BarriersLoopOutBehindOnlySignal, par_Fd);
    }

    sprintf (Entry, "BarriersLoopOutBehindSignalAndWaitForProcess %s", s);
    if (BarriersLoopOutBehindSignalAndWait != NULL) {
        if (strlen (BarriersLoopOutBehindSignalAndWait) == 0) {
            BarriersLoopOutBehindSignalAndWait = NULL;
        }
        IniFileDataBaseWriteString (SCHED_INI_SECTION, Entry, BarriersLoopOutBehindSignalAndWait, par_Fd);
    }


    // Write the INI Entry
    sprintf(tmp_str, "%d,%d#%d,%d,%d,%d,%d,%d,%d,%s,%d,%s,%d,%d", 
            priority, 
            time_steps,
            (int)delay,
            timeout,
            RangeControlBeforeActiveFlags,
            RangeControlBehindActiveFlags,
            RangeControlStopSchedFlag,
            RangeControlOutput,
            RangeErrorCounterFlag,
            RangeErrorCounter,
            RangeControlFlag,
            RangeControl,
            RangeControlPhysFlag,
            RangeControlLimitValues);
    if (IniFileDataBaseWriteString(SCHED_INI_SECTION, s, tmp_str, par_Fd) == 0) {
        return -1;
    }
    return 0;
}

int read_extprocinfos_from_ini (int par_Fd,
                                const char *ext_proc_name,
                                int *priority,
                                int *time_steps,
                                int *delay,
                                int *timeout,
                                char *RangeErrorCounter,
                                char *RangeControl,
                                unsigned int *RangeControlFlags,
                                char *BBPrefix,
                                char *Scheduler,
                                char *BarriersBeforeOnlySignal,
                                char *BarriersBeforeSignalAndWait,
                                char *BarriersBehindOnlySignal,
                                char *BarriersBehindSignalAndWait,
                                char *BarriersLoopOutBeforeOnlySignal,
                                char *BarriersLoopOutBeforeSignalAndWait,
                                char *BarriersLoopOutBehindOnlySignal,
                                char *BarriersLoopOutBehindSignalAndWait)
{
    char  tmp_str[INI_MAX_LINE_LENGTH];
    const char *s;
    char *h;
    char *p[13];
    char Entry[MAX_PATH];

    s = GetProcessNameFromPath (ext_proc_name);

    if (BBPrefix != NULL) {
        sprintf (Entry, "BBPrefixForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BBPrefix, 31, par_Fd);
    }
    if (Scheduler != NULL) {
        sprintf (Entry, "SchedulerForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "Scheduler", Scheduler, MAX_SCHEDULER_NAME_LENGTH, par_Fd);
    }
    if (BarriersBeforeOnlySignal != NULL) {
        sprintf (Entry, "BarriersBeforeOnlySignalForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersBeforeOnlySignal, INI_MAX_LINE_LENGTH, par_Fd);
    }
    if (BarriersBeforeSignalAndWait != NULL) {
        sprintf (Entry, "BarriersBeforeSignalAndWaitForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersBeforeSignalAndWait, INI_MAX_LINE_LENGTH, par_Fd);
    }
    if (BarriersBehindOnlySignal != NULL) {
        sprintf (Entry, "BarriersBehindOnlySignalForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersBehindOnlySignal, INI_MAX_LINE_LENGTH, par_Fd);
    }
    if (BarriersBehindSignalAndWait != NULL) {
        sprintf (Entry, "BarriersBehindSignalAndWaitForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersBehindSignalAndWait, INI_MAX_LINE_LENGTH, par_Fd);
    }
    if (BarriersLoopOutBeforeOnlySignal != NULL) {
        sprintf (Entry, "BarriersLoopOutBeforeOnlySignalForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersLoopOutBeforeOnlySignal, INI_MAX_LINE_LENGTH, par_Fd);
    }
    if (BarriersLoopOutBeforeSignalAndWait != NULL) {
        sprintf (Entry, "BarriersLoopOutBeforeSignalAndWaitForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersLoopOutBeforeSignalAndWait, INI_MAX_LINE_LENGTH, par_Fd);
    }
    if (BarriersLoopOutBehindOnlySignal != NULL) {
        sprintf (Entry, "BarriersLoopOutBehindOnlySignalForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersLoopOutBehindOnlySignal, INI_MAX_LINE_LENGTH, par_Fd);
    }
    if (BarriersLoopOutBehindSignalAndWait != NULL) {
        sprintf (Entry, "BarriersLoopOutBehindSignalAndWaitForProcess %s", s);
        IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", BarriersLoopOutBehindSignalAndWait, INI_MAX_LINE_LENGTH, par_Fd);
    }

    // Set the default values
    *priority = 150;
    *time_steps = 1;
    *delay = 0;
    *timeout = -1;
    if (RangeErrorCounter != NULL) *RangeErrorCounter = '\0';
    if (RangeControl != NULL) *RangeControl = '\0';
    if (RangeControlFlags != NULL) *RangeControlFlags = 0;
    // Read the INI entry
    if (IniFileDataBaseReadString(SCHED_INI_SECTION, s,
                                  "", tmp_str, sizeof(tmp_str) - 1,
                                  par_Fd) > 0) {

        // comma separated
        p[0]=p[1]=p[2]=p[3]=p[4]=p[5]=p[6]=p[7]=p[8]=p[9]=p[10]=p[11]=p[12] = NULL;
        StringCommaSeparate (tmp_str, &p[0], &p[1], &p[2], &p[3], &p[4], &p[5], &p[6], &p[7], &p[8], &p[9], &p[10], &p[11], &p[12], NULL);
        if (p[0] != NULL) *priority = strtol (p[0], NULL, 0);
        if (p[1] != NULL) {
            *time_steps = strtol (p[1], &h, 0);
            if (*h == '#') {
                *delay = strtol (h+1, NULL, 0);
            }
        }
        if (p[2] != NULL) *timeout = strtol (p[2], NULL, 0);
        if (RangeControlFlags != NULL) {
            if ((p[3] != NULL) && (strtol (p[3], NULL, 0))) *RangeControlFlags |= RANGE_CONTROL_BEFORE_ACTIVE_FLAG;   // active before
            if ((p[3] != NULL) && ((strtol (p[3], NULL, 0) & 0x2) == 0x2)) *RangeControlFlags |= RANGE_CONTROL_BEFORE_INIT_AS_DEACTIVE;   // erst mal deaktiv fuer alle Variable
            if ((p[4] != NULL) && (strtol (p[4], NULL, 0))) *RangeControlFlags |= RANGE_CONTROL_BEHIND_ACTIVE_FLAG;   // active behind
            if ((p[4] != NULL) && ((strtol (p[4], NULL, 0) & 0x2) == 0x2)) *RangeControlFlags |= RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE;   // erst mal deaktiv fuer alle Variable
            if ((p[5] != NULL) && (strtol (p[5], NULL, 0))) *RangeControlFlags |= RANGE_CONTROL_STOP_SCHEDULER_FLAG;   // scheduler stop
            if (p[6] != NULL) *RangeControlFlags |= strtol (p[6], NULL, 0) & 0xF;              // Messagebox, Write to File, ...
            if ((p[7] != NULL) && (strtol (p[7], NULL,0))) *RangeControlFlags |= RANGE_CONTROL_COUNTER_VARIABLE_FLAG;   // Range Error Counter
            if ((p[9] != NULL) && (strtol (p[9], NULL, 0))) *RangeControlFlags |= RANGE_CONTROL_CONTROL_VARIABLE_FLAG;   // Range Control Variable
            if ((p[11] != NULL) && (strtol (p[11], NULL, 0))) *RangeControlFlags |= RANGE_CONTROL_PHYSICAL_FLAG;   // Range Control physikalisch
            if ((p[12] != NULL) && (strtol (p[12], NULL, 0))) *RangeControlFlags |= RANGE_CONTROL_LIMIT_VALUES_FLAG;   // limit to min max range
        }
        if ((p[8] != NULL) && (RangeErrorCounter != NULL)) strcpy (RangeErrorCounter, p[8]); 
        if ((p[10] != NULL) && (RangeControl != NULL)) strcpy (RangeControl, p[10]); 
        return 0;
    } else return UNKNOWN_PROCESS;
}

int set_process_state (PID pid, int state)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == pid) {
                pTcb->state = state;
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return 0;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;     
}

int change_process_priority (char *pname, int prio, int time_steps, int delay)
{

    int x;
    int old_prio;

    if ((prio <= 1) || (prio > 1000)) return -1;
    if (pname == NULL) return -1;
    if (!strcmp (pname, "init")) return -1;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb, *pTcb2;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (!strcmpi (pname, pTcb->name)) {      // Prozess gefunden
                
                // Remove process from the process list
                if (pTcb->next != NULL) {
                    pTcb->next->prev = pTcb->prev;
                }
                if (pTcb->prev != NULL) {
                    pTcb->prev->next = pTcb->next;
                }
                // If first process erster Prozess, change list root
                if (SchedulerData[x].FirstTcb == pTcb) {
                    SchedulerData[x].FirstTcb = pTcb->next;
                }
                // If first process erster Prozess, change list end
                if (SchedulerData[x].LastTcb == pTcb) {
                    SchedulerData[x].LastTcb = pTcb->prev;
                }
                old_prio = pTcb->prio;
                pTcb->prio = prio;      // Change the priority
                pTcb->delay = delay;
                pTcb->time_steps = time_steps;
                // Search a Process with the same sample step rate "time_steps"
                for (pTcb2 = SchedulerData[x].FirstTcb; pTcb2 != NULL; pTcb2 = pTcb2->next) {
                    if (pTcb2->time_steps == pTcb->time_steps) {
                        pTcb->time_counter = (pTcb2->time_counter + pTcb2->delay) - pTcb->delay;
                        if (pTcb->time_counter < 0) {
                            pTcb->time_counter += pTcb->time_steps;
                        }
                        break;
                    }
                }

                 // There is no process inside the list
                if (SchedulerData[x].FirstTcb == NULL) {
                    SchedulerData[x].FirstTcb = SchedulerData[x].LastTcb = pTcb;
                } else {
                    // Search inside process list (belong the priority)
                    for (pTcb2 = SchedulerData[x].FirstTcb; pTcb2 != NULL;
                         pTcb2 = pTcb2->next) {
                        if (pTcb->prio < pTcb2->prio) {
                            break;
                        }
                    }
                    // Insert process accordingly
                    if (pTcb2 == NULL) {   // Add to the end
                        pTcb->prev = SchedulerData[x].LastTcb;
                        pTcb->next = NULL;
                        SchedulerData[x].LastTcb = pTcb->prev->next = pTcb;
                    } else if (pTcb2->prev == NULL) {  // Add to the beginning
                        pTcb->prev = NULL;
                        pTcb->next = pTcb2;
                        SchedulerData[x].FirstTcb = pTcb2->prev = pTcb;
                    } else {                       // Insert somewhere in the middle
                        pTcb->next = pTcb2;
                        pTcb->prev = pTcb2->prev;
                        pTcb->prev->next = pTcb->next->prev = pTcb;
                    }
                }
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return old_prio;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return UNKNOWN_PROCESS;     
}

// SVL file

typedef struct {
    char *ProcessName;
    char *SVLFileName;
    uint64_t TimeStamp;
} LOAD_SVL_AFTER_START_PROCESS_ELEMENT;

static LOAD_SVL_AFTER_START_PROCESS_ELEMENT *LoadSvlAfterStartPorcess;
static int LoadSvlAfterStartPorcessSize;
static int LoadSvlAfterStartPorcessPos;

static int GetAndRemoveEntryByName(const char *ProcessName, char *SvlFileName)
{
    int x;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < LoadSvlAfterStartPorcessPos; x++) {
        if (!Compare2ProcessNames(ProcessName, LoadSvlAfterStartPorcess[x].ProcessName)) {
            if (SvlFileName != NULL) strcpy(SvlFileName, LoadSvlAfterStartPorcess[x].SVLFileName);
            my_free(LoadSvlAfterStartPorcess[x].ProcessName);
            my_free(LoadSvlAfterStartPorcess[x].SVLFileName);
            for (;  x < (LoadSvlAfterStartPorcessPos-1); x++) {
                LoadSvlAfterStartPorcess[x] = LoadSvlAfterStartPorcess[x+1];
            }
            LoadSvlAfterStartPorcessPos--;
            LeaveCriticalSection (&PipeSchedCriticalSection);
            return 1;
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return 0;
}

static void SVLRemoveEntryByTimeStamp(uint64_t TimeStamp)
{
    int x;
    for (x = 0; x < LoadSvlAfterStartPorcessPos; x++) {
        if (TimeStamp > LoadSvlAfterStartPorcess[x].TimeStamp) {
            my_free(LoadSvlAfterStartPorcess[x].ProcessName);
            my_free(LoadSvlAfterStartPorcess[x].SVLFileName);
            for (;  x < (LoadSvlAfterStartPorcessPos-1); x++) {
                LoadSvlAfterStartPorcess[x] = LoadSvlAfterStartPorcess[x+1];
            }
            LoadSvlAfterStartPorcessPos--;
        }
    }
}

void SetSVLFileLoadedBeforeInitProcessFileName (const char *ProcessName,
                                                const char *SVLFileName, int INIFlag)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    if (INIFlag) {
        sprintf (Entry, "SVL before init %s", GetProcessNameFromPath (ProcessName));
        IniFileDataBaseWriteString(SCHED_INI_SECTION, Entry, SVLFileName, GetMainFileDescriptor());
    } else {
        int Pos;
        uint64_t TimeStamp = get_timestamp_counter();
        EnterCriticalSection (&PipeSchedCriticalSection);
        // Remove all that is older
        SVLRemoveEntryByTimeStamp(TimeStamp + 8 * (uint64_t)get_sched_periode_timer_clocks());
        Pos = LoadSvlAfterStartPorcessPos;
        LoadSvlAfterStartPorcessPos++;
        if (LoadSvlAfterStartPorcessPos >= LoadSvlAfterStartPorcessSize) {
            LoadSvlAfterStartPorcessSize = LoadSvlAfterStartPorcessPos;
            LoadSvlAfterStartPorcess = (LOAD_SVL_AFTER_START_PROCESS_ELEMENT*)my_realloc(LoadSvlAfterStartPorcess, sizeof(LOAD_SVL_AFTER_START_PROCESS_ELEMENT) * (size_t)LoadSvlAfterStartPorcessSize);
        }
        LoadSvlAfterStartPorcess[Pos].ProcessName = my_malloc (strlen(ProcessName)+1);
        strcpy(LoadSvlAfterStartPorcess[Pos].ProcessName, ProcessName);
        LoadSvlAfterStartPorcess[Pos].SVLFileName = my_malloc (strlen(SVLFileName)+1);
        strcpy(LoadSvlAfterStartPorcess[Pos].SVLFileName, SVLFileName);
        LoadSvlAfterStartPorcess[Pos].TimeStamp = get_timestamp_counter();
        LeaveCriticalSection (&PipeSchedCriticalSection);
    }
}

int GetSVLFileLoadedBeforeInitProcessFileName (const char *ProcessName,
                                               char *SVLFileName)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    if (GetAndRemoveEntryByName (ProcessName, SVLFileName)) {
        return (int)strlen (SVLFileName);
    } else {
        sprintf (Entry, "SVL before init %s", GetProcessNameFromPath (ProcessName));
        return (int)IniFileDataBaseReadString(SCHED_INI_SECTION, Entry,
                                              "", SVLFileName, MAX_PATH, GetMainFileDescriptor());
    }
}


// A2L file

typedef struct {
    char *ProcessName;
    char *A2LFileName;
    int UpdateAddrFlag;
    uint64_t TimeStamp;
} ASSOCIATED_A2L_TO_START_PROCESS_ELEMENT;

static ASSOCIATED_A2L_TO_START_PROCESS_ELEMENT *AssociatedA2LStartPorcess;
static int AssociatedA2LStartPorcessSize;
static int AssociatedA2LStartPorcessPos;

static int AssociatedA2LGetAndRemoveEntryByName(const char *ProcessName, char *A2LFileName, int *ret_UpdateFlags)
{
    int x;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < AssociatedA2LStartPorcessPos; x++) {
        if (!Compare2ProcessNames(ProcessName, AssociatedA2LStartPorcess[x].ProcessName)) {
            if (A2LFileName != NULL) strcpy(A2LFileName, AssociatedA2LStartPorcess[x].A2LFileName);
            if (ret_UpdateFlags != NULL) *ret_UpdateFlags = AssociatedA2LStartPorcess[x].UpdateAddrFlag;
            my_free(AssociatedA2LStartPorcess[x].ProcessName);
            my_free(AssociatedA2LStartPorcess[x].A2LFileName);
            for (;  x < (AssociatedA2LStartPorcessPos-1); x++) {
                AssociatedA2LStartPorcess[x] = AssociatedA2LStartPorcess[x+1];
            }
            AssociatedA2LStartPorcessPos--;
            LeaveCriticalSection (&PipeSchedCriticalSection);
            return 1;
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return 0;
}

static void AssociatedA2LRemoveEntryByTimeStamp(uint64_t TimeStamp)
{
    int x;
    for (x = 0; x < AssociatedA2LStartPorcessPos; x++) {
        if (TimeStamp > AssociatedA2LStartPorcess[x].TimeStamp) {
            my_free(AssociatedA2LStartPorcess[x].ProcessName);
            my_free(AssociatedA2LStartPorcess[x].A2LFileName);
            for (;  x < (AssociatedA2LStartPorcessPos-1); x++) {
                AssociatedA2LStartPorcess[x] = AssociatedA2LStartPorcess[x+1];
            }
            AssociatedA2LStartPorcessPos--;
        }
    }
}

void SetA2LFileAssociatedToProcessFileName (const char *ProcessName,
                                            const char *A2LFileName,
                                            int UpdateAddrFlag, int INIFlag)
{
    if (INIFlag) {
        char Entry[INI_MAX_ENTRYNAME_LENGTH];
        char Line[512+64];
        sprintf (Entry, "A2L associated to %s", GetProcessNameFromPath (ProcessName));
        sprintf (Line, "%s, 0x%X", A2LFileName, UpdateAddrFlag);
        IniFileDataBaseWriteString(SCHED_INI_SECTION, Entry, Line, GetMainFileDescriptor());
    } else {
        int Pos;
        uint64_t TimeStamp = get_timestamp_counter();
        EnterCriticalSection (&PipeSchedCriticalSection);
        // Remove all that is older
        AssociatedA2LRemoveEntryByTimeStamp(TimeStamp + 8 * (uint64_t)get_sched_periode_timer_clocks());
        Pos = AssociatedA2LStartPorcessPos;
        AssociatedA2LStartPorcessPos++;
        if (AssociatedA2LStartPorcessPos >= AssociatedA2LStartPorcessSize) {
            AssociatedA2LStartPorcessSize = AssociatedA2LStartPorcessPos;
            AssociatedA2LStartPorcess = (ASSOCIATED_A2L_TO_START_PROCESS_ELEMENT*)my_realloc(AssociatedA2LStartPorcess, sizeof(ASSOCIATED_A2L_TO_START_PROCESS_ELEMENT) * (size_t)AssociatedA2LStartPorcessSize);
        }
        AssociatedA2LStartPorcess[Pos].ProcessName = my_malloc (strlen(ProcessName)+1);
        strcpy(AssociatedA2LStartPorcess[Pos].ProcessName, ProcessName);
        AssociatedA2LStartPorcess[Pos].A2LFileName = my_malloc (strlen(A2LFileName)+1);
        strcpy(AssociatedA2LStartPorcess[Pos].A2LFileName, A2LFileName);
        AssociatedA2LStartPorcess[Pos].UpdateAddrFlag = UpdateAddrFlag;
        AssociatedA2LStartPorcess[Pos].TimeStamp = get_timestamp_counter();
        LeaveCriticalSection (&PipeSchedCriticalSection);
    }
}

int GetA2LFileAssociatedProcessFileName (const char *ProcessName,
                                         char *A2LFileName,
                                         int *UpdateAddrFlag)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[512+64];

    if (AssociatedA2LGetAndRemoveEntryByName (ProcessName, A2LFileName, UpdateAddrFlag)) {
        return (int)strlen (A2LFileName);
    } else {
        int Ret;
        char *a, *u;
        sprintf (Entry, "A2L associated to %s", GetProcessNameFromPath (ProcessName));
        Ret = (int)IniFileDataBaseReadString(SCHED_INI_SECTION, Entry,
                                              "", Line, sizeof(Line), GetMainFileDescriptor());
        strcpy(A2LFileName, "");
        *UpdateAddrFlag = 0;
        switch (StringCommaSeparate(Line, &a, &u, NULL)) {
        case 2:
            *UpdateAddrFlag = strtoul(u, NULL, 0);
            strcpy(A2LFileName, a);
            break;
        case 1:
            strcpy(A2LFileName, a);
            break;
        default:
            break;
        }
        return (int)strlen(A2LFileName);
    }
}


void SetBeforeProcessEquationFileName (const char *ProcessName,
                                       const char *EquFileName)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    sprintf (Entry, "Equation before %s", GetProcessNameFromPath (ProcessName));
    IniFileDataBaseWriteString(SCHED_INI_SECTION, Entry, EquFileName, GetMainFileDescriptor());
}

void SetBehindProcessEquationFileName (const char *ProcessName,
                                       const char *EquFileName)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    sprintf (Entry, "Equation behind %s", GetProcessNameFromPath (ProcessName));
    IniFileDataBaseWriteString(SCHED_INI_SECTION, Entry, EquFileName, GetMainFileDescriptor());
}

int GetBeforeProcessEquationFileName (const char *ProcessName,
                                      char *EquFileName)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    sprintf (Entry, "Equation before %s", GetProcessNameFromPath (ProcessName));
    return (int)IniFileDataBaseReadString(SCHED_INI_SECTION, Entry,
                                          "", EquFileName, MAX_PATH, GetMainFileDescriptor());
}

int GetBehindProcessEquationFileName (const char *ProcessName,
                                      char *EquFileName)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    sprintf (Entry, "Equation behind %s", GetProcessNameFromPath (ProcessName));
    return (int)IniFileDataBaseReadString(SCHED_INI_SECTION, Entry,
                                          "", EquFileName, MAX_PATH, GetMainFileDescriptor());
}

void DelBeforeProcessEquationFile (const char *ProcessName)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    sprintf (Entry, "Equation before %s", GetProcessNameFromPath (ProcessName));
    IniFileDataBaseWriteString(SCHED_INI_SECTION, Entry, NULL, GetMainFileDescriptor());
}

void DelBehindProcessEquationFile (const char *ProcessName)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    sprintf (Entry, "Equation behind %s", GetProcessNameFromPath (ProcessName));
    IniFileDataBaseWriteString(SCHED_INI_SECTION, Entry, NULL, GetMainFileDescriptor());
}

static void CheckEnvironmentVariale(void)
{
    char Path[MAX_PATH];
    // 1. XilEnv EnvVar
    SearchAndReplaceEnvironmentStrings("%XILENV_DLL_PATH%", Path, sizeof(Path));
    if (!strcmp("%XILENV_DLL_PATH%", Path)) {
        // 2. System EnvVar
        int Ret = GetEnvironmentVariable("XILENV_DLL_PATH", Path, sizeof(Path));
        if ((Ret == 0) || (Ret >= MAX_PATH)) {
            // 3. XilEnv EXE path
            SearchAndReplaceEnvironmentStrings("%XILENV_EXE_DIR%", Path, sizeof(Path));
        }
    }
    SetEnvironmentVariable("XILENV_DLL_PATH", Path);

    if (GetNoXcp ()) {
        SetEnvironmentVariable("XilEnv_NoXcp", "yes");
    }
#define ENVIRONMENT_VARNAME_INSTANCE    "XilEnv_Instance"
#define ENVIRONMENT_VARNAME_PORT        "XilENv_Port"
    if (strlen(s_main_ini_val.InstanceName)) SetEnvironmentVariable(ENVIRONMENT_VARNAME_INSTANCE, s_main_ini_val.InstanceName);
    if (s_main_ini_val.ExternProcessLoginSocketPort > 0) {
        char Help[64];
        sprintf (Help, "%i", s_main_ini_val.ExternProcessLoginSocketPort);
        SetEnvironmentVariable(ENVIRONMENT_VARNAME_PORT, Help);
    }
}

int IsFmuFile(const char *name)
{
    const char *p = name;
    while (*p != 0) p++;
    if ((p - name) > 4) {
        if ((*(p - 4) == '.') &&
            ((*(p - 3) == 'F') || (*(p - 3) == 'f')) &&
            ((*(p - 2) == 'M') || (*(p - 2) == 'm')) &&
            ((*(p - 1) == 'U') || (*(p - 1) == 'u'))) {
            return 1;
        }
    }
    return 0;
}

int IsScqFile(const char *name)
{
    const char *p = name;
    while (*p != 0) p++;
    if ((p - name) > 4) {
        if ((*(p - 4) == '.') &&
            ((*(p - 3) == 'S') || (*(p - 3) == 's')) &&
            ((*(p - 2) == 'C') || (*(p - 2) == 'c')) &&
            ((*(p - 1) == 'Q') || (*(p - 1) == 'q'))) {
            return 1;
        }
    }
    return 0;
}

#ifdef _WIN32
static void StringCopyChangeForwardSlashesToBackSlashes(char *par_Dst, const char *par_Src)
{
    const char *s = par_Src;
    char *d = par_Dst;
    while (*s != 0) {
        if (*s == '/') *d = '\\';
        else *d = *s;
        d++;
        s++;
    }
    *d = 0;
}
#else
static void StringCopyChangeBackSlashesToForwardSlashes(char *par_Dst, const char *par_Src)
{
    const char *s = par_Src;
    char *d = par_Dst;
    while (*s != 0) {
        if (*s == '\\') *d = '/';
        else *d = *s;
        d++;
        s++;
    }
    *d = 0;
}
#endif

int activate_extern_process (const char *name, int timeout, char **ret_NoErrMsg)
{
#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char *CmdLine;

    ZeroMemory (&si, sizeof (si));
    si.cb = sizeof (si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_MINIMIZE;
    ZeroMemory(&pi, sizeof(pi));

    CheckEnvironmentVariale();

    if (IsFmuFile(name)) {
        CmdLine = my_malloc (MAX_PATH + strlen (name) + strlen (s_main_ini_val.InstanceName) + 32);
        SearchAndReplaceEnvironmentStrings("%XILENV_EXE_DIR%", CmdLine, MAX_PATH);
        strcat (CmdLine, "\\ExtProc_FMUExtract.exe -fmu ");
        if (strlen (s_main_ini_val.InstanceName) > 0) {
            sprintf (CmdLine + strlen(CmdLine), "\"%s\" -i %s", name, s_main_ini_val.InstanceName);
        } else {
            sprintf (CmdLine + strlen(CmdLine), "\"%s\"", name);
        }
    } else if (IsScqFile(name)) {
        CmdLine = my_malloc (MAX_PATH + strlen (name) + strlen (s_main_ini_val.InstanceName) + 32);
        SearchAndReplaceEnvironmentStrings("%XILENV_EXE_DIR%", CmdLine, MAX_PATH);
        strcat (CmdLine, "\\ExtProc_Qemu.exe -scq ");
        if (strlen (s_main_ini_val.InstanceName) > 0) {
            sprintf (CmdLine + strlen(CmdLine), "\"%s\" -i %s", name, s_main_ini_val.InstanceName);
        } else {
            sprintf (CmdLine + strlen(CmdLine), "\"%s\"", name);
        }
    } else {
        CmdLine = my_malloc (strlen (name) + strlen (s_main_ini_val.InstanceName) + 32);
        if (strlen (s_main_ini_val.InstanceName) > 0) {
            sprintf (CmdLine, "\"%s\" -i %s", name, s_main_ini_val.InstanceName);
        } else {
            sprintf (CmdLine, "\"%s\"", name);
        }
    }

    // Use the largest timeout ([Scheduler] \ Login Timeout oder START_PROCESS_EX
    if (timeout > s_main_ini_val.ExternProcessLoginTimeout) {
        AddExecutableToTimeoutControl (name, timeout);
    } else {
        AddExecutableToTimeoutControl (name, s_main_ini_val.ExternProcessLoginTimeout);
    }
    // Start external process
    if (!CreateProcess (NULL, CmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) {

        char *lpMsgBuf;
        DWORD dw = GetLastError ();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        RemoveExecutableFromTimeoutControl (name);
        if (ret_NoErrMsg != NULL) {
            *ret_NoErrMsg = my_malloc(strlen(lpMsgBuf)+1);
            if (*ret_NoErrMsg != NULL) strcpy(*ret_NoErrMsg, lpMsgBuf);
        } else {
            ThrowError (1, "error cannot start process \"%s\" \"%s\" \"%s\"", name, CmdLine, lpMsgBuf);
        }
        LocalFree (lpMsgBuf);
        my_free (CmdLine);
        return UNKNOWN_PROCESS;
    } else {
        // Close process and thread handles.
         CloseHandle(pi.hProcess);
         CloseHandle(pi.hThread);
    }
    my_free (CmdLine);
#else
    UNUSED(ret_NoErrMsg);
    char *CmdLine;
    const char *Args[10];
    int ArgPos = 0;
    memset(Args, 0, sizeof(Args));

    CheckEnvironmentVariale();

    if (IsFmuFile(name)) {
        CmdLine = my_malloc (MAX_PATH);
        SearchAndReplaceEnvironmentStrings("%XILENV_EXE_DIR%", CmdLine, MAX_PATH);
        strcat (CmdLine, "/ExtProc_FMUExtract.EXE");
        Args[ArgPos++] = CmdLine;
        Args[ArgPos++] = "-fmu";
        Args[ArgPos++] = name;
    } else if (IsScqFile(name)) {
        CmdLine = my_malloc (MAX_PATH + strlen (name) + strlen (s_main_ini_val.InstanceName) + 32);
        SearchAndReplaceEnvironmentStrings("%XILENV_EXE_DIR%", CmdLine, MAX_PATH);
        strcat (CmdLine, "/ExtProc_Qemu.EXE");
        Args[ArgPos++] = CmdLine;
        Args[ArgPos++] = "-scq";
        Args[ArgPos++] = name;
    } else {
        CmdLine = my_malloc (strlen (name)+ 1);
        StringCopyChangeBackSlashesToForwardSlashes(CmdLine, name);
        Args[ArgPos++] = CmdLine;
    }
    if (strlen (s_main_ini_val.InstanceName) > 0) {
        Args[ArgPos++] = "-i";
        Args[ArgPos++] = s_main_ini_val.InstanceName;
    }
    // Use the largest timeout ([Scheduler] \ Login Timeout oder START_PROCESS_EX
    if (timeout > s_main_ini_val.ExternProcessLoginTimeout) {
        AddExecutableToTimeoutControl (name, timeout);
    } else {
        AddExecutableToTimeoutControl (name, s_main_ini_val.ExternProcessLoginTimeout);
    }
    // Start externen process
    switch (fork()) {
    case 0: // Child
        {
            /*FILE *fh = fopen("/tmp/fork.txt", "wt");
            fprintf (fh, "start: \"%s\"\n", CmdLine);
            for (int x = 0; Args[x] != NULL; x++) {
                fprintf(fh, "%i: \"%s\"\n", x, Args[x]);
            }
            fflush(fh);*/
            if (execv(CmdLine, (char*const*)Args)) { // will only return if an error occur
                /*fprintf (fh, "cannot start %s", strerror(errno));
                fflush(fh);*/
                exit(1);
            }
            /*fprintf(fh, "this should never printed\n");
            fflush(fh);*/
        }
        break;
    case -1:
        *ret_NoErrMsg = my_malloc (strlen (strerror(errno)) + 1);
        strcpy(*ret_NoErrMsg, strerror(errno));
        return -1;
    }
#endif
    return 0;
}

void activate_extern_process_free_err_msg (char *par_ErrMsg)
{
    if (par_ErrMsg != NULL) {
        my_free (par_ErrMsg);
    }
}


double GetRealtimeFactor (void)
{
    // Always main scheduler
    return SchedulerData[0].RealtimeFactorFiltered;
}

uint32_t GetCycleCounter (void)
{
    return (uint32_t)SchedulerData[0].Cycle;
}

uint64_t GetCycleCounter64 (void)
{
    return SchedulerData[0].Cycle;
}


int get_real_running_process_name (char *pname)
{

    TASK_CONTROL_BLOCK *pTcb = GetPointerToTaskControlBlock (GetCurrentPid ());
    if (pTcb == NULL) {
        strcpy (pname, GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME));
        return 0;  // it is not inside a process
    } else {
        if (pTcb->Type == EXTERN_ASYNC) {
            if (GetProsessShortExeFilename(pTcb->name, pname, MAX_PATH) != 0) {
                strcpy (pname, pTcb->name);
            }
        } else {
            strcpy (pname, pTcb->name);
        }
        return 1;
    }
}

int SetPriority (char *txt)
{
    int Ret = 0;
    int priority = THREAD_PRIORITY_NORMAL;

    if (!strcmp ("PRIORITY_NORMAL_NO_SLEEP", txt)) {
        priority = THREAD_PRIORITY_NORMAL;
        Ret = 1;
    } else if (!strcmp ("PRIORITY_NORMAL", txt)) priority = THREAD_PRIORITY_NORMAL;
    else if (!strcmp ("PRIORITY_BELOW_NORMAL", txt)) priority = THREAD_PRIORITY_BELOW_NORMAL;
    else if (!strcmp ("PRIORITY_LOWEST", txt)) priority = THREAD_PRIORITY_LOWEST;
    else if (!strcmp ("PRIORITY_IDLE", txt)) priority = THREAD_PRIORITY_IDLE;
#ifdef _WIN32
    if (SetThreadPriority (GetCurrentThread (), priority) == 0) {
        ThrowError (1, "cannot set priority");
    }
#else
    pthread_setschedprio(pthread_self(), priority);
#endif
    return Ret;
}

int wait_for_termination (void)
{
    int x, Ret = 0;
    for (x = 0; x < SchedulerCount; x++) {
        if (SchedulerData[x].State != SCHED_IS_TERMINATED_STATE) {
            Ret = 1;
        }
    }
    return Ret;
}


int GetProsessLongExeFilename (const char *par_ProcessName, char *ret_ExeFilename, int par_MaxChars)
{
    const char *p = par_ProcessName;
    const char *ppo = NULL;
    const char *pka = NULL;
    const char *pbs = NULL;

    while (*p != 0) {
        if (*p == '.') ppo = p;
        else if (*p == '@') pka = p;
        else if ((*p == '\\') || (*p == '/')) pbs = p + 1;
        p++;
    }
    if (ppo > pka) pka = NULL;
    if (pbs > pka) pbs = NULL;
    if (ppo != NULL) {  // Last '.' inside process name
        if ((((ppo[1] == 'e') || (ppo[1] == 'E')) &&
             ((ppo[2] == 'x') || (ppo[2] == 'X')) &&
             ((ppo[3] == 'e') || (ppo[3] == 'E'))) ||  // followed by "EXE"
            (((ppo[1] == 'f') || (ppo[1] == 'F')) &&
             ((ppo[2] == 'm') || (ppo[2] == 'M')) &&
             ((ppo[3] == 'u') || (ppo[3] == 'U')))) {  // followed by "FMU"
            int Len;
            if (pka != NULL) {
                Len = (int)(pka - par_ProcessName);
            } else {
                Len = (int)(p - par_ProcessName);
            }
            if ((Len + 1) >= par_MaxChars) {
                return -1;   // Error name to long
            }
            MEMCPY (ret_ExeFilename, par_ProcessName, (size_t)Len);
            ret_ExeFilename[Len] = 0;
            return 0;
        }
    }
    return -2; // It is not external process
}


int GetProsessShortExeFilename (const char *par_ProcessName, char *ret_ExeFilename, int par_MaxChars)
{
    const char *p = par_ProcessName;
    const char *ppo = NULL;
    const char *pka = NULL;
    const char *pbs = NULL;

    while (*p != 0) {
        if (*p == '.') ppo = p;
        else if (*p == '@') pka = p;
        else if ((*p == '\\') || (*p == '/')) pbs = p + 1;
        p++;
    }
    if (ppo > pka) pka = NULL;
    if ((pka != NULL) && (pbs > pka)) pbs = NULL;
    if (ppo != NULL) {  // Last '.' inside process name
        if ((((ppo[1] == 'e') || (ppo[1] == 'E')) &&
             ((ppo[2] == 'x') || (ppo[2] == 'X')) &&
             ((ppo[3] == 'e') || (ppo[3] == 'E'))) ||
            (((ppo[1] == 'f') || (ppo[1] == 'F')) &&
             ((ppo[2] == 'm') || (ppo[2] == 'M')) &&
             ((ppo[3] == 'u') || (ppo[3] == 'U'))) ||
            (((ppo[1] == 's') || (ppo[1] == 'S')) &&
             ((ppo[2] == 'c') || (ppo[2] == 'C')) &&
             ((ppo[3] == 'q') || (ppo[3] == 'Q'))) ||
            (((ppo[1] == 'e') || (ppo[1] == 'E')) &&
             ((ppo[2] == 'l') || (ppo[2] == 'L')) &&
             ((ppo[3] == 'f') || (ppo[3] == 'F'))) ||
            (((ppo[1] == 'd') || (ppo[1] == 'D')) &&
             ((ppo[2] == 'l') || (ppo[2] == 'L')) &&
             ((ppo[3] == 'l') || (ppo[3] == 'L')))) {  // followed by "EXE", "FMU" oder "DLL"
            int Len;
            if (pbs != NULL){
                par_ProcessName = pbs;
            }
            if (pka != NULL) {
                Len = (int)(pka - par_ProcessName);
            } else {
                Len = (int)(p - par_ProcessName);
            }
            if ((Len + 1) >= par_MaxChars) {
                return -1;   // Error name to long
            }
            MEMCPY (ret_ExeFilename, par_ProcessName, (size_t)Len);
            ret_ExeFilename[Len] = 0;
            return 0;
        }
    }
    return -2; // It is not external process
}

int GetShortDllFilename (const char *par_DllLongName, char *ret_DllShortName, int par_MaxChars)
{
    const char *p = par_DllLongName;
    const char *ppo = NULL;
    const char *pka = NULL;
    const char *pbs = NULL;

    while (*p != 0) {
        if (*p == '.') ppo = p;
        else if (*p == '@') pka = p;
        else if ((*p == '\\') || (*p == '/')) pbs = p + 1;
        p++;
    }
    if (ppo > pka) pka = NULL;
    if ((pka != NULL) && (pbs > pka)) pbs = NULL;
    if (ppo != NULL) {  // last '.' inside process name
        if (((ppo[1] == 'd') || (ppo[1] == 'D')) &&
            ((ppo[2] == 'l') || (ppo[2] == 'L')) &&
            ((ppo[3] == 'l') || (ppo[3] == 'L'))) {  // followed by "DLL"
            int Len;
            if (pbs != NULL){
                par_DllLongName = pbs;
            }
            if (pka != NULL) {
                Len = (int)(pka - par_DllLongName);
            } else {
                Len = (int)(p - par_DllLongName);
            }
            if ((Len + 1) >= par_MaxChars) {
                return -1;   // Error name to long
            }
            MEMCPY (ret_DllShortName, par_DllLongName, (size_t)Len);
            ret_DllShortName[Len] = 0;
            return 0;
        }
    }
    return -2; // it is not external process
}

#define MAX_REDRYS   16
//#define RETRY_DEBUG_PRINT
static int LockAllProcessesInsideOneExecutable (char *par_ExecutableName, int par_MaxWaitTime, int par_ErrorBehavior, const char *par_OperationDescription,
                                               int *ret_PidsSameExe, int *ret_PidsSameExeCount, int par_SizeOfPidsSameExe, const char *par_FileName, int par_LineNr)
{
    int x;
    TASK_CONTROL_BLOCK *LockedTcbs[64];
    int Pos = 0;
    int RetryCounter = 0;

__retry:
    Pos = 0;
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if ((pTcb->InsideExecutableName != NULL) &&
                !strcmp(pTcb->InsideExecutableName, par_ExecutableName)) {
                if (pTcb->LockedByThreadId == GetCurrentThreadId()) {
                    char ShortProcessName[MAX_PATH];
                    TruncatePathFromProcessName (ShortProcessName, pTcb->name);
                    ThrowError (ERROR_CRITICAL, "Thread %i has already locked the process (%s)!",
                           pTcb->LockedByThreadId, ShortProcessName);
                    return -1;
                }
#ifdef _WIN32
                DWORD WaitResult = WaitForSingleObject (pTcb->Mutex,    // handle to mutex
                                                        (DWORD)par_MaxWaitTime);  // no time-out interval
                switch (WaitResult) {
                case WAIT_OBJECT_0:
                    if (pTcb->LockedByThreadId != 0) {
                        ThrowError (1, "This should never happen %s (%i)!", __FILE__, __LINE__);
                    }
                    pTcb->LockedByThreadId = GetCurrentThreadId();
                    pTcb->SrcFileName = par_FileName;
                    pTcb->SrcLineNr = par_LineNr;
                    LockedTcbs[Pos] = pTcb;
                    Pos++;
                    if ((*ret_PidsSameExeCount+1) < par_SizeOfPidsSameExe) {
                        ret_PidsSameExe[*ret_PidsSameExeCount] = pTcb->pid;
                        (*ret_PidsSameExeCount)++;
                    }
                    break; // alles OK
                case WAIT_TIMEOUT:
                    RetryCounter++;
                    if (RetryCounter > MAX_REDRYS) {
                        if (par_ErrorBehavior == ERROR_BEHAVIOR_ERROR_MESSAGE) {
                            char ShortProcessName[MAX_PATH];
                            TruncatePathFromProcessName (ShortProcessName, pTcb->name);
                            if (par_OperationDescription == NULL) {
                                par_OperationDescription = "";
                            }
                            if (ThrowError (ERROR_OKCANCEL, "timeout during wait till the end of the cyclic function of process \"%s\" reached. "
                                                       "OK to continue wait or Cancle to terminate the operation \"%s\"", ShortProcessName, par_OperationDescription)
                                    == IDCANCEL) {
                                return -1;
                            }
                        } else {
                            return -1;
                        }
                    } else {
#ifdef RETRY_DEBUG_PRINT
                        // release all locks and retry again
                        FILE *fh = fopen ("c:\\temp\\retry.txt", "at");
                        if (fh != NULL) {
                            fprintf (fh, "(%i) cannot lock process \"%s\"\n"
                                         "release all locks and retry again\n", RetryCounter, pTcb->name);
                        }
#endif
                        *ret_PidsSameExeCount = 0;
                        for (Pos--; Pos >= 0; Pos--) {
#ifdef RETRY_DEBUG_PRINT
                            if (fh != NULL) {
                                fprintf (fh, "  release \"%s\"\n", LockedTcbs[Pos]->name);
                            }
#endif
                            LockedTcbs[Pos]->LockedByThreadId = 0;
                            LockedTcbs[Pos]->SrcFileName = NULL;
                            LockedTcbs[Pos]->SrcLineNr = 0;
                            ReleaseMutex(LockedTcbs[Pos]->Mutex);
                        }
#ifdef RETRY_DEBUG_PRINT
                        if (fh != NULL) {
                            fclose(fh);
                        }
#endif
                        goto __retry;
                    }
                    break;
                default:
                    /*{
                        char *lpMsgBuf;
                        DWORD dw = GetLastError ();
                        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                       FORMAT_MESSAGE_FROM_SYSTEM |
                                       FORMAT_MESSAGE_IGNORE_INSERTS,
                                       NULL,
                                       dw,
                                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                       (LPTSTR) &lpMsgBuf,
                                       0, NULL );

                         ThrowError (1, "error during wait till the end of the cyclic function of process \"%s\" reached \"%s\"", pTcb->name, lpMsgBuf);
                         LocalFree (lpMsgBuf);
                    }
                    break;*/
                    return -1;
                }
#else
                struct timespec to;
                to.tv_sec = time(NULL) + par_MaxWaitTime / 1000;
                to.tv_sec = 0;

                if (pthread_mutex_timedlock(&(pTcb->Mutex), &to) == 0) {
                    if (pTcb->LockedByThreadId != 0) {
                        ThrowError (1, "This should never happen %s (%i)!", __FILE__, __LINE__);
                    }
                    pTcb->LockedByThreadId = GetCurrentThreadId();
                    pTcb->SrcFileName = par_FileName;
                    pTcb->SrcLineNr = par_LineNr;
                    if ((*ret_PidsSameExeCount+1) < par_SizeOfPidsSameExe) {
                        ret_PidsSameExe[*ret_PidsSameExeCount] = pTcb->pid;
                        (*ret_PidsSameExeCount)++;
                    }
                    break; // alles OK
                } else {
                    RetryCounter++;
                    if (RetryCounter > MAX_REDRYS) {
                        if (par_ErrorBehavior == ERROR_BEHAVIOR_ERROR_MESSAGE) {
                            char ShortProcessName[MAX_PATH];
                            TruncatePathFromProcessName(ShortProcessName, pTcb->name);
                            if (par_OperationDescription == NULL) {
                                par_OperationDescription = "";
                            }
                            if (ThrowError(ERROR_OKCANCEL, "timeout during wait till the end of the cyclic function of process \"%s\" reached. "
                                "OK to continue wait or Cancle to terminate the operation \"%s\"", ShortProcessName, par_OperationDescription)
                                == IDCANCEL) {
                                return -1;
                            }
                        }
                        else {
                            return -1;
                        }
                    }
                    else {
#ifdef RETRY_DEBUG_PRINT
                        // release all locks and retry again
                        FILE *fh = fopen("/temp/retry.txt", "at");
                        if (fh != NULL) {
                            fprintf(fh, "(%i) cannot lock process \"%s\"\n"
                                "release all locks and retry again\n", RetryCounter, pTcb->name);
                        }
#endif
                        *ret_PidsSameExeCount = 0;
                        for (Pos--; Pos >= 0; Pos--) {
#ifdef RETRY_DEBUG_PRINT
                            if (fh != NULL) {
                                fprintf(fh, "  release \"%s\"\n", LockedTcbs[Pos]->name);
                            }
#endif
                            LockedTcbs[Pos]->LockedByThreadId = 0;
                            LockedTcbs[Pos]->SrcFileName = NULL;
                            LockedTcbs[Pos]->SrcLineNr = 0;
                            pthread_mutex_unlock(&(LockedTcbs[Pos]->Mutex));
                        }
#ifdef RETRY_DEBUG_PRINT
                        if (fh != NULL) {
                            fclose(fh);
                        }
#endif
                        goto __retry;
                    }
                }
#endif
            }
        }
    }
    return 0;
}

int WaitUntilProcessIsNotActiveAndThanLockItEx (int par_Pid, int par_MaxWaitTime, int par_ErrorBehavior, const char *par_OperationDescription,
                                                int *ret_PidsSameExe, int *ret_PidsSameExeCount, int par_SizeOfPidsSameExe,
                                                const char *par_FileName, int par_LineNr)
{
    int x;
#ifndef _WIN32
    int MaxWaitTime;
#endif
    // Bei Echtzeitprozesse ist kein Lock moeglich
    if (s_main_ini_val.ConnectToRemoteMaster) {
        if ((par_Pid & 0x10000000) == 0x10000000) {
            *ret_PidsSameExeCount = 1;
            ret_PidsSameExe[0] = par_Pid;
            return 0;
        }
    }

    *ret_PidsSameExeCount = 0;

    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {

                if (pTcb->NumberOfProcessesInsideExecutable > 1) {
                    return LockAllProcessesInsideOneExecutable(pTcb->InsideExecutableName, par_MaxWaitTime, par_ErrorBehavior, par_OperationDescription,
                                                               ret_PidsSameExe, ret_PidsSameExeCount, par_SizeOfPidsSameExe, par_FileName, par_LineNr);
                }

                if (pTcb->LockedByThreadId == GetCurrentThreadId()) {
                    char ShortProcessName[MAX_PATH];
                    TruncatePathFromProcessName (ShortProcessName, pTcb->name);
                    ThrowError (ERROR_CRITICAL, "Thread %i has already locked the process (%s)!",
                           pTcb->LockedByThreadId, ShortProcessName);
                    return -1;
                }
#ifdef _WIN32
                while (1) {
                    DWORD WaitResult = WaitForSingleObject (pTcb->Mutex,    // handle to mutex
                                                            (DWORD)par_MaxWaitTime);  // no time-out interval
                    switch (WaitResult) {
                    case WAIT_OBJECT_0:
                        if (pTcb->LockedByThreadId != 0) {
                            ThrowError (1, "This should never happen %s (%i)!", __FILE__, __LINE__);
                        }
                        pTcb->LockedByThreadId = GetCurrentThreadId();
                        pTcb->SrcFileName = par_FileName;
                        pTcb->SrcLineNr = par_LineNr;
                        if ((*ret_PidsSameExeCount+1) < par_SizeOfPidsSameExe) {
                            ret_PidsSameExe[*ret_PidsSameExeCount] = pTcb->pid;
                            (*ret_PidsSameExeCount)++;
                        }
                        return 0;
                    case WAIT_TIMEOUT:
                        if (par_ErrorBehavior == ERROR_BEHAVIOR_ERROR_MESSAGE) {
                            char ShortProcessName[MAX_PATH];
                            TruncatePathFromProcessName (ShortProcessName, pTcb->name);
                            if (par_OperationDescription == NULL) {
                                par_OperationDescription = "";
                            }
                            if (ThrowError (ERROR_OKCANCEL, "timeout during wait till the end of the cyclic function of process \"%s\" reached. "
                                                       "OK to continue wait or Cancle to terminate the operation \"%s\"", ShortProcessName, par_OperationDescription)
                                    == IDCANCEL) {
                                return -1;
                            }
                        } else {
                            return -1;
                        }
                        break;
                    default:
                        /*{
                            char *lpMsgBuf;
                            DWORD dw = GetLastError ();
                            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                           FORMAT_MESSAGE_FROM_SYSTEM |
                                           FORMAT_MESSAGE_IGNORE_INSERTS,
                                           NULL,
                                           dw,
                                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                           (LPTSTR) &lpMsgBuf,
                                           0, NULL );

                             ThrowError (1, "error during wait till the end of the cyclic function of process \"%s\" reached \"%s\"", pTcb->name, lpMsgBuf);
                             LocalFree (lpMsgBuf);
                        }
                        break;*/
                        return -1;
                    }
                }
#else
                MaxWaitTime = par_MaxWaitTime;
                while (1) {
                    if (pthread_mutex_trylock(&(pTcb->Mutex)) == 0) {
                        if (pTcb->LockedByThreadId != 0) {
                            ThrowError (1, "This should never happen %s (%i)!", __FILE__, __LINE__);
                        }
                        pTcb->LockedByThreadId = GetCurrentThreadId();
                        pTcb->SrcFileName = par_FileName;
                        pTcb->SrcLineNr = par_LineNr;
                        if ((*ret_PidsSameExeCount+1) < par_SizeOfPidsSameExe) {
                            ret_PidsSameExe[*ret_PidsSameExeCount] = pTcb->pid;
                            (*ret_PidsSameExeCount)++;
                        }
                        /*if (pTcb->NumberOfProcessesInsideExecutable > 1) {
                            return LockAllOtherProcessesForExecutable(x, pTcb->InsideExecutableName, par_MaxWaitTime, par_ErrorBehavior, par_OperationDescription,
                                                                      ret_PidsSameExe, ret_PidsSameExeCount, par_SizeOfPidsSameExe, par_FileName, par_LineNr);
                        } else {
                            return 0;
                        }*/
                        return 0;
                    } else {
                        if (MaxWaitTime > 0) {
                            MaxWaitTime -= 10;  // 10ms
                            usleep(10*1000);
                        } else {
                            if (par_ErrorBehavior == ERROR_BEHAVIOR_ERROR_MESSAGE) {
                                char ShortProcessName[MAX_PATH];
                                TruncatePathFromProcessName (ShortProcessName, pTcb->name);
                                if (par_OperationDescription == NULL) {
                                    par_OperationDescription = "";
                                }
                                if (ThrowError (ERROR_OKCANCEL, "timeout during wait till the end of the cyclic function of process \"%s\" reached. "
                                                           "OK to continue wait or Cancle to terminate the operation \"%s\"", ShortProcessName, par_OperationDescription)
                                        == IDCANCEL) {
                                    return -1;
                                } else {
                                    MaxWaitTime = par_MaxWaitTime;  // nochmals Timeout abwarten
                                }
                            } else {
                                return -1;
                            }
                        }
                    }
                }
#endif
            }
        }
    }
    return -1;
}

int WaitUntilProcessIsNotActiveAndThanLockIt (int par_Pid, int par_MaxWaitTime, int par_ErrorBehavior, const char *par_OperationDescription,
                                              const char *par_FileName, int par_LineNr)
{
    int PidSameExe[16];
    int PidSameExeCount;
    return WaitUntilProcessIsNotActiveAndThanLockItEx (par_Pid, par_MaxWaitTime, par_ErrorBehavior, par_OperationDescription,
                                                       PidSameExe, &PidSameExeCount, 16,
                                                       par_FileName, par_LineNr);

}


static int UnLockAllOtherProcessesForExecutable(int par_Pid, char *par_ExecutableName)
{
    int x;
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if ((pTcb->pid != par_Pid) &&
                (pTcb->InsideExecutableName != NULL) &&
                !strcmp(pTcb->InsideExecutableName, par_ExecutableName)) {
                if (pTcb->LockedByThreadId != GetCurrentThreadId()) {
                    ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
                }
                pTcb->LockedByThreadId = 0;
                pTcb->SrcFileName = NULL;
                pTcb->SrcLineNr = 0;
#ifdef _WIN32
                if (ReleaseMutex (pTcb->Mutex)) {
                    ;
                } else {
                    char *lpMsgBuf;
                    DWORD dw = GetLastError ();
                    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                   FORMAT_MESSAGE_FROM_SYSTEM |
                                   FORMAT_MESSAGE_IGNORE_INSERTS,
                                   NULL,
                                   dw,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   (LPTSTR) &lpMsgBuf,
                                   0, NULL );

                     ThrowError (1, "ReleaseMutex () of process \"%s\"  \"%s\"", pTcb->name, lpMsgBuf);
                     LocalFree (lpMsgBuf);
                     return -1;
                }
#else
                pthread_mutex_unlock(&(pTcb->Mutex));
                //return 0;
#endif
            }
        }
    }
    return 0;
}

int UnLockProcess (int par_Pid)
{
    int x;

    // Realtime processes cannot lock
    if (s_main_ini_val.ConnectToRemoteMaster) {
        if ((par_Pid & 0x10000000) == 0x10000000) return 0;
    }

    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {
                if (pTcb->LockedByThreadId != GetCurrentThreadId()) {
                    ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
                }
                pTcb->LockedByThreadId = 0;
                pTcb->SrcFileName = NULL;
                pTcb->SrcLineNr = 0;
#ifdef _WIN32
                if (ReleaseMutex (pTcb->Mutex)) {
                    if (pTcb->NumberOfProcessesInsideExecutable > 1) {
                        return UnLockAllOtherProcessesForExecutable(par_Pid, pTcb->InsideExecutableName);
                    } else {
                        return 0;
                    }
                } else {
                    char *lpMsgBuf;
                    DWORD dw = GetLastError ();
                    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                   FORMAT_MESSAGE_FROM_SYSTEM |
                                   FORMAT_MESSAGE_IGNORE_INSERTS,
                                   NULL,
                                   dw,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   (LPTSTR) &lpMsgBuf,
                                   0, NULL );

                     ThrowError (1, "ReleaseMutex () of process \"%s\"  \"%s\"", pTcb->name, lpMsgBuf);
                     LocalFree (lpMsgBuf);
                     return -1;
                }
#else
                pthread_mutex_unlock(&(pTcb->Mutex));
                if (pTcb->NumberOfProcessesInsideExecutable > 1) {
                    return UnLockAllOtherProcessesForExecutable(x, pTcb->InsideExecutableName);
                } else {
                    return 0;
                }
#endif
            }
        }
    }
    return -1;
}


int WaitUntilProcessIsNotLocked (TASK_CONTROL_BLOCK *pTcb, char *par_FileName, int par_LineNr)
{
#ifdef _WIN32
    DWORD WaitResult = WaitForSingleObject (pTcb->Mutex,    // handle to mutex
                                            INFINITE);  // no time-out interval
    if (WaitResult == WAIT_OBJECT_0) {
        if (pTcb->LockedByThreadId != 0) {
            ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
        }
        pTcb->LockedByThreadId = GetCurrentThreadId();
        pTcb->SrcFileName = par_FileName;
        pTcb->SrcLineNr = par_LineNr;

        return 0; // OK
    } else {
        char *lpMsgBuf;
        DWORD dw = GetLastError ();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "timeout during wait till the end of the cyclic function of process \"%s\" reached \"%s\"", pTcb->name, lpMsgBuf);
        LocalFree (lpMsgBuf);
    }
    return -1;
#else
    pthread_mutex_lock(&(pTcb->Mutex));
    pTcb->LockedByThreadId = GetCurrentThreadId();
    pTcb->SrcFileName = par_FileName;
    pTcb->SrcLineNr = par_LineNr;

    return 0; // OK
#endif
}

void EndOfProcessCycle (TASK_CONTROL_BLOCK *pTcb)
{
    if (pTcb->LockedByThreadId != GetCurrentThreadId()) {
        ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
    }
    pTcb->LockedByThreadId = 0;
    pTcb->SrcFileName = NULL;
    pTcb->SrcLineNr = 0;
#ifdef _WIN32
    if (ReleaseMutex (pTcb->Mutex)) {
        return;
    } else {
        char *lpMsgBuf;
        DWORD dw = GetLastError ();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );

         ThrowError (1, "ReleaseMutex () of process \"%s\"  \"%s\"", pTcb->name, lpMsgBuf);
         LocalFree (lpMsgBuf);
    }
    return;
#else
    pthread_mutex_unlock(&(pTcb->Mutex));
#endif
}


void InitWaitUntilProcessIsNotActive (TASK_CONTROL_BLOCK *pTcb)
{
#if 0
    if (FhLocking == NULL) {
        FhLocking = fopen ("c:\\tmp\\Locking.txt", "wt");
    }
#endif
    if (pTcb->Type != EXTERN_ASYNC) {
        ThrowError (1, "internal process \"%s\" should not be a target of wait until process not active", pTcb->name);
    }
    pTcb->WorkingFlag = 0;
    pTcb->LockedFlag = 0;
    pTcb->SchedWaitForUnlocking = 0;
    pTcb->WaitForEndOfCycleFlag = 0;

#ifdef _WIN32
    pTcb->Mutex = CreateMutex (NULL,              // default security attributes
                               FALSE,             // initially not owned
                               NULL);             // unnamed mutex
#else
    pthread_mutex_init(&(pTcb->Mutex), NULL);
#endif

}

void TerminatWaitUntilProcessIsNotActive (TASK_CONTROL_BLOCK *pTcb)
{
    if (pTcb->Type != EXTERN_ASYNC) {
        ThrowError (1, "internal process \"%s\" should not be a target of wait until process not active", pTcb->name);
    }
    if (pTcb->WorkingFlag || pTcb->LockedFlag || pTcb->SchedWaitForUnlocking || pTcb->WaitForEndOfCycleFlag) {
        ThrowError (1, "pTcb->WorkingFlag = %i, pTcb->LockedFlag = %i, pTcb->SchedWaitForUnlocking = %i, pTcb->WaitForEndOfCycleFlag = %i, ",
               pTcb->WorkingFlag, pTcb->LockedFlag, pTcb->SchedWaitForUnlocking, pTcb->WaitForEndOfCycleFlag);
    }
#ifdef _WIN32
    CloseHandle (pTcb->Mutex);
#else
    pthread_mutex_destroy(&(pTcb->Mutex));
#endif
}


char *GetNameOfAllSchedulers (void)
{
    int x;
    char *Ret;
    int LenOfRet = 0;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        LenOfRet += (int)strlen (SchedulerData[x].SchedulerName);
    }
    LenOfRet += SchedulerCount;
    Ret = my_malloc (LenOfRet);
    if (Ret != NULL) {
        Ret[0] = 0;
        for (x = 0; x < SchedulerCount; x++) {
            strcat (Ret, SchedulerData[x].SchedulerName);
            if (x < (SchedulerCount - 1)) {
                strcat (Ret, ";");
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    if (Ret == NULL) {
        ThrowError (1, "Out of memory");
    }
    return Ret;
}


int GetSchedulingInformation (int32_t *ret_SeparateCyclesForRefAndInitFunction,
                              uint64_t *ret_SchedulerCycleCounter,
                              int64_t *ret_SchedulerPeriod,
                              uint64_t *ret_MainSchedulerCycleCounter,
                              uint64_t *ret_ProcessCycleCounter,
                              int32_t *ret_ProcessPerid,
                              int32_t *ret_ProcessDelay)
{
    SCHEDULER_DATA *pSchedulerData = GetCurrentScheduler ();
    if (pSchedulerData != NULL) {        
        *ret_SeparateCyclesForRefAndInitFunction = s_main_ini_val.SeparateCyclesForRefAndInitFunction;
        *ret_SchedulerCycleCounter = pSchedulerData->Cycle;
        *ret_SchedulerPeriod = pSchedulerData->SchedPeriodInNanoSecond;
        *ret_MainSchedulerCycleCounter = GetCycleCounter64();
        if (pSchedulerData->CurrentTcb != NULL) {
            *ret_ProcessCycleCounter = pSchedulerData->CurrentTcb->call_count;
            *ret_ProcessPerid = pSchedulerData->CurrentTcb->time_steps;
            *ret_ProcessDelay = pSchedulerData->CurrentTcb->delay;
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

int SchedulerLoopOut (char *par_BarrierBefore, char *par_BarrierBehind, char *SnapShotDataIn, int SnapShotSizeIn, char *SnapShotDataOut)
{
    int SnapShotSize;
    SCHEDULER_DATA *pSchedulerData = GetCurrentScheduler ();
    if (pSchedulerData != NULL) {
        WalkThroughBarrierBeforeLoopOut (pSchedulerData->CurrentTcb, par_BarrierBefore, 0, 0);
        CopyFromPipeToBlackboard (pSchedulerData->CurrentTcb, SnapShotDataIn, SnapShotSizeIn);
        WalkThroughBarrierBeforeLoopOut (pSchedulerData->CurrentTcb, par_BarrierBefore, 1, 0);

        WalkThroughBarrierBehindLoopOut (pSchedulerData->CurrentTcb, par_BarrierBehind, 0, 0);
        SnapShotSize = CopyFromBlackbardToPipe (pSchedulerData->CurrentTcb, SnapShotDataOut);
        WalkThroughBarrierBehindLoopOut (pSchedulerData->CurrentTcb, par_BarrierBehind, 1, 0);
    }
    return SnapShotSize;
}


int SchedulerLogout (int par_ImmediatelyFlag)
{
    SCHEDULER_DATA *pSchedulerData = GetCurrentScheduler ();
    if (pSchedulerData != NULL) {
        if (pSchedulerData->CurrentTcb != NULL) {
            if (pSchedulerData->CurrentTcb->Type == EXTERN_ASYNC) {
                if (!par_ImmediatelyFlag) {
                    pSchedulerData->CurrentTcb->state = EX_PROCESS_TERMINATE;
                }
                return 0;
            }
        }
    }
    return -1;
}

int SetSchedulerBreakPoint (const char *BreakPoint)
{
    // Breakpoint only possible inside Scheduler 0
    SCHEDULER_DATA *pSchedulerData = &(SchedulerData[0]);
    if (BreakPoint == NULL) {
        pSchedulerData->BreakPointActive = 0;
    } else {
        int Len = (int)strlen(BreakPoint) + 1;
        if (pSchedulerData->BreakPointString == NULL) InitializeCriticalSection(&(pSchedulerData->BreakPointCriticalSection));
        EnterCriticalSection(&(pSchedulerData->BreakPointCriticalSection));
        if (Len > pSchedulerData->SizeofBreakPointBuffer) {
            pSchedulerData->SizeofBreakPointBuffer = Len;
            pSchedulerData->BreakPointString = my_realloc(pSchedulerData->BreakPointString, pSchedulerData->SizeofBreakPointBuffer);
            if (pSchedulerData->BreakPointString == NULL) {
                pSchedulerData->SizeofBreakPointBuffer = 0;
                pSchedulerData->BreakPointActive = 0;
                LeaveCriticalSection(&(pSchedulerData->BreakPointCriticalSection));
                return -1;
            }
        }
        MEMCPY (pSchedulerData->BreakPointString, BreakPoint, (size_t)Len);
        pSchedulerData->BreakPointActive = 1;
        LeaveCriticalSection(&(pSchedulerData->BreakPointCriticalSection));
    }
    return 0;
}

int CheckBreakPoint (void)
{
    double Value;
    // Breakpoint only possible inside Scheduler 0
    SCHEDULER_DATA *pSchedulerData = &(SchedulerData[0]);

    EnterCriticalSection(&(pSchedulerData->BreakPointCriticalSection));
    if (direct_solve_equation_err_sate(pSchedulerData->BreakPointString, &Value) != 0) {
        pSchedulerData->BreakPointActive = 0;
        LeaveCriticalSection(&(pSchedulerData->BreakPointCriticalSection));
        DeactivateBreakpointFromOtherThread();
        return 0;
    }
    LeaveCriticalSection(&(pSchedulerData->BreakPointCriticalSection));

    if (pSchedulerData->BreakPointHitFlag) {
        pSchedulerData->BreakPointHitFlag = 0;
    } else {
        if ((Value < 0) || (Value >= 1)) {
            pSchedulerData->BreakPointHitFlag = 1;
            return 1;
        }
    }
    return 0;
}

unsigned int GetProcessRangeControlFlags(int par_Pid)
{
    int x;

    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {
                volatile unsigned int Ret = pTcb->RangeControlFlags;
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return Ret;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return 0;
}

void RemoveAllPendingStartStopSchedulerRequestsOfThisThread(int par_DisableCounter)
{
    RemoveAllPendingRequestsOfThisThread(&(SchedulerData[0].StopReq), par_DisableCounter);
}

void GetStopRequestCounters (int *ret_System, int *ret_User, int *ret_Rpc)
{
    StopRequestGetCounters (&(SchedulerData[0].StopReq), ret_System, ret_User, ret_Rpc);
}

int DisconnectA2LFromProcess(int par_Pid)
{
    int x;
    EnterCriticalSection (&PipeSchedCriticalSection);
    for (x = 0; x < SchedulerCount; x++) {
        TASK_CONTROL_BLOCK *pTcb;
        // Walk through all processes of this scheduler
        for (pTcb = SchedulerData[x].FirstTcb; pTcb != NULL; pTcb = pTcb->next) {
            if (pTcb->pid == par_Pid) {
                pTcb->A2LLinkNr = 0;
                LeaveCriticalSection (&PipeSchedCriticalSection);
                return 0;
            }
        }
    }
    LeaveCriticalSection (&PipeSchedCriticalSection);
    return -1;
}

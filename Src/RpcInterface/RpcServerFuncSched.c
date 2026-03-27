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


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <memory.h>
#include <fcntl.h>

#include "Config.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Files.h"
#include "EquationParser.h"
#include "Wildcards.h"
#include "ScriptMessageFile.h"
#include "Scheduler.h"
#include "ProcessEquations.h"
#include "MainValues.h"
#include "RpcControlProcess.h"

#include "RpcSocketServer.h"
#include "RpcFuncSched.h"

#define UNUSED(x) (void)(x)

static void __SCStopSchedulerSchedDisableCallBack (void *Parameter)
{
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Parameter;
    Connection->SchedulerDisableCounter++;
    RemoteProcedureWakeWaitForConnection(Connection);
}

static int RPCFunc_StopScheduler(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_DataIn);
    //RPC_API_STOP_SCHEDULER_MESSAGE *In = (RPC_API_STOP_SCHEDULER_MESSAGE*)par_DataIn;
    RPC_API_STOP_SCHEDULER_MESSAGE_ACK *Out = (RPC_API_STOP_SCHEDULER_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_SCHEDULER_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_SCHEDULER_MESSAGE_ACK);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        Out->Header.ReturnValue = 0;  // will be ignored (no error code)
    } else {
        RemoteProcedureMarkedForWaitForConnection(par_Connection);
        Out->Header.ReturnValue = disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_RPC, __SCStopSchedulerSchedDisableCallBack, par_Connection);
        RemoteProcedureWaitForConnection(par_Connection);
    }
    return Out->Header.StructSize;
}

static void __SCContinueSchedulerSchedEnableCallBack (void *Parameter)
{
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Parameter;
    Connection->SchedulerDisableCounter--;
    if ((Connection->DebugFlags & DEBUG_PRINT_COMMAND_NAME) == DEBUG_PRINT_COMMAND_NAME) {
        fprintf(Connection->DebugFile, "%i %" PRIu64 ": __SCContinueSchedulerSchedEnableCallBack\n", Connection->ConnectionNr, GetCycleCounter64());
        if ((Connection->DebugFlags & DEBUG_PRINT_COMMAND_FFLUSH) == DEBUG_PRINT_COMMAND_FFLUSH) {
            fflush(Connection->DebugFile);
        }
    }
    RemoteProcedureWakeWaitForConnection(Connection);
}

static int RPCFuncContinueScheduler(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    //RPC_API_CONTINUE_SCHEDULER_MESSAGE *In = (RPC_API_CONTINUE_SCHEDULER_MESSAGE*)par_DataIn;
    RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK *Out = (RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK);

    Out->Header.ReturnValue = 0;
    if (s_main_ini_val.ConnectToRemoteMaster) {
        ; // will be ignored
    } else {
        //enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
        RemoteProcedureMarkedForWaitForConnection(par_Connection);
        enable_scheduler_with_callback (SCHEDULER_CONTROLED_BY_RPC, __SCContinueSchedulerSchedEnableCallBack, par_Connection);
        RemoteProcedureWaitForConnection(par_Connection);
    }
    return Out->Header.StructSize;
}

static int RPCFuncIsSchedulerRunning(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    //RPC_API_IS_SCHEDULER_RUNNING_MESSAGE *In = (RPC_API_IS_SCHEDULER_RUNNING_MESSAGE*)par_DataIn;
    RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK *Out = (RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK);

    // return 1 if scheduler is not stopped from this connection and 0 if stopped from this connection
    Out->Header.ReturnValue = (par_Connection->SchedulerDisableCounter == 0); //get_scheduler_state();

    return Out->Header.StructSize;
}

static int RPCFunc_StartProcess(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_START_PROCESS_MESSAGE *In = (RPC_API_START_PROCESS_MESSAGE*)par_DataIn;
    RPC_API_START_PROCESS_MESSAGE_ACK *Out = (RPC_API_START_PROCESS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_PROCESS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_PROCESS_MESSAGE_ACK);

    Out->Header.ReturnValue = start_process_env_var ((char*)In + In->OffsetProcessName);

    return Out->Header.StructSize;
}

static int RPCFunc_StartProcessAndLoadSvl(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *ProcessName;
    char *SvlName;
    RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE *In = (RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE*)par_DataIn;
    RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK *Out = (RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK);

    ProcessName = (char*)In + In->OffsetProcessName;
    SvlName = (char*)In + In->OffsetSvlName;
    if (strlen (SvlName)) {
        const char *p = GetProcessNameFromPath (ProcessName);
        if (p != ProcessName) {
            char ShortProcessName[MAX_PATH];
            strncpy (ShortProcessName, p, sizeof (ShortProcessName));
            ShortProcessName[sizeof (ShortProcessName)-1] = 0;
#ifdef _WIN32
            strupr ((char*)ShortProcessName);
#endif
            SetSVLFileLoadedBeforeInitProcessFileName (ShortProcessName, (char*)SvlName, 0);
        } else SetSVLFileLoadedBeforeInitProcessFileName ("", "", 0);
    } else SetSVLFileLoadedBeforeInitProcessFileName ("", "", 0);

    Out->Header.ReturnValue = start_process_env_var ((char*)ProcessName);

    return Out->Header.StructSize;
}


static int RPCFunc_StartProcessEx(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *ProcessName;
    char *SvlName;
    char *BBPrefix;
    char *RangeErrorCounter;
    char *RangeControl;

    RPC_API_START_PROCESS_EX_MESSAGE *In = (RPC_API_START_PROCESS_EX_MESSAGE*)par_DataIn;
    RPC_API_START_PROCESS_EX_MESSAGE_ACK *Out = (RPC_API_START_PROCESS_EX_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_PROCESS_EX_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_PROCESS_EX_MESSAGE_ACK);

    ProcessName = (char*)In + In->OffsetProcessName;
    SvlName = (char*)In + In->OffsetSvlName;
    BBPrefix = (char*)In + In->OffsetBBPrefix;
    RangeErrorCounter = (char*)In + In->OffsetRangeErrorCounter;
    RangeControl = (char*)In + In->OffsetRangeControl;
    Out->Header.ReturnValue = start_process_ex (ProcessName,
                                                   In->Prio,
                                                   In->Cycle,
                                                   In->Delay,
                                                   In->Timeout,
                                                   SvlName,
                                                   "", 0,
                                                   BBPrefix,
                                                   In->UseRangeControl,  // If 0 all following parameter will be ignored
                                                   In->RangeControlBeforeActiveFlags,
                                                   In->RangeControlBehindActiveFlags,
                                                   In->RangeControlStopSchedFlag,
                                                   In->RangeControlOutput,
                                                   In->RangeErrorCounterFlag,
                                                   RangeErrorCounter,
                                                   In->RangeControlVarFlag,
                                                   RangeControl,
                                                   In->RangeControlPhysFlag,
                                                   In->RangeControlLimitValues, NULL);

    return Out->Header.StructSize;
}

static int RPCFunc_StopProcess(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_STOP_PROCESS_MESSAGE *In = (RPC_API_STOP_PROCESS_MESSAGE*)par_DataIn;
    RPC_API_STOP_PROCESS_MESSAGE_ACK *Out = (RPC_API_STOP_PROCESS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_PROCESS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_PROCESS_MESSAGE_ACK);
    Out->Header.ReturnValue = terminate_process (get_pid_by_name((char*)In + In->OffsetProcessName));

    return Out->Header.StructSize;
}


static int RPCFunc_GetNextProcess(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_GET_NEXT_PROCESS_MESSAGE *In = (RPC_API_GET_NEXT_PROCESS_MESSAGE*)par_DataIn;
    RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK *Out = (RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK);
    Out->OffsetReturnValue = sizeof(RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK) - 1;

    if (strlen ((char*)In + In->OffsetFilter) >= MAX_PATH) {
        Out->Header.ReturnValue = -1;
        return sizeof (RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK);
    } else  {
        int Count = 0;
        char *Name;
        if (In->Flag) par_Connection->GetNextProcessCounter = 0;
        char *Filter = (char*)In + In->OffsetFilter;
        READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2);
        while ((Name = read_next_process_name(Buffer)) != NULL) {
            if (Count >= par_Connection->GetNextProcessCounter) {
                par_Connection->GetNextProcessCounter++;
                if (!Compare2StringsWithWildcards(Name, Filter)) {
                    int Len = (int)strlen(Name) + 1;
                    Out->Header.ReturnValue = 1;
                    StringCopyMaxCharTruncate ((char*)Out + Out->OffsetReturnValue, Name, Len);
                    Out->Header.StructSize += Len;
                    return Out->Header.StructSize;
                }
            }
            Count++;
        }
        close_read_next_process_name(Buffer);
        Out->Header.ReturnValue = 0;
        Out->Data[0] = 0;
        return Out->Header.StructSize;
    }
}

static int RPCFunc_GetProcessState(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_PROCESS_STATE_MESSAGE *In = (RPC_API_GET_PROCESS_STATE_MESSAGE*)par_DataIn;
    RPC_API_GET_PROCESS_STATE_MESSAGE_ACK *Out = (RPC_API_GET_PROCESS_STATE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_PROCESS_STATE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_PROCESS_STATE_MESSAGE_ACK);

    Out->Header.ReturnValue = get_pid_by_name ((char*)In + In->OffsetProcessName);
    if (Out->Header.ReturnValue > 0) {
        Out->Header.ReturnValue = get_process_state (Out->Header.ReturnValue);
    }

    return Out->Header.StructSize;
}

static void __SCDoNextCyclesDisableCallBack (void *Parameter)
{
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Parameter;
    Connection->SchedulerDisableCounter++;
    Connection->DoNextCycleFlag = 0;
}


static int RPCFunc_DoNextCycles(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_DO_NEXT_CYCLES_MESSAGE *In = (RPC_API_DO_NEXT_CYCLES_MESSAGE*)par_DataIn;
    RPC_API_DO_NEXT_CYCLES_MESSAGE_ACK *Out = (RPC_API_DO_NEXT_CYCLES_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_DO_NEXT_CYCLES_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DO_NEXT_CYCLES_MESSAGE_ACK);

    Out->Header.ReturnValue = 0;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        ;  // will be ignored
    } else {
        if ((In->Cycles > 0) &&
            (par_Connection->SchedulerDisableCounter == 1)) {
            par_Connection->SchedulerDisableCounter--;
            par_Connection->DoNextCycleFlag = 1;
            switch(make_n_next_cycles (SCHEDULER_CONTROLED_BY_RPC, In->Cycles, NULL, __SCDoNextCyclesDisableCallBack, par_Connection)) {
            case 0: // stop scheduler request are added during the scheduler is running.
            case 1: // stop scheduler request are added but the scheduler was already stopped.
                break;
            default: // all others
                __SCDoNextCyclesDisableCallBack(par_Connection);
                break;
            }
        } else {
            Out->Header.ReturnValue = -1;
        }
    }
    return Out->Header.StructSize;
}


static void __SCDoNextCyclesAndWaitSchedDisableCallBack (void *Parameter)
{
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Parameter;
    Connection->SchedulerDisableCounter++;
    Connection->DoNextCycleFlag = 0;
    if ((Connection->DebugFlags & DEBUG_PRINT_COMMAND_NAME) == DEBUG_PRINT_COMMAND_NAME) {
        fprintf(Connection->DebugFile, "%i %" PRIu64 ": __SCDoNextCyclesAndWaitSchedDisableCallBack\n", Connection->ConnectionNr, GetCycleCounter64());
        if ((Connection->DebugFlags & DEBUG_PRINT_COMMAND_FFLUSH) == DEBUG_PRINT_COMMAND_FFLUSH) {
            fflush(Connection->DebugFile);
        }
    }
    RemoteProcedureWakeWaitForConnection(Connection);
}

static int RPCFunc_DoNextCyclesAndWait(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE *In = (RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE*)par_DataIn;
    RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK *Out = (RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK);

    if (s_main_ini_val.ConnectToRemoteMaster) {
        ;  // will be ignored
    } else {
        if ((In->Cycles > 0) &&
            (par_Connection->SchedulerDisableCounter == 1)) {
            par_Connection->SchedulerDisableCounter--;
            par_Connection->DoNextCycleFlag = 1;
            RemoteProcedureMarkedForWaitForConnection(par_Connection);
            switch(make_n_next_cycles(SCHEDULER_CONTROLED_BY_RPC, In->Cycles, NULL, __SCDoNextCyclesAndWaitSchedDisableCallBack, par_Connection)) {
            case 0: // stop scheduler request are added during the scheduler is running.
            case 1: // stop scheduler request are added but the scheduler was already stopped.
                RemoteProcedureWaitForConnection(par_Connection);
                break;
            default: // all others
                __SCDoNextCyclesAndWaitSchedDisableCallBack(par_Connection);
                break;
            }
        } else {
            Out->Header.ReturnValue = -1;
        }
    }
    Out->Header.ReturnValue = 0;

    return Out->Header.StructSize;
}

static int RPCFunc_DoNextConditionsCyclesAndWait(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE *In = (RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE*)par_DataIn;
    RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE_ACK *Out = (RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE_ACK);

    if (s_main_ini_val.ConnectToRemoteMaster) {
        ;  // will be ignored
    } else {
        if ((In->Cycles > 0) &&
            (par_Connection->SchedulerDisableCounter == 1)) {
            char *Equation = NULL;
            if (In->OffsetConditions >= 0) {
                Equation = (char*)In + In->OffsetConditions;
            }
            par_Connection->SchedulerDisableCounter--;
            par_Connection->DoNextCycleFlag = 1;
            RemoteProcedureMarkedForWaitForConnection(par_Connection);
            switch(make_n_next_cycles(SCHEDULER_CONTROLED_BY_RPC, In->Cycles, Equation, __SCDoNextCyclesAndWaitSchedDisableCallBack, par_Connection)) {
            case 0: // stop scheduler request are added during the scheduler is running.
            case 1: // stop scheduler request are added but the scheduler was already stopped.
                RemoteProcedureWaitForConnection(par_Connection);
                break;
            default: // all others
                __SCDoNextCyclesAndWaitSchedDisableCallBack(par_Connection);
                break;
            }
        } else {
            Out->Header.ReturnValue = -1;
        }
    }
    Out->Header.ReturnValue = 0;

    return Out->Header.StructSize;
}

static int RPCFunc_AddBeforeProcessEquationFromFile(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *ErrString = NULL;
    char *ProcName;
    char *EquFile;

    RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE *In = (RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE*)par_DataIn;
    RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK *Out = (RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK);

    ProcName = (char*)In + In->OffsetProcessName;
    EquFile = (char*)In + In->OffsetEquationFile;
    Out->Header.ReturnValue = AddBeforeProcessEquationFromFile (In->Nr, ProcName, EquFile, 1, &ErrString);
    switch (Out->Header.ReturnValue) {
    case -1:
        ThrowError (ERROR_NO_STOP, "process name: %s not found", ProcName);
        break;
    case -2:
        ThrowError (ERROR_NO_STOP, "file: %s not found", EquFile);
        break;
    case -3:
        ThrowError (ERROR_NO_STOP, "inside equation file \"%s\" are errors: %s", EquFile, ErrString);
        if (ErrString != NULL) FreeErrStringBuffer (&ErrString);
        break;
    case -4:
        ThrowError (ERROR_NO_STOP, "process name: %s not found", ProcName);
        break;
    default:
        break;
    }

    return Out->Header.StructSize;
}

static int RPCFunc_AddBehindProcessEquationFromFile(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *ErrString = NULL;
    char *ProcName;
    char *EquFile;

    RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE *In = (RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE*)par_DataIn;
    RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK *Out = (RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK);

    ProcName = (char*)In + In->OffsetProcessName;
    EquFile = (char*)In + In->OffsetEquationFile;
    Out->Header.ReturnValue = AddBehindProcessEquationFromFile (In->Nr, ProcName, EquFile, 1, &ErrString);
    switch (Out->Header.ReturnValue) {
    case -1:
        ThrowError (ERROR_NO_STOP, "process name: %s not found", ProcName);
        break;
    case -2:
        ThrowError (ERROR_NO_STOP, "file: %s not found", EquFile);
        break;
    case -3:
        ThrowError (ERROR_NO_STOP, "inside equation file \"%s\" are errors: %s", EquFile, ErrString);
        if (ErrString != NULL) FreeErrStringBuffer (&ErrString);
        break;
    case -4:
        ThrowError (ERROR_NO_STOP, "process name: %s not found", ProcName);
        break;
    default:
        break;
    }

    return Out->Header.StructSize;
}

static int RPCFunc_DelBeforeProcessEquations(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *ProcName;

    RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE *In = (RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE*)par_DataIn;
    RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK *Out = (RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK);

    ProcName = (char*)In + In->OffsetProcessName;

    Out->Header.ReturnValue = DelBeforeProcessEquations (In->Nr, ProcName);

    return Out->Header.StructSize;
}

static int RPCFunc_DelBehindProcessEquations(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *ProcName;

    RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE *In = (RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE*)par_DataIn;
    RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK *Out = (RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK);

    ProcName = (char*)In + In->OffsetProcessName;

    Out->Header.ReturnValue = DelBehindProcessEquations (In->Nr, ProcName);

    return Out->Header.StructSize;
}

static void __SCWaitUntilCallBackBehind (int TimeoutFlag, uint32_t Remainder, void *Parameter)
{
    UNUSED(TimeoutFlag);
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Parameter;
    Connection->WaitUntilRemainder = Remainder;
    RemoteProcedureWakeWaitForConnection(Connection);
}

static int RPCFunc_WaitUntil(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    char *Equation;

    RPC_API_WAIT_UNTIL_MESSAGE *In = (RPC_API_WAIT_UNTIL_MESSAGE*)par_DataIn;
    RPC_API_WAIT_UNTIL_MESSAGE_ACK *Out = (RPC_API_WAIT_UNTIL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_WAIT_UNTIL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_WAIT_UNTIL_MESSAGE_ACK);

    Equation = (char*)In + In->OffsetEquation;

    if (In->Cycles > 0) {
        RemoteProcedureMarkedForWaitForConnection(par_Connection);
        SetDelayFunc (par_Connection, (uint32_t)In->Cycles, NULL, NULL, __SCWaitUntilCallBackBehind, par_Connection, 0, Equation);
        RemoteProcedureWaitForConnection(par_Connection);

        Out->Header.ReturnValue = (int)par_Connection->WaitUntilRemainder;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartProcessEx2(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *ProcessName;
    char *SvlName;
    char *A2LName;
    char *BBPrefix;
    char *RangeErrorCounter;
    char *RangeControl;

    RPC_API_START_PROCESS_EX2_MESSAGE *In = (RPC_API_START_PROCESS_EX2_MESSAGE*)par_DataIn;
    RPC_API_START_PROCESS_EX2_MESSAGE_ACK *Out = (RPC_API_START_PROCESS_EX2_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_PROCESS_EX2_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_PROCESS_EX2_MESSAGE_ACK);

    ProcessName = (char*)In + In->OffsetProcessName;
    SvlName = (char*)In + In->OffsetSvlName;
    A2LName = (char*)In + In->OffsetA2LName;
    BBPrefix = (char*)In + In->OffsetBBPrefix;
    RangeErrorCounter = (char*)In + In->OffsetRangeErrorCounter;
    RangeControl = (char*)In + In->OffsetRangeControl;
    Out->Header.ReturnValue = start_process_ex (ProcessName,
                                                   In->Prio,
                                                   In->Cycle,
                                                   In->Delay,
                                                   In->Timeout,
                                                   SvlName,
                                                   A2LName, In->A2LFlags,
                                                   BBPrefix,
                                                   In->UseRangeControl,  // If 0 all following parameter will be ignored
                                                   In->RangeControlBeforeActiveFlags,
                                                   In->RangeControlBehindActiveFlags,
                                                   In->RangeControlStopSchedFlag,
                                                   In->RangeControlOutput,
                                                   In->RangeErrorCounterFlag,
                                                   RangeErrorCounter,
                                                   In->RangeControlVarFlag,
                                                   RangeControl,
                                                   In->RangeControlPhysFlag,
                                                   In->RangeControlLimitValues, NULL);

    return Out->Header.StructSize;
}


int AddSchedFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_SCHEDULER_CMD, 0, RPCFunc_StopScheduler, sizeof(RPC_API_STOP_SCHEDULER_MESSAGE), sizeof(RPC_API_STOP_SCHEDULER_MESSAGE), STRINGIZE(RPC_API_STOP_SCHEDULER_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_SCHEDULER_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CONTINUE_SCHEDULER_CMD, 0, RPCFuncContinueScheduler, sizeof(RPC_API_CONTINUE_SCHEDULER_MESSAGE), sizeof(RPC_API_CONTINUE_SCHEDULER_MESSAGE), STRINGIZE(RPC_API_CONTINUE_SCHEDULER_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_IS_SCHEDULER_RUNNING_CMD, 0, RPCFuncIsSchedulerRunning, sizeof(RPC_API_IS_SCHEDULER_RUNNING_MESSAGE), sizeof(RPC_API_IS_SCHEDULER_RUNNING_MESSAGE), STRINGIZE(RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_MEMBERS), STRINGIZE(RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_PROCESS_CMD, 1, RPCFunc_StartProcess, sizeof(RPC_API_START_PROCESS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_PROCESS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_PROCESS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_PROCESS_AND_LOAD_SVL_CMD, 1, RPCFunc_StartProcessAndLoadSvl, sizeof(RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_PROCESS_EX_CMD, 1, RPCFunc_StartProcessEx, sizeof(RPC_API_START_PROCESS_EX_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_PROCESS_EX_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_PROCESS_EX_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_PROCESS_CMD, 1, RPCFunc_StopProcess, sizeof(RPC_API_STOP_PROCESS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_STOP_PROCESS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_PROCESS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_NEXT_PROCESS_CMD, 1, RPCFunc_GetNextProcess, sizeof(RPC_API_GET_NEXT_PROCESS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_NEXT_PROCESS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_PROCESS_STATE_CMD, 1, RPCFunc_GetProcessState, sizeof(RPC_API_GET_PROCESS_STATE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_PROCESS_STATE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_PROCESS_STATE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DO_NEXT_CYCLES_CMD, 0, RPCFunc_DoNextCycles, sizeof(RPC_API_STOP_PROCESS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_STOP_PROCESS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_PROCESS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DO_NEXT_CYCLES_AND_WAIT_CMD, 0, RPCFunc_DoNextCyclesAndWait, sizeof(RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_CMD, 1, RPCFunc_AddBeforeProcessEquationFromFile, sizeof(RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_CMD, 1, RPCFunc_AddBehindProcessEquationFromFile, sizeof(RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_CMD, 1, RPCFunc_DelBeforeProcessEquations, sizeof(RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_CMD, 1, RPCFunc_DelBehindProcessEquations, sizeof(RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_WAIT_UNTIL_CMD, 0, RPCFunc_WaitUntil, sizeof(RPC_API_WAIT_UNTIL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_WAIT_UNTIL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_WAIT_UNTIL_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_PROCESS_EX2_CMD, 1, RPCFunc_StartProcessEx2, sizeof(RPC_API_START_PROCESS_EX2_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_PROCESS_EX2_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_PROCESS_EX2_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_CMD, 0, RPCFunc_DoNextConditionsCyclesAndWait, sizeof(RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE_ACK_MEMBERS));

    return 0;
}

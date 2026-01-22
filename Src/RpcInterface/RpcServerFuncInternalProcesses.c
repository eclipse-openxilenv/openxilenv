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
#include <memory.h>
#include <fcntl.h>

#include "Config.h"

#include "MemZeroAndCopy.h"
#include "EnvironmentVariables.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Message.h"
#include "InterfaceToScript.h"
#include "MainValues.h"

#include "RpcControlProcess.h"
#include "RpcSocketServer.h"
#include "RpcFuncInternalProcesses.h"

#define UNUSED(x) (void)(x)

static int RPCFunc_StartScript(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    int Vid;
    RPC_API_START_SCRIPT_MESSAGE *In = (RPC_API_START_SCRIPT_MESSAGE*)par_DataIn;
    RPC_API_START_SCRIPT_MESSAGE_ACK *Out = (RPC_API_START_SCRIPT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_SCRIPT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_SCRIPT_MESSAGE_ACK);


    if (get_pid_by_name ("Script") <= 0) start_process ("Script");
    Vid = add_bbvari ("Script", BB_UBYTE, NULL);
    if (read_bbvari_ubyte (Vid) != RUNNING) {
        SearchAndReplaceEnvironmentStrings ((char*)In + In->OffsetScriptFile, script_filename, MAX_PATH);
        script_status_flag = START;
        write_bbvari_ubyte (Vid, START);  // this will be overwriten by the script process
        remove_bbvari (Vid);
        Out->Header.ReturnValue = 0;
    } else {
        remove_bbvari (Vid);
        Out->Header.ReturnValue = -3;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StopScript(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    RPC_API_STOP_SCRIPT_MESSAGE_ACK *Out = (RPC_API_STOP_SCRIPT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_SCRIPT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_SCRIPT_MESSAGE_ACK);

    script_status_flag = 0;

    return Out->Header.StructSize;
}


static void __SCStartRecorderCallBackBefore (void *Param)
{
    int pid;
    RPC_API_START_RECORDER_MESSAGE *In = (RPC_API_START_RECORDER_MESSAGE*)Param;
    char *CfgFile  = (char*)In + In->OffsetCfgFile;

    pid = get_pid_by_name (PN_TRACE_RECORDER);
    if (pid > 0) {
        write_message (pid, REC_CONFIG_FILENAME_MESSAGE, (int)strlen (CfgFile) + 1, CfgFile);
    }
}

static void __SCStartRecorderCallBackBehind (int TimeoutFlag, uint32_t Remainder, void *Parameter)
{
    UNUSED(Remainder);
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Parameter;
    Connection->ReturnValue = (TimeoutFlag) ? -1 : 0;
    RemoteProcedureWakeWaitForConnection(Connection);
}


static int RPCFunc_StartRecorder(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_START_RECORDER_MESSAGE *In = (RPC_API_START_RECORDER_MESSAGE*)par_DataIn;
    RPC_API_START_RECORDER_MESSAGE_ACK *Out = (RPC_API_START_RECORDER_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_RECORDER_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_RECORDER_MESSAGE_ACK);

    if (s_main_ini_val.DontWaitForResponseOfStartRecorderRPC) {
        // If DONT_WAIT_FOR_RESPONSE_OF_START_RECORDER_RPC is set start recorder direct and don't wait for it
        __SCStartRecorderCallBackBefore (In);
        Out->Header.ReturnValue = 0;
    } else {
        RemoteProcedureMarkedForWaitForConnection(par_Connection);
        SetDelayFunc (par_Connection, 1000, __SCStartRecorderCallBackBefore, In, __SCStartRecorderCallBackBehind, par_Connection, HDREC_ACK, NULL);
        RemoteProcedureWaitForConnection(par_Connection);
        Out->Header.ReturnValue = par_Connection->ReturnValue;
    }

    return Out->Header.StructSize;
}

static int RPCFunc_RecorderAddComment(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *Comment;
    RPC_API_RECORDER_ADD_COMMENT_MESSAGE *In = (RPC_API_RECORDER_ADD_COMMENT_MESSAGE*)par_DataIn;
    RPC_API_RECORDER_ADD_COMMENT_MESSAGE_ACK *Out = (RPC_API_RECORDER_ADD_COMMENT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_RECORDER_ADD_COMMENT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_RECORDER_ADD_COMMENT_MESSAGE_ACK);

    Comment = (char*)In + In->OffsetComment;
    if (write_message(get_pid_by_name(TRACE_RECORDER), HDREC_COMMENT_MESSAGE, strlen(Comment) + 1, Comment)) {
        Out->Header.ReturnValue = -1;
    } else {
        Out->Header.ReturnValue = 0;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StopRecorder(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    int Pid;
    RPC_API_STOP_RECORDER_MESSAGE_ACK *Out = (RPC_API_STOP_RECORDER_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_RECORDER_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_RECORDER_MESSAGE_ACK);

    Pid = get_pid_by_name (PN_TRACE_RECORDER);
    if (Pid <= 0) Out->Header.ReturnValue = -1;
    write_message_as (Pid, STOP_REC_MESSAGE, 0, NULL, get_pid_by_name ("RPC_Control"));

    return Out->Header.StructSize;
}

static int RPCFunc_StartPlayer(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    int Pid;
    char *CfgFile;
    RPC_API_START_PLAYER_MESSAGE *In = (RPC_API_START_PLAYER_MESSAGE*)par_DataIn;
    RPC_API_START_PLAYER_MESSAGE_ACK *Out = (RPC_API_START_PLAYER_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_PLAYER_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_PLAYER_MESSAGE_ACK);

    Pid = get_pid_by_name(PN_STIMULI_PLAYER);
    if (Pid <= 0) Out->Header.ReturnValue = -1;
    CfgFile  = (char*)In + In->OffsetCfgFile;
    write_message_as (Pid, HDPLAY_CONFIG_FILENAME_MESSAGE, (int)strlen(CfgFile) + 1, CfgFile, get_pid_by_name ("RPC_Control"));

    return Out->Header.StructSize;
}

static int RPCFunc_StopPlayer(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    int Pid;
    RPC_API_STOP_PLAYER_MESSAGE_ACK *Out = (RPC_API_STOP_PLAYER_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_PLAYER_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_PLAYER_MESSAGE_ACK);

    Pid = get_pid_by_name (PN_STIMULI_PLAYER);
    if (Pid <= 0) Out->Header.ReturnValue = -1;
    write_message_as (Pid, HDPLAY_STOP_MESSAGE, 0, NULL, get_pid_by_name ("RPC_Control"));

    return Out->Header.StructSize;
}

static int RPCFunc_StartEquations(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    int Pid;
    char *CfgFile;
    RPC_API_START_EQUATIONS_MESSAGE *In = (RPC_API_START_EQUATIONS_MESSAGE*)par_DataIn;
    RPC_API_START_EQUATIONS_MESSAGE_ACK *Out = (RPC_API_START_EQUATIONS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_EQUATIONS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_EQUATIONS_MESSAGE_ACK);

    Pid = get_pid_by_name (PN_EQUATION_COMPILER);
    if (Pid <= 0) Out->Header.ReturnValue = -1;
    CfgFile  = (char*)In + In->OffsetCfgFile;
    write_message_as (Pid, TRIGGER_FILENAME_MESSAGE, (int)strlen(CfgFile) + 1, CfgFile, get_pid_by_name ("RPC_Control"));

    return Out->Header.StructSize;
}

static int RPCFunc_StopEquations(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    int Pid;
    RPC_API_STOP_EQUATIONS_MESSAGE_ACK *Out = (RPC_API_STOP_EQUATIONS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_EQUATIONS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_EQUATIONS_MESSAGE_ACK);

    Pid =  get_pid_by_name (PN_EQUATION_COMPILER);
    if (Pid <= 0) Out->Header.ReturnValue = -1;
     write_message_as (Pid, TRIGGER_STOP_MESSAGE, 0, NULL, get_pid_by_name ("RPC_Control"));

    return Out->Header.StructSize;
}

static int RPCFunc_StartGenerator(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    int Pid;
    char *CfgFile;
    RPC_API_START_GENERATOR_MESSAGE *In = (RPC_API_START_GENERATOR_MESSAGE*)par_DataIn;
    RPC_API_START_GENERATOR_MESSAGE_ACK *Out = (RPC_API_START_GENERATOR_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_GENERATOR_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_GENERATOR_MESSAGE_ACK);

    Pid = get_pid_by_name (PN_SIGNAL_GENERATOR_COMPILER);
    CfgFile  = (char*)In + In->OffsetCfgFile;
    write_message_as (Pid, RAMPEN_FILENAME, (int)strlen(CfgFile) + 1, CfgFile, get_pid_by_name ("RPC_Control"));
    write_message_as (Pid, RAMPEN_START, 0, NULL, get_pid_by_name ("RPC_Control"));

    return Out->Header.StructSize;
}

static int RPCFunc_StopGenerator(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    int Pid;
    RPC_API_STOP_GENERATOR_MESSAGE_ACK *Out = (RPC_API_STOP_GENERATOR_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_GENERATOR_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_GENERATOR_MESSAGE_ACK);

    Pid = get_pid_by_name (PN_SIGNAL_GENERATOR_COMPILER);
    if (Pid <= 0) Out->Header.ReturnValue = -1;
    write_message_as (Pid, RAMPEN_STOP, 0, NULL, get_pid_by_name ("RPC_Control"));

    return Out->Header.StructSize;
}

int AddInternProcessFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_SCRIPT_CMD, 0, RPCFunc_StartScript, sizeof(RPC_API_START_SCRIPT_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_SCRIPT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_SCRIPT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_SCRIPT_CMD, 0, RPCFunc_StopScript, sizeof(RPC_API_START_SCRIPT_MESSAGE), sizeof(RPC_API_START_SCRIPT_MESSAGE), STRINGIZE(RPC_API_START_SCRIPT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_SCRIPT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_RECORDER_CMD, 0, RPCFunc_StartRecorder, sizeof(RPC_API_START_RECORDER_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_RECORDER_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_RECORDER_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_RECORDER_CMD, 0, RPCFunc_StopRecorder, sizeof(RPC_API_STOP_RECORDER_MESSAGE), sizeof(RPC_API_STOP_RECORDER_MESSAGE), STRINGIZE(RPC_API_STOP_RECORDER_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_RECORDER_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_PLAYER_CMD, 0, RPCFunc_StartPlayer, sizeof(RPC_API_START_PLAYER_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_PLAYER_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_PLAYER_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_PLAYER_CMD, 0, RPCFunc_StopPlayer, sizeof(RPC_API_STOP_PLAYER_MESSAGE), sizeof(RPC_API_STOP_PLAYER_MESSAGE), STRINGIZE(RPC_API_STOP_PLAYER_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_PLAYER_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_EQUATIONS_CMD, 0, RPCFunc_StartEquations, sizeof(RPC_API_START_EQUATIONS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_EQUATIONS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_EQUATIONS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_EQUATIONS_CMD, 0, RPCFunc_StopEquations, sizeof(RPC_API_STOP_EQUATIONS_MESSAGE), sizeof(RPC_API_STOP_EQUATIONS_MESSAGE), STRINGIZE(RPC_API_STOP_EQUATIONS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_EQUATIONS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_GENERATOR_CMD, 0, RPCFunc_StartGenerator, sizeof(RPC_API_START_GENERATOR_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_GENERATOR_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_GENERATOR_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_GENERATOR_CMD, 0, RPCFunc_StopGenerator, sizeof(RPC_API_STOP_GENERATOR_MESSAGE), sizeof(RPC_API_STOP_GENERATOR_MESSAGE), STRINGIZE(RPC_API_STOP_GENERATOR_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_GENERATOR_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_RECORDER_ADD_COMMENT_CMD, 0, RPCFunc_RecorderAddComment, sizeof(RPC_API_RECORDER_ADD_COMMENT_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_RECORDER_ADD_COMMENT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_RECORDER_ADD_COMMENT_MESSAGE_ACK_MEMBERS));
    return 0;
}

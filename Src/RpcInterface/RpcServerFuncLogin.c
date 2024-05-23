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
#include <memory.h>
#include "Config.h"
#include "StringMaxChar.h"
#include "RpcSocketServer.h"
#include "RpcFuncLogin.h"

#include "EquationParser.h"
#include "ThrowError.h"
#include "Scheduler.h"

#define UNUSED(x) (void)(x)


static int RPCFunc_Login(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    RPC_API_LOGIN_MESSAGE_ACK *Out = (RPC_API_LOGIN_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_LOGIN_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOGIN_MESSAGE_ACK);
    Out->Version = XILENV_VERSION;
    par_Connection->ConnectedFlag = 1;
    return sizeof (RPC_API_LOGIN_MESSAGE_ACK);
}

static int RPCFunc_LogOut(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_LOGOUT_MESSAGE *In = (RPC_API_LOGOUT_MESSAGE*)par_DataIn;
    RPC_API_LOGOUT_MESSAGE_ACK *Out = (RPC_API_LOGOUT_MESSAGE_ACK*)par_DataOut;

    SetShouldExit(In->ShouldTerminate, In->SetExitCode, In->ExitCode);

    memset (Out, 0, sizeof (RPC_API_LOGOUT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOGOUT_MESSAGE_ACK);
    par_Connection->ConnectedFlag = 0;
    return -(int)sizeof(RPC_API_LOGOUT_MESSAGE_ACK);
}

static int RPCFunc_GetVersion(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    RPC_API_GET_VERSION_MESSAGE_ACK *Out = (RPC_API_GET_VERSION_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_GET_VERSION_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VERSION_MESSAGE_ACK);
    Out->Version =  XILENV_VERSION;
    Out->MinorVersion = XILENV_MINOR_VERSION;
    return sizeof (RPC_API_GET_VERSION_MESSAGE_ACK);
}

static int RPCFunc_Ping(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_PING_MESSAGE *In = (RPC_API_PING_MESSAGE*)par_DataIn;
    RPC_API_PING_MESSAGE_ACK *Out = (RPC_API_PING_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_PING_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_PING_MESSAGE);
    MEMCPY (Out->Data, In->Data, sizeof(Out->Data));
    return sizeof (RPC_API_PING_MESSAGE_ACK);
}

static int RPCFunc_ShouldBeTerminated(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_DataIn);
    RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK *Out = (RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK);
    StopAcceptingNewConnections(par_Connection);
    return sizeof(RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK);
}

static int RPCFunc_ClientPingAck(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_DataIn);
    UNUSED(par_DataOut);
    par_Connection->WaitToReceiveAlivePingAck = 0;
    return 0;  // no response!
}

int AddLoginFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOGIN_CMD, 0, RPCFunc_Login, sizeof(RPC_API_LOGIN_MESSAGE), sizeof(RPC_API_LOGIN_MESSAGE), STRINGIZE(RPC_API_LOGIN_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOGIN_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOGOUT_CMD, 0, RPCFunc_LogOut, sizeof(RPC_API_LOGOUT_MESSAGE), sizeof(RPC_API_LOGOUT_MESSAGE), STRINGIZE(RPC_API_LOGOUT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOGOUT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VERSION_CMD, 0, RPCFunc_GetVersion, sizeof(RPC_API_GET_VERSION_MESSAGE), sizeof(RPC_API_GET_VERSION_MESSAGE), STRINGIZE(RPC_API_GET_VERSION_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VERSION_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_PING_CMD, 0, RPCFunc_Ping, sizeof(RPC_API_PING_MESSAGE), sizeof(RPC_API_PING_MESSAGE), STRINGIZE(RPC_API_PING_MESSAGE_MEMBERS), STRINGIZE(RPC_API_PING_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SHOULD_BE_TERMINATED_CMD, 0, RPCFunc_ShouldBeTerminated, sizeof(RPC_API_SHOULD_BE_TERMINATED_MESSAGE), sizeof(RPC_API_SHOULD_BE_TERMINATED_MESSAGE), STRINGIZE(RPC_API_SHOULD_BE_TERMINATED_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_PING_TO_CLINT_CMD, 0, RPCFunc_ClientPingAck, sizeof(RPC_API_PING_TO_CLINT_MESSAGE), sizeof(RPC_API_PING_TO_CLINT_MESSAGE), STRINGIZE(RPC_API_PING_TO_CLINT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_PING_TO_CLINT_MESSAGE_ACK_MEMBERS));
    return 0;
}

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
//#include <sys/stat.h>
#include <fcntl.h>

#include "Config.h"

#include "MyMemory.h"
#include "XcpControl.h"

#include "RpcControlProcess.h"
#include "RpcSocketServer.h"
#include "RpcFuncXcp.h"

#define UNUSED(x) (void)(x)


static int RPCFunc_LoadXcpConfig(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_LOAD_XCP_CONFIG_MESSAGE *In = (RPC_API_LOAD_XCP_CONFIG_MESSAGE*)par_DataIn;
    RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK *Out = (RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        Out->Header.ReturnValue = LoadConfig_XCP (In->Connection, (char*)In + In->OffsetXcpFile);
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static void CleanUpXcpVariables(RPC_CONNECTION *par_Connection, int par_Nr)
{
    int x;
    for (x = 0; x < par_Connection->XCPVarCounter[par_Nr]; x++) {
        if (par_Connection->XCPVariable[par_Nr][x] != NULL) my_free(par_Connection->XCPVariable[par_Nr][x]);
    }
    par_Connection->XCPVarCounter[par_Nr] = 0;
    if (par_Connection->XCPVariable[par_Nr] != NULL) my_free(par_Connection->XCPVariable[par_Nr]);
    par_Connection->XCPVariable[par_Nr] = NULL;
}

static int RPCFunc_StartXcpBegin(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_START_XCP_BEGIN_MESSAGE *In = (RPC_API_START_XCP_BEGIN_MESSAGE*)par_DataIn;
    RPC_API_START_XCP_BEGIN_MESSAGE_ACK *Out = (RPC_API_START_XCP_BEGIN_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_START_XCP_BEGIN_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_XCP_BEGIN_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        CleanUpXcpVariables(par_Connection, In->Connection);
        Out->Header.ReturnValue = 0;
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartXcpAddVar(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_START_XCP_ADD_VAR_MESSAGE *In = (RPC_API_START_XCP_ADD_VAR_MESSAGE*)par_DataIn;
    RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK *Out = (RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        if (par_Connection->XCPVarCounter[In->Connection] < 100) {
            par_Connection->XCPVariable[In->Connection] = my_realloc(par_Connection->XCPVariable[In->Connection], sizeof(char*) * (size_t)(par_Connection->XCPVarCounter[In->Connection] + 1));
            par_Connection->XCPVariable[In->Connection][par_Connection->XCPVarCounter[In->Connection]] = my_malloc(strlen((char*)In + In->OffsetLabel) + 1);
            strcpy(par_Connection->XCPVariable[In->Connection][par_Connection->XCPVarCounter[In->Connection]], (char*)In + In->OffsetLabel);
            par_Connection->XCPVarCounter[In->Connection]++;

        } else {
            Out->Header.ReturnValue = -1;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartXcpEnd(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_START_XCP_END_MESSAGE *In = (RPC_API_START_XCP_END_MESSAGE*)par_DataIn;
    RPC_API_START_XCP_END_MESSAGE_ACK *Out = (RPC_API_START_XCP_END_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_START_XCP_END_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_XCP_END_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        if (par_Connection->XCPVarCounter[In->Connection] > 0) {
            Out->Header.ReturnValue = Start_XCP(In->Connection, START_MEASSUREMENT, par_Connection->XCPVarCounter[In->Connection], par_Connection->XCPVariable[In->Connection]);
            CleanUpXcpVariables(par_Connection, In->Connection);
        } else {
            Out->Header.ReturnValue = -1;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StopXcp(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_STOP_XCP_MESSAGE *In = (RPC_API_STOP_XCP_MESSAGE*)par_DataIn;
    RPC_API_STOP_XCP_MESSAGE_ACK *Out = (RPC_API_STOP_XCP_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_STOP_XCP_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_XCP_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        Out->Header.ReturnValue = Stop_XCP(In->Connection, STOP_MEASSUREMENT);
        CleanUpXcpVariables(par_Connection, In->Connection);
    }
    return Out->Header.StructSize;
}


static void CleanUpXcpParameters(RPC_CONNECTION *par_Connection, int par_Nr)
{
    int x;
    for (x = 0; x < par_Connection->XCPCalCounter[par_Nr]; x++) {
        if (par_Connection->XCPParameter[par_Nr][x] != NULL) my_free(par_Connection->XCPParameter[par_Nr][x]);
    }
    par_Connection->XCPCalCounter[par_Nr] = 0;
    if (par_Connection->XCPParameter[par_Nr] != NULL) my_free(par_Connection->XCPParameter[par_Nr]);
    par_Connection->XCPParameter[par_Nr] = NULL;
}


static int RPCFunc_StartXcpCalBegin(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_START_XCP_CAL_BEGIN_MESSAGE *In = (RPC_API_START_XCP_CAL_BEGIN_MESSAGE*)par_DataIn;
    RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK *Out = (RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        CleanUpXcpParameters(par_Connection, In->Connection);
        Out->Header.ReturnValue = 0;
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartXcpCalAddVar(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE *In = (RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE*)par_DataIn;
    RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK *Out = (RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        if (par_Connection->XCPCalCounter[In->Connection] < 100) {
            par_Connection->XCPParameter[In->Connection] = my_realloc(par_Connection->XCPParameter[In->Connection], sizeof(char*) * (size_t)(par_Connection->XCPCalCounter[In->Connection] + 1));
            par_Connection->XCPParameter[In->Connection][par_Connection->XCPCalCounter[In->Connection]] = my_malloc(strlen((char*)In + In->OffsetLabel) + 1);
            strcpy(par_Connection->XCPParameter[In->Connection][par_Connection->XCPCalCounter[In->Connection]], (char*)In + In->OffsetLabel);
            par_Connection->XCPCalCounter[In->Connection]++;

        } else {
            Out->Header.ReturnValue = -1;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartXcpCalEnd(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_START_XCP_CAL_END_MESSAGE *In = (RPC_API_START_XCP_CAL_END_MESSAGE*)par_DataIn;
    RPC_API_START_XCP_CAL_END_MESSAGE_ACK *Out = (RPC_API_START_XCP_CAL_END_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_START_XCP_CAL_END_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_XCP_CAL_END_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        if (par_Connection->XCPCalCounter[In->Connection] > 0) {
            Out->Header.ReturnValue = Start_XCP(In->Connection, START_CALIBRATION, par_Connection->XCPCalCounter[In->Connection], par_Connection->XCPParameter[In->Connection]);
            CleanUpXcpParameters(par_Connection, In->Connection);
        } else {
            Out->Header.ReturnValue = -1;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StopXcpCal(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_STOP_XCP_CAL_MESSAGE *In = (RPC_API_STOP_XCP_CAL_MESSAGE*)par_DataIn;
    RPC_API_STOP_XCP_CAL_MESSAGE_ACK *Out = (RPC_API_STOP_XCP_CAL_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_STOP_XCP_CAL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_XCP_CAL_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        Out->Header.ReturnValue = Stop_XCP(In->Connection, STOP_CALIBRATION);
        CleanUpXcpParameters(par_Connection, In->Connection);
    }
    return Out->Header.StructSize;
}

int AddXcpFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOAD_XCP_CONFIG_CMD, 1, RPCFunc_LoadXcpConfig, sizeof(RPC_API_LOAD_XCP_CONFIG_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_LOAD_XCP_CONFIG_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_XCP_BEGIN_CMD, 1, RPCFunc_StartXcpBegin, sizeof(RPC_API_START_XCP_BEGIN_MESSAGE), sizeof(RPC_API_START_XCP_BEGIN_MESSAGE), STRINGIZE(RPC_API_START_XCP_BEGIN_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_XCP_BEGIN_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_XCP_ADD_VAR_CMD, 1, RPCFunc_StartXcpAddVar, sizeof(RPC_API_START_XCP_ADD_VAR_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_XCP_ADD_VAR_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_XCP_END_CMD, 1, RPCFunc_StartXcpEnd, sizeof(RPC_API_START_XCP_END_MESSAGE), sizeof(RPC_API_START_XCP_END_MESSAGE), STRINGIZE(RPC_API_START_XCP_END_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_XCP_END_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_XCP_CMD, 1, RPCFunc_StopXcp, sizeof(RPC_API_STOP_XCP_MESSAGE), sizeof(RPC_API_STOP_XCP_MESSAGE), STRINGIZE(RPC_API_STOP_XCP_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_XCP_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_XCP_CAL_BEGIN_CMD, 1, RPCFunc_StartXcpCalBegin, sizeof(RPC_API_START_XCP_CAL_BEGIN_MESSAGE), sizeof(RPC_API_START_XCP_CAL_BEGIN_MESSAGE), STRINGIZE(RPC_API_START_XCP_CAL_BEGIN_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_XCP_CAL_ADD_VAR_CMD, 1, RPCFunc_StartXcpCalAddVar, sizeof(RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_XCP_CAL_END_CMD, 1, RPCFunc_StartXcpCalEnd, sizeof(RPC_API_START_XCP_CAL_END_MESSAGE), sizeof(RPC_API_START_XCP_CAL_END_MESSAGE), STRINGIZE(RPC_API_START_XCP_CAL_END_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_XCP_CAL_END_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_XCP_CAL_CMD, 1, RPCFunc_StopXcpCal, sizeof(RPC_API_STOP_XCP_CAL_MESSAGE), sizeof(RPC_API_STOP_XCP_CAL_MESSAGE), STRINGIZE(RPC_API_STOP_XCP_CAL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_XCP_CAL_MESSAGE_ACK_MEMBERS));
    return 0;
}

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

#include "MyMemory.h"
#include "StringMaxChar.h"
#include "CcpControl.h"

#include "RpcControlProcess.h"
#include "RpcSocketServer.h"
#include "RpcFuncCcp.h"

#define UNUSED(x) (void)(x)



static int RPCFunc_LoadCcpConfig(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_LOAD_CCP_CONFIG_MESSAGE *In = (RPC_API_LOAD_CCP_CONFIG_MESSAGE*)par_DataIn;
    RPC_API_LOAD_CCP_CONFIG_MESSAGE_ACK *Out = (RPC_API_LOAD_CCP_CONFIG_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_LOAD_CCP_CONFIG_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOAD_CCP_CONFIG_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        Out->Header.ReturnValue = LoadConfig_CCP (In->Connection, (char*)In + In->OffsetCcpFile);
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static void CleanUpCcpVariables(RPC_CONNECTION *par_Connection, int par_Nr)
{
    int x;
    for (x = 0; x < par_Connection->CCPVarCounter[par_Nr]; x++) {
        if (par_Connection->CCPVariable[par_Nr][x] != NULL) my_free(par_Connection->CCPVariable[par_Nr][x]);
    }
    par_Connection->CCPVarCounter[par_Nr] = 0;
    if (par_Connection->CCPVariable[par_Nr] != NULL) my_free(par_Connection->CCPVariable[par_Nr]);
    par_Connection->CCPVariable[par_Nr] = NULL;
}

static int RPCFunc_StartCcpBegin(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_START_CCP_BEGIN_MESSAGE *In = (RPC_API_START_CCP_BEGIN_MESSAGE*)par_DataIn;
    RPC_API_START_CCP_BEGIN_MESSAGE_ACK *Out = (RPC_API_START_CCP_BEGIN_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_CCP_BEGIN_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_CCP_BEGIN_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        CleanUpCcpVariables(par_Connection, In->Connection);
        Out->Header.ReturnValue = 0;
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartCcpAddVar(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_START_CCP_ADD_VAR_MESSAGE *In = (RPC_API_START_CCP_ADD_VAR_MESSAGE*)par_DataIn;
    RPC_API_START_CCP_ADD_VAR_MESSAGE_ACK *Out = (RPC_API_START_CCP_ADD_VAR_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_CCP_ADD_VAR_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_CCP_ADD_VAR_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        if (par_Connection->CCPVarCounter[In->Connection] < 100) {
            par_Connection->CCPVariable[In->Connection] = my_realloc(par_Connection->CCPVariable[In->Connection], sizeof(char*) * (size_t)(par_Connection->CCPVarCounter[In->Connection] + 1));
            par_Connection->CCPVariable[In->Connection][par_Connection->CCPVarCounter[In->Connection]] = StringMalloc((char*)In + In->OffsetLabel);
            par_Connection->CCPVarCounter[In->Connection]++;
        } else {
            Out->Header.ReturnValue = -1;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartCcpEnd(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_START_CCP_END_MESSAGE *In = (RPC_API_START_CCP_END_MESSAGE*)par_DataIn;
    RPC_API_START_CCP_END_MESSAGE_ACK *Out = (RPC_API_START_CCP_END_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_CCP_END_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_CCP_END_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        if (par_Connection->CCPVarCounter[In->Connection] > 0) {
            Out->Header.ReturnValue = Start_CCP(In->Connection, START_MEASSUREMENT, par_Connection->CCPVarCounter[In->Connection], par_Connection->CCPVariable[In->Connection]);
            CleanUpCcpVariables(par_Connection, In->Connection);
        } else {
            Out->Header.ReturnValue = -1;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StopCcpEnd(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_STOP_CCP_MESSAGE *In = (RPC_API_STOP_CCP_MESSAGE*)par_DataIn;
    RPC_API_STOP_CCP_MESSAGE_ACK *Out = (RPC_API_STOP_CCP_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_CCP_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_CCP_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        Out->Header.ReturnValue = Stop_CCP(In->Connection, STOP_MEASSUREMENT);
        CleanUpCcpVariables(par_Connection, In->Connection);
    }
    return Out->Header.StructSize;
}



static void CleanUpCcpParameters(RPC_CONNECTION *par_Connection, int par_Nr)
{
    int x;
    for (x = 0; x < par_Connection->CCPCalCounter[par_Nr]; x++) {
        if (par_Connection->CCPParameter[par_Nr][x] != NULL) my_free(par_Connection->CCPParameter[par_Nr][x]);
    }
    par_Connection->CCPCalCounter[par_Nr] = 0;
    if (par_Connection->CCPParameter[par_Nr] != NULL) my_free(par_Connection->CCPParameter[par_Nr]);
    par_Connection->CCPParameter[par_Nr] = NULL;
}

static int RPCFunc_StartCcpCalBegin(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_START_CCP_CAL_BEGIN_MESSAGE *In = (RPC_API_START_CCP_CAL_BEGIN_MESSAGE*)par_DataIn;
    RPC_API_START_CCP_CAL_BEGIN_MESSAGE_ACK *Out = (RPC_API_START_CCP_CAL_BEGIN_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_CCP_CAL_BEGIN_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_CCP_CAL_BEGIN_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        CleanUpCcpParameters(par_Connection, In->Connection);
        Out->Header.ReturnValue = 0;
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartCcpCalAddVar(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE *In = (RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE*)par_DataIn;
    RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE_ACK *Out = (RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        if (par_Connection->CCPCalCounter[In->Connection] < 100) {
            par_Connection->CCPParameter[In->Connection] = my_realloc(par_Connection->CCPParameter[In->Connection], sizeof(char*) * (size_t)(par_Connection->CCPCalCounter[In->Connection] + 1));
            par_Connection->CCPParameter[In->Connection][par_Connection->CCPCalCounter[In->Connection]] = StringMalloc((char*)In + In->OffsetLabel);
            par_Connection->CCPCalCounter[In->Connection]++;
        } else {
            Out->Header.ReturnValue = -1;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StartCcpCalEnd(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_START_CCP_CAL_END_MESSAGE *In = (RPC_API_START_CCP_CAL_END_MESSAGE*)par_DataIn;
    RPC_API_START_CCP_CAL_END_MESSAGE_ACK *Out = (RPC_API_START_CCP_CAL_END_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_START_CCP_CAL_END_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_CCP_CAL_END_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        if (par_Connection->CCPCalCounter[In->Connection] > 0) {
            Out->Header.ReturnValue = Start_CCP(In->Connection, START_CALIBRATION, par_Connection->CCPCalCounter[In->Connection], par_Connection->CCPParameter[In->Connection]);
            CleanUpCcpParameters(par_Connection, In->Connection);
        } else {
            Out->Header.ReturnValue = -1;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_StopCcpCalEnd(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);

    RPC_API_STOP_CCP_CAL_MESSAGE *In = (RPC_API_STOP_CCP_CAL_MESSAGE*)par_DataIn;
    RPC_API_STOP_CCP_CAL_MESSAGE_ACK *Out = (RPC_API_STOP_CCP_CAL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_STOP_CCP_CAL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_CCP_CAL_MESSAGE_ACK);
    if ((In->Connection >= 0) && (In->Connection < 4)) {
        Out->Header.ReturnValue = Stop_CCP(In->Connection, STOP_CALIBRATION);
        CleanUpCcpParameters(par_Connection, In->Connection);
    }
    return Out->Header.StructSize;
}

int AddCcpFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOAD_CCP_CONFIG_CMD, 1, RPCFunc_LoadCcpConfig, sizeof(RPC_API_LOAD_CCP_CONFIG_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_LOAD_CCP_CONFIG_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOAD_CCP_CONFIG_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_CCP_BEGIN_CMD, 1, RPCFunc_StartCcpBegin, sizeof(RPC_API_START_CCP_BEGIN_MESSAGE), sizeof(RPC_API_START_CCP_BEGIN_MESSAGE), STRINGIZE(RPC_API_START_CCP_BEGIN_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_CCP_BEGIN_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_CCP_ADD_VAR_CMD, 1, RPCFunc_StartCcpAddVar, sizeof(RPC_API_START_CCP_ADD_VAR_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_CCP_ADD_VAR_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_CCP_ADD_VAR_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_CCP_END_CMD, 1, RPCFunc_StartCcpEnd, sizeof(RPC_API_START_CCP_END_MESSAGE), sizeof(RPC_API_START_CCP_END_MESSAGE), STRINGIZE(RPC_API_START_CCP_END_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_CCP_END_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_CCP_CMD, 1, RPCFunc_StopCcpEnd, sizeof(RPC_API_STOP_CCP_MESSAGE), sizeof(RPC_API_STOP_CCP_MESSAGE), STRINGIZE(RPC_API_STOP_CCP_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_CCP_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_CCP_CAL_BEGIN_CMD, 1, RPCFunc_StartCcpCalBegin, sizeof(RPC_API_START_CCP_CAL_BEGIN_MESSAGE), sizeof(RPC_API_START_CCP_CAL_BEGIN_MESSAGE), STRINGIZE(RPC_API_START_CCP_CAL_BEGIN_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_CCP_CAL_BEGIN_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_CCP_CAL_ADD_VAR_CMD, 1, RPCFunc_StartCcpCalAddVar, sizeof(RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_CCP_CAL_END_CMD, 1, RPCFunc_StartCcpCalEnd, sizeof(RPC_API_START_CCP_CAL_END_MESSAGE), sizeof(RPC_API_START_CCP_CAL_END_MESSAGE), STRINGIZE(RPC_API_START_CCP_CAL_END_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_CCP_CAL_END_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_CCP_CAL_CMD, 1, RPCFunc_StopCcpCalEnd, sizeof(RPC_API_STOP_CCP_CAL_MESSAGE), sizeof(RPC_API_STOP_CCP_CAL_MESSAGE), STRINGIZE(RPC_API_STOP_CCP_CAL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_STOP_CCP_CAL_MESSAGE_ACK_MEMBERS));
    return 0;
}

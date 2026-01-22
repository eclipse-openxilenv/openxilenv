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

#include "StringMaxChar.h"

#include "LoadSaveToFile.h"
#include "ExtProcessReferences.h"

#include "A2LLink.h"
#include "A2LLinkThread.h"
#include "A2LAccessData.h"

#include "RpcSocketServer.h"
#include "RpcFuncCalibration.h"

#define UNUSED(x) (void)(x)

static int RPCFunc_LoadSvl(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_LOAD_SVL_MESSAGE *In = (RPC_API_LOAD_SVL_MESSAGE*)par_DataIn;
    RPC_API_LOAD_SVL_MESSAGE_ACK *Out = (RPC_API_LOAD_SVL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_LOAD_SVL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOAD_SVL_MESSAGE_ACK);
    Out->Header.ReturnValue = (ScriptWriteSVLToProcess((char*)In + In->OffsetSvlFilename, (char*)In + In->OffsetProcess) == 0);
    return Out->Header.StructSize;
}


static int RPCFunc_SaveSvl(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SAVE_SVL_MESSAGE *In = (RPC_API_SAVE_SVL_MESSAGE*)par_DataIn;
    RPC_API_SAVE_SVL_MESSAGE_ACK *Out = (RPC_API_SAVE_SVL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SAVE_SVL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SAVE_SVL_MESSAGE_ACK);
    Out->Header.ReturnValue = ScriptWriteProcessToSVL((char*)In + In->OffsetSvlFilename, (char*)In + In->OffsetProcess, (char*)In + In->OffsetFilter);
    return Out->Header.StructSize;
}

static int RPCFunc_SaveSal(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SAVE_SAL_MESSAGE *In = (RPC_API_SAVE_SAL_MESSAGE*)par_DataIn;
    RPC_API_SAVE_SAL_MESSAGE_ACK *Out = (RPC_API_SAVE_SAL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SAVE_SAL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SAVE_SAL_MESSAGE_ACK);
    Out->Header.ReturnValue = ScriptWriteProcessToSAL((char*)In + In->OffsetSalFilename, (char*)In + In->OffsetProcess, (char*)In + In->OffsetFilter, 0);
    return Out->Header.StructSize;
}

static int RPCFunc_GetSymbolRaw(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_SYMBOL_RAW_MESSAGE *In = (RPC_API_GET_SYMBOL_RAW_MESSAGE*)par_DataIn;
    RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK *Out = (RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK);

    Out->Header.ReturnValue = GetRawValueOfOneSymbol((char*)In + In->OffsetSymbol,
                       (char*)In + In->OffsetProcess,
                       In->Flags & 0x1000, &(Out->Value));    // 0x1000 -> no call of ThrowError!

    return Out->Header.StructSize;
}

static int RPCFunc_SetSymbolRaw(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_SYMBOL_RAW_MESSAGE *In = (RPC_API_SET_SYMBOL_RAW_MESSAGE*)par_DataIn;
    RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK *Out = (RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK);

    Out->Header.ReturnValue = SetRawValueOfOneSymbol((char*)In + In->OffsetSymbol,
                       (char*)In + In->OffsetProcess,
                       In->Flags & 0x1000, In->DataType, In->Value);    // 0x1000 -> no call of ThrowError!

    return Out->Header.StructSize;
}

// A2L Links

static int RPCFunc_SetupLinkToExternProcess(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE *In = (RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE*)par_DataIn;
    RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK *Out = (RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK);

    Out->Header.ReturnValue = A2LSetupLinkToExternProcess((char*)In + In->OffsetA2LFileName, (char*)In + In->OffsetProcessName,
                                                          In->UpdateFlag);
    return Out->Header.StructSize;
}

static int RPCFunc_GetLinkToExternProcess(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE *In = (RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE*)par_DataIn;
    RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK *Out = (RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK));
    Out->Header.StructSize = (int)sizeof(RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK);

    Out->Header.ReturnValue = A2LGetLinkToExternProcess((char*)In + In->OffsetProcessName);
    return Out->Header.StructSize;
}

static int RPCFunc_GetIndexFromLink(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_INDEX_FROM_LINK_MESSAGE *In = (RPC_API_GET_INDEX_FROM_LINK_MESSAGE*)par_DataIn;
    RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK *Out = (RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK));
    Out->Header.StructSize = (int)sizeof(RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK);

    Out->Header.ReturnValue = A2LGetIndexFromLink(In->LinkNr, (char*)In + In->OffsetLabel, In->TypeMask);
    return Out->Header.StructSize;
}

static int RPCFunc_GetNextSymbolFromLink(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE *In = (RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE*)par_DataIn;
    RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK *Out = (RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK));
    Out->Header.StructSize = (int)sizeof(RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK);

    Out->Header.ReturnValue = A2LGetNextSymbolFromLink(In->LinkNr, In->Index, In->TypeMask, (char*)In + In->OffsetFilter, Out->Data, In->MaxChar);
    if (Out->Header.ReturnValue >= 0) {
        Out->Header.StructSize += (int)strlen(Out->Data);
        Out->OffsetRetName = (int)sizeof(RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK) - 1;
    } else {
        Out->OffsetRetName = -1;
    }
    return Out->Header.StructSize;
}

typedef struct {
    RPC_CONNECTION *Connection;
    void *Data;
    const char *Error;
} GET_SET_DATA_FROM_TO_LINK;

static int __RPCFunc_GetDataFromLink(void *par_IndexData, int par_FetchDataChannelNo, void *par_Param)
{
    UNUSED(par_FetchDataChannelNo);
    INDEX_DATA_BLOCK *IndexData = (INDEX_DATA_BLOCK*)par_IndexData;

    GET_SET_DATA_FROM_TO_LINK *Parameter = (GET_SET_DATA_FROM_TO_LINK*)par_Param;
    if (IndexData->Size == 1) {
        Parameter->Data = IndexData->Data->Data;
        Parameter->Error = IndexData->Data->Error;
    } else {
        Parameter->Data = NULL;
#define STRINGIFY(s) #s
        Parameter->Error = "error" __FILE__ " (" STRINGIFY(__LINE__) ")";
    }
    RemoteProcedureWakeWaitForConnection(Parameter->Connection);
    return 0;
}

static int RPCFunc_GetDataFromLink(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    void *Data;
    //UNUSED(par_Connection);
    const char* Error;
    int Size;
    RPC_API_GET_DATA_FROM_LINK_MESSAGE *In = (RPC_API_GET_DATA_FROM_LINK_MESSAGE*)par_DataIn;
    RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK *Out = (RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK));
    Out->Header.StructSize = (int)sizeof(RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK);
    GET_SET_DATA_FROM_TO_LINK Parameter;
    Parameter.Data = NULL;
    Parameter.Connection = par_Connection;

    int GetDataChannelNo = NewDataFromLinkChannel(In->LinkNr, 1000*1000*1000, // 1 sec timeout
                                                  __RPCFunc_GetDataFromLink, (void*)&Parameter);

    INDEX_DATA_BLOCK *idb = GetIndexDataBlock(1);
    idb->Data->LinkNo = In->LinkNr;
    idb->Data->Index = In->Index;
    idb->Data->Data = NULL;
    idb->Data->Flags = In->PhysFlag;
    RemoteProcedureMarkedForWaitForConnection(par_Connection);

    A2LGetDataFromLinkReq(GetDataChannelNo, 0, idb);   // 0 -> Read data

    RemoteProcedureWaitForConnection(par_Connection);

    DeleteDataFromLinkChannel(GetDataChannelNo);
    Data = Parameter.Data;
    Error = Parameter.Error;
    FreeIndexDataBlock(idb);

    if (Data != NULL) {
        Out->Header.ReturnValue = 0;
        Size = *(int*)Data;  // the size of the structure are stored inside the first 4 bytes
        Out->Header.StructSize += Size - 1;
        Out->OffsetData = sizeof(RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK) - 1;
        MEMCPY((char*)Out + Out->OffsetData, Data, Size);
        Out->OffsetRetError = -1;
        FreeA2lData(Data);   // the data from A2LGetDataFromLink must be deleted!
    } else {
        Out->Header.ReturnValue = -1;
        Size = (int)strlen(Error);
        Out->Header.StructSize += Size;
        Out->OffsetRetError = (int)sizeof(RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK) - 1;
        MEMCPY((char*)Out + Out->OffsetRetError, Error, Size + 1);
        Out->OffsetData = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_SetDataToLink(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    //UNUSED(par_Connection);
    const char* Error;
    int Size;
    RPC_API_SET_DATA_TO_LINK_MESSAGE *In = (RPC_API_SET_DATA_TO_LINK_MESSAGE*)par_DataIn;
    RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK *Out = (RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK));
    Out->Header.StructSize = (int)sizeof(RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK);
    GET_SET_DATA_FROM_TO_LINK Parameter;
    Parameter.Data = NULL;
    Parameter.Connection = par_Connection;

    int GetDataChannelNo = NewDataFromLinkChannel(In->LinkNr, 1000*1000*1000, // 1 sec timeout
                                                  __RPCFunc_GetDataFromLink, (void*)&Parameter);

    INDEX_DATA_BLOCK *idb = GetIndexDataBlock(1);
    idb->Data->LinkNo = In->LinkNr;
    idb->Data->Index = In->Index;
    idb->Data->Data = (char*)In + In->OffsetData;
    idb->Data->Flags = 0;
    RemoteProcedureMarkedForWaitForConnection(par_Connection);

    A2LGetDataFromLinkReq(GetDataChannelNo, 1, idb);  // 1 -> Write data

    RemoteProcedureWaitForConnection(par_Connection);

    DeleteDataFromLinkChannel(GetDataChannelNo);
    Error = Parameter.Error;
    FreeIndexDataBlock(idb);

    if (Out->Header.ReturnValue == 0) {
        Out->OffsetRetError = -1;
    } else {
        Size = (int)strlen(Error);
        Out->Header.StructSize += Size;
        Out->OffsetRetError = (int)sizeof(RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK) - 1;
        MEMCPY((char*)Out + Out->OffsetRetError, Error, Size + 1);
    }
    return Out->Header.StructSize;
}

static int RPCFunc_ReferenceMeasurementToBlackboard(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE *In = (RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE*)par_DataIn;
    RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK *Out = (RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK));
    Out->Header.StructSize = (int)sizeof(RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK);

    Out->Header.ReturnValue = A2LReferenceMeasurementToBlackboard(In->LinkNr, In->Index, In->DirFlags, 1);
    return Out->Header.StructSize;
}

static int RPCFunc_DereferenceMeasurementFromBlackboard(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE *In = (RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE*)par_DataIn;
    RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK *Out = (RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK));
    Out->Header.StructSize = (int)sizeof(RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK);

    Out->Header.ReturnValue = A2LReferenceMeasurementToBlackboard(In->LinkNr, In->Index, 0, 0);
    return Out->Header.StructSize;
}



int AddCalibrationFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOAD_SVL_CMD, 1, RPCFunc_LoadSvl, sizeof(RPC_API_LOAD_SVL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_LOAD_SVL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOAD_SVL_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SAVE_SVL_CMD, 1, RPCFunc_SaveSvl, sizeof(RPC_API_SAVE_SVL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SAVE_SVL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SAVE_SVL_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SAVE_SAL_CMD, 1, RPCFunc_SaveSal, sizeof(RPC_API_SAVE_SAL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SAVE_SAL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SAVE_SAL_MESSAGE_ACK_MEMBERS));

    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_SYMBOL_RAW_CMD, 0, RPCFunc_GetSymbolRaw, sizeof(RPC_API_GET_SYMBOL_RAW_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_SYMBOL_RAW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_SYMBOL_RAW_CMD, 0, RPCFunc_SetSymbolRaw, sizeof(RPC_API_SET_SYMBOL_RAW_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_SYMBOL_RAW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK_MEMBERS));

    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_CMD, 0, RPCFunc_SetupLinkToExternProcess, sizeof(RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_LINK_TO_EXTERN_PROCESS_CMD, 0, RPCFunc_GetLinkToExternProcess, sizeof(RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_LINK_TO_EXTERN_PROCESSS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_LINK_TO_EXTERN_PROCESSS_MESSAGE_ACK_MEMBERS));

    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_INDEX_FROM_LINK_CMD, 0, RPCFunc_GetIndexFromLink, sizeof(RPC_API_GET_INDEX_FROM_LINK_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_INDEX_FROM_LINK_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_NEXT_SYMBOL_FROM_LINK_CMD, 0, RPCFunc_GetNextSymbolFromLink, sizeof(RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK_MEMBERS));

    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_DATA_FROM_LINK_CMD, 0, RPCFunc_GetDataFromLink, sizeof(RPC_API_GET_DATA_FROM_LINK_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_DATA_FROM_LINK_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_DATA_TO_LINK_CMD, 0, RPCFunc_SetDataToLink, sizeof(RPC_API_SET_DATA_TO_LINK_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_DATA_TO_LINK_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK_MEMBERS));

    AddFunctionToRemoteAPIFunctionTable2(RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_CMD, 0, RPCFunc_ReferenceMeasurementToBlackboard, sizeof(RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE), sizeof(RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE), STRINGIZE(RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_MEMBERS), STRINGIZE(RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_CMD, 0, RPCFunc_DereferenceMeasurementFromBlackboard, sizeof(RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE), sizeof(RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE), STRINGIZE(RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK_MEMBERS));

    return 0;
}

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
#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include "Config.h"
#include "MainValues.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "IniDataBase.h"
#include "tcb.h"
#include "Scheduler.h"
#include "GetNextStructEntry.h"
#include "Scheduler.h"
#include "ScBbCopyLists.h"
#include "VirtualNetwork.h"
#include "ExtProcessRefFilter.h"
#include "ScriptMessageFile.h"
#include "PipeMessages.h"
#include "SocketMessages.h"
#include "UnixDomainSocketMessages.h"
#include "BaseMessages.h"
#include "RemoteMasterOther.h"

#define UNUSED(x) (void)(x)

//#define FUNCTION_RUNTIME
//#include "fkt_runtime.h"

// #define USE_DEBUG_API

#define UNUSED(x) (void)(x)

#define TERMINATE_EXTERN_PROCESS_IMMEDIATELY  -12345678

static CRITICAL_SECTION LoggingFileCS;
static CRITICAL_SECTION BuffersCS;


void MessageLoggingToFile (FILE *LoggingFile, int par_LineNr, int par_Dir, int par_Pid, void *par_Message)
{
    if (LoggingFile != NULL) {
        PIPE_API_BASE_CMD_MESSAGE *Message = (PIPE_API_BASE_CMD_MESSAGE*)par_Message;
        uint64_t Tick = GetTickCount64();
        EnterCriticalSection(&LoggingFileCS);;
        if (par_Dir) {
            fprintf (LoggingFile, "%" PRIu64 " -> %i (%04i): ", Tick, par_Pid, par_LineNr);
        } else {
            fprintf (LoggingFile, "%" PRIu64 " %i -> (%04i): ", Tick, par_Pid, par_LineNr);
        }
        fprintf (LoggingFile, "Func = %i\n", Message->Command);
        fflush (LoggingFile);
        LeaveCriticalSection(&LoggingFileCS);;
    }
}

static void *GetFreeMessageBuffer(void *par_Buffer)
{
    void *Ret = NULL;
    static void *FreeBuffers[64];
    static int FreeBuffersPos = 0;

    EnterCriticalSection(&BuffersCS);
    if (par_Buffer == NULL) {
        // get
        if (FreeBuffersPos > 0) {
            FreeBuffersPos--;
            Ret = FreeBuffers[FreeBuffersPos];
        } else {
            // add a new one
            Ret = my_malloc(PIPE_MESSAGE_BUFSIZE);
        }
    } else {
        if (FreeBuffersPos < 64) {
            Ret = FreeBuffers[FreeBuffersPos] = par_Buffer;
            FreeBuffersPos++;
        }
    }
    LeaveCriticalSection(&BuffersCS);
    if (Ret == NULL) {
        ThrowError(1, "more than 64 message buffers %s (%i)", __FILE__, __LINE__);
    }
    return Ret;
}

int GetAssociatedSchedulerName (char *Processname, char *ret_SchedulerName, int MaxLen) 
{
    int s;
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char ShortProcessName[MAX_PATH];
    char SchedulerName[MAX_PATH];
    char SchedulerName2[MAX_PATH];
    int Fd = GetMainFileDescriptor();

    TruncatePathFromProcessName (ShortProcessName, Processname);
    sprintf (Entry, "SchedulerForProcess %s", ShortProcessName);
    if (IniFileDataBaseReadString (SCHED_INI_SECTION, Entry, "", SchedulerName, sizeof (SchedulerName), Fd) <= 0) {
        return -1;  // There are no infos inside the INI-DB for this process
    }
    // Check if the requested scheduler exists
    for (s = 0; ; s++) {
        sprintf (Entry, "Scheduler_%i", s);
        if (IniFileDataBaseReadString (SCHED_INI_SECTION, Entry, "", SchedulerName2, sizeof (SchedulerName2), Fd) <= 0) {
            break;
        }
        if (!strcmp (SchedulerName2, SchedulerName)) {
            if ((int)strlen (SchedulerName) < MaxLen) {
                strcpy (ret_SchedulerName, SchedulerName);
                return 0;
            } else {
                return -1;
            }
        }
    }
    return -1;
}


int InitExternProcessMessages (char *par_Prefix, int par_LogingFlag, unsigned int par_SocketPort)
{
    int Ret = 0;
    InitializeCriticalSection(&BuffersCS);
    if (par_LogingFlag) {
        InitializeCriticalSection(&LoggingFileCS);
    }
#ifdef _WIN32
    if (InitPipeMessages (par_Prefix, par_LogingFlag) != 0) Ret = -1;
#else
    if (UnixDomainSocket_InitMessages (par_Prefix, par_LogingFlag) != 0) Ret = 1;
#endif
    if (par_SocketPort > 0) {
        char Help[64];
        sprintf (Help, "%u", par_SocketPort);
        if (Socket_InitMessages (Help, par_LogingFlag) != 0) Ret = 1;
    }
    return Ret;
}

void TerminateExternProcessMessages (void)
{
    Socket_TerminateMessages();
}

int PipeAddBbvariCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                             PIPE_API_ADD_BBVARI_CMD_MESSAGE *pAddBbvariCmdMessage,
                             PIPE_API_ADD_BBVARI_CMD_MESSAGE_ACK *pAddBbvariCmdMessageAck)
{
    UNUSED(pTcb);
    VID Vid;
    int PipeDataType;
    int DataType;
    int AddrNotName;
    char *Name;
    uint64_t Ptr;
    int AllInfosFlag;
    char Label[BBVARI_NAME_SIZE];

    Name = pAddBbvariCmdMessage->Data + pAddBbvariCmdMessage->NameStructOffset;

    AddrNotName = ((Name[0] == '0') && (Name[1] == 'x'));

    AllInfosFlag = pAddBbvariCmdMessage->DataType & 0x1000;
    DataType = pAddBbvariCmdMessage->DataType & 0xFF;
    if (IfUnknowWaitDataTypeConvert (DataType, &PipeDataType)) {
        DataType = BB_UNKNOWN_WAIT;
    }

    /* Name is not a label but an address. The label should be find by the address inside the debug information */
    if (AddrNotName ||
        (pAddBbvariCmdMessage->AddressValidFlag && (DataType == BB_GET_DATA_TYPE_FOM_DEBUGINFO))){
        int TypeFromDebugInfos;
        int Ret;
        if (AddrNotName) Ptr = strtoull (Name, NULL, 16);  // Addresse are included as string inside the name
        else Ptr = pAddBbvariCmdMessage->Address;
        if ((Ret = DbgInfoTranslatePointerToLabel (Ptr, Label, sizeof (Label), &TypeFromDebugInfos, pAddBbvariCmdMessage->Pid)) != 0) {
            Vid = Ret;  // LABEL_NAME_TOO_LONG or ADDRESS_NO_VALID_LABEL
            goto __OUT;
        } else {
            if (!AddrNotName) {
                strcpy (Label, Name);
            }

        }
        if  (DataType == BB_GET_DATA_TYPE_FOM_DEBUGINFO) {
            DataType = TypeFromDebugInfos;
        }
    } else {
        if (ConvertLabelAsapCombatibleInOut (Name, Label, sizeof (Label), 0)) {   // replace :: by ._. if necessary
            strcpy (Label, Name);
        }
    }

    if (((pAddBbvariCmdMessage->Dir & PIPE_API_REFERENCE_VARIABLE_DIR_IGNORE_REF_FILTER) != PIPE_API_REFERENCE_VARIABLE_DIR_IGNORE_REF_FILTER) &&
        !ExternProcessReferenceMatchFilter(pAddBbvariCmdMessage->Pid, Label)) {
        Vid = VARIABLE_REF_LIST_FILTERED;
    } else {
        Vid = add_bbvari_all_infos (pAddBbvariCmdMessage->Pid,
                                    Label,
                                    DataType | AllInfosFlag,
                                    pAddBbvariCmdMessage->Data + pAddBbvariCmdMessage->UnitStructOffset,
                                    pAddBbvariCmdMessage->ConversionType,
                                    pAddBbvariCmdMessage->Data + pAddBbvariCmdMessage->ConversionStructOffset,
                                    pAddBbvariCmdMessage->Min.d,
                                    pAddBbvariCmdMessage->Max.d,
                                    pAddBbvariCmdMessage->Width,
                                    pAddBbvariCmdMessage->Prec,
                                    (int)pAddBbvariCmdMessage->RgbColor,
                                    pAddBbvariCmdMessage->StepType,
                                    pAddBbvariCmdMessage->Step.d,
                                    pAddBbvariCmdMessage->Dir,
                                    pAddBbvariCmdMessage->ValueValidFlag,
                                    pAddBbvariCmdMessage->Value,
                                    pAddBbvariCmdMessage->Address,
                                    pAddBbvariCmdMessage->AddressValidFlag,
                                    &(pAddBbvariCmdMessageAck->RealType));
        if (Vid > 0) {  // Only if variable are successful added to blackboard add it to copy list
            // The copy list must be always consistent to that inside the external Process
            int TypePipe;
            if (!IfUnknowDataTypeConvert (PipeDataType, &TypePipe)) {
                // If no type conversion than  blackboard type equal to pipe type
                if ((TypePipe == BB_GET_DATA_TYPE_FOM_DEBUGINFO) ||
                    (TypePipe == BB_UNKNOWN)) {   // add_bbvai (..., BB_UNKNOWN, ...)
                    TypePipe = pAddBbvariCmdMessageAck->RealType;
                }
            }
            if (pAddBbvariCmdMessage->AddressValidFlag &&
                (pAddBbvariCmdMessage->Address != 0) &&
                (pAddBbvariCmdMessage->UniqueId != 0) &&
                (pAddBbvariCmdMessage->UniqueId != 0xFFFFFFFFFFFFFFFFULL)) {  // Only add the reference to the copy list
                int Ret = InsertVariableToCopyLists (pAddBbvariCmdMessage->Pid, Vid,
                                                    pAddBbvariCmdMessage->Address,
                                                    pAddBbvariCmdMessageAck->RealType,
                                                    TypePipe,
                                                    pAddBbvariCmdMessage->Dir & 0xFFFF,
                                                    pAddBbvariCmdMessage->UniqueId);
                if (Ret < 0) {
                    remove_bbvari_pid(Vid, pAddBbvariCmdMessage->Pid);
                    Vid = Ret;
                }
            }
        }
    }
__OUT:
    pAddBbvariCmdMessageAck->Command = PIPE_API_ADD_BBVARI_CMD;
    pAddBbvariCmdMessageAck->StructSize = sizeof (PIPE_API_ADD_BBVARI_CMD_MESSAGE_ACK);
    pAddBbvariCmdMessageAck->Vid = Vid;
    pAddBbvariCmdMessageAck->ReturnValue = 0;

    return sizeof (PIPE_API_ADD_BBVARI_CMD_MESSAGE_ACK); 
}

int PipeAttachBbvariCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                PIPE_API_ATTACH_BBVARI_CMD_MESSAGE *pAttachBbvariCmdMessage,
                                PIPE_API_ATTACH_BBVARI_CMD_MESSAGE_ACK *pAttachBbvariCmdMessageAck)
{
    UNUSED(pTcb);
    VID Vid = attach_bbvari_by_name (pAttachBbvariCmdMessage->Name, pAttachBbvariCmdMessage->Pid);
    pAttachBbvariCmdMessageAck->Vid = Vid;

    pAttachBbvariCmdMessageAck->Command = PIPE_API_ATTACH_BBVARI_CMD;
    pAttachBbvariCmdMessageAck->StructSize = sizeof (PIPE_API_ATTACH_BBVARI_CMD_MESSAGE_ACK);
    pAttachBbvariCmdMessageAck->ReturnValue = 0;

    return sizeof (PIPE_API_ATTACH_BBVARI_CMD_MESSAGE_ACK);
}


int PipeRemoveBbvariCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                PIPE_API_REMOVE_BBVARI_CMD_MESSAGE *pRemoveBbvariCmdMessage,
                                PIPE_API_REMOVE_BBVARI_CMD_MESSAGE_ACK *pRemoveBbvariCmdMessageAck)
{
    UNUSED(pTcb);
    int Ret;

    // remove_bbvari_extp() will return the attach counter or a negativ value if an error occur.
    Ret = (remove_bbvari_extp (pRemoveBbvariCmdMessage->Vid, pRemoveBbvariCmdMessage->Pid, pRemoveBbvariCmdMessage->Dir, pRemoveBbvariCmdMessage->DataType) < 0);
    if ((pRemoveBbvariCmdMessage->Addr != 0) &&
        (pRemoveBbvariCmdMessage->UniqueId != 0) &&
        (pRemoveBbvariCmdMessage->UniqueId != 0xFFFFFFFFFFFFFFFFULL)) {  // Only remove from copy list if address and unique-ID are valid
        Ret = Ret || RemoveVariableFromCopyLists (pRemoveBbvariCmdMessage->Pid, pRemoveBbvariCmdMessage->Vid, pRemoveBbvariCmdMessage->UniqueId, pRemoveBbvariCmdMessage->Addr);
    }
    pRemoveBbvariCmdMessageAck->Command = PIPE_API_REMOVE_BBVARI_CMD;
    pRemoveBbvariCmdMessageAck->StructSize = sizeof (PIPE_API_REMOVE_BBVARI_CMD_MESSAGE_ACK);
    pRemoveBbvariCmdMessageAck->ReturnValue = Ret;
    return sizeof (PIPE_API_REMOVE_BBVARI_CMD_MESSAGE_ACK); 
}

int PipeGetLabelnameByAddressCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                         PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE *pGetLabelnameByAddressCmdMessage,
                                         PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK *pGetLabelnameByAddressCmdMessageAck)
{
    UNUSED(pTcb);
    int Ret;
    int DataType;     // Will not used
    Ret = DbgInfoTranslatePointerToLabel (pGetLabelnameByAddressCmdMessage->Address,
                                          pGetLabelnameByAddressCmdMessageAck->Label,
                                          BBVARI_NAME_SIZE, 
                                          &DataType,
                                          pGetLabelnameByAddressCmdMessage->Pid);
    if (Ret) {  // Error case -> empty string
        pGetLabelnameByAddressCmdMessageAck->Label[0] = 0;
    }
    pGetLabelnameByAddressCmdMessageAck->Command = PIPE_API_GET_LABEL_BY_ADDRESS_CMD;
    pGetLabelnameByAddressCmdMessageAck->StructSize = (int)(sizeof (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK) + strlen (pGetLabelnameByAddressCmdMessageAck->Label));
    pGetLabelnameByAddressCmdMessageAck->ReturnValue = Ret;

    return (int)(sizeof (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK) + strlen (pGetLabelnameByAddressCmdMessageAck->Label));
}


int PipeGetReferencedLabelnameByVidCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                               PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE *pGetReferencedLabelnameByVidCmdMessage,
                                               PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK *pGetReferemcedLabelnameByVidCmdMessageAck)
{
    UNUSED(pTcb);
    int Ret;

    Ret = GetReferencedLabelNameByVid (pGetReferencedLabelnameByVidCmdMessage->Pid,
                                       pGetReferencedLabelnameByVidCmdMessage->Vid,
                                       pGetReferemcedLabelnameByVidCmdMessageAck->Label,
                                       BBVARI_NAME_SIZE);
    if (Ret) {  // Error case -> empty string
        pGetReferemcedLabelnameByVidCmdMessageAck->Label[0] = 0;
    }
    pGetReferemcedLabelnameByVidCmdMessageAck->Command = PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD;
    pGetReferemcedLabelnameByVidCmdMessageAck->StructSize = (int32_t)(sizeof (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK) + strlen (pGetReferemcedLabelnameByVidCmdMessageAck->Label));
    pGetReferemcedLabelnameByVidCmdMessageAck->ReturnValue = Ret;

    return (int)(sizeof (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK) + strlen (pGetReferemcedLabelnameByVidCmdMessageAck->Label));
}


int PipeWriteToMessageFileCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                      PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE *pWriteToMessageFileCmdMessage,
                                      PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE_ACK *pWriteToMessageFileCmdMessageAck)
{
    UNUSED(pTcb);
    char ProcessName[MAX_PATH];
    char Help[MAX_PATH + 100];

    if (!GetProcessNameWithoutPath (pWriteToMessageFileCmdMessage->Pid, ProcessName)) {
        sprintf (Help, "Message from prozess %s:", ProcessName);
    } else {
        sprintf (Help, "Message from unknown prozess");
    }
    AddScriptMessage (Help);
    AddScriptMessage (pWriteToMessageFileCmdMessage->Text);
    pWriteToMessageFileCmdMessageAck->Command = PIPE_API_WRITE_TO_MSG_FILE_CMD;
    pWriteToMessageFileCmdMessageAck->StructSize = sizeof (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE_ACK);
    pWriteToMessageFileCmdMessageAck->ReturnValue = 0;

    return (int)sizeof (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE_ACK);
}

int PipeErrorPopupMessageAndWaitCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                            PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE *pWriteToMessageFileCmdMessage,
                                            PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE_ACK *pWriteToMessageFileCmdMessageAck)
{
    UNUSED(pTcb);
    pWriteToMessageFileCmdMessageAck->Command = PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD;
    pWriteToMessageFileCmdMessageAck->StructSize = sizeof (PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE_ACK);
    pWriteToMessageFileCmdMessageAck->ReturnValue = ThrowError(pWriteToMessageFileCmdMessage->Level,
                                                          pWriteToMessageFileCmdMessage->Text);

    return (int)sizeof (PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE_ACK);
}

int PipeOpenVirtualNetworkChannelCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                             PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE *pOpenVirtualNetworkChannelCmdMessage,
                                             PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK *pOpenVirtualNetworkChannelCmdMessageAck)
{
    pOpenVirtualNetworkChannelCmdMessageAck->Command = PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD;
    pOpenVirtualNetworkChannelCmdMessageAck->StructSize = sizeof (PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK);
    pOpenVirtualNetworkChannelCmdMessageAck->ReturnValue =VirtualNetworkOpen(pTcb->pid, pOpenVirtualNetworkChannelCmdMessage->Type,
                                                                             pOpenVirtualNetworkChannelCmdMessage->Channel, 64*1024);
    return (int)sizeof (PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK);
}

int PipeCloseVirtualNetworkChannelCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                              PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE *pCloseVirtualNetworkChannelCmdMessage,
                                              PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK *pCloseVirtualNetworkChannelCmdMessageAck)
{
    pCloseVirtualNetworkChannelCmdMessageAck->Command = PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD;
    pCloseVirtualNetworkChannelCmdMessageAck->StructSize = sizeof (PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK);
    pCloseVirtualNetworkChannelCmdMessageAck->ReturnValue =VirtualNetworkCloseByHandle(pTcb->pid, pCloseVirtualNetworkChannelCmdMessage->Handle);
    return (int)sizeof (PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK);
}


int PipeWriteBbVariableCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                   PIPE_API_WRITE_BBVARI_CMD_MESSAGE *pWriteBbVariCmdMessage,
                                   PIPE_API_WRITE_BBVARI_CMD_MESSAGE_ACK *pWriteBbVariCmdMessageAck)
{
    UNUSED(pTcb);
    write_bbvari_union_pid (pWriteBbVariCmdMessage->Pid, pWriteBbVariCmdMessage->Vid, pWriteBbVariCmdMessage->DataType, pWriteBbVariCmdMessage->Value);

    pWriteBbVariCmdMessageAck->Command = PIPE_API_WRITE_BBVARI_CMD;
    pWriteBbVariCmdMessageAck->StructSize = sizeof (PIPE_API_WRITE_BBVARI_CMD_MESSAGE_ACK);
    pWriteBbVariCmdMessageAck->ReturnValue = 0;

    return (int)sizeof (PIPE_API_WRITE_BBVARI_CMD_MESSAGE_ACK);
}

int PipeReadBbVariableCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                  PIPE_API_READ_BBVARI_CMD_MESSAGE *pReadBbVariCmdMessage,
                                  PIPE_API_READ_BBVARI_CMD_MESSAGE_ACK *pReadBbVariCmdMessageAck)
{
    UNUSED(pTcb);
    pReadBbVariCmdMessageAck->Value = read_bbvari_union (pReadBbVariCmdMessage->Vid);

    pReadBbVariCmdMessageAck->Command = PIPE_API_WRITE_BBVARI_CMD;
    pReadBbVariCmdMessageAck->StructSize = sizeof (PIPE_API_READ_BBVARI_CMD_MESSAGE_ACK);
    pReadBbVariCmdMessageAck->ReturnValue = 0;

    return (int)sizeof (PIPE_API_READ_BBVARI_CMD_MESSAGE_ACK);
}


int PipeLoopOutAndSyncWithBarrierCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                             PIPE_API_LOOP_OUT_CMD_MESSAGE *pLoopOutCmdMessage,
                                             PIPE_API_LOOP_OUT_CMD_MESSAGE_ACK *pLoopOutCmdMessageAck)
{
    UNUSED(pTcb);
    pLoopOutCmdMessageAck->Command = PIPE_API_WRITE_BBVARI_CMD;
    pLoopOutCmdMessageAck->StructSize = sizeof (PIPE_API_READ_BBVARI_CMD_MESSAGE_ACK);
    pLoopOutCmdMessageAck->ReturnValue = 0;

    pLoopOutCmdMessageAck->SnapShotSize = SchedulerLoopOut (pLoopOutCmdMessage->BarrierBeforeName, pLoopOutCmdMessage->BarrierBehindName,
                                                            pLoopOutCmdMessage->SnapShotData, pLoopOutCmdMessage->SnapShotSize,
                                                            pLoopOutCmdMessageAck->SnapShotData);

    return (int)(sizeof (PIPE_API_LOOP_OUT_CMD_MESSAGE_ACK) - 1) + pLoopOutCmdMessageAck->SnapShotSize;
}

int PipeGetSchedulingInformationCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                                           PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE *pSchedulingInformationCmdMessage,
                                           PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE_ACK *pSchedulingInformationCmdMessageAck)
{
    UNUSED(pTcb);
    pSchedulingInformationCmdMessageAck->Command = PIPE_API_GET_SCHEDUING_INFORMATION_CMD;
    pSchedulingInformationCmdMessageAck->StructSize = sizeof (PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE_ACK);
    pSchedulingInformationCmdMessageAck->ReturnValue = GetSchedulingInformation (&pSchedulingInformationCmdMessageAck->SeparateCyclesForRefAndInitFunction,
                                                                                &pSchedulingInformationCmdMessageAck->SchedulerCycle,
                                                                                &pSchedulingInformationCmdMessageAck->SchedulerPeriod,
                                                                                &pSchedulingInformationCmdMessageAck->MainSchedulerCycleCounter,
                                                                                &pSchedulingInformationCmdMessageAck->ProcessCycle,
                                                                                &pSchedulingInformationCmdMessageAck->ProcessPeriod,
                                                                                &pSchedulingInformationCmdMessageAck->ProcessDelay);
    return (int)(sizeof (PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE_ACK));
}

int PipeLogoutCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                          PIPE_API_LOGOUT_MESSAGE *pLogoutCmdMessage,
                          PIPE_API_LOGOUT_MESSAGE_ACK *pLogoutCmdMessageAck)
{
    UNUSED(pTcb);
    pLogoutCmdMessageAck->Command = PIPE_API_WRITE_BBVARI_CMD;
    pLogoutCmdMessageAck->StructSize = sizeof (PIPE_API_LOGOUT_MESSAGE_ACK);
    pLogoutCmdMessageAck->ReturnValue =  SchedulerLogout (pLogoutCmdMessage->ImmediatelyFlag);

    if (pLogoutCmdMessage->ImmediatelyFlag) {
        return TERMINATE_EXTERN_PROCESS_IMMEDIATELY;   // This telles the caller that no more ACK will be replayed by the external process
    }
    return (int)sizeof (PIPE_API_LOGOUT_MESSAGE_ACK);
}

int DecodePipeCmdMessage (TASK_CONTROL_BLOCK *pTcb,
                          PIPE_API_BASE_CMD_MESSAGE *pReceiveMessageBuffer, DWORD Len,
                          PIPE_API_BASE_CMD_MESSAGE_ACK *pTransmitMessageBuffer)
{
    UNUSED(Len);
    int ResponseLen;

    switch (pReceiveMessageBuffer->Command) {
    case PIPE_API_ADD_BBVARI_CMD:
        ResponseLen = PipeAddBbvariCmdMessage (pTcb,
                                               (PIPE_API_ADD_BBVARI_CMD_MESSAGE *)pReceiveMessageBuffer,
                                               (PIPE_API_ADD_BBVARI_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_ATTACH_BBVARI_CMD:
        ResponseLen = PipeAttachBbvariCmdMessage (pTcb,
                                                  (PIPE_API_ATTACH_BBVARI_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                  (PIPE_API_ATTACH_BBVARI_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_REMOVE_BBVARI_CMD:
        ResponseLen = PipeRemoveBbvariCmdMessage (pTcb,
                                                  (PIPE_API_REMOVE_BBVARI_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                  (PIPE_API_REMOVE_BBVARI_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_GET_LABEL_BY_ADDRESS_CMD:
        ResponseLen = PipeGetLabelnameByAddressCmdMessage (pTcb,
                                                           (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                           (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD:
        ResponseLen = PipeGetReferencedLabelnameByVidCmdMessage (pTcb,
                                                                 (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                                 (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_WRITE_TO_MSG_FILE_CMD:
        ResponseLen = PipeWriteToMessageFileCmdMessage (pTcb,
                                                        (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                        (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD:
            ResponseLen = PipeErrorPopupMessageAndWaitCmdMessage (pTcb,
                                                                  (PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                                  (PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;

    case PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD:
        ResponseLen = PipeOpenVirtualNetworkChannelCmdMessage (pTcb,
                                                               (PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                               (PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;

    case PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD:
        ResponseLen = PipeCloseVirtualNetworkChannelCmdMessage (pTcb,
                                                                (PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                                (PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;

    case PIPE_API_WRITE_BBVARI_CMD:
        ResponseLen = PipeWriteBbVariableCmdMessage (pTcb,
                                                     (PIPE_API_WRITE_BBVARI_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                     (PIPE_API_WRITE_BBVARI_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_READ_BBVARI_CMD:
        ResponseLen = PipeReadBbVariableCmdMessage (pTcb,
                                                    (PIPE_API_READ_BBVARI_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                    (PIPE_API_READ_BBVARI_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_LOOP_OUT_CMD:
        ResponseLen = PipeLoopOutAndSyncWithBarrierCmdMessage (pTcb,
                                                               (PIPE_API_LOOP_OUT_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                               (PIPE_API_LOOP_OUT_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_GET_SCHEDUING_INFORMATION_CMD:
        ResponseLen = PipeGetSchedulingInformationCmdMessage (pTcb,
                                                             (PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE *)pReceiveMessageBuffer,
                                                             (PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE_ACK *)pTransmitMessageBuffer);
        break;
    case PIPE_API_LOGOUT_CMD:
        ResponseLen = PipeLogoutCmdMessage (pTcb,
                                            (PIPE_API_LOGOUT_MESSAGE *)(void*)pReceiveMessageBuffer,
                                            (PIPE_API_LOGOUT_MESSAGE_ACK *)(void*)pTransmitMessageBuffer);
        break;
        //...
    default:
        ResponseLen = 0;  // Unknown message (do nothing)
        break;
    }
    return ResponseLen;
}


void ClosePipeToExternProcess (TASK_CONTROL_BLOCK *pTcb)
{
    pTcb->CloseConnectionToExternProcess(pTcb->hPipe);
}


void SendKillExternalProcessMessage (TASK_CONTROL_BLOCK *pTcb)
{
    DWORD Success, BytesWritten;
    PIPE_API_KILL_WITH_NO_RESPONSE_MESSAGE Message;

    Message.Command = PIPE_API_KILL_WITH_NO_RESPONSE_CMD;
    Message.StructSize = sizeof (Message);
    if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid,& Message);
    Success = pTcb->WriteToConnection (pTcb->hPipe,
                                         (void*)&Message,             // message
                                         sizeof (Message),            // message length
                                         &BytesWritten);              // bytes written
    if (!Success || (BytesWritten != sizeof (Message))) {
        //ThrowError (1, "cannot send kill command to %s", pTcb->name);
    }
}




int CallExternProcessFunction (int Function, uint64_t Cycle, TASK_CONTROL_BLOCK *pTcb,
                               PIPE_API_BASE_CMD_MESSAGE *pTransmitMessageBuffer, int par_SnapShotSize, int par_Size,
                               PIPE_API_BASE_CMD_MESSAGE_ACK *pReceiveMessageBuffer)
{
    DWORD Success, BytesWritten, BytesRead;

    pTransmitMessageBuffer->Command = Function;
    pTransmitMessageBuffer->StructSize = par_Size;
    ((PIPE_API_CALL_FUNC_CMD_MESSAGE*)pTransmitMessageBuffer)->Cycle = Cycle;
    ((PIPE_API_CALL_FUNC_CMD_MESSAGE*)pTransmitMessageBuffer)->SnapShotSize = par_SnapShotSize;

    if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, pTransmitMessageBuffer);
    Success = pTcb->WriteToConnection (pTcb->hPipe,
                                       (void*)pTransmitMessageBuffer,
                                       par_Size,
                                       &BytesWritten);
    if (Success && (BytesWritten == (size_t)par_Size)) {
        DWORD ReadPipePart;
        BytesRead = 0;
        for (;;) {
            pTcb->wait_on = 1;
            Success = pTcb->ReadFromConnection (pTcb->hPipe, (void*)((char*)pReceiveMessageBuffer + BytesRead), PIPE_MESSAGE_BUFSIZE - BytesRead, &ReadPipePart);
            if (Success) {
                BytesRead += ReadPipePart;
            }
            if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 0, pTcb->pid, pReceiveMessageBuffer);
            pTcb->wait_on = 0;
            if (Success && (BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE_ACK))
                        && (BytesRead >= (DWORD)pReceiveMessageBuffer->StructSize)) {
                if (pReceiveMessageBuffer->Command == Function) {
                    return 0;
                } else {
                    // external process transmit a request and not the acknowledge for the  cyclic function
                    int ResponseLen;
                    ResponseLen = DecodePipeCmdMessage (pTcb, (PIPE_API_BASE_CMD_MESSAGE*)pReceiveMessageBuffer, BytesRead,
                                                        (PIPE_API_BASE_CMD_MESSAGE_ACK*)pTransmitMessageBuffer);
                    if (ResponseLen > 0) {
                        if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, pTransmitMessageBuffer);
                        Success = pTcb->WriteToConnection (pTcb->hPipe,                     // pipe handle
                                                           (void*)pTransmitMessageBuffer,   // message
                                                           ResponseLen,                     // message length
                                                           &BytesWritten);                  // bytes written
                        if (!Success) {
#ifdef _WIN32
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

                            ThrowError (1, "WriteFile to pipe failed to send acknowledge to \"%s\" for command request %i \"%s\"",
                                   pTcb->name, ((PIPE_API_BASE_CMD_MESSAGE*)pReceiveMessageBuffer)->Command, lpMsgBuf);
                            LocalFree (lpMsgBuf);
#else
                            ThrowError (1, "WriteFile to pipe failed to send acknowledge to \"%s\" for command request %i \"%i\"",
                                   pTcb->name, ((PIPE_API_BASE_CMD_MESSAGE*)pReceiveMessageBuffer)->Command, errno);
#endif
                            break;
                        }
                    } else if (ResponseLen == TERMINATE_EXTERN_PROCESS_IMMEDIATELY) {
                        if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, pTransmitMessageBuffer);
                        Success = pTcb->WriteToConnection  (pTcb->hPipe,
                                                              (void*)pTransmitMessageBuffer,
                                                              sizeof (PIPE_API_LOGOUT_MESSAGE_ACK),
                                                              &BytesWritten);
                        return -2;  // -2 -> Do not send aK kill signal
                    } else {
                        ThrowError (1, "not expected message command=%i in function %i call of process (%i), \"%s\" %i", pReceiveMessageBuffer->Command, Function, pTcb->pid, pTcb->name, __LINE__);
                        break;
                    }
                }
                BytesRead = 0;
            } else {
                if (!Success) {
                    ThrowError (1, "extern process %s close himself (maybe he call exit() or abort() function or he is killed by an exception or stopped from a debugger)", pTcb->name);
                    break;
                }
            }
        }
    } else {
        // Cannot transmit message to extern process
#ifdef _WIN32
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

        ThrowError (1, "WriteFile to pipe failed to send cyclic function request to \"%s\"  \"%s\"", pTcb->name, lpMsgBuf);
        LocalFree (lpMsgBuf);
#else
        ThrowError (1, "Write to pipe failed to send cyclic function request to \"%s\"  \"%i\"", pTcb->name, errno);
#endif
    }
    return -1;
}

static int OtherCmd (TASK_CONTROL_BLOCK *pTcb, PIPE_API_BASE_CMD_MESSAGE *Buffer, DWORD BytesRead)
{
    // The external process send something else and not the acknowledge of the cyclic function
    int ResponseLen;
    DWORD Success, BytesWritten;
    PIPE_API_BASE_CMD_MESSAGE_ACK *Buffer2;
    int Ret = 0;

    Buffer2 = GetFreeMessageBuffer(NULL);
    ResponseLen = DecodePipeCmdMessage (pTcb, Buffer, BytesRead, Buffer2);
    if (ResponseLen > 0) {
        if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, Buffer2);
        Success = pTcb->WriteToConnection (pTcb->hPipe,
                                             (void*)Buffer2,
                                             ResponseLen,
                                             &BytesWritten);
        if (!Success) {
#ifdef _WIN32
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

            ThrowError (1, "WriteFile to pipe failed \"%s\" inside scm_ref_vari().1 \"%s\"", lpMsgBuf, pTcb->name);
            LocalFree (lpMsgBuf);
#else
            ThrowError (1, "Write to pipe failed \"%i\" inside scm_ref_vari().1 \"%s\"", errno, pTcb->name);
#endif
            Ret = -1;
        }
    } else {
        ThrowError (1, "not expected message command=%i in function %i call of process (%i) \"%s\" %i", Buffer->Command, PIPE_API_READ_MEMORY_CMD, pTcb->pid, pTcb->name, __LINE__);
        Ret = -1;
    }
    GetFreeMessageBuffer(Buffer2);
    return Ret;
}

int scm_read_bytes (uint64_t address, PID pid, char *dest, int len)
{
    SCHEDULER_DATA *pSchedulerData;
    TASK_CONTROL_BLOCK *pTcb;
    PIPE_API_BASE_CMD_MESSAGE *Buffer;
    int Ret = 0;

    if (len > MAX_MEMORY_BLOCK_SIZE) return -1;

    // Is it a realtime process inside the remote master?
    if ((s_main_ini_val.ConnectToRemoteMaster) && ((pid & 0x10000000) == 0x10000000)) {
        return rm_ReadBytes (pid, address, len, dest);
    }

    pTcb = GetPointerToTaskControlBlock (pid);
    if (pTcb == NULL) {
        ThrowError (1, "cannot read from external process %i, it is not running", pid);
        return -1;
    }
#ifdef USE_DEBUG_API    
    if (pTcb->UseDebugInterfaceForRead == 0) {
        CheckDebugInterfaceForReadWrite (pTcb);
    }
    if (pTcb->UseDebugInterfaceForRead == 1) {
        int Ret;
        if (ReadProcessMemory (pTcb->ProcHandle, (void*)address, dest, len, &Ret)) {
            return Ret;   // Daten wurden erfolgreich ueber das Debug-Interface gelesen
        } else {
            // Kann nicht ueber das Debug-Interface lesen, deshalb ab jetzt ueber Messages lesen!
            pTcb->UseDebugInterfaceForRead = -1;
        }
    } 
#endif
    if ((pSchedulerData = GetSchedulerProcessIsRunning (pid)) != NULL) {
        DWORD Success, BytesWritten, BytesRead;

        PIPE_API_READ_MEMORY_CMD_MESSAGE ReadMemoryCmdMessage;

        if (pTcb->WorkingFlag) {
            ThrowError (1, "cannot read from process because process is running");
            return -1;
        }

        ReadMemoryCmdMessage.Command = PIPE_API_READ_MEMORY_CMD;
        ReadMemoryCmdMessage.StructSize = sizeof (ReadMemoryCmdMessage);
        ReadMemoryCmdMessage.Address = address;
        ReadMemoryCmdMessage.Size = (uint32_t)len;
        if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, &ReadMemoryCmdMessage);
        Success = pTcb->WriteToConnection (pTcb->hPipe,
                                           (void*)&ReadMemoryCmdMessage,
                                           sizeof (ReadMemoryCmdMessage),
                                           &BytesWritten);
        if (Success && (BytesWritten == sizeof (ReadMemoryCmdMessage))) {
            Buffer = GetFreeMessageBuffer(NULL);
            for (;;) {
                Success = pTcb->ReadFromConnection (pTcb->hPipe, (void*)Buffer, PIPE_MESSAGE_BUFSIZE, &BytesRead);
                if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 0, pTcb->pid, Buffer);
                if (Success && (BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE_ACK))) {
                    if (Buffer->Command == PIPE_API_READ_MEMORY_CMD) {
                        int Size = (int)((PIPE_API_READ_MEMORY_CMD_MESSAGE_ACK*)(Buffer))->Size;
                        if (Size > len) {
                            ThrowError (1, "more bytes returned (%i) than requested (%i)!", Size, len);
                            Ret = -1;
                            break;  // for(;;)
                        }
                        MEMCPY (dest, 
                                ((PIPE_API_READ_MEMORY_CMD_MESSAGE_ACK*)(Buffer))->Memory,
                                (size_t)Size);
                        Ret = Size;
                        break;  // for(;;)
                    } else {
                        // External process send something else and not the acknowledge to the PIPE_API_READ_MEMORY_CMD command
                        if (OtherCmd (pTcb, Buffer, BytesRead)) {
                            break;  // for(;;)
                        }
                    }
                } else {
                    ThrowError (1, "extern process %s close himself (maybe he call exit() or abort() function or he is killed by an exception or stopped from a debugger)", pTcb->name);
                    break;  // for(;;)
                }
            }
            GetFreeMessageBuffer(Buffer);
        } else {
            // Cannot transmit message to extern process
#ifdef _WIN32
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

            ThrowError (1, "WriteFile to pipe failed \"%s\" inside scm_read_bytes().2 \"%s\"", lpMsgBuf, pTcb->name);
            LocalFree (lpMsgBuf);
#else
            ThrowError (1, "Write to pipe failed \"%i\" inside scm_read_bytes().2 \"%s\"", errno, pTcb->name);
#endif
        }
    }
    return Ret;
}

int scm_write_bytes (uint64_t address, PID pid, unsigned char *src, int len)
{
    SCHEDULER_DATA *pSchedulerData;
    TASK_CONTROL_BLOCK *pTcb;
    PIPE_API_WRITE_MEMORY_CMD_MESSAGE *WriteMemoryCmdMessage;
    PIPE_API_BASE_CMD_MESSAGE *Buffer;
    int Ret = 0;

    if (len > MAX_MEMORY_BLOCK_SIZE) return -1;

    // Is it a realtime process inside the remote master?
    if ((s_main_ini_val.ConnectToRemoteMaster) && ((pid & 0x10000000) == 0x10000000)) {
        return rm_WriteBytes (pid, address, len, src);
    }

    pTcb = GetPointerToTaskControlBlock (pid);
    if (pTcb == NULL) return -1;
#ifdef USE_DEBUG_API    
    if (pTcb->UseDebugInterfaceForWrite == 0) {
        CheckDebugInterfaceForReadWrite (pTcb);
    }
    if (pTcb->UseDebugInterfaceForWrite == 1) {
        int Ret;
        if (WriteProcessMemory (pTcb->ProcHandle, (void*)address, src, len, &Ret)) {
            return Ret;   // Daten wurden erfolgreich ueber das Debug-Interface geschrieben
        } else {
            // Kann nicht ueber das Debug-Interface schreiben, deshalb ab jetzt ueber Messages lesen!
            pTcb->UseDebugInterfaceForWrite = -1;
        }
    }
#endif
    if ((pSchedulerData = GetSchedulerProcessIsRunning (pid)) != NULL) {
        DWORD Success, BytesWritten, BytesRead;

        if (pTcb->WorkingFlag) {
            ThrowError (1, "cannot write to process because process is running");
            return -1;
        }
        WriteMemoryCmdMessage = (PIPE_API_WRITE_MEMORY_CMD_MESSAGE*)_alloca (sizeof (PIPE_API_WRITE_MEMORY_CMD_MESSAGE) - 1 + (size_t)len);

        WriteMemoryCmdMessage->Command = PIPE_API_WRITE_MEMORY_CMD;
        WriteMemoryCmdMessage->StructSize = (int)sizeof (PIPE_API_WRITE_MEMORY_CMD_MESSAGE) + len - 1;
        WriteMemoryCmdMessage->Address = address;
        WriteMemoryCmdMessage->Size = (unsigned long)len;
        MEMCPY (WriteMemoryCmdMessage->Memory, src, (size_t)len);
        if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, WriteMemoryCmdMessage);
        Success = pTcb->WriteToConnection (pTcb->hPipe,
                                             (void*)WriteMemoryCmdMessage,
                                             (int)sizeof (PIPE_API_WRITE_MEMORY_CMD_MESSAGE) + len - 1,
                                             &BytesWritten);
        if (Success && (BytesWritten == ((DWORD)sizeof (PIPE_API_WRITE_MEMORY_CMD_MESSAGE) + (DWORD)len - 1))) {
            Buffer = GetFreeMessageBuffer(NULL);
            for (;;) {
                Success = pTcb->ReadFromConnection (pTcb->hPipe, (void*)Buffer, PIPE_MESSAGE_BUFSIZE, &BytesRead);
                if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 0, pTcb->pid, Buffer);
                if (Success && (BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE_ACK))) {
                    if (Buffer->Command == PIPE_API_WRITE_MEMORY_CMD) {
                        Ret = (int)(((PIPE_API_WRITE_MEMORY_CMD_MESSAGE_ACK*)(Buffer))->Size);
                        break;   // for(;;)
                    } else {
                        // External process send something else and not the acknowledge to the des PIPE_API_WRITE_MEMORY_CMD command
                        if (OtherCmd (pTcb, Buffer, BytesRead)) {
                            break;   // for(;;)
                        }
                    }
                } else {
                    ThrowError (1, "extern process %s close himself (maybe he call exit() or abort() function or he is killed by an exception or stopped from a debugger)", pTcb->name);
                    break;   // for(;;)
                }
            }
            GetFreeMessageBuffer(Buffer);
        } else {
            // Cannot transmit message to extern process
#ifdef _WIN32
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

            ThrowError (1, "WriteFile to pipe failed \"%s\" inside scm_write_bytes().2 \"%s\"", lpMsgBuf, pTcb->name);
            LocalFree (lpMsgBuf);
#else
            ThrowError (1, "Write to pipe failed \"%i\" inside scm_write_bytes().2 \"%s\"", errno, pTcb->name);
#endif
        }
    }
    return Ret;
}


int scm_write_bytes_pid_array (uint64_t address, PID *pid_array, int size, unsigned char *src, int len)
{
    int x;
    int Ret = 0;
    for (x = 0; x < size; x++) {
        Ret = scm_write_bytes(address, pid_array[x], src, len);
        if (Ret <= 0) {
            break;
        }
    }
    return Ret;
}

int scm_ref_vari_lock_flag(uint64_t address, PID pid, const char *name, int type, int dir, int lock_flag)
{
    TASK_CONTROL_BLOCK *pTcb;
    PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE *ReferenceVariableCmdMessage;
    PIPE_API_BASE_CMD_MESSAGE *Buffer;
    int Ret = -1;

    // Check if it is a base data type(only this can be added into the blackboard
    if (((type < BB_BYTE) || (type > BB_DOUBLE)) &&
        (type != BB_QWORD) && (type != BB_UQWORD)) {
        return -1;
    }

    // If it is a process from the remote master
    if ((s_main_ini_val.ConnectToRemoteMaster) && ((pid & 0x10000000) == 0x10000000)) {
        return rm_ReferenceVariable (pid, address, name, type, dir);
    }

    if (!lock_flag || (WaitUntilProcessIsNotActiveAndThanLockIt (pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "read from external process memory", __FILE__, __LINE__) == 0)) {
        pTcb = GetPointerToTaskControlBlock (pid);

        if (pTcb != NULL) {
            DWORD Success, BytesWritten, BytesRead;

            if (pTcb->WorkingFlag) {
                if (lock_flag) UnLockProcess (pid);
                ThrowError (1, "cannot reference the variable \"%s\" because process \"%s\" is running", pTcb->name);
                return -1;
            }
            ReferenceVariableCmdMessage = (PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE*)_alloca (sizeof (PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE) + strlen (name));

            ReferenceVariableCmdMessage->Command = PIPE_API_REFERENCE_VARIABLE_CMD;
            ReferenceVariableCmdMessage->StructSize = (int)(sizeof (PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE) + strlen (name));
            ReferenceVariableCmdMessage->Address = address;
            ReferenceVariableCmdMessage->Type = type;
            ReferenceVariableCmdMessage->Dir = dir |
                                               PIPE_API_REFERENCE_VARIABLE_DIR_IGNORE_REF_FILTER;  // do not filter manual referenced signals
            strcpy (ReferenceVariableCmdMessage->Name, name);
            if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, ReferenceVariableCmdMessage);
            Success = pTcb->WriteToConnection (pTcb->hPipe,
                                                 (void*)ReferenceVariableCmdMessage,
                                                 ReferenceVariableCmdMessage->StructSize,
                                                 &BytesWritten);
            if (Success && (BytesWritten == (DWORD)ReferenceVariableCmdMessage->StructSize)) {
                Buffer = GetFreeMessageBuffer(NULL);
                for (;;) {
                    Success = pTcb->ReadFromConnection (pTcb->hPipe, (void*)Buffer, PIPE_MESSAGE_BUFSIZE, &BytesRead);
                    if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile,__LINE__, 0, pTcb->pid, Buffer);
                    if (Success) {
                        if ((BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE_ACK)) &&
                            (Buffer->Command == PIPE_API_REFERENCE_VARIABLE_CMD)) {
                            Ret = ((PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE_ACK*)(Buffer))->ReturnValue;
                            break;   // for(;;)
                        } else {
                            // External process send something else and not the acknowledge to the PIPE_API_REFERENCE_VARIABLE_CMD command
                            if (OtherCmd (pTcb, Buffer, BytesRead)) {
                                break;   // for(;;)
                            }
                        }
                    } else {
                        ThrowError (1, "extern process %s close himself (maybe he call exit() or abort() function or he is killed by an exception or stopped from a debugger)", pTcb->name);
                        break;   // for(;;)
                    }
                }
                GetFreeMessageBuffer(Buffer);
            } else {
                // Cannot transmit message to extern process
#ifdef _WIN32
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

                ThrowError (1, "WriteFile to pipe failed \"%s\" inside scm_ref_vari().2 \"%s\"", lpMsgBuf, pTcb->name);
                LocalFree (lpMsgBuf);
#else
                ThrowError (1, "Write to pipe failed \"%i\" inside scm_ref_vari().2 \"%s\"", errno, pTcb->name);
#endif
            }
        }
        if (lock_flag) UnLockProcess (pid);
    }
    return Ret;
}

int scm_ref_vari(uint64_t address, PID pid, char *name, int type, int dir)
{
    return scm_ref_vari_lock_flag (address, pid, name, type, dir, 1);
}

int scm_unref_vari_lock_flag(uint64_t address, PID pid, char *name, int type, int lock_flag)
{
    TASK_CONTROL_BLOCK *pTcb;
    PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE *DeReferenceVariableCmdMessage;
    PIPE_API_BASE_CMD_MESSAGE *Buffer;
    int Ret = -1;

    // If it is a process from the remote master
    if ((s_main_ini_val.ConnectToRemoteMaster) && ((pid & 0x10000000) == 0x10000000)) {
        return rm_DeReferenceVariable (pid, address, name, type);
    }

    if (!lock_flag || (WaitUntilProcessIsNotActiveAndThanLockIt (pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "read from external process memory", __FILE__, __LINE__) == 0)) {
        pTcb = GetPointerToTaskControlBlock (pid);

        if (pTcb != NULL) {
            DWORD Success, BytesWritten, BytesRead;
            char NameWithPrefix[BBVARI_NAME_SIZE];
            int LenName;
            int LenPrefix;

            if (pTcb->WorkingFlag) {
                if (lock_flag) UnLockProcess (pid);
                ThrowError (1, "cannot unrefeference variable \"%s\" to process because process \"%s\" is running", name, pTcb->name);
                return -1;
            }
            DeReferenceVariableCmdMessage = (PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE*)_alloca (sizeof (PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE) + strlen (name));
            DeReferenceVariableCmdMessage->Command = PIPE_API_DEREFERENCE_VARIABLE_CMD;
            DeReferenceVariableCmdMessage->StructSize = (int)(sizeof (PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE) + strlen (name));
            DeReferenceVariableCmdMessage->Address = address;
            DeReferenceVariableCmdMessage->Type = type;
            if (GetBlackboarPrefixForProcess (pid, NameWithPrefix, sizeof(NameWithPrefix))) {
                NameWithPrefix[0] = 0;  // No prefix if an error occurs
            }
            LenPrefix = (int)strlen (NameWithPrefix);
            LenName = (int)strlen (name);
            if ((LenPrefix + LenName) >= BBVARI_NAME_SIZE) LenName = BBVARI_NAME_SIZE - LenPrefix - 1;
            MEMCPY (NameWithPrefix + LenPrefix, name, (size_t)LenName);
            NameWithPrefix[LenPrefix + LenName] = 0;
            DeReferenceVariableCmdMessage->Vid = get_bbvarivid_by_name (NameWithPrefix);
            strcpy (DeReferenceVariableCmdMessage->Name, name);
            if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, DeReferenceVariableCmdMessage);
            Success = pTcb->WriteToConnection (pTcb->hPipe,
                                               (void*)DeReferenceVariableCmdMessage,         // message
                                               DeReferenceVariableCmdMessage->StructSize,    // message length
                                               &BytesWritten);                               // bytes written
            if (Success && (BytesWritten == (DWORD)DeReferenceVariableCmdMessage->StructSize)) {
                Buffer = GetFreeMessageBuffer(NULL);
                for (;;) {
                    Success = pTcb->ReadFromConnection (pTcb->hPipe, (void*)Buffer, PIPE_MESSAGE_BUFSIZE, &BytesRead);
                    if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 0, pTcb->pid, Buffer);
                    if (Success && (BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE_ACK))) {
                        if (Buffer->Command == PIPE_API_DEREFERENCE_VARIABLE_CMD) {
                            Ret = ((PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE_ACK*)(Buffer))->ReturnValue;
                            break;  // for(;;)
                        } else {
                            // External process send something else and not the acknowledge to the PIPE_API_DEREFERENCE_VARIABLE_CMD command
                            if (OtherCmd (pTcb, Buffer, BytesRead)) {
                                break;   // for(;;)
                            }
                        }
                    } else {
                        ThrowError (1, "extern process %s close himself (maybe he call exit() or abort() function or he is killed by an exception or stopped from a debugger)", pTcb->name);
                        break;   // for(;;)
                    }
                }
                GetFreeMessageBuffer(Buffer);
            } else {
                // Cannot transmit message to extern process
#ifdef _WIN32
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

                ThrowError (1, "WriteFile to pipe failed \"%s\" inside scm_unref_vari().2 \"%s\"", lpMsgBuf, pTcb->name);
                LocalFree (lpMsgBuf);
#else
                ThrowError (1, "Write to pipe failed \"%i\" inside scm_unref_vari().2 \"%s\"", errno, pTcb->name);
#endif
            }
        }
        if (lock_flag) UnLockProcess (pid);
    }
    return Ret;
}

int scm_unref_vari (uint64_t address, PID pid, char *name, int type)
{
    return scm_unref_vari_lock_flag (address, pid, name, type, 1);
}

int scm_write_section_to_exe (PID pid, char *Section)
{
    TASK_CONTROL_BLOCK *pTcb;
    PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE *WriteSectionBackToExeCmdMessage;
    PIPE_API_BASE_CMD_MESSAGE *Buffer;
    int Size;
    int Ret = -1;

    if (WaitUntilProcessIsNotActiveAndThanLockIt (pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "read from external process memory", __FILE__, __LINE__) == 0) {
        pTcb = GetPointerToTaskControlBlock (pid);

        if (pTcb != NULL) {
            DWORD Success, BytesWritten, BytesRead;

            if (pTcb->WorkingFlag) {
                UnLockProcess (pid);
                ThrowError (1, "cannot write section \"%s\" to process because process \"%s\" is running", Section, pTcb->name);
                return -1;
            }
            Size = (int)(sizeof (PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE) + strlen (Section));
            WriteSectionBackToExeCmdMessage = (PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE*)_alloca ((size_t)Size);

            WriteSectionBackToExeCmdMessage->Command = PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD;
            WriteSectionBackToExeCmdMessage->StructSize = Size;
            strcpy (WriteSectionBackToExeCmdMessage->SectionName, Section);
            if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, WriteSectionBackToExeCmdMessage);
            Success = pTcb->WriteToConnection (pTcb->hPipe,
                                                 (void*)WriteSectionBackToExeCmdMessage,
                                                 WriteSectionBackToExeCmdMessage->StructSize,
                                                 &BytesWritten);
            if (Success && (BytesWritten == (DWORD)Size)) {
                Buffer = GetFreeMessageBuffer(NULL);
                for (;;) {
                    Success = pTcb->ReadFromConnection (pTcb->hPipe, (void*)Buffer, PIPE_MESSAGE_BUFSIZE, &BytesRead);
                    if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 0, pTcb->pid, Buffer);
                    if (Success && (BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE_ACK))) {
                        if (Buffer->Command == PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD) {
                            Ret = !((PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE_ACK*)(Buffer))->ReturnValue;
                            break;  // for(;;)
                        } else {
                            // External process send something else and not the acknowledge to the PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD command
                            if (OtherCmd (pTcb, Buffer, BytesRead)) {
                                break;   // for(;;)
                            }
                        }
                    } else {
                        ThrowError (1, "extern process %s close himself (maybe he call exit() or abort() function or he is killed by an exception or stopped from a debugger)", pTcb->name);
                        break;   // for(;;)
                    }
                }
                GetFreeMessageBuffer(Buffer);
            } else {
                // Cannot transmit message to extern process
#ifdef _WIN32
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

                ThrowError (1, "WriteFile to pipe failed \"%s\" inside scm_write_section_to_exe() \"%s\"", lpMsgBuf, pTcb->name);
                LocalFree (lpMsgBuf);
#else
                ThrowError (1, "Write to pipe failed \"%i\" inside scm_write_section_to_exe() \"%s\"", errno, pTcb->name);
#endif
            }
        }
        UnLockProcess (pid);
    }
    return Ret;
}

int dereference_all_blackboard_variables (int pid)
{
    TASK_CONTROL_BLOCK *pTcb;
    PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE DereferenceAllCmdMessage;
    PIPE_API_BASE_CMD_MESSAGE *Buffer;
    int Ret = -1;

    pTcb = GetPointerToTaskControlBlock (pid);

    if (pTcb != NULL) {
        DWORD Success, BytesWritten, BytesRead;

        if (pTcb->WorkingFlag) {
            ThrowError (1, "cannot dereference all variable of process because process \"%s\" is running", pTcb->name);
            return -1;
        }

        // Delete the copy list(s) before, this will speed up execution
        RemoveAllVariableFromCopyLists(pid);

        DereferenceAllCmdMessage.Command = PIPE_API_DEREFERENCE_ALL_CMD;
        DereferenceAllCmdMessage.StructSize = sizeof (PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE);
        if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 1, pTcb->pid, &DereferenceAllCmdMessage);
        Success = pTcb->WriteToConnection (pTcb->hPipe,
                                           (void*)&DereferenceAllCmdMessage,       // message
                                           DereferenceAllCmdMessage.StructSize,    // message length
                                           &BytesWritten);                         // bytes written
        if (Success && (BytesWritten == sizeof (PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE))) {
            Buffer = GetFreeMessageBuffer(NULL);
            for (;;) {
                Success = pTcb->ReadFromConnection (pTcb->hPipe, (void*)Buffer, PIPE_MESSAGE_BUFSIZE, &BytesRead);
                if (pTcb->ConnectionLoggingFile) MessageLoggingToFile (pTcb->ConnectionLoggingFile, __LINE__, 0, pTcb->pid, Buffer);
                if (Success && (BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE_ACK))) {
                    if (Buffer->Command == PIPE_API_DEREFERENCE_ALL_CMD) {
                        Ret = ((PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE_ACK*)(Buffer))->ReturnValue;
                        break;   // for(;;)
                    } else {
                        // External process send something else and not the acknowledge to the PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD command
                        if (OtherCmd (pTcb, Buffer, BytesRead)) {
                            break;   // for(;;)
                        }
                    }
                } else {
                    ThrowError (1, "extern process %s close himself (maybe he call exit() or abort() function or he is killed by an exception or stopped from a debugger)", pTcb->name);
                    break; // for(;;)
                }
            }
            GetFreeMessageBuffer (Buffer);
        } else {
            // Cannot transmit message to extern process
#ifdef _WIN32
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

            ThrowError (1, "WriteFile to pipe failed \"%s\" inside dereference_all() \"%s\"", lpMsgBuf, pTcb->name);
            LocalFree (lpMsgBuf);
#else
            ThrowError (1, "Write to pipe failed \"%i\" inside dereference_all() \"%s\"", errno, pTcb->name);
#endif
        }
    }
    return Ret;
}

int CheckExternProcessComunicationProtocolVersion (PIPE_API_LOGIN_MESSAGE *LoginMessage)
{
    if (LoginMessage->Version == EXTERN_PROCESS_COMUNICATION_PROTOCOL_VERSION) {   // the current one
        return 1;
    } else {
        ThrowError (1, "process \"%s\" have wrong version of communication library (%i) XilEnv (%i) login ignored",
               LoginMessage->ProcessName, LoginMessage->Version, EXTERN_PROCESS_COMUNICATION_PROTOCOL_VERSION);
        return 0;
    }
}

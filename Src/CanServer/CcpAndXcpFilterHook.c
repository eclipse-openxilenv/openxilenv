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


#include "Config.h"
//#include "sftint_.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Message.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ReadCanCfg.h"
#include "TimeStamp.h"
#include "CanFifo.h"
#include "CcpMessages.h"
#include "CcpAndXcpFilterHook.h"


#define UNUSED(x) (void)(x)

int GlobalXcpCcpActiveFlag;   // If a XCP or a CCP connection is active this flag must be set

typedef struct {
    CCP_CONFIG CppConfig;
    int CcpActive;
    int CcpDtoTimeoutActive;
    uint64_t Timeout;
    uint64_t TimeStampTimeout;
    uint64_t ReceiveFlags;
    uint64_t ReceiveFlagsMask;
    int AllVariableReceivedOneTimeFlag;
    CCP_VARIABLES_CONFIG *CppVariConf;
} XCP_CONNECTION;

#define MAX_CONNECTIONS   8

static XCP_CONNECTION XcpConnections[MAX_CONNECTIONS];
// 0..3 XCP connections
// 4..7 CCP connections
static int ConNoList[MAX_CONNECTIONS + 1] = { -1,-1,-1,-1,-1,-1,-1,-1,-1};    // last element is always -1 (end of list)


static int AddConToList (int ConNo)
{
    int x;
    for (x = 0; x < MAX_CONNECTIONS; x++) {
        if (ConNoList[x] == ConNo) {
            return -1;
        } else if (ConNoList[x] == -1) {
            ConNoList[x] = ConNo;
            return x;
        }
    }
    return -1;
}

#ifdef DEBUG_XCP_PRINTS
static void DebugPrintConList (void)
{
    int x;

    for (x = 0; x < MAX_CONNECTIONS; x++) {
        ThrowError (1, "ConNoList[%i] = %i", x, ConNoList[x]);
    }
}
#endif

static int RemoveConFromList (int ConNo)
{
    int x, y;

#ifdef DEBUG_XCP_PRINTS
    ThrowError(1, "before remove connection %i", ConNo);
    DebugPrintConList();
#endif
    for (x = y = 0; x < MAX_CONNECTIONS + 1; x++) {
        ConNoList[y] = ConNoList[x];
        if (ConNoList[x] != ConNo) {
            y++;
        }
    }
#ifdef DEBUG_XCP_PRINTS
    ThrowError(1, "after removed connection %i", ConNo);
    DebugPrintConList();
#endif
    return (x == y) ? -1 : 0;
}

#define GET_CON_NO(Pos)  ConNoList[Pos]



static int AddOrRemoveBBVaris (int AddOrRemoveFlag, XCP_CONNECTION *pCon)
{
    CCP_DTO_PACKAGE *Dto;
    int v, DTOIdx;

    for (DTOIdx = 0; DTOIdx < pCon->CppVariConf->DTOPackagesCount; DTOIdx++) {
        Dto = &(pCon->CppVariConf->DTO_Packages[DTOIdx]);
        for (v = 0; v < Dto->VarCount; v++) {
            //ThrowError(1, "AddOrRemoveFlag=%i, Vid=%i", AddOrRemoveFlag, Dto->Entrys[v].Vid);
            if (AddOrRemoveFlag) attach_bbvari (Dto->Entrys[v].Vid);
            else remove_bbvari (Dto->Entrys[v].Vid);
        }
    }
    return 0;
}


int CcpMessageFilter (MESSAGE_HEAD *mhead, NEW_CAN_SERVER_CONFIG *csc)
{
    static int ActiveConnectionNo;
    MESSAGE_HEAD mhdummy;
    int ConnectionNo;
    int ret = 0;

    switch (mhead->mid) {
    case CCP_SET_ACTIVE_CON_NO:
        read_message (&mhdummy, (char*)&ConnectionNo, sizeof (int));
        if ((ConnectionNo >= 0) && (ConnectionNo < MAX_CONNECTIONS)) {
            ActiveConnectionNo = ConnectionNo;
        }
        ret = 1;
        break;
    case CCP_AKTIVATE:
        read_message (&mhdummy, (char*)&ConnectionNo, sizeof (int));
        if ((ConnectionNo >= 0) && (ConnectionNo < MAX_CONNECTIONS)) {
            AddConToList (ConnectionNo);
            XcpConnections[ConnectionNo].CcpActive = 1;
            GlobalXcpCcpActiveFlag = 1;
        }
#ifdef DEBUG_XCP_PRINTS
        ThrowError(1, "CCP_AKTIVATE GlobalXcpCcpActiveFlag = %i", GlobalXcpCcpActiveFlag);
        DebugPrintConList();
#endif
        ret = 1;
        break;
    case CCP_DEAKTIVATE:
        read_message (&mhdummy, (char*)&ConnectionNo, sizeof (int));
        if ((ConnectionNo >= 0) && (ConnectionNo < MAX_CONNECTIONS)) {
            int x;
            XcpConnections[ConnectionNo].CcpActive = 1;
            XcpConnections[ConnectionNo].CcpDtoTimeoutActive = 0;
            RemoveConFromList (ConnectionNo);
            // are there any connection active
            GlobalXcpCcpActiveFlag = 0;
            for (x = 0; (GET_CON_NO(x) >= 0) && (x < MAX_CONNECTIONS); x++) {
                if (XcpConnections[GET_CON_NO(x)].CcpActive) GlobalXcpCcpActiveFlag = 1;
            }
        }
#ifdef DEBUG_XCP_PRINTS
        ThrowError(1, "CCP_DEAKTIVATE GlobalXcpCcpActiveFlag = %i", GlobalXcpCcpActiveFlag);
        DebugPrintConList();
#endif
        ret = 1;
        break;
    case CCP_DTO_ACTIVATE_TIMEOUT:
        read_message (&mhdummy, (char*)&ConnectionNo, sizeof (int));
        if ((ConnectionNo >= 0) && (ConnectionNo < MAX_CONNECTIONS)) {
            XcpConnections[ConnectionNo].CcpDtoTimeoutActive = 1;
            XcpConnections[ConnectionNo].Timeout = (uint64_t)TIMERCLKFRQ / 2;  // Fixed timeout 0.5s
            XcpConnections[ConnectionNo].TimeStampTimeout = get_timestamp_counter() + XcpConnections[ConnectionNo].Timeout;
        }
        ret = 1;
        break;
    case CCP_CRO:
        {
            CCP_CRO_CAN_OBJECT CanObject;
            read_message (&mhdummy, (char*)&CanObject, sizeof (CCP_CRO_CAN_OBJECT));
             if (CanObject.Channel >= csc->channel_count) {
                 ThrowError (1, "CRO-Message channel %i not valid (0...%i)", CanObject.Channel, csc->channel_count - 1);
             } else {
				 csc->channels[CanObject.Channel].queue_write_can (csc, CanObject.Channel,
                                                                   (uint32_t)CanObject.Id,
                                                                   CanObject.Data,   // Byte 0 is the connection nunber
                                                                   (unsigned char)CanObject.ExtFlag,
                                                                   (unsigned char)CanObject.Size);
                 WriteCanMessageFromBus2Fifos (CanObject.Channel,
                                               (uint32_t)CanObject.Id,
                                               CanObject.Data,
                                               (unsigned char)CanObject.ExtFlag,
                                               (unsigned char)CanObject.Size, 1, (uint64_t)blackboard_infos.ActualCycleNumber);
            }
        }
        ret = 1;
        break;
    case CCP_SET_CRM_AND_DTOS_IDS:
        read_message (&mhdummy, (char*)&XcpConnections[ActiveConnectionNo].CppConfig, sizeof (CCP_CONFIG));
        /*ThrowError (1, "CCP-Config:");
        ThrowError (1, "  Channel = %i", CppConfig.Channel);
        ThrowError (1, "  ExtIds = %i", CppConfig.ExtIds);
        ThrowError (1, "  ByteOrder = %i", CppConfig.ByteOrder);
        ThrowError (1, "  CRO_id = 0x%X", CppConfig.CRO_id);
        ThrowError (1, "  CRM_id = 0x%X", CppConfig.CRM_id);
        ThrowError (1, "  DTO_id = 0x%X", CppConfig.DTO_id);*/
        ret = 1;
        break;
    case CPP_DTOS_DATA_STRUCTS:
        if (XcpConnections[ActiveConnectionNo].CppVariConf != NULL) {
            AddOrRemoveBBVaris (0, &XcpConnections[ActiveConnectionNo]);  // remove all variables
            my_free (XcpConnections[ActiveConnectionNo].CppVariConf);
            XcpConnections[ActiveConnectionNo].CppVariConf = NULL;
        }
        if ((XcpConnections[ActiveConnectionNo].CppVariConf = (CCP_VARIABLES_CONFIG*)my_malloc (mhead->size)) != NULL) {
            read_message (&mhdummy, (char*)XcpConnections[ActiveConnectionNo].CppVariConf, mhead->size);
            XcpConnections[ActiveConnectionNo].ReceiveFlagsMask = 0;
            XcpConnections[ActiveConnectionNo].ReceiveFlagsMask = ~XcpConnections[ActiveConnectionNo].ReceiveFlagsMask;
            XcpConnections[ActiveConnectionNo].ReceiveFlagsMask <<= XcpConnections[ActiveConnectionNo].CppVariConf->DTOPackagesCount;
            XcpConnections[ActiveConnectionNo].ReceiveFlagsMask = ~XcpConnections[ActiveConnectionNo].ReceiveFlagsMask;
            XcpConnections[ActiveConnectionNo].ReceiveFlags = 0;
            XcpConnections[ActiveConnectionNo].AllVariableReceivedOneTimeFlag = 0;
            AddOrRemoveBBVaris (1, &XcpConnections[ActiveConnectionNo]);  // add all variables
        } else remove_message ();
        ret = 1;
        break;
    case CCP_REMOVE_BBVARIS:
        read_message (&mhdummy, (char*)&ConnectionNo, sizeof (int));
        if (XcpConnections[ActiveConnectionNo].CppVariConf != NULL) {
            AddOrRemoveBBVaris (0, &XcpConnections[ActiveConnectionNo]);  // remove all variables
            my_free (XcpConnections[ActiveConnectionNo].CppVariConf);
            XcpConnections[ActiveConnectionNo].CppVariConf = NULL;
        }
        ret = 1;
        break;
    }
    return ret;
}


__inline static unsigned char* SwapBytes (unsigned char *Data, int Size, int ByteOrder)
{
    unsigned char help;

    if (ByteOrder) {
        if (Size == 2) {
            help = Data[0];
            Data[0] = Data[1];
            Data[1] = help;
        } else if (Size == 4) {
            help = Data[0];
            Data[0] = Data[3];
            Data[3] = help;
            help = Data[1];
            Data[1] = Data[2];
            Data[2] = help;
        }
    }
    return Data;
}

int CppCanMessageFilter (NEW_CAN_SERVER_CONFIG *csc, int Channel, uint32_t Id, unsigned char *Data, int Size)
{
    UNUSED(csc);
    int DTOIdx;
    CCP_DTO_PACKAGE *Dto;
    double value;
    int v;
    int x;

    for (x = 0; GET_CON_NO(x) >= 0; x++) {
        XCP_CONNECTION *pCon = &XcpConnections[GET_CON_NO(x)];
        if (!pCon->CcpActive) continue;

        if (pCon->CppConfig.Channel == Channel) {
            if ((pCon->CppConfig.CRM_id == Id) && (((int)(unsigned char)Data[0] == 0xFF) ||
                                                   ((int)(unsigned char)Data[0] == 0xFE))) {    // Command ACK
                //ThrowError (1, "got an ACK");
                CCP_CRO_CAN_OBJECT CanObject;

                CanObject.Channel = GET_CON_NO(x);
                CanObject.Id = Id;
                CanObject.ExtFlag = 0;
                CanObject.Size = Size;
                MEMCPY (CanObject.Data, Data, 8);
                write_important_message ((pCon->CppConfig.XcpOrCcp) ? get_pid_by_name ("XCP_Control") : get_pid_by_name ("CCP_Control"), 
                                         CCP_CRM_ACK, sizeof (CanObject), (char*)&CanObject);
                return 1;    // Message is a CCP CRM message
            } else if (pCon->CcpDtoTimeoutActive && (pCon->CppVariConf != NULL)) {
                if ((pCon->CppConfig.DTO_id & 0x1FFFFFFF)== Id) {   // Data object
                    DTOIdx = (int)(unsigned char)Data[0];
                    if (DTOIdx < pCon->CppVariConf->DTOPackagesCount) {
                        pCon->ReceiveFlags |= (uint64_t)1 << DTOIdx;
                        Dto = &(pCon->CppVariConf->DTO_Packages[DTOIdx]);
                        Dto->Timeout = 0;   // reset timeout
                        for (v = 0; v < Dto->VarCount; v++) {
                            switch (Dto->Entrys[v].Type) {
                            case BB_BYTE:
                                value = *(signed char*)&(Data[Dto->Entrys[v].ByteOffset]);
                                break;
                            case BB_UBYTE:
                                value = *(unsigned char*)&(Data[Dto->Entrys[v].ByteOffset]);
                                break;
                            case BB_WORD:
                                value = *(short*)(void*)SwapBytes(&(Data[Dto->Entrys[v].ByteOffset]), 2, pCon->CppConfig.ByteOrder);
                                break;
                            case BB_UWORD:
                                value = *(unsigned short*)(void*)SwapBytes(&(Data[Dto->Entrys[v].ByteOffset]), 2, pCon->CppConfig.ByteOrder);
                                break;
                            case BB_DWORD:
                                value = *(int32_t*)(void*)SwapBytes(&(Data[Dto->Entrys[v].ByteOffset]), 4, pCon->CppConfig.ByteOrder);
                                break;
                            case BB_UDWORD:
                                value = *(uint32_t*)(void*)SwapBytes(&(Data[Dto->Entrys[v].ByteOffset]), 4, pCon->CppConfig.ByteOrder);
                                break;
                            case BB_FLOAT:
                                value = (double)*(float*)(void*)SwapBytes(&(Data[Dto->Entrys[v].ByteOffset]), 4, pCon->CppConfig.ByteOrder);
                                break;
                            default:
                                value = 0;
                            }
                            write_bbvari_minmax_check (Dto->Entrys[v].Vid, value);
                        }
                    } else {
                        // ThrowError (1, "not valid DTO index 0x%02X returned from TCU", DTOIdx);
                    }
                    return 1; // Message is a CCP DTO message
                }
            }
        }
    }
    return 0;   // Message is not a CCP-Message
}

void CcpCyclic (void)
{
    int i;

    // Do only timeout control
    for (i = 0; GET_CON_NO(i) >= 0; i++) {
        XCP_CONNECTION *pCon = &XcpConnections[GET_CON_NO(i)];
        if (pCon->CcpActive && pCon->CcpDtoTimeoutActive && (pCon->CppVariConf != NULL)) {
            if (pCon->ReceiveFlags == pCon->ReceiveFlagsMask) {
                // All DTOs was received one time
                if (!pCon->AllVariableReceivedOneTimeFlag) {
                    // Report that the first time all variablen was received
                    write_message ((pCon->CppConfig.XcpOrCcp) ? get_pid_by_name ("XCP_Control") : get_pid_by_name ("CCP_Control"), 
                                   CCP_RECEIVE_ALL_VARIABLE_FIRST_TIME, sizeof (int), (char*)&GET_CON_NO(i));
                    pCon->AllVariableReceivedOneTimeFlag = 1;
                }
                pCon->ReceiveFlags = 0;
                pCon->TimeStampTimeout = get_timestamp_counter() + pCon->Timeout;
            } else {
                if (TSCOUNT_LARGER_TS (get_timestamp_counter(), pCon->TimeStampTimeout)) {
                    // Timeout!
                    write_message ((pCon->CppConfig.XcpOrCcp) ? get_pid_by_name ("XCP_Control") : get_pid_by_name ("CCP_Control"), 
                                   CCP_DTO_TIMEOUT, sizeof (int), (char*)&GET_CON_NO(i));
                    pCon->TimeStampTimeout = get_timestamp_counter() + pCon->Timeout;
                    pCon->CcpDtoTimeoutActive = 0;
                }
            }
        }
    }
}

void CcpTerminate (void)
{
    int i;
    if (GlobalXcpCcpActiveFlag) {
        for (i = 0; GET_CON_NO(i) >= 0; i++) {
            XCP_CONNECTION *pCon = &XcpConnections[GET_CON_NO(i)];
            pCon->CcpActive = 0;
            pCon->CcpDtoTimeoutActive = 0;
            if (pCon->CppVariConf != NULL) {
                // If a connection is actice stop timeout control
                write_message ((pCon->CppConfig.XcpOrCcp) ? get_pid_by_name ("XCP_Control") : get_pid_by_name ("CCP_Control"), 
                               CCP_DTO_TIMEOUT, sizeof (int), (char*)&GET_CON_NO(i));
                AddOrRemoveBBVaris (0, pCon);
                my_free (pCon->CppVariConf);
                pCon->CppVariConf = NULL;
            }
        }
        GlobalXcpCcpActiveFlag = 0;
    }
}

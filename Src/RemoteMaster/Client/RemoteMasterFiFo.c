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


#include<stdio.h>
#ifdef _WIN32
#include <WinSock2.h>
#else
#endif

#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Scheduler.h"
#include "Fifos.h"

#include "StructsRM_FiFo.h"
#include "RemoteMasterNet.h"

#include "RemoteMasterFiFo.h"

#define CHECK_ANSWER(Req,Ack)

int rm_RegisterNewRxFiFoName (int RxPid, const char *Name)
{
    RM_REGISTER_NEW_RX_FIFO_NAME_REQ *Req;
    RM_REGISTER_NEW_RX_FIFO_NAME_ACK Ack;
    int SizeOfStruct;
    int NameLen;
    NameLen = (int)strlen (Name) + 1;
    SizeOfStruct =(int)sizeof(RM_REGISTER_NEW_RX_FIFO_NAME_REQ) + NameLen;
    Req = (RM_REGISTER_NEW_RX_FIFO_NAME_REQ*)_alloca((size_t)SizeOfStruct);
    Req->RxPid = (uint32_t)RxPid;
    Req->Offset_Name = sizeof (RM_REGISTER_NEW_RX_FIFO_NAME_REQ);
    MEMCPY (Req + 1, Name, (size_t)NameLen);
    TransactRemoteMaster (RM_REGISTER_NEW_RX_FIFO_NAME_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_UnRegisterRxFiFo (int FiFoId, int RxPid)
{
    RM_UNREGISTER_RX_FIFO_REQ Req;
    RM_UNREGISTER_RX_FIFO_ACK Ack;
    Req.FiFoId = (uint32_t)FiFoId;
    Req.RxPid = (uint32_t)RxPid;
    TransactRemoteMaster (RM_UNREGISTER_RX_FIFO_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_TxAttachFiFo (int RxPid, const char *Name)
{
    RM_RX_ATTACH_FIFO_REQ *Req;
    RM_RX_ATTACH_FIFO_ACK Ack;
    int SizeOfStruct;
    int NameLen;
    NameLen = (int)strlen (Name) + 1;
    SizeOfStruct = (int)sizeof(RM_RX_ATTACH_FIFO_REQ) + NameLen;
    Req = (RM_RX_ATTACH_FIFO_REQ*)_alloca((size_t)SizeOfStruct);
    Req->RxPid = (uint32_t)RxPid;
    Req->Offset_Name = sizeof (RM_RX_ATTACH_FIFO_REQ);
    MEMCPY (Req + 1, Name, (size_t)NameLen);
    TransactRemoteMaster (RM_RX_ATTACH_FIFO_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_SyncFiFos (void)
{
    static RM_SYNC_FIFOS_REQ *Req;
    static RM_SYNC_FIFOS_ACK *Ack;
    static int SizeOfReqStruct;
    static int SizeOfAckStruct;
    int DataToTransmit;

    if (Req == NULL) {
        SizeOfReqStruct = 1024*1024 + sizeof(RM_SYNC_FIFOS_REQ);
        SizeOfAckStruct = 1024*1024 + sizeof(RM_SYNC_FIFOS_ACK);
        Req = (RM_SYNC_FIFOS_REQ*)my_malloc(SizeOfReqStruct);
        Ack = (RM_SYNC_FIFOS_ACK*)my_malloc(SizeOfAckStruct);
    }

    switch(SyncTxFiFos (Req + 1, SizeOfReqStruct - (int)sizeof(RM_SYNC_FIFOS_REQ), &DataToTransmit)) {
    case 0:
    case 1:
        break;
    case 2:
    case 3:
        SizeOfReqStruct += 256 * 1024;
        SizeOfAckStruct += 256 * 1024;
        Req = (RM_SYNC_FIFOS_REQ*)my_realloc(Req, SizeOfReqStruct);
        Ack = (RM_SYNC_FIFOS_ACK*)my_realloc(Ack, SizeOfAckStruct);
        break;
    default:
        ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
        break;
    }
    Req->Len = (uint32_t)DataToTransmit;
    Req->Offset_Buffer = sizeof (RM_SYNC_FIFOS_REQ);
    TransactRemoteMaster (RM_SYNC_FIFOS_CMD, Req, (int)sizeof(RM_SYNC_FIFOS_REQ) + DataToTransmit, Ack, SizeOfAckStruct);
    SyncRxFiFos (Ack + 1, (int)Ack->Len);
    CHECK_ANSWER(Req, Ack);
    return Ack->Ret;
}

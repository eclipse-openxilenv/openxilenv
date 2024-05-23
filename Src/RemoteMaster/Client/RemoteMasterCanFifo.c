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
#include<WinSock2.h>
#else
#endif

#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Scheduler.h"

#include "Message.h"
#include "StructsRM_CanFifo.h"
#include "RemoteMasterNet.h"

#include "RemoteMasterMessage.h"

#define CHECK_ANSWER(Req,Ack)

int rm_CreateCanFifos (int Depth, int FdFlag)
{
    RM_CAN_FIFO_CREATE_CAN_FIFOS_REQ Req;
    RM_CAN_FIFO_CREATE_CAN_FIFOS_ACK Ack;

    Req.Depth = Depth;
    Req.FdFlag = FdFlag;
    TransactRemoteMaster (RM_CAN_FIFO_CREATE_CAN_FIFOS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_DeleteCanFifos (int Handle)
{
    RM_CAN_FIFO_DELETE_CAN_FIFOS_REQ Req;
    RM_CAN_FIFO_DELETE_CAN_FIFOS_ACK Ack;

    Req.Handle = Handle;
    TransactRemoteMaster (RM_CAN_FIFO_DELETE_CAN_FIFOS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_FlushCanFifo (int Handle, int Flags)
{
    RM_CAN_FIFO_FLUSH_CAN_FIFO_REQ Req;
    RM_CAN_FIFO_FLUSH_CAN_FIFO_ACK Ack;

    Req.Handle = Handle;
    Req.Flags =Flags;
    TransactRemoteMaster (RM_CAN_FIFO_FLUSH_CAN_FIFO_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_ReadCanMessageFromFifo2Process (int Handle, CAN_FIFO_ELEM *pCanMessage)
{
    RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_REQ Req;
    RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK Ack;

    Req.Handle = Handle;
    TransactRemoteMaster (RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *pCanMessage = Ack.ret_CanMessage;
    return Ack.Ret;
}

int rm_ReadCanMessagesFromFifo2Process (int Handle, CAN_FIFO_ELEM *pCanMessage, int MaxMessages)
{
    RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_REQ Req;
    RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK *Ack;
    int SizeOfStruct;
    CAN_FIFO_ELEM *Src;
    int x;

    SizeOfStruct = (int)sizeof(RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK) + MaxMessages * (int)sizeof (CAN_FIFO_ELEM);
    Ack = (RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK*)_alloca((size_t)SizeOfStruct);
    Req.Handle = Handle;
    Req.MaxMessages = MaxMessages;
    TransactRemoteMaster (RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_CMD, &Req, sizeof(Req), Ack, SizeOfStruct);
    CHECK_ANSWER(Req, Ack);
    Src = (CAN_FIFO_ELEM*)((char*)Ack + Ack->Offset_CanMessage);
    for (x = 0; (x < Ack->Ret) && (x < MaxMessages); x++) {
        pCanMessage[x] = Src[x];
    }
    return Ack->Ret;
}

int rm_WriteCanMessagesFromProcess2Fifo (int Handle, CAN_FIFO_ELEM *pCanMessage, int MaxMessages)
{
    RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ *Req;
    RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK Ack;
    int SizeOfStruct;

    SizeOfStruct = (int)sizeof(RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ) + MaxMessages * (int)sizeof (CAN_FIFO_ELEM);
    Req = (RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Handle = Handle;
    Req->MaxMessages = MaxMessages;
    Req->Offset_CanMessage = sizeof(RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ);
    MEMCPY (Req + 1, pCanMessage, (size_t)MaxMessages * sizeof (CAN_FIFO_ELEM));
    TransactRemoteMaster (RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_SetAcceptMask4CanFifo (int Handle, CAN_ACCEPT_MASK *cam, int Size)
{
    RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_REQ *Req;
    RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_ACK Ack;
    int SizeOfStruct;

    SizeOfStruct = (int)sizeof(RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_REQ) + Size * (int)sizeof (CAN_ACCEPT_MASK);
    Req = (RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Handle = Handle;
    Req->Size = Size;
    Req->Offset_cam = sizeof(RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_REQ);
    MEMCPY (Req + 1, cam, (size_t)Size * sizeof (CAN_ACCEPT_MASK));
    TransactRemoteMaster (RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_WriteCanMessageFromProcess2Fifo (int Handle, CAN_FIFO_ELEM *pCanMessage)
{
    RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_REQ Req;
    RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK Ack;

    Req.Handle = Handle;
    Req.CanMessage = *pCanMessage;
    TransactRemoteMaster (RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}


int rm_ReadCanFdMessageFromFifo2Process (int Handle, CAN_FD_FIFO_ELEM *pCanMessage)
{
    RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_REQ Req;
    RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK Ack;

    Req.Handle = Handle;
    TransactRemoteMaster (RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *pCanMessage = Ack.ret_CanMessage;
    return Ack.Ret;
}

int rm_ReadCanFdMessagesFromFifo2Process (int Handle, CAN_FD_FIFO_ELEM *pCanMessage, int MaxMessages)
{
    RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_REQ Req;
    RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK *Ack;
    int SizeOfStruct;
    CAN_FD_FIFO_ELEM *Src;
    int x;

    SizeOfStruct = (int)sizeof(RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK) + MaxMessages * (int)sizeof (CAN_FD_FIFO_ELEM);
    Ack = (RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK*)_alloca((size_t)SizeOfStruct);
    Req.Handle = Handle;
    Req.MaxMessages = MaxMessages;
    TransactRemoteMaster (RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_CMD, &Req, sizeof(Req), Ack, SizeOfStruct);
    CHECK_ANSWER(Req, Ack);
    Src = (CAN_FD_FIFO_ELEM*)((char*)Ack + Ack->Offset_CanMessage);
    for (x = 0; (x < Ack->Ret) && (x < MaxMessages); x++) {
        pCanMessage[x] = Src[x];
    }
    return Ack->Ret;
}

int rm_WriteCanFdMessagesFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage, int MaxMessages)
{
    RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ *Req;
    RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK Ack;
    int SizeOfStruct;

    SizeOfStruct = (int)sizeof(RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ) + MaxMessages * (int)sizeof (CAN_FD_FIFO_ELEM);
    Req = (RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Handle = Handle;
    Req->MaxMessages = MaxMessages;
    Req->Offset_CanMessage = sizeof(RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ);
    MEMCPY (Req + 1, pCanMessage, (size_t)MaxMessages * sizeof (CAN_FD_FIFO_ELEM));
    TransactRemoteMaster (RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_WriteCanFdMessageFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage)
{
    RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_REQ Req;
    RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK Ack;

    Req.Handle = Handle;
    Req.CanMessage = *pCanMessage;
    TransactRemoteMaster (RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}



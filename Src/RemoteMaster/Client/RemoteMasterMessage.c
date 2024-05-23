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
#include "StructsRM_Message.h"
#include "RemoteMasterNet.h"

#include "RemoteMasterMessage.h"

#define CHECK_ANSWER(Req,Ack)

int rm_write_message_ts_as (int pid, int mid, uint64_t timestamp, int msize, char *mblock, int tx_pid)
{
    RM_MESSAGE_WRITE_MESSAGE_TS_AS_REQ *Req;
    RM_MESSAGE_WRITE_MESSAGE_TS_AS_ACK Ack;
    int SizeOfStruct;

    SizeOfStruct = (int)sizeof(RM_MESSAGE_WRITE_MESSAGE_TS_AS_REQ) + msize;
    Req = (RM_MESSAGE_WRITE_MESSAGE_TS_AS_REQ*)_alloca((size_t)SizeOfStruct);
    Req->pid = pid;
    Req->mid = mid;
    Req->timestamp = timestamp;
    Req->msize = msize;
    Req->Offset_mblock = sizeof (RM_MESSAGE_WRITE_MESSAGE_TS_AS_REQ);
    MEMCPY (Req + 1, mblock, (size_t)msize);
    Req->tx_pid = tx_pid;
    TransactRemoteMaster (RM_MESSAGE_WRITE_MESSAGE_TS_AS_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_write_important_message_ts (int pid, int mid, uint64_t timestamp, int msize, char *mblock)
{
    RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_REQ *Req;
    RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_ACK Ack;
    int SizeOfStruct;

    SizeOfStruct = (int)sizeof(RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_REQ) + msize;
    Req = (RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_REQ*)_alloca((size_t)SizeOfStruct);
    Req->pid = pid;
    Req->mid = mid;
    Req->timestamp = timestamp;
    Req->msize = msize;
    Req->Offset_mblock = sizeof (RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_REQ);
    MEMCPY (Req + 1, mblock, (size_t)msize);
    Req->tx_pid = GET_PID();
    TransactRemoteMaster (RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_test_message (MESSAGE_HEAD *mhead)
{
    RM_MESSAGE_TEST_MESSAGE_AS_REQ Req;
    RM_MESSAGE_TEST_MESSAGE_AS_ACK Ack;

    Req.pid = GET_PID();
    TransactRemoteMaster (RM_MESSAGE_TEST_MESSAGE_AS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *mhead = Ack.ret_mhead;
    return Ack.Ret;
}

int rm_read_message (MESSAGE_HEAD *mhead, char *message, int maxlen)
{
    RM_MESSAGE_READ_MESSAGE_AS_REQ Req;
    RM_MESSAGE_READ_MESSAGE_AS_ACK *Ack;
    int SizeOfStruct;

    SizeOfStruct = (int)sizeof(RM_MESSAGE_READ_MESSAGE_AS_ACK) + maxlen;
    Ack = (RM_MESSAGE_READ_MESSAGE_AS_ACK*)_alloca((size_t)SizeOfStruct);
    Req.pid = GET_PID();
    Req.maxlen = maxlen;
    TransactRemoteMaster (RM_MESSAGE_READ_MESSAGE_AS_CMD, &Req, sizeof(Req), Ack, SizeOfStruct);
    CHECK_ANSWER(Req, Ack);
    *mhead = Ack->ret_mhead;
    MEMCPY (message, Ack + 1, Ack->PackageHeader.SizeOf - sizeof (RM_MESSAGE_READ_MESSAGE_AS_ACK));
    return Ack->Ret;
}

int rm_remove_message (void)
{
    RM_MESSAGE_REMOVE_MESSAGE_AS_REQ Req;
    RM_MESSAGE_REMOVE_MESSAGE_AS_ACK Ack;

    Req.pid = GET_PID();
    TransactRemoteMaster (RM_MESSAGE_REMOVE_MESSAGE_AS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

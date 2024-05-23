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
#include "StringMaxChar.h"
#include "Scheduler.h"
#include "EquationParser.h"
#include "Message.h"
#include "CanDataBase.h"
#include "CanFifo.h"
#include "CanRecorder.h"

#include "RpcControlProcess.h"
#include "RpcSocketServer.h"
#include "RpcFuncCan.h"

#define UNUSED(x) (void)(x)

static int RPCFunc_LoadCanVariant(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_LOAD_CAN_VARIANTE_MESSAGE *In = (RPC_API_LOAD_CAN_VARIANTE_MESSAGE*)par_DataIn;
    RPC_API_LOAD_CAN_VARIANTE_MESSAGE_ACK *Out = (RPC_API_LOAD_CAN_VARIANTE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_LOAD_CAN_VARIANTE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOAD_CAN_VARIANTE_MESSAGE_ACK);
    Out->Header.ReturnValue = LoadCANVarianteScriptCommand ((char*)In + In->OffsetCanFile, In->Channel);
    return Out->Header.StructSize;
}

static int RPCFunc_LoadAndSelCanVariant(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE *In = (RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE*)par_DataIn;
    RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE_ACK *Out = (RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE_ACK);
    Out->Header.ReturnValue = 0;
    LoadAndSelectCANNodeScriptCommand ((char*)In + In->OffsetCanFile, In->Channel);
    return Out->Header.StructSize;
}

static int RPCFunc_AppendCanVariant(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_APPEND_CAN_VARIANTE_MESSAGE *In = (RPC_API_APPEND_CAN_VARIANTE_MESSAGE*)par_DataIn;
    RPC_API_APPEND_CAN_VARIANTE_MESSAGE_ACK *Out = (RPC_API_APPEND_CAN_VARIANTE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_APPEND_CAN_VARIANTE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_APPEND_CAN_VARIANTE_MESSAGE_ACK);
    Out->Header.ReturnValue = 0;
    AppendCANVarianteScriptCommand ((char*)In + In->OffsetCanFile, In->Channel);
    return Out->Header.StructSize;
}

static int RPCFunc_DelAllCanVariant(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    //RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE *In = (RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE*)par_DataIn;
    RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE_ACK *Out = (RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE_ACK);
    Out->Header.ReturnValue = 0;
    DeleteAllCANVariantesScriptCommand ();
    return Out->Header.StructSize;
}


extern int CheckCANMessageQueueScript (void);  // lives in TransmitCanCmd.cpp

static int RPCFunc_TransmitCAN(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    CAN_FIFO_ELEM CANMessage;
    int CanFiFoHandle;
    RPC_API_TRANSMIT_CAN_MESSAGE *In = (RPC_API_TRANSMIT_CAN_MESSAGE*)par_DataIn;
    RPC_API_TRANSMIT_CAN_MESSAGE_ACK *Out = (RPC_API_TRANSMIT_CAN_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_TRANSMIT_CAN_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_TRANSMIT_CAN_MESSAGE_ACK);

    // Is the CAN message queue already exists?
    CanFiFoHandle = CheckCANMessageQueueScript ();
    CANMessage.channel = (uint8_t)In->Channel;
    CANMessage.id = (uint32_t)In->Id;
    CANMessage.ext = (uint8_t)In->Ext;
    CANMessage.size = (uint8_t)In->Size;
    if (CANMessage.size > 8) CANMessage.size = 8;
    MEMCPY (CANMessage.data, (char*)In + In->Offset_uint8_Size_Data, CANMessage.size);

    Out->Header.ReturnValue = WriteCanMessageFromProcess2Fifo (CanFiFoHandle, &CANMessage);
    return Out->Header.StructSize;
}

static int RPCFunc_OpenCanQueue(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_OPEN_CAN_QUEUE_MESSAGE *In = (RPC_API_OPEN_CAN_QUEUE_MESSAGE*)par_DataIn;
    RPC_API_OPEN_CAN_QUEUE_MESSAGE_ACK *Out = (RPC_API_OPEN_CAN_QUEUE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_OPEN_CAN_QUEUE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_OPEN_CAN_QUEUE_MESSAGE_ACK);
    par_Connection->CanFifoHandle = CreateCanFifos (In->Depth, 0);
    if (par_Connection->CanFifoHandle > 0) Out->Header.ReturnValue = 0;
    else Out->Header.ReturnValue = -1;
    return Out->Header.StructSize;
}

static int RPCFunc_SetCANAcceptanceWindows(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE *In = (RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE*)par_DataIn;
    RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE_ACK *Out = (RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE_ACK);
    Out->Header.ReturnValue = SetAcceptMask4CanFifo (par_Connection->CanFifoHandle,
                                                     (CAN_ACCEPT_MASK*)((char*)In + In->Offset_CanAcceptance_Size_Windows),
                                                     In->Size);
    return Out->Header.StructSize;
}

static int RPCFunc_FlushCanQueue(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_FLUSH_CAN_QUEUE_MESSAGE *In = (RPC_API_FLUSH_CAN_QUEUE_MESSAGE*)par_DataIn;
    RPC_API_FLUSH_CAN_QUEUE_MESSAGE_ACK *Out = (RPC_API_FLUSH_CAN_QUEUE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_FLUSH_CAN_QUEUE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_FLUSH_CAN_QUEUE_MESSAGE_ACK);
    FlushCanFifo(par_Connection->CanFifoHandle, In->Flags);
    Out->Header.ReturnValue = FlushCanFifo(par_Connection->CanFifoHandle, In->Flags);
    return Out->Header.StructSize;
}

static int RPCFunc_ReadCANQueue(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    int x, Max;
    CAN_FIFO_ELEM* Help;

    RPC_API_READ_CAN_QUEUE_MESSAGE *In = (RPC_API_READ_CAN_QUEUE_MESSAGE*)par_DataIn;
    RPC_API_READ_CAN_QUEUE_MESSAGE_ACK *Out = (RPC_API_READ_CAN_QUEUE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_READ_CAN_QUEUE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_READ_CAN_QUEUE_MESSAGE_ACK);
    Out->Offset_CanObject_ReadElements_Messages = sizeof(*Out) - 1;
    if (In->ReadMaxElements < 0) Max = 0;
    else if (In->ReadMaxElements > 64) Max = 64;
    else Max = In->ReadMaxElements;

    Help = (CAN_FIFO_ELEM*)((char*)Out + Out->Offset_CanObject_ReadElements_Messages);
    for (x = 0; x < Max; x++) {
        if (ReadCanMessageFromFifo2Process (par_Connection->CanFifoHandle,
                                            (CAN_FIFO_ELEM*)&(Help[x])) < 1) {
            break;
        }
    }
    Out->Header.StructSize += x * (int)sizeof(CAN_FIFO_ELEM);

    Out->Header.ReturnValue = Out->ReadElements = x;

    return Out->Header.StructSize;
}

static int RPCFunc_TransmitCANQueue(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    CAN_FIFO_ELEM* Help;
    RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE *In = (RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE*)par_DataIn;
    RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE_ACK *Out = (RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE_ACK);

    Help = (CAN_FIFO_ELEM*)((char*)In + In->Offset_CanObject_WriteElements_Messages);
    Out->Header.ReturnValue = WriteCanMessagesFromProcess2Fifo (par_Connection->CanFifoHandle, Help, In->WriteElements);

    return Out->Header.StructSize;
}

static int RPCFunc_CloseCANQueue(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_DataIn);
    //RPC_API_CLOSE_CAN_QUEUE_MESSAGE *In = (RPC_API_CLOSE_CAN_QUEUE_MESSAGE*)par_DataIn;
    RPC_API_CLOSE_CAN_QUEUE_MESSAGE_ACK *Out = (RPC_API_CLOSE_CAN_QUEUE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_CLOSE_CAN_QUEUE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CLOSE_CAN_QUEUE_MESSAGE_ACK);
    Out->Header.ReturnValue = DeleteCanFifos (par_Connection->CanFifoHandle);
    par_Connection->CanFifoHandle = 0;
    return Out->Header.StructSize;
}

static int RPCFunc_SetCanErr(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_CAN_ERR_MESSAGE *In = (RPC_API_SET_CAN_ERR_MESSAGE*)par_DataIn;
    RPC_API_SET_CAN_ERR_MESSAGE_ACK *Out = (RPC_API_SET_CAN_ERR_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SET_CAN_ERR_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_CAN_ERR_MESSAGE_ACK);
    Out->Header.ReturnValue = ScriptSetCanErr (In->Channel, In->Id, In->Startbit, In->Bitsize,
                                               (char*)In + In->OffsetByteorder, In->Cycles, In->BitErrValue);
    return Out->Header.StructSize;
}

static int RPCFunc_SetCanErrSignalName(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE *In = (RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE*)par_DataIn;
    RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE_ACK *Out = (RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE_ACK);
    Out->Header.ReturnValue = ScriptSetCanErrSignalName (In->Channel, In->Id, (char*)In + In->OffsetSignalName,
                                                         In->Cycles, In->BitErrValue);
    return Out->Header.StructSize;
}

static int RPCFunc_ClearCanErr(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    // RPC_API_CLEAR_CAN_ERR_MESSAGE *In = (RPC_API_CLEAR_CAN_ERR_MESSAGE*)par_DataIn;
    RPC_API_CLEAR_CAN_ERR_MESSAGE_ACK *Out = (RPC_API_CLEAR_CAN_ERR_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_CLEAR_CAN_ERR_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CLEAR_CAN_ERR_MESSAGE_ACK);
    Out->Header.ReturnValue = ScriptClearCanErr ();
    return Out->Header.StructSize;
}


static void __SCSetCanSignalConversionCallBackBehind (int TimeoutFlag, uint32_t Remainder, void *Param)
{
    UNUSED(TimeoutFlag);
    UNUSED(Remainder);
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Param;
    MESSAGE_HEAD mhead;
    if (test_message (&mhead)) {
        if (mhead.mid == NEW_CANSERVER_SET_SIG_CONV) {
            int Ret;
            read_message (&mhead, (char *)&Ret, sizeof (int));
            Connection->ReturnValue = Ret;
        }
    }
    RemoteProcedureWakeWaitForConnection(Connection);
}

static int RPCFunc_SetCanSignalConvertion(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    struct EXEC_STACK_ELEM *ExecStack;
    int UseCANData;
    RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE *In = (RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE*)par_DataIn;
    RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK *Out = (RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK);


    ExecStack = solve_equation_replace_parameter_with_can ((char*)In + In->OffsetConvertion, &UseCANData);
    if (ExecStack == NULL) {
        Out->Header.ReturnValue = -1;
    } else {
        if (SendAddOrRemoveReplaceCanSigConvReq (In->Channel, In->Id, (char*)In + In->OffsetSignalName, ExecStack, UseCANData)) {
            Out->Header.ReturnValue = -1;
        } else {
            if (get_scheduler_state()) {
                RemoteProcedureMarkedForWaitForConnection(par_Connection);
                SetDelayFunc (par_Connection, 10000, NULL, NULL,
                              __SCSetCanSignalConversionCallBackBehind, par_Connection,
                              NEW_CANSERVER_SET_SIG_CONV, NULL);
                RemoteProcedureWaitForConnection(par_Connection);
            } else {
                Out->Header.ReturnValue = 0;
            }
        }
    }
    return Out->Header.StructSize;
}

static void __SCResetCanSignalConversionCallBackBehind (int TimeoutFlag, uint32_t Remainder, void *Param)
{
    UNUSED(TimeoutFlag);
    UNUSED(Remainder);
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Param;
    MESSAGE_HEAD mhead;
    if (test_message (&mhead)) {
        if (mhead.mid == NEW_CANSERVER_SET_SIG_CONV) {
            int Ret;
            read_message (&mhead, (char *)&Ret, sizeof (int));
            Connection->ReturnValue = Ret;
        }
    }
    RemoteProcedureWakeWaitForConnection(Connection);
}


static int RPCFunc_ResetCanSignalConvertion(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE *In = (RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE*)par_DataIn;
    RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK *Out = (RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK);
    Out->Header.ReturnValue = ScriptClearCanErr ();

    if (SendAddOrRemoveReplaceCanSigConvReq (In->Channel, In->Id, (char*)In + In->OffsetSignalName, NULL, 0)) {
        Out->Header.ReturnValue = -1;
    } else {
        if (get_scheduler_state()) {
            RemoteProcedureMarkedForWaitForConnection(par_Connection);
            SetDelayFunc (par_Connection, 10000, NULL, NULL,
                          __SCResetCanSignalConversionCallBackBehind, par_Connection,
                          NEW_CANSERVER_SET_SIG_CONV, NULL);
            RemoteProcedureWaitForConnection(par_Connection);
        } else {
            Out->Header.ReturnValue = 0;
        }
    }
    return Out->Header.StructSize;
}

static void __SCResetAllCanSignalConversionCallBackBehind (int TimeoutFlag, uint32_t Remainder, void *Param)
{
    UNUSED(TimeoutFlag);
    UNUSED(Remainder);
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Param;
    MESSAGE_HEAD mhead;
    if (test_message (&mhead)) {
        if (mhead.mid == NEW_CANSERVER_SET_SIG_CONV) {
            int Ret;
            read_message (&mhead, (char *)&Ret, sizeof (int));
            Connection->ReturnValue = Ret;
        }
    }
    RemoteProcedureWakeWaitForConnection(Connection);
}

static int RPCFunc_ResetAllCanSignalConvertion(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE *In = (RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE*)par_DataIn;
    RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK *Out = (RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK);
    Out->Header.ReturnValue = ScriptClearCanErr ();

    if (SendAddOrRemoveReplaceCanSigConvReq (In->Channel, In->Id, NULL, NULL, 0)) {
        return -1;
    } else {
        if (get_scheduler_state()) {
            RemoteProcedureMarkedForWaitForConnection(par_Connection);
            SetDelayFunc (par_Connection, 10000, NULL, NULL,
                          __SCResetAllCanSignalConversionCallBackBehind, par_Connection,
                          NEW_CANSERVER_SET_SIG_CONV, NULL);
            RemoteProcedureWaitForConnection(par_Connection);
        } else {
            Out->Header.ReturnValue = 0;
        }
    }
    return Out->Header.StructSize;
}

static int RPCFunc_SetCanChannelCount(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE *In = (RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE*)par_DataIn;
    RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE_ACK *Out = (RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE_ACK);
    Out->Header.ReturnValue = ScriptSetCANChannelCount (In->ChannelCount);
    return Out->Header.StructSize;
}

// CAN FD

extern int CheckCANFDMessageQueueScript (void);  // lives in TransmitCanCmd.cpp

static int RPCFunc_TransmitCANFd(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    CAN_FD_FIFO_ELEM CANMessage;
    int CanFiFoHandle;
    RPC_API_TRANSMIT_CAN_FD_MESSAGE *In = (RPC_API_TRANSMIT_CAN_FD_MESSAGE*)par_DataIn;
    RPC_API_TRANSMIT_CAN_FD_MESSAGE_ACK *Out = (RPC_API_TRANSMIT_CAN_FD_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_TRANSMIT_CAN_FD_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_TRANSMIT_CAN_FD_MESSAGE_ACK);

    // Is the CAN message queue already exists?
    CanFiFoHandle = CheckCANFDMessageQueueScript ();
    CANMessage.channel = (uint8_t)In->Channel;
    CANMessage.id = (uint32_t)In->Id;
    CANMessage.ext = (uint8_t)In->Ext;
    CANMessage.size = (uint8_t)In->Size;
    if (CANMessage.size > 64) CANMessage.size = 64;
    MEMCPY (CANMessage.data, (char*)In + In->Offset_uint8_Size_Data, CANMessage.size);

    Out->Header.ReturnValue = WriteCanFdMessageFromProcess2Fifo (CanFiFoHandle, &CANMessage);
    return Out->Header.StructSize;
}

static int RPCFunc_OpenCanFdQueue(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE *In = (RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE*)par_DataIn;
    RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE_ACK *Out = (RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE_ACK);
    par_Connection->CanFifoHandle = CreateCanFifos (In->Depth, In->FdFlag);
    if (par_Connection->CanFifoHandle > 0) Out->Header.ReturnValue = 0;
    else Out->Header.ReturnValue = -1;
    return Out->Header.StructSize;
}

static int RPCFunc_ReadCANFdQueue(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    int x, Max;
    CAN_FD_FIFO_ELEM* Help;

    RPC_API_READ_CAN_FD_QUEUE_MESSAGE *In = (RPC_API_READ_CAN_FD_QUEUE_MESSAGE*)par_DataIn;
    RPC_API_READ_CAN_FD_QUEUE_MESSAGE_ACK *Out = (RPC_API_READ_CAN_FD_QUEUE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_READ_CAN_FD_QUEUE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_READ_CAN_FD_QUEUE_MESSAGE_ACK);
    Out->Offset_CanFdObject_ReadElements_Messages = sizeof(*Out) - 1;
    if (In->ReadMaxElements < 0) Max = 0;
    else if (In->ReadMaxElements > 64) Max = 64;
    else Max = In->ReadMaxElements;

    Help = (CAN_FD_FIFO_ELEM*)((char*)Out + Out->Offset_CanFdObject_ReadElements_Messages);
    for (x = 0; x < Max; x++) {
        if (ReadCanFdMessageFromFifo2Process (par_Connection->CanFifoHandle,
                                              (CAN_FD_FIFO_ELEM*)&(Help[x])) < 1) {
            break;
        }
    }
    Out->Header.StructSize += x * (int)sizeof(CAN_FD_FIFO_ELEM);

    Out->Header.ReturnValue = Out->ReadElements = x;

    return Out->Header.StructSize;
}

static int RPCFunc_TransmitCANFdQueue(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    CAN_FD_FIFO_ELEM* Help;
    RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE *In = (RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE*)par_DataIn;
    RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE_ACK *Out = (RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE_ACK);

    Help = (CAN_FD_FIFO_ELEM*)((char*)In + In->Offset_CanFdObject_WriteElements_Messages);
    Out->Header.ReturnValue = WriteCanFdMessagesFromProcess2Fifo (par_Connection->CanFifoHandle, Help, In->WriteElements);

    return Out->Header.StructSize;
}

static int RPCFunc_StartCANRecorder(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    RPC_API_START_CAN_RECORDER_MESSAGE *In = (RPC_API_START_CAN_RECORDER_MESSAGE*)par_DataIn;
    RPC_API_START_CAN_RECORDER_MESSAGE_ACK *Out = (RPC_API_START_CAN_RECORDER_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_START_CAN_RECORDER_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_START_CAN_RECORDER_MESSAGE_ACK);
    if (par_Connection->CanRecorderHandle != NULL) {
        Out->Header.ReturnValue = -1;
    } else {
        par_Connection->CanRecorderHandle = (void*)SetupCanRecorder ((char*)In + In->Offset_FileName,
                                                       (char*)In + In->Offset_Equation,
                                                       In->DisplayColumnCounterFlag,
                                                       In->DisplayColumnTimeAbsoluteFlag,
                                                       In->DisplayColumnTimeDiffFlag,
                                                       In->DisplayColumnTimeDiffMinMaxFlag,
                                                       (CAN_ACCEPT_MASK*)((char*)In + In->Offset_CanAcceptance_Size_Windows),
                                                       In->Size);
        if (par_Connection->CanRecorderHandle != NULL) {
            AddCanRecorder(par_Connection->CanRecorderHandle);
            Out->Header.ReturnValue = 0;
        } else {
            Out->Header.ReturnValue = -1;
        }
    }
    return Out->Header.StructSize;
}


static int RPCFunc_StopCANRecorder(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_DataIn);
    //RPC_API_STOP_CAN_RECORDER_MESSAGE *In = (RPC_API_STOP_CAN_RECORDER_MESSAGE*)par_DataIn;
    RPC_API_STOP_CAN_RECORDER_MESSAGE_ACK *Out = (RPC_API_STOP_CAN_RECORDER_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_STOP_CAN_RECORDER_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_STOP_CAN_RECORDER_MESSAGE_ACK);
    if (par_Connection->CanRecorderHandle != NULL) {
        StopCanRecorder (par_Connection->CanRecorderHandle);
        Out->Header.ReturnValue = 0;
    } else {
        Out->Header.ReturnValue = -1;
    }
    par_Connection->CanRecorderHandle = NULL;
    return Out->Header.StructSize;
}

int AddCanFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOAD_CAN_VARIANTE_CMD, 1, RPCFunc_LoadCanVariant, sizeof(RPC_API_LOAD_CAN_VARIANTE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_LOAD_CAN_VARIANTE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOAD_CAN_VARIANTE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOAD_AND_SEL_CAN_VARIANTE_CMD, 1, RPCFunc_LoadAndSelCanVariant, sizeof(RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_APPEND_CAN_VARIANTE_CMD, 1, RPCFunc_AppendCanVariant, sizeof(RPC_API_APPEND_CAN_VARIANTE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_APPEND_CAN_VARIANTE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_APPEND_CAN_VARIANTE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DEL_ALL_CAN_VARIANTE_CMD, 1, RPCFunc_DelAllCanVariant, sizeof(RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE), sizeof(RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE), STRINGIZE(RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_TRANSMIT_CAN_CMD, 0, RPCFunc_TransmitCAN, sizeof(RPC_API_TRANSMIT_CAN_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_TRANSMIT_CAN_MESSAGE_MEMBERS), STRINGIZE(RPC_API_TRANSMIT_CAN_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_OPEN_CAN_QUEUE_CMD, 0, RPCFunc_OpenCanQueue, sizeof(RPC_API_OPEN_CAN_QUEUE_MESSAGE), sizeof(RPC_API_OPEN_CAN_QUEUE_MESSAGE), STRINGIZE(RPC_API_OPEN_CAN_QUEUE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_OPEN_CAN_QUEUE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_CMD, 0, RPCFunc_SetCANAcceptanceWindows, sizeof(RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_FLUSH_CAN_QUEUE_CMD, 0, RPCFunc_FlushCanQueue, sizeof(RPC_API_FLUSH_CAN_QUEUE_MESSAGE), sizeof(RPC_API_FLUSH_CAN_QUEUE_MESSAGE), STRINGIZE(RPC_API_FLUSH_CAN_QUEUE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_FLUSH_CAN_QUEUE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_READ_CAN_QUEUE_CMD, 0, RPCFunc_ReadCANQueue, sizeof(RPC_API_READ_CAN_QUEUE_MESSAGE), sizeof(RPC_API_READ_CAN_QUEUE_MESSAGE), STRINGIZE(RPC_API_READ_CAN_QUEUE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_READ_CAN_QUEUE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_TRANSMIT_CAN_QUEUE_CMD, 0, RPCFunc_TransmitCANQueue, sizeof(RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CLOSE_CAN_QUEUE_CMD, 0, RPCFunc_CloseCANQueue, sizeof(RPC_API_CLOSE_CAN_QUEUE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_CLOSE_CAN_QUEUE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CLOSE_CAN_QUEUE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_CAN_ERR_CMD, 0, RPCFunc_SetCanErr, sizeof(RPC_API_SET_CAN_ERR_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_CAN_ERR_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_CAN_ERR_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_CAN_ERR_SIGNAL_NAME_CMD, 0, RPCFunc_SetCanErrSignalName, sizeof(RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CLEAR_CAN_ERR_CMD, 0, RPCFunc_ClearCanErr, sizeof(RPC_API_CLEAR_CAN_ERR_MESSAGE), sizeof(RPC_API_CLEAR_CAN_ERR_MESSAGE), STRINGIZE(RPC_API_CLEAR_CAN_ERR_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CLEAR_CAN_ERR_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_CAN_SIGNAL_CONVERTION_CMD, 0, RPCFunc_SetCanSignalConvertion, sizeof(RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_RESET_CAN_SIGNAL_CONVERTION_CMD, 0, RPCFunc_ResetCanSignalConvertion, sizeof(RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_MEMBERS), STRINGIZE(RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_CMD, 0, RPCFunc_ResetAllCanSignalConvertion, sizeof(RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_MESSAGE), sizeof(RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_MESSAGE), STRINGIZE(RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_MESSAGE_MEMBERS), STRINGIZE(RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_CAN_CHANNEL_COUNT_CMD, 1, RPCFunc_SetCanChannelCount, sizeof(RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE), sizeof(RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE), STRINGIZE(RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE_ACK_MEMBERS));
    // CAN FD
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_TRANSMIT_CAN_FD_CMD, 0, RPCFunc_TransmitCANFd, sizeof(RPC_API_TRANSMIT_CAN_FD_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_TRANSMIT_CAN_FD_MESSAGE_MEMBERS), STRINGIZE(RPC_API_TRANSMIT_CAN_FD_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_OPEN_CAN_FD_QUEUE_CMD, 0, RPCFunc_OpenCanFdQueue, sizeof(RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE), sizeof(RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE), STRINGIZE(RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_READ_CAN_FD_QUEUE_CMD, 0, RPCFunc_ReadCANFdQueue, sizeof(RPC_API_READ_CAN_FD_QUEUE_MESSAGE), sizeof(RPC_API_READ_CAN_FD_QUEUE_MESSAGE), STRINGIZE(RPC_API_READ_CAN_FD_QUEUE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_READ_CAN_FD_QUEUE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_TRANSMIT_CAN_FD_QUEUE_CMD, 0, RPCFunc_TransmitCANFdQueue, sizeof(RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE_ACK_MEMBERS));
    // CAN Recorder
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_START_CAN_RECORDER_CMD, 0, RPCFunc_StartCANRecorder, sizeof(RPC_API_START_CAN_RECORDER_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_START_CAN_RECORDER_MESSAGE_MEMBERS), STRINGIZE(RPC_API_START_CAN_RECORDER_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_STOP_CAN_RECORDER_CMD, 0, RPCFunc_StopCANRecorder, sizeof(RPC_API_STOP_CAN_RECORDER_MESSAGE), sizeof(RPC_API_STOP_CAN_RECORDER_MESSAGE), STRINGIZE(RPC_API_STOP_CAN_RECORDER_MESSAGE_MEMBERS), STRINGIZE(RRPC_API_STOP_CAN_RECORDER_MESSAGE_ACK_MEMBERS));

    return 0;
}


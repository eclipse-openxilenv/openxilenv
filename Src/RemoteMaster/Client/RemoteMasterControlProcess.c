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


#include <malloc.h>

#include "tcb.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ThrowError.h"
#include "Fifos.h"
#include "Message.h"
#include "Scheduler.h"
#include "IniDataBase.h"
#include "ReadCanCfg.h"
#include "EquationList.h"
#include "ScriptMessageFile.h"
#include "DebugInfoDB.h"
#include "ExtProcessReferences.h"

#include "RemoteMasterBlackboard.h"
#include "RemoteMasterOther.h"
#include "RemoteMasterControlProcess.h"

static int InitRemoteMasterControl (void)
{
    return 0;
}

static void TerminateRemoteMasterControl (void)
{
}


static CAN_CARD_INFOS CANCardInfos;
static int ChannelsCanCardType[MAX_CAN_CHANNELS];
static char *ChannelsCanCardName[MAX_CAN_CHANNELS];

int GetCanCardType (int Channel)
{
    if ((Channel < 0) || (Channel >= MAX_CAN_CHANNELS)) return -1;
    return ChannelsCanCardType[Channel];
}

char *GetCanCardName (int Channel)
{
    if ((Channel < 0) || (Channel >= MAX_CAN_CHANNELS)) return "out of range";
    if (ChannelsCanCardName[Channel] != NULL) {
        return ChannelsCanCardName[Channel];
    } else {
        return "not available";
    }
}


void ReadCANCardInfosFromQueue (void)
{
    int x, y;
    int c;

    MESSAGE_HEAD mhead;
    for (x = 0; x < MAX_CAN_CHANNELS; x++) {
        ChannelsCanCardType[x] = -1;
    }
    read_message (&mhead, (char*)&CANCardInfos, sizeof (CANCardInfos));
    c = 0;
    for (x = 0; x < CANCardInfos.NumOfCards; x++) {
        for (y = 0; y < CANCardInfos.Cards[x].NumOfChannels; y++) {
            if (c < MAX_CAN_CHANNELS) {
                ChannelsCanCardType[c] = CANCardInfos.Cards[x].CardType;
                ChannelsCanCardName[c] = CANCardInfos.Cards[x].CANCardString;
            }
            c++;
        }
    }
}


static void *CheckBufferSize (void **Buffer, int *Size, int NeededSize)
{
    if ((*Buffer == NULL) || (*Size < NeededSize)) {
        *Buffer = my_realloc(*Buffer, (size_t)NeededSize);
        *Size = NeededSize;
    }
    return *Buffer;
}

static void CyclicRemoteMasterControl (void)
{
    int LoopCounter;
    int ToHostPCFifoHandle;
    MESSAGE_HEAD mhead;
    static void *Buffer;
    static int BufferSize;

    ToHostPCFifoHandle = GetToHostPCFifoHandle();
    if (ToHostPCFifoHandle >= 0) {
        FIFO_ENTRY_HEADER Header;
        LoopCounter = 0;
        while (CheckFiFo(ToHostPCFifoHandle, &Header) > 0) {
            switch(Header.MessageId) {
            case RM_BLACKBOARD_READ_BBVARI_FROM_INI_CMD:
                {
                    RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ *Message = (RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    rm_req_read_varinfos_from_ini (Message);
                }
                break;
            case RM_BLACKBOARD_WRITE_BBVARI_INFOS_CMD:
                {
                    RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ *Message = (RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    rm_req_write_varinfos_to_ini (Message);
                }
                break;
            case RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_CMD:
                {
                    RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_REQ *Message = (RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    rm_CallObserationCallbackFunction (Message);
                }
                break;
            case RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_CMD:
                {
                    RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_REQ *Message = (RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    AttachRegisterEquationPid (Message->Pid, Message->UniqueNumber);
                }
                break;
            case RM_BLACKBOARD_DETACH_REGISTER_EQUATION_CMD:
                {
                    RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_REQ *Message = (RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    DetachRegisterEquationPid (Message->Pid, Message->UniqueNumber);
                }
                break;
            case RM_ADD_SCRIPT_MESSAGE_CMD:
                {
                    RM_ADD_SCRIPT_MESSAGE_REQ *Message = (RM_ADD_SCRIPT_MESSAGE_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    AddScriptMessageOnlyMessageWindow ((char*)Message + Message->OffsetText);
                }
                break;
            case RM_ERROR_MESSAGE_CMD:
                {
                    RM_ERROR_MESSAGE_REQ *Message = (RM_ERROR_MESSAGE_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    // ther are only RT_INFO_MESSAGE and RT_ERROR possible
                    if (Message->Level != RT_INFO_MESSAGE) Message->Level = RT_ERROR;
                    ThrowErrorWithCycle(Message->Level, Message->Cycle, (char*)Message + Message->OffsetText);
                }
                break;
            case RM_REALTIME_PROCESS_STARTED_CMD:
                {
                    RM_REALTIME_PROCESS_STARTED_REQ *Message = (RM_REALTIME_PROCESS_STARTED_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);;
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    application_update_start_process (Message->Pid, (char*)Message + Message->OffsetProcessName, NULL);

                    rereference_all_vari_from_ini(Message->Pid, 0, 0);

                }
                break;
            case RM_REALTIME_PROCESS_STOPPED_CMD:
                {
                    RM_REALTIME_PROCESS_STOPPED_REQ *Message = (RM_REALTIME_PROCESS_STOPPED_REQ*)CheckBufferSize(&Buffer, &BufferSize, Header.Size);
                    ReadFromFiFo(ToHostPCFifoHandle, &Header, (char*)Message, Header.Size);
                    application_update_terminate_process ((char*)Message + Message->OffsetProcessName, NULL);
                }
                break;

            default:
                ThrowError (1, "got unknowm fifo entry %i with size %i from process %i", Header.MessageId, Header.Size, Header.TramsmiterPid);
                break;
            }
            if (LoopCounter > 100) break;
        }
    }


    while (test_message(&mhead)) {
        switch(mhead.mid) {
        case NEW_CAN_INI:
            remove_message ();
            if (LoadCanConfigAndStart (GetMainFileDescriptor()) == NULL) {
                ThrowError (1, "cannot load and start CAN configuration");
            }
            break;
        case NEW_CAN_ATTACH_CFG_ACK:
            remove_message ();
            LoadCanConfigAndStart (-1);  // only release th lock
            break;

        case RT_FOPEN:
            {
                char SrcFilename[MAX_PATH];
                char DstFilename[MAX_PATH];
                static int Counter;

                // new DestFile
                Counter++;  // that is not thread save!
                PrintFormatToString (DstFilename, sizeof(DstFilename), "/tmp/copy_from_client_tmp_%i", Counter);
                read_message (&mhead, (char *)&SrcFilename, sizeof (SrcFilename));
                SrcFilename[sizeof (SrcFilename) - 1] = 0;
                if (rm_CopyFile(SrcFilename, DstFilename) == 0) {
                    write_message (mhead.pid, RT_FOPEN_ACK, (int)strlen(DstFilename) + 1, DstFilename);
                }
            }
            break;
        case GET_CAN_CARD_INFOS_ACK:
            ReadCANCardInfosFromQueue();
            break;
        case TRIGGER_ACK:
            remove_message ();
            break;
        default:
            remove_message ();
            ThrowError (1, "receive unknown message %i from %i", mhead.mid, (int)mhead.pid);
            break;
        }
    }
}


TASK_CONTROL_BLOCK RemoteMasterControl_Tcb
    = INIT_TASK_COTROL_BLOCK("RemoteMasterControl", INTERN_ASYNC, 10000, CyclicRemoteMasterControl, InitRemoteMasterControl, TerminateRemoteMasterControl, 1024*1024);


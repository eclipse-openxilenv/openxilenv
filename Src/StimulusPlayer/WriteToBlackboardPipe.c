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
#include "Config.h"
#include "SharedDataTypes.h"
#include "ReadFromBlackboardPipe.h"
#include "WriteToBlackboardPipe.h"
#include "StimulusPlayer.h"
#include "TimeStamp.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "Message.h"
#include "Fifos.h"
#include "BlackboardAccess.h"
#ifdef REMOTE_MASTER
#include "RealtimeScheduler.h"
#else
#include "Scheduler.h"
#endif

#define WRITE_PIPE_REQ_FIFO_NAME "WritePipeReqFiFo"
#define WRITE_PIPE_ACK_FIFO_NAME "WritePipeAckFiFo"
#define WRITE_PIPE_DATA_FIFO_NAME "WritePipeDataFiFo"

typedef struct {
    FIFO_ENTRY_HEADER Header;
    VARI_IN_PIPE *vari_message;
    int32_t *wrpipe_vids;
    int variable_count;
    int max_variable_count;
    uint64_t time_offset;
    int wait;
    int status;
    uint64_t save_timestamp;
    int flush_message_queue;
    int tr_vid;
    double triggervalue;
    PID send_ack_pid;
    int suspend;
    uint64_t suspend_ts;
    int last_data_message;   // last data message
} WRPipeData;

static int ControlReqFiFo;
static int DataFiFo;

static int WriteToFiFoOrMessageQueue(uint32_t MessageId, int Size, void *Data)
{
    int FiFoId;
    int TxPid = GET_PID();

	FiFoId = TxAttachFiFo(TxPid, WRITE_PIPE_ACK_FIFO_NAME);
    if (FiFoId > 0) {
        return WriteToFiFo(FiFoId, MessageId, TxPid, get_timestamp_counter(), Size, Data);
    } else {
        return -1;
    }
}

static void StopStimuli(WRPipeData *Data)
{
    int x;
    if (Data->status == WRPIPE_PLAY_STATUS) {
        /* empty message queue at next cycle */
        Data->flush_message_queue = 1;
        Data->status = WRPIPE_SLEEP_STATUS;
        if (WriteToFiFoOrMessageQueue (HDPLAY_STOP_MESSAGE, 0, NULL)) {
            ThrowError(1, "cannot transmit HDPLAY_STOP_MESSAGE to fifo");
        }
        /* wait that there is no more cycles */
        Data->wait = 0;
    }
    for (x = 0; x < Data->variable_count; x++) {
        if (Data->wrpipe_vids[x] > 0) remove_bbvari (Data->wrpipe_vids[x]);
        Data->wrpipe_vids[x] = 0;
    }
    if (Data->tr_vid > 0) {
        remove_bbvari (Data->tr_vid);
        Data->tr_vid = 0L;
    }

}

static VID vid_hdplay_suspend;

void cyclic_wrpipe (void)
{
    int varicount, x;
    TRIGGER_INFO_MESS trigg_info;
    double refvalue;

    static WRPipeData Data;

    if ((Data.status == WRPIPE_WAIT_TRIGGER_SMALLER) || (Data.status == WRPIPE_WAIT_TRIGGER_LARGER)) {
        if (Data.tr_vid > 0) {
            refvalue = read_bbvari_convert_double (Data.tr_vid);
            if ((Data.status == WRPIPE_WAIT_TRIGGER_SMALLER) && (refvalue < Data.triggervalue))
                Data.status = WRPIPE_WAIT_TRIGGER_LARGER;
            else if ((Data.status == WRPIPE_WAIT_TRIGGER_LARGER) && (refvalue > Data.triggervalue)) {
                Data.status = WRPIPE_PLAY_STATUS;
                Data.time_offset = get_timestamp_counter ();
                if (WriteToFiFoOrMessageQueue (HDPLAY_TRIGGER_MESSAGE, 0, NULL)) {
                    ThrowError(1, "cannot transmit HDPLAY_TRIGGER_MESSAGE to fifo");
                }
            }
        } else {
            Data.status = WRPIPE_PLAY_STATUS;
            Data.time_offset = get_timestamp_counter ();
            if (WriteToFiFoOrMessageQueue (HDPLAY_TRIGGER_MESSAGE, 0, NULL)) {
                ThrowError(1, "cannot transmit HDPLAY_TRIGGER_MESSAGE to fifo");
            }
        }
    }

    if (Data.send_ack_pid) {
        if (WriteToFiFoOrMessageQueue (WRPIPE_ACK, 0, NULL)) {
            ThrowError(1, "cannot transmit WRPIPE_ACK to fifo");
        }
        Data.send_ack_pid = 0;
    }
    if (Data.flush_message_queue) {
        for (x = 0; x < 50; x++) { /* max. 50 messages at one cycles */
            if (CheckFiFo(DataFiFo, &(Data.Header)) > 0) {
                RemoveOneMessageFromFiFo (DataFiFo);
            } else {
                Data.flush_message_queue = 0;    /* Queue is empty */
                break;
            }
        }
    }
    if (Data.status == WRPIPE_PLAY_STATUS) {
        /* There are no old message, read new message */
        if (!Data.wait) {
__READ_NEXT_MESSAGE:
            if (ReadFromFiFo (DataFiFo, &(Data.Header), (char*)(Data.vari_message), sizeof (struct PIPE_VARI) * Data.max_variable_count) == 0) {
                if (Data.Header.MessageId == PLAY_DATA_MESSAGE) {
					Data.wait = 1;
				} else if (Data.Header.MessageId == PLAY_DATA_MESSAGE_PING) {
					Data.wait = 1;
                    // response with a ping
                    if (WriteToFiFoOrMessageQueue (PLAY_DATA_MESSAGE_PING, sizeof(Data.Header.Timestamp), &(Data.Header.Timestamp))) {
                        ThrowError(1, "cannot transmit PLAY_DATA_MESSAGE_PING to fifo");
                    }
                } else if (Data.Header.MessageId == PLAY_NO_MORE_DATA_MESSAGE) {
                    StopStimuli (&Data);
				} else {
                    ThrowError (1, "expect a data message");
				}
            }
            Data.save_timestamp = Data.Header.Timestamp;
        }
        /* check if player was suspended */
        if (read_bbvari_ubyte (vid_hdplay_suspend)) {
            if (!Data.suspend) {
                Data.suspend_ts = get_timestamp_counter ();
                Data.suspend = 1;
            } else {
                Data.time_offset += get_timestamp_counter () - Data.suspend_ts;
                Data.suspend_ts = get_timestamp_counter ();
            }
        } else {
            Data.suspend = 0;
        }
        /* processing message in current time slot ? */
        if ((Data.wait) && (!Data.suspend) &&
            TSCOUNT_LARGER_TS (get_timestamp_counter (), Data.save_timestamp + Data.time_offset)) {
            Data.wait = 0;
            varicount = Data.Header.Size / (int)sizeof (struct PIPE_VARI);
            for (x = 0; x < varicount; x++) {
                if (Data.vari_message[x].vid > 0) {
                    /* Variable found, write to blackboard */
                    write_bbvari_union (Data.vari_message[x].vid, Data.vari_message[x].value);
                }
            }
            goto __READ_NEXT_MESSAGE;
        }
    }
    if (CheckFiFo(ControlReqFiFo, &Data.Header) > 0) {
        switch (Data.Header.MessageId) {
            case SEND_VIDS_MESSAGE:
                /* If there are old VID frames give them free */
                for (x = 0; x < Data.variable_count; x++) {
                    if (Data.wrpipe_vids[x] > 0) {
                        remove_bbvari (Data.wrpipe_vids[x]);
                        Data.wrpipe_vids[x] = 0;
                    }
                }
                Data.variable_count = (int)(Data.Header.Size / sizeof(int));
                if (Data.variable_count > Data.max_variable_count) {
                    Data.max_variable_count = Data.variable_count;
                    Data.wrpipe_vids = (int*)my_realloc(Data.wrpipe_vids, Data.max_variable_count * sizeof(int));
                    Data.vari_message = (VARI_IN_PIPE*)my_realloc(Data.vari_message, Data.max_variable_count * sizeof(VARI_IN_PIPE));
                    if ((Data.wrpipe_vids == NULL) || (Data.vari_message == NULL)) {
                        ThrowError(1, "out of memory");
                        break;
                    }
                }
                /* Read variable ID's */
                ReadFromFiFo (ControlReqFiFo, &Data.Header, (char*)Data.wrpipe_vids, sizeof (int32_t) * Data.max_variable_count);

                for (x = 0; x < Data.variable_count; x++) {
                    if (Data.wrpipe_vids[x] > 0) attach_bbvari (Data.wrpipe_vids[x]);
                }
                Data.status = WRPIPE_BUILD_FRAME_STATUS;
                break;
            case START_WRPIPE_MESSAGE:
                /* Read current time and store time offset */
                RemoveOneMessageFromFiFo (ControlReqFiFo);
                if (Data.status == WRPIPE_DEFINE_TRIGGER)
                    Data.status = WRPIPE_WAIT_TRIGGER_SMALLER;
                else {
                    Data.status = WRPIPE_PLAY_STATUS;
                    Data.time_offset = get_timestamp_counter ();
                }
                Data.send_ack_pid = Data.Header.TramsmiterPid; /* Transmit ACK inside next cycle to PID */
                break;
            case STOP_WRPIPE_MESSAGE:
                /* setze Zeitoffset auf max. Wert */
                RemoveOneMessageFromFiFo (ControlReqFiFo);
                StopStimuli (&Data);
                break;
            case TRIGGER_INFO_MESSAGE:
                ReadFromFiFo (ControlReqFiFo, &Data.Header, (char*)&trigg_info, sizeof (TRIGGER_INFO_MESS));
                Data.tr_vid = trigg_info.trigger_vid;
                if (Data.tr_vid > 0) attach_bbvari (Data.tr_vid);
                Data.triggervalue = trigg_info.trigger_value;
                Data.status = WRPIPE_DEFINE_TRIGGER;
                /* report to hdplay prozess that wrpipe prozess has get all data */
                if (WriteToFiFoOrMessageQueue (HDPLAY_ACK_MESSAGE, 0, NULL)) {
                    ThrowError(1, "cannot transmit HDPLAY_ACK_MESSAGE to fifo");
                }
                break;
            default:
                ThrowError (1, "unknown message %i send by %i",
                       Data.Header.MessageId, Data.Header.TramsmiterPid);
                RemoveOneMessageFromFiFo (ControlReqFiFo);
        }
    }
}

int init_wrpipe (void)
{
    char Name[BBVARI_NAME_SIZE];
    ControlReqFiFo = CreateNewRxFifo (GET_PID(), 128 * 1024, WRITE_PIPE_REQ_FIFO_NAME);
    DataFiFo = CreateNewRxFifo (GET_PID(), 1024 * 1024, WRITE_PIPE_DATA_FIFO_NAME);
    vid_hdplay_suspend = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_STIMULUS_PLAYER, ".suspend", Name, sizeof(Name)),
                                     BB_UBYTE, "[]");
    return 0;
}

void terminate_wrpipe (void)
{
    DeleteFiFo (ControlReqFiFo, GET_PID());
    DeleteFiFo (DataFiFo, GET_PID());
    remove_bbvari (vid_hdplay_suspend);
}

TASK_CONTROL_BLOCK wrpipe_tcb
#ifdef REMOTE_MASTER
    = INIT_TASK_COTROL_BLOCK(STIMULI_QUEUE, INTERN_SYNC, 21, cyclic_wrpipe, init_wrpipe, terminate_wrpipe, 262144);
#else
    = INIT_TASK_COTROL_BLOCK(STIMULI_QUEUE, INTERN_ASYNC, 21, cyclic_wrpipe, init_wrpipe, terminate_wrpipe, 262144);
#endif


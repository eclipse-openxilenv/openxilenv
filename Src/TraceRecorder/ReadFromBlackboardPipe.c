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

#include "Scheduler.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Message.h"
#include "Fifos.h"
#include "BlackboardAccess.h"
#ifdef REMOTE_MASTER
#include "RealtimeScheduler.h"
#endif

PIPE_FRAME rdpipe_frames[MAX_FIFOS];
int frame_count;
struct PIPE_VARI *msgbuffer;
int buffcount;

void analyze_message (void)
{
    int x, f, i;
    FIFO_ENTRY_HEADER Header;
    int mvaricount;
    TRIGGER_INFO_MESS trigg_info;

    for (f = 0; f < MAX_FIFOS; f++) {
        if (CheckFiFo (rdpipe_frames[f].ReqFiFo, &Header) > 0) {
            switch (Header.MessageId) {
            case RDPIPE_LOGON:
                if (f == RECORDER_FIFO_NR) {
                    rdpipe_frames[f].AckFiFo = TxAttachFiFo(GET_PID(), READ_PIPE_RECORDER_ACK_FIFO_NAME);
                    rdpipe_frames[f].DataFiFo = TxAttachFiFo(GET_PID(), READ_PIPE_RECORDER_DATA_FIFO_NAME);
                } else if (f == OSCILLOSCOPE_FIFO_NR) {
                    rdpipe_frames[f].AckFiFo = TxAttachFiFo(GET_PID(), READ_PIPE_OSCILLOSCOPE_ACK_FIFO_NAME);
                    rdpipe_frames[f].DataFiFo = TxAttachFiFo(GET_PID(), READ_PIPE_OSCILLOSCOPE_DATA_FIFO_NAME);
                } else {
                    ThrowError (1, "not a oscilloscope or recorder fifo");
                    break;
                }
                rdpipe_frames[f].pipe_overflow_message = 0;
                rdpipe_frames[f].pid = Header.TramsmiterPid;
                rdpipe_frames[f].wrflag = 0;
                rdpipe_frames[f].vids = (int32_t*)my_malloc (Header.Size);
                rdpipe_frames[f].dec_phys_mask = (int8_t*)my_calloc ((size_t)(Header.Size/4), 1);
                ReadFromFiFo (rdpipe_frames[f].ReqFiFo, &Header, (char*)rdpipe_frames[f].vids, Header.Size);
                for (x = 0; x < (int)Header.Size / (int)sizeof (int32_t); x++)
                    attach_bbvari_unknown_wait (rdpipe_frames[f].vids[x]);
                rdpipe_frames[f].trigg_flag = 1;  /* If the trigger variable would not be trnsmited */
                rdpipe_frames[f].trigg_status = 0;
                rdpipe_frames[f].trigg_vid = 0L;
                rdpipe_frames[f].cycle_count = 0L;
                rdpipe_frames[f].max_cycle = 0xFFFFFFFFL;
                if (buffcount < (int)Header.Size / (int)sizeof (int32_t)) {
                    buffcount = Header.Size / (int)sizeof (int32_t);
                    if (msgbuffer != NULL)
                        my_free (msgbuffer);
                    msgbuffer = (struct PIPE_VARI*)my_malloc ((size_t)buffcount * sizeof (struct PIPE_VARI));
                }
                break;
            case RDPIPE_DEC_PHYS_MASK:
                ReadFromFiFo (rdpipe_frames[f].ReqFiFo, &Header, (char*)rdpipe_frames[f].dec_phys_mask, Header.Size);
                break;
            case RDPIPE_SEND_TRIGGER:
                if (Header.Size == sizeof (TRIGGER_INFO_MESS)) {
                    ReadFromFiFo (rdpipe_frames[f].ReqFiFo, &Header, (char*)&trigg_info, sizeof (TRIGGER_INFO_MESS));
                    rdpipe_frames[f].trigg_vid = trigg_info.trigger_vid;
                    attach_bbvari_unknown_wait (rdpipe_frames[f].trigg_vid);
                    rdpipe_frames[f].trigger_value = trigg_info.trigger_value;
                    rdpipe_frames[f].trigg_flag = 0;
                    rdpipe_frames[f].trigg_status = 0;
                }
                break;
            case RDPIPE_SEND_LENGTH:
                ReadFromFiFo (rdpipe_frames[f].ReqFiFo, &Header, (char*)&(rdpipe_frames[f].max_cycle), sizeof (uint32_t));
                break;
            case RDPIPE_SEND_WRFLAG:
                if (Header.Size == sizeof (int64_t)) {
                    ReadFromFiFo (rdpipe_frames[f].ReqFiFo, &Header, (char*)&rdpipe_frames[f].wrflag, sizeof (int64_t));
                }
                /* Read all variables which are include inside the frame from the blackboard */
                /* without checking the write flag (first line inside the recording file */
                /* and write this into the message queue (if trigger is not active) */
                if (rdpipe_frames[f].trigg_flag) {
                    mvaricount = read_frame (&rdpipe_frames[f]);
                    if (WriteToFiFo (rdpipe_frames[f].DataFiFo, RECORD_MESSAGE, GET_PID(), get_timestamp_counter(),
                        mvaricount * (int)sizeof (struct PIPE_VARI), msgbuffer)) {
                        ThrowError (3, "pipe overflow 1");
                    }
                }
                WriteToFiFo (rdpipe_frames[f].AckFiFo, RDPIPE_ACK, GET_PID(), get_timestamp_counter(), 0, NULL);
                break;
            case RDPIPE_LOGOFF:
                /* Search PID */
                if (Header.Size == 0) {
                    rdpipe_frames[f].pid = -1;
                    if (rdpipe_frames[f].vids != NULL) {
                        for (i = 0; rdpipe_frames[f].vids[i] != 0; i++)
                            remove_bbvari_unknown_wait (rdpipe_frames[f].vids[i]);
                        my_free (rdpipe_frames[f].vids);
                        my_free (rdpipe_frames[f].dec_phys_mask);
                        rdpipe_frames[f].vids = NULL;
                        rdpipe_frames[f].dec_phys_mask = NULL;
                    }
                    if (rdpipe_frames[f].trigg_vid > 0) {
                        remove_bbvari_unknown_wait (rdpipe_frames[f].trigg_vid);
                        rdpipe_frames[f].trigg_vid = 0L;
                    }
                    rdpipe_frames[f].wrflag = 0;
                    RemoveOneMessageFromFiFo (rdpipe_frames[f].ReqFiFo);
                }
                break;
            default:
                {
                    ThrowError (1, "unknown message %i send from %li, my pid = %li", Header.MessageId, Header.TramsmiterPid, GET_PID());
                    RemoveOneMessageFromFiFo (rdpipe_frames[f].ReqFiFo);
                }
            }
        }
    }
}

int test_read_frame (PIPE_FRAME *pframe)
{
    int i = 0;
    int fnr;

    for (fnr = 0; pframe->vids[fnr] > 0; fnr++) {
        if (test_wrflag (pframe->vids[fnr], pframe->wrflag)) {
            reset_wrflag (pframe->vids[fnr], pframe->wrflag);
            if (pframe->dec_phys_mask[fnr] &&
                (get_bbvari_conversiontype (pframe->vids[fnr]) == 1)) {
                msgbuffer[i].value.d = read_bbvari_equ (pframe->vids[fnr]);
                msgbuffer[i].vid = pframe->vids[fnr];
                msgbuffer[i].type = BB_DOUBLE;
            } else {
                msgbuffer[i].value = read_bbvari_union (pframe->vids[fnr]);
                msgbuffer[i].vid = pframe->vids[fnr];
                msgbuffer[i].type = get_bbvaritype (pframe->vids[fnr]);
            }
            i++;
        }
    }
    return i;
}

int read_frame (PIPE_FRAME *pframe)
{
    int type, fnr;

    for (fnr = 0; pframe->vids[fnr] > 0; fnr++) {
        reset_wrflag (pframe->vids[fnr], pframe->wrflag);
        type = get_bbvaritype (pframe->vids[fnr]);
        if ((type != BB_UNKNOWN_WAIT) &&
            pframe->dec_phys_mask[fnr] &&
            (get_bbvari_conversiontype (pframe->vids[fnr]) == 1)) {
            msgbuffer[fnr].value.d = read_bbvari_equ (pframe->vids[fnr]);
            msgbuffer[fnr].vid = pframe->vids[fnr];
            msgbuffer[fnr].type = BB_DOUBLE;
        } else {
            msgbuffer[fnr].value = read_bbvari_union (pframe->vids[fnr]);
            msgbuffer[fnr].vid = pframe->vids[fnr];
            msgbuffer[fnr].type = type;
        }
    }
    return fnr;
}


int trigger_test (PIPE_FRAME *pframe)
{
    double value;

    value = read_bbvari_convert_double (pframe->trigg_vid);
    if (value > pframe->trigger_value) {
        if (rdpipe_frames->trigg_status) return 1;
    } else rdpipe_frames->trigg_status = 1;
    return 0;
}

void cyclic_rdpipe (void)
{
    int f;
    int mvaricount;
    PIPE_FRAME *pframe;

    /* Check if message received */
    analyze_message ();

    /* Blackboard control for each frame one fifo */
    for (f = 0; f < MAX_FIFOS; f++) {
        pframe = &rdpipe_frames[f];
        if (pframe->wrflag != 0) {  /* was write flag transmitted ? */
            if (pframe->trigg_flag) {
                if (pframe->cycle_count++ > pframe->max_cycle) {
                    WriteToFiFo (pframe->DataFiFo, STOP_REC_MESSAGE, GET_PID(), get_timestamp_counter(), 0, NULL);
                } else {
                    /* Only transmit messages if one variable has changed */
                    if ((mvaricount = read_frame (pframe)) > 0) {
                        if (WriteToFiFo (pframe->DataFiFo, RECORD_MESSAGE, GET_PID(), get_timestamp_counter(),
                            mvaricount * (int)sizeof (struct PIPE_VARI), (char*)msgbuffer)) {
                            if (!pframe->pipe_overflow_message) {
                                ThrowError (3, "pipe overflow 2");
                                pframe->pipe_overflow_message = 1;
                            }
                        }
                        pframe->refresh_count = 0;
                    } else if (++(pframe->refresh_count) > 32) {
                        mvaricount = read_frame (pframe);
                        if (WriteToFiFo(pframe->DataFiFo, RECORD_MESSAGE, GET_PID(), get_timestamp_counter(),
                            mvaricount * (int)sizeof (struct PIPE_VARI), (char*)msgbuffer)) {
                            if (!pframe->pipe_overflow_message) {
                                ThrowError (3, "pipe overflow 3");
                                pframe->pipe_overflow_message = 1;
                            }
                        }
                        pframe->refresh_count = 0;
                    }
                }
            } else {
                  if ((pframe->trigg_flag = trigger_test (pframe)) > 0) {
                    mvaricount = read_frame (pframe);
                    if (WriteToFiFo(pframe->DataFiFo, RECORD_MESSAGE, GET_PID(), get_timestamp_counter(),
                        mvaricount * (int)sizeof (struct PIPE_VARI), (char*)msgbuffer)) {
                        if (!pframe->pipe_overflow_message) {
                            ThrowError (3, "pipe overflow 4");
                            pframe->pipe_overflow_message = 1;
                        }
                    }
                    pframe->refresh_count = 0;
                }
            }
        }
    }
}

int init_rdpipe (void)
{
    rdpipe_frames[RECORDER_FIFO_NR].ReqFiFo = CreateNewRxFifo (GET_PID(), 64*1024, READ_PIPE_RECORDER_REQ_FIFO_NAME);
    rdpipe_frames[OSCILLOSCOPE_FIFO_NR].ReqFiFo = CreateNewRxFifo (GET_PID(), 64*1024, READ_PIPE_OSCILLOSCOPE_REQ_FIFO_NAME);
    frame_count = 0;           /* Count of frames = 0 */
    msgbuffer = NULL;          /* There are no message buffer */
    buffcount = 0;
    return 0;
}

void terminate_rdpipe (void)
{
    int f, i;

    for (f = 0; f < MAX_FIFOS; f++) {
        if (rdpipe_frames[f].vids != NULL) {
            for (i = 0; rdpipe_frames[f].vids[i] > 0; i++) {
                remove_bbvari_unknown_wait (rdpipe_frames[f].vids[i]);
            }
            my_free (rdpipe_frames[f].vids);
            rdpipe_frames[f].vids = NULL;
        }
        if (rdpipe_frames[f].dec_phys_mask != NULL) {
            my_free (rdpipe_frames[f].dec_phys_mask);
        }
        rdpipe_frames[f].dec_phys_mask = NULL;
        DeleteFiFo(rdpipe_frames[f].ReqFiFo, GET_PID());
    }
    if (msgbuffer != NULL) my_free (msgbuffer);
}

TASK_CONTROL_BLOCK rdpipe_tcb =
#ifdef REMOTE_MASTER
        INIT_TASK_COTROL_BLOCK (TRACE_QUEUE, INTERN_SYNC, 210, cyclic_rdpipe, init_rdpipe, terminate_rdpipe, 256*1024);
#else
        INIT_TASK_COTROL_BLOCK (TRACE_QUEUE, INTERN_ASYNC, 210, cyclic_rdpipe, init_rdpipe, terminate_rdpipe, 256*1024);
#endif

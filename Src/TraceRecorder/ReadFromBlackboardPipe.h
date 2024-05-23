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


#ifndef READFROMBLACKBOARDPIPE_H
#define READFROMBLACKBOARDPIPE_H

#include <stdint.h>
#include <stdlib.h>
#include "tcb.h"

#define MAX_FIFOS     2
#define RECORDER_FIFO_NR        0
#define OSCILLOSCOPE_FIFO_NR    1

#define READ_PIPE_RECORDER_REQ_FIFO_NAME     "ReadPipeRecorderReqFiFo"
#define READ_PIPE_RECORDER_ACK_FIFO_NAME     "ReadPipeRecorderAckFiFo"
#define READ_PIPE_RECORDER_DATA_FIFO_NAME    "ReadPipeRecorderDataFiFo"
#define READ_PIPE_OSCILLOSCOPE_REQ_FIFO_NAME     "ReadPipeOscilloscopeReqFiFo"
#define READ_PIPE_OSCILLOSCOPE_ACK_FIFO_NAME     "ReadPipeOscilloscopeAckFiFo"
#define READ_PIPE_OSCILLOSCOPE_DATA_FIFO_NAME    "ReadPipeOscilloscopeDataFiFo"


typedef struct {
    int32_t pid;                /*  Process ID of the receiver */
    int32_t Fill2;
    int32_t *vids;              /*  List of the variable IDs */
    int8_t *dec_phys_mask;      /*  List of the physical flags */
    uint64_t wrflag;            /*  The write flag to check if someboady has written to the blackboard variables */
    int32_t trigg_vid;          /*  Trigger variable ID */
    double trigger_value;       /*  Trigger level */
    int32_t trigg_flag;         /*  Trigger flag */
    int32_t trigg_status;       /*  Trigger status */
    int32_t refresh_count;      /*  Counter this will incremented each cycle no message is transmitted
                                    otherwise it will be set to 0. If it reachs 32 a complete frame will be transmitted */
    uint32_t cycle_count;
    uint32_t max_cycle;
    int32_t Fill3;

    int ReqFiFo;
    int AckFiFo;
    int DataFiFo;

    int pipe_overflow_message;

} PIPE_FRAME;                /* 64Bytes */

typedef struct {
    double trigger_value;   /* Trigger level */
    int32_t trigger_vid;    /* Trigger variable */
    int32_t Fill1;
} TRIGGER_INFO_MESS;

typedef struct PIPE_VARI {
    int32_t vid;            /* Variable ID */
    int32_t type;           /* Data type (UBYTE, BYTE, WORD,...) */
    union BB_VARI value;    /* union of the value */
} VARI_IN_PIPE;             /* One variable entry inside the message
                               size 16 Bytes */


void analyze_message (void);
int read_frame (PIPE_FRAME *pframe);
int test_read_frame (PIPE_FRAME *pframe);
int trigger_test (PIPE_FRAME *pframe);

/* ------------------------------------------------------------ *\
 *    Task Control Block                                     *
\* ------------------------------------------------------------ */

extern TASK_CONTROL_BLOCK rdpipe_tcb;
#endif


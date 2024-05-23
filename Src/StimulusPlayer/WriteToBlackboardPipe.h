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


#ifndef WRITETOBLACKBOARDPIPE_H
#define WRITETOBLACKBOARDPIPE_H

#include <stdint.h>
#include <stdlib.h>
#include "tcb.h"

#define WRPIPE_SLEEP_STATUS          0
#define WRPIPE_BUILD_FRAME_STATUS    1
#define WRPIPE_PLAY_STATUS           2
#define WRPIPE_DEFINE_TRIGGER        3
#define WRPIPE_WAIT_TRIGGER_SMALLER  4
#define WRPIPE_WAIT_TRIGGER_LARGER   5

#define WRITE_PIPE_REQ_FIFO_NAME "WritePipeReqFiFo"
#define WRITE_PIPE_ACK_FIFO_NAME "WritePipeAckFiFo"
#define WRITE_PIPE_DATA_FIFO_NAME "WritePipeDataFiFo"

void cyclic_wrpipe (void);
int init_wrpipe (void);
void terminate_wrpipe (void);

extern TASK_CONTROL_BLOCK wrpipe_tcb;

extern struct BB_ELEM **frame_wrpipe;

#endif


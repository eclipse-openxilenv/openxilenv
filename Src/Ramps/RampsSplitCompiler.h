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


#ifndef RAMPSSPLITCOMPILER_H
#define RAMPSSPLITCOMPILER_H

#include <stdlib.h>
#include "Config.h"
#include "tcb.h"
#include "Blackboard.h"
#include "ExecutionStack.h"
#include "RampsStruct.h"

#ifdef RAMPSSPLITCOMPILER_C
#define extern
#endif


#define TRIGGER_STATE_TRIGGER       3
#define TRIGGER_STATE_WAIT_SMALLER  2
#define TRIGGER_STATE_WAIT_LARGER   1
#define TRIGGER_STATE_OFF           0


/* Zustaende des async Rampen-Prozess */
#define WAIT_RAMPEN_FILENAME   0
#define READ_RAMPEN_FILE       1
#define CALCULATE_RAMPEN       3
#define READ_RAMPEN_FILE_DONE  2


extern RAMPEN_INFO *rampen_info_struct;

/* verstrichene Zeit ab der Rampen-Fkt. laufen */
extern double begin_rampen_time
#ifdef extern
= 0.0
#endif /* extern */
;

extern void rm_cyclic_async_rampen (void);
extern void rm_terminate_async_rampen (void);
extern int rm_init_async_rampen (void);

/* Task-Control-Block des Rampenprozesses muss mit eiem Zeiger in
   PROCESSL.H dem Kernel bekannt gemacht werden. */
extern TASK_CONTROL_BLOCK rm_async_rampen_tcb
#ifdef extern
    = INIT_TASK_COTROL_BLOCK("rm_" SIGNAL_GENERATOR_COMPILER, INTERN_ASYNC, 30, rm_cyclic_async_rampen, rm_init_async_rampen, rm_terminate_async_rampen, 1024)
#endif
;

/* Zeiger auf Blackboard-Zugriffs-ID */
extern int *vids_rampen;
extern int vid_state_rampen;

#undef extern
#endif


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


#pragma once

#include "RemoteMasterLock.h"

typedef struct {
    int size;              /* Size of the message buffer */
    int rp;                /* Current read position */
    int wp;                /* Current write position */
    int count;             /* Number of messages inside the queue */
    char *buffer;          /* ZPointer to the memory storing the messages */
    REMOTE_MASTER_LOCK SpinLock;
} MESSAGE_BUFFER;          /* Input message queue of an internal process */

typedef struct {
    int Nr;
    void *ExecStack;
} PROC_EXEC_STACK;

typedef struct TCB {                 // Task control block
    int pid;                         // Process ID
    char name[512];                  // Process name
    int Type;                        // Process type
//#define INTERN_ASYNC      0 // internal process
#define INTERN_SYNC       1 // internal realtime process
#define EXTERN_ASYNC      2 // external process
#define REALTIME_PROCESS  3 // interner realtime Prozess

    int active;                     // Flag is set if process is running
    int state;                      // Process state
    int prio;                       // Execution order
    void(*cyclic) (void);           // Cyclic function
    int(*init) (void);              // Init function
    void(*terminat) (void);         // Terminate function
    int delay;
    int time_counter;                // Cycle counter
    int time_steps;                  // Cycle divider
    int message_size;                // Size of the message queue
    MESSAGE_BUFFER *message_buffer;  // Pointer to the message queue (if NULL process have no queue)
    struct TCB *next;                // Pointer to the next process (if its addd to an scheduler)
    struct TCB *prev;                // Pointer to the previous process (if its addd to an scheduler)

    int assigned_scheduler;
    int should_be_stopped;

    uint64_t bb_mask;

    uint64_t call_count;
    uint64_t CycleTime;
    uint64_t CycleTimeMin;
    uint64_t CycleTimeMax;

    int VidRuntime;

    int BeforeProcessEquationCounter;
    PROC_EXEC_STACK *BeforeProcessEquations;
    int BehindProcessEquationCounter;
    PROC_EXEC_STACK *BehindProcessEquations;

} TASK_CONTROL_BLOCK;

#define INIT_TASK_COTROL_BLOCK(name, type, priority, cyclic_function, init_function, terminate_function, message_queue_size) \
      { \
        -1,                       /* Process is not running */ \
        name,                     /* Process name */ \
        type,                     /* Process type */ \
        0,                        /* Process sleepingt = 0 */ \
        0,                        /* Process state = 0 */ \
        priority,                 /* Execution order */ \
        cyclic_function, \
        init_function, \
        terminate_function, \
        0,                        /* Delay */ \
        0,                        /* timecounter starts with 0 */ \
        1,                        /* Call process in each time slot */ \
        message_queue_size,       /* Size of the message queue */ \
        NULL,                     /* Pointer to the message queue */ \
        NULL,                     /* Pointer to the next process (if its addd to an scheduler) */ \
        NULL,                     /* Pointer to the previous process (if its addd to an scheduler) */ \
        0,                        /* assigned_scheduler */ \
        0,                        /* should_be_stopped */ \
	    0,                        /* bb_mask */ \
		0,                        /* call_count; */ \
		0,                        /* CycleTime */ \
		UINT64_MAX,               /* CycleTimeMin */ \
		0,                        /*  CycleTimeMax */ \
        0,                        /* VidRuntime */ \
        0,                        /* BeforeProcessEquationCounter */ \
        NULL,                     /* BeforeProcessEquations */ \
        0,                        /* BehindProcessEquationCounter */ \
        NULL                      /* BehindProcessEquations */ \
    }


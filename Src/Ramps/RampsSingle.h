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


#ifndef RAMPSSINGLE_H
#define RAMPSSINGLE_H

#include <stdlib.h>
#include "Config.h"
#include "tcb.h"
#include "Blackboard.h"
#include "ExecutionStack.h"

#ifdef RAMPSSINGLE_C
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

typedef struct {
    char name[BBVARI_NAME_SIZE];
    int type;
    #define RAMPE_NORMAL   0
    #define RAMPE_ADDIEREN 1
    struct EXEC_STACK_ELEM **ampl_exec_stack;
    struct EXEC_STACK_ELEM **time_exec_stack;
    double *ampl;
    double *time;
    double steig;
    double dt;
    int count;
    int size;
    int state;
#define RAMPE_WARTET       0
#define RAMPE_LAEUFT       1
#define RAMPE_WIRD_BEENDET 3
#define RAMPE_BEENDET      4
} RAMPEN_ELEM;        /* 38 Bytes */

#define MAX_RAMPEN 100

typedef struct {
    int anz_rampen;
    int trigger_flag;
    int trigger_state;
    NAME_STRING trigger_var;
    double trigger_schw;
    RAMPEN_ELEM rampen[MAX_RAMPEN];     /* max 100 Rampen */
    long vids[MAX_RAMPEN+3];
    unsigned long max_loops;
    unsigned long loop_count;
    /* verstrichene Zeit ab der Rampen-Fkt. laufen */
    double begin_rampen_time;
    /* Zeiger auf Blackboard-Zugriffs-ID */

} RAMPEN_INFO;             /* mit MAX_RAMPEN == 50  1912 Bytes */

//extern RAMPEN_INFO *rampen_info_struct;

extern void cyclic_async_rampen (void);
extern void terminate_async_rampen (void);
extern int init_async_rampen (void);

/* zyklisch aufzurufende Funktion des Testprozesses */
extern void cyclic_rampen (void);

/* Init.-Funktion des Testprozesses setzt globale Variable
   auf Anfangswerte, meldet seine Variable beim Blackboard an*/
extern int init_rampen (void);

/* terminate Funktion des Testprozesses, wird beim Beenden des
   Testprozesses aufgerufen */
extern void terminate_rampen (void);

/* Task-Control-Block des Rampenprozesses muss mit eiem Zeiger in
   PROCESSL.H dem Kernel bekannt gemacht werden. */
extern TASK_CONTROL_BLOCK sync_rampen_tcb
#ifdef extern
    = INIT_TASK_COTROL_BLOCK(SIGNAL_GENERATOR_CALCULATOR, INTERN_ASYNC, 30, cyclic_rampen, init_rampen, terminate_rampen, 1024)
#endif
;

/* Task-Control-Block des Rampenprozesses muss mit eiem Zeiger in
   PROCESSL.H dem Kernel bekannt gemacht werden. */
extern TASK_CONTROL_BLOCK async_rampen_tcb
#ifdef extern
    = INIT_TASK_COTROL_BLOCK(SIGNAL_GENERATOR_COMPILER, INTERN_ASYNC, 30, cyclic_async_rampen, init_async_rampen, terminate_async_rampen, 1024)
#endif
;


#undef extern
#endif


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


#ifndef RAMPSSPLITEXECUTE_H
#define RAMPSSPLITEXECUTE_H

#include <stdlib.h>
#include "Config.h"
#include "tcb.h"

#ifdef RAMPSSPLITEXECUTE_C
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
= {
    -1,                       /* Prozess nicht aktiv */
    SIGNAL_GENERATOR_CALCULATOR, /* Prozess-Name */
     1, //INTERN_ASYNC,            /* es handelt sich um einen SYNC-Prozess */
     0,                       /* Prozess Schlaeft = 0 */
     0,
     31,                      /* Prioritaet */
     cyclic_rampen,
     init_rampen,
     terminate_rampen,
     0,                       /* Timeout */
     0,                       /* timecounter zu Anfang 0 */
     1,                       /* Prozess in jedem Zeittakt aufrufen */
     131072,                     /* 128K Byte Message queue */
     NULL,
     NULL,
     NULL
}
#endif
;

#undef extern
#endif


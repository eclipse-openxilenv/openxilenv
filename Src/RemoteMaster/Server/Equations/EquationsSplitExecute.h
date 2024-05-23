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


#ifndef EQUATIONSSPLITEXECUTE_H
#define EQUATIONSSPLITEXECUTE_H

#include <stdlib.h>
#include "Config.h"
#include "tcb.h"

#ifdef EQUATIONSSPLITEXECUTE_C
#define extern
#endif


extern void cyclic_trigger (void);
extern int init_trigger (void);
extern void terminate_trigger (void);


/* Task-Control-Block des Triggerprozesses muss mit eiem Zeiger in
   PROCESSL.H dem Kernel bekannt gemacht werden. */
extern TASK_CONTROL_BLOCK sync_trigger_tcb
#ifdef extern
= {
    -1,                       /* Prozess nicht aktiv */
    EQUATION_CALCULATOR,      /* Prozess-Name */
     1,                       /* es handelt sich um einen SYNC-Prozess */
     0,                       /* Prozess Schlaeft = 0 */
     0,
     41,                      /* Prioritaet */
     cyclic_trigger,
     init_trigger,
     terminate_trigger,
     0,                       /* Timeout */
     0,                       /* timecounter zu Anfang 0 */
     1,                       /* Prozess in jedem Zeittakt aufrufen */
     10*1024,                 /* 10KByte Messagequeue */
     NULL,
     NULL,
     NULL
}
#endif
;

#undef extern
#endif


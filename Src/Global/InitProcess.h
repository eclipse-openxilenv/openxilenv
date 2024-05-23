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


#ifndef INITPROCESS_H
#define INITPROCESS_H

#include <stdlib.h>
#include "tcb.h"
#include "Blackboard.h"

#ifdef INITPROCESS_C
#define extern
#endif

int write_process_list2ini (void);
int SetStartupScript (char *par_StartupScript);
int SetRemoteMasterExecutableCmdLine (char *par_RemoteMasterExecutable);
char *GetRemoteMasterExecutableCmdLine (void);

int StartTerminateScript (void);
void SetCallFromProcess (char *par_CallFromProcess);

int AddExternProcessTerminateOrResetBBVariable (int pid);
int RemoveExternProcessTerminateOrResetBBVariable (int pid);

void InitInitCriticalSection (void);

extern void cyclic_init (void);
extern int init_init (void);
extern void terminate_init (void);

extern TASK_CONTROL_BLOCK init_tcb
#ifdef extern
= INIT_TASK_COTROL_BLOCK("init", INTERN_ASYNC,  1, cyclic_init,init_init, terminate_init, 1024);
#endif
;

#undef extern
#endif


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


#ifndef RPCCONTROLPROCESS_H
#define RPCCONTROLPROCESS_H

#include "tcb.h"

struct CAN_RECORDER;

void SetDelayFunc (void *par_Connection, uint32_t par_Delay,
                   void (*par_BeforeFuncPtr) (void* Param), void *par_BeforeParm,
                   void (*par_BehindFuncPtr) (int TimeoutFlag, uint32_t Remainder, void* Param), void *par_BehindParm,
                   int par_WaitingForMsgId, char *Equation);

void CleanUpWaitFuncForConnection(void *par_Connection);

int GetRPCControlPid (void);

void InitRPCControlWaitStruct(void);

int AddCanRecorder(struct CAN_RECORDER *par_CanRecorders);


/* ------------------------------------------------------------ *\
 *    Task Control Block                                     *
\* ------------------------------------------------------------ */

extern TASK_CONTROL_BLOCK rpc_controll_tcb;
#endif

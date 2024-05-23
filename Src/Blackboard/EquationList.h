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


#ifndef EQUATIONLIST_H
#define EQUATIONLIST_H

#include <stdint.h>
#include "Platform.h"
#include "Wildcards.h"
#include "ExecutionStack.h"

int RegisterEquation (int Pid, const char *Equation, struct EXEC_STACK_ELEM *ExecStack, const char *AdditionalInfos,
                     uint32_t TypeFlags);
#define EQU_TYPE_BLACKBOARD             0x01
#define EQU_TYPE_GLOBAL_EQU_CALCULATOR  0x02 
#define EQU_TYPE_BEFORE_PROCESS         0x04 
#define EQU_TYPE_BEHIND_PROCESS         0x08
#define EQU_TYPE_CAN                    0x10 
#define EQU_TYPE_ANALOG                 0x20
#define EQU_TYPE_RAMPE                  0x40
#define EQU_TYPE_WAIT_UNTIL             0x80


typedef struct {
    uint64_t UniqueNumber;
    uint32_t AttachCounter;
    uint32_t CycleLoaded;
    uint32_t CycleDeleted;
    uint32_t TypeFlags;
    time_t TimeLoaded;
    char *Equation;
    char *AdditionalInfos;
    int32_t Pids[32];
    uint16_t ProcAttachCounters[32];
    int UsedFlag;
} ACTIVE_EQUATION_ELEM;


int AttachRegisterEquation (uint64_t UniqueNumber);

int DetachRegisterEquation (uint64_t UniqueNumber);

int AttachRegisterEquationPid (int Pid, uint64_t UniqueNumber);

int DetachRegisterEquationPid (int Pid, uint64_t UniqueNumber);

char *GetRegisterEquationString (uint64_t UniqueNumber);

void InitEquationList (void);
void TerminateEquationList (void);

int GetActiveEquationCount (void);

int GetActiveEquationSnapShot (INCLUDE_EXCLUDE_FILTER *par_Filter, int par_TypeMask, int *SnapshotSize,
                               ACTIVE_EQUATION_ELEM **ret_SnapShot, int *StringBufferSize, char **ret_StringBuffer);

#endif

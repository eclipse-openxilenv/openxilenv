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


#include <stdint.h>
#include "Platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Config.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#include "MainValues.h"
#include "Files.h"
#include "ExecutionStack.h"
#include "Wildcards.h"
//#define FUNCTION_RUNTIME
#include "RunTimeMeasurement.h"

#include "EquationList.h"

static ACTIVE_EQUATION_ELEM *Equations;
static int SizeOfEquations;
static int LastSearchPosEquations;
static int EquationsCount;

static uint32_t UniqueNumberCounter;

static CRITICAL_SECTION EquListCriticalSection;

static uint64_t GetUniqueNumber (void)
{
    uint64_t Ret;
    //ENTER_CS (&EquListCriticalSection);
    UniqueNumberCounter++;
    Ret = (uint64_t)UniqueNumberCounter << 32;
    //LEAVE_CS (&EquListCriticalSection);
    return Ret;
}

static uint64_t __RegisterEquation (int Pid, const char *Equation, const char *AdditionalInfos, uint32_t TypeFlags)
{
    int x;
    uint64_t Ret;
    int FoundFreePos = -1;

    ENTER_CS (&EquListCriticalSection);

    for (x = LastSearchPosEquations; x < SizeOfEquations; x++) {
        if (Equations[x].UsedFlag == 0) {
            // There is one free element available
            LastSearchPosEquations = x;
            FoundFreePos = x;
        }
    }
    if (FoundFreePos == -1) {
        for (x = 0; x < LastSearchPosEquations; x++) {
            if (Equations[x].UsedFlag == 0) {
                // There is one free element available
                LastSearchPosEquations = x;
                FoundFreePos = x;
            }
        }
    }
    if (FoundFreePos == -1) {
        int SizeOfEquationsSave = SizeOfEquations;
        SizeOfEquations += 100 + (SizeOfEquations >> 2);
        Equations = (ACTIVE_EQUATION_ELEM*) my_realloc (Equations, (size_t)SizeOfEquations * sizeof (ACTIVE_EQUATION_ELEM));
        if (Equations == NULL) {
            ThrowError (ERROR_SYSTEM_CRITICAL, "out of memory stop!");
            return 0;
        }
        for (x = SizeOfEquationsSave; x < SizeOfEquations; x++) {
            Equations[x].UsedFlag = 0;
            Equations[x].Equation = NULL;
            Equations[x].AdditionalInfos = NULL;
        }
        FoundFreePos = SizeOfEquationsSave;
    }
    EquationsCount++;
    Equations[FoundFreePos].UsedFlag = 1;
    Equations[FoundFreePos].UniqueNumber = Ret = GetUniqueNumber() + (uint64_t)FoundFreePos;
    Equations[FoundFreePos].AttachCounter = 1;
    MEMSET (&Equations[FoundFreePos].Pids, 0, sizeof (Equations[FoundFreePos].Pids));
    if (Pid) {
        Equations[FoundFreePos].Pids[0] = Pid;
        Equations[FoundFreePos].ProcAttachCounters[0] = 1;
    }
    time (&Equations[FoundFreePos].TimeLoaded);
    Equations[FoundFreePos].CycleLoaded = read_bbvari_udword (CycleCounterVid);
    Equations[FoundFreePos].TypeFlags = TypeFlags;
    Equations[FoundFreePos].Equation = StringMalloc (Equation);
    if (Equations[FoundFreePos].Equation == NULL) {
        LEAVE_CS (&EquListCriticalSection);
        ThrowError (ERROR_SYSTEM_CRITICAL, "out of memory stop!");
        return 0;
    }
    Equations[FoundFreePos].AdditionalInfos = NULL;
    if (AdditionalInfos != NULL) {
        if (strlen (AdditionalInfos)) {
            Equations[FoundFreePos].AdditionalInfos = StringMalloc (AdditionalInfos);
            if (Equations[FoundFreePos].AdditionalInfos == NULL) {
                LEAVE_CS (&EquListCriticalSection);
                ThrowError (ERROR_SYSTEM_CRITICAL, "out of memory stop!");
                return 0;
            }
        } else {
            Equations[FoundFreePos].AdditionalInfos = NULL;
        }
    }
    LEAVE_CS (&EquListCriticalSection);
    return Ret;
}


int RegisterEquation (int Pid, const char *Equation, struct EXEC_STACK_ELEM *ExecStack, const char *AdditionalInfos, uint32_t TypeFlags)
{
    uint64_t UniqueNumber;
    BEGIN_RUNTIME_MEASSUREMENT ("RegisterEquation")
    if (ExecStack != NULL) {
        if (Pid < 0) {
            Pid = GET_PID ();
        }
        UniqueNumber = __RegisterEquation (Pid, Equation, AdditionalInfos, TypeFlags);
        ExecStack->param.unique_number = UniqueNumber;
    }
    END_RUNTIME_MEASSUREMENT
    return 0;
}


int AttachRegisterEquationPid (int Pid, uint64_t UniqueNumber)
{
    int Pos;
    int x, xx = -1;
    int Ret = -1;

    ENTER_CS (&EquListCriticalSection);

    Pos = (int)(UniqueNumber & 0xFFFFFFFFULL);
    if (Pos < SizeOfEquations) {
        if (UniqueNumber == Equations[Pos].UniqueNumber) {
            Equations[Pos].AttachCounter++;
            for (x = 0; x < 32; x++) {
                if (Equations[Pos].Pids[x] == Pid) {
                    Equations[Pos].ProcAttachCounters[x]++;
                    break;
                } else if ((xx == -1) && (Equations[Pos].Pids[x] == 0)) {
                    xx = x;
                }
            }
            if ((xx >= 0) && (x == 32)) {  // Process is not in there
                Equations[Pos].Pids[xx] = Pid;
                Equations[Pos].ProcAttachCounters[xx] = 1;
            }
            Ret = 0;
        }
    }
    LEAVE_CS (&EquListCriticalSection);
    return Ret;
}


int AttachRegisterEquation (uint64_t UniqueNumber)
{
    int Pid = GET_PID();
    return AttachRegisterEquationPid (Pid, UniqueNumber);
}


static void RemoveAllOldEquations (int par_Immediately)
{
    int x;
    uint32_t RefTime = GetTickCount ();
    int Removed = 0;

    for (x = 0; x < SizeOfEquations; x++) {
        if (Equations[x].UsedFlag) {
            if (par_Immediately ||
                ((Equations[x].AttachCounter == 0) && (Equations[x].CycleDeleted < RefTime))) {   // now realy delete it
                my_free(Equations[x].Equation);
                Equations[x].Equation = NULL;
                if (Equations[x].AdditionalInfos != NULL) {
                    my_free(Equations[x].AdditionalInfos);
                    Equations[x].AdditionalInfos = NULL;
                }
                Equations[x].UsedFlag = 0;
                Removed++;
            }
        } else if (Equations[x].Equation != NULL) {
            ThrowError (1, "Internal error, that should never happen %s(%i)", __FILE__, __LINE__);
        }
    }
}

int DetachRegisterEquationPid (int Pid, uint64_t UniqueNumber)
{
    int Pos;
    int x;
    int Ret = -1;

    Pos = (int)(UniqueNumber & 0xFFFFFFFFULL);
    ENTER_CS (&EquListCriticalSection);
    if (Pos < SizeOfEquations) {
        if (UniqueNumber == Equations[Pos].UniqueNumber) {
            if (Equations[Pos].AttachCounter > 0) {
                Equations[Pos].AttachCounter--;
            } 
            if (Equations[Pos].AttachCounter == 0) {
                // This will deleted after 10s (here it will only marked for deletion
                Equations[Pos].CycleDeleted = GetTickCount () + 10000;
                EquationsCount--;
            }
            for (x = 0; x < 32; x++) {
                if (Equations[Pos].Pids[x] == Pid) {
                    if (Equations[Pos].ProcAttachCounters[x] > 0) {
                        Equations[Pos].ProcAttachCounters[x]--;
                    }
                    if (Equations[Pos].ProcAttachCounters[x] == 0) {
                        Equations[Pos].Pids[x] = 0;
                    }
                    Ret = 0;
                    break;
                }
            }
        }
    }
    RemoveAllOldEquations (0);
    LEAVE_CS (&EquListCriticalSection);
    return Ret;
}

int DetachRegisterEquation (uint64_t UniqueNumber)
{
    int16_t Pid = (int16_t)GET_PID();
    return DetachRegisterEquationPid (Pid, UniqueNumber);
}


char *GetRegisterEquationString (uint64_t UniqueNumber)
{
    int Pos = (int)(UniqueNumber & 0xFFFFFFFFULL);
    if (Pos < SizeOfEquations) {
        if (Equations[Pos].UsedFlag && (Equations[Pos].AttachCounter != 0)) {
            if (UniqueNumber == Equations[Pos].UniqueNumber) {
                return Equations[Pos].Equation;
            }
        }
    }
    return NULL;
}

void InitEquationList (void)
{
    INIT_CS (&EquListCriticalSection);
}

void TerminateEquationList (void)
{
    ENTER_CS (&EquListCriticalSection);
    RemoveAllOldEquations(1);
    if (Equations != NULL) my_free(Equations);
    Equations = NULL;
    SizeOfEquations = 0;
    LastSearchPosEquations = 0;
    EquationsCount = 0;
    LEAVE_CS (&EquListCriticalSection);
}

int GetActiveEquationCount (void)
{
    return EquationsCount;
}


int GetActiveEquationSnapShot (INCLUDE_EXCLUDE_FILTER *par_Filter, int par_TypeMask, int *SnapshotSize,
                               ACTIVE_EQUATION_ELEM **ret_SnapShot, int *StringBufferSize, char **ret_StringBuffer)
{
    int x, y = 0;
    size_t Len = 0;
    int Pos = 0;
    int Count = 0;

    ENTER_CS (&EquListCriticalSection);
    if (Equations != NULL) {
        for (x = 0; x < SizeOfEquations; x++) {
            if ((Equations[x].UsedFlag != 0) && (Equations[x].AttachCounter > 0)) {
                Count++;
                Len += strlen (Equations[x].Equation) + 1;
                if (Equations[x].AdditionalInfos != NULL) {
                    Len += strlen (Equations[x].AdditionalInfos) + 1;
                }
            }
        }
    }
    if (((*ret_SnapShot == NULL) && (Count > 0)) || (*SnapshotSize < Count)) {
        *SnapshotSize = Count;
        *ret_SnapShot = (ACTIVE_EQUATION_ELEM *)my_realloc (*ret_SnapShot, (size_t)Count * sizeof (ACTIVE_EQUATION_ELEM)+1); // +1 at least 1 byte
    }
    if ((*ret_StringBuffer == NULL) || (*StringBufferSize < (int)Len)) {
        *StringBufferSize = (int)Len;
        *ret_StringBuffer = (char*)my_realloc (*ret_StringBuffer, Len+1);  // +1 at least 1 byte
    }
    if (*ret_StringBuffer != NULL) {
        for (x = 0; x < SizeOfEquations; x++) {
            if ((Equations[x].UsedFlag != 0) && (Equations[x].AttachCounter > 0)) {
                if ((Equations[x].TypeFlags & (uint32_t)par_TypeMask) != 0) {
                    if (CheckIncludeExcludeFilter (Equations[x].Equation, par_Filter)) {
                        (*ret_SnapShot)[y] = Equations[x];
                        Len = strlen (Equations[x].Equation) + 1;
                        MEMCPY (*ret_StringBuffer + Pos, Equations[x].Equation, Len);
                         (*ret_SnapShot)[y].Equation = *ret_StringBuffer + Pos;
                        Pos += (int)Len;
                        (*ret_SnapShot)[y].AdditionalInfos = NULL;
                        if (Equations[x].AdditionalInfos != NULL) {
                            Len = strlen (Equations[x].AdditionalInfos) + 1;
                            MEMCPY (*ret_StringBuffer + Pos, Equations[x].AdditionalInfos, Len);
                            (*ret_SnapShot)[y].AdditionalInfos = *ret_StringBuffer + Pos;
                            Pos += (int)Len;
                        }
                        y++;
                    }
                }
            }
        }
    }
    LEAVE_CS (&EquListCriticalSection);
    return y;
}



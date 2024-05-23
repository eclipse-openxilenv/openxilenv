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


#include "Platform.h"
#include "string.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "Scheduler.h"
#include "ExternLoginTimeoutControl.h"


//#define DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL "c:\\tmp\\debug_extern_login_timeout_control.txt"

#define UNUSED(x) (void)(x)

typedef struct {
    int Active;
    int Filler;
    char *ExecutableName;
    int ProcessCount;
    int ExpectedProcessCount;
    unsigned long long TimeoutTime;
} EXTERN_PROCESS_TIMEOUT_ELEM;

static EXTERN_PROCESS_TIMEOUT_ELEM ExternProcessTimeouts[MAX_EXTERN_PROCESSES];

static CRITICAL_SECTION ExternProcessTimeoutCriticalSection;

volatile int WaitForAllLoginCompleteFlag;
static CRITICAL_SECTION WaitForLoginCompleteCriticalSection;
static CONDITION_VARIABLE WaitForLoginCompleteConditionVariable;


#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
static FILE *ExternProcessTimeoutDebugFile;
#endif

void InitExternProcessTimeouts (void)
{
    WaitForAllLoginCompleteFlag = 0;
    InitializeCriticalSection (&ExternProcessTimeoutCriticalSection);
    InitializeCriticalSection (&WaitForLoginCompleteCriticalSection);
    InitializeConditionVariable(&WaitForLoginCompleteConditionVariable);

#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
    ExternProcessTimeoutDebugFile = fopen (DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL, "wt");
#endif
}

int AddExecutableToTimeoutControl (const char *par_ExecutableName, int par_Timeout)
{
    int x;
    EnterCriticalSection (&ExternProcessTimeoutCriticalSection);
    for (x = 0; x < MAX_EXTERN_PROCESSES; x++) {
        if (!ExternProcessTimeouts[x].Active) {
            ExternProcessTimeouts[x].Active = 1;
            ExternProcessTimeouts[x].ProcessCount = 0;
            ExternProcessTimeouts[x].ExpectedProcessCount = 0;
            ExternProcessTimeouts[x].ExecutableName = my_malloc (strlen (par_ExecutableName) + 1);
            if (ExternProcessTimeouts[x].ExecutableName  == NULL) {
                LeaveCriticalSection (&ExternProcessTimeoutCriticalSection);
                ThrowError (1, "Out of memory add executable \"%s\" to timeout control", par_ExecutableName);
                return -1;
            }
            strcpy (ExternProcessTimeouts[x].ExecutableName, par_ExecutableName);
            ExternProcessTimeouts[x].TimeoutTime = GetTickCount64() + (uint64_t)par_Timeout * 1000;
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
            fprintf (ExternProcessTimeoutDebugFile, "AddExecutableToTimeoutControl %i \"%s\"\n", x, ExternProcessTimeouts[x].ExecutableName);
            fflush (ExternProcessTimeoutDebugFile);
#endif
            WaitForAllLoginCompleteFlag = 1;
            break;
        }
    }
    LeaveCriticalSection (&ExternProcessTimeoutCriticalSection);
    if (x == MAX_EXTERN_PROCESSES) {
        ThrowError (1, "No free element to add executable \"%s\" to timeout control", par_ExecutableName);
        return -1;
    }
    return 0;
}

int RemoveExecutableFromTimeoutControl (const char *par_ExecutableName)
{
    int x;
    int WaitCounter = 0;
    int Ret = -1;

    EnterCriticalSection (&ExternProcessTimeoutCriticalSection);
    for (x = 0; x < MAX_EXTERN_PROCESSES; x++) {
        if (ExternProcessTimeouts[x].Active) {
            if (!Compare2ExecutableNames (ExternProcessTimeouts[x].ExecutableName, par_ExecutableName)) {
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
                fprintf (ExternProcessTimeoutDebugFile, "RemoveExecutableFromTimeoutControl %i \"%s\"\n", x, ExternProcessTimeouts[x].ExecutableName);
                fflush (ExternProcessTimeoutDebugFile);
#endif
                my_free (ExternProcessTimeouts[x].ExecutableName);
                ExternProcessTimeouts[x].ExecutableName = NULL;
                ExternProcessTimeouts[x].Active = 0;
                Ret = 0;
            } else {
                WaitCounter++;
            }
        }
    }
    if (!WaitCounter) WaitForAllLoginCompleteFlag = 0;
    LeaveCriticalSection (&ExternProcessTimeoutCriticalSection);
    return Ret;
}


int LoginProcess (char *par_ExecutableName, int par_Number, int par_Count)
{
    UNUSED(par_Number);
    int x;
    int Empty = -1;
    int Ret = -1;

    EnterCriticalSection (&ExternProcessTimeoutCriticalSection);
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
    fprintf (ExternProcessTimeoutDebugFile, "LoginProcess (\"%s\", %i, %i)\n", par_ExecutableName, par_Number, par_Count);
    fflush (ExternProcessTimeoutDebugFile);
#endif
    for (x = 0; x < MAX_EXTERN_PROCESSES; x++) {
        if (ExternProcessTimeouts[x].Active) {
            // was already known?
            if (!Compare2ExecutableNames (ExternProcessTimeouts[x].ExecutableName, par_ExecutableName)) {
                // Is it first process from this executable
                if (ExternProcessTimeouts[x].ExpectedProcessCount == 0) {
                    ExternProcessTimeouts[x].ExpectedProcessCount = par_Count;
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
                    fprintf (ExternProcessTimeoutDebugFile, "  !!!First Process of Executable!!!\n");
                    fflush (ExternProcessTimeoutDebugFile);
#endif
                }
                // Than increment the process counter
                ExternProcessTimeouts[x].ProcessCount++;
                if (ExternProcessTimeouts[x].ProcessCount >= ExternProcessTimeouts[x].ExpectedProcessCount) {
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
                    fprintf (ExternProcessTimeoutDebugFile, "  Remove %i, \"%s\" %i / %i\n", x, ExternProcessTimeouts[x].ExecutableName,
                             ExternProcessTimeouts[x].ProcessCount,ExternProcessTimeouts[x].ExpectedProcessCount);
                    fflush (ExternProcessTimeoutDebugFile);
#endif
                    my_free (ExternProcessTimeouts[x].ExecutableName);
                    ExternProcessTimeouts[x].ExecutableName = NULL;
                    ExternProcessTimeouts[x].Active = 0;
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
                    fprintf (ExternProcessTimeoutDebugFile, "  !!!Last Process of Executable!!!\n");
                    fflush (ExternProcessTimeoutDebugFile);
#endif
                    Ret = 0;    // Now all processes of this executable has been registerd
                } else {
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
                    fprintf (ExternProcessTimeoutDebugFile, "  not removed %i, \"%s\" %i / %i\n", x, ExternProcessTimeouts[x].ExecutableName,
                             ExternProcessTimeouts[x].ProcessCount,ExternProcessTimeouts[x].ExpectedProcessCount);
                    fflush (ExternProcessTimeoutDebugFile);
#endif
                    Ret = ExternProcessTimeouts[x].ExpectedProcessCount - ExternProcessTimeouts[x].ProcessCount;  // Count of still expected logins
                }
                goto __OUT;
            }
        } else {
            if (Empty < 0) {
                Empty = x;
            }
        }
    }
    if (Empty >= 0) {
        if (par_Count > 1) {
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
            fprintf (ExternProcessTimeoutDebugFile, "  !!!not added by AddExecutableToTimeoutControl add it now!!!\n");
            fflush (ExternProcessTimeoutDebugFile);
#endif
            ExternProcessTimeouts[Empty].Active = 1;
            ExternProcessTimeouts[Empty].ProcessCount = 1;
            ExternProcessTimeouts[Empty].ExpectedProcessCount = par_Count;
            ExternProcessTimeouts[Empty].ExecutableName = my_malloc (strlen (par_ExecutableName) + 1);
            if (ExternProcessTimeouts[Empty].ExecutableName  == NULL) {
                ThrowError (1, "Out of memory add executable \"%s\" to timeout control", par_ExecutableName);
                goto __OUT;
            }
            strcpy (ExternProcessTimeouts[Empty].ExecutableName, par_ExecutableName);
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
            fprintf (ExternProcessTimeoutDebugFile, "  Add %i \"%s\" %i / %i\n", x, ExternProcessTimeouts[Empty].ExecutableName,
                     ExternProcessTimeouts[Empty].ProcessCount,ExternProcessTimeouts[Empty].ExpectedProcessCount);
            fflush (ExternProcessTimeoutDebugFile);
#endif
            ExternProcessTimeouts[Empty].TimeoutTime = GetTickCount64() + 10 * 1000;        // TODO: Timeout from basic setttings INI entry
            WaitForAllLoginCompleteFlag = 1;
        }
    } else {
        ThrowError (1, "No free element to add executable \"%s\" to timeout control", par_ExecutableName);
    }
__OUT:
    LeaveCriticalSection (&ExternProcessTimeoutCriticalSection);
    return Ret;
}


void CheckTimeOuts (void)
{
    int x, y;
    int WaitForCounter = 0;
    unsigned long long Time = GetTickCount64();

    EnterCriticalSection (&ExternProcessTimeoutCriticalSection);
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
    {
        SCHEDULER_DATA *Scheduler = GetCurrentScheduler ();
        fprintf (ExternProcessTimeoutDebugFile, "CheckTimeOuts \"%s\" %" PRIu64 "\n", Scheduler->SchedulerName, Time);
        fflush (ExternProcessTimeoutDebugFile);
    }
#endif
    if (WaitForAllLoginCompleteFlag) {
        for (x = 0; x < MAX_EXTERN_PROCESSES; x++) {
            if (ExternProcessTimeouts[x].Active) {
                if (Time > ExternProcessTimeouts[x].TimeoutTime) {
                    unsigned long long dT;
                    int Ret;

#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
                    fprintf (ExternProcessTimeoutDebugFile, "  Timeout %" PRIu64 " > %" PRIu64 " %i \"%s\"\n", Time, ExternProcessTimeouts[x].TimeoutTime,
                             x, ExternProcessTimeouts[x].ExecutableName,
                             ExternProcessTimeouts[x].ProcessCount,ExternProcessTimeouts[x].ExpectedProcessCount);
                    fflush (ExternProcessTimeoutDebugFile);
#endif
                    //LeaveCriticalSection (&ExternProcessTimeoutCriticalSection);
                    Ret = ThrowError (ERROR_OKCANCEL, "Timeout during start process \"%s\". This process is started but it does not login (%i/%i) "
                                                 "should ignore the process and continue (OK) or wait longer (Cancel)",
                                 ExternProcessTimeouts[x].ExecutableName, ExternProcessTimeouts[x].ProcessCount, ExternProcessTimeouts[x].ExpectedProcessCount);
                    //EnterCriticalSection (&ExternProcessTimeoutCriticalSection);
                    switch (Ret) {
                    case IDOK:
                        // Remove process from the timeout control
                        if (ExternProcessTimeouts[x].ExecutableName != NULL) {
                            my_free (ExternProcessTimeouts[x].ExecutableName);
                            ExternProcessTimeouts[x].ExecutableName = NULL;
                        }
                        ExternProcessTimeouts[x].Active = 0;
                        break;
                    case IDCANCEL:
                        WaitForCounter++;
                        ExternProcessTimeouts[x].TimeoutTime = GetTickCount64() + 100*1000;   // Increase timeout  +100s
                        break;
                    }
                    // Increase all others
                    dT = GetTickCount64() - Time;
                    for (y = 0; y < MAX_EXTERN_PROCESSES; y++) {
                        if (ExternProcessTimeouts[y].Active) {
                            ExternProcessTimeouts[y].TimeoutTime += dT;
                        }
                    }
                } else {
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
                    fprintf (ExternProcessTimeoutDebugFile, "  no Timeout %" PRIu64 " <= %" PRIu64 " %i \"%s\"\n", Time, ExternProcessTimeouts[x].TimeoutTime,
                             x, ExternProcessTimeouts[x].ExecutableName,
                             ExternProcessTimeouts[x].ProcessCount,ExternProcessTimeouts[x].ExpectedProcessCount);
                    fflush (ExternProcessTimeoutDebugFile);
#endif
                    WaitForCounter++;
                }

            }
        }
        if (WaitForCounter == 0) {
#ifdef DEBUG_EXTERN_LOGIN_TIMEOUT_CONTROL
                    fprintf (ExternProcessTimeoutDebugFile, "  !!!continue all schedulers!!!\n");
                    fflush (ExternProcessTimeoutDebugFile);
#endif

            // If all login timeouts timer expires or all logins are reported the scheduler can be continue running
            EnterCriticalSection(&WaitForLoginCompleteCriticalSection);
            WaitForAllLoginCompleteFlag = 0;
            LeaveCriticalSection(&WaitForLoginCompleteCriticalSection);
            WakeAllConditionVariable(&WaitForLoginCompleteConditionVariable);
        }
    }
    LeaveCriticalSection (&ExternProcessTimeoutCriticalSection);
}


void WaitForAllLoginComplete (void)
{
    EnterCriticalSection(&WaitForLoginCompleteCriticalSection);
    while (WaitForAllLoginCompleteFlag) {
        SleepConditionVariableCS (&WaitForLoginCompleteConditionVariable, &WaitForLoginCompleteCriticalSection, 100);
        LeaveCriticalSection(&WaitForLoginCompleteCriticalSection);
        CheckTimeOuts();
        EnterCriticalSection(&WaitForLoginCompleteCriticalSection);
    }
    LeaveCriticalSection(&WaitForLoginCompleteCriticalSection);

}

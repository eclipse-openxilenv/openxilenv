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


#include "tcb.h"
#include "Scheduler.h"
#include "ExecutionStack.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "Files.h"
#include "EquationParser.h"
#include "EquationList.h"
#include "RemoteMasterScheduler.h"
#include "MainValues.h"
#include "ProcessEquations.h"

#define UNUSED(x) (void)(x)

void ClacBeforeProcessEquations (TASK_CONTROL_BLOCK *tcb)
{
    int x;
    EnterCriticalSection(&tcb->CriticalSection);
    for (x = 0; x < tcb->BeforeProcessEquationCounter; x++) {
        if (tcb->BeforeProcessEquations[x].ExecStack == NULL) break;
        execute_stack ((struct EXEC_STACK_ELEM *)tcb->BeforeProcessEquations[x].ExecStack);
    }
    LeaveCriticalSection(&tcb->CriticalSection);
}

void ClacBehindProcessEquations (TASK_CONTROL_BLOCK *tcb)
{
    int x;
    EnterCriticalSection(&tcb->CriticalSection);
    for (x = 0; x < tcb->BehindProcessEquationCounter; x++) {
        if (tcb->BehindProcessEquations[x].ExecStack == NULL) break;
        execute_stack ((struct EXEC_STACK_ELEM *)tcb->BehindProcessEquations[x].ExecStack);
    }
    LeaveCriticalSection(&tcb->CriticalSection);
}


int AddBeforeProcessEquation (int Nr, int Pid, void *ExecStack)
{
    int x, y, Ret = -1;
    TASK_CONTROL_BLOCK *tcb;

    tcb = GetPointerToTaskControlBlock (Pid);
    if (tcb == NULL) return -1;
    x = 0;
    EnterCriticalSection(&tcb->CriticalSection);
    while (1) {
        if (tcb->BeforeProcessEquations != NULL) {
            for (; x < tcb->BeforeProcessEquationCounter; x++) {
                if (tcb->BeforeProcessEquations[x].ExecStack == NULL) {
                    tcb->BeforeProcessEquations[x].Nr = Nr;
                    tcb->BeforeProcessEquations[x].ExecStack = ExecStack;   
                    Ret = 0;
                    goto __OUT;
                }
            }
        }
        tcb->BeforeProcessEquationCounter += 10;
        tcb->BeforeProcessEquations = (PROC_EXEC_STACK*)my_realloc (tcb->BeforeProcessEquations,
            (size_t)tcb->BeforeProcessEquationCounter * sizeof (PROC_EXEC_STACK));
        if (tcb->BeforeProcessEquations == NULL) {
            tcb->BeforeProcessEquationCounter = 0;
            Ret = -1;
            goto __OUT;
        }
        for (y = x; y < tcb->BeforeProcessEquationCounter; y++) {
            tcb->BeforeProcessEquations[y].ExecStack = NULL;
        }
    }
__OUT:
    LeaveCriticalSection(&tcb->CriticalSection);
    return Ret;
}


int AddBehindProcessEquation (int Nr, int Pid, void *ExecStack)
{
    int x, y, Ret = -1;
    TASK_CONTROL_BLOCK *tcb;

    tcb = GetPointerToTaskControlBlock (Pid);
    if (tcb == NULL) return -1;
    x = 0;
    EnterCriticalSection(&tcb->CriticalSection);
    while (1) {
        if (tcb->BehindProcessEquations != NULL) {
            for (; x < tcb->BehindProcessEquationCounter; x++) {
                if (tcb->BehindProcessEquations[x].ExecStack == NULL) {
                    tcb->BehindProcessEquations[x].Nr = Nr;
                    tcb->BehindProcessEquations[x].ExecStack = ExecStack;
                    Ret = 0;
                    goto __OUT;                }
            }
        }
        tcb->BehindProcessEquationCounter += 10;
        tcb->BehindProcessEquations = (PROC_EXEC_STACK*)my_realloc (tcb->BehindProcessEquations,
            (size_t)tcb->BehindProcessEquationCounter * sizeof (PROC_EXEC_STACK));
        if (tcb->BehindProcessEquations == NULL) {
            tcb->BehindProcessEquationCounter = 0;
            Ret = -1;
            goto __OUT;
        }
        for (y = x; y < tcb->BehindProcessEquationCounter; y++) {
            tcb->BehindProcessEquations[y].ExecStack = NULL;
        }
    }
__OUT:
    LeaveCriticalSection(&tcb->CriticalSection);
    return Ret;
}


int DelBeforeProcessEquations (int Nr, const char *ProcName)
{
    int x, y;
    PROC_EXEC_STACK Help;
    TASK_CONTROL_BLOCK *tcb;
    int Pid;
    int ret = -1;

    Pid = get_pid_by_name (ProcName);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        if (rm_IsRealtimeProcessPid (Pid)) {
            return rm_DelBeforeProcessEquations(Nr, Pid);
        }
    }
    tcb = GetPointerToTaskControlBlock (Pid);
    if (tcb == NULL) return -1;
    EnterCriticalSection(&tcb->CriticalSection);
    for (y = x = 0; x < tcb->BeforeProcessEquationCounter; x++) {
        if (tcb->BeforeProcessEquations[x].ExecStack == NULL) break;
        if ((Nr == -1) || (tcb->BeforeProcessEquations[x].Nr == Nr)) {
            ret = 0;
            remove_exec_stack_for_process ((struct EXEC_STACK_ELEM *)tcb->BeforeProcessEquations[x].ExecStack, Pid);
            tcb->BeforeProcessEquations[x].ExecStack = NULL;
        } else {
            Help = tcb->BeforeProcessEquations[x];
            tcb->BeforeProcessEquations[x].ExecStack = NULL;
            tcb->BeforeProcessEquations[y] = Help;
            y++;
        }
    }
    LeaveCriticalSection(&tcb->CriticalSection);
    return ret;
}

int DelBehindProcessEquations (int Nr, const char *ProcName)
{
    int x, y;
    PROC_EXEC_STACK Help;
    TASK_CONTROL_BLOCK *tcb;
    int Pid;
    int ret = -1;

    Pid = get_pid_by_name (ProcName);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        if (rm_IsRealtimeProcessPid (Pid)) {
            return rm_DelBehindProcessEquations(Nr, Pid);
        }
    }
    tcb = GetPointerToTaskControlBlock (Pid);
    if (tcb == NULL) return -1;
    EnterCriticalSection(&tcb->CriticalSection);
    for (y = x = 0; x < tcb->BehindProcessEquationCounter; x++) {
        if (tcb->BehindProcessEquations[x].ExecStack == NULL) break;
        if ((Nr == -1) || (tcb->BehindProcessEquations[x].Nr == Nr)) {
            ret = 0;
            remove_exec_stack_for_process ((struct EXEC_STACK_ELEM *)tcb->BehindProcessEquations[x].ExecStack, Pid);
            tcb->BehindProcessEquations[x].ExecStack = NULL;
        } else {
            Help = tcb->BehindProcessEquations[x];
            tcb->BehindProcessEquations[x].ExecStack = NULL;
            tcb->BehindProcessEquations[y] = Help;
            y++;
        }
    }
    LeaveCriticalSection(&tcb->CriticalSection);
    return ret;
}

static void remove_comment (FILE *file)
{
    int one_char;
    while ((one_char = fgetc (file)) != '\n')
        if (one_char == EOF) return;
}

static int AppendErrorString(const char *par_OneErrString, char **ret_ErrString, int par_LenErrorString)
{
    int Len = strlen(par_OneErrString) + 1;
    if (par_LenErrorString == 0) {
        *ret_ErrString = my_malloc(Len);
    } else {
        *ret_ErrString = my_realloc(*ret_ErrString, par_LenErrorString + Len);
    }
    if (*ret_ErrString == NULL) {
        return 0;
    }
    if (par_LenErrorString > 0) {
        (*ret_ErrString)[par_LenErrorString - 1] = '\n';
        strcpy(*ret_ErrString + par_LenErrorString, par_OneErrString);
    } else {
        strcpy(*ret_ErrString, par_OneErrString);
    }
    return par_LenErrorString + Len;
}

static int AddBeforeProcessEquationString (int Nr, int Pid, int RtProc, const char *EquString, const char *Filename, int AddNotExistingVars, char **ErrString)
{
    UNUSED(RtProc);
    struct EXEC_STACK_ELEM *ExecStack;
    int Ret;

    Ret = solve_equation_for_process (EquString, Pid, AddNotExistingVars, &ExecStack, ErrString);
    if (ExecStack == NULL) return -2;
    RegisterEquation (Pid, EquString, ExecStack, Filename, EQU_TYPE_BEFORE_PROCESS);
    if (s_main_ini_val.ConnectToRemoteMaster &&
        rm_IsRealtimeProcessPid (Pid)) {
        if (rm_AddBeforeProcessEquation (Nr, Pid, ExecStack)) {
            Ret = -2;
        }
        DetachRegisterEquationPid(Pid, ExecStack->param.unique_number);
    } else {
        if (AddBeforeProcessEquation (Nr, Pid, ExecStack)) {
            Ret = -2;
        }
    }
    return Ret;
}

// Return:
// -1 -> Prozess doesn't exist
// -2 -> Equation file doesn't exist
// -3 -> A line has more than 65535 characters
// -4 -> Error inside equation stop
// -5 -> error inside equation but continue
int AddBeforeProcessEquationFromFile (int Nr, const char *ProcName, const char *EquFile, int AddNotExistingVars, char **ErrString)
{
    char Buffer[65536];
    char *pg = Buffer;
    FILE *fh;
    int c;
    int Pid;
    int RtProc = 0;
    int Ret = 0;
    int ErrStringLen = 0;

    Pid = get_pid_by_name (ProcName);
    if (Pid <= 0) return -1;

    if (Nr == 0) SetBeforeProcessEquationFileName (ProcName, EquFile);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        RtProc = rm_IsRealtimeProcess (ProcName);
    } else {
        RtProc = 0;
    }
    /* open equation file */
    if ((fh = open_file (EquFile, "rt")) == NULL) {
        ThrowError (1, "cannot open file %s", EquFile);
        return -2;
    }

    /* read one line */
    while ((c = fgetc (fh)) != EOF) {
        if (isspace (c)) continue;
        if (c == ';') {
            char *OneErrString;
            remove_comment(fh);
            if (pg == Buffer) {  // white spaces and comment
                continue;
            }
            *pg = '\0';
            switch (AddBeforeProcessEquationString (Nr, Pid, RtProc, Buffer, EquFile, AddNotExistingVars, &OneErrString)) {
            case -1:   // error continue
                ErrStringLen = AppendErrorString(OneErrString, ErrString, ErrStringLen);
                if (OneErrString != NULL) FreeErrStringBuffer(&OneErrString);
                Ret = -5;
                break;
            case -2:   // error stop
                ErrStringLen = AppendErrorString(OneErrString, ErrString, ErrStringLen);
                if (OneErrString != NULL) FreeErrStringBuffer(&OneErrString);
                Ret = -4;
                goto ERROR_LABEL;
            default:
                break;
            }
            pg = Buffer;
        } else {
            if (pg - Buffer >= 65535) {
                ThrowError (1, "equation exceed 65535 charecters");
                Ret = -3;
                goto ERROR_LABEL;
            }
            *pg++ = (char)c;
        }
    }
    close_file (fh);
    return Ret;
ERROR_LABEL:
    close_file (fh);
    DelBeforeProcessEquations (Nr, ProcName);
    return Ret;
}

static int AddBehindProcessEquationString (int Nr, int Pid, int RtProc, const char *EquString, const char *Filename, int AddNotExistingVars, char **ErrString)
{
    UNUSED(RtProc);
    struct EXEC_STACK_ELEM *ExecStack;
    int Ret;

    Ret = solve_equation_for_process (EquString, Pid, AddNotExistingVars, &ExecStack, ErrString);
    if (ExecStack == NULL) return -2;
    RegisterEquation (Pid, EquString, ExecStack, Filename, EQU_TYPE_BEHIND_PROCESS);
    if (s_main_ini_val.ConnectToRemoteMaster &&
        rm_IsRealtimeProcessPid (Pid)) {
        if (rm_AddBehindProcessEquation (Nr, Pid, ExecStack)) {
            Ret = -2;
        }
        DetachRegisterEquationPid(Pid, ExecStack->param.unique_number);
    } else {
        if (AddBehindProcessEquation (Nr, Pid, ExecStack)) {
            Ret = -2;
        }
    }
    return Ret;
}

// Return:
// -1 -> Prozess doesn't exist
// -2 -> Equation file doesn't exist
// -3 -> A line has more than 65535 characters
// -4 -> Error inside equation stop
// -5 -> error inside equation but continue
int AddBehindProcessEquationFromFile (int Nr, const char *ProcName, const char *EquFile, int AddNotExistingVars, char **ErrString)
{
    char Buffer[65536];
    char *pg = Buffer;
    FILE *fh;
    int c;
    int Pid;
    int RtProc = 0;
    int Ret = 0;
    int ErrStringLen = 0;

    Pid = get_pid_by_name (ProcName);
    if (Pid <= 0) return -1;
    if (Nr == 0) SetBehindProcessEquationFileName (ProcName, EquFile);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        RtProc = rm_IsRealtimeProcess (ProcName);
    } else {
        RtProc = 0;
    }
    /* open equation file */
    if ((fh = open_file (EquFile, "rt")) == NULL) {
        ThrowError (1, "cannot open file %s", EquFile);
        return -2;
    }

    /* read one line */
    while ((c = fgetc (fh)) != EOF) {
        if (isspace (c)) continue;
        if (c == ';') {
            char *OneErrString;
            remove_comment(fh);
            if (pg == Buffer) {   // white spaces and comment
                continue;
            }
            *pg = '\0';
            switch (AddBehindProcessEquationString (Nr, Pid, RtProc, Buffer, EquFile, AddNotExistingVars, &OneErrString)) {
            case -1:   // error continue
                ErrStringLen = AppendErrorString(OneErrString, ErrString, ErrStringLen);
                if (OneErrString != NULL) FreeErrStringBuffer(&OneErrString);
                Ret = -5;
                break;
            case -2:   // error stop
                ErrStringLen = AppendErrorString(OneErrString, ErrString, ErrStringLen);
                if (OneErrString != NULL) FreeErrStringBuffer(&OneErrString);
                Ret = -4;
                goto ERROR_LABEL;
            default:
                break;
            }
            pg = Buffer;
        } else {
            if (pg - Buffer >= 65535) {
                ThrowError (1, "equation exceed 65535 charecters");
                Ret = -3;
                goto ERROR_LABEL;
            }
            *pg++ = (char)c;
        }
    }
    close_file (fh);
    return Ret;
ERROR_LABEL:
    close_file (fh);
    DelBehindProcessEquations (Nr, ProcName);
    return Ret;
}


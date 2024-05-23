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


#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "Config.h"

#include "EquationsSingle.h"

#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "Files.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#include "Message.h"
#include "Scheduler.h"
#include "EquationList.h"

typedef struct {
    struct EXEC_STACK_ELEM **EquationExecStacks;
    int EquationCounter;
} TRIGGER_EQUATIONS;

static TRIGGER_EQUATIONS Calculator;
static TRIGGER_EQUATIONS Compiler;

static CRITICAL_SECTION TriggerCriticalSection;
static int TriggerCriticalSectionIsInit;

static int EquationProcessState;
static int EquationProcessStateLast;

static int EquationProcessStateVid;
static int DontAddNotExistingVars;

static void remove_comment (FILE *file)
{
    int one_char;
    while ((one_char = fgetc (file)) != '\n')
        if (one_char == EOF) return;
}

static void cleanup_trigger_infos_cs (TRIGGER_EQUATIONS *Equations, int cs)
{
    int x;

    if (cs) EnterCriticalSection (&TriggerCriticalSection);
    if (Equations->EquationExecStacks != NULL) {
        for (x = 0; x < Equations->EquationCounter; x++) {
            remove_exec_stack (Equations->EquationExecStacks[x]);
        }
        my_free (Equations->EquationExecStacks);
        Equations->EquationExecStacks = NULL;
        Equations->EquationCounter = 0;
    }
    if (cs) LeaveCriticalSection (&TriggerCriticalSection);
}

static void cleanup_trigger_infos (TRIGGER_EQUATIONS *Equations)
{
    cleanup_trigger_infos_cs (Equations, 1);
}

static void CopyEquationInfos (TRIGGER_EQUATIONS *To, TRIGGER_EQUATIONS *From)
{
    int x;

    EnterCriticalSection (&TriggerCriticalSection);
    cleanup_trigger_infos_cs (To, 0);
    if (From->EquationExecStacks != NULL) {
        To->EquationCounter = From->EquationCounter;
        To->EquationExecStacks =(struct EXEC_STACK_ELEM**)my_malloc (To->EquationCounter * sizeof (struct EXEC_STACK_ELEM*));
        for (x = 0; x < To->EquationCounter; x++) {
            int size;
            size = sizeof_exec_stack (From->EquationExecStacks[x]);
            To->EquationExecStacks[x] = (struct EXEC_STACK_ELEM*)my_malloc (size);
            copy_exec_stack (To->EquationExecStacks[x], From->EquationExecStacks[x]);
            attach_exec_stack (To->EquationExecStacks[x]);
        }
    }
    LeaveCriticalSection (&TriggerCriticalSection);
}

void CyclicEquationExecution (void)
{
    int x;
    int Ret;
    MESSAGE_HEAD mhead;

    while (test_message (&mhead)) {
        switch (mhead.mid) {
        case TRIGGER_START_MESSAGE:
            CopyEquationInfos (&Calculator, &Compiler);
            break;
        case TRIGGER_STOP_MESSAGE:
            cleanup_trigger_infos (&Calculator);
            break;
        }
        remove_message ();
    }

    write_bbvari_ubyte (EquationProcessStateVid, (unsigned char)EquationProcessState);
    for (x = 0; x < Calculator.EquationCounter; x++) {
        if (EquationProcessState) {
            if ((Ret = execute_stack_ret_err_code (Calculator.EquationExecStacks[x], NULL)) != 0) {
                ThrowError (1, "equation stopped because equation \"%s\", %s",
                       GetRegisterEquationString (Calculator.EquationExecStacks[x]->param.unique_number),
                       (Ret == -2) ? "nonlinear conversion writing with phys()" : "internel error");
                EquationProcessState = 0;
                cleanup_trigger_infos (&Calculator);
            }
        }
    }
}

int InitEquationExecution (void)
{
    if (!TriggerCriticalSectionIsInit) {
        TriggerCriticalSectionIsInit = 1;
        InitializeCriticalSection(&TriggerCriticalSection);
    }
    DontAddNotExistingVars = 0;
    const char *Name = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_EQUATION_CALCULATOR);
    EquationProcessStateVid = add_bbvari (Name, BB_UBYTE, NULL);
    if (EquationProcessStateVid <= 0) {
        ThrowError (1, "cannot attach variable '%s'", Name);
        return 1;
    }
    set_bbvari_conversion (EquationProcessStateVid, 2, "0 0 \"Sleep\"; 1 1 \"Active\";");
    EquationProcessState = 0;
    EquationProcessStateLast = 0;

    return 0;
}

void TerminateEquationExecution (void)
{
    cleanup_trigger_infos(&Calculator);
    if (EquationProcessStateVid  > 0) remove_bbvari (EquationProcessStateVid);
}

static int ReadEquationFile (char *par_EquationFileName, char **errstring)
{
    char LineBuffer[65536];
    char *pg = LineBuffer;
    FILE *fh;
    int c;
    int x = 0;

    /* Open equation file */
    if ((fh = open_file (par_EquationFileName, "rt")) == NULL) {
        ThrowError (1, "cannot open file %s", par_EquationFileName);
        return -1;
    }

    /* Read from equation file till end of file reached */
    while ((c = fgetc (fh)) != EOF) {
        if (isspace (c)) continue;
        if (c == ';') {
            remove_comment(fh);
            if (pg == LineBuffer) {  // Empty line with comment
                continue;
            }
            *pg = '\0';
            Compiler.EquationExecStacks = (struct EXEC_STACK_ELEM**)my_realloc (Compiler.EquationExecStacks,
                                                                                 (x+1) * sizeof (struct EXEC_STACK_ELEM*));
            if (Compiler.EquationExecStacks == NULL) {
                ThrowError (1, "out of memmory");
                goto ERROR_LABEL;
            }
            /* Translate equation to a execution stack */
            if (DontAddNotExistingVars) {
                Compiler.EquationExecStacks[x] = solve_equation_no_add_bbvari (LineBuffer, errstring, DontAddNotExistingVars);
            } else {
                Compiler.EquationExecStacks[x] = solve_equation (LineBuffer);
            }
            if (Compiler.EquationExecStacks[x] == NULL) {
                /* If there are an error inside the equation delete all translated before */
                for (x--; x >= 0; x--)
                    remove_exec_stack (Compiler.EquationExecStacks[x]);
                my_free (Compiler.EquationExecStacks);
                Compiler.EquationExecStacks = NULL;
                goto ERROR_LABEL;
            }
            RegisterEquation (-1, LineBuffer, Compiler.EquationExecStacks[x], par_EquationFileName, EQU_TYPE_GLOBAL_EQU_CALCULATOR);
            x++;
            pg = LineBuffer;
        } else {
            if (pg - LineBuffer >= 65535) {
                 ThrowError (1, "equation exceed 65535 charecters");
                 goto ERROR_LABEL;
            }
            *pg++ = (char)c;
        }
    }

    close_file (fh);
    Compiler.EquationCounter = x;   /* remember the number of translated equations */
    return 0;
ERROR_LABEL:
    close_file (fh);
    return -1;
}

void CyclicEquationCompiler (void)
{

    MESSAGE_HEAD mhead;
    char trigger_filename[MAX_PATH];
    char *errstring;
    int len;

    while (test_message (&mhead)) {
        switch (mhead.mid) {
        case TRIGGER_DONT_ADD_NOT_EXITING_VARS:
            read_message (&mhead, (char*)&DontAddNotExistingVars, sizeof(DontAddNotExistingVars));
            break;
        case TRIGGER_FILENAME_MESSAGE:
            read_message (&mhead, trigger_filename, MAX_PATH);
            EquationProcessState = 0;
            cleanup_trigger_infos (&Compiler);
            errstring = NULL;
            if (!ReadEquationFile (trigger_filename, &errstring)) {
                 EquationProcessState = 1;
            }
            DontAddNotExistingVars = 0;   // This must be reseted always it is only valid one time
            if (errstring != NULL) {
                 len = strlen(errstring) + 1;
            } else {
                 len = 0;
            }
            write_message (mhead.pid, TRIGGER_ACK, len, errstring);
            if (errstring != NULL) {
                 my_free(errstring);
            }
            write_message (get_pid_by_name (EQUATION_CALCULATOR), TRIGGER_START_MESSAGE, 0, "");
            break;
        case TRIGGER_STOP_MESSAGE:
            EquationProcessState = 0;
            write_message (get_pid_by_name (EQUATION_CALCULATOR), TRIGGER_STOP_MESSAGE, 0, "");
            cleanup_trigger_infos (&Compiler);
            remove_message ();
            break;
        default:
            ThrowError (1, "catch unknown message %i from %i",
                   mhead.mid, mhead.pid);
            remove_message ();
        }
    }

}

void TerminateEquationCompiler (void)
{
    PID pid;

    if ((pid = get_pid_by_name (PN_EQUATION_CALCULATOR)) > 0) {
        terminate_process (pid);
    }
    EquationProcessState = 0;
    EquationProcessStateLast = 0;
    cleanup_trigger_infos (&Compiler);
}

int InitEquationCompiler (void)
{
    if (!TriggerCriticalSectionIsInit) {
        TriggerCriticalSectionIsInit = 1;
        InitializeCriticalSection(&TriggerCriticalSection);
    }
    EquationProcessState = 0;
    Compiler.EquationCounter = 0;
    return 0;
}

TASK_CONTROL_BLOCK EquationExecutionTcb =
        INIT_TASK_COTROL_BLOCK(EQUATION_CALCULATOR, INTERN_ASYNC, 41, CyclicEquationExecution, InitEquationExecution, TerminateEquationExecution, 1024);

TASK_CONTROL_BLOCK EquationCompilerTcb =
        INIT_TASK_COTROL_BLOCK(EQUATION_COMPILER, INTERN_ASYNC, 40, CyclicEquationCompiler, InitEquationCompiler, TerminateEquationCompiler, 1024);

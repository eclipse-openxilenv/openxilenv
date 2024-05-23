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
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "Config.h"

#include "EquationsSplitCompiler.h"

#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Files.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#include "Message.h"
#include "Scheduler.h"
#include "EquationList.h"

static struct EXEC_STACK_ELEM **EquationExecStacks;
static int EquationCounter;
static int DontAddNotExistingVars;

static void CleanupTriggerInfos (void)
{
    int x;

    if (EquationExecStacks != NULL) {
        for (x = 0; x < EquationCounter; x++) {
            remove_exec_stack (EquationExecStacks[x]);
        }
        my_free (EquationExecStacks);
        EquationExecStacks = NULL;
        EquationCounter = 0;
    }
}

static void RemoveComment (FILE *file)
{
    int one_char;
    while ((one_char = fgetc (file)) != '\n')
        if (one_char == EOF) return;
}

static int ReadEquationFile (char *trigger_filename, char **errstring)
{
    char EquationBuffer[65536];
    char *pg = EquationBuffer;
    FILE *fh;
    int c;
    int x = 0;

    /* Open equation file */
    if ((fh = open_file (trigger_filename, "rt")) == NULL) {
        ThrowError (1, "cannot open file %s", trigger_filename);
        return -1;
    }

    /* Read from equation file till end */
    while ((c = fgetc (fh)) != EOF) {
        if (isspace (c)) continue;
        if (c == ';') {
            RemoveComment(fh);
            if (pg == EquationBuffer) {  // Empty line with comment
                continue;
            }
            *pg = '\0';
            EquationExecStacks = (struct EXEC_STACK_ELEM**)my_realloc (EquationExecStacks,
                                                                        (x+1) * sizeof (struct EXEC_STACK_ELEM*));
            if (EquationExecStacks == NULL) {
                ThrowError (1, "out of memmory");
                goto ERROR_LABEL;
            }
            /* Translate equation */
            if (DontAddNotExistingVars) {
                EquationExecStacks[x] = solve_equation_no_add_bbvari (EquationBuffer, errstring, DontAddNotExistingVars);
            } else {
                EquationExecStacks[x] = solve_equation (EquationBuffer);
            }
            if (EquationExecStacks[x] == NULL) {
                /* If there is an error inside this equation delete all readed before */
                for (x--; x >= 0; x--) {
                    remove_exec_stack (EquationExecStacks[x]);
                }
                my_free (EquationExecStacks);
                EquationExecStacks = NULL;
                goto ERROR_LABEL;
            } else {
                RegisterEquation (-1, EquationBuffer, EquationExecStacks[x], "", EQU_TYPE_GLOBAL_EQU_CALCULATOR);
            }
            x++;
            pg = EquationBuffer;
        } else {
            if (pg - EquationBuffer >= 65535) {
                 ThrowError (1, "equation exceed 65535 charecters");
                 goto ERROR_LABEL;
            }
            *pg++ = (char)c;
        }
    }

    close_file (fh);
    EquationCounter = x;   /* Enable equations for execution process */
    return 0;
ERROR_LABEL:
    close_file (fh);
    return -1;
}

void rm_CyclicEquationCompiler (void)
{
    MESSAGE_HEAD mhead;
    char trigger_filename[MAX_PATH];
    int x;
    char *errstring;

    while (test_message (&mhead)) {
        switch (mhead.mid) {
        case TRIGGER_DONT_ADD_NOT_EXITING_VARS:
            read_message (&mhead, (char*)&DontAddNotExistingVars, sizeof(DontAddNotExistingVars));
            break;
        case TRIGGER_FILENAME_MESSAGE:
            read_message (&mhead, trigger_filename, MAX_PATH);
            CleanupTriggerInfos ();
            errstring = NULL;
            if (!ReadEquationFile (trigger_filename, &errstring)) {
                write_message (get_pid_by_name (EQUATION_CALCULATOR),
                               TRIGGER_EUATIONCONTER_MESSAGE,
                               sizeof (int),
                               (char*)&EquationCounter);
                for (x = 0; x < EquationCounter; x++) {
                    write_message (get_pid_by_name (EQUATION_CALCULATOR),
                                   TRIGGER_EUATIONS_MESSAGE,
                                   sizeof_exec_stack (EquationExecStacks[x]),
                                   (char*)EquationExecStacks[x]);
                }
            }
            DontAddNotExistingVars = 0;   // default is reset
            if (errstring != NULL) {
                x = strlen(errstring) + 1;
            } else {
                x = 0;
            }
            write_message (mhead.pid, TRIGGER_ACK, x, errstring);
            if (errstring != NULL) {
                my_free(errstring);
            }
            break;
        case TRIGGER_ERROR_MESSAGE:
            {
                struct {
                    uint64_t UniqueNumber;
                    int ErrCode;
                } ErrMsg;
                read_message (&mhead, (char*)&ErrMsg, sizeof (ErrMsg));
                ThrowError (1, "equation stopped because equation \"%s\", %s",
                       GetRegisterEquationString (ErrMsg.UniqueNumber),
                       (ErrMsg.ErrCode == -2) ? "nonlinear conversion writing with phys()" : "internel error");
            }
            write_message (get_pid_by_name (EQUATION_CALCULATOR), TRIGGER_STOP_MESSAGE, 0, NULL);
            CleanupTriggerInfos ();
            remove_message ();
            break;
        case TRIGGER_STOP_MESSAGE:
            write_message (get_pid_by_name (EQUATION_CALCULATOR), TRIGGER_STOP_MESSAGE, 0, NULL);
            CleanupTriggerInfos ();
            remove_message ();
            break;
        default:
            ThrowError (1, "catch unknown message %i from %i",
                   mhead.mid, mhead.pid);
            remove_message ();
        }
    }
}

void rm_TerminateEquationCompiler (void)
{
    PID pid;

    if ((pid = get_pid_by_name (PN_EQUATION_CALCULATOR)) > 0) {
        terminate_process (pid);
    }
    CleanupTriggerInfos ();
}

int rm_InitEquationCompiler (void)
{
    DontAddNotExistingVars = 0;
    EquationCounter = 0;
    return 0;
}

TASK_CONTROL_BLOCK rm_EquationCompilerTcb =
        INIT_TASK_COTROL_BLOCK("rm_" EQUATION_COMPILER, INTERN_ASYNC, 40, rm_CyclicEquationCompiler, rm_InitEquationCompiler, rm_TerminateEquationCompiler, 1024);


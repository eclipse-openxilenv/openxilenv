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


#define EQUATIONSSPLITEXECUTE_C

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "Config.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "Message.h"
#include "RealtimeScheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ExecutionStack.h"
#include "EquationsSplitExecute.h"

static struct EXEC_STACK_ELEM **trigger_exec_stacks;
static int trigger_equation_counter;
static int vid_state_trigger;
//static int vid_debug;

static void cleanup_trigger_infos (void);

void cyclic_trigger (void)
{
    int x;
    int Ret;
    MESSAGE_HEAD mhead;
    static int equ_counter;
    int max_message_per_cycle = 100;

    while (test_message (&mhead) && (max_message_per_cycle-- > 0)) {
        //printf("%i: %llu - %llu = %llu\n", mhead.mid, get_timestamp_counter(),  mhead.timestamp, get_timestamp_counter() - mhead.timestamp);
        switch (mhead.mid) {
        case TRIGGER_EUATIONS_MESSAGE:
            if (!trigger_equation_counter) break;    // File enthaelt keine Gleichungen
            if (equ_counter >= trigger_equation_counter) {
                ThrowError(1, "too much equations as defined in TRIGGER_EUATIONCONTER_MESSAGE");
            }
            trigger_exec_stacks[equ_counter] = (struct EXEC_STACK_ELEM *)my_malloc (mhead.size);
            read_message (&mhead, (char*)trigger_exec_stacks[equ_counter], mhead.size);
            attach_exec_stack (trigger_exec_stacks[equ_counter]);
            equ_counter++;
            if (trigger_equation_counter) write_bbvari_ubyte (vid_state_trigger, (unsigned char)1);
            break;
        case TRIGGER_EUATIONCONTER_MESSAGE:
            if (mhead.size == sizeof (int)) {
                cleanup_trigger_infos ();                // raeume alte Gleichungen auf falls vorhanden
                read_message (&mhead, (char*)&trigger_equation_counter, sizeof (int));
                equ_counter = 0;
                trigger_exec_stacks = (struct EXEC_STACK_ELEM **)my_calloc (trigger_equation_counter, sizeof (void*));
            }
            break;
        case TRIGGER_STOP_MESSAGE:
            remove_message ();
            cleanup_trigger_infos ();                // raeume alte Gleichungen auf falls vorhanden
            write_bbvari_ubyte (vid_state_trigger, (unsigned char)0);
            break;
        default:
            ThrowError(1, "unknown message %i send bei %i", mhead.mid, mhead.pid);
            remove_message ();
        }
    }
    if (trigger_equation_counter == equ_counter) {
        for (x = 0; x < trigger_equation_counter; x++) {
            if ((Ret = execute_stack_ret_err_code (trigger_exec_stacks[x], NULL)) != 0) {
                struct {
                    uint64_t UniqueNumber;
                    int ErrCode;
                } ErrMsg;  // same Struct in trigasyn.c!!!
                write_bbvari_ubyte (vid_state_trigger, (unsigned char)0);
                ErrMsg.ErrCode = Ret;
                ErrMsg.UniqueNumber = trigger_exec_stacks[x]->param.unique_number;
                write_message (get_pid_by_name (EQUATION_COMPILER),
                               TRIGGER_ERROR_MESSAGE,
                               sizeof (ErrMsg),
                               (char*)&ErrMsg);
                cleanup_trigger_infos ();
            }
        }
    }
}


int init_trigger (void)
{
    vid_state_trigger = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_EQUATION_CALCULATOR), BB_UBYTE, NULL);
    if (vid_state_trigger <= 0) {
        ThrowError(1, "cannot attach variable 'Trigger'");
        return 1;
    }
    set_bbvari_conversion(vid_state_trigger, 2, "0 0 \"Sleep\"; 1 1 \"Active\";");
    return 0;
}


void terminate_trigger (void)
{
    if (vid_state_trigger  > 0) remove_bbvari (vid_state_trigger);
    cleanup_trigger_infos ();                // raeume alte Gleichungen auf falls vorhanden
}


static void cleanup_trigger_infos (void)
{
    int x;

    if (trigger_exec_stacks != NULL) {
        for (x = 0; x < trigger_equation_counter; x++) {
            remove_exec_stack (trigger_exec_stacks[x]);
        }
        my_free (trigger_exec_stacks);
        trigger_exec_stacks = NULL;
        trigger_equation_counter = 0;
    }
}



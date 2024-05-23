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
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "Config.h"

#define RAMPSSINGLE_C
#include "RampsSingle.h"

#include "ConfigurablePrefix.h"
#include "Scheduler.h"
#include "BlackboardAccess.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Message.h"
#include "ReadConfig.h"
#include "Files.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#include "EquationList.h"

static RAMPEN_INFO *rampen_info_struct;
static int *vids_rampen;
static int vid_state_rampen;


static void start_tiggered (void)
{
    int x;
    RAMPEN_ELEM *hrp;

    rampen_info_struct->trigger_state = TRIGGER_STATE_TRIGGER;
    /* Calculate all ramps inside on time sample */
    for (x = 0; x < rampen_info_struct->anz_rampen; x++) {
        /* shortened notation */
        hrp = &rampen_info_struct->rampen[x];
        hrp->count = 0;
        // If first sample point calculate variable here
        if (hrp->ampl_exec_stack[hrp->count] != NULL)
            hrp->ampl[hrp->count] = execute_stack (hrp->ampl_exec_stack[hrp->count]);
        if (hrp->time_exec_stack[hrp->count] != NULL)
            hrp->time[hrp->count] = execute_stack (hrp->time_exec_stack[hrp->count]);
    }
    rampen_info_struct->begin_rampen_time = 0.0;
    write_bbvari_ubyte (vid_state_rampen, 2); /* XilEnv.Generator */

}

void cyclic_rampen (void)
{
    int x, y;
    double value;
    double dt;
    double trigger_value;
    RAMPEN_ELEM *hrp;
    int alle_rampen_beendet;
    MESSAGE_HEAD mhead;

    if (test_message (&mhead)) {
        switch (mhead.mid) {
        case RAMPEN_START:
            for (x = 0; x < rampen_info_struct->anz_rampen; x++) {
                if (attach_bbvari (rampen_info_struct->vids[x+2]) < 0) {
                    // Error message?
                }
                /* shortened notation */
                hrp = &rampen_info_struct->rampen[x];
                for (y = 0; y < hrp->size; y++) {
                    if (hrp->ampl_exec_stack[y] != NULL)
                        attach_exec_stack (hrp->ampl_exec_stack[y]);
                    if (hrp->time_exec_stack[y] != NULL)
                        attach_exec_stack (hrp->time_exec_stack[y]);
                }
            }

            rampen_info_struct->trigger_state = TRIGGER_STATE_WAIT_SMALLER;
            remove_message ();
            write_message (mhead.pid, RAMPEN_ACK, 0, NULL);
            break;
        case RAMPEN_STOP:
            rampen_info_struct->trigger_state = TRIGGER_STATE_OFF;
            alle_rampen_beendet = 1;
            remove_message ();
            goto RAMPEN_STOP_MESSAGE;
        default:
            ThrowError (1, "receive unknown message %i from %i",
                   mhead.mid, mhead.pid);
            remove_message ();
        }
    }

    /* Triggern */
    if ((rampen_info_struct->trigger_state == TRIGGER_STATE_WAIT_SMALLER) ||
        (rampen_info_struct->trigger_state == TRIGGER_STATE_WAIT_LARGER)) {  /* Ramps doesn't run at the moment */
        write_bbvari_ubyte (vid_state_rampen, 1); /* Generator waiting for trigger */
        if (rampen_info_struct->trigger_flag == 0) {
            start_tiggered();
        } else {
            trigger_value = read_bbvari_convert_double (vids_rampen[1]);

            if (rampen_info_struct->trigger_state == TRIGGER_STATE_WAIT_SMALLER) {
                if (rampen_info_struct->trigger_flag == 1) {
                    if (trigger_value < rampen_info_struct->trigger_schw)
                        rampen_info_struct->trigger_state = TRIGGER_STATE_WAIT_LARGER;
                } else {
                    if (trigger_value > rampen_info_struct->trigger_schw)
                        rampen_info_struct->trigger_state = TRIGGER_STATE_WAIT_LARGER;
                }
            } else if (rampen_info_struct->trigger_state == TRIGGER_STATE_WAIT_LARGER) {
                if (rampen_info_struct->trigger_flag == 1) {
                    if (trigger_value > rampen_info_struct->trigger_schw) {
                        start_tiggered();
                    }
                } else {
                    if (trigger_value < rampen_info_struct->trigger_schw) {
                        start_tiggered();
                    }
                }
            }
        }
    }
    if (rampen_info_struct->trigger_state == TRIGGER_STATE_TRIGGER) {

        /* Read time step from blackboard */
        dt = read_bbvari_double (vids_rampen[0]) * GET_TCB()->time_steps;

        alle_rampen_beendet = 1;
        value = 0.0;
        /* calculate all ramps inside on time slot */
        for (x = 0; x < rampen_info_struct->anz_rampen; x++) {
            /* shortened notation */
            hrp = &rampen_info_struct->rampen[x];
            /* Ramp are running */
            if (hrp->state != RAMPE_BEENDET) {
                /* Next sample point? */
                if (hrp->time[hrp->count] <= rampen_info_struct->begin_rampen_time) {
                    /* There are additional sample points */
                    if (hrp->count < hrp->size-1) {
                        hrp->dt =  rampen_info_struct->begin_rampen_time - hrp->time[hrp->count];

                        /* If there are an equation calculate them */
                        if (hrp->ampl_exec_stack[hrp->count+1] != NULL)
                            hrp->ampl[hrp->count+1] = execute_stack (hrp->ampl_exec_stack[hrp->count+1]);
                        if (hrp->time_exec_stack[hrp->count+1] != NULL)
                            hrp->time[hrp->count+1] = execute_stack (hrp->time_exec_stack[hrp->count+1]);

                        if ((hrp->time[hrp->count+1] > hrp->time[hrp->count])) {
                            hrp->steig = (hrp->ampl[hrp->count] - hrp->ampl[hrp->count+1]) /
                                         (hrp->time[hrp->count] - hrp->time[hrp->count+1]);
                            value = hrp->ampl[hrp->count] + hrp->steig * hrp->dt;
                        } else {
                            value = hrp->ampl[hrp->count];
                        }
                        hrp->count++;
                        hrp->state = RAMPE_LAEUFT;
                    } else {   /* No more sample points */
                        value = hrp->ampl[hrp->count];
                        hrp->state = RAMPE_WIRD_BEENDET;
                    }
                } else {
                    if (hrp->state == RAMPE_LAEUFT) {
                        hrp->dt += dt;
                        value = hrp->ampl[hrp->count-1] + hrp->steig * hrp->dt;
                    }
                }
            } else {
                value = 0.0;
            }
            if ((hrp->state == RAMPE_LAEUFT) || (hrp->state == RAMPE_WIRD_BEENDET)) {
                double bb_v;
                if (hrp->type == RAMPE_ADDIEREN) {
                    bb_v = read_bbvari_convert_double (vids_rampen[x+2]);
                    write_bbvari_minmax_check (vids_rampen[x+2], value + bb_v);
                } else write_bbvari_minmax_check (vids_rampen[x+2], value);
            }
            if (hrp->state == RAMPE_WIRD_BEENDET) {
                hrp->state = RAMPE_BEENDET;
            }
            if (hrp->state != RAMPE_BEENDET) {
                alle_rampen_beendet = 0;
            }
        }
        rampen_info_struct->begin_rampen_time += dt;

        if (read_bbvari_ubyte (vid_state_rampen) == 0) { /* XilEnv.Generator */
            alle_rampen_beendet = 1;
        }

RAMPEN_STOP_MESSAGE:
        /* If last ramp are ended notify this to the SIGNAL_GENERATOR_COMPILER process */
        if (alle_rampen_beendet) {
            int pid;
            // attach-Counter verringern
            for (x = 0; x < rampen_info_struct->anz_rampen; x++) {
                remove_bbvari (rampen_info_struct->vids[x+2]);
                /* shortened notation */
                hrp = &rampen_info_struct->rampen[x];
                for (y = 0; y < hrp->size; y++) {
                    if (hrp->ampl_exec_stack[y] != NULL)
                        attach_exec_stack (hrp->ampl_exec_stack[y]);
                    if (hrp->time_exec_stack[y] != NULL)
                        attach_exec_stack (hrp->time_exec_stack[y]);
                }
            }
            pid = get_pid_by_name (PN_SIGNAL_GENERATOR_COMPILER);
            if (pid > 0) {
                write_message (pid, RAMPEN_STOP, 0, NULL);
            }
            rampen_info_struct->trigger_state = TRIGGER_STATE_OFF;
            write_bbvari_ubyte (vid_state_rampen, 0);  /*XilEnv.Generator */
            for (x = 0; x < rampen_info_struct->anz_rampen; x++) {
                rampen_info_struct->rampen[x].count = 0;
                rampen_info_struct->rampen[x].state = RAMPE_WARTET;
            }
            rampen_info_struct->anz_rampen = 0;
        }
    }
}


int init_rampen (void)
{
    vid_state_rampen = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_GENERATOR), BB_UBYTE, NULL);
    if (vid_state_rampen <= 0) {
        ThrowError (1, "cannot attach variable 'Generator'");
        return 1;
    }
    set_bbvari_conversion (vid_state_rampen, 2, "0 0 \"Sleep\"; 1 1 \"Trigger\"; 2 2 \"Play\";");
    write_bbvari_ubyte (vid_state_rampen, 0);  /* XilEnv.Generator */
    if (rampen_info_struct == NULL) {
        if ((rampen_info_struct = my_calloc (1, sizeof (RAMPEN_INFO))) == NULL) {
            return -1;
        }
    }
    return 0;
}


void terminate_rampen (void)
{
    remove_bbvari (vid_state_rampen);
    vid_state_rampen = 0L;
}


/* Compiler process */

void cleanup_rampenstruct (void)
{
    int x, y;
    for (x = 0; x < rampen_info_struct->anz_rampen; x++) {
        if (rampen_info_struct->rampen[x].ampl != NULL)
            my_free (rampen_info_struct->rampen[x].ampl);
        rampen_info_struct->rampen[x].ampl = NULL;
        if (rampen_info_struct->rampen[x].time)
            my_free (rampen_info_struct->rampen[x].time);
        rampen_info_struct->rampen[x].time = NULL;
        if (rampen_info_struct->rampen[x].ampl_exec_stack != NULL) {
            for (y = 0; y < rampen_info_struct->rampen[x].size; y++)
                remove_exec_stack (rampen_info_struct->rampen[x].ampl_exec_stack[y]);
            my_free (rampen_info_struct->rampen[x].ampl_exec_stack);
            rampen_info_struct->rampen[x].ampl_exec_stack = NULL;
        }
        if (rampen_info_struct->rampen[x].time_exec_stack != NULL) {
            for (y = 0; y < rampen_info_struct->rampen[x].size; y++)
                remove_exec_stack (rampen_info_struct->rampen[x].time_exec_stack[y]);
            my_free (rampen_info_struct->rampen[x].time_exec_stack);
            rampen_info_struct->rampen[x].time_exec_stack = NULL;
        }
    }
    if (vids_rampen != NULL) {
        for (x = 0; vids_rampen[x] > 0; x++)
            remove_bbvari (vids_rampen[x]);
        my_free (vids_rampen);
        vids_rampen = NULL;
    }
}

static void rm_comment (FILE *file, int *lc)
{
    int one_char;
    while ((one_char = fgetc (file)) != '\n')
        if (one_char == EOF) return;
    *lc += 1;
}

static int read_word_lc (FILE *file, char *word, int maxlen, int *lc)
{
    int char_count = 0;
    int one_char;
    int end_of_word = 0;
    char *wp = word;

    do {
        one_char = fgetc (file);
        if (one_char == '\n') *lc += 1;
        if (one_char == ';') rm_comment (file, lc);
    } while ((one_char == ',') || (one_char == '\n') ||
           (one_char == '\t') || (one_char == '\r') ||
           (one_char == ' ') || (one_char == ';'));
    do {
        if ((one_char == EOF) || (one_char == ',') ||
            (one_char == '\n') || (one_char == '\t') ||
            (one_char == '\r') || (one_char == ' ') ||
            (one_char == ';')) {
            if (one_char == ';') rm_comment (file, lc);
            *wp = '\0';
            end_of_word = 1;
        } else {
            *wp++ = (char)one_char;
            if (++char_count >= maxlen) {
                *wp = '\0';
                return -1;
            }
        }
        if (!end_of_word) {
            one_char = fgetc (file);
            if (one_char == '\n') *lc += 1;
        }
    } while (!end_of_word);
    if (one_char == EOF) return EOF;
    return 0;
}

int ReadPairOfValues (char *word, double *a, double *b, int *lc)
{
    char str_a[100], str_b[100], *hpa, *hpb, *hz;
    int count = 0;
    int state = 0;


    hz = word;
    hpa = str_a;
    hpb = str_b;
    while (*hz != '\0') {
        if (isdigit (*hz) || (*hz == 'e') || (*hz == 'E') || (*hz == '-') || (*hz == '+') || (*hz == '.')) {
            if (++count >= 100) {
                ThrowError (1, "to many charecter in value %s (%i)", word, *lc);
                return 1;
            }
            switch (state) {
                case 0:
                    *hpa++ = *hz++;
                    break;
                case 1:
                    *hpb++ = *hz++;
                    break;
                default:
                    ThrowError (1, "to many values %s (%i)", word, *lc);
                    return 1;
            }

        } else if (*hz == '/') {
            count = 0;
            state++;
            hz++;
        } else {
            ThrowError (1, "cannot convert values %s (%i)", word, *lc);
            return 1;
        }
    }
    *hpa = '\0';
    *hpb = '\0';
    *a = atof (str_a);
    *b = atof (str_b);
    return 0;
}


/* Read double values from the file and check the syntax */
int ReadDoubleValueFromFile (FILE *fh, double *v)
{
    int c, count = 0, x, point = 0;
    char txt[100], *p = txt;

    c = fgetc (fh);
    if ((c == '+') || (c == '-')) {    /* [+-]? */
        *p++ = (char)c;
        count++;
        c = fgetc (fh);
    }

    x = 0;                             /* [0-9]? */
    while ((c >= '0') && (c <= '9')) {
        if (++count >= 100) return -1;
        x++;
        *p++ = (char)c;
        c = fgetc (fh);
    }
    if (x == 0) return -1;

    if (c == '.') {                    /* [.]? */
        if (++count >= 100) return -1;
        point = 1;
        *p++ = (char)c;
        c = fgetc (fh);
    }

    x = 0;                             /* [0-9]? */
    while ((c >= '0') && (c <= '9')) {
        if (++count >= 100) return -1;
        x++;
        *p++ = (char)c;
        c = fgetc (fh);
    }
    if (point && (x == 0)) return -1;

    if ((c == 'e') || (c == 'E')) {    /* [eE]? */
        *p++ = (char)c;
        if (++count >= 100) return -1;
        c = fgetc (fh);

        if ((c == '+') || (c == '-')) {    /* [+-]? */
            *p++ = (char)c;
            if (++count >= 100) return -1;
            c = fgetc (fh);
        }

        x = 0;                             /* [0-9]* */
        while ((c >= '0') && (c <= '9')) {
            if (++count >= 100) return -1;
            x++;
            *p++ = (char)c;
            c = fgetc (fh);
        }
        if (x == 0) return -1;
    }
    *p = '\0';
    if (v != NULL) *v = atof (txt);
    ungetc (c, fh);
    return 0;
}


static int ReadEquationTillClosedBracketFromFile (FILE *fh, struct EXEC_STACK_ELEM **exec_stackp, int *lc)
{
    int c, klammer_count = 0;
    int count = 0;
    char txt[2048], *p = txt;

    do {
        c = fgetc (fh);
        if (c == ')') klammer_count--;
        if (c == '(') klammer_count++;
        if (c == EOF) {
            ThrowError (1, "unexpected end of file in equation (%i)\n", *lc);
            *txt = '\0';
            return -1;
        }
        if (isspace (c)) {
            if (c == '\n') *lc += 1;
            continue;
        }
        if (++count >= 2047) return -1;
        *p++ = (char)c;
    } while ((c != ')') || (klammer_count > 0));
    *p = '\0';

    if (exec_stackp != NULL) {
        /* Translate equation into an execution stack */
        *exec_stackp = solve_equation (txt);
        RegisterEquation (-1, txt, *exec_stackp, "", EQU_TYPE_RAMPE);
    }
    return 0;
}


static int ReadValuePairFromFile (FILE *fh, double *value, double *time,
                                  struct EXEC_STACK_ELEM **ampl_exec_stack,
                                  struct EXEC_STACK_ELEM **time_exec_stack, int *lc)
{
    int c;

    do {
        c = fgetc (fh);
        if (c == ';') do {
            c = fgetc (fh);
            if (c == '\n') *lc += 1;
        } while ((c != '\n') && (c != EOF));
    } while (isspace(c));
    if (c == EOF) return EOF;
    ungetc (c, fh);
    if (c == '(') {
        if (ReadEquationTillClosedBracketFromFile (fh, ampl_exec_stack, lc)) {
            ThrowError (1, "syntax error in equation (%i)", *lc);
            return -1;
        }
    } else {
        if (ReadDoubleValueFromFile (fh, value)) {
            ThrowError (1, "syntax error in value (%i)", *lc);
            return -1;
        }
    }

    do { c = fgetc (fh); if (c == '\n') *lc += 1; } while (isspace(c));
    if (c != '/') {
        ThrowError (1, "syntax error missing '/' token between value and time (%i)", *lc);
        return -1;
    }
    do { c = fgetc (fh); if (c == '\n') *lc += 1; } while (isspace(c));
    ungetc (c, fh);

    if (c == '(') {
        if (ReadEquationTillClosedBracketFromFile (fh, time_exec_stack, lc)) {
            ThrowError (1, "syntax error in equation (%i)", *lc);
            return -1;
        }
    } else {
        if (ReadDoubleValueFromFile (fh, time)) {
            ThrowError (1, "syntax error in time (%i)", *lc);
            return -1;
        }
    }
    return 0;
}


#define CLOSE_FILE(fh) \
    { close_file (fh); fh = NULL; }

void cyclic_async_rampen (void)
{
    FILE *rampen_fh;
    char word[BBVARI_NAME_SIZE];
    RAMPEN_INFO *rpi;
    int wp_count = 0;
    int r_count;
    int run;
    int x, c;
    char fname[MAX_PATH];
    static int rampen_state = WAIT_RAMPEN_FILENAME;
    int pid;
    static int pid_ack;
    int linecounter;

    MESSAGE_HEAD mhead;

    if (test_message (&mhead)) {
        switch (mhead.mid) {
        case RAMPEN_START:
            pid_ack = mhead.pid;
            if (rampen_state == READ_RAMPEN_FILE_DONE) {
                if ((pid = get_pid_by_name (PN_SIGNAL_GENERATOR_CALCULATOR)) <= 0) {
                    ThrowError (1, "SignalGeneratorCalculator prozess not started");
                } else {
                    write_message (pid, RAMPEN_START, 0, NULL);
                    if (vid_state_rampen > 0) {
                        write_bbvari_ubyte (vid_state_rampen, 0);  /* XilEnv.Generator */
                    }
                    rampen_state = CALCULATE_RAMPEN;
                }
            } else ThrowError (1, "no ramp file loaded");
            remove_message ();
            break;
        case RAMPEN_STOP:
            remove_message ();
            rampen_state = WAIT_RAMPEN_FILENAME;
            if ((pid = get_pid_by_name (PN_SIGNAL_GENERATOR_CALCULATOR)) <= 0) {
                ThrowError (1, "SignalGeneratorCalculator prozess not running");
            } else {
                if (mhead.pid == pid) { /* Was transmitted from remote process */
                    cleanup_rampenstruct ();
                } else {                /* otherwise it was transmitted locally */
                    write_message (pid, RAMPEN_STOP, 0, NULL);
                }
            }
            break;
        case RAMPEN_FILENAME:
            if (rampen_state == WAIT_RAMPEN_FILENAME) {
                rampen_state = READ_RAMPEN_FILE;
                read_message (&mhead, fname, MAX_PATH);
            } else {
                ThrowError (1, "Ramps are already running. You have to stop the ranps before you can start them again");
                remove_message ();
            }
            break;
        case RAMPEN_ACK:
            write_message (pid_ack, RAMPEN_ACK, 0, NULL);
            remove_message ();
            break;
        default:
            ThrowError (1, "receive unknown message %i from %i",
                   mhead.mid, mhead.pid);
            remove_message ();
        }
    }

    if (rampen_state == READ_RAMPEN_FILE) {
        rpi = rampen_info_struct;           /* shortened notation */

        rpi->anz_rampen = 0;
        rpi->trigger_state = TRIGGER_STATE_OFF;
        for (run = 1; run <= 2; run++) {
            r_count = 0;

            if ((rampen_fh = open_file (fname, "rt")) == NULL) {
                ThrowError (1, "cannot open file %s", fname);
                rampen_state = WAIT_RAMPEN_FILENAME;
                return;
            }
            linecounter = 1;

            if (run == 2) {
                /* Alloc memory for the ramps */
                for (x = 0; x < rpi->anz_rampen; x++) {
                    if ((rpi->rampen[x].ampl = (double*)my_malloc ((size_t)rpi->rampen[x].size * sizeof (double))) == NULL) {
                        ThrowError (1, "out of memory cannot store ramps");
                        rampen_state = WAIT_RAMPEN_FILENAME;
                        CLOSE_FILE (rampen_fh);
                        return;
                    }
                    if ((rpi->rampen[x].time = (double*)my_malloc ((size_t)rpi->rampen[x].size * sizeof (double))) == NULL) {
                        ThrowError (1, "out of memory cannot store ramps");
                        rampen_state = WAIT_RAMPEN_FILENAME;
                        CLOSE_FILE (rampen_fh);
                        return;
                    }
                    if ((rpi->rampen[x].ampl_exec_stack = (struct EXEC_STACK_ELEM**)my_calloc ((size_t)rpi->rampen[x].size, sizeof (struct EXEC_STACK_ELEM*))) == NULL) {
                        ThrowError (1, "out of memory cannot store ramps");
                        rampen_state = WAIT_RAMPEN_FILENAME;
                        CLOSE_FILE (rampen_fh);
                        return;
                    }
                    if ((rpi->rampen[x].time_exec_stack = (struct EXEC_STACK_ELEM**)my_calloc ((size_t)rpi->rampen[x].size, sizeof (struct EXEC_STACK_ELEM*))) == NULL) {
                        ThrowError (1, "out of memory cannot store ramps");
                        rampen_state = WAIT_RAMPEN_FILENAME;
                        CLOSE_FILE (rampen_fh);
                        return;
                    }
                }
            }

            if (read_word_lc (rampen_fh, word, 256, &linecounter) == EOF) {
                ThrowError (1, "no RAMPE defined in file %s", fname);
                rampen_state = WAIT_RAMPEN_FILENAME;
                CLOSE_FILE (rampen_fh);
                return;
            }

            /* Trigger token */
            if (!strcmp ("TRIGGER", word)) {
                rpi->trigger_flag = 1; /* there was a trigger variable defined */

                if (read_word_lc (rampen_fh, word, 256, &linecounter) == EOF) {
                    ThrowError (1, "incorrect trigger condition %s (%i)", fname, linecounter);
                    rampen_state = WAIT_RAMPEN_FILENAME;
                    CLOSE_FILE (rampen_fh);
                    return;
                }

                strncpy (rpi->trigger_var, word, BBVARI_NAME_SIZE);

                if (read_word_lc (rampen_fh, word, 256, &linecounter) == EOF) {
                    ThrowError (1, "incorrect trigger condition %s (%i)", fname, linecounter);
                    rampen_state = WAIT_RAMPEN_FILENAME;
                    CLOSE_FILE (rampen_fh);
                    return;
                }

                if (word[0] == '>') {
                    rpi->trigger_flag = 1;
                } else if (word[0] == '<') {
                    rpi->trigger_flag = 2;
                } else {
                    ThrowError (1, "'%s' is an incorrect trigger condition only  > or < are allowed in TRIGGER line %s (%i)", word, fname, linecounter);
                    rampen_state = WAIT_RAMPEN_FILENAME;
                    CLOSE_FILE (rampen_fh);
                    return;
                }

                /* Trigger value */
                if (ReadDoubleValueFromFile (rampen_fh, &(rpi->trigger_schw))) {
                    ThrowError (1, "incorrect trigger value %s (%i)", fname, linecounter);
                    rampen_state = WAIT_RAMPEN_FILENAME;
                    CLOSE_FILE (rampen_fh);
                    return;
                }
            } else {
                rpi->trigger_flag = 0;
            }

            /* Ramps */
            while (1) {
                if ((rpi->trigger_flag != 0) || (r_count != 0)) {
                    if (read_word_lc (rampen_fh, word, 256, &linecounter) == EOF) break;  // while (1)
                }
                if (strcmp ("RAMPE", word) && strcmp ("RAMPEPLUS", word)) {
                    if (strlen (word) > 0) {
                        ThrowError (1, "token 'RAMPE' or 'RAMPEPLUS' expected %s (%i)", fname , linecounter);
                        rampen_state = WAIT_RAMPEN_FILENAME;
                        CLOSE_FILE (rampen_fh);
                        return;
                    }
                } else {
                        if (run == 1) {
                            rpi->anz_rampen++;
                            rpi->rampen[r_count].size = 0;
                        }
                        if (!strcmp ("RAMPE", word))
                            rpi->rampen[r_count].type = RAMPE_NORMAL;
                        else if (!strcmp ("RAMPEPLUS", word))
                            rpi->rampen[r_count].type = RAMPE_ADDIEREN;

                        rpi->rampen[r_count].count = 0;
                        rpi->rampen[r_count].state = RAMPE_WARTET;
                        wp_count = 0;
                        r_count++;

                        if (r_count > MAX_RAMPEN) {
                            ThrowError (1, "too many ramps defined only %i ramps allowed %s", (int)MAX_RAMPEN, fname);
                            rampen_state = WAIT_RAMPEN_FILENAME;
                            CLOSE_FILE (rampen_fh);
                            return;
                        }

                        /* Read namen of the variable */
                        if (read_word_lc (rampen_fh, word, sizeof(word), &linecounter) == EOF) {
                            ThrowError (1, "unexpected ende of file %s (%i)", fname, linecounter);
                            rampen_state = WAIT_RAMPEN_FILENAME;
                            CLOSE_FILE (rampen_fh);
                            return;
                        }
                        strncpy (rpi->rampen[r_count-1].name, word, BBVARI_NAME_SIZE);

                        do {
                            do { c = fgetc (rampen_fh); if (c == '\n') linecounter++; } while (isspace(c));
                            ungetc (c, rampen_fh);
                            while (c == ';') {
                                do { c = fgetc (rampen_fh); if (c == '\n') linecounter++; } while ((c != '\n') && (c != EOF));
                                do { c = fgetc (rampen_fh); if (c == '\n') linecounter++; } while (isspace(c));
                                ungetc (c, rampen_fh);
                            }
                            if ((c == 'R') || (c == EOF)) break; /* New ramp or file end */
                            if (run == 2) {
                                if (ReadValuePairFromFile (rampen_fh,
                                        &(rpi->rampen[r_count-1].ampl[wp_count]),
                                        &(rpi->rampen[r_count-1].time[wp_count]),
                                        &(rpi->rampen[r_count-1].ampl_exec_stack[wp_count]),
                                        &(rpi->rampen[r_count-1].time_exec_stack[wp_count]), &linecounter)) {

                                    rampen_state = WAIT_RAMPEN_FILENAME;
                                    break;
                                }
                                if (wp_count > 0) {
                                    if ((rpi->rampen[r_count-1].time[wp_count] < rpi->rampen[r_count-1].time[wp_count-1]) &&
                                        (rpi->rampen[r_count-1].time_exec_stack[wp_count] == NULL) &&
                                        (rpi->rampen[r_count-1].time_exec_stack[wp_count-1] == NULL)) {
                                        ThrowError (1, "time values not in the right order %s (%i)", fname, linecounter);
                                        rampen_state = WAIT_RAMPEN_FILENAME;
                                        break;
                                    }
                                }
                            } else if (run == 1) {
                                if (ReadValuePairFromFile (rampen_fh, NULL, NULL, NULL, NULL, &linecounter)) {
                                    rampen_state = WAIT_RAMPEN_FILENAME;
                                    break;
                                }
                                rpi->rampen[r_count-1].size++;
                            }
                            wp_count++;
                        } while (1);
                    }
            }
            CLOSE_FILE (rampen_fh);
        }

        if ((rpi->vids[0] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME), BB_DOUBLE, "s")) < 0) {
            ThrowError (1, "cannot read variable 'abt_per'");
            rampen_state = WAIT_RAMPEN_FILENAME;
            return;
        }
        if (rpi->trigger_flag) {
            if ((rpi->vids[1] = add_bbvari (rpi->trigger_var, BB_UNKNOWN, NULL)) < 0) {
                ThrowError (1, "cannot read trigger variable '%s'", rpi->trigger_var);
                rampen_state = WAIT_RAMPEN_FILENAME;
                return;
            }
        } else {
            /* If there are no trigger variable fill up the frame */
            if ((rpi->vids[1] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME), BB_DOUBLE, "s")) < 0) {
                ThrowError (1, "cannot read variable 'abt_per'");
                rampen_state = WAIT_RAMPEN_FILENAME;
                return;
            }
        }

        /* Register the ramps variable inside the blackboard */
        for (x = 0; x < rpi->anz_rampen; x++) {
            if ((rpi->vids[x+2] = add_bbvari (rpi->rampen[x].name, BB_UNKNOWN, NULL)) < 0) {
                rpi->vids[x+2] = add_bbvari (rpi->rampen[x].name, BB_DOUBLE, NULL);
            }
        }
        rpi->vids[x+3] = 0;

        if ((vids_rampen = (int*)my_malloc ((size_t)(rpi->anz_rampen+3) * sizeof (int))) == NULL) {
            ThrowError (ERROR_CRITICAL, "out of memory");
            rampen_state = WAIT_RAMPEN_FILENAME;
            return;
        }
        for (x = 0;  x < rpi->anz_rampen+2; x++)
            vids_rampen[x] = rpi->vids[x];
        vids_rampen[x] = 0;

        rampen_state = READ_RAMPEN_FILE_DONE;
    }
}


void terminate_async_rampen (void)
{
    PID pid;

    if ((pid = get_pid_by_name (PN_SIGNAL_GENERATOR_CALCULATOR)) > 0) {
        terminate_process (pid);
    }
    cleanup_rampenstruct ();
    if (rampen_info_struct != NULL) my_free (rampen_info_struct);
}

int init_async_rampen (void)
{
    if (rampen_info_struct == NULL) {
        if ((rampen_info_struct = my_calloc (1, sizeof (RAMPEN_INFO))) == NULL) {
            return -1;
        }
    }
    return 0;
}



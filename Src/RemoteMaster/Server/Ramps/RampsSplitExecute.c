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
#include "RampsStruct.h"
#define RAMPSSPLITEXECUTE_C
#include "RampsSplitExecute.h"
#include "ExecutionStack.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "Message.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "TimeStamp.h"

static void cleanup_rampenstruct (void);

static RAMPEN_INFO ris;
static VID vid_state_rampen;
/* Zeiger auf Blackboard-Zugriffs-ID */
static VID *vids_rampen;
/* verstrichene Zeit ab der Rampen-Fkt. laufen */
static double begin_rampen_time;


void cyclic_rampen (void)
{
    int x;
    double value;
    double dt;
    double trigger_value;
    RAMPEN_ELEM *hrp;
    /* verstrichene Zeit ab der Rampen-Fkt. laufen */
    static double begin_rampen_time;
    int alle_rampen_beendet;
    MESSAGE_HEAD mhead;
    static int rc, tsc, asc, aesc, tesc;
    int max_message_per_cycle = 100;
    static int err_counter;

    while (test_message (&mhead) && (max_message_per_cycle-- > 0)) {
        switch (mhead.mid) {
        case RAMPEN_VIDS:
            rc = 0;        // Rampenzaehler nullsetzen
            err_counter = 0;
            vids_rampen = (VID*)my_malloc (mhead.size);
            if (vids_rampen == NULL) {
                remove_message ();
                ThrowError(1, "out of memory");
                err_counter++;
            } else {
                read_message (&mhead, (char*)vids_rampen, mhead.size);
                for (x = 0; x < (int)mhead.size / (int)sizeof (VID); x++) {
                    attach_bbvari (vids_rampen[x]);
                }
            }
            break;
        case RAMPEN_ANZAHL_STUETZSTELLEN:
            if (err_counter) {
                remove_message ();
                break;
            }
            if (mhead.size == sizeof (int)) {
                read_message (&mhead, (char*)&(ris.rampen[rc].size), sizeof (int));
                x = ris.rampen[rc].size;
                if (x <= 0) {
                    ThrowError(1, "RAMPE without sample points not allowed");
                    err_counter++;
                } else {
                    ris.rampen[rc].ampl = (double*)my_malloc (sizeof (double) * x);
                    ris.rampen[rc].time = (double*)my_malloc (sizeof (double) * x);
                    ris.rampen[rc].ampl_exec_stack = (struct EXEC_STACK_ELEM**)my_malloc (sizeof (void*) * x);
                    ris.rampen[rc].time_exec_stack = (struct EXEC_STACK_ELEM**)my_malloc (sizeof (void*) * x);
                    if ((ris.rampen[rc].ampl == NULL) ||
                        (ris.rampen[rc].time == NULL) ||
                        (ris.rampen[rc].ampl_exec_stack == NULL) ||
                        (ris.rampen[rc].time_exec_stack == NULL)) {
                        ThrowError(1, "out of memory");
                        err_counter++;
                    }
                }
                tsc = 0;  // Stuetzstellen Zaehler (time) nullsetzen
                asc = 0;  // Stuetzstellen Zaehler (ampl) nullsetzen
                tesc = 0;  // Stuetzstellen Zaehler (time_exec_stack) nullsetzen
                aesc = 0;  // Stuetzstellen Zaehler (ampl_exec_stack) nullsetzen
                rc++;
            } else {
                remove_message ();
                ThrowError(1, "RAMPEN_ANZAHL_STUETZSTELLEN != sizeof (int)!!!");
                err_counter++;
            }
            break;
        case RAMPEN_ZEIT_STUETZSTELLE:
            if (err_counter) {
                remove_message ();
                break;
            }
            if (mhead.size == sizeof (double)) {
                read_message (&mhead, (char*)&(ris.rampen[rc-1].time[tsc]), sizeof (double));
                tsc++;
            } else {
                remove_message ();
                ThrowError(1, "RAMPEN_ZEIT_STUETZSTELLE != sizeof (double)!!!");
                err_counter++;
            }
            break;
        case RAMPEN_AMPL_STUETZSTELLE:
            if (err_counter) {
                remove_message ();
                break;
            }
            if (mhead.size == sizeof (double)) {
                read_message (&mhead, (char*)&(ris.rampen[rc-1].ampl[asc]), sizeof (double));
                asc++;
            } else {
                remove_message ();
                ThrowError(1, "RAMPEN_AMPL_STUETZSTELLE != sizeof (double)!!!");
                err_counter++;
            }
            break;
        case RAMPEN_ZEIT_EXEC_STACK_STUETZSTELLE:
            if (err_counter) {
                remove_message ();
                break;
            }
            if (mhead.size == 0) {
                remove_message ();
                ris.rampen[rc-1].time_exec_stack[tesc] = NULL;
            } else {
                ris.rampen[rc-1].time_exec_stack[tesc] = (struct EXEC_STACK_ELEM*)my_malloc (mhead.size);
                if (ris.rampen[rc-1].time_exec_stack[tesc] == NULL) {
                     remove_message ();
                     ThrowError(1, "out of memory");
                     err_counter++;
                } else {
                     read_message (&mhead, (char*)ris.rampen[rc-1].time_exec_stack[tesc], mhead.size);
                     attach_exec_stack (ris.rampen[rc-1].time_exec_stack[tesc]);
                }
            }
            tesc++;
            break;
        case RAMPEN_AMPL_EXEC_STACK_STUETZSTELLE:
            if (err_counter) {
                remove_message ();
                break;
            }
            if (mhead.size == 0) {
                remove_message ();
                ris.rampen[rc-1].ampl_exec_stack[aesc] = NULL;
            } else {
                ris.rampen[rc-1].ampl_exec_stack[aesc] = (struct EXEC_STACK_ELEM*)my_malloc (mhead.size);
                if (ris.rampen[rc-1].ampl_exec_stack[aesc] == NULL) {
                     remove_message ();
                     ThrowError(1, "out of memory");
                     err_counter++;
                } else {
                     read_message (&mhead, (char*)ris.rampen[rc-1].ampl_exec_stack[aesc], mhead.size);
                     attach_exec_stack (ris.rampen[rc-1].ampl_exec_stack[aesc]);
                }
            }
            aesc++;
            break;

        case RAMPEN_START:
            if (err_counter) {
                remove_message ();
                ThrowError(1, "RAMPE will stopped immediately because of syntax errors in *.gen file");
                write_message (mhead.pid, RAMPEN_ACK, 0, NULL);
                write_message (mhead.pid, RAMPEN_STOP, 0, NULL);  // Ende der Rampe wegen Fehler gleich Bestaetigung senden
                break;
            }
            ris.trigger_state = TRIGGER_STATE_WAIT_SMALLER;
            ris.anz_rampen = rc;
            {
                struct {
                    int trigger_flag;
                    int fill;
                    double trigger_schw;
                } start_message;   // Achtung Struktur muss identisch im Usermode Programm sein
                read_message (&mhead, (char*)&(start_message), sizeof (start_message));
                ris.trigger_flag = start_message.trigger_flag;
                ris.trigger_schw = start_message.trigger_schw;
                //error (1, "ris.trigger_schw = %i", (int)ris.trigger_schw);
                write_message (mhead.pid, RAMPEN_ACK, 0, NULL);
            }
            break;
        case RAMPEN_STOP:
            ris.trigger_state = TRIGGER_STATE_OFF;
            alle_rampen_beendet = 1;
            remove_message ();
            goto RAMPEN_STOP_MESSAGE;   /* ugly ugly ugly */
        default:
            ThrowError(1, "receive unknown message %i von %i",
                   mhead.mid, mhead.pid);
            remove_message ();
        }
    }

    /* Triggern */
    if ((ris.trigger_state == TRIGGER_STATE_WAIT_SMALLER) ||
        (ris.trigger_state == TRIGGER_STATE_WAIT_LARGER)) {  /* Rampen laufen noch nicht */
        write_bbvari_ubyte (vid_state_rampen, 1); /* Generator wartet auf Trigger */
        if (ris.trigger_flag == 0) {
            ris.trigger_state = TRIGGER_STATE_TRIGGER;
            begin_rampen_time = 0.0;
            write_bbvari_ubyte (vid_state_rampen, 2); /* Generator laeuft */
        } else {
            trigger_value = read_bbvari_convert_double (vids_rampen[1]);

            if (ris.trigger_state == TRIGGER_STATE_WAIT_SMALLER) {
                if (ris.trigger_flag == 1) {
                    if (trigger_value < ris.trigger_schw)
                        ris.trigger_state = TRIGGER_STATE_WAIT_LARGER;
                } else {
                    if (trigger_value > ris.trigger_schw)
                        ris.trigger_state = TRIGGER_STATE_WAIT_LARGER;
                }
            } else if (ris.trigger_state == TRIGGER_STATE_WAIT_LARGER) {
                if (ris.trigger_flag == 1) {
                    if (trigger_value > ris.trigger_schw) {
                        ris.trigger_state = TRIGGER_STATE_TRIGGER;
                        begin_rampen_time = 0.0;
                        write_bbvari_ubyte (vid_state_rampen, 2); /* Generator */
                        //SYNC_WRITE_UBYTE(frame_state_rampen, 1, 0); /* Gen_Start */
                    }
                } else {
                    if (trigger_value < ris.trigger_schw) {
                        ris.trigger_state = TRIGGER_STATE_TRIGGER;
                        begin_rampen_time = 0.0;
                        write_bbvari_ubyte (vid_state_rampen, 2); /* Generator */
                        //SYNC_WRITE_UBYTE(frame_state_rampen, 1, 0); /* Gen_Start */
                    }
                }
            }
        }
    }
    if (ris.trigger_state == TRIGGER_STATE_TRIGGER) {

        /* lese Zeitraster aus BB */
        dt = read_bbvari_double (vids_rampen[0]) * (double)sync_rampen_tcb.time_steps;

        alle_rampen_beendet = 1;

        /* bearbeite alle Rampen in einem Zeittakt */
        for (x = 0; x < ris.anz_rampen; x++) {
            /* verkuerzte Schreibweise */
            hrp = &ris.rampen[x];
            /* Rampe laeuft gerade */
            if (hrp->state != RAMPE_BEENDET) { /* 15.09.95 Bi */
                /* naechste Stuetzstelle ? */
                if (hrp->time[hrp->count] <= begin_rampen_time) {
                    /* weitere Stuetzstellen vorhanden */
                    if (hrp->count < hrp->size-1) {
                        /* wenn Gleichungen vorhanden hier berechnen */
                        if (hrp->count == 0) {
                            if (hrp->ampl_exec_stack[hrp->count] != NULL)
                                hrp->ampl[hrp->count] = execute_stack (hrp->ampl_exec_stack[hrp->count]);
                            if (hrp->time_exec_stack[hrp->count] != NULL)
                                hrp->time[hrp->count] = execute_stack (hrp->time_exec_stack[hrp->count]);
                        }
                        hrp->dt =  begin_rampen_time - hrp->time[hrp->count];

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
                    } else {   /* keine weiterer Stuetzpunkt */
                        if (hrp->ampl_exec_stack[hrp->count] != NULL)
                            hrp->ampl[hrp->count] = execute_stack (hrp->ampl_exec_stack[hrp->count]);
                        value = hrp->ampl[hrp->count];
                        /* 15.09.95 Bi */
                        hrp->state = RAMPE_WIRD_BEENDET;
                    }
                } else {
                    if (hrp->state == RAMPE_LAEUFT) {
                        hrp->dt += dt;
                        value = hrp->ampl[hrp->count-1] + hrp->steig * hrp->dt;
                    }
                }
            }
            if ((hrp->state == RAMPE_LAEUFT) || (hrp->state == RAMPE_WIRD_BEENDET)) {
                /* 15.09.95 Bi */
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
        begin_rampen_time += dt;


        /* 15.09.95 Bi */
        if (read_bbvari_ubyte (vid_state_rampen) == 0) { /* Generator */
            alle_rampen_beendet = 1;
        }

RAMPEN_STOP_MESSAGE:
        /* wenn letzte Rampe beendet ist melde dies an async-Prozess */
        if (alle_rampen_beendet) {
            PID pid;
            pid = get_pid_by_name (SIGNAL_GENERATOR_COMPILER);
            if (pid > 0) {
                write_message (pid, RAMPEN_STOP, 0, NULL);
            }
            ris.trigger_state = TRIGGER_STATE_OFF;
                                         /* TRIGGER_STATE_WAIT_SMALLER; */
            write_bbvari_ubyte (vid_state_rampen, 0);  /* Generator */
            for (x = 0; x < ris.anz_rampen; x++) {
                ris.rampen[x].count = 0;
                ris.rampen[x].state = RAMPE_WARTET;
            }
            cleanup_rampenstruct ();  // alles wieder freigeben
        }
    }
}

int init_rampen (void)
{
    vid_state_rampen = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_GENERATOR), BB_UBYTE, NULL);
    if (vid_state_rampen <= 0) {
        ThrowError(1, "cannot attach variable 'Generator'");
        return 1;
    }
    set_bbvari_conversion(vid_state_rampen, 2, "0 0 \"Sleep\"; 1 1 \"Trigger\"; 2 2 \"Play\";");
    write_bbvari_ubyte (vid_state_rampen, 0);  /* Generator */
    return 0;
}

void terminate_rampen (void)
{
    remove_bbvari (vid_state_rampen);
    vid_state_rampen = 0L;
}

static void cleanup_rampenstruct (void)
{
    /* raeumt noch nicht komplett auf !? */
    int x, y;
    for (x = 0; x < ris.anz_rampen; x++) {
        if (ris.rampen[x].ampl != NULL)
            my_free (ris.rampen[x].ampl);
        ris.rampen[x].ampl = NULL;
        if (ris.rampen[x].time)
            my_free (ris.rampen[x].time);
        ris.rampen[x].time = NULL;
        if (ris.rampen[x].ampl_exec_stack != NULL) {
            for (y = 0; y < ris.rampen[x].size; y++)
                remove_exec_stack (ris.rampen[x].ampl_exec_stack[y]);
            my_free (ris.rampen[x].ampl_exec_stack);
            ris.rampen[x].ampl_exec_stack = NULL;
        }
        if (ris.rampen[x].time_exec_stack != NULL) {
            for (y = 0; y < ris.rampen[x].size; y++)
                remove_exec_stack (ris.rampen[x].time_exec_stack[y]);
            my_free (ris.rampen[x].time_exec_stack);
            ris.rampen[x].time_exec_stack = NULL;
        }
    }
    ris.anz_rampen = 0;
    if (vids_rampen != NULL) {
        //set_owner_process (PN_SIGNAL_GENERATOR_CALCULATOR);
        for (x = 0; vids_rampen[x] > 0; x++)
            remove_bbvari (vids_rampen[x]);
        //set_owner_process (NULL);
        my_free (vids_rampen);
        vids_rampen = NULL;
    }
}


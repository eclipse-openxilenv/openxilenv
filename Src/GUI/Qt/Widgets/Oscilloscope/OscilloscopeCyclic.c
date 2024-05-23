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
#include <float.h>

#include "Scheduler.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "Fifos.h"
#include "ReadFromBlackboardPipe.h"
#include "TraceRecorder.h"
#include "GetNotANumber.h"
#include "TimeStamp.h"
#include "Config.h"
#include "OscilloscopeData.h"
#include "OscilloscopeCyclic.h"


#define UNUSED(x) (void)(x)

static OSCILLOSCOPE_DATA *all_oszidatas[MAX_OSCILLOSCOPE_WINDOWS];
static int oszidata_count;
static double StaticNotANumber;

int RegisterOscilloscopeToCyclic (OSCILLOSCOPE_DATA *poszidata)
{
    int x;

    EnterOsziCycleCS ();
    for (x = 0; x < oszidata_count; x++) {
        if (all_oszidatas[x] == poszidata) {
            LeaveOsziCycleCS ();
            return -1;   // Is already inside
        }
    }
    if (x == MAX_OSCILLOSCOPE_WINDOWS) {
        LeaveOsziCycleCS ();
        return -2;   // To many oscilloscopes
    }
    all_oszidatas[x] = poszidata;
    oszidata_count++;
    LeaveOsziCycleCS ();
    return 0;
}

int UnregisterOscilloscopeFromCyclic (OSCILLOSCOPE_DATA *poszidata)
{
    int x;

    EnterOsziCycleCS ();
    for (x = 0; x < oszidata_count; x++) {
        if (all_oszidatas[x] == poszidata) {
            for (; x < (oszidata_count - 1); x++) {
                all_oszidatas[x] = all_oszidatas[x+1];
            }
            oszidata_count--;
            LeaveOsziCycleCS ();
            return 0;   // successfull deleted
        }
    }
    LeaveOsziCycleCS ();
    return -1; // not found
}


typedef struct VID_OSZIWIN_SRUCT {
    OSCILLOSCOPE_DATA *poszidata;
    int left_right;
    int pos;
     struct VID_OSZIWIN_SRUCT *next;
} VID_OSZIWIN_REFELEM;


static VID_OSZIWIN_REFELEM *owc_ref;
static size_t owc_ref_size = 1000;

static size_t vid_owc_size = 1000;
static int32_t *vids;
static char *dec_phys_flags;
static VID_OSZIWIN_REFELEM **vids_connect2;
static double /*float*/ *stamp;
static int oszi_vari_count;

static int reconnect_rdpipe = 0;    /* 0 -> Connection stays active 1 -> Connection terminate and restart */
static int connection_rdpipe = 0;   /* 0 -> Coonection not active */

static uint64_t oszi_ts_counter;
static uint64_t time_steps;

static CRITICAL_SECTION OsciCycleCriticalSection;

static CRITICAL_SECTION OsciWidgetCriticalSections[MAX_OSCILLOSCOPE_WINDOWS];
static int UsedFlagOsciWidgetCriticalSections[MAX_OSCILLOSCOPE_WINDOWS];


static int OscilloscopeReqFiFo;
static int OscilloscopeAckFiFo;
static int OscilloscopeDataFiFo;

int AllocOsciWidgetCriticalSection (void)
{
    int x;
    EnterOsziCycleCS ();
    for (x = 0; x < MAX_OSCILLOSCOPE_WINDOWS; x++) {
        if (UsedFlagOsciWidgetCriticalSections[x] == 0) {
            INIT_CS (&(OsciWidgetCriticalSections[x]));
            UsedFlagOsciWidgetCriticalSections[x] = 1;
            LeaveOsziCycleCS ();
            return x;
        }
    }
    LeaveOsziCycleCS ();
    return -1;
}

void EnterOsciWidgetCriticalSection (int par_Number)
{
    if ((par_Number < MAX_OSCILLOSCOPE_WINDOWS) && (par_Number >= 0)) {
        ENTER_CS (&(OsciWidgetCriticalSections[par_Number]));
    } else {
        ThrowError (1, "Internal error in %s (%i)", __FILE__, __LINE__);
    }
}

void LeaveOsciWidgetCriticalSection (int par_Number)
{
    if ((par_Number < MAX_OSCILLOSCOPE_WINDOWS) && (par_Number >= 0)) {
        LEAVE_CS (&(OsciWidgetCriticalSections[par_Number]));
    } else {
        ThrowError (1, "Internal error in %s (%i)", __FILE__, __LINE__);
    }
}

void DeleteOsciWidgetCriticalSection (int par_Number)
{
    if ((par_Number < MAX_OSCILLOSCOPE_WINDOWS) && (par_Number >= 0)) {
        EnterOsziCycleCS ();
        DELETE_CS (&(OsciWidgetCriticalSections[par_Number]));
        UsedFlagOsciWidgetCriticalSections[par_Number] = 0;
        LeaveOsziCycleCS ();
    } else {
        ThrowError (1, "Internal error in %s (%i)", __FILE__, __LINE__);
    }
}


void EnterOsziCycleCS (void)
{
    ENTER_CS (&OsciCycleCriticalSection);
}

void LeaveOsziCycleCS (void)
{
    LEAVE_CS (&OsciCycleCriticalSection);
}

static int mbuffsize;
static struct PIPE_VARI *mbuffer;

static void CheckBuffer (int par_Lock)
{
    static int BufferFlag = 0;
    if (par_Lock) EnterOsziCycleCS();

    if (!BufferFlag) {
        mbuffsize = 0;
        reconnect_rdpipe = 0;
        connection_rdpipe = 0;

        if (((owc_ref = (VID_OSZIWIN_REFELEM*)my_calloc (owc_ref_size, sizeof (VID_OSZIWIN_REFELEM))) == NULL) ||
            ((vids = (int32_t*)my_calloc (vid_owc_size, sizeof (int32_t))) == NULL) ||
            ((dec_phys_flags = (char*)my_calloc (vid_owc_size, sizeof (char))) == NULL) ||
            ((vids_connect2 = (VID_OSZIWIN_REFELEM**)my_calloc (vid_owc_size, sizeof (VID_OSZIWIN_REFELEM*))) == NULL) ||
             ((stamp = (double*)my_calloc (vid_owc_size, sizeof (double))) == NULL)) {
              if (par_Lock) LeaveOsziCycleCS();
              ThrowError (1, "not enough memory");
              return;
        }
        BufferFlag = 1;
    }
    if (par_Lock) LeaveOsziCycleCS();
}


void SwitchOscilloscopeOnline (OSCILLOSCOPE_DATA *poszidata, uint64_t par_StartTime, int par_EnterCS)
{
    int x;

    if (par_EnterCS) EnterOsciWidgetCriticalSection (poszidata->CriticalSectionNumber);
    poszidata->cycle_counter_at_start = get_timestamp_counter();
    poszidata->t_current_buffer_end = 0;

    poszidata->t_current_to_update_end = 0;
    poszidata->t_current_updated_end = 0;

    poszidata->time_step_cursor = 0;
    poszidata->t_cursor = 0;
    poszidata->t_ref_point = 0;
    poszidata->ref_point_flag = 0;
    poszidata->diff_to_ref_point_flag = 0;


    poszidata->t_window_start = par_StartTime;
    poszidata->t_window_end = poszidata->t_window_start + poszidata->t_window_size;
    poszidata->t_current_buffer_end =  poszidata->t_window_start;
    poszidata->t_current_to_update_end = poszidata->t_window_start;
    poszidata->t_current_updated_end = poszidata->t_window_start;

    poszidata->t_current_buffer_end_save = poszidata->t_current_buffer_end;
    poszidata->t_current_buffer_end_diff = 0;

    for (x = 0; x < 21; x++) {
        poszidata->wr_poi_left[x] = 0;
        poszidata->depth_left[x] = 0;
        poszidata->wr_poi_right[x] = 0;
        poszidata->depth_right[x] = 0;
        poszidata->new_since_last_draw_left[x] = 0;
        poszidata->new_since_last_draw_right[x] = 0;
    }

    if (par_EnterCS) LeaveOsciWidgetCriticalSection (poszidata->CriticalSectionNumber);
}

void SwitchAllSyncronOffline(void)
{
    int x;
    ENTER_CS (&OsciCycleCriticalSection);
    for (x = 0; x < oszidata_count; x++) {
        EnterOsciWidgetCriticalSection (all_oszidatas[x]->CriticalSectionNumber);
        if (all_oszidatas[x]->state) {
            all_oszidatas[x]->state = 0;
            all_oszidatas[x]->t_current_to_update_end = all_oszidatas[x]->t_current_buffer_end;
            all_oszidatas[x]->t_current_updated_end = all_oszidatas[x]->t_current_to_update_end;
            all_oszidatas[x]->t_cursor = all_oszidatas[x]->t_current_to_update_end;
            all_oszidatas[x]->t_window_end = all_oszidatas[x]->t_current_to_update_end;
            all_oszidatas[x]->t_window_start = all_oszidatas[x]->t_window_end - all_oszidatas[x]->t_window_size;
        }
        LeaveOsciWidgetCriticalSection (all_oszidatas[x]->CriticalSectionNumber);
    }
    LEAVE_CS (&OsciCycleCriticalSection);
}

static void stamp2oszibuffers (uint64_t timestamp)
{
    OSCILLOSCOPE_DATA *poszidata;
    VID_OSZIWIN_REFELEM *connect2;
    int x, pos, left_right;
    double /*float*/ *buffer;
    int wr_poi;
    int depth;
    uint64_t Time;
    int new_since_last_draw;

    ENTER_CS (&OsciCycleCriticalSection);

    Time = timestamp;
    // Increment all oscilloscope buffer by one time slot
    for (x = 0; x < oszidata_count; x++) {
        EnterOsciWidgetCriticalSection (all_oszidatas[x]->CriticalSectionNumber);
        if (all_oszidatas[x]->state) {
            all_oszidatas[x]->t_current_buffer_end = Time;
        }
    }

    for (x = 0; x < oszi_vari_count; x++) {
        for (connect2 = vids_connect2[x]; connect2 != NULL; connect2 = connect2->next) {
            poszidata = connect2->poszidata;
            if (poszidata->state) {
                pos = connect2->pos;
                left_right = connect2->left_right;
                if (left_right) {  /* Right */
                    wr_poi = poszidata->wr_poi_right[pos];
                    depth = poszidata->depth_right[pos];
                    buffer = poszidata->buffer_right[pos];
                    new_since_last_draw = poszidata->new_since_last_draw_right[pos];
                } else {           /* Left */
                    wr_poi = poszidata->wr_poi_left[pos];
                    depth = poszidata->depth_left[pos];
                    buffer = poszidata->buffer_left[pos];
                    new_since_last_draw = poszidata->new_since_last_draw_left[pos];
                }
                /* Tranfer stamp to buffer */
                if (buffer != NULL) buffer[wr_poi] = stamp[x];
                /* Increment write pointer */
                wr_poi++;
                if (wr_poi >= poszidata->buffer_depth) {
                    wr_poi -= poszidata->buffer_depth;
                }
                if (new_since_last_draw < poszidata->buffer_depth) new_since_last_draw++;
                else new_since_last_draw = poszidata->buffer_depth;
                if (depth < poszidata->buffer_depth) depth++;
                if (left_right) {  /* Right */
                    poszidata->wr_poi_right[pos] = wr_poi;
                    poszidata->depth_right[pos] = depth;
                    poszidata->new_since_last_draw_right[pos] = new_since_last_draw;
                } else {           /* Left */
                    poszidata->wr_poi_left[pos] = wr_poi;
                    poszidata->depth_left[pos] = depth;
                    poszidata->new_since_last_draw_left[pos] = new_since_last_draw;
                }
                if ((poszidata->state == 1) &&            /* It is online and */
                    (poszidata->trigger_flag == 2)) {     /* pre-tigger ? */
                    if (poszidata->pre_trigger_counter == 0) {
                        ; // do nothing
                    } else poszidata->pre_trigger_counter--;
                }
            }
        }
    }
    for (x = 0; x < oszidata_count; x++) {
        LeaveOsciWidgetCriticalSection (all_oszidatas[x]->CriticalSectionNumber);
    }

    LEAVE_CS (&OsciCycleCriticalSection);
}

static void message2stamp_copy (struct PIPE_VARI *mbuffer, int varicount)
{
    int x, y, y_save;

    ENTER_CS (&OsciCycleCriticalSection);
    y = 0;
    for (x = 0; x < varicount; x++) {
          for (y_save = y; y <= oszi_vari_count; y++) {
            if (vids[y] == 0) { /* Variable doesn't exist anymore */
                y = y_save;
                break;
            }
            if (vids[y] == mbuffer[x].vid) {
                switch (mbuffer[x].type) {
                case BB_BYTE:
                    stamp[y] = (double)mbuffer[x].value.b;
                    break;
                case BB_UBYTE:
                    stamp[y] = (double)mbuffer[x].value.ub;
                    break;
                case BB_WORD:
                    stamp[y] = (double)mbuffer[x].value.w;
                    break;
                case BB_UWORD:
                    stamp[y] = (double)mbuffer[x].value.uw;
                    break;
                case BB_DWORD:
                    stamp[y] = (double)mbuffer[x].value.dw;
                    break;
                case BB_UDWORD:
                    stamp[y] = (double)mbuffer[x].value.udw;
                    break;
                case BB_QWORD:
                    stamp[y] = (double)mbuffer[x].value.qw;
                    break;
                case BB_UQWORD:
                    stamp[y] = (double)mbuffer[x].value.uqw;
                    break;
                case BB_FLOAT:
                    stamp[y] = (double)mbuffer[x].value.f;
                    break;
                case BB_DOUBLE:
                    stamp[y] = mbuffer[x].value.d;
                    break;
                case BB_UNKNOWN_WAIT:
                    stamp[y] = StaticNotANumber;
                    break;
                default:
                    stamp[y] = 0.0;
                }
                y++;
                break;
            }
        }
    }
    LEAVE_CS (&OsciCycleCriticalSection);
}

void message2stamp (struct PIPE_VARI *mbuffer, uint64_t ts, int varicount)
{
    int i = 0;

    if (TSCOUNT_SMALLER_TS (oszi_ts_counter + time_steps/2, ts)) {
        message2stamp_copy (mbuffer, varicount);
    }
    while (TSCOUNT_SMALLER_TS (oszi_ts_counter + time_steps/2, ts)) {
        if (i++ > 100000L) {
            ThrowError (1, "internal error in oszicycl.c %i", (int)__LINE__);
            break;
        }
        stamp2oszibuffers (ts);
        oszi_ts_counter += time_steps;
    }
}


int add_connection_listelem (int x, int32_t vid, char dec_phys_flag, int left_right, int pos,
                             OSCILLOSCOPE_DATA *poszidata)
{
    UNUSED(vid);
    UNUSED(dec_phys_flag);
    VID_OSZIWIN_REFELEM *connect2, *connect2_prev = NULL;
    size_t i;

     /* Got to the end of the list */
    for (connect2 = vids_connect2[x]; connect2 != NULL; connect2 = connect2->next)
        connect2_prev = connect2;
    /* search an empty element */
    for (i = 0; i < owc_ref_size; i++) {
        if (owc_ref[i].poszidata == NULL) {
            owc_ref[i].poszidata = poszidata;
            owc_ref[i].left_right = left_right;
            owc_ref[i].pos = pos;
            owc_ref[i].next = NULL;
            if (connect2_prev == NULL) {  /* first list element */
                vids_connect2[x] = &owc_ref[i];
            } else {                      /* additional list element */
                connect2_prev->next = &owc_ref[i];
            }
            return 0;
        }
    }
    return -1;
}

int add_connection (int32_t vid, char dec_phys_flag, int left_right, int pos,
                    OSCILLOSCOPE_DATA *poszidata)
{
     int x, ret;

     /* Search if VID exists */
     for (x = 0; x < (int)vid_owc_size; x++) {
          if ((vids[x] == vid) && (dec_phys_flags[x] == dec_phys_flag)) {
                ret = (add_connection_listelem (x, vid, dec_phys_flag, left_right, pos, poszidata));
                return ret;
          }
     }
     /* VID doesn't exist therfor add a new entry */
     for (x = 0; x < (int)vid_owc_size; x++) {
          if (vids[x] == 0) {     /* found an empty element */
                vids[x] = vid;
                dec_phys_flags[x] = dec_phys_flag;
                reconnect_rdpipe = 1;
                ret = add_connection_listelem (x, vid, dec_phys_flag, left_right, pos, poszidata);
                return ret;
          }
     }
     return -1;
}

int remove_connection (int32_t vid, char dec_phys_flag, int left_right, int pos,
                       OSCILLOSCOPE_DATA *poszidata)
{
    VID_OSZIWIN_REFELEM *connect2, *connect2_prev = NULL;
    int x;

    for (x = 0; x < oszi_vari_count; x++) {
        if ((vids[x] == vid) && (dec_phys_flags[x] == dec_phys_flag)) {
            for (connect2 = vids_connect2[x]; connect2 != NULL; connect2 = connect2->next) {
                if ((connect2->poszidata == poszidata) &&
                    (connect2->left_right == left_right) &&
                    (connect2->pos == pos)) {
                     connect2->poszidata = NULL;
                     if (connect2_prev == NULL) {
                         if (connect2->next == NULL) { /* Was the last one */
                             vids[x] = 0;
                             dec_phys_flags[x] = 0;
                             vids_connect2[x] = NULL;
                             reconnect_rdpipe = 1;
                         } else {
                             vids_connect2[x] = connect2->next;
                         }
                     } else {
                         connect2_prev->next = connect2->next;
                     }
                     return 0;
                 }
                 connect2_prev = connect2;
             }
        }
    }
    return -1;
}

int update_vid_owc (OSCILLOSCOPE_DATA *poszidata)
{
     size_t x;
     int y, found;
     VID_OSZIWIN_REFELEM *connect2;

     ENTER_CS (&OsciCycleCriticalSection);
     if (get_pid_by_name (PN_OSCILLOSCOPE) <= 0) {
         LEAVE_CS (&OsciCycleCriticalSection);
         return -1;
     }

     CheckBuffer(0);

     if (vids_connect2 != NULL) {
         /* First delete all none existing connections */
         for (x = 0; x < (size_t)oszi_vari_count; x++) {
              connect2 = vids_connect2[x];
              for (connect2 = vids_connect2[x]; connect2 != NULL; connect2 = connect2->next) {
                    if (connect2->poszidata == poszidata) {
                         /* Find the connection and is it valid? */
                         if (connect2->left_right) {
                              if ((poszidata->vids_right[connect2->pos] != vids[x]) ||
                                  (poszidata->dec_phys_right[connect2->pos] != dec_phys_flags[x]))
                                    remove_connection (vids[x], dec_phys_flags[x], 1, connect2->pos, poszidata);
                         } else {
                              if ((poszidata->vids_left[connect2->pos] != vids[x]) ||
                                  (poszidata->dec_phys_left[connect2->pos] != dec_phys_flags[x]))
                                    remove_connection (vids[x], dec_phys_flags[x], 0, connect2->pos, poszidata);
                         }
                    }
              }
         }
         /* Than add all new connectionsn */
        for (y = 0; y < 21; y++) {
            if (poszidata->vids_left[y] > 0) {
                found = 0;
                for (x = 0; (x < vid_owc_size) && !found; x++) {
                    if ((poszidata->vids_left[y] == vids[x]) &&
                        (poszidata->dec_phys_left[y] == dec_phys_flags[x]))  {
                        for (connect2 = vids_connect2[x]; connect2 != NULL; connect2 = connect2->next) {
                            if ((connect2->poszidata == poszidata) &&
                                (connect2->pos == y) &&
                                (connect2->left_right == 0)) {
                                /* Connection already exists */
                                found = 1;
                                break;
                            }
                        }
                    }
                }
                if (!found) add_connection (poszidata->vids_left[y],
                                            poszidata->dec_phys_left[y],
                                            0, y, poszidata);
            }
            if (poszidata->vids_right[y] > 0) {
                found = 0;
                for (x = 0; (x < vid_owc_size) && !found; x++) {
                    if (poszidata->vids_right[y] == vids[x]) {
                        for (connect2 = vids_connect2[x]; connect2 != NULL; connect2 = connect2->next) {
                            if ((connect2->poszidata == poszidata) &&
                                (connect2->pos == y) &&
                                (connect2->left_right == 1)) {
                                /* Connection already exists */
                                found = 1;
                                break;
                            }
                        }
                    }
                }
                if (!found) add_connection (poszidata->vids_right[y],
                                            poszidata->dec_phys_right[y],
                                            1, y, poszidata);
              }
         }
        /* Concentrate vids */
        y = 0;
        for (x = 0; x < vid_owc_size; x++) {
            int32_t save_vid;
            char save_dec_phys_flag;
            VID_OSZIWIN_REFELEM *save_vids_connect2;
            double /*float*/ save_stamp;
            if (vids[x] > 0) {
                save_vid = vids[x];
                save_dec_phys_flag = dec_phys_flags[x];
                save_vids_connect2 = vids_connect2[x];
                save_stamp = stamp[x];
                vids[x] = 0;
                dec_phys_flags[x] = 0;
                vids_connect2[x] = NULL;
                stamp[x] = 0.0;
                vids[y] = save_vid;
                dec_phys_flags[y] = save_dec_phys_flag;
                stamp[y] = save_stamp;
                vids_connect2[y] = save_vids_connect2;
                y++;
            }
        }
        oszi_vari_count = y;   /* Store the number of variables */
    }
    LEAVE_CS (&OsciCycleCriticalSection);
    return 0;
}

static uint64_t wrflag;

static int ErrorTimoutFlag;
static uint64_t ErrorTimout;
static int logon_rdpipe (void)
{
    int pid;

    /* Search "TraceQueue" process */
    if ((pid = get_pid_by_name (PN_TRACE_QUEUE)) < 0) {
        if (!ErrorTimoutFlag) {
            ErrorTimoutFlag = 1;
            ErrorTimout = get_timestamp_counter() + 4 * get_sched_periode_timer_clocks64();
            return 1;  // have to wait
        } else {
            if (ErrorTimout < get_timestamp_counter()) {
                ErrorTimoutFlag = 0;
                ThrowError (1, "prozess 'TraceQueue' not found");
                return -1;
            } else {
                return 1; // have to wait
            }
        }
    } else ErrorTimoutFlag = 0;

    /* If there already exist a connection disconnect this */
    if (connection_rdpipe) {
        /* Send logoff to the "TraceQueue" process */
        if (WriteToFiFo (OscilloscopeReqFiFo, RDPIPE_LOGOFF, GET_PID(), get_timestamp_counter(), 0, NULL)) {
            ThrowError (1, "pipe overflow\n");
        }
        connection_rdpipe = 0;
    } else oszi_ts_counter = get_timestamp_counter ();

    if (oszi_vari_count) {       /* Select at least one signal */
        if ((OscilloscopeReqFiFo = TxAttachFiFo(GET_PID(), READ_PIPE_OSCILLOSCOPE_REQ_FIFO_NAME)) < 0) {
            if (!ErrorTimoutFlag) {
                ErrorTimoutFlag = 1;
                ErrorTimout = get_timestamp_counter() + 4 * get_sched_periode_timer_clocks64();
                return 1; // have to wait
            } else {
                if (ErrorTimout < get_timestamp_counter()) {
                    ErrorTimoutFlag = 0;
                    ThrowError (1, "fifo \"%s\" not found", READ_PIPE_OSCILLOSCOPE_REQ_FIFO_NAME);
                    return -1;
                } else {
                    return 1; // have to wait
                }
            }
        } else {
            ErrorTimoutFlag = 0;
         }

        /* Send logoin to the "TraceQueue" process */
        if (WriteToFiFo (OscilloscopeReqFiFo, RDPIPE_LOGON, GET_PID(), get_timestamp_counter(), (oszi_vari_count+1) * (int)sizeof (int32_t), (char*)vids)) {
            ThrowError (1, "pipe overflow\n");
            return -1;
        }
        if (WriteToFiFo (OscilloscopeReqFiFo, RDPIPE_DEC_PHYS_MASK, GET_PID(), get_timestamp_counter(), oszi_vari_count+1, dec_phys_flags)) {
            ThrowError (1, "pipe overflow\n");
            return -1;
        }
        /* Fetch free WR flag */
        if (get_free_wrflag ((PID)GET_PID (), &wrflag)) {
            ThrowError (1, "no free WR-flag");
            return -1;
        }
        /* Send WR flag "TraceQueue" process */
        if (WriteToFiFo (OscilloscopeReqFiFo, RDPIPE_SEND_WRFLAG, GET_PID(), get_timestamp_counter(), sizeof (uint64_t), (char*)&wrflag)) {
            ThrowError (1, "pipe overflow");
            return -1;
        }
        connection_rdpipe = 1;
    }
    return 0;
}

void cyclic_oszi (void)
{
    int varicount;
    FIFO_ENTRY_HEADER Header;
    int message_count = 0;

    if (reconnect_rdpipe) {
        switch (logon_rdpipe ()) {
        case 0:
            reconnect_rdpipe = 0;
            break;
        case 1:   // There is no connection to the "TraceQueue" process but wait
            break;
        case -1:  // Error case no connection to "TraceQueue" process
            reconnect_rdpipe = 0;
            connection_rdpipe = 0;
            break;
        }
    }
    if (connection_rdpipe) {
        while ((CheckFiFo (OscilloscopeDataFiFo, &Header) > 0) && (message_count++ < 20)) {
                                     /* Check if messages a received */
                                     /* Than read all messages  */
                                     /* but only 20 messages in one cycle */
            switch (Header.MessageId) {      /* which one */
            case RECORD_MESSAGE:    /* Data message */
                if (Header.Size > mbuffsize) {  /* Are the buffer large enough? */
                    mbuffsize = Header.Size;
                    mbuffer = (struct PIPE_VARI*)my_realloc (mbuffer, mbuffsize);
                }
                /* Read message content */
                ReadFromFiFo (OscilloscopeDataFiFo, &Header, (char*)mbuffer, mbuffsize);
                varicount = Header.Size / (int)sizeof (struct PIPE_VARI);

                message2stamp (mbuffer, Header.Timestamp, varicount);
                break;
            case RDPIPE_ACK:
                RemoveOneMessageFromFiFo (OscilloscopeDataFiFo);
                break;
            default:
                RemoveOneMessageFromFiFo (OscilloscopeDataFiFo);
                ThrowError (1, "unknown message  %i:  %i\n", Header.TramsmiterPid, Header.TramsmiterPid);
                break;
            }
        }
        if (CheckFiFo (OscilloscopeAckFiFo, &Header) > 0) {
            switch (Header.MessageId) {      /* If yes which one */
            case RDPIPE_ACK:
                RemoveOneMessageFromFiFo (OscilloscopeAckFiFo);
                break;
            default:
                RemoveOneMessageFromFiFo (OscilloscopeAckFiFo);
                ThrowError (1, "unknown message  %i:  %i\n", Header.TramsmiterPid, Header.TramsmiterPid);
                break;
            }
        }
    }
}

int init_oszi (void)
{
    oszi_ts_counter = 0UL;
    time_steps = get_sched_periode_timer_clocks64 ();

    OscilloscopeAckFiFo = CreateNewRxFifo (GET_PID(), 1024, READ_PIPE_OSCILLOSCOPE_ACK_FIFO_NAME);
    OscilloscopeDataFiFo = CreateNewRxFifo (GET_PID(), 1024*1024, READ_PIPE_OSCILLOSCOPE_DATA_FIFO_NAME);

    CheckBuffer(1);

    return 0;
}


void terminate_oszi (void)
{
    int pid;

    if (connection_rdpipe) {
       /* Search "TraceQueue" process */
       if ((pid = get_pid_by_name (PN_TRACE_QUEUE)) > 0) {
           /* Send login to "TraceQueue" process */
           WriteToFiFo (OscilloscopeReqFiFo, RDPIPE_LOGOFF, GET_PID(), get_timestamp_counter(), 0, NULL);
       }
    }
    ENTER_CS (&OsciCycleCriticalSection);
    if (owc_ref != NULL) my_free (owc_ref);
    owc_ref = NULL;
    if (vids != NULL) my_free (vids);
    vids = NULL;
    if (dec_phys_flags != NULL) my_free (dec_phys_flags);
    dec_phys_flags = NULL;
    if (vids_connect2 != NULL) my_free (vids_connect2);
    vids_connect2 = NULL;
    if (stamp != NULL) my_free (stamp);
    stamp = NULL;
    if (mbuffer != NULL) my_free (mbuffer);
    mbuffer = NULL;
    LEAVE_CS (&OsciCycleCriticalSection);

    DeleteFiFo (OscilloscopeAckFiFo, GET_PID());
    DeleteFiFo (OscilloscopeDataFiFo, GET_PID());
}


/* ------------------------------------------------------------ *\
 *    Task Control Block                                     *
\* ------------------------------------------------------------ */

TASK_CONTROL_BLOCK oszi_tcb =
    INIT_TASK_COTROL_BLOCK(OSCILLOSCOPE, INTERN_ASYNC, 212, cyclic_oszi, init_oszi, terminate_oszi, 4*1024*1024);
#if 0
{
     -1,                      /* Prozess nicht aktiv */
     OSCILLOSCOPE,            /* Prozess-Name */
     0,                       /* es handelt sich um einen SYNC-Prozess */
     0,                       /* Prozess Schlaeft = 0 */
     0,
     212,                     /* Prioritaet */
     cyclic_oszi,
     init_oszi,
     terminate_oszi,
     1000,                    /* Timeout */
     0,                       /* timecounter zu Anfang 0 */
     1,                       /* Prozess in jedem Zeittakt aufrufen */
     4*1024*1024, //524288, //262144,
     NULL,
     NULL,
     NULL
};
#endif

void InitOsciCycleCriticalSection (void)
{
    // check NAN
    double nan = GetNotANumber();
    if (!_isnan(nan)) {
        ThrowError (1, "NAN not correct");
    }
    INIT_CS (&OsciCycleCriticalSection);

    StaticNotANumber = GetNotANumber();

    CheckBuffer(1);
}

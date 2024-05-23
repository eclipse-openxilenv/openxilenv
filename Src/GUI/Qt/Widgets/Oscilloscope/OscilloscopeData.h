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


#ifndef OSCILLOSCOPEDATA_H
#define OSCILLOSCOPEDATA_H

#include <stdint.h>

#ifdef __GNUC__
#include <math.h>   // gcc NAN
#endif

#ifdef __cplusplus
#include <cfloat>
extern "C" {
#include "Wildcards.h"
}
#else
#include <float.h>
#include "Wildcards.h"  // wegen INCLUDE_EXCLUDE_FILTER
#endif


#define MAX_OSCILLOSCOPE_WINDOWS       100

#define OSCILLOSCOPE_DESC_FRAME_WIDTH  85
#define OSCILLOSCOPE_TIME_AXIS_HIGHT   25


#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,8)
typedef struct OSCILLOSCOPE_DATA_STRUCT {

    int CriticalSectionNumber;

    int state;            /* 0 -> off; 1 -> on; 2 -> trigger   */
    int left_axis_on;     /* Display left axis  */
    int right_axis_on;    /* Display right axis */

    int sel_left_right;   /* 1 -> right 0 -> left */
    int sel_pos_left;
    int sel_pos_right;

    long vids_left[21];    /* 21. are the trigger channel */
    long vids_right[21];
    char dec_phys_left[21];
    char dec_phys_right[21];
    char dec_hex_bin_left[21];   // 0 -> decimal, 1 -> hexadecimal, 2 -> binary
    char dec_hex_bin_right[21];
    char vars_disable_left[21];    /*  With this flag you can enable/disable diplaing signals */
    char vars_disable_right[21];

    char *name_left[21];
    char *name_right[21];

    int buffer_depth;
    double *buffer_left[21];
    double *buffer_right[21];
    int wr_poi_left[21];
    int depth_left[21];
    int wr_poi_right[21];
    int depth_right[21];

    int new_since_last_draw_left[21];
    int new_since_last_draw_right[21];

    uint64_t t_window_start;
    uint64_t t_window_end;
    uint64_t t_window_size;           /* t_window_size = t_window_end - t_window_start */
    double t_window_size_configered;  /* t_window_size without zoom */
    uint64_t t_step_win;              /* Step width if we reached the window border s*/
    uint64_t t_step;                  /* Step with in nanoseconds example: 10000000 for 10ms  */

    uint64_t cycle_counter_at_start;
    uint64_t time_step_cursor;
    uint64_t time_step_ref_point;

    uint64_t t_current_to_update_end;
    uint64_t  t_current_updated_end;
    uint64_t  t_current_buffer_end;

    uint64_t t_current_buffer_end_save;
    uint64_t t_current_buffer_end_diff;

    uint64_t  t_cursor;
    uint64_t  t_ref_point;       // Time point of the refernce for difference measurement
    int ref_point_flag;          // Is the rerence point active
    int diff_to_ref_point_flag;  // Is the difference measurement active
    double pixelpersec;
    int draw_widget_width;
    int window_step_width;    /* Scrolling steps if ttme reach window border (10%, 20%, 50% or 100%) */

    int zoom_pos;
    double y_zoom[10];       /* y_zoom[0] current zoom factor y_zoom[1..9] history */
    double y_off[10];        /* y_off[0] current Oofset y_off[1..9] history */
    uint64_t  t_window_base[10];
    uint64_t  t_window_width[10];

    int sample_steps;        /* 1,2,3,4,... */
    double min_left[20];
    double max_left[20];
    double min_right[20];
    double max_right[20];

    int color_left[20];
    int color_right[20];
    int LineSize_right[20];
    int LineSize_left[20];

    char txt_align_left[20];
    char txt_align_right[20];

    char presentation_left[20];  // 0 -> Lines, 1 -> Points
    char presentation_right[20];

    int trigger_flag;      /* 0 -> No trigger active,
                              1 -> Trigger continuous
                              2 -> Trigger singel shot  */
    char trigger_flank;     /* '>' -> rising edge
                               '<' -> falling edge */
    int trigger_state;
    int pre_trigger;       /* Number of cycles  */
    long pre_trigger_counter;
    long trigger_vid;      /* Trigger variable */
    char *name_trigger;
    double trigger_value;  /* Trigger threshold */

    int width_all_right_desc;
    int width_all_left_desc;

    int FontAverageCharWidth;

    INCLUDE_EXCLUDE_FILTER *IncExcFilter;

    int xy_view_flag;  // If this is set use XY view otherwise use time view
    char BackgroundImage[260];  // MAX_PATH !
    int BackgroundImageLoaded;
#define NO_BACKGROUND_IMAGE             0
#define SHOULD_LOAD_BACKGROUND_IMAGE    1
#define BACKGROUND_IMAGE_LOADED         2
#define SHOULD_DELETE_BACKGROUND_IMAGE  3
#define BACKGROUND_IMAGE_ERROR         -1

    double NotANumber;
} OSCILLOSCOPE_DATA;
#pragma pack(pop)

inline double FiFoPosRight (OSCILLOSCOPE_DATA *par_Data, int par_Index, uint64_t par_t)
{
    if (par_t <= par_Data->t_current_to_update_end) {
        int BufferPos = (int)(((par_Data->t_current_to_update_end - par_t) / par_Data->t_step));
        int Depth = par_Data->depth_right[par_Index];
        int Wr = par_Data->wr_poi_right[par_Index];
        if (BufferPos < Depth) {
            int MemPos =  Wr - 1 - BufferPos;
            if (MemPos < 0) MemPos = Depth + MemPos;
            if (MemPos >= 0) {
                double Value = par_Data->buffer_right[par_Index][MemPos];
                return Value;
            }
        }
    }
    return par_Data->NotANumber;
}

inline double FiFoPosLeft (OSCILLOSCOPE_DATA *par_Data, int par_Index, uint64_t par_t)
{
    if (par_t <= par_Data->t_current_to_update_end) {
        int BufferPos = (int)(((par_Data->t_current_to_update_end - par_t) / par_Data->t_step));
        int Depth = par_Data->depth_left[par_Index];
        int Wr = par_Data->wr_poi_left[par_Index];
        if (BufferPos < Depth) {
            int MemPos =  Wr - 1 - BufferPos;
            if (MemPos < 0) MemPos = Depth + MemPos;
            if (MemPos >= 0) {
                double Value = par_Data->buffer_left[par_Index][MemPos];
                return Value;
            }
        }
    }
    return par_Data->NotANumber;
}

#ifdef __cplusplus
}
#endif

#endif // OSCILLOSCOPEDATA_H


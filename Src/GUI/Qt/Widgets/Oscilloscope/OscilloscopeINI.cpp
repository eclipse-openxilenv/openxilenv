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
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
extern "C" {
#include "Config.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Blackboard.h"
#include "Scheduler.h"
#include "IniDataBase.h"
#include "MainValues.h"
#include "my_udiv128.h"
#include "OscilloscopeData.h"
}
#include "QtIniFile.h"
#include "OscilloscopeINI.h"

#include <QString>

// Write the configuration to the INI file
void write_osziwin_ini (const char *SectionName, OSCILLOSCOPE_DATA* poszidata, int xpos, int ypos, int xsize, int ysize, int minimized)
{
    char txt[BBVARI_NAME_SIZE+100], name[16];
    int x;
    int Fd = GetMainFileDescriptor();

    IniFileDataBaseWriteString(SectionName, "type", "Oscilloscope", Fd);
    IniFileDataBaseWriteYesNo(SectionName, "xy", poszidata->xy_view_flag, Fd);
    IniFileDataBaseWriteString(SectionName, "xy_backgraound", poszidata->BackgroundImage, Fd);
    IniFileDataBaseWriteInt(SectionName, "left", xpos, Fd);
    IniFileDataBaseWriteInt(SectionName, "top", ypos, Fd);
    IniFileDataBaseWriteInt(SectionName, "width", xsize, Fd);
    IniFileDataBaseWriteInt(SectionName, "height", ysize, Fd);
    IniFileDataBaseWriteInt(SectionName, "icon", minimized, Fd);
    IniFileDataBaseWriteInt(SectionName, "left_axis_on", poszidata->left_axis_on, Fd);
    IniFileDataBaseWriteInt(SectionName, "right_axis_on", poszidata->right_axis_on, Fd);
    IniFileDataBaseWriteInt(SectionName, "sel_left_right", poszidata->sel_left_right, Fd);
    IniFileDataBaseWriteInt(SectionName, "sel_pos_left", poszidata->sel_pos_left, Fd);
    IniFileDataBaseWriteInt(SectionName, "sel_pos_right", poszidata->sel_pos_right, Fd);
    IniFileDataBaseWriteInt(SectionName, "buffer_depth", poszidata->buffer_depth, Fd);
    IniFileDataBaseWriteInt(SectionName, "sample_steps", poszidata->sample_steps, Fd);
    IniFileDataBaseWriteInt(SectionName, "window_step_width", poszidata->window_step_width, Fd);

    IniFileDataBaseWriteFloat(SectionName, "time_window", poszidata->t_window_size_configered, Fd);

    IniFileDataBaseWriteFloat(SectionName, "y_zoom", poszidata->y_zoom[poszidata->zoom_pos], Fd);
    IniFileDataBaseWriteFloat(SectionName, "y_off", poszidata->y_off[poszidata->zoom_pos], Fd);
    if ((poszidata->trigger_flag) && (poszidata->trigger_vid > 0)) {
        PrintFormatToString (txt, sizeof(txt), "%s %c %f, %i, %i", poszidata->name_trigger,
                 poszidata->trigger_flank, poszidata->trigger_value,
                 poszidata->trigger_flag, poszidata->pre_trigger);
    } else {
        STRING_COPY_TO_ARRAY (txt, "off");
    }
    IniFileDataBaseWriteString(SectionName, "trigger", txt, Fd);

    for (x = 0; x < 20; x++) {
        if (poszidata->vids_left[x] > 0) {
            const char *notation;
            if (poszidata->dec_phys_left[x]) {
                notation = "phys";
            } else switch (poszidata->dec_hex_bin_left[x]) {
            case 0:
            default:
                notation = "dec";
                break;
            case 1:
                notation = "hex";
                break;
            case 2:
                notation = "bin";
                break;
            }
            PrintFormatToString (txt, sizeof(txt), "%s, %f, %f, (%i,%i,%i), %s, %i, %s, %s, %s",
                     poszidata->name_left[x],
                     poszidata->min_left[x],
                     poszidata->max_left[x],
                     (int)GetRValue (poszidata->color_left[x]),
                     (int)GetGValue (poszidata->color_left[x]),
                     (int)GetBValue (poszidata->color_left[x]),
                     notation,
                     poszidata->LineSize_left[x],
                     (poszidata->txt_align_left[x]) ? "r" : "l",
                     (poszidata->vars_disable_left[x]) ? "d" : "e",
                     (poszidata->presentation_left[x]) ? "p" : "l");
            PrintFormatToString (name, sizeof(name), "vl%i", x);
            IniFileDataBaseWriteString(SectionName, name, txt, Fd);
        } else {
            PrintFormatToString (name, sizeof(name), "vl%i", x);
            IniFileDataBaseWriteString(SectionName, name, NULL, Fd);
        }
    }
    for (x = 0; x < 20; x++) {
        if (poszidata->vids_right[x] > 0) {
            const char *notation;
            if (poszidata->dec_phys_right[x]) {
                notation = "phys";
            } else switch (poszidata->dec_hex_bin_right[x]) {
            case 0:
            default:
                notation = "dec";
                break;
            case 1:
                notation = "hex";
                break;
            case 2:
                notation = "bin";
                break;
            }
            PrintFormatToString (txt, sizeof(txt), "%s, %f, %f, (%i,%i,%i), %s, %i, %s, %s, %s",
                     poszidata->name_right[x],
                     poszidata->min_right[x],
                     poszidata->max_right[x],
                     (int)GetRValue (poszidata->color_right[x]),
                     (int)GetGValue (poszidata->color_right[x]),
                     (int)GetBValue (poszidata->color_right[x]),
                     notation,
                     poszidata->LineSize_right[x],
                     (poszidata->txt_align_right[x]) ? "r" : "l",
                     (poszidata->vars_disable_right[x]) ? "d" : "e",
                     (poszidata->presentation_right[x]) ? "p" : "l"
                     );
            PrintFormatToString (name, sizeof(name), "vr%i", x);
            IniFileDataBaseWriteString(SectionName, name, txt, Fd);
        } else {
            PrintFormatToString (name, sizeof(name), "vr%i", x);
            IniFileDataBaseWriteString(SectionName, name, NULL, Fd);
        }
    }
    IniFileDataBaseWriteInt(SectionName, "right_desc_width", poszidata->width_all_right_desc, Fd);
    IniFileDataBaseWriteInt(SectionName, "left_desc_width", poszidata->width_all_left_desc, Fd);

    if (poszidata->IncExcFilter != NULL) {
        SaveIncludeExcludeListsToIni (poszidata->IncExcFilter , SectionName, "", Fd);
    }
}

// Read the configuration from the INI file
void read_osziwin_ini (const char *SectionName, OSCILLOSCOPE_DATA* poszidata)
{
    char txt1[BBVARI_NAME_SIZE+100], txt2[32], entry[32], *p;
    int x, red, green, blue, LineSize;
    long vid;
    char vari_name[BBVARI_NAME_SIZE];
    int Fd = GetMainFileDescriptor();

    poszidata->xy_view_flag = IniFileDataBaseReadYesNo(SectionName, "xy", 0, Fd);
    IniFileDataBaseReadString(SectionName, "xy_backgraound", "", poszidata->BackgroundImage, sizeof (poszidata->BackgroundImage), Fd);
    if (strlen (poszidata->BackgroundImage)) {
        poszidata->BackgroundImageLoaded = SHOULD_LOAD_BACKGROUND_IMAGE;
    } else {
        poszidata->BackgroundImageLoaded = NO_BACKGROUND_IMAGE;
    }
    poszidata->left_axis_on = IniFileDataBaseReadInt(SectionName, "left_axis_on", 1, Fd);
    poszidata->right_axis_on = IniFileDataBaseReadInt(SectionName, "right_axis_on", 0, Fd);
    poszidata->sel_left_right = IniFileDataBaseReadInt(SectionName, "sel_left_right", 0, Fd);
    poszidata->sel_pos_left = IniFileDataBaseReadInt(SectionName, "sel_pos_left", 0, Fd);
    poszidata->sel_pos_right = IniFileDataBaseReadInt(SectionName, "sel_pos_right", 0, Fd);
    poszidata->buffer_depth = IniFileDataBaseReadInt(SectionName, "buffer_depth", s_main_ini_val.OscilloscopeDefaultBufferDepth, Fd);
    if (poszidata->buffer_depth < 1024) poszidata->buffer_depth = 1024;
    if (poszidata->buffer_depth > 1000000) poszidata->buffer_depth = 1000000;
    poszidata->sample_steps = IniFileDataBaseReadInt(SectionName, "sample_steps", 1, Fd);
    poszidata->window_step_width = IniFileDataBaseReadInt(SectionName, "window_step_width", 20, Fd);

    poszidata->t_window_size_configered = IniFileDataBaseReadFloat(SectionName, "time_window", 10.0, Fd);
    if (poszidata->t_window_size_configered < 0.001) poszidata->t_window_size_configered = 0.001;
    if (poszidata->t_window_size_configered > 1000.0) poszidata->t_window_size_configered = 1000.0;
    poszidata->t_window_size = (uint64_t)(poszidata->t_window_size_configered * TIMERCLKFRQ);
    poszidata->t_window_start = 0;
    poszidata->t_window_end = poszidata->t_window_size;

    poszidata->t_step_win = my_umuldiv64(poszidata->t_window_end - poszidata->t_window_start, (uint64_t)poszidata->window_step_width, 100);

    if (!IniFileDataBaseReadString(SectionName, "trigger", "", txt1, sizeof (txt1), Fd) ||
        (sscanf (txt1, "%s %c %lf, %i, %i", txt2, &poszidata->trigger_flank,
                 &poszidata->trigger_value, &poszidata->trigger_flag,
                 &poszidata->pre_trigger) != 5) ||
        (poszidata->trigger_flag < 0) || (poszidata->trigger_flag > 2) ||
        ((poszidata->trigger_flank != '>') && (poszidata->trigger_flank != '<'))) {
        poszidata->trigger_flag = 0;
        poszidata->trigger_value = 0.0;
        poszidata->trigger_vid = 0;
        poszidata->pre_trigger = 0;
    } else {
        if ((poszidata->trigger_vid = add_bbvari (txt2, BB_UNKNOWN_WAIT, NULL)) > 0) {
            poszidata->vids_left[20] = poszidata->trigger_vid;
            poszidata->name_trigger = StringRealloc (poszidata->name_trigger, txt2);
            poszidata->name_left[20] = StringRealloc (poszidata->name_left[20], txt2);

            if ((poszidata->buffer_left[20] = (double*)my_calloc ((size_t)poszidata->buffer_depth, sizeof (double))) == NULL) {
                remove_bbvari_unknown_wait (poszidata->vids_left[20]);
                poszidata->vids_left[20] = 0;
                poszidata->trigger_vid = 0;
            }
            poszidata->state = 3;  // Wait for trigger event
            poszidata->trigger_state = 0;
        }
    }

    for (x = 0; x < 20; x++) {
        poszidata->LineSize_right[x] = 1;
        poszidata->LineSize_left[x]  = 1;

        PrintFormatToString (entry, sizeof(entry), "vl%i", x);
        ENTER_GCS ();
        if ((IniFileDataBaseReadString(SectionName, entry, "", txt1, sizeof (txt1), Fd)) &&
            ((p = strtok(txt1, ",")) != NULL) &&
            (p[0] && (sscanf(p, "%s", vari_name) == 1)) &&
            // Register variable
            ((vid = add_bbvari (vari_name, BB_UNKNOWN_WAIT, NULL)) > 0)) {
            poszidata->vids_left[x] = vid;
            poszidata->name_left[x] = StringRealloc (poszidata->name_left[x], vari_name);
            if ((poszidata->buffer_left[x] = (double*)my_calloc ((size_t)poszidata->buffer_depth, sizeof(double))) == NULL) {
                ThrowError (1, "no free memory");
                poszidata->vids_left[x] = 0;
            }
            // min.
            if (((p = strtok(NULL, ",")) == NULL) ||
                (!p[0] || (sscanf(p, "%lf", &poszidata->min_left[x]) != 1))) {
                poszidata->min_left[x] = -1.0;
            }
            // max.
            if (((p = strtok(NULL, ",")) == NULL) ||
                (!p[0] || (sscanf(p, "%lf", &poszidata->max_left[x]) != 1))) {
                poszidata->max_left[x] = 1.0;
            }
            if (poszidata->max_left[x] <= poszidata->min_left[x]) poszidata->max_left[x] = poszidata->min_left[x] + 1.0;
            // Color
            if (((p = strtok(NULL, "(")) == NULL) ||
                ((p = strtok(NULL, ")")) == NULL) ||
                (!p[0] || (sscanf(p, "%d,%d,%d", &red, &green, &blue) != 3)))
                poszidata->color_left[x] = RGB(0,0,0);
            else poszidata->color_left[x] = (int)RGB(red, green, blue);
            // Physical. or decimal
            if ((p = strtok(NULL, ",")) == NULL) {
                poszidata->dec_phys_left[x] = 0;
                poszidata->dec_hex_bin_left[x] = 0;
            } else {
                while (isspace (*p)) p++;
                if (!strcmp (p, "phys")) {
                    poszidata->dec_phys_left[x] = 1;
                    poszidata->dec_hex_bin_left[x] = 0;
                } else if (!strcmp (p, "hex")) {
                    poszidata->dec_phys_left[x] = 0;
                    poszidata->dec_hex_bin_left[x] = 1;
                } else if (!strcmp (p, "bin")) {
                    poszidata->dec_phys_left[x] = 0;
                    poszidata->dec_hex_bin_left[x] = 2;
                } else {
                    poszidata->dec_phys_left[x] = 0;
                    poszidata->dec_hex_bin_left[x] = 0;
                }
            }
            // Line width
            if (((p = strtok(NULL, ",")) == NULL) ||
                (!p[0] || (sscanf(p,"%d", &LineSize) !=1))) {
                LineSize = 1;
            }
            if ( (LineSize<1) || (LineSize>10) ) LineSize = 1;
            poszidata->LineSize_left[x] = LineSize;
            // right/left alignmet of the variable name
            poszidata->txt_align_left[x] = 0;
            if ((p = strtok(NULL, ",")) != NULL) {
                while (isspace (*p)) p++;
                if ((*p == 'r') || (*p == 'R')) {
                    poszidata->txt_align_left[x] = 1;
                }
            }
            // disable (crossed out)
            poszidata->vars_disable_left[x] = 0;
            if ((p = strtok(NULL, ",")) != NULL) {
                while (isspace (*p)) p++;
                if ((*p == 'd') || (*p == 'D')) {
                    poszidata->vars_disable_left[x] = 1;
                }
            }
            // Lines or points
            poszidata->presentation_left[x] = 0;
            if ((p = strtok(NULL, ",")) != NULL) {
                while (isspace (*p)) p++;
                if ((*p == 'p') || (*p == 'P')) {
                    poszidata->presentation_left[x] = 1;
                }
            }
        }
        PrintFormatToString (entry, sizeof(entry), "vr%i", x);
        if ((IniFileDataBaseReadString(SectionName, entry, "", txt1, sizeof (txt1), Fd)) &&
            ((p = strtok(txt1, ",")) != NULL) &&
            (p[0] && (sscanf(p, "%s", vari_name) == 1)) &&
            // Register variable
            ((vid = add_bbvari (vari_name, BB_UNKNOWN_WAIT, NULL)) > 0)) {
            poszidata->vids_right[x] = vid;
            poszidata->name_right[x] = StringRealloc (poszidata->name_right[x], vari_name);
            if ((poszidata->buffer_right[x] = (double*)my_calloc ((size_t)poszidata->buffer_depth, sizeof(double))) == NULL) {
                ThrowError (1, "no free memory");
                poszidata->vids_right[x] = 0;
            }
            // min.
            if (((p = strtok(NULL, ",")) == NULL) ||
                (!p[0] || (sscanf(p, "%lf", &poszidata->min_right[x]) != 1))) {
                poszidata->min_right[x] = -1.0;
            }
            // max.
            if (((p = strtok(NULL, ",")) == NULL) ||
                (!p[0] || (sscanf(p, "%lf", &poszidata->max_right[x]) != 1))) {
                poszidata->max_right[x] = -1.0;
            }
            if (poszidata->max_right[x] <= poszidata->min_right[x]) poszidata->max_right[x] = poszidata->min_right[x] + 1.0;
            // Color
            if (((p = strtok(NULL, "(")) == NULL) ||
                ((p = strtok(NULL, ")")) == NULL) ||
                (!p[0] || (sscanf(p, "%d,%d,%d", &red, &green, &blue) != 3)))
                poszidata->color_right[x] = RGB(0,0,0);
            else poszidata->color_right[x] = (int)RGB(red, green, blue);
            // Physical. or decimal
            if ((p = strtok(NULL, ",")) == NULL) {
                poszidata->dec_phys_right[x] = 0;
                poszidata->dec_hex_bin_right[x] = 0;
            } else {
                while (isspace (*p)) p++;
                if (!strcmp (p, "phys")) {
                    poszidata->dec_phys_right[x] = 1;
                    poszidata->dec_hex_bin_right[x] = 0;
                } else if (!strcmp (p, "hex")) {
                    poszidata->dec_phys_right[x] = 0;
                    poszidata->dec_hex_bin_right[x] = 1;
                } else if (!strcmp (p, "bin")) {
                    poszidata->dec_phys_right[x] = 0;
                    poszidata->dec_hex_bin_right[x] = 2;
                } else {
                    poszidata->dec_phys_right[x] = 0;
                    poszidata->dec_hex_bin_right[x] = 0;
                }
            }
            // Line width
            if (((p = strtok(NULL, ",")) == NULL) ||
                (!p[0] || (sscanf(p,"%d", &LineSize) !=1))) {
                LineSize = 1;
            }
            if ( (LineSize<1) || (LineSize>10) ) LineSize = 1;
            poszidata->LineSize_right[x] = LineSize;
            // right/left alignmet of thev ariable name
            poszidata->txt_align_right[x] = 0;
            if ((p = strtok(NULL, ",")) != NULL) {
                while (isspace (*p)) p++;
                if ((*p == 'r') || (*p == 'R')) {
                    poszidata->txt_align_right[x] = 1;
                }
            }
            // disable (crossed out)
            poszidata->vars_disable_right[x] = 0;
            if ((p = strtok(NULL, ",")) != NULL) {
                while (isspace (*p)) p++;
                if ((*p == 'd') || (*p == 'D')) {
                    poszidata->vars_disable_right[x] = 1;
                }
            }
            // Lines or points
            poszidata->presentation_right[x] = 0;
            if ((p = strtok(NULL, ",")) != NULL) {
                while (isspace (*p)) p++;
                if ((*p == 'p') || (*p == 'P')) {
                    poszidata->presentation_right[x] = 1;
                }
            }
        }
        LEAVE_GCS ();
    }

    poszidata->width_all_right_desc = IniFileDataBaseReadInt(SectionName, "right_desc_width", 8, Fd);
    poszidata->width_all_left_desc = IniFileDataBaseReadInt(SectionName, "left_desc_width", 8, Fd);

    poszidata->IncExcFilter = BuildIncludeExcludeFilterFromIni (SectionName, "", Fd);
}

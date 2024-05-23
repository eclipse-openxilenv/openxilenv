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

#include "Config.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Blackboard.h"
#include "Scheduler.h"
#include "Files.h"

#include "FileExtensions.h"


#include "OscilloscopeData.h"
#include "OscilloscopeCyclic.h"
#include "OscilloscopeFile.h"

#define UNUSED(x) (void)(x)

static int WriteStimuliFileHeadDatFormat (FILE *fh, const char *Filename, OSCILLOSCOPE_DATA* poszidata, const char *par_Title)
{
    int x;

    if (fprintf (fh, "Oscilloscope Window %s  (%s)\n", par_Title, get_date_time_string ()) < 0) return -1;
    if (fprintf (fh, "FORMAT\n") < 0) return -1;
    if (fprintf (fh, "%s\n", Filename) < 0) return -1;
    if (fprintf (fh, "%f\n", (double)poszidata->t_step / TIMERCLKFRQ) < 0) return -1;

    /* Write Variable namen */
    if (fprintf (fh, "sample\ttime\t") < 0) return -1;
    // First left side
    for (x = 0; x < 20; x++) {
        if ((poszidata->vids_left[x] > 0) && (!poszidata->vars_disable_left[x])) {
            if (fprintf (fh, "%s\t", poszidata->name_left[x]) < 0) return -1;
        }
    }
    // Than the right side
    for (x = 0; x < 20; x++) {
        if ((poszidata->vids_right[x] > 0) && (!poszidata->vars_disable_right[x])){
            if (fprintf (fh, "%s\t", poszidata->name_right[x]) < 0) return -1;
        }
    }
    if (fprintf (fh, "\n") < 0) return -1;

    /* Write the units */
    if (fprintf (fh, "[count]\t[s]\t") < 0) return -1;
    for (x = 0; x < 20; x++) {
        if ((poszidata->vids_left[x] > 0) && (!poszidata->vars_disable_left[x])) {
            char unit[64];
            int pos = 0;
            get_bbvari_unit (poszidata->vids_left[x], unit, sizeof (unit));
            // Replace whitespaces with '*'
            while (unit[pos] != '\0') {
                if (isspace(unit[pos])) unit[pos] = '*';
                pos++;
            }
            if (fprintf (fh, "[%s]\t", unit) < 0) return -1;
        }
    }
    for (x = 0; x < 20; x++) {
        if ((poszidata->vids_right[x] > 0) && (!poszidata->vars_disable_right[x])) {
            char unit[64];
            int pos = 0;
            get_bbvari_unit (poszidata->vids_right[x], unit, sizeof (unit));
            // Replace whitespaces with '*'
            while (unit[pos] != '\0') {
                if (isspace(unit[pos])) unit[pos] = '*';
                pos++;
            }
            if (fprintf (fh, "[%s]\t", unit) < 0) return -1;
        }
    }
    if (fprintf (fh, "\n") < 0) return -1;
    if (fprintf (fh, "DATA\n") < 0) return -1;

    return 0;
}

// Write oscilloscope buffer content as stimulation file
int WriteOsziBuffer2StimuliFile (OSCILLOSCOPE_DATA* poszidata, const char *par_FileName, int par_Format, const char *par_Title)
{
    UNUSED(par_Format);
    FILE *fh;
    int x, s;
    int MaxSamplesLeft[20];
    int MaxSamplesRight[20];
    int MaxMaxSamples;
    uint64_t SampleTime;
    uint64_t StartTime;
    uint64_t EndTime;

    if ((fh = open_file (par_FileName, "wt")) == NULL) {
        ThrowError (1, "cannot open file %s", par_FileName);
        return -1;
    }

    EnterOsciWidgetCriticalSection (poszidata->CriticalSectionNumber);

    /* First calculate the number of samples for each channel (this can be different) */
    MaxMaxSamples = 0;
    for (x = 0; x < 20; x++) {
        if ((poszidata->vids_left[x] > 0) && (!poszidata->vars_disable_left[x])) {
            MaxSamplesLeft[x] = poszidata->depth_left[x] - 1;
        } else MaxSamplesLeft[x] = 0;
        if (MaxSamplesLeft[x] > MaxMaxSamples) MaxMaxSamples = MaxSamplesLeft[x];
    }
    for (x = 0; x < 20; x++) {
        if ((poszidata->vids_right[x] > 0) && (!poszidata->vars_disable_right[x])) {
            MaxSamplesRight[x] = poszidata->depth_right[x] - 1;
        } else MaxSamplesRight[x] = 0;
        if (MaxSamplesRight[x] > MaxMaxSamples) MaxMaxSamples = MaxSamplesRight[x];
    }

    EndTime = poszidata->t_current_to_update_end;
    if (((uint64_t)MaxMaxSamples * poszidata->t_step) < EndTime) {
        StartTime = EndTime - (uint64_t)MaxMaxSamples * poszidata->t_step;
    } else {
        StartTime = 0;
    }
    if (WriteStimuliFileHeadDatFormat (fh, par_FileName, poszidata, par_Title)) goto  __ERROR;

    SampleTime = StartTime;
    for (SampleTime = StartTime, s = 0; SampleTime < EndTime; SampleTime += poszidata->t_step, s++) {
        if (fprintf (fh, "%i\t%g\t", s, (double)(SampleTime - StartTime) / TIMERCLKFRQ) < 0)  goto __ERROR;
        for (x = 0; x < 20; x++) {
            if ((poszidata->vids_left[x] > 0) && (!poszidata->vars_disable_left[x])) {
                double Value = FiFoPosLeft (poszidata, x, SampleTime);
                if (fprintf (fh, "%g\t", Value) < 0) goto __ERROR;
            }             }
        for (x = 0; x < 20; x++) {
            if ((poszidata->vids_right[x] > 0) && (!poszidata->vars_disable_right[x])) {
                double Value = FiFoPosRight (poszidata, x, SampleTime);
                if (fprintf (fh, "%g\t", Value) < 0) goto __ERROR;
            }
        }
        if (fprintf (fh, "\n") < 0) goto __ERROR;
    }
    LeaveOsciWidgetCriticalSection (poszidata->CriticalSectionNumber);
    close_file (fh);
    return 0;
__ERROR:
    LeaveOsciWidgetCriticalSection (poszidata->CriticalSectionNumber);
    ThrowError (1, "cannot write to file %s (no space left on device)", par_FileName);
    close_file (fh);
    return -1;
}

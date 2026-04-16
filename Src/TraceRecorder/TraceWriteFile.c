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


#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Config.h"
#include "StringMaxChar.h"
#include "TimeStamp.h"
#include "Files.h"
#include "ReadFromBlackboardPipe.h"
#include "TraceRecorder.h"
#include "StimulusPlayer.h"
#include "Blackboard.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "ThrowError.h"
#include "EnvironmentVariables.h"
#include "TraceWriteMdfFile.h"
#include "TraceWriteFile.h"


#define UNUSED(x) (void)(x)

#ifndef MAXINT
#define MAXINT 0x7FFFFFFF
#endif


int conv_rec_time_steps;    /*  Recording step size */

int TailStimuliFile (FILE *fh)
{
    close_file (fh);
    return 0;
}

int open_write_stimuli_head (START_MESSAGE_DATA hdrec_data,
                             int32_t *vids, FILE **pfile)
{
    int32_t *vids_point;
    char unit[64];

    if ((*pfile = OpenFile4WriteWithPrefix (hdrec_data.Filename, "wt")) == NULL) {
        ThrowError (1, "kann Datei %s nicht oeffnen", hdrec_data.Filename);
        return CANNOT_OPEN_RECFILE;
    }
    /* schreibe Kopf des Stimuli-Files */
    fprintf (*pfile, "%s  (%s)\n", hdrec_data.Comment, get_date_time_string ());
    fprintf (*pfile, "FORMAT\n");
    fprintf (*pfile, "%s\n", hdrec_data.RecordingName);
    fprintf (*pfile, "%f\n", hdrec_data.SamplePeriod);

    conv_rec_time_steps = hdrec_data.RecTimeSteps;

    /* schreibe Variablenamen */
    fprintf (*pfile, "sample\ttime\t");
    for (vids_point = vids; *vids_point > 0; vids_point++) {
        char variname[BBVARI_NAME_SIZE];
        GetBlackboardVariableName (*vids_point, variname, sizeof(variname));
        fprintf (*pfile, "%s\t", variname);
    }
    fprintf (*pfile, "\n");

    /* schreibe Einheiten */
    fprintf (*pfile, "[count]\t[s]\t");
    for (vids_point = vids; *vids_point > 0; vids_point++) {
        get_bbvari_unit (*vids_point, unit, sizeof (unit));
        { /* KIE: BUG: 01-10-2001 Leerzeichen durch . ersetzen */
           unsigned int pos=0;
           while(unit[pos] != '\0')
           {
              if(isspace(unit[pos])) { unit[pos] = '*'; }
              pos++;
           }
        }
        fprintf (*pfile, "[%s]\t", unit);
    }
    fprintf (*pfile, "\n");
    fprintf (*pfile, "DATA\n");

    return 0;
}


int write_ringbuff_stimuli (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count)
{
    int i;
    int status = -1;

    for (i = 0; i < rpvari_count; i++) {
        /* und schreibe Stempel in Stimuli-File */
        switch (stamp->EntryList[i].type) {
        case BB_BYTE:
            status = fprintf (file, "%i\t", (int)stamp->EntryList[i].value.b);
            break;
        case BB_UBYTE:
            status = fprintf (file, "%u\t", (unsigned int)stamp->EntryList[i].value.ub);
            break;
        case BB_WORD:
            status = fprintf (file, "%i\t", stamp->EntryList[i].value.w);
            break;
        case BB_UWORD:
            status = fprintf (file, "%u\t", stamp->EntryList[i].value.uw);
            break;
        case BB_DWORD:
            status = fprintf (file, "%i\t", stamp->EntryList[i].value.dw);
            break;
        case BB_UDWORD:
            status = fprintf (file, "%u\t", stamp->EntryList[i].value.udw);
            break;
        case BB_QWORD:
            status = fprintf (file, "%" PRIi64 "\t", stamp->EntryList[i].value.qw);
            break;
        case BB_UQWORD:
            status = fprintf (file, "%" PRIu64 "\t", stamp->EntryList[i].value.uqw);
            break;
        case BB_FLOAT:
            status = fprintf (file, "%01.10g\t", (double)stamp->EntryList[i].value.f);
            break;
        case BB_DOUBLE:
            if (i == 1) {
                status = fprintf (file, "%.4f\t", stamp->EntryList[i].value.d);
            } else {
                status = fprintf (file, "%01.20g\t", stamp->EntryList[i].value.d);
            }
            break;
        }
    }
    fprintf (file, "\n");   /* neue Zeile */
    return status;
}


int WriteCommentToStimuli (FILE *File, uint64_t Tmestamp, const char *Comment)
{
    // do nothing
    return 0;
}

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

static void write_pascal_string (void *dest, char *src, int maxlen)
{
    int count = 0;
    unsigned char *hp;

    hp = (unsigned char*)dest;
    hp++;

    while (*src != '\0') {
       if (++count > maxlen) break;
       *hp++ = (unsigned char)*src++;
    }
    *(unsigned char*)dest = (unsigned char)count;

    while (count++ < maxlen) *hp++ = '\0';
}

int open_write_mcsrec_head (START_MESSAGE_DATA hdrec_data,
                            int32_t *vids, FILE **pfile)
{
    int *vids_point;
    recoFileHdr hdr;
    recoSrcRef srcref;
    /*recoElemRef elemref;*/
    char help_string1[80];
    char help_string2[80];
    short x;
    char sref = 1;
    long fpos;
    int var_count;


    if ((*pfile = OpenFile4WriteWithPrefix (hdrec_data.Filename, "w+b")) == NULL) {
        ThrowError (1, "cannot open file %s", hdrec_data.Filename);
        return CANNOT_OPEN_RECFILE;
    }
    hdr.fileEnd = 0;
    //hdr.filePos = 0x322;  //0x0312;   /* ??? macht gredi halt auch so! */
    hdr.vers = 3;
    //hdr.dataBeg = 0x322;  //0x0312;   /* ??? macht gredi halt auch so! */
    write_pascal_string (hdr.usrCmnt, hdrec_data.Comment, 79);
    hdr.sampleRate = (float)(hdrec_data.SamplePeriod * 1000.0);
    hdr.eventTick = 0.0;
    hdr.nSrcRefs = 4;
    hdr.nElemRefs = (unsigned short)hdrec_data.VariableCount;

    var_count = x = 0;
    for (vids_point = vids; *vids_point > 0; vids_point++) {
        var_count++;
        if (var_count > 127) {
            ThrowError (1, "cannot record MCS-REC files, not more than 127 variables allowed");
            close_file (*pfile);
            return CANNOT_OPEN_RECFILE;
        }
        switch (get_bbvaritype(*vids_point)) {
        case BB_BYTE:
        case BB_UBYTE:
            x++;
            break;
        case BB_WORD:
        case BB_UWORD:
            x+=2;
            break;
        case BB_DWORD:
        case BB_UDWORD:
            x+=4;
            break;
        case BB_QWORD:    // Gredi kann keine 64Bits?!
        case BB_UQWORD:
            x+=4;
            break;
        case BB_FLOAT:
        case BB_DOUBLE:
            x+=4;
            break;
        default:
            ThrowError (1, "cannot record variables with unknown datatype"); //write double or float variables to MCS record file");
            close_file (*pfile);
            return CANNOT_OPEN_RECFILE;
        }
    }
    hdr.recSize = (unsigned short)(x + 4);

    fwrite (&hdr, sizeof (recoFileHdr), 1, *pfile);

    if (strlen (hdrec_data.DescriprionName)) {
        char temp[1024];
        SearchAndReplaceEnvironmentStrings (hdrec_data.DescriprionName, temp, sizeof (temp));
        write_pascal_string (srcref.robName, temp, 79);
    } else {
        char Name[BBVARI_NAME_SIZE];
        write_pascal_string (srcref.robName, ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME, ".ROB", Name, sizeof(Name)), 79);
    }
    write_pascal_string (srcref.binName, "", 79);

    fwrite (&srcref, sizeof (recoSrcRef), 1, *pfile);

    /* don't know why */
    write_pascal_string (srcref.robName, "", 79);
    fwrite (&srcref, sizeof (recoSrcRef), 1, *pfile);
    fwrite (&srcref, sizeof (recoSrcRef), 1, *pfile);
    fwrite (&srcref, sizeof (recoSrcRef), 1, *pfile);
    /* ???? */

    conv_rec_time_steps = hdrec_data.RecTimeSteps;


    x = 4;
    for (vids_point = vids; *vids_point > 0; vids_point++) {
        char variname[BBVARI_NAME_SIZE];
        GetBlackboardVariableName (*vids_point, variname, sizeof(variname));
        STRING_COPY_TO_ARRAY (help_string1, variname);
        // strupr (help_string1);
        write_pascal_string (help_string2, help_string1, 64);

        /*write_pascal_string (elemref.kname, help_string, 16);
        elemref.recoOfs = x;
        elemref.srcFile = 1;
        fwrite (&elemref, sizeof (recoElemRef), 1, *pfile);*/
        fwrite (&x, sizeof (short), 1, *pfile);
        fwrite (&sref, sizeof (char), 1, *pfile);
        fwrite (help_string2, strlen (help_string2), 1, *pfile);

        switch (get_bbvaritype(*vids_point)) {
        case BB_BYTE:
        case BB_UBYTE:
            x++;
            break;
        case BB_WORD:
        case BB_UWORD:
            x+=2;
            break;
        case BB_DWORD:
        case BB_UDWORD:
            x+=4;
            break;
        case BB_QWORD:    // Gredi kann keine 64Bits?!
        case BB_UQWORD:
            x+=4;
            break;
        case BB_FLOAT:
        case BB_DOUBLE:
            x+=4;
            break;
        }
    }

    fpos = ftell (*pfile);
    if (!fseek (*pfile, 0L, SEEK_SET)) {
        fread (&hdr, sizeof (recoFileHdr), 1, *pfile);
        hdr.filePos = fpos;
        hdr.dataBeg = (unsigned short)fpos;
        fseek (*pfile, 0L, SEEK_SET);
        fwrite (&hdr, sizeof (recoFileHdr), 1, *pfile);
        fseek (*pfile, fpos, SEEK_SET);
    }

    return 0;
}


int TailMcsRecFile (FILE *fh, unsigned int Samples)
{
    UNUSED(Samples);
    recoFileHdr hdr;
    long pos;
    pos = ftell (fh);
    if (!fseek (fh, 0L, SEEK_SET)) {
        fread (&hdr, sizeof (recoFileHdr), 1, fh);
        hdr.fileEnd = pos;
        fseek (fh, 0L, SEEK_SET);
        fwrite (&hdr, sizeof (recoFileHdr), 1, fh);
    }
    close_file (fh);
    return 0;
}

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

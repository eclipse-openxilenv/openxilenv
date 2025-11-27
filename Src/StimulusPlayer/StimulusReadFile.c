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


#define FILERDOP_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Platform.h"
#include "Files.h"
#include "ReadFromBlackboardPipe.h"
#include "TraceRecorder.h"
#include "StimulusPlayer.h"
#include "Blackboard.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "StimulusReadDatFile.h"
#include "StimulusReadMdfFile.h"
#include "StimulusReadMdf4File.h"
#include "StimulusReadFile.h"


/* This will read all variable names from a stimulus files  (with ; separation) */
char *ReadStimulHeaderVariabeles (const char *par_Filename)
{
    if (IsMdf4Format(par_Filename, NULL)) {
        return Mdf4ReadStimulHeaderVariabeles (par_Filename);
    } else if (IsMdfFormat(par_Filename, NULL)) {
        return MdfReadStimulHeaderVariabeles (par_Filename);
    } else {
        return DatReadStimulHeaderVariabeles (par_Filename);
    }
}

/* Returns a pointer to the  Stimulus file */
/* or NULL on error */
STIMULI_FILE* OpenAndReadStimuliHeader (const char *par_Filename, const char *vari_list)
{
    if (IsMdf4Format(par_Filename, NULL)) {
        return Mdf4OpenAndReadStimuliHeader (par_Filename, vari_list);
    } else if (IsMdfFormat(par_Filename, NULL)) {
        return MdfOpenAndReadStimuliHeader (par_Filename, vari_list);
    } else {
        return DatOpenAndReadStimuliHeader (par_Filename, vari_list);
    }
}

int GetNumberOfStimuliVariables (STIMULI_FILE*par_File)
{
    if (par_File->FileType == MDF4_FILE) {
        return Mdf4GetNumberOfStimuliVariables (par_File);
    } else if (par_File->FileType == MDF_FILE) {
        return MdfGetNumberOfStimuliVariables (par_File);
    } else {
        return DatGetNumberOfStimuliVariables (par_File);
    }
}

int *GetStimuliVariableIds (STIMULI_FILE*par_File)
{
    if (par_File->FileType == MDF4_FILE) {
        return Mdf4GetStimuliVariableIds (par_File);
    } else if (par_File->FileType == MDF_FILE) {
        return MdfGetStimuliVariableIds (par_File);
    } else {
        return DatGetStimuliVariableIds (par_File);
    }
}

/* This will read one line from the simulus file and build a message */
int ReadOneStimuliTimeSlot (STIMULI_FILE* par_File, VARI_IN_PIPE *pipevari_list, uint64_t *ret_t)
{
    if (par_File->FileType == MDF4_FILE) {
        return Mdf4ReadOneStimuliTimeSlot (par_File, pipevari_list, ret_t);
    } else if (par_File->FileType == MDF_FILE) {
        return MdfReadOneStimuliTimeSlot (par_File, pipevari_list, ret_t);
    } else {
        return DatReadOneStimuliTimeSlot (par_File, pipevari_list, ret_t);
    }
}

void CloseStimuliFile(STIMULI_FILE *par_File)
{
    if (par_File->FileType == MDF4_FILE) {
        Mdf4CloseStimuliFile (par_File);
    } else if (par_File->FileType == MDF_FILE) {
        MdfCloseStimuliFile (par_File);
    } else {
        DatCloseStimuliFile (par_File);
    }
}

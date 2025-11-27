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


#ifndef STIMULUSREADFILE_H
#define STIMULUSREADFILE_H

#include "TraceRecorder.h"
#include "Blackboard.h"

#define STIMULI_END_OF_FILE (-1)

typedef struct{
    enum {NO_FILE, DAT_FILE, MDF_FILE, MDF4_FILE} FileType;
    void *File;
} STIMULI_FILE;

char *ReadStimulHeaderVariabeles (const char *par_Filename);

STIMULI_FILE* OpenAndReadStimuliHeader (const char *par_Filename, const char *vari_list);

int GetNumberOfStimuliVariables (STIMULI_FILE*par_File);

int *GetStimuliVariableIds (STIMULI_FILE*par_File);

int ReadOneStimuliTimeSlot (STIMULI_FILE* par_File, VARI_IN_PIPE *pipevari_list, uint64_t *ret_t);

void CloseStimuliFile(STIMULI_FILE* par_File);

#endif

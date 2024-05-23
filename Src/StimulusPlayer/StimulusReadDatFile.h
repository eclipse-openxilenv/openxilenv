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


#ifndef STIMULUSREADDATFILE_H
#define STIMULUSREADDATFILE_H

#include <stdio.h>
#include "StimulusReadFile.h"

typedef struct {
    FILE *fh;
    int *columns;
    int *vids;
    int *dtypes;
    int size_of_vids;
    int column_count;
    int variable_count;
    double sample_rate;
    uint64_t sample_rate_uint64;
    uint64_t current_sample_time;
    int line_counter;
} DAT_STIMULI_FILE;

char* DatReadStimulHeaderVariabeles  (const char *par_Filename);

STIMULI_FILE* DatOpenAndReadStimuliHeader (const char *par_Filename, const char *vari_list);

int DatGetNumberOfStimuliVariables (STIMULI_FILE*par_File);

int *DatGetStimuliVariableIds (STIMULI_FILE*par_File);

int DatReadOneStimuliTimeSlot (STIMULI_FILE* par_File, VARI_IN_PIPE *pipevari_list, uint64_t *ret_t);

void DatCloseStimuliFile(STIMULI_FILE* par_File);

#endif // STIMULUSREADDATFILE_H

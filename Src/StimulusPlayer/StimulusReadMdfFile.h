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


#ifndef STIMULUSREADMDFFILE_H
#define STIMULUSREADMDFFILE_H

#include "Platform.h"
#include "TraceRecorder.h"
#include "Blackboard.h"
#include "StimulusReadFile.h"

typedef  struct {
    int Vid;
    int DataType;
    int BbDataType;
    int NumberOfBits;
    int StartBitOffset;
} MDF_RECORD_ENTY;

typedef struct {
    MDF_RECORD_ENTY *Entrys;
    int NumberOfEntrys;
    int Id;
    int SizeOfOneRecord;
    uint64_t CurrentTime;
    uint32_t CurrentFilePos;
    uint32_t TimeVariableNumberOfBits;
    uint32_t TimeVariableStartBitOffset;
    uint32_t TimeVariableDataType;
    uint32_t NumberOfRecords;
    uint32_t CurrentRecord;
    double TimeVariableFactor;
    double TimeVariableOffset;
    int TimeAreInNanoSec;
} MDF_RECORD_CACHE;

typedef struct {
    unsigned char *BlockData;
    MDF_RECORD_CACHE *Records;
    int NumberOfChannelGroups;
    uint32_t OffsetInsideFile;
    uint32_t EndOfInsideFile;
    uint32_t SizeOfBlock;
    uint32_t StartCacheFilePos;
    uint32_t EndCacheFilePos;
    uint32_t CurrentFilePos;
    uint32_t LargesRecord;
    int NumberOfRecordIDs;
} MDF_DATA_BLOCK_CACHE;

typedef struct {
    MDF_DATA_BLOCK_CACHE *DataBlocks;
    int NumberOfDataBlocks;
} MDF_CACHE;

typedef struct {
    MY_FILE_HANDLE fh;
    uint32_t SizeOfFile;
    int *vids;
    int *dtypes;
    int size_of_vids;
    int column_count;
    int variable_count;
    uint64_t StartTimeStamp;  // will not used
    uint64_t FirstTimeStamp;
    MDF_CACHE Cache;
} MDF_STIMULI_FILE;


int IsMdfFormat(const char *par_Filename, int *ret_Version);

char *MdfReadStimulHeaderVariabeles (const char *par_Filename);

STIMULI_FILE *MdfOpenAndReadStimuliHeader (const char *par_Filename, const char *par_Variables);

int MdfGetNumberOfStimuliVariables (STIMULI_FILE*par_File);

int *MdfGetStimuliVariableIds(STIMULI_FILE *par_File);

int MdfReadOneStimuliTimeSlot (STIMULI_FILE* par_File, VARI_IN_PIPE *pipevari_list, uint64_t *ret_t);

void MdfCloseStimuliFile(STIMULI_FILE* par_File);

#endif

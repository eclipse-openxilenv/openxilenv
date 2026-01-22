/*
 * Copyright 2024 ZF Friedrichshafen AG
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

#ifndef STIMULUSREADMDF4FILE_H
#define STIMULUSREADMDF4FILE_H

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
} MDF4_RECORD_ENTRY;

typedef struct {
    MDF4_RECORD_ENTRY *Entrys;
    int NumberOfEntrys;
    uint64_t Id;
    int SizeOfOneRecord;
    uint64_t CurrentTime;
    uint64_t CurrentFilePos;
    uint32_t TimeVariableNumberOfBits;
    uint32_t TimeVariableStartBitOffset;
    uint32_t TimeVariableDataType;
    uint32_t NumberOfRecords;
    uint32_t CurrentRecord;
    double TimeVariableFactor;
    double TimeVariableOffset;
    int TimeAreInNanoSec;
    char *Name;
} MDF4_RECORD_CACHE;

typedef struct {
    uint64_t OffsetInsideFile;
    uint64_t SizeOfBlock;
    uint64_t SizeOfDeflatedBlock;  // if 0 no copression
    uint32_t InflateType;
    uint32_t Parameter;
} MDF4_DATA_BLOCK_CACHE;


typedef struct {
    unsigned char *BlockData;
    unsigned char *BlockDeflatedData;
    unsigned char *BlockDeflatedAndTransposedData;
    MDF4_RECORD_CACHE *Records;
    int NumberOfChannelGroups;
    int NoOfAllSignals;

    MDF4_DATA_BLOCK_CACHE *Blocks;
    int NoOfBlocks;
    int CurrentBlock;

    uint64_t SizeOfBlock;
    uint64_t LargestBlockSize;
    uint64_t LargestDeflatedBlockSize;
    uint64_t StartCacheFilePos;
    uint64_t EndCacheFilePos;
    uint64_t CurrentFilePos;
    uint32_t LargesRecord;
    int HaveTransposedData;

    int SizeOfRecordID;
} MDF4_DATA_GROUP_CACHE;

typedef struct {
    MDF4_DATA_GROUP_CACHE *DataGroups;
    int NumberOfDataGroups;
} MDF4_CACHE;

typedef struct {
    MY_FILE_HANDLE fh;
    uint64_t SizeOfFile;
    int *vids;
    int *dtypes;
    int size_of_vids;
    int column_count;
    int variable_count;
    uint64_t StartTimeStamp;
    uint64_t FirstTimeStamp;
    MDF4_CACHE Cache;
} MDF4_STIMULI_FILE;

int IsMdf4Format(const char *par_Filename, int *ret_Version);

char *Mdf4ReadStimulHeaderVariabeles (const char *par_Filename);

STIMULI_FILE *Mdf4OpenAndReadStimuliHeader (const char *par_Filename, const char *par_Variables);

int Mdf4GetNumberOfStimuliVariables (STIMULI_FILE*par_File);

int *Mdf4GetStimuliVariableIds(STIMULI_FILE *par_File);

int Mdf4ReadOneStimuliTimeSlot (STIMULI_FILE* par_File, VARI_IN_PIPE *pipevari_list, uint64_t *ret_t);

void Mdf4CloseStimuliFile(STIMULI_FILE* par_File);

#endif

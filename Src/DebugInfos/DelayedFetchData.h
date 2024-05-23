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


#pragma once

#include <stdint.h>

typedef int (__cdecl* FetchDataCallbackFunction)(int par_OptParam, int par_DataGroupNo, int par_Success);

typedef struct DATA_BLOCK_S {
    uint64_t StartAddress;
    uint32_t BlockSize;
    uint32_t MaxBlockSize;
    void *Data;
    struct DATA_BLOCK_S *Next;
} DATA_BLOCK;

typedef struct DATA_BLOCK_GROUP_S {
    int Used;
    int BlockNo;
    FetchDataCallbackFunction CallbackFunction;
    int OptParam;
    int Dir;
//    uint32_t BlockCount;
    DATA_BLOCK *StartList;
    DATA_BLOCK *Current;
    struct DATA_BLOCK_GROUP_S *Next;
} DATA_BLOCK_GROUP;

enum FetchDataChannelType {GET_FROM_EXTERM_PROCESS_TYPE, GET_FROM_XCP_CONNECTION_TYPE, GET_FROM_CCP_CONNECTION_TYPE, GET_FROM_S19FILE_TYPE };

void InitFetchData(void);
void TerminateFetchData(void);

int NewFetchDataChannel(enum FetchDataChannelType par_FetchDataChannelType, int par_Property, uint64_t par_Timeout);

int CloseFetchDataChannel (int par_FetchDataChannelNo);

int GetDataBlockGroup(int par_Dir);
DATA_BLOCK *GetDataBlockGroupListStartPointer(int par_DataGroupNo);
void FreeDataBlockGroup(int par_DataGroupNo);
int IsDataBlockGroupEmpty(int par_DataGroupNo);


int GetDataFromDataBlockGroup(int par_DataGroupNo, uint64_t par_StartAddress, uint32_t par_BlockSize, char *ret_Data);
int AddToFetchDataBlockToDataBlockGroup(int par_DataGroupNo, uint64_t par_StartAddress, uint32_t par_BlockSize, void *par_Data);

// This should be called by the scheduler
void CheckFetchDataAfterProcess(int par_Pid);

int InitFetchingData(int par_FetchDataChannelNo, int par_DataGroupNo, FetchDataCallbackFunction par_CallbackFunction, int par_OptParam);

void AbortFetchData(int par_FetchDataChannelNo, int par_DataGroupNo);

// This should be called by the XCP ctrl process each cycle
int FetchDataXcpCyclic(int par_ConnectionNo, uint64_t *ret_TargetSrcAddress, void **ret_DstAddress);

int FetchDataXcpCyclicDone(int par_ConnectionNo, int par_Success);

// only for debuging
void PrintBlockGroupToFile(FILE *par_File, int par_DataGroupNo);

// Each window should add a fetch channel
//   int fc = NewFetchDataChannel(GET_FROM_EXTERM_PROCESS_TYPE, pid, 1000*1000*1000llu);

// To get a data block:
// call:
//   int dbg = GetDataBlockGroup();
//   for (;;) AddToFetchDataBlockToDataBlockGroup(fc, dbg, a[], s[])
//   InitFetchingData(fc, dbg, Callback);

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


#include <stdint.h>
#include <inttypes.h>
#include "Platform.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Scheduler.h"
#include "BaseMessages.h"

#include "DelayedFetchData.h"


//#define WRITE_DEBUG_LOG
#ifdef WRITE_DEBUG_LOG
static FILE *Debug;
#define DEBUG_LOG0(s) fprintf(Debug, s),fflush(Debug)
#define DEBUG_LOG(s, ...) fprintf(Debug, s, __VA_ARGS__),fflush(Debug)
#else
#define DEBUG_LOG0(s)
#define DEBUG_LOG(s, ...)
#endif

typedef struct FETCH_DATA_CHANNEL_S {
    int Used;
    int ChannelNo;
    enum FetchDataChannelType Type;
    union {
        int Property;
        struct {
            int Pid;
        } ExternProcess;
        struct {
            int ConnectionNo;
        } Xcp;
        struct {
            int ConnectionNo;
        } Ccp;
        struct {
            int FileHandle;
        } S19;
    } Properties;
    uint64_t Timeout;
    uint64_t TimeoutAbs;
    DATA_BLOCK_GROUP *ToFetchDataBlockGroups;
    struct FETCH_DATA_CHANNEL_S *Next;
} FETCH_DATA_CHANNEL;

static CRITICAL_SECTION CriticalSection;


void InitFetchData(void)
{
#ifdef WRITE_DEBUG_LOG
    Debug = fopen ("c:\\temp\\DelayedFetchData.txt", "wt");
    DEBUG_LOG0("Start debug:\n");
#endif
    InitializeCriticalSection (&CriticalSection);
}

static DATA_BLOCK *FreeDataBlocks;

static DATA_BLOCK_GROUP **DataBlockGroups;
static int DataBlockGroupCount;
static int DataBlockGroupSize;
static DATA_BLOCK_GROUP *FreeDataBlockGroups;

static FETCH_DATA_CHANNEL **FetchDataChannels;
static int FetchDataChannelCount;
static int FetchDataChannelSize;
static FETCH_DATA_CHANNEL *FreeFetchDataChannels;

#define CONNECTION_TYPE_COUNT 4
// If a fetch request cannot fulfill immediately it will be added to that lists
static FETCH_DATA_CHANNEL *OpenRequestsFetchDataChannels[CONNECTION_TYPE_COUNT];
static int OpenRequestsFetchDataChannelCount[CONNECTION_TYPE_COUNT];

void TerminateFetchData(void)
{
#ifdef WRITE_DEBUG_LOG
    DEBUG_LOG0("Stop debug:\n");
    fclose(Debug);
#endif
    EnterCriticalSection(&CriticalSection);
    while (FreeDataBlockGroups != NULL) {
        DATA_BLOCK_GROUP *Next = FreeDataBlockGroups->Next;
        my_free(FreeDataBlockGroups);
        FreeDataBlockGroups = Next;
    }
    if (FreeDataBlockGroups != NULL) my_free(FreeDataBlockGroups);
    FreeDataBlockGroups = NULL;
    DataBlockGroupCount = 0;
    FetchDataChannelSize = 0;
    while (FreeFetchDataChannels != NULL) {
        FETCH_DATA_CHANNEL *Next = FreeFetchDataChannels->Next;
        my_free(FreeFetchDataChannels);
        FreeFetchDataChannels = Next;
    }
    if (FetchDataChannels != NULL) my_free(FetchDataChannels);
    FreeFetchDataChannels = NULL;
    FetchDataChannelCount = 0;
    FetchDataChannelSize = 0;

    while (FreeDataBlocks != NULL) {
        DATA_BLOCK *Next = FreeDataBlocks->Next;
        my_free(FreeDataBlocks->Data);
        my_free(FreeDataBlocks);
        FreeDataBlocks = Next;
    }
    FreeDataBlocks = NULL;

    if (DataBlockGroups != NULL) my_free(DataBlockGroups);
    DataBlockGroups = NULL;
    DataBlockGroupCount = 0;
    DataBlockGroupSize = 0;

    LeaveCriticalSection(&CriticalSection);
}

#ifdef WRITE_DEBUG_LOG
// this must be called inside the CriticalSection
static void DebugCheckFetchList(int par_Type)
{
    int x;
    FETCH_DATA_CHANNEL *fdc = OpenRequestsFetchDataChannels[par_Type];
    for (x = 0; (fdc != NULL) && (x < OpenRequestsFetchDataChannelCount[par_Type]); x++) {
        if (fdc == fdc->Next) {
            DEBUG_LOG0("There is an endless loop");
        }
        fdc = fdc->Next;
    }
    if ((fdc != NULL) && (x != OpenRequestsFetchDataChannelCount[par_Type])) {
        DEBUG_LOG0("Wrong list element count");
    }
}

// this must be called inside the CriticalSection
static void DebugCheckIfInsideFetchList(int par_Type, int par_No)
{
    int x;
    FETCH_DATA_CHANNEL *fdc = OpenRequestsFetchDataChannels[par_Type];
    for (x = 0; (fdc != NULL) && (x < OpenRequestsFetchDataChannelCount[par_Type]); x++) {
        if (fdc == fdc->Next) {
            DEBUG_LOG0("There is an endless loop");
        }
        if (par_No == fdc->ChannelNo) {
            DEBUG_LOG0("Already inside list");
        }
        fdc = fdc->Next;
    }
    if ((fdc != NULL) && (x != OpenRequestsFetchDataChannelCount[par_Type])) {
        DEBUG_LOG0("Wrong list element count");
    }
}
static void PrintDataBlocksToFile(char *par_Txt, DATA_BLOCK *par_DataBlock)
{
    if (Debug != NULL) {
        int Count = 0;
        DATA_BLOCK *db = par_DataBlock;
        fprintf(Debug, "  %s =", par_Txt);
        while (db != NULL) {
            fprintf(Debug, " %p->%p", db, db->Next);
            db = db->Next;
            Count++;
            if (Count > 10) {
                fprintf(Debug, " ...");
                break;
            }
        }
        fprintf(Debug, "\n");
        fflush(Debug);
    }
}
#else
#define DebugCheckFetchList(par_Type)
#define DebugCheckIfInsideFetchList(par_Type, par_No)
#define PrintDataBlocksToFile(par_Txt, par_DataBlock)
#endif

int NewFetchDataChannel(enum FetchDataChannelType par_FetchDataChannelType, int par_Property, uint64_t par_Timeout)
{
    int Ret = -1;
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("NewFetchDataChannel(%i, %i, %" PRIu64 ")\n", par_FetchDataChannelType, par_Property, par_Timeout);
    if (FreeFetchDataChannels != NULL) {
        Ret = FreeFetchDataChannels->ChannelNo;
        FreeFetchDataChannels = FreeFetchDataChannels->Next;
    } else {
        // add a new one
        if (FetchDataChannelCount >= FetchDataChannelSize) {
            FetchDataChannelSize += 16 + (FetchDataChannelSize >> 2);
            FetchDataChannels = (FETCH_DATA_CHANNEL**)my_realloc(FetchDataChannels, sizeof(FetchDataChannels[0]) * FetchDataChannelSize);
            if (FetchDataChannels == NULL) {
                LeaveCriticalSection(&CriticalSection);
                ThrowError (1, "out of memory");
                return -1;
            }
        }
        FetchDataChannels[FetchDataChannelCount] = (FETCH_DATA_CHANNEL*)my_malloc(sizeof(FETCH_DATA_CHANNEL));
        if (FetchDataChannels[FetchDataChannelCount] == NULL) {
            LeaveCriticalSection(&CriticalSection);
            ThrowError (1, "out of memory");
            return -1;
        }
        FetchDataChannels[FetchDataChannelCount]->ChannelNo = FetchDataChannelCount;
        Ret = FetchDataChannelCount;
        FetchDataChannelCount++;
    }
    FetchDataChannels[Ret]->Used = 1;
    FetchDataChannels[Ret]->Type = par_FetchDataChannelType;
    FetchDataChannels[Ret]->Properties.Property = par_Property;
    FetchDataChannels[Ret]->Timeout = par_Timeout;
    FetchDataChannels[Ret]->ToFetchDataBlockGroups = NULL;
    FetchDataChannels[Ret]->Next = NULL;
    LeaveCriticalSection(&CriticalSection);
    return Ret;
}

int CloseFetchDataChannel (int par_FetchDataChannelNo)
{
    int Ret = -1;
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("CloseFetchDataChannel(%i)\n", par_FetchDataChannelNo);
    if ((par_FetchDataChannelNo >= 0) && (par_FetchDataChannelNo < FetchDataChannelCount)) {
        FETCH_DATA_CHANNEL *fdc = FetchDataChannels[par_FetchDataChannelNo];
        if ((fdc != NULL) && (fdc->Used)) {
            if (fdc->ToFetchDataBlockGroups != NULL) {
                DATA_BLOCK_GROUP *dbg = fdc->ToFetchDataBlockGroups;
                if (dbg->StartList != NULL) {
                    DATA_BLOCK *db = dbg->StartList;
                    //PrintDataBlocksToFile("FreeDataBlocks before", FreeDataBlocks);
                    //PrintDataBlocksToFile("to add", db);
                    while (db->Next != NULL) {
                        db = db->Next;
                    }
                    db->Next = FreeDataBlocks;
                    FreeDataBlocks = dbg->StartList;
                    //PrintDataBlocksToFile("FreeDataBlocks behind", FreeDataBlocks);
                    dbg->StartList = NULL;
                }
                dbg->Used = 0;
            }
            fdc->Used = 0;
            fdc->Next = FreeFetchDataChannels;
            FreeFetchDataChannels = fdc;
            Ret = 0;
        }
    }
    LeaveCriticalSection(&CriticalSection);
    return Ret;
}

int GetDataBlockGroup(int par_Dir)
{
    int Ret = -1;
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG0("GetDataBlockGroup()\n");
    if (FreeDataBlockGroups != NULL) {
        Ret = FreeDataBlockGroups->BlockNo;
        FreeDataBlockGroups = FreeDataBlockGroups->Next;
    } else {
        // add a new one
        if (DataBlockGroupCount >= DataBlockGroupSize) {
            DataBlockGroupSize += 16 + (DataBlockGroupSize >> 2);
            DataBlockGroups = (DATA_BLOCK_GROUP**)my_realloc(DataBlockGroups, sizeof(DataBlockGroups[0]) * DataBlockGroupSize);
            if (DataBlockGroups == NULL) {
                LeaveCriticalSection(&CriticalSection);
                ThrowError (1, "out of memory");
                return -1;
            }
        }
        DataBlockGroups[DataBlockGroupCount] = (DATA_BLOCK_GROUP*)my_malloc(sizeof(DATA_BLOCK_GROUP));
        if (DataBlockGroups[DataBlockGroupCount] == NULL) {
            LeaveCriticalSection(&CriticalSection);
            ThrowError (1, "out of memory");
            return -1;
        }
        DataBlockGroups[DataBlockGroupCount]->BlockNo = DataBlockGroupCount;
        Ret = DataBlockGroupCount;
        DataBlockGroupCount++;
    }
    DataBlockGroups[Ret]->Used = 1;
    DataBlockGroups[Ret]->Dir = par_Dir;
    DataBlockGroups[Ret]->Next = NULL;
    DataBlockGroups[Ret]->StartList = NULL;
    DataBlockGroups[Ret]->Current = NULL;
    DataBlockGroups[Ret]->CallbackFunction = NULL;
    LeaveCriticalSection(&CriticalSection);
    return Ret;
}

void FreeDataBlockGroup(int par_DataGroupNo)
{
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("FreeDataBlockGroup(%i)\n", par_DataGroupNo);
    if ((par_DataGroupNo >= 0) && (par_DataGroupNo < DataBlockGroupCount)) {
        DATA_BLOCK_GROUP *dbg = DataBlockGroups[par_DataGroupNo];
        if (!dbg->Used) {
            LeaveCriticalSection(&CriticalSection);
            //ThrowError(1, "internal error FreeDataBlockGroup()");
            // this can happen if the A2L file is reloaded!
            return;
        }
        dbg->Used = 0;
        // move the data blocks to the free list
        if (dbg->StartList != NULL) {
            DATA_BLOCK *db = dbg->StartList;
            while (db->Next != NULL) db = db->Next;
            db->Next = FreeDataBlocks;
            FreeDataBlocks = dbg->StartList;
            dbg->StartList = NULL;
        }
        dbg->Next = FreeDataBlockGroups;
        FreeDataBlockGroups = dbg;
    }
    PrintDataBlocksToFile("FreeDataBlocks", FreeDataBlocks);
    LeaveCriticalSection(&CriticalSection);
}

DATA_BLOCK *GetDataBlockGroupListStartPointer(int par_DataGroupNo)
{
    DATA_BLOCK *Ret = NULL;
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("GetDataBlockGroupListStartPointer(%i)\n", par_DataGroupNo);
    if ((par_DataGroupNo >= 0) && (par_DataGroupNo < DataBlockGroupCount)) {
        DATA_BLOCK_GROUP *dbg = DataBlockGroups[par_DataGroupNo];
        Ret = dbg->StartList;
    }
    LeaveCriticalSection(&CriticalSection);
    return Ret;
}

int IsDataBlockGroupEmpty(int par_DataGroupNo)
{
    int Ret = -1;
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("IsDataBlockGroupEmpty(%i)", par_DataGroupNo);
    if ((par_DataGroupNo >= 0) && (par_DataGroupNo < DataBlockGroupCount)) {
        DATA_BLOCK_GROUP *dbg = DataBlockGroups[par_DataGroupNo];
        Ret = (dbg->StartList == NULL);
    }
    DEBUG_LOG(" = %i\n", Ret);
    LeaveCriticalSection(&CriticalSection);
    return Ret;

}

int GetDataFromDataBlockGroup(int par_DataGroupNo, uint64_t par_StartAddress, uint32_t par_BlockSize, char *ret_Data)
{
    MEMSET(ret_Data, 0, par_BlockSize);
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("GetDataFromDataBlockGroup(%i, 0x%" PRIX64 ", %u)\n", par_DataGroupNo, par_StartAddress, par_BlockSize);
    if ((par_DataGroupNo >= 0) && (par_DataGroupNo < DataBlockGroupCount)) {
        DATA_BLOCK_GROUP *dbg = DataBlockGroups[par_DataGroupNo];
        // first check if this can be added to an existing block
        DATA_BLOCK *db = dbg->StartList;
        while(db != NULL) {
            if ((par_StartAddress >= db->StartAddress) &&
                ((db->StartAddress + db->BlockSize) <= par_StartAddress)) {
                int y = 0;
                for (int x = (int)(par_StartAddress - db->StartAddress);
                     (x < (int)db->BlockSize) && (y < (int)par_BlockSize); x++, y++) {
                    ret_Data[y] = ((char*)db->Data)[x];
                }
            } else if ((db->StartAddress < (par_StartAddress + par_BlockSize)) &&
                       (db->StartAddress >= par_StartAddress)) {
                int y = 0;
                for (int x = (int)(par_StartAddress - db->StartAddress);
                     (x < (int)par_BlockSize) && (y < (int)db->BlockSize); x++, y++) {
                    ret_Data[x] = ((char*)db->Data)[y];
                }
            }
            db = db->Next;
        }
    }
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

int AddToFetchDataBlockToDataBlockGroup(int par_DataGroupNo, uint64_t par_StartAddress, uint32_t par_BlockSize, void *par_Data)
{
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("AddToFetchDataBlockToDataBlockGroup(%i, 0x%" PRIX64 ", %u)\n", par_DataGroupNo, par_StartAddress, par_BlockSize);
    if ((par_DataGroupNo >= 0) && (par_DataGroupNo < DataBlockGroupCount)) {
        DATA_BLOCK_GROUP *dbg = DataBlockGroups[par_DataGroupNo];
        PrintDataBlocksToFile("dbg->StartList", dbg->StartList);
        // first check if this can be added to an existing block
        DATA_BLOCK *pdb, *ndb, *db = dbg->StartList;
        while(db != NULL) {
            // can it be added at the end?
            if ((par_StartAddress >= db->StartAddress) &&
                (par_StartAddress <= (db->StartAddress + db->BlockSize))) {
                DEBUG_LOG0("  can added at the end\n");
                int Offset = (int)(par_StartAddress - db->StartAddress);
                int NewBlockSize = Offset + par_BlockSize;
                if (NewBlockSize > (int)db->MaxBlockSize) {
                    db->MaxBlockSize = NewBlockSize + 64;
                    DEBUG_LOG("  resize block to %i\n", db->MaxBlockSize);
                    db->Data = my_realloc(db->Data, db->MaxBlockSize);
                    if (db->Data == NULL) {
                        LeaveCriticalSection(&CriticalSection);
                        ThrowError (1, "out of memory");
                        return -1;
                    }
                }
                if (par_Data != NULL) MEMCPY((char*)db->Data + Offset, par_Data, par_BlockSize);
                db->BlockSize = NewBlockSize;
                if (db->BlockSize > db->MaxBlockSize) ThrowError(1, "Internal error: %s(%i)" __FILE__, __LINE__);
                // can this block connect to the next one?
                if (db->Next != NULL) {
                    ndb = db->Next;
                    if (ndb->StartAddress <= (db->StartAddress + db->BlockSize)) {
                        DEBUG_LOG0("  block connect to the next one\n");
                        db->Next = ndb->Next;
                        int Offset2 = (int)(ndb->StartAddress - db->StartAddress);
                        int NewBlockSize2 = Offset2 + ndb->BlockSize;
                        if (NewBlockSize2 > (int)db->MaxBlockSize) {
                            db->MaxBlockSize += NewBlockSize2 + 64;
                            DEBUG_LOG("  resize block to %i\n", db->MaxBlockSize);
                            db->Data = my_realloc(db->Data, db->MaxBlockSize);
                            if (db->Data == NULL) {
                                LeaveCriticalSection(&CriticalSection);
                                ThrowError (1, "out of memory");
                                return -1;
                            }
                        }
                        if (par_Data != NULL) MEMCPY((char*)db->Data + Offset2, ndb->Data, ndb->BlockSize);
                        db->BlockSize += ndb->BlockSize;
                        if (db->BlockSize > db->MaxBlockSize) ThrowError(1, "Internal error: %s(%i)" __FILE__, __LINE__);
                        // free the merged block
                        ndb->Next = FreeDataBlocks;
                        FreeDataBlocks = ndb;
                    }

                }
                goto __OUT;
            // can it be added at the begining?
            } else if (((par_StartAddress + par_BlockSize) >= db->StartAddress) &&
                        (par_StartAddress < (db->StartAddress + db->BlockSize))) {
                DEBUG_LOG0("  can added to the begining\n");
                int Offset = (int)(db->StartAddress - par_StartAddress);
                int NewBlockSize = Offset + par_BlockSize;
                if (NewBlockSize > (int)db->MaxBlockSize) {
                    db->MaxBlockSize = NewBlockSize + 64;
                    DEBUG_LOG("  resize block to %i\n", db->MaxBlockSize);
                    db->Data = my_realloc(db->Data, db->MaxBlockSize);
                    if (db->Data == NULL) {
                        LeaveCriticalSection(&CriticalSection);
                        ThrowError (1, "out of memory");
                        return -1;
                    }
                }
                if (par_Data != NULL) {
                    memmove((char*)db->Data + Offset, db->Data, db->BlockSize);
                    MEMCPY(db->Data, par_Data, par_BlockSize);
                }
                db->StartAddress = par_StartAddress;
                db->BlockSize = NewBlockSize;
                if (db->BlockSize > db->MaxBlockSize) ThrowError(1, "Internal error: %s(%i)" __FILE__, __LINE__);
                goto __OUT;
            }
            db = db->Next;
        }

        DEBUG_LOG0("  use a new block\n");
        // than check if something is free for reuse and is large enough
        pdb = NULL;
        ndb = FreeDataBlocks;
        while(ndb != NULL) {
            if (ndb->MaxBlockSize >= par_BlockSize) {
                if (pdb == NULL) {
                    FreeDataBlocks = ndb->Next;
                } else {
                    pdb->Next = ndb->Next;
                }
                goto __MATCH;
            }
            ndb = ndb->Next;
        }
        ndb = (DATA_BLOCK*)my_malloc(sizeof(DATA_BLOCK));
        if (ndb == NULL) {
            LeaveCriticalSection(&CriticalSection);
            ThrowError (1, "out of memory");
            return -1;
        }
        // use minimum 64 bytes
        if (par_BlockSize > 64) {
            ndb->MaxBlockSize = par_BlockSize;
        } else {
            ndb->MaxBlockSize = 64;
        }
        ndb->Data = my_malloc(ndb->MaxBlockSize);
__MATCH:
        ndb->BlockSize = par_BlockSize;
        if (ndb->BlockSize > ndb->MaxBlockSize) ThrowError(1, "Internal error: %s(%i)" __FILE__, __LINE__);
        ndb->StartAddress = par_StartAddress;
        if (par_Data != NULL) {
            MEMCPY(ndb->Data, par_Data, par_BlockSize);
        }
        // insert sorted (lower adress first)
        if (dbg->StartList == NULL) {
             dbg->StartList = ndb;
             ndb->Next = NULL;
        } else {
            DATA_BLOCK *pdb = NULL;
            db = dbg->StartList;
            while(1) {
                if (par_StartAddress < db->StartAddress) {
                    if (pdb == NULL) {
                        dbg->StartList = ndb;
                    } else {
                        pdb->Next = ndb;
                    }
                    ndb->Next = db;
                    break;
                }
                if (db->Next == NULL) {
                    ndb->Next = NULL;
                    db->Next = ndb;
                    break;
                } else {
                    pdb = db;
                    db = db->Next;
                }
            }
        }
        PrintDataBlocksToFile("dbg->StartList", dbg->StartList);
    }
__OUT:
    PrintDataBlocksToFile("FreeDataBlocks", FreeDataBlocks);
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

// This should be called by the scheduler
void CheckFetchDataAfterProcess(int par_Pid)
{
    EnterCriticalSection(&CriticalSection);
    if (OpenRequestsFetchDataChannelCount[GET_FROM_EXTERM_PROCESS_TYPE] > 0) {
        int Success = 1;
        FETCH_DATA_CHANNEL *prev_fdc = NULL;
        FETCH_DATA_CHANNEL *fdc = OpenRequestsFetchDataChannels[GET_FROM_EXTERM_PROCESS_TYPE];
        while (fdc != NULL) {
            if (fdc->Properties.ExternProcess.Pid == par_Pid) {
                DEBUG_LOG("CheckFetchDataAfterProcess(%i)\n", par_Pid);
                if (prev_fdc != NULL) prev_fdc->Next = fdc->Next;
                else OpenRequestsFetchDataChannels[GET_FROM_EXTERM_PROCESS_TYPE] = fdc->Next;
                OpenRequestsFetchDataChannelCount[GET_FROM_EXTERM_PROCESS_TYPE]--;
                DebugCheckFetchList(GET_FROM_EXTERM_PROCESS_TYPE);
                prev_fdc = fdc;
                LeaveCriticalSection(&CriticalSection);
                // now the process is not running it schould be able to read the data from there
                if (WaitUntilProcessIsNotActiveAndThanLockIt (par_Pid, 10,
                                                              ERROR_BEHAVIOR_NO_ERROR_MESSAGE,
                                                              "read from external process memory", __FILE__, __LINE__) == 0) {
                    FetchDataCallbackFunction Callback = fdc->ToFetchDataBlockGroups->CallbackFunction;
                    DATA_BLOCK *dp = fdc->ToFetchDataBlockGroups->StartList; //Current;
                    DEBUG_LOG("  dp = %p\n", dp);
                    while(dp != NULL) {
                        int Size;
                        if (fdc->ToFetchDataBlockGroups->Dir) {
                            Size = scm_write_bytes (dp->StartAddress, par_Pid, (unsigned char*)dp->Data, dp->BlockSize);
                        } else {
                            Size = scm_read_bytes (dp->StartAddress, par_Pid, (char*)dp->Data, dp->BlockSize);
                        }
                        DEBUG_LOG("    StartAddress = 0x%" PRIX64 ", BlockSize = %i -> Size = %i\n", dp->StartAddress, dp->BlockSize, Size);
                        if (Size != (int)dp->BlockSize) {
                            Success = 0;
                            break;
                        }
                        if (dp == dp->Next) {
                            ThrowError(1, "Internal error: %s(%i)" __FILE__, __LINE__);
                            break;
                        }
                        dp = dp->Next;
                    }
                    UnLockProcess (par_Pid);
                    Callback(fdc->ToFetchDataBlockGroups->OptParam, fdc->ToFetchDataBlockGroups->BlockNo, Success);
                    EnterCriticalSection(&CriticalSection);
                    break;
                } else {
                    // not handled
                    if (fdc->TimeoutAbs == 0) {
                        DEBUG_LOG0("  not handled set timeout\n");
                        fdc->TimeoutAbs = fdc->Timeout + GetSimulatedTimeInNanoSecond();
                        EnterCriticalSection(&CriticalSection);
                        fdc->Next = OpenRequestsFetchDataChannels[GET_FROM_EXTERM_PROCESS_TYPE];
                        OpenRequestsFetchDataChannels[GET_FROM_EXTERM_PROCESS_TYPE] = fdc;
                        OpenRequestsFetchDataChannelCount[GET_FROM_EXTERM_PROCESS_TYPE]++;
                        DebugCheckFetchList(GET_FROM_EXTERM_PROCESS_TYPE);
                    } else if (fdc->TimeoutAbs >= GetSimulatedTimeInNanoSecond()) {
                        DEBUG_LOG0("  not handled add it again\n");
                        EnterCriticalSection(&CriticalSection);
                        fdc->Next = OpenRequestsFetchDataChannels[GET_FROM_EXTERM_PROCESS_TYPE];
                        OpenRequestsFetchDataChannels[GET_FROM_EXTERM_PROCESS_TYPE] = fdc;
                        OpenRequestsFetchDataChannelCount[GET_FROM_EXTERM_PROCESS_TYPE]++;
                        DebugCheckFetchList(GET_FROM_EXTERM_PROCESS_TYPE);
                    } else {
                        // Callback with not successfull!
                        DEBUG_LOG0("  not handled and timed out\n");
                        fdc->ToFetchDataBlockGroups->CallbackFunction(fdc->ToFetchDataBlockGroups->OptParam, fdc->ToFetchDataBlockGroups->BlockNo, 0);
                        EnterCriticalSection(&CriticalSection);
                    }
                }
            }
            fdc = fdc->Next;
        }
    }
    LeaveCriticalSection(&CriticalSection);
}

// this must be called inside the CriticalSection
static int CheckIfInsideFetchList(int par_Type, int par_No)
{
    int x;
    FETCH_DATA_CHANNEL *fdc = OpenRequestsFetchDataChannels[par_Type];
    for (x = 0; (fdc != NULL) && (x < OpenRequestsFetchDataChannelCount[par_Type]); x++) {
        if (par_No == fdc->ChannelNo) {
            return 1;
        }
        fdc = fdc->Next;
    }
    return 0;
}


int InitFetchingData(int par_FetchDataChannelNo, int par_DataGroupNo, FetchDataCallbackFunction par_CallbackFunction, int par_OptParam)
{
    int Ret = -1;
    int CheckImmediatelyPid = 0;
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("InitFetchingData(%i, %i, %p, %i)\n", par_FetchDataChannelNo, par_DataGroupNo, par_CallbackFunction, par_OptParam);
    if ((par_FetchDataChannelNo >= 0) && (par_FetchDataChannelNo < FetchDataChannelCount)) {
        if ((par_DataGroupNo >= 0) && (par_DataGroupNo < DataBlockGroupCount)) {
            FETCH_DATA_CHANNEL *fdc = FetchDataChannels[par_FetchDataChannelNo];
            int Type = fdc->Type;
            if (!CheckIfInsideFetchList(Type, par_FetchDataChannelNo)) {  // do not add it twice to the list
                if (Type < CONNECTION_TYPE_COUNT) {
                    DATA_BLOCK_GROUP *dbg = DataBlockGroups[par_DataGroupNo];
                    fdc->ToFetchDataBlockGroups = dbg;
                    fdc->ToFetchDataBlockGroups->CallbackFunction = par_CallbackFunction;
                    fdc->ToFetchDataBlockGroups->OptParam = par_OptParam;
                    fdc->TimeoutAbs = 0;
                    DebugCheckFetchList(Type);
                    DebugCheckIfInsideFetchList(Type, par_FetchDataChannelNo);
                    fdc->Next = OpenRequestsFetchDataChannels[Type];
                    OpenRequestsFetchDataChannels[Type] = fdc;
                    OpenRequestsFetchDataChannelCount[Type]++;
                    DebugCheckFetchList(fdc->Type);
                    if (Type == GET_FROM_EXTERM_PROCESS_TYPE) {
                        CheckImmediatelyPid = fdc->Properties.ExternProcess.Pid;
                    }
                    Ret = 0;  // successfully added
                }
            } else {
                Ret = 1;  // it is already inside the list
            }
        }
    }
    LeaveCriticalSection(&CriticalSection);

   if (CheckImmediatelyPid) CheckFetchDataAfterProcess(CheckImmediatelyPid);
   return Ret;
}

void AbortFetchData(int par_FetchDataChannelNo, int par_DataGroupNo)
{
    EnterCriticalSection(&CriticalSection);
    DEBUG_LOG("AbortFetchData(%i, %i)\n", par_FetchDataChannelNo, par_DataGroupNo);
    if ((par_FetchDataChannelNo >= 0) && (par_FetchDataChannelNo < FetchDataChannelCount)) {
        if ((par_DataGroupNo >= 0) && (par_DataGroupNo < DataBlockGroupCount)) {
            FETCH_DATA_CHANNEL *fdc = FetchDataChannels[par_FetchDataChannelNo];
            if (fdc->Type < CONNECTION_TYPE_COUNT) {
                DATA_BLOCK_GROUP *dbg = DataBlockGroups[par_DataGroupNo];
                // move the data blocks to the free list
                if (dbg->StartList != NULL) {
                    DATA_BLOCK *db = dbg->StartList;
                    while (db->Next != NULL) db = db->Next;
                    db->Next = FreeDataBlocks;
                    FreeDataBlocks = dbg->StartList;
                    dbg->StartList = NULL;
                }
                // remove the peding request
                fdc->Next = OpenRequestsFetchDataChannels[fdc->Type];
                OpenRequestsFetchDataChannels[fdc->Type] = fdc;
                OpenRequestsFetchDataChannelCount[fdc->Type]--;
                DebugCheckFetchList(fdc->Type);
            }
        }
    }
    LeaveCriticalSection(&CriticalSection);
}

#if 0
// This should be called by the XCP ctrl process each cycle
int FetchDataXcpCyclic(int par_ConnectionNo, uint64_t *ret_TargetSrcAddress, void **ret_DstAddress)
{
    if (OpenRequestsFetchDataChannelCount[GET_FROM_XCP_CONNECTION_TYPE] > 0) {
        int Ret = 0;
        EnterCriticalSection(&CriticalSection);
        FETCH_DATA_CHANNEL *fdc = OpenRequestsFetchDataChannels[GET_FROM_XCP_CONNECTION_TYPE];
        while (fdc != NULL) {
            if (fdc->Properties.Xcp.ConnectionNo == par_ConnectionNo) {
                DATA_BLOCK *dp = fdc->ToFetchDataBlockGroups->Current;
                *ret_TargetSrcAddress = dp->StartAddress;
                *ret_DstAddress = dp->Data;
                Ret = dp->BlockSize;
                break;
            }
        }
        LeaveCriticalSection(&CriticalSection);
        return Ret;
    }
    return 0;
}

int FetchDataXcpCyclicDone(int par_ConnectionNo, int par_Success)
{
    if (OpenRequestsFetchDataChannelCount[GET_FROM_XCP_CONNECTION_TYPE] > 0) {
        int Ret = 0;
        FetchDataCallbackFunction Callback = NULL;
        EnterCriticalSection(&CriticalSection);
        FETCH_DATA_CHANNEL *prev_fdc = NULL;
        FETCH_DATA_CHANNEL *fdc = OpenRequestsFetchDataChannels[GET_FROM_XCP_CONNECTION_TYPE];
        while (fdc != NULL) {
            if (fdc->Properties.Xcp.ConnectionNo == par_ConnectionNo) {
                if (!par_Success) {
                    Callback = fdc->ToFetchDataBlockGroups->CallbackFunction;
                    break;
                }
                DATA_BLOCK *dp = fdc->ToFetchDataBlockGroups->Current;
                if (dp->Next == NULL) {
                    // this was the last block
                    Callback = fdc->ToFetchDataBlockGroups->CallbackFunction;
                    if (prev_fdc != NULL) prev_fdc->Next = fdc->Next;
                    else OpenRequestsFetchDataChannels[GET_FROM_XCP_CONNECTION_TYPE] = NULL;
                    OpenRequestsFetchDataChannelCount[GET_FROM_XCP_CONNECTION_TYPE]--;
                } else {
                    // there are at least one more block
                    fdc->ToFetchDataBlockGroups->Current = dp->Next;
                    Ret = 1;
                }
                break;
            }
        }
        LeaveCriticalSection(&CriticalSection);
        if (Callback != NULL) Callback(fdc->ToFetchDataBlockGroups->OptParam, fdc->ToFetchDataBlockGroups->BlockNo, par_Success);
        return Ret;
    }
    return 0;
}
#endif

// only for debuging
void PrintBlockGroupToFile(FILE *par_File, int par_DataGroupNo)
{
    DATA_BLOCK *Current = GetDataBlockGroupListStartPointer(par_DataGroupNo);
    fprintf (par_File, "DataGroup = %i:\n", par_DataGroupNo);
    for (int x = 0; Current != NULL; Current = Current->Next, x++) {
        fprintf (par_File, "  %i (0x%" PRIX64 ")[%i]: ", x, Current->StartAddress, Current->BlockSize);
        for (int i = 0; i < (int)Current->BlockSize; i++) {
            fprintf(par_File, " %02X", (uint32_t)((uint8_t*)Current->Data)[i]);
        }
        fprintf(par_File, "\n");
        fflush(par_File);
    }
}

// Each window should add a fetch channel
//   int fc = NewFetchDataChannel(GET_FROM_EXTERM_PROCESS_TYPE, pid, 1000*1000*1000llu);

// To get a data block:
// call:
//   int dbg = GetDataBlockGroup();
//   for (;;) AddToFetchDataBlockToDataBlockGroup(fc, dbg, a[], s[])
//   InitFetchingData(fc, dbg, Callback);
//    inside the callback call
//      DATA_BLOCK *db = GetDataBlockGroupListStartPointer(dbg);
//      FreeDataBlockGroup(dbg);



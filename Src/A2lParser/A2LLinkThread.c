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
#include "Platform.h"
#include "ThrowError.h"
#include "MyMemory.h"

#include "A2LAccessData.h"
#include "A2LLink.h"
#include "DelayedFetchData.h"
#include "A2LLinkThread.h"

//#define WRITE_DEBUG_LOG
#ifdef WRITE_DEBUG_LOG
static FILE *Debug;
#define DEBUG_LOG0(s) fprintf(Debug, s),fflush(Debug)
#define DEBUG_LOG(s, ...) fprintf(Debug, s, __VA_ARGS__),fflush(Debug)
#else
#define DEBUG_LOG0(s)
#define DEBUG_LOG(s, ...)
#endif

#define UNUSED(x) (void)(x)

static CRITICAL_SECTION CriticalSection;
static CONDITION_VARIABLE ConditionVariable;
static int SleepFlag;

#define DEBUG_CRITICAL_SECTIONS

#ifdef DEBUG_CRITICAL_SECTIONS
static int32_t DebugCriticalSectionLineNr;
static DWORD DebugCriticalSectionThreadId;
static void EnterCriticalSectionDebug(CRITICAL_SECTION *cs, int32_t LineNr)
{
    EnterCriticalSection(cs);
    if (DebugCriticalSectionThreadId) {
        ThrowError (1, "Internal error, that should never happen %s(%i)", __FILE__, __LINE__);
    }
    DebugCriticalSectionLineNr = LineNr;
    DebugCriticalSectionThreadId = GetCurrentThreadId();
}

static void LeaveCriticalSectionDebug(CRITICAL_SECTION *cs)
{
    if (DebugCriticalSectionThreadId != GetCurrentThreadId()) {
        ThrowError (1, "Internal error, that should never happen %s(%i)", __FILE__, __LINE__);
    }
    DebugCriticalSectionLineNr = 0;
    DebugCriticalSectionThreadId = 0;
    LeaveCriticalSection(cs);
}
static void SleepConditionVariableCSDebug (CONDITION_VARIABLE *cv, CRITICAL_SECTION *cs, DWORD t, int32_t LineNr)
{
    if (DebugCriticalSectionThreadId != GetCurrentThreadId()) {
        ThrowError (1, "Internal error, that should never happen %s(%i)", __FILE__, __LINE__);
    }
    DebugCriticalSectionLineNr = 0;
    DebugCriticalSectionThreadId = 0;
    SleepConditionVariableCS (cv, cs, t);
    if (DebugCriticalSectionThreadId) {
        ThrowError (1, "Internal error, that should never happen %s(%i)", __FILE__, __LINE__);
    }
    DebugCriticalSectionLineNr = LineNr;
    DebugCriticalSectionThreadId = GetCurrentThreadId();
}

#define ENTER_CRITICAL_SECTION(cs) EnterCriticalSectionDebug((cs), __LINE__)
#define LEAVE_CRITICAL_SECTION(cs) LeaveCriticalSectionDebug((cs))
#define SLEEP_CONDITIONAL_VARIABLE_CS(cv, cs, t) SleepConditionVariableCSDebug((cv), (cs), (t), __LINE__)
#else
#define ENTER_CRITICAL_SECTION(cs) ENTER_CRITICAL_SECTION((cs))
#define LEAVE_CRITICAL_SECTION(cs) LEAVE_CRITICAL_SECTION((cs))
#define SLEEP_CONDITIONAL_VARIABLE_CS(cv, cs, t) SleepConditionVariableCS((cv), (cs), (t))
#endif

typedef struct GET_DATA_FROM_LINK_CHANNEL_S {
    int Used;
    int ChannelNo;
    int LinkNo;
    int FetchDataChannelNo;
    A2LGetDataFromLinkAckCallback Callback;
    void *Param;
    struct GET_DATA_FROM_LINK_CHANNEL_S *Next;
} GET_DATA_FROM_LINK_CHANNEL;

static GET_DATA_FROM_LINK_CHANNEL **GetDataFromLinkChannels;
static int GetDataFromLinkChannelCount;
static int GetDataFromLinkChannelSize;
static GET_DATA_FROM_LINK_CHANNEL *FreeGetDataFromLinkChannels;


typedef struct GET_DATA_FROM_LINK_REQ_S {
    int Used;
    int BlockNo;
    int ChannelNo;
    int LinkNo;
    INDEX_DATA_BLOCK *IndexData;
    int Success;
    const char *Error;
    int FetchDataChannelNo;
    A2LGetDataFromLinkAckCallback Callback;
    void *Param;
    int DimensionGroupNo;
    int DataGroupNo;
    int State;
    int Dir;    // 0 -> RD, 1 -> WR
    struct GET_DATA_FROM_LINK_REQ_S *Next;
} GET_DATA_FROM_LINK_REQ;

static GET_DATA_FROM_LINK_REQ **DataFromLinkReqests;
static int DataFromLinkReqestCount;
static int DataFromLinkReqestSize;
static GET_DATA_FROM_LINK_REQ *StartDataFromLinkReqests;
static GET_DATA_FROM_LINK_REQ *FreeDataFromLinkReqests;


static INDEX_DATA_BLOCK *FreeLinkDataBlocks;

INDEX_DATA_BLOCK *GetIndexDataBlock(int par_Size)
{
    INDEX_DATA_BLOCK *ldb;
    INDEX_DATA_BLOCK *pldb = NULL;

    ENTER_CRITICAL_SECTION(&CriticalSection);
    DEBUG_LOG("GetIndexDataBlock(%i)\n", par_Size);

    // than check if something is free for reuse and is large enough
    ldb = FreeLinkDataBlocks;
    while(ldb != NULL) {
        if (ldb->MaxSize >= (uint32_t)par_Size) {
            if (pldb == NULL) {
                FreeLinkDataBlocks = ldb->Next;
            } else {
                pldb->Next = ldb->Next;
            }
            goto __MATCH;
        }
        pldb = ldb;
        ldb = ldb->Next;
    }
    ldb = (INDEX_DATA_BLOCK*)my_malloc(sizeof(INDEX_DATA_BLOCK));
    if (ldb == NULL) {
        LEAVE_CRITICAL_SECTION(&CriticalSection);
        ThrowError (1, "out of memory");
        return NULL;
    }
    // use minimum 64 bytes
    if (par_Size > 64) {
        ldb->MaxSize = par_Size;
    } else {
        ldb->MaxSize = 64;
    }
    ldb->Data = (INDEX_DATA_BLOCK_ELEM*)my_malloc(sizeof(INDEX_DATA_BLOCK_ELEM) * ldb->MaxSize);
    if (ldb->Data == NULL) {
        LEAVE_CRITICAL_SECTION(&CriticalSection);
        ThrowError (1, "out of memory");
        return NULL;
    }
__MATCH:
    ldb->Size = par_Size;
    ldb->CurrentPos = 0;
    LEAVE_CRITICAL_SECTION(&CriticalSection);
    return ldb;
}


void FreeIndexDataBlock(INDEX_DATA_BLOCK *par_LinkDataBlocks)
{
    ENTER_CRITICAL_SECTION(&CriticalSection);
    DEBUG_LOG("FreeIndexDataBlock(%p)\n", par_LinkDataBlocks);
    par_LinkDataBlocks->Next = FreeLinkDataBlocks;
    FreeLinkDataBlocks = par_LinkDataBlocks;
    LEAVE_CRITICAL_SECTION(&CriticalSection);
}

int NewDataFromLinkChannel(int par_LinkNo, uint64_t par_Timeout,
                           A2LGetDataFromLinkAckCallback par_Callback, void *par_Param)
{
    int Ret = -1;
    int Pid;
    ENTER_CRITICAL_SECTION(&CriticalSection);
    DEBUG_LOG("NewDataFromLinkChannel(%i, %" PRIu64 ", %p, %p)\n", par_LinkNo, par_Timeout, par_Callback, par_Param);

    if (FreeGetDataFromLinkChannels != NULL) {
        Ret = FreeGetDataFromLinkChannels->ChannelNo;
        FreeGetDataFromLinkChannels = FreeGetDataFromLinkChannels->Next;
    } else {
        // add a new one
        if (GetDataFromLinkChannelCount >= GetDataFromLinkChannelSize) {
            GetDataFromLinkChannelSize += 16 + (GetDataFromLinkChannelSize >> 2);
            GetDataFromLinkChannels = (GET_DATA_FROM_LINK_CHANNEL**)my_realloc(GetDataFromLinkChannels, sizeof(GetDataFromLinkChannels[0]) * GetDataFromLinkChannelSize);
            if (GetDataFromLinkChannels == NULL) {
                LEAVE_CRITICAL_SECTION(&CriticalSection);
                ThrowError (1, "out of memory");
                return -1;
            }
        }
        GetDataFromLinkChannels[GetDataFromLinkChannelCount] = (GET_DATA_FROM_LINK_CHANNEL*)my_malloc(sizeof(GET_DATA_FROM_LINK_CHANNEL));
        if (GetDataFromLinkChannels[GetDataFromLinkChannelCount] == NULL) {
            LEAVE_CRITICAL_SECTION(&CriticalSection);
            ThrowError (1, "out of memory");
            return -1;
        }
        GetDataFromLinkChannels[GetDataFromLinkChannelCount]->ChannelNo = GetDataFromLinkChannelCount;
        Ret = GetDataFromLinkChannelCount;
        GetDataFromLinkChannelCount++;
    }
    GetDataFromLinkChannels[Ret]->Used = 1;
    GetDataFromLinkChannels[Ret]->LinkNo = par_LinkNo;
    Pid = A2LGetLinkedToPid(par_LinkNo);
    GetDataFromLinkChannels[Ret]->FetchDataChannelNo = NewFetchDataChannel(GET_FROM_EXTERM_PROCESS_TYPE, Pid, par_Timeout);
    GetDataFromLinkChannels[Ret]->Callback = par_Callback;
    GetDataFromLinkChannels[Ret]->Param = par_Param;
    GetDataFromLinkChannels[Ret]->Next = NULL;
    LEAVE_CRITICAL_SECTION(&CriticalSection);
    return Ret;
}

int DeleteDataFromLinkChannel (int par_DataFromLinkChannelNo)
{
    int Ret = -1;
    ENTER_CRITICAL_SECTION(&CriticalSection);
    DEBUG_LOG("DeleteDataFromLinkChannel(%i)\n", par_DataFromLinkChannelNo);
    if ((par_DataFromLinkChannelNo >= 0) && (par_DataFromLinkChannelNo < GetDataFromLinkChannelCount)) {
        GET_DATA_FROM_LINK_CHANNEL *gdflc = GetDataFromLinkChannels[par_DataFromLinkChannelNo];
        if ((gdflc != NULL) && (gdflc->Used)) {
            CloseFetchDataChannel(gdflc->FetchDataChannelNo);
            gdflc->Used = 0;
            gdflc->Next = FreeGetDataFromLinkChannels;
            FreeGetDataFromLinkChannels = gdflc;
            Ret = 0;
        }
    }
    LEAVE_CRITICAL_SECTION(&CriticalSection);
    return Ret;
}

int A2LGetDataFromLinkReq(int par_DataFromLinkChannelNo, int par_Dir, INDEX_DATA_BLOCK *par_IndexData)
{
    int Ret = -1;
    ENTER_CRITICAL_SECTION(&CriticalSection);
    DEBUG_LOG("A2LGetDataFromLinkReq(%i, %p)\n", par_DataFromLinkChannelNo, par_IndexData);

    if ((par_DataFromLinkChannelNo < 0) || (par_DataFromLinkChannelNo >= GetDataFromLinkChannelCount) ||
        (GetDataFromLinkChannels[par_DataFromLinkChannelNo]->ChannelNo != par_DataFromLinkChannelNo)) {
        LEAVE_CRITICAL_SECTION(&CriticalSection);
        if (par_DataFromLinkChannelNo >= 0) {
            ThrowError (1, "Internal error: unknown par_DataFromLinkChannelNo = %i", par_DataFromLinkChannelNo);
        }
        return -1;

    }

    if (FreeDataFromLinkReqests != NULL) {
        Ret = FreeDataFromLinkReqests->BlockNo;
        FreeDataFromLinkReqests = FreeDataFromLinkReqests->Next;
    } else {
        // add a new one
        if (DataFromLinkReqestCount >= DataFromLinkReqestSize) {
            DataFromLinkReqestSize += 16 + (DataFromLinkReqestSize >> 2);
            DataFromLinkReqests = (GET_DATA_FROM_LINK_REQ**)my_realloc(DataFromLinkReqests, sizeof(DataFromLinkReqests[0]) * DataFromLinkReqestSize);
            if (DataFromLinkReqests == NULL) {
                LEAVE_CRITICAL_SECTION(&CriticalSection);
                ThrowError (1, "out of memory");
                return -1;
            }
        }
        DataFromLinkReqests[DataFromLinkReqestCount] = (GET_DATA_FROM_LINK_REQ*)my_malloc(sizeof(GET_DATA_FROM_LINK_REQ));
        if (DataFromLinkReqests[DataFromLinkReqestCount] == NULL) {
            LEAVE_CRITICAL_SECTION(&CriticalSection);
            ThrowError (1, "out of memory");
            return -1;
        }
        DataFromLinkReqests[DataFromLinkReqestCount]->BlockNo = DataFromLinkReqestCount;
        Ret = DataFromLinkReqestCount;
        DataFromLinkReqestCount++;
    }
    DataFromLinkReqests[Ret]->Used = 1;
    DataFromLinkReqests[Ret]->LinkNo = GetDataFromLinkChannels[par_DataFromLinkChannelNo]->LinkNo;
    DataFromLinkReqests[Ret]->IndexData = par_IndexData;
    DataFromLinkReqests[Ret]->FetchDataChannelNo = GetDataFromLinkChannels[par_DataFromLinkChannelNo]->FetchDataChannelNo;
    DataFromLinkReqests[Ret]->Callback = GetDataFromLinkChannels[par_DataFromLinkChannelNo]->Callback;
    DataFromLinkReqests[Ret]->Param = GetDataFromLinkChannels[par_DataFromLinkChannelNo]->Param;
    DataFromLinkReqests[Ret]->DimensionGroupNo = -1;
    DataFromLinkReqests[Ret]->DataGroupNo = -1;
    DataFromLinkReqests[Ret]->Dir = par_Dir;

    DataFromLinkReqests[Ret]->Next = StartDataFromLinkReqests;
    StartDataFromLinkReqests = DataFromLinkReqests[Ret];

    if (SleepFlag) {
        DEBUG_LOG0("WAKEUP from A2LGetDataFromLinkReq\n");
        SleepFlag = 0;
        LEAVE_CRITICAL_SECTION(&CriticalSection);
        WakeAllConditionVariable(&ConditionVariable);
    } else {
        LEAVE_CRITICAL_SECTION(&CriticalSection);
    }
    return Ret;
}

static void FreeFetchDataRequest(GET_DATA_FROM_LINK_REQ *par_Request)
{
    ENTER_CRITICAL_SECTION (&CriticalSection);
    DEBUG_LOG("FreeFetchDataRequest(%p)\n", par_Request);
    par_Request->Next = FreeDataFromLinkReqests;
    FreeDataFromLinkReqests = par_Request;
    LEAVE_CRITICAL_SECTION (&CriticalSection);
}

typedef struct {
    int OptParam;
    int DataGroupNo;
    int Succes;
    int Fill;
} CALLBACK_ACK;

CALLBACK_ACK *CallbackAck;
int CallbackAckSize;
int CallbackAckPos;

// this will becalled from DelayedFetchData.c
int FetchDataCallbackFunc (int par_OptParam, int par_DataGroupNo, int par_Success)
{
    ENTER_CRITICAL_SECTION (&CriticalSection);
    DEBUG_LOG("FetchDataCallbackFunc(%i, %i, %i)\n", par_OptParam, par_DataGroupNo, par_Success);
    if (CallbackAckPos >= CallbackAckSize) {
        CallbackAckSize += 16;
        CallbackAck = (CALLBACK_ACK*)my_realloc(CallbackAck, sizeof(CALLBACK_ACK) * CallbackAckSize);
        if (CallbackAck == NULL) {
            LEAVE_CRITICAL_SECTION (&CriticalSection);
            ThrowError(1, "out of memory");
            return -1;
        }
    }
    CallbackAck[CallbackAckPos].OptParam = par_OptParam;
    CallbackAck[CallbackAckPos].DataGroupNo = par_DataGroupNo;
    CallbackAck[CallbackAckPos].Succes = par_Success;
    CallbackAck[CallbackAckPos].Fill = 0;
    CallbackAckPos++;
    if (SleepFlag) {
        DEBUG_LOG0("WAKEUP from callback\n");
        SleepFlag = 0;
        LEAVE_CRITICAL_SECTION (&CriticalSection);
        WakeAllConditionVariable(&ConditionVariable);
    } else {
        LEAVE_CRITICAL_SECTION (&CriticalSection);
    }
    return 0;
}

// must be called inside CriticalSection
int ContinueFetchData (int par_OptParam, int par_DataGroupNo, int par_Success)
{
    DEBUG_LOG("ContinueFetchData(%i, %i, %i)\n", par_OptParam, par_DataGroupNo, par_Success);
    UNUSED(par_DataGroupNo);
    if ((par_OptParam >= 0) && (par_OptParam < DataFromLinkReqestCount)) {
        uint32_t x;
        int OneDataNotZero = 0;
        GET_DATA_FROM_LINK_REQ *r = DataFromLinkReqests[par_OptParam];
        LEAVE_CRITICAL_SECTION (&CriticalSection);
        switch(r->State) {
        case STATE_READ_DIMENSIONS_ACK_DATA_REQ:  // DIMENSIONS
            for (x = 0; x < r->IndexData->Size; x++) {
                if (r->IndexData->Data[x].Data != NULL) {
                    DEBUG_LOG("call A2LGetDataFromLinkX(%i, %i, %p, 0x%X, %i, %i, %i)\n",
                               r->LinkNo,
                               r->IndexData->Data[x].Index,
                               r->IndexData->Data[x].Data,
                               r->IndexData->Data[x].Flags,
                               r->State,
                               r->DimensionGroupNo,
                               r->DataGroupNo);
                    r->IndexData->Data[x].Data = A2LGetDataFromLinkState(r->LinkNo,
                                                                         r->IndexData->Data[x].Index,
                                                                         r->IndexData->Data[x].Data,
                                                                         r->IndexData->Data[x].Flags,
                                                                         r->State,
                                                                         r->DimensionGroupNo,
                                                                         r->DataGroupNo,
                                                                         &r->IndexData->Data[x].Error);
                }
                r->IndexData->Data[x].Success = r->IndexData->Data[x].Success && (r->IndexData->Data[x].Data != NULL);
                OneDataNotZero |= (r->IndexData->Data[x].Data != NULL);
            }
            r->Success = r->Success && par_Success;
            DEBUG_LOG("  OneDataNotZero = %i\n", OneDataNotZero);
            if (OneDataNotZero) {
                r->State = STATE_READ_DATA;
                InitFetchingData(r->FetchDataChannelNo, r->DataGroupNo, FetchDataCallbackFunc, r->BlockNo);
            } else {
                r->State = STATE_ERROR;
            }
            break;
        case STATE_READ_DATA:  // DATA
            for (x = 0; x < r->IndexData->Size; x++) {
                if (r->IndexData->Data[x].Data != NULL) {
                    // only for debuging
#ifdef WRITE_DEBUG_LOG
                    PrintBlockGroupToFile(Debug, r->DimensionGroupNo);
                    PrintBlockGroupToFile(Debug, r->DataGroupNo);
#endif
                    DEBUG_LOG("call A2LGetDataFromLinkX(%i, %i, %p, 0x%X, %i, %i, %i)\n",
                               r->LinkNo,
                               r->IndexData->Data[x].Index,
                               r->IndexData->Data[x].Data,
                               r->IndexData->Data[x].Flags,
                               r->State,
                               r->DimensionGroupNo,
                               r->DataGroupNo);
                    r->IndexData->Data[x].Data = A2LGetDataFromLinkState(r->LinkNo,
                                                                         r->IndexData->Data[x].Index,
                                                                         r->IndexData->Data[x].Data,
                                                                         r->IndexData->Data[x].Flags,
                                                                         r->State,
                                                                         r->DimensionGroupNo,
                                                                         r->DataGroupNo,
                                                                         &r->IndexData->Data[x].Error);
                }
                r->IndexData->Data[x].Success = r->IndexData->Data[x].Success && (r->IndexData->Data[x].Data != NULL);
                OneDataNotZero |= (r->IndexData->Data[x].Data != NULL);
            }
            DEBUG_LOG("  OneDataNotZero = %i\n", OneDataNotZero);
            FreeDataBlockGroup(r->DimensionGroupNo);
            FreeDataBlockGroup(r->DataGroupNo);
            r->Success = r->Success && par_Success;
            if (OneDataNotZero) {
                r->State = STATE_SUCCESSFUL;
            } else {
                r->State = STATE_ERROR;
            }
            r->Callback(r->IndexData, r->FetchDataChannelNo, r->Param);
            FreeFetchDataRequest(r);
            break;
        case STATE_READ_MASK_ACK_WRITE_DATA_REQ:
            for (x = 0; x < r->IndexData->Size; x++) {
                if (r->IndexData->Data[x].Data != NULL) {
                    DEBUG_LOG("call A2LSetDataToLinkX(%i, %i, %p, STATE_READ_DIMENSIONS_REQ, %i, %i)\n",
                               r->LinkNo,
                               r->IndexData->Data[x].Index,
                               r->IndexData->Data[x].Data,
                               r->DimensionGroupNo,
                               r->DataGroupNo);
                    int Ret = A2LSetDataToLinkState(r->LinkNo,
                                                    r->IndexData->Data[x].Index,
                                                    r->IndexData->Data[x].Data,
                                                    STATE_READ_MASK_ACK_WRITE_DATA_REQ, //STATE_READ_MASK_REQ,
                                                    r->DimensionGroupNo,
                                                    r->DataGroupNo,
                                                    &r->IndexData->Data[x].Error);
                    r->IndexData->Data[x].Success = r->IndexData->Data[x].Success && (Ret == 0);
                }
                OneDataNotZero |= (r->IndexData->Data[x].Data != NULL);
            }
            r->Success = r->Success && par_Success;
            DEBUG_LOG("  OneDataNotZero = %i\n", OneDataNotZero);
            if (OneDataNotZero) {
                r->State = STATE_WRITE_DATA;
                InitFetchingData(r->FetchDataChannelNo, r->DataGroupNo, FetchDataCallbackFunc, r->BlockNo);
            } else {
                r->State = STATE_ERROR;
                FreeDataBlockGroup(r->DimensionGroupNo);
                FreeDataBlockGroup(r->DataGroupNo);
                r->Callback(r->IndexData, r->FetchDataChannelNo, r->Param);
                FreeFetchDataRequest(r);
            }
            break;
        case STATE_WRITE_DATA:
            FreeDataBlockGroup(r->DimensionGroupNo);
            FreeDataBlockGroup(r->DataGroupNo);
            r->Success = r->Success && par_Success;
            r->State = STATE_SUCCESSFUL;
            r->Callback(r->IndexData, r->FetchDataChannelNo, r->Param);
            FreeFetchDataRequest(r);
            break;
        case STATE_SUCCESSFUL:
            break;
        default:
            ThrowError(1, "Internal error: wrong state");
            break;
        }
        ENTER_CRITICAL_SECTION (&CriticalSection);
    }
    return 0;
}

static int A2LLinkThreadActiveFlag;

#define UNUSED(x) (void)(x)

#ifdef _WIN32
static DWORD WINAPI A2LGetDataFromLinkThreadFunc (LPVOID lpParam)
#else
static void* A2LGetDataFromLinkThreadFunc (void* lpParam)
#endif
{
    UNUSED(lpParam);
    ENTER_CRITICAL_SECTION (&CriticalSection);
    while (A2LLinkThreadActiveFlag) {
        if ((CallbackAckPos == 0) && (StartDataFromLinkReqests == NULL)) {
            DEBUG_LOG0("GOTO SLEEP\n");
            SleepFlag = 1;
            SLEEP_CONDITIONAL_VARIABLE_CS(&ConditionVariable, &CriticalSection, INFINITE); // 100);
            DEBUG_LOG0("IS WAKEUP\n");
        } else {
            DEBUG_LOG0("DO NOT SLEEP\n");
        }
        if (CallbackAckPos > 0) {
            CallbackAckPos--;
            int OptParam = CallbackAck[CallbackAckPos].OptParam;
            int DataGroupNo = CallbackAck[CallbackAckPos].DataGroupNo;
            int Succes = CallbackAck[CallbackAckPos].Succes;
            ContinueFetchData(OptParam, DataGroupNo, Succes);
        } else {
            while (StartDataFromLinkReqests != NULL) {
                uint32_t x;
                int OneDataNotZero = 0;
                GET_DATA_FROM_LINK_REQ *r = StartDataFromLinkReqests;
                StartDataFromLinkReqests = StartDataFromLinkReqests->Next;
                LEAVE_CRITICAL_SECTION (&CriticalSection);
                r->DimensionGroupNo = GetDataBlockGroup(0);  // always read
                r->DataGroupNo = GetDataBlockGroup(r->Dir);
                r->Success = 1;  // begin with success and each action can reset for error reporting
                for (x = 0; x < r->IndexData->Size; x++) {
                    if (r->Dir) {
                        DEBUG_LOG("call A2LSetDataToLinkX(%i, %i, %p, STATE_READ_DIMENSIONS_REQ, %i, %i)\n",
                                   r->LinkNo,
                                   r->IndexData->Data[x].Index,
                                   r->IndexData->Data[x].Data,
                                   r->DimensionGroupNo,
                                   r->DataGroupNo);
                        int Ret = A2LSetDataToLinkState(r->LinkNo,
                                                    r->IndexData->Data[x].Index,
                                                    r->IndexData->Data[x].Data,
                                                    STATE_READ_MASK_REQ,
                                                    r->DimensionGroupNo,
                                                    r->DataGroupNo,
                                                    &r->IndexData->Data[x].Error);
                        r->IndexData->Data[x].Success = (Ret == 0);
                        OneDataNotZero |= (Ret == 0);
                    } else {
                        DEBUG_LOG("call A2LGetDataFromLinkX(%i, %i, %p, 0x%X, STATE_READ_DIMENSIONS_REQ, %i, %i)\n",
                                   r->LinkNo,
                                   r->IndexData->Data[x].Index,
                                   r->IndexData->Data[x].Data,
                                   r->IndexData->Data[x].Flags,
                                   r->DimensionGroupNo,
                                   r->DataGroupNo);
                        r->IndexData->Data[x].Data = A2LGetDataFromLinkState(r->LinkNo,
                                                      r->IndexData->Data[x].Index,
                                                      r->IndexData->Data[x].Data,
                                                      r->IndexData->Data[x].Flags,
                                                      STATE_READ_DIMENSIONS_REQ,
                                                      r->DimensionGroupNo,
                                                      r->DataGroupNo,
                                                      &r->IndexData->Data[x].Error);
                        r->IndexData->Data[x].Success = (r->IndexData->Data[x].Data != NULL);
                    }
                    OneDataNotZero |= (r->IndexData->Data[x].Data != NULL);
                }
                if (OneDataNotZero) {
                    if (IsDataBlockGroupEmpty(r->DimensionGroupNo)) {
                        // have no dimensions to read from start with the data request
                        for (x = 0; x < r->IndexData->Size; x++) {
                            if (r->Dir) {
                                DEBUG_LOG("call A2LSetDataToLinkX(%i, %i, %p, STATE_READ_DIMENSIONS_REQ, %i, %i)\n",
                                           r->LinkNo,
                                           r->IndexData->Data[x].Index,
                                           r->IndexData->Data[x].Data,
                                           r->DimensionGroupNo,
                                           r->DataGroupNo);
                                int Ret = A2LSetDataToLinkState(r->LinkNo,
                                                            r->IndexData->Data[x].Index,
                                                            r->IndexData->Data[x].Data,
                                                            STATE_READ_MASK_ACK_WRITE_DATA_REQ,
                                                            r->DimensionGroupNo,
                                                            r->DataGroupNo,
                                                            &r->IndexData->Data[x].Error);
                                r->IndexData->Data[x].Success = r->IndexData->Data[x].Success && (Ret == 0);
                                OneDataNotZero |= (Ret == 0);
                            } else {
                                DEBUG_LOG("call A2LGetDataFromLinkX(%i, %i, %p, 0x%X, STATE_READ_DIMENSIONS_ACK_DATA_REQ, %i, %i)\n",
                                           r->LinkNo,
                                           r->IndexData->Data[x].Index,
                                           r->IndexData->Data[x].Data,
                                           r->IndexData->Data[x].Flags,
                                           r->DimensionGroupNo,
                                           r->DataGroupNo);
                                r->IndexData->Data[x].Data = A2LGetDataFromLinkState(r->LinkNo,
                                                              r->IndexData->Data[x].Index,
                                                              r->IndexData->Data[x].Data,
                                                              r->IndexData->Data[x].Flags,
                                                              STATE_READ_DIMENSIONS_ACK_DATA_REQ,
                                                              r->DimensionGroupNo,
                                                              r->DataGroupNo,
                                                              &r->IndexData->Data[x].Error);
                                r->IndexData->Data[x].Success = r->IndexData->Data[x].Success && (r->IndexData->Data[x].Data != NULL);
                                OneDataNotZero |= (r->IndexData->Data[x].Data != NULL);
                            }
                        }
                        if (OneDataNotZero) {
                            if (r->Dir) {
                                r->State = STATE_WRITE_DATA;
                            } else {
                                r->State = STATE_READ_DATA;
                            }
                            InitFetchingData(r->FetchDataChannelNo, r->DataGroupNo, FetchDataCallbackFunc, r->BlockNo);
                        } else {
                             r->State = STATE_ERROR;
                             FreeDataBlockGroup(r->DimensionGroupNo);
                             FreeDataBlockGroup(r->DataGroupNo);
                             r->Callback(r->IndexData, r->FetchDataChannelNo, r->Param);
                             FreeFetchDataRequest(r);
                        }
                    } else if (OneDataNotZero) {
                        if (r->Dir) {
                            r->State = STATE_READ_MASK_ACK_WRITE_DATA_REQ;
                        } else {
                            r->State = STATE_READ_DIMENSIONS_ACK_DATA_REQ;
                        }
                        InitFetchingData(r->FetchDataChannelNo, r->DimensionGroupNo, FetchDataCallbackFunc, r->BlockNo);
                    } else {
                        r->State = STATE_ERROR;
                        FreeDataBlockGroup(r->DimensionGroupNo);
                        FreeDataBlockGroup(r->DataGroupNo);
                        r->Callback(r->IndexData, r->FetchDataChannelNo, r->Param);
                        FreeFetchDataRequest(r);
                    }
                } else {
                    r->State = STATE_ERROR;
                    FreeDataBlockGroup(r->DimensionGroupNo);
                    FreeDataBlockGroup(r->DataGroupNo);
                    r->Callback(r->IndexData, r->FetchDataChannelNo, r->Param);
                    FreeFetchDataRequest(r);
                }
                ENTER_CRITICAL_SECTION (&CriticalSection);
            }
        }
    }
    A2LLinkThreadActiveFlag = 2; // is now stopped
    LEAVE_CRITICAL_SECTION (&CriticalSection);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}


void InitA2lLinkThread(void)
{
#ifdef _WIN32
    DWORD ThreadId;
#else
    pthread_t Thread;
    pthread_attr_t Attr;
#endif
#ifdef WRITE_DEBUG_LOG
    Debug = fopen ("c:\\temp\\A2LLinkThread.txt", "wt");
    DEBUG_LOG0("Start debug:\n");
#endif
    InitializeCriticalSection (&CriticalSection);
    InitializeConditionVariable(&ConditionVariable);

    A2LLinkThreadActiveFlag = 1;
#ifdef _WIN32
    CreateThread (NULL,              // no security attribute
                  0,                 // default stack size
                  A2LGetDataFromLinkThreadFunc,    // thread proc
                  NULL,    // thread parameter
                  0,                 // not suspended
                  &ThreadId);      // returns thread ID
#else
    pthread_attr_init(&Attr);
    if (pthread_create(&Thread, &Attr, A2LGetDataFromLinkThreadFunc, NULL) != 0) {
        ThrowError(1, "cannot create thread\n");
    }
    pthread_attr_destroy(&Attr);
#endif
}

void TerminateA2lLinkThread(void)
{
    int x;
    A2LLinkThreadActiveFlag = 0;
    WakeAllConditionVariable(&ConditionVariable);
    for (x= 0; (x < 100) && (A2LLinkThreadActiveFlag == 0); x++) {
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10*1000);
#endif
    }
    ENTER_CRITICAL_SECTION (&CriticalSection);
    while (FreeGetDataFromLinkChannels != NULL) {
        GET_DATA_FROM_LINK_CHANNEL *Next = FreeGetDataFromLinkChannels->Next;
        my_free(FreeGetDataFromLinkChannels);
        FreeGetDataFromLinkChannels = Next;
    }
    if (GetDataFromLinkChannels != NULL) my_free(GetDataFromLinkChannels);
    GetDataFromLinkChannels = NULL;
    GetDataFromLinkChannelCount = 0;
    GetDataFromLinkChannelSize = 0;

    while (FreeDataFromLinkReqests != NULL) {
        GET_DATA_FROM_LINK_REQ *Next = FreeDataFromLinkReqests->Next;
        my_free(FreeDataFromLinkReqests);
        FreeDataFromLinkReqests = Next;
    }
    if(DataFromLinkReqests != NULL) my_free(DataFromLinkReqests);
    DataFromLinkReqests = NULL;
    DataFromLinkReqestCount = 0;
    DataFromLinkReqestSize = 0;

    while (FreeLinkDataBlocks != NULL) {
        INDEX_DATA_BLOCK *Next = FreeLinkDataBlocks->Next;
        my_free(FreeLinkDataBlocks->Data);
        my_free(FreeLinkDataBlocks);
        FreeLinkDataBlocks = Next;
    }

    if (CallbackAck) my_free(CallbackAck);
    CallbackAckSize = 0;
    CallbackAckPos = 0;

    LEAVE_CRITICAL_SECTION (&CriticalSection);
}

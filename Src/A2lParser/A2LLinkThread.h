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


#ifndef A2LLINKTHREAD_H
#define A2LLINKTHREAD_H

#include <stdint.h>

typedef struct INDEX_DATA_BLOCK_ELEM_S {
    int Index;
    int Flags;  // Phys, ...
    void *Data;
    const char *Error;
    uint64_t User;
    int Success;
} INDEX_DATA_BLOCK_ELEM;

typedef struct INDEX_DATA_BLOCK_S {
    uint32_t Size;
    uint32_t MaxSize;
    uint32_t CurrentPos;
    INDEX_DATA_BLOCK_ELEM *Data;
    struct INDEX_DATA_BLOCK_S *Next;
} INDEX_DATA_BLOCK;

INDEX_DATA_BLOCK *GetIndexDataBlock(int par_Size);
void FreeIndexDataBlock(INDEX_DATA_BLOCK *par_LinkDataBlocks);


typedef int (__cdecl* A2LGetDataFromLinkAckCallback)(void *parLinkData, int par_FetchDataChannelNo, void *par_Param);

int NewDataFromLinkChannel(int par_LinkNo, uint64_t par_Timeout,
                           A2LGetDataFromLinkAckCallback par_Callback, void *par_Param);

int DeleteDataFromLinkChannel(int par_DataFromLinkChannelNo);

int A2LGetDataFromLinkReq(int par_DataFromLinkChannelNo, int par_Dir, INDEX_DATA_BLOCK *par_IndexData);

void InitA2lLinkThread(void);
void TerminateA2lLinkThread(void);



/* first call
 * m_ChannelNo = NewDataFromLinkChannel()
 * by each update call:
 *  IndexData = GetFreeIndexDataBlock(elements);
 *  fill all elements of IndexData
 *
 * callback
 *
 *  call A2LGetDataFromLinkReq(m_ChannelNo, IndexData);
 *  call FreeIndexDataBlock(IndexData);
 * DeleteDataFromLinkChannel(m_ChannelNo);
 */
#endif

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


#ifndef REMOTEMASTERNET_H
#define REMOTEMASTERNET_H

#include <stdint.h>
#include<stdio.h>
#ifdef _WIN32
//#include<winsock2.h>
#include "basetsd.h"
#define MY_SOCKET UINT_PTR
#else
#define MY_SOCKET int
#endif

#define MAX_SOCKET_THREAD_CONNECTIONS  16

typedef struct {
    uint16_t Number;
    uint16_t PackageCounter;
    uint32_t ThreadId;
    MY_SOCKET Socket;
    uint32_t CreateCounter;
} SOCKET_THREAD_ELEMENT;

int InitRemoteMaster(char *par_RemoteMasterAddr, int par_RemoteMasterPort);
int SentToRemoteMaster (void *par_Data, int par_Command, int par_len);
//int ReceiveFromeRemoteMaster (void *ret_Data, int par_len);

int TransmitToRemoteMaster (int par_Command, void *par_Data, int par_len);
int TransactRemoteMaster (int par_Command, void *par_DataToRM, int par_LenDataToRM, void *ret_DataFromRM, int par_LenDataFromRM);

//int ReceiveFromeRemoteMasterDynBuf (SOCKET_THREAD_ELEMENT *par_SocketForThread, void *ret_Data, int par_len, void **ret_ptrToData, int *ret_len);
int TransactRemoteMasterDynBuf (int par_Command, void *par_DataToRM, int par_LenDataToRM, void *ret_DataFromRM, int par_LenDataFromRM, void **ret_ptrDataFromRM, int *ret_LenDataFromRM);
void RemoteMasterFreeDynBuf (void *par_buffer);

MY_SOCKET RemoteMasterGetSocket(void);

void CloseAllOtherSocketsToRemoteMaster(void);

// for dialog
int RemoteMasterGetRowCount(void);
int RemoteMasterGetNextCallToStatistic(int par_Index, uint64_t *ret_CallCounter,
                                       uint64_t *ret_LastTime,
                                       uint64_t *ret_AccumulatedTime,
                                       uint64_t *ret_MaxTime,
                                       uint64_t *ret_MinTime);
void ResetRemoteMasterCallStatistics(void);

// this function should be called by a thread before it will be terminated
void CloseSocketToRemoteMasterForThisThead(void);

#endif

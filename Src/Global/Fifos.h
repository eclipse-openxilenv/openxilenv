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


#ifndef FIFOS_H
#define FIFOS_H

#include <stdint.h>

#define FIFO_ERR_OUT_OF_MEMORY     -1101
#define FIFO_ERR_INVALID_PARAMETER -1102
#define FIFO_ERR_UNKNOWN_HANDLE    -1103
#define FIFO_ERR_WRONG_THREAD      -1104
#define FIFO_ERR_NOT_ENOUGH_SPACE  -1105
#define FIFO_ERR_MESSAGE_TO_BIG    -1106
#define FIFO_ERR_NO_DATA           -1107
#define FIFO_ERR_NO_FREE_FIFO      -1108
#define FIFO_ERR_UNKNOWN_NAME      -1109


#define SYNC_FIFO_SIZE   (8*1024*1024)


typedef struct {
	union {
		struct {
			uint8_t Flag;
			uint8_t Fill1;
			uint8_t Fill2;
			uint8_t Fill3;
		} Used;
		uint32_t Flags;
	} Used;
    int32_t TramsmiterPid;
    uint32_t MessageId;
    uint64_t Timestamp;
    int32_t Size;      // Number of daten bytes without header
    int32_t FiFoId;    // This is importand for fifo 0, he is mediating between remote master and XilEnv
} FIFO_ENTRY_HEADER;


int InitFiFos(void);
void TerminateFifos(void);

int CreateNewRxFifo (int RxPid, int Size, const char *Name);  // Return is FiFo handle
int TxAttachFiFo(int TxPid, const char *Name);  // Return is FiFo handle
int WriteToFiFo (int32_t FiFoId, uint32_t MessageId, int TramsmiterPid, uint64_t Timestamp, int Len, const void *Data);
int ReadFromFiFo (int FiFoId, FIFO_ENTRY_HEADER *Header, void *Data, int MaxLen);
int CheckFiFo (int FiFoId, FIFO_ENTRY_HEADER *Header);
int RemoveOneMessageFromFiFo (int FiFoId);
int DeleteFiFo(int FiFoId, int RxPid);

// This is only neccessary for remote master
// Each time a network package is reseved from the remote master this must be called
int SyncRxFiFos (void *Buffer, int Len);

// This must be call cyclic
int SyncTxFiFos (void *Buffer, int Len, int *DataToTransmit);
// Return:
//     1 -> new data to transmit to remote master
//     2 -> new data to transmit to remote master but buffer was to small

#ifdef REMOTE_MASTER
int RegisterNewRxFiFoName(int RxPid, char *Name);
int UnRegisterRxFiFo(int FiFoId, int RxPid);
#endif

#endif // FIFOS_H

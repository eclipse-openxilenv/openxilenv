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
#include <stdio.h>
#include <string.h>

#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#ifndef REMOTE_MASTER
#include "MainValues.h"
#include "RemoteMasterFiFo.h"
#endif
#include "Scheduler.h"
#include "Message.h"
#include "Fifos.h"

#define UNUSED(x) (void)(x)

/*
#ifdef REMOTE_MASTER
#define FIFO_LOG_TO_FILE  "/tmp/fifo.txt"
#else
#define FIFO_LOG_TO_FILE  "c:\\temp\\fifo.txt"
#endif
*/

/*#ifdef FIFO_LOG_TO_FILE
#include "files.h"
#endif*/


#ifdef REMOTE_MASTER

#include "RemoteMasterLock.h"

static REMOTE_MASTER_LOCK FiFosCriticalSection;
#define InitializeCriticalSection(x) RemoteMasterInitLock(x)
#define DeleteCriticalSection(x) RemoteMasterDestroyLock(x)
#define EnterCriticalSection(x)  RemoteMasterLock(x, __LINE__, __FILE__)
#define LeaveCriticalSection(x)  RemoteMasterUnlock(x)

#else

static CRITICAL_SECTION FiFosCriticalSection;

#endif

// Locking should be active otherwise we have problems during startup
#if 1
#define XEnterCriticalSection(x)  EnterCriticalSection(x)
#define XLeaveCriticalSection(x)  LeaveCriticalSection(x)
#else
#define XEnterCriticalSection(x)  
#define XLeaveCriticalSection(x)  
#endif


typedef struct {
    uint32_t IsOnline; 
    int32_t FiFoId;
    int32_t Size;
    int32_t RxPid;
    void *Data;
    void *Wr;
    void *Rd;
	uint64_t DeleteTimeStamp;
    char Name[64];
	uint32_t TxCounter32;
	uint32_t RxCounter32;
	uint8_t TxCounter;
	uint8_t RxCounter;
	uint8_t Fill1;
	uint8_t Fill2;
	uint8_t Fill3;
	uint8_t Fill4;
	uint8_t Fill5;
	uint8_t Fill6;
} FIFO;   // 4 * 4 + 4 * 8 + 64 + 2 * 4 + 8 * 1 = 128Bytes


#define MAX_FIFOS   128

static int32_t FiFoCounter;
static FIFO Fifos[MAX_FIFOS];

#ifdef FIFO_LOG_TO_FILE
static FILE *FifoLogFh;

void DebugPrintHeader(char *Prefix, FIFO_ENTRY_HEADER *Header)
{
    fprintf (FifoLogFh, "%sMessage Header:\n", Prefix);
    //fprintf (FifoLogFh, "%sUsed.Flag = %u\n", Prefix, (int)Header->Used.Flag);
    fprintf (FifoLogFh, "%sTramsmiterPid = %u\n", Prefix, Header->TramsmiterPid);
    if ((Header->MessageId & MESSAGE_THROUGH_FIFO_MASK) == MESSAGE_THROUGH_FIFO_MASK) {
        fprintf (FifoLogFh, "%sMessageId(MQ) = %u\n", Prefix, Header->MessageId & 0x0FFFFFFF);
    } else {
        fprintf (FifoLogFh, "%sMessageId = %u\n", Prefix, Header->MessageId);
    }
    fprintf (FifoLogFh, "%sTimestamp = %" PRIu64 "\n", Prefix, Header->Timestamp);
    fprintf (FifoLogFh, "%sSize = %u\n", Prefix, Header->Size);
    fprintf (FifoLogFh, "%sFiFoId = %u\n", Prefix, Header->FiFoId);
}
#endif

int CreateNewRxFifo (int RxPid, int Size, const char *Name)   // if Size == 0 it only register the fifo name
{
    int32_t x;

#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "CreateNewRxFifo(RxPid = %i, Size = %i, Name = \"%s\")\n", RxPid, Size, Name);
#endif

#ifndef REMOTE_MASTER
    if ((!s_main_ini_val.ConnectToRemoteMaster) || (RxPid == 0)) {
#endif
        EnterCriticalSection (&FiFosCriticalSection);
        for (x = 0; x < MAX_FIFOS; x++) {
            if (Fifos[x].FiFoId == 0) {  // free fifo
                if (FiFoCounter < 0x7FFF) FiFoCounter++;
                else FiFoCounter = 1;
                Fifos[x].FiFoId = (FiFoCounter << 16) + x;
                break;
            }
        }
        LeaveCriticalSection (&FiFosCriticalSection);

        if (x == MAX_FIFOS) return FIFO_ERR_NO_FREE_FIFO;
#ifndef REMOTE_MASTER
    } else {
        int FiFoId;
        FiFoId = rm_RegisterNewRxFiFoName(RxPid, Name);
        if (FiFoId < 0) return FIFO_ERR_NO_FREE_FIFO;
        x =  FiFoId & 0xFFFF;
        Fifos[x].FiFoId = FiFoId;
    }
#endif
#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "    FiFoId[%i] = 0x%X\n", x, Fifos[x].FiFoId);
#endif
    Fifos[x].Size = Size;

    Fifos[x].TxCounter32 = 0;
    Fifos[x].RxCounter32 = 0;
    Fifos[x].TxCounter = 1;  // range is 1...255
	Fifos[x].RxCounter = 1;

    StringCopyMaxCharTruncate(Fifos[x].Name, Name, sizeof (Fifos[x].Name) - 1);
    Fifos[x].RxPid = RxPid;
	if (Size > 0) {
        Fifos[x].Data = my_calloc((size_t)Size, 1);
		Fifos[x].Wr = Fifos[x].Data;
		Fifos[x].Rd = Fifos[x].Data;
        if (Fifos[x].Data == NULL) {
			Fifos[x].FiFoId = 0;
			ThrowError(1, "cannot create FiFo \"%s\" for processes \"%i\" (out of memory)", Name, RxPid);
			return FIFO_ERR_OUT_OF_MEMORY;
		}
	} else {
		Fifos[x].Data = NULL;
		Fifos[x].Wr = NULL;
		Fifos[x].Rd = NULL;
	}
    EnterCriticalSection(&FiFosCriticalSection);
    Fifos[x].IsOnline = 1;
    LeaveCriticalSection(&FiFosCriticalSection);
    return Fifos[x].FiFoId; // Return is fifo Handle
}

int TxAttachFiFo(int TxPid, const char *Name)  // Return is fifo Handle
{
#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "TxAttachFiFo(TxPid = %i, Name = \"%s\")\n", TxPid, Name);
#endif

#ifndef REMOTE_MASTER
    if (s_main_ini_val.ConnectToRemoteMaster) {
        return rm_TxAttachFiFo (TxPid, Name);  // If remote master is active use he will aktive then he will administer all fifo namen
    } else {
#endif
        int x;
        for (x = 0; x < MAX_FIFOS; x++) {
            if (Fifos[x].IsOnline) {
                if (Fifos[x].FiFoId != 0) {
                    if (!strcmp(Fifos[x].Name, Name)) {
#ifdef FIFO_LOG_TO_FILE
                        fprintf(FifoLogFh, "    Fifos[%i].FiFoId = 0x%X\n", x, Fifos[x].FiFoId);
#endif
                        return  Fifos[x].FiFoId;
                    }
                }
            }
        }
#ifndef REMOTE_MASTER
    }
#endif
#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "    not found\n");
#endif
    return FIFO_ERR_UNKNOWN_NAME;
}

int WriteToFiFo (int32_t FiFoId, uint32_t MessageId, int TramsmiterPid, uint64_t Timestamp, int Len, const void *Data)
{
    int FiFoNr;

#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "WriteToFiFo(FiFoId = %i, MessageId = %i, TramsmiterPid = %i, Timestamp = %" PRIu64 ", len = %i, Data)\n",
             FiFoId, MessageId, TramsmiterPid, Timestamp, Len);
#endif

#ifdef REMOTE_MASTER
    FiFoNr = 0;     // immer FiFo 0 zum Senden verwenden
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
        FiFoNr = 0;     // Use always fifo 0 to transmit
    } else {
        FiFoNr = FiFoId & 0xFFFF;
        if ((FiFoNr < 0) || (FiFoNr >= MAX_FIFOS)) return FIFO_ERR_INVALID_PARAMETER;
        if (Fifos[FiFoNr].FiFoId != FiFoId) return FIFO_ERR_UNKNOWN_HANDLE;
    }
#endif
    if (1) {
        FIFO_ENTRY_HEADER Header;
        int Space;
        int parta, partb;
        char *Dest;
        char *Src;
        unsigned char *Used;

        XEnterCriticalSection(&FiFosCriticalSection);
        if (Fifos[FiFoNr].Rd > Fifos[FiFoNr].Wr) {
            Space = (int)((char*)(Fifos[FiFoNr].Rd) - (char*)(Fifos[FiFoNr].Wr));
        } else {
            Space = Fifos[FiFoNr].Size - (int)((char*)(Fifos[FiFoNr].Wr) - (char*)(Fifos[FiFoNr].Rd));
        }

        // 2 bytes more than the size of the message is long  (for the next used byte)!
        if (Space < ((int)sizeof(FIFO_ENTRY_HEADER) + Len + 2)) {
#ifdef FIFO_LOG_TO_FILE
            fprintf (FifoLogFh, "    error FIFO_ERR_NOT_ENOUGH_SPACE\n");
#endif
            XLeaveCriticalSection(&FiFosCriticalSection);
            return FIFO_ERR_NOT_ENOUGH_SPACE;
        }
        Used = (unsigned char*)(Fifos[FiFoNr].Wr);

        // Construct the header
		Header.Used.Flags = 0;
        Header.MessageId = MessageId;
        Header.Timestamp = Timestamp;
        Header.Size = Len;
        Header.TramsmiterPid = TramsmiterPid;
        Header.FiFoId = FiFoId;
        // first the message head
        Dest = (char*)Fifos[FiFoNr].Wr;
        Src = (char*)&Header;
        parta = (int)(Fifos[FiFoNr].Size - (int)(Dest - (char*)Fifos[FiFoNr].Data));
        if (parta >= (int)sizeof(FIFO_ENTRY_HEADER)) {
            MEMCPY(Dest, Src, sizeof(FIFO_ENTRY_HEADER));
            Dest += sizeof(FIFO_ENTRY_HEADER);
        } else {
#ifdef FIFO_LOG_TO_FILE
            fprintf (FifoLogFh, "    !!!!!!!!!!!SPLIT COPY!!!!!!!!\n");
#endif
            partb = (int)sizeof(FIFO_ENTRY_HEADER) - parta;
            MEMCPY(Dest, Src, (size_t)parta);
            MEMCPY(Fifos[FiFoNr].Data, &Src[parta], (size_t)partb);
            Dest = (char*)Fifos[FiFoNr].Data + partb;
        }

        // than the data
        parta = (int)(Fifos[FiFoNr].Size - (Dest - (char*)Fifos[FiFoNr].Data));
        if (parta >= Len) {
            MEMCPY(Dest, Data, (size_t)Len);
            Dest += Len;
        }  else {
#ifdef FIFO_LOG_TO_FILE
            fprintf (FifoLogFh, "    !!!!!!!!!!!SPLIT COPY!!!!!!!!\n");
#endif
            partb = Len - parta;
            MEMCPY(Dest, Data, (size_t)parta);
            MEMCPY(Fifos[FiFoNr].Data, (const char*)Data + parta, (size_t)partb);
            Dest = (char*)Fifos[FiFoNr].Data + partb;
        }
        if ((Dest - (char*)Fifos[FiFoNr].Data) >= Fifos[FiFoNr].Size) {
            Dest = (char*)Fifos[FiFoNr].Data;
        }
        Fifos[FiFoNr].Wr = (void*)Dest;
        *(char*)(Fifos[FiFoNr].Wr) = 0;

		Fifos[FiFoNr].TxCounter32++;
		if (Fifos[FiFoNr].TxCounter == 255) Fifos[FiFoNr].TxCounter = 1;
		else Fifos[FiFoNr].TxCounter++;
        // Now set the fifo entry to valid
		*Used = Fifos[FiFoNr].TxCounter;
        XLeaveCriticalSection(&FiFosCriticalSection);
        return 0;
    }
}

static int WriteToFiFoDistribute(int32_t FiFoId, uint32_t MessageId, int32_t TramsmiterPid, unsigned long long Timestamp, int Len, void *Data)
{
	int FiFoNr;
	FIFO *FiFoDebug;

#ifdef FIFO_LOG_TO_FILE
	fprintf(FifoLogFh, "WriteToFiFoDistribute(FiFoId = %i, MessageId = %i, TramsmiterPid = %i, Timestamp = %" PRIu64 ", len = %i, Data)\n",
		FiFoId, MessageId, TramsmiterPid, Timestamp, Len);
#endif

	FiFoNr = FiFoId & 0xFFFF;
    if(FiFoNr == 0) {
        ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
    }
	if ((FiFoNr < 0) || (FiFoNr >= MAX_FIFOS)) return FIFO_ERR_INVALID_PARAMETER;
	FiFoDebug = &(Fifos[FiFoNr]);
	if (Fifos[FiFoNr].FiFoId != FiFoId) return FIFO_ERR_UNKNOWN_HANDLE;
	if (1) {
		FIFO_ENTRY_HEADER Header;
		int Space;
		int parta, partb;
		char *Dest;
		char *Src;
		unsigned char *Used;

        XEnterCriticalSection(&FiFosCriticalSection);

		if (Fifos[FiFoNr].Rd > Fifos[FiFoNr].Wr) {
			Space = (int)((char*)(Fifos[FiFoNr].Rd) - (char*)(Fifos[FiFoNr].Wr));
		}
		else {
			Space = Fifos[FiFoNr].Size - (int)((char*)(Fifos[FiFoNr].Wr) - (char*)(Fifos[FiFoNr].Rd));
		}

        // 2 bytes more than the size of the message is long  (for the next used byte)!
        if (Space < ((int)sizeof(FIFO_ENTRY_HEADER) + Len + 2)) {
#ifdef FIFO_LOG_TO_FILE
			fprintf(FifoLogFh, "    error FIFO_ERR_NOT_ENOUGH_SPACE\n");
#endif
            XLeaveCriticalSection(&FiFosCriticalSection);
			return FIFO_ERR_NOT_ENOUGH_SPACE;
		}
        Used = (unsigned char*)(Fifos[FiFoNr].Wr);

        // Construct the header
        Header.Used.Flags = 0;
		Header.MessageId = MessageId;
		Header.Timestamp = Timestamp;
		Header.Size = Len;
		Header.TramsmiterPid = TramsmiterPid;
		Header.FiFoId = FiFoId;
        // first the message head
        Dest = (char*)Fifos[FiFoNr].Wr;
		Src = (char*)&Header;
		parta = (int)(Fifos[FiFoNr].Size - (Dest - (char*)Fifos[FiFoNr].Data));
        if (parta >= (int)sizeof(FIFO_ENTRY_HEADER)) {
			MEMCPY(Dest, Src, sizeof(FIFO_ENTRY_HEADER));
			Dest += sizeof(FIFO_ENTRY_HEADER);
		}
		else {
#ifdef FIFO_LOG_TO_FILE
			fprintf(FifoLogFh, "    !!!!!!!!!!!SPLIT COPY!!!!!!!!\n");
#endif
            partb = (int)sizeof(FIFO_ENTRY_HEADER) - parta;
            MEMCPY(Dest, Src, (size_t)parta);
            MEMCPY(Fifos[FiFoNr].Data, &Src[parta], (size_t)partb);
			Dest = (char*)Fifos[FiFoNr].Data + partb;
		}

        // than the data
        parta = (int)(Fifos[FiFoNr].Size - (Dest - (char*)Fifos[FiFoNr].Data));
		if (parta >= Len) {
            MEMCPY(Dest, Data, (size_t)Len);
			Dest += Len;
		}
		else {
#ifdef FIFO_LOG_TO_FILE
			fprintf(FifoLogFh, "    !!!!!!!!!!!SPLIT COPY!!!!!!!!\n");
#endif
			partb = Len - parta;
            MEMCPY(Dest, Data, (size_t)parta);
            MEMCPY(Fifos[FiFoNr].Data, (char*)Data + parta, (size_t)partb);
			Dest = (char*)Fifos[FiFoNr].Data + partb;
		}
		if ((Dest - (char*)Fifos[FiFoNr].Data) >= Fifos[FiFoNr].Size) {
			Dest = (char*)Fifos[FiFoNr].Data;
		}
		Fifos[FiFoNr].Wr = (void*)Dest;
		*(char*)(Fifos[FiFoNr].Wr) = 0;
		Fifos[FiFoNr].TxCounter32++;
		if (Fifos[FiFoNr].TxCounter == 255) Fifos[FiFoNr].TxCounter = 1;
        else Fifos[FiFoNr].TxCounter++;
        // Now set the fifo entry to valid
        *Used = Fifos[FiFoNr].TxCounter;
        XLeaveCriticalSection(&FiFosCriticalSection);
		return 0;
	}
}

int ReadFromFiFo (int FiFoId, FIFO_ENTRY_HEADER *Header, void *Data, int MaxLen)
{
    int FiFoNr;
    FIFO *FiFoDebug;
	unsigned char UsedCounter;


#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "ReadFromFiFo(FiFoId = %i, Header,Data, MaxLen = %i)\n",
             FiFoId, MaxLen);
#endif
    FiFoNr = FiFoId & 0xFFFF;
    if ((FiFoNr < 0) || (FiFoNr >= MAX_FIFOS)) return FIFO_ERR_INVALID_PARAMETER;
    FiFoDebug = &(Fifos[FiFoNr]);
    if (Fifos[FiFoNr].FiFoId != FiFoId) return FIFO_ERR_UNKNOWN_HANDLE;
    XEnterCriticalSection(&FiFosCriticalSection);
    UsedCounter = *(unsigned char*)Fifos[FiFoNr].Rd;
    if (UsedCounter) {    // Is used?
        int parta, partb;
        char *Dest;
        char *Src;
        int Len;

        // Copy message head
        Dest = (char*)Header;
        Src = (char*)Fifos[FiFoNr].Rd;

        parta = (int)(Fifos[FiFoNr].Size - (Src - (char*)Fifos[FiFoNr].Data));
        if (parta >= (int)sizeof (FIFO_ENTRY_HEADER)) {
            MEMCPY (Dest, Src, sizeof (FIFO_ENTRY_HEADER));
            Src += sizeof (FIFO_ENTRY_HEADER);
        } else {
#ifdef FIFO_LOG_TO_FILE
            fprintf (FifoLogFh, "    !!!!!!!!!!!SPLIT COPY!!!!!!!!\n");
#endif
            partb = (int)sizeof(FIFO_ENTRY_HEADER) - parta;
            MEMCPY (Dest, Src, (size_t)parta);
            MEMCPY (&Dest[parta], Fifos[FiFoNr].Data, (size_t)partb);
            Src = (char*)Fifos[FiFoNr].Data + partb;
        }
        Len = Header->Size;
        if ((Len >= 0) && (Len <= MaxLen)) {
            // Than the data
            Dest = (char*)Data;
            parta = (int)(Fifos[FiFoNr].Size - (Src - (char*)Fifos[FiFoNr].Data));
            if (parta >= Len) {
                MEMCPY(Dest, Src, (size_t)Len);
                Src += Len;
            }  else {
#ifdef FIFO_LOG_TO_FILE
            fprintf (FifoLogFh, "    !!!!!!!!!!!SPLIT COPY!!!!!!!!\n");
#endif
                partb = Len - parta;
                MEMCPY(Dest, Src, (size_t)parta);
                MEMCPY(Dest + parta, Fifos[FiFoNr].Data, (size_t)partb);
                Src = (char*)Fifos[FiFoNr].Data + partb;
            }
            if ((Src - (char*)Fifos[FiFoNr].Data) >= Fifos[FiFoNr].Size) {
                Src = (char*)Fifos[FiFoNr].Data;
            }
            Fifos[FiFoNr].Rd = Src;
#ifdef FIFO_LOG_TO_FILE
            DebugPrintHeader("    ", Header);
#endif
            Fifos[FiFoNr].RxCounter32++;
            if (Fifos[FiFoNr].RxCounter == 255) Fifos[FiFoNr].RxCounter = 1;
            else Fifos[FiFoNr].RxCounter++;
            if (UsedCounter != Fifos[FiFoNr].RxCounter) {
                ThrowError(1, " internal error inside fifos %s(%i): UsedCounter=0x%02X != Fifos[FiFoNr].RxCounter=0x%02X", __FILE__, __LINE__, (int)UsedCounter, (int)Fifos[FiFoNr].RxCounter);
            }
            XLeaveCriticalSection(&FiFosCriticalSection);

            return 0;
        }
#ifdef FIFO_LOG_TO_FILE
        fprintf (FifoLogFh, "    error FIFO_ERR_MESSAGE_TO_BIG\n");
#endif
        XLeaveCriticalSection(&FiFosCriticalSection);
        return FIFO_ERR_MESSAGE_TO_BIG;
    }
#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "    error FIFO_ERR_NO_DATA\n");
#endif
    XLeaveCriticalSection(&FiFosCriticalSection);
    return FIFO_ERR_NO_DATA;
}


int CheckFiFo (int FiFoId, FIFO_ENTRY_HEADER *Header)
{
    int FiFoNr;
    FIFO *FiFoDebug;
	unsigned char UsedCounter;

#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "CheckFiFo(FiFoId = %i, Header)\n",
             FiFoId);
#endif
    FiFoNr = FiFoId & 0xFFFF;
    if ((FiFoNr < 0) || (FiFoNr >= MAX_FIFOS)) return FIFO_ERR_INVALID_PARAMETER;
    FiFoDebug = &(Fifos[FiFoNr]);
    if (Fifos[FiFoNr].FiFoId != FiFoId) return FIFO_ERR_UNKNOWN_HANDLE;
    XEnterCriticalSection(&FiFosCriticalSection);
    UsedCounter = *(unsigned char*)Fifos[FiFoNr].Rd;
    if (UsedCounter) {    // Is Used?
		int parta, partb;
        char *Dest;
        char *Src;

        // Copy message head
        Dest = (char*)Header;
        Src = (char*)Fifos[FiFoNr].Rd;
        if ((Src - (char*)Fifos[FiFoNr].Data) >= Fifos[FiFoNr].Size) {
            Src = (char*)Fifos[FiFoNr].Data;
        }

        parta = (int)(Fifos[FiFoNr].Size - (Src - (char*)Fifos[FiFoNr].Data));
        if (parta >= (int)sizeof (FIFO_ENTRY_HEADER)) {
            MEMCPY (Dest, Src, sizeof (FIFO_ENTRY_HEADER));
        } else {
#ifdef FIFO_LOG_TO_FILE
            fprintf (FifoLogFh, "    !!!!!!!!!!!SPLIT COPY!!!!!!!!\n");
#endif
            partb = (int)sizeof(FIFO_ENTRY_HEADER) - parta;
            MEMCPY (Dest, Src, (size_t)parta);
            MEMCPY (&Dest[parta], Fifos[FiFoNr].Data, (size_t)partb);
        }
#ifdef FIFO_LOG_TO_FILE
        DebugPrintHeader("    ", Header);
#endif
        XLeaveCriticalSection(&FiFosCriticalSection);
        return 1;
    }
#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "    error FIFO_ERR_NO_DATA\n");
#endif
    XLeaveCriticalSection(&FiFosCriticalSection);
    return FIFO_ERR_NO_DATA;
}

int RemoveOneMessageFromFiFo (int FiFoId)
{
    int FiFoNr;
    FIFO_ENTRY_HEADER Header;
	unsigned char UsedCounter;


#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "RemoveOneMessageFromFiFo(FiFoId = %i)\n",
             FiFoId);
#endif
    FiFoNr = FiFoId & 0xFFFF;
    if ((FiFoNr < 0) || (FiFoNr >= MAX_FIFOS)) return FIFO_ERR_INVALID_PARAMETER;
    if (Fifos[FiFoNr].FiFoId != FiFoId) return FIFO_ERR_UNKNOWN_HANDLE;
    XEnterCriticalSection(&FiFosCriticalSection);
    UsedCounter = *(unsigned char*)Fifos[FiFoNr].Rd;
    if (UsedCounter) {    // Is used?
		int parta, partb;
        char *Dest;
        char *Src;
        int Len;

        // Copy message head
        Dest = (char*)&Header;
        Src = (char*)Fifos[FiFoNr].Rd;

        parta = (int)(Fifos[FiFoNr].Size - (Src - (char*)Fifos[FiFoNr].Data));
        if (parta >= (int)sizeof (FIFO_ENTRY_HEADER)) {
            MEMCPY (Dest, Src, sizeof (FIFO_ENTRY_HEADER));
            Src += sizeof (FIFO_ENTRY_HEADER);
        } else {
#ifdef FIFO_LOG_TO_FILE
            fprintf (FifoLogFh, "    !!!!!!!!!!!SPLIT COPY!!!!!!!!\n");
#endif
            partb = (int)sizeof(FIFO_ENTRY_HEADER) - parta;
            MEMCPY (Dest, Src, (size_t)parta);
            MEMCPY (&Dest[parta], Fifos[FiFoNr].Data, (size_t)partb);
            Src = (char*)Fifos[FiFoNr].Data + partb;
        }
#ifdef FIFO_LOG_TO_FILE
        DebugPrintHeader("    ", &Header);
#endif
        Len = Header.Size;
        if (Len >= 0) {
            // Then the data
            parta = (int)(Fifos[FiFoNr].Size - (Src - (char*)Fifos[FiFoNr].Data));
            if (parta >= Len) {
                Src += Len;
            }  else {
                partb = Len - parta;
                Src = (char*)Fifos[FiFoNr].Data + partb;
            }
            if ((Src - (char*)Fifos[FiFoNr].Data) >= Fifos[FiFoNr].Size) {
                Src = (char*)Fifos[FiFoNr].Data;
            }
            Fifos[FiFoNr].Rd = Src;

            Fifos[FiFoNr].RxCounter32++;
            if (Fifos[FiFoNr].RxCounter == 255) Fifos[FiFoNr].RxCounter = 1;
            else Fifos[FiFoNr].RxCounter++;
            if (UsedCounter != Fifos[FiFoNr].RxCounter) {
                ThrowError(1, " internal error inside fifos %s(%i): UsedCounter=0x%02X != Fifos[FiFoNr].RxCounter=0x%02X", __FILE__, __LINE__, (int)UsedCounter, (int)Fifos[FiFoNr].RxCounter);
            }
            XLeaveCriticalSection(&FiFosCriticalSection);
            return 0;
        }
#ifdef FIFO_LOG_TO_FILE
        fprintf (FifoLogFh, "    error FIFO_ERR_MESSAGE_TO_BIG\n");
#endif
        XLeaveCriticalSection(&FiFosCriticalSection);
        return FIFO_ERR_MESSAGE_TO_BIG;
    }
#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "    error FIFO_ERR_NO_DATA\n");
#endif
    XLeaveCriticalSection(&FiFosCriticalSection);
    return FIFO_ERR_NO_DATA;
}

int DeleteFiFo(int FiFoId, int RxPid)
{
    UNUSED(RxPid);
	int x;
	int FiFoNr;

	uint64_t TimeStamp = get_timestamp_counter();

    EnterCriticalSection (&FiFosCriticalSection);
	for (x = 0; x < MAX_FIFOS; x++) {
        if (Fifos[x].FiFoId == -1) {  // Mark the fifo for deletion
            if (Fifos[x].DeleteTimeStamp <= TimeStamp) {
				my_free(Fifos[x].Data);
                Fifos[x].FiFoId = 0;   // Now the fifo cn be reused
			}
		}
	}
    LeaveCriticalSection (&FiFosCriticalSection);

#ifdef FIFO_LOG_TO_FILE
	fprintf(FifoLogFh, "DeleteFiFo(FiFoId = %i, RxPid = %i)\n",
		FiFoId, RxPid);
#endif
	FiFoNr = FiFoId & 0xFFFF;
	if ((FiFoNr < 0) || (FiFoNr >= MAX_FIFOS)) return FIFO_ERR_INVALID_PARAMETER;
	if (Fifos[FiFoNr].FiFoId != FiFoId) return FIFO_ERR_UNKNOWN_HANDLE;
    Fifos[FiFoNr].IsOnline = 0;
    Fifos[FiFoNr].FiFoId = -1;   // Should be delete
    Fifos[FiFoNr].DeleteTimeStamp = get_timestamp_counter() + 4 * get_sched_periode_timer_clocks64();  // Fifo can be delete after 4 cycles

    return 0;
}

// This is only neccessary for remote master
// Each time a network package is reseved from the remote master this must be called
int SyncRxFiFos (void *Buffer, int Len)
{
    int MessageCount = 0;
#ifdef FIFO_LOG_TO_FILE
	fprintf(FifoLogFh, "SyncRxFiFos(Buffer, Len = %i)\n", Len);
#endif
    if (Len > 0) {
        FIFO_ENTRY_HEADER *Header = (FIFO_ENTRY_HEADER*)Buffer;
		for (; ; MessageCount++) {
            if (Len > (int)((char*)Header - (char*)Buffer)) {
                if ((Header->MessageId & MESSAGE_THROUGH_FIFO_MASK) == MESSAGE_THROUGH_FIFO_MASK) {
                    if ((Header->MessageId & IMPORTANT_MESSAGE_THROUGH_FIFO_MASK) == IMPORTANT_MESSAGE_THROUGH_FIFO_MASK) {
                        write_important_message_ts_as(Header->FiFoId, Header->MessageId & ~0xF0000000U, Header->Timestamp, Header->Size, (char*)(Header + 1), Header->TramsmiterPid);
                    } else {
                        write_message_ts_as(Header->FiFoId, Header->MessageId & ~0xF0000000U, Header->Timestamp, Header->Size, (char*)(Header + 1), Header->TramsmiterPid);
                    }
                } else {
                    WriteToFiFoDistribute (Header->FiFoId, Header->MessageId, Header->TramsmiterPid, Header->Timestamp, Header->Size, Header + 1);
                }
                Header = (FIFO_ENTRY_HEADER*)(void*)((char*)(Header + 1) + Header->Size);
            } else {
                break;
            }
        }
#ifdef FIFO_LOG_TO_FILE
		if (MessageCount) fprintf(FifoLogFh, "    %i Messages found\n", MessageCount);
#endif
	}
    return MessageCount;
}

// This must be call cyclic
int SyncTxFiFos (void *Buffer, int Len, int *DataToTransmit)
{
    FIFO_ENTRY_HEADER *Header;
    void *Data;
    int FiFoId = Fifos[0].FiFoId;  // Inside fifo 0 all TX messages be included
    int Count = 0;
    int Pos, PosSave;
    int MaxLen;

#ifdef FIFO_LOG_TO_FILE
	fprintf(FifoLogFh, "SyncTxFiFos(Buffer, Len = %i, DataToTransmit)\n", Len);
#endif

    Pos = 0;
    while (1) {
        PosSave = Pos;
        Header = (FIFO_ENTRY_HEADER*)(void*)((char*)Buffer + Pos);
        Pos += sizeof(FIFO_ENTRY_HEADER);
        Data = (void*)((char*)Buffer + Pos);
        MaxLen = Len - Pos;
        if (MaxLen <= 0) {
            *DataToTransmit = PosSave;
            if (Count) return 0x3;
            else return 0x2;
        }
        switch (ReadFromFiFo (FiFoId, Header, Data, MaxLen)) {
        case 0:
			Pos += Header->Size;
            Count++;
            break;
        case FIFO_ERR_NO_DATA:
			*DataToTransmit = PosSave;
#ifdef FIFO_LOG_TO_FILE
			fprintf(FifoLogFh, "FIFO_ERR_NO_DATA: Count = %i, DataToTransmit = %i\n", Count, *DataToTransmit);
#endif
			if (Count) return 0x1;
			return 0x0;
        case FIFO_ERR_MESSAGE_TO_BIG:
			*DataToTransmit = PosSave;
#ifdef FIFO_LOG_TO_FILE
			fprintf(FifoLogFh, "FIFO_ERR_MESSAGE_TO_BIG: Count = %i, DataToTransmit = %i\n", Count, *DataToTransmit);
#endif
			if (Count) return 0x3;
            return 0x2;
        default:
#ifdef FIFO_LOG_TO_FILE
			fprintf(FifoLogFh, "default: Count = %i\n", Count);
#endif
            // Error should be do something?
            return 0x0;
        }
    }
}
// Return:
//     1 -> new data to transmit to remote master
//     2 -> new data to transmit to remote master but buffer was to small


int InitFiFos (void)
{
#ifdef FIFO_LOG_TO_FILE
    static char Buffer[2];
    //FifoLogFh = open_file (FIFO_LOG_TO_FILE, "wt");
	FifoLogFh = fopen(FIFO_LOG_TO_FILE, "wt");
	printf("file \"%s\" open!\n", FIFO_LOG_TO_FILE);
	setvbuf(FifoLogFh, Buffer, _IOFBF, 2);  // no buffer
#endif

	if (sizeof(FIFO) != 128) {
		printf("sizeof(FIFO) = %i != 128\n", (int)sizeof(FIFO));
	}
#ifdef REMOTE_MASTER
    return CreateNewRxFifo (0, SYNC_FIFO_SIZE, "SyncFiFo");
#else
    InitializeCriticalSection (&FiFosCriticalSection);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        return CreateNewRxFifo (0, SYNC_FIFO_SIZE, "SyncFiFo");
    }
#endif
    return 0;
}

void TerminateFifos(void)
{
    int x;
    EnterCriticalSection (&FiFosCriticalSection);
    for (x = 0; x < MAX_FIFOS; x++) {
        if (Fifos[x].FiFoId == -1) {  // Mark for deletion
            my_free(Fifos[x].Data);
            Fifos[x].FiFoId = 0;   // Now the fifo can be used again
        } else if (Fifos[x].FiFoId != 0) {
            //ThrowError(1, "not closd fifo %s", Fifos[x].Name);
        }
    }
    LeaveCriticalSection (&FiFosCriticalSection);
}

#ifdef REMOTE_MASTER
int RegisterNewRxFiFoName(int RxPid, char *Name)
{
	return CreateNewRxFifo(RxPid, 0, Name);   // if Size == 0 it only register the fifo name
}


int UnRegisterRxFiFo(int FiFoId, int RxPid)
{
	return DeleteFiFo(FiFoId, RxPid);

}
#endif

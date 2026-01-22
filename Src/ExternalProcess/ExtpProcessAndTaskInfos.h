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


#ifndef EXTPROCESSANDTREADINFOS_H
#define EXTPROCESSANDTREADINFOS_H

#include "SharedDataTypes.h"
#include "PipeMessagesShared.h"
#include "VirtualNetworkShared.h"
#include "XilEnvExtProc.h"


typedef struct {
    char MessageFilename[MAX_PATH];
    int Flags;
    int MessageToFileFlag;
    int MaxErrorMessageCounter;
    int ErrorMessageCounter;
    int MessageCounterVid;
    char *HeaderText;
    char *pFilename;
} MESSAGE_OUTPUT_STREAM_CONFIG;

typedef struct {
    HANDLE ReqEvent;
    HANDLE AckEvent;
    HANDLE AsyncCommunicationThreadHandle;
    int TerminateFlag;  // if != 0 than thread should be terminate themself
} ASYNC_ALIVE_PING_THREAD_PARAM;

typedef struct {
    void *FirstAddress;
    void *LastAddress;
} REFERENCE_OVERLAY_BUFFER;

typedef struct {
    int Size;
    int Count;
    REFERENCE_OVERLAY_BUFFER *Entrys;
} REFERENCE_OVERLAY_BUFFERS;


typedef struct {
    union SC_BB_VARI *Buffer;  // fixed address! no realloc sizeof(union SC_BB_VARI) + strlen(name) + 1
    unsigned long long UniqueId;
    int Vid;
    enum SC_BB_DATA_TYPES Type;
    int AttachCounter;  
    int DirectFlag;     // if set directly read or write
} BLACKBOARD_WRITE_READ_CACHE;

typedef struct {
    int Size;
    int Count;
    BLACKBOARD_WRITE_READ_CACHE *Entrys;
} BLACKBOARD_WRITE_READ_CACHES;


typedef struct {
    uint32_t IsOnline;
    int32_t FiFoHandle;
    int32_t Size;
    int32_t Type;
    void *InData;
    void *InWr;
    void *InRd;
    void *OutData;
    void *OutWr;
    void *OutRd;
    int32_t Channel;
    uint32_t InMessagesCounter;
    uint32_t InLostMessagesCounter;
    uint32_t OutMessagesCounter;
    uint32_t OutLostMessagesCounter;

    // Message buffers
    // Flexray has 1024 slots
#define VIRTUAL_NETWORK_MAX_BUFFERS  1024 
    uint16_t RdBufferCount;
    uint16_t WrBufferCount;
    uint16_t RdPositions[VIRTUAL_NETWORK_MAX_BUFFERS];
    uint16_t WrPositions[VIRTUAL_NETWORK_MAX_BUFFERS];
    VIRTUAL_NETWORK_BUFFER *RdMessageBuffers[VIRTUAL_NETWORK_MAX_BUFFERS];
    VIRTUAL_NETWORK_BUFFER *WrMessageBuffers[VIRTUAL_NETWORK_MAX_BUFFERS];
    int MainHandle;
} VIRTUAL_NETWORK_FIFO;  

typedef struct {
    char InstanceName[MAX_PATH];
    char ServerName[MAX_PATH];
    char TaskName[64];
    char ProcessAtTaskName[MAX_PATH+64];
    char ExecutableName[MAX_PATH+64];
    char DllName[MAX_PATH];
    DWORD ThreadID;
    HANDLE PipeOrSocketHandle;
    ASYNC_ALIVE_PING_THREAD_PARAM AlivePing;

    int CallConvention;
#define CALL_CONVENTION_CDECL   0
#define CALL_CONVENTION_STDCALL 1
    union {
        struct {
            void (__cdecl *reference) (void);        // the refeence function
            int (__cdecl *init) (void);              // the init function
            void (__cdecl *cyclic) (void);           // the cyclic function
            void (__cdecl *terminate) (void);        // the terminte
        } Cdecl;
        struct {
            void (__stdcall *reference) (void);      // the refeence function
            int (__stdcall *init) (void);            // the init function
            void (__stdcall *cyclic) (void);         // the cyclic function
            void (__stdcall *terminate) (void);      // the terminte
        } Stdcall;
    } FuncPtrs;

    PIPE_MESSAGE_REF_COPY_LISTS CopyLists;

    REFERENCE_OVERLAY_BUFFERS OverlayBuffers;   // Collect all canged memory locations between 2 cycles to excluded this during walk through the reference lists

    BLACKBOARD_WRITE_READ_CACHES BbCaches;   // Faster access for read_bbvari_xxx() and write_bbvari_xxx() functions

    int sc_process_identifier;
    
    int Number;
    int ThreadCount;
    int State;   // 0 -> nicht aktiv, 1 -> aktiv
    unsigned long long CycleCount;
    unsigned long long LastCycleCount;

    int Priority;                     // if -1 than it is not defined
    int CycleDivider;                 // if -1 than it is not defined
    int Delay;                        // if -1 than it is not defined
    int LoginTimeout;

    PIPE_API_BASE_CMD_MESSAGE *pCmdMessageBuffer;
    PIPE_API_BASE_CMD_MESSAGE_ACK *pCmdMessageAckBuffer;

#ifdef _WIN32
    HANDLE XcpMutex;
#else
    pthread_mutex_t XcpMutex;
#endif
    void *ExecutableBaseAddress;
    void *DllBaseAddress;
    int DllSize;

    // Function pointers
    HANDLE (*ConnectTo) (char *par_InstanceName, char *par_ServerName, int par_Timout_ms, int par_ErrMessageFlag);
    void (*DisconnectFrom) (HANDLE hPipe);
	BOOL (*TransactMessage) (HANDLE hNamedPipe,
                             LPVOID lpInBuffer,
                             DWORD nInBufferSize,
                             LPVOID lpOutBuffer,
                             DWORD nOutBufferSize,
                             LPDWORD lpBytesRead);
	BOOL (*ReadMessage) (HANDLE hFile,
						 LPVOID lpBuffer,
						 DWORD nNumberOfBytesToRead,
						 LPDWORD lpNumberOfBytesRead);
    BOOL (*WriteMessage) (HANDLE hFile,
                          LPCVOID lpBuffer,
						  DWORD nNumberOfBytesToWrite,
						  LPDWORD lpNumberOfBytesWritten);

    int InsideErrorCallFlag;

#ifdef _WIN32
    HANDLE TaskMutex;
#else
    pthread_mutex_t TaskMutex;
#endif
    int MutexCount;
    const char *TaskMutexFile;
    int TaskMutexLine;
    const char *TaskMutexReleaseCallFromFile;
    int TaskMutexReleaseCallFromLine;
    const char *TaskMutexWaitForCallFromFile;
    int TaskMutexWaitForCallFromLine;

    // Virtual network
    int VirtualNetworkHandles[16];
    int VirtualNetworkHandleCount;

    VIRTUAL_NETWORK_FIFO *Fifos;
    int FifoCounter;
    CRITICAL_SECTION FiFosCriticalSection;

} EXTERN_PROCESS_TASK_INFOS_STRUCT;


typedef struct {
    char Prefix[64];
    char ExecutableName[MAX_PATH];
    int UserSetExecutableName;   // Is 1 than don't call GetModuleName()
    void *ExecutableBaseAddress;
    int ThreadPos; 
    int ThreadCount;   // min. 1 max. 16
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TasksInfos[MAX_PROCESSES_INSIDE_ONE_EXECUTABLE];

#ifdef _WIN32
    HANDLE XcpMutex;
#else
    pthread_mutex_t XcpMutex;
#endif
#ifdef _WIN32
    HANDLE ProcessMutex;
#else
    pthread_mutex_t ProcessMutex;
#endif
    char WriteBackExeToDir[MAX_PATH];

    int NoGuiFlag;
    int Err2Msg;
    int NoXcpFlag;

    MESSAGE_OUTPUT_STREAM_CONFIG MessageOutputStreamConfig;

} EXTERN_PROCESS_INFOS_STRUCT;

#endif

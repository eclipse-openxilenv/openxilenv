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


#ifndef TCB_H
#define TCB_H

#include <stdint.h>
#include "Platform.h"
#include <stdio.h>
#include "GlobalDataTypes.h"

#include "SharedDataTypes.h"
#include "PipeMessagesShared.h"

/*************************************************************************
*                          constants and macros
*/

#define INTERN_ASYNC      0 // internal process
#define INTERN_SYNC       1 // internal realtime process
#define EXTERN_ASYNC      2 // external process

#define PROCESS_AKTIV         0 // Process is running
#define PROCESS_SLEEP         1 // Process is sleeping
#define PROCESS_NONSLEEP      2 // Process cannot sleep
#define EX_PROCESS_REFERENCE  3
#define EX_PROCESS_INIT       4
#define EX_PROCESS_TERMINATE  5
#define EX_PROCSS_LOGIN       6
#define WIN_PROCESS           7
#define RT_PROCESS_INIT       8
#define RT_PROCESS_TERMINATE  9
#define EX_PROCESS_WAIT_FOR_END_OF_CYCLE_TO_GO_TO_TERM   10

/*************************************************************************
*                              data types
*/

typedef char NAME_STRING[512];

typedef struct {
    int size;              /* Size of the message buffer */
    int rp;                /* Current read position */
    int wp;                /* Current write position */
    int count;             /* Number of messages inside the queue */
    char *buffer;          /* Pointer to the memory storing the messages */
} MESSAGE_BUFFER;          /* Input message queue of an internal process */


typedef struct {
   int Nr;
   void *ExecStack;
} PROC_EXEC_STACK;

typedef struct TCB {                 // Task control block
    PID pid;                         // Process ID
    NAME_STRING name;                // Process name
    int Type;                        // Process type
    int Active;                      // Flag is set if process is running
    int state;                       // Process state
    int prio;                        // Order inside the scheduling loop
    void (*cyclic) (void);           // cyclic function
    int (*init) (void);              // Init function
    void (*terminat) (void);         // Terminat function
    int timeout;                     // Timeout                                     
    int time_counter;                // Cycle counter
    int time_steps;                  // Cycle divider
    int message_size;                // Size of the message queue
    MESSAGE_BUFFER *message_buffer;  // Pointer to the message queue (if NULL process have no queue)
    struct TCB *next;                // Pointer to the next process (if its addd to an scheduler)
    struct TCB *prev;                // Pointer to the previous process (if its addd to an scheduler)
    uint64_t min_time;               // Time measurement for realtime processes
    uint64_t max_time;
    uint64_t last_time;
    uint64_t sum_time;
    int32_t vid_time;
    uint64_t call_count;        // Number of cyclic calles
    uint64_t bb_access_mask;    // Maske for blackboard access
    uint64_t wr_flag_mask;      // Maske for blackboard write flags
    short BeforeProcessEquationCounter;
    PROC_EXEC_STACK *BeforeProcessEquations;
    short BehindProcessEquationCounter;
    PROC_EXEC_STACK *BehindProcessEquations;

    DWORD ProcessId;
    int delay;
    DWORD actual_timeout;
    HANDLE hPipe;
    int SocketOrPipe;

    int IsStartedInDebugger;  // This is 1 if process is strated inside a debugger
    int MachineType;          // If 0 -> 32Bit Windows 1 -> 64Bit Windows, 2 -> 32Bit Linux, 3 -> 64Bit Linux
    HANDLE hAsyncAlivePingReqEvent;    // With this event XilEnv can test if an external process is alive
    HANDLE hAsyncAlivePingAckEvent;

    // Read and write memory of an external procss through the dbug API from windows
    HANDLE ProcHandle;
    int UseDebugInterfaceForWrite;
    int UseDebugInterfaceForRead;

    int wait_on;                     // This is set if a scheduler is waiting till an external process cyclic function call is finisched

    PIPE_MESSAGE_REF_COPY_LISTS CopyLists;

    int BarrierBehindCount;
    int BarrierBehindNr[8];
    int PosInsideBarrierBehind[8];

    int BarrierBeforeCount;
    int BarrierBeforeNr[8];
    int PosInsideBarrierBefore[8];

    int BarrierLoopOutBehindCount;
    int BarrierLoopOutBehindNr[8];
    int PosInsideBarrierLoopOutBehind[8];

    int BarrierLoopOutBeforeCount;
    int BarrierLoopOutBeforeNr[8];
    int PosInsideBarrierLoopOutBefore[8];

    // Oly for debugging purpose this will handl by a mutx now
    int WorkingFlag;   // Process is inside his cyclic/init/terminate/reference function
    int LockedFlag;    // Process not allowed to enter his cyclic/init/terminate/reference function, he has to wait
    int SchedWaitForUnlocking;  // Scheduler wait that he can call cyclic/init/terminate/reference of the process
    int WaitForEndOfCycleFlag;  // There is waiting thomebody that the process finished it cyclic/init/terminate/reference function

    DWORD LockedByThreadId;
    // Only for debugging
    const char *SrcFileName;
    int SrcLineNr;

#ifdef _WIN32
    HANDLE Mutex;
#else
    pthread_mutex_t Mutex;
#endif

    PIPE_API_BASE_CMD_MESSAGE* TransmitMessageBuffer;
    PIPE_API_BASE_CMD_MESSAGE_ACK* ReceiveMessageBuffer;

    // Range-Control
    unsigned int RangeControlFlags;
#define RANGE_CONTROL_BEFORE_ACTIVE_FLAG      0x10000000U
#define RANGE_CONTROL_BEHIND_ACTIVE_FLAG      0x04000000U
#define RANGE_CONTROL_STOP_SCHEDULER_FLAG     0x20000000U
#define RANGE_CONTROL_OUTPUT_MASK             0x0000000FU     // Messagebox, Write to File, ...
#define RANGE_CONTROL_COUNTER_VARIABLE_FLAG   0x40000000U
#define RANGE_CONTROL_CONTROL_VARIABLE_FLAG   0x80000000U
#define RANGE_CONTROL_PHYSICAL_FLAG           0x01000000U
#define RANGE_CONTROL_LIMIT_VALUES_FLAG       0x02000000U
#define RANGE_CONTROL_BEFORE_INIT_AS_DEACTIVE 0x00100000U
#define RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE 0x00200000U

    int RangeCounterVid;
    int RangeControlVid;

    char *DllName;     // If the process lives inside a DLL this contains the DLL name otherwise the pointer is NULL.
    char *InsideExecutableName;
    int NumberInsideExecutable;
    int NumberOfProcessesInsideExecutable;

    // Function pointers
    DWORD (*ReadFromConnection) (HANDLE Handle, void *Buffer, DWORD BufferSize, LPDWORD BytesRead);
    DWORD (*WriteToConnection) (HANDLE Handle, void *Buffer, int nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten);
    void  (*CloseConnectionToExternProcess) (HANDLE Handle);
    FILE *ConnectionLoggingFile;

    uint64_t ExecutableBaseAddress;
    uint64_t DllBaseAddress;

    int ExternProcessComunicationProtocolVersion;

    int VirtualNetworkHandles[16];
    int VirtualNetworkHandleCount;

    int A2LLinkNr;

    CRITICAL_SECTION CriticalSection;
} TASK_CONTROL_BLOCK;

#ifdef _WIN32
#ifndef PTHREAD_MUTEX_INITIALIZER //__GNUC__
#define PTHREAD_MUTEX_INITIALIZER  NULL
#endif
#define CRITICAL_SECTION_INITIALIZER {0, }
#else
#define CRITICAL_SECTION_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif


#define INIT_TASK_COTROL_BLOCK(name, type, priority, cyclic_function, init_function, terminate_function, message_queue_size) \
      { \
        -1,                       /* Process is not running */ \
        name,                     /* Process name */ \
        type,                     /* Process type INTERN_ASYNC or INTERN_SYNC */ \
        0,                        /* Process is sleping = 0 */ \
        0,                        /* Process state = 0 */ \
        priority,                 /* Order */ \
        cyclic_function, \
        init_function, \
        terminate_function, \
        10000,                    /* Timeout = 10 sec */ \
        0,                        /* Timeout counter starts with 0 */ \
        1,                        /* Process should b call inside each time slot */ \
        message_queue_size,       /* Size of the message queue */ \
        NULL,                     /* Pointer to the message queue */ \
        NULL,                     /* Pointer to the next process (if its addd to an scheduler) */ \
        NULL,                     /* Pointer to the previous process (if its addd to an scheduler) */ \
        UINT64_MAX,               /* Time masurement for realtime processes */ \
        0, \
        0, \
        0, \
        -1,                       /* Time variabele ID */ \
        0,                        /* Number of cyclic calles */ \
        0,                        /* Maske for blackboard access */ \
        0,                        /* Maske for blackboard write flags */ \
        0,                        /* BeforeProcessEquationCounter */ \
        NULL,                     /* BeforeProcessEquations */ \
        0,                        /* BehindProcessEquationCounter */ \
        NULL,                     /* BehindProcessEquations */ \
\
        0,                        /* ProcessId */ \
        0,                        /* delay */ \
        0,                        /* actual_timeout */ \
        NULL_INT_OR_PTR,          /* hPipe */ \
        -1,                       /* SocketOrPipe */ \
\
        0,                        /* IsStartedInDebugger */ \
        0,                        /* MashineType */ \
        NULL_INT_OR_PTR,          /* hAsyncAlivePingReqEvent */ \
        NULL_INT_OR_PTR,          /* hAsyncAlivePingAckEvent */ \
\
        NULL_INT_OR_PTR,          /* ProcHandle */ \
        0,                        /* UseDebugInterfaceForWrite */ \
        0,                        /* UseDebugInterfaceForRead */ \
\
        0,                        /* wait_on */ \
        {{0,0,0,0,NULL}, \
         {0,0,0,0,NULL},{0,0,0,0,NULL}, \
         {0,0,0,0,NULL},{0,0,0,0,NULL}, \
         {0,0,0,0,NULL},{0,0,0,0,NULL}, \
         {0,0,0,0,NULL},{0,0,0,0,NULL}}, /* CopyLists */ \
\
        0,                        /* BarrierBehindCount */ \
        {0,0,0,0,0,0,0,0},        /* BarrierBehindNr[8] */ \
        {0,0,0,0,0,0,0,0},        /* PosInsideBarrierBehind[8] */ \
\
        0,                        /* BarrierBeforeCount */ \
        {0,0,0,0,0,0,0,0},        /* BarrierBeforeNr[8] */ \
        {0,0,0,0,0,0,0,0},        /* PosInsideBarrierBefore[8] */ \
\
        0,                        /* BarrierLoopOutBehindCount */ \
        {0,0,0,0,0,0,0,0},        /* BarrierLoopOutBehindNr[8] */ \
        {0,0,0,0,0,0,0,0},        /* PosInsideBarrierLoopOutBehind[8] */ \
\
        0,                        /* BarrierLoopOutBeforeCount */ \
        {0,0,0,0,0,0,0,0},        /* BarrierLoopOutBeforeNr[8] */ \
        {0,0,0,0,0,0,0,0},        /* PosInsideBarrierLoopOutBefore[8] */ \
\
        0,                        /* WorkingFlag */ \
        0,                        /* LockedFlag */ \
        0,                        /* SchedWaitForUnlocking */ \
        0,                        /* WaitForEndOfCycleFlag */ \
\
        0,                        /* LockedByThreadId */ \
\
        NULL,                     /* *SrcFileName */ \
        0,                        /* SrcLineNr */ \
\
        PTHREAD_MUTEX_INITIALIZER, /* Mutex */ \
\
        NULL,                     /* TransmitMessageBuffer */ \
        NULL,                     /* ReceiveMessageBuffer */ \
\
        0,                        /* RangeControlFlags */ \
\
        -1,                       /* RangeCounterVid */ \
        -1,                       /* RangeControlVid */ \
\
        NULL,                     /* *DllName */ \
        NULL,                     /* *InsideExecutableName */ \
        0,                        /* NumberInsideExecutable */ \
        0,                        /* NumberOfProcessesInsideExecutable */ \
\
        NULL,                     /* *ReadFromConnection */ \
        NULL,                     /* *WriteToConnection */ \
        NULL,                     /* *CloseConnectionToExternProcess */ \
        NULL,                     /* *ConnectionLoggingFile */ \
        0,                        /* ExecutableBaseAddress */ \
        0,                        /* DllBaseAddress */ \
        0,                        /* ExternProcessComunicationProtocolVersion */ \
        {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},  /* VirtualNetworkHandles[16] */ \
        0,                         /* VirtualNetworkHandleCount */ \
        0,                         /* A2LLinkNr */ \
        CRITICAL_SECTION_INITIALIZER \
    }

#endif

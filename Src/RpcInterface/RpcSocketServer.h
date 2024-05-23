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


#ifndef RPCSOCKETSERVER_H
#define RPCSOCKETSERVER_H

#include <stdint.h>
#include <stdio.h>

#include "Platform.h"
#include "RpcFuncBase.h"

#define RPC_MAX_CONNECTIONS  8
#define RPC_API_MAX_MESSAGE_SIZE (256*1024)

#define CONNECTION_SHOULD_BE_CLOSED   0

typedef struct {
/* 8 */    uint64_t Socket;
/* 8 */    uint64_t Thread;
/* 4 */    uint32_t ThreadId;
/* 4 */    uint32_t StructSize;
/* 1 */    uint8_t UsedFlag;
/* 1 */    uint8_t ConnectedFlag;
/* 1 */    uint8_t ConnectionType;   // 0 -> named pipe 1 -> socket
/* 1 */    uint8_t WaitToReceiveAlivePingAck;
/* 4 */    uint32_t Filler;
/* 8 */    void *ReceiveBuffer;
/* 8 */    void *TransmitBuffer;
/* 4 */    int32_t ReceiveBufferSize;
/* 4 */    int32_t TransmitBufferSize;

/* 4 */    volatile int32_t WaitFlag;
/* 4 */    uint32_t WaitUntilRemainder;
/* 4 */    int32_t ReturnValue;
/* 4 */    int32_t GetNextProcessCounter;

/* 4 */    int32_t CanFifoHandle;
/* 4 */    int32_t DebugFlags;
/* 4 */    int32_t GetNextIndex;
/* 4 */    int32_t ConnectionNr;

/* 8 */    FILE *DebugFile;
#define DEBUG_PRINT_COMMAND_NAME               1
#define DEBUG_PRINT_COMMAND_PARAMETER_AS_HEX   2
#define DEBUG_PRINT_COMMAND_RETURN_AS_HEX      4
#define DEBUG_PRINT_COMMAND_PARAMETER_DECODED  8
#define DEBUG_PRINT_COMMAND_RETURN_DECODED    16
#define DEBUG_PRINT_COMMAND_FFLUSH          1024
#define DEBUG_PRINT_COMMAND_ATTACH          2048
#define DEBUG_PRINT_COMMAND_ONE_FILE        4096

/*72 */
           CRITICAL_SECTION CriticalSection;
           CONDITION_VARIABLE ConditionVariable;

           int CCPVarCounter[4];
           char **CCPVariable[4];
           int CCPCalCounter[4];
           char **CCPParameter[4];

           int XCPVarCounter[4];
           char **XCPVariable[4];
           int XCPCalCounter[4];
           char **XCPParameter[4];

           int SchedulerDisableCounter;
           int DoNextCycleFlag;

           void *CanRecorderHandle;

          // int8_t Filler[128-80 + sizeof(CRITICAL_SECTION) + sizeof(CONDITION_VARIABLE)];    // Cacheline 128Byte
} RPC_CONNECTION;


typedef struct {
/* 8 */    int (*FunctionCallPtr)(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *In, RPC_API_BASE_MESSAGE_ACK *Out);
/* 8 */    char *CommandString;
/* 4 */    int32_t Command;
/* 4 */    int32_t MinMessageSize;
/* 4 */    int32_t MaxMessageSize;
/* 4 */    int32_t HaveToSyncWithGUI;    
} FUNCTION_CALL_TABLE;  /* 32Bytes */


int RemoteProcedureDecodeMessage(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *In, RPC_API_BASE_MESSAGE_ACK *Out);
#define AddFunctionToRemoteAPIFunctionTable(par_Command, par_HaveToSyncWithGUI, \
                                            par_FunctionCallPtr, \
                                            par_MinMessageSize, par_MaxMessageSize) \
    AddFunctionToRemoteAPIFunctionTable_(par_Command, #par_Command "\0\0", par_HaveToSyncWithGUI, \
                                         par_FunctionCallPtr, \
                                         par_MinMessageSize, par_MaxMessageSize)

#define AddFunctionToRemoteAPIFunctionTable2(par_Command, par_HaveToSyncWithGUI, \
                                            par_FunctionCallPtr, \
                                            par_MinMessageSize, par_MaxMessageSize, par_StructReq, par_StructAck) \
    AddFunctionToRemoteAPIFunctionTable_(par_Command, #par_Command "\0" par_StructReq "\0" par_StructAck, par_HaveToSyncWithGUI, \
                                         par_FunctionCallPtr, \
                                         par_MinMessageSize, par_MaxMessageSize)

int AddFunctionToRemoteAPIFunctionTable_ (int par_Command, char *par_CommandString, int par_HaveToSyncWithGUI,
                                          int (*par_FunctionCallPtr)(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *In, RPC_API_BASE_MESSAGE_ACK *Out),
                                          int par_MinMessageSize, int par_MaxMessageSize);



int StartRemoteProcedureCallThread (int par_SocketOrNamedPipe, char *par_Prefix, int par_Port);

void RemoteProcedureWakeWaitForConnection(RPC_CONNECTION *par_Connection);
void RemoteProcedureWaitForConnection(RPC_CONNECTION *par_Connection);
void RemoteProcedureMarkedForWaitForConnection(RPC_CONNECTION *par_Connection);

void StopAcceptingNewConnections(RPC_CONNECTION *par_Connection);

void SetShouldExit(int par_ShouldTerminate, int par_SetExitCode, int par_ExitCode);
void ExecuteShouldExit(void);


#endif // RPCSOCKETSERVER_H

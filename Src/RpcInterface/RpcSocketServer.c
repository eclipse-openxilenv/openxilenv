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


#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <inttypes.h>
#include "Platform.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define SOCKET int
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include "ThrowError.h"
#include "Files.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "Scheduler.h"
#include "MainValues.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "MainWinowSyncWithOtherThreads.h"

#include "RpcFuncBase.h"
#include "RpcFuncLogin.h"
#include "RpcFuncMisc.h"
#include "RpcFuncSched.h"
#include "RpcFuncInternalProcesses.h"
#include "RpcFuncGui.h"
#include "RpcFuncBlackboard.h"
#include "RpcFuncCalibration.h"
#include "RpcFuncCan.h"
#include "RpcFuncCcp.h"
#include "RpcFuncXcp.h"

#include "CanFifo.h"
#include "RemoteMasterNet.h"

#include "RpcControlProcess.h"

#include "RpcSocketServer.h"

// 64Kbyte Pipes
#define RPC_MESSAGE_BUFSIZE     (256*1024)
#define MAX_FUNCTION_TABLE_SIZE  500

#define XILENV_RPC_PIPE_NAME  "\\\\.\\pipe\\XilEnvRemoteProcedureCall"

#define UNUSED(x) (void)(x)

static FUNCTION_CALL_TABLE FunctionCallTable[MAX_FUNCTION_TABLE_SIZE];

static RPC_CONNECTION Connections [RPC_MAX_CONNECTIONS];
static FILE* LogFileHandle;
static int LogFileHandleAttachCounter;

static int NewConnectionsAllowed;

static CRITICAL_SECTION CriticalSection;
static CONDITION_VARIABLE ConditionVariable;

#ifdef _WIN32
    static HANDLE NewConnectionThread;
#else
    static pthread_t NewConnectionThread;
#endif

static int MustSyncWithGUI(RPC_API_BASE_MESSAGE *In)
{
    if ((In->Command < 0) && (In->Command >= MAX_FUNCTION_TABLE_SIZE)) return 0;
    return FunctionCallTable[In->Command].HaveToSyncWithGUI;
}

static void PrintHexToDebugFile (FILE *par_File, const char *par_Header, const unsigned char *par_Data, int par_Size)
{
    int x;
    fprintf (par_File, "\t%s (%i):\n", par_Header, par_Size);
    for (x = 0; x < par_Size; x++) {
        if ((x % 16) == 0) fprintf (par_File, "\t\t0x%04X:  %02X ", x, (unsigned int)par_Data[x]);
        else if ((x % 16) == 15) fprintf (par_File, "%02X\n", (unsigned int)par_Data[x]);
        else if ((x % 8) == 0) fprintf (par_File, "  %02X ", (unsigned int)par_Data[x]);
        else if ((x % 4) == 0) fprintf (par_File, " %02X ", (unsigned int)par_Data[x]);
        else fprintf (par_File, "%02X ", (unsigned int)par_Data[x]);
    }
    if ((x % 16) != 0) fprintf (par_File, "\n");
}

static const char *GetReqStructDefString(const char *par_CommandString)
{
    return par_CommandString+strlen(par_CommandString)+1;
}

static const char *GetAckStructDefString(const char *par_CommandString)
{
    const char *p = GetReqStructDefString(par_CommandString);
    return p+strlen(p)+1;
}

static int GetArrayDimensionDecodeToDebugFile(const char *par_StartName, const char *par_StructDefString, const unsigned char *par_Data, int par_Size)
{
    UNUSED(par_Size);
    const char *p = par_StructDefString;
    const char *Type, *Name;
    int TypeLen, NameLen;
    int PosInsideStruct = 0;
    int Ret = -1;

    do {
        while (isspace(*p)) p++;
        Type = p;
        while (!isspace(*p) && (*p != 0)) p++;
        TypeLen = (int)(p - Type);
        while (isspace(*p)) p++;
        Name = p;
        while (!isspace(*p) && (*p != ';') && (*p != 0)) p++;
        NameLen = (int)(p - Name);
        if (*p == ';') {
            p++;
            if (!strncmp("RPC_API_BASE_MESSAGE", Type, (size_t)TypeLen)) {
                PosInsideStruct += sizeof(RPC_API_BASE_MESSAGE);
            } else if (!strncmp("RPC_API_BASE_MESSAGE_ACK", Type, (size_t)TypeLen)) {
                PosInsideStruct += sizeof(RPC_API_BASE_MESSAGE_ACK);
            } else if (!strncmp("int32_t", Type, (size_t)TypeLen)) {
                Ret = *(const int32_t*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(int32_t);
            } else if (!strncmp("uint32_t", Type, (size_t)TypeLen)) {
                Ret = (int)(*(const uint32_t*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(uint32_t);
            } else if (!strncmp("int16_t", Type, (size_t)TypeLen)) {
                Ret = (int)*(const int16_t*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(int16_t);
            } else if (!strncmp("uint16_t", Type, (size_t)TypeLen)) {
                Ret =(int)*(const uint16_t*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(uint16_t);
            } else if (!strncmp("int8_t", Type, (size_t)TypeLen)) {
                Ret = (int)*(const int8_t*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(int8_t);
            } else if (!strncmp("uint8_t", Type, (size_t)TypeLen)) {
                Ret = (int)*(const uint8_t*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(uint8_t);
            } else if (!strncmp("int64_t", Type, (size_t)TypeLen)) {
                Ret = (int)*(const int64_t*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(int64_t);
            } else if (!strncmp("uint64_t", Type, (size_t)TypeLen)) {
                Ret =  (int)*(const uint64_t*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(uint64_t);
            } else if (!strncmp("double", Type, (size_t)TypeLen)) {
                Ret =  (int)*(const double*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(double);
            } else if (!strncmp("float", Type, (size_t)TypeLen)) {
                Ret =  (int)*(const float*)(const void*)(par_Data + PosInsideStruct);
                PosInsideStruct += sizeof(float);
            } else if (!strncmp("char", Type, (size_t)TypeLen) && !strncmp("Data[1]", Name, (size_t)NameLen)) {
                // this can be at the end of the structure
                // wird nicht ausgegeben!
            } else {
                break;  // do/while
            }
            if (Ret >= 1) {
                if (!strncmp(par_StartName, Name, (size_t)NameLen)) {
                    if (par_StartName[NameLen] == '_') {
                        return Ret;   // es wurde eine Dimension gefunden
                    }
                }
            }
            Ret = -1;
        }
    } while (*p != 0);
    return Ret;
}

static void PrintDecodedToDebugFile (FILE *par_File, const char *par_Header, const char *par_StructDefString, const unsigned char *par_Data, int par_Size)
{
    const char *p = par_StructDefString;
    const char *Type, *Name;
    int TypeLen, NameLen;
    int PosInsideStruct = 0;
    fprintf (par_File, "\t%s (%s):\n", par_Header, par_StructDefString);
    // Decode the structure definition string
    // it must be: datatype whitepspace membername;
    do {
        while (isspace(*p)) p++;
        Type = p;
        while (!isspace(*p) && (*p != 0)) p++;
        TypeLen = (int)(p - Type);
        while (isspace(*p)) p++;
        Name = p;
        while (!isspace(*p) && (*p != ';') && (*p != 0)) p++;
        NameLen = (int)(p - Name);
        if (*p == ';') {
            p++;
            if (!strncmp("RPC_API_BASE_MESSAGE", Type, (size_t)TypeLen)) {
                const RPC_API_BASE_MESSAGE *v = (const RPC_API_BASE_MESSAGE*)(const void*)(par_Data + PosInsideStruct);
                fprintf (par_File, "\t\t%.*s %.*s = {%i, %i}\n", TypeLen, Type, NameLen, Name, v->StructSize, v->Command);
                PosInsideStruct += sizeof(RPC_API_BASE_MESSAGE);
                // will be not printed
            } else if (!strncmp("RPC_API_BASE_MESSAGE_ACK", Type, (size_t)TypeLen)) {
                const RPC_API_BASE_MESSAGE_ACK *v = (const RPC_API_BASE_MESSAGE_ACK*)(const void*)(par_Data + PosInsideStruct);
                fprintf (par_File, "\t\t%.*s %.*s = {%i, %i, %i}\n", TypeLen, Type, NameLen, Name, v->StructSize, v->Command, v->ReturnValue);
                PosInsideStruct += sizeof(RPC_API_BASE_MESSAGE_ACK);
                // will be not printed
            } else if (!strncmp("int32_t", Type, (size_t)TypeLen)) {
                int32_t v = *(const int32_t*)(const void*)(par_Data + PosInsideStruct);
                fprintf (par_File, "\t\t%.*s %.*s = %i\n", TypeLen, Type, NameLen, Name, v);
                if (!strncmp("Offset", Name, 6)) {
                    if (Name[6] == '_') {
                        // If "Offset" followed by _ than a data type and the dimension of an array will follow.
                        if (!strncmp("int32_", Name + 7, 6)) {
                            int Size = GetArrayDimensionDecodeToDebugFile(Name + 7 + 6, par_StructDefString, par_Data, par_Size);
                            if (Size > 0) {
                                if ((Size * (int)sizeof (int32_t) + v) <= par_Size) {
                                    int x;
                                    fprintf (par_File, "\t\t\t[%i] ", Size);
                                    for (x = 0; x < Size; x++) {
                                        fprintf (par_File, " %i", *(const int32_t*)(const void*)(par_Data + v + x * (int)sizeof(int32_t)));
                                    }
                                    fprintf (par_File, "\n");
                                } else {
                                    fprintf (par_File, "\t\t\tarray size (%i) too larg\n", Size);
                                }
                            } else {
                                fprintf (par_File, "\t\t\tno array dimension found\n");
                            }
                        } else if (!strncmp("uint8_", Name + 7, 6)) {
                            int Size = GetArrayDimensionDecodeToDebugFile(Name + 7 + 6, par_StructDefString, par_Data, par_Size);
                            if (Size > 0) {
                                if ((Size * (int)sizeof (uint8_t) + v) <= par_Size) {
                                    int x;
                                    fprintf (par_File, "\t\t\t[%i] ", Size);
                                    for (x = 0; x < Size; x++) {
                                        fprintf (par_File, " %u", (uint32_t)*(const int8_t*)(par_Data + v + x * (int)sizeof(uint8_t)));
                                    }
                                    fprintf (par_File, "\n");
                                } else {
                                    fprintf (par_File, "\t\t\tarray size (%i) too larg\n", Size);
                                }
                            } else {
                                fprintf (par_File, "\t\t\tno array dimension found\n");
                            }
                        } else if (!strncmp("double_", Name + 7, 7)) {
                            int Size = GetArrayDimensionDecodeToDebugFile(Name + 7 + 7, par_StructDefString, par_Data, par_Size);
                            if (Size > 0) {
                                if ((Size * (int)sizeof (double) + v) <= par_Size) {
                                    int x;
                                    fprintf (par_File, "\t\t\t[%i] ", Size);
                                    for (x = 0; x < Size; x++) {
                                        fprintf (par_File, " %g", *(const double*)(const void*)(par_Data + v + x * (int)sizeof(double)));
                                    }
                                    fprintf (par_File, "\n");
                                } else {
                                    fprintf (par_File, "\t\t\tarray size (%i) too larg\n", Size);
                                }
                            } else {
                                fprintf (par_File, "\t\t\tno array dimension found\n");
                            }
                        } else if (!strncmp("CanAcceptance_", Name + 7, 14)) {
                            int Size = GetArrayDimensionDecodeToDebugFile(Name + 7 + 14, par_StructDefString, par_Data, par_Size);
                            if (Size > 0) {
                                if ((Size * (int)sizeof (CAN_ACCEPT_MASK) + v) <= par_Size) {
                                    int x;
                                    fprintf (par_File, "\t\t\t[%i] ", Size);
                                    for (x = 0; x < Size; x++) {
                                        const CAN_ACCEPT_MASK *Element = (const CAN_ACCEPT_MASK*)(const void*)(par_Data + v + x * (int)sizeof(CAN_ACCEPT_MASK));
                                        fprintf (par_File, " {%i,%i,%i}", Element->Channel, Element->Start, Element->Stop);
                                    }
                                    fprintf (par_File, "\n");
                                } else {
                                    fprintf (par_File, "\t\t\tarray size (%i) too larg\n", Size);
                                }
                            } else {
                                fprintf (par_File, "\t\t\tno array dimension found\n");
                            }
                        } else if (!strncmp("CanObject_", Name + 7, 10)) {
                            int Size = GetArrayDimensionDecodeToDebugFile(Name + 7 + 10, par_StructDefString, par_Data, par_Size);
                            if (Size > 0) {
                                if ((Size * (int)sizeof (CAN_FIFO_ELEM) + v) <= par_Size) {
                                    int x;
                                    fprintf (par_File, "\t\t\t[%i]\n", Size);
                                    for (x = 0; x < Size; x++) {
                                        int i;
                                        const CAN_FIFO_ELEM *Element = (const CAN_FIFO_ELEM*)(const void*)(par_Data + v + x * (int)sizeof(CAN_FIFO_ELEM));
                                        fprintf (par_File, "\t\t\t{%u,{", Element->id);
                                        for (i = 0; (i < 8) && (i < Element->size); i++) {
                                            fprintf (par_File, "%02X ", (int)Element->data[i]);
                                        }
                                        fprintf (par_File, "},%u,%u,%u,%u,%" PRIu64 "}\n", Element->size, Element->ext, Element->channel, Element->node, Element->timestamp);
                                     }
                                    fprintf (par_File, "\n");
                                } else {
                                    fprintf (par_File, "\t\t\tarray size (%i) too larg\n", Size);
                                }
                            } else {
                                fprintf (par_File, "\t\t\tno array dimension found\n");
                            }
                        }
                    } else if (!strncmp("CanFdObject_", Name + 7, 12)) {
                        int Size = GetArrayDimensionDecodeToDebugFile(Name + 7 + 12, par_StructDefString, par_Data, par_Size);
                        if (Size > 0) {
                            if ((Size * (int)sizeof (CAN_FD_FIFO_ELEM) + v) <= par_Size) {
                                int x;
                                fprintf (par_File, "\t\t\t[%i]\n", Size);
                                for (x = 0; x < Size; x++) {
                                    int i;
                                    const CAN_FD_FIFO_ELEM *Element = (const CAN_FD_FIFO_ELEM*)(const void*)(par_Data + v + x * (int)sizeof(CAN_FD_FIFO_ELEM));
                                    fprintf (par_File, "\t\t\t{%u,{", Element->id);
                                    for (i = 0; (i < 64) && (i < Element->size); i++) {
                                        fprintf (par_File, "%02X ", (int)Element->data[i]);
                                    }
                                    fprintf (par_File, "},%u,%u,%u,%u,%" PRIu64 "}\n", Element->size, Element->ext, Element->channel, Element->node, Element->timestamp);
                                 }
                                fprintf (par_File, "\n");
                            } else {
                                fprintf (par_File, "\t\t\tarray size (%i) too larg\n", Size);
                            }
                        } else {
                            fprintf (par_File, "\t\t\tno array dimension found\n");
                        }
                    } else {
                        // Wenn Offet ohne _ dann ist es immer ein String
                        fprintf (par_File, "\t\t\t\"%.*s\"\n", par_Size - v, (par_Data + v));
                    }
                }
                PosInsideStruct += sizeof(int32_t);
            } else if (!strncmp("uint32_t", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %u\n", TypeLen, Type, TypeLen, Name, *(const uint32_t*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(uint32_t);
            } else if (!strncmp("int16_t", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %i\n", TypeLen, Type, TypeLen, Name, (int)*(const int16_t*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(int16_t);
            } else if (!strncmp("uint16_t", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %u\n", TypeLen, Type, TypeLen, Name, (unsigned int)*(const uint16_t*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(uint16_t);
            } else if (!strncmp("int8_t", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %i\n", TypeLen, Type, TypeLen, Name, (int)*(const int8_t*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(int8_t);
            } else if (!strncmp("uint8_t", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %u\n", TypeLen, Type, TypeLen, Name, (unsigned int)*(const uint8_t*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(uint8_t);
            } else if (!strncmp("int64_t", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %" PRIi64 "\n", TypeLen, Type, TypeLen, Name, *(const int64_t*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(int64_t);
            } else if (!strncmp("uint64_t", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %" PRIu64 "\n", TypeLen, Type, TypeLen, Name, *(const uint64_t*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(uint64_t);
            } else if (!strncmp("double", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %g\n", TypeLen, Type, TypeLen, Name, *(const double*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(double);
            } else if (!strncmp("float", Type, (size_t)TypeLen)) {
                fprintf (par_File, "\t\t%.*s %.*s = %g\n", TypeLen, Type, TypeLen, Name, (double)*(const float*)(const void*)(par_Data + PosInsideStruct));
                PosInsideStruct += sizeof(float);
            } else if (!strncmp("char", Type, (size_t)TypeLen) && !strncmp("Data[1]", Name, (size_t)NameLen)) {
                // this can be at the end of the structure
                // wird nicht ausgegeben!
            } else {
                fprintf (par_File, "unknown type \"%.*s\" name \"%.*s\" stop decoding\n", TypeLen, Type, TypeLen, Name);
                break;  // do/while
            }

        }
    } while (*p != 0);

}

int RemoteProcedureDecodeMessage(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *In, RPC_API_BASE_MESSAGE_ACK *Out)
{
    int Ret;
    if ((In->Command < 0) && (In->Command >= MAX_FUNCTION_TABLE_SIZE)) {
        ThrowError (1, "Remote procedure call message for command %i out range (0...%i)",
               In->Command, (int)MAX_FUNCTION_TABLE_SIZE);
        return -1;
    };
    if ((In->StructSize < FunctionCallTable[In->Command].MinMessageSize) && (In->StructSize > FunctionCallTable[In->Command].MaxMessageSize)) {
        ThrowError (1, "Remote procedure call message for command %i are larger/smaller as expected (%i/%i)",
               In->Command, FunctionCallTable[In->Command].MaxMessageSize, FunctionCallTable[In->Command].MinMessageSize);
        return -1;
    }
    if (FunctionCallTable[In->Command].FunctionCallPtr == NULL) {
        ThrowError (1, "Unknown remote procedure call message for command %i",
               In->Command);
        return -1;
    }
    if((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_NAME) == DEBUG_PRINT_COMMAND_NAME) {
        if((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_ONE_FILE) == DEBUG_PRINT_COMMAND_ONE_FILE) {
            fprintf (par_Connection->DebugFile, "%i %" PRIu64 ": %s\n", par_Connection->ConnectionNr, GetCycleCounter64(), FunctionCallTable[In->Command].CommandString);
        } else {
            fprintf (par_Connection->DebugFile, "%" PRIu64 ": %s\n", GetCycleCounter64(), FunctionCallTable[In->Command].CommandString);
        }    
        if((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_PARAMETER_AS_HEX) == DEBUG_PRINT_COMMAND_PARAMETER_AS_HEX) {
            PrintHexToDebugFile(par_Connection->DebugFile, "InHex", (unsigned char*)In, In->StructSize);
        }
        if((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_PARAMETER_DECODED) == DEBUG_PRINT_COMMAND_PARAMETER_DECODED) {
            PrintDecodedToDebugFile(par_Connection->DebugFile, "InDecoded", GetReqStructDefString(FunctionCallTable[In->Command].CommandString), (unsigned char*)In, In->StructSize);
        }
        if ((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_FFLUSH) == DEBUG_PRINT_COMMAND_FFLUSH) {
            fflush(par_Connection->DebugFile);
        }
    }

    Ret = FunctionCallTable[In->Command].FunctionCallPtr(par_Connection, In, Out);

    if((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_NAME) == DEBUG_PRINT_COMMAND_NAME) {
        if (Ret > 0) {
            if((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_RETURN_AS_HEX) == DEBUG_PRINT_COMMAND_RETURN_AS_HEX) {
                PrintHexToDebugFile(par_Connection->DebugFile, "OutHex", (unsigned char*)Out, Out->StructSize);
            }

            if((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_RETURN_DECODED) == DEBUG_PRINT_COMMAND_RETURN_DECODED) {
                PrintDecodedToDebugFile(par_Connection->DebugFile, "OutDecoded", GetAckStructDefString(FunctionCallTable[In->Command].CommandString), (unsigned char*)Out, Out->StructSize);
            }
            if((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_FFLUSH) == DEBUG_PRINT_COMMAND_FFLUSH) {
                fflush(par_Connection->DebugFile);
            }
        } else {
            fprintf (par_Connection->DebugFile, "  no ACK transmitted\n");
        }
    }
    return Ret;
}

static int SyncWithGUI(RPC_CONNECTION *par_Connection, const RPC_API_BASE_MESSAGE *In, RPC_API_BASE_MESSAGE_ACK *Out)
{
    return RemoteProcedureCallRequestFromOtherThread(par_Connection, In, Out);
}

static void *RemoteProcedureGetReceiveBuffer(RPC_CONNECTION *par_Connection, int par_NeededSize)
{
    if (par_NeededSize > par_Connection->ReceiveBufferSize) {
        par_Connection->ReceiveBufferSize = par_NeededSize;
        par_Connection->ReceiveBuffer = realloc (par_Connection->ReceiveBuffer, (size_t)par_Connection->ReceiveBufferSize);
        if (par_Connection->ReceiveBuffer == NULL) {
            par_Connection->ReceiveBufferSize = 0;
        }
    }
    return par_Connection->ReceiveBuffer;
}

static void *RemoteProcedureGetTransmitBuffer(RPC_CONNECTION *par_Connection, int par_NeededSize)
{
    if (par_NeededSize > par_Connection->TransmitBufferSize) {
        par_Connection->TransmitBufferSize = par_NeededSize;
        par_Connection->TransmitBuffer = realloc (par_Connection->TransmitBuffer, (size_t)par_Connection->TransmitBufferSize);
        if (par_Connection->TransmitBuffer == NULL) {
            par_Connection->TransmitBufferSize = 0;
        }
    }
    return par_Connection->TransmitBuffer;
}

static void SetupConnection(int par_ConnectionNr, uint64_t par_Socket, uint8_t par_ConnectionType)
{
    char OptionString[3];

    memset(&(Connections[par_ConnectionNr]), 0, sizeof(Connections[0]));
    Connections[par_ConnectionNr].UsedFlag = 1;
    Connections[par_ConnectionNr].ConnectionType = par_ConnectionType;
    Connections[par_ConnectionNr].WaitToReceiveAlivePingAck = 0;
    Connections[par_ConnectionNr].Socket = par_Socket;
    Connections[par_ConnectionNr].ConnectionNr = par_ConnectionNr;

    InitializeCriticalSection (&(Connections[par_ConnectionNr].CriticalSection));
    InitializeConditionVariable (&(Connections[par_ConnectionNr].ConditionVariable));

    // Die Flags sind fuer alle Connections gleich
    Connections[par_ConnectionNr].DebugFlags =  s_main_ini_val.RpcDebugLoggingFlags;

    if (Connections[par_ConnectionNr].DebugFlags) {
        char DebugFileName[MAX_PATH];
        char TempPath[MAX_PATH];
#ifdef _WIN32
        GetTempPath(sizeof(TempPath), TempPath);
#else
        strcpy(TempPath, "/tmp/");
#endif
        if ((Connections[par_ConnectionNr].DebugFlags & DEBUG_PRINT_COMMAND_ATTACH) == DEBUG_PRINT_COMMAND_ATTACH) {
            strcpy(OptionString, "at");
        } else {
            strcpy(OptionString, "wt");
        }
        if ((Connections[par_ConnectionNr].DebugFlags & DEBUG_PRINT_COMMAND_ONE_FILE) == DEBUG_PRINT_COMMAND_ONE_FILE) {
            StringCopyMaxCharTruncate(DebugFileName, TempPath, sizeof(DebugFileName));
            StringAppendMaxCharTruncate(DebugFileName, "RPC_Log.log", sizeof(DebugFileName));
        } else {
            char Txt[16];
            sprintf (Txt, "%i", par_ConnectionNr);
            StringCopyMaxCharTruncate(DebugFileName, TempPath, sizeof(DebugFileName));
            StringAppendMaxCharTruncate(DebugFileName, "RPC_Log_", sizeof(DebugFileName));
            StringAppendMaxCharTruncate(DebugFileName, Txt, sizeof(DebugFileName));
            StringAppendMaxCharTruncate(DebugFileName, ".log", sizeof(DebugFileName));
        }
        if (((Connections[par_ConnectionNr].DebugFlags & DEBUG_PRINT_COMMAND_ONE_FILE) == DEBUG_PRINT_COMMAND_ONE_FILE) && (LogFileHandle != NULL)) {
            Connections[par_ConnectionNr].DebugFile = LogFileHandle;  // Nimm das File von Connection 0
            LogFileHandleAttachCounter++;
        }else {
            Connections[par_ConnectionNr].DebugFile = open_file (DebugFileName, OptionString);
            if (Connections[par_ConnectionNr].DebugFile == NULL) {
                Connections[par_ConnectionNr].DebugFlags = 0;
                ThrowError (1, "cannot write log file \"%s\n", DebugFileName);
            }
            if ((Connections[par_ConnectionNr].DebugFlags & DEBUG_PRINT_COMMAND_ONE_FILE) == DEBUG_PRINT_COMMAND_ONE_FILE) {
                LogFileHandle = Connections[par_ConnectionNr].DebugFile;
                LogFileHandleAttachCounter = 1;
            }
        }
    }
}

static void FreeConnection(RPC_CONNECTION *par_Connection)
{
    int x;
    EnterCriticalSection(&CriticalSection);
    par_Connection->UsedFlag = 0;
    if (par_Connection->ReceiveBuffer != NULL) free (par_Connection->ReceiveBuffer);
    if (par_Connection->TransmitBuffer != NULL) free (par_Connection->TransmitBuffer);
    for (x = 0; x < 4; x++) {
        if (par_Connection->CCPVariable[x] != NULL) free (par_Connection->CCPVariable[x]);
        if (par_Connection->CCPParameter[x] != NULL) free (par_Connection->CCPParameter[x]);
        if (par_Connection->XCPVariable[x] != NULL) free (par_Connection->XCPVariable[x]);
        if (par_Connection->XCPParameter[x] != NULL) free (par_Connection->XCPParameter[x]);
    }
    if (par_Connection->DebugFile !=NULL) {
        fprintf (par_Connection->DebugFile, "\nclose connection %i\n", par_Connection->ConnectionNr);
        if ((par_Connection->DebugFlags & DEBUG_PRINT_COMMAND_ONE_FILE) == DEBUG_PRINT_COMMAND_ONE_FILE) {
            LogFileHandleAttachCounter--;
            if (LogFileHandleAttachCounter == 0) {
                fprintf (par_Connection->DebugFile, "\nall connections closed, file closed\n\n");
                close_file(LogFileHandle);
                LogFileHandle = NULL;
            }
        } else {
            fprintf (par_Connection->DebugFile, "\nfile closed\n\n");
            close_file(par_Connection->DebugFile);
        }
    }

    DeleteCriticalSection(&(par_Connection->CriticalSection));
    // das gibt es nicht: DeleteConditionVariable (&(Connection->ConditionVariable));
    memset (par_Connection, 0, sizeof(*par_Connection));
    LeaveCriticalSection(&CriticalSection);
}


static int ReceiveFromRemoteProcedureCallDynBuf (RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE **ret_ptrToData)
{
    char *dyn_buffer;
    int buffer_pos = 0;
    int receive_message_size = 0;

    dyn_buffer = (char*)RemoteProcedureGetReceiveBuffer(par_Connection, sizeof(RPC_API_BASE_MESSAGE));
    if (dyn_buffer == NULL) return -1;

    // ersmal nur Header lesen
    do {
        int ReadBytes = recv (par_Connection->Socket, dyn_buffer + buffer_pos, (int)sizeof(RPC_API_BASE_MESSAGE) - buffer_pos, 0);
        if (ReadBytes > 0) {
            buffer_pos += ReadBytes;
            if (receive_message_size == 0) {
                receive_message_size = (int)*(int32_t*)(void*)dyn_buffer;
            }
        } else if (ReadBytes == 0) {
            fprintf(stderr, "Connection closed\n");
            return 0;
        } else {
#ifdef _WIN32
            ThrowError (1, "recv failed with error: %d\n", WSAGetLastError());
#else
            ThrowError (1, "recv failed with error: %d\n", errno);
#endif
            return 0;
        }
    } while (buffer_pos < (int)sizeof(RPC_API_BASE_MESSAGE));

    // Rest der Message in den dynamischen Teil
    dyn_buffer = (char*)RemoteProcedureGetReceiveBuffer(par_Connection, receive_message_size);
    if (dyn_buffer == NULL) return -1;

    buffer_pos = sizeof(RPC_API_BASE_MESSAGE);  // das sollte eh so sein!?

    while (buffer_pos < receive_message_size) {
        int ReadBytes = recv (par_Connection->Socket, dyn_buffer + buffer_pos, receive_message_size - buffer_pos, 0);
        if (ReadBytes > 0) {
            buffer_pos += ReadBytes;
        } else if (ReadBytes == 0) {
            fprintf(stderr, "Connection closed\n");
            return 0;
        } else {
#ifdef _WIN32
            ThrowError(1, "recv failed with error: %d\n", WSAGetLastError());
#else
            ThrowError(1, "recv failed with error: %d\n", errno);
#endif
            return 0;
        }
    }
    *ret_ptrToData = (RPC_API_BASE_MESSAGE*)(void*)dyn_buffer;
    return receive_message_size;
}

#ifdef _WIN32
static DWORD WINAPI RemoteProcedureCallSocketServerThreadFunction (LPVOID lpParam)
#else
static void* RemoteProcedureCallSocketServerThreadFunction (void* lpParam)
#endif
{
    int ShouldStop = 0;
    int ToTransmitDataLen;
    RPC_API_BASE_MESSAGE *Receive;
    RPC_API_BASE_MESSAGE_ACK *Transmit;
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)lpParam;

    // ert mal sicher stellen, dass der Prozess laeft!
    if (get_pid_by_name ("RPC_Control") < 0) {
        start_process("RPC_Control");
    }
    // erst mal einen grossen Transmit Puffer anlegen!
    Transmit = RemoteProcedureGetTransmitBuffer(Connection, RPC_MESSAGE_BUFSIZE);
    while (ReceiveFromRemoteProcedureCallDynBuf(Connection, &Receive)) {
        if (MustSyncWithGUI(Receive)) {
            ToTransmitDataLen = SyncWithGUI(Connection, Receive, Transmit);
        } else {
            ToTransmitDataLen = RemoteProcedureDecodeMessage(Connection, Receive, Transmit);
            /*if (ToTransmitDataLen == CONNECTION_SHOULD_BE_CLOSED) {
                break;   // keine Antwort auf Command RPC_API_LOGOUT_CMD
            }*/
        }
        if (ToTransmitDataLen < 0) {
            ShouldStop = 1;
            ToTransmitDataLen = -ToTransmitDataLen;
        }
        if (ToTransmitDataLen >= (int)sizeof(RPC_API_BASE_MESSAGE_ACK)) {
            send((SOCKET)Connection->Socket, (char*)Transmit, ToTransmitDataLen, 0);
        }
        if (ShouldStop) break;
    }
#ifdef _WIN32
    closesocket((SOCKET)Connection->Socket);
#else
    close((SOCKET)Connection->Socket);
#endif
    RemoveAllPendingStartStopSchedulerRequestsOfThisThread(Connection->SchedulerDisableCounter);
    CleanUpWaitFuncForConnection(Connection);
    if (Connection->CanFifoHandle > 0) {
        DeleteCanFifos(Connection->CanFifoHandle);
        Connection->CanFifoHandle = 0;
    }
    FreeConnection(Connection);
    if (ShouldStop) {
        ExecuteShouldExit();
    }

    CloseSocketToRemoteMasterForThisThead();

    return 0;  // exit Thread
}


static SOCKET CreateRemoteProcedureSocket (int par_Port)
{
    // Two socket descriptors which are just integer numbers used to access a socket
    SOCKET sock_descriptor;

    // Two socket address structures - One for the server itself and the other for client
    struct sockaddr_in serv_addr;

#ifdef _WIN32
    int iResult;
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        ThrowError (1, "WSAStartup failed with error: %d\n", iResult);
        return INVALID_SOCKET;
    }
#endif

    // Create socket of domain - Internet (IP) address, type - Stream based (TCP) and protocol unspecified
    // since it is only useful when underlying stack allows more than one protocol and we are choosing one.
    // 0 means choose the default protocol.
    sock_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    // A valid descriptor is always a positive value
#ifdef _WIN32
    if (sock_descriptor == INVALID_SOCKET) {
#else
    if (sock_descriptor <= 0) {
#endif
        ThrowError (1, "Failed creating socket");
#ifdef _WIN32
        return INVALID_SOCKET;
#else
        return -1;
#endif
    }

    {
        int optval = 1;
        setsockopt(sock_descriptor, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof (optval));
    }

    // Initialize the server address struct to zero
    memset((char *)&serv_addr, 0, sizeof(serv_addr));

    // Fill server's address family
    serv_addr.sin_family = AF_INET;

    // Server should allow connections from any ip address
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // 16 bit port number on which server listens
    // The function htons (host to network short) ensures that an integer is interpretted
    // correctly (whether little endian or big endian) even if client and server have different architectures
    serv_addr.sin_port = htons((unsigned short)par_Port);

    // Attach the server socket to a port. This is required only for server since we enforce
    // that it does not select a port randomly on it's own, rather it uses the port specified
    // in serv_addr struct.
    if (bind(sock_descriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        ThrowError (1, "Failed to bind");
#ifdef _WIN32
        return INVALID_SOCKET;
#else
        return -1;
#endif
    }

    // Server should start listening - This enables the program to halt on accept call (coming next)
    // and wait until a client connects. Also it specifies the size of pending connection requests queue
    // i.e. in this case it is 5 which means 5 clients connection requests will be held pending while
    // the server is already processing another connection request.
    listen(sock_descriptor, 5);

    return sock_descriptor;
}

#ifdef _WIN32
static int NamedPipeReceiveFromRemoteProcedureCallDynBuf (RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE **ret_ptrToData)
{
    char *dyn_buffer;
    int buffer_pos = 0;
    int receive_message_size = 0;

    dyn_buffer = (char*)RemoteProcedureGetReceiveBuffer(par_Connection, sizeof(RPC_API_BASE_MESSAGE));
    if (dyn_buffer == NULL) return -1;

    // ersmal nur Header lesen
    do {
        BOOL Ret;
        DWORD BytesRead;
        Ret = ReadFile ((HANDLE)par_Connection->Socket, dyn_buffer + buffer_pos, (DWORD)sizeof(RPC_API_BASE_MESSAGE) - (DWORD)buffer_pos, &BytesRead, NULL);
        if ((Ret != 0) && (BytesRead > 0)) {
            buffer_pos += BytesRead;
            if (receive_message_size == 0) {
                receive_message_size = (int)*(uint32_t*)(void*)dyn_buffer;
            }
        } else if ((Ret == 0) || (BytesRead == 0)) {
            fprintf(stderr, "Connection closed\n");
            return 0;
        } else {
            fprintf(stderr, "ReadFile failed with error: %d\n", (int)GetLastError());
            return 0;
        }
    } while (buffer_pos < (int)sizeof(RPC_API_BASE_MESSAGE));

    // Rest der Message in den dynamischen Teil
    dyn_buffer = (char*)RemoteProcedureGetReceiveBuffer(par_Connection, receive_message_size);
    if (dyn_buffer == NULL) return -1;

    buffer_pos = sizeof(RPC_API_BASE_MESSAGE);  // das sollte eh so sein!?

    while (buffer_pos < receive_message_size) {
        BOOL Ret;
        DWORD BytesRead;
        Ret = ReadFile ((HANDLE)par_Connection->Socket, dyn_buffer + buffer_pos, (DWORD)(receive_message_size - buffer_pos), &BytesRead, NULL);
        if ((Ret != 0) && (BytesRead > 0)) {
            buffer_pos += BytesRead;
        } else if (BytesRead == 0) {
            fprintf(stderr, "Connection closed\n");
            return 0;
        } else {
            fprintf(stderr, "ReadFile failed with error: %d\n", (int)GetLastError());
            return 0;
        }
    }
    *ret_ptrToData = (RPC_API_BASE_MESSAGE*)(void*)dyn_buffer;
    return receive_message_size;
}


static DWORD WINAPI NamedPipeRemoteProcedureCallServerThreadFunction (LPVOID lpParam)
{
    int ShouldStop = 0;
    int ToTransmitDataLen;
    RPC_API_BASE_MESSAGE *Receive;
    RPC_API_BASE_MESSAGE_ACK *Transmit;
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)lpParam;

    // ert mal sicher stellen, dass der Prozess laeft!
    if (get_pid_by_name ("RPC_Control") < 0) {
        start_process("RPC_Control");
    }
    // erst mal einen grossen Transmit Puffer anlegen!
    Transmit = RemoteProcedureGetTransmitBuffer(Connection, RPC_MESSAGE_BUFSIZE);
    while (NamedPipeReceiveFromRemoteProcedureCallDynBuf(Connection, &Receive)) {
        if (MustSyncWithGUI(Receive)) {
            ToTransmitDataLen = SyncWithGUI(Connection, Receive, Transmit);
        } else {
            ToTransmitDataLen = RemoteProcedureDecodeMessage(Connection, Receive, Transmit);
            /*if (ToTransmitDataLen == CONNECTION_SHOULD_BE_CLOSED) {
                break;   // keine Antwort auf Command RPC_API_LOGOUT_CMD
            }*/
        }
        if (ToTransmitDataLen < 0) {
            ShouldStop = 1;
            ToTransmitDataLen = -ToTransmitDataLen;
        }
        if (ToTransmitDataLen >= (int)sizeof(RPC_API_BASE_MESSAGE_ACK)) {
            DWORD NumberOfBytesWritten;
            BOOL Ret;
            Ret = WriteFile((HANDLE)Connection->Socket, Transmit, (DWORD)ToTransmitDataLen, &NumberOfBytesWritten, NULL);
        }
        if (ShouldStop) break;
    }
    RemoveAllPendingStartStopSchedulerRequestsOfThisThread(Connection->SchedulerDisableCounter);
    CleanUpWaitFuncForConnection(Connection);
    if (Connection->CanFifoHandle > 0) {
        DeleteCanFifos(Connection->CanFifoHandle);
        Connection->CanFifoHandle = 0;
    }
    FlushFileBuffers((HANDLE)Connection->Socket);
    DisconnectNamedPipe ((HANDLE)Connection->Socket);
    CloseHandle((HANDLE)Connection->Socket);

    FreeConnection(Connection);

    if (ShouldStop) {
        ExecuteShouldExit();
    }

    CloseSocketToRemoteMasterForThisThead();

    return 0;  // exit Thread
}


static HANDLE CreateRemoteProcedureNamedPipe (char *par_Prefix)
{
    HANDLE hPipe;
    char Pipename[MAX_PATH];

    sprintf (Pipename, XILENV_RPC_PIPE_NAME "_%s", par_Prefix);

    hPipe = CreateNamedPipe (Pipename, PIPE_ACCESS_DUPLEX, PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, PIPE_MESSAGE_BUFSIZE, PIPE_MESSAGE_BUFSIZE, 0, NULL);

    // Break if the pipe handle is valid.
    if (hPipe == INVALID_HANDLE_VALUE) {
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "cannot create named pipe \"%s\" (%s)\n", Pipename, lpMsgBuf);
        LocalFree (lpMsgBuf);
        return INVALID_HANDLE_VALUE;
    }
    return hPipe;
}
#endif

#ifdef _WIN32
static uint64_t ConvertHandleToUint64 (HANDLE In)
{
    union {
        HANDLE In;
        uint64_t Out;
    } x;
    x.In = In;
    return x.Out;
}
#endif

static SOCKET SocketToClose;

#ifdef _WIN32
int RemoteProcedureServerLoginThread (LPVOID lpParam)
#else
void* RemoteProcedureServerLoginThread (void* lpParam)
#endif
{
    SOCKET Socket;
    char Instance[MAX_PATH];
    char *p;
    int Port;
    int SocketOrNamedPipe;

    InitializeCriticalSection (&CriticalSection);
    InitializeConditionVariable (&ConditionVariable);


    SocketOrNamedPipe = strtol( (char*)lpParam, &p, 0);
    if (*p != ':') {
        ThrowError(1, "expected ':' %s (%i)", __FILE__, __LINE__);
#ifdef _WIN32
        return -1;
#else
        return NULL;
#endif
    }
    p++;  // jump over ':'
    strcpy (Instance, p);
    p = strstr(Instance, "@");
    if (p != NULL) {
        Port = atoi (p + 1);  // a port was defined
        *p = 0;
    } else {
        Port = 1810;  // use default port
    }

    AddLoginFunctionToTable();
    AddMiscFunctionToTable();
    AddSchedFunctionToTable();
    AddInternProcessFunctionToTable();
    AddGuiFunctionToTable();
    AddBlackboardFunctionToTable();
    AddCalibrationFunctionToTable();
    AddCanFunctionToTable();
    AddCcpFunctionToTable();
    AddXcpFunctionToTable();

    NewConnectionsAllowed = 1;

    if (SocketOrNamedPipe) {
        // Sockets
        SocketToClose = Socket = CreateRemoteProcedureSocket(Port);
#ifdef _WIN32
        if (Socket == INVALID_SOCKET) return 1;
#else
        if (Socket < 0) return NULL;
#endif
        for (;;) {  // Endless loop
            struct sockaddr_in client_addr;
#ifdef _WIN32
            int size;
#else
            socklen_t size;
#endif
            size = sizeof(client_addr);
            SOCKET conn_desc;

            // Server blocks on this call until a client tries to establish connection.
            // When a connection is established, it returns a 'connected socket descriptor' different
            // from the one created earlier.
            conn_desc = accept(Socket, (struct sockaddr *)&client_addr, &size);
#ifdef _WIN32
            if (conn_desc == INVALID_SOCKET) {
#else
            if (conn_desc < 0) {
#endif
                EnterCriticalSection (&CriticalSection);
                if (!NewConnectionsAllowed) {
#ifdef _WIN32
                    closesocket(Socket);
#else
                    close(Socket);
#endif
                    NewConnectionsAllowed = 2;
                    LeaveCriticalSection(&CriticalSection);
                    WakeAllConditionVariable(&ConditionVariable);
                    break; // for(;;)
                }
                LeaveCriticalSection(&CriticalSection);
                if (NewConnectionsAllowed != 2) ThrowError (1, "Failed accepting connection\n");
            } else {
                int x;
                EnterCriticalSection(&CriticalSection);
                for (x = 0; x < RPC_MAX_CONNECTIONS; x++) {
                    if (Connections[x].UsedFlag == 0) {
#ifdef _WIN32
                        DWORD ThreadId;
#else
                        pthread_t Thread;
                        pthread_attr_t Attr;
#endif


                        SetupConnection(x, (uint64_t)conn_desc, 1);
#ifdef _WIN32
                        Connections[x].Thread = ConvertHandleToUint64(CreateThread (NULL,              // no security attribute
                                                                        0,                 // default stack size
                                                                        RemoteProcedureCallSocketServerThreadFunction,    // thread proc
                                                                        (void*)&(Connections[x]),    // thread parameter
                                                                        0,                 // not suspended
                                                                        &ThreadId));      // returns thread ID
                        Connections[x].ThreadId = ThreadId;
#else
                        pthread_attr_init(&Attr);
                        if (pthread_create(&Thread, &Attr, RemoteProcedureCallSocketServerThreadFunction, (void*)&(Connections[x])) != 0) {
                            ThrowError (1, "cannot create remote procedure call thread\n");
                            return NULL;
                        }
                        Connections[x].ThreadId = Thread;
                        pthread_attr_destroy(&Attr);
#endif

                        break;
                    }
                }
                LeaveCriticalSection(&CriticalSection);
                if (x == RPC_MAX_CONNECTIONS) {
                    ThrowError (1, "Too many remote connection max. %i allowed\n", (int)RPC_MAX_CONNECTIONS);
                }
            }
        }
    } else {
#ifdef _WIN32
        // Named pipes
        for (;;) {  // endless loop
            HANDLE hPipe;
            DWORD Connected;
            if ((hPipe = CreateRemoteProcedureNamedPipe (Instance)) != INVALID_HANDLE_VALUE) {
                Connected = ConnectNamedPipe (hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
                EnterCriticalSection (&CriticalSection);
                if (!NewConnectionsAllowed) {
                    CloseHandle(hPipe);
                    NewConnectionsAllowed = 2;
                    LeaveCriticalSection(&CriticalSection);
                    WakeAllConditionVariable(&ConditionVariable);
                    break; // for(;;)
                }
                LeaveCriticalSection(&CriticalSection);
                if (Connected) {
                    int x;
                    EnterCriticalSection(&CriticalSection);
                    for (x = 0; x < RPC_MAX_CONNECTIONS; x++) {
                        if (Connections[x].UsedFlag == 0) {
                            DWORD ThreadId;

                            SetupConnection(x, (uint64_t)hPipe, 0);

                            Connections[x].Thread = ConvertHandleToUint64(CreateThread (NULL,              // no security attribute
                                                                            0,                 // default stack size
                                                                            NamedPipeRemoteProcedureCallServerThreadFunction,    // thread proc
                                                                            &(Connections[x]),    // thread parameter
                                                                            0,                 // not suspended
                                                                            &ThreadId));      // returns thread ID
                            Connections[x].ThreadId = ThreadId;
                            break;
                        }
                    }
                    LeaveCriticalSection(&CriticalSection);
                    if (x == RPC_MAX_CONNECTIONS) {
                        ThrowError (1, "Too many remote connection max. %i allowed\n", (int)RPC_MAX_CONNECTIONS);
                    }
                }
            }
        }
#else
        //ThrowError (1, "named pipes are only supported with windows");
#endif
    }
    return 0;
}

int AddFunctionToRemoteAPIFunctionTable_ (int par_Command, char *par_CommandString, int par_HaveToSyncWithGUI,
                                          int (*par_FunctionCallPtr)(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *In, RPC_API_BASE_MESSAGE_ACK *Out),
                                          int par_MinMessageSize, int par_MaxMessageSize)
{
    if (par_Command >= MAX_FUNCTION_TABLE_SIZE) {
        ThrowError (1, "Internal error %s (%i) %i \"%s\" > %i", __FILE__, __LINE__, par_Command, par_CommandString, MAX_FUNCTION_TABLE_SIZE);
        return -1;
    }
    if (FunctionCallTable[par_Command].Command != 0) {
        ThrowError (1, "Internal error %s (%i) command %i \"%s\" already in use", __FILE__, __LINE__, par_Command, par_CommandString);
        return -1;
    }
    FunctionCallTable[par_Command].Command = par_Command;
    FunctionCallTable[par_Command].FunctionCallPtr = par_FunctionCallPtr;
    FunctionCallTable[par_Command].HaveToSyncWithGUI = par_HaveToSyncWithGUI;
    FunctionCallTable[par_Command].MinMessageSize = par_MinMessageSize;
    FunctionCallTable[par_Command].MaxMessageSize = par_MaxMessageSize;
    FunctionCallTable[par_Command].CommandString = par_CommandString;
    return 0;
}

int StartRemoteProcedureCallThread (int par_SocketOrNamedPipe, char *par_Prefix, int par_Port)
{
#ifdef _WIN32
    HANDLE hThread;
    DWORD dwThreadId = 0;
#else
    pthread_t Thread;
    pthread_attr_t Attr;
#endif
    static char StaticInstanceName[MAX_PATH];

    // This have to be copied to a static variable because it will be accessed later fron the created thread
    sprintf (StaticInstanceName, "%i:%s@%i", par_SocketOrNamedPipe, par_Prefix, par_Port);

#ifdef _WIN32
    NewConnectionThread =
    hThread = CreateThread (
            NULL,              // no security attribute
            0,                 // default stack size
            (LPTHREAD_START_ROUTINE)RemoteProcedureServerLoginThread,    // thread proc
            StaticInstanceName,    // thread parameter
            0,                 // not suspended
            &dwThreadId);      // returns thread ID

    if (hThread == NULL)  {
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "cannot create remote procedure call thread (%s)\n", lpMsgBuf);
        LocalFree (lpMsgBuf);
        return -1;
    }
#else
    pthread_attr_init(&Attr);
    if (pthread_create(&Thread, &Attr, RemoteProcedureServerLoginThread, (void*)StaticInstanceName) != 0) {
        ThrowError (1, "cannot create remote procedure call thread\n");
        return -1;
    }
    NewConnectionThread = Thread;
    pthread_attr_destroy(&Attr);
#endif
    return 0;
}

void RemoteProcedureWakeWaitForConnection(RPC_CONNECTION *par_Connection)
{
    EnterCriticalSection (&(par_Connection->CriticalSection));
    par_Connection->WaitFlag = 0;
    LeaveCriticalSection (&(par_Connection->CriticalSection));
    WakeAllConditionVariable(&(par_Connection->ConditionVariable));
}

void RemoteProcedureWaitForConnection(RPC_CONNECTION *par_Connection)
{
    EnterCriticalSection (&(par_Connection->CriticalSection));
    while (par_Connection->WaitFlag) {
        SleepConditionVariableCS(&(par_Connection->ConditionVariable), &(par_Connection->CriticalSection), 1000);
        if (par_Connection->WaitFlag) {
            if (par_Connection->WaitToReceiveAlivePingAck) {
               par_Connection->WaitToReceiveAlivePingAck = 0;
               RPC_API_PING_TO_CLINT_MESSAGE_ACK Ack;
               if (par_Connection->ConnectionType) {
                   int ReadBytes = recv (par_Connection->Socket, (char*)&Ack, sizeof(Ack), 0);
                   if ((ReadBytes != sizeof(Ack)) ||
                       (Ack.Header.Command != RPC_API_PING_TO_CLINT_CMD)) {
                       par_Connection->WaitFlag = 0;  // no Ack the connection are closed or broken
                   }
               } else {
                   DWORD BytesRead;
                   if (!ReadFile ((HANDLE)par_Connection->Socket, (char*)&Ack, (DWORD)sizeof(Ack), &BytesRead, NULL) ||
                       (BytesRead != sizeof(Ack)) ||
                       (Ack.Header.Command != RPC_API_PING_TO_CLINT_CMD)) {
                       par_Connection->WaitFlag = 0;  // no Ack the connection are closed or broken
                   }
               }
            } else {
                // send a alive ping
                RPC_API_PING_TO_CLINT_MESSAGE Req;
                memset(&Req, 0, sizeof(Req));
                Req.Header.Command = RPC_API_PING_TO_CLINT_CMD;
                Req.Header.StructSize = sizeof(Req);
                if (par_Connection->ConnectionType) {
                    send((SOCKET)par_Connection->Socket, (char*)&Req, sizeof(Req), 0);
                } else {
                    DWORD NumberOfBytesWritten;
                    WriteFile((HANDLE)par_Connection->Socket, (char*)&Req, (DWORD)sizeof(Req), &NumberOfBytesWritten, NULL);
                }
                par_Connection->WaitToReceiveAlivePingAck = 1;
            }
        }
    }
    LeaveCriticalSection (&(par_Connection->CriticalSection));
}

void RemoteProcedureMarkedForWaitForConnection(RPC_CONNECTION *par_Connection)
{
    EnterCriticalSection (&(par_Connection->CriticalSection));
    par_Connection->WaitFlag = 1;
    LeaveCriticalSection (&(par_Connection->CriticalSection));
}

void StopAcceptingNewConnections(RPC_CONNECTION *par_Connection)
{
    UNUSED(par_Connection);
    EnterCriticalSection (&CriticalSection);
    if (NewConnectionsAllowed == 1) {
        NewConnectionsAllowed = 0;
    #ifdef _WIN32
        // this is neccessary for named pipes
        CancelSynchronousIo(NewConnectionThread);
        closesocket(SocketToClose);
    #else
        //pthread_kill(NewConnectionThread, SIGINT);
        close(SocketToClose);
    #endif
        for (int x = 0; x < 10; x++) {
            if (NewConnectionsAllowed == 2) {
                break;
            } else {
                SleepConditionVariableCS(&ConditionVariable, &CriticalSection, 100);
            }
        }
    }
    LeaveCriticalSection (&CriticalSection);
}

static int ShouldTerminate;
static int SetExitCode;
static int ExitCode;

void SetShouldExit(int par_ShouldTerminate, int par_SetExitCode, int par_ExitCode)
{
    ShouldTerminate = par_ShouldTerminate;
    SetExitCode = par_SetExitCode;
    ExitCode = par_ExitCode;
}

void ExecuteShouldExit(void)
{
    char Name[BBVARI_NAME_SIZE];
    if (SetExitCode) {
        int Vid;
        Vid = get_bbvarivid_by_name(ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD, "ExitCode", Name, sizeof(Name)));
        if (Vid <= 0) {
            ThrowError (1, "cannot attach variable \"%s\"", Name);
        }
        write_bbvari_dword_x(GetGuiPid(), Vid, ExitCode);
    }
    if (ShouldTerminate) {
        int Vid;
        enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
        Vid = get_bbvarivid_by_name(ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD, "exit", Name, sizeof(Name)));
        if (Vid <= 0) {
            ThrowError (1, "cannot attach variable \"%s\"", Name);
        }
        write_bbvari_uword_x(GetGuiPid(), Vid, 1);
    }
}

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

#include <stdlib.h>

#include "Platform.h"
#include <stdio.h>
#include <malloc.h>
#include "Config.h"
#include "CanFifo.h"

#include "SharedDataTypes.h"
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
#include "RpcFuncClientA2lLinks.h"
#include "RpcClientSocket.h"
#include "MemZeroAndCopy.h"
#include "StringMaxChar.h"

#ifdef __linux__ 
    //linux code goes here
    #define ALLOCA(size) alloca(size)
    #define FREEA(size) 
    #define THREAD_LOCAL  __thread
    #define NULL_OR_0   0
#elif _WIN32
    // windows code goes here
    #define ALLOCA(size) _malloca(size)
    #define FREEA(size) _freea(size)
    #define THREAD_LOCAL __declspec(thread)
    #define NULL_OR_0   NULL
#else
    #error "no target defined"
#endif

static HANDLE Socket
#ifdef _WIN32
    = INVALID_HANDLE_VALUE;
#else
    = 0;
#endif
static int SocketOrNamedPipe 
#ifdef _WIN32
    = 0;
#else
    = 1;
#endif

#define UNUSED(x) (void)(x)

#ifdef _USRDLL
#ifdef _WIN32
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
#endif
#else
#ifndef _LIB
#error es muss entweder _USRDLL oder _LIB definiert sein
#endif
#endif


SCRPCDLL_API int __STDCALL__ XilEnv_GetAPIVersion(void)
{
    return (int)XILENV_VERSION;
}

SCRPCDLL_API char * __STDCALL__ XilEnv_GetAPIModulePath(void)
{
#ifdef _WIN32
    static char Ret[MAX_PATH];
    HMODULE hModule;

#ifdef SCRPCDLL_EXPORTS
#ifdef _WIN64
    hModule = GetModuleHandle ("XilEnvRpc.dll");
#else
    hModule = GetModuleHandle ("XilEnvRpc.dll");
#endif
#else
    hModule = GetModuleHandle ("_sc2py.dll");
#endif
    GetModuleFileName (hModule, Ret, sizeof (Ret));
    return Ret;
#else
    return "not implemented";
#endif
}

SCRPCDLL_API int __STDCALL__ XilEnv_GetVersion(void)
{
    RPC_API_GET_VERSION_MESSAGE Req = {0};
    RPC_API_GET_VERSION_MESSAGE_ACK Ack;
    int Ret;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_VERSION_CMD, &Req.Header, sizeof(Req), &Ack.Header, sizeof(Ack));
    if (Ret != sizeof(Ack)) {
        return -1;
    }
    return Ack.Version;
}


SCRPCDLL_API int __STDCALL__ XilEnv_DisconnectFrom(void)
{
    if ((Socket != INVALID_HANDLE_VALUE) && (Socket != NULL_OR_0)) {
        RPC_API_LOGOUT_MESSAGE Req = {0};
        RPC_API_GET_VERSION_MESSAGE_ACK Ack;
        int Ret;

        Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOGOUT_CMD, &Req.Header, sizeof(Req), &Ack.Header, sizeof(Ack));
        DisconnectFromRemoteProcedureCallServer(SocketOrNamedPipe, Socket);
        Socket = INVALID_HANDLE_VALUE;
    }
    return 0;
}

SCRPCDLL_API int __STDCALL__ XilEnv_DisconnectAndClose(int SetErrorLevelFlag, int ErrorLevel)
{
    if ((Socket != INVALID_HANDLE_VALUE) && (Socket != NULL_OR_0)) {
        RPC_API_SHOULD_BE_TERMINATED_MESSAGE Req1 = {0};
        RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK Ack1;
        RPC_API_LOGOUT_MESSAGE Req2 = {0};
        int Ret;
    
        Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SHOULD_BE_TERMINATED_CMD, &Req1.Header, sizeof(Req1), &Ack1.Header, sizeof(Ack1));
        if (Ret != sizeof(Ack1)) {
            return -1;
        }
        Req2.ShouldTerminate = 1;
        Req2.SetExitCode = SetErrorLevelFlag;
        Req2.ExitCode = ErrorLevel;
        Ret = SentToRemoteProcedureCallServer(SocketOrNamedPipe, Socket, RPC_API_LOGOUT_CMD, &Req2.Header, sizeof(Req2));
        DisconnectFromRemoteProcedureCallServer(SocketOrNamedPipe, Socket);
        Socket = INVALID_HANDLE_VALUE;
    }
    return 0;
}

SCRPCDLL_API int __STDCALL__ XilEnv_ConnectToInstance(const char* NetAddr, const char *InstanceName)
{
    int Ret;
    char *p;
    RPC_API_LOGIN_MESSAGE Req = {0};
    RPC_API_LOGIN_MESSAGE_ACK Ack;


    if (XilEnv_IsConnectedTo()) {
        return -1;  // already connected
    }
    if (InstanceName == NULL) InstanceName = "";

    if ((NetAddr[0] == 'P') && (NetAddr[1] == ':')) {
        SocketOrNamedPipe = 0;   // Named Pipes
        NetAddr += 2;
    } else if ((NetAddr[0] == 'S') && (NetAddr[1] == ':')) {
        SocketOrNamedPipe = 1;   // Sockets
        NetAddr += 2;
    } else {
        SocketOrNamedPipe = 0;   // Named pipes or unix domain sockets
    }
    if (SocketOrNamedPipe) {
        int Port;
        char ServerName[MAX_PATH];
        StringCopyMaxCharTruncate(ServerName, NetAddr, sizeof(ServerName));
        p = strstr(ServerName, "@");
        if (p != NULL) {
            Port = atoi(p+1);
            *p = 0;
        } else {
            Port = 1810;
        }
        Socket = ConnectToRemoteProcedureCallServer(ServerName, Port);
    } else {
#ifdef _WIN32
        Socket = NamedPipeConnectToRemoteProcedureCallServer(NetAddr, InstanceName, 1000);
#else
        Socket = UnixDomainSocketConnectToRemoteProcedureCallServer(NetAddr, InstanceName, 1000);
#endif
    }
    if ((Socket == NULL_OR_0) || (Socket == INVALID_HANDLE_VALUE)) {
        return -1;
    }

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOGIN_CMD, &Req.Header, sizeof(Req), &Ack.Header, sizeof(Ack));
    if (Ret != sizeof(Ack)) {
        return -1;
    }
    if (Ack.Version != XILENV_VERSION) {
        XilEnv_DisconnectFrom();
        return -2; // Wrong version
    }
    return 0;
}

SCRPCDLL_API int __STDCALL__ XilEnv_ConnectTo(const char* NetAddr)
{
    return XilEnv_ConnectToInstance(NetAddr, "");
}

SCRPCDLL_API int __STDCALL__ XilEnv_IsConnectedTo(void)
{
    int Ret = 0;
    if ((Socket != NULL_OR_0) && (Socket != INVALID_HANDLE_VALUE)) {
        Ret = 1;
    }
    // If handel is valid call GetVersion to check if the connection is alive
    if (Ret) {
        if (XilEnv_GetVersion () < 0) {
            Ret = 0;
        }
    }
    return Ret;
}


/* Others */
SCRPCDLL_API int __STDCALL__ XilEnv_CreateFileWithContent (const char *Filename,
                                                       const char *Content)
{
    RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE *Req;
    RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK Ack;
    int Ret;
    size_t LenFilename, LenContent;
    size_t Size;

    LenFilename = strlen(Filename) + 1;
    LenContent = strlen(Content) + 1;
    Size = sizeof(*Req) + LenFilename + LenContent;
    Req = (RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetFilename = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetFilename, Filename, LenFilename);
    Req->OffsetContent = Req->OffsetFilename + LenFilename;
    MEMCPY ((char*)Req + Req->OffsetContent, Content, LenContent);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CREATE_FILE_WITH_CONTENT_CMD, &(Req->Header), Size, &Ack.Header, sizeof(Ack));
    if (Ret != sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

#define  GET_ENVIRONMENT_VARIABLE_MAX_SIZE    4096

// Remark: the return buffer will be overwriten by the next remote call
SCRPCDLL_API char* __STDCALL__ XilEnv_GetEnvironVar (const char *EnvironVar)
{
    RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE *Req;
    RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK *Ack;
    int Ret;
    size_t Len;
    size_t Size;

    Len = strlen(EnvironVar) + 1;
    Size = sizeof(*Req) + Len;
    Req = (RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->MaxSize = GET_ENVIRONMENT_VARIABLE_MAX_SIZE;
    Req->OffsetVariableName = (int32_t)sizeof(*Req) - 1;
    MEMCPY (Req->Data, EnvironVar, Len);
    Ret = RemoteProcedureCallTransactDynBuf (SocketOrNamedPipe, Socket, RPC_API_GET_ENVIRONMENT_VARIABLE_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return NULL;
    }
    if (Ack->Header.ReturnValue != 0) {
        return NULL;
    }
    return ((char*)Ack + Ack->OffsetVariableValue);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetEnvironVarOwnBuffer(const char* EnvironVar, char* buffer, int size)
{
    char* env_string;
    int ret;
    env_string = XilEnv_GetEnvironVar(EnvironVar);
    if (env_string != NULL) {
        ret = strlen(env_string) + 1;
        if (ret <= size) {
            MEMCPY(buffer, env_string, ret);
        }
        else {
            MEMCPY(buffer, env_string, size);
            buffer[size - 1] = 0;  // truncate
        }
    }
    else {
        ret = -1;
    }
    return ret;
}

SCRPCDLL_API int __STDCALL__ XilEnv_SetEnvironVar (const char *EnvironVar, const char *EnvironValue)
{
    RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE *Req;
    RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenEnvironVar, LenEnvironValue;
    size_t Size;

    LenEnvironVar = strlen(EnvironVar) + 1;
    LenEnvironValue = strlen(EnvironValue) + 1;
    Size = sizeof(*Req) + LenEnvironVar + LenEnvironValue;
    Req = (RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetVariableName = (int32_t)sizeof(*Req) - 1;
    Req->OffsetVariableValue = Req->OffsetVariableName + LenEnvironVar;
    MEMCPY (Req->Data, EnvironVar, LenEnvironVar);
    MEMCPY (Req->Data + LenEnvironVar, EnvironValue, LenEnvironValue);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_ENVIRONMENT_VARIABLE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


SCRPCDLL_API int __STDCALL__ XilEnv_ChangeSettings (const char *SettingName, const char *ValueString)
{
    RPC_API_SET_CHANGE_SETTINGS_MESSAGE *Req;
    RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSettingName, LenValueString;
    size_t Size;

    LenSettingName = strlen(SettingName) + 1;
    LenValueString = strlen(ValueString) + 1;
    Size = sizeof(*Req) + LenSettingName + LenValueString;
    Req = (RPC_API_SET_CHANGE_SETTINGS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffseSettingName = (int32_t)sizeof(*Req) - 1;
    Req->OffsetSettingValue = Req->OffseSettingName + LenSettingName;
    MEMCPY (Req->Data, SettingName, LenSettingName);
    MEMCPY (Req->Data + LenSettingName, ValueString, LenValueString);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CHANGE_SETTINGS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API int __STDCALL__ XilEnv_TextOut (const char *TextOut)
{
    RPC_API_TEXT_OUT_MESSAGE *Req;
    RPC_API_TEXT_OUT_MESSAGE_ACK Ack;
    int Ret;
    size_t Len;
    size_t Size;

    Len = strlen(TextOut) + 1;
    Size = sizeof(*Req) + Len;
    Req = (RPC_API_TEXT_OUT_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetText = (int32_t)sizeof(*Req) - 1;
    MEMCPY (Req->Data, TextOut, Len);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_TEXT_OUT_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


SCRPCDLL_API int __STDCALL__ XilEnv_ErrorTextOut (int ErrLevel, const char *TextOut)
{
    UNUSED(ErrLevel);
    RPC_API_ERROR_TEXT_OUT_MESSAGE *Req;
    RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK Ack;
    int Ret;
    size_t Len;
    size_t Size;

    Len = strlen(TextOut) + 1;
    Size = sizeof(*Req) + Len;
    Req = (RPC_API_ERROR_TEXT_OUT_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetText = (int32_t)sizeof(*Req) - 1;
    MEMCPY (Req->Data, TextOut, Len);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ERROR_TEXT_OUT_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


SCRPCDLL_API unsigned long long __STDCALL__ XilEnv_CreateFile (const char *lpFileName,
                                                           DWORD dwDesiredAccess,
                                                           DWORD dwShareMode,
                                                           DWORD dwCreationDisposition,
                                                           DWORD dwFlagsAndAttributes)
{
    RPC_API_CREATE_FILE_MESSAGE *Req;
    RPC_API_CREATE_FILE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenFileName;
    size_t Size;

    LenFileName = strlen(lpFileName) + 1;
    Size = sizeof(*Req) + LenFileName;
    Req = (RPC_API_CREATE_FILE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->dwDesiredAccess = dwDesiredAccess;
    Req->dwShareMode = dwShareMode;
    Req->dwCreationDisposition = dwCreationDisposition;
    Req->dwFlagsAndAttributes = dwFlagsAndAttributes;
    Req->OffsetFilename = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetFilename, lpFileName, LenFileName);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CREATE_FILE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return 0;
    }
    return Ack.Handle;
}

SCRPCDLL_API int __STDCALL__ XilEnv_CloseHandle (unsigned long long hObject)
{
    RPC_API_CLOSE_HANDLE_MESSAGE *Req;
    RPC_API_CLOSE_HANDLE_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_CLOSE_HANDLE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Handle = hObject;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CLOSE_HANDLE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return 0;
    }
    return Ack.Header.Command;
}

SCRPCDLL_API int __STDCALL__ XilEnv_ReadFile (unsigned long long hFile,
                                          unsigned char *lpBuffer,
                                          DWORD nNumberOfBytesToRead,
                                          LPDWORD lpNumberOfBytesRead)
{
    RPC_API_READ_FILE_MESSAGE *Req;
    RPC_API_READ_FILE_MESSAGE_ACK *Ack;
    int Ret;

    Req = (RPC_API_READ_FILE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Handle = hFile;
    Req->nNumberOfBytesToRead = nNumberOfBytesToRead;
    Ret = RemoteProcedureCallTransactDynBuf (SocketOrNamedPipe, Socket, RPC_API_READ_FILE_CMD, &(Req->Header), sizeof(*Req), (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return 0;
    }
    if (Ack->Header.ReturnValue) {
        MEMCPY(lpBuffer, (char*)Ack + Ack->Offset_uint8_NumberOfBytesRead_Buffer, Ack->NumberOfBytesRead);
        *lpNumberOfBytesRead = Ack->NumberOfBytesRead;
    }
    return Ack->Header.ReturnValue;
}



SCRPCDLL_API int __STDCALL__ XilEnv_WriteFile (unsigned long long hFile,
                                           unsigned char *lpBuffer,
                                           DWORD nNumberOfBytesToWrite,
                                           LPDWORD lpNumberOfBytesWritten)
{
    RPC_API_WRITE_FILE_MESSAGE *Req;
    RPC_API_WRITE_FILE_MESSAGE_ACK *Ack;
    int Ret;
    size_t Size;

    Size = sizeof(*Req) + nNumberOfBytesToWrite;
    Req = (RPC_API_WRITE_FILE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);

    Req->nNumberOfBytesToWrite = nNumberOfBytesToWrite;
    Req->Handle = hFile;
    Req->nNumberOfBytesToWrite = nNumberOfBytesToWrite;
    Req->Offset_uint8_nNumberOfBytesToWrite_Buffer = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->Offset_uint8_nNumberOfBytesToWrite_Buffer, lpBuffer, nNumberOfBytesToWrite);
    Ret = RemoteProcedureCallTransactDynBuf (SocketOrNamedPipe, Socket, RPC_API_WRITE_FILE_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return 0;
    }
    if (Ack->Header.ReturnValue) {
        *lpNumberOfBytesWritten = Ack->NumberOfBytesWritten;
    } else {
        *lpNumberOfBytesWritten = 0;
    }
    return Ack->Header.ReturnValue;
}


SCRPCDLL_API int __STDCALL__ XilEnv_CopyFileToLocal (const char *SourceFilename,
                                                     const char *DestinationFilename)
{
    int Ret;
    HANDLE DstFileHandle = INVALID_HANDLE_VALUE;
    unsigned long long SrcFileHandle = XilEnv_CreateFile (SourceFilename,
                                                 FILE_READ_DATA,
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                 OPEN_EXISTING,
                                                 FILE_ATTRIBUTE_NORMAL);
    if ((HANDLE)SrcFileHandle != INVALID_HANDLE_VALUE) {
        DstFileHandle = CreateFile (DestinationFilename,
                                    FILE_WRITE_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL_OR_0);
         if (DstFileHandle != INVALID_HANDLE_VALUE) {
             DWORD NumberOfBytesRead;
             DWORD NumberOfBytesWritten;
             unsigned char Buffer[8*1024];
             for (;;) {
                 if (XilEnv_ReadFile (SrcFileHandle,
                                  Buffer,
                                  sizeof (Buffer),
                                  &NumberOfBytesRead)) {
                     if (NumberOfBytesRead == 0) {
                         Ret = 0;
                         goto __DONE;    // End of file
                     }
                     if (!WriteFile (DstFileHandle,
                                     Buffer,
                                     NumberOfBytesRead,
                                     &NumberOfBytesWritten,
                                     NULL)) {
                         Ret = -1;
                         goto __DONE;    // Error during write
                     }
                     if (NumberOfBytesRead != NumberOfBytesWritten) {
                         Ret = -1;
                         goto __DONE;    // There is something wrong during write
                     }
                 } else {
                     Ret = -1;
                     goto __DONE;    // There is something wrong during read
                 }
             } 
         } else {
             Ret = -1;
         }
    } else Ret = -1;
__DONE:
    if ((HANDLE)SrcFileHandle != INVALID_HANDLE_VALUE) XilEnv_CloseHandle (SrcFileHandle);
    if (DstFileHandle != INVALID_HANDLE_VALUE) CloseHandle (DstFileHandle);
    return Ret;
}

SCRPCDLL_API int __STDCALL__ XilEnv_CopyFileFromLocal (const char *SourceFilename,
                                                       const char *DestinationFilename)
{
    int Ret;
    unsigned long long DstFileHandle = (unsigned long long)INVALID_HANDLE_VALUE;
    HANDLE SrcFileHandle = CreateFile (SourceFilename,
                                       FILE_READ_DATA,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL_OR_0);
    if (SrcFileHandle != INVALID_HANDLE_VALUE) {
        DstFileHandle = XilEnv_CreateFile (DestinationFilename,
                                       FILE_WRITE_DATA,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL);
         if ((HANDLE)DstFileHandle != INVALID_HANDLE_VALUE) {
             DWORD NumberOfBytesRead;
             DWORD NumberOfBytesWritten;
             unsigned char Buffer[8*1024];
             for (;;) {
                 if (ReadFile (SrcFileHandle,
                               Buffer,
                               sizeof (Buffer),
                               &NumberOfBytesRead,
                               NULL)) {
                     if (NumberOfBytesRead == 0) {
                         Ret = 0;
                         goto __DONE;    // End of file
                     }
                     if (!XilEnv_WriteFile (DstFileHandle,
                                        Buffer,
                                        NumberOfBytesRead,
                                        &NumberOfBytesWritten)) {
                         Ret = -1;
                         goto __DONE;    // Error during write
                     }
                     if (NumberOfBytesRead != NumberOfBytesWritten) {
                         Ret = -1;
                         goto __DONE;    // There is something wrong during write
                     }
                 } else {
                     Ret = -1;
                     goto __DONE;    // There is something wrong during read
                 }
             } 
         } else {
             Ret = -1;
         }
    } else Ret = -1;
__DONE:
    if (SrcFileHandle != INVALID_HANDLE_VALUE) CloseHandle (SrcFileHandle);
    if ((HANDLE)DstFileHandle != INVALID_HANDLE_VALUE) XilEnv_CloseHandle (DstFileHandle);
    return Ret;
}


/* Scheduler */
SCRPCDLL_API void __STDCALL__ XilEnv_StopScheduler(void)
{
    RPC_API_STOP_SCHEDULER_MESSAGE *Req;
    RPC_API_STOP_SCHEDULER_MESSAGE_ACK Ack;
    int Ret;
    Req = (RPC_API_STOP_SCHEDULER_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_SCHEDULER_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
}

SCRPCDLL_API void __STDCALL__ XilEnv_ContinueScheduler(void)
{
    RPC_API_CONTINUE_SCHEDULER_MESSAGE *Req;
    RPC_API_CONTINUE_SCHEDULER_MESSAGE_ACK Ack;
    int Ret;
    Req = (RPC_API_CONTINUE_SCHEDULER_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CONTINUE_SCHEDULER_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
}

SCRPCDLL_API int __STDCALL__ XilEnv_IsSchedulerRunning(void)
{
    RPC_API_IS_SCHEDULER_RUNNING_MESSAGE *Req;
    RPC_API_IS_SCHEDULER_RUNNING_MESSAGE_ACK Ack;
    int Ret;
    Req = (RPC_API_IS_SCHEDULER_RUNNING_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_IS_SCHEDULER_RUNNING_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API int __STDCALL__ XilEnv_StartProcess(const char* name)
{
    RPC_API_START_PROCESS_MESSAGE *Req;
    RPC_API_START_PROCESS_MESSAGE_ACK Ack;
    int Ret;
    size_t Len;
    size_t Size;

    Len = strlen(name) + 1;
    Size = sizeof(*Req) + Len;
    Req = (RPC_API_START_PROCESS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY (Req->Data, name, Len);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_PROCESS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API int __STDCALL__ XilEnv_StartProcessAndLoadSvl(const char* ProcessName, const char* SvlName)
{
    RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE *Req;
    RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName, LenSvlName;
    size_t Size;

    LenProcessName = strlen(ProcessName) + 1;
    LenSvlName = strlen(SvlName) + 1;
    Size = sizeof(*Req) + LenProcessName + LenSvlName;
    Req = (RPC_API_START_PROCESS_AND_LOAD_SVL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);
    Req->OffsetSvlName = Req->OffsetProcessName + LenProcessName;
    MEMCPY ((char*)Req + Req->OffsetSvlName, SvlName, LenSvlName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_PROCESS_AND_LOAD_SVL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API int __STDCALL__ XilEnv_StartProcessEx(const char* ProcessName,
                                               int Prio, 
                                               int Cycle, 
                                               short Delay, 
                                               int Timeout, 
                                               const char *SVLFile, 
                                               const char *BBPrefix,
                                               int UseRangeControl,  // If 0, than the following paramter will be ignored
                                               int RangeControlBeforeActiveFlags,
                                               int RangeControlBehindActiveFlags,
                                               int RangeControlStopSchedFlag,
                                               int RangeControlOutput,
                                               int RangeErrorCounterFlag,
                                               const char *RangeErrorCounter,
                                               int RangeControlVarFlag,
                                               const char *RangeControl,
                                               int RangeControlPhysFlag,
                                               int RangeControlLimitValues)
{
    RPC_API_START_PROCESS_EX_MESSAGE *Req;
    RPC_API_START_PROCESS_EX_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName, LenSVLFile, LenBBPrefix, LenRangeErrorCounter, LenRangeControl;
    size_t Size;

    if (SVLFile == NULL) SVLFile = "";
    if (BBPrefix == NULL) BBPrefix = "";
    if (RangeErrorCounter == NULL) RangeErrorCounter = "";
    if (RangeControl == NULL) RangeControl = "";

    LenProcessName = strlen(ProcessName) + 1;
    LenSVLFile = strlen(SVLFile) + 1;
    LenBBPrefix = strlen(BBPrefix) + 1;
    LenRangeErrorCounter = strlen(RangeErrorCounter) + 1;
    LenRangeControl = strlen(RangeControl) + 1;

    Size = sizeof(*Req) + LenProcessName + LenSVLFile + LenBBPrefix + LenRangeErrorCounter + LenRangeControl;
    Req = (RPC_API_START_PROCESS_EX_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Prio = Prio;
    Req->Cycle = Cycle;
    Req->Delay = Delay;
    Req->Timeout = Timeout;
    Req->UseRangeControl = UseRangeControl;
    Req->RangeControlBeforeActiveFlags = RangeControlBeforeActiveFlags;
    Req->RangeControlBehindActiveFlags = RangeControlBehindActiveFlags;
    Req->RangeControlStopSchedFlag = RangeControlStopSchedFlag;
    Req->RangeControlOutput = RangeControlOutput;
    Req->RangeErrorCounterFlag = RangeErrorCounterFlag;
    Req->RangeControlVarFlag = RangeControlVarFlag;
    Req->RangeControlPhysFlag = RangeControlPhysFlag;
    Req->RangeControlLimitValues = RangeControlLimitValues;
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);
    
    Req->OffsetSvlName = Req->OffsetProcessName + LenProcessName;
    MEMCPY ((char*)Req + Req->OffsetSvlName, SVLFile, LenSVLFile);
    
    Req->OffsetBBPrefix = Req->OffsetSvlName + LenSVLFile;
    MEMCPY ((char*)Req + Req->OffsetBBPrefix, BBPrefix, LenBBPrefix);
    
    Req->OffsetRangeErrorCounter = Req->OffsetBBPrefix + LenBBPrefix;
    MEMCPY ((char*)Req + Req->OffsetRangeErrorCounter, RangeErrorCounter, LenRangeErrorCounter);
    
    Req->OffsetRangeControl = Req->OffsetRangeErrorCounter + LenRangeErrorCounter;
    MEMCPY ((char*)Req + Req->OffsetRangeControl, RangeControl, LenRangeControl);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_PROCESS_EX_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API int __STDCALL__ XilEnv_StartProcessEx2(const char* ProcessName,
                                                int Prio, 
                                                int Cycle, 
                                                short Delay, 
                                                int Timeout, 
                                                const char *SVLFile, 
                                                const char *A2LFile, 
                                                int A2LFlags,
                                                const char *BBPrefix,
                                                int UseRangeControl,  // If 0, than the following paramter will be ignored
                                                int RangeControlBeforeActiveFlags,
                                                int RangeControlBehindActiveFlags,
                                                int RangeControlStopSchedFlag,
                                                int RangeControlOutput,
                                                int RangeErrorCounterFlag,
                                                const char *RangeErrorCounter,
                                                int RangeControlVarFlag,
                                                const char *RangeControl,
                                                int RangeControlPhysFlag,
                                                int RangeControlLimitValues)
{
    RPC_API_START_PROCESS_EX2_MESSAGE *Req;
    RPC_API_START_PROCESS_EX2_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName, LenSVLFile, LenA2LFile, LenBBPrefix, LenRangeErrorCounter, LenRangeControl;
    size_t Size;

    if (SVLFile == NULL) SVLFile = "";
    if (A2LFile == NULL) A2LFile = "";
    if (BBPrefix == NULL) BBPrefix = "";
    if (RangeErrorCounter == NULL) RangeErrorCounter = "";
    if (RangeControl == NULL) RangeControl = "";

    LenProcessName = strlen(ProcessName) + 1;
    LenSVLFile = strlen(SVLFile) + 1;
    LenA2LFile = strlen(A2LFile) + 1;
    LenBBPrefix = strlen(BBPrefix) + 1;
    LenRangeErrorCounter = strlen(RangeErrorCounter) + 1;
    LenRangeControl = strlen(RangeControl) + 1;

    Size = sizeof(*Req) + LenProcessName + LenSVLFile+ LenA2LFile + LenBBPrefix + LenRangeErrorCounter + LenRangeControl;
    Req = (RPC_API_START_PROCESS_EX2_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Prio = Prio;
    Req->Cycle = Cycle;
    Req->Delay = Delay;
    Req->Timeout = Timeout;
    Req->A2LFlags = A2LFlags;
    Req->UseRangeControl = UseRangeControl;
    Req->RangeControlBeforeActiveFlags = RangeControlBeforeActiveFlags;
    Req->RangeControlBehindActiveFlags = RangeControlBehindActiveFlags;
    Req->RangeControlStopSchedFlag = RangeControlStopSchedFlag;
    Req->RangeControlOutput = RangeControlOutput;
    Req->RangeErrorCounterFlag = RangeErrorCounterFlag;
    Req->RangeControlVarFlag = RangeControlVarFlag;
    Req->RangeControlPhysFlag = RangeControlPhysFlag;
    Req->RangeControlLimitValues = RangeControlLimitValues;
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);
    
    Req->OffsetSvlName = Req->OffsetProcessName + LenProcessName;
    MEMCPY ((char*)Req + Req->OffsetSvlName, SVLFile, LenSVLFile);
    
    Req->OffsetA2LName = Req->OffsetSvlName + LenSVLFile;
    MEMCPY ((char*)Req + Req->OffsetA2LName, A2LFile, LenA2LFile);

    Req->OffsetBBPrefix = Req->OffsetA2LName + LenA2LFile;
    MEMCPY ((char*)Req + Req->OffsetBBPrefix, BBPrefix, LenBBPrefix);

    Req->OffsetRangeErrorCounter = Req->OffsetBBPrefix + LenBBPrefix;
    MEMCPY ((char*)Req + Req->OffsetRangeErrorCounter, RangeErrorCounter, LenRangeErrorCounter);
    
    Req->OffsetRangeControl = Req->OffsetRangeErrorCounter + LenRangeErrorCounter;
    MEMCPY ((char*)Req + Req->OffsetRangeControl, RangeControl, LenRangeControl);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_PROCESS_EX_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API int __STDCALL__ XilEnv_StopProcess(const char* name)
{
    RPC_API_STOP_PROCESS_MESSAGE *Req;
    RPC_API_STOP_PROCESS_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName;
    size_t Size;

    LenProcessName = strlen(name) + 1;
    Size = sizeof(*Req) + LenProcessName;
    Req = (RPC_API_STOP_PROCESS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, name, LenProcessName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_PROCESS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

// Remark: the return buffer will be overwriten by the next remote call
SCRPCDLL_API char* __STDCALL__ XilEnv_GetNextProcess (int flag, char* filter)
{
    RPC_API_GET_NEXT_PROCESS_MESSAGE *Req;
    RPC_API_GET_NEXT_PROCESS_MESSAGE_ACK *Ack;
    int Ret;
    size_t Len;
    size_t Size;

    Len = strlen(filter) + 1;
    Size = sizeof(*Req) + Len;
    Req = (RPC_API_GET_NEXT_PROCESS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetFilter = (int32_t)sizeof(*Req) - 1;
    MEMCPY (Req->Data, filter, Len);
    Ret = RemoteProcedureCallTransactDynBuf (SocketOrNamedPipe, Socket, RPC_API_GET_NEXT_PROCESS_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return NULL;
    }
    if (Ack->Header.ReturnValue == 0) {
        return NULL;
    }
    return ((char*)Ack + Ack->OffsetReturnValue);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetNextProcessOwnBuffer(int flag, char* filter, char* buffer, int size)
{
    char* process_string;
    int ret;
    process_string = XilEnv_GetNextProcess(flag, filter);
    if (process_string != NULL) {
        ret = strlen(process_string) + 1;
        if (ret <= size) {
            MEMCPY(buffer, process_string, ret);
        }
        else {
            MEMCPY(buffer, process_string, size);
            buffer[size - 1] = 0;  // truncate
        }
    }
    else {
        ret = -1;
    }
    return ret;
}

SCRPCDLL_API int __STDCALL__ XilEnv_GetProcessState(const char* name)
{
    RPC_API_GET_PROCESS_STATE_MESSAGE *Req;
    RPC_API_GET_PROCESS_STATE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName;
    size_t Size;

    LenProcessName = strlen(name) + 1;
    Size = sizeof(*Req) + LenProcessName;
    Req = (RPC_API_GET_PROCESS_STATE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, name, LenProcessName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_PROCESS_STATE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API void __STDCALL__ XilEnv_DoNextCycles (int Cycles)
{
    RPC_API_DO_NEXT_CYCLES_MESSAGE *Req;
    RPC_API_DO_NEXT_CYCLES_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_DO_NEXT_CYCLES_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Cycles = Cycles;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DO_NEXT_CYCLES_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
}

SCRPCDLL_API void __STDCALL__ XilEnv_DoNextCyclesAndWait (int Cycles)
{
    RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE *Req;
    RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_DO_NEXT_CYCLES_AND_WAIT_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Cycles = Cycles;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DO_NEXT_CYCLES_AND_WAIT_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
}

SCRPCDLL_API void __STDCALL__ XilEnv_DoNextConditionsCyclesAndWait (const char *ConditionsEquation, int Cycles)
{
    RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE *Req;
    RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE_ACK Ack;
    int Ret;
    size_t LenConditionsEquation;
    size_t Size;

    LenConditionsEquation = strlen(ConditionsEquation) + 1;
    Size = sizeof(*Req) + LenConditionsEquation;

    Req = (RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    Req->Cycles = Cycles;
    Req->OffsetConditions = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetConditions, ConditionsEquation, LenConditionsEquation);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DO_NEXT_CONDITIONS_CYCLES_AND_WAIT_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
}

SCRPCDLL_API int __STDCALL__ XilEnv_AddBeforeProcessEquationFromFile(int Nr, const char *ProcessName, const char *EquFile)
{
    RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE *Req;
    RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName, LenEquFile;
    size_t Size;

    LenProcessName = strlen(ProcessName) + 1;
    LenEquFile = strlen(EquFile) + 1;
    Size = sizeof(*Req) + LenProcessName + LenEquFile;
    Req = (RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);
    Req->OffsetEquationFile = Req->OffsetProcessName + LenProcessName;
    MEMCPY ((char*)Req + Req->OffsetEquationFile, EquFile, LenEquFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ADD_BEFORE_PROCESS_EQUATION_FROM_FILE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API int __STDCALL__ XilEnv_AddBehindProcessEquationFromFile(int Nr, const char *ProcessName, const char *EquFile)
{
    RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE *Req;
    RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName, LenEquFile;
    size_t Size;

    LenProcessName = strlen(ProcessName) + 1;
    LenEquFile = strlen(EquFile) + 1;
    Size = sizeof(*Req) + LenProcessName + LenEquFile;
    Req = (RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);
    Req->OffsetEquationFile = Req->OffsetProcessName + LenProcessName;
    MEMCPY ((char*)Req + Req->OffsetEquationFile, EquFile, LenEquFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ADD_BEHIND_PROCESS_EQUATION_FROM_FILE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

SCRPCDLL_API void __STDCALL__ XilEnv_DelBeforeProcessEquations(int Nr, const char *ProcessName)
{
    RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE *Req;
    RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName;
    size_t Size;

    LenProcessName = strlen(ProcessName) + 1;
    Size = sizeof(*Req) + LenProcessName;
    Req = (RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DEL_BEFORE_PROCESS_EQUATIONS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
}

SCRPCDLL_API void __STDCALL__ XilEnv_DelBehindProcessEquations(int Nr, const char *ProcessName)
{
    RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE *Req;
    RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName;
    size_t Size;

    LenProcessName = strlen(ProcessName) + 1;
    Size = sizeof(*Req) + LenProcessName;
    Req = (RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DEL_BEHIND_PROCESS_EQUATIONS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
}

SCRPCDLL_API int __STDCALL__ XilEnv_WaitUntil (const char *Equation, int Cycles)
{
    RPC_API_WAIT_UNTIL_MESSAGE *Req;
    RPC_API_WAIT_UNTIL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenEquation;
    size_t Size;

    LenEquation = strlen(Equation) + 1;
    Size = sizeof(*Req) + LenEquation;
    Req = (RPC_API_WAIT_UNTIL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetEquation = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetEquation, Equation, LenEquation);
    Req->Cycles = Cycles;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_WAIT_UNTIL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


/* internal processes */

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartScript(const char* scrfile)
{
    RPC_API_START_SCRIPT_MESSAGE *Req;
    RPC_API_START_SCRIPT_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSriptFile;
    size_t Size;

    LenSriptFile= strlen(scrfile) + 1;
    Size = sizeof(*Req) + LenSriptFile;
    Req = (RPC_API_START_SCRIPT_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetScriptFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetScriptFile, scrfile, LenSriptFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_SCRIPT_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopScript(void)
{
    RPC_API_STOP_SCRIPT_MESSAGE *Req;
    RPC_API_STOP_SCRIPT_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_STOP_SCRIPT_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_SCRIPT_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartRecorder(const char* cfgfile)
{
    RPC_API_START_RECORDER_MESSAGE *Req;
    RPC_API_START_RECORDER_MESSAGE_ACK Ack;
    int Ret;
    size_t LenCfgFile;
    size_t Size;

    LenCfgFile= strlen(cfgfile) + 1;
    Size = sizeof(*Req) + LenCfgFile;
    Req = (RPC_API_START_RECORDER_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetCfgFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetCfgFile, cfgfile, LenCfgFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_RECORDER_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopRecorder(void)
{
    RPC_API_STOP_RECORDER_MESSAGE *Req;
    RPC_API_STOP_RECORDER_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_STOP_RECORDER_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_RECORDER_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_RecorderAddComment(const char* Comment)
{
    RPC_API_RECORDER_ADD_COMMENT_MESSAGE *Req;
    RPC_API_RECORDER_ADD_COMMENT_MESSAGE_ACK Ack;
    int Ret;
    size_t LenComment;
    size_t Size;

    LenComment= strlen(Comment) + 1;
    Size = sizeof(*Req) + LenComment;
    Req = (RPC_API_RECORDER_ADD_COMMENT_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetComment = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetComment, Comment, LenComment);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_RECORDER_ADD_COMMENT_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartPlayer(const char* cfgfile)
{
    RPC_API_START_PLAYER_MESSAGE *Req;
    RPC_API_START_PLAYER_MESSAGE_ACK Ack;
    int Ret;
    size_t LenCfgFile;
    size_t Size;

    LenCfgFile= strlen(cfgfile) + 1;
    Size = sizeof(*Req) + LenCfgFile;
    Req = (RPC_API_START_PLAYER_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetCfgFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetCfgFile, cfgfile, LenCfgFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_PLAYER_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopPlayer(void)
{
    RPC_API_STOP_PLAYER_MESSAGE *Req;
    RPC_API_STOP_PLAYER_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_STOP_PLAYER_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_PLAYER_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartEquations(const char* equfile)
{
    RPC_API_START_EQUATIONS_MESSAGE *Req;
    RPC_API_START_EQUATIONS_MESSAGE_ACK Ack;
    int Ret;
    size_t LenEquFile;
    size_t Size;

    LenEquFile= strlen(equfile) + 1;
    Size = sizeof(*Req) + LenEquFile;
    Req = (RPC_API_START_EQUATIONS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetCfgFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetCfgFile, equfile, LenEquFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_EQUATIONS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopEquations(void)
{
    RPC_API_STOP_EQUATIONS_MESSAGE *Req;
    RPC_API_STOP_EQUATIONS_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_STOP_EQUATIONS_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_EQUATIONS_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartGenerator(const char* genfile)
{
    RPC_API_START_GENERATOR_MESSAGE *Req;
    RPC_API_START_GENERATOR_MESSAGE_ACK Ack;
    int Ret;
    size_t LenGenFile;
    size_t Size;

    LenGenFile= strlen(genfile) + 1;
    Size = sizeof(*Req) + LenGenFile;
    Req = (RPC_API_START_GENERATOR_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetCfgFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetCfgFile, genfile, LenGenFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_GENERATOR_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopGenerator(void)
{
    RPC_API_STOP_GENERATOR_MESSAGE *Req;
    RPC_API_STOP_GENERATOR_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_STOP_GENERATOR_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_GENERATOR_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

/* GUI */
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadDesktop(const char* file)
{
    RPC_API_LOAD_DESKTOP_MESSAGE *Req;
    RPC_API_LOAD_DESKTOP_MESSAGE_ACK Ack;
    int Ret;
    size_t LenFile;
    size_t Size;

    LenFile= strlen(file) + 1;
    Size = sizeof(*Req) + LenFile;
    Req = (RPC_API_LOAD_DESKTOP_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetFile, file, LenFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOAD_DESKTOP_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SaveDesktop(const char* file)
{
    RPC_API_SAVE_DESKTOP_MESSAGE *Req;
    RPC_API_SAVE_DESKTOP_MESSAGE_ACK Ack;
    int Ret;
    size_t LenFile;
    size_t Size;

    LenFile= strlen(file) + 1;
    Size = sizeof(*Req) + LenFile;
    Req = (RPC_API_SAVE_DESKTOP_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetFile, file, LenFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SAVE_DESKTOP_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ClearDesktop(void)
{
    RPC_API_SAVE_DESKTOP_MESSAGE *Req;
    RPC_API_SAVE_DESKTOP_MESSAGE_ACK Ack;
    int Ret;
    size_t Size;

    Size = sizeof(*Req);
    Req = (RPC_API_SAVE_DESKTOP_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CLEAR_DESKTOP_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CreateDialog(const char* DialogName)
{
    RPC_API_CREATE_DIALOG_MESSAGE *Req;
    RPC_API_CREATE_DIALOG_MESSAGE_ACK Ack;
    int Ret;
    size_t LenDialogName;
    size_t Size;

    LenDialogName= strlen(DialogName) + 1;
    Size = sizeof(*Req) + LenDialogName;
    Req = (RPC_API_CREATE_DIALOG_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetDialogName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetDialogName, DialogName, LenDialogName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CREATE_DIALOG_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddDialogItem(const char* Description, const char* VariName)
{
    RPC_API_ADD_DIALOG_ITEM_MESSAGE *Req;
    RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK Ack;
    int Ret;
    size_t LenDescription, LenVariName;
    size_t Size;

    LenDescription = strlen(Description) + 1;
    LenVariName = strlen(VariName) + 1;
    Size = sizeof(*Req) + LenDescription + LenVariName;
    Req = (RPC_API_ADD_DIALOG_ITEM_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetDescription = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetDescription, Description, LenDescription);
    Req->OffsetVariName = Req->OffsetDescription + LenDescription;
    MEMCPY ((char*)Req + Req->OffsetVariName, VariName, LenVariName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ADD_DIALOG_ITEM_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ShowDialog(void)
{
    RPC_API_SHOW_DIALOG_MESSAGE *Req;
    RPC_API_SHOW_DIALOG_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_SHOW_DIALOG_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SHOW_DIALOG_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_IsDialogClosed(void)
{
    RPC_API_IS_DIALOG_CLOSED_MESSAGE *Req;
    RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_IS_DIALOG_CLOSED_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_IS_DIALOG_CLOSED_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SelectSheet (const char* SheetName)
{
    RPC_API_SELECT_SHEET_MESSAGE *Req;
    RPC_API_SELECT_SHEET_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSheetName;
    size_t Size;

    LenSheetName = strlen(SheetName) + 1;
    Size = sizeof(*Req) + LenSheetName;
    Req = (RPC_API_SELECT_SHEET_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSheetName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSheetName, SheetName, LenSheetName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SELECT_SHEET_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddSheet (const char* SheetName)
{
    RPC_API_ADD_SHEET_MESSAGE *Req;
    RPC_API_ADD_SHEET_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSheetName;
    size_t Size;

    LenSheetName = strlen(SheetName) + 1;
    Size = sizeof(*Req) + LenSheetName;
    Req = (RPC_API_ADD_SHEET_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSheetName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSheetName, SheetName, LenSheetName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ADD_SHEET_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DeleteSheet (const char* SheetName)
{
    RPC_API_DELETE_SHEET_MESSAGE *Req;
    RPC_API_DELETE_SHEET_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSheetName;
    size_t Size;

    LenSheetName = strlen(SheetName) + 1;
    Size = sizeof(*Req) + LenSheetName;
    Req = (RPC_API_DELETE_SHEET_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSheetName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSheetName, SheetName, LenSheetName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DELETE_SHEET_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_RenameSheet (const char* OldSheetName, const char* NewSheetName)
{
    RPC_API_RENAME_SHEET_MESSAGE *Req;
    RPC_API_RENAME_SHEET_MESSAGE_ACK Ack;
    int Ret;
    size_t LenOldSheetName, LenNewSheetName;
    size_t Size;

    LenOldSheetName = strlen(OldSheetName) + 1;
    LenNewSheetName = strlen(NewSheetName) + 1;
    Size = sizeof(*Req) + LenOldSheetName + LenNewSheetName;
    Req = (RPC_API_RENAME_SHEET_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetOldSheetName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetOldSheetName, OldSheetName, LenOldSheetName);
    Req->OffsetNewSheetName = Req->OffsetOldSheetName + LenOldSheetName;
    MEMCPY ((char*)Req + Req->OffsetNewSheetName, NewSheetName, LenNewSheetName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_RENAME_SHEET_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_OpenWindow (const char* WindowName)
{
    RPC_API_OPEN_WINDOW_MESSAGE *Req;
    RPC_API_OPEN_WINDOW_MESSAGE_ACK Ack;
    int Ret;
    size_t LenWindowName;
    size_t Size;

    LenWindowName = strlen(WindowName) + 1;
    Size = sizeof(*Req) + LenWindowName;
    Req = (RPC_API_OPEN_WINDOW_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetWindowName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetWindowName, WindowName, LenWindowName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_OPEN_WINDOW_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CloseWindow (const char* WindowName)
{
    RPC_API_CLOSE_WINDOW_MESSAGE *Req;
    RPC_API_CLOSE_WINDOW_MESSAGE_ACK Ack;
    int Ret;
    size_t LenWindowName;
    size_t Size;

    LenWindowName = strlen(WindowName) + 1;
    Size = sizeof(*Req) + LenWindowName;
    Req = (RPC_API_CLOSE_WINDOW_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetWindowName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetWindowName, WindowName, LenWindowName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CLOSE_WINDOW_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DeleteWindow (const char* WindowName)
{
    RPC_API_DELETE_WINDOW_MESSAGE *Req;
    RPC_API_DELETE_WINDOW_MESSAGE_ACK Ack;
    int Ret;
    size_t LenWindowName;
    size_t Size;

    LenWindowName = strlen(WindowName) + 1;
    Size = sizeof(*Req) + LenWindowName;
    Req = (RPC_API_DELETE_WINDOW_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetWindowName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetWindowName, WindowName, LenWindowName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DELETE_WINDOW_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ImportWindow (const char* WindowName, const char* FileName)
{
    RPC_API_IMPORT_WINDOW_MESSAGE *Req;
    RPC_API_IMPORT_WINDOW_MESSAGE_ACK Ack;
    int Ret;
    size_t LenWindowName, LenFileName;
    size_t Size;

    LenWindowName = strlen(WindowName) + 1;
    LenFileName = strlen(FileName) + 1;
    Size = sizeof(*Req) + LenFileName + LenFileName;
    Req = (RPC_API_IMPORT_WINDOW_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetWindowName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetWindowName, WindowName, LenWindowName);
    Req->OffsetFileName = Req->OffsetWindowName + LenWindowName;
    MEMCPY ((char*)Req + Req->OffsetFileName, FileName, LenFileName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_IMPORT_WINDOW_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ExportWindow (const char* SheetName, const char* WindowName, const char* FileName)
{
    RPC_API_EXPORT_WINDOW_MESSAGE *Req;
    RPC_API_EXPORT_WINDOW_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSheetName, LenWindowName, LenFileName;
    size_t Size;

    LenSheetName = strlen(SheetName) + 1;
    LenWindowName = strlen(WindowName) + 1;
    LenFileName = strlen(FileName) + 1;
    Size = sizeof(*Req) + LenFileName + LenFileName;
    Req = (RPC_API_EXPORT_WINDOW_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSheetName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSheetName, SheetName, LenSheetName);
    Req->OffsetWindowName = Req->OffsetSheetName + LenSheetName;
    MEMCPY ((char*)Req + Req->OffsetWindowName, WindowName, LenWindowName);
    Req->OffsetFileName = Req->OffsetWindowName + LenWindowName;
    MEMCPY ((char*)Req + Req->OffsetFileName, FileName, LenFileName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_EXPORT_WINDOW_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


/* Blackboard */

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddVari(const char* label, int type, const char* unit)
{
    RPC_API_ADD_BBVARI_MESSAGE *Req;
    RPC_API_ADD_BBVARI_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel, LenUnit;
    size_t Size;

    if (unit == NULL) unit = "";
    LenLabel = strlen(label) + 1;
    LenUnit = strlen(unit) + 1;
    Size = sizeof(*Req) + LenLabel + LenUnit;
    Req = (RPC_API_ADD_BBVARI_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Type = type;
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, label, LenLabel);
    Req->OffsetUnit = Req->OffsetLabel + LenLabel;
    MEMCPY ((char*)Req + Req->OffsetUnit, unit, LenUnit);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ADD_BBVARI_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_RemoveVari(int vid)
{
    RPC_API_REMOVE_BBVARI_MESSAGE *Req;
    RPC_API_REMOVE_BBVARI_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_REMOVE_BBVARI_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_REMOVE_BBVARI_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AttachVari(const char* label)
{
    RPC_API_ATTACH_BBVARI_MESSAGE *Req;
    RPC_API_ATTACH_BBVARI_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel;
    size_t Size;

    LenLabel = strlen(label) + 1;
    Size = sizeof(*Req) + LenLabel;
    Req = (RPC_API_ATTACH_BBVARI_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, label, LenLabel);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ATTACH_BBVARI_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_Get(int vid)
{
    RPC_API_GET_MESSAGE *Req;
    RPC_API_GET_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_GET_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if ((Ret < sizeof(Ack)) || (Ack.Header.ReturnValue != 0)) {
        return 0.0;
    }
    return Ack.ReturnValue;
}

CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetPhys(int vid)
{
    RPC_API_GET_PHYS_MESSAGE *Req;
    RPC_API_GET_PHYS_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_GET_PHYS_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_PHYS_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if ((Ret < sizeof(Ack)) || (Ack.Header.ReturnValue != 0)) {
        return 0.0;
    }
    return Ack.ReturnValue;
}

CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_Set(int vid, double value)
{
    RPC_API_SET_MESSAGE *Req;
    RPC_API_SET_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_SET_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Req->Value = value;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetPhys(int vid, double value)
{
    RPC_API_SET_PHYS_MESSAGE *Req;
    RPC_API_SET_PHYS_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_SET_PHYS_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Req->Value = value;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_PHYS_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_Equ(const char* equ)
{
    RPC_API_EQU_MESSAGE *Req;
    RPC_API_EQU_MESSAGE_ACK Ack;
    int Ret;
    size_t LenEquation;
    size_t Size;

    LenEquation = strlen(equ) + 1;
    Size = sizeof(*Req) + LenEquation;
    Req = (RPC_API_EQU_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetEquation = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetEquation, equ, LenEquation);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_EQU_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if ((Ret < sizeof(Ack)) || (Ack.Header.ReturnValue != 0)) {
        return 0.0;
    }
    return Ack.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WrVariEnable(const char* label, const char* process)
{
    RPC_API_WR_VARI_ENABLE_MESSAGE *Req;
    RPC_API_WR_VARI_ENABLE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel, LenProcess;
    size_t Size;

    LenLabel = strlen(label) + 1;
    LenProcess = strlen(process) + 1;
    Size = sizeof(*Req) + LenLabel + LenProcess;
    Req = (RPC_API_WR_VARI_ENABLE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, label, LenLabel);
    Req->OffsetProcess = Req->OffsetLabel + LenLabel;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_WR_VARI_ENABLE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WrVariDisable(const char* label, const char* process)
{
    RPC_API_WR_VARI_DISABLE_MESSAGE *Req;
    RPC_API_WR_VARI_DISABLE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel, LenProcess;
    size_t Size;

    LenLabel = strlen(label) + 1;
    LenProcess = strlen(process) + 1;
    Size = sizeof(*Req) + LenLabel + LenProcess;
    Req = (RPC_API_WR_VARI_DISABLE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, label, LenLabel);
    Req->OffsetProcess = Req->OffsetLabel + LenLabel;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_WR_VARI_DISABLE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_IsWrVariEnabled(const char* label, const char* process)
{
    RPC_API_IS_WR_VARI_ENABLED_MESSAGE *Req;
    RPC_API_IS_WR_VARI_ENABLED_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel, LenProcess;
    size_t Size;

    LenLabel = strlen(label) + 1;
    LenProcess = strlen(process) + 1;
    Size = sizeof(*Req) + LenLabel + LenProcess;
    Req = (RPC_API_IS_WR_VARI_ENABLED_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, label, LenLabel);
    Req->OffsetProcess = Req->OffsetLabel + LenLabel;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_IS_WR_VARI_ENABLED_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadRefList(const char* reflist, const char* process)
{
    RPC_API_LOAD_REF_LIST_MESSAGE *Req;
    RPC_API_LOAD_REF_LIST_MESSAGE_ACK Ack;
    int Ret;
    size_t LenRefList, LenProcess;
    size_t Size;

    LenRefList = strlen(reflist) + 1;
    LenProcess = strlen(process) + 1;
    Size = sizeof(*Req) + LenRefList + LenProcess;
    Req = (RPC_API_LOAD_REF_LIST_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetRefList = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetRefList, reflist, LenRefList);
    Req->OffsetProcess = Req->OffsetRefList + LenRefList;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOAD_REF_LIST_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddRefList(const char* reflist, const char* process)
{
    RPC_API_ADD_REF_LIST_MESSAGE *Req;
    RPC_API_ADD_REF_LIST_MESSAGE_ACK Ack;
    int Ret;
    size_t LenRefList, LenProcess;
    size_t Size;

    LenRefList = strlen(reflist) + 1;
    LenProcess = strlen(process) + 1;
    Size = sizeof(*Req) + LenRefList + LenProcess;
    Req = (RPC_API_ADD_REF_LIST_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetRefList = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetRefList, reflist, LenRefList);
    Req->OffsetProcess = Req->OffsetRefList + LenRefList;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ADD_REF_LIST_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SaveRefList(const char* reflist, const char* process)
{
    RPC_API_SAVE_REF_LIST_MESSAGE *Req;
    RPC_API_SAVE_REF_LIST_MESSAGE_ACK Ack;
    int Ret;
    size_t LenRefList, LenProcess;
    size_t Size;

    LenRefList = strlen(reflist) + 1;
    LenProcess = strlen(process) + 1;
    Size = sizeof(*Req) + LenRefList + LenProcess;
    Req = (RPC_API_SAVE_REF_LIST_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetRefList = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetRefList, reflist, LenRefList);
    Req->OffsetProcess = Req->OffsetRefList + LenRefList;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SAVE_REF_LIST_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariConversionType(int vid)
{
    RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE *Req;
    RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_CONVERSION_TYPE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

// Remark: the return buffer will be overwriten by the next remote call
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetVariConversionString(int vid)
{
    RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE *Req;
    RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_ACK *Ack;
    int Ret;
    
    Req = (RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_CONVERSION_STRING_CMD, &(Req->Header), sizeof(*Req), (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return NULL;
    }
    if ((Ack->Header.ReturnValue != 1) &&
        (Ack->Header.ReturnValue != 2)) {
        return NULL;
    }
    return ((char*)Ack + Ack->OffsetConversionString);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariConversionStringOwnBuffer(int vid, char* buffer, int size)
{
    char* conv_string;
    int ret;
    conv_string = XilEnv_GetVariConversionString(vid);
    if (conv_string != NULL) {
        ret = strlen(conv_string) + 1;
        if (ret <= size) {
            MEMCPY(buffer, conv_string, ret);
        } else {
            MEMCPY(buffer, conv_string, size);
            buffer[size - 1] = 0;  // truncate
        }
    } else {
        ret = -1;
    }
    return ret;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariConversion(int vid, int type, const char* conv_string)
{
    RPC_API_SET_VARI_CONVERSION_MESSAGE *Req;
    RPC_API_SET_VARI_CONVERSION_MESSAGE_ACK Ack;
    int Ret;
    size_t LenConversionString;
    size_t Size;

    LenConversionString = strlen(conv_string) + 1;
    Size = sizeof(*Req) + LenConversionString;
    Req = (RPC_API_SET_VARI_CONVERSION_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Vid = vid;
    Req->Type = type;
    Req->OffsetConversionString = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetConversionString, conv_string, LenConversionString);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_VARI_CONVERSION_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariType(int vid)
{
    RPC_API_GET_VARI_TYPE_MESSAGE *Req;
    RPC_API_GET_VARI_TYPE_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_GET_VARI_TYPE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_TYPE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

// Remark: the return buffer will be overwriten by the next remote call
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetVariUnit(int vid)
{
    RPC_API_GET_VARI_UNIT_MESSAGE *Req;
    RPC_API_GET_VARI_UNIT_MESSAGE_ACK *Ack;
    int Ret;
    
    Req = (RPC_API_GET_VARI_UNIT_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_UNIT_CMD, &(Req->Header), sizeof(*Req), (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return NULL;
    }
    if (Ack->Header.ReturnValue != 0) {
        return NULL;
    }
    return ((char*)Ack + Ack->OffsetUnit);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariUnitOwnBuffer(int vid, char *buffer, int size)
{
    char* unit_string;
    int ret;
    unit_string = XilEnv_GetVariUnit(vid);
    if (unit_string != NULL) {
        ret = strlen(unit_string) + 1;
        if (ret <= size) {
            MEMCPY(buffer, unit_string, ret);
        }
        else {
            MEMCPY(buffer, unit_string, size);
            buffer[size - 1] = 0;  // truncate
        }
    }
    else {
        ret = -1;
    }
    return ret;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariUnit(int vid, char *unit)
{
    RPC_API_SET_VARI_UNIT_MESSAGE *Req;
    RPC_API_SET_VARI_UNIT_MESSAGE_ACK Ack;
    int Ret;
    size_t LenUnit;
    size_t Size;

    LenUnit = strlen(unit) + 1;
    Size = sizeof(*Req) + LenUnit;
    Req = (RPC_API_SET_VARI_UNIT_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Vid = vid;
    Req->OffsetUnit = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetUnit, unit, LenUnit);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_VARI_UNIT_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetVariMin(int vid)
{
    RPC_API_GET_VARI_MIN_MESSAGE *Req;
    RPC_API_GET_VARI_MIN_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_GET_VARI_MIN_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_MIN_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if ((Ret < sizeof(Ack)) || (Ack.Header.ReturnValue != 0)) {
        return 0.0;
    }
    return Ack.ReturnValue;
}

CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetVariMax(int vid)
{
    RPC_API_GET_VARI_MAX_MESSAGE *Req;
    RPC_API_GET_VARI_MAX_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_GET_VARI_MAX_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_MAX_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if ((Ret < sizeof(Ack)) || (Ack.Header.ReturnValue != 0)) {
        return 0.0;
    }
    return Ack.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariMin(int vid, double min)
{
    RPC_API_SET_VARI_MIN_MESSAGE *Req;
    RPC_API_SET_VARI_MIN_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_SET_VARI_MIN_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Req->Min = min;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_VARI_MIN_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariMax(int vid, double max)
{
    RPC_API_SET_VARI_MAX_MESSAGE *Req;
    RPC_API_SET_VARI_MAX_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_SET_VARI_MAX_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Req->Max = max;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_VARI_MAX_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

// Remark: the return buffer will be overwriten by the next remote call
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetNextVari (int flag, char* filter)
{
    RPC_API_GET_NEXT_VARI_MESSAGE *Req;
    RPC_API_GET_NEXT_VARI_MESSAGE_ACK *Ack;
    int Ret;
    size_t LenFilter;
    size_t Size;

    LenFilter = strlen(filter) + 1;
    Size = sizeof(*Req) + LenFilter;
    Req = (RPC_API_GET_NEXT_VARI_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Flag = flag;
    Req->OffsetFilter = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetFilter, filter, LenFilter);

    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_GET_NEXT_VARI_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return NULL;
    }
    if (Ack->Header.ReturnValue != 1) {
        return NULL;
    }
    return ((char*)Ack + Ack->OffsetLabel);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetNextVariOwnBuffer(int flag, char* filter, char* buffer, int size)
{
    char* name_string;
    int ret;
    name_string = XilEnv_GetNextVari(flag, filter);
    if (name_string != NULL) {
        ret = strlen(name_string) + 1;
        if (ret <= size) {
            MEMCPY(buffer, name_string, ret);
        }
        else {
            MEMCPY(buffer, name_string, size);
            buffer[size - 1] = 0;  // truncate
        }
    }
    else {
        ret = -1;
    }
    return ret;
}

// Remark: the return buffer will be overwriten by the next remote call
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetNextVariEx (int flag, char* filter, char *process, int AccessFlags)
{
    RPC_API_GET_NEXT_VARI_EX_MESSAGE *Req;
    RPC_API_GET_NEXT_VARI_EX_MESSAGE_ACK *Ack;
    int Ret;
    size_t LenFilter, LenProcess;
    size_t Size;

    LenFilter = strlen(filter) + 1;
    LenProcess = strlen(process) + 1;
    Size = sizeof(*Req) + LenFilter + LenProcess;
    Req = (RPC_API_GET_NEXT_VARI_EX_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Flag = flag;
    Req->AccessFlags = AccessFlags;
    Req->OffsetFilter = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetFilter, filter, LenFilter);
    Req->OffsetProcess =  Req->OffsetFilter + LenFilter;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);

    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_GET_NEXT_VARI_EX_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return NULL;
    }
    if (Ack->Header.ReturnValue != 1) {
        return NULL;
    }
    return ((char*)Ack + Ack->OffsetLabel);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetNextVariExOwnBuffer(int flag, char* filter, char* process, int AccessFlags, char* buffer, int size)
{
    char* name_string;
    int ret;
    name_string = XilEnv_GetNextVariEx(flag, filter, process, AccessFlags);
    if (name_string != NULL) {
        ret = strlen(name_string) + 1;
        if (ret <= size) {
            MEMCPY(buffer, name_string, ret);
        }
        else {
            MEMCPY(buffer, name_string, size);
            buffer[size - 1] = 0;  // truncate
        }
    }
    else {
        ret = -1;
    }
    return ret;
}

// Remark: the return buffer will be overwriten by the next remote call
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetVariEnum (int vid, double value)
{
    RPC_API_GET_VARI_ENUM_MESSAGE *Req;
    RPC_API_GET_VARI_ENUM_MESSAGE_ACK *Ack;
    int Ret;

    Req = (RPC_API_GET_VARI_ENUM_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Vid = vid;
    Req->Value = value;

    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_ENUM_CMD, &(Req->Header), sizeof(*Req), (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return NULL;
    }
    if (Ack->Header.ReturnValue != 0) {
        return NULL;
    }
    return ((char*)Ack + Ack->OffsetEnum);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariEnumOwnBuffer(int vid, double value, char* buffer, int size)
{
    char* enum_string;
    int ret;
    enum_string = XilEnv_GetVariEnum(vid, value);
    if (enum_string != NULL) {
        ret = strlen(enum_string) + 1;
        if (ret <= size) {
            MEMCPY(buffer, enum_string, ret);
        }
        else {
            MEMCPY(buffer, enum_string, size);
            buffer[size - 1] = 0;  // truncate
        }
    }
    else {
        ret = -1;
    }
    return ret;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariDisplayFormatWidth (int vid)
{
    RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE *Req;
    RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariDisplayFormatPrec (int vid)
{
    RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE *Req;
    RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariDisplayFormat (int vid, int width, int prec)
{
    RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE *Req;
    RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE_ACK Ack;
    int Ret;
    
    Req = (RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Req->Vid = vid;
    Req->Width = width;
    Req->Prec = prec;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_VARI_DISPLAY_FORMAT_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ImportVariProperties (const char* Filename)
{
    RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE *Req;
    RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE_ACK Ack;
    int Ret;
    size_t LenFilename;
    size_t Size;

    LenFilename = strlen(Filename) + 1;
    Size = sizeof(*Req) + LenFilename;
    Req = (RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetFilename = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetFilename, Filename, LenFilename);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_IMPORT_VARI_PROPERTIES_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_EnableRangeControl (const char* ProcessNameFilter, const char* VariableNameFilter)
{
    RPC_API_ENABLE_RANGE_CONTROL_MESSAGE *Req;
    RPC_API_ENABLE_RANGE_CONTROL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessNameFilter, LenVariableNameFilter;
    size_t Size;

    LenProcessNameFilter = strlen(ProcessNameFilter) + 1;
    LenVariableNameFilter = strlen(VariableNameFilter) + 1;
    Size = sizeof(*Req) + LenProcessNameFilter + LenVariableNameFilter;
    Req = (RPC_API_ENABLE_RANGE_CONTROL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessNameFilter = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessNameFilter, ProcessNameFilter, LenProcessNameFilter);
    Req->OffsetVariableNameFilter = Req->OffsetProcessNameFilter + LenProcessNameFilter;
    MEMCPY ((char*)Req + Req->OffsetVariableNameFilter, VariableNameFilter, LenVariableNameFilter);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_ENABLE_RANGE_CONTROL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DisableRangeControl (const char* ProcessNameFilter, const char* VariableNameFilter)
{
    RPC_API_DISABLE_RANGE_CONTROL_MESSAGE *Req;
    RPC_API_DISABLE_RANGE_CONTROL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessNameFilter, LenVariableNameFilter;
    size_t Size;

    LenProcessNameFilter = strlen(ProcessNameFilter) + 1;
    LenVariableNameFilter = strlen(VariableNameFilter) + 1;
    Size = sizeof(*Req) + LenProcessNameFilter + LenVariableNameFilter;
    Req = (RPC_API_DISABLE_RANGE_CONTROL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessNameFilter = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessNameFilter, ProcessNameFilter, LenProcessNameFilter);
    Req->OffsetVariableNameFilter = Req->OffsetProcessNameFilter + LenProcessNameFilter;
    MEMCPY ((char*)Req + Req->OffsetVariableNameFilter, VariableNameFilter, LenVariableNameFilter);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DISABLE_RANGE_CONTROL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WriteFrame (int *Vids, signed char *PhysOrRaw, double *ValueFrame, int Elements)
{
    RPC_API_WRITE_FRAME_MESSAGE *Req;
    RPC_API_WRITE_FRAME_MESSAGE_ACK Ack;
    int Ret;
    size_t LenVids, LenPhysOrRaw, LenValueFrame;
    size_t Size;

    LenVids = Elements * sizeof(int);
    LenPhysOrRaw = Elements * sizeof(int8_t);
    LenValueFrame = Elements * sizeof(double);
    Size = sizeof(*Req) + LenVids + LenValueFrame;
    if (PhysOrRaw != NULL) {
        Size += LenPhysOrRaw;
    }
    Req = (RPC_API_WRITE_FRAME_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Elements = Elements;
    Req->Offset_int32_Elements_Vids = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->Offset_int32_Elements_Vids, Vids, LenVids);
    if (PhysOrRaw == NULL) {
        Req->Offset_int8_Elements_PhysOrRaw = 0;
        Req->Offset_double_Elements_ValueFrame = Req->Offset_int32_Elements_Vids + LenVids;
    } else {
        Req->Offset_int8_Elements_PhysOrRaw = Req->Offset_int32_Elements_Vids + LenVids;
        MEMCPY ((char*)Req + Req->Offset_int8_Elements_PhysOrRaw, PhysOrRaw, LenPhysOrRaw);
        Req->Offset_double_Elements_ValueFrame = Req->Offset_int8_Elements_PhysOrRaw + LenPhysOrRaw;
    }
    MEMCPY ((char*)Req + Req->Offset_double_Elements_ValueFrame, ValueFrame, LenValueFrame);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_WRITE_FRAME_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetFrame (int *Vids, signed char *PhysOrRaw, double *RetValueFrame, int Elements)
{
    RPC_API_GET_FRAME_MESSAGE *Req;
    RPC_API_GET_FRAME_MESSAGE_ACK *Ack;
    int Ret;
    size_t LenVids, LenPhysOrRaw;
    size_t Size;

    LenVids = Elements * sizeof(int);
    LenPhysOrRaw = Elements * sizeof(signed char);
    Size = sizeof(*Req) + LenVids;
    if (PhysOrRaw != NULL) {
        Size += LenPhysOrRaw;
    }
    Req = (RPC_API_GET_FRAME_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Elements = Elements;
    Req->Offset_int32_Elements_Vids = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->Offset_int32_Elements_Vids, Vids, LenVids);
    if (PhysOrRaw == NULL) {
        Req->Offset_int8_Elements_PhysOrRaw = 0;
    } else {
        Req->Offset_int8_Elements_PhysOrRaw = Req->Offset_int32_Elements_Vids + LenVids;
        MEMCPY ((char*)Req + Req->Offset_int8_Elements_PhysOrRaw, PhysOrRaw, LenPhysOrRaw);
    }

    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_GET_FRAME_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return -1;
    }
    if (Ack->Header.ReturnValue != 0) {
        return -1;
    }
    MEMCPY (RetValueFrame, Ack->Data, Elements * sizeof(double));

    return Ack->Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WriteFrameWaitReadFrame (int *WriteVids, signed char *WritePhysOrRaw, double *WriteValues, int WriteSize,
                                                               int *ReadVids, signed char *ReadPhysOrRaw, double *ReadValuesRet, int ReadSize)
{
    RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE *Req;
    RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE_ACK *Ack;
    int Ret;
    size_t LenWriteVids,  LenWritePhysOrRaw, LenWriteValues, LenReadVids, LenReadPhysOrRaw;
    size_t Size;


    LenWriteVids = WriteSize * sizeof(int);
    LenWritePhysOrRaw = WriteSize * sizeof(int8_t);
    LenWriteValues = WriteSize * sizeof(double);
    LenReadVids = ReadSize * sizeof(int);
    LenReadPhysOrRaw = WriteSize * sizeof(int8_t);

    Size = sizeof(*Req) + LenWriteVids + LenWriteValues + LenReadVids;
    if (WritePhysOrRaw != NULL) {
        Size += LenWritePhysOrRaw;
    }
    if (ReadPhysOrRaw != NULL) {
        Size += LenReadPhysOrRaw;
    }

    Req = (RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->WriteSize = WriteSize;
    Req->ReadSize = ReadSize;
    Req->Offset_int32_Write_Vids = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->Offset_int32_Write_Vids, WriteVids, LenWriteVids);
    if (WritePhysOrRaw == NULL) {
        Req->Offset_int8_Write_PhysOrRaw = 0;
        Req->Offset_double_Write_Values = Req->Offset_int32_Write_Vids + LenWriteVids;
    } else {
        Req->Offset_int8_Write_PhysOrRaw = Req->Offset_int32_Write_Vids + LenWriteVids;
        MEMCPY ((char*)Req + Req->Offset_int8_Write_PhysOrRaw, WritePhysOrRaw, LenWritePhysOrRaw);
        Req->Offset_double_Write_Values = Req->Offset_int8_Write_PhysOrRaw + LenWritePhysOrRaw;
    }
    MEMCPY ((char*)Req + Req->Offset_double_Write_Values, WriteValues, LenWriteValues);
    Req->Offset_int32_Read_Vids = Req->Offset_double_Write_Values + LenWriteValues;
    MEMCPY ((char*)Req + Req->Offset_int32_Read_Vids, ReadVids, LenReadVids);
    if (ReadPhysOrRaw == NULL) {
        Req->Offset_int8_Read_PhysOrRaw = 0;
    } else {
        Req->Offset_int8_Read_PhysOrRaw = Req->Offset_int32_Read_Vids + LenReadVids;
        MEMCPY ((char*)Req + Req->Offset_int8_Read_PhysOrRaw, ReadPhysOrRaw, LenReadPhysOrRaw);
    }

    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_WRITE_FRAME_WAIT_READ_FRAME_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return -1;
    }
    if (Ack->Header.ReturnValue != 0) {
        return -1;
    }
    MEMCPY (ReadValuesRet, Ack->Data, ReadSize * sizeof(double));

    return Ack->Header.ReturnValue;
}

/* Calibration */
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadSvl(const char* svlfile, const char* process)
{
    RPC_API_LOAD_SVL_MESSAGE *Req;
    RPC_API_LOAD_SVL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSvlFilename, LenProcess;
    size_t Size;

    LenSvlFilename = strlen(svlfile) + 1;
    LenProcess = strlen(process) + 1;
    Size = sizeof(*Req) + LenSvlFilename + LenProcess;
    Req = (RPC_API_LOAD_SVL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSvlFilename = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSvlFilename, svlfile, LenSvlFilename);
    Req->OffsetProcess = Req->OffsetSvlFilename + LenSvlFilename;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOAD_SVL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SaveSvl(const char* svlfile, const char* process, const char* filter)
{
    RPC_API_SAVE_SVL_MESSAGE *Req;
    RPC_API_SAVE_SVL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSvlFilename, LenProcess, LenFilter;
    size_t Size;

    LenSvlFilename = strlen(svlfile) + 1;
    LenProcess = strlen(process) + 1;
    LenFilter = strlen(filter) + 1;
    Size = sizeof(*Req) + LenSvlFilename + LenProcess + LenFilter;
    Req = (RPC_API_SAVE_SVL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSvlFilename = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSvlFilename, svlfile, LenSvlFilename);
    Req->OffsetProcess = Req->OffsetSvlFilename + LenSvlFilename;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);
    Req->OffsetFilter = Req->OffsetProcess + LenProcess;
    MEMCPY ((char*)Req + Req->OffsetFilter, filter, LenFilter);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SAVE_SVL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SaveSal(const char* salfile, const char* process, const char* filter)
{
    RPC_API_SAVE_SAL_MESSAGE *Req;
    RPC_API_SAVE_SAL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSalFilename, LenProcess, LenFilter;
    size_t Size;

    LenSalFilename = strlen(salfile) + 1;
    LenProcess = strlen(process) + 1;
    LenFilter = strlen(filter) + 1;
    Size = sizeof(*Req) + LenSalFilename + LenProcess + LenFilter;
    Req = (RPC_API_SAVE_SAL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSalFilename = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSalFilename, salfile, LenSalFilename);
    Req->OffsetProcess = Req->OffsetSalFilename + LenSalFilename;
    MEMCPY ((char*)Req + Req->OffsetProcess, process, LenProcess);
    Req->OffsetFilter = Req->OffsetProcess + LenProcess;
    MEMCPY ((char*)Req + Req->OffsetFilter, filter, LenFilter);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SAVE_SAL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API enum BB_DATA_TYPES __STDCALL__ XilEnv_GetSymbolRaw(const char* Symbol, const char* Process, int Flags, union BB_VARI *ret_Value)
{
    RPC_API_GET_SYMBOL_RAW_MESSAGE *Req;
    RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSymbol, LenProcess;
    size_t Size;

    LenSymbol = strlen(Symbol) + 1;
    LenProcess = strlen(Process) + 1;
    Size = sizeof(*Req) + LenSymbol + LenProcess;
    Req = (RPC_API_GET_SYMBOL_RAW_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSymbol = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSymbol, Symbol, LenSymbol);
    Req->OffsetProcess = Req->OffsetSymbol + LenSymbol;
    MEMCPY ((char*)Req + Req->OffsetProcess, Process, LenProcess);
    Req->Flags = Flags;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_SYMBOL_RAW_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    } else {
        *ret_Value = Ack.Value;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetSymbolRaw(const char* Symbol, const char* Process, int Flags, enum BB_DATA_TYPES DataType, union BB_VARI Value)
{
    RPC_API_SET_SYMBOL_RAW_MESSAGE *Req;
    RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSymbol, LenProcess;
    size_t Size;

    LenSymbol = strlen(Symbol) + 1;
    LenProcess = strlen(Process) + 1;
    Size = sizeof(*Req) + LenSymbol + LenProcess;
    Req = (RPC_API_SET_SYMBOL_RAW_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSymbol = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSymbol, Symbol, LenSymbol);
    Req->OffsetProcess = Req->OffsetSymbol + LenSymbol;
    MEMCPY ((char*)Req + Req->OffsetProcess, Process, LenProcess);
    Req->Flags = Flags;
    Req->DataType = DataType;
    Req->Value = Value;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_SYMBOL_RAW_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


// A2LLink

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetupLinkToExternProcess(const char *A2LFileName, const char *ProcessName, int UpdateFlag)
{
    RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE *Req;
    RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK Ack;
    int Ret;
    size_t LenA2LFileName, LenProcessName;
    size_t Size;

    LenA2LFileName = strlen(A2LFileName) + 1;
    LenProcessName = strlen(ProcessName) + 1;
    Size = sizeof(*Req) + LenA2LFileName + LenProcessName;
    Req = (RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetA2LFileName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetA2LFileName, A2LFileName, LenA2LFileName);
    Req->OffsetProcessName = Req->OffsetA2LFileName + LenA2LFileName;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);
    Req->UpdateFlag =UpdateFlag;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkToExternProcess(const char *ProcessName)
{
    RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE *Req;
    RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK Ack;
    int Ret;
    size_t LenProcessName;
    size_t Size;

    LenProcessName = strlen(ProcessName) + 1;
    Size = sizeof(*Req) + LenProcessName;
    Req = (RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetProcessName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_LINK_TO_EXTERN_PROCESS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetIndexFromLink(int LinkNr, const char *Label, int TypeMask)
{
    RPC_API_GET_INDEX_FROM_LINK_MESSAGE *Req;
    RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel;
    size_t Size;

    LenLabel = strlen(Label) + 1;
    Size = sizeof(*Req) + LenLabel;
    Req = (RPC_API_GET_INDEX_FROM_LINK_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, Label, LenLabel);
    Req->LinkNr = LinkNr;
    Req->TypeMask = TypeMask;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_INDEX_FROM_LINK_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetNextSymbolFromLink(int LinkNr, int Index, int TypeMask, const char *Filter, char *ret_Label, int MaxChar)
{
    RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE *Req;
    RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK *Ack;
    int Ret;
    size_t LenFilter, LenRetLabel;
    size_t Size;

    LenFilter = strlen(Filter) + 1;
    Size = sizeof(*Req) + LenFilter;
    Req = (RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetFilter = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetFilter, Filter, LenFilter);
    Req->LinkNr = LinkNr;
    Req->Index = Index;
    Req->TypeMask = TypeMask;
    Req->MaxChar = MaxChar;

    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_GET_NEXT_SYMBOL_FROM_LINK_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
   if (Ret <= 0) {
        return -1;
    }
    if (Ack->Header.ReturnValue < 0) {
        return -1;
    }
    LenRetLabel = strlen((char*)Ack + Ack->OffsetRetName) + 1;
    if ((int)LenRetLabel < MaxChar) {
        MEMCPY (ret_Label, (char*)Ack + Ack->OffsetRetName, LenRetLabel);
    } else {
        MEMCPY (ret_Label, (char*)Ack + Ack->OffsetRetName, MaxChar - 1);
        ret_Label[MaxChar-1] = 0;
    }
    return Ack->Header.ReturnValue;
}


CFUNC SCRPCDLL_API XILENV_LINK_DATA* __STDCALL__ XilEnv_GetDataFromLink(int LinkNr, int Index, XILENV_LINK_DATA *Reuse, int PhysFlag, const char **ret_Error)
{
    RPC_API_GET_DATA_FROM_LINK_MESSAGE *Req;
    RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK *Ack;
    int Ret;
    size_t Size;

    Size = sizeof(*Req);
    Req = (RPC_API_GET_DATA_FROM_LINK_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->LinkNr = LinkNr;
    Req->Index = Index;
    Req->PhysFlag = PhysFlag;

    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_GET_DATA_FROM_LINK_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        if (ret_Error != NULL) {
            *ret_Error = "unknown";
        }
        return NULL;
    }
    if (Ack->Header.ReturnValue < 0) {
        if (ret_Error != NULL) {
            if (Ack->OffsetRetError > 0) {
                *ret_Error = (char*)Ack + Ack->OffsetRetError;
            } else {
                *ret_Error = "unknown";
            }
        }
        return NULL;
    } else {
        // this must be free
        return XilEnvInternal_SetUpNewLinkData((char*)Ack + Ack->OffsetData, Reuse);
    }
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetDataToLink(int LinkNr, int Index, XILENV_LINK_DATA *Data, const char **ret_Error)
{
    RPC_API_SET_DATA_TO_LINK_MESSAGE *Req;
    RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK *Ack;
    int Ret;
    size_t LenData;
    size_t Size;

    LenData = *(int*)(Data->Data);   // the first 4 bytes are the size of the structure
    Size = sizeof(*Req) + LenData - 1;
    Req = (RPC_API_SET_DATA_TO_LINK_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetData = (int)sizeof(*Req) - 1;
    MEMCPY((char*)Req + Req->OffsetData, Data->Data, LenData);
    Req->LinkNr = LinkNr;
    Req->Index = Index;

    Ret = RemoteProcedureCallTransactDynBuf(SocketOrNamedPipe, Socket, RPC_API_SET_DATA_TO_LINK_CMD, &(Req->Header), Size, (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        if (ret_Error != NULL) {
            *ret_Error = "unknown";
        }
        return -1;
    }
    if (Ack->Header.ReturnValue < 0) {
        if (ret_Error != NULL) {
            if (Ack->OffsetRetError > 0) {
                *ret_Error = (char*)Ack + Ack->OffsetRetError;
            } else {
                *ret_Error = "unknown";
            }
        }
        return -1;
    } 
    return Ack->Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ReferenceMeasurementToBlackboard(int LinkNr, int Index, int DirFlags)
{
    RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE *Req;
    RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK Ack;
    int Ret;
    size_t Size;

    Size = sizeof(*Req);
    Req = (RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->LinkNr = LinkNr;
    Req->Index = Index;
    Req->DirFlags = DirFlags;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret <= 0) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DereferenceMeasurementFromBlackboard(int LinkNr, int Index)
{
    RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE *Req;
    RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK Ack;
    int Ret;
    size_t Size;

    Size = sizeof(*Req);
    Req = (RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->LinkNr = LinkNr;
    Req->Index = Index;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret <= 0) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

// CAN

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadCanVariante(const char* canfile, int channel)
{
    RPC_API_LOAD_CAN_VARIANTE_MESSAGE *Req;
    RPC_API_LOAD_CAN_VARIANTE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenCanFile;
    size_t Size;

    LenCanFile = strlen(canfile) + 1;
    Size = sizeof(*Req) + LenCanFile;
    Req = (RPC_API_LOAD_CAN_VARIANTE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = channel;
    Req->OffsetCanFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetCanFile, canfile, LenCanFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOAD_CAN_VARIANTE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadAndSelCanVariante(const char* canfile, int channel)
{
    RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE *Req;
    RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenCanFile;
    size_t Size;

    LenCanFile = strlen(canfile) + 1;
    Size = sizeof(*Req) + LenCanFile;
    Req = (RPC_API_LOAD_AND_SEL_CAN_VARIANTE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = channel;
    Req->OffsetCanFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetCanFile, canfile, LenCanFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOAD_AND_SEL_CAN_VARIANTE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AppendCanVariante(const char* canfile, int channel)
{
    RPC_API_APPEND_CAN_VARIANTE_MESSAGE *Req;
    RPC_API_APPEND_CAN_VARIANTE_MESSAGE_ACK Ack;
    int Ret;
    size_t LenCanFile;
    size_t Size;

    LenCanFile = strlen(canfile) + 1;
    Size = sizeof(*Req) + LenCanFile;
    Req = (RPC_API_APPEND_CAN_VARIANTE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = channel;
    Req->OffsetCanFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetCanFile, canfile, LenCanFile);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_APPEND_CAN_VARIANTE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_DelAllCanVariants(void)
{
    RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE *Req;
    RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_DEL_ALL_CAN_VARIANTE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DEL_ALL_CAN_VARIANTE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
}


CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TransmitCAN (int channel, int id, int ext, int size, unsigned char data0, unsigned char data1,
                                                   unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5,
                                                   unsigned char data6, unsigned char data7)
{
    RPC_API_TRANSMIT_CAN_MESSAGE *Req;
    RPC_API_TRANSMIT_CAN_MESSAGE_ACK Ack;
    int Ret;
    size_t LenCanData;
    size_t Size;

    LenCanData = 8;
    Size = sizeof(*Req) + LenCanData;
    Req = (RPC_API_TRANSMIT_CAN_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = channel;
    Req->Id = id;
    Req->Ext = ext;
    Req->Size = size;
    Req->Data[0] = data0;
    Req->Data[1] = data1;
    Req->Data[2] = data2;
    Req->Data[3] = data3;
    Req->Data[4] = data4;
    Req->Data[5] = data5;
    Req->Data[6] = data6;
    Req->Data[7] = data7;
    Req->Offset_uint8_Size_Data = (int32_t)sizeof(*Req) - 1;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_TRANSMIT_CAN_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TransmitCANFd (int channel, int id, int ext, int size, unsigned char *data)
{
    RPC_API_TRANSMIT_CAN_FD_MESSAGE *Req;
    RPC_API_TRANSMIT_CAN_FD_MESSAGE_ACK Ack;
    int Ret;
    size_t LenCanData;
    size_t Size;

    LenCanData = 64;
    Size = sizeof(*Req) + LenCanData;
    Req = (RPC_API_TRANSMIT_CAN_FD_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = channel;
    Req->Id = id;
    Req->Ext = ext;
    if (size > 64) size = 64;
    Req->Size = size;
    MEMCPY(Req->Data, data, size);
    Req->Offset_uint8_Size_Data = (int32_t)sizeof(*Req) - 1;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_TRANSMIT_CAN_FD_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_OpenCANQueue (int Depth)
{
    RPC_API_OPEN_CAN_QUEUE_MESSAGE *Req;
    RPC_API_OPEN_CAN_QUEUE_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_OPEN_CAN_QUEUE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Depth = Depth;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_OPEN_CAN_QUEUE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_OpenCANFdQueue (int Depth, int FdFlag)
{
    RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE *Req;
    RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_OPEN_CAN_FD_QUEUE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Depth = Depth;
    Req->FdFlag = FdFlag;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_OPEN_CAN_FD_QUEUE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCANAcceptanceWindows (int Elements, CAN_ACCEPT_MASK* pWindows)
{
    RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE *Req;
    RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE_ACK Ack;
    int Ret;
    size_t Size;

    Size = sizeof(*Req) - 1 + Elements * sizeof(*pWindows);
    Req = (RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Size = Elements;
    Req->Offset_CanAcceptance_Size_Windows =  sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->Offset_CanAcceptance_Size_Windows, pWindows, Elements * sizeof(*pWindows));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_CAN_ACCEPTANCE_WINDOWS_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_FlushCANQueue (int Flags)
{
    RPC_API_FLUSH_CAN_QUEUE_MESSAGE *Req;
    RPC_API_FLUSH_CAN_QUEUE_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_FLUSH_CAN_QUEUE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Flags = Flags;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_FLUSH_CAN_QUEUE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

// Remark: the return buffer will be overwriten by the next remote call
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ReadCANQueue (int ReadMaxElements,
                                                    CAN_FIFO_ELEM* pElements)
{
    RPC_API_READ_CAN_QUEUE_MESSAGE *Req;
    RPC_API_READ_CAN_QUEUE_MESSAGE_ACK *Ack;
    int Ret;

    Req = (RPC_API_READ_CAN_QUEUE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->ReadMaxElements = ReadMaxElements;
    Ret = RemoteProcedureCallTransactDynBuf (SocketOrNamedPipe, Socket, RPC_API_READ_CAN_QUEUE_CMD, &(Req->Header), sizeof(*Req), (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return -1;
    }
    if (Ack->Header.ReturnValue > 0) {
        MEMCPY(pElements, (char*)Ack + Ack->Offset_CanObject_ReadElements_Messages, Ack->Header.ReturnValue * sizeof(CAN_FIFO_ELEM));
    }
    return Ack->Header.ReturnValue;
}

// Remark: the return buffer will be overwriten by the next remote call
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ReadCANFdQueue (int ReadMaxElements,
                                                      CAN_FD_FIFO_ELEM* pElements)
{
    RPC_API_READ_CAN_FD_QUEUE_MESSAGE *Req;
    RPC_API_READ_CAN_FD_QUEUE_MESSAGE_ACK *Ack;
    int Ret;

    Req = (RPC_API_READ_CAN_FD_QUEUE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->ReadMaxElements = ReadMaxElements;
    Ret = RemoteProcedureCallTransactDynBuf (SocketOrNamedPipe, Socket, RPC_API_READ_CAN_FD_QUEUE_CMD, &(Req->Header), sizeof(*Req), (RPC_API_BASE_MESSAGE_ACK **)&Ack);
    if (Ret <= 0) {
        return -1;
    }
    if (Ack->Header.ReturnValue > 0) {
        MEMCPY(pElements, (char*)Ack + Ack->Offset_CanFdObject_ReadElements_Messages, Ack->Header.ReturnValue * sizeof(CAN_FD_FIFO_ELEM));
    }
    return Ack->Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TransmitCANQueue (int WriteElements,
                                                        CAN_FIFO_ELEM *pElements)
{
    RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE *Req;
    RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE_ACK Ack;
    int Ret;
    size_t Size;

    Size = sizeof(*Req) - 1 + WriteElements * sizeof(CAN_FIFO_ELEM);
    Req = (RPC_API_TRANSMIT_CAN_QUEUE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->WriteElements = WriteElements;
    Req->Offset_CanObject_WriteElements_Messages =  sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->Offset_CanObject_WriteElements_Messages, pElements, WriteElements * sizeof(CAN_FIFO_ELEM));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_TRANSMIT_CAN_QUEUE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}
   
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TransmitCANFdQueue (int WriteElements,
                                                          CAN_FD_FIFO_ELEM *pElements)
{
    RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE *Req;
    RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE_ACK Ack;
    int Ret;
    size_t Size;

    Size = sizeof(*Req) - 1 + WriteElements * sizeof(CAN_FD_FIFO_ELEM);
    Req = (RPC_API_TRANSMIT_CAN_FD_QUEUE_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->WriteElements = WriteElements;
    Req->Offset_CanFdObject_WriteElements_Messages =  sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->Offset_CanFdObject_WriteElements_Messages, pElements, WriteElements * sizeof(CAN_FD_FIFO_ELEM));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_TRANSMIT_CAN_FD_QUEUE_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CloseCANQueue (void)
{
    RPC_API_CLOSE_CAN_QUEUE_MESSAGE *Req;
    RPC_API_CLOSE_CAN_QUEUE_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_CLOSE_CAN_QUEUE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CLOSE_CAN_QUEUE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


// CAN bit error
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanErr (int Channel, int Id, int Startbit, int Bitsize, char *Byteorder, uint32_t Cycles, uint64_t BitErrValue)
{
    RPC_API_SET_CAN_ERR_MESSAGE *Req;
    RPC_API_SET_CAN_ERR_MESSAGE_ACK Ack;
    int Ret;
    size_t LenByteorder;
    size_t Size;

    LenByteorder = strlen(Byteorder) + 1;
    Size = sizeof(*Req) + LenByteorder;
    Req = (RPC_API_SET_CAN_ERR_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = Channel;
    Req->Id = Id;
    Req->Startbit = Startbit;
    Req->Bitsize = Bitsize;
    Req->OffsetByteorder = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetByteorder, Byteorder, LenByteorder);
    Req->Cycles = Cycles;
    Req->BitErrValue = BitErrValue;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_CAN_ERR_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanErrSignalName (int Channel, int Id, char *Signalname, uint32_t Cycles, uint64_t BitErrValue)
{
    RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE *Req;
    RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSignalname;
    size_t Size;

    LenSignalname = strlen(Signalname) + 1;
    Size = sizeof(*Req) + LenSignalname;
    Req = (RPC_API_SET_CAN_ERR_SIGNAL_NAME_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = Channel;
    Req->Id = Id;
    Req->OffsetSignalName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSignalName, Signalname, LenSignalname);
    Req->Cycles = Cycles;
    Req->BitErrValue = BitErrValue;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_CAN_ERR_SIGNAL_NAME_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ClearCanErr (void)
{
    RPC_API_CLEAR_CAN_ERR_MESSAGE *Req;
    RPC_API_CLEAR_CAN_ERR_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_CLEAR_CAN_ERR_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_CLEAR_CAN_ERR_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanSignalConversion (int Channel, int Id, const char *Signalname, const char *Conversion)
{
    RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE *Req;
    RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSignalname, LenConvertion;
    size_t Size;

    LenSignalname = strlen(Signalname) + 1;
    LenConvertion = strlen(Conversion) + 1;
    Size = sizeof(*Req) + LenSignalname + LenConvertion;
    Req = (RPC_API_SET_CAN_SIGNAL_CONVERTION_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = Channel;
    Req->Id = Id;
    Req->OffsetSignalName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSignalName, Signalname, LenSignalname);
    Req->OffsetConvertion = Req->OffsetSignalName + LenSignalname;
    MEMCPY ((char*)Req + Req->OffsetConvertion, Conversion, LenConvertion);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_CAN_SIGNAL_CONVERTION_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ResetCanSignalConversion (int Channel, int Id, const char *Signalname)
{
    RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE *Req;
    RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSignalname;
    size_t Size;

    LenSignalname = strlen(Signalname) + 1;
    Size = sizeof(*Req) + LenSignalname;
    Req = (RPC_API_RESET_CAN_SIGNAL_CONVERTION_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Channel = Channel;
    Req->Id = Id;
    Req->OffsetSignalName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSignalName, Signalname, LenSignalname);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_RESET_CAN_SIGNAL_CONVERTION_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ResetAllCanSignalConversion (int Channel, int Id)
{
    RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_MESSAGE *Req;
    RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Channel = Channel;
    Req->Id = Id;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_RESET_ALL_CAN_SIGNAL_CONVERTION_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanChannelCount (int ChannelCount)
{
    RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE *Req;
    RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE_ACK Ack;
    int Ret;
 
    Req = (RPC_API_SET_CAN_CHANNEL_COUNT_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->ChannelCount = ChannelCount;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_CAN_CHANNEL_COUNT_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret != sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanChannelStartupState (int Channel, int StartupState)
{
    RPC_API_SET_CAN_CHANNEL_STARTUP_STATE_MESSAGE *Req;
    RPC_API_SET_CAN_CHANNEL_STARTUP_STATE_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_SET_CAN_CHANNEL_STARTUP_STATE_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Channel = Channel;
    Req->StartupState = StartupState;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_CAN_CHANNEL_STARTUP_STATE_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret != sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCANRecorder (const char *par_FileName,
                                                        const char *par_TriggerEqu,
                                                        int par_DisplayColumnCounterFlag,
                                                        int par_DisplayColumnTimeAbsoluteFlag,
                                                        int par_DisplayColumnTimeDiffFlag,
                                                        int par_DisplayColumnTimeDiffMinMaxFlag, 
                                                        int Elements, CAN_ACCEPT_MASK* pWindows)
{
    UNUSED(par_DisplayColumnCounterFlag);
    UNUSED(par_DisplayColumnTimeAbsoluteFlag);
    UNUSED(par_DisplayColumnTimeDiffFlag);
    UNUSED(par_DisplayColumnTimeDiffMinMaxFlag);
    RPC_API_START_CAN_RECORDER_MESSAGE *Req;
    RPC_API_START_CAN_RECORDER_MESSAGE_ACK Ack;
    int Ret;
    size_t Size;
    size_t LenFileName = strlen(par_FileName) + 1;
    size_t LenTriggerEqu = strlen(par_TriggerEqu) + 1;

    Size = sizeof(*Req) - 1 + LenFileName + LenTriggerEqu + Elements * sizeof(*pWindows);
    Req = (RPC_API_START_CAN_RECORDER_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Offset_FileName = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->Offset_FileName, par_FileName, LenFileName);
    Req->Offset_Equation = Req->Offset_FileName + LenFileName;
    MEMCPY ((char*)Req + Req->Offset_Equation, par_TriggerEqu, LenTriggerEqu);

    Req->Size = Elements;
    Req->Offset_CanAcceptance_Size_Windows =  Req->Offset_Equation + LenTriggerEqu;
    MEMCPY ((char*)Req + Req->Offset_CanAcceptance_Size_Windows, pWindows, Elements * sizeof(*pWindows));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_CAN_RECORDER_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopCANRecorder (void)
{
    RPC_API_STOP_CAN_RECORDER_MESSAGE *Req;
    RPC_API_STOP_CAN_RECORDER_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_STOP_CAN_RECORDER_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_CAN_RECORDER_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

// CCP

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadCCPConfig(int Connection, const char* CcpFile)
{
    RPC_API_LOAD_CCP_CONFIG_MESSAGE *Req;
    RPC_API_LOAD_CCP_CONFIG_MESSAGE_ACK Ack;
    int Ret;
    size_t LenCcpFile;
    size_t Size;

    LenCcpFile = strlen(CcpFile) + 1;
    Size = sizeof(*Req) + LenCcpFile;
    Req = (RPC_API_LOAD_CCP_CONFIG_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Connection = Connection;
    Req->OffsetCcpFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetCcpFile, CcpFile, LenCcpFile);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOAD_CCP_CONFIG_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPBegin(int Connection)
{
    RPC_API_START_CCP_BEGIN_MESSAGE *Req;
    RPC_API_START_CCP_BEGIN_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_START_CCP_BEGIN_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_CCP_BEGIN_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPAddVar(int Connection, const char* Label)
{
    RPC_API_START_CCP_ADD_VAR_MESSAGE *Req;
    RPC_API_START_CCP_ADD_VAR_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel;
    size_t Size;

    LenLabel = strlen(Label) + 1;
    Size = sizeof(*Req) + LenLabel;
    Req = (RPC_API_START_CCP_ADD_VAR_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Connection = Connection;
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, Label, LenLabel);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_CCP_ADD_VAR_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPEnd(int Connection)
{
    RPC_API_START_CCP_END_MESSAGE *Req;
    RPC_API_START_CCP_END_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_START_CCP_END_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_CCP_END_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopCCP(int Connection)
{
    RPC_API_STOP_CCP_MESSAGE *Req;
    RPC_API_STOP_CCP_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_STOP_CCP_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_CCP_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPCalBegin(int Connection)
{
    RPC_API_START_CCP_CAL_BEGIN_MESSAGE *Req;
    RPC_API_START_CCP_CAL_BEGIN_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_START_CCP_CAL_BEGIN_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_CCP_CAL_BEGIN_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPCalAddVar(int Connection, const char* Label)
{
    RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE *Req;
    RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel;
    size_t Size;

    LenLabel = strlen(Label) + 1;
    Size = sizeof(*Req) + LenLabel;
    Req = (RPC_API_START_CCP_CAL_ADD_VAR_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Connection = Connection;
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, Label, LenLabel);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_CCP_CAL_ADD_VAR_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPCalEnd(int Connection)
{
    RPC_API_START_CCP_CAL_END_MESSAGE *Req;
    RPC_API_START_CCP_CAL_END_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_START_CCP_CAL_END_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_CCP_CAL_END_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopCCPCal(int Connection)
{
    RPC_API_STOP_CCP_CAL_MESSAGE *Req;
    RPC_API_STOP_CCP_CAL_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_STOP_CCP_CAL_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_CCP_CAL_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}


// XCP

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadXCPConfig(int Connection, const char* XcpFile)
{
    RPC_API_LOAD_XCP_CONFIG_MESSAGE *Req;
    RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK Ack;
    int Ret;
    size_t LenXcpFile;
    size_t Size;

    LenXcpFile = strlen(XcpFile) + 1;
    Size = sizeof(*Req) + LenXcpFile;
    Req = (RPC_API_LOAD_XCP_CONFIG_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Connection = Connection;
    Req->OffsetXcpFile = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetXcpFile, XcpFile, LenXcpFile);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_LOAD_XCP_CONFIG_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPBegin(int Connection)
{
    RPC_API_START_XCP_BEGIN_MESSAGE *Req;
    RPC_API_START_XCP_BEGIN_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_START_XCP_BEGIN_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_XCP_BEGIN_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPAddVar(int Connection, const char* Label)
{
    RPC_API_START_XCP_ADD_VAR_MESSAGE *Req;
    RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel;
    size_t Size;

    LenLabel = strlen(Label) + 1;
    Size = sizeof(*Req) + LenLabel;
    Req = (RPC_API_START_XCP_ADD_VAR_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Connection = Connection;
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, Label, LenLabel);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_XCP_ADD_VAR_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPEnd(int Connection)
{
    RPC_API_START_XCP_END_MESSAGE *Req;
    RPC_API_START_XCP_END_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_START_XCP_END_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_XCP_END_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopXCP(int Connection)
{
    RPC_API_STOP_XCP_MESSAGE *Req;
    RPC_API_STOP_XCP_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_STOP_XCP_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_XCP_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPCalBegin(int Connection)
{
    RPC_API_START_XCP_CAL_BEGIN_MESSAGE *Req;
    RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_START_XCP_CAL_BEGIN_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_XCP_CAL_BEGIN_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPCalAddVar(int Connection, const char* Label)
{
    RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE *Req;
    RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK Ack;
    int Ret;
    size_t LenLabel;
    size_t Size;

    LenLabel = strlen(Label) + 1;
    Size = sizeof(*Req) + LenLabel;
    Req = (RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->Connection = Connection;
    Req->OffsetLabel = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetLabel, Label, LenLabel);
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_XCP_CAL_ADD_VAR_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPCalEnd(int Connection)
{
    RPC_API_START_XCP_CAL_END_MESSAGE *Req;
    RPC_API_START_XCP_CAL_END_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_START_XCP_CAL_END_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_START_XCP_CAL_END_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopXCPCal(int Connection)
{
    RPC_API_STOP_XCP_CAL_MESSAGE *Req;
    RPC_API_STOP_XCP_CAL_MESSAGE_ACK Ack;
    int Ret;

    Req = (RPC_API_STOP_XCP_CAL_MESSAGE*)RemoteProcedureGetTransmitBuffer(sizeof(*Req));
    MEMSET(Req, 0, sizeof(*Req));
    Req->Connection = Connection;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_STOP_XCP_CAL_CMD, &(Req->Header), sizeof(*Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}



CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ReferenceSymbol(const char* Symbol, const char *DisplayName, const char* Process, const char *Unit, int ConversionType, const char *Conversion,
                                                      double Min, double Max, int Color, int Width, int Precision, int Flags)
{
    RPC_API_REFERENCE_SYMBOL_MESSAGE *Req;
    RPC_API_REFERENCE_SYMBOL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSymbol, LenDisplayName, LenProcess, LenUnit, LenConversion;
    size_t Size;
    if (DisplayName == NULL) DisplayName = "";
    if (Unit == NULL) Unit = "";
    if (Conversion == NULL) Conversion = "";

    LenSymbol = strlen(Symbol) + 1;
    LenDisplayName = strlen(DisplayName) + 1;
    LenProcess = strlen(Process) + 1;
    LenUnit = strlen(Unit) + 1;
    LenConversion = strlen(Conversion) + 1;
    Size = sizeof(*Req) + LenSymbol + LenProcess + LenUnit + LenConversion;
    Req = (RPC_API_REFERENCE_SYMBOL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSymbol = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSymbol, Symbol, LenSymbol);
    Req->OffsetDisplayName = Req->OffsetSymbol + LenSymbol;
    MEMCPY ((char*)Req + Req->OffsetDisplayName, DisplayName, LenDisplayName);
    Req->OffsetProcess = Req->OffsetDisplayName + LenDisplayName;
    MEMCPY ((char*)Req + Req->OffsetProcess, Process, LenProcess);
    
    Req->OffsetUnit = Req->OffsetProcess + LenProcess;
    MEMCPY ((char*)Req + Req->OffsetUnit, Unit, LenUnit);

    Req->ConversionType = ConversionType;
    Req->OffsetConversion = Req->OffsetUnit + LenUnit;
    MEMCPY ((char*)Req + Req->OffsetConversion, Conversion, LenConversion);
    Req->Min = Min;
    Req->Max = Max;
    Req->Color = Color;
    Req->Width = Width;
    Req->Precision = Precision;
    Req->Flags = Flags;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_REFERENCE_SYMBOL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DereferenceSymbol(const char* Symbol, const char* Process, int Flags)
{
    RPC_API_DEREFERENCE_SYMBOL_MESSAGE *Req;
    RPC_API_DEREFERENCE_SYMBOL_MESSAGE_ACK Ack;
    int Ret;
    size_t LenSymbol, LenProcess;
    size_t Size;

    LenSymbol = strlen(Symbol) + 1;
    LenProcess = strlen(Process) + 1;
    Size = sizeof(*Req) + LenSymbol + LenProcess;
    Req = (RPC_API_DEREFERENCE_SYMBOL_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetSymbol = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetSymbol, Symbol, LenSymbol);
    Req->OffsetProcess = Req->OffsetSymbol + LenSymbol;
    MEMCPY ((char*)Req + Req->OffsetProcess, Process, LenProcess);
    Req->Flags = Flags;

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_DEREFERENCE_SYMBOL_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetRaw(int vid, union BB_VARI *ret_Value)
{
    RPC_API_GET_RAW_MESSAGE Req = {0};
    RPC_API_GET_RAW_MESSAGE_ACK Ack;
    int Ret;
    
    Req.Vid = vid;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_GET_RAW_CMD, &(Req.Header), sizeof(Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    *ret_Value = Ack.Value;
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetRaw(int vid, enum BB_DATA_TYPES Type, union BB_VARI Value, int Flags)
{
    RPC_API_SET_RAW_MESSAGE Req = {0};
    RPC_API_SET_RAW_MESSAGE_ACK Ack;
    int Ret;
    
    Req.Vid = vid;
    Req.Type = Type;
    Req.Value = Value;
    Req.Flags = Flags;
    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_SET_RAW_CMD, &(Req.Header), sizeof(Req), &(Ack.Header), sizeof(Ack));
    if (Ret < (int)sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ExportA2lMeasurementList(const char* MeasureList, const char* Process)
{
    RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE *Req;
    RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK Ack;
    int Ret;
    size_t LenRefList, LenProcess;
    size_t Size;

    LenRefList = strlen(MeasureList) + 1;
    LenProcess = strlen(Process) + 1;
    Size = sizeof(*Req) + LenRefList + LenProcess;
    Req = (RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetRefList = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetRefList, MeasureList, LenRefList);
    Req->OffsetProcess = Req->OffsetRefList + LenRefList;
    MEMCPY ((char*)Req + Req->OffsetProcess, Process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_EXPORT_A2L_MEASUREMENT_LIST_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ImportA2lMeasurementList(const char* MeasureList, const char* Process)
{
    RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE *Req;
    RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK Ack;
    int Ret;
    size_t LenRefList, LenProcess;
    size_t Size;

    LenRefList = strlen(MeasureList) + 1;
    LenProcess = strlen(Process) + 1;
    Size = sizeof(*Req) + LenRefList + LenProcess;
    Req = (RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE*)RemoteProcedureGetTransmitBuffer(Size);
    MEMSET(Req, 0, Size);
    Req->OffsetRefList = (int32_t)sizeof(*Req) - 1;
    MEMCPY ((char*)Req + Req->OffsetRefList, MeasureList, LenRefList);
    Req->OffsetProcess = Req->OffsetRefList + LenRefList;
    MEMCPY ((char*)Req + Req->OffsetProcess, Process, LenProcess);

    Ret = RemoteProcedureCallTransact(SocketOrNamedPipe, Socket, RPC_API_IMPORT_A2L_MEASUREMENT_LIST_CMD, &(Req->Header), Size, &(Ack.Header), sizeof(Ack));
    if (Ret < sizeof(Ack)) {
        return -1;
    }
    return Ack.Header.ReturnValue;
}

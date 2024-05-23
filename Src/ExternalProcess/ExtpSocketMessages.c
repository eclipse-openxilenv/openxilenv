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

// nicht direkt stdint.h da es den Header unter VS 2003 nicht gibt
#include "my_stdint.h"

#include "Platform.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define INVALID_HANDLE_VALUE (-1)
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define SOCKET int
#endif

#include "XilEnvExtProc.h"
#include "ExtpProcessAndTaskInfos.h"

#include "ExtpSocketMessages.h"

#ifdef _WIN32
#ifndef __GNUC__
#pragma comment(lib, "Ws2_32.lib")
#endif
#endif

static HANDLE XilEnvInternal_ConnectToSocket (char *par_InstanceName, char *par_ServerName, int par_Timout_ms, int par_ErrMessageFlag)
{
    SOCKET Socket;
    struct addrinfo Tip;
    struct addrinfo *AddrInfoList;
    struct addrinfo *AddrInfo;
    char PortString[32];
    char ServerName[MAX_PATH];
    char *p;
    int ErrCode;
#ifdef _WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        if (par_ErrMessageFlag) ThrowError (1, "Call of WSAStartup failed");
        return INVALID_HANDLE_VALUE;
    }
#endif
    Socket = INVALID_SOCKET;

    memset(&Tip, 0, sizeof(Tip));
    Tip.ai_family = AF_INET;
    Tip.ai_socktype = SOCK_STREAM;
    Tip.ai_protocol = IPPROTO_TCP;

    strcpy (ServerName, par_ServerName);
    p = strstr(ServerName, "@");
    if (p != NULL) {
        sprintf (PortString, "%s", p + 1);  // own port defined
        *p = 0;
    } else {
        sprintf (PortString, "%i", 1800);  // use default port
    }

    AddrInfo = NULL;
    if((ErrCode = getaddrinfo(ServerName, PortString, &Tip, &AddrInfoList)) != 0) {
        if (par_ErrMessageFlag) ThrowError (1, "Call of getaddrinfo failed with error: %d (%s)", ErrCode, gai_strerror(ErrCode));
        return INVALID_HANDLE_VALUE;
    }
    AddrInfo = AddrInfoList;
    while(AddrInfo != NULL) {
        Socket = socket(AddrInfo->ai_family, AddrInfo->ai_socktype,  AddrInfo->ai_protocol);
        if (Socket == INVALID_SOCKET) {
#ifdef _WIN32
            if (par_ErrMessageFlag) ThrowError (1, "Create socket failed with error code: %ld", WSAGetLastError());
#else
            if (par_ErrMessageFlag) ThrowError(1, "Create socket failed with error code: %i\n", errno);
#endif
            return INVALID_HANDLE_VALUE;
        }

        // Connect to XILEnv server.
        if(connect (Socket, AddrInfo->ai_addr, (int)AddrInfo->ai_addrlen) == SOCKET_ERROR) {
#ifdef _WIN32
            closesocket((SOCKET)Socket);
#else
            close(Socket);
#endif
            Socket = (SOCKET)INVALID_HANDLE_VALUE;
            AddrInfo = AddrInfo->ai_next;
        } else {
            AddrInfo = NULL;
        }
    }

    freeaddrinfo(AddrInfoList);

    if (Socket == INVALID_SOCKET) {
        if (par_ErrMessageFlag) ThrowError (1, "Cannot connect to server");
        return INVALID_HANDLE_VALUE;
    }

    return (HANDLE)Socket;
}


static void XilEnvInternal_DisConnectFromSocket (HANDLE Socket)
{
#ifdef _WIN32
    closesocket((SOCKET)Socket);
#else
    close(Socket);
#endif
}

static BOOL XilEnvInternal_TransactMessageSocket (HANDLE Socket,
                                                   LPVOID lpInBuffer,
                                                   DWORD nInBufferSize,
                                                   LPVOID lpOutBuffer,
                                                   DWORD nOutBufferSize,
                                                   LPDWORD lpBytesRead)
{
    DWORD buffer_pos = 0;

    do {
        int SendBytes = send ((SOCKET)Socket, (const char*)lpInBuffer + buffer_pos, nInBufferSize - buffer_pos, 0);
        if (SendBytes == SOCKET_ERROR) {
            return FALSE;
        }
        buffer_pos += SendBytes;
    } while (buffer_pos < nInBufferSize);

    buffer_pos = 0;

    /*do*/ {
        int ReceiveBytes = recv ((SOCKET)Socket, (char*)lpOutBuffer + buffer_pos, nOutBufferSize - buffer_pos, 0);
        if (ReceiveBytes == SOCKET_ERROR) {
            return FALSE;
        }
        buffer_pos += ReceiveBytes;
    } /*while (buffer_pos < nOutBufferSize);*/
    *lpBytesRead = buffer_pos;
    return TRUE;
}


BOOL XilEnvInternal_ReadMessageSocket (HANDLE Socket,
                                       LPVOID lpBuffer,
                                       DWORD nNumberOfBytesToRead,
                                       LPDWORD lpNumberOfBytesRead)
{
    int ReceiveBytes;
    ReceiveBytes = recv ((SOCKET)Socket, (char*)lpBuffer, nNumberOfBytesToRead, 0);
    if (ReceiveBytes <= 0) {
        return FALSE;
    } else {
        *lpNumberOfBytesRead = (DWORD)ReceiveBytes;
        return TRUE;
    }
}

BOOL XilEnvInternal_WriteMessageSocket (HANDLE Socket,
                                        LPCVOID lpBuffer,
                                        DWORD nNumberOfBytesToWrite,
                                        LPDWORD lpNumberOfBytesWritten)
{
    int SendBytes;
    SendBytes = send ((SOCKET)Socket, (const char*)lpBuffer, nNumberOfBytesToWrite, 0);
    if (SendBytes <= 0) {
        return FALSE;
    } else {
        *lpNumberOfBytesWritten = SendBytes;
        return TRUE;
    }
}


int XilEnvInternal_InitSocketCommunication(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo)
{
    TaskInfo->ConnectTo = XilEnvInternal_ConnectToSocket;
    TaskInfo->DisconnectFrom = XilEnvInternal_DisConnectFromSocket;
	TaskInfo->TransactMessage = XilEnvInternal_TransactMessageSocket;
	TaskInfo->ReadMessage = XilEnvInternal_ReadMessageSocket;
    TaskInfo->WriteMessage = XilEnvInternal_WriteMessageSocket;

    return 0;
}

int XilEnvInternal_IsSocketInstanceIsRunning(char *par_InstanceName, char *par_ServerName, int par_Timout_ms)
{
    PIPE_API_PING_MESSAGE Req;
    PIPE_API_PING_MESSAGE_ACK Ack;
    DWORD BytesRead;
    int Ret = 0;

    HANDLE Socket = XilEnvInternal_ConnectToSocket(par_InstanceName, par_ServerName, par_Timout_ms, 0);
    if (Socket == INVALID_HANDLE_VALUE) {
        return 0;   // is not running
    }
    Req.Command = PIPE_API_PING_CMD;
    Ret = XilEnvInternal_TransactMessageSocket(Socket, &Req, sizeof(Req), &Ack, sizeof(Ack), &BytesRead);
    XilEnvInternal_DisConnectFromSocket(Socket);
    return Ret;
}

int XilEnvInternal_WaitTillSocketInstanceIsRunning(char *par_InstanceName, char *par_ServerName, int par_Timout_ms)
{
    PIPE_API_PING_MESSAGE Req;
    PIPE_API_PING_MESSAGE_ACK Ack;
    DWORD BytesRead;
    int Ret = 0;

#if (_MSC_Ver < 1500)
    uint32_t End = GetTickCount() + par_Timout_ms;
#else
    uint64_t End = GetTickCount64() + par_Timout_ms;
#endif

    while (1) {
        HANDLE Socket = XilEnvInternal_ConnectToSocket(par_InstanceName, par_ServerName, par_Timout_ms, 0);
        if (Socket == INVALID_HANDLE_VALUE) {
#if (_MSC_Ver < 1500)
            if (GetTickCount() > End) break;   // Timeout expired
#else
            if (GetTickCount64() > End) break;   // Timeout Zeit abgelaufen
#endif

#ifdef _WIN32
            Sleep(100);
#else
            usleep(100 * 1000);
#endif
            continue;   // is not running
        }
        Req.Command = PIPE_API_PING_CMD;
        Ret = XilEnvInternal_TransactMessageSocket(Socket, &Req, sizeof(Req), &Ack, sizeof(Ack), &BytesRead);
        XilEnvInternal_DisConnectFromSocket(Socket);
        if (Ret) break;
    }
    return Ret;
}

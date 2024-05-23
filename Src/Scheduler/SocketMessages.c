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
#include "Platform.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include "Config.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "tcb.h"
#include "Scheduler.h"
#include "GetNextStructEntry.h"
#include "Scheduler.h"
#include "ScBbCopyLists.h"
#include "BaseMessages.h"
#include "SocketMessages.h"

#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib") //Winsock library
#endif

// #define USE_DEBUG_API

#define TERMINATE_EXTERN_PROCESS_IMMEDIATELY  -12345678


static int LoggingFlag;
static FILE *LoggingFile;

static int NewConnectionsAllowed;
static SOCKET ToCloseSocket;

static SOCKET CreateLoginSocket (char *par_Prefix)
{
    SOCKET sock_descriptor;
    struct sockaddr_in serv_addr;

    sock_descriptor = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (sock_descriptor == INVALID_SOCKET) {
#else
    if (sock_descriptor < 0) {
#endif
        ThrowError (1, "Failed creating socket");
        return (SOCKET)-1;
    }

    {
        int optval = 1;
        setsockopt(sock_descriptor, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof (optval));
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_addr.s_addr = INADDR_ANY;

    serv_addr.sin_port = htons((unsigned short)atoi(par_Prefix));

    if (bind(sock_descriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        ThrowError (1, "Failed to bind");
#ifdef _WIN32
        return INVALID_SOCKET;
#else
        return -1;
#endif
    }

    listen(sock_descriptor, 5);

    NewConnectionsAllowed = 1;

    return sock_descriptor;
}

static DWORD Socket_ReadFile (HANDLE Socket, void *Buffer, DWORD BufferSize, LPDWORD BytesRead)
{
    int len;
    len = recv((SOCKET)Socket, Buffer, (int)BufferSize, 0);
    if (len <= 0) return FALSE;
    *BytesRead = (DWORD)len;
    return TRUE;
}

static DWORD Socket_WriteFile (HANDLE Socket, void *Buffer, int nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten)
{
    int len = send((SOCKET)Socket, Buffer, nNumberOfBytesToWrite, 0);
    if (len <= 0) {
        return FALSE;
    }
    *lpNumberOfBytesWritten = (DWORD)len;
    return TRUE;
}

static void Socket_CloseConnectionToExternProcess (HANDLE Socket)
{
#ifdef _WIN32
    closesocket((SOCKET)Socket);
#else
    close((SOCKET)Socket);
#endif
}

static HANDLE BuildHandleFrom32BitInt (ALIVE_PING_HANDLE_ULONG x)
{
    union {
        uint32_t x32[2];
        HANDLE x64;
    } v;
    v.x32[0] = x;
    v.x32[1] = 0;
    return v.x64;
}

#ifdef _WIN32
static DWORD WINAPI SocketLoginThreadProc (LPVOID lpParam)
#else
static void* SocketLoginThreadProc (void* lpParam)
#endif
{
    SOCKET Socket;
    DWORD Success, BytesRead, BytesWritten;
    PIPE_API_LOGIN_MESSAGE *LoginMessage;
    PIPE_API_LOGIN_MESSAGE_ACK LoginMessageAck;
#ifdef _WIN32
    int iResult;
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        ThrowError (1, "WSAStartup failed with error: %d\n", iResult);
        return (DWORD)-1;
    }
#endif

    ToCloseSocket = Socket = CreateLoginSocket((char *)lpParam);
#ifdef _WIN32
    if (Socket == INVALID_SOCKET) return 1;
#else
    if (Socket < 0) return NULL;
#endif

    LoginMessage = (PIPE_API_LOGIN_MESSAGE*)my_malloc (PIPE_MESSAGE_BUFSIZE);
    for (;;) {  // Endless loop
        struct sockaddr_in client_addr;
#ifdef _WIN32
        int size;
#else
        socklen_t size;
#endif
        size = sizeof(client_addr);
        SOCKET conn_desc;

        // The server will blocks on this call until a client tries to establish connection.
        // If a connection is established, it returns a connected socket descriptor this is different
        // from the one created earlier.
        conn_desc = accept(Socket, (struct sockaddr *)&client_addr, &size);
#ifdef _WIN32
        if (conn_desc == INVALID_SOCKET) {
#else
        if (conn_desc <= 0) {
#endif
            if (!NewConnectionsAllowed) {
#ifdef _WIN32
                closesocket(Socket);
#else
                close(Socket);
#endif
                NewConnectionsAllowed = 2;
                break; // for(;;)
            }
            ThrowError (1, "Failed accepting connection\n");
        } else {
            Success = Socket_ReadFile ((HANDLE)conn_desc, (void*)LoginMessage, PIPE_MESSAGE_BUFSIZE, &BytesRead);
            if (Success) {
                if (LoggingFlag) MessageLoggingToFile (LoggingFile,__LINE__, 0, -1, LoginMessage);
                if (LoginMessage->Command == PIPE_API_LOGIN_CMD) {
                    if (Success && (BytesRead == sizeof (PIPE_API_LOGIN_MESSAGE))) {  // receive a valid login message
                        int Pid;
                        if (CheckExternProcessComunicationProtocolVersion(LoginMessage)) {
                            if (get_pid_by_name(LoginMessage->ProcessName) > 0) {
                                ThrowError (1, "process \"%s\" already running login ignored", LoginMessage->ProcessName);
                                Pid = -1;
                                LoginMessageAck.ReturnValue = PIPE_API_LOGIN_ERROR_PROCESS_ALREADY_RUNNING;
                            } else {
                                Pid = AddExternPorcessToScheduler (LoginMessage->ProcessName, LoginMessage->ExecutableName, LoginMessage->ProcessNumber, LoginMessage->NumberOfProcesses,
                                                                   LoginMessage->ProcessId, (HANDLE)conn_desc, 1,
                                                                   LoginMessage->Priority, LoginMessage->CycleDivider, LoginMessage->Delay,
                                                                   LoginMessage->IsStartedInDebugger,
                                                                   LoginMessage->Machine,
                                                                   BuildHandleFrom32BitInt(LoginMessage->hAsyncAlivePingReqEvent),
                                                                   BuildHandleFrom32BitInt(LoginMessage->hAsyncAlivePingAckEvent),
                                                                   LoginMessage->DllName,
                                                                   &Socket_ReadFile,
                                                                   &Socket_WriteFile,
                                                                   &Socket_CloseConnectionToExternProcess,
                                                                   LoggingFile,
                                                                   LoginMessage->ExecutableBaseAddress,
                                                                   LoginMessage->DllBaseAddress,
                                                                   LoginMessage->Version);
                                if (Pid < 0) {
                                    LoginMessageAck.ReturnValue = PIPE_API_LOGIN_ERROR_NO_FREE_PID; // Error
                                } else {
                                    LoginMessageAck.ReturnValue = PIPE_API_LOGIN_SUCCESS;
                                }
                            }
                        } else {
                            Pid = -1;
                            LoginMessageAck.ReturnValue = PIPE_API_LOGIN_ERROR_WRONG_VERSION;
                        }
                        LoginMessageAck.Command = PIPE_API_LOGIN_CMD;
                        LoginMessageAck.Pid = Pid;
                        LoginMessageAck.Version = XILENV_VERSION;
                        LoginMessageAck.StructSize = sizeof (LoginMessageAck);
                        if (LoggingFlag) MessageLoggingToFile (LoggingFile,__LINE__, 1, Pid, &LoginMessageAck);
                        Success = Socket_WriteFile ((HANDLE)conn_desc,                  // pipe handle
                                                    (void*)&LoginMessageAck,            // message
                                                    sizeof (LoginMessageAck),           // message length
                                                    &BytesWritten);                     // bytes written
                        if (!Success) {
#ifdef _WIN32
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
                            ThrowError (1, "WriteFile to pipe failed to acknowledge login \"%s\"", lpMsgBuf);
                            LocalFree (lpMsgBuf);
#else
                            ThrowError (1, "Write to socket failed to acknowledge login \"%i\"", errno);
#endif
                        }
                    }
                } else if (LoginMessage->Command == PIPE_API_PING_CMD) {
                    // It is not a login it is a ping
                    PIPE_API_PING_MESSAGE_ACK PingAck;
                    PingAck.Command = PIPE_API_PING_CMD;
                    PingAck.ReturnValue = 0;
                    PingAck.Version = XILENV_VERSION;
                    PingAck.StructSize = sizeof (PingAck);
                    if (LoggingFlag) MessageLoggingToFile (LoggingFile ,__LINE__, 1, -1, &PingAck);
                    Success = Socket_WriteFile ((HANDLE)conn_desc,       // pipe handle
                                                (void*)&PingAck,         // message
                                                sizeof (PingAck),        // message length
                                                &BytesWritten);          // bytes written
#ifdef _WIN32
                    closesocket(conn_desc);
#else
                    close(conn_desc);
#endif
                }
            }
        }
    }
    my_free(LoginMessage);
    return 0;
}

static int CreateLoginThread (char *par_Prefix)
{
#ifdef _WIN32
    HANDLE hThread;
    DWORD dwThreadId = 0; 
#else
    pthread_t Thread;
    pthread_attr_t Attr;
#endif

    static char StaticInstanceName[MAX_PATH];

    strcpy (StaticInstanceName, par_Prefix);

#ifdef _WIN32
    hThread = CreateThread (
            NULL,                   // no security attribute
            0,                      // default stack size
            SocketLoginThreadProc,  // thread proc
            StaticInstanceName,     // thread parameter
            0,                      // not suspended
            &dwThreadId);           // returns thread ID

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
        ThrowError (1, "cannot create login thread (%s)\n", lpMsgBuf);
        LocalFree (lpMsgBuf);
        return -1;
    }
#else
    pthread_attr_init(&Attr);
    if (pthread_create(&Thread, &Attr, SocketLoginThreadProc, (void*)StaticInstanceName) != 0) {
        ThrowError (1, "cannot create login thread\n");
        return -1;
    }
    pthread_attr_destroy(&Attr);

#endif
    return 0;
}

int Socket_InitMessages (char *par_Prefix, int par_LogingFlag)
{
    if (par_LogingFlag) {
        LoggingFlag = 1;
#ifdef _WIN32
        LoggingFile = fopen ("c:\\temp\\socket_logging.txt", "wt");
#else
        LoggingFile = fopen ("/tmp/socket_logging.txt", "wt");
#endif
    }
    return CreateLoginThread (par_Prefix);
}


void Socket_TerminateMessages (void)
{
    int x;

    if (NewConnectionsAllowed) {
        NewConnectionsAllowed = 0;
#ifdef _WIN32
        closesocket(ToCloseSocket);
#else
        close(ToCloseSocket);
#endif
        for (x = 0; x < 100; x ++) {
            if (NewConnectionsAllowed == 2) break;
#ifdef _WIN32
            Sleep(10);
#else
            usleep(10*1000);
#endif
        }
    }
}



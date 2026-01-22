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
#define SOCKET int
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
#endif
#include <stdlib.h>
#include <stdio.h>
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "MainValues.h"
#include "RemoteMasterBlackboard.h"
#include "RemoteMasterScheduler.h"
#include "RemoteMasterOther.h"
#include "RemoteMasterCopyStartExecutable.h"
#include "GetElfSectionVersionInfos.h"

#include "ErrorDialog.h"

#include "RemoteMasterNet.h"
#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib") //Winsock Library
#else
#define SOCKET_ERROR (-1)
#endif

//#define LOG_REMOTE_MASTER_CALL     "c:\\temp\\remote_master_calls.txt"
#define COLLECT_REMOTE_MASTER_CALL_STATISTICS

#if 0
#define SERVER "192.168.0.3"  //ip address of udp server
#define BUFLEN (16*1024)  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data
#endif


static SOCKET_THREAD_ELEMENT SocketThreadConnections[MAX_SOCKET_THREAD_CONNECTIONS];
static CRITICAL_SECTION RemoteMasterCriticalSection;

#ifdef _WIN32
#define socklen_t int
#endif

void SetSocketOptions(SOCKET Socket)
{
    int iResult = 0;
    BOOL bOptVal = FALSE;
    socklen_t bOptLen = sizeof (BOOL);

    int iOptVal;
    socklen_t iOptLen;

    iOptVal = 0;
    iOptLen = sizeof(iOptVal);
    iResult = getsockopt(Socket, SOL_SOCKET, SO_KEEPALIVE, (char *) &iOptVal, &iOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("SO_KEEPALIVE Value: %i\n", iOptVal);
    }

    bOptVal = FALSE;
    bOptLen = sizeof(bOptVal);
    iResult = getsockopt(Socket, SOL_SOCKET, SO_DONTROUTE, (char *) &bOptVal, &bOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("SO_DONTROUTE Value: %i\n", bOptVal);
    }
    bOptVal = TRUE;
    setsockopt(Socket, SOL_SOCKET, SO_DONTROUTE, (const char*)&bOptVal, sizeof(bOptVal));
    if (iResult == SOCKET_ERROR) {
        printf("cannot set TCP_NODELAY\n");
    }
    bOptVal = FALSE;
    bOptLen = sizeof(bOptVal);
    iResult = getsockopt(Socket, SOL_SOCKET, SO_DONTROUTE, (char *) &bOptVal, &bOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("SO_DONTROUTE Value: %i\n", bOptVal);
    }

    // Receive Buffer
    iOptVal = 0;
    iOptLen = sizeof(iOptVal);
    iResult = getsockopt(Socket, SOL_SOCKET, SO_RCVBUF, (char *) &iOptVal, &iOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("SO_RCVBUF Value: %i\n", iOptVal);
    }
    iOptVal = 0x10000;
    setsockopt(Socket, SOL_SOCKET, SO_RCVBUF, (const char *)&iOptVal, sizeof(iOptLen));
    if (iResult == SOCKET_ERROR) {
        printf("cannot set SO_RCVBUF\n");
    }
    iOptVal = 0;
    iOptLen = sizeof(iOptVal);
    iResult = getsockopt(Socket, SOL_SOCKET, SO_RCVBUF, (char *) &iOptVal, &iOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("SO_RCVBUF Value: %i\n", iOptVal);
    }

    // transmit buffer
    iOptVal = 0;
    iOptLen = sizeof(iOptVal);
    iResult = getsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (char *) &iOptVal, &iOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("SO_SNDBUF Value: %i\n", iOptVal);
    }
    iOptVal = 0x10000;
    setsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (const char*)&iOptVal, sizeof(iOptLen));
    if (iResult == SOCKET_ERROR) {
        printf("cannot set SO_SNDBUF\n");
    }
    iOptVal = 0;
    iOptLen = sizeof(iOptVal);
    iResult = getsockopt(Socket, SOL_SOCKET, SO_SNDBUF, (char *) &iOptVal, &iOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("SO_SNDBUF Value: %i\n", iOptVal);
    }

    bOptVal = FALSE;
    bOptLen = sizeof(bOptVal);
    iResult = getsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, (char *) &bOptVal, &bOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("TCP_NODELAY Value: %i\n", bOptVal);
    }
    bOptVal = TRUE;
    setsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&bOptVal, sizeof(bOptVal));
    if (iResult == SOCKET_ERROR) {
        printf("cannot set TCP_NODELAY\n");
    }
    bOptVal = FALSE;
    bOptLen = sizeof(bOptVal);
    iResult = getsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, (char *) &bOptVal, &bOptLen);
    if (iResult != SOCKET_ERROR) {
        printf("TCP_NODELAY Value: %i\n", bOptVal);
    }
}

/*
int rm_init (uint64_t PreAllocMemorySize)
{
    RM_INIT_REQ Req;
    RM_INIT_ACK Ack;

    Req.PreAllocMemorySize = PreAllocMemorySize;
    TransactRemoteMaster (RM_INIT_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    //CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}*/

static int ReqToClientServerRunning = 0;

static int ConnectThreadToRemoteMaster (SOCKET_THREAD_ELEMENT *par_SocketThreadElement);

#define UNUSED(x) (void)(x)

#ifdef _WIN32
static DWORD WINAPI RemoteMasterServerThread (LPVOID parameter)
#else
static void *RemoteMasterServerThread (void* parameter)
#endif
{
    UNUSED(parameter);
    static char receive_buffer[16*1024];  // TODO: sollte dynamisch angepasst werden!!!
//    char transmit_buffer[16*1024];  // TODO: sollte dynamisch angepasst werden!!!
    uint32_t buffer_pos;
    uint32_t receive_message_size;
    //uint32_t transmit_message_size;
    int len;
    SOCKET_THREAD_ELEMENT SocketThreadElement;
    SOCKET conn_desc;

    // erster Socket ist zum Empfangen von Events vom Remote-Master!
    // das heisst dies muss vor allen andrenen Verbindungen zum Remote-Master
    // erstellt werden.
    //ReqToClientServerRunning = 1;  // schon mal hier setzen sonst bleibt die error-Funktion haengen
    if (ConnectThreadToRemoteMaster (&SocketThreadElement)) {
        ReqToClientServerRunning = -1;
#ifdef _WIN32
        return 1;
#else
        return NULL;
#endif
    }
    if (SocketThreadElement.CreateCounter != 0) {
        ThrowError (1, "not the first socket used for events");
    }
    conn_desc = SocketThreadElement.Socket;
    ReqToClientServerRunning = 1;

    while (1) {
        buffer_pos = 0;
        receive_message_size = 0;
        len = recv(conn_desc, receive_buffer, sizeof(uint32_t), 0);
        if (len != sizeof(uint32_t)) {
#ifdef _WIN32
            int ErrCode = WSAGetLastError();
#else
            int ErrCode = errno;
#endif
            ThrowError (1, "Failed receiving (%i)\n", ErrCode);
            break;
        } else {
            receive_message_size = *(uint32_t*)receive_buffer;
            buffer_pos = sizeof(uint32_t);
            do {
                len = recv(conn_desc, receive_buffer + buffer_pos, (int)(receive_message_size - buffer_pos), 0);
                if (len > 0) {
                    buffer_pos += (uint32_t)len;
                } else {
                    ThrowError (1, "Failed receiving\n");
                }
            } while (buffer_pos < receive_message_size);
        }
        if (receive_message_size != buffer_pos) {
            ThrowError (1, "receive_message_size != buffer_pos  (%i != %i), len =  %i",receive_message_size, buffer_pos, len);
        }

        uint32_t Command = ((RM_PACKAGE_HEADER*)receive_buffer)->Command;
        if (Command == RM_SCHEDULER_CYCLIC_EVENT_CMD) {
            rm_SchedulerCycleEvent ((RM_SCHEDULER_CYCLIC_EVENT_REQ*)receive_buffer);
        } else if (Command == RM_BLACKBOARD_WRITE_BBVARI_INFOS_CMD) {
            rm_req_write_varinfos_to_ini ((RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ*)receive_buffer);
        } else if (Command == RM_BLACKBOARD_READ_BBVARI_FROM_INI_CMD) {
            rm_req_read_varinfos_from_ini ((RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ*)receive_buffer);
        } else if (Command == RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_CMD) {
            rm_CallObserationCallbackFunction ((RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_REQ*)receive_buffer);
        } else {
            ThrowError (1, "got an unknown command %d", Command);
            continue;  // kein ACK senden
        }
/* no Ack
        buffer_pos = 0;
        transmit_message_size = receive_message_size;
        do {
            len = send(conn_desc, transmit_buffer + buffer_pos, transmit_message_size - buffer_pos, 0);
            if (len > 0) {
                buffer_pos += len;
            } else {
                ThrowError (1, "Failed transmiting\n");
            }
        } while (buffer_pos < transmit_message_size);
        */
    }
#ifdef _WIN32
    closesocket(conn_desc);
    //closesocket(sock_descriptor);
    return 0;
#else
    close(conn_desc);
    //close(sock_descriptor);
    return NULL;
#endif
}


static int StartFromRemoteMasterServer (void)
{
#ifdef _WIN32
    HANDLE hThread;
    DWORD dwThreadId;

    hThread = CreateThread (NULL,              // no security attribute
                            256*1024,                 // default stack size
                            RemoteMasterServerThread,    // thread proc
                            (void*)NULL,       // thread parameter
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
        ThrowError (1, "cannot create remote master thread (%s)\n", lpMsgBuf);
        LocalFree (lpMsgBuf);
        return -1;
    }

    while (!ReqToClientServerRunning) {
        Sleep (10);
        CheckErrorMessagesBeforeMainWindowIsOpen();
    }
    //Sleep(1000);
#else
    pthread_t Thread;
    pthread_attr_t Attr;
    int Ret;

    pthread_attr_init(&Attr);

    /* Set a specific stack size  */
    Ret = pthread_attr_setstacksize(&Attr, 256*1024);
    if (Ret) {
        printf("pthread setstacksize failed\n");
        return -1;
    }
    if (pthread_create(&Thread, &Attr, RemoteMasterServerThread, (void*)NULL) != 0) {
        ThrowError (1, "cannot create remote master thread\n");
        return -1;
    }
    pthread_attr_destroy(&Attr);
#endif
    return 0;
}


#ifdef COLLECT_REMOTE_MASTER_CALL_STATISTICS
#define COLLECT_REMOTE_MASTER_CALL_STATISTICS_SIZE  500
typedef struct {
    uint64_t CallCounter;
    uint64_t LastTime;
    uint64_t AccumulatedTime;
    uint64_t MaxTime;
    uint64_t MinTime;
    uint64_t Fill1;
    uint64_t Fill2;
    uint64_t Fill3;
} REMOTE_MASTER_CALL_INFO;
static REMOTE_MASTER_CALL_INFO *RemoteMasterCallStatistics;


void ResetRemoteMasterCallStatistics(void)
{
    int x;
    for (x = 0; x < COLLECT_REMOTE_MASTER_CALL_STATISTICS_SIZE; x++) {
        RemoteMasterCallStatistics[x].CallCounter = 0;
        RemoteMasterCallStatistics[x].LastTime = 0;
        RemoteMasterCallStatistics[x].MaxTime = 0;
        RemoteMasterCallStatistics[x].MinTime = 0xFFFFFFFFFFFFFFFFULL;
        RemoteMasterCallStatistics[x].AccumulatedTime = 0;
    }
}

static void RemoteMasterAddCallToStatistic(int par_Command, uint64_t par_Time)
{
    if ((par_Command >= COLLECT_REMOTE_MASTER_CALL_STATISTICS_SIZE) || (par_Command < 0)) return;
    EnterCriticalSection (&RemoteMasterCriticalSection);
    RemoteMasterCallStatistics[par_Command].CallCounter++;
    RemoteMasterCallStatistics[par_Command].LastTime = par_Time;
    RemoteMasterCallStatistics[par_Command].AccumulatedTime += par_Time;
    if (par_Time > RemoteMasterCallStatistics[par_Command].MaxTime) RemoteMasterCallStatistics[par_Command].MaxTime = par_Time;
    if (par_Time < RemoteMasterCallStatistics[par_Command].MinTime) RemoteMasterCallStatistics[par_Command].MinTime = par_Time;
    LeaveCriticalSection (&RemoteMasterCriticalSection);
}

static uint64_t GetPerformanceCounter(void)
{
#ifdef _WIN32
    union {
        uint64_t ui64;
        LARGE_INTEGER li;
    } Ret;
    QueryPerformanceCounter(&(Ret.li));
    return Ret.ui64;
#else
    uint64_t ret;
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    ret = time.tv_sec * 1000000000ULL + time.tv_nsec;
    return ret;
#endif
}

//#define START_TIME_MEASUREMENT    uint64_t StartTime = __rdtsc();
//#define END_TIME_MEASUREMENT()    (__rdtsc() - StartTime)

#define START_TIME_MEASUREMENT    uint64_t StartTime = GetPerformanceCounter();
#define END_TIME_MEASUREMENT()    (GetPerformanceCounter() - StartTime)


#else
#define RemoteMasterAddCallToStatistic(par_Command, par_Time)
#define START_TIME_MEASUREMENT
#define END_TIME_MEASUREMENT()
#endif

int RemoteMasterGetNextCallToStatistic(int par_Index, uint64_t *ret_CallCounter,
                                       uint64_t *ret_LastTime,
                                       uint64_t *ret_AccumulatedTime,
                                       uint64_t *ret_MaxTime,
                                       uint64_t *ret_MinTime)
{
#ifdef COLLECT_REMOTE_MASTER_CALL_STATISTICS
    if (!s_main_ini_val.ConnectToRemoteMaster) return -1;
    if ((par_Index >= COLLECT_REMOTE_MASTER_CALL_STATISTICS_SIZE) || (par_Index < 0)) return -1;
    EnterCriticalSection (&RemoteMasterCriticalSection);
    if (par_Index == 0) {
        int x;
        *ret_CallCounter = 0;
        *ret_LastTime = 0;
        *ret_AccumulatedTime = 0;
        *ret_MaxTime = 0;
        *ret_MinTime = 0;
        for (x = 0; x < COLLECT_REMOTE_MASTER_CALL_STATISTICS_SIZE; x++) {
            *ret_CallCounter += RemoteMasterCallStatistics[x].CallCounter;
            *ret_AccumulatedTime += RemoteMasterCallStatistics[x].AccumulatedTime;
        }
    } else {
        *ret_CallCounter = RemoteMasterCallStatistics[par_Index].CallCounter;
        *ret_LastTime = RemoteMasterCallStatistics[par_Index].LastTime;
        *ret_AccumulatedTime = RemoteMasterCallStatistics[par_Index].AccumulatedTime;
        *ret_MaxTime = RemoteMasterCallStatistics[par_Index].MaxTime;
        *ret_MinTime = RemoteMasterCallStatistics[par_Index].MinTime;
    }
    LeaveCriticalSection (&RemoteMasterCriticalSection);
    return par_Index + 1;
#else
    return -1;
#endif
}

int RemoteMasterGetRowCount(void)
{
#ifdef COLLECT_REMOTE_MASTER_CALL_STATISTICS
    return COLLECT_REMOTE_MASTER_CALL_STATISTICS_SIZE;
#else
    return 0;
#endif
}

#ifdef LOG_REMOTE_MASTER_CALL
static FILE *LogRemoteMasterCall;
#endif

int InitRemoteMaster(char *par_RemoteMasterAddr, int par_RemoteMasterPort)
{
     UNUSED(par_RemoteMasterAddr);
     UNUSED(par_RemoteMasterPort);

#ifdef _WIN32
    int iResult;
    WSADATA wsaData;

    InitializeCriticalSection (&RemoteMasterCriticalSection);
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        ThrowError (1, "WSAStartup failed with error: %d\n", iResult);
        return -1;
    }
#else
#endif
#ifdef LOG_REMOTE_MASTER_CALL
    LogRemoteMasterCall = fopen(LOG_REMOTE_MASTER_CALL, "wt");
#endif
#ifdef COLLECT_REMOTE_MASTER_CALL_STATISTICS
    RemoteMasterCallStatistics = (REMOTE_MASTER_CALL_INFO*)my_malloc(COLLECT_REMOTE_MASTER_CALL_STATISTICS_SIZE * sizeof(REMOTE_MASTER_CALL_INFO));
    if (RemoteMasterCallStatistics == NULL) {
        ThrowError (1, "cannot allocate memory for remote call statistics");
    } else {
        ResetRemoteMasterCallStatistics();
    }
#endif

    //RemoteMasterWakeOnLan(Mac);
    //RemoteMasterGetMacAddress(s_main_ini_val.RemoteMasterName, Mac);

    if (s_main_ini_val.CopyAndStartRemoteMaster) {
        char SharedLibrarySrcPath[MAX_PATH];
        char SharedLibraryDstDir[MAX_PATH];
        char SharedLibraryDstPath[MAX_PATH];
        char *p;

        if (RemoteMasterPing(s_main_ini_val.RemoteMasterName,
                             s_main_ini_val.RemoteMasterPort + 2) != 0) {
            ThrowError (1, "cannot connect to remote master box (address = \"%s\", port = %i)",
                   s_main_ini_val.RemoteMasterName,
                   s_main_ini_val.RemoteMasterPort);
            return -1;
        }

        RemoteMasterSyncDateTime(s_main_ini_val.RemoteMasterName,
                                 s_main_ini_val.RemoteMasterPort + 2);

        RemoteMasterKillAll(s_main_ini_val.RemoteMasterName,
                            s_main_ini_val.RemoteMasterPort + 2,
                            s_main_ini_val.RemoteMasterCopyTo);

        RemoteMasterRemoveVarLogMessages(s_main_ini_val.RemoteMasterName,
                                         s_main_ini_val.RemoteMasterPort + 2);

        STRING_COPY_TO_ARRAY (SharedLibraryDstDir, s_main_ini_val.RemoteMasterCopyTo);
        p = strrchr(SharedLibraryDstDir, '/');
        if (p != NULL) {
            *p = 0;
        }

        STRING_COPY_TO_ARRAY (SharedLibraryDstPath, s_main_ini_val.RemoteMasterCopyTo);
        p = strrchr(SharedLibraryDstPath, '/');
        if (p == NULL) {
            STRING_COPY_TO_ARRAY(SharedLibraryDstPath, "libLinuxRemoteMasterCore.so");
        } else {
            StringCopyMaxCharTruncate(p, "/libLinuxRemoteMasterCore.so", sizeof(SharedLibraryDstPath) - (p - SharedLibraryDstPath));
        }

#ifdef _WIN32
        GetModuleFileName (GetModuleHandle(NULL), SharedLibrarySrcPath, sizeof (SharedLibrarySrcPath));
        p = strrchr (SharedLibrarySrcPath, '\\');
        if (p == NULL) p = SharedLibrarySrcPath + strlen(SharedLibrarySrcPath);
        StringCopyMaxCharTruncate(p, "\\rt_linux\\libLinuxRemoteMasterCore.so", sizeof(SharedLibrarySrcPath) - (p - SharedLibrarySrcPath));
#else
        readlink("/proc/self/exe", SharedLibrarySrcPath, sizeof (SharedLibrarySrcPath));
        p = strrchr (SharedLibrarySrcPath, '/');
        if (p == NULL) p = SharedLibrarySrcPath + strlen(SharedLibrarySrcPath);
        StringCopyMaxCharTruncate(p, "/rt_linux/libLinuxRemoteMasterCore.so", sizeof(SharedLibrarySrcPath) - (p - SharedLibrarySrcPath));
#endif
        if (access(SharedLibrarySrcPath, 0)) {
            ThrowError(MESSAGE_STOP, "File %s dosen't exist", SharedLibrarySrcPath);
            return -1;
        }
        if (access(GetRemoteMasterExecutable(), 0)) {
            ThrowError(MESSAGE_STOP, "File %s dosen't exist", GetRemoteMasterExecutable());
            return -1;
        }
        CheckIfExecutableAndXilEnvAndScharedLibraryMatches(GetRemoteMasterExecutable(),
                                                            SharedLibrarySrcPath);

        RemoteMasterCopyAndStartExecutable(SharedLibrarySrcPath,
                                           SharedLibraryDstPath,
                                           SharedLibraryDstDir,
                                           s_main_ini_val.RemoteMasterName,
                                           s_main_ini_val.RemoteMasterPort + 2,
                                           0); //   nicht starten

        RemoteMasterCopyAndStartExecutable(GetRemoteMasterExecutable(),
                                           s_main_ini_val.RemoteMasterCopyTo,
                                           SharedLibraryDstDir,
                                           s_main_ini_val.RemoteMasterName,
                                           s_main_ini_val.RemoteMasterPort + 2,
                                           1); //   gleich starten
    }
    return StartFromRemoteMasterServer();
}

static int ConnectThreadToRemoteMaster (SOCKET_THREAD_ELEMENT *par_SocketThreadElement)
{
    static uint32_t CreateCounter;

    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
#ifdef _WIN32
    int SocketErrCode = 0;
#endif
    char PortString[32];
    int Ret = -1;

    MEMSET(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    PrintFormatToString (PortString, sizeof(PortString), "%i", s_main_ini_val.RemoteMasterPort); //RemoteMasterPort);
    // Resolve the server address and port
    iResult = getaddrinfo(s_main_ini_val.RemoteMasterName, PortString, &hints, &result);
    if (iResult != 0) {
        ThrowError (1, "getaddrinfo failed with error: %d (%s)\n", iResult, gai_strerror(iResult));
        return -1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        par_SocketThreadElement->Socket = socket(ptr->ai_family, ptr->ai_socktype,  ptr->ai_protocol);
#ifdef _WIN32
        if (par_SocketThreadElement->Socket == INVALID_SOCKET) {
            ThrowError (1, "socket failed with error: %ld\n", WSAGetLastError());
            return -1;
        }
#else
        if (par_SocketThreadElement->Socket < 0) {
            ThrowError (1, "socket failed with error: %ld\n", errno);
            return -1;
        }
#endif

        SetSocketOptions(par_SocketThreadElement->Socket);

        // Connect to server.
        iResult = connect (par_SocketThreadElement->Socket, ptr->ai_addr, (int)ptr->ai_addrlen);

#ifdef _WIN32
        if (iResult == SOCKET_ERROR) {
            SocketErrCode = WSAGetLastError();
            closesocket(par_SocketThreadElement->Socket);
            par_SocketThreadElement->Socket = INVALID_SOCKET;
            continue;
        } else Ret = 0;
#else
        if (iResult != 0) {
            close(par_SocketThreadElement->Socket);
            continue;
        } else Ret = 0;
#endif
        break;
    }

    freeaddrinfo(result);

#ifdef _WIN32
    if (par_SocketThreadElement->Socket == INVALID_SOCKET) {
#else
    if (par_SocketThreadElement->Socket < 0) {
#endif
        if (s_main_ini_val.CopyAndStartRemoteMaster) {
            ThrowError (1, "Unable to connect to the remote master on %s (port=%s)",
                   s_main_ini_val.RemoteMasterName, PortString);
        } else {
            ThrowError (1, "Unable to connect to the remote master\n"
                      "on %s (port=%s)\n"
                      "the option\n"
                      "\"Copy and start remote executable\"\n"
                      "inside the basic settings is not set",
                   s_main_ini_val.RemoteMasterName, PortString);
        }
        Ret = -1;
    }

    par_SocketThreadElement->CreateCounter = CreateCounter;
    CreateCounter++;

    return Ret;
}

static inline SOCKET_THREAD_ELEMENT* GetSocketForThread(void)
{
    int x;
    SOCKET_THREAD_ELEMENT* Ret = NULL;
    DWORD ThreadId = GetCurrentThreadId();
    for (x = 0; x < MAX_SOCKET_THREAD_CONNECTIONS; x++) {
        if (SocketThreadConnections[x].ThreadId == ThreadId) {
            return &SocketThreadConnections[x];
        }
    }
    // there is no socket assigned for this thread
    EnterCriticalSection (&RemoteMasterCriticalSection);
    // search if one can be reuse
    for (x = 0; x < MAX_SOCKET_THREAD_CONNECTIONS; x++) {
        if ((SocketThreadConnections[x].ThreadId == 0) &&
            (SocketThreadConnections[x].Socket != 0)) {
            SocketThreadConnections[x].ThreadId = ThreadId;
            LeaveCriticalSection (&RemoteMasterCriticalSection);
            return &SocketThreadConnections[x];
        }
    }
    // create a new connection to the remote master
    for (x = 0; x < MAX_SOCKET_THREAD_CONNECTIONS; x++) {
        if (SocketThreadConnections[x].ThreadId == 0) {
            SocketThreadConnections[x].ThreadId = ThreadId;
            SocketThreadConnections[x].Number = (uint16_t)x;
            SocketThreadConnections[x].PackageCounter = 0;
            if (ConnectThreadToRemoteMaster (&SocketThreadConnections[x])) {
                SocketThreadConnections[x].ThreadId = 0;
                Ret = NULL;
            } else {
                Ret = &SocketThreadConnections[x];
            }
            break;
        }
    }
    LeaveCriticalSection (&RemoteMasterCriticalSection);
    if (Ret == NULL) {
        ThrowError (1, "cannot get socket handelfor thread");
    }
    return Ret;
}

void CloseSocketToRemoteMasterForThisThead(void)
{
    if (s_main_ini_val.ConnectToRemoteMaster) {
        int x;
        DWORD ThreadId = GetCurrentThreadId();
        EnterCriticalSection (&RemoteMasterCriticalSection);
        for (x = 0; x < MAX_SOCKET_THREAD_CONNECTIONS; x++) {
            if (SocketThreadConnections[x].ThreadId == ThreadId) {
                // do not kill the connection keep it for later usage
                /*LeaveCriticalSection (&RemoteMasterCriticalSection);
                rm_KillThread ();
                EnterCriticalSection (&RemoteMasterCriticalSection);*/
                SocketThreadConnections[x].ThreadId = 0;
                break;
            }
        }
        LeaveCriticalSection (&RemoteMasterCriticalSection);
    }
}


void CloseAllOtherSocketsToRemoteMaster(void)
{
    int x;
    DWORD ThreadId = GetCurrentThreadId();
    for (x = 0; x < MAX_SOCKET_THREAD_CONNECTIONS; x++) {
        if (SocketThreadConnections[x].ThreadId != 0) {
            if (SocketThreadConnections[x].ThreadId != ThreadId) {
#ifdef _WIN32
                closesocket(SocketThreadConnections[x].Socket);
#else
                close(SocketThreadConnections[x].Socket);
#endif
                SocketThreadConnections[x].ThreadId = 0;
            }
        }
    }
}

static int SentToRemoteMasterLocal (SOCKET_THREAD_ELEMENT *par_SocketForThread, void *par_Data, int par_Command, int par_len)
{
    int buffer_pos = 0;
    ((RM_PACKAGE_HEADER*)par_Data)->SizeOf = (uint32_t)par_len;
    ((RM_PACKAGE_HEADER*)par_Data)->Command = (uint32_t)par_Command;
    ((RM_PACKAGE_HEADER*)par_Data)->ChannelNumber = par_SocketForThread->Number;
    ((RM_PACKAGE_HEADER*)par_Data)->PackageCounter = par_SocketForThread->PackageCounter++;
    ((RM_PACKAGE_HEADER*)par_Data)->ThreadId = GetCurrentThreadId();

#ifdef LOG_REMOTE_MASTER_CALL
    if (LogRemoteMasterCall != NULL) {
        fprintf (LogRemoteMasterCall, "SendToRemoteMaster(%i,%i,%i)\n", par_SocketForThread->Number, par_Command, par_len);
        fflush(LogRemoteMasterCall);
    }
#endif

    do {
        int SendBytes = send (par_SocketForThread->Socket, (char*)par_Data + buffer_pos, par_len - buffer_pos, 0);
#ifdef _WIN32
        if (SendBytes == SOCKET_ERROR) {
            char *lpMsgBuf;
            int ErrorCode = WSAGetLastError();
            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           ErrorCode,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR) &lpMsgBuf,
                           0, NULL );
            ThrowError (1, "send data to remote master failed with error code %i (%s)\n", ErrorCode, lpMsgBuf);
            LocalFree (lpMsgBuf);
            return -1;
        }
#else
        if (SendBytes < 0) {
            ThrowError (1, "send failed with error: \"%s\"\n", strerror(errno));
            return -1;
        }
#endif
        buffer_pos += SendBytes;
    } while (buffer_pos < par_len);

    return 0;
}

int SentToRemoteMaster (void *par_Data, int par_Command, int par_len)
{
    SOCKET_THREAD_ELEMENT *SocketForThread = GetSocketForThread();
    if (SocketForThread != NULL) {
        return SentToRemoteMasterLocal(SocketForThread, par_Data, par_Command, par_len);
    }
    return -1;
}

MY_SOCKET RemoteMasterGetSocket(void)
{
    SOCKET_THREAD_ELEMENT *SocketForThread = GetSocketForThread();
    if (SocketForThread != NULL) {
        return SocketForThread->Socket;
    } else {
        return (SOCKET)-1;
    }
}

int TransmitToRemoteMaster (int par_Command, void *par_Data, int par_len)
{
    int Ret;
    START_TIME_MEASUREMENT
    SOCKET_THREAD_ELEMENT *SocketForThread = GetSocketForThread();
    Ret = SentToRemoteMasterLocal (SocketForThread, par_Data, par_Command, par_len);
    RemoteMasterAddCallToStatistic(par_Command, END_TIME_MEASUREMENT());
    return Ret;
}

static int ReceiveFromeRemoteMasterLocal (SOCKET_THREAD_ELEMENT *par_SocketForThread, void *ret_Data, int par_len)
{  
    int buffer_pos = 0;
    uint32_t receive_message_size = 0;
    do {
        int ReadBytes = recv (par_SocketForThread->Socket, (char*)ret_Data + buffer_pos, par_len - buffer_pos, 0);
        if (ReadBytes > 0) {
            buffer_pos += ReadBytes;
            if (receive_message_size == 0) {
                receive_message_size = *(uint32_t*)ret_Data;
            }
        } else if (ReadBytes == 0) {
            printf ("0Bytes\n");
            //ThrowError (1, "Connection closed\n");
            /*char *ErrString = GetLastSysErrorString ();
            ThrowError (1, "recv() error code: %d \"%s\"", WSAGetLastError(), ErrString);
            FreeLastSysErrorString (ErrString);
            return -1;*/
        } else {
#ifdef _WIN32
            ThrowError (1, "recv failed with error: %d\n", WSAGetLastError());
#else
            ThrowError (1, "recv failed with error: %d\n", errno);
#endif
            return -1;
        }
    } while ((uint32_t)buffer_pos < receive_message_size);
#ifdef LOG_REMOTE_MASTER_CALL
    if (LogRemoteMasterCall != NULL) {
        fprintf (LogRemoteMasterCall, "ReceiveFromeRemoteMaster(%i,%i,%i,%i)\n",
                 par_SocketForThread->Number,
                 ((RM_PACKAGE_HEADER*)ret_Data)->Command,
                 ((RM_PACKAGE_HEADER*)ret_Data)->SizeOf,
                 receive_message_size);
        fflush(LogRemoteMasterCall);
    }
#endif
    return (int)receive_message_size;
}

int TransactRemoteMaster (int par_Command, void *par_DataToRM, int par_LenDataToRM, void *ret_DataFromRM, int par_LenDataFromRM)
{
    int Ret = -1;

    START_TIME_MEASUREMENT
    SOCKET_THREAD_ELEMENT *SocketForThread = GetSocketForThread();
    if (SocketForThread != NULL) {
        if (SentToRemoteMasterLocal (SocketForThread, par_DataToRM, par_Command, par_LenDataToRM) == 0) {
            if ((Ret = ReceiveFromeRemoteMasterLocal (SocketForThread, ret_DataFromRM, par_LenDataFromRM)) <= 0) {
                Ret = -1;
            }
        }
    }
    RemoteMasterAddCallToStatistic(par_Command, END_TIME_MEASUREMENT());
    return Ret;
}


static int ReceiveFromeRemoteMasterDynBufLocal (SOCKET_THREAD_ELEMENT *par_SocketForThread, void *ret_Data, int par_len, void **ret_ptrToData, int *ret_len)
{
    int buffer_size;
    char *dyn_buffer;
    //RM_PACKAGE_HEADER *for_debug = (RM_PACKAGE_HEADER*)ret_Data;

    int buffer_pos = 0;
    uint32_t receive_message_size = 0;
    do {
        int ReadBytes = recv (par_SocketForThread->Socket, (char*)ret_Data + buffer_pos, par_len - buffer_pos, 0);
        if (ReadBytes > 0) {
            buffer_pos += ReadBytes;
            if (receive_message_size == 0) {
                receive_message_size = *(uint32_t*)ret_Data;
            }
        } else if (ReadBytes == 0) {
            ThrowError (1, "Connection closed\n");
            return -1;
        } else {
#ifdef _WIN32
            ThrowError (1, "recv failed with error: %d\n", WSAGetLastError());
#else
            ThrowError (1, "recv failed with error: %d\n", errno);
#endif
            return -1;
        }
    } while (buffer_pos < par_len);

    buffer_size = (int)receive_message_size - par_len;  // Rest of the message inside the dynamic part

    dyn_buffer = my_malloc (buffer_size);
    if (dyn_buffer == NULL) return -1;
    buffer_pos = 0;
    do {
        int ReadBytes = recv (par_SocketForThread->Socket, (char*)dyn_buffer + buffer_pos, buffer_size - buffer_pos, 0);
        if (ReadBytes > 0) {
            buffer_pos += ReadBytes;
        } else if (ReadBytes == 0) {
            ThrowError (1, "Connection closed\n");
            return -1;
        } else {
#ifdef _WIN32
            ThrowError (1, "recv failed with error: %d\n", WSAGetLastError());
#else
            ThrowError (1, "recv failed with error: %d\n", errno);
#endif
            return -1;
        }
    } while (buffer_pos < buffer_size);
    *ret_len = buffer_size;
    *ret_ptrToData = dyn_buffer;
#ifdef LOG_REMOTE_MASTER_CALL
    if (LogRemoteMasterCall != NULL) {
        fprintf (LogRemoteMasterCall, "ReceiveFromeRemoteMasterDynBuf(%i,%i,%i,%i)\n",
                 par_SocketForThread->Number,
                 ((RM_PACKAGE_HEADER*)ret_Data)->Command,
                 ((RM_PACKAGE_HEADER*)ret_Data)->SizeOf,
                 receive_message_size);
        fflush(LogRemoteMasterCall);
    }
#endif
    return (int)receive_message_size;
}

int TransactRemoteMasterDynBuf (int par_Command, void *par_DataToRM, int par_LenDataToRM, void *ret_DataFromRM, int par_LenDataFromRM, void **ret_ptrDataFromRM, int *ret_LenDataFromRM)
{
    int Ret = -1;
    SOCKET_THREAD_ELEMENT *SocketForThread = GetSocketForThread();
    START_TIME_MEASUREMENT
    if (SocketForThread != NULL) {
        int Ret;
        if (SentToRemoteMasterLocal (SocketForThread, par_DataToRM, par_Command, par_LenDataToRM) == 0) {
            if ((Ret = ReceiveFromeRemoteMasterDynBufLocal (SocketForThread, ret_DataFromRM, par_LenDataFromRM, ret_ptrDataFromRM, ret_LenDataFromRM)) <= 0) {
                Ret = -1;
            }
        }
    }
    RemoteMasterAddCallToStatistic(par_Command, END_TIME_MEASUREMENT());
    return Ret;
}

void RemoteMasterFreeDynBuf (void *par_buffer)
{
    my_free (par_buffer);
}



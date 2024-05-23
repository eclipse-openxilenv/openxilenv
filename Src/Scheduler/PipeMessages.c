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


#ifdef _WIN32
#include <Windows.h>
#include <malloc.h>
#include <stdio.h>
#include "config.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "IniDataBase.h"
#include "tcb.h"
#include "Scheduler.h"
#include "ScBbCopyLists.h"
#include "BaseMessages.h"
#include "PipeMessages.h"


// #define USE_DEBUG_API

#define TERMINATE_EXTERN_PROCESS_IMMEDIATELY  -12345678

static int LoggingFlag;
static FILE *LoggingFile;


static HANDLE CreateLoginPipe (char *par_Prefix)
{
    HANDLE hPipe;
    char Pipename[MAX_PATH];

    sprintf (Pipename, PIPE_NAME "_%s", par_Prefix);

    hPipe = CreateNamedPipe (Pipename, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, PIPE_MESSAGE_BUFSIZE, PIPE_MESSAGE_BUFSIZE, 0, NULL);
 
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

static void Pipe_CloseConnectionToExternProcess (HANDLE Pipe)
{
    FlushFileBuffers(Pipe);
    DisconnectNamedPipe (Pipe);
    CloseHandle(Pipe);
}

DWORD Pipe_ReadFile (HANDLE Pipe, void *Buffer, DWORD BufferSize, LPDWORD BytesRead)
{
    return (DWORD)ReadFile (Pipe, Buffer, (DWORD)BufferSize, BytesRead, NULL);
}

DWORD Pipe_WriteFile (HANDLE Pipe, void *Buffer, int nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten)
{
    return (DWORD)WriteFile(Pipe, Buffer, (DWORD)nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL);
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

static int LoopFlag;
static char *PipeNamePostfix;

static DWORD WINAPI LoginThreadProc (LPVOID lpParam)
{
    HANDLE hPipe;
    BOOL Success;
    DWORD BytesRead, BytesWritten, Connected;
    PIPE_API_LOGIN_MESSAGE *LoginMessage;
    PIPE_API_LOGIN_MESSAGE_ACK LoginMessageAck;

    // remember the pipe name for termination
    PipeNamePostfix = my_malloc(strlen((char*)lpParam) + 1);
    strcpy(PipeNamePostfix, (char*)lpParam);
    LoginMessage = (PIPE_API_LOGIN_MESSAGE*)my_malloc (PIPE_MESSAGE_BUFSIZE);
    while (LoopFlag) {  // endless loop
        if ((hPipe = CreateLoginPipe ((char *)lpParam)) != INVALID_HANDLE_VALUE) {
            Connected = ConnectNamedPipe (hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
            if (Connected) {
                Success = ReadFile (hPipe, (void*)LoginMessage, PIPE_MESSAGE_BUFSIZE, &BytesRead, NULL);
                if (LoggingFlag) MessageLoggingToFile (LoggingFile,__LINE__, 0, -1, LoginMessage);
                if (LoginMessage->Command == PIPE_API_LOGIN_CMD) {
                    int SizeOf = sizeof (PIPE_API_LOGIN_MESSAGE);
                    if (Success && ((BytesRead == SizeOf) ||
                                    (BytesRead == 884))) {  // receive a valid login message
                        int Pid;
                        if (CheckExternProcessComunicationProtocolVersion(LoginMessage)) {
                            if (get_pid_by_name(LoginMessage->ProcessName) > 0) {
                                ThrowError (1, "process \"%s\" already running login ignored", LoginMessage->ProcessName);
                                Pid = -1;
                                LoginMessageAck.ReturnValue = PIPE_API_LOGIN_ERROR_PROCESS_ALREADY_RUNNING;
                            } else {
                                Pid = AddExternPorcessToScheduler (LoginMessage->ProcessName, LoginMessage->ExecutableName, LoginMessage->ProcessNumber, LoginMessage->NumberOfProcesses,
                                                                   LoginMessage->ProcessId, hPipe, 0,
                                                                   LoginMessage->Priority, LoginMessage->CycleDivider, LoginMessage->Delay,
                                                                   LoginMessage->IsStartedInDebugger,
                                                                   LoginMessage->Machine,
                                                                   BuildHandleFrom32BitInt(LoginMessage->hAsyncAlivePingReqEvent),
                                                                   BuildHandleFrom32BitInt(LoginMessage->hAsyncAlivePingAckEvent),
                                                                   LoginMessage->DllName,
                                                                   &Pipe_ReadFile,
                                                                   &Pipe_WriteFile,
                                                                   &Pipe_CloseConnectionToExternProcess,
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
                        Success = WriteFile (hPipe,                     // pipe handle
                                             (void*)&LoginMessageAck,   // message
                                             sizeof (LoginMessageAck),  // message length
                                             &BytesWritten,             // bytes written
                                             NULL);                     // not overlapped
                        if (!Success) {
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
                        }
                    }
                } else if (LoginMessage->Command == PIPE_API_PING_CMD) {
                    // It is not a login, it is a ping
                    PIPE_API_PING_MESSAGE_ACK PingAck;
                    PingAck.Command = PIPE_API_PING_CMD;
                    PingAck.ReturnValue = 0;
                    PingAck.Version = XILENV_VERSION;
                    PingAck.StructSize = sizeof (PingAck);
                    if (LoggingFlag) MessageLoggingToFile (LoggingFile,__LINE__, 1, -1, &PingAck);
                    Success = WriteFile (hPipe,                  // pipe handle
                                         (void*)&PingAck,        // message
                                         sizeof (PingAck),       // message length
                                         &BytesWritten,          // bytes written
                                         NULL);                  // not overlapped
                    CloseHandle (hPipe);
                }
            }
        }
    }
    my_free(LoginMessage);
    LoopFlag = 2;  // Loop is terminated
    return 0;
}

static HANDLE hThread;

static int CreateLoginThread (char *par_Prefix)
{
    DWORD dwThreadId = 0; 
    static char StaticInstanceName[MAX_PATH];

    strcpy (StaticInstanceName, par_Prefix);
    LoopFlag = 1;
    hThread = CreateThread ( 
            NULL,                // no security attribute
            0,                   // default stack size
            LoginThreadProc,     // thread proc
            StaticInstanceName,  // thread parameter
            0,                   // not suspended
            &dwThreadId);        // returns thread ID

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
    return 0;
}

int InitPipeMessages (char *par_Prefix, int par_LogingFlag)
{
    if (par_LogingFlag) {
        LoggingFlag = 1;
        LoggingFile = fopen ("pipe_logging.txt", "wt");
    }
    return CreateLoginThread (par_Prefix);
}

int TerminatePipeMessages (void)
{
    int x;
    HANDLE hPipe;
    char Pipename[MAX_PATH];
    int LoopCounter = 0;

    if ((strlen(PIPE_NAME) + strlen(PipeNamePostfix) + 1) > sizeof(Pipename)) {
        return -1;
    }
    sprintf (Pipename, PIPE_NAME "_%s", PipeNamePostfix);
    my_free(PipeNamePostfix);

    LoopFlag = 0;

    while (1) {
        hPipe = CreateFile (Pipename,       // pipe name
                            GENERIC_READ |  // read and write access
                            GENERIC_WRITE,
                            0,              // no sharing
                            NULL,           // default security attributes
                            OPEN_EXISTING,  // opens existing pipe
                            0,              // default attributes
                            NULL);          // no template file

        // Break if the pipe handle is valid.
        if (hPipe != INVALID_HANDLE_VALUE) {
            break;  // while(1)
        }
        // Exit if an error other than ERROR_PIPE_BUSY occurs.
        switch (GetLastError()) {
        case ERROR_PIPE_BUSY:
            if (LoopCounter > 10) {
                return -1;
            }
            Sleep (100);
            LoopCounter++;
            break; // switch(GetLastError())
        default:  // all other
            return -1;
        }
    }
    CloseHandle(hPipe);
    for (x = 0; (x < 10) && (LoopFlag != 2); x++) {
        Sleep(10);
    }
    return (x < 10) ? 0 : -1;
}

#endif

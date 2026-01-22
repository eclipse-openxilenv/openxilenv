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


#include <Windows.h>
#include <stdio.h>

#include "PrintFormatToString.h"
#include "XilEnvExtProc.h"
#include "ExtpProcessAndTaskInfos.h"

#include "ExtpPipeMessages.h"

static HANDLE XilEnvInternal_ConnectToPipe (char *par_InstanceName, char *par_ServerName, int par_Timout_ms, int par_ErrMessageFlag)
{
    int LoopCounter = 0;
    HANDLE hPipe;
    DWORD Status;
    DWORD Mode;
    char Pipename[MAX_PATH];

    PrintFormatToString (Pipename, sizeof(Pipename), PIPE_NAME "_%s", par_InstanceName);

    while (1) {
        hPipe = CreateFile (Pipename,   // pipe name
                            GENERIC_READ |  // read and write access
                            GENERIC_WRITE,
                            0,              // no sharing
                            NULL,           // default security attributes
                            OPEN_EXISTING,  // opens existing pipe
                            0,              // default attributes
                            NULL);          // no template file

        // Break if the pipe handle is valid.
        if (hPipe != INVALID_HANDLE_VALUE) {
            break;
        }
        // Exit if an error other than ERROR_PIPE_BUSY occurs.
        switch (GetLastError()) {
        case ERROR_PIPE_BUSY:
        case ERROR_FILE_NOT_FOUND:
            if (LoopCounter > (par_Timout_ms / 100)) {
                if (par_ErrMessageFlag) {
                    ThrowError (1, "cannot open named pipe \"%s\" for communication with XilEnv instance \"%s\"", Pipename, par_InstanceName);
                }
                return INVALID_HANDLE_VALUE;
            }
            Sleep (100);
            LoopCounter++;
            break;
        default:
            {
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
                ThrowError (1, "cannot open named pipe \"%s\" for communication with XilEnv instance \"%s\": %s", Pipename, par_InstanceName, lpMsgBuf);
                LocalFree (lpMsgBuf);
            }
            return INVALID_HANDLE_VALUE;
        }
    }

    Mode = PIPE_READMODE_MESSAGE;
    Status = SetNamedPipeHandleState (hPipe,    // pipe handle
                                      &Mode,  // new pipe mode
                                      NULL,     // don't set maximum bytes
                                      NULL);    // don't set maximum time
    if (!Status) {
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
        ThrowError (1, "SetNamedPipeHandleState failed: %s", lpMsgBuf);
        LocalFree (lpMsgBuf);
        return INVALID_HANDLE_VALUE;
    }

    return hPipe;
}


static void XilEnvInternal_DisConnectFromrPipe (HANDLE hPipe)
{
    CloseHandle (hPipe);
}


static BOOL XilEnvInternal_TransactMessagePipe (HANDLE hNamedPipe,
                                                 LPVOID lpInBuffer,
                                                 DWORD nInBufferSize,
                                                 LPVOID lpOutBuffer,
                                                 DWORD nOutBufferSize,
                                                LPDWORD lpBytesRead)
{
	BOOL Ret = TransactNamedPipe (hNamedPipe,
                                  lpInBuffer,
                                  nInBufferSize,
                                  lpOutBuffer,
                                  nOutBufferSize,
                                  lpBytesRead,
                                  NULL);
    return (Ret == TRUE) || (GetLastError() == ERROR_MORE_DATA);
}


BOOL XilEnvInternal_ReadMessagePipe (HANDLE hFile,
							          PVOID lpBuffer,
							          DWORD nNumberOfBytesToRead,
							          LPDWORD lpNumberOfBytesRead)
{
	return ReadFile (hFile,
			         lpBuffer,
                     nNumberOfBytesToRead,
                     lpNumberOfBytesRead,
                     NULL);
}

BOOL XilEnvInternal_WriteMessagePipe (HANDLE hFile,
                                       LPCVOID lpBuffer,
								       DWORD nNumberOfBytesToWrite,
								       LPDWORD lpNumberOfBytesWritten)
{
	return WriteFile (hFile,
                      lpBuffer,
					  nNumberOfBytesToWrite,
					  lpNumberOfBytesWritten,
                      NULL);
}

int XilEnvInternal_InitPipeCommunication(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo)
{
    TaskInfo->ConnectTo = XilEnvInternal_ConnectToPipe;
    TaskInfo->DisconnectFrom = XilEnvInternal_DisConnectFromrPipe;
	TaskInfo->TransactMessage = XilEnvInternal_TransactMessagePipe;
	TaskInfo->ReadMessage = XilEnvInternal_ReadMessagePipe;
    TaskInfo->WriteMessage = XilEnvInternal_WriteMessagePipe;

    return 0;
}

int XilEnvInternal_IsPipeInstanceIsRunning(char *par_InstanceName, char *par_ServerName, int par_Timout_ms)
{
    PIPE_API_PING_MESSAGE Req;
    PIPE_API_PING_MESSAGE_ACK Ack;
    DWORD BytesRead;
    int Ret;

    HANDLE Socket = XilEnvInternal_ConnectToPipe(par_InstanceName, par_ServerName, par_Timout_ms, 0);
    if (Socket == INVALID_HANDLE_VALUE) {
        return 0;   // Is not running
    }
    Req.Command = PIPE_API_PING_CMD;
    Ret = XilEnvInternal_TransactMessagePipe(Socket, &Req, sizeof(Req), &Ack, sizeof(Ack), &BytesRead);
    XilEnvInternal_DisConnectFromrPipe(Socket);
    return Ret;
}

int XilEnvInternal_WaitTillPipeInstanceIsRunning(char *par_InstanceName, char *par_ServerName, int par_Timout_ms)
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
        HANDLE Socket = XilEnvInternal_ConnectToPipe(par_InstanceName, par_ServerName, par_Timout_ms, 0);
        if (Socket == INVALID_HANDLE_VALUE) {
#if (_MSC_Ver < 1500)
            if (GetTickCount() > End) break;   // Timeout expired
#else
            if (GetTickCount64() > End) break;   // Timeout Zeit abgelaufen
#endif
            Sleep(100);
            continue;   // is not running
        }
        Req.Command = PIPE_API_PING_CMD;
        Ret = XilEnvInternal_TransactMessagePipe(Socket, &Req, sizeof(Req), &Ack, sizeof(Ack), &BytesRead);
        XilEnvInternal_DisConnectFromrPipe(Socket);
        if (Ret) break;
    }
    return Ret;
}

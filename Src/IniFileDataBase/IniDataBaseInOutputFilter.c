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

#include "Platform.h"
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#endif

#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "IniDatabaseInOutputFilter.h"

#define SIZE_OF_PIPE_FILE_ARRAY  16
static FILE *PipeFileArray[SIZE_OF_PIPE_FILE_ARRAY];
static FILE *ErrFileArray[SIZE_OF_PIPE_FILE_ARRAY];
static HANDLE ProcessArray[SIZE_OF_PIPE_FILE_ARRAY];

static int AddToPipeFileArray(FILE *par_PipeFile, FILE *par_Err, HANDLE par_Process)
{
    int x;
    for (x = 0; x < SIZE_OF_PIPE_FILE_ARRAY; x++) {
        if (PipeFileArray[x] == NULL) {
            PipeFileArray[x] = par_PipeFile;
            ErrFileArray[x] = par_Err;
            ProcessArray[x] = par_Process;
            return 0;
        }
    }
    return -1;
}

int CloseInOrOutputFilterProcessPipe(FILE *par_PipeFile)
{
#ifdef _WIN32
    int x;
    DWORD ExitCode;
    for (x = 0; x < SIZE_OF_PIPE_FILE_ARRAY; x++) {
        if (PipeFileArray[x] == par_PipeFile) {
            PipeFileArray[x] = NULL;
            fclose(par_PipeFile);
            // look if there is something in the StdErr pipe
            if (ErrFileArray[x] != NULL) {
                int Size = 1024;
                int Pos = 0;
                char *ErrString = my_malloc(Size);
                while (1) {
                    int c = fgetc(ErrFileArray[x]);
                    if (c == EOF) {
                        ErrString[Pos] = 0;
                        break;
                    } else {
                        ErrString[Pos] = c;
                        Pos++;
                    }
                }
                if (Pos > 0) {
                    ThrowError(1, "convert throw an error: %s", ErrString);
                }
                fclose(ErrFileArray[x]);
            }
            WaitForSingleObject(ProcessArray[x], 5000);
            GetExitCodeProcess(ProcessArray[x], &ExitCode);
            CloseHandle(ProcessArray[x]);
            return 0;
        }
    }
#else
    ThrowError(1, "the call parameters -ini_input_filter, -ini_output_filter and -ini_filter are available only on windows");
#endif
    return -1;
}

int IsInOrOutputFilterProcessPipe(FILE *par_PipeFile)
{
    int x;
    for (x = 0; x < SIZE_OF_PIPE_FILE_ARRAY; x++) {
        if (PipeFileArray[x] == par_PipeFile) {
            return 1;
        }
    }
    return 0;
}

FILE *CreateInOrOutputFilterProcessPipe(const char *par_ExecName, const char *par_FileName, int par_InOrOut)
{
    FILE *Ret = NULL;
    FILE *Err = NULL;
#ifdef _WIN32
    char CmdLine[2*MAX_PATH+64];
    PROCESS_INFORMATION ProcInfo;
    STARTUPINFO StartInfo;
    BOOL Success = FALSE;
    SECURITY_ATTRIBUTES Attr;
    HANDLE ChildStdInRd = NULL;
    HANDLE ChildStdInWr = NULL;
    HANDLE ChildStdOutRd = NULL;
    HANDLE ChildStdOutWr = NULL;
    HANDLE ChildStdErrRd = NULL;
    HANDLE ChildStdErrWr = NULL;

    Attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    Attr.bInheritHandle = TRUE;
    Attr.lpSecurityDescriptor = NULL;

    if (par_InOrOut == 1) {
        // StdIn
        if (!CreatePipe(&ChildStdInRd, &ChildStdInWr, &Attr, 0)) {
            ThrowError(1, "CreatePipe");
            return NULL;
        }
        // not inherited
        if (!SetHandleInformation(ChildStdInWr, HANDLE_FLAG_INHERIT, 0)) {
            ThrowError(1, "SetHandleInformation");
            return NULL;
        }
    } else {
        // StdOut
        if (!CreatePipe(&ChildStdOutRd, &ChildStdOutWr, &Attr, 0)) {
            ThrowError(1, "CreatePipe");
            return NULL;
        }
        // not inherited
        if (!SetHandleInformation(ChildStdOutRd, HANDLE_FLAG_INHERIT, 0)) {
            ThrowError(1, "SetHandleInformation");
            return NULL;
        }
    }
    // StdErr
    if (!CreatePipe(&ChildStdErrRd, &ChildStdErrWr, &Attr, 0)) {
        ThrowError(1, "CreatePipe");
        return NULL;
    }
    // not inherited
    if (!SetHandleInformation(ChildStdErrRd, HANDLE_FLAG_INHERIT, 0)) {
        ThrowError(1, "SetHandleInformation");
        return NULL;
    }
    ZeroMemory(&ProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&StartInfo, sizeof(STARTUPINFO));
    StartInfo.cb = sizeof(STARTUPINFO);

    StringCopyMaxCharTruncate(CmdLine, "\"", sizeof(CmdLine));
    STRING_APPEND_TO_ARRAY(CmdLine, par_ExecName);
    STRING_APPEND_TO_ARRAY(CmdLine, "\"");
    if (par_InOrOut == 1) {
        StartInfo.hStdInput = ChildStdInRd;
        STRING_APPEND_TO_ARRAY(CmdLine, " -o ");
    } else {
        StartInfo.hStdOutput = ChildStdOutWr;
        STRING_APPEND_TO_ARRAY(CmdLine, " -i ");
    }
    StartInfo.dwFlags |= STARTF_USESTDHANDLES;
    STRING_APPEND_TO_ARRAY(CmdLine, "\"");
    STRING_APPEND_TO_ARRAY(CmdLine, par_FileName);
    STRING_APPEND_TO_ARRAY(CmdLine, "\"");
    StartInfo.hStdError = ChildStdErrWr;

    Success = CreateProcess(NULL,
                            CmdLine,     // command line
                            NULL,        // process security attributes
                            NULL,        // primary thread security attributes
                            TRUE,        // handles are inherited
                            0,           // creation flags
                            NULL,        // use parent's environment
                            NULL,        // use parent's current directory
                            &StartInfo,  // STARTUPINFO pointer
                            &ProcInfo);  // receives PROCESS_INFORMATION

    if (!Success)  {
        ThrowError(1, "cannot create filter process \"%s\"", CmdLine);
        return NULL;
    } else {
        //CloseHandle(ProcInfo.hProcess);
        CloseHandle(ProcInfo.hThread);

        // Close handles to the stdin and stdout pipes no longer needed by the child process.
        // If they are not explicitly closed, there is no way to recognize that the child process has ended.
        if (par_InOrOut == 1) {
            CloseHandle(ChildStdInRd);
            int fd = _open_osfhandle((intptr_t)ChildStdInWr, _O_WRONLY);
            Ret = fdopen(fd, "wt");
            if (Ret == NULL) {
                char *Error = GetLastSysErrorString();
                ThrowError(1, "fdopen() %s", Error);
            }
        } else {
            CloseHandle(ChildStdOutWr);
            int fd = _open_osfhandle((intptr_t)ChildStdOutRd, _O_RDONLY);
            Ret = fdopen(fd, "rt");
            if (Ret == NULL) {
                char *Error = GetLastSysErrorString();
                ThrowError(1, "fdopen() %s", Error);
            }
        }
        // StdErr
        CloseHandle(ChildStdErrWr);
        int fd = _open_osfhandle((intptr_t)ChildStdErrRd, _O_RDONLY);
        Err = fdopen(fd, "rt");
        if (Err == NULL) {
            char *Error = GetLastSysErrorString();
            ThrowError(1, "fdopen() %s", Error);
        }
    }
    AddToPipeFileArray(Ret, Err, ProcInfo.hProcess);
#else
    ThrowError(1, "the call parameters -ini_input_filter, -ini_output_filter and -ini_filter are available only on windows");
#endif
    return Ret;
}

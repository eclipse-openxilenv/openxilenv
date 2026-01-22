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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <process.h>
#else
#include <sys/wait.h>
#endif

#include "MemZeroAndCopy.h"
#include "InterfaceToScript.h"
#include "Message.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Files.h"
#include "ScriptStartExe.h"
#include "Scheduler.h"
#include "MainValues.h"
#include "MyMemory.h"
#include "ThrowError.h"

#define UNUSED(x) (void)(x)

HANDLE start_exe (char *par_CommandString, char **ret_ErrMsg)
{
#ifdef _WIN32
    PROCESS_INFORMATION ProcInfo;
    STARTUPINFO         StartInfo;
    SECURITY_ATTRIBUTES sa;

    MEMSET (&StartInfo, 0, sizeof (StartInfo));
    StartInfo.cb = sizeof(STARTUPINFO);
    StartInfo.dwFlags = STARTF_USESHOWWINDOW;
    if (s_main_ini_val.ScriptStartExeAsIcon) {
        StartInfo.wShowWindow   = SW_MINIMIZE;
    } else {
        StartInfo.wShowWindow   = SW_SHOWDEFAULT;
    }
    StartInfo.lpReserved    = NULL;
    StartInfo.lpDesktop     = NULL;
    StartInfo.lpTitle       = NULL;
    StartInfo.cbReserved2   = 0;
    StartInfo.lpReserved2   = NULL;
    MEMSET (&sa, 0, sizeof (sa));
    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = FALSE;

    // Start external process
    if (CreateProcess (NULL,
                       par_CommandString,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_NEW_CONSOLE,
                       NULL,
                       NULL,
                       &StartInfo,
                       &ProcInfo)) {

        return ProcInfo.hProcess;
    } else {
        char *p;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR)ret_ErrMsg,
                       0, NULL);
        // Remove whitespaces or carriage returns from the end
        p = *ret_ErrMsg;
        while (*p != 0) p++;
        while ((p > *ret_ErrMsg) && isspace(*(p-1))) p--;
        *p = 0;

        return INVALID_HANDLE_VALUE;
    }
#else
    pid_t Pid;
    char c;
    int pipefd[2]; // 0 -> read, 1 -> write

    if (pipe(pipefd) != 0) {
        ThrowError (1, "cannot create a pipe \"%s\"", strerror(errno));
        return INVALID_HANDLE_VALUE;
    }
    Pid = fork();
    switch(Pid) {
    case -1:
        *ret_ErrMsg = strerror(errno);
        close (pipefd[0]);
        close (pipefd[1]);
        return INVALID_HANDLE_VALUE;
    case 0:  // Child
        {
            int ret;
            int is_script = 0;
            FILE *fh;
            char *Cmd;
            char *Par = NULL;
            char *EndCmd;
            char *c = par_CommandString;
            while ((*c != 0) && isspace(*c)) c++;
            Cmd = c;
            while ((*c != 0) && !isspace(*c)) c++;
            EndCmd = c;
            while ((*c != 0) && isspace(*c)) c++;
            if (c != 0) {
                Par = c;
            }
            *EndCmd = 0;
            fh = fopen(Cmd, "rt");
            if (fh != NULL) {
                if (getc(fh) == '#') {
                    if (getc(fh) == '!') {
                        is_script = 1;
                    }
                }
                fclose(fh);
            }

            if (is_script) {
                ret = execl("/bin/bash", "bash" "-c", Cmd, Par, NULL);
            } else {
                ret = execl(Cmd, Par, NULL);
            }
            // kommt nur im Fehlerfall hier her
            {
                char *p = strerror(errno);
                write(pipefd[1], p, strlen(p)+1);
            }
            close (pipefd[1]);
            exit (EXIT_FAILURE);
        }
        return INVALID_HANDLE_VALUE;
    default:  // Parent
        close (pipefd[1]);
        read(pipefd[0], &c, 1);
        if (c == 0) {
            // alles gut
            return (HANDLE)Pid;
        } else {
            int pos = 0;
            int size = 0;
            char *p = NULL;
            do {
                if (size <= (pos+1)) size += 1024;
                p = my_realloc(p, size);
                p[pos] = c;
                pos++;
                read(pipefd[0], &c, 1);
            } while (c != 0);
            p[pos] = 0;
            *ret_ErrMsg = p;
            return INVALID_HANDLE_VALUE;
        }
        close (pipefd[0]);
    }
#endif
}

int wait_for_end_of_exe (HANDLE hProcess, int par_Timeout, DWORD *ret_ExitCode)
{
#ifdef _WIN32
    if (par_Timeout > 0) {
        // wait till application is terminated
        WaitForSingleObject (hProcess, par_Timeout);
    }
    if (GetExitCodeProcess(hProcess, ret_ExitCode))  {
        if (*ret_ExitCode == 259) {
            return 0;
        }
        return 1;
    } else {
        return 0;
    }
#else
    UNUSED(par_Timeout);
    int Status;
    // If a command is not found, the child process created to execute it returns a status of 127.
    // If a command is found but is  not  executable, the return status is 126.
    if (waitpid((pid_t)hProcess, &Status, WNOHANG) == (pid_t)hProcess) {
        *ret_ExitCode = (DWORD)WEXITSTATUS(Status);
        return 1;
    } else {
        return 0;
    }
#endif
}

void FreeStartExeErrorMsgBuffer(char *par_ErrMsg)
{
#ifdef _WIN32
    LocalFree (par_ErrMsg);
#else
    my_free(par_ErrMsg);
#endif
}

void StartExeFreeProcessHandle(HANDLE hProcess)
{
#ifdef _WIN32
    if (hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(hProcess);
    }
#else
    UNUSED(hProcess);
#endif
}

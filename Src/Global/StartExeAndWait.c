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
#include "Platform.h"
#include "MyMemory.h"
#include "MemZeroAndCopy.h"
#include "PrintFormatToString.h"
#ifdef _WIN32
#include <process.h>
#endif
#include "StartExeAndWait.h"


/* GLOBAL DEFINES */
#ifndef FALSE
 #define FALSE 0
#endif
#ifndef TRUE
 #define TRUE  1
#endif

int StartExeAndWait(char *exename,char *aufrufparameter)
{
#ifdef _WIN32
    static STARTUPINFO         sStartInfo;
    static SECURITY_ATTRIBUTES sa;
    static PROCESS_INFORMATION sProcInfo;

    DWORD  ExitCode;

    BOOL returnwert;
    char hstring[2048+2];

    MEMSET (&sStartInfo, 0, sizeof (sStartInfo));
    sStartInfo.cb            = sizeof(STARTUPINFO);
    sStartInfo.dwFlags       = STARTF_USESHOWWINDOW;
    sStartInfo.wShowWindow   = SW_SHOWDEFAULT;
    sStartInfo.lpReserved    = NULL;
    sStartInfo.lpDesktop     = NULL;
    sStartInfo.lpTitle       = NULL;
    sStartInfo.cbReserved2   = 0;
    sStartInfo.lpReserved2   = NULL;
    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = TRUE;

    /* Delete infos from last StartExeAndWait call */
    if(sProcInfo.hProcess) {
        CloseHandle(sProcInfo.hProcess);
    }
    sProcInfo.hProcess = 0;

    PrintFormatToString(hstring,sizeof(hstring),"%s %s",exename,aufrufparameter);
    returnwert = CreateProcess(NULL,
                          hstring,
                          NULL,
                          NULL,
                          FALSE,
                          CREATE_NEW_CONSOLE,
                          NULL,
                          NULL,
                          &sStartInfo,
                          &sProcInfo);
    if (!returnwert) {
        switch (GetLastError()) {
        case 0: /* system out of memory resources */
        break;

        case ERROR_BAD_FORMAT:
        break;

        case ERROR_FILE_NOT_FOUND:
        break;

        case ERROR_PATH_NOT_FOUND:
        break;

        default:
        break;
        }
    } else { /* CreateProcess was successfull */
        /* Open handle to process otherwise no GetExitCodeProcess() not possible */
        sProcInfo.hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, sProcInfo.dwProcessId   );
   }

    while(1) {
        if ((GetExitCodeProcess(sProcInfo.hProcess, &ExitCode)) == FALSE) { /* function call failed */
            return (FALSE);
        }

        if(ExitCode != STILL_ACTIVE) {
            CloseHandle(sProcInfo.hProcess);
            sProcInfo.hProcess   = 0;
            return(TRUE);
        }
   }
#else
    char *CommandLine;
    int Ret;
    int Len = strlen (exename) + strlen (aufrufparameter) + 32;
    CommandLine = my_malloc (Len);
    PrintFormatToString (CommandLine, Len, "%s %s", exename, aufrufparameter);
    Ret = system (CommandLine);
    my_free (CommandLine);
    return Ret;
#endif
}

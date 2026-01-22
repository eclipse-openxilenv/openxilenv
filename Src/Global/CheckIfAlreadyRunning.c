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
#include "PrintFormatToString.h"
#include "ThrowError.h"
#include "MainValues.h"
#include "FileExtensions.h"
#include "CheckIfAlreadyRunning.h"


#ifdef _WIN32
int TryToLockInstance (char *Instance)
{
    HGLOBAL hMap;
    char Name[MAX_PATH + 100];

    PrintFormatToString (Name, sizeof(Name), "XilEnv_with_instance_%s_active", Instance);
    hMap = CreateFileMapping (INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0L, sizeof (HWND), Name);
    if (hMap == 0) {
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL);
        LocalFree (lpMsgBuf);
    }
    if (hMap != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMap);
        return 0;
    } else {
        return 1;
    }
}

#else

int TryToLockInstance (char *Instance)
{
    char Name[MAX_PATH + 100];
    if (CheckOpenIPCFile(Instance, "Lock", Name, sizeof(Name), DIR_CREATE_EXIST, FILENAME_CREATE_EXIST) == 0) {
        int fh = open(Name, O_RDWR);
        if (fh > 0) {
            if (lockf(fh, F_TLOCK, 1) == -1) {
                close(fh);
                return 0;   // ist von einem anderen Instanz gelockt
            } else {
                return 1;   // ist jetz von dieser Instanz gelockt
            }
        }
    }
    return 1;   // im Fehlerfall lieber mal starten!?
}
#endif

// This function lives inside AlreadyRunnung.c
extern int WaitUntilXilEnvCanStart(char *par_InstanceName, void *par_Application);

int CheckIfAlreadyRunning (char *par_InstanceName, void *par_Application)
{
    if (!TryToLockInstance (par_InstanceName)) {
        return WaitUntilXilEnvCanStart (par_InstanceName, par_Application);
    } else {
        return 0;
    }
}

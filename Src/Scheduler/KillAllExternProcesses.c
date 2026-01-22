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
#include <stdlib.h>
#include "Platform.h"

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#endif

#include "ThrowError.h"
#include "PrintFormatToString.h"
#include "PipeMessagesShared.h"
#include "KillAllExternProcesses.h"

#ifdef _WIN32
static HANDLE hEvent;
#else
static sem_t *KillAllExternProcessSemaphore;
#endif

int InitKillAllExternProcesses (char *par_Prefix)
{
#ifndef _WIN32
    int SemValue;
#endif
    char EventName[MAX_PATH];

    PrintFormatToString (EventName, sizeof(EventName), KILL_ALL_EXTERN_PROCESS_EVENT "_%s", par_Prefix);   // The name can have a "Global\" or "Local\" prefix to explicitly create the object in the global or session namespace
#ifdef _WIN32
    hEvent = CreateEvent (NULL, TRUE, FALSE, EventName);
    if (hEvent == NULL) {
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
        ThrowError (1, "CreateEvent failed (%s)\n", lpMsgBuf);
        LocalFree (lpMsgBuf);
        return -1;
    }
#else
    KillAllExternProcessSemaphore = sem_open(EventName, O_CREAT, 0777, 0);
    sem_getvalue(KillAllExternProcessSemaphore, &SemValue);
    if (KillAllExternProcessSemaphore == NULL) ThrowError (1, "cannot create semaphore %s", EventName);
    // Semaphore 0 setzen
    else do {
        sem_trywait(KillAllExternProcessSemaphore);
        sem_getvalue(KillAllExternProcessSemaphore, &SemValue);
    } while (SemValue != 0);
#endif
    return 0;
}


void KillAllExternProcesses (void)
{
#ifdef _WIN32
    PulseEvent (hEvent);
#else
    int SemValue;
    do {
        sem_post(KillAllExternProcessSemaphore);
        usleep(10*1000);  // 10ms warten
        sem_getvalue(KillAllExternProcessSemaphore, &SemValue);
    } while (SemValue == 0);
    sem_trywait(KillAllExternProcessSemaphore);
#endif

}

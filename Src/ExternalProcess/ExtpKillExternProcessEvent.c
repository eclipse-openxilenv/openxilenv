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
#include <string.h>
#include <ctype.h>
#include "Platform.h"
#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#endif
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "XilEnvExtProc.h"
#include "ExtpBaseMessages.h"
#include "PipeMessagesShared.h"
#include "ExtpKillExternProcessEvent.h"


#ifdef _WIN32
static DWORD WINAPI XilEnvInternal_KillExternProcessThreadFunction (LPVOID lpParam)
#else
static void* XilEnvInternal_KillExternProcessThreadFunction(void* lpParam)
#endif
{
#ifdef _WIN32
    HANDLE hEvent;
#else
    sem_t *KillAllExternProcessSemaphore;
#endif
    char EventName[MAX_PATH];
    PrintFormatToString (EventName, sizeof(EventName), KILL_ALL_EXTERN_PROCESS_EVENT "_%s", (char*)lpParam);   // The name can have a "Global\" or "Local\" prefix to explicitly create the object in the global or session namespace
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
    }
    // Wait on the kill event
    WaitForSingleObject (hEvent, INFINITE);
#else
    KillAllExternProcessSemaphore = sem_open(EventName, O_RDWR | O_CREAT, 0777, 0);
    if (KillAllExternProcessSemaphore == NULL) {
        ThrowError(1, "sem_open (\"%s\") get error %i, %s", EventName, errno, strerror(errno));
        return 0;
    }
    sem_wait(KillAllExternProcessSemaphore);
#endif
    KillExternProcessHimSelf ();

    return 0;
}

int XilEnvInternal_StartKillEventThread (char *par_Prefix)
{
    char *PrefixBuffer;
    int Len = strlen(par_Prefix) + 1;

    // This memory will never give free
    PrefixBuffer = (char*)malloc(Len);
    StringCopyMaxCharTruncate (PrefixBuffer, par_Prefix, Len);

#ifdef _WIN32
    HANDLE hThread;
    DWORD dwThreadId = 0; 

    hThread = CreateThread ( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            XilEnvInternal_KillExternProcessThreadFunction,    // thread proc
            (void*)PrefixBuffer,    // thread parameter 
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
        ThrowError (1, "cannot create terminate extern process thread (%s)\n", lpMsgBuf);
        free (lpMsgBuf);
        return -1;
    }
#else
    int ret;
    pthread_t Thread;
    pthread_attr_t Attr;

    pthread_attr_init(&Attr);
    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&Attr, 256 * 1024);
    if (ret) {
        printf("pthread setstacksize failed\n");
        return -1;
    }
    if (pthread_create(&Thread, &Attr, XilEnvInternal_KillExternProcessThreadFunction, (void*)PrefixBuffer) != 0) {
        ThrowError(1, "cannot create communication thread %s", strerror(errno));
        return -1;
    }
#endif
    return 0;
}


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


#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include "Platform.h"

#define NO_ERROR_FUNCTION_DECLARATION

#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "ConfigurablePrefix.h"
#include "Scheduler.h"
#include "ScriptMessageFile.h"
#include "ErrorDialog.h"
#include "ThrowError.h"

#define UNUSED(x) (void)(x)

int ThrowErrorString (int level, uint64_t Cycle, const char *MessageBuffer)
{
    char *HeaderBuffer;
    int SizeOfHeaderBuffer;
    char processname[MAX_PATH];

    get_real_running_process_name (processname, sizeof(processname));

    SizeOfHeaderBuffer = 20 + strlen (processname);
    HeaderBuffer = (char*)my_malloc (SizeOfHeaderBuffer);

    switch(level) {
    default:
    case MESSAGE_STOP:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Information from %s", processname);
        break;
    case ERROR_NO_CRITICAL_STOP:
    case ERROR_OKCANCEL:
    case ERROR_OK_OKALL_CANCEL:
    case ERROR_OK_CANCEL_CANCELALL:
    case ERROR_OK_OKALL_CANCEL_CANCELALL:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Error in %s", processname);
        break;
    case MESSAGE_OKCANCEL:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Message from %s", processname);
        break;
    case QUESTION_OKCANCEL:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Question from %s", processname);
        break;
    case ERROR_CRITICAL:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Critical error in %s", processname);
        break;
    case ERROR_SYSTEM_CRITICAL:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Fatal error in %s", processname);
        break;
    case INFO_NO_STOP:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Information from %s:", processname);
        break;
    case WARNING_NO_STOP:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Warning from %s:", processname);
        break;
    case ERROR_NO_STOP:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Error in %s:", processname);
        break;
    case INFO_NOT_WRITEN_TO_MSG_FILE:
        PrintFormatToString (HeaderBuffer, SizeOfHeaderBuffer, "Information from %s:", processname);
        break;
    }
#ifdef NO_GUI
    printf ("Cycle %" PRIu64 " %s: %s\n", Cycle, HeaderBuffer, MessageBuffer);
    if (GetError2MessageFlag()) {
        FILE *fh;
        if ((fh = fopen(GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_ERROR_FILE), "a")) != NULL) {
            fprintf(fh, "%s\n  %s\n", HeaderBuffer, MessageBuffer);
            fclose(fh);
        }
        AddScriptMessage(HeaderBuffer);
        AddScriptMessage(MessageBuffer);
    }
    my_free(HeaderBuffer);
    my_free(MessageBuffer);
    return IDOK;
#else
    return ErrorMessagesFromOtherThreadInst_Error(level, Cycle, HeaderBuffer, MessageBuffer, IsMainThread());
#endif
}

int ThrowError (int level, const char *format, ...)
{
    int Ret;
    int Size, Len;
    char *MessageBuffer;
    va_list args;

    Size = 256;  // first try with 256 bytes
    do {
        Size = Size + (Size >> 1);  // add 50% more
        MessageBuffer = (char*)(my_malloc((size_t)(Size)));
        if (MessageBuffer == NULL) {
            return IDOK;
        }
        va_start(args, format);
        Len = vsnprintf(MessageBuffer, (size_t)(Size), format, args);
        va_end(args);
    } while (Len >= Size);

    va_start(args, format);
    Ret = ThrowErrorString(level, GetCycleCounter(), MessageBuffer);
    va_end(args);
    // my_free(MessageBuffer);  !!! not free the memory here this will happen inside ErrorMessagesFromOtherThread::Error !!!
    return Ret;
}

int ThrowErrorWithCycle (int level, uint64_t Cycle, const char *format, ...)
{
    int Ret;
    int Size, Len;
    char *MessageBuffer = NULL;
    va_list args;

    Size = 256;  // first try with 256 bytes
    do {
        Size = Size + (Size >> 1);  // add 50% more
        MessageBuffer = (char*)(my_realloc(MessageBuffer, (size_t)(Size)));
        if (MessageBuffer == NULL) {
            return IDOK;
        }
        va_start(args, format);
        Len = vsnprintf(MessageBuffer, (size_t)(Size), format, args);
        va_end(args);
    } while (Len >= Size);
    Ret = ThrowErrorString(level, Cycle, MessageBuffer);
    // my_free(MessageBuffer);  !!! not free the memory here this will happen inside ErrorMessagesFromOtherThread::Error !!!
    return Ret;
}

int conv_error2message = 0;

void SetError2Message (int flag)
{
    conv_error2message = flag;
}

int GetError2MessageFlag (void)
{
    return conv_error2message;
}

char *GetLastSysErrorString (void)
{
#ifdef _WIN32
    LPTSTR lpMsgBuf;
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   GetLastError (),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                   (LPTSTR)&lpMsgBuf,
                   0,
                   NULL);
    return lpMsgBuf;
#else
    return strerror(errno);
#endif
}

void FreeLastSysErrorString (char *String)
{
#ifdef _WIN32
    LocalFree (String);
#else
    UNUSED(String);
#endif
}


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


#include <stdarg.h>
#include <stdio.h>
#include "Platform.h"
#include "XilEnvExtProc.h"
#include "ExtpBlackboard.h"
#include "ExtpBaseMessages.h"
#include "ExtpMemoryAllocation.h"
#include "ExtpExtError.h"
#include "ExtpError.h"

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ ThrowError(int level,            // Error level
                                                        char const *format,   // Format string (same as printf)
                                                        ...                   // Parameter list
)
{
    int len, size = 128;
    char *buffer = NULL;
    int ret;

    va_list args;
    va_start(args, format);
    do {
        size <<= 1; // double the size
        buffer = (char*)XilEnvInternal_realloc(buffer, size);
        if (buffer == NULL) break;
#if ((defined _WIN32) && !(defined __GNUC__))
#if (_MSC_VER > 1400)
        len = _vsnprintf_s(buffer, size, size - 1, format, args);
#else
        len = _vsnprintf(buffer, size, format, args);
#endif
#else
        len = vsnprintf(buffer, size - 1, format, args);
#endif
    } while ((len <= 0) || (len >= (size - 1)));
    if (buffer == NULL) {
        return -1;
    }

    va_end(args);
    ret = ThrowErrorNoVariableArguments(level, buffer);
    if (buffer != NULL) XilEnvInternal_free(buffer);
    return ret;
}

int XilEnvInternal_ThrowErrorNoVariableArguments (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos,
                                                 int level,                       // Error level
                                                 char const *text)                // Output string
{
    int Ret = IDOK;

    if (TaskInfos == NULL) {
        message_output_stream_inner_function(text, 0, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 1);
    } else {
        // Avoid recursive calling
        if (!TaskInfos->InsideErrorCallFlag) {
            TaskInfos->InsideErrorCallFlag = 1;
            message_output_stream_inner_function (text, TaskInfos->State, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 1);
            TaskInfos->InsideErrorCallFlag = 0;
        }
    }
__OUT:
    return Ret;
}

// same as error function above
int XilEnvInternal_ThrowError(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos, 
                               int level,                       // Error level
                               char const *format,              // Format string (same as printf)
                               ...                              // Parameter list
)
{
    int len, size = 128;
    char *buffer = NULL;
    int ret;

    va_list args;
    va_start(args, format);
    do {
        size <<= 1; // double the size
        buffer = (char*)XilEnvInternal_realloc(buffer, size);
        if (buffer == NULL) break;
#if ((defined _WIN32) && !(defined __GNUC__))
#if (_MSC_VER > 1400)
        len = _vsnprintf_s(buffer, size, size - 1, format, args);
#else
        len = _vsnprintf(buffer, size, format, args);
#endif
#else
        len = vsnprintf(buffer, size - 1, format, args);
#endif
    } while ((len <= 0) || (len >= (size - 1)));
    if (buffer == NULL) {
        return -1;
    }

    va_end(args);
    ret = XilEnvInternal_ThrowErrorNoVariableArguments(TaskInfos, level, buffer);
    if (buffer != NULL) XilEnvInternal_free(buffer);
    return ret;
}


EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ ThrowErrorNoVariableArguments (int level,                       // Error level
                                                                          char const *text)                // Output string
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos;
    int Ret = IDOK;

    TaskInfos = XilEnvInternal_GetTaskPtr ();
    Ret = XilEnvInternal_ThrowErrorNoVariableArguments(TaskInfos, level, text);
    if (TaskInfos != NULL) {
        XilEnvInternal_ReleaseTaskPtr(TaskInfos);
    } 
    return Ret;
}

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ ThrowErrorVariableArguments (int level,                       // Error level
                                                            const char *format,
                                                            va_list argptr)
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos;
    int Ret = IDOK;
    int len, size = 128;
    char *buffer = NULL;

    do {
        size <<= 1; // double the size
        buffer = (char*)XilEnvInternal_realloc (buffer, size);
        if (buffer == NULL) break;
#if ((defined _WIN32) && !(defined __GNUC__))
#if (_MSC_VER > 1400)
        len = _vsnprintf_s (buffer, size, size-1, format, argptr);
#else
        len = _vsnprintf (buffer, size, format, argptr);
#endif
#else
        len = vsnprintf(buffer, size-1, format, argptr);
#endif
    } while ((len <= 0) || (len >= (size-1)));
    if (buffer == NULL) {
        return -1;
    }

    TaskInfos = XilEnvInternal_GetTaskPtr ();
    if (TaskInfos == NULL) {
        message_output_stream_inner_function(buffer, 0, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 1);
    } else {
        // Avoid recursive calling
        if (!TaskInfos->InsideErrorCallFlag) {
            TaskInfos->InsideErrorCallFlag = 1;
            message_output_stream_inner_function (buffer, TaskInfos->State, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 1);
            TaskInfos->InsideErrorCallFlag = 0;
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfos);
    }
__OUT:
    if (buffer != NULL) XilEnvInternal_free (buffer);
    return Ret;
}

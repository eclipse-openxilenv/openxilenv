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
#include <time.h>
#include <stdarg.h>
#include "Config.h"
#include "Platform.h"

#include "XilEnvExtProc.h"
#include "ExtpBlackboard.h"
#include "ExtpBaseMessages.h"
#include "ExtpMemoryAllocation.h"

#include "ExtpExtError.h"

static void TerminateSelf (void)
{
    char *Buffer;
    int Vid;
    int Len = strlen(DEFAULT_PREFIX_TYPE_SHORT_BLACKBOARD);
    Len += strlen("exit");
    Len += 1;
    Buffer = (char*)XilEnvInternal_malloc(Len);
    strcpy(Buffer, DEFAULT_PREFIX_TYPE_SHORT_BLACKBOARD);
    strcat(Buffer, "exit");
    Vid = add_bbvari (Buffer, BB_UWORD, NULL);
    write_bbvari_uword (Vid, 1);}

static int OpenMessageFileDelayed (void)
{
    FILE *fh;
    char Flags[3];
    EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos();

    XilEnvInternal_LockProcessPtr(ProcessInfos);
    if (ProcessInfos->MessageOutputStreamConfig.pFilename != NULL) {

        if ((ProcessInfos->MessageOutputStreamConfig.Flags & APPEND_TO_EXISTING_FILE_FLAG) == APPEND_TO_EXISTING_FILE_FLAG) {
            strcpy (Flags, "a");
        } else {
            strcpy (Flags, "w");
        }
        if ((fh = fopen (ProcessInfos->MessageOutputStreamConfig.pFilename, Flags)) == NULL) {
            ProcessInfos->MessageOutputStreamConfig.MessageToFileFlag = 0;
            XilEnvInternal_ReleaseProcessPtr(ProcessInfos);
            if ((ProcessInfos->MessageOutputStreamConfig.Flags & NO_ERR_MSG_IF_INIT_ERR_FLAG) != NO_ERR_MSG_IF_INIT_ERR_FLAG) {
                ThrowError (1, "cannot open file %s", ProcessInfos->MessageOutputStreamConfig.pFilename);
            }
            return -1;
        } else {
            ProcessInfos->MessageOutputStreamConfig.MessageToFileFlag = 1;
            if (ProcessInfos->MessageOutputStreamConfig.HeaderText != NULL) {
                fputs (ProcessInfos->MessageOutputStreamConfig.HeaderText, fh);
                XilEnvInternal_free (ProcessInfos->MessageOutputStreamConfig.HeaderText);
                ProcessInfos->MessageOutputStreamConfig.HeaderText = NULL;
                fputs ("\n", fh);
            }
            fclose (fh);
        }
    }
    XilEnvInternal_ReleaseProcessPtr(ProcessInfos);
    return 0;
}

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ init_message_output_stream (const char *para_MessageFilename, int para_Flags, int para_MaxErrorMessageCounter,
                                                                         const char *para_HeaderText)
{
    int Ret = 0;
    EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos();

    XilEnvInternal_LockProcessPtr(ProcessInfos);
    ProcessInfos->MessageOutputStreamConfig.pFilename = NULL;
    ProcessInfos->MessageOutputStreamConfig.Flags = para_Flags;
    ProcessInfos->MessageOutputStreamConfig.MaxErrorMessageCounter = para_MaxErrorMessageCounter;
    ProcessInfos->MessageOutputStreamConfig.ErrorMessageCounter = 0;

    if (para_HeaderText != NULL) {
        ProcessInfos->MessageOutputStreamConfig.HeaderText = XilEnvInternal_malloc (strlen (para_HeaderText) + 1);
        if (ProcessInfos->MessageOutputStreamConfig.HeaderText != NULL) {
            strcpy (ProcessInfos->MessageOutputStreamConfig.HeaderText, para_HeaderText);
        }
    }
    if ((para_Flags & MESSAGE_COUNTER_FLAG) == MESSAGE_COUNTER_FLAG) {
        ProcessInfos->MessageOutputStreamConfig.MessageCounterVid = add_bbvari ("MessageCounter", BB_UDWORD, "");
        write_bbvari_udword (ProcessInfos->MessageOutputStreamConfig.MessageCounterVid, 0);
        Ret = -1;
    }
#if defined(_WIN32) || defined(__linux__)
    if ((para_Flags & ENVVAR_ERR_MSG_FLAG) == ENVVAR_ERR_MSG_FLAG) {
        if (para_MessageFilename != NULL) { 
            if (!GetEnvironmentVariable (para_MessageFilename,
                                         ProcessInfos->MessageOutputStreamConfig.MessageFilename, MAX_PATH)) {
                ProcessInfos->MessageOutputStreamConfig.MessageToFileFlag = 0;
                XilEnvInternal_ReleaseProcessPtr(ProcessInfos);
                if ((para_Flags & NO_ERR_MSG_IF_INIT_ERR_FLAG) != NO_ERR_MSG_IF_INIT_ERR_FLAG) {
                    ThrowError (1, "environment variable %s doesn't exist", para_MessageFilename);
                }
                return -1;
            } else {
                ProcessInfos->MessageOutputStreamConfig.pFilename = ProcessInfos->MessageOutputStreamConfig.MessageFilename;
            }
        } else {
            ProcessInfos->MessageOutputStreamConfig.MessageToFileFlag = 0;
            XilEnvInternal_ReleaseProcessPtr(ProcessInfos);
            if ((para_Flags & NO_ERR_MSG_IF_INIT_ERR_FLAG) != NO_ERR_MSG_IF_INIT_ERR_FLAG) {
                ThrowError (1, "invalid environment variable NULL");
            }
            return -1;
        }
    } else
#endif
    if ((para_Flags & FILENAME_ERR_MSG_FLAG) == FILENAME_ERR_MSG_FLAG) {
        if (para_MessageFilename != NULL) {
            strncpy (ProcessInfos->MessageOutputStreamConfig.MessageFilename, para_MessageFilename, MAX_PATH);
            ProcessInfos->MessageOutputStreamConfig.MessageFilename[MAX_PATH-1] = 0;
            ProcessInfos->MessageOutputStreamConfig.pFilename = ProcessInfos->MessageOutputStreamConfig.MessageFilename;
        } else {
            ProcessInfos->MessageOutputStreamConfig.MessageToFileFlag = 0;
            XilEnvInternal_ReleaseProcessPtr(ProcessInfos);
            if ((para_Flags & NO_ERR_MSG_IF_INIT_ERR_FLAG) != NO_ERR_MSG_IF_INIT_ERR_FLAG) {
                ThrowError (1, "invalid file name NULL");
            }
            return -1;
        }
    }
    XilEnvInternal_ReleaseProcessPtr(ProcessInfos);

    // immediately open the file
    if ((para_Flags & OPEN_FILE_AT_FIRST_MSG_FLAG) != OPEN_FILE_AT_FIRST_MSG_FLAG) {
         Ret = OpenMessageFileDelayed ();
    }
    return Ret;
}

static void message_popup (const char *par_Message, int par_Connected, int par_NoGui, int par_Err2Msg)
{
    EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos ();

    if (par_Connected) {
        EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        XilEnvInternal_PipeErrorPopupMessageAndWait(TaskInfo, 1, par_Message);
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    } else {
        if (par_NoGui || par_Err2Msg) {
            printf ("message from \"%s\":\n   %s\n", ProcessInfos->ExecutableName, par_Message);
        } else {
#if _WIN32
            char app_name[MAX_PATH + 32];
            strcpy (app_name, "message from ");
            GetModuleFileName (NULL, app_name + strlen (app_name), MAX_PATH);
            MessageBox (NULL,
                        par_Message,
                        app_name,
                        MB_ICONSTOP | MB_SETFOREGROUND | MB_OK);
#else
            printf("message from \"%s\":\n   %s\n", ProcessInfos->ExecutableName, par_Message);
#endif
        }
    }
}

void message_output_stream_inner_function (const char *loc_message, int par_Connected, int par_NoGui, int par_Err2Msg, int par_ErrMsg)
{
    EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos();

    ProcessInfos->MessageOutputStreamConfig.ErrorMessageCounter++;
    if (par_Connected && !par_ErrMsg && ((ProcessInfos->MessageOutputStreamConfig.Flags & MESSAGE_COUNTER_FLAG) == MESSAGE_COUNTER_FLAG)) {
        write_bbvari_udword (ProcessInfos->MessageOutputStreamConfig.MessageCounterVid, ProcessInfos->MessageOutputStreamConfig.ErrorMessageCounter);
    }

    if (par_ErrMsg || ((ProcessInfos->MessageOutputStreamConfig.Flags & NO_POPUP_ERR_MSG_FLAG) != NO_POPUP_ERR_MSG_FLAG)) {
        // Make a popup window with a message
        message_popup (loc_message, par_Connected, par_NoGui, par_Err2Msg);
    }
    if (((ProcessInfos->MessageOutputStreamConfig.Flags & WRITE_MSG_TO_SCRIPT_MSG_FILE) == WRITE_MSG_TO_SCRIPT_MSG_FILE) && par_Connected) {
        XilEnvInternal_PipeWriteToMessageFile (loc_message);
    }

    // Generate the message file here delayed
    if (ProcessInfos->MessageOutputStreamConfig.HeaderText != NULL) {
        OpenMessageFileDelayed ();
    }
    if (ProcessInfos->MessageOutputStreamConfig.MessageToFileFlag) {
        FILE *loc_ausgabehandle;

        XilEnvInternal_LockProcessPtr(ProcessInfos);
        if ((loc_ausgabehandle = fopen (ProcessInfos->MessageOutputStreamConfig.MessageFilename, "a")) == NULL) {
            XilEnvInternal_ReleaseProcessPtr(ProcessInfos);
            ThrowError (1, "cannot open file %s", ProcessInfos->MessageOutputStreamConfig.MessageFilename);
        } else {
            unsigned char *loc_datum_als_string;
            time_t loc_datum;
            // Read date and time
            time (&loc_datum);
            loc_datum_als_string = ctime (&loc_datum);

            fprintf (loc_ausgabehandle,
                     "\n\nDate/Time: %s\n%s\n\n",
                     loc_datum_als_string,
                     loc_message);
            // Is the maximul allowed error reached?
            if (ProcessInfos->MessageOutputStreamConfig.MaxErrorMessageCounter &&
                (ProcessInfos->MessageOutputStreamConfig.ErrorMessageCounter >= ProcessInfos->MessageOutputStreamConfig.MaxErrorMessageCounter)) {
                TerminateSelf ();
                fprintf (loc_ausgabehandle,
                         "\n%d ERRORS:\nautomatically terminate XilEnv\n",
                         ProcessInfos->MessageOutputStreamConfig.ErrorMessageCounter);
            }
            fclose (loc_ausgabehandle);
            XilEnvInternal_ReleaseProcessPtr(ProcessInfos);
        }
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ message_output_stream (const char *format, ...)
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos;
    int len, size = 128;
    char *buffer = NULL;

    va_list args;
    va_start(args, format);
    do {
        size <<= 1; // double the buffer size
        buffer = (char*)XilEnvInternal_realloc (buffer, size);
        if (buffer == NULL) break;
#if ((defined _WIN32) && !(defined __GNUC__))
#if (_MSC_VER > 1400)
        len = _vsnprintf_s (buffer, size, size-1, format, args);
#else
        len = _vsnprintf (buffer, size, format, args);
#endif
#else
        len = vsnprintf(buffer, size-1, format, args);
#endif
    } while ((len <= 0) || (len >= (size-1)));
    va_end(args);
    if (buffer == NULL) {
        return;
    }
    TaskInfos = XilEnvInternal_GetTaskPtr ();
    if (TaskInfos == NULL) {
        message_output_stream_inner_function(buffer, 0, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 0);
    } else {
        int State = TaskInfos->State;
        XilEnvInternal_ReleaseTaskPtr(TaskInfos);
        message_output_stream_inner_function(buffer, State, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 0);
    }
    if (buffer != NULL) XilEnvInternal_free(buffer); 
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ message_output_stream_no_variable_arguments (const char *string)
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos;
    TaskInfos = XilEnvInternal_GetTaskPtr ();
    if (TaskInfos == NULL) {
        message_output_stream_inner_function(string, 0, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 0);
    } else {
        int State = TaskInfos->State;
        XilEnvInternal_ReleaseTaskPtr(TaskInfos);
        message_output_stream_inner_function(string, State, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 0);
    }
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ message_output_stream_va_list (const char *format, va_list argptr)
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos;
    int len, size = 128;
    char *buffer = NULL;

    do {
        size <<= 1; //  double the buffer size
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
        return;
    }

    TaskInfos = XilEnvInternal_GetTaskPtr ();
    if (TaskInfos == NULL) {
        message_output_stream_inner_function(buffer, 0, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 0);
    } else {
        int State = TaskInfos->State;
        XilEnvInternal_ReleaseTaskPtr(TaskInfos);
        message_output_stream_inner_function(buffer, State, XilEnvInternal_GetNoGuiFlag(), XilEnvInternal_GetErr2MsgFlag(), 0);
    }
    if (buffer != NULL) XilEnvInternal_free(buffer); 
}

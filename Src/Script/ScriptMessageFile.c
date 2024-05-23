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
#include <string.h>
#include <time.h>
#include "MainValues.h"
#include "FileExtensions.h"
#include "Message.h"
#include "Config.h"
#include "Files.h"
#include "ThrowError.h"
#include "ConfigurablePrefix.h"

#include "MessageWindow.h"

#include "InterfaceToScript.h"

#include "ScriptMessageFile.h"

static int FlagForFirstCall = TRUE;
static FILE *messagefile = NULL;
static char messagefilename[MAX_PATH_LAENGE] = "script.msg";

void init_messagefilename (char *prefix)
{
	sprintf (messagefilename, "%sscript.msg", prefix);
}

void AddScriptMessageOnlyMessageWindow (const char *szText)
{
    AddMessages (szText);
}

void AddScriptMessage (const char *szText)
{
    AddScriptMessageOnlyMessageWindow (szText);
    AddScriptMessageOnlyMessageFile (szText);
}


static CRITICAL_SECTION ScriptMessageCriticalSection;

void EnterScriptMessageCS (void)
{
    ENTER_CS (&ScriptMessageCriticalSection);
}

void LeaveScriptMessageCS (void)
{
    LEAVE_CS (&ScriptMessageCriticalSection);
}

void InitScriptMessageCS (void)
{
    INIT_CS (&ScriptMessageCriticalSection);
}


void InitScriptMessageCriticalSection (void)
{
    InitScriptMessageCS();
}

void AddScriptMessageOnlyMessageFile(const char *szText)
{

    // If it is the first call?
    if(FlagForFirstCall) {
        // Print the file header
        GenerateNewMessageFile();
        FlagForFirstCall = FALSE;
    }
    // Is the message file closed than reopen it
    if(messagefile == NULL) {
        messagefile = open_file(messagefilename, "a");
    }
    EnterScriptMessageCS ();
    if (messagefile != NULL) {
        fputs ("\n", messagefile);
        fputs (szText, messagefile);
    }
    LeaveScriptMessageCS ();
}

int GenerateNewMessageFile (void)
{
    char *Date;
    time_t DateTime;

    CloseScriptMessageFile (0);

    ScriptIdentifyPath (messagefilename);

    messagefile = open_file (messagefilename, "wt");
    if (messagefile == NULL) {
#ifdef _WIN32
        DWORD ErrorCode;
        LPVOID lpMsgBuf;

        ErrorCode = GetLastError();

        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       ErrorCode,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                       (LPTSTR) &lpMsgBuf,
                       0, NULL);
        ThrowError (1, "cannot open file %s (%s)" , messagefilename, lpMsgBuf);
        // Free the buffer allocated by FormateMessage-Call
        LocalFree( lpMsgBuf );
#else
        ThrowError (1, "cannot open file %s" , messagefilename);
#endif
        return(1);
    }

    fprintf (messagefile, "\nThis file is generated automatically from %s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME));
    fprintf (messagefile, "\n%s message file: %s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), messagefilename);
    fprintf (messagefile, "\n%s version    : %.3lf", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), XILENV_VERSION/1000.0);
#if (XILENV_MINOR_VERSION < 0)
    fprintf (messagefile, " pre%i", XILENV_MINOR_VERSION);
#elif (XILENV_MINOR_VERSION > 0)
    fprintf (messagefile, " patch-%i", XILENV_MINOR_VERSION);
#endif
    time (&DateTime);
    Date  = ctime(&DateTime);
    fprintf (messagefile, "\ndate               : %s", Date);
    fprintf (messagefile, "\n\n++++++++++++++++++++++++++++++++++++++++++++++");
    return(0);
}

void CloseScriptMessageFile (int ReopenAbillityFlag)
{
    EnterScriptMessageCS ();
    if (!ReopenAbillityFlag) {
        FlagForFirstCall = TRUE;
    }
    if (messagefile != NULL) {
        fflush (messagefile);
        AddScriptMessageOnlyMessageWindow ("close script file");
        close_file (messagefile);
        messagefile = NULL;
    }
    LeaveScriptMessageCS ();
}

FILE * GetMessageFileHandle(void)
{
    return (messagefile);
}


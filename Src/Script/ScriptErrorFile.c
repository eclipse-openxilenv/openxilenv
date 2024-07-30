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
#include "Platform.h"
#include "ConfigurablePrefix.h"
#include "ScriptErrorFile.h"

#include "Config.h"
#include "Files.h"
#include <time.h>

#include "MainWindow.h"

#include "InterfaceToScript.h"

static FILE *ErrorFile;
/* Name of the error file */
static char ErrorFileName [MAX_PATH] = "script.err";

void init_ErrorFilename (char *prefix)
{
    sprintf (ErrorFileName, "%sscript.err", prefix);
}

static uint32_t nr_of_errors   = 0;
static uint32_t nr_of_warnings = 0;

static void DeleteErrorFile(void);

int GenerateNewErrorFile(void)
{
    // reset the counter
    nr_of_errors   = 0;
    nr_of_warnings = 0;

    // If message file ist still open, close it
    CloseScriptErrorFile();

    // extend message file name with path
    // Important if script will change the directory
    ScriptIdentifyPath (ErrorFileName);

    // delete existing file from previos run
    DeleteErrorFile();

    ErrorFile = open_file (ErrorFileName, "w+");
    if (ErrorFile == NULL) {
        nr_of_errors++;
        return(1);
    }

    // Write file header
    fprintf (ErrorFile, "\nThis file is generated automatically from %s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME));
    fprintf (ErrorFile, "\n%s error file: %s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), ErrorFileName);
    fprintf (ErrorFile, "\n%s version    : %d.%d.%d", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), XILENV_VERSION, XILENV_MINOR_VERSION, XILENV_PATCH_VERSION);
    /* Date and time */
    {
    char *Date;
    time_t DateAndTime;
    time (&DateAndTime);
    Date   = ctime(&DateAndTime);
    fprintf (ErrorFile, "\ndate               : %s", Date);
    }
    fprintf (ErrorFile, "\n\n++++++++++++++++++++++++++++++++++++++++++++++");
    fprintf (ErrorFile, "\n");

    return(0);
}


static void DeleteErrorFile(void)
{
    remove (ErrorFileName);
}

void CloseScriptErrorFile (void)
{
    if (ErrorFile != NULL) {
        if (nr_of_errors) {
            fprintf (ErrorFile, "\n\n!!!ERROR!!! %u Errors found in the Scriptfile(s)", nr_of_errors);
        }
        if (nr_of_warnings) {
            fprintf (ErrorFile, "\n\n!WARNING! %u Warnings found in the Scriptfile(s)", nr_of_warnings);
        }
        if ((nr_of_errors == 0) && (nr_of_warnings == 0)) {
            fprintf (ErrorFile, "\n\nno warnings or errors found in the Scriptfile(s)");
        }
        close_file(ErrorFile);
        ErrorFile = NULL;
    }
}

uint32_t GetErrorCounter(void)
{
    return(nr_of_errors);
}

void AddScriptError (char *szText, char counter_increment)
{
    if(ErrorFile == NULL) {
        ErrorFile = open_file(ErrorFileName, "a");
    }

    if(ErrorFile != NULL) {
        if (szText == NULL) {
            fflush (ErrorFile);
        } else {
            fprintf (ErrorFile,"\n %s", szText);
            fflush (ErrorFile);
        }
    }

    // Increment the counter
    switch (counter_increment) {
    case NO_COUNTER: // do nothing
        break;
    case WARN_COUNTER: nr_of_warnings ++;
        break;
    case ERR_COUNTER:  nr_of_errors ++;
        break;
    default: // Error
        break;
    }
}

// If an error occur inside the script XilEnv should be terminate if the switch -ScriptErrExit
// is set

static int ScriptErrExitFlags = 0;

void SetScriptErrExit (int par_Flags)
{
    ScriptErrExitFlags |= par_Flags;
}

int ExitIfErrorInScript (void)
{
    if ((ScriptErrExitFlags & TERMINATE_XILENV_ON_SCRIPT_EXECUTION_ERROR) == TERMINATE_XILENV_ON_SCRIPT_EXECUTION_ERROR) {
        CloseFromOtherThread(1, 1);  // exit XilEnv with return code 1
    }
    return 0;
}

int ExitIfSyntaxErrorInScript (void)
{
    if ((ScriptErrExitFlags & TERMINATE_XILENV_ON_SCRIPT_SYNTAX_ERROR) == TERMINATE_XILENV_ON_SCRIPT_SYNTAX_ERROR) {
        CloseFromOtherThread(1, 1);  // exit XilEnv with return code 1
    }
    return 0;
}

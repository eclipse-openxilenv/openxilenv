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
#include <ctype.h>
#include "Platform.h"
#include <malloc.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "Config.h"
#include "Scheduler.h"
#include "Files.h"
#include "MainValues.h"
#include "Script.h"
#include "EnvironmentVariables.h"
#include "StringMaxChar.h"
#include "InterfaceToScript.h"

char           script_filename[MAX_PATH];
unsigned char  script_status_flag;

// this function will extend a file name with whole current path
// if it the file name doesn't include a complete path starting with a
// drive letter
// Remark the par_Filename must be >= MAX_PATH
void ScriptIdentifyPath (char *par_Filename)
{

    if ((par_Filename != NULL) && (strlen(par_Filename) > 0)) {
        char Buffer[MAX_PATH];
        SearchAndReplaceEnvironmentStrings (par_Filename, Buffer, MAX_PATH);

        if ((((Buffer[0] >= 'a') && (Buffer[0] <= 'z')) ||
             ((Buffer[0] >= 'A') && (Buffer[0] <= 'Z'))) &&
              (Buffer[1] == ':') &&
              ((Buffer[2] == '\\') || (Buffer[2] == '/'))) {
            StringCopyMaxCharTruncate(par_Filename, Buffer, MAX_PATH);
        } else {
#ifdef _WIN32
            getcwd (par_Filename, MAX_PATH);
            StringAppendMaxCharTruncate(par_Filename, "\\", MAX_PATH);
            StringAppendMaxCharTruncate(par_Filename, Buffer, MAX_PATH);
#else
            StringCopyMaxCharTruncate(par_Filename, Buffer, MAX_PATH);
#endif
        }
    }
    return;
}

#define  BUFFER_SIZE 1024

int copy_file (const char *quelldateiname, const char *zieldateiname)
{
    char Buffer[BUFFER_SIZE];
    size_t r;
    int Count = 0;
    FILE *ReadFile;
    FILE *WriteFile;

    /* Check if file name is valid and has no wildcards */
    if (strstr(quelldateiname, "*") || strstr(quelldateiname, "?") ||
        strstr(zieldateiname , "*") || strstr(zieldateiname , "?")) {
        return (-2); /* There are wildcards inside the filename */
    }

    ReadFile  = fopen(quelldateiname, "rb");
    WriteFile = fopen(zieldateiname , "wb");
    if ((ReadFile == NULL) || (WriteFile == NULL)) {
        return (-1);
    }

    do {
        r = fread(Buffer, 1, BUFFER_SIZE, ReadFile);
        fwrite (Buffer, 1, r, WriteFile);
        Count += (int)r;
    } while (r == BUFFER_SIZE);

    fclose(ReadFile);
    fclose(WriteFile);
    return (Count);
}


int init_script      (void);
void cyclic_script    (void);
void terminate_script (void);

TASK_CONTROL_BLOCK script_tcb
    = INIT_TASK_COTROL_BLOCK(SCRIPT_PN, INTERN_ASYNC, 220, cyclic_script, init_script, terminate_script, 64);


int GetStrictScriptSyntaxCheckState (void)
{  
    return 1;  // is now always active
}

void SetStrictScriptSyntaxCheck (void)
{  
    //  is now always active
}

void ResetStrictScriptSyntaxCheck (void)
{  
    // is now always active
}


const char *GetRunningScriptPath (void)
{
    if ((script_status_flag == RUNNING) ||
        (script_status_flag == START)) {
        return GetCurrentScriptPath ();
    }
    return NULL;
}

int GetRunningScriptLine (void)
{
    if ((script_status_flag == RUNNING) ||
        (script_status_flag == START)) {
        return GetCurrentLineNr ();    
    }
    return -1;
}


void init_debugfilename (char *prefix);
void init_messagefilename (char *prefix);
void init_ErrorFilename (char *prefix);

int SetScriptOutputFilenamesPrefix (char *par_Prefix)
{
    strncpy (s_main_ini_val.ScriptOutputFilenamesPrefix, par_Prefix, 200);
    s_main_ini_val.ScriptOutputFilenamesPrefix[200] = 0;
	init_debugfilename (s_main_ini_val.ScriptOutputFilenamesPrefix);
	init_messagefilename (s_main_ini_val.ScriptOutputFilenamesPrefix);
    init_ErrorFilename (s_main_ini_val.ScriptOutputFilenamesPrefix);
    return 0;
}

int StartScript(const char *par_Script)
{
    int Ret;
    ScriptEnterCriticalSection();
    if (script_status_flag != RUNNING) {
        SearchAndReplaceEnvironmentStrings (par_Script, script_filename, MAX_PATH);
        script_status_flag = START;
        Ret = 0;
    } else {
        Ret = -1;
    }
    ScriptLeaveCriticalSection();
    return Ret;
}

int GetScriptState()
{
    return (script_status_flag == RUNNING);
}

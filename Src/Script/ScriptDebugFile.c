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
#include "MainValues.h"
#include "ConfigurablePrefix.h"
#include "ScriptDebugFile.h"
#include "Message.h"
#include "BlackboardAccess.h"
#include "IniDataBase.h"

#include "Config.h"
#include "Files.h"
#include <time.h>

#include "InterfaceToScript.h"

FILE *DebugFile;
/* Name of the debug file */
static char debugfilename [MAX_PATH_LAENGE] = "script.dbg";

void init_debugfilename (char *prefix)
{
	sprintf (debugfilename, "%sscript.dbg", prefix);
}

static void DeleteDebugFile(void);

int GenerateNewDebugFile(void)
{
    if (!s_main_ini_val.DontMakeScriptDebugFile) {
        // If message file is already open close it
        CloseScriptDebugFile();

        // Extend message file name with path
        // Important if script will change the directory
        ScriptIdentifyPath (debugfilename);

        // delete existing file from previos XilEnv run
        DeleteDebugFile();

        DebugFile = open_file (debugfilename, "w+");
        if (DebugFile == NULL) {
            return(1);
        }
        char IniFileName[MAX_PATH];
        IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), IniFileName, sizeof(IniFileName));

        // Write the file head
        fprintf (DebugFile, "\nThis file is generated automatically from %s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME));
        fprintf (DebugFile, "\n%s debug file file: %s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), debugfilename);
        fprintf (DebugFile, "\n%s version    : %d.%d.%d", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), XILENV_VERSION, XILENV_MINOR_VERSION, XILENV_PATCH_VERSION);
        fprintf (DebugFile, "\n%s INI file   : %s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), IniFileName);
        /* Date and time */
        {
            char *Date;
            time_t DateAndTime;
            time (&DateAndTime);
            Date   = ctime(&DateAndTime);
            fprintf (DebugFile, "\ndate               : %s", Date);
        }
        fprintf (DebugFile, "\n\n++++++++++++++++++++++++++++++++++++++++++++++");
        fprintf (DebugFile, "\n");
    } else {
        // If message file is sill open
        CloseScriptDebugFile();
        // And delete it
        DeleteDebugFile();
    }
    return 0;
}

static void DeleteDebugFile(void)
{
    remove (debugfilename);
}

void CloseScriptDebugFile(void)
{
    if (DebugFile != NULL) {
        close_file(DebugFile);
        DebugFile = NULL;
    }
 }

void AddScriptDebug(char *szText)
{
  if (!s_main_ini_val.DontMakeScriptDebugFile) {
      if(DebugFile == NULL) {
          DebugFile = open_file(debugfilename, "a");
      }
      if(DebugFile != NULL) {
          fprintf(DebugFile,"\n%10u %s", read_bbvari_udword (CycleCounterVid), szText);
      }
  }
}


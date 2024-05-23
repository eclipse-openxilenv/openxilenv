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
#include "Config.h"
#include "Files.h"
#include "StringMaxChar.h"
#include "MainValues.h"
#include "InitProcess.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "ScriptErrorFile.h"
#include "IniFileDosNotExistDialog.h"
#include "InterfaceToScript.h"

#include "ParseCommandLine.h"

#define CMDL_SCR_PARA     "-script"
#define CMDL_INI_PARA     "-ini"
#define CMDL_CALLFROM_PARAM "-CallFrom"
#define CMDL_INSTANCE_PARAM "-Instance"

#define ENVIRONMENT_VARNAME_CALLFROM    "XilEnv_CallFrom"
#define ENVIRONMENT_VARNAME_INSTANCE    "XilEnv_Instance"

#define UNUSED(x) (void)(x)

#ifndef _WIN32
static char *CommandLineBuffer;

int SaveCommandArgs(int argc, char *argv[])
{
    int x;
    int Len = 1;

    for (x = 1; x < argc; x++) {
        Len += strlen(argv[x]) + 1;
    }
    CommandLineBuffer = my_malloc(Len);
    if (CommandLineBuffer == NULL) {
        return -1;
    }
    CommandLineBuffer[0] = 0;
    for (x = 1; x < argc; x++) {
        strcat(CommandLineBuffer, argv[x]);
        if (x < (argc-1)) strcat(CommandLineBuffer, " ");
    }
    return 0;
}

static char *GetCommandLineSavedBuffer(void)
{
    return CommandLineBuffer;
}
#endif

// Start XilEnvGui as icon (-icon parameter)
static int StartAsIconFlag;
static int NoGuiFlag =
#ifdef NO_GUI
        1;
#else
        0;
#endif

static int NoXcpFlag;

int ShouldStartedAsIcon (void)
{
    return StartAsIconFlag;
}

int OpenWithNoGui (void)
{
    return NoGuiFlag;
}

int GetNoXcp (void)
{
    return NoXcpFlag;
}


static char *GetNextParameter(char **CommandLine, const char *par_Ignored)
{
    UNUSED(par_Ignored);
    int QuotationMarkCounter = 0;
    char *s;
    char *Ret;
    s = *CommandLine;
    while (isspace (*s)) s++;

    if (*s == 0) return NULL;   // no next parameter

    // remove tailing "
    while (*s == '\"') {
        s++;
        while (isspace (*s)) s++;
        QuotationMarkCounter++;
    }
    if (*s == 0) return NULL; // there was a " but no next parameter

    // there starts the parameter
    Ret = s;

    while ((*s != 0) &&                                        // parameter stops because end of line
           (*s != '\"') &&                                     // or "
           !((QuotationMarkCounter == 0) && isspace (*s))) {   // or a white char outside ""
        s++;
    }

    if (QuotationMarkCounter > 0) {
        // remove following "
        while ((QuotationMarkCounter-- > 0) && (*s == '\"')) {
            *s = 0;
            s++;
            while (isspace(*s)) s++;
        }
    } else {
        if (*s != 0) {
            *s = 0;
            s++;
        }
    }

    *CommandLine = s;
    return Ret;
}

int ParseCommandLine (char *ret_IniFile, unsigned int par_MaxCharsIniFile,
                      char *ret_Instance, unsigned int par_MaxCharsInstance, void *par_Application)
{
    char *szCmdLine;
    char *szToken;

    LPSTR lpCmdLineSys;
    LPSTR lpCmdLine;

    int CallFromParameterFound = 0;
    int InstanceParameterFound = 0;

    *ret_Instance = 0;  // default is no instance name
    *ret_IniFile = 0;
#ifdef NO_GUI
    NoGuiFlag = 1;
#endif

    // EnterGlobalCriticalSection ();

#ifdef _WIN32
    lpCmdLineSys = GetCommandLine();
#else
    lpCmdLineSys = GetCommandLineSavedBuffer ();
#endif
    // Make a copy the string will changed
    lpCmdLine = my_malloc (strlen (lpCmdLineSys)+1);
    if (lpCmdLine == NULL) {
        ThrowError (1, "out of memory");
        return -1;
    }
    strcpy (lpCmdLine, lpCmdLineSys);


    // Search inside the complete command line string
    for (szCmdLine = lpCmdLine; (szToken = GetNextParameter(&szCmdLine, " ")) != NULL;) {
        // Maybe XilEnv have an other name
        if (stricmp(szToken, "-program_name") == 0) {
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                SetProgramNameIntoMainSettings(szToken);
            } else {
                // Leave the loop if no more token found
                break;
            }
        }

        if (stricmp(szToken, "-exit_on_script_syntax_error") == 0) {
            SetScriptErrExit(TERMINATE_XILENV_ON_SCRIPT_SYNTAX_ERROR);
        }

        if (stricmp(szToken, "-exit_on_script_execution_error") == 0) {
            SetScriptErrExit(TERMINATE_XILENV_ON_SCRIPT_EXECUTION_ERROR);
        }

        if (stricmp(szToken, "-scripterrexit") == 0) {
            SetScriptErrExit (TERMINATE_XILENV_ON_SCRIPT_EXECUTION_ERROR | TERMINATE_XILENV_ON_SCRIPT_SYNTAX_ERROR);
        }

        if (stricmp(szToken, "-err2msg") == 0) {
            SetError2Message (1);
        }

        if (stricmp(szToken, "-icon") == 0) {
            StartAsIconFlag = 1;
        }

        if (stricmp(szToken, "-nogui") == 0) {
            NoGuiFlag = 1;
        }

        if (stricmp(szToken, "-noxcp") == 0) {
            NoXcpFlag = 1;
        }

        if (stricmp(szToken, "-darkmode") == 0) {
            SetDarkModeIntoMainSettings(1);
        }

        if (stricmp(szToken, "-nodarkmode") == 0) {
            SetDarkModeIntoMainSettings(0);
        }

        if (stricmp(szToken, "-sys") == 0) {
            // get next token
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                if (access(szToken, 0) == 0) {
                    SetRemoteMasterExecutableCmdLine (szToken);
                }
            } else {
                // Leave the loop if no more token found
                break;
            }            
        }

        if (stricmp(szToken, "-port") == 0) {
            // get next token
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                if (access(szToken, 0) == 0) {
                    SetExternProcessLoginSocketPort (szToken);
                }
            } else {
                // Leave the loop if no more token found
                break;
            }
        }

        // Script parameter
        if (stricmp(szToken, CMDL_SCR_PARA) == 0) {
            // get next token
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                SetStartupScript (szToken);
            } else {
                // Leave the loop if no more token found
                break;
            }
        }

        // INI file parameter
        if (stricmp(szToken, CMDL_INI_PARA) == 0) {
            // get next token
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                if (access(szToken, 0) == 0) {
                    StringCopyMaxCharTruncate(ret_IniFile, szToken, par_MaxCharsIniFile);
                } else {
                    char NewSelectedIniFile[MAX_PATH];
                    // If -err2msg flag set no error message leave XilEnv immediately
                    if (GetError2MessageFlag()) return -1;

                    if (IniFileDosNotExistDialog (szToken, NewSelectedIniFile, sizeof (NewSelectedIniFile),
                                                  par_Application)) {
                        return -1;
                    } else {
                        StringCopyMaxCharTruncate(ret_IniFile, NewSelectedIniFile, par_MaxCharsIniFile);
                    }
                }
            } else {
                // Leave the loop if no more token found
                break;
            }
        }
        if (stricmp(szToken, CMDL_CALLFROM_PARAM) == 0) {
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                SetCallFromProcess (szToken);
                CallFromParameterFound = 1;
            } else {
                // Leave the loop if no more token found
                break;
            }
        }

        if (stricmp(szToken, CMDL_INSTANCE_PARAM) == 0) {
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                StringCopyMaxCharTruncate(ret_Instance, szToken, par_MaxCharsInstance);
                InstanceParameterFound = 1;
            } else {
                // Leave the loop if no more token found
                break;
            }
        }

        // Prefix parameter
        if (stricmp(szToken, "-prefix") == 0) {
            // get next token
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                SetScriptOutputFilenamesPrefix (szToken);
            } else {
                // Leave the loop if no more token found
                break;
            }
        }
        // Write a log file with all files would be attached
        if (stricmp(szToken, "-file_access_logging") == 0) {
            // get next token
            if ((szToken = GetNextParameter(&szCmdLine, " ")) != NULL) {
                s_main_ini_val.LogFileForAccessedFilesFlag = 1;
                strncpy (s_main_ini_val.LogFileForAccessedFilesName, szToken, sizeof (s_main_ini_val.LogFileForAccessedFilesName) - 1);
                s_main_ini_val.LogFileForAccessedFilesName[sizeof (s_main_ini_val.LogFileForAccessedFilesName) - 1] = 0;
            } else {
                // Leave the loop if no more token found
                break;
            }
        }

    } // for(;;)

    // Are there a INI file name transfered? otherwise ask for one
    if (ret_IniFile[0] == 0) {
        char NewSelectedIniFile[MAX_PATH];
        switch (IniFileDosNotExistDialog (ret_IniFile, NewSelectedIniFile, sizeof (NewSelectedIniFile),
                                          par_Application)) {
        case -1:   // Exit
            if (lpCmdLine != NULL) {
                my_free (lpCmdLine);
            }
            return -1;
        case 0:  // generated a new INI file
                 // or choised an existing INI file
            StringCopyMaxCharTruncate(ret_IniFile, NewSelectedIniFile, par_MaxCharsIniFile);
            break;
        }
    }

    if (!CallFromParameterFound) {
        int Ret;
        char CallFrom[8*MAX_PATH];   // Max. 8 * nested process calls: Extproc1 calls Extproc2 calls ... Extproc8 calls XilEnv
        Ret = (int)GetEnvironmentVariable(ENVIRONMENT_VARNAME_CALLFROM, CallFrom, sizeof(CallFrom));
        if (Ret >= (int)sizeof(CallFrom)) {
            ThrowError (1, "too many call froms (will be ignored)");
        } else if (Ret > 0) {
            SetCallFromProcess (CallFrom);
        }
    }
    if (!InstanceParameterFound) {
        int Ret;
        char Instance[MAX_PATH];   // Max. 8 * nested process calls: Extproc1 calls Extproc2 calls ... Extproc8 calls XilEnv
        Ret = (int)GetEnvironmentVariable(ENVIRONMENT_VARNAME_INSTANCE, Instance, sizeof(Instance));
        if (Ret >= (int)sizeof(Instance)) {
            ThrowError (1, "too many call froms (will be ignored)");
        } else if (Ret > 0) {
            if (strlen (Instance) < par_MaxCharsInstance) {
                strcpy (ret_Instance, Instance);
            } else {
                MEMCPY (ret_Instance, Instance, par_MaxCharsInstance-1);
                ret_Instance[par_MaxCharsInstance-1] = 0;
            }

        }
    } else {
        SetEnvironmentVariable(ENVIRONMENT_VARNAME_INSTANCE, ret_Instance);
    }
    // LeaveGlobalCriticalSection ();

    if (lpCmdLine != NULL) {
        my_free (lpCmdLine);
    }

    if (NoGuiFlag) {
        if (!SetEnvironmentVariable("XilEnv_NoGui", "yes")) {
            ThrowError (1, "cannot set environment variable \"XilEnv_NoGui\" (-nogui will be ignored)");
        }
    }
    return 0;
}


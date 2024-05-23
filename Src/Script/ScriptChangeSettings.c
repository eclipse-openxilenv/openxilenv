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

#include "MyMemory.h"
#include "MainValues.h"
//#include "toolbar.h"
#include "Blackboard.h"
#include "ThrowError.h"

#include "MainWinowSyncWithOtherThreads.h"

#include "ScriptChangeSettings.h"

//#define DEBUG_FPRINTF_LOGGING

static int CheckYesNo (const char *Param, int *pErr)
{
    if (!stricmp ("yes", Param)) {
        *pErr = 0;
        return 1;
    } else if (!stricmp ("no", Param)) {
        *pErr = 0;
        return 0;
    } else {
        *pErr = -2;
        return 0;
    }
}

static struct CHANGE_SETTINGS_STACK_ELEM {
    char *Name;
    char *Value;
} *ChangeSettingsStack;
static int ChangeSettingsStackSize;
static int ChangeSettingsStackPos;

void ResetChangeSettingsStack(void)
{
    int x;
    for (x = 0; x < ChangeSettingsStackPos; x++) {
        my_free(ChangeSettingsStack[x].Value);
    }
    ChangeSettingsStackPos = 0;
}

#ifdef DEBUG_FPRINTF_LOGGING
static void DebugPrintStack(char *Prefix, char *Header, char *Name)
{
    static FILE *fh;
    if (fh == NULL) {
        fh = fopen ("c:\\temp\\settings_stack.txt", "wt");
    }
    if (fh != NULL) {
        int x;
        fprintf (fh, "%s****(%s %s)****\n", Prefix, Header, Name);
        for (x = 0; x < ChangeSettingsStackPos; x++) {
            if (ChangeSettingsStack[x].Name == NULL) fprintf (fh, "  %i. empty\n", x );
            else fprintf (fh, "%s  %i. %s = %s\n", Prefix, x, ChangeSettingsStack[x].Name, ChangeSettingsStack[x].Value);
        }
        fflush (fh);
    }
}
#endif

static int PushToStack (char *Name, char *Value)
{
    int x;
    int ChangeSettingsStackSizeLast;

#ifdef DEBUG_FPRINTF_LOGGING
    DebugPrintStack("", "PushToStack",Name);
#endif
    if (ChangeSettingsStackPos >= ChangeSettingsStackSize) {
        // add 10 more elements
        ChangeSettingsStackSizeLast = ChangeSettingsStackSize;
        ChangeSettingsStackSize += 10;
        ChangeSettingsStack = (struct CHANGE_SETTINGS_STACK_ELEM*)my_realloc (ChangeSettingsStack,
                                                                              (size_t)ChangeSettingsStackSize * sizeof(struct CHANGE_SETTINGS_STACK_ELEM));
        if (ChangeSettingsStack == NULL) {
            ThrowError (1, "out of memory");
            return -1;
        }
        for (x = ChangeSettingsStackSizeLast+1; x < ChangeSettingsStackSize; x++) {
            ChangeSettingsStack[x].Name = NULL;
        }
    }
    ChangeSettingsStack[ChangeSettingsStackPos].Name = Name;
    ChangeSettingsStack[ChangeSettingsStackPos].Value = Value;
    ChangeSettingsStackPos++;
#ifdef DEBUG_FPRINTF_LOGGING
    DebugPrintStack("    ", "PushToStack done", Name);
#endif
    return 0;
}

static char * AllocYesNo (int Value)
{
    char *Ret = my_malloc(4);
    if (Ret == NULL) {
        ThrowError (1, "out of memory");
        return NULL;
    }
    if (Value) strcpy (Ret, "Yes");
    else strcpy (Ret, "No");
    return Ret;
}

static char *GetPopFromStack (char *Name)   // Remark: The returned pointer points to a memory block
                                            // that must be give free witht my_free()
{
    int x;
#ifdef DEBUG_FPRINTF_LOGGING
    DebugPrintStack("", "GetPopFromStack", Name);
#endif
    for (x = ChangeSettingsStackPos-1; x >= 0; x--) {  // Search from the end of stack
        if (!strcmp (ChangeSettingsStack[x].Name, Name)) {
            char *Ret = ChangeSettingsStack[x].Value;
            for (; x < (ChangeSettingsStackPos-1); x++) {
                ChangeSettingsStack[x].Name = ChangeSettingsStack[x+1].Name;
                ChangeSettingsStack[x].Value = ChangeSettingsStack[x+1].Value;
            }
            ChangeSettingsStackPos--;
#ifdef DEBUG_FPRINTF_LOGGING
            DebugPrintStack("    ", "GetPopFromStack done", Name);
#endif
            return Ret;
        }
    }
    return NULL;  // not found
}

// NOT_FASTER_THAN_REALTIME
static int NOT_FASTER_THAN_REALTIME_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) {
        s_main_ini_val.NotFasterThanRealTime = Flag;
        SetNotFasterThanRealtimeState (Flag);
    }
    return Err;
}
static char *NOT_FASTER_THAN_REALTIME_get_func (void)
{
    return AllocYesNo(s_main_ini_val.NotFasterThanRealTime);
}

// DONT_MAKE_SCRIPT_DEBUGFILE
static int DONT_MAKE_SCRIPT_DEBUGFILE_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.DontMakeScriptDebugFile = Flag;
    return Err;
}
static char *DONT_MAKE_SCRIPT_DEBUGFILE_get_func (void)
{
    return AllocYesNo(s_main_ini_val.DontMakeScriptDebugFile);
}

// SCRIPT_START_EXE_AS_ICON
static int SCRIPT_START_EXE_AS_ICON_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.ScriptStartExeAsIcon = Flag;
    return Err;
}
static char* SCRIPT_START_EXE_AS_ICON_get_func (void)
{
    return AllocYesNo(s_main_ini_val.ScriptStartExeAsIcon);
}

// SCRIPT_STOP_IF_CCP_ERROR
static int SCRIPT_STOP_IF_CCP_ERROR_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.ScriptStopIfCcpError = Flag;
    return Err;
}
static char* SCRIPT_STOP_IF_CCP_ERROR_get_func (void)
{
    return AllocYesNo(s_main_ini_val.ScriptStopIfCcpError);
}

// SCRIPT_STOP_IF_CCP_ERROR
static int SCRIPT_STOP_IF_XCP_ERROR_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.ScriptStopIfXcpError = Flag;
    return Err;
}
static char* SCRIPT_STOP_IF_XCP_ERROR_get_func (void)
{
    return AllocYesNo(s_main_ini_val.ScriptStopIfXcpError);
}

// STOP_PLAYER_IF_SCRIPT_STOPPED
static int STOP_PLAYER_IF_SCRIPT_STOPPED_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.StopPlayerIfScriptStopped = Flag;
    return Err;
}
static char* STOP_PLAYER_IF_SCRIPT_STOPPED_get_func (void)
{
    return AllocYesNo(s_main_ini_val.StopPlayerIfScriptStopped);
}

// STOP_RECORDER_IF_SCRIPT_STOPPED
static int STOP_RECORDER_IF_SCRIPT_STOPPED_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.StopRecorderIfScriptStopped = Flag;
    return Err;
}
static char* STOP_RECORDER_IF_SCRIPT_STOPPED_get_func (void)
{
    return AllocYesNo(s_main_ini_val.StopRecorderIfScriptStopped);
}

// STOP_GENERATOR_IF_SCRIPT_STOPPED
static int STOP_GENERATOR_IF_SCRIPT_STOPPED_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.StopGeneratorIfScriptStopped = Flag;
    return Err;
}
static char* STOP_GENERATOR_IF_SCRIPT_STOPPED_get_func (void)
{
    return AllocYesNo(s_main_ini_val.StopGeneratorIfScriptStopped);
}

// STOP_EQUATION_IF_SCRIPT_STOPPED
static int STOP_EQUATION_IF_SCRIPT_STOPPED_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.StopEquationIfScriptStopped = Flag;
    return Err;
}
static char* STOP_EQUATION_IF_SCRIPT_STOPPED_get_func (void)
{
    return AllocYesNo(s_main_ini_val.StopEquationIfScriptStopped);
}

// DONT_INIT_EXISTING_VARIABLES_DURING_RESTART_OF_CAN
static int DONT_INIT_EXISTING_VARIABLES_DURING_RESTART_OF_CAN_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.DontUseInitValuesForExistingCanVariables = Flag;
    return Err;
}
static char* DONT_INIT_EXISTING_VARIABLES_DURING_RESTART_OF_CAN_get_func (void)
{
    return AllocYesNo(s_main_ini_val.DontUseInitValuesForExistingCanVariables);
}

// DONT_WAIT_FOR_RESPONSE_OF_START_RECORDER_RPC
static int DONT_WAIT_FOR_RESPONSE_OF_START_RECORDER_RPC_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.DontWaitForResponseOfStartRecorderRPC = (char)Flag;
    return Err;
}
static char* DONT_WAIT_FOR_RESPONSE_OF_START_RECORDER_RPC_get_func (void)
{
    return AllocYesNo(s_main_ini_val.DontWaitForResponseOfStartRecorderRPC);
}

// SAVE_REF_LIST_IN_NEW_FORMAT_AS_DEFAULT
static int SAVE_REF_LIST_IN_NEW_FORMAT_AS_DEFAULT_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.SaveRefListInNewFormatAsDefault = Flag;
    return Err;
}
static char* SAVE_REF_LIST_IN_NEW_FORMAT_AS_DEFAULT_get_func (void)
{
    return AllocYesNo(s_main_ini_val.SaveRefListInNewFormatAsDefault);
}

// CONVERT_ERRORS_TO_MESSAGE
static int CONVERT_ERRORS_TO_MESSAGE_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) SetError2Message (Flag);
    return Err;
}
static char* CONVERT_ERRORS_TO_MESSAGE_get_func (void)
{
    return AllocYesNo(GetError2MessageFlag());
}

// ADD_BBVARI_DEFAULT_UNI
static int ADD_BBVARI_DEFAULT_UNIT_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err = 0;
    if (!OnlyCheckFlag) {
        if (strlen (Value) >= (BBVARI_UNIT_SIZE - 1)) {
            Err = -1;
        }
    }
    if (!Err && !OnlyCheckFlag) strcpy (s_main_ini_val.Script_ADD_BBVARI_DefaultUnit, Value);
    return Err;
}
static char* ADD_BBVARI_DEFAULT_UNIT_get_func (void)
{
    char *Buffer = my_malloc (strlen(s_main_ini_val.Script_ADD_BBVARI_DefaultUnit) + 1);
    if (Buffer == NULL) {
        ThrowError (1, "out of memory");
        return NULL;
    } else {
        strcpy (Buffer, s_main_ini_val.Script_ADD_BBVARI_DefaultUnit);
    }
    return Buffer;
}

// STOP_SCRIPT_IF_LABEL_DOS_NOT_EXIST
static int STOP_SCRIPT_IF_LABEL_DOS_NOT_EXIST_INSIDE_SVL_set_func (int OnlyCheckFlag, const char* Value)
{
    int Err, Flag = CheckYesNo (Value, &Err);
    if (!Err && !OnlyCheckFlag) s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVL = Flag;
    return Err;
}

static char* STOP_SCRIPT_IF_LABEL_DOS_NOT_EXIST_INSIDE_SVL_get_func (void)
{
    return AllocYesNo(s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVL);
}

static struct CHANGE_SETTINGS_KEYWORD {
    char *Name;
    int (*SetFuncPtr)(int OnlyCheckFlag, const char* Value);
    char* (*GetFuncPtr)(void);
} ChangeSettingsKeywords[] = {
{"NOT_FASTER_THAN_REALTIME", NOT_FASTER_THAN_REALTIME_set_func, NOT_FASTER_THAN_REALTIME_get_func},
{"DONT_MAKE_SCRIPT_DEBUGFILE", DONT_MAKE_SCRIPT_DEBUGFILE_set_func, DONT_MAKE_SCRIPT_DEBUGFILE_get_func},
{"SCRIPT_START_EXE_AS_ICON", SCRIPT_START_EXE_AS_ICON_set_func, SCRIPT_START_EXE_AS_ICON_get_func},
{"SCRIPT_STOP_IF_CCP_ERROR", SCRIPT_STOP_IF_CCP_ERROR_set_func, SCRIPT_STOP_IF_CCP_ERROR_get_func},
{"SCRIPT_STOP_IF_XCP_ERROR", SCRIPT_STOP_IF_XCP_ERROR_set_func, SCRIPT_STOP_IF_XCP_ERROR_get_func},
{"STOP_PLAYER_IF_SCRIPT_STOPPED", STOP_PLAYER_IF_SCRIPT_STOPPED_set_func, STOP_PLAYER_IF_SCRIPT_STOPPED_get_func},
{"STOP_RECORDER_IF_SCRIPT_STOPPED", STOP_RECORDER_IF_SCRIPT_STOPPED_set_func, STOP_RECORDER_IF_SCRIPT_STOPPED_get_func},
{"STOP_GENERATOR_IF_SCRIPT_STOPPED", STOP_GENERATOR_IF_SCRIPT_STOPPED_set_func, STOP_GENERATOR_IF_SCRIPT_STOPPED_get_func},
{"STOP_EQUATION_IF_SCRIPT_STOPPED", STOP_EQUATION_IF_SCRIPT_STOPPED_set_func, STOP_EQUATION_IF_SCRIPT_STOPPED_get_func},
{"DONT_INIT_EXISTING_VARIABLES_DURING_RESTART_OF_CAN", DONT_INIT_EXISTING_VARIABLES_DURING_RESTART_OF_CAN_set_func, DONT_INIT_EXISTING_VARIABLES_DURING_RESTART_OF_CAN_get_func},
{"DONT_WAIT_FOR_RESPONSE_OF_START_RECORDER_RPC", DONT_WAIT_FOR_RESPONSE_OF_START_RECORDER_RPC_set_func, DONT_WAIT_FOR_RESPONSE_OF_START_RECORDER_RPC_get_func},
{"SAVE_REF_LIST_IN_NEW_FORMAT_AS_DEFAULT", SAVE_REF_LIST_IN_NEW_FORMAT_AS_DEFAULT_set_func, SAVE_REF_LIST_IN_NEW_FORMAT_AS_DEFAULT_get_func},
{"CONVERT_ERRORS_TO_MESSAGE", CONVERT_ERRORS_TO_MESSAGE_set_func, CONVERT_ERRORS_TO_MESSAGE_get_func},
{"STOP_SCRIPT_IF_LABEL_DOS_NOT_EXIST_INSIDE_SVL", STOP_SCRIPT_IF_LABEL_DOS_NOT_EXIST_INSIDE_SVL_set_func, STOP_SCRIPT_IF_LABEL_DOS_NOT_EXIST_INSIDE_SVL_get_func},
{"ADD_BBVARI_DEFAULT_UNIT", ADD_BBVARI_DEFAULT_UNIT_set_func, ADD_BBVARI_DEFAULT_UNIT_get_func}
};

#define ELEMT_COUNT   ((int)(sizeof(ChangeSettingsKeywords)/sizeof(ChangeSettingsKeywords[0])))


int ScriptChangeBasicSettings (int Command, int OnlyCheckFlag, const char *Param1, const char *Param2)
{
    int x;
    int Err = 0;
    for (x = 0; x < ELEMT_COUNT; x++) {
        if (!strcmp (Param1, ChangeSettingsKeywords[x].Name)) {
            switch (Command) {
            case CHANGE_SETTINGS_COMMAND_SET:
                Err = ChangeSettingsKeywords[x].SetFuncPtr (OnlyCheckFlag, Param2);
                break;
            case CHANGE_SETTINGS_COMMAND_PUSH_SET:
                if (!OnlyCheckFlag) {
                    char *Value = ChangeSettingsKeywords[x].GetFuncPtr();
                    if (Value != NULL) {
                        Err = PushToStack (ChangeSettingsKeywords[x].Name, Value);
                    } else {
                        Err = -1;
                    }
                }
                Err = Err | ChangeSettingsKeywords[x].SetFuncPtr (OnlyCheckFlag, Param2);
                break;
            case CHANGE_SETTINGS_COMMAND_GET:
                // Remark: Param2 is than not a char* but a char**
                *((char**)(void*)Param2) = ChangeSettingsKeywords[x].GetFuncPtr();
                break;
            case CHANGE_SETTINGS_COMMAND_POP:
                if (!OnlyCheckFlag) {
                    char *Value = GetPopFromStack (ChangeSettingsKeywords[x].Name);
                    if (Value != NULL) {
                        Err = ChangeSettingsKeywords[x].SetFuncPtr (OnlyCheckFlag, Value);
                        my_free (Value);
                    } else {
                        Err = -2;  // -2 means no stack entry with this name
                    }
                }
                break;
            default:
                Err = -1;
                break;
            }
            return Err;
        }
    }
    return -1;   // unknown setting name
}




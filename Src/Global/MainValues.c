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
#include <stdlib.h>

#include "Config.h"
#include "MyMemory.h"
#include "IniDataBase.h"
#include "FileExtensions.h"
#include "IniSectionEntryDefines.h"
#include "Files.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ConfigurablePrefix.h"
#include "ThrowError.h"
#include "InitProcess.h"
#include "EnvironmentVariables.h"

#include "MainValues.h"

MAIN_INI_VAL  s_main_ini_val;             // Global configuration
int CycleCounterVid;
int DisplayUnitForNonePhysicalValues;

static char *szIntProcessnames[]  = { "Script", "Generator",
                                      "Player", "Trigger",
                                      "Recorder", NULL};


static void ReadOneConfigurablePrefix(int par_No, const char *par_Entry, const char *par_Default)
{
    if (s_main_ini_val.ConfigurablePrefix[par_No] != NULL) {
        StringFree(s_main_ini_val.ConfigurablePrefix[par_No]);
    }
    s_main_ini_val.ConfigurablePrefix[par_No] =
        IniFileDataBaseReadStringBuffer(OPT_SECTION, par_Entry, par_Default, GetMainFileDescriptor());
    // the "none" string will be replace by an empty string
    if (!strcmp(s_main_ini_val.ConfigurablePrefix[par_No], "none")) {
        s_main_ini_val.ConfigurablePrefix[par_No][0] = 0;
    }
}

static void ReadRenameProcessFromTo(int par_Fd)
{
    int x;
    char Entry[128];
    char From[MAX_PATH], To[MAX_PATH];

    for (x = 0; x < 16; x++) {
        PrintFormatToString (Entry, sizeof(Entry), "Rename_%i_Process_From", x);
        if (IniFileDataBaseReadString(OPT_SECTION, Entry, "",  From, sizeof(From), par_Fd) > 0) {
            PrintFormatToString (Entry, sizeof(Entry), "Rename_%i_Process_To", x);
            if (IniFileDataBaseReadString(OPT_SECTION, Entry, "", To, sizeof(To), par_Fd) > 0) {
                if ((s_main_ini_val.RenameProcessFromTo == NULL) || (s_main_ini_val.RenameProcessFromToPos >= s_main_ini_val.RenameProcessFromToSize)) {
                    s_main_ini_val.RenameProcessFromToSize += 8;
                    s_main_ini_val.RenameProcessFromTo = my_realloc(s_main_ini_val.RenameProcessFromTo,
                                                                    s_main_ini_val.RenameProcessFromToSize * sizeof(s_main_ini_val.RenameProcessFromTo[0]));
                }
                if (s_main_ini_val.RenameProcessFromTo != NULL) {
                    s_main_ini_val.RenameProcessFromTo[s_main_ini_val.RenameProcessFromToPos].From = StringMalloc(From);
                    s_main_ini_val.RenameProcessFromTo[s_main_ini_val.RenameProcessFromToPos].To = StringMalloc(To);
                }
                s_main_ini_val.RenameProcessFromToPos++;
            } else break;
        } else break;
    }
}

const char *RenameProcessByBasicSettingsTable(const char *par_From)
{
    int x;
    if (s_main_ini_val.RenameProcessFromTo != NULL) {
        for (x = 0; x < s_main_ini_val.RenameProcessFromToPos; x++) {
            if (!strcmp(par_From, s_main_ini_val.RenameProcessFromTo[x].From)) {
                return s_main_ini_val.RenameProcessFromTo[x].To;
            }
        }
    }
    return par_From;
}

int ReadBasicConfigurationFromIni(MAIN_INI_VAL *sp_main_ini)
{
    char  tmp_str[MAX_PATH];
    char *p;
    int   i;
    int Fd = GetMainFileDescriptor();

    // Title of the main window
    IniFileDataBaseReadString(OPT_SECTION, OPT_PROJECT_TEXT, OPT_PROJECT_DEF,
                            sp_main_ini->ProjectName,
                            sizeof (sp_main_ini->ProjectName),
                            Fd);
    StringCopyMaxCharTruncate (sp_main_ini->TempProjectName, sp_main_ini->ProjectName, sizeof(sp_main_ini->TempProjectName));

    // Work folder
    IniFileDataBaseReadString(OPT_SECTION, OPT_WORKDIR_TEXT, OPT_WORKDIR_DEF,
                            sp_main_ini->WorkDir,
                            sizeof (sp_main_ini->WorkDir),
                            Fd);

    sp_main_ini->WriteProtectIniProcessList = IniFileDataBaseReadInt("InitStartProcesses",
                                                    "WriteProtect",
                                                    0, Fd);
    sp_main_ini->SwitchAutomaticSaveIniOff = IniFileDataBaseReadInt(OPT_SECTION,
                                                    "SwitchOffAutomaticSaveIniFile",
                                                    0, Fd);
    sp_main_ini->AskSaveIniAtExit = IniFileDataBaseReadInt(OPT_SECTION,
                                                    "AskToSaveIniFileAtExit",
                                                    0, Fd);
    sp_main_ini->ShouldStopSchedulerWhileDialogOpen = IniFileDataBaseReadInt(OPT_SECTION,
                                                    "StopSchedulerWhileDialogsAreOpen",
                                                    1, Fd);
    sp_main_ini->SeparateCyclesForRefAndInitFunction = IniFileDataBaseReadInt(OPT_SECTION,
                                                    "SeparateCyclesForRefAndInitFunction",
                                                    0, Fd);
    sp_main_ini->RememberReferencedLabels = IniFileDataBaseReadInt(OPT_SECTION,
                                                    "RememberManualReferencedLabels",
                                                    1, Fd);
    sp_main_ini->NotFasterThanRealTime = IniFileDataBaseReadInt(OPT_SECTION,
                                                    "NotFasterThanRealtime",
                                                    0, Fd);
    sp_main_ini->UseRelativePaths = IniFileDataBaseReadInt(OPT_SECTION,
                                                    "UseRelativePathes",
                                                    0, Fd);
    sp_main_ini->SuppressDisplayNonExistValues = IniFileDataBaseReadInt(OPT_SECTION,
                                                    "SuppressDisplayNonExistingValues",
                                                    0, Fd);
    // Width of the main window
    if ((sp_main_ini->MainWindowWidth = IniFileDataBaseReadInt(OPT_SECTION,
                                                OPT_XDIM_TEXT,
                                                OPT_XDIM_DEF,
                                                Fd)) <= 0) {
        sp_main_ini->MainWindowWidth = OPT_XDIM_DEF;
    }

    // Height of the main window
    if ((sp_main_ini->MainWindowHeight = IniFileDataBaseReadInt(OPT_SECTION,
                                                OPT_YDIM_TEXT,
                                                OPT_YDIM_DEF,
                                                Fd)) <= 0) {
        sp_main_ini->MainWindowHeight = OPT_YDIM_DEF;
    }

    sp_main_ini->Maximized = IniFileDataBaseReadInt(OPT_SECTION,
                                                OPT_MAXIMIZED_TEXT,
                                                OPT_MAXIMIZED_DEF,
                                                Fd);

    // X position of the control panels
    sp_main_ini->ControlPanelXPos = IniFileDataBaseReadInt(OPT_SECTION,
                                                OPT_CPNL_XPOS_TEXT,
                                                OPT_CPNL_XPOS_DEF,
                                                Fd);

    // Y position of the control panels
    sp_main_ini->ControlPanelYPos = IniFileDataBaseReadInt(OPT_SECTION,
                                                OPT_CPNL_YPOS_TEXT,
                                                OPT_CPNL_YPOS_DEF,
                                                Fd);

    // Cycle for limited scheduler run
    if ((sp_main_ini->NumberOfNextCycles = IniFileDataBaseReadUInt(OPT_SECTION,
                                                    OPT_CYCLES_TEXT,
                                                    OPT_CYCLES_DEF,
                                                    Fd)) < 1)
    {
    sp_main_ini->NumberOfNextCycles = 1;
    }

    // Selected internal Prozess inside the control panel
    IniFileDataBaseReadString(OPT_SECTION, OPT_ACT_CTRL_TEXT,
                            "Script", tmp_str, sizeof (tmp_str),
                            Fd);

    for (i = 0; szIntProcessnames[i] != NULL; i++) {
        if (strcmp(tmp_str, szIntProcessnames[i]) == 0) {
            s_main_ini_val.ControlPanelSelectedInternalProcess = i;
            break;
        }
    }
    if (szIntProcessnames[i] == NULL) {
        s_main_ini_val.ControlPanelSelectedInternalProcess = 1;
    }

    // Size of blackboard
    if ((sp_main_ini->BlackboardSize =
        IniFileDataBaseReadInt( OPT_SECTION, OPT_BB_SIZE_TEXT,
                                OPT_BB_SIZE_DEF, Fd)) < 2) {
        sp_main_ini->BlackboardSize = OPT_BB_SIZE_DEF;
    }

    // Editor path
    IniFileDataBaseReadString(OPT_SECTION, OPT_ED_PATH_TEXT,
                            OPT_ED_PATH_DEF, sp_main_ini->Editor,
                            sizeof(sp_main_ini->Editor), Fd);
    IniFileDataBaseReadString(OPT_SECTION, OPT_ED_X_PATH_TEXT,
                            OPT_ED_PATH_DEF, sp_main_ini->EditorX,
                            sizeof(sp_main_ini->EditorX), Fd);

    sp_main_ini->ScriptMessageDialogVisable = (char)
        IniFileDataBaseReadInt( OPT_SECTION, OPT_SCRMSG_TEXT,
                                OPT_SCRMSG_DEF, Fd);

    // Scheduler periode (simulated time slice)
    IniFileDataBaseReadString(SCHED_SECTION, SCHED_PERIODE_TEXT,
                            SCHED_PERIODE_DEF, tmp_str, sizeof (tmp_str),
                            Fd);
    if ((sp_main_ini->SchedulerPeriode = atof (tmp_str)) <= 0.0) {
        sp_main_ini->SchedulerPeriode = atof (SCHED_PERIODE_DEF);
    }
    sp_main_ini->SyncWithFlexray = IniFileDataBaseReadYesNo ("Scheduler", "SyncWithFlexray", 0, Fd);

    // Scheduler priority
    IniFileDataBaseReadString(OPT_SECTION, "Priority",
                            "PRIORITY_IDLE", sp_main_ini->Priority, sizeof (sp_main_ini->Priority),
                            Fd);

    DisplayUnitForNonePhysicalValues =
        IniFileDataBaseReadInt(OPT_SECTION, "DisplayUnitForNonePhysicalValues",
                            0, Fd);

    sp_main_ini->StatusbarCar =
        IniFileDataBaseReadInt( OPT_SECTION, "StatusbarCar", 0, Fd);

    IniFileDataBaseReadString(OPT_SECTION, "StatusbarCarSpeed",
                            "", sp_main_ini->SpeedStatusbarCar,
                            sizeof (sp_main_ini->SpeedStatusbarCar),
                            Fd);
    sp_main_ini->NoCaseSensitiveFilters =
        IniFileDataBaseReadInt( OPT_SECTION, "NoCaseSensitiveFilters", 0, Fd);
    sp_main_ini->AsapCombatibleLabelnames =
        IniFileDataBaseReadInt( OPT_SECTION, "AsapCombatibleLabelNames", 0, Fd);

    sp_main_ini->ReplaceAllNoneAsapCombatibleChars  =
        IniFileDataBaseReadInt( OPT_SECTION, "ReplaceAllNoneAsapCombatibleChars", 0, Fd);

    IniFileDataBaseReadString (OPT_SECTION, "ReadModuleStaticLabels", "no",
                            tmp_str, sizeof(tmp_str), Fd);
    if (!strcmp (tmp_str, "yes")) {
        sp_main_ini->ViewStaticSymbols = 1;
        IniFileDataBaseReadString(OPT_SECTION, "ExtendStaticLabelsWithFilename", "no",
                                tmp_str, sizeof(tmp_str), Fd);
        if (!strcmp (tmp_str, "yes")) {
            sp_main_ini->ExtendStaticLabelsWithFilename = 1;
        } else {
            sp_main_ini->ExtendStaticLabelsWithFilename = 0;
        }
    } else {
        sp_main_ini->ViewStaticSymbols = 0;
        sp_main_ini->ExtendStaticLabelsWithFilename = 0;
    }
    IniFileDataBaseReadString (OPT_SECTION, "SaveSortedINIFiles",
                                "no", tmp_str, sizeof (tmp_str), Fd);
    if (!strcmp (tmp_str, "yes")) sp_main_ini->SaveSortedINIFiles = 1;
    else sp_main_ini->SaveSortedINIFiles = 0;
    IniFileDataBaseReadString (OPT_SECTION, "MakeBackupBeforeSaveINIFiles",
                                "yes", tmp_str, sizeof (tmp_str), Fd);
    if (!strcmp (tmp_str, "yes")) sp_main_ini->MakeBackupBeforeSaveINIFiles = 1;
    else sp_main_ini->MakeBackupBeforeSaveINIFiles = 0;

    IniFileDataBaseReadString (OPT_SECTION, "DontMakeScriptDebugFile",
                                "no", tmp_str, sizeof (tmp_str), Fd);
    if (!strcmp (tmp_str, "yes")) sp_main_ini->DontMakeScriptDebugFile = 1;
    else sp_main_ini->DontMakeScriptDebugFile = 0;

    IniFileDataBaseReadString (OPT_SECTION, "DefaultMinMaxValues",
                                "Min0Max1", tmp_str, sizeof (tmp_str), Fd);
    if (!strcmp (tmp_str, "MinMaxDatatype")) sp_main_ini->DefaultMinMaxValues = DEFAULT_MINMAX_DATATYPE;
    else if (!strcmp (tmp_str, "MinMaxDatatypeButNotMoreThan")) sp_main_ini->DefaultMinMaxValues = DEFAULT_MINMAX_DATATYPE_BUT_NOT_MORE_THAN;
    else sp_main_ini->DefaultMinMaxValues = DEFAULT_MIN_0_MAX_1;
    IniFileDataBaseReadString (OPT_SECTION, "DefaultMinMinValue",
                                "0.0", tmp_str, sizeof (tmp_str), Fd);
    sp_main_ini->MinMinValues = strtod (tmp_str, NULL);
    IniFileDataBaseReadString (OPT_SECTION, "DefaultMaxMaxValue",
                                "100000.0", tmp_str, sizeof (tmp_str), Fd);
    sp_main_ini->MaxMaxValues = strtod (tmp_str, NULL);
    if (sp_main_ini->MinMinValues <= sp_main_ini->MaxMaxValues) sp_main_ini->MaxMaxValues = sp_main_ini->MinMinValues + 1.0;

    if (IniFileDataBaseReadString (OPT_SECTION, "TerminateScript",
        "", sp_main_ini->TerminateScript,  sizeof (sp_main_ini->TerminateScript), Fd) > 0) {
        sp_main_ini->TerminateScriptFlag = 1;
    } else sp_main_ini->TerminateScriptFlag = 0;

    IniFileDataBaseReadString (OPT_SECTION, "ScriptStartExeAsIcon",
                                "no", tmp_str, sizeof (tmp_str), Fd);
    if (!strcmp (tmp_str, "yes")) sp_main_ini->ScriptStartExeAsIcon = 1;
    else sp_main_ini->ScriptStartExeAsIcon = 0;

    sp_main_ini->StopPlayerIfScriptStopped = 1;
    sp_main_ini->StopRecorderIfScriptStopped = 1;
    sp_main_ini->StopGeneratorIfScriptStopped = 1;
    sp_main_ini->StopEquationIfScriptStopped = 1;

    sp_main_ini->HideControlPanelLock = 0;
    IniFileDataBaseReadString (OPT_SECTION, "HideControlPanel",
                                "no", tmp_str, sizeof (tmp_str), Fd);
    if (!strcmp (tmp_str, "yes")) sp_main_ini->HideControlPanel = 1;
    else sp_main_ini->HideControlPanel = 0;

    sp_main_ini->DontUseInitValuesForExistingCanVariables = 0;

    IniFileDataBaseReadString (OPT_SECTION, "ScrictAtomicEndAtomicCheck",
                               "yes", tmp_str, sizeof (tmp_str), Fd);
    if (!strcmp (tmp_str, "yes")) sp_main_ini->ScrictAtomicEndAtomicCheck = 1;
    else sp_main_ini->ScrictAtomicEndAtomicCheck = 0;

    sp_main_ini->CopyBB2ProcOnlyIfWrFlagSet = (char)IniFileDataBaseReadYesNo (OPT_SECTION, "CopyBB2ProcOnlyIfWrFlagSet", 0, Fd);
    sp_main_ini->AllowBBVarsWithSameAddr = (char)IniFileDataBaseReadYesNo (OPT_SECTION, "AllowBBVarsWithSameAddr", 0, Fd);

    sp_main_ini->SaveRefListInNewFormatAsDefault = IniFileDataBaseReadYesNo (OPT_SECTION, "SaveRefListInNewFormatAsDefault", 0, Fd);
    
    sp_main_ini->UseTempIniForFunc.RefLists = (char)IniFileDataBaseReadYesNo (OPT_SECTION, "StoreReferenceListsOnlynonePersistence", 0, Fd);;

    sp_main_ini->ExternProcessLoginTimeout = IniFileDataBaseReadInt (SCHED_INI_SECTION, LI_TIMEOUT_INI_ENTRY, 60, Fd);

    IniFileDataBaseReadString (OPT_SECTION, "SelectStartupApplicationStyle",
                               "", sp_main_ini->SelectStartupAppStyle,  sizeof (sp_main_ini->SelectStartupAppStyle), Fd);

    // Text window:
    sp_main_ini->TextDefaultShowUnitColumn = IniFileDataBaseReadYesNo (OPT_SECTION, "TextDefaultShowUnitColumn", 1, Fd);
    sp_main_ini->TextDefaultShowDispayTypeColumn = IniFileDataBaseReadYesNo (OPT_SECTION, "TextDefaultShowDispayTypeColumn", 1, Fd);
    MEMSET (sp_main_ini->TextDefaultFont, 0, sizeof (sp_main_ini->TextDefaultFont));
    sp_main_ini->TextDefaultFontSize = 0;
    if (IniFileDataBaseReadString(OPT_SECTION, "TextDefaultFont", "", tmp_str, sizeof (tmp_str), Fd) > 0) {
        char *Name, *Size;
        if (StringCommaSeparate (tmp_str, &Name, &Size, NULL) == 2) {
            strncpy (sp_main_ini->TextDefaultFont, Name, sizeof (sp_main_ini->TextDefaultFont)-1);
            sp_main_ini->TextDefaultFontSize = atol(Size);
            if (sp_main_ini->TextDefaultFontSize < 6) sp_main_ini->TextDefaultFontSize = 6;
            if (sp_main_ini->TextDefaultFontSize > 100) sp_main_ini->TextDefaultFontSize = 100;
        }
    }
    // oscilloscope window:
    sp_main_ini->OscilloscopeDefaultBufferDepth = IniFileDataBaseReadInt (OPT_SECTION, "OscilloscopeDefaultBufferDepth", 131072, Fd);
    MEMSET (sp_main_ini->OscilloscopeDefaultFont, 0, sizeof (sp_main_ini->OscilloscopeDefaultFont));
    sp_main_ini->OscilloscopeDefaultFontSize = 0;
    if (IniFileDataBaseReadString(OPT_SECTION, "OscilloscopeDefaultFont", "", tmp_str, sizeof (tmp_str), Fd) > 0) {
        char *Name, *Size;
        if (StringCommaSeparate (tmp_str, &Name, &Size, NULL) == 2) {
            strncpy (sp_main_ini->OscilloscopeDefaultFont, Name, sizeof (sp_main_ini->OscilloscopeDefaultFont)-1);
            sp_main_ini->OscilloscopeDefaultFontSize = atol(Size);
            if (sp_main_ini->OscilloscopeDefaultFontSize < 6) sp_main_ini->OscilloscopeDefaultFontSize = 6;
            if (sp_main_ini->OscilloscopeDefaultFontSize > 100) sp_main_ini->OscilloscopeDefaultFontSize = 100;
        }
    }

    IniFileDataBaseReadString (OPT_SECTION, "Version", "-1", tmp_str, sizeof (tmp_str), Fd);

    sp_main_ini->VersionIniFileWasWritten = strtol(tmp_str, &p, 0);
    while (isspace(*p)) p++;
    if (!strncmp("(patch-", p, 7)) {
        sp_main_ini->PatchNumberIniFileWasWritten = (strtol(p+7, &p, 0));
    } else if (!strncmp("(pre-", p, 5)) {
        sp_main_ini->PatchNumberIniFileWasWritten = (strtol(p+5, &p, 0));
    } else {
        sp_main_ini->PatchNumberIniFileWasWritten = 0;
    }
    sp_main_ini->ShouldConnectToRemoteMaster =
    sp_main_ini->ConnectToRemoteMaster = IniFileDataBaseReadYesNo (OPT_SECTION, "ConnectToRemoteMaster", 0, Fd);
    IniFileDataBaseReadString (OPT_SECTION, "RemoteMasterName", "192.168.0.3", sp_main_ini->RemoteMasterName, sizeof(sp_main_ini->RemoteMasterName), Fd);
    sp_main_ini->RemoteMasterPort = IniFileDataBaseReadInt (OPT_SECTION, "RemoteMasterPort", 8888U, Fd);
    sp_main_ini->CopyAndStartRemoteMaster = IniFileDataBaseReadYesNo (OPT_SECTION, "CopyAndStartRemoteMaster", 1, Fd);
    IniFileDataBaseReadString (OPT_SECTION, "RemoteMasterExecutable", "", sp_main_ini->RemoteMasterExecutable, sizeof(sp_main_ini->RemoteMasterExecutable), Fd);
    sp_main_ini->RemoteMasterExecutableResoved = NULL;
    IniFileDataBaseReadString (OPT_SECTION, "RemoteMasterCopyTo", "/tmp/LinuxRemoteMaster.out", sp_main_ini->RemoteMasterCopyTo, sizeof(sp_main_ini->RemoteMasterCopyTo), Fd);

    IniFileDataBaseReadString (OPT_SECTION, "RpcOverSocketOrNamedPipe",
                                "NamedPipe", tmp_str, sizeof (tmp_str), Fd);
    if (!_strcmpi (tmp_str, "Socket")) sp_main_ini->RpcOverSocketOrNamedPipe = 1;
    else sp_main_ini->RpcOverSocketOrNamedPipe = 0;
    sp_main_ini->RpcDebugLoggingFlags = IniFileDataBaseReadInt (OPT_SECTION, "RpcDebugLoggingFlags", 0, Fd);
    sp_main_ini->RpcSocketPort = IniFileDataBaseReadInt (OPT_SECTION, "RpcSocketPort", 1810, Fd);

#ifdef _WIN32
    {
        BOOL Flag;
        if (IsProcessInJob(GetCurrentProcess(), NULL, &Flag)) {
            sp_main_ini->RunningInsideAJob = Flag;
        } else {
            sp_main_ini->RunningInsideAJob = 0;
        }
    }
#else
    sp_main_ini->RunningInsideAJob = 0; // Jobs gibt es nur unter Windows
#endif

     if (sp_main_ini->ShouldStopSchedulerWhileDialogOpen &&
         !sp_main_ini->ShouldConnectToRemoteMaster) {
         sp_main_ini->StopSchedulerWhileDialogOpen = 1;
     } else  {
         sp_main_ini->StopSchedulerWhileDialogOpen = 0;
     }

    s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVL =
    s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVLIni = IniFileDataBaseReadYesNo (OPT_SECTION, "StopScriptIfLabelDosNotExistInsideSVL", 0, Fd);

    s_main_ini_val.ExternProcessLoginSocketPortIni = IniFileDataBaseReadUInt (OPT_SECTION, "ExternProcessLoginSocketPort", 0, Fd);

    s_main_ini_val.TerminateSchedulerExternProcessTimeout = IniFileDataBaseReadUInt (OPT_SECTION, "TerminateSchedulerExternProcessTimeout", 10, Fd);

    s_main_ini_val.WriteMemoryLeakFile = IniFileDataBaseReadYesNo(OPT_SECTION, "WriteMemoryLeakFile", 0, Fd);

    s_main_ini_val.ShouldUseDarkModeIni = IniFileDataBaseReadYesNo(OPT_SECTION, "DarkMode", 0, Fd);
    if (!s_main_ini_val.DarkModeWasSetByUser) {  // Dark mode was not set before?
        s_main_ini_val.DarkMode = s_main_ini_val.ShouldUseDarkModeIni;
    }
    if (s_main_ini_val.RenameExecutableActive) {
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME, "PrefixTypeProgramName", DEFAULT_PREFIX_TYPE_PROGRAM_NAME);

        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD, "PrefixTypeShortBlackboard", DEFAULT_PREFIX_TYPE_SHORT_BLACKBOARD);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, "PrefixTypeLongBlackboard", DEFAULT_PREFIX_TYPE_LONG_BLACKBOARD);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD, "PrefixTypeLong2Blackboard", DEFAULT_PREFIX_TYPE_LONG2_BLACKBOARD);

        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_ERROR_FILE, "PrefixTypeErrorFile", DEFAULT_PREFIX_TYPE_ERROR_FILE);

        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES, "PrefixTypeCANNames", DEFAULT_PREFIX_TYPE_CAN_NAMES);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_FLEXRAY_NAMES, "PrefixTypeFlexrayNames", DEFAULT_PREFIX_TYPE_FLEXRAY_NAMES);

        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CYCLE_COUNTER, "PrefixTypeCycleCounter", DEFAULT_PREFIX_TYPE_CYCLE_COUNTER);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME, "PrefixTypeSampleTime", DEFAULT_PREFIX_TYPE_SAMPLE_TIME);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_FREQUENCY, "PrefixTypeSampleFrequency", DEFAULT_PREFIX_TYPE_SAMPLE_FREQUENCY);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_OWN_EXIT_CODE, "PrefixTypeOwnExitCode", DEFAULT_PREFIX_TYPE_OWN_EXIT_CODE);

        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SCRIPT, "PrefixTypeScript", DEFAULT_PREFIX_TYPE_SCRIPT);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_GENERATOR, "PrefixTypeGenerator", DEFAULT_PREFIX_TYPE_GENERATOR);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_TRACE_RECORDER, "PrefixTypeTraceRecorder", DEFAULT_PREFIX_TYPE_TRACE_RECORDER);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_STIMULUS_PLAYER, "PrefixTypeStimulusPlayer", DEFAULT_PREFIX_TYPE_STIMULUS_PLAYER);
        ReadOneConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_EQUATION_CALCULATOR, "PrefixTypeEquationCalculator", DEFAULT_PREFIX_TYPE_EQUATION_CALCULATOR);

        ReadRenameProcessFromTo(Fd);
    }
    return 0;
}

int WriteBasicConfigurationToIni(MAIN_INI_VAL *sp_main_ini)
{
    char tmp_str[MAX_PATH];
    int Fd = GetMainFileDescriptor();

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->WriteProtectIniProcessList);
    if (IniFileDataBaseWriteString("InitStartProcesses", "WriteProtect",
                                 tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->SwitchAutomaticSaveIniOff);
    if (IniFileDataBaseWriteString(OPT_SECTION, "SwitchOffAutomaticSaveIniFile",
                                 tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->AskSaveIniAtExit);
    if (IniFileDataBaseWriteString(OPT_SECTION, "AskToSaveIniFileAtExit",
                                 tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->ShouldStopSchedulerWhileDialogOpen);
    if (IniFileDataBaseWriteString(OPT_SECTION, "StopSchedulerWhileDialogsAreOpen",
                                   tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->SeparateCyclesForRefAndInitFunction);
    if (IniFileDataBaseWriteString(OPT_SECTION, "SeparateCyclesForRefAndInitFunction",
                                   tmp_str, Fd) == 0)
    {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->RememberReferencedLabels);
    if (IniFileDataBaseWriteString(OPT_SECTION, "RememberManualReferencedLabels",
                                 tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->NotFasterThanRealTime);
    if (IniFileDataBaseWriteString(OPT_SECTION, "NotFasterThanRealtime",
                                 tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->UseRelativePaths);
    if (IniFileDataBaseWriteString(OPT_SECTION, "UseRelativePathes",
                                 tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->SuppressDisplayNonExistValues);
    if (IniFileDataBaseWriteString(OPT_SECTION, "SuppressDisplayNonExistingValues",
                                 tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_PROJECT_TEXT,
                                sp_main_ini->ProjectName, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_WORKDIR_TEXT,
                                sp_main_ini->WorkDir, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->MainWindowWidth);
    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_XDIM_TEXT,
                                tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->MainWindowHeight);
    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_YDIM_TEXT,
                                tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->Maximized);
    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_MAXIMIZED_TEXT,
                                tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->ControlPanelXPos);
    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_CPNL_XPOS_TEXT,
                                tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->ControlPanelYPos);
    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_CPNL_YPOS_TEXT,
                                tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->NumberOfNextCycles);
    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_CYCLES_TEXT,
                                tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_ACT_CTRL_TEXT,
                        szIntProcessnames[sp_main_ini->ControlPanelSelectedInternalProcess],
                        Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%d", sp_main_ini->BlackboardSize);
    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_BB_SIZE_TEXT,
                                tmp_str, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_ED_PATH_TEXT,
                                sp_main_ini->Editor, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_ED_X_PATH_TEXT,
                                sp_main_ini->EditorX, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    if (IniFileDataBaseWriteString(OPT_SECTION, OPT_SCRMSG_TEXT,
                                sp_main_ini->ScriptMessageDialogVisable ? "1" : "0",
                                Fd) == 0) {
        return INI_WRITE_ERROR;
    }


    if (IniFileDataBaseWriteFloat(SCHED_SECTION, SCHED_PERIODE_TEXT,
                                sp_main_ini->SchedulerPeriode, Fd) == 0) {
        return INI_WRITE_ERROR;
    }

    IniFileDataBaseWriteYesNo ("Scheduler", "SyncWithFlexray", sp_main_ini->SyncWithFlexray , Fd);

    IniFileDataBaseWriteString(OPT_SECTION, "Priority",
                              sp_main_ini->Priority, Fd);

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%i", DisplayUnitForNonePhysicalValues);
    IniFileDataBaseWriteString(OPT_SECTION, "DisplayUnitForNonePhysicalValues",
                              tmp_str, Fd);

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%i", sp_main_ini->StatusbarCar);
    IniFileDataBaseWriteString(OPT_SECTION, "StatusbarCar",
                              tmp_str, Fd);
    IniFileDataBaseWriteString(OPT_SECTION, "StatusbarCarSpeed",
                              sp_main_ini->SpeedStatusbarCar, Fd);
    PrintFormatToString (tmp_str, sizeof(tmp_str), "%i", sp_main_ini->NoCaseSensitiveFilters);
    IniFileDataBaseWriteString(OPT_SECTION, "NoCaseSensitiveFilters",
                              tmp_str, Fd);
    PrintFormatToString (tmp_str, sizeof(tmp_str), "%i", sp_main_ini->AsapCombatibleLabelnames);
    IniFileDataBaseWriteString(OPT_SECTION, "AsapCombatibleLabelNames",
                              tmp_str, Fd);

    PrintFormatToString (tmp_str, sizeof(tmp_str), "%i", sp_main_ini->ReplaceAllNoneAsapCombatibleChars);
    IniFileDataBaseWriteString(OPT_SECTION, "ReplaceAllNoneAsapCombatibleChars",
                              tmp_str, Fd);

    if (sp_main_ini->ViewStaticSymbols) {
        StringCopyMaxCharTruncate (tmp_str, "yes", sizeof(tmp_str));
    } else {
        StringCopyMaxCharTruncate (tmp_str, "no", sizeof(tmp_str));
    }
    IniFileDataBaseWriteString(OPT_SECTION, "ReadModuleStaticLabels",
                              tmp_str, Fd);
    if (sp_main_ini->ExtendStaticLabelsWithFilename) {
        StringCopyMaxCharTruncate (tmp_str, "yes", sizeof(tmp_str));
    } else {
        StringCopyMaxCharTruncate (tmp_str, "no", sizeof(tmp_str));
    }
    IniFileDataBaseWriteString(OPT_SECTION, "ExtendStaticLabelsWithFilename",
                              tmp_str, Fd);

    if (sp_main_ini->SaveSortedINIFiles) {
        StringCopyMaxCharTruncate (tmp_str, "yes", sizeof(tmp_str));
    } else {
        StringCopyMaxCharTruncate (tmp_str, "no", sizeof(tmp_str));
    }
    IniFileDataBaseWriteString(OPT_SECTION, "SaveSortedINIFiles",
                              tmp_str, Fd);

    if (sp_main_ini->MakeBackupBeforeSaveINIFiles) {
        StringCopyMaxCharTruncate (tmp_str, "yes", sizeof(tmp_str));
    } else {
        StringCopyMaxCharTruncate (tmp_str, "no", sizeof(tmp_str));
    }
    IniFileDataBaseWriteString(OPT_SECTION, "MakeBackupBeforeSaveINIFiles",
                              tmp_str, Fd);

    if (sp_main_ini->DontMakeScriptDebugFile) {
        StringCopyMaxCharTruncate (tmp_str, "yes", sizeof(tmp_str));
    } else {
        StringCopyMaxCharTruncate (tmp_str, "no", sizeof(tmp_str));
    }
    IniFileDataBaseWriteString(OPT_SECTION, "DontMakeScriptDebugFile",
                              tmp_str, Fd);


    switch (sp_main_ini->DefaultMinMaxValues) {
    case DEFAULT_MINMAX_DATATYPE:
        StringCopyMaxCharTruncate (tmp_str, "MinMaxDatatype", sizeof(tmp_str));
        break;
    case DEFAULT_MINMAX_DATATYPE_BUT_NOT_MORE_THAN:
        StringCopyMaxCharTruncate (tmp_str, "MinMaxDatatypeButNotMoreThan", sizeof(tmp_str));
        break;
    case DEFAULT_MIN_0_MAX_1:
    default:
        StringCopyMaxCharTruncate (tmp_str, "Min0Max1", sizeof(tmp_str));
        break;
    }
    IniFileDataBaseWriteString (OPT_SECTION, "DefaultMinMaxValues", tmp_str, Fd);
    PrintFormatToString (tmp_str, sizeof(tmp_str), "%g", sp_main_ini->MinMinValues);
    IniFileDataBaseWriteString (OPT_SECTION, "DefaultMinMinValue", tmp_str, Fd);
    PrintFormatToString (tmp_str, sizeof(tmp_str), "%g", sp_main_ini->MaxMaxValues);
    IniFileDataBaseWriteString (OPT_SECTION, "DefaultMaxMaxValue", tmp_str, Fd);
    IniFileDataBaseWriteString (OPT_SECTION, "TerminateScript",
                               sp_main_ini->TerminateScript, Fd);

    if (sp_main_ini->ScriptStartExeAsIcon) {
        StringCopyMaxCharTruncate (tmp_str, "yes", sizeof(tmp_str));
    } else {
        StringCopyMaxCharTruncate (tmp_str, "no", sizeof(tmp_str));
    }
    IniFileDataBaseWriteString(OPT_SECTION, "ScriptStartExeAsIcon",
                              tmp_str, Fd);

    if (s_main_ini_val.HideControlPanel) {
        IniFileDataBaseWriteString (OPT_SECTION, "HideControlPanel",
                                     "yes", Fd);
    } else {
        IniFileDataBaseWriteString (OPT_SECTION, "HideControlPanel",
                                     "no", Fd);
    }

    if (sp_main_ini->ScrictAtomicEndAtomicCheck) {
        StringCopyMaxCharTruncate (tmp_str, "yes", sizeof(tmp_str));
    } else {
        StringCopyMaxCharTruncate (tmp_str, "no", sizeof(tmp_str));
    }
    IniFileDataBaseWriteString(OPT_SECTION, "ScrictAtomicEndAtomicCheck",
                              tmp_str, Fd);

    IniFileDataBaseWriteYesNo (OPT_SECTION, "StoreReferenceListsOnlynonePersistence", sp_main_ini->UseTempIniForFunc.RefLists, Fd);

    IniFileDataBaseWriteYesNo (OPT_SECTION, "SaveRefListInNewFormatAsDefault", sp_main_ini->SaveRefListInNewFormatAsDefault, Fd);

    IniFileDataBaseWriteInt (SCHED_INI_SECTION, LI_TIMEOUT_INI_ENTRY, sp_main_ini->ExternProcessLoginTimeout, Fd);

    IniFileDataBaseWriteString (OPT_SECTION, "SelectStartupApplicationStyle", sp_main_ini->SelectStartupAppStyle,  Fd);

    // Text window
    IniFileDataBaseWriteYesNo (OPT_SECTION, "TextDefaultShowUnitColumn", sp_main_ini->TextDefaultShowUnitColumn, Fd);
    IniFileDataBaseWriteYesNo (OPT_SECTION, "TextDefaultShowDispayTypeColumn", sp_main_ini->TextDefaultShowDispayTypeColumn, Fd);
    if (strlen(sp_main_ini->TextDefaultFont)) {
        char Help[16];
        PrintFormatToString (Help, sizeof(Help), "%i", sp_main_ini->TextDefaultFontSize);
        StringCopyMaxCharTruncate(tmp_str, sp_main_ini->TextDefaultFont, sizeof(tmp_str));
        STRING_APPEND_TO_ARRAY(tmp_str, ", ");
        STRING_APPEND_TO_ARRAY(tmp_str, Help);
        IniFileDataBaseWriteString (OPT_SECTION, "TextDefaultFont", tmp_str, Fd);
    } else {
        IniFileDataBaseWriteString (OPT_SECTION, "TextDefaultFont", NULL, Fd);
    }
    // oscilloscope window
    IniFileDataBaseWriteInt (OPT_SECTION, "OscilloscopeDefaultBufferDepth", sp_main_ini->OscilloscopeDefaultBufferDepth, Fd);
    if (strlen(sp_main_ini->OscilloscopeDefaultFont)) {
        char Help[16];
        PrintFormatToString (Help, sizeof(Help), "%i", sp_main_ini->OscilloscopeDefaultFontSize);
        StringCopyMaxCharTruncate(tmp_str, sp_main_ini->OscilloscopeDefaultFont, sizeof(tmp_str));
        STRING_APPEND_TO_ARRAY(tmp_str, ", ");
        STRING_APPEND_TO_ARRAY(tmp_str, Help);
        IniFileDataBaseWriteString (OPT_SECTION, "OscilloscopeDefaultFont", tmp_str, Fd);
    } else {
        IniFileDataBaseWriteString (OPT_SECTION, "OscilloscopeDefaultFont", NULL, Fd);
    }

    if (s_main_ini_val.ShouldConnectToRemoteMaster) {
        IniFileDataBaseWriteYesNo (OPT_SECTION, "ConnectToRemoteMaster", 1, Fd);
        IniFileDataBaseWriteString (OPT_SECTION, "RemoteMasterName", s_main_ini_val.RemoteMasterName, Fd);
        IniFileDataBaseWriteInt (OPT_SECTION, "RemoteMasterPort", s_main_ini_val.RemoteMasterPort, Fd);
        IniFileDataBaseWriteYesNo (OPT_SECTION, "CopyAndStartRemoteMaster", s_main_ini_val.CopyAndStartRemoteMaster, Fd);
        IniFileDataBaseWriteString (OPT_SECTION, "RemoteMasterExecutable", s_main_ini_val.RemoteMasterExecutable, Fd);
        IniFileDataBaseWriteString (OPT_SECTION, "RemoteMasterCopyTo", s_main_ini_val.RemoteMasterCopyTo, Fd);
    } else {
        IniFileDataBaseWriteString (OPT_SECTION, "ConnectToRemoteMaster", NULL, Fd);
        IniFileDataBaseWriteString (OPT_SECTION, "RemoteMasterName", NULL, Fd);
        IniFileDataBaseWriteString (OPT_SECTION, "RemoteMasterPort", NULL, Fd);
        IniFileDataBaseWriteString (OPT_SECTION, "CopyAndStartRemoteMaster", NULL, Fd);
        IniFileDataBaseWriteString (OPT_SECTION, "RemoteMasterExecutable", NULL, Fd);
        IniFileDataBaseWriteString (OPT_SECTION, "RemoteMasterCopyTo", NULL, Fd);
    }
    IniFileDataBaseWriteString (OPT_SECTION, "RpcOverSocketOrNamedPipe",
                               (sp_main_ini->RpcOverSocketOrNamedPipe != 0) ? "Socket" : "NamedPipe", Fd);

    IniFileDataBaseWriteString (OPT_SECTION, "RpcOverSocketOrNamedPipe",
                               (sp_main_ini->RpcOverSocketOrNamedPipe != 0) ? "Socket" : "NamedPipe", Fd);
    IniFileDataBaseWriteInt (OPT_SECTION, "RpcDebugLoggingFlags", s_main_ini_val.RpcDebugLoggingFlags, Fd);
    IniFileDataBaseWriteInt (OPT_SECTION, "RpcSocketPort", s_main_ini_val.RpcSocketPort, Fd);

    IniFileDataBaseWriteYesNo (OPT_SECTION, "StopScriptIfLabelDosNotExistInsideSVL", s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVLIni, Fd);

    IniFileDataBaseWriteUInt (OPT_SECTION, "ExternProcessLoginSocketPort", s_main_ini_val.ExternProcessLoginSocketPortIni, Fd);

    IniFileDataBaseWriteYesNo(OPT_SECTION, "DarkMode", s_main_ini_val.ShouldUseDarkModeIni, Fd);

    return 0;
}


double GetSchedulingPeriode (void)
{
    return s_main_ini_val.SchedulerPeriode;
}


int SetProjectName (char *Name)
{
     strncpy (s_main_ini_val.TempProjectName, Name, sizeof (s_main_ini_val.TempProjectName));
     s_main_ini_val.TempProjectName[sizeof (s_main_ini_val.TempProjectName) - 1] = 0;
     //UpdateMainWindowHeadText ();
     return 0;
}

char *GetRemoteMasterExecutable(void)
{
    if (GetRemoteMasterExecutableCmdLine() != NULL) {
        return GetRemoteMasterExecutableCmdLine();
    } else {
        if (s_main_ini_val.RemoteMasterExecutableResoved == NULL) {
            char Help[1024];
            SearchAndReplaceEnvironmentStrings(s_main_ini_val.RemoteMasterExecutable, Help, sizeof(Help));
            s_main_ini_val.RemoteMasterExecutableResoved = StringMalloc(Help);
            if (s_main_ini_val.RemoteMasterExecutableResoved == NULL) {
                ThrowError(1, "cannot get remote master executable");
            }
        }
        if (s_main_ini_val.RemoteMasterExecutableResoved != NULL) {
            return s_main_ini_val.RemoteMasterExecutableResoved;
        } else {
            return s_main_ini_val.RemoteMasterExecutable;
        }
    }
}

void SetExternProcessLoginSocketPort(const char *PortString)
{
    s_main_ini_val.ExternProcessLoginSocketPort = strtoul(PortString, NULL, 0);
}

unsigned int GetExternProcessLoginSocketPort(void)
{
    if (s_main_ini_val.ExternProcessLoginSocketPort > 0) return s_main_ini_val.ExternProcessLoginSocketPort;
    else return s_main_ini_val.ExternProcessLoginSocketPortIni;
}

int IsExternProcessLoginSocketPortConfigured(void)
{
    if ((s_main_ini_val.ExternProcessLoginSocketPort > 0) ||
            (s_main_ini_val.ExternProcessLoginSocketPortIni > 0)) {
        return 1;
    } else return 0;
}

void InitMainSettings(void)
{
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME] = StringMalloc(DEFAULT_PREFIX_TYPE_PROGRAM_NAME);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD] = StringMalloc(DEFAULT_PREFIX_TYPE_SHORT_BLACKBOARD);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD] = StringMalloc(DEFAULT_PREFIX_TYPE_LONG_BLACKBOARD);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD] = StringMalloc(DEFAULT_PREFIX_TYPE_LONG2_BLACKBOARD);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_ERROR_FILE] = StringMalloc(DEFAULT_PREFIX_TYPE_ERROR_FILE);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_CAN_NAMES] = StringMalloc(DEFAULT_PREFIX_TYPE_CAN_NAMES);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_FLEXRAY_NAMES] = StringMalloc(DEFAULT_PREFIX_TYPE_FLEXRAY_NAMES);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_CYCLE_COUNTER] = StringMalloc(DEFAULT_PREFIX_TYPE_CYCLE_COUNTER);

    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME] = StringMalloc(DEFAULT_PREFIX_TYPE_SAMPLE_TIME);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_SAMPLE_FREQUENCY] = StringMalloc(DEFAULT_PREFIX_TYPE_SAMPLE_FREQUENCY);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_OWN_EXIT_CODE] = StringMalloc(DEFAULT_PREFIX_TYPE_OWN_EXIT_CODE);

    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_SCRIPT] = StringMalloc(DEFAULT_PREFIX_TYPE_SCRIPT);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_GENERATOR] = StringMalloc(DEFAULT_PREFIX_TYPE_GENERATOR);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_TRACE_RECORDER] = StringMalloc(DEFAULT_PREFIX_TYPE_TRACE_RECORDER);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_STIMULUS_PLAYER] = StringMalloc(DEFAULT_PREFIX_TYPE_STIMULUS_PLAYER);
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_EQUATION_CALCULATOR] = StringMalloc(DEFAULT_PREFIX_TYPE_EQUATION_CALCULATOR);
}

void SetProgramNameIntoMainSettings(const char *par_ProgramName)
{
    if (s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME] != NULL) {
        StringFree(s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME]);
    }
    s_main_ini_val.ConfigurablePrefix[CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME] = StringMalloc(par_ProgramName);
    s_main_ini_val.RenameExecutableActive = 1;
}

void SetIniFilterProgram(const char *par_FilterProgramName, int par_Flags)
{
    s_main_ini_val.IniFilterProgram = StringMalloc(par_FilterProgramName);
    s_main_ini_val.IniFilterProgramFlags = par_Flags;
}


void SetDarkModeIntoMainSettings(int par_DarkMode)
{
    if (par_DarkMode < 0)  {
        s_main_ini_val.DarkMode = 0;
        s_main_ini_val.DarkModeWasSetByUser = 0;
    } else {
        s_main_ini_val.DarkMode = par_DarkMode;
        s_main_ini_val.DarkModeWasSetByUser = 1;
    }
}

void SetEnableLegacyEnvironmentVariables(void)
{
    s_main_ini_val.EnableLegacyEnvironmentVariables = 1;
}

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
#ifndef _WIN32
//#include <dlfcn.h>
#include <sys/auxv.h>
#endif
#include <string.h>

#include "Config.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#include "ThrowError.h"
#include "ConfigurablePrefix.h"
#include "MyMemory.h"
#include "Files.h"
#include "IniDataBase.h"
#include "IniFileDontExist.h"
#include "MainValues.h"
#include "FileExtensions.h"
#include "InitProcess.h"
#include "ScriptMessageFile.h"
#include "DebugInfos.h"
#include "ExtProcessReferences.h"
#include "ExtProcessRefFilter.h"
#include "EquationList.h"
#include "RpcSocketServer.h"
#include "UniqueNumber.h"
#include "ParseCommandLine.h"
#include "OscilloscopeCyclic.h"
#include "DebugInfoDB.h"
#include "KillAllExternProcesses.h"
#include "MessageWindow.h"
#include "ErrorDialog.h"
#include "Fifos.h"
#include "CanFifo.h"
#include "VirtualCanDriver.h"
#include "RemoteMasterNet.h"
#include "RemoteMasterBlackboard.h"
#include "ErrorDialog.h"
#include "ExtProcessRefFilter.h"
#include "A2LLink.h"
#include "A2LLinkThread.h"
#include "DelayedFetchData.h"
#include "EnvironmentVariables.h"
#include "RemoteMasterOther.h"
#include "MainWinowSyncWithOtherThreads.h"
#include "WindowIniHelper.h"
#include "CheckIfAlreadyRunning.h"
#include "RpcControlProcess.h"
#include "VirtualNetwork.h"
#ifndef NO_GUI
#include "BlackboardObserver.h"
#endif
#ifdef _WIN32
#include "PipeMessages.h"
#endif
#include "StartupInit.h"

#define UNUSED(x) (void)(x)

static int status_vars[6];
static PID init_pid;

int StartupInit (void * par_Application)
{
    char IniFile[MAX_PATH];
    char InstanceName[MAX_PATH];
    int Fd;
    int EntryIdx;
    int typeIndex;
    char VarName[INI_MAX_ENTRYNAME_LENGTH];
    char txt[512*100];
    double tmpDouble;
    VID bbVarID;
    int RealtimeSchedulerHelperPid = -1;

    // This must be done first. some init function may rise errors
    InitPipeSchedulerCriticalSections ();

    InitErrorDialog();

    InitMessageWindowCriticalSections ();

    InitUniqueNumbers ();

    InitDebugInfosCriticalSections ();

    InitOsciCycleCriticalSection ();

    InitCANFifoCriticalSection();

    VirtualNetworkInit();

    InitInitCriticalSection ();

    InitRPCControlWaitStruct();

    InitFetchData();
    InitA2lLinkThread();

    if (init_memory ()) {
        return -1;
    }

    InitScriptMessageCriticalSection ();

    InitProcessRefAddrCriticalSection ();

    InitEnvironmentVariables();

    InitExternProcessReferenceFilters();

    A2LLinkInitCriticalSection();

    InitMainSettings();

    if (init_files()) {
        return -1;
    }
    if (IniFileDataBaseInit()) {
        return -1;
    }

    InitEquationList ();

    // check the command line parameters
    if (ParseCommandLine (IniFile, sizeof(IniFile), InstanceName, sizeof(InstanceName), par_Application) != 0) {
        return -1;
    }

    // Check if a XilEnv with this instance is running
    if (CheckIfAlreadyRunning (InstanceName, par_Application)) {
        return -1;
    }

    Fd = IniFileDataBaseOpen(IniFile);
    if (Fd < 0) {
        ThrowError (1, "cannot read INI-File %s to DB", IniFile);
        return -1;
    }
    SetMainFileDescriptor(Fd);
    AddIniFileToHistory(IniFile);

    if (IniFileDataBaseReadInt("BasicSettings", "Convert Error to Message", 0, Fd)) {
        SetError2Message (1);
    }

    // Read the INI basic settings
    if (ReadBasicConfigurationFromIni(&s_main_ini_val) != 0) {
        ThrowError(ERROR_NO_CRITICAL_STOP, "Error while reading INI-File %s (no basic settings)!!!", Fd);
        return -1;
    }
    strcpy (s_main_ini_val.InstanceName, InstanceName);
    s_main_ini_val.QtApplicationPointer = par_Application;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        uint32_t Version;
        int32_t PatchVersion;
        if (InitRemoteMaster(s_main_ini_val.RemoteMasterName, s_main_ini_val.RemoteMasterPort) != 0) {
            return -1;
        }

        RealtimeSchedulerHelperPid = rm_Init (512 * 1024 * 1024, 0, &Version, &PatchVersion);
        if ((Version != XILENV_VERSION) || (PatchVersion != XILENV_MINOR_VERSION)) {
#ifdef _WIN32
            char Path[MAX_PATH];
            GetModuleFileName (GetModuleHandle(NULL), Path, sizeof (Path));
#else
            char *Path;
            /*void *handle;
            handle = dlopen(NULL);
            if (handle != NULL) {

            }*/
            Path = (char*)getauxval(AT_EXECFN);
            if (Path == NULL) Path = "unknown";
#endif
            if (ThrowError(ERROR_OKCANCEL, "Version conflict: Version of remote master executable is %u (%i) and version of %s is %i (%i)\n"
                                      "Remote executable \"%s\"\n"
                                      "XilEnv executable \"%s\"\n"
                                      "Press OK to terminate XilEnv\n or Cancle to continue, if you know what you are doing",
                      Version, PatchVersion,
                      GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME),
                      XILENV_VERSION, XILENV_MINOR_VERSION, s_main_ini_val.RemoteMasterExecutable, Path) == IDOK) {
                rm_Kill();
                return -1;
            }
        }
    }

    InitFiFos ();

    if (s_main_ini_val.WorkDir[0] != '\0') {
        char TempString[MAX_PATH];
        // replace environment variablen inside the working directory
        SearchAndReplaceEnvironmentStrings (s_main_ini_val.WorkDir,
                                            TempString,
                                            sizeof(s_main_ini_val.WorkDir));

        if (!SetCurrentDirectory (TempString)) {
            char txt[2*MAX_PATH+50];
            strcpy (txt, "work directory %s not exist, use ");
            GetCurrentDirectory ((DWORD)(sizeof (txt) - strlen (txt)), txt + strlen (txt));
            ThrowError(ERROR_NO_CRITICAL_STOP, txt, TempString);
        }
    } else {
        // If there is no working directory defined use the current one
        strcpy (s_main_ini_val.WorkDir, ".");
    }

    // Store the  working directory directly after starting XilEnv
    // Some script commands can change the working directory
    GetCurrentDirectory (sizeof (s_main_ini_val.StartDirectory), s_main_ini_val.StartDirectory);

    if (init_blackboard(s_main_ini_val.BlackboardSize, s_main_ini_val.CopyBB2ProcOnlyIfWrFlagSet, s_main_ini_val.AllowBBVarsWithSameAddr, (char)GetError2MessageFlag ()) != 0) {
        ThrowError(ERROR_NO_CRITICAL_STOP,
              "XilEnv: Can't initialize Blackboard !!!");
        return -1;
    }

    if (s_main_ini_val.ConnectToRemoteMaster) {
        uint64_t Mask;
        get_bb_accessmask (RealtimeSchedulerHelperPid, &Mask, "");
    }

    InitKillAllExternProcesses (InstanceName);
    // If there are not terminated extern process terminte them now
    KillAllExternProcesses ();

    InitPipeSchedulers (InstanceName);

    // Add user defined blackboard variables
    EntryIdx = 0;
    while ((EntryIdx = IniFileDataBaseGetNextEntryName(EntryIdx, "UserDefinedBBVariables", VarName, sizeof(VarName), Fd)) >= 0)
    {
        char *type, *value, *unit;
        IniFileDataBaseReadString("UserDefinedBBVariables", VarName, "", txt, sizeof (txt), Fd);
        StringCommaSeparate (txt, &type, &value, &unit, NULL);
        tmpDouble = strtod(value,NULL);
        typeIndex = atoi(type);
        switch (typeIndex) {
        case 0:
            bbVarID = add_bbvari(VarName, BB_BYTE, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 1:
            bbVarID = add_bbvari(VarName, BB_UBYTE, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 2:
            bbVarID = add_bbvari(VarName, BB_WORD, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 3:
            bbVarID = add_bbvari(VarName, BB_UWORD, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 4:
            bbVarID = add_bbvari(VarName, BB_DWORD, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 5:
            bbVarID = add_bbvari(VarName, BB_UDWORD, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 6:
            bbVarID = add_bbvari(VarName, BB_QWORD, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 7:
            bbVarID = add_bbvari(VarName, BB_UQWORD, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 8:
            bbVarID = add_bbvari(VarName, BB_FLOAT, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        case 9:
            bbVarID = add_bbvari(VarName, BB_DOUBLE, unit);
            write_bbvari_minmax_check(bbVarID, tmpDouble);
            break;
        default:
            add_bbvari(VarName, BB_UNKNOWN, unit);
            break;
        }
    }

    status_vars[0] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SCRIPT), BB_UBYTE, "");
    status_vars[1] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_GENERATOR), BB_UBYTE, "");
    status_vars[2] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_STIMULUS_PLAYER), BB_UBYTE, "");
    status_vars[3] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_EQUATION_CALCULATOR), BB_UBYTE, "");
    status_vars[4] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_TRACE_RECORDER), BB_UBYTE, "");
    // exit code if an program is startet with START_EXE script command (not the XilEnv exit code)
    status_vars[5] = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD, "ExitCode", txt, sizeof(txt)), BB_DWORD, "");

    SetNotFasterThanRealtimeState (s_main_ini_val.NotFasterThanRealTime);

    SetSuppressDisplayNonExistValuesState (s_main_ini_val.SuppressDisplayNonExistValues);

    s_main_ini_val.DontCallSleep = SetPriority (s_main_ini_val.Priority);

    // Start remote control thread
    StartRemoteProcedureCallThread (s_main_ini_val.RpcOverSocketOrNamedPipe, InstanceName, s_main_ini_val.RpcSocketPort);

    init_pid = start_process("init");

    return 0;
}


static BOOL QuitScriptRunningFlag = FALSE;
static BOOL SaveIniFileFlag = TRUE;
static BOOL StartStoppingSchedulerFlag = FALSE;

int RequestForTermination (void)
{
    // If there is running something that schould not interrupt
    if (s_main_ini_val.TerminateScriptFlag) {
        if (QuitScriptRunningFlag) {
            if (ThrowError (ERROR_OKCANCEL, "a terminate script \"%s\" is running\n"
                                       "press OK to terminate %s immediately or cancel to continue",
                                       s_main_ini_val.TerminateScript,
                                       GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME)) == IDOK) {
                QuitScriptRunningFlag = 0;
                s_main_ini_val.TerminateScriptFlag = 0;
                goto EXIT_IMMEDIATELY;
            }
        } else {
            StartTerminateScript ();
            QuitScriptRunningFlag = TRUE;
        }
        return 0;
    }
    if (!StartStoppingSchedulerFlag) {
        // No more external process logins
        TerminateExternProcessMessages ();

        if (CheckIfAllSchedulerAreTerminated()) {
             goto EXIT_IMMEDIATELY;
        }
        StartStoppingSchedulerFlag = 1;
        // If -nogui was set wait longer  (+100s)
        SetTerminateAllSchedulerTimeout (s_main_ini_val.TerminateSchedulerExternProcessTimeout + ((OpenWithNoGui()) ? 100 : 0));
        TerminateAllSchedulerRequest();
        return 0;
    } else {
        if (!CheckIfAllSchedulerAreTerminated()) {
            if (ThrowError (ERROR_OKCANCEL, "not all scheduler are stopped\n"
                            "press OK to terminate %s immediately or cancel to continue waiting",
                            GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME)) == IDOK) {
                s_main_ini_val.TerminateScriptFlag = 0;
                goto EXIT_IMMEDIATELY;
            }
        }
    }

EXIT_IMMEDIATELY:
    if (s_main_ini_val.SwitchAutomaticSaveIniOff == 0) {
        char IniFileName[MAX_PATH];
        IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), IniFileName, sizeof(IniFileName));
        if (access (IniFileName, 2) != 0) {   // Cannot write to INI file
            if (ThrowError (ERROR_OKCANCEL, "cannot save %s (no write access) close anyway?", IniFileName) != IDOK) {
                return 0;   // terminate immediately
            }
            SaveIniFileFlag = FALSE;
        }
    } else {
        if (s_main_ini_val.AskSaveIniAtExit == 1) {
            char IniFileName[MAX_PATH];
            IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), IniFileName, sizeof(IniFileName));
            if (ThrowError (ERROR_OKCANCEL, "Automatic save of the ini is switched off - save %s anyway?", IniFileName) == IDOK) {
                SaveIniFileFlag = TRUE;
            }
        }
    }
    return 1;  // terminate
}


int TerminateSelf (void)
{
    int i;

    // stop remote control login thread
    StopAcceptingNewConnections(NULL);

    // no more logins!
#ifdef _WIN32
    TerminatePipeMessages ();
#endif
    TerminateErrorDialog ();

    for (i = 0; i < 6; i++) {
        remove_bbvari (status_vars[i]);
    }

    if (s_main_ini_val.SwitchAutomaticSaveIniOff) {
        IniFileDataBaseClose(GetMainFileDescriptor());
        SetMainFileDescriptor(-1);
    } else {
        // At the end save all configurations
        WriteBasicConfigurationToIni (&s_main_ini_val);
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_WriteBackVariableSectionCache();
        }
        IniFileDataBaseSave(GetMainFileDescriptor(), NULL, INIFILE_DATABAE_OPERATION_REMOVE);
    }

    KillAllExternProcesses ();

    TerminateDebugInfos();

#ifndef NO_GUI
    StopBlackboardObserver();
#endif

    close_blackboard();

    TerminateEquationList();

    TerminateA2lLinkThread();
    TerminateFetchData();

    VirtualNetworkTerminate();

    if (s_main_ini_val.ConnectToRemoteMaster) {
        rm_Terminate();
        rm_Kill();
    }

    TerminateFifos();

    IniFileDataBaseTerminate();

    TerminateUniqueNumbers();

    terminate_memory();  // this must be the last one!

    return 0;
}

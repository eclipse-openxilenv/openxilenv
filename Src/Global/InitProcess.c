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


#define INITPROCESS_C

#include <stdio.h>
#include <string.h>
#include "Platform.h"

#include "Config.h"
#include "ReadConfig.h"
#include "Files.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "Message.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "IniDataBase.h"
#include "MainValues.h"
#include "EnvironmentVariables.h"
#include "Scheduler.h"
#include "InterfaceToScript.h"
#include "StartupInit.h"
#include "MainWindow.h"
#include "SharedDataTypes.h"
#include "MainWindow.h"
#include "InitProcess.h"


static CRITICAL_SECTION InitCriticalSection;

void InitInitCriticalSection (void)
{
    InitializeCriticalSection (&InitCriticalSection);
}

static int ExternProcessTerminateOrResetCount;
static struct {
    int State;
    uint32_t Timeout;
    int Vid;
    int Pid;
    char *Name;
} ExternProcessTerminateOrResets[32];

static void DeleteExternProcessTerminateOrResetBBVariablebyIndex (int Index)
{
    int x;
    if (ExternProcessTerminateOrResets[Index].Name != NULL) my_free (ExternProcessTerminateOrResets[Index].Name);
    attach_bbvari(ExternProcessTerminateOrResets[Index].Vid);
    write_bbvari_udword(ExternProcessTerminateOrResets[Index].Vid, 0);   // reset it before release
    remove_bbvari(ExternProcessTerminateOrResets[Index].Vid);
    for ( x = Index + 1; x < ExternProcessTerminateOrResetCount; x++) {
        ExternProcessTerminateOrResets[x-1] = ExternProcessTerminateOrResets[x];
    }
    ExternProcessTerminateOrResetCount--;
}

int AddExternProcessTerminateOrResetBBVariable (int pid)
{
    int ret;
    char ProcessName[MAX_PATH];
    char ProcessNameWithoutPath[MAX_PATH];
    char BlackboardVariable[BBVARI_NAME_SIZE];
    int vid;

    ret = GetProcessNameWithoutPath (pid, ProcessNameWithoutPath);
    if (ret) return ret;
    sprintf (BlackboardVariable, "%s.ExternProcess.%s.Action", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD), ProcessNameWithoutPath);
    vid = add_bbvari(BlackboardVariable, BB_UDWORD, "");
    if (vid <= 0) {
        ThrowError (1, "cannot add external process action variable %s", BlackboardVariable);
        return vid;
    }
    set_bbvari_conversion(vid, BB_CONV_TEXTREP, "0 0 \"none\"; 1 1 \"terminate\"; 2 2\"reset\"; 3 99 \"undef\"; 100 * \"userdef\";");
    if (ExternProcessTerminateOrResetCount <= (int)(sizeof(ExternProcessTerminateOrResets) / sizeof(ExternProcessTerminateOrResets[0]))) {
        EnterCriticalSection (&InitCriticalSection);
        ExternProcessTerminateOrResets[ExternProcessTerminateOrResetCount].Vid = vid;
        ExternProcessTerminateOrResets[ExternProcessTerminateOrResetCount].Pid = pid;
        ExternProcessTerminateOrResets[ExternProcessTerminateOrResetCount].State = 0;
        if (get_name_by_pid (pid, ProcessName) == 0) {
            ExternProcessTerminateOrResets[ExternProcessTerminateOrResetCount].Name = (char*)my_malloc(strlen(ProcessName)+1);
            if (ExternProcessTerminateOrResets[ExternProcessTerminateOrResetCount].Name != NULL) strcpy (ExternProcessTerminateOrResets[ExternProcessTerminateOrResetCount].Name, ProcessName);
        } else {
            ExternProcessTerminateOrResets[ExternProcessTerminateOrResetCount].Name  = NULL;
        }
        ExternProcessTerminateOrResetCount++;
        LeaveCriticalSection (&InitCriticalSection);
    } else {
        ThrowError (1, "cannot add external process action variable %s because the limit of 32 externel processes are reached", BlackboardVariable);
    }
    return 0;
}

int RemoveExternProcessTerminateOrResetBBVariable (int pid)
{
    int x;
    int ret;
    char ProcessNameWithoutPath[MAX_PATH];
    char BlackboardVariable[BBVARI_NAME_SIZE];
    int vid;

    ret = GetProcessNameWithoutPath (pid, ProcessNameWithoutPath);
    if (ret) return ret;
    sprintf (BlackboardVariable, "%s.ExternProcess.%s.Action", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD), ProcessNameWithoutPath);

    vid = get_bbvarivid_by_name (BlackboardVariable);
    if (vid <= 0) {
        ThrowError (1, "cannot remove external process action variable %s", BlackboardVariable);
        return vid;
    }
    EnterCriticalSection (&InitCriticalSection);
    for (x = 0; x < ExternProcessTerminateOrResetCount; x++) {
        if (ExternProcessTerminateOrResets[x].Vid == vid) {
            if (ExternProcessTerminateOrResets[x].State < 2) {
                DeleteExternProcessTerminateOrResetBBVariablebyIndex (x);
            }
            LeaveCriticalSection (&InitCriticalSection);
            return 0;
        }
    }
    LeaveCriticalSection (&InitCriticalSection);
    ThrowError (1, "cannot remove external process action variable %s because it is unknown", BlackboardVariable);
    return -1;
}

static int exit_vid;
static int exitcode_vid;
static int period_vid;

static int init_cycle_state;

static int RealtimeFactor_vid;

#define IDLE_STATE                                       0
#define WAIT_UNTIL_DSP_IS_CONFIGURED_STATE               1
#define WAIT_UNTIL_CAN_IS_CONFIGURED_STATE               2
#define START_ALL_EXTERNAL_PROCESSES_STATE               3 
#define START_ALL_EXTERNAL_PROCESSES_DONE_STATE          4  
#define START_START_STARTUP_SCRIPT_STATE                 5

static double period;

static char *StartupScript;

int SetStartupScript (char *par_StartupScript)
{
    StartupScript = my_malloc (strlen (par_StartupScript) + 1);
    strcpy (StartupScript, par_StartupScript);
    return 0;
}

static char *RemoteMasterExecutable;

int SetRemoteMasterExecutableCmdLine (char *par_RemoteMasterExecutable)
{
    RemoteMasterExecutable = my_malloc (strlen (par_RemoteMasterExecutable) + 1);
    strcpy (RemoteMasterExecutable, par_RemoteMasterExecutable);
    return 0;
}

char *GetRemoteMasterExecutableCmdLine (void)
{
    return RemoteMasterExecutable;
}

static int ActivateStartupScript (void)
{
    if (StartupScript != NULL) {
		if (get_pid_by_name("Script") == UNKNOWN_PROCESS) {
		    if (ThrowError (ERROR_OKCANCEL, "Script process not started to execute startup script \"%s\"!\nPlease press OK to start process \"Script\"!", StartupScript) == IDOK) {
                if (start_process ("Script") < 0) {
                    ThrowError (1, "cannot start prozess \"Script\"");
                }
			}
		}
		SearchAndReplaceEnvironmentStrings (StartupScript, script_filename, MAX_PATH);
        script_status_flag = 1;
        my_free (StartupScript);
        StartupScript = NULL;
    }
    return 0;
}

int init_init (void)
{
    static int done_flag;
    long vids[5];
    char str_period[32];
    char Name[BBVARI_NAME_SIZE];

    if (done_flag) {
        ThrowError (1, "process cannot restart");
        return -1;
    } else done_flag = 1;
    init_cycle_state = WAIT_UNTIL_DSP_IS_CONFIGURED_STATE;

    vids[0] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_FREQUENCY), BB_DOUBLE, "Hz");
    period_vid = vids[1] = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME), BB_DOUBLE, "s");
    vids[2] = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD, "Version", Name, sizeof(Name)), BB_UWORD, "");
    vids[3] = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD, "Realtime", Name, sizeof(Name)), BB_UWORD, "");
    vids[4] = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD, "Version.Patch", Name, sizeof(Name)), BB_WORD, "");
    exit_vid = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD, "exit", Name, sizeof(Name)), BB_UWORD, "");
    exitcode_vid = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_OWN_EXIT_CODE, "", Name, sizeof(Name)), BB_DWORD, "");

    IniFileDataBaseReadString("Scheduler", "Period", "0.01", str_period,
                               sizeof (str_period), GetMainFileDescriptor());

    RealtimeFactor_vid = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".RealtimeFactor", Name, sizeof(Name)), BB_DOUBLE, "-");

    CycleCounterVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CYCLE_COUNTER), BB_UDWORD, "");

    period = atof (str_period);
    if (period > 10.0) period = 10.0;
    if (period < 0.000001) period = 0.000001;

    write_bbvari_double (vids[0], 1.0/period);
    write_bbvari_double (vids[1], period);
    write_bbvari_uword (vids[2], XILENV_VERSION);
    write_bbvari_word (vids[4], XILENV_MINOR_VERSION);
    if (s_main_ini_val.ConnectToRemoteMaster) {
        write_bbvari_uword (vids[3], 1);
    } else {
        write_bbvari_uword (vids[3], 0);
    }
    return 0;
}

int write_process_list2ini (void)
{
    int x, y;
    char *pname;
    char entry[32];
    char path[512];
    char *ProcessNames[MAX_PROCESSES];
    char *p;
    char ExeName[MAX_PATH];

    memset (ProcessNames, 0, sizeof (ProcessNames));
    if (s_main_ini_val.StartDirectory[0] == 0) {
        path[0] = 0;
    } else sprintf (path, "%s\\", s_main_ini_val.StartDirectory);
    if (!s_main_ini_val.WriteProtectIniProcessList) {
        int LastProcessNr = 0;
        READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (1);
        for (x = 0; x < MAX_PROCESSES; x++) {
            do {
                pname = read_next_process_name (Buffer);
            } while ((pname != NULL) && (!strcmp (pname, "init") || !strcmp (pname, "RemoteMasterControl")));   // Do not save init process inside INI file
            sprintf (entry, "P%i", x);
            if (pname != NULL) {
                if (strstr (pname, path) == pname) { // If the beginning of the process is the same as current path
                    p = pname + strlen (path);
                } else {
                    p = pname;
                }
                // If it is an external process look if ist is already inside the list
                if (GetProsessLongExeFilename (p, ExeName, sizeof (ExeName)) == 0) {
                    p = ExeName;
                    for (y = 0; y < x; y++) {
                        if (ProcessNames[y] != NULL) {
                            if (!strcmpi (ProcessNames[y], p)) {
                                break;
                            }
                        }
                    }
                    if (y != x) {
                        x--;
                        continue;  // EXE is alread saved inside the list
                    }
                }
                ProcessNames[x] = my_malloc (strlen (p) + 1);
                strcpy (ProcessNames[x], p);
                IniFileDataBaseWriteString ("InitStartProcesses", entry, p, GetMainFileDescriptor());
                LastProcessNr = x;
            } else IniFileDataBaseWriteString ("InitStartProcesses", entry, NULL, GetMainFileDescriptor());
        }
        close_read_next_process_name(Buffer);

        for (x = 0; x < MAX_PROCESSES; x++) {
            if (ProcessNames[x] != NULL) my_free (ProcessNames[x]);
        }
    }
    return 0;
}

void terminate_init (void)
{
    write_process_list2ini ();
}

int CheckIfCANIsConfigured (void)
{
    int pid;
    MESSAGE_HEAD mhead;
    static int cycle;
    static int configured;

    if (configured) return 1;
    pid = get_pid_by_name ("CANServer");
    if (pid <= 0) {
        configured = 1;
        return 1;   // CAN-Server is not avtive
    }
    if (!cycle) {
        write_message (pid, IS_NEW_CANSERVER_RUNNING, 0, NULL);
        cycle++;
    } else if (cycle >= 10) cycle = 0;
    else cycle++;
    if (test_message (&mhead)) {
        remove_message ();
        if ((mhead.mid == IS_NEW_CANSERVER_RUNNING) &&
            (mhead.pid == pid)) {
            configured = 1;
            return 1;        // If CAN-Server response it is configured
        }
    }
    return 0; 
}

int CheckIfDSPIsConfigured (void)
{
    int pid;
    MESSAGE_HEAD mhead;
    static int cycle;
    static int configured;

    if (configured) return 1;
    pid = get_pid_by_name ("DSP56301");
    if (pid <= 0) {
        configured = 1;
        return 1;   // DSP process is not avtive
    }
    if (!cycle) {
        write_message (pid, 0xFFFF0, 0, NULL);
        cycle++;
    } else if (cycle >= 10) cycle = 0;
    else cycle++;
    if (test_message (&mhead)) {
        remove_message ();
        if ((mhead.mid == 0xFFFF0) &&
            (mhead.pid == pid)) {
            configured = 1;
            return 1;        // If DSP-Prozess response it is configured
        }
    }
    return 0; 
}

static int extern_process_login_enable_flag;

static int TerminateScriptState;

static uint64_t CompareDoubleEqual(double a, double b)
{
    double diff = a - b;
    if ((diff <= 0.0) && (diff >= 0.0)) return 1;
    else return 0;
}

void cyclic_init (void)
{
    int x;

    // Wait till all internal processes are configured
    switch (init_cycle_state) {
    case IDLE_STATE:
        // do nothing
        break;
    case WAIT_UNTIL_DSP_IS_CONFIGURED_STATE:
        // Wait till all configurations are loaded
        if (s_main_ini_val.ConnectToRemoteMaster) {
            if (CheckIfDSPIsConfigured ()) {
                init_cycle_state = WAIT_UNTIL_CAN_IS_CONFIGURED_STATE;
            }
        } else {
            init_cycle_state = WAIT_UNTIL_CAN_IS_CONFIGURED_STATE;
        }
        break;
    case WAIT_UNTIL_CAN_IS_CONFIGURED_STATE:
        // Wait till all configurations are loaded
        if (CheckIfCANIsConfigured ()) {
            init_cycle_state = START_ALL_EXTERNAL_PROCESSES_STATE;
            // enable external process login
            // Now external processes can be logged in
            extern_process_login_enable_flag = 1;
        }
        break;
    case START_ALL_EXTERNAL_PROCESSES_STATE:
        init_cycle_state = START_ALL_EXTERNAL_PROCESSES_DONE_STATE;
        break;
    case START_ALL_EXTERNAL_PROCESSES_DONE_STATE:
        // If all processes from process liste are started
        // Login-Array deaktivieren
        init_cycle_state = START_START_STARTUP_SCRIPT_STATE;
        break;
    case START_START_STARTUP_SCRIPT_STATE:
        // If a startup script is set start it now
        ActivateStartupScript ();
        init_cycle_state = IDLE_STATE;
        break;
    }
    if (exit_vid > 0) {
        if (read_bbvari_uword (exit_vid) != 0) {
            int ExitCode = read_bbvari_dword (exitcode_vid);
            exit_vid = 0;   // Send this only one time otherwise there will be rise error message like
                            // "cannot write to INI file" if remote master is active
            CloseFromOtherThread (ExitCode, 1);
        }
    }
    // Control that nobody has change the sample period
    if (!CompareDoubleEqual(read_bbvari_double (period_vid), period)) {
        write_bbvari_double (period_vid, period);
    }
    if (TerminateScriptState) {
        switch (TerminateScriptState) {
        case 1:   // Check if a terminate script is running
            script_status_flag = 0;
            TerminateScriptState++;
            break;
        case 2:   // Start terminate script
            SearchAndReplaceEnvironmentStrings (s_main_ini_val.TerminateScript, script_filename, MAX_PATH);
            script_status_flag = START;
            TerminateScriptState++;
            break;
        case 3:   // Wait till terminate script is finished
            if (script_status_flag != RUNNING) {
                TerminateScriptState++;
            }
            break;
        case 4:
            CloseFromOtherThread (0, 0);  // do not set the exit code again
            s_main_ini_val.TerminateScriptFlag = 0;
            TerminateScriptState++;
            break;
        default:
            break;
        }
    }
    // Put the Realtime Factor into the blackboard
    write_bbvari_double (RealtimeFactor_vid, GetRealtimeFactor());
    //DebugNotFasterThanRealtime ();

    EnterCriticalSection (&InitCriticalSection);

    for (x = 0; x < ExternProcessTerminateOrResetCount; x++) {
        switch (ExternProcessTerminateOrResets[x].State) {
        default:
        case 0:
            switch (read_bbvari_udword (ExternProcessTerminateOrResets[x].Vid)) {
            case 0:
            default:
                break;   // do nothing
            case 1:
                // terminate
                terminate_process(ExternProcessTerminateOrResets[x].Pid);
                ExternProcessTerminateOrResets[x].State = 1;
                break;
            case 2:
                // Save timeout
                {
                    int timeout = get_process_timeout(ExternProcessTerminateOrResets[x].Pid);
                    if (timeout <= 0) {
                        timeout = s_main_ini_val.ExternProcessLoginTimeout;
                    }
                    ExternProcessTerminateOrResets[x].Timeout = (uint32_t)timeout;
                }
                // terminate
                ExternProcessTerminateOrResets[x].State = 2;
                terminate_process(ExternProcessTerminateOrResets[x].Pid);
                break;
            }
            break;
        case 1:
            // Wait till process are terminaed
            if (get_process_state(ExternProcessTerminateOrResets[x].Pid) == UNKNOWN_PROCESS) {
                write_bbvari_udword(ExternProcessTerminateOrResets[x].Vid, 0);   // reset it immediately
                ExternProcessTerminateOrResets[x].State = 0;
            }
            break;
        case 2:
            // Wait till process are terminaed
            if (get_process_state(ExternProcessTerminateOrResets[x].Pid) == UNKNOWN_PROCESS) {
                // Now restart him again
                if (ExternProcessTerminateOrResets[x].Name != NULL) {
                    char ProcessExecutableName[MAX_PATH];
                    TruncateTaskNameFromProcessName (ProcessExecutableName, ExternProcessTerminateOrResets[x].Name);
                    if (activate_extern_process(ProcessExecutableName, -1, NULL)) {
                        ThrowError (1, "cannot restart externel process\"%s\"", ProcessExecutableName);
                        ExternProcessTerminateOrResets[x].State = 0;
                        DeleteExternProcessTerminateOrResetBBVariablebyIndex(x);
                    } else {
                        // Convert timeout from Seconds to system ticks
                        ExternProcessTerminateOrResets[x].Timeout = GetTickCount() + ExternProcessTerminateOrResets[x].Timeout * 1000; 
                        ExternProcessTerminateOrResets[x].State = 3;
                    }
                } else {
                    ExternProcessTerminateOrResets[x].State = 0;
                    DeleteExternProcessTerminateOrResetBBVariablebyIndex(x);
                }
            }
            break;
        case 3:
            // Wait till process is started again
            if (ExternProcessTerminateOrResets[x].Name != NULL) {
                int Pid = get_pid_by_name(ExternProcessTerminateOrResets[x].Name);
                if (Pid != UNKNOWN_PROCESS) {
                    ExternProcessTerminateOrResets[x].Pid = Pid;
                    ExternProcessTerminateOrResets[x].State = 0;
                    write_bbvari_udword(ExternProcessTerminateOrResets[x].Vid, 0);   // reset it immediately
                    DeleteExternProcessTerminateOrResetBBVariablebyIndex(x);
                } else {
                    if (GetTickCount() >  ExternProcessTerminateOrResets[x].Timeout) {
                        ThrowError (1, "cannot restart externel process\"%s\" timeout", ExternProcessTerminateOrResets[x].Name);
                        ExternProcessTerminateOrResets[x].Pid = Pid;
                        ExternProcessTerminateOrResets[x].State = 0;
                        write_bbvari_udword(ExternProcessTerminateOrResets[x].Vid, 0);   // reset it immediately
                        DeleteExternProcessTerminateOrResetBBVariablebyIndex(x);
                    }
                }
            } else {
                ExternProcessTerminateOrResets[x].State = 0;
                write_bbvari_udword(ExternProcessTerminateOrResets[x].Vid, 0);   // reset it immediately
                DeleteExternProcessTerminateOrResetBBVariablebyIndex(x);
            }
            break;
        }
    }
    LeaveCriticalSection (&InitCriticalSection);
}

int StartTerminateScript (void)
{
    TerminateScriptState = 1;
    return 0;
}

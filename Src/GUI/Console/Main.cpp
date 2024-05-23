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
#include <locale.h>

#include "MainWindow.h"

extern "C" {
#include "ThrowError.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "MainValues.h"
#include "StartupInit.h"
#include "ParseCommandLine.h"
#include "ScriptErrorFile.h"
#include "Scheduler.h"
    extern int rm_Terminate(void);
    extern void rm_Kill(void);
}

static int RunningFlag;
static CRITICAL_SECTION TerminateCriticalSection;
static CONDITION_VARIABLE TerminateConditionVariable;
static bool ExitCodeSeeded = false;
static int ExitCode;


void terminate_main_loop(int par_ExitCode, int par_ExitCodeValid)
{
    if (!ExitCodeSeeded) {    // set Exit Code only once!
        if (par_ExitCodeValid) {
            ExitCode = par_ExitCode;
            ExitCodeSeeded = true;
        }
    }
    RunningFlag = 1;
    WakeAllConditionVariable(&TerminateConditionVariable);
}

int main(int argc, char *argv[])
{
    int Ret;
#ifndef _WIN32
    // damit auch unter Linux auf die argumente in ParseCommandLine() zugegriffen werden kann
    SaveCommandArgs(argc, argv);
#endif

    // Always with local "C" use '.' not ',' character for numbers
    setlocale(LC_ALL, "C");


    // That is always active independent of the parameter "-scripterrexit"
    SetScriptErrExit(TERMINATE_XILENV_ON_SCRIPT_EXECUTION_ERROR | TERMINATE_XILENV_ON_SCRIPT_SYNTAX_ERROR);

    if (StartupInit(NULL)) {
        return 1;
    }

    // Now the scheduler can run
    SchedulersStartingShot();

    // Wait till XilEnv should be treminated
    InitializeCriticalSection(&TerminateCriticalSection);
    InitializeConditionVariable(&TerminateConditionVariable);
    
    EnterCriticalSection(&TerminateCriticalSection);
    while (!RunningFlag) {
        SleepConditionVariableCS_INFINITE(&TerminateConditionVariable, &TerminateCriticalSection);
    }
    LeaveCriticalSection(&TerminateCriticalSection);

    if (RequestForTermination() == 1) {
        goto __EXIT;
    }
    else {
        while (1) {
            if (CheckIfAllSchedulerAreTerminated()) {  // If all scheduler are terminated send the close event again
                goto __EXIT;
            } else {
                if (CheckTerminateAllSchedulerTimeout()) {
                    char *ProcessNames;
                    switch (ThrowError(ERROR_OKCANCEL, "cannot terminate all schedulers press OK to terminate %s anyway or cancel to continue waiting\n"
                        "following processe are not stopped:\n%s",
                         GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), ProcessNames = GetProcessInsideExecutionFunctionSemicolonSeparated())) {
                    case IDOK:
                        goto __EXIT;
                        break;
                    default:
                    case IDCANCEL:
                        SetTerminateAllSchedulerTimeout(10);
                        break;
                    }
                    if (ProcessNames != nullptr) my_free(ProcessNames);
                }
            }
        }
    }
__EXIT:
    TerminateSelf();

    if (s_main_ini_val.ConnectToRemoteMaster) {
        rm_Terminate();
        rm_Kill();
    }

    return ExitCode;
}

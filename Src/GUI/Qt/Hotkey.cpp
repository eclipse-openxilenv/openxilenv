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


#include "Hotkey.h"
#include "MainWindow.h"
#include "StringHelpers.h"

extern "C"
{
//    #include "IniDataBase.h"
    #include "MainValues.h"
    #include "EquationParser.h"
    #include "EnvironmentVariables.h"
    #include "InterfaceToScript.h"
    #include "Scheduler.h"
}

cSCHotkey::cSCHotkey(int type, QString formula)
{
    cSCHotkey::type = type;
    cSCHotkey::formula = formula;
}

cSCHotkey::~cSCHotkey()
{

}

void cSCHotkey::activateHotkeys()
{
    switch (type) {
    case 1:   // single shot equation
        if (formula != nullptr) {
            direct_solve_equation_no_error_message (QStringToConstChar(formula));
        }
        break;
    case 2:   // start script
            if (formula == nullptr) {
                QString Filename = GetScriptFilenameFromControlPannel();
                if (Filename.size() > 0){
                    strcpy(script_filename, QStringToConstChar(Filename));
                    SearchAndReplaceEnvironmentStrings (script_filename, script_filename, MAX_PATH);
                    script_status_flag = START;
                }
            } else {
                // Dateiname in globale Variable uebernehmen und Flag setzen
                if (script_status_flag != RUNNING) {
                    strcpy(script_filename, QStringToConstChar(formula));
                    SearchAndReplaceEnvironmentStrings (script_filename, script_filename, MAX_PATH);
                    script_status_flag = START;
                }
            }
        break;
    case 3:   // stop script
        script_status_flag = 0;
        break;
    case 4:
        //SelectNextSheetHotkey ();
        break;
    case 5:   // Run control continue
        enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
        break;
    case 6:   // Run control stop
        disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_USER, nullptr, nullptr);
        break;
    case 7:   // Run control next one
        make_n_next_cycles(SCHEDULER_CONTROLED_BY_USER, 1, nullptr, nullptr);
        break;
    case 8:   // Run control next xxx
        make_n_next_cycles(SCHEDULER_CONTROLED_BY_USER, s_main_ini_val.NumberOfNextCycles,
                           nullptr, nullptr);
        break;
    default:
        break;
    }
}


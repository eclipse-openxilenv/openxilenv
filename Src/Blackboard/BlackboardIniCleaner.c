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
#include "IniDataBase.h"
#include "Blackboard.h"
#include "MainValues.h"
#include "RemoteMasterBlackboard.h"
#include "BlackboardIniCleaner.h"

int CleanVariablesSection (void)
{
    int EntryIdx = 0;
    int Counter = 0;
    char Entry[INI_MAX_ENTRYNAME_LENGTH];

    if (s_main_ini_val.ConnectToRemoteMaster) {
        rm_WriteBackVariableSectionCache();
    }

    while ((EntryIdx = IniFileDataBaseGetNextEntryName(EntryIdx, "Variables", Entry, sizeof(Entry), GetMainFileDescriptor())) >= 0) {
        if (!get_bbvarivid_by_name (Entry)) {   // Variable doesn't exist
            Counter++;                          // remove them from INI section
            IniFileDataBaseWriteString("Variables", Entry, NULL, GetMainFileDescriptor());
        }
    }

    if (s_main_ini_val.ConnectToRemoteMaster) {
        rm_InitVariableSectionCache();
    }

    return Counter;
}

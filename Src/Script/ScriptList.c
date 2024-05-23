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
#include <stdlib.h>
#include <stdio.h>

#include "MyMemory.h"
#include "ThrowError.h"
#include "Blackboard.h"
#include "Scheduler.h"
#include "DebugInfoAccessExpression.h"
#include "DebugInfoDB.h"
#include "GetNextStructEntry.h"
#include "UniqueNumber.h"

#include "ScriptList.h"


// List of referenced variable so they can dereference later

typedef struct {
    char *Name;
    uint64_t Address;
    int Type;
    int Fill;
} SCRIPT_REFERENCE_VARI_ELEM;

typedef struct {
    int Pid;
    int Connection;
    int VariCount;
    SCRIPT_REFERENCE_VARI_ELEM *Varis;
} SCRIPT_REFERENCE_VARIS;

static SCRIPT_REFERENCE_VARIS *ScriptReferenceVaris;
static int ScriptReferenceVarisSize;
static int ScriptReferenceVarisCount;


static int ScriptSearchReferenceVariList (int Pid, int Connection)
{
    int x;

    for (x = 0; x < ScriptReferenceVarisCount; x++) {
        if (ScriptReferenceVaris[x].Pid == Pid) {
            if (ScriptReferenceVaris[x].Connection == Connection) {
                return x;
            }
        }
    }
    return -1;
}

int ScriptDelReferenceVariList (int Pid, int Connection)
{
    int x, Pos;

    if ((Pos = ScriptSearchReferenceVariList (Pid, Connection)) >= 0) { 
        for (x = 0; x < ScriptReferenceVaris[Pos].VariCount; x++) {
            if (ScriptReferenceVaris[Pos].Varis[x].Name != NULL) {
                if (scm_unref_vari (ScriptReferenceVaris[Pos].Varis[x].Address, 
                                    Pid, 
                                    ScriptReferenceVaris[Pos].Varis[x].Name, 
                                    ScriptReferenceVaris[Pos].Varis[x].Type)) {
                    ThrowError (1, "cannot dereference variable (extern process not running or halted in debugger)");
                }
                my_free (ScriptReferenceVaris[Pos].Varis[x].Name);
            }
        }
        my_free (ScriptReferenceVaris[Pos].Varis);

        for (x = Pos; x < (ScriptReferenceVarisCount-1); x++) {
            ScriptReferenceVaris[x] = ScriptReferenceVaris[x+1];
        }

        ScriptReferenceVaris[x].Pid = -1;
        ScriptReferenceVaris[x].Connection = -1;
        ScriptReferenceVaris[x].VariCount = 0;
        ScriptReferenceVaris[x].Varis = NULL;

        ScriptReferenceVarisCount--;
        return 0;
    }
    return -1;
}

int ScriptAddReferenceVariList (char *ProcessName, int Pid, int Connection, int VariCount, char *Varis[])
{
    int x;

    if (ScriptSearchReferenceVariList (Pid, Connection) >= 0) {  // If already exist first delete it
        ScriptDelReferenceVariList (Pid, Connection);
    }
    if (ScriptReferenceVarisCount == ScriptReferenceVarisSize) {
        ScriptReferenceVarisSize += 64;
        ScriptReferenceVaris = (SCRIPT_REFERENCE_VARIS*)my_realloc (ScriptReferenceVaris, sizeof (SCRIPT_REFERENCE_VARIS) * (size_t)ScriptReferenceVarisSize);
        if (ScriptReferenceVaris == NULL) {
            ThrowError (ERROR_SYSTEM_CRITICAL, "out of memory stop!");
            return -1;
        }
    }
    ScriptReferenceVaris[ScriptReferenceVarisCount].Pid = Pid;
    ScriptReferenceVaris[ScriptReferenceVarisCount].Connection = Connection;
    ScriptReferenceVaris[ScriptReferenceVarisCount].VariCount = VariCount;

    ScriptReferenceVaris[ScriptReferenceVarisCount].Varis = (SCRIPT_REFERENCE_VARI_ELEM*)my_calloc ((size_t)VariCount, sizeof (SCRIPT_REFERENCE_VARI_ELEM));
    if (ScriptReferenceVaris[ScriptReferenceVarisCount].Varis == NULL) {
        ThrowError (ERROR_SYSTEM_CRITICAL, "out of memory stop!");
        return -1;
    }
    if (WaitUntilProcessIsNotActiveAndThanLockIt (Pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot refrence variable", __FILE__, __LINE__) == 0) {
        PROCESS_APPL_DATA *pappldata;
        int UniqueId;
        int loc_Pid;

        UniqueId = AquireUniqueNumber();
        if (UniqueId < 0) {
            ThrowError (ERROR_SYSTEM_CRITICAL, "out of unique ids stop!");
            return -1;
        }
        pappldata = ConnectToProcessDebugInfos (UniqueId,
                                                ProcessName, &loc_Pid,
                                                NULL,
                                                NULL,
                                                NULL,
                                                0);
        if (pappldata == NULL) {
            FreeUniqueNumber (UniqueId);
            ThrowError (ERROR_SYSTEM_CRITICAL, "no debug infos for process \"%s\"", ProcessName);
            return -1;
        }

        for (x = 0; x < VariCount; x++) {
            uint64_t address;
            int32_t typenr;
            int type;
            char variname[BBVARI_NAME_SIZE];

            strncpy (variname, Varis[x], BBVARI_NAME_SIZE);
            variname[BBVARI_NAME_SIZE-1] = 0;

            if (appl_label (pappldata, Pid, variname, &address, &typenr)) {
                ConvertLabelAsapCombatible (variname, sizeof (variname), 0);   // replace :: with ._. if necessary
                ThrowError (1, "try to refrence unknown variable %s in process %s\n delete it ?", variname, ProcessName);
            } else if ((type = get_base_type_bb_type_ex (typenr, pappldata)) < 0) {
                ConvertLabelAsapCombatible (variname, sizeof (variname), 0);   // replace:: with ._. if necessary
                ThrowError (1, "rerefrence variable %s with unknown type in process %s", variname, ProcessName);
            } else {
                if (!scm_ref_vari (address, (short)Pid, variname, type, 0x3)) {
                    ScriptReferenceVaris[ScriptReferenceVarisCount].Varis[x].Address = address;
                    ScriptReferenceVaris[ScriptReferenceVarisCount].Varis[x].Type = type;
                    ScriptReferenceVaris[ScriptReferenceVarisCount].Varis[x].Name = my_malloc (strlen (variname) + 1); 
                    if (ScriptReferenceVaris[ScriptReferenceVarisCount].Varis[x].Name == NULL) {
                        ThrowError (ERROR_SYSTEM_CRITICAL, "out of memory stop!");
                        RemoveConnectFromProcessDebugInfos(UniqueId);
                        FreeUniqueNumber (UniqueId);
                        UnLockProcess (Pid);
                        return -1;
                    }
                    strcpy (ScriptReferenceVaris[ScriptReferenceVarisCount].Varis[x].Name, variname);
                }
            }
        }
        RemoveConnectFromProcessDebugInfos(UniqueId);
        FreeUniqueNumber (UniqueId);
        UnLockProcess (Pid);
    }

    ScriptReferenceVarisCount++;

    return 0;
}

void ScriptDeleteAllReferenceVariLists (void)
{
    int x;

    for (x = ScriptReferenceVarisCount-1; x >= 0; x--) {
        ScriptDelReferenceVariList (ScriptReferenceVaris[x].Pid, ScriptReferenceVaris[x].Connection);
    }
    ScriptReferenceVarisCount = 0;
}

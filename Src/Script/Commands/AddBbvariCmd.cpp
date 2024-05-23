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


#include <stdio.h>

extern "C" {
#include "MyMemory.h"
#include "Blackboard.h"
#include "IniDataBase.h"
#include "MainValues.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cAddBbvariCmd)


int cAddBbvariCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cAddBbvariCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    int vid;
    char *unit;
    BB_DATA_TYPES vartyp_bb = BB_DOUBLE; /* standard DOUBLE */

    if (!IsValidVariableName (par_Parser->GetParameter (0))) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "\"%s\" is not a valid variable name!", par_Parser->GetParameter (0));
        return -1;
    }

    if(par_Parser->GetParameterCounter () >= 2) { // If there is defined adata type
        char *TypeStr = par_Parser->GetParameter (1);
        if      (!stricmp (TypeStr, "UBYTE") || !stricmp (TypeStr, "BB_UBYTE")) { vartyp_bb = BB_UBYTE;  }
        else if (!stricmp (TypeStr, "UWORD") || !stricmp (TypeStr, "BB_UWORD")) { vartyp_bb = BB_UWORD;  }
        else if (!stricmp (TypeStr, "FLOAT") || !stricmp (TypeStr, "BB_FLOAT")) { vartyp_bb = BB_FLOAT;  }
        else if (!stricmp (TypeStr, "DOUBLE") || !stricmp (TypeStr, "BB_DOUBLE")){ vartyp_bb = BB_DOUBLE; }
        else if (!stricmp (TypeStr, "UDWORD") || !stricmp (TypeStr, "BB_UDWORD")){ vartyp_bb = BB_UDWORD; }
        else if (!stricmp (TypeStr, "DWORD") || !stricmp (TypeStr, "BB_DWORD")) { vartyp_bb = BB_DWORD;  }
        else if (!stricmp (TypeStr, "BYTE" ) || !stricmp (TypeStr, "BB_BYTE" )) { vartyp_bb = BB_BYTE;   }
        else if (!stricmp (TypeStr, "WORD" ) || !stricmp (TypeStr, "BB_WORD" )) { vartyp_bb = BB_WORD;   }
        else if (!stricmp (TypeStr, "UQWORD") || !stricmp (TypeStr, "BB_UQWORD")){ vartyp_bb = BB_UQWORD; }
        else if (!stricmp (TypeStr, "QWORD") || !stricmp (TypeStr, "BB_QWORD")) { vartyp_bb = BB_QWORD;  }
        else {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "\"%s\" is not a valid data type!", TypeStr);
            return -1;
        }
    }
    /* Check if th variable already exist */
    if (get_bbvarivid_by_name (par_Parser->GetParameter (0)) > 0) {
        int returnwert = get_bbvaritype_by_name (par_Parser->GetParameter (0));
        /* Only if type is "unknown" do not add it again */
        if ((returnwert != BB_UNKNOWN_WAIT) && (returnwert != vartyp_bb)) {
            par_Parser->Error (SCRIPT_PARSER_WARNING, "ADD_BBVARI(%s) Variable already exists with other vartype !", par_Parser->GetParameter (0));
            return 0;
        }
    }
    if (par_Parser->GetParameterCounter () == 3) {
        unit = par_Parser->GetParameter (2);
    } else {
        // Check if there is a INI file entry for this variable
        char tmp[INI_MAX_LINE_LENGTH]; 
        if (IniFileDataBaseReadString (VARI_SECTION, par_Parser->GetParameter (0),
                                       "", tmp, sizeof (tmp), GetMainFileDescriptor()) == 0) {
            unit = s_main_ini_val.Script_ADD_BBVARI_DefaultUnit; // "USER";
        } else {
            unit = nullptr;
        }
    }
    // now the variable can be added to the blackboard
    vid = add_bbvari (par_Parser->GetParameter (0), vartyp_bb, unit); 
    if (vid <= 0) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Adding new variable not possible !");
        return -1;
    }
    return 0;
}

int cAddBbvariCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cAddBbvariCmd AddBbvariCmd ("ADD_BBVARI",                        
                                   1, 
                                   3,  
                                   nullptr,
                                   FALSE, 
                                   FALSE, 
                                   0,
                                   0,
                                   CMD_INSIDE_ATOMIC_ALLOWED);


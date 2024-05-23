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

extern "C" {
#include "MyMemory.h"
#include "Scheduler.h"
#include "LoadSaveToFile.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cLoadSymBinCmd)


int cLoadSymBinCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cLoadSymBinCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    /* Parameter[0] = file name SYM file  */
    /* Parameter[1] = file name BIN file  */
    /* Parameter[2] = Process name        */
    /* Parameter[3] = 0=LSB_FIRST/ 1=MSB_FIRST */
    /* Parameter[4] = Address          */
    char Path[MAX_PATH];
    strncpy (Path, par_Parser->GetParameter (0), MAX_PATH);
    Path[MAX_PATH-1] = 0;
    ScriptIdentifyPath (Path);
    
    unsigned char ByteOrder = 99;

    if (par_Parser->GetParameterCounter () > 3) {
        if (!stricmp (par_Parser->GetParameter (3), "MSB_FIRST")) {
            ByteOrder = MSB_FIRST_FORMAT;
        } else if (!stricmp (par_Parser->GetParameter (3), "LSB_FIRST")) {
            ByteOrder = LSB_FIRST_FORMAT;
        } else {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "LSB_FIRST/MSB_FIRST format specifier is not correct spelled!");
            return -1;
        }
    } else {
        ByteOrder = LSB_FIRST_FORMAT;
    }
    /* Check if process name (third parameter) exists */
    if (get_pid_by_name (par_Parser->GetParameter (2)) == UNKNOWN_PROCESS) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Cannot find process-name '%s' !", par_Parser->GetParameter (2));
        return -1;
    }
    int returnwert = ScriptWriteValuesToProcess (par_Parser->GetParameter (0), 
                                                 par_Parser->GetParameter (1), 
                                                 par_Parser->GetParameter (2), 
                                                 ByteOrder,
                                                 par_Parser->GetParameter (4));
    if (!returnwert) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "LOAD_SYMBIN not possible !");
        return -1;
    }
    return 0;
}

int cLoadSymBinCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cLoadSymBinCmd LoadSymBinCmd ("LOAD_SYMBIN",                        
                                     3, 
                                     5,  
                                     nullptr,
                                     FALSE, 
                                     FALSE, 
                                     0,
                                     0,
                                     CMD_INSIDE_ATOMIC_ALLOWED);


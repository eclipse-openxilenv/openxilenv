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
#include "Message.h"
#include "CcpControl.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cLoadCcpCfgCmd)


int cLoadCcpCfgCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cLoadCcpCfgCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    int Connection;
    int FilenameOffset;

    if (par_Parser->GetParameterCounter () == 2) {
        if (par_Parser->SolveEquationForParameter (0, &Connection, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                               par_Parser->GetParameter (0));
            return -1;
        } 
        if ((Connection < 0) || (Connection >= 4)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "LOAD_CCP_CFG channel number (%i) out of range (0...3)", Connection);
            return -1;
        }
        FilenameOffset = 1;
    } else {
        Connection = 0;
        FilenameOffset = 0;
    }
    char Path[MAX_PATH];
    strncpy (Path, par_Parser->GetParameter (FilenameOffset), MAX_PATH);
    Path[MAX_PATH-1] = 0;
    ScriptIdentifyPath (Path);
    if (LoadConfig_CCP (Connection, Path) != FALSE) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "LOAD_CCP_CFG (%s) not possible - perhaps file doesn't exists!", Path);
        return -1;
    }
    return 0;
}

int cLoadCcpCfgCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cLoadCcpCfgCmd LoadCcpCfgCmd ("LOAD_CCP_CFG",                        
                               1, 
                               2,  
                               nullptr,
                               FALSE, 
                               FALSE, 
                               0,
                               0,
                               CMD_INSIDE_ATOMIC_ALLOWED);

static cLoadCcpCfgCmd LoadCcpCfgCmd2 ("LOAD_CCP_CFG2",                        
                                2, 
                                2,  
                                nullptr,
                                FALSE, 
                                FALSE, 
                                0,
                                0,
                                CMD_INSIDE_ATOMIC_ALLOWED);

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
#include "XcpControl.h"
#include "MainValues.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStopXcpCalCmd)


int cStopXcpCalCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cStopXcpCalCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int Connection;

    if (par_Parser->SolveEquationForParameter (0, &Connection, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (0));
        return -1;
    } 
    if ((Connection < 0) || (Connection >= 4)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "STOP_XCP_CAL channel number (%i) out of range (0...3)", Connection);
        return -1;
    }
    if (Stop_XCP (Connection, STOP_CALIBRATION) != FALSE) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "STOP_XCP_CAL not possible ! What about CCP-process ?");
        return -1;
    }
    par_Executor->SetData (Connection);

    return 0;
}

int cStopXcpCalCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(Cycles);
    int ccp_error;
    char *Help;
    if (Is_XCP_CommandDone (par_Executor->GetData (), &ccp_error, &Help) != FALSE) {
        if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("\t\t/* STOP_XCP_CAL needed %d cycles */\n", par_Executor->GetWaitCyclesNeeded ());
        if (ccp_error) {
            par_Parser->Error (s_main_ini_val.ScriptStopIfCcpError ? SCRIPT_PARSER_FATAL_ERROR : SCRIPT_PARSER_ERROR_CONTINUE, 
                               "STOP_XCP_CAL command got a protocoll error (%i) = \"%s\"", ccp_error, Help);
        } 
        return 0;
    }
    return 1;
}

static cStopXcpCalCmd StopXcpCalCmd ("STOP_XCP_CAL",                        
                               1, 
                               1,  
                               nullptr,
                               FALSE, 
                               TRUE,  
                               5000,
                               0,
                               CMD_INSIDE_ATOMIC_NOT_ALLOWED);


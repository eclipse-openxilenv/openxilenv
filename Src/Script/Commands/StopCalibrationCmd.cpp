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
#include "Scheduler.h"
#include "CcpControl.h"
#include "XcpControl.h"
#include "ScriptList.h"
#include "MainValues.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStopCalibrationCmd)


int cStopCalibrationCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

#define STOP_XCP_CMD  1
#define STOP_CCP_CMD  2
#define STOP_REF_CMD  5

int cStopCalibrationCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int Connection;

    if (par_Parser->SolveEquationForParameter (1, &Connection, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (1));
        return -1;
    } 
    if ((Connection < 0) || (Connection >= 4)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "STOP_CALIBRATION channel number (%i) out of range (0...3)", Connection);
        return -1;
    }
    par_Executor->SetData (Connection);

    if (!stricmp ("XCP", par_Parser->GetParameter (0))) {
        par_Executor->SetData (0, STOP_XCP_CMD);
        if (Stop_XCP (Connection, STOP_CALIBRATION) != FALSE) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "STOP_CALIBRATION (XCP) not possible ! What about XCP-process ?");
            return -1;
        }
    } else if (!stricmp ("CCP", par_Parser->GetParameter (0))) {
        par_Executor->SetData (0, STOP_CCP_CMD);
        if (Stop_CCP (Connection, STOP_CALIBRATION) != FALSE) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "STOP_CALIBRATION (CCP) not possible ! What about CCP-process ?");
            return -1;
        }
   } else {
        int Pid;
        par_Executor->SetData (0, STOP_REF_CMD);
        if ((Pid = get_pid_by_name (par_Parser->GetParameter (0))) >= 0) {  // ist es ein Prozess?
            ScriptDelReferenceVariList (Pid, Connection + 0x1000);
        } else {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "STOP_CALIBRATION \"%s\" not possible! Allowed are XCP, CCP, or a running process",
                               par_Parser->GetParameter (0));
            return -1;
        
        }
    } 
    return 0;
}

int cStopCalibrationCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(Cycles);
    int ccp_error;
    char *Help;
    switch (par_Executor->GetData (0)) {
    case STOP_CCP_CMD:
        if (Is_CCP_CommandDone (par_Executor->GetData (), &ccp_error, &Help) != FALSE) {
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("\t\t/* STOP_CALIBRATION() needed %d cycles */\n", par_Executor->GetWaitCyclesNeeded ());
            if (ccp_error) {
                par_Parser->Error (s_main_ini_val.ScriptStopIfCcpError ? SCRIPT_PARSER_FATAL_ERROR : SCRIPT_PARSER_ERROR_CONTINUE, 
                                   "STOP_CALIBRATION command got a protocoll error (%i) = \"%s\"", ccp_error, Help);
            } 
            return 0;
        }
        break;
    case STOP_XCP_CMD:
        if (Is_XCP_CommandDone (par_Executor->GetData (), &ccp_error, &Help) != FALSE) {
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("\t\t/* STOP_CALIBRATION() needed %d cycles */\n", par_Executor->GetWaitCyclesNeeded ());
            if (ccp_error) {
                par_Parser->Error (s_main_ini_val.ScriptStopIfXcpError ? SCRIPT_PARSER_FATAL_ERROR : SCRIPT_PARSER_ERROR_CONTINUE, 
                                   "START_MEASUREMENT command got a protocoll error (%i) = \"%s\"", ccp_error, Help);
            } 
            return 0;
        }
        break;
    case STOP_REF_CMD:
        return 0;
    }
    return 1;
}

static cStopCalibrationCmd StopCalibrationCmd ("STOP_CALIBRATION",                        
                                               2, 
                                               2,  
                                               nullptr,
                                               FALSE, 
                                               TRUE,  
                                               2000,
                                               0,
                                               CMD_INSIDE_ATOMIC_NOT_ALLOWED);




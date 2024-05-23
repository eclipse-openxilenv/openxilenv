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

DEFINE_CMD_CLASS(StartCalibrationCmd)


int StartCalibrationCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

#define START_XCP_CMD  1
#define START_CCP_CMD  2
#define START_AZG_CMD  3
#define START_EDIC_CMD 4
#define START_REF_CMD  5

int StartCalibrationCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int Connection;

    char *ParamPtrs[MAX_PARAMETERS];
    int ParamCount = par_Parser->GetParameterCounter ();
    if (ParamCount > MAX_PARAMETERS) {
        par_Parser->Error (SCRIPT_PARSER_WARNING, "max. %i parameter are allowed inside %s command", MAX_PARAMETERS, GetName());
        ParamCount = MAX_PARAMETERS;
    }
    for (int i = 0; i < ParamCount; i++) {
        ParamPtrs[i] = par_Parser->GetParameter (i);
    }

    if (par_Parser->SolveEquationForParameter (1, &Connection, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (1));
        return -1;
    } 
    if ((Connection < 0) || (Connection >= 4)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_CALIBRATION channel number (%i) out of range (0...3)", Connection);
        return -1;
    }
    par_Executor->SetData (Connection);

    if (!stricmp ("XCP", par_Parser->GetParameter (0))) {
        par_Executor->SetData (0, START_XCP_CMD);
        if (Start_XCP (Connection, START_CALIBRATION, ParamCount - 2, ParamPtrs + 2) != FALSE) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_MEASUREMENT (XCP) not possible ! What about XCP-process ");
            return -1;
        }
    } else if (!stricmp ("CCP", par_Parser->GetParameter (0))) {
        par_Executor->SetData (0, START_CCP_CMD);
        if (Start_CCP (Connection, START_CALIBRATION, ParamCount - 2, ParamPtrs + 2) != FALSE) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_MEASUREMENT (CCP) not possible ! What about CCP-process ");
            return -1;
        }
    } else if (!stricmp ("AZG", par_Parser->GetParameter (0))) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_CALIBRATION (AZG) not possible!");
        return -1;
    } else if (!stricmp ("EDIC", par_Parser->GetParameter (0))) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_CALIBRATION (EDIC) not possible!");
        return -1;
   } else {
        int Pid;
        par_Executor->SetData (0, START_REF_CMD);
        if ((Pid = get_pid_by_name (par_Parser->GetParameter (0))) >= 0) {  // is it a process?
            ScriptAddReferenceVariList (par_Parser->GetParameter (0), Pid, Connection + 0x1000, ParamCount - 2, ParamPtrs + 2);
        } else {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_CALIBRATION (%s) not possible! Allowed are XCP, CCP, AZG, EDIC, or a running process",
                               par_Parser->GetParameter (0));
            return -1;
        
        }
    } 
    return 0;
}

int StartCalibrationCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(Cycles);
    int ccp_error;
    char *Help;
    switch (par_Executor->GetData (0)) {
    case START_CCP_CMD:
        if (Is_CCP_CommandDone (par_Executor->GetData (), &ccp_error, &Help) != FALSE) {
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("\t\t/* START_CALIBRATION() needed %d cycles */\n", par_Executor->GetWaitCyclesNeeded ());
            if (ccp_error) {
                par_Parser->Error (s_main_ini_val.ScriptStopIfCcpError ? SCRIPT_PARSER_FATAL_ERROR : SCRIPT_PARSER_ERROR_CONTINUE, 
                                   "START_CALIBRATION command got a protocoll error (%i) = \"%s\"", ccp_error, Help);
            } 
            return 0;
        }
        break;
    case START_XCP_CMD:
        if (Is_XCP_CommandDone (par_Executor->GetData (), &ccp_error, &Help) != FALSE) {
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("\t\t/* START_CALIBRATION() needed %d cycles */\n", par_Executor->GetWaitCyclesNeeded ());
            if (ccp_error) {
                par_Parser->Error (s_main_ini_val.ScriptStopIfXcpError ? SCRIPT_PARSER_FATAL_ERROR : SCRIPT_PARSER_ERROR_CONTINUE, 
                                   "START_CALIBRATION command got a protocoll error (%i) = \"%s\"", ccp_error, Help);
            } 
            return 0;
        }
        break;
    case START_AZG_CMD:
    case START_EDIC_CMD:
    case START_REF_CMD:
        return 0;
    }
    return 1;
}

static StartCalibrationCmd StartMeasurementCmd ("START_CALIBRATION",                        
                                                 3, 
                                                 MAX_PARAMETERS,
                                                 nullptr,
                                                 FALSE, 
                                                 TRUE,  
                                                 20000,
                                                 0,
                                                 CMD_INSIDE_ATOMIC_NOT_ALLOWED);


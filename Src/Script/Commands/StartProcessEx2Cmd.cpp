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
#include "Config.h"
#include "MyMemory.h"
#include "Files.h"
#include "Scheduler.h"
#include "A2LLink.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStartProcessEx2Cmd)


int cStartProcessEx2Cmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cStartProcessEx2Cmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    int Prio, Cycle, Timeout, Help;
    int Delay;
    const char *SVLFile, *A2LFile, *BBPrefix;
    int UseRangeControl = 0;  // if 0 ignore all folowing parameter
    int RangeControlBeforeActiveFlags = 0;
    int RangeControlBehindActiveFlags = 0;
    int RangeControlStopSchedFlag = 0;
    int RangeControlOutput = 0;
    int RangeErrorCounterFlag = 0;
    int RangeControlVarFlag = 0;
    int RangeControlPhysFlag = 0;
    int RangeControlLimitValues = 0;

    int Offset = 0;
    int A2LFlags = A2L_LINK_NO_FLAGS;

    if (par_Parser->GetParameterCounter () >= 2) {
        if (par_Parser->SolveEquationForParameter (1, &Prio, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
    } else Prio = -1;
    if (par_Parser->GetParameterCounter () >= 3) {
        if (par_Parser->SolveEquationForParameter (2, &Cycle, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
    } else Cycle = -1;
    if (par_Parser->GetParameterCounter () >= 4) {
        if (par_Parser->SolveEquationForParameter (3, &Delay, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
    } else Delay = -1;
    if (par_Parser->GetParameterCounter () >= 5) {
        if (par_Parser->SolveEquationForParameter (4, &Timeout, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
    } else Timeout = -1;
    if (par_Parser->GetParameterCounter () >= 6) {
        SVLFile = par_Parser->GetParameter (5); 
    } else {
        SVLFile = "";
    }
    if (par_Parser->GetParameterCounter () >= 7) {
        A2LFile = par_Parser->GetParameter (6);
    } else {
        A2LFile = "";
    }
    if (par_Parser->GetParameterCounter () >= 8) {
        int x;
        for (x = 0; (par_Parser->GetParameterCounter() > (7 + Offset)) && (x < 6); x++) {
            if (!stricmp(par_Parser->GetParameter (7 + Offset), "A2L_UPDATE")) {
                Offset++;
                A2LFlags |= A2L_LINK_UPDATE_FLAG | A2L_LINK_UPDATE_ZERO_FLAG;
            } else if (!stricmp(par_Parser->GetParameter (7 + Offset), "A2L_DLL_OFFSET")) {
                Offset++;
                A2LFlags |= A2L_LINK_ADDRESS_TRANSLATION_DLL_FLAG;
            } else if (!stricmp(par_Parser->GetParameter (7 + Offset), "A2L_MULTI_DLL_OFFSET")) {
                Offset++;
                A2LFlags |= A2L_LINK_ADDRESS_TRANSLATION_MULTI_DLL_FLAG;
            } else if (!stricmp(par_Parser->GetParameter (7 + Offset), "A2L_REMEMBER_REF_LABELS")) {
                Offset++;
                A2LFlags |= A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG;
            } else if (!stricmp(par_Parser->GetParameter (7 + Offset), "A2L_IGNORE_MOD_COMMON_ALIGNMENTS")) {
                 Offset++;
                 A2LFlags |= A2L_LINK_IGNORE_MOD_COMMON_ALIGNMENTS_FLAG;
             } else if (!stricmp(par_Parser->GetParameter (7 + Offset), "A2L_IGNORE_RECORD_LAYOUT_ALIGNMENTS")) {
                 Offset++;
                 A2LFlags |= A2L_LINK_IGNORE_RECORD_LAYOUT_ALIGNMENTS_FLAG;
            } else if (!stricmp(par_Parser->GetParameter (7 + Offset), "A2L_IGNORE_READ_ONLY")) {
                Offset++;
                A2LFlags |= A2L_LINK_IGNORE_READ_ONLY_FLAG;
            } else{
                break; // for(;;)
            }
        }
    } else {
        A2LFile = "";
    }
    if (par_Parser->GetParameterCounter () >= 8 + Offset) {
        BBPrefix = par_Parser->GetParameter (7 + Offset);
    } else {
        BBPrefix = "";
    }
    if (par_Parser->GetParameterCounter () >= 9 + Offset) {
        UseRangeControl = 1;
        if (par_Parser->GetParameterCounter () < 16 + Offset) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_PROCESS_EX with range control need 15 parameters");
            return -1;
        }
        if (par_Parser->SolveEquationForParameter (8 + Offset, &Help, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
        switch(static_cast<int>(Help)) {
        case 0:
            RangeControlBeforeActiveFlags = 0;
            break;
        case 1:
            RangeControlBeforeActiveFlags = 1;
            break;
        case 3:
            RangeControlBeforeActiveFlags = 3;
            break;
        default:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "range control parameter can only 0, 1, or 3");
            return -1;
        }
        if (par_Parser->SolveEquationForParameter (9 + Offset, &Help, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
        switch(static_cast<int>(Help)) {
        case 0:
            RangeControlBehindActiveFlags = 0;
            break;
        case 1:
            RangeControlBehindActiveFlags = 1;
            break;
        case 3:
            RangeControlBehindActiveFlags = 3;
            break;
        default:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "range control parameter can only 0, 1, or 3");
            return -1;
        }
        if (par_Parser->SolveEquationForParameter (10 + Offset, &Help, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
        RangeControlStopSchedFlag = Help != 0;
        if (par_Parser->SolveEquationForParameter (11 + Offset, &Help, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
        RangeControlOutput = Help & 0xF;
        RangeErrorCounterFlag = strlen (par_Parser->GetParameter (12 + Offset)) != 0;
        RangeControlVarFlag = strlen (par_Parser->GetParameter (13 + Offset)) != 0;
        if (par_Parser->SolveEquationForParameter (14 + Offset, &Help, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
        RangeControlPhysFlag = Help != 0;
        if (par_Parser->SolveEquationForParameter (15 + Offset, &Help, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation contains error ! Perhaps a variable does not exist...");
            return -1;
        }
        RangeControlLimitValues = Help != 0;
    } else {
        UseRangeControl = 0;
    }

    char *ErrMsg = nullptr;
    int ErrCode = start_process_ex (par_Parser->GetParameter (0), Prio, Cycle, Delay, Timeout,
                                    SVLFile,
                                    A2LFile, A2LFlags,
                                    BBPrefix,
                                    UseRangeControl,  // if 0 ignore all folowing range control parameter
                                    RangeControlBeforeActiveFlags,
                                    RangeControlBehindActiveFlags,
                                    RangeControlStopSchedFlag,
                                    RangeControlOutput,
                                    RangeErrorCounterFlag,
                                    par_Parser->GetParameter (11 + Offset),
                                    RangeControlVarFlag,
                                    par_Parser->GetParameter (12 + Offset),
                                    RangeControlPhysFlag,
                                    RangeControlLimitValues,
                                    &ErrMsg);
    if ((ErrCode < 0) &&
        (ErrCode != PROCESS_RUNNING)) {  // 0 -> extern process and > 0 intern process is started, it should be not an error if the process already running.
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Cannot start process \"%s\" %s", par_Parser->GetParameter (0), (ErrMsg != nullptr) ? ErrMsg : "");
        if(ErrMsg != nullptr) activate_extern_process_free_err_msg(ErrMsg);
        return -1;
    }
    return 0;
}

int cStartProcessEx2Cmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cStartProcessEx2Cmd StartProcessEx2Cmd ("START_PROCESS_EX2",
                                             1, 
                                             24,
                                             nullptr,
                                             FALSE, 
                                             FALSE,  
                                             0,
                                             0,
                                             CMD_INSIDE_ATOMIC_ALLOWED);

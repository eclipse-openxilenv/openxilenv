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
#include "Blackboard.h"
#include "BlackboardAccess.h"
}


#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cCallProcCmd)


int cCallProcCmd::SyntaxCheck (cParser *par_Parser)
{
    cProc *Proc = par_Parser->SerchProcByName (par_Parser->GetParameter (0));
    if (Proc == nullptr) {
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "call unknown Proc \"%s\"", par_Parser->GetParameter (0));
        return -1;
    } else {
        if (Proc->GetParamCount () != (par_Parser->GetParameterCounter () - 1)) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "wrong parameter count %i (expecting %i) in proc \"%s\"", 
                               par_Parser->GetParameterCounter () - 1, Proc->GetParamCount (), par_Parser->GetParameter (0));
            return -1;
        }

        for (int x = 1; x < par_Parser->GetParameterCounter (); x++) {  // 1. parameter is the calling procedure
            char *Parameter = par_Parser->GetParameter (x);
            if (*Parameter == '&') {   // it is a reference
                if (Proc->IsParamRefOrValue (x - 1) != 1 /*REFERENCE_PARAMETER*/) {
                    par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "parameter numer %i \"%s\" should be a value and not a reference", x, Parameter); 
                }                   
            } else {
                if (Proc->IsParamRefOrValue (x - 1) != 0 /*VALUE_PARAMETER*/) {
                    par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "parameter numer %i \"%s\" should be a reference and not a value", x, Parameter); 
                }                   
            }
        }
        // The jump target address will be stored inside OptParameter of  the  Cmd table
        par_Parser->SetOptParameter (static_cast<uint32_t>(Proc->GetProcIdx ()), 0);
        return 0;
    }
}

int cCallProcCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int x;

    int CurrentStackPos = par_Executor->Stack.GetCurrentStackPos ();

    // first save the return address on the stack
    par_Executor->Stack.AddProcToStack (par_Executor->GetCurrentProcIdx (), 
                                        par_Executor->GetNextIp (), // return address
                                        par_Executor->GetCurrentProcName ());

    int NewProcIdx = static_cast<int>(par_Executor->GetOptParameterCurrentCmd ());
    cProc *Proc = par_Executor->SetCurrentProcByIdx (NewProcIdx);
    // store the parameter on the stack
    for (x = 0; x < Proc->GetParamCount (); x++) {
        if (Proc->GetParameterType (x) == 0 /*VALUE_PARAMETER*/) {
            double Value;
            if (par_Parser->SolveEquationForParameter (x + 1, &Value, CurrentStackPos)) { // +1 first parameter is the procedure name
                return -1;
            }
            par_Executor->Stack.AddLocalVariableToStack (Proc->GetParameterName (x), Value);
        } else {
            if (par_Parser->GetParameter (x + 1)[0] != '&') {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "expecting a reference and not a value at %i parameter calling procedure %s", x, Proc->GetName ());
                return -1;
            } else {
                int64_t Ref = par_Executor->Stack.GetRefToLocalVariable (par_Parser->GetParameter (x + 1), CurrentStackPos);
                if (Ref > 0) {
                    par_Executor->Stack.AddRefToLocalVariableToStack (Proc->GetParameterName (x), Ref);
                } else {
                    Ref = add_bbvari (par_Parser->GetParameter (x + 1) + 1, BB_UNKNOWN, nullptr);
                    if (Ref < 0) {
                        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot find variable reference %s", par_Parser->GetParameter (x + 1) + 1);
                        return -1;
                    }
                    Ref |= 1ULL << 32;
                    par_Executor->Stack.AddRefToLocalVariableToStack (Proc->GetParameterName (x), Ref);
                }
            }
        }
    }
    return 0;
}

int cCallProcCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cCallProcCmd CallProcCmd ("CALL_PROC",                        
                                 1, 
                                 MAX_PARAMETERS,  
                                 nullptr,
                                 FALSE, 
                                 FALSE,  
                                 0,
                                 0,
                                 CMD_INSIDE_ATOMIC_ALLOWED);

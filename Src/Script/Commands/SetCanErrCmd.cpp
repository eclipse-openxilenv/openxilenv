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


#include <stdint.h>
#include "Platform.h"
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "MyMemory.h"
#include "EquationParser.h"
#include "CanDataBase.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cSetCanErrCmd)


int cSetCanErrCmd::SyntaxCheck (cParser *par_Parser)
{
    if ((par_Parser->GetParameterCounter () != 5) &&
        (par_Parser->GetParameterCounter () != 7)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "expecting 5 or 7 patameters not %i", 
                           par_Parser->GetParameterCounter ());
        return -1;
    }
    return 0;
}

int cSetCanErrCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    double erg;
    int ParamOffset;
    int Channel;
    int Id;
    int Cycles;
    uint64_t BitErrValue;
    char *Ptr;

    if (par_Parser->GetParameterCounter () == 5) {   // Signal name as parameter
        ParamOffset = 0;
    } else if (par_Parser->GetParameterCounter () == 7) {  // Bit position, size, byte order as parameter
        ParamOffset = 2;
    } else {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (0));
        return -1;
    }
    if (par_Parser->SolveEquationForParameter (0, &Channel, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (0));
        return -1;
    }
    if (par_Parser->SolveEquationForParameter (1, &Id, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (1));
        return -1;
    }
    if (par_Parser->SolveEquationForParameter (3 + ParamOffset, &Cycles, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (3 + ParamOffset));
        return -1;
    }
    Ptr = nullptr;
#ifdef _WIN32
    BitErrValue = _strtoui64 (par_Parser->GetParameter (4 + ParamOffset), &Ptr, 0);
#else
    BitErrValue = strtoull (par_Parser->GetParameter (4 + ParamOffset), &Ptr, 0);
#endif
    if ((Ptr == nullptr) || (*Ptr != 0)) {  // There was more than a 64 bit value (equation)
        if (direct_solve_equation_err_sate (par_Parser->GetParameter (4 + ParamOffset), &erg)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                               par_Parser->GetParameter (4 + ParamOffset));
            return -1;
        }
        BitErrValue = static_cast<uint64_t>(erg);
    }
    if (par_Parser->GetParameterCounter () == 5) {   // Signal name as Parameter
        if (ScriptSetCanErrSignalName (Channel, Id, par_Parser->GetParameter (2), static_cast<uint32_t>(Cycles), BitErrValue)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Channel %i not in use or signal \"%s\" not found", 
                               Channel, par_Parser->GetParameter (2));
            return -1;
        }
    } else {  // Bit position, size, byte order as parameter
        int Startbit;
        int Bitsize;
        if (par_Parser->SolveEquationForParameter (2, &Startbit, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                               par_Parser->GetParameter (2));
            return -1;
        }
        if (par_Parser->SolveEquationForParameter (3, &Bitsize, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                               par_Parser->GetParameter (3));
            return -1;
        }
        if (ScriptSetCanErr (Channel, Id, Startbit, Bitsize, par_Parser->GetParameter (4), static_cast<uint32_t>(Cycles), BitErrValue)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Channel %i not in use other wrong parameter"); 
            return -1;
        }
    }
    return 0;
}

int cSetCanErrCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cSetCanErrCmd SetCanErrCmd ("SET_CAN_ERR",                        
                                   5, 
                                   7,  
                                   nullptr,
                                   FALSE, 
                                   FALSE,  
                                   0,
                                   0,
                                   CMD_INSIDE_ATOMIC_ALLOWED);

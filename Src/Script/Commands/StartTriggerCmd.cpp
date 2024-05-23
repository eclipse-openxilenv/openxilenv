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
#include "Message.h"
#include "EquationParser.h"
#include "ProcessEquations.h"
#include "InterfaceToScript.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStartTriggerCmd)


int cStartTriggerCmd::SyntaxCheck (cParser *par_Parser)
{
    int StartFileOffset;
    int StopFileOffset;

    if (par_Parser->HasCmdEmbeddedFile (&StartFileOffset, &StopFileOffset)) {
        switch (par_Parser->GetParameterCounter ()) {
        case 0:
        case 3:
            break;  // OK
        default:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_TRIGGER with embedded file need none or 3 parameters and not %i", 
                               par_Parser->GetParameterCounter ());
            return -1;
        }
    } else {
        switch (par_Parser->GetParameterCounter ()) {
        case 1:
        case 4:
            break;  // OK
        default:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_TRIGGER without embedded file need 1 or 4 parameters and not %i", 
                               par_Parser->GetParameterCounter ());
            return -1;
        }
    }
    return 0;
}

int cStartTriggerCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    const char *Filename;
    int StartFileOffset;
    int StopFileOffset;
    char Path[MAX_PATH];
    int PosOfBeforeBehind;
    int PosOfProccesname;
    char *ErrString = nullptr;


    if (par_Parser->HasCmdEmbeddedFile (&StartFileOffset, &StopFileOffset)) {
        Filename = par_Parser->GetEmbeddedFilename ();
        if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* Embedded file \"%s\" */", Filename);
    } else {
        if (par_Parser->GetParameterCounter () == 1) {
            Filename = strncpy (Path, par_Parser->GetParameter (0), MAX_PATH);
        } else {
            // == 4
            Filename = strncpy (Path, par_Parser->GetParameter (1), MAX_PATH);
        }
        Path[MAX_PATH-1] = 0;
        ScriptIdentifyPath (Path);
    }
    switch (par_Parser->GetParameterCounter ()) {
    case 0:
    case 1:
        if (!par_Parser->get_flag_add_bbvari_automatic ()) {
            par_Parser->SendMessageToProcess (nullptr, 0, PN_EQUATION_COMPILER, TRIGGER_DONT_ADD_NOT_EXITING_VARS);
        }
        par_Parser->SendMessageToProcess (Filename, static_cast<int>(strlen (Filename)) + 1, PN_EQUATION_COMPILER, TRIGGER_FILENAME_MESSAGE);
        SetTimeout (1, 1000);
        return 0;
    case 3:  // (BlockNr, Before/Behind, Processname) {...}
        PosOfBeforeBehind = 1;
        PosOfProccesname = 2;
        break;
    case 4: // (BlockNr, Filename, Before/Behind, Processname)
        PosOfBeforeBehind = 2;
        PosOfProccesname = 3;
        break;
    default:
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "this shoud never happen %s (%i)", __FILE__, __LINE__);
        return -1;
    }
    int EquNr;
    if (par_Parser->SolveEquationForParameter (0, &EquNr, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (0));
        return -1;
    }
    int ReturnValue;
    if (!stricmp ("before", par_Parser->GetParameter (PosOfBeforeBehind))) {
        ReturnValue = AddBeforeProcessEquationFromFile (EquNr,
                                                       par_Parser->GetParameter (PosOfProccesname), 
                                                       Filename, par_Parser->get_flag_add_bbvari_automatic (),
                                                       &ErrString);
    } else if (!stricmp("behind", par_Parser->GetParameter (PosOfBeforeBehind))) {
        ReturnValue = AddBehindProcessEquationFromFile (EquNr,
                                                       par_Parser->GetParameter (PosOfProccesname), 
                                                       Filename, par_Parser->get_flag_add_bbvari_automatic (),
                                                       &ErrString);
    } else {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Only BEFORE or BEHIND is allowed");
        return -1;
    }
    if (ReturnValue) {
        switch (ReturnValue) {
        case -1:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "process name: %s not found",
                              par_Parser->GetParameter (PosOfProccesname));
            return -1;
        case -2:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "file: %s not found", Filename);
            return -1;
        case -3:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "one line exceed 65535 charecters in file: %s", Filename);
            return -1;
        case -4:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "inside equation file \"%s\" are errors:\n%s",
                              Filename, ErrString);
            if (ErrString != nullptr) FreeErrStringBuffer (&ErrString);
            return -1;
        case -5:
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "inside equation file \"%s\" are errors:\n%s",
                              Filename, ErrString);
            if (ErrString != nullptr) FreeErrStringBuffer (&ErrString);
            break;
        default:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unknown error (%i)", ReturnValue);
            return -1;
        }
    }
    SetTimeout (0, 0);

    return 0;
}

int cStartTriggerCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(Cycles);
    MESSAGE_HEAD mhead;
    if (test_message (&mhead)) {
        if (mhead.mid == TRIGGER_ACK) {
            remove_message ();
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("\t\t/* START_TRIGGER() needed %d cycles */\n", par_Executor->GetWaitCyclesNeeded ());
            return 0;
        }
    }
    return 1;
}

static cStartTriggerCmd StartTriggerCmd ("START_TRIGGER",                        
                                         1, 
                                         4,  
                                         "trigger.tri",              
                                         FALSE, 
                                         TRUE,  
                                         1000,
                                         0,
                                         CMD_INSIDE_ATOMIC_ALLOWED);

static cStartTriggerCmd StartTriggerCmd2 ("START_EQUATION",                        
                                          1, 
                                          4,  
                                          "trigger.tri",              
                                          FALSE, 
                                          TRUE,  
                                          1000,
                                          0,
                                          CMD_INSIDE_ATOMIC_ALLOWED);

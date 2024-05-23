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
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cAddBbvariAutomaticCmd)


int cAddBbvariAutomaticCmd::SyntaxCheck (cParser *par_Parser)
{
    if (stricmp (par_Parser->GetParameter (0), "ON") &&
        stricmp (par_Parser->GetParameter (0), "OFF") &&
        stricmp (par_Parser->GetParameter (0), "REMOVE") &&
        stricmp (par_Parser->GetParameter (0), "STOP") &&
        stricmp (par_Parser->GetParameter (0), "REMOVE") &&
        stricmp (par_Parser->GetParameter (0), "OFF_EX") &&
        stricmp (par_Parser->GetParameter (0), "STOP_EX")) {
        par_Parser->Error (SCRIPT_PARSER_WARNING, "ADD_BBVARI_AUTOMATIC \"%s\" not allowed (possible are ON/OFF/REMOVE/STOP) ", par_Parser->GetParameter (0));
    }
    return 0;
}

int cAddBbvariAutomaticCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    if (!stricmp (par_Parser->GetParameter (0), "ON")) {
        par_Parser->set_flag_add_bbvari_automatic (ADD_BBVARI_AUTOMATIC_ON);
    } else if (!stricmp (par_Parser->GetParameter (0), "REMOVE")) {
        par_Parser->set_flag_add_bbvari_automatic (ADD_BBVARI_AUTOMATIC_REMOVE);
    } else if (!stricmp (par_Parser->GetParameter (0), "STOP")) {
        par_Parser->set_flag_add_bbvari_automatic (ADD_BBVARI_AUTOMATIC_STOP);
    } else if (!stricmp (par_Parser->GetParameter (0), "OFF")) {
        par_Parser->set_flag_add_bbvari_automatic (ADD_BBVARI_AUTOMATIC_OFF);
    } else if (!stricmp (par_Parser->GetParameter (0), "STOP_EX")) {
        par_Parser->set_flag_add_bbvari_automatic (ADD_BBVARI_AUTOMATIC_STOP_EX);
    } else if (!stricmp (par_Parser->GetParameter (0), "OFF_EX")) {
        par_Parser->set_flag_add_bbvari_automatic (ADD_BBVARI_AUTOMATIC_OFF_EX);
    } else {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "ADD_BBVARI_AUTOMATIC \"%s\" not allowed (possible are ON/OFF/REMOVE/STOP/OFF_EX/STOP_EX) ", par_Parser->GetParameter (0));
        return -1;
    }
    return 0;
}

int cAddBbvariAutomaticCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cAddBbvariAutomaticCmd AddBbvariAutomaticCmd ("ADD_BBVARI_AUTOMATIC",                        
                                                     1, 
                                                     1,  
                                                     nullptr,
                                                     FALSE, 
                                                     FALSE, 
                                                     0,
                                                     0,
                                                     CMD_INSIDE_ATOMIC_ALLOWED);


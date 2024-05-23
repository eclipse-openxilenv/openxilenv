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
#include "InterfaceToScript.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStartRecorderCmd)


int cStartRecorderCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cStartRecorderCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    const char *Filename;
    int StartFileOffset;
    int StopFileOffset;
    char Path[MAX_PATH];

    if (par_Parser->HasCmdEmbeddedFile (&StartFileOffset, &StopFileOffset)) {
        Filename = par_Parser->GetEmbeddedFilename ();
        if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* Embedded file \"%s\" */", Filename);
    } else {
        Filename = strncpy (Path, par_Parser->GetParameter (0), MAX_PATH);
        Path[MAX_PATH-1] = 0;
        ScriptIdentifyPath (Path);
    }
    par_Parser->SendMessageToProcess (Filename, static_cast<int>(strlen (Filename)) + 1, PN_TRACE_RECORDER, REC_CONFIG_FILENAME_MESSAGE);
    return 0;
}

int cStartRecorderCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(Cycles);
    MESSAGE_HEAD mhead;
    if (test_message (&mhead)) {
        if (mhead.mid == HDREC_ACK) {
            remove_message ();
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("\t\t/* START_REC() needed %d cycles */\n", par_Executor->GetWaitCyclesNeeded ());
            return 0;
        } else {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "got an unkown message %i from %i", mhead.mid, mhead.pid);
            remove_message ();
        }
    }
    return 1;
}

static cStartRecorderCmd StartRecorderCmd ("START_RECORDER",                        
                                           1, 
                                           1,  
                                           "recorder.cfg",              
                                           FALSE, 
                                           TRUE,  
                                           1000,
                                           0,
                                           CMD_INSIDE_ATOMIC_ALLOWED);

static cStartRecorderCmd StartRecorderCmd2 ("START_REC",                        
                                            1, 
                                            1,  
                                            "recorder.cfg",              
                                            FALSE, 
                                            TRUE,  
                                            1000,
                                            0,
                                            CMD_INSIDE_ATOMIC_ALLOWED);

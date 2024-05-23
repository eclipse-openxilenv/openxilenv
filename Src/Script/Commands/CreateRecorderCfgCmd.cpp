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

DEFINE_CMD_CLASS(cCreateRecorderCfgCmd)



int cCreateRecorderCfgCmd::SyntaxCheck (cParser *par_Parser)
{
    int StartFileOffset;
    int StopFileOffset;

    if (!par_Parser->HasCmdEmbeddedFile (&StartFileOffset, &StopFileOffset)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "CREATE_RECORDER_CFG() need an embedded {}-file");
        return -1;
    }
    return 0;
}

int cCreateRecorderCfgCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    const char *FilenameSrc;
    const char *FilenameDst;

    FilenameSrc = par_Parser->GetEmbeddedFilename ();
    FilenameDst = par_Parser->GetParameter (0);
    if (FilenameSrc == nullptr) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot copy file (no valid source file)");
        return -1;
    }
    if (FilenameDst == nullptr) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot copy file (no valid destination file)");
        return -1;
    }
    int errornr = copy_file (FilenameSrc, FilenameDst);
    if (errornr < 0) {
       par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot copy \"%s\" to \"%s\"", FilenameSrc, FilenameDst);
       return -1;
    }
    return 0;
}

int cCreateRecorderCfgCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cCreateRecorderCfgCmd CreateRecorderCfgCmd ("CREATE_RECORDER_CFG",                        
                                           2, // 1 Parameter + IncludedFile
                                           2,
                                           "recorder.cfg",              
                                           FALSE, 
                                           FALSE,
                                           0,
                                           0,
                                           CMD_INSIDE_ATOMIC_ALLOWED);

static cCreateRecorderCfgCmd CreateRecorderCfgCmd2 ("CREATE_REC_CFG",                        
                                            2, // 1 Parameter + IncludedFile
                                            2,
                                            "recorder.cfg",              
                                            FALSE, 
                                            FALSE,
                                            0,
                                            0,
                                            CMD_INSIDE_ATOMIC_ALLOWED);

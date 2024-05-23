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

#include "Config.h"
extern "C" {
#include "MyMemory.h"
#include "ScriptMessageFile.h"
#include "Message.h"
#include "Scheduler.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cRecorderAddCommentCmd)


int cRecorderAddCommentCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    par_Parser->SytaxCheckMessageOutput (0);
    return 0;
}

int cRecorderAddCommentCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    char *MessageBuffer = par_Parser->FormatMessageOutputString (0);
    if (MessageBuffer == nullptr) return -1;
    // remove the "MESSAGE: " prefix
    if (strncmp(MessageBuffer, "MESSAGE: ", 9) == 0) {
        MessageBuffer += 9;
    }
    if (write_message(get_pid_by_name(TRACE_RECORDER), HDREC_COMMENT_MESSAGE, strlen(MessageBuffer) + 1, MessageBuffer)) {
        par_Parser->Error (SCRIPT_PARSER_WARNING, "cannot write comment to recorder");
    }
    return 0;
}

int cRecorderAddCommentCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cRecorderAddCommentCmd RecorderAddCommentCmd ("RECORDER_ADD_COMMENT",
                               0, 
                               MAX_PARAMETERS,
                               nullptr,
                               FALSE, 
                               FALSE,  
                               0,
                               0,
                               CMD_INSIDE_ATOMIC_ALLOWED);

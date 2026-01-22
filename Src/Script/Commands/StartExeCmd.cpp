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
#include <malloc.h>

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Files.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ScriptMessageFile.h"
#include "ScriptStartExe.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStartExeCmd)


int cStartExeCmd::SyntaxCheck (cParser *par_Parser)
{
    if (par_Parser->GetParameterCounter () < 1) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_EXE need at last one parameter");
        return -1;
    }
    return 0;
}

int cStartExeCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    size_t CmdLineLen = 1;
    char *p, *CmdLine;
    // Set ExitCode inside blackboard to 0
    write_bbvari_dword (par_Executor->GetExitCodeVid (), 0);
    
    // close the message file, possible the calling application will need access to this file
    CloseScriptMessageFile (1);  // reopen?
    par_Executor->CloseHtmlReportFile ();

    // Build command line with executable name and all calling parameters
    for (int i = 0; i < par_Parser->GetParameterCounter (); i++) {
        CmdLineLen += strlen (par_Parser->GetParameter (i)) + 3;
    }
    p = CmdLine = static_cast<char*>(_alloca (CmdLineLen));
    for (int i = 0; i < par_Parser->GetParameterCounter (); i++) {
        StringCopyMaxCharTruncate (p, par_Parser->GetParameter (i), CmdLineLen - (p - CmdLine));
        p += strlen (p);
        *p++ = ' ';
    }
    *p = 0;
    // start application
    StartExeFreeProcessHandle(par_Executor->GetStartExeProcessHandle());
    char *ErrMsg;
    HANDLE hProcess = start_exe (CmdLine, &ErrMsg);
    par_Executor->SetStartExeProcessHandle(hProcess);
    if (hProcess == INVALID_HANDLE_VALUE) { // all o.k.
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "START_EXE (%s) \"%s\"", CmdLine, ErrMsg);
        FreeStartExeErrorMsgBuffer(ErrMsg);
        return -1;
    } else {
        return 0;
    }
}

int cStartExeCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cStartExeCmd StartExeCmd ("START_EXE",                        
                                 1, 
                                 MAX_PARAMETERS,
                                 nullptr,
                                 FALSE, 
                                 FALSE,  
                                 0,
                                 0,
                                 CMD_INSIDE_ATOMIC_ALLOWED);

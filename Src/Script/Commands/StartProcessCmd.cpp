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
#include <ctype.h>

extern "C" {
#include "Config.h"
#include "MyMemory.h"
#include "Files.h"
#include "Scheduler.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStartProcessCmd)


int cStartProcessCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

#ifndef _WIN32
char *strupr(char *Src)
{
    char *Dst;
    for (Dst = Src; *Dst!=0; Dst++) {
        *Dst = toupper(*Dst);
    }
    return Src;
}
#endif

int cStartProcessCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    // If there are 2 parameter the second defines a SVL file
    // The SVL file will be loaded directly after starting the process
    if (par_Parser->GetParameterCounter () == 2) {
        const char *p = GetProcessNameFromPath (par_Parser->GetParameter (0));
        if (p != par_Parser->GetParameter (0)) {
            char ProcessName[MAX_PATH];
            strncpy (ProcessName, p, sizeof (ProcessName));
            ProcessName[sizeof (ProcessName)-1] = 0;
#ifdef _WIN32
            strupr (ProcessName);
#endif
            SetSVLFileLoadedBeforeInitProcessFileName (ProcessName, par_Parser->GetParameter (1), 0);             
        } else SetSVLFileLoadedBeforeInitProcessFileName ("", "", 0);
    } else SetSVLFileLoadedBeforeInitProcessFileName ("", "", 0);
    char *ErrMsg = nullptr;
    int ErrCode = start_process_err_msg (par_Parser->GetParameter (0), &ErrMsg);
    if ((ErrCode < 0) &&
        (ErrCode != PROCESS_RUNNING)) {  // 0 -> extern process and > 0 intern process is started, it should be not an error if the process already running.
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Cannot start process \"%s\" %s", par_Parser->GetParameter (0), (ErrMsg != nullptr) ? ErrMsg : "");
        if(ErrMsg != nullptr) activate_extern_process_free_err_msg(ErrMsg);
        return -1;
    }
    return 0;
}

int cStartProcessCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cStartProcessCmd StartProcessCmd ("START_PROCESS",                        
                                         1, 
                                         2,  
                                         nullptr,
                                         FALSE, 
                                         FALSE,  
                                         0,
                                         0,
                                         CMD_INSIDE_ATOMIC_ALLOWED);

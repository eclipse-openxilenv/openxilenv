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
#include "Files.h"
#include "ScriptMessageFile.h"
#include "ScriptHtmlFunctions.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cReportNlCmd)


int cReportNlCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cReportNlCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    // Build message string
    char *ReportNlBuffer = par_Parser->FormatMessageOutputString (1);
    if (ReportNlBuffer == nullptr) return -1;
    if (par_Executor->GetHtmlReportFile () == nullptr) {
        // If output file is not open try to open it
        par_Executor->ReopenHtmlReportFile ();
    }
    if (par_Executor->GetHtmlReportFile () != nullptr) {
        // Add a newline
        fprintf  (par_Executor->GetHtmlReportFile (), "\n");
    }

    return 0;
}

int cReportNlCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cReportNlCmd ReportNlCmd1 ("REPORT_NL",                        
                                  0, 
                                  0,  
                                  nullptr,
                                  FALSE, 
                                  FALSE,  
                                  0,
                                  0,
                                  CMD_INSIDE_ATOMIC_ALLOWED);

static cReportNlCmd ReportNlCmd2 ("REPORT_RAW_NL",                        
                                  0, 
                                  0,  
                                  nullptr,
                                  FALSE, 
                                  FALSE,  
                                  0,
                                  0,
                                  CMD_INSIDE_ATOMIC_ALLOWED);

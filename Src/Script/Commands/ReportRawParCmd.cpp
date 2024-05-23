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

DEFINE_CMD_CLASS(cReportRawParCmd)


int cReportRawParCmd::SyntaxCheck (cParser *par_Parser)
{
    par_Parser->SytaxCheckMessageOutput (0);
    return 0;
}

int cReportRawParCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    // Build message string
    char *ReportRawParBuffer = par_Parser->FormatMessageOutputStringNoPrefix (0);
    if (ReportRawParBuffer == nullptr) return -1;
    if (par_Executor->GetHtmlReportFile () == nullptr) {
        // if the HTML is not open try to open it now
        par_Executor->ReopenHtmlReportFile ();
    }
    if (par_Executor->GetHtmlReportFile () != nullptr) {
        // if the HTML is not open try to open it now
        fprintf  (par_Executor->GetHtmlReportFile (), "%s", ReportRawParBuffer);
    }

    return 0;
}

int cReportRawParCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cReportRawParCmd ReportRawParCmd ("REPORT_RAW_PAR",                        
                                         -2,    // everything up to the first ',' are the first parameter
                                         MAX_PARAMETERS,
                                         nullptr,
                                         FALSE, 
                                         FALSE,  
                                         0,
                                         0,
                                         CMD_INSIDE_ATOMIC_ALLOWED);


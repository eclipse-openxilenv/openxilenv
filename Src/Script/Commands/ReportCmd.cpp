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

DEFINE_CMD_CLASS(cReportCmd)


int cReportCmd::SyntaxCheck (cParser *par_Parser)
{
    par_Parser->SytaxCheckMessageOutput (1);
    return 0;
}

int cReportCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int also_to_msg_file_flag;
#define MAX_ZEILENLAENGE 8192
    char string_before[MAX_ZEILENLAENGE];
    char string_behind[MAX_ZEILENLAENGE];

    also_to_msg_file_flag = get_html_strings (string_before, sizeof(string_before), string_behind, sizeof(string_behind), par_Parser->GetParameter (0));
    // Build message string
    char *ReportBuffer = par_Parser->FormatMessageOutputString (1);
    if (ReportBuffer == nullptr) return -1;
    if (also_to_msg_file_flag) AddScriptMessage (ReportBuffer);
    if (par_Executor->GetHtmlReportFile () == nullptr) {
        // if the HTML is not open try to open it now
        par_Executor->ReopenHtmlReportFile ();
    }
    if (par_Executor->GetHtmlReportFile() != nullptr) {
        // Write message to HTML file
        fprintf(par_Executor->GetHtmlReportFile(), "<p>%s %s %s</p>", string_before, ReportBuffer, string_behind);
    }

    return 0;
}

int cReportCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cReportCmd ReportCmd ("REPORT",                        
                             1, 
                             MAX_PARAMETERS,
                             nullptr,
                             FALSE, 
                             FALSE,  
                             0,
                             0,
                             CMD_INSIDE_ATOMIC_ALLOWED);


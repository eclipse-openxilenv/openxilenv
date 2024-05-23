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
#include <stdlib.h>

extern "C" {
#include "MyMemory.h"
}

#include "Parser.h"
#include "Executor.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cUsingCmd)


int cUsingCmd::SyntaxCheck (cParser *par_Parser)
{
    int StartIp;
    char FilePath[MAX_PATH];
#ifdef _WIN32
    char *FilePart;
    DWORD PathSize;
#else
    int fh;
#endif

#ifdef _WIN32
    PathSize = GetFullPathName (par_Parser->GetParameter (0), MAX_PATH, FilePath, &FilePart);
    if (PathSize == 0) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot open file \"%s\"", par_Parser->GetParameter (0));
        return -1;
    } else if (PathSize >= MAX_PATH) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "path of file \"%s\" excess %i chars", par_Parser->GetParameter (0), MAX_PATH);
        return -1;
    }
    HANDLE hFile = CreateFile (FilePath,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               nullptr,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);

    if (hFile == reinterpret_cast<HANDLE>(HFILE_ERROR)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot open file \"%s\"", par_Parser->GetParameter (0));
        return -1;
    }
    //GetFinalPathNameByHandle (hFile, FilePath, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    CloseHandle (hFile);
#else
    if (realpath(par_Parser->GetParameter (0), FilePath) == nullptr) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot open file \"%s\"", par_Parser->GetParameter (0));
        return -1;
    }
    fh = open (FilePath, O_RDONLY);
    if (fh < 0) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot open file \"%s\"", par_Parser->GetParameter (0));
        return -1;
    }
    close(fh);
#endif

    // wenn File schon Compiliert ist tue nichts
    if (par_Parser->IsFileAlreadyLoadedAndChecked (FilePath, &StartIp)) {
        return 0;
    } else {
        // das File zum Parsen merken
        if (par_Parser->AddToParseFile (FilePath, par_Parser->GetCurrentIp (), static_cast<uint32_t>(par_Parser->GetStartCmdLineNr ()), 1) < 0) {
            return -1;
        }
        // Wechsel zum neuen USING Script-File (aber nur zum Proc-Name parsen)
        if (par_Parser->ChangeToFileIfNotLoadedChecked (FilePath, &StartIp, 1, 1) < 0) {
            return -1;
        } else {
            return 0;
        }
    }
}

int cUsingCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Parser);
    // do nothing
    par_Executor->NextCmdInSameCycle ();
    return 0;
}

int cUsingCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cUsingCmd UsingCmd ("USING",                        
                           1, 
                           1,  
                           nullptr,
                           FALSE, 
                           FALSE,  
                           0,
                           1,  // This command will consider in USING mode
                           CMD_INSIDE_ATOMIC_ALLOWED);    

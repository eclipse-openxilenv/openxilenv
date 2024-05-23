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

DEFINE_CMD_CLASS(cRunCmd)


int cRunCmd::SyntaxCheck (cParser *par_Parser)
{
    int StartIp;
    char FilePath[MAX_PATH];
#ifdef _WIN32
    char *FilePart;
    DWORD PathSize;
#else
    int fh;
#endif

    if (par_Parser->GetNumNoneSolvedEnvVars (0)) {
        // If the parameter has unresolved environment variable throw a warning
        par_Parser->Error (SCRIPT_PARSER_WARNING, "CALL/RUN a script \"%s\" which includes an unknown environment variable at compile time, try to resolve it at run time. This script must have been refered before with USING", par_Parser->GetParameter (0));
        par_Parser->SetOptParameter (0xFFFFFFFFUL, 0);
    } else {
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

        if (par_Parser->IsFileAlreadyLoadedAndChecked (FilePath, &StartIp)) {
            par_Parser->SetOptParameter (static_cast<uint32_t>(StartIp), 0);
        } else {
            if (par_Parser->AddToParseFile (FilePath, par_Parser->GetCurrentIp (), static_cast<uint32_t>(par_Parser->GetStartCmdLineNr ()), 0) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

int cRunCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    par_Executor->Stack.AddRunReturnToScript (par_Executor->GetCurrentIp (), 
                                              par_Parser->GetCurrentScriptFile ()->GetFilename ());
    if (par_Executor->GetOptParameterCurrentCmd () == 0xFFFFFFFFUL) {
        cFileCache *FileCache = cFileCache::IsFileCached (par_Parser->GetParameter (0));
        if (FileCache == nullptr) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot call script \"%s\" the filename include a environment variable but is not refered with USING", par_Parser->GetParameter (0));
            return -1;
        }
        par_Executor->SetNextIp (FileCache->GetIpStart ());
    } else {
        par_Executor->SetNextIp (static_cast<int>(par_Executor->GetOptParameterCurrentCmd ()));
    }
    return 0;
}

int cRunCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cRunCmd RunCmd ("RUN",                        
                       1, 
                       1,  
                       nullptr,
                       FALSE, 
                       FALSE,  
                       0,
                       0,
                       CMD_INSIDE_ATOMIC_ALLOWED);
static cRunCmd CAllCmd ("CALL",                        
                       1, 
                       1,  
                       nullptr,
                       FALSE, 
                       FALSE,  
                       0,
                       0,
                       CMD_INSIDE_ATOMIC_ALLOWED);

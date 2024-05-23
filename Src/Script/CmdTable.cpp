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
#include "Tokenizer.h"
#include "CmdTable.h"


void cCmdTable::DebugPrintTableToFile (char *par_Filename)
{
    FILE *fh;
    int x;

    fh = fopen (par_Filename, "wt");
    if (fh == nullptr) return;
    // First list all script files
    fprintf (fh, "\nAlle geladenen Script-Files:\n");
    for (x = 0; ; x++) {
        cFileCache *FileCache = cFileCache::IsFileCached (x);
        if (FileCache == nullptr) break;
        fprintf (fh, "%04i %s  Ip = %i...%i\n", x, FileCache->GetFilename (), FileCache->GetIpStart (), FileCache->GetIpEnd ());
    }
    fprintf (fh, "\nCommand table size %u\n", PosInTable);
    fprintf (fh, "\nFileNr Ip CmdIdx Command Line FileOffset OptParam Reserved\n");
    for (x = 0; x < static_cast<int>(PosInTable); x++) {
        fprintf (fh, "%02i\t", Table[x].FileNr);
        fprintf (fh, "%06i\t", x);
        fprintf (fh, "%03i\t", static_cast<int>(Table[x].CmdIdx));
        fprintf (fh, "%16s\t", cTokenizer::GetCmdToTokenName (Table[x].CmdIdx));
        fprintf (fh, "%04i\t", Table[x].LineNr);
        fprintf (fh, "%06i\t", Table[x].FileOffset);
        fprintf (fh, "%08i\t", Table[x].OptParameter);
        fprintf (fh, "%08i\t", Table[x].Reserved);
        if (Table[x].BreakPointIdx >= 0) {
            fprintf (fh, "B%i\t", static_cast<int>(Table[x].BreakPointIdx));
        }
        fprintf (fh, "\n");
    }
    fclose (fh);
}


void cCmdTable::OutCommandToFile (FILE *fh, int par_CmdIdx)
{
    if (par_CmdIdx < static_cast<int>(PosInTable)) {
        fputs (cTokenizer::GetCmdToTokenName (Table[par_CmdIdx].CmdIdx), fh);
    } 
}

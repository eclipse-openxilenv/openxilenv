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


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "StringMaxChar.h"
#include "IniDataBase.h"
#include "MyMemory.h"

#include "ReplaceFuncWithProg.h"

int ReplaceFunctionWithProgram (char *Function, int *pRet, int ParamCount, ...)
{
    char *p;
    char *CommandLine;
    size_t Len;
    char ProgramName[MAX_PATH];
    va_list marker;

    
    if (IniFileDataBaseReadString("ReplaceFunctionWithProgramCall", Function, "", ProgramName, sizeof(ProgramName), GetMainFileDescriptor()) > 0) {
        Len = strlen (ProgramName) + 1 + strlen (Function) + 2;
        va_start (marker, ParamCount);     /* Initialize variable arguments. */
        while ((p = va_arg (marker, char*)) != NULL) {
            Len += strlen (p) + 1;
        }
        va_end (marker);              /* Reset variable arguments.      */

        if ((CommandLine = my_malloc (Len)) == NULL) return 0;
        StringCopyMaxCharTruncate (CommandLine, ProgramName, Len);
        StringAppendMaxCharTruncate (CommandLine, " ", Len);
        StringAppendMaxCharTruncate (CommandLine, Function, Len);
        StringAppendMaxCharTruncate (CommandLine, " ", Len);
        va_start (marker, ParamCount);     /* Initialize variable arguments. */
        while ((p = va_arg (marker, char*)) != NULL) {
            StringAppendMaxCharTruncate (CommandLine, p, Len);
            StringAppendMaxCharTruncate (CommandLine, " ", Len);
        }
        va_end (marker);              /* Reset variable arguments.      */
        *pRet = system (CommandLine);
        my_free (CommandLine);
        return 1;
    }
    return 0;
}

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

#include <stdio.h>
#include "PrintFormatToString.h"

int VariableArgumentsListPrintFormatToString(char *ret_DestBuffer, int par_SizeOfDestBuffer, const char *par_Format,  va_list vlist)
{
    int Ret;

    if ((par_SizeOfDestBuffer > 0) &&
        (ret_DestBuffer != NULL) &&
        (par_Format != NULL)) {
#ifdef _WIN32
        Ret = vsnprintf_s (ret_DestBuffer, par_SizeOfDestBuffer, par_SizeOfDestBuffer, par_Format, vlist);
#else
        Ret = vsnprintf (ret_DestBuffer, par_SizeOfDestBuffer, par_Format, vlist);
#endif
        if (Ret < 0) {
            if (ret_DestBuffer != NULL) {
                *ret_DestBuffer = 0;
            }
            Ret = 0;
        } else if (Ret >= par_SizeOfDestBuffer) {
            Ret = par_SizeOfDestBuffer - 1;
        }
    } else {
        Ret = 0;
    }
    return Ret;
}

int PrintFormatToString(char *ret_DestBuffer, int par_SizeOfDestBuffer, const char *par_Format, ...)
{
    va_list args;
    int Ret;

    va_start (args, par_Format);
    if ((par_SizeOfDestBuffer > 0) &&
        (ret_DestBuffer != NULL) &&
        (par_Format != NULL)) {
#ifdef _WIN32
        Ret = vsnprintf_s (ret_DestBuffer, par_SizeOfDestBuffer, par_SizeOfDestBuffer, par_Format, args);
#else
        Ret = vsnprintf (ret_DestBuffer, par_SizeOfDestBuffer, par_Format, args);
#endif
        if (Ret < 0) {
            if (ret_DestBuffer != NULL) {
                *ret_DestBuffer = 0;
            }
            Ret = 0;
        } else if (Ret >= par_SizeOfDestBuffer) {
            Ret = par_SizeOfDestBuffer - 1;
        }
    } else {
        Ret = 0;
    }
    va_end (args);
    return Ret;
}

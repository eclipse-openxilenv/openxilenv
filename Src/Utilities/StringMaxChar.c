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
#include <string.h>
#include <memory.h>

#include "MyMemory.h"
#include "StringMaxChar.h"

void StringCopyMaxCharTruncate(char *par_Dst, const char *par_Src, int par_MaxChar)
{
    if ((par_Dst != NULL) && (par_Src != NULL)) {
        size_t Len = strlen(par_Src) + 1;
        if (Len > (size_t)par_MaxChar) {
            if ( par_MaxChar > 0) {
                int Size = par_MaxChar - 1;
                // truncation
                MEMCPY(par_Dst, par_Src, Size);
                par_Dst[par_MaxChar - 1] = 0;
            }
        } else {
            MEMCPY(par_Dst, par_Src, Len);
        }
    }
}

void StringAppendMaxCharTruncate(char *par_Dst, const char *par_Src, int par_MaxChar)
{
    size_t Len1, Len2;
    if ((par_Dst != NULL) && (par_Src != NULL)) {
        Len1 = strlen(par_Dst);
        Len2 = strlen(par_Src) + 1;
        if ((Len1 + Len2) > (size_t)par_MaxChar) {
            if ((Len1 + 1) < (size_t)par_MaxChar) {
                size_t Size = par_MaxChar - Len1 - 1;
                // truncation
                MEMCPY(par_Dst + Len1, par_Src, Size);
                par_Dst[par_MaxChar - 1] = 0;
            }
        } else {
            MEMCPY(par_Dst + Len1, par_Src, Len2);
        }
    }
}

char *StringMalloc(const char *par_Src)
{
    if (par_Src != NULL) {
        size_t Len = strlen(par_Src) + 1;
        char *Ret = my_malloc(Len);
        if (Ret != NULL) MEMCPY(Ret, par_Src, Len);
        return Ret;
    } else {
        return NULL;
    }
}

char *StringRealloc(char *par_Dst, const char *par_Src)
{
    if (par_Src != NULL) {
        size_t Len = strlen(par_Src) + 1;
        char *Ret = my_realloc(par_Dst, Len);
        if (Ret != NULL) MEMCPY(Ret, par_Src, Len);
        return Ret;
    } else {
        return NULL;
    }
}

void StringFree(const char *par_Src)
{
    my_free(par_Src);
}

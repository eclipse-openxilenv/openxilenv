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
#ifdef REMOTE_MASTER
#define MAX_CONFIGURABLE_PREFIXES  32
static struct {
    char *ConfigurablePrefix[32];
} s_main_ini_val;
#else
#include "MainValues.h"
#endif
#include "ConfigurablePrefix.h"

char *ExtendsWithConfigurablePrefix(enum ConfigurablePrefixType par_PrefixType, const char *par_Src,
                                    char *par_Buffer, int par_BufferSize)
{
    if ((par_PrefixType >=0) && (par_PrefixType < MAX_CONFIGURABLE_PREFIXES) &&
        (s_main_ini_val.ConfigurablePrefix[par_PrefixType] != NULL)) {
        StringCopyMaxCharTruncate(par_Buffer, s_main_ini_val.ConfigurablePrefix[par_PrefixType], par_BufferSize);
        StringAppendMaxCharTruncate(par_Buffer, par_Src, par_BufferSize);
    } else {
        // out of range -> no prefix
        StringCopyMaxCharTruncate(par_Buffer, par_Src, par_BufferSize);
    }
    return par_Buffer;
}

const char *GetConfigurablePrefix(enum ConfigurablePrefixType par_PrefixType)
{
    if ((par_PrefixType >=0) && (par_PrefixType < MAX_CONFIGURABLE_PREFIXES) &&
        (s_main_ini_val.ConfigurablePrefix[par_PrefixType] != NULL)) {
        return s_main_ini_val.ConfigurablePrefix[par_PrefixType];
    } else {
        // out of range -> no prefix
        return "";
    }
}

void RemoteMasterInitConfigurablePrefix(const char *par_BaseAddress, uint32_t *par_OffsetConfigurablePrefix)
{
    int x;

    for (x = 0; x < MAX_CONFIGURABLE_PREFIXES; x++) {
        if (par_OffsetConfigurablePrefix[x] > 0) {
            s_main_ini_val.ConfigurablePrefix[x] = StringMalloc(par_BaseAddress + par_OffsetConfigurablePrefix[x]);
        } else {
            s_main_ini_val.ConfigurablePrefix[x] = NULL;
        }
    }
}


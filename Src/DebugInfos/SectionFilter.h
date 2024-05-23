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


#ifndef SECTIONFILTER_H
#define SECTIONFILTER_H

#include "DebugInfos.h"

typedef struct {
    int NumOfAddrRanges;
    struct {
        uint64_t Start;
        uint64_t End;
    } AddrRages[1];
} SECTION_ADDR_RANGES_FILTER;

typedef struct {
    const char *Section;
    const char *EntryPrefix;
    char *ProcessName;
    const char *DlgName;
    int AllProcsSelected;
    int ProcListEnabled;
} SECTION_FILTER_DLG_PARAM;

SECTION_ADDR_RANGES_FILTER *BuildAddrRangeFilter (PROCESS_APPL_DATA *pappldata, char *Section, char *Prefix, int par_Fd);
SECTION_ADDR_RANGES_FILTER *BuildGlobalAddrRangeFilter (PROCESS_APPL_DATA *pappldata);
    
SECTION_ADDR_RANGES_FILTER *DeleteAddrRangeFilter (SECTION_ADDR_RANGES_FILTER *AddrRanges);

int CheckAddressRangeFilter (uint64_t Address, SECTION_ADDR_RANGES_FILTER *AddrRanges);

void DebugPrintAddressRangeFilter (SECTION_ADDR_RANGES_FILTER *AddrRanges, const char *Filename);

#endif

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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "Platform.h"

#include "MyMemory.h"
#include "ThrowError.h"
#include "Wildcards.h"
#include "DebugInfoDB.h"
#include "Scheduler.h"
#include "IniDataBase.h"

#include "SectionFilter.h"


SECTION_ADDR_RANGES_FILTER *BuildAddrRangeFilter (PROCESS_APPL_DATA *pappldata, char *Section, char *Prefix, int par_Fd)
{
    INCLUDE_EXCLUDE_FILTER *Filter;
    SECTION_ADDR_RANGES_FILTER *AddrRanges;
    int x, xx, MatchCount;
    uint64_t BaseAddress;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    if (pappldata == NULL) return NULL;
    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return NULL;
    }
    BaseAddress = AssociatedProcess->BaseAddress;
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }

    if ((AssociatedDebugInfos->SectionSizeOfRawData == NULL) ||
        (AssociatedDebugInfos->SectionVirtualSize == NULL) ||
        (AssociatedDebugInfos->SectionVirtualAddress == NULL) ||
        (AssociatedDebugInfos->SectionNames == NULL) ||
        (AssociatedDebugInfos->NumOfSections <= 0)) return NULL;

    Filter = BuildIncludeExcludeFilterFromIni (Section, Prefix, par_Fd);
    if (Filter == NULL) return NULL;

    MatchCount = 0;
    for (x = 0; x < AssociatedDebugInfos->NumOfSections; x++) {
        if (CheckIncludeExcludeFilter (AssociatedDebugInfos->SectionNames[x], Filter)) {
            MatchCount++;
        }
    }
    AddrRanges = my_malloc (sizeof (SECTION_ADDR_RANGES_FILTER) + (size_t)MatchCount * sizeof (AddrRanges->AddrRages));
    if (AddrRanges == NULL) {
        FreeIncludeExcludeFilter (Filter);
        ThrowError (1, "out of memory");
        return NULL;
    }
    for (xx = x = 0; x < AssociatedDebugInfos->NumOfSections; x++) {
        if (CheckIncludeExcludeFilter (AssociatedDebugInfos->SectionNames[x], Filter)) {

            AddrRanges->AddrRages[xx].Start = BaseAddress + AssociatedDebugInfos->SectionVirtualAddress[x];
            AddrRanges->AddrRages[xx].End = AddrRanges->AddrRages[xx].Start +
                                            AssociatedDebugInfos->SectionVirtualSize[x];
            xx++;
        }
    }
    AddrRanges->NumOfAddrRanges = xx; 
    FreeIncludeExcludeFilter (Filter);

    return AddrRanges;
}

SECTION_ADDR_RANGES_FILTER *BuildGlobalAddrRangeFilter (PROCESS_APPL_DATA *pappldata)
{
    return BuildAddrRangeFilter (pappldata, "BasicSettings", "GlobalSection_", GetMainFileDescriptor());
}

SECTION_ADDR_RANGES_FILTER *DeleteAddrRangeFilter (SECTION_ADDR_RANGES_FILTER *AddrRanges)
{
    if (AddrRanges != NULL) my_free (AddrRanges);
    return NULL;
}

int CheckAddressRangeFilter (uint64_t Address, SECTION_ADDR_RANGES_FILTER *AddrRanges)
{
    int x;

    // If there is no section filter defined
    if (AddrRanges == NULL) return 1;
    if (!AddrRanges->NumOfAddrRanges) return 1;

    for (x = 0; x < AddrRanges->NumOfAddrRanges; x++) {
        if ((Address  >= AddrRanges->AddrRages[x].Start) &&
            (Address  < AddrRanges->AddrRages[x].End)) {
            return 1;
        }
    }
    return 0;
}

void DebugPrintAddressRangeFilter (SECTION_ADDR_RANGES_FILTER *AddrRanges, const char *Filename)
{
    int x;
    FILE *fh;

    // If there is no section filter defined
    if (AddrRanges == NULL) return;

    fh = fopen(Filename, "wt");
    if (fh != NULL) {
        fprintf (fh, "NumOfAddrRanges = %i\n", AddrRanges->NumOfAddrRanges);

        for (x = 0; x < AddrRanges->NumOfAddrRanges; x++) {
            fprintf (fh, "AddrRages[%i].Start = %" PRIX64 "\n", x, AddrRanges->AddrRages[x].Start);
            fprintf (fh, "AddrRages[%i].End = %" PRIX64 "\n", x, AddrRanges->AddrRages[x].End);
        }
        fclose(fh);
    }
}


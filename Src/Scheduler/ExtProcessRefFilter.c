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
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ThrowError.h"
#include "Files.h"
#include "ReadConfig.h"
#include "ExtProcessRefFilter.h"

typedef struct {
    char *Filter;
    //uint64_t Fill;
} EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT;

typedef struct {
    int Pid;
    int State;
    int Size;
    int Pos;
    EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT *Filter;
} EXTERN_PROCESS_REFERENCE_FILTER;


static EXTERN_PROCESS_REFERENCE_FILTER Filter[MAX_EXT_PROCESS_REF_FILTERS];
static CRITICAL_SECTION FilterCriticalSection;

static int CompareFunction (void const *s1, void const *s2)
{
    const EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT *a1, *a2;
    a1 = (const EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT*)s1;
    a2 = (const EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT*)s2;
    return strcmp(a1->Filter, a2->Filter);
}

static int GetPosOfPid(int par_Pid)
{
    int x;
    for (x = 0; x < MAX_EXT_PROCESS_REF_FILTERS; x++) {
        if (Filter[x].Pid == par_Pid) {
            return x;
        }
    }
    return -1;
}

static int AddPosOfPid(int par_Pid)
{
    int x;
    for (x = 0; x < MAX_EXT_PROCESS_REF_FILTERS; x++) {
        if (Filter[x].Pid == 0) {
            Filter[x].Pid = par_Pid;
            return x;
        }
    }
    return -1;
}

static void ResetExternProcessReferenceFilter(int par_Idx)
{
    int x;

    if (Filter[par_Idx].Filter != NULL) {
        for (x = 0; x < Filter[par_Idx].Pos; x++) {
            if (Filter[par_Idx].Filter[x].Filter != NULL) {
                my_free(Filter[par_Idx].Filter[x].Filter);
            }
        }
        my_free(Filter[par_Idx].Filter);
    }
    Filter[par_Idx].State = 0;
    Filter[par_Idx].Size = 0;
    Filter[par_Idx].Pos = 0;
    Filter[par_Idx].Filter = NULL;
}

int BuildExternProcessReferenceFilter(int par_Pid, const char *par_ProcessPath)
{
    char Label[512];
    int lc = 1;
    int Ret;
    int Idx;

    PrintFormatToString (Label, sizeof(Label), "%s.RefFilter", par_ProcessPath);
    FILE *fh = open_file(Label, "rt");
    if (fh != NULL) {
        EnterCriticalSection(&FilterCriticalSection);
        Idx = AddPosOfPid (par_Pid);
        if (Idx >= 0) {
            ResetExternProcessReferenceFilter (Idx);
            while (read_word(fh, Label, sizeof(Label), &lc) == 0) {
                int Len;
                if (Filter[Idx].Pos >= Filter[Idx].Size) {
                    Filter[Idx].Size += 128 + (Filter[Idx].Size >> 3);
                    Filter[Idx].Filter = (EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT*)my_realloc(Filter[Idx].Filter, Filter[Idx].Size * sizeof(EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT));
                }
                if (Filter[Idx].Filter == NULL) {
                    goto __ERROR_UNLOCK;
                }
                Len = (int)strlen(Label) + 1;
                Filter[Idx].Filter[Filter[Idx].Pos].Filter = (char*)my_malloc(Len);
                if (Filter[Idx].Filter[Filter[Idx].Pos].Filter == NULL) {
                    goto __ERROR_UNLOCK;
                }
                MEMCPY (Filter[Idx].Filter[Filter[Idx].Pos].Filter, Label, Len);
                Filter[Idx].Pos++;
            }
        } else {
            goto __ERROR_UNLOCK;
        }
        close_file(fh);
        qsort(Filter[Idx].Filter, Filter[Idx].Pos, sizeof (EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT), CompareFunction);
        Filter[Idx].State = 1;
        Ret = 0;
        goto __OUT_UNLOCK;
    } else {
        return -1;
    }
__ERROR_UNLOCK:
    Ret = -1;
    ThrowError (1, "out of memory");
__OUT_UNLOCK:
    LeaveCriticalSection(&FilterCriticalSection);
    return Ret;
}

void FreeExternProcessReferenceFilter(int par_Pid)
{
    int Idx;

    EnterCriticalSection(&FilterCriticalSection);

    Idx = GetPosOfPid (par_Pid);
    if (Idx >= 0) {
        Filter[Idx].Pid = 0;
        ResetExternProcessReferenceFilter (Idx);
    }
    LeaveCriticalSection(&FilterCriticalSection);
}


int ExternProcessReferenceMatchFilter (int par_Pid, const char *par_RefName)
{
    int Idx;
    int Ret;

    EnterCriticalSection(&FilterCriticalSection);

    Idx = GetPosOfPid (par_Pid);
    if (Idx >= 0) {
        EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT Ref;
        if (Filter[Idx].State != 1) {
            // Filter are not active
            Ret = 1;
        }
        Ref.Filter = (char*)par_RefName;
        if (bsearch(&Ref, Filter[Idx].Filter, Filter[Idx].Pos, sizeof (EXTERN_PROCESS_REFERENCE_FILTER_ELEMENT), CompareFunction) == NULL) {
            Ret = 0;  // not found
        } else {
            Ret = 1;  // found
        }
    } else {
        Ret = 1;  // Should not happen
    }
    LeaveCriticalSection(&FilterCriticalSection);
    return Ret;
}

void InitExternProcessReferenceFilters(void)
{
    InitializeCriticalSection(&FilterCriticalSection);
}

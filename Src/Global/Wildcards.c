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
#include <string.h>
#include <ctype.h>
#include "Platform.h"
#include "MainValues.h"
#include "IniDataBase.h"
#include "MyMemory.h"
#include "StringMaxChar.h"

#include "RunTimeMeasurement.h"

#include "Wildcards.h"

static int WildcardMatchInternal(const char* par_Pettern, const char* par_String)
{
    const char* Mask;
    const char* Name;

__RESTART:
    Mask = par_Pettern;
    for (Name = par_String; *Name != 0; Name++) {
        if (*Mask == '*') {
            par_Pettern = Mask + 1;
            if (!*par_Pettern) {
                return 1;
            }
            par_String = Name;
            goto __RESTART;
        }
        else if (*Mask != '?') {
            if (*Name != *Mask) {
                par_String++;
                goto __RESTART;
            }
        }
        Mask++;
    }
    while (*Mask == '*') {
        Mask++;
    }
    return (*Mask == 0);
}


int Compare2StringsWithWildcardsAlwaysCaseSensitive(const char *par_String, const char *par_Pattern)
{
    int Ret;

    BEGIN_RUNTIME_MEASSUREMENT ("Compare2StringsWithWildcardsAlwaysCaseSensitive")

    Ret = !WildcardMatchInternal(par_Pattern, par_String);

    END_RUNTIME_MEASSUREMENT

    return Ret;
}

#ifndef _WIN32
static char *_strlwr(char *Src)
{
    char *Dst;
    for (Dst = Src; *Dst!=0; Dst++) {
        *Dst = tolower(*Dst);
    }
    return Src;
}
#endif


int Compare2StringsWithWildcardsCaseSensitive (const char *string, const char *wildcard, int CaseSensetiv)
{
    if (CaseSensetiv) {
        return Compare2StringsWithWildcardsAlwaysCaseSensitive (string, wildcard);
    } else {
        char *s1;
        char *s2;
        s1 = (char*)_alloca(strlen (string)+1);
        strcpy (s1, string);
        _strlwr (s1);
        s2 = (char*)_alloca(strlen (wildcard)+1);
        strcpy (s2, wildcard);
        _strlwr (s2);
        return Compare2StringsWithWildcardsAlwaysCaseSensitive (s1, s2);
    }
}

int Compare2StringsWithWildcards (const char *string, const char *wildcard)
{
    if (!s_main_ini_val.NoCaseSensitiveFilters) { 
        return Compare2StringsWithWildcardsAlwaysCaseSensitive (string, wildcard);
    } else {
        return Compare2StringsWithWildcardsCaseSensitive (string, wildcard, 0);
    }

}

int CheckIncludeExcludeFilter (const char *string, INCLUDE_EXCLUDE_FILTER *Filter)
{
    int i, e;
    int hit;

    if (Filter->MainFilter != NULL) {
         if (Compare2StringsWithWildcards (string, Filter->MainFilter)) { 
             return 0;   // Main filter exists but don't match
         }
    }

    if (Filter->IncludeSize == 0) goto __NO_INCLUDE_FILTER;
    for (i = 0; i < Filter->IncludeSize; i++) {
        if (!Compare2StringsWithWildcards (string, Filter->IncludeArray[i])) {     // Include match
          __NO_INCLUDE_FILTER:
            hit = 1;
            for (e = 0; e < Filter->ExcludeSize; e++) {
                if (!Compare2StringsWithWildcards (string, Filter->ExcludeArray[e])) {     // Exclude match
                    hit = 0;                                                               // therefore no hit
                }
            }
            return hit;      // Include match bur no exclude match therefore it is a hit
        }
    }
    return 0; // No include match therefore no hit
}


int CheckIncludeExcludeFilter_OnlyLabel (const char *Label, INCLUDE_EXCLUDE_FILTER *Filter)
{
    int i, e;
    int hit;

    if (!Filter->LabelComparePosible) return 1;

    if (Filter->MainFilter_OnlyLabel != NULL) {
         if (Compare2StringsWithWildcards (Label, Filter->MainFilter_OnlyLabel)) { 
             return 0;   // Main filter exists but don't match
         }
    }

    if (Filter->IncludeSize == 0) goto __NO_INCLUDE_FILTER;
    for (i = 0; i < Filter->IncludeSize; i++) {
        if (!Compare2StringsWithWildcards (Label, Filter->IncludeArray_OnlyLabel[i])) {     // Include match
          __NO_INCLUDE_FILTER:
            hit = 1;
            for (e = 0; e < Filter->ExcludeSize; e++) {
                if (!Compare2StringsWithWildcards (Label, Filter->ExcludeArray_OnlyLabel[e])) {     // Exclude match
                    hit = 0;                                                               // therefore no hit
                }
            }
            return hit;      // Include match bur no exclude match therefore it is a hit
        }
    }
    return 0; // No include match therefore no hit
}

void FreeIncludeExcludeFilter (INCLUDE_EXCLUDE_FILTER *Filter)
{
    int x;

    if (Filter != NULL) {
        if (Filter->MainFilter != NULL) {
            my_free (Filter->MainFilter);
        }
        if (Filter->IncludeArray != NULL) {
            for (x = 0; x < Filter->IncludeSize; x++) {
                if (Filter->IncludeArray[x] != NULL) my_free (Filter->IncludeArray[x]);
            }
            my_free (Filter->IncludeArray);
        }
        if (Filter->ExcludeArray != NULL) {
            for (x = 0; x < Filter->ExcludeSize; x++) {
                if (Filter->ExcludeArray[x] != NULL) my_free (Filter->ExcludeArray[x]);
            }
            my_free (Filter->ExcludeArray);
        }
        if (Filter->MainFilter_OnlyLabel != NULL) {
            my_free (Filter->MainFilter_OnlyLabel);
        }
        if (Filter->IncludeArray_OnlyLabel != NULL) {
            for (x = 0; x < Filter->IncludeSize; x++) {
                if (Filter->IncludeArray_OnlyLabel[x] != NULL) my_free (Filter->IncludeArray_OnlyLabel[x]);
            }
            my_free (Filter->IncludeArray_OnlyLabel);
        }
        if (Filter->ExcludeArray_OnlyLabel != NULL) {
            for (x = 0; x < Filter->ExcludeSize; x++) {
                if (Filter->ExcludeArray_OnlyLabel[x] != NULL) my_free (Filter->ExcludeArray_OnlyLabel[x]);
            }
            my_free (Filter->ExcludeArray_OnlyLabel);
        }

        my_free (Filter);
    }
}


int CompareIncludeExcludeFilter (INCLUDE_EXCLUDE_FILTER *Filter1, INCLUDE_EXCLUDE_FILTER *Filter2)
{
    int x;

    if ((Filter1 != NULL) && (Filter2 != NULL)) {
        if ((Filter1->MainFilter != NULL) && (Filter2->MainFilter != NULL)) {
            if (strcmp (Filter1->MainFilter, Filter2->MainFilter)) return 1;
        } else if ((Filter1->MainFilter != NULL) || (Filter2->MainFilter != NULL)) {
            return 1;
        }

        if (Filter1->IncludeSize != Filter2->IncludeSize) {
            return 1;
        }
        if ((Filter1->IncludeArray != NULL) && (Filter2->IncludeArray != NULL)) {
            for (x = 0; x < Filter1->IncludeSize; x++) {
                if (strcmp (Filter1->IncludeArray[x], Filter2->IncludeArray[x])) return 1;
            }
        }
        if (Filter1->ExcludeSize != Filter2->ExcludeSize) {
            return 1;
        }
        if ((Filter1->ExcludeArray != NULL) && (Filter2->ExcludeArray != NULL)) {
            for (x = 0; x < Filter1->ExcludeSize; x++) {
                if (strcmp (Filter1->ExcludeArray[x], Filter2->ExcludeArray[x])) return 1;
            }
        }
    } else if ((Filter1 != NULL) || (Filter2 != NULL)) {
        return 1;
    }
    return 0;
}


static char *BuildLabelFilter (char *Filter)
{
    int x;
    char *Ret;

    for (x = 0; (Filter[x] != 0) && (Filter[x] != '.'); x++);
    if (x == 0) return NULL;
    Ret = (char*)my_malloc (x + 2);
    if (Ret == NULL) return NULL;
    MEMCPY (Ret, Filter, (size_t)x);
    Ret[x] = '*';
    Ret[x+1] = 0;
    return Ret;
}


static void BuildOnlyLabelFilter (INCLUDE_EXCLUDE_FILTER *IncExcludeFilter)
{
    int x;

    IncExcludeFilter->IncludeArray_OnlyLabel = (char**)my_calloc ((size_t)IncExcludeFilter->IncludeSize, sizeof (char*));
    IncExcludeFilter->ExcludeArray_OnlyLabel = (char**)my_calloc ((size_t)IncExcludeFilter->ExcludeSize, sizeof (char*));
    if ((IncExcludeFilter->IncludeArray_OnlyLabel == NULL) || (IncExcludeFilter->ExcludeArray_OnlyLabel == NULL)) return;

    IncExcludeFilter->LabelComparePosible = 1;
    if (IncExcludeFilter->MainFilter) {
        IncExcludeFilter->MainFilter_OnlyLabel = BuildLabelFilter (IncExcludeFilter->MainFilter);
        if (IncExcludeFilter->MainFilter_OnlyLabel == NULL) IncExcludeFilter->LabelComparePosible = 0;
    }
    for (x = 0; x < IncExcludeFilter->IncludeSize; x++) {
        IncExcludeFilter->IncludeArray_OnlyLabel[x] = BuildLabelFilter (IncExcludeFilter->IncludeArray[x]);
        if (IncExcludeFilter->IncludeArray_OnlyLabel[x] == NULL) IncExcludeFilter->LabelComparePosible = 0;
    }
    for (x = 0; x < IncExcludeFilter->ExcludeSize; x++) {
        IncExcludeFilter->ExcludeArray_OnlyLabel[x] = BuildLabelFilter (IncExcludeFilter->ExcludeArray[x]);
        if (IncExcludeFilter->ExcludeArray_OnlyLabel[x] == NULL) IncExcludeFilter->LabelComparePosible = 0;
    }
}


INCLUDE_EXCLUDE_FILTER *BuildIncludeExcludeFilterFromIni (const char *Section, const char *Prefix, int par_Fd)
{
    char Buffer[INI_MAX_LINE_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    INCLUDE_EXCLUDE_FILTER *Ret;
    int x;
    
    Ret = (INCLUDE_EXCLUDE_FILTER*)my_calloc (1, sizeof(INCLUDE_EXCLUDE_FILTER));
    if (Ret == NULL) goto __ERROR;
    // Main filter due to historic reasons.
    sprintf (Entry, "%sfilter", Prefix);
    if (IniFileDataBaseReadString (Section, Entry, "", Buffer, sizeof(Buffer), par_Fd)) {
        Ret->MainFilter = (char*)my_malloc (strlen (Buffer) + 1);
        if (Ret->MainFilter == NULL) goto __ERROR;
        strcpy (Ret->MainFilter, Buffer);
    } else {
        Ret->MainFilter = NULL;
    }

    // Calculate the size of the filter
    for (x = 0; x < 100000; x++) {
        sprintf (Entry, "%sIncludeFilter_%i", Prefix, x);
        if (!IniFileDataBaseReadString (Section, Entry, "", Buffer, sizeof(Buffer), par_Fd)) break;
    }
    Ret->IncludeSize = x;
    for (x = 0; x < 100000; x++) {
        sprintf (Entry, "%sExcludeFilter_%i", Prefix, x);
        if (!IniFileDataBaseReadString (Section, Entry, "", Buffer, sizeof(Buffer), par_Fd)) break;
    }
    Ret->ExcludeSize = x;
    Ret->IncludeArray = (char**)my_calloc ((size_t)Ret->IncludeSize, sizeof (char*));
    Ret->ExcludeArray = (char**)my_calloc ((size_t)Ret->ExcludeSize, sizeof (char*));
    if ((Ret->IncludeArray == NULL) || (Ret->ExcludeArray == NULL)) goto __ERROR;
    // Read the filters
    for (x = 0; x < Ret->IncludeSize; x++) {
        sprintf (Entry, "%sIncludeFilter_%i", Prefix, x);
        if (!IniFileDataBaseReadString (Section, Entry, "", Buffer, sizeof(Buffer), par_Fd)) break;
        Ret->IncludeArray[x] = (char*)my_malloc (strlen (Buffer) + 1);
        if (Ret->IncludeArray[x] == NULL) goto __ERROR;
        strcpy (Ret->IncludeArray[x], Buffer);

    }
    for (x = 0; x < Ret->ExcludeSize; x++) {
        sprintf (Entry, "%sExcludeFilter_%i", Prefix, x);
        if (!IniFileDataBaseReadString (Section, Entry, "", Buffer, sizeof(Buffer), par_Fd)) break;
        Ret->ExcludeArray[x] = (char*)my_malloc (strlen (Buffer) + 1);
        if (Ret->ExcludeArray[x] == NULL) goto __ERROR;
        strcpy (Ret->ExcludeArray[x], Buffer);
    }
    BuildOnlyLabelFilter (Ret);
    return Ret;
  __ERROR:
    FreeIncludeExcludeFilter (Ret);
    return NULL;
}


int SaveIncludeExcludeListsToIni (INCLUDE_EXCLUDE_FILTER *Filter, const char *Section, const char *Prefix, int par_Fd)
{
    int i, e;
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Buffer[INI_MAX_LINE_LENGTH];

    if (Filter->MainFilter != NULL) {
        sprintf (Entry, "%sfilter", Prefix);
        IniFileDataBaseWriteString (Section, Entry, Filter->MainFilter, par_Fd);
    }

    for (i = 0; i < 10000; i++) {
        sprintf (Entry, "%sIncludeFilter_%i", Prefix, i);
        if (i < Filter->IncludeSize) {
            IniFileDataBaseWriteString (Section, Entry, Filter->IncludeArray[i], par_Fd);
        } else {
            // It the old list is longer than the new one delete this
            if (IniFileDataBaseReadString (Section, Entry, "", Buffer, sizeof(Buffer), par_Fd)) {
                IniFileDataBaseWriteString (Section, Entry, NULL, par_Fd);   // delete the entry
            } else {
                break;
            }
        }
    }
    for (e = 0; e < 10000; e++) {
        sprintf (Entry, "%sExcludeFilter_%i", Prefix, e);
        if (e < Filter->ExcludeSize) {
            IniFileDataBaseWriteString (Section, Entry, Filter->ExcludeArray[e], par_Fd);
        } else {
            // It the old list is longer than the new one delete this
            if (IniFileDataBaseReadString (Section, Entry, "", Buffer, sizeof(Buffer), par_Fd)) {
                IniFileDataBaseWriteString (Section, Entry, NULL, par_Fd);    // delete the entry
            } else {
                break;
            }
        }
    }
    return 0;
}

void SetMainFilterInsideIncludeExcludeFilter (INCLUDE_EXCLUDE_FILTER *Filter, const char *MainFilter)
{
    if (Filter->MainFilter != NULL) {
        my_free (Filter->MainFilter);
    }
    Filter->MainFilter = my_malloc (strlen (MainFilter) + 1);
    if (Filter->MainFilter != NULL) strcpy (Filter->MainFilter, MainFilter);
}


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


#ifndef WILDCARDS_H
#define WILDCARDS_H

#define MAX_LEV_STRING_LEN 512

int Compare2StringsWithWildcardsAlwaysCaseSensitive(const char *string, const char *wildcard);
//  Compare2StringsWithWildcards will be look in [BasicSettings], "Case sensitive filters" if the filter should be case sensetive
int Compare2StringsWithWildcards (const char *string, const char *wildcard);


typedef struct {
    char *MainFilter;
    char **IncludeArray;   // List of the include rule
    char **ExcludeArray;   // List of the exclude rule
    int IncludeSize;       // size of the include-rule-Liste
    int ExcludeSize;       // size of the exclude-rule-Liste
    int LabelComparePosible;
    char *MainFilter_OnlyLabel;
    char **IncludeArray_OnlyLabel;   // List of the include rule
    char **ExcludeArray_OnlyLabel;   // List of the exclude rule
} INCLUDE_EXCLUDE_FILTER;

INCLUDE_EXCLUDE_FILTER *BuildIncludeExcludeFilterFromIni (const char *Section, const char *Prefix, int par_Fd);

int SaveIncludeExcludeListsToIni (INCLUDE_EXCLUDE_FILTER *Filter, const char *Section, const char *Prefix, int par_Fd);

void FreeIncludeExcludeFilter (INCLUDE_EXCLUDE_FILTER *Filter);

void SetMainFilterInsideIncludeExcludeFilter (INCLUDE_EXCLUDE_FILTER *Filter, const char *MainFilter);

//  Compare2StringsWithWildcards will be look in [BasicSettings], "Case sensitive filters" if the filter should be case sensetive
int CheckIncludeExcludeFilter (const char *string, INCLUDE_EXCLUDE_FILTER *Filter);
int CheckIncludeExcludeFilter_OnlyLabel (const char *Label, INCLUDE_EXCLUDE_FILTER *Filter);


int Compare2StringsWithWildcardsCaseSensitive (const char *string, const char *wildcard, int CaseSensetiv);

int CompareIncludeExcludeFilter (INCLUDE_EXCLUDE_FILTER *Filter1, INCLUDE_EXCLUDE_FILTER *Filter2);

#endif


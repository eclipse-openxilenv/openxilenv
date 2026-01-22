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
#include <string.h>

#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Blackboard.h"
#include "RemoteMasterLock.h"

#include "BlackboardIniCache.h"


typedef struct {
    char *VariableName;
    BBVARI_INI_CACHE_ENTRY *Entry;
} INI_SECTION_ENTRY;


static INI_SECTION_ENTRY *IniVariableSectionCache;
static int IniVariableSectionCachePos;
static int IniVariableSectionCacheSize;

static REMOTE_MASTER_LOCK BlackboardIniCacheCriticalSection;
#define InitializeCriticalSection(x) RemoteMasterInitLock(x)
#define EnterCriticalSection(x)  RemoteMasterLock(x, __LINE__, __FILE__)
#define LeaveCriticalSection(x)  RemoteMasterUnlock(x)


static int32_t GetPosOf(const char *par_VariableName)
{
    int32_t l = 0;
    int32_t r = (int32_t)IniVariableSectionCachePos;
    while (l < r) {
        int32_t m = (l + r) >> 1;
        if (strcmp(IniVariableSectionCache[m].VariableName, par_VariableName) < 0) {
            l = m + 1;
        }
        else {
            r = m;
        }
    }
    return l;    // return Size if Value larger than largestes inside Array
}


static char *ReplaceString(char *Dst, const char *Src)
{
    if (Dst != NULL) {
        my_free(Dst);
        Dst = NULL;
    }
    if (Src != NULL) {
        if (Src[0] != 0) {
            int Len = strlen(Src) + 1;
            Dst = my_malloc(Len);
            if (Dst == NULL) {
                ThrowError(1, "realtime out of memmory");
                return NULL;
            }
            MEMCPY(Dst, Src, Len);
        }
    }
    return Dst;
}

static int CheckAndResize(void)
{
    if (IniVariableSectionCachePos >= IniVariableSectionCacheSize) {
        IniVariableSectionCacheSize += 1024;
        IniVariableSectionCache = my_realloc(IniVariableSectionCache, IniVariableSectionCacheSize * sizeof(INI_SECTION_ENTRY));
        if (IniVariableSectionCache == NULL) {
            ThrowError(1, "realtime out of memmory");
            return -1;
        }
    }
    return 0;
}

static int AddNewElem(int pos, const char *par_VariableName)
{
    int Len;

    IniVariableSectionCachePos++;
    Len = strlen(par_VariableName) + 1;
    IniVariableSectionCache[pos].VariableName = my_malloc(Len);
    if (IniVariableSectionCache[pos].VariableName == NULL) {
        ThrowError(1, "realtime out of memmory");
        return -1;
    }
    MEMCPY(IniVariableSectionCache[pos].VariableName, par_VariableName, Len);
    IniVariableSectionCache[pos].Entry = my_malloc(sizeof(BBVARI_INI_CACHE_ENTRY));
    if (IniVariableSectionCache[pos].Entry == NULL) {
        ThrowError(1, "realtime out of memmory");
        return -1;
    }
    STRUCT_ZERO_INIT(*(IniVariableSectionCache[pos].Entry), BBVARI_INI_CACHE_ENTRY);
    return 0;
}

int BlackboardIniCache_AddEntry(const char *par_VariableName, const char *par_Unit, const char *par_Conversion,
                                double par_Min, double par_Max, double par_Step,
                                uint8_t par_Width, uint8_t par_Prec, uint8_t par_StepType, uint8_t par_ConversionType,
                                uint32_t par_RgbColor, uint32_t par_Flags)
{
    int pos;
    int Len;
    BBVARI_INI_CACHE_ENTRY *Entry;
    int Ret = 0;
    
    RemoteMasterLock(&BlackboardIniCacheCriticalSection, __LINE__, __FILE__);
    
    // Cache leeren
    if ((par_Flags == INI_CACHE_CLEAR) && (par_VariableName == NULL)) {
        int x = 0;
        for (x = 0; x < IniVariableSectionCachePos; x++) {
            if (IniVariableSectionCache[x].VariableName != NULL) my_free(IniVariableSectionCache[x].VariableName);
            if (IniVariableSectionCache[x].Entry != NULL) {
                if (IniVariableSectionCache[x].Entry->Unit) my_free(IniVariableSectionCache[x].Entry->Unit);
                if (IniVariableSectionCache[x].Entry->Conversion) my_free(IniVariableSectionCache[x].Entry->Conversion);
                my_free(IniVariableSectionCache[x].Entry);
            }
            IniVariableSectionCache[x].VariableName = NULL;
            IniVariableSectionCache[x].Entry = NULL;
        }
        IniVariableSectionCachePos = 0;
    } else {
        if (IniVariableSectionCachePos == 0) {
            pos = 0;   // der erste Eintrag
            goto __ADD;
        }
        pos = GetPosOf(par_VariableName);
        if (pos == IniVariableSectionCachePos) {
        __ADD:
            // ans Ende hinzufuegen
            if (CheckAndResize()) {
                Ret = -1;
                goto __OUT;
            }
            if (AddNewElem(pos, par_VariableName)) {
                Ret = -1;
                goto __OUT;
            }
        }
        else {
            if (strcmp(par_VariableName, IniVariableSectionCache[pos].VariableName)) {
                // noch nicht in der Liste -> neues Element einbauen
                if (CheckAndResize()) {
                    Ret = -1;
                    goto __OUT;
                }
                // Verschieben
                memmove(&IniVariableSectionCache[pos + 1],
                    &IniVariableSectionCache[pos],
                    (IniVariableSectionCachePos - (uint32_t)pos) * sizeof(INI_SECTION_ENTRY));
                if (AddNewElem(pos, par_VariableName)) {
                    Ret = -1;
                    goto __OUT;
                }
            }
            else {
                // ueberschreiben
                if (IniVariableSectionCache[pos].Entry->Unit != NULL) my_free(IniVariableSectionCache[pos].Entry->Unit);
                if (IniVariableSectionCache[pos].Entry->Conversion != NULL) my_free(IniVariableSectionCache[pos].Entry->Conversion);
                STRUCT_ZERO_INIT(*(IniVariableSectionCache[pos].Entry), BBVARI_INI_CACHE_ENTRY);
            }
        }
        // neues Element befuellen
        Entry = IniVariableSectionCache[pos].Entry;
        if ((par_Flags & INI_CACHE_ENTRY_FLAG_UNIT) == INI_CACHE_ENTRY_FLAG_UNIT) {
            Entry->Unit = ReplaceString(Entry->Unit, par_Unit);
        }
        if ((par_Flags & INI_CACHE_ENTRY_FLAG_CONVERSION) == INI_CACHE_ENTRY_FLAG_CONVERSION) {
            Entry->Conversion = ReplaceString(Entry->Conversion, par_Conversion);
            Entry->ConversionType = par_ConversionType;
        }
        if ((par_Flags & INI_CACHE_ENTRY_FLAG_MIN) == INI_CACHE_ENTRY_FLAG_MIN) {
            Entry->Min = par_Min;
        }
        if ((par_Flags & INI_CACHE_ENTRY_FLAG_MAX) == INI_CACHE_ENTRY_FLAG_MAX) {
            Entry->Max = par_Max;
        }
        if ((par_Flags & INI_CACHE_ENTRY_FLAG_STEP) == INI_CACHE_ENTRY_FLAG_STEP) {
            Entry->Step = par_Step;
            Entry->StepType = par_StepType;
        }
        if ((par_Flags & INI_CACHE_ENTRY_FLAG_WIDTH_PREC) == INI_CACHE_ENTRY_FLAG_WIDTH_PREC) {
            Entry->Prec = par_Prec;
            Entry->Width = par_Width;
        }
        if ((par_Flags & INI_CACHE_ENTRY_FLAG_COLOR) == INI_CACHE_ENTRY_FLAG_COLOR) {
            Entry->RgbColor = par_RgbColor;
        }
    }
__OUT:
    RemoteMasterUnlock(&BlackboardIniCacheCriticalSection);
    return Ret;
}

static int MyStringCopy(char *Dst, char *Src, int MaxChars)
{
    int Count = 1;
    while ((*Dst != 0) && (Count < MaxChars)) {
        *Dst++ = *Src++;
        Count++;
    }
    *Dst = 0;
    return Count - 1;
}

BBVARI_INI_CACHE_ENTRY *BlackboardIniCache_GetEntryRef(const char *par_VariableName)
{
    int pos;

    pos = GetPosOf(par_VariableName);
    if ((pos < 0) || (pos >= IniVariableSectionCachePos)) {
        return NULL;
    } else {
        if (!strcmp(par_VariableName, IniVariableSectionCache[pos].VariableName)) {
            return IniVariableSectionCache[pos].Entry;
        }
        else {
            return NULL;
        }
    }
}

int BlackboardIniCache_GetNextEntry(int par_Index, char **ret_VariableName, BBVARI_INI_CACHE_ENTRY **ret_Entry)
{
    int Ret;
    if ((par_Index < IniVariableSectionCachePos) && (par_Index >= 0)) {
        *ret_VariableName = IniVariableSectionCache[par_Index].VariableName;
        *ret_Entry = IniVariableSectionCache[par_Index].Entry;
        Ret = par_Index + 1;;
    } else {
        Ret = -1;
    }
    return Ret;
}

void InitBlackboardIniCache(void)
{
    RemoteMasterInitLock(&BlackboardIniCacheCriticalSection);
}

void BlackboardIniCache_Lock(void)
{
    RemoteMasterLock(&BlackboardIniCacheCriticalSection, __LINE__, __FILE__);
}

void BlackboardIniCache_Unlock(void)
{
    RemoteMasterUnlock(&BlackboardIniCacheCriticalSection);
}

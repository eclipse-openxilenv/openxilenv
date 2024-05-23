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


#pragma once

#include "Blackboard.h"


typedef struct {
    /*08*/  char *Unit;
    /*08*/  char *Conversion;   

    // Darstellung:
    /*08*/  double Min;
    /*08*/  double Max;
    /*08*/  double Step;

    /*01*/  uint8_t Width;
    /*01*/  uint8_t Prec;
    /*01*/  uint8_t StepType;
    /*01*/  uint8_t ConversionType;

    /*04*/  uint32_t RgbColor;
} BBVARI_INI_CACHE_ENTRY;


#define INI_CACHE_ENTRY_FLAG_UNIT          0x1
#define INI_CACHE_ENTRY_FLAG_MIN           0x2
#define INI_CACHE_ENTRY_FLAG_MAX           0x4
#define INI_CACHE_ENTRY_FLAG_STEP          0x8
#define INI_CACHE_ENTRY_FLAG_WIDTH_PREC    0x10
#define INI_CACHE_ENTRY_FLAG_CONVERSION    0x20
#define INI_CACHE_ENTRY_FLAG_COLOR         0x40
#define INI_CACHE_ENTRY_FLAG_ALL_INFOS     0x0000FFFFUL
#define INI_CACHE_CLEAR                    0xA5A50000UL

int BlackboardIniCache_AddEntry(const char *par_VariableName, const char *par_Unit, const char *par_Conversion,
                                double par_Min, double par_Max,  double Step, 
                                uint8_t par_Width, uint8_t par_Prec, uint8_t par_StepType, uint8_t par_ConversionType,
                                uint32_t par_RgbColor, uint32_t par_Flags);
BBVARI_INI_CACHE_ENTRY *BlackboardIniCache_GetEntryRef(const char *par_VariableName);
int BlackboardIniCache_GetNextEntry(int par_Index, char **ret_VariableName, BBVARI_INI_CACHE_ENTRY **ret_Entry);

void InitBlackboardIniCache(void);
void BlackboardIniCache_Lock(void);
void BlackboardIniCache_Unlock(void);

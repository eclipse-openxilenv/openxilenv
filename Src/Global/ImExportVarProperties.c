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
#include <string.h>
#include <stdio.h>
#include "MemZeroAndCopy.h"
#include "Blackboard.h"
#include "MainValues.h"
#include "FileExtensions.h"
#include "ThrowError.h"
#include "Wildcards.h"
#include "Scheduler.h"
#include "IniDataBase.h"
#include "MyMemory.h"

#include "ImExportVarProperties.h"

int ImportOneVariablePropertiesFlags (int par_Fd,
                                      char *Variname,
                                      int ImportUnit,
                                      int ImportMinMax,
                                      int ImportConvertion,
                                      int ImportDisplay,
                                      int ImportStep,
                                      int ImportColor)
{
    BB_VARIABLE BbVariElem, BbVariElemOld;
    BB_VARIABLE_ADDITIONAL_INFOS AdditionalInfos, AdditionalInfosOld;
    int Vid;
    char *Properties;

    Properties = (char*)my_malloc (INI_MAX_LONGLINE_LENGTH);
    if (Properties == NULL) {
        return BB_VAR_ADD_INFOS_MEM_ERROR;
    }

    // read old properties
    IniFileDataBaseReadString ("Variables", Variname, "", Properties, INI_MAX_LONGLINE_LENGTH, par_Fd);
    STRUCT_ZERO_INIT (BbVariElemOld, BB_VARIABLE);
    STRUCT_ZERO_INIT (AdditionalInfosOld, BB_VARIABLE_ADDITIONAL_INFOS);
    BbVariElemOld.pAdditionalInfos = &AdditionalInfosOld;
    set_default_varinfo (&BbVariElemOld, BB_UNKNOWN);
    set_varinfo_to_inientrys (&BbVariElemOld, Properties);

    // read new properties
    IniFileDataBaseReadString ("Variables", Variname, "", Properties, INI_MAX_LONGLINE_LENGTH, par_Fd);
    STRUCT_ZERO_INIT (BbVariElem, BB_VARIABLE);
    STRUCT_ZERO_INIT (AdditionalInfos, BB_VARIABLE_ADDITIONAL_INFOS);
    BbVariElem.pAdditionalInfos = &AdditionalInfos;
    set_default_varinfo (&BbVariElem, BB_UNKNOWN);
    set_varinfo_to_inientrys (&BbVariElem, Properties);

    Vid = get_bbvarivid_by_name (Variname);
    // Unit
    if (ImportUnit) {
        if (BBWriteUnit((const char*)BbVariElem.pAdditionalInfos->Unit, &BbVariElemOld) != 0) {
            my_free (Properties);
            // No memory available
            return BB_VAR_ADD_INFOS_MEM_ERROR;
        }
        if (Vid > 0) {
            set_bbvari_unit (Vid, (char*)BbVariElemOld.pAdditionalInfos->Unit);
        }
    }
    // Min. Max.
    if (ImportMinMax) {
        if (Vid > 0) {
            if (BbVariElemOld.pAdditionalInfos->Max <= BbVariElem.pAdditionalInfos->Min) {
                set_bbvari_max (Vid, BbVariElem.pAdditionalInfos->Max);
                set_bbvari_min (Vid, BbVariElem.pAdditionalInfos->Min);
            } else {
                set_bbvari_min (Vid, BbVariElem.pAdditionalInfos->Min);
                set_bbvari_max (Vid, BbVariElem.pAdditionalInfos->Max);
            }
        }
        BbVariElemOld.pAdditionalInfos->Min = BbVariElem.pAdditionalInfos->Min;
        BbVariElemOld.pAdditionalInfos->Max = BbVariElem.pAdditionalInfos->Max;
    }
    // Conversion
    if (ImportConvertion) {
        BbVariElemOld.pAdditionalInfos->Conversion.Type = BbVariElem.pAdditionalInfos->Conversion.Type;
        switch (BbVariElemOld.pAdditionalInfos->Conversion.Type) {
        case BB_CONV_NONE:
            break;
        case BB_CONV_FORMULA:
            if (BBWriteFormulaString((const char*)BbVariElem.pAdditionalInfos->Conversion.Conv.Formula.FormulaString, &BbVariElemOld) != 0) {
                my_free (Properties);
                // No memory available
                return BB_VAR_ADD_INFOS_MEM_ERROR;
            }
            break;
        case BB_CONV_TEXTREP:
            if (BBWriteEnumString((const char*)BbVariElem.pAdditionalInfos->Conversion.Conv.Formula.FormulaString, &BbVariElemOld) != 0) {
                my_free (Properties);
                // No memory available
                return BB_VAR_ADD_INFOS_MEM_ERROR;
            }
        case BB_CONV_FACTOFF:
        case BB_CONV_OFFFACT:
        case BB_CONV_TAB_INTP:
        case BB_CONV_TAB_NOINTP:
        case BB_CONV_RAT_FUNC:
        case BB_CONV_REF:
            // todo
            break;
        }
        if (Vid > 0) {
            set_bbvari_conversion (Vid, BbVariElemOld.pAdditionalInfos->Conversion.Type, (char*)BbVariElemOld.pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
        }
    }
    // Display format
    if (ImportDisplay) {
        BbVariElemOld.pAdditionalInfos->Width = BbVariElem.pAdditionalInfos->Width;
        BbVariElemOld.pAdditionalInfos->Prec = BbVariElem.pAdditionalInfos->Prec;
        if (Vid > 0) {
            set_bbvari_format (Vid, BbVariElemOld.pAdditionalInfos->Width, BbVariElemOld.pAdditionalInfos->Prec);
        }
    }
    // Step width
    if (ImportStep) {
        BbVariElemOld.pAdditionalInfos->StepType = BbVariElem.pAdditionalInfos->StepType;
        BbVariElemOld.pAdditionalInfos->Step = BbVariElem.pAdditionalInfos->Step;
        if (Vid > 0) {
            set_bbvari_step (Vid, BbVariElemOld.pAdditionalInfos->StepType, BbVariElemOld.pAdditionalInfos->Step);
        }
    }
    // Color
    if (ImportColor) {
        BbVariElemOld.pAdditionalInfos->RgbColor = BbVariElem.pAdditionalInfos->RgbColor;
        if (Vid > 0) {
            set_bbvari_color (Vid, (int)BbVariElemOld.pAdditionalInfos->RgbColor);
        }
    }
    write_varinfos_to_ini (&BbVariElemOld, INI_CACHE_ENTRY_FLAG_ALL_INFOS);

    free_all_additionl_info_memorys (&AdditionalInfos);
    free_all_additionl_info_memorys (&AdditionalInfosOld);
    my_free (Properties);
    return 0;
}

int ScriptImportVariablesProperties (char *Filename)
{
    char Variname[INI_MAX_ENTRYNAME_LENGTH];
    int EntryIdx;

    int Fd = IniFileDataBaseOpen(Filename);
    if (Fd <= 0) {
        return -1;
    }
    EntryIdx = 0;
    while ((EntryIdx = IniFileDataBaseGetNextEntryName(EntryIdx, "Variables", Variname, sizeof(Variname), Fd)) >= 0) {
        ImportOneVariablePropertiesFlags (Fd, Variname, 1, 1, 1, 1, 1, 1);
    }
    IniFileDataBaseClose (Fd);
    return 0;
}

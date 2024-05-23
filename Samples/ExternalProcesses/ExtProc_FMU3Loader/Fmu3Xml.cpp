#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#endif
#include "pugixml.hpp"
extern "C" {
#include "XilEnvExtProc.h"
#include "Fmu3Struct.h"
}

#include "Fmu3Xml.h"


#ifdef _WIN32
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif

static int AddModelVariable(FMU *ret_Fmu, 
                            const char *par_Name, 
                            const char *par_Initial,
                            const char *par_Variability,
                            const char *par_Causality, 
                            const char *par_Pausality,
                            const char *par_Description,
                            const char *par_ValueReference,
                            int par_TypeNo,
                            const char *par_TypeString,
                            const char *par_Start,
                            int par_ConversionType,
                            const char *par_ConversionString,
                            double par_Min,
                            double par_Max)
{
    Model_Variables *Variables;
    fmi3Float32 InitValueFloat32;
    fmi3Float64 InitValueFloat64;
    fmi3Int8 InitValueInt8;
    fmi3UInt8 InitValueUInt8;
    fmi3Int16 InitValueInt16;
    fmi3UInt16 InitValueUInt16;
    fmi3Int32 InitValueInt32;
    fmi3UInt32 InitValueUInt32;
    fmi3Int64 InitValueInt64;
    fmi3UInt64 InitValueUInt64;
    fmi3Boolean InitValueBoolean;
    Model_void_Variables *ModelTypeVariables;

    int ByteSize;

    if (!strcmp("parameter", par_Causality)) {
        Variables = &ret_Fmu->Parameters;
    } else if (!strcmp("input", par_Causality)) {
        Variables = &ret_Fmu->Inputs;
    } else if (!strcmp("output", par_Causality)) {
        Variables = &ret_Fmu->Outputs;
    } else if (!strcmp("independent", par_Causality)) {
        Variables = &ret_Fmu->Independent;
    } else if (!strcmp("local", par_Causality) || !strcmp("", par_Causality)) {   // default is local!
        Variables = &ret_Fmu->Locals;
    } else { // all other are not supported
        Variables = NULL;
    }
    if (Variables != NULL) {
        switch (par_TypeNo) {
        case 0:    // 0 -> Float32
            InitValueFloat32 = (fmi3Float32)strtod(par_Start, NULL);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Float32;
            ByteSize = sizeof(fmi3Float32);
            break;
        case 1:    // 1 -> Float64
            InitValueFloat64 = strtod(par_Start, NULL);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Float64;
            ByteSize = sizeof(fmi3Float64);
            break;
        case 2:    // 2 -> Int8
            InitValueInt8 = (fmi3Int8)strtol(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Int8;
            ByteSize = sizeof(fmi3Int8);
            break;
        case 3:    // 3 -> UInt8
            InitValueUInt8 = (fmi3UInt8)strtoul(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->UInt8;
            ByteSize = sizeof(fmi3UInt8);
            break;
        case 4:    // 4 -> Int16
            InitValueInt16 = (fmi3Int16)strtol(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Int16;
            ByteSize = sizeof(fmi3Int16);
            break;
        case 5:    // 5 -> UInt16
            InitValueUInt16 = (fmi3UInt16)strtoul(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->UInt16;
            ByteSize = sizeof(fmi3UInt16);
            break;
        case 6:    // 6 -> Int32
            InitValueInt32 = strtol(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Int32;
            ByteSize = sizeof(fmi3Int32);
            break;
        case 7:    // 7 -> UInt32
            InitValueUInt32 = strtoul(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->UInt32;
            ByteSize = sizeof(fmi3UInt32);
            break;
        case 8:    // 8 -> Int64
            InitValueInt64 = strtoll(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Int64;
            ByteSize = sizeof(fmi3Int64);
            break;
        case 9:    // 9 -> UInt64
            InitValueUInt64 = strtoull(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->UInt64;
            ByteSize = sizeof(fmi3UInt64);
            break;
        case 10:    // 10 -> Boolean
            InitValueBoolean = (strtoul(par_Start, NULL, 0) > 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Boolean;
            ByteSize = sizeof(fmi3Boolean);
            break;
        default:   // all other are not supported
            ModelTypeVariables = NULL;
            ByteSize = 0;
            break;
        }
        if (ModelTypeVariables != NULL) {
            int x = ModelTypeVariables->Count++;
            if (ModelTypeVariables->Count >= ModelTypeVariables->Size) {
                ModelTypeVariables->Size += 10;
                if (((ModelTypeVariables->Names = (char**)realloc(ModelTypeVariables->Names, ModelTypeVariables->Size * sizeof(char*))) == NULL) ||
                    ((ModelTypeVariables->References = (fmi3ValueReference*)realloc(ModelTypeVariables->References, ModelTypeVariables->Size * sizeof(fmi3ValueReference))) == NULL) ||
                    ((ModelTypeVariables->Values = (void*)realloc(ModelTypeVariables->Values, ModelTypeVariables->Size * ByteSize)) == NULL) ||
                    ((ModelTypeVariables->StartValues = (void*)realloc(ModelTypeVariables->StartValues, ModelTypeVariables->Size * ByteSize)) == NULL) ||
                    ((ModelTypeVariables->SoftcarInfos = (AdditionalSoftcarInfos*)realloc(ModelTypeVariables->SoftcarInfos, ModelTypeVariables->Size * sizeof(AdditionalSoftcarInfos))) == NULL)) {
//                    ((ModelTypeVariables->ConversionType = (int*)realloc(ModelTypeVariables->ConversionType, ModelTypeVariables->Size * sizeof(int))) == NULL) ||
//                    ((ModelTypeVariables->ConversionString = (char**)realloc(ModelTypeVariables->ConversionString, ModelTypeVariables->Size * sizeof(char*))) == NULL)) {
                        ThrowError (1, "out of memory");
                        return -1;
                }
            }
            if ((ModelTypeVariables->Names[x] = (char*)malloc (strlen(par_Name) + 1)) == NULL) {
                ThrowError (1, "out of memory");
                return -1;
            }
            strcpy (ModelTypeVariables->Names[x], par_Name);
            ModelTypeVariables->References[x] = strtoul(par_ValueReference, NULL, 0);
            switch(par_TypeNo) {
            case 0:    // 0 -> Float32
                ((fmi3Float32*)ModelTypeVariables->Values)[x] = ((fmi3Float32*)ModelTypeVariables->StartValues)[x] = InitValueFloat32;
                break;
            case 1:    // 1 -> Float64
                ((fmi3Float64*)ModelTypeVariables->Values)[x] = ((fmi3Float64*)ModelTypeVariables->StartValues)[x] = InitValueFloat64;
                break;
            case 2:    // 2 -> Int8
                ((fmi3Int8*)ModelTypeVariables->Values)[x] = ((fmi3Int8*)ModelTypeVariables->StartValues)[x] = InitValueInt8;
                break;
            case 3:    // 2 -> UInt8
                ((fmi3UInt8*)ModelTypeVariables->Values)[x] = ((fmi3UInt8*)ModelTypeVariables->StartValues)[x] = InitValueUInt8;
                break;
            case 4:    // 4 -> Int16
                ((fmi3Int16*)ModelTypeVariables->Values)[x] = ((fmi3Int16*)ModelTypeVariables->StartValues)[x] = InitValueInt16;
                break;
            case 5:    // 5 -> UInt16
                ((fmi3UInt16*)ModelTypeVariables->Values)[x] = ((fmi3UInt16*)ModelTypeVariables->StartValues)[x] = InitValueUInt16;
                break;
            case 6:    // 6 -> Int32
                ((fmi3Int32*)ModelTypeVariables->Values)[x] = ((fmi3Int32*)ModelTypeVariables->StartValues)[x] = InitValueInt32;
                break;
            case 7:    // 7 -> UInt32
                ((fmi3UInt32*)ModelTypeVariables->Values)[x] = ((fmi3UInt32*)ModelTypeVariables->StartValues)[x] = InitValueUInt32;
                break;
            case 8:    // 8 -> Int64
                ((fmi3Int64*)ModelTypeVariables->Values)[x] = ((fmi3Int64*)ModelTypeVariables->StartValues)[x] = InitValueInt64;
                break;
            case 9:    // 9 -> UInt64
                ((fmi3UInt64*)ModelTypeVariables->Values)[x] = ((fmi3UInt64*)ModelTypeVariables->StartValues)[x] = InitValueUInt64;
                break;
            case 10:    // 10 -> Boolean
                ((fmi3Boolean*)ModelTypeVariables->Values)[x] = ((fmi3Boolean*)ModelTypeVariables->StartValues)[x] = InitValueBoolean;
                break;
            default:   // all other are not supported
                break;
            }
            if (((par_ConversionType == 1) || (par_ConversionType == 2)) && (par_ConversionString != NULL)) {
                ModelTypeVariables->SoftcarInfos[x].ConversionType = par_ConversionType;
                if ((ModelTypeVariables->SoftcarInfos[x].ConversionString = (char*)malloc (strlen(par_ConversionString) + 1)) == NULL) {
                    ThrowError (1, "out of memory");
                    return -1;
                }
                strcpy (ModelTypeVariables->SoftcarInfos[x].ConversionString, par_ConversionString);
            } else {
                ModelTypeVariables->SoftcarInfos[x].ConversionType = 0;
                ModelTypeVariables->SoftcarInfos[x].ConversionString = NULL;
            }
            ModelTypeVariables->SoftcarInfos[x].min = par_Min;
            ModelTypeVariables->SoftcarInfos[x].max = par_Max;

            return 0;
        }
    }
    return -1;
}


int ReferenceAllVariablesToBlackboard(FMU *par_Fmu)
{
    int x;

    // todo: this have do be selectable
    //par_Fmu->SyncParametersWithBlackboard = false;
    par_Fmu->SyncLocalsWithBlackboard = true;

    if (par_Fmu->SyncParametersWithBlackboard) {
        // Parameters
        for (x = 0; x < par_Fmu->Parameters.Float32.Count; x++) {
            reference_float_var_flags(par_Fmu->Parameters.Float32.Values + x, par_Fmu->Parameters.Float32.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.Float64.Count; x++) {
            reference_float_var_flags(par_Fmu->Parameters.Float32.Values + x, par_Fmu->Parameters.Float32.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.Int8.Count; x++) {
            reference_byte_var_flags(par_Fmu->Parameters.Int8.Values + x, par_Fmu->Parameters.Int8.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.UInt8.Count; x++) {
            reference_ubyte_var_flags(par_Fmu->Parameters.UInt8.Values + x, par_Fmu->Parameters.UInt8.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.Int16.Count; x++) {
            reference_word_var_flags(par_Fmu->Parameters.Int16.Values + x, par_Fmu->Parameters.Int16.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.UInt16.Count; x++) {
            reference_uword_var_flags(par_Fmu->Parameters.UInt16.Values + x, par_Fmu->Parameters.UInt16.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.Int32.Count; x++) {
            reference_dword_var_flags((WSC_TYPE_INT32*)(par_Fmu->Parameters.Int32.Values) + x, par_Fmu->Parameters.Int32.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.UInt32.Count; x++) {
            reference_udword_var_flags((WSC_TYPE_UINT32*)(par_Fmu->Parameters.UInt32.Values) + x, par_Fmu->Parameters.UInt32.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.Int64.Count; x++) {
            reference_qword_var_flags((long long*)par_Fmu->Parameters.Int64.Values + x, par_Fmu->Parameters.Int64.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.UInt64.Count; x++) {
            reference_uqword_var_flags((unsigned long long*)par_Fmu->Parameters.UInt64.Values + x, par_Fmu->Parameters.UInt64.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.Boolean.Count; x++) {
            switch(sizeof(par_Fmu->Parameters.Boolean.Values[0])) {
            default:
            case 1:
                reference_ubyte_var_flags((unsigned char*)par_Fmu->Parameters.Boolean.Values + x, par_Fmu->Parameters.Boolean.Names[x], "", READ_ONLY_REFERENCE);
                break;
            case 2:
                reference_uword_var_flags((unsigned short*)par_Fmu->Parameters.Boolean.Values + x, par_Fmu->Parameters.Boolean.Names[x], "", READ_ONLY_REFERENCE);
                break;
            case 4:
                reference_udword_var_flags((WSC_TYPE_UINT32*)par_Fmu->Parameters.Boolean.Values + x, par_Fmu->Parameters.Boolean.Names[x], "", READ_ONLY_REFERENCE);
                break;
            case 8:
                reference_uqword_var_flags((unsigned long long*)par_Fmu->Parameters.Boolean.Values + x, par_Fmu->Parameters.Boolean.Names[x], "", READ_ONLY_REFERENCE);
                break;
            }
        }
    }

    // Inputs
    for (x = 0; x < par_Fmu->Inputs.Float32.Count; x++) {
        reference_float_var_all_infos_flags(par_Fmu->Inputs.Float32.Values + x, par_Fmu->Inputs.Float32.Names[x], "", 
                                            par_Fmu->Inputs.Float32.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.Float32.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Inputs.Float32.SoftcarInfos[x].min, par_Fmu->Inputs.Float32.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.Float64.Count; x++) {
        reference_double_var_all_infos_flags(par_Fmu->Inputs.Float64.Values + x, par_Fmu->Inputs.Float64.Names[x], "", 
                                             par_Fmu->Inputs.Float64.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.Float64.SoftcarInfos[x].ConversionString,
                                             par_Fmu->Inputs.Float64.SoftcarInfos[x].min, par_Fmu->Inputs.Float64.SoftcarInfos[x].max, 
                                             WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.Int8.Count; x++) {
        reference_byte_var_all_infos_flags(par_Fmu->Inputs.Int8.Values + x, par_Fmu->Inputs.Int8.Names[x], "", 
                                           par_Fmu->Inputs.Int8.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.Int8.SoftcarInfos[x].ConversionString,
                                           par_Fmu->Inputs.Int8.SoftcarInfos[x].min, par_Fmu->Inputs.Int8.SoftcarInfos[x].max, 
                                           WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.UInt8.Count; x++) {
        reference_ubyte_var_all_infos_flags(par_Fmu->Inputs.UInt8.Values + x, par_Fmu->Inputs.UInt8.Names[x], "", 
                                            par_Fmu->Inputs.UInt8.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.UInt8.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Inputs.UInt8.SoftcarInfos[x].min, par_Fmu->Inputs.UInt8.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.Int16.Count; x++) {
        reference_word_var_all_infos_flags(par_Fmu->Inputs.Int16.Values + x, par_Fmu->Inputs.Int16.Names[x], "", 
                                           par_Fmu->Inputs.Int16.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.Int16.SoftcarInfos[x].ConversionString,
                                           par_Fmu->Inputs.Int16.SoftcarInfos[x].min, par_Fmu->Inputs.Int16.SoftcarInfos[x].max, 
                                           WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.UInt16.Count; x++) {
        reference_uword_var_all_infos_flags(par_Fmu->Inputs.UInt16.Values + x, par_Fmu->Inputs.UInt16.Names[x], "", 
                                            par_Fmu->Inputs.UInt16.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.UInt16.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Inputs.UInt16.SoftcarInfos[x].min, par_Fmu->Inputs.UInt16.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.Int32.Count; x++) {
        reference_dword_var_all_infos_flags((WSC_TYPE_INT32*)(par_Fmu->Inputs.Int32.Values) + x, par_Fmu->Inputs.Int32.Names[x], "", 
                                            par_Fmu->Inputs.Int32.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.Int32.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Inputs.Int32.SoftcarInfos[x].min, par_Fmu->Inputs.Int32.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.UInt32.Count; x++) {
        reference_udword_var_all_infos_flags((WSC_TYPE_UINT32*)(par_Fmu->Inputs.UInt32.Values) + x, par_Fmu->Inputs.UInt32.Names[x], "", 
                                             par_Fmu->Inputs.UInt32.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.UInt32.SoftcarInfos[x].ConversionString,
                                             par_Fmu->Inputs.UInt32.SoftcarInfos[x].min, par_Fmu->Inputs.UInt32.SoftcarInfos[x].max, 
                                             WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.Int64.Count; x++) {
        reference_qword_var_all_infos_flags((long long*)par_Fmu->Inputs.Int64.Values + x, par_Fmu->Inputs.Int64.Names[x], "",
                                            par_Fmu->Inputs.Int64.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.Int64.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Inputs.Int64.SoftcarInfos[x].min, par_Fmu->Inputs.Int64.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.UInt64.Count; x++) {
        reference_uqword_var_all_infos_flags((unsigned long long*)par_Fmu->Inputs.UInt64.Values + x, par_Fmu->Inputs.UInt64.Names[x], "",
                                             par_Fmu->Inputs.UInt64.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.UInt64.SoftcarInfos[x].ConversionString,
                                             par_Fmu->Inputs.UInt64.SoftcarInfos[x].min, par_Fmu->Inputs.UInt64.SoftcarInfos[x].max, 
                                             WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.Boolean.Count; x++) {
        switch(sizeof(par_Fmu->Inputs.Boolean.Values[0])) {
        default:
        case 1:
            reference_ubyte_var_flags((unsigned char*)par_Fmu->Inputs.Boolean.Values + x, par_Fmu->Inputs.Boolean.Names[x], "", READ_ONLY_REFERENCE);
            break;
        case 2:
            reference_uword_var_flags((unsigned short*)par_Fmu->Inputs.Boolean.Values + x, par_Fmu->Inputs.Boolean.Names[x], "", READ_ONLY_REFERENCE);
            break;
        case 4:
            reference_udword_var_flags((WSC_TYPE_UINT32*)par_Fmu->Inputs.Boolean.Values + x, par_Fmu->Inputs.Boolean.Names[x], "", READ_ONLY_REFERENCE);
            break;
        case 8:
            reference_uqword_var_flags((unsigned long long*)par_Fmu->Inputs.Boolean.Values + x, par_Fmu->Inputs.Boolean.Names[x], "", READ_ONLY_REFERENCE);
            break;
        }
    }

    // Outputs
    for (x = 0; x < par_Fmu->Outputs.Float32.Count; x++) {
        reference_float_var_all_infos_flags(par_Fmu->Outputs.Float32.Values + x, par_Fmu->Outputs.Float32.Names[x], "", 
                                            par_Fmu->Outputs.Float32.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.Float32.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Outputs.Float32.SoftcarInfos[x].min, par_Fmu->Outputs.Float32.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.Float64.Count; x++) {
        reference_double_var_all_infos_flags(par_Fmu->Outputs.Float64.Values + x, par_Fmu->Outputs.Float64.Names[x], "", 
                                             par_Fmu->Outputs.Float64.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.Float64.SoftcarInfos[x].ConversionString,
                                             par_Fmu->Outputs.Float64.SoftcarInfos[x].min, par_Fmu->Outputs.Float64.SoftcarInfos[x].max, 
                                             WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.Int8.Count; x++) {
        reference_byte_var_all_infos_flags(par_Fmu->Outputs.Int8.Values + x, par_Fmu->Outputs.Int8.Names[x], "", 
                                           par_Fmu->Outputs.Int8.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.Int8.SoftcarInfos[x].ConversionString,
                                           par_Fmu->Outputs.Int8.SoftcarInfos[x].min, par_Fmu->Outputs.Int8.SoftcarInfos[x].max, 
                                           WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.UInt8.Count; x++) {
        reference_ubyte_var_all_infos_flags(par_Fmu->Outputs.UInt8.Values + x, par_Fmu->Outputs.UInt8.Names[x], "", 
                                            par_Fmu->Outputs.UInt8.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.UInt8.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Outputs.UInt8.SoftcarInfos[x].min, par_Fmu->Outputs.UInt8.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.Int16.Count; x++) {
        reference_word_var_all_infos_flags(par_Fmu->Outputs.Int16.Values + x, par_Fmu->Outputs.Int16.Names[x], "", 
                                           par_Fmu->Outputs.Int16.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.Int16.SoftcarInfos[x].ConversionString,
                                           par_Fmu->Outputs.Int16.SoftcarInfos[x].min, par_Fmu->Outputs.Int16.SoftcarInfos[x].max, 
                                           WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.UInt16.Count; x++) {
        reference_uword_var_all_infos_flags(par_Fmu->Outputs.UInt16.Values + x, par_Fmu->Outputs.UInt16.Names[x], "", 
                                            par_Fmu->Outputs.UInt16.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.UInt16.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Outputs.UInt16.SoftcarInfos[x].min, par_Fmu->Outputs.UInt16.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.Int32.Count; x++) {
        reference_dword_var_all_infos_flags((WSC_TYPE_INT32*)(par_Fmu->Outputs.Int32.Values) + x, par_Fmu->Outputs.Int32.Names[x], "", 
                                            par_Fmu->Outputs.Int32.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.Int32.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Outputs.Int32.SoftcarInfos[x].min, par_Fmu->Outputs.Int32.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.UInt32.Count; x++) {
        reference_udword_var_all_infos_flags((WSC_TYPE_UINT32*)(par_Fmu->Outputs.UInt32.Values) + x, par_Fmu->Outputs.UInt32.Names[x], "", 
                                             par_Fmu->Outputs.UInt32.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.UInt32.SoftcarInfos[x].ConversionString,
                                             par_Fmu->Outputs.UInt32.SoftcarInfos[x].min, par_Fmu->Outputs.UInt32.SoftcarInfos[x].max, 
                                             WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.Int64.Count; x++) {
        reference_qword_var_all_infos_flags((long long*)par_Fmu->Outputs.Int64.Values + x, par_Fmu->Outputs.Int64.Names[x], "",
                                            par_Fmu->Outputs.Int64.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.Int64.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Outputs.Int64.SoftcarInfos[x].min, par_Fmu->Outputs.Int64.SoftcarInfos[x].max, 
                                            WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.UInt64.Count; x++) {
        reference_uqword_var_all_infos_flags((unsigned long long*)par_Fmu->Outputs.UInt64.Values + x, par_Fmu->Outputs.UInt64.Names[x], "",
                                             par_Fmu->Outputs.UInt64.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.UInt64.SoftcarInfos[x].ConversionString,
                                             par_Fmu->Outputs.UInt64.SoftcarInfos[x].min, par_Fmu->Outputs.UInt64.SoftcarInfos[x].max, 
                                             WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.Boolean.Count; x++) {
        switch(sizeof(par_Fmu->Outputs.Boolean.Values[0])) {
        default:
        case 1:
            reference_ubyte_var_flags((unsigned char*)par_Fmu->Outputs.Boolean.Values + x, par_Fmu->Outputs.Boolean.Names[x], "", WRITE_ONLY_REFERENCE);
            break;
        case 2:
            reference_uword_var_flags((unsigned short*)par_Fmu->Outputs.Boolean.Values + x, par_Fmu->Outputs.Boolean.Names[x], "", WRITE_ONLY_REFERENCE);
            break;
        case 4:
            reference_udword_var_flags((WSC_TYPE_UINT32*)par_Fmu->Outputs.Boolean.Values + x, par_Fmu->Outputs.Boolean.Names[x], "", WRITE_ONLY_REFERENCE);
            break;
        case 8:
            reference_uqword_var_flags((unsigned long long*)par_Fmu->Outputs.Boolean.Values + x, par_Fmu->Outputs.Boolean.Names[x], "", WRITE_ONLY_REFERENCE);
            break;
        }
    }

    if (par_Fmu->SyncLocalsWithBlackboard) {
        // Locals (Measurements)
        for (x = 0; x < par_Fmu->Locals.Float32.Count; x++) {
            reference_float_var_all_infos_flags(par_Fmu->Locals.Float32.Values + x, par_Fmu->Locals.Float32.Names[x], "", 
                                                par_Fmu->Locals.Float32.SoftcarInfos[x].ConversionType, par_Fmu->Locals.Float32.SoftcarInfos[x].ConversionString,
                                                par_Fmu->Locals.Float32.SoftcarInfos[x].min, par_Fmu->Locals.Float32.SoftcarInfos[x].max, 
                                                WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.Float64.Count; x++) {
            reference_double_var_all_infos_flags(par_Fmu->Locals.Float64.Values + x, par_Fmu->Locals.Float64.Names[x], "", 
                                                 par_Fmu->Locals.Float64.SoftcarInfos[x].ConversionType, par_Fmu->Locals.Float64.SoftcarInfos[x].ConversionString,
                                                 par_Fmu->Locals.Float64.SoftcarInfos[x].min, par_Fmu->Locals.Float64.SoftcarInfos[x].max, 
                                                 WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.Int8.Count; x++) {
            reference_byte_var_all_infos_flags(par_Fmu->Locals.Int8.Values + x, par_Fmu->Locals.Int8.Names[x], "", 
                                               par_Fmu->Locals.Int8.SoftcarInfos[x].ConversionType, par_Fmu->Locals.Int8.SoftcarInfos[x].ConversionString,
                                               par_Fmu->Locals.Int8.SoftcarInfos[x].min, par_Fmu->Locals.Int8.SoftcarInfos[x].max, 
                                               WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.UInt8.Count; x++) {
            reference_ubyte_var_all_infos_flags(par_Fmu->Locals.UInt8.Values + x, par_Fmu->Locals.UInt8.Names[x], "", 
                                                par_Fmu->Locals.UInt8.SoftcarInfos[x].ConversionType, par_Fmu->Locals.UInt8.SoftcarInfos[x].ConversionString,
                                                par_Fmu->Locals.UInt8.SoftcarInfos[x].min, par_Fmu->Locals.UInt8.SoftcarInfos[x].max, 
                                                WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.Int16.Count; x++) {
            reference_word_var_all_infos_flags(par_Fmu->Locals.Int16.Values + x, par_Fmu->Locals.Int16.Names[x], "", 
                                               par_Fmu->Locals.Int16.SoftcarInfos[x].ConversionType, par_Fmu->Locals.Int16.SoftcarInfos[x].ConversionString,
                                               par_Fmu->Locals.Int16.SoftcarInfos[x].min, par_Fmu->Locals.Int16.SoftcarInfos[x].max, 
                                               WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.UInt16.Count; x++) {
            reference_uword_var_all_infos_flags(par_Fmu->Locals.UInt16.Values + x, par_Fmu->Locals.UInt16.Names[x], "", 
                                                par_Fmu->Locals.UInt16.SoftcarInfos[x].ConversionType, par_Fmu->Locals.UInt16.SoftcarInfos[x].ConversionString,
                                                par_Fmu->Locals.UInt16.SoftcarInfos[x].min, par_Fmu->Locals.UInt16.SoftcarInfos[x].max, 
                                                WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.Int32.Count; x++) {
            reference_dword_var_all_infos_flags((WSC_TYPE_INT32*)(par_Fmu->Locals.Int32.Values) + x, par_Fmu->Locals.Int32.Names[x], "", 
                                                par_Fmu->Locals.Int32.SoftcarInfos[x].ConversionType, par_Fmu->Locals.Int32.SoftcarInfos[x].ConversionString,
                                                par_Fmu->Locals.Int32.SoftcarInfos[x].min, par_Fmu->Locals.Int32.SoftcarInfos[x].max, 
                                                WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.UInt32.Count; x++) {
            reference_udword_var_all_infos_flags((WSC_TYPE_UINT32*)(par_Fmu->Locals.UInt32.Values) + x, par_Fmu->Locals.UInt32.Names[x], "", 
                                                 par_Fmu->Locals.UInt32.SoftcarInfos[x].ConversionType, par_Fmu->Locals.UInt32.SoftcarInfos[x].ConversionString,
                                                 par_Fmu->Locals.UInt32.SoftcarInfos[x].min, par_Fmu->Locals.UInt32.SoftcarInfos[x].max, 
                                                 WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.Int64.Count; x++) {
            reference_qword_var_all_infos_flags((long long*)par_Fmu->Locals.Int64.Values + x, par_Fmu->Locals.Int64.Names[x], "",
                                                par_Fmu->Locals.Int64.SoftcarInfos[x].ConversionType, par_Fmu->Locals.Int64.SoftcarInfos[x].ConversionString,
                                                par_Fmu->Locals.Int64.SoftcarInfos[x].min, par_Fmu->Locals.Int64.SoftcarInfos[x].max, 
                                                WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.UInt64.Count; x++) {
            reference_uqword_var_all_infos_flags((unsigned long long*)par_Fmu->Locals.UInt64.Values + x, par_Fmu->Locals.UInt64.Names[x], "",
                                                 par_Fmu->Locals.UInt64.SoftcarInfos[x].ConversionType, par_Fmu->Locals.UInt64.SoftcarInfos[x].ConversionString,
                                                 par_Fmu->Locals.UInt64.SoftcarInfos[x].min, par_Fmu->Locals.UInt64.SoftcarInfos[x].max, 
                                                 WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.Boolean.Count; x++) {
            switch(sizeof(par_Fmu->Locals.Boolean.Values[0])) {
            default:
            case 1:
                reference_ubyte_var_flags((unsigned char*)par_Fmu->Locals.Boolean.Values + x, par_Fmu->Locals.Boolean.Names[x], "", WRITE_ONLY_REFERENCE);
                break;
            case 2:
                reference_uword_var_flags((unsigned short*)par_Fmu->Locals.Boolean.Values + x, par_Fmu->Locals.Boolean.Names[x], "", WRITE_ONLY_REFERENCE);
                break;
            case 4:
                reference_udword_var_flags((WSC_TYPE_UINT32*)par_Fmu->Locals.Boolean.Values + x, par_Fmu->Locals.Boolean.Names[x], "", WRITE_ONLY_REFERENCE);
                break;
            case 8:
                reference_uqword_var_flags((unsigned long long*)par_Fmu->Locals.Boolean.Values + x, par_Fmu->Locals.Boolean.Names[x], "", WRITE_ONLY_REFERENCE);
                break;
            }
        }
    }
    return 0;
}

struct EnumList {
    char *Name;
    char *Value;
};

int sort_func(const void* s1, const void* s2)
{
    int v1 = atoi(((struct EnumList*)s1)->Value);
    int v2 = atoi(((struct EnumList*)s2)->Value);
    return v1 - v2;
}

char *Return0String(void)
{
    char *Ret;
    Ret = (char*)malloc(1);
    Ret[0] = 0;
    return Ret;
}

char *BuildEnumString(pugi::xml_node &DescNode, const char *declared_type, double *ret_Min, double *ret_Max)
{
    char *Ret;
    if (declared_type != NULL) {
        pugi::xml_node TypeDefinitionsNode = DescNode.child("TypeDefinitions");
        if (TypeDefinitionsNode) {
            pugi::xml_node SimpleTypeNode = TypeDefinitionsNode.find_child_by_attribute("SimpleType", "name", declared_type);
            if (SimpleTypeNode) {
                pugi::xml_node EnumerationNode = SimpleTypeNode.child("Enumeration");
                if (EnumerationNode) {
                    // fist only count the elements 
                    int NumberOfEnums = 0;
                    for (pugi::xml_node ItemNode = EnumerationNode.child("Item"); ItemNode; ItemNode = ItemNode.next_sibling("Item")) {
                        NumberOfEnums++;
                    }
                    // Build a list
                    struct EnumList *List = (struct EnumList*)malloc(sizeof(struct EnumList) * NumberOfEnums);
                    if (List == NULL) return NULL;
                    int ElemCount = 0;
                    for (pugi::xml_node ItemNode = EnumerationNode.child("Item"); ItemNode && (ElemCount < NumberOfEnums); ItemNode = ItemNode.next_sibling("Item"), ElemCount++) {
                        const char *name = ItemNode.attribute("name").value();
                        const char *value = ItemNode.attribute("value").value();
                        List[ElemCount].Name = (char*)malloc (strlen(name)+1);
                        List[ElemCount].Value = (char*)malloc (strlen(value)+1);
                        if ((List[ElemCount].Name == NULL) || (List[ElemCount].Value == NULL)) return NULL;
                        strcpy(List[ElemCount].Name, name);
                        strcpy(List[ElemCount].Value, value);
                    }
                    // now sort the list (dont know if it muss be sorted inside the xml file)
                    qsort(List, ElemCount, sizeof(struct EnumList), sort_func);
                    // now we are save that the first value are the smallest value.

                    *ret_Min = atof(List[0].Value);
                    *ret_Max = atof(List[ElemCount-1].Value);

                    int LineLen = 1;  // last 0 char
                    for (int x = 0; x < ElemCount; x++) {
                        // one entry: Value_Value_"Name";_
                        LineLen += 2 * (int)strlen (List[x].Value);
                        LineLen += (int)strlen (List[x].Name);
                        LineLen += 6; // additional chars
                    }
                    if (LineLen < SC_MAX_CONV_EXT_SIZE) {             
                        Ret = (char*)malloc(LineLen);
                        Ret[0] = 0;
                        for (int x = 0; x < ElemCount; x++) {
                            strcat(Ret, List[x].Value);
                            strcat(Ret, " ");
                            strcat(Ret, List[x].Value);
                            strcat(Ret, " \"");
                            strcat(Ret, List[x].Name);
                            strcat(Ret, "\"; ");

                            free(List[x].Value);
                            free(List[x].Name);
                        }
                    } else {
                        Ret = NULL;
                    }
                    free(List);
                    return Ret;
                }
            }
        }
    }
    *ret_Min = SC_NAN;
    *ret_Max = SC_NAN;
    return NULL;
}


int GetMinMax(pugi::xml_node &DescNode, const char *par_TypeName, const char *declared_type, double *ret_Min, double *ret_Max)
{
    *ret_Min = SC_NAN;
    *ret_Max = SC_NAN;
    if ((par_TypeName != nullptr) && (strlen(par_TypeName) > 0)) {
        pugi::xml_node TypeDefinitionsNode = DescNode.child("TypeDefinitions");
        if (TypeDefinitionsNode) {
            char TypeString[64];
            sprintf(TypeString, "%sType", par_TypeName);
            pugi::xml_node SimpleTypeNode = TypeDefinitionsNode.find_child_by_attribute(TypeString, "name", declared_type);
            if (SimpleTypeNode) {
                pugi::xml_node RealNode = SimpleTypeNode.child("Real");
                if (RealNode) {
                    const char *max = RealNode.attribute("max").value();
                    const char *min = RealNode.attribute("min").value();
                    if ((max != NULL) && (min != NULL)) {
                        *ret_Min = atof(min);
                        *ret_Max = atof(max);
                        return 0;
                    }
                }
            }
        }
    }
    return -1;
}

static int ReadOneVariable(FMU *Fmu, pugi::xml_node ScalNode, pugi::xml_node DescNode, int DataTypeNo, const char *DataTypeString)
{
    int Ret = 0;
    const char *name = ScalNode.attribute("name").value();
    const char *valueReference = ScalNode.attribute("valueReference").value();
    const char *causality = ScalNode.attribute("causality").value();
    const char *start = ScalNode.attribute("start").value();
    const char *variability = ScalNode.attribute("variability").value();
    const char *pausality = ScalNode.attribute("pausality").value();
    const char *description = ScalNode.attribute("description").value();
    const char *declaredType = ScalNode.attribute("declaredType").value();
    double min, max;

    GetMinMax(DescNode, DataTypeString, declaredType, &min, &max);

    //char *enum_string = BuildEnumString(DescNode, declared_type, &min, &max);

    Ret = AddModelVariable(Fmu, name, start, variability, causality, pausality, description, valueReference, DataTypeNo, DataTypeString, start, 0, NULL, min, max);
    return Ret;
}

static int ReadAllVariablesOfOneTypVariable(FMU *Fmu, pugi::xml_node DescNode, pugi::xml_node VarNode, int par_TypeNo, const char *par_TypeString)
{
    int Ret = 0;
    for (pugi::xml_node ScalNode = VarNode.child(par_TypeString); ScalNode; ScalNode = ScalNode.next_sibling(par_TypeString)) {
        Ret = ReadOneVariable(Fmu, ScalNode, DescNode, par_TypeNo, par_TypeString);
        if (Ret) break;
    }
    return Ret;
}

int ReadFmuXml(FMU *ret_Fmu, const char *par_XmlFileName)
{
    pugi::xml_document doc;
    int Ret = 0;

    pugi::xml_parse_result result = doc.load_file(par_XmlFileName);
    pugi::xml_node DescNode = doc.child("fmiModelDescription");

    if (strcmp (DescNode.attribute("fmiVersion").value(), "3.0")) {
        ThrowError (1, "only FMI version 3.0 are supported");
        return -1;
    }

    const char *instantiationToken = DescNode.attribute("instantiationToken").value();
    strncpy (ret_Fmu->instantiationToken, instantiationToken, 63);
    const char *modelName = DescNode.attribute("modelName").value();
    ret_Fmu->ModelName = (char*)malloc (strlen(modelName) + 1);
    strcpy(ret_Fmu->ModelName, modelName);
    
    pugi::xml_node CoSimulationNode = DescNode.child("CoSimulation");
    const char *modelIdentifier = CoSimulationNode.attribute("modelIdentifier").value();
    ret_Fmu->ModelIdentifier = (char*)malloc (strlen(modelIdentifier) + 1);
    strcpy(ret_Fmu->ModelIdentifier, modelIdentifier);

    pugi::xml_node VarNode = DescNode.child("ModelVariables");

    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 0, "Float32")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 1, "Float64")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 2, "Int8")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 3, "UInt8")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 4, "Int16")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 5, "UInt16")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 6, "Int32")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 7, "UInt32")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 8, "Int64")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 9, "UInt64")) return -1;
    if (ReadAllVariablesOfOneTypVariable(ret_Fmu, DescNode, VarNode, 10, "Boolean")) return -1;
    return Ret;
}

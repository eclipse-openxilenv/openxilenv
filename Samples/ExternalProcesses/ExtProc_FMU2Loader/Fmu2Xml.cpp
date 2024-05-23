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
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#endif
#include "pugixml.hpp"
extern "C" {
#include "XilEnvExtProc.h"
#include "Fmu2Struct.h"
}

#include "Fmu2Xml.h"

static int AddModelVariable(FMU *ret_Fmu, 
                            const char *par_Name, 
                            const char *par_Initial,
                            const char *par_Variability,
                            const char *par_Causality, 
                            const char *par_Pausality,
                            const char *par_Description,
                            const char *par_ValueReference,
                            int par_Type,
                            const char *par_Start,
                            int par_ConversionType,
                            const char *par_ConversionString,
                            double par_Min,
                            double par_Max)
{
    Model_Variables *Variables;
    fmi2Real InitValueReal;
    fmi2Integer InitValueInteger;
    fmi2Boolean InitValueBoolean;
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
        switch (par_Type) {
        case 0:    // 0 -> Real
            InitValueReal = strtod(par_Start, NULL);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Real;
            ByteSize = sizeof(fmi2Real);
            break;
        case 1:    // 1 -> Integer
            InitValueInteger = strtol(par_Start, NULL, 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Integer;
            ByteSize = sizeof(fmi2Integer);
            break;
        case 2:    // 2 -> Boolean
            InitValueBoolean = (strtoul(par_Start, NULL, 0) > 0);
            ModelTypeVariables = (Model_void_Variables*)&Variables->Boolean;
            ByteSize = sizeof(fmi2Boolean);
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
                    ((ModelTypeVariables->References = (fmi2ValueReference*)realloc(ModelTypeVariables->References, ModelTypeVariables->Size * sizeof(fmi2ValueReference))) == NULL) ||
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
            switch(par_Type) {
            case 0:    // 0 -> Real
                ((fmi2Real*)ModelTypeVariables->Values)[x] = ((fmi2Real*)ModelTypeVariables->StartValues)[x] = InitValueReal;
                break;
            case 1:    // 1 -> Integer
                ((fmi2Integer*)ModelTypeVariables->Values)[x] = ((fmi2Integer*)ModelTypeVariables->StartValues)[x] = InitValueInteger;
                break;
            case 2:    // 2 -> Boolean
                ((fmi2Boolean*)ModelTypeVariables->Values)[x] = ((fmi2Boolean*)ModelTypeVariables->StartValues)[x] = InitValueBoolean;
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
    par_Fmu->SyncParametersWithBlackboard = false;
    par_Fmu->SyncLocalsWithBlackboard = true;

    if (par_Fmu->SyncParametersWithBlackboard) {
        // Parameters
        for (x = 0; x < par_Fmu->Parameters.Real.Count; x++) {
            reference_double_var_flags(par_Fmu->Parameters.Real.Values + x, par_Fmu->Parameters.Real.Names[x], "", READ_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Parameters.Integer.Count; x++) {
            reference_dword_var_flags((WSC_TYPE_INT32*)par_Fmu->Parameters.Integer.Values + x, par_Fmu->Parameters.Integer.Names[x], "", READ_ONLY_REFERENCE);
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
    for (x = 0; x < par_Fmu->Inputs.Real.Count; x++) {
        reference_double_var_all_infos_flags(par_Fmu->Inputs.Real.Values + x, par_Fmu->Inputs.Real.Names[x], "", 
                                             par_Fmu->Inputs.Real.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.Real.SoftcarInfos[x].ConversionString,
                                             par_Fmu->Inputs.Real.SoftcarInfos[x].min, par_Fmu->Inputs.Real.SoftcarInfos[x].max, 
                                             WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, READ_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Inputs.Integer.Count; x++) {
        reference_dword_var_all_infos_flags((WSC_TYPE_INT32*)par_Fmu->Inputs.Integer.Values + x, par_Fmu->Inputs.Integer.Names[x], "", 
                                            par_Fmu->Inputs.Integer.SoftcarInfos[x].ConversionType, par_Fmu->Inputs.Integer.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Inputs.Integer.SoftcarInfos[x].min, par_Fmu->Inputs.Integer.SoftcarInfos[x].max, 
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
    for (x = 0; x < par_Fmu->Outputs.Real.Count; x++) {
        reference_double_var_all_infos_flags(par_Fmu->Outputs.Real.Values + x, par_Fmu->Outputs.Real.Names[x], "", 
                                             par_Fmu->Outputs.Real.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.Real.SoftcarInfos[x].ConversionString,
                                             par_Fmu->Outputs.Real.SoftcarInfos[x].min, par_Fmu->Outputs.Real.SoftcarInfos[x].max, 
                                             WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
    }
    for (x = 0; x < par_Fmu->Outputs.Integer.Count; x++) {
        reference_dword_var_all_infos_flags((WSC_TYPE_INT32*)par_Fmu->Outputs.Integer.Values + x, par_Fmu->Outputs.Integer.Names[x], "", 
                                            par_Fmu->Outputs.Integer.SoftcarInfos[x].ConversionType, par_Fmu->Outputs.Integer.SoftcarInfos[x].ConversionString,
                                            par_Fmu->Outputs.Integer.SoftcarInfos[x].min, par_Fmu->Outputs.Integer.SoftcarInfos[x].max, 
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
        for (x = 0; x < par_Fmu->Locals.Real.Count; x++) {
            reference_double_var_all_infos_flags(par_Fmu->Locals.Real.Values + x, par_Fmu->Locals.Real.Names[x], "", 
                                                 par_Fmu->Locals.Real.SoftcarInfos[x].ConversionType, par_Fmu->Locals.Real.SoftcarInfos[x].ConversionString,
                                                 par_Fmu->Locals.Real.SoftcarInfos[x].min, par_Fmu->Locals.Real.SoftcarInfos[x].max, 
                                                 WIDTH_UNDEFINED, PREC_UNDEFINED, COLOR_UNDEFINED, STEPS_UNDEFINED, 0.0, WRITE_ONLY_REFERENCE);
        }
        for (x = 0; x < par_Fmu->Locals.Integer.Count; x++) {
            reference_dword_var_all_infos_flags((WSC_TYPE_INT32*)par_Fmu->Locals.Integer.Values + x, par_Fmu->Locals.Integer.Names[x], "", 
                                                par_Fmu->Locals.Integer.SoftcarInfos[x].ConversionType, par_Fmu->Locals.Integer.SoftcarInfos[x].ConversionString,
                                                par_Fmu->Locals.Integer.SoftcarInfos[x].min, par_Fmu->Locals.Integer.SoftcarInfos[x].max, 
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


int GetMinMax(pugi::xml_node &DescNode, pugi::xml_node &TypeNode, double *ret_Min, double *ret_Max)
{
    *ret_Min = SC_NAN;
    *ret_Max = SC_NAN;
    const char *declared_type = TypeNode.attribute("declaredType").value();
    if ((declared_type != nullptr) && (strlen(declared_type) > 0)) {
        pugi::xml_node TypeDefinitionsNode = DescNode.child("TypeDefinitions");
        if (TypeDefinitionsNode) {
            pugi::xml_node SimpleTypeNode = TypeDefinitionsNode.find_child_by_attribute("SimpleType", "name", declared_type);
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
    const char *max = TypeNode.attribute("max").value();
    const char *min = TypeNode.attribute("min").value();
    if ((max != NULL) && (strlen(max) > 0) && (min != NULL) && (strlen(min) > 0)) {
        *ret_Min = atof(min);
        *ret_Max = atof(max);
        return 0;
    }
    return -1;
}


int ReadFmuXml(FMU *ret_Fmu, const char *par_XmlFileName)
{
    pugi::xml_document doc;
    int Ret = 0;

    pugi::xml_parse_result result = doc.load_file(par_XmlFileName);
    pugi::xml_node DescNode = doc.child("fmiModelDescription");

    if (strcmp (DescNode.attribute("fmiVersion").value(), "2.0")) {
        ThrowError (1, "only FMI version 2.0 are supported");
        return -1;
    }

    ret_Fmu->NumberOfEventIndicators = strtoul (DescNode.attribute("numberOfEventIndicators").value(), NULL, 0);
    const char *guid = DescNode.attribute("guid").value();
    strncpy (ret_Fmu->Guid, guid, 63);
    const char *modelName = DescNode.attribute("modelName").value();
    ret_Fmu->ModelName = (char*)malloc (strlen(modelName) + 1);
    strcpy(ret_Fmu->ModelName, modelName);
    
    pugi::xml_node CoSimulationNode = DescNode.child("CoSimulation");
    const char *modelIdentifier = CoSimulationNode.attribute("modelIdentifier").value();
    ret_Fmu->ModelIdentifier = (char*)malloc (strlen(modelIdentifier) + 1);
    strcpy(ret_Fmu->ModelIdentifier, modelIdentifier);

    pugi::xml_node VarNode = DescNode.child("ModelVariables");
    for (pugi::xml_node ScalNode = VarNode.child("ScalarVariable"); ScalNode; ScalNode = ScalNode.next_sibling("ScalarVariable")) {
        const char *name = ScalNode.attribute("name").value();
        /*if (!strcmp("CbxOut.Bus_CubixState.act_state_TSA", name)) {
            printf("Stop");
        }*/
        const char *initial = ScalNode.attribute("initial").value();
        const char *variability = ScalNode.attribute("variability").value();
        const char *pausality = ScalNode.attribute("pausality").value();
        const char *causality = ScalNode.attribute("causality").value();
        const char *description = ScalNode.attribute("description").value();
        const char *valueReference = ScalNode.attribute("valueReference").value();
        pugi::xml_node TypeNode = ScalNode.child("Real");
        if (TypeNode) {
             double min, max;
             GetMinMax(DescNode, TypeNode, &min, &max);
             const char *start = TypeNode.attribute("start").value();
             Ret = AddModelVariable(ret_Fmu, name, initial, variability, causality, pausality, description, valueReference, 0, start, 0, NULL, min, max);
        } else {
             TypeNode = ScalNode.child("Integer");
             if (TypeNode) {
                 double min, max;
                 GetMinMax(DescNode, TypeNode, &min, &max);  // are min and max exists with integer? 
                 const char *start = TypeNode.attribute("start").value();
                 Ret = AddModelVariable(ret_Fmu, name, initial, variability, causality, pausality, description, valueReference, 1, start, 0, NULL, min, max);
            } else {
                TypeNode = ScalNode.child("Boolean");
                if (TypeNode) {
                    const char *start = TypeNode.attribute("start").value();
                    Ret = AddModelVariable(ret_Fmu, name, initial, variability, causality, pausality, description, valueReference, 2, start, 0, NULL, SC_NAN, SC_NAN);
                } else {
                    TypeNode = ScalNode.child("String");
                    if (TypeNode) {
                        const char *start = TypeNode.attribute("start").value();
                    } else {
                        TypeNode = ScalNode.child("Enumeration");  // same as interger
                        if (TypeNode) {
                            const char *declared_type = TypeNode.attribute("declaredType").value();
                            double min, max;
                            char *enum_string = BuildEnumString(DescNode, declared_type, &min, &max);
                            const char *start = TypeNode.attribute("start").value();
                            Ret = AddModelVariable(ret_Fmu, name, initial, variability, causality, pausality, description, valueReference, 1, start, 2, enum_string, min, max);
                            if (enum_string != NULL) free (enum_string);
                        }
                    }
                }
             }
        }
        // all other types will be ignored!
        //ThrowError (1, "Name = %s", Name);
    }
    /*if (Ret == 0) {
        Ret = ReferenceAllVariablesToBlackboard(ret_Fmu);
    }*/
    return Ret;
}

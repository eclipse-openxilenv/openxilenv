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


#ifndef __A2LLINK_H
#define __A2LLINK_H

#include <stdint.h>

void A2LLinkInitCriticalSection (void);
#define A2L_LINK_NO_FLAGS                              0x0
#define A2L_LINK_UPDATE_FLAG                           0x1
#define A2L_LINK_UPDATE_IGNORE_FLAG                    0x2
#define A2L_LINK_UPDATE_ZERO_FLAG                      0x4
#define A2L_LINK_ADDRESS_TRANSLATION_DLL_FLAG          0x8
#define A2L_LINK_ADDRESS_TRANSLATION_MULTI_DLL_FLAG    0x10
#define A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG       0x20
#define A2L_LINK_IGNORE_MOD_COMMON_ALIGNMENTS_FLAG     0x40
#define A2L_LINK_IGNORE_RECORD_LAYOUT_ALIGNMENTS_FLAG  0x80
#define A2L_LINK_IGNORE_READ_ONLY_FLAG                 0x100

int A2LLinkToExternProcess(const char *par_A2LFileName, int par_Pid, int par_Flags);
int A2LCloseLink(int par_LinkNr, const char *par_ProcessName);

int A2LSetupLinkToExternProcess(const char *par_A2LFileName, const char *par_ProcessName, int par_UpdateFlag);
int A2LGetLinkToExternProcess(const char *par_ProcessName);

#define A2L_LABEL_TYPE_ALL                        0xFFFF
#define A2L_LABEL_TYPE_MEASUREMENT                  0xFF
#define A2L_LABEL_TYPE_SINGEL_VALUE_MEASUREMENT      0x1
#define A2L_LABEL_TYPE_1_DIM_ARRAY_MEASUREMENT       0x2
#define A2L_LABEL_TYPE_2_DIM_ARRAY_MEASUREMENT       0x4
#define A2L_LABEL_TYPE_3_DIM_ARRAY_MEASUREMENT       0x8
#define A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT   0x10
#define A2L_LABEL_TYPE_REFERENCED_MEASUREMENT       0x20

#define A2L_LABEL_TYPE_CALIBRATION                0xFF00
#define A2L_LABEL_TYPE_SINGLE_VALUE_CALIBRATION    0x100
#define A2L_LABEL_TYPE_ASCII_CALIBRATION           0x200
#define A2L_LABEL_TYPE_VAL_BLK_CALIBRATION         0x400
#define A2L_LABEL_TYPE_CURVE_CALIBRATION           0x800
#define A2L_LABEL_TYPE_MAP_CALIBRATION            0x1000
#define A2L_LABEL_TYPE_CUBOID_CALIBRATION         0x2000
#define A2L_LABEL_TYPE_CUBE_4_CALIBRATION         0x4000
#define A2L_LABEL_TYPE_CUBE_5_CALIBRATION         0x8000
#define A2L_LABEL_TYPE_AXIS_CALIBRATION          0x10000

int A2LGetIndexFromLink(int par_LinkNr, const char *par_Label, int par_TypeMask);
int A2LGetFunctionIndexFromLink(int par_LinkNr, const char *par_Function);
int A2LGetNextFunctionMemberFromLink (int par_LinkNr, int par_FuncIdx, int par_MemberIdx, const char *par_Filter, unsigned int par_Flags,
                                      char *ret_Name, int par_MaxSize, int *ret_Type);
int A2LGetNextSymbolFromLink(int par_LinkNr, int par_Index, int par_TypeMask, const char *par_Filter, char *ret_Name, int par_MaxChar);
int A2LGetNextFunktionFromLink(int par_LinkNr, int par_Index, const char *par_Filter, char *ret_Name, int par_MaxChar);

#define A2L_GET_PHYS_FLAG            0x1
#define A2L_GET_TEXT_REPLACE_FLAG    0x2
#define A2L_GET_UNIT_FLAG            0x4
void *A2LGetDataFromLinkState(int par_LinkNr, int par_Index, void *par_Reuse, int par_Flags,
                              int par_State, int par_DimGroupNo, int par_DataGroupNo, const char **ret_Error);

int A2LSetDataToLinkState(int par_LinkNr, int par_Index, void *par_Data,
                          int par_State, int par_DataGroupMaskNo, int par_DataGroupNo, const char **ret_Error);


int A2LIncDecGetReferenceToDataFromLink(int par_LinkNr, int par_Index, int par_Offset, int par_DirFlags, int par_Vid);

int A2LGetReadWritablFlagsFromLink(int par_LinkNr, int par_Index, int par_UserDefFlag);

int A2lGetMeasurementInfos (int par_LinkNr, int par_Index,
                            char *ret_Name, int par_MaxSizeName,
                            int *ret_Type, uint64_t *ret_Address,
                            int *ret_ConvType, char *ret_Conv, int par_MaxSizeConv,
                            char *ret_Unit, int par_MaxSizeUnit,
                            int *ret_FormatLength, int *ret_FormatLayout,
                            int *ret_XDim, int *ret_YDim, int *ret_ZDim,
                            double *ret_Min, double *ret_Max, int *ret_IsWritable);

int A2LReferenceMeasurementToBlackboard(int par_LinkNr, int par_Idx, int par_Dir, int par_ReferenceFlag);

int A2LGetCharacteristicInfoType(int par_LinkNr, int par_Index);  // 1 -> value, 4 -> curve, 5 -> map
int A2LGetMeasurementOrCharacteristicAddress(int par_LinkNr, int par_Index, uint64_t *ret_Address);
/*int A2LGetCCharacteristicInfoMinMax(int par_LinkNr, int par_Index, int par_AxisNo, double *ret_Min, double *ret_Max);
int A2LGetCharacteristicInfoInput(int par_LinkNr, int par_Index, int par_AxisNo, char *ret_Input, int par_MaxLen);
int A2LGetCharacteristicInfoUnit(int par_LinkNr, int par_Index, int par_AxisNo, char *ret_Unit, int par_MaxLen);
int A2LGetCharacteristicInfoConversion(int par_LinkNr, int par_Index, int par_AxisNo, char *ret_Conversion, int par_MaxLen);
*/
typedef struct {
    double m_Min;
    double m_Max;
    double m_RawMin;
    double m_RawMax;
    char *m_Input;
    char *m_Unit;
    char *m_Conversion;
    int m_ConvType;
    int m_FormatLength;
    int m_FormatLayout;
 } CHARACTERISTIC_AXIS_INFO;
int A2LGetCharacteristicAxisInfos(int par_LinkNr, int par_Index, int par_AxisNo,
                                  CHARACTERISTIC_AXIS_INFO *ret_AxisInfos);

int A2LStoreAllReferencesForProcess(int par_LinkNr, const char *par_ProcessName);
int A2LLoadAllReferencesForProcess(int par_LinkNr, const char *par_ProcessName);

int A2LGetLinkedToPid(int par_LinkNr);
int GetBaseAddressOf(int par_Pid, int par_Flags, uint64_t *ret_BaseAddress);

#endif

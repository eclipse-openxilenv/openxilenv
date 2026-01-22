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


#include <inttypes.h>
#include "A2LCalMapData.h"
#include "StringHelpers.h"

#include <QString>

extern "C" {
    #include "MyMemory.h"
    #include "MemZeroAndCopy.h"
    #include "PrintFormatToString.h"
    #include "ThrowError.h"
    #include "TextReplace.h"
    #include "BlackboardAccess.h"
    #include "EquationParser.h"
    #include "Scheduler.h"
    #include "ReadWriteValue.h"
    #include "DebugInfoAccessExpression.h"
    #include "GetNextStructEntry.h"
    #include "BlackboardConversion.h"
    #include "BlackboardConvertFromTo.h"
    #include "Compare2DoubleEqual.h"
    #include "A2LLink.h"
    #include "A2LLinkThread.h"
}
#include "A2LCalWidgetSync.h"

#include "QVariantConvert.h"

A2LCalMapData::A2LCalMapData()
{
    m_NoRecursiveSetPickMapFlag = false;
    m_Data = nullptr;
    m_LinkNo = -1;
    m_GetDataChannelNo = -1;
    m_Index = -1;

    m_PickMap = NULL;
    m_XDimPickMap = -1;
    m_YDimPickMap = -1;
    m_XPickLast = 0;
    m_YPickLast = 0;

    m_XVid = -1;
    m_YVid = -1;
    m_DecIncOffset = 1.0;

    m_PhysicalFlag = false;
    STRUCT_ZERO_INIT(m_XAxisInfo, CHARACTERISTIC_AXIS_INFO);
    STRUCT_ZERO_INIT(m_YAxisInfo, CHARACTERISTIC_AXIS_INFO);
    STRUCT_ZERO_INIT(m_ZAxisInfo, CHARACTERISTIC_AXIS_INFO);
}

A2LCalMapData::~A2LCalMapData()
{
    Clear();
}

void A2LCalMapData::Reset(bool Free)
{
    ResetAxisInfo(&m_XAxisInfo, Free);
    ResetAxisInfo(&m_YAxisInfo, Free);
    ResetAxisInfo(&m_ZAxisInfo, Free);

    if (m_XVid > 0) remove_bbvari_unknown_wait (m_XVid);
    if (m_YVid > 0) remove_bbvari_unknown_wait (m_YVid);
}

void A2LCalMapData::SetProcess(QString &par_ProcessName, void *par_CallbaclParam)
{
    m_ProcessName = par_ProcessName;
    m_LinkNo = A2LGetLinkToExternProcess(QStringToConstChar(par_ProcessName));
    if (m_GetDataChannelNo >= 0) {
        DeleteDataFromLinkChannel(m_GetDataChannelNo);
        m_GetDataChannelNo = -1;
    }
    if (m_LinkNo >= 0) {
        m_GetDataChannelNo = NewDataFromLinkChannel(m_LinkNo, 100000000, GlobalNotifiyGetDataFromLinkAckCallback,  par_CallbaclParam);
    }
}

QString A2LCalMapData::GetProcess()
{
    return m_ProcessName;
}

void A2LCalMapData::ProcessTerminated()
{
    Clear();
}

void A2LCalMapData::ProcessStarted()
{
    // ToDo
}

QString A2LCalMapData::GetCharacteristicName()
{
    return m_CharacteristicName;
}

void A2LCalMapData::SetCharacteristicName(QString &par_Characteristic)
{
    m_CharacteristicName = par_Characteristic;
    if (m_LinkNo >= 0) {
        m_Index = A2LGetIndexFromLink(m_LinkNo, QStringToConstChar(par_Characteristic),
                                      A2L_LABEL_TYPE_VAL_BLK_CALIBRATION |
                                      A2L_LABEL_TYPE_CURVE_CALIBRATION   |
                                      A2L_LABEL_TYPE_MAP_CALIBRATION);
        if (m_Index >= 0) {
            int Type = A2LGetCharacteristicInfoType(m_LinkNo, m_Index);
            if (A2LGetCharacteristicAxisInfos(m_LinkNo, m_Index, 0, &m_ZAxisInfo) == 0) {  // 0 -> map
            }
            if ((Type == 4) || (Type == 5)) {  // curve and map
                if (A2LGetCharacteristicAxisInfos(m_LinkNo, m_Index, 1, &m_XAxisInfo) == 0) {  // 1 -> x axis
                    if ((m_XAxisInfo.m_Input != nullptr) && (strlen(m_XAxisInfo.m_Input) > 1)) {
                        // try to attach
                        m_XVid = get_bbvarivid_by_name(m_XAxisInfo.m_Input);
                        if (m_XVid > 0) {
                            attach_bbvari_unknown_wait(m_XVid);
                        } else {
                            int XInputIdx = A2LGetIndexFromLink(m_LinkNo, m_XAxisInfo.m_Input,
                                                                A2L_LABEL_TYPE_SINGEL_VALUE_MEASUREMENT | A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT);
                            if (XInputIdx >= 0) {
                                if (A2LReferenceMeasurementToBlackboard(m_LinkNo, XInputIdx, 0x2, 1) == 0) {
                                    m_XVid = get_bbvarivid_by_name(m_XAxisInfo.m_Input);
                                    if (m_XVid > 0) {
                                        attach_bbvari_unknown_wait(m_XVid);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (Type == 5) {  // map
                if (A2LGetCharacteristicAxisInfos(m_LinkNo, m_Index, 2, &m_YAxisInfo) == 0) {  // 2 -> y axis
                    if ((m_YAxisInfo.m_Input != nullptr) && (strlen(m_YAxisInfo.m_Input) > 1)) {
                        // try to attach
                        m_YVid = get_bbvarivid_by_name(m_YAxisInfo.m_Input);
                        if (m_XVid > 0) {
                            attach_bbvari_unknown_wait(m_YVid);
                        } else {
                            int YInputIdx = A2LGetIndexFromLink(m_LinkNo, m_YAxisInfo.m_Input,
                                                                A2L_LABEL_TYPE_SINGEL_VALUE_MEASUREMENT | A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT);
                            if (YInputIdx >= 0) {
                                if (A2LReferenceMeasurementToBlackboard(m_LinkNo, YInputIdx, 0x2, 1) == 0) {
                                    m_YVid = get_bbvarivid_by_name(m_YAxisInfo.m_Input);
                                    if (m_YVid > 0) {
                                        attach_bbvari_unknown_wait(m_YVid);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (m_ZAxisInfo.m_ConvType == 0) m_PhysicalFlag = false; // if no formula do not try to display physical
            // inside A2L file the min/max values are physical
            CalcRawMinMaxValues();
        }
    }
}

int A2LCalMapData::GetLinkNo()
{
    return m_LinkNo;
}

int A2LCalMapData::GetIndex()
{
    return m_Index;
}

int A2LCalMapData::GetType()
{
    int Ret = 0;
    int ArrayCount = GetLinkDataArrayCount(m_Data);
    switch(ArrayCount) {
    case 1:
    case 2:
        Ret = 1;  // curve
        break;
    case 3:
        Ret = 2;  // map
        break;
    }
    return Ret;
}

bool A2LCalMapData::IsPhysical()
{
    return m_PhysicalFlag;
}

void A2LCalMapData::SetPhysical(bool par_Physical)
{
    m_PhysicalFlag = par_Physical;
}

int A2LCalMapData::GetXDim()
{
    if (GetLinkDataArrayCount(m_Data) >= 1) {
        return GetLinkDataArraySize(m_Data, 0);
    }
    return 0;
}

int A2LCalMapData::GetYDim()
{
    if (GetLinkDataArrayCount(m_Data) >= 3) {
        return GetLinkDataArraySize(m_Data, 1);
    }
    return 1;
}

int A2LCalMapData::GetDataType(enum ENUM_AXIS_TYPE par_Axis, int par_No)
{
    int ArrayNo = TranslateAxisToArrayNo(par_Axis);
    A2L_SINGLE_VALUE *Element = GetLinkArrayValueData(m_Data, ArrayNo, par_No);
    if (Element == nullptr) return 0;
    return Element->TargetType;
}

const char *A2LCalMapData::GetXInput()
{
    if (m_XAxisInfo.m_Input == nullptr) return "";
    return m_XAxisInfo.m_Input;
}

const char *A2LCalMapData::GetXUnit()
{
    if (m_XAxisInfo.m_Unit == nullptr) return "";
    return m_XAxisInfo.m_Unit;
}

const char *A2LCalMapData::GetXConversion()
{
    if (m_XAxisInfo.m_Conversion == nullptr) return "";
    return m_XAxisInfo.m_Conversion;
}

int A2LCalMapData::GetXConversionType()
{
    return m_XAxisInfo.m_ConvType;
}

const char *A2LCalMapData::GetYInput()
{
    if (m_YAxisInfo.m_Input == nullptr) return "";
    return m_YAxisInfo.m_Input;
}

const char *A2LCalMapData::GetYUnit()
{
    if (m_YAxisInfo.m_Unit == nullptr) return "";
    return m_YAxisInfo.m_Unit;
}

const char *A2LCalMapData::GetYConversion()
{
    if (m_YAxisInfo.m_Conversion == nullptr) return "";
    return m_YAxisInfo.m_Conversion;
}

int A2LCalMapData::GetYConversionType()
{
    return m_YAxisInfo.m_ConvType;
}

const char *A2LCalMapData::GetMapUnit()
{
    if (m_ZAxisInfo.m_Unit == nullptr) return "";
    return m_ZAxisInfo.m_Unit;
}

const char *A2LCalMapData::GetMapConversion()
{
    if (m_ZAxisInfo.m_Conversion == nullptr) return "";
    return m_ZAxisInfo.m_Conversion;
}

int A2LCalMapData::GetMapConversionType()
{
    return m_ZAxisInfo.m_ConvType;
}

int A2LCalMapData::TranslateAxisToArrayNo(enum A2LCalMapData::ENUM_AXIS_TYPE par_Axis)
{
    switch (par_Axis) {
    case ENUM_AXIS_TYPE_X_AXIS:
        switch (m_Data->Type) {
        case A2L_DATA_TYPE_ASCII:
        case A2L_DATA_TYPE_VAL_BLK:
            return -1;
        case A2L_DATA_TYPE_CURVE:
        case A2L_DATA_TYPE_MAP:
            return 0;
        default:
            return -1;
        }
    case ENUM_AXIS_TYPE_Y_AXIS:
        switch (m_Data->Type) {
        case A2L_DATA_TYPE_ASCII:
        case A2L_DATA_TYPE_VAL_BLK:
        case A2L_DATA_TYPE_CURVE:
            return -1;
        case A2L_DATA_TYPE_MAP:
            return 1;
        default:
            return -1;
        }
    case ENUM_AXIS_TYPE_MAP:
        switch (m_Data->Type) {
        case A2L_DATA_TYPE_ASCII:
        case A2L_DATA_TYPE_VAL_BLK:
            return 0;
        case A2L_DATA_TYPE_CURVE:
            return 1;
        case A2L_DATA_TYPE_MAP:
            return 2;
        default:
            return -1;
        }
    }
    return -1;
}

void A2LCalMapData::SetNoRecursiveSetPickMapFlag(bool par_Flag)
{
    m_NoRecursiveSetPickMapFlag = par_Flag;
}

bool A2LCalMapData::GetNoRecursiveSetPickMapFlag()
{
    return m_NoRecursiveSetPickMapFlag;
}

// par_AxisNo = 0 -> X-axis
// par_AxisNo = 1 -> Y-axis
// par_AxisNo = 2 -> Map
A2L_SINGLE_VALUE *A2LCalMapData::GetValue(int par_AxisNo, int x, int y)
{
    if (m_Data == nullptr) return nullptr;
    enum ENUM_AXIS_TYPE AxisNo = (enum ENUM_AXIS_TYPE)par_AxisNo;
    int ArrayNo = TranslateAxisToArrayNo(AxisNo);
    if (ArrayNo < 0) return nullptr;
    int ElementNo;
    if (AxisNo == ENUM_AXIS_TYPE_MAP) {
        int XDim = GetXDim();
        int YDim = GetYDim();
        if ((XDim <= 0) || (YDim <= 0)) return nullptr;
        ElementNo = y * XDim + x;
    } else {
        ElementNo = x;
    }
    return GetLinkArrayValueData(m_Data, ArrayNo, ElementNo);
#if 1
#else
    int DimCount = GetLinkDataDimCount(m_Data);
    if (DimCount == 0) {
        return GetLinkSingleValueData(m_Data);
    } else if (par_AxisNo < DimCount) {
        if (DimCount == 2) {
            return GetLinkArrayValueData(m_Data, par_AxisNo, x);        // y will be ignored
        } else if (DimCount == 3) {
            int ElementNo;
            if (par_AxisNo == 2) {
                int XDim = GetXDim(); //GetLinkDataArraySize(m_Data, 0);
                int YDim = GetYDim(); //GetLinkDataArraySize(m_Data, 1);
                if ((XDim <= 0) || (YDim <= 0)) return nullptr;
                ElementNo = y * XDim + x;
                return GetLinkArrayValueData(m_Data, par_AxisNo, ElementNo);
            } else {
                return GetLinkArrayValueData(m_Data, par_AxisNo, x);        // y will be ignored
            }
        }
    }
    return nullptr;
#endif
}

double A2LCalMapData::GetRawValue(int axis, int x, int y)
{
    A2L_SINGLE_VALUE *Value = GetValue(axis, x, y);
    if (Value == nullptr) return 0.0;
    return ConvertRawValueToDouble(Value);
}

double A2LCalMapData::GetDoubleValue(int axis, int x, int y)
{
    A2L_SINGLE_VALUE *ValuePtr = GetValue(axis, x, y);
    if (ValuePtr == nullptr) return 0.0;
    double Value = GetRawValue(axis, x, y);
    switch (axis) {
    case 0:
        return Conv(Value, m_XAxisInfo);
    case 1:
        return Conv(Value, m_YAxisInfo);
    case 2:
        return Conv(Value, m_ZAxisInfo);
    default:
        return 0.0;
    }
}

double A2LCalMapData::GetValueNorm(int axis, int x, int y)
{
    A2L_SINGLE_VALUE *ValuePtr = GetValue(axis, x, y);
    if (ValuePtr == nullptr) return 0.0;
    double Value = GetRawValue(axis, x, y);
    switch (axis) {
    case 0:
        return ConvNorm(Value, m_XAxisInfo);
    case 1:
        return ConvNorm(Value, m_YAxisInfo);
    case 2:
        return ConvNorm(Value, m_ZAxisInfo);
    default:
        return 0.0;
    }
}

QString A2LCalMapData::ConvertToString(double Value, CHARACTERISTIC_AXIS_INFO &AxisInfo, bool par_PhysFlag)
{
    if (par_PhysFlag) {
        switch (AxisInfo.m_ConvType) {
        default:
        case BB_CONV_NONE:
        case BB_CONV_FORMULA:
        case BB_CONV_FACTOFF:
        case BB_CONV_OFFFACT:
        case BB_CONV_TAB_INTP:
        case BB_CONV_TAB_NOINTP:
        case BB_CONV_RAT_FUNC:
        {
            A2L_SINGLE_VALUE Help;
            Help.Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
            Help.Value.Double = Conv(Value, AxisInfo);
            return ConvertValueToString(&Help, AxisInfo.m_FormatLayout);
        }
        case BB_CONV_TEXTREP:
        {
            char Help[256];
            int Color;
            if (convert_value2textreplace ((int64_t)Value, AxisInfo.m_Conversion,
                                          Help, sizeof(Help), &Color) == 0) {
                return QString(Help);
            } else {
                return QString();
            }
        }
        }
    } else {
        // Raw
        A2L_SINGLE_VALUE Help;
        Help.Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
        Help.Value.Double = Value;
        return ConvertValueToString(&Help, AxisInfo.m_FormatLayout);
    }
    return QString();
}

QString A2LCalMapData::GetValueString(int axis, int x, int y)
{
    A2L_SINGLE_VALUE *ValuePtr = GetValue(axis, x, y);
    if (ValuePtr == nullptr) return QString();
    double Value = GetRawValue(axis, x, y);
    A2L_SINGLE_VALUE Help;
    Help.Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
    switch (axis) {
    case 0:
        return ConvertToString(Value, m_XAxisInfo, m_PhysicalFlag);
    case 1:
        return ConvertToString(Value, m_YAxisInfo, m_PhysicalFlag);
    case 2:
        return ConvertToString(Value, m_ZAxisInfo, m_PhysicalFlag);
    default:
        return QString();
    }
}

QString A2LCalMapData::ConvertValueToString(A2L_SINGLE_VALUE *par_Value, int par_Precision)
{
    QString Ret;
    char String[128];
    switch (par_Value->Type) {
    case A2L_ELEM_TYPE_INT:
        PrintFormatToString (String, sizeof(String), "%" PRIi64 "", par_Value->Value.Int);
        Ret = QString(String);
        break;
    case A2L_ELEM_TYPE_UINT:
        PrintFormatToString (String, sizeof(String), "%" PRIu64 "", par_Value->Value.Uint);
        Ret = QString(String);
        break;
    case A2L_ELEM_TYPE_DOUBLE:
        {
            int Prec = 15;
            while (1) {
                PrintFormatToString (String, sizeof(String), "%.*g", Prec, par_Value->Value.Double);
                if ((Prec++) == 18 || (par_Value->Value.Double == strtod (String, NULL))) break;
            }
            Ret = QString(String);
        }
        break;
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        Ret = QString().number(par_Value->Value.Double, 'f', par_Precision);
        break;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
        PrintFormatToString (String, sizeof(String), "\"%s\"", par_Value->Value.String);
        Ret = QString(String);
        break;
    case A2L_ELEM_TYPE_ERROR:
    default:
        Ret = QString("error");
        break;
    }
    return Ret;
}

bool A2LCalMapData::XAxisNotCalibratable()
{
    return false;  // ToDo:
}

bool A2LCalMapData::YAxisNotCalibratable()
{
    return false; // ToDo:
}

double A2LCalMapData::Conv (double par_Value, CHARACTERISTIC_AXIS_INFO &par_AxisInfo)
{
    double Ret;
    bool ConvError = false;
    if (m_PhysicalFlag) {
        switch (par_AxisInfo.m_ConvType) {
        default:
        case BB_CONV_NONE:
            Ret = par_Value;
            break;
        case BB_CONV_FORMULA:
            if ((par_AxisInfo.m_Conversion != nullptr) && (strlen (par_AxisInfo.m_Conversion) > 0)) {
                if (direct_solve_equation_err_state_replace_value (par_AxisInfo.m_Conversion, par_Value, &Ret)) {
                    ConvError = true;
                }
            } else {
                Ret = par_Value;
            }
            break;
        case BB_CONV_FACTOFF:
            if (Conv_FactorOffsetRawToPhysFromString(par_AxisInfo.m_Conversion, par_Value, &Ret)) {
                ConvError = true;
            }
            break;
        case BB_CONV_OFFFACT:
            if (Conv_OffsetFactorRawToPhysFromString(par_AxisInfo.m_Conversion, par_Value, &Ret)) {
                ConvError = true;
            }
            break;
        case BB_CONV_TAB_INTP:
            if (Conv_TableInterpolRawToPhysFromString(par_AxisInfo.m_Conversion, par_Value, &Ret)) {
                ConvError = true;
            }
            break;
        case BB_CONV_TAB_NOINTP:
            if (Conv_TableNoInterpolRawToPhysFromString(par_AxisInfo.m_Conversion, par_Value, &Ret)) {
                ConvError = true;
            }
            break;
        case BB_CONV_RAT_FUNC:
            if (Conv_RationalFunctionRawToPhysFromString(par_AxisInfo.m_Conversion, par_Value, &Ret)) {
                ConvError = true;
            }
            break;
        }
        if (ConvError) {
            ThrowError (1, "switch to raw view because of errors inside conversion \"%s\" from the label \"%s\"",
                       par_AxisInfo.m_Conversion, m_CharacteristicName.toLatin1().data());
            m_PhysicalFlag = false;  // switch back to raw!
            Ret = par_Value;
        }
    } else {
        Ret = par_Value;   // no conversion
    }
    return Ret;
}

double A2LCalMapData::ConvNorm (double par_Value, CHARACTERISTIC_AXIS_INFO &par_AxisInfo)
{
    double Ret;
    if (m_PhysicalFlag) {
        Ret = Conv(par_Value, par_AxisInfo);
        if (par_AxisInfo.m_Max <= par_AxisInfo.m_Min) par_AxisInfo.m_Max = par_AxisInfo.m_Min + 1.0;
        Ret = (Ret - par_AxisInfo.m_Min) / (par_AxisInfo.m_Max - par_AxisInfo.m_Min);
    } else {
        Ret = par_Value;
        if (par_AxisInfo.m_RawMax <= par_AxisInfo.m_RawMin) par_AxisInfo.m_RawMax = par_AxisInfo.m_RawMin + 1.0;
        Ret = (Ret - par_AxisInfo.m_RawMin) / (par_AxisInfo.m_RawMax - par_AxisInfo.m_RawMin);
    }
    return Ret;
}


void A2LCalMapData::SetValue (A2L_SINGLE_VALUE *par_Value, union BB_VARI par_ToValue, enum BB_DATA_TYPES par_Type)
{
    switch(par_Value->Type) {
    case A2L_ELEM_TYPE_INT:
        switch (par_Type) {
        case BB_BYTE:
            par_Value->Value.Int = par_ToValue.b;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UBYTE:
            par_Value->Value.Int = (int64_t)par_ToValue.ub;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_WORD:
            par_Value->Value.Int = par_ToValue.w;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UWORD:
            par_Value->Value.Int = (int64_t)par_ToValue.uw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_DWORD:
            par_Value->Value.Int = par_ToValue.dw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UDWORD:
            par_Value->Value.Int = (int64_t)par_ToValue.udw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_FLOAT:
            par_Value->Value.Int = (int64_t)par_ToValue.f;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_DOUBLE:
            par_Value->Value.Int = (int64_t)par_ToValue.d;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_QWORD:
            par_Value->Value.Int = par_ToValue.qw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UQWORD:
            par_Value->Value.Int = (int64_t)par_ToValue.uqw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        default:
            break;
        }
        break;
    case A2L_ELEM_TYPE_UINT:
        switch (par_Type) {
        case BB_BYTE:
            par_Value->Value.Uint = (uint64_t)par_ToValue.b;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UBYTE:
            par_Value->Value.Uint = par_ToValue.ub;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_WORD:
            par_Value->Value.Uint = (uint64_t)par_ToValue.w;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UWORD:
            par_Value->Value.Uint = par_ToValue.uw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_DWORD:
            par_Value->Value.Uint = (uint64_t)par_ToValue.dw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UDWORD:
            par_Value->Value.Uint = par_ToValue.udw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_FLOAT:
            par_Value->Value.Uint = (uint64_t)par_ToValue.f;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_DOUBLE:
            par_Value->Value.Uint = (uint64_t)par_ToValue.d;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_QWORD:
            par_Value->Value.Uint = (uint64_t)par_ToValue.qw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UQWORD:
            par_Value->Value.Uint = par_ToValue.uqw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        default:
            break;
        }
        break;
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        switch (par_Type) {
        case BB_BYTE:
            par_Value->Value.Double = par_ToValue.b;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UBYTE:
            par_Value->Value.Double = par_ToValue.ub;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_WORD:
            par_Value->Value.Double = (double)par_ToValue.w;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UWORD:
            par_Value->Value.Double = (double)par_ToValue.uw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_DWORD:
            par_Value->Value.Double = (double)par_ToValue.dw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UDWORD:
            par_Value->Value.Double = (double)par_ToValue.udw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_FLOAT:
            par_Value->Value.Double = (double)par_ToValue.f;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_DOUBLE:
            par_Value->Value.Double = par_ToValue.d;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_QWORD:
            par_Value->Value.Double = (double)par_ToValue.qw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        case BB_UQWORD:
            par_Value->Value.Double = (double)par_ToValue.uqw;
            par_Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            break;
        default:
            break;
        }
        break;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    case A2L_ELEM_TYPE_ERROR:
    default:
        break;
    }
}


/*
// Divider muss positiv sein!
double A2LCalMapData::DivideButMinumumDecByOne (double Value, int Type, double Divider)
{
#if 0
    double Help_double = Value / Divider;
    double Min, Max;
    get_datatype_min_max_value (Type, &Min, &Max);
    if ((Help_double >= Max) && (Divider <= 1.0)) return Max;
    if ((Help_double <= Min) && (Divider >= 1.0)) return Min;
    // Wenn es ein Integer Datenty ist:
    if ((Type >= BB_BYTE) && (Type <= BB_UDWORD)) {
       int64_t Help_i64 = static_cast<int64_t>(Help_double + 0.5);
       if (static_cast<int64_t>(Value) == Help_i64) {
           // Wert hat sich nicht geaendert dann mind. um 1 veraendern
           if (Divider > 1.0) {
               Help_i64--;
           } else {
               Help_i64++;
           }
       }
       return static_cast<double>(Help_i64);
    } else if ((Type >= BB_FLOAT) && (Type <= BB_DOUBLE)) {
        if (Help_double == 0.0) {
            if (Divider > 1.0) {
                Help_double = -(ZAxisMax - ZAxisMin) / 1000.0;
            } else {
                Help_double = (ZAxisMax - ZAxisMin) / 1000.0;
            }
        }
    }
    return Help_double;
#endif
    return 0.0;
}

// Factor muss positiv sein!
double A2LCalMapData::MutiplyButMinumumIncByOne (double Value, int Type, double Factor)
{
#if 0
    double Help_double = Value * Factor;
    double Min, Max;
    get_datatype_min_max_value (Type, &Min, &Max);
    if ((Help_double >= Max) && (Factor >= 1.0)) return Max;
    if ((Help_double <= Min) && (Factor <= 1.0)) return Min;
    // Wenn es ein Integer Datenty ist:
    if ((Type >= BB_BYTE) && (Type <= BB_UDWORD)) {
       int64_t Help_i64 = static_cast<int64_t>(Help_double + 0.5);
       if (static_cast<int64_t>(Value) == Help_i64) {
           // Wert hat sich nicht geaendert dann mind. um 1 veraendern
           if (Factor > 1.0) {
               Help_i64++;
           } else {
               Help_i64--;
           }
       }
       return static_cast<double>(Help_i64);
    } else if ((Type >= BB_FLOAT) && (Type <= BB_DOUBLE)) {
        if (Help_double == 0.0) {
            if (Factor > 1.0) {
                Help_double = (ZAxisMax - ZAxisMin) / 1000.0;
            } else {
                Help_double = -(ZAxisMax - ZAxisMin) / 1000.0;
            }
        }
    }
    return Help_double;
#endif
    return 0.0;
}
*/

double A2LCalMapData::AddButMinumumIncOrDecByOne (double Value, int Type, double Add)
{
    double Help_double = Value + Add;
    double Min, Max;
    get_datatype_min_max_value (Type, &Min, &Max);
    if ((Help_double >= Max) && (Add > 0.0)) return Max;
    if ((Help_double <= Min) && (Add < 0.0)) return Min;
    // Wenn es ein Integer Datenty ist:
    if ((Type >= BB_BYTE) && (Type <= BB_UDWORD)) {
       int64_t Help_i64 = static_cast<int64_t>(Help_double + 0.5);
       if (static_cast<int64_t>(Value) == Help_i64) {
           // Wert hat sich nicht geaendert dann mind. um 1 veraendern
           if (Add > 0.0) {
               Help_i64++;
           } else {
               Help_i64--;
           }
       }
       return static_cast<double>(Help_i64);
    }
    return Help_double;
}

void A2LCalMapData::ResetAxisInfo(CHARACTERISTIC_AXIS_INFO *AxisInfo, bool Free)
{
    if (Free) {
        if (AxisInfo->m_Unit != nullptr) my_free(AxisInfo->m_Unit);
        if (AxisInfo->m_Input != nullptr) my_free(AxisInfo->m_Input);
        if (AxisInfo->m_Conversion != nullptr) my_free(AxisInfo->m_Conversion);
    }
    AxisInfo->m_Max = 1.0;
    AxisInfo->m_Min = 0.0;
    AxisInfo->m_Unit = nullptr;
    AxisInfo->m_Input = nullptr;
    AxisInfo->m_Conversion = nullptr;
    AxisInfo->m_ConvType = 0;
    AxisInfo->m_FormatLayout = 8;
    AxisInfo->m_FormatLength = 8;
}


double A2LCalMapData::GetMaxMinusMin (int axis)
{
    CHARACTERISTIC_AXIS_INFO *AxisInfo;
    switch (axis) {
    case 0:  // X-Achse
        AxisInfo = &m_XAxisInfo;
        break;
    case 1:  // Y-Achse
        AxisInfo = &m_YAxisInfo;
        break;
    case 2:  // Map
        AxisInfo = &m_ZAxisInfo;
        break;
    default:
        return 1.0;
    }
    if (m_PhysicalFlag) {
        return AxisInfo->m_Max - AxisInfo->m_Min;
    } else {
        return AxisInfo->m_RawMax - AxisInfo->m_RawMin;
    }
}

#define X_AXIS_NO 0
#define Y_AXIS_NO 1
#define MAP_NO    2

void A2LCalMapData::AdjustMinMax()
{
    int x, y;
    double xv, yv, zv;
    double XAxisMin, YAxisMin, ZAxisMin;
    double XAxisMax, YAxisMax, ZAxisMax;

    XAxisMin = YAxisMin = ZAxisMin = +1.0E37;
    XAxisMax = YAxisMax = ZAxisMax = -1.0E37;
    int XDim = GetXDim();
    int YDim = GetYDim();
    for (y = 0; y < YDim; y++) {
        for (x = 0; x < XDim; x++) {
            xv = GetDoubleValue (X_AXIS_NO, x, 0);
            yv = GetDoubleValue (Y_AXIS_NO, y, 0);
            zv = GetDoubleValue (MAP_NO, x, y);
            if (xv < XAxisMin) XAxisMin = xv;
            if (xv > XAxisMax) XAxisMax = xv;
            if (yv < YAxisMin) YAxisMin = yv;
            if (yv > YAxisMax) YAxisMax = yv;
            if (zv < ZAxisMin) ZAxisMin = zv;
            if (zv > ZAxisMax) ZAxisMax = zv;
        }
    }
    // make sure min < max
    if (XAxisMin >= XAxisMax) XAxisMin -= 1.0, XAxisMax += 1.0;
    if (YAxisMin >= YAxisMax) YAxisMin -= 1.0, YAxisMax += 1.0;
    if (ZAxisMin >= ZAxisMax) ZAxisMin -= 1.0, ZAxisMax += 1.0;
    XAxisMin -= (XAxisMax - XAxisMin) / 10;
    YAxisMin -= (YAxisMax - YAxisMin) / 10;
    ZAxisMin -= (ZAxisMax - ZAxisMin) / 10;
    XAxisMax += (XAxisMax - XAxisMin) / 10;
    YAxisMax += (YAxisMax - YAxisMin) / 10;
    ZAxisMax += (ZAxisMax - ZAxisMin) / 10;

    m_XAxisInfo.m_Min = XAxisMin;
    m_XAxisInfo.m_Max = XAxisMax;
    m_YAxisInfo.m_Min = YAxisMin;
    m_YAxisInfo.m_Max = YAxisMax;
    m_ZAxisInfo.m_Min = ZAxisMin;
    m_ZAxisInfo.m_Max = ZAxisMax;

    CalcRawMinMaxValues();
}


int A2LCalMapData::ChangeValueOP (int op,
                                  double par_Value, //union BB_VARI par_Value, enum BB_DATA_TYPES par_Type,
                                  enum ENUM_AXIS_TYPE par_Axis, int x, int y)
{
    A2L_SINGLE_VALUE *ValuePtr = GetValue(par_Axis, x, y);
    union BB_VARI ToValue;
    union BB_VARI FromValue;
    double Help;

    switch (op) {
    case 'i':
    case 'I':
        FromValue.d = par_Value;
        sc_convert_from_to(BB_DOUBLE, &FromValue, ValuePtr->TargetType, &ToValue);
        SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        break;
    case '+':
        Help = ConvertRawValueToDouble(ValuePtr);
        Help += par_Value;
        FromValue.d = Help;
        sc_convert_from_to(BB_DOUBLE, &FromValue, ValuePtr->TargetType, &ToValue);
        SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        break;
    case '-':
        Help = ConvertRawValueToDouble(ValuePtr);
        Help -= par_Value;
        FromValue.d = Help;
        sc_convert_from_to(BB_DOUBLE, &FromValue, ValuePtr->TargetType, &ToValue);
        SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        break;
    case '*':
        Help = ConvertRawValueToDouble(ValuePtr);
        Help *= par_Value;
        FromValue.d = Help;
        sc_convert_from_to(BB_DOUBLE, &FromValue, ValuePtr->TargetType, &ToValue);
        SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        break;
    case '/':
        if ((par_Value > 0.0) || (par_Value < 0.0)) {
            Help = ConvertRawValueToDouble(ValuePtr);
            Help /= par_Value;
            FromValue.d = Help;
            sc_convert_from_to(BB_DOUBLE, &FromValue, ValuePtr->TargetType, &ToValue);
            SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        }
        break;
    case 'o':
    case 'O':
        m_DecIncOffset = par_Value;
        break;
    case 'p':
    case 'P':
        Help = ConvertRawValueToDouble(ValuePtr);
        Help += m_DecIncOffset;
        FromValue.d = Help;
        sc_convert_from_to(BB_DOUBLE, &FromValue, ValuePtr->TargetType, &ToValue);
        SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        break;
    case 'm':
    case 'M':
        Help = ConvertRawValueToDouble(ValuePtr);
        Help -= m_DecIncOffset;
        FromValue.d = Help;
        sc_convert_from_to(BB_DOUBLE, &FromValue, ValuePtr->TargetType, &ToValue);
        SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        break;
    case 1:  // Add aber den Wert mindestens um 1 veraendern
        Help = ConvertRawValueToDouble(ValuePtr);
        Help = AddButMinumumIncOrDecByOne(Help, ValuePtr->TargetType, par_Value);
        FromValue.d = Help;
        sc_convert_from_to(BB_DOUBLE, &FromValue, ValuePtr->TargetType, &ToValue);
        SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        break;
    }
    return 0;
}

int A2LCalMapData::WriteDataToTargetLink(bool par_Readback)
{
    if ((m_LinkNo > 0) && (m_GetDataChannelNo >= 0)) {
        INDEX_DATA_BLOCK *idb = GetIndexDataBlock(1);
        idb->Data->LinkNo = m_LinkNo;
        idb->Data->Index = m_Index;
        idb->Data->Data = m_Data;
        idb->Data->Flags = 0;
        int Ret = A2LGetDataFromLinkReq(m_GetDataChannelNo, 1, idb);
        if (!Ret && par_Readback) {
            UpdateReq();
        }
    }
    return 0;
}


int A2LCalMapData::ChangeValueOP(int op, QVariant par_Value, enum ENUM_AXIS_TYPE par_Axis, int x, int y)
{
    A2L_SINGLE_VALUE *ValuePtr = GetValue(par_Axis, x, y);
    union BB_VARI ToValue;
    union FloatOrInt64 Value;
    union FloatOrInt64 OldValue;
    union FloatOrInt64 NewValue;
    int NewValueType;
    int Type = ConvertQVariantToFloatOrInt64(par_Value, &Value);
    int OldValueType = ConvertA2LValueToFloatOrInt64(ValuePtr, &OldValue);

    switch (op) {
    case 'i':
    case 'I':
        if (sc_convert_from_FloatOrInt64_to_BB_VARI(Type, Value, ValuePtr->TargetType, &ToValue) == 0) {
            SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        }
        break;
    case '+':
        NewValueType = add_FloatOrInt64(OldValue, OldValueType, Value, Type, &NewValue);
        if (sc_convert_from_FloatOrInt64_to_BB_VARI(NewValueType, NewValue, ValuePtr->TargetType, &ToValue) == 0) {
            SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        }
        break;
    case '-':
        NewValueType = sub_FloatOrInt64(OldValue, OldValueType, Value, Type, &NewValue);
        if (sc_convert_from_FloatOrInt64_to_BB_VARI(NewValueType, NewValue, ValuePtr->TargetType, &ToValue) == 0) {
            SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        }
        break;
    case '*':
        NewValueType = mul_FloatOrInt64(OldValue, OldValueType, Value, Type, &NewValue);
        if (sc_convert_from_FloatOrInt64_to_BB_VARI(NewValueType, NewValue, ValuePtr->TargetType, &ToValue) == 0) {
            SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        }
        break;
    case '/':
        /*
        NewValueType = div_FloatOrInt64(OldValue, OldValueType, Value, Type, &NewValue);
        if (sc_convert_from_FloatOrInt64_to_BB_VARI(NewValueType, NewValue, ValuePtr->TargetType, &ToValue) == 0) {
            SetValue(ValuePtr, ToValue, static_cast<enum BB_DATA_TYPES>(ValuePtr->TargetType));
        }*/
        break;
    case 'o':
    case 'O':
        //DecIncOffset = Value;
        break;
    case 'p':
    case 'P':
        //NewValue = LocalSetValue(axis, x, y, LocalGetValue(axis, x, y) + DecIncOffset);
        //map_data[y * xdim + x] += DecIncOffset;
        break;
    case 'm':
    case 'M':
        //NewValue = LocalSetValue(axis, x, y, LocalGetValue(axis, x, y) - DecIncOffset);
        //map_data[y * xdim + x] -= DecIncOffset;
        break;
    case 1:  // Add aber den Wert mindestens um 1 veraendern
        //NewValue = AddButMinumumIncOrDecByOne(LocalGetValue(axis, x, y), BBType, Value);
        //NewValue = LocalSetValue(axis, x, y, NewValue);
        break;
    }
    return 0;
}

static void CalcRawMinMaxValuesOneAxis(CHARACTERISTIC_AXIS_INFO *par_AxisInfo, const char *par_AxisName)
{
    double Help;
    char *errstring;

    if (par_AxisInfo->m_ConvType == 1) { // formula
        // min
        if (calc_raw_value_for_phys_value (par_AxisInfo->m_Conversion, par_AxisInfo->m_Min, "", BB_DOUBLE, &par_AxisInfo->m_RawMin, &Help, &errstring)) {
            par_AxisInfo->m_RawMin = par_AxisInfo->m_Min;
            ThrowError (1, "cannot convert physical min. value of %s axis to raw value because the formula \"%s\" has error \"%s\"",
                   par_AxisName, par_AxisInfo->m_Conversion, errstring);
            par_AxisInfo->m_ConvType = 0;
            FreeErrStringBuffer (&errstring);
        }
        // max
        if (calc_raw_value_for_phys_value (par_AxisInfo->m_Conversion, par_AxisInfo->m_Max, "", BB_DOUBLE, &par_AxisInfo->m_RawMax, &Help, &errstring)) {
            par_AxisInfo->m_RawMax = par_AxisInfo->m_Max;
            ThrowError (1, "cannot convert physical max. value of x axis to raw value because the formula \"%s\" has error \"%s\"",
                   par_AxisName, par_AxisInfo->m_Conversion, errstring);
            par_AxisInfo->m_ConvType = 0;
            FreeErrStringBuffer (&errstring);
        }
    } else {
        par_AxisInfo->m_RawMin = par_AxisInfo->m_Min;
        par_AxisInfo->m_RawMax = par_AxisInfo->m_Max;
    }
}

void A2LCalMapData::CalcRawMinMaxValues()
{
    // X-Achse
    CalcRawMinMaxValuesOneAxis(&m_XAxisInfo, "x");
    int Type = A2LGetCharacteristicInfoType(m_LinkNo, m_Index);

    if (Type == 5) {   // map
        // Y-Achse
        CalcRawMinMaxValuesOneAxis(&m_YAxisInfo, "y");
    }
    // Map
    CalcRawMinMaxValuesOneAxis(&m_ZAxisInfo, "map");
}

int A2LCalMapData::UpdateReq()
{
    if ((m_LinkNo > 0) && (m_GetDataChannelNo >= 0) && (m_Index >= 0)) {
        INDEX_DATA_BLOCK *idb = GetIndexDataBlock(1);
        idb->Data->LinkNo = m_LinkNo;
        idb->Data->Index = m_Index;
        idb->Data->Data = m_Data;
        idb->Data->Flags = 0;
        return  A2LGetDataFromLinkReq(m_GetDataChannelNo, 0, idb);
    } else {
        return -1;
    }
}

int A2LCalMapData::UpdateAck(void *par_Data)
{
    m_Data = DupA2lData((A2L_DATA*)par_Data);
    if (m_Data == nullptr) {
        return -1;
    }
    int XDim = GetXDim();
    int YDim = GetYDim();
    if ((XDim != m_XDimPickMap) || (YDim != m_YDimPickMap)) {
        if (m_PickMap != nullptr) {
            my_free(m_PickMap);
        }
        m_XDimPickMap = XDim;
        m_YDimPickMap = YDim;
        if ((m_XDimPickMap < 1) || (m_XDimPickMap > 10000) ||
            (m_YDimPickMap < 1) || (m_YDimPickMap > 10000)) {
            ThrowError(1, "Internal error: %s (%i)", __FILE__, __LINE__);
        }
        m_PickMap = (char*)my_calloc(m_XDimPickMap * m_YDimPickMap, sizeof(m_PickMap[0]));
    }
    return 0;
}

bool A2LCalMapData::IsValid()
{
    return m_Data != nullptr;
}

void A2LCalMapData::GetLastPick(int *ret_XPick, int *ret_YPick)
{
    *ret_XPick = m_XPickLast;
    *ret_YPick = m_YPickLast;
}

void A2LCalMapData::SetPick(int par_X, int par_Y, int par_Op)
{
    if ((m_Data != nullptr) && (m_PickMap != nullptr)) {
        int Pos = par_Y * m_XDimPickMap + par_X;
        if (Pos < (m_XDimPickMap * m_YDimPickMap)) {
            switch (par_Op) {
            case 1:  // set
                m_PickMap[Pos] = 1;    // Bit set
                break;
            case 2:
                m_PickMap[Pos] = 0;    // Bit reset
                break;
            case 3:
                m_PickMap[Pos] ^= 1;    // Bit toggel
                break;
            default:
                break;
            }
            if (m_PickMap[Pos]) {
                m_XPickLast = par_X;
                m_YPickLast = par_Y;
            }
        }
    }
}

bool A2LCalMapData::IsPicked(int par_X, int par_Y)
{
    if ((m_Data != nullptr) && (m_PickMap != nullptr)) {
        int Pos = par_Y * m_XDimPickMap + par_X;
        if (Pos < (m_XDimPickMap * m_YDimPickMap)) {
            return (m_PickMap[Pos] != 0);
        }
    }
    return false;
}

CHARACTERISTIC_AXIS_INFO *A2LCalMapData::GetAxisInfos(int par_AxisNo)
{
    switch (par_AxisNo) {
    case 0:  // X axis
        return &m_XAxisInfo;
    case 1:  // Y axis
        return &m_YAxisInfo;
    case 2:  // Map
        return &m_ZAxisInfo;
    default:
        return nullptr;
    }
}

double A2LCalMapData::GetDecIncOffset()
{
    return m_DecIncOffset;
}

void A2LCalMapData::SetDecIncOffset(double par_DecIncOffset)
{
    m_DecIncOffset = par_DecIncOffset;
}

int A2LCalMapData::GetAddress(uint64_t *ret_Address)
{
    if (m_Index < 0) return -1;
    return A2LGetMeasurementOrCharacteristicAddress(m_LinkNo, m_Index, ret_Address);
}

void A2LCalMapData::Clear()
{
    if (m_GetDataChannelNo >= 0){
        DeleteDataFromLinkChannel(m_GetDataChannelNo);
        m_GetDataChannelNo = -1;
    }
    if (m_Data != nullptr) {
        FreeA2lData(m_Data);
        //free(m_Data);  // not my_free !!!
        m_Data = nullptr;
    }
    Reset(true);
    m_XDimPickMap = -1;
    m_YDimPickMap = -1;
    m_XPickLast = 0;
    m_YPickLast = 0;
    if (m_PickMap != nullptr) my_free(m_PickMap);
    m_PickMap = nullptr;
}

//#pragma optimize( "", off )

double A2LCalMapData::MapAccess (double *ret_point_x, double *ret_point_y, double *ret_point_z)
{
#if 1
    double  z0, z1 = 0.0;
    int i, j;
    double ret;

    int XDim = GetXDim();
    int YDim = GetYDim();

    double xWert = read_bbvari_convert_double (m_XVid);
    double yWert = (GetType() == 2) ? read_bbvari_convert_double (m_YVid) : 0.0;

    // Eingangsbereich
    double FirstXValue = GetRawValue(X_AXIS_NO, 0, 0);
    double LastXValue = GetRawValue(X_AXIS_NO, XDim - 1, 0);
    if (xWert > LastXValue) {
        xWert = LastXValue;
    } else if (xWert < FirstXValue) {
        xWert = FirstXValue;
    }
    double FirstYValue = GetRawValue(Y_AXIS_NO, 0, 0);
    double LastYValue = GetRawValue(Y_AXIS_NO, YDim - 1, 0);
    if (yWert > LastYValue) {
        yWert = LastYValue;
    } else if (yWert < FirstYValue) {
        yWert = FirstYValue;
    }

    /* x-Intervall ermitteln */
    for (i = 0; i < XDim; i++) {
        double v = GetRawValue(X_AXIS_NO, i, 0);
        if (xWert <= v)
            break;
    }
    if (i == 0) {
        xWert = FirstXValue;
    } else if (i == XDim) {
        xWert = LastXValue;
        i -= 2;
    } else {
        i--;
    }

    /* y-Intervall ermitteln */
    for (j = 0; j < YDim; j++) {
        double v = GetRawValue(Y_AXIS_NO, j, 0);
        if (yWert <= v)
            break;
    }
    if (j == 0) {
        yWert = FirstYValue;
    } else if (j == YDim) {
        yWert = LastYValue;
        j -= 2;
    } else {
        j--;
    }

    double xi_p0 = GetRawValue(X_AXIS_NO, i, 0);
    double xi_p1 = GetRawValue(X_AXIS_NO, i + 1, 0);
    double yj_p0 = GetRawValue(Y_AXIS_NO, j, 0);
    double yj_p1 = GetRawValue(Y_AXIS_NO, j + 1, 0);
    double zji_p0 = GetRawValue(MAP_NO, XDim * j + i, 0);
    double zji_p1 = GetRawValue(MAP_NO, XDim * j + i + 1, 0);

    if (CompareDoubleEqual_int(xi_p1, xi_p0)) {
        z0 = zji_p0;
    } else {
        if (zji_p1 >= zji_p0) {
            z0 = (((zji_p1 - zji_p0) * (xWert - xi_p0))
                 / (xi_p1 - xi_p0)) + zji_p0;
        } else {
            z0 = zji_p0 - (((zji_p0 - zji_p1) * (xWert - xi_p0))
                 / (xi_p1 - xi_p0));
        }
        j++;
        zji_p0 = GetRawValue(MAP_NO, XDim * j + i, 0);
        zji_p1 = GetRawValue(MAP_NO, XDim * j + i + 1, 0);

        if (zji_p1 >= zji_p0) {
            z1 = (((zji_p1 - zji_p0) * (xWert - xi_p0))
                 / (xi_p1 - xi_p0)) + zji_p0;
        } else {
            z1 = zji_p0 - (((zji_p0 - zji_p1) * (xWert - xi_p0))
                 / (xi_p1 - xi_p0));
        }
    }
    if (CompareDoubleEqual_int(yj_p1, yj_p0)) {
        ret = z0;
    } else {
        if (z1 >= z0) {
            ret = (((z1 - z0) * (yWert - yj_p0))
                  / (yj_p1 - yj_p0)) + z0;
        } else {
            ret = z0 - (((z0 - z1) * (yWert - yj_p0))
                       / (yj_p1 - yj_p0));
        }
    }
    *ret_point_x = ConvNorm (xWert, m_XAxisInfo);
    *ret_point_y = ConvNorm (yWert, m_YAxisInfo);
    *ret_point_z = ConvNorm (ret, m_ZAxisInfo);
    return ret;
#endif
    return 0.0;
}
//#pragma optimize( "", on )


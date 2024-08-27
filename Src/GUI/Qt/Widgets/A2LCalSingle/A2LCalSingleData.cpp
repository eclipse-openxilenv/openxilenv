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

#include "A2LCalSingleData.h"
#include "A2LCalWidgetSync.h"
#include "StringHelpers.h"

#include <float.h>
#include <QString>
#include <QFontMetrics>

extern "C" {
    #include "MyMemory.h"
    #include "ThrowError.h"
    #include "TextReplace.h"
    #include "BlackboardAccess.h"
    #include "EquationParser.h"
    #include "Scheduler.h"
    #include "ReadWriteValue.h"
    #include "DebugInfoAccessExpression.h"
    #include "GetNextStructEntry.h"
    #include "BlackboardConvertFromTo.h"
    #include "Compare2DoubleEqual.h"
    #include "A2LLink.h"
    #include "A2LLinkThread.h"
}

#include "QVariantConvert.h"

extern FILE *Debug_fh;

A2LCalSingleData::A2LCalSingleData()
{
    m_Exists = false;
    m_HasChanged = true;
    m_Data = nullptr;
    m_LinkNo = -1;
    m_Index = -1;

    m_DisplayType = -1;
    m_NamePixelWidth = 0;
    m_ValuePixelWidth = 0;
    m_UnitPixelWidth = 0;

    m_DecIncOffset = 1.0;

    m_PhysicalFlag = false;
    memset(&m_Info, 0, sizeof(m_Info));
}

A2LCalSingleData::~A2LCalSingleData()
{
    Clear();
}

void A2LCalSingleData::Reset(bool Free)
{
    ResetAxisInfo(&m_Info, Free);
}

void A2LCalSingleData::SetProcess(QString &par_ProcessName)
{
    m_ProcessName = par_ProcessName;
    m_LinkNo = A2LGetLinkToExternProcess(QStringToConstChar(par_ProcessName));
}

QString A2LCalSingleData::GetProcess()
{
    return m_ProcessName;
}

void A2LCalSingleData::ProcessTerminated()
{
    Clear();
}

void A2LCalSingleData::ProcessStarted()
{
    // ToDo
}

QString A2LCalSingleData::GetCharacteristicName()
{
    return m_CharacteristicName;
}

void A2LCalSingleData::SetCharacteristicName(QString &par_Characteristic)
{
    m_CharacteristicName = par_Characteristic;
    if (m_LinkNo >= 0) {
        CheckExistance(m_LinkNo);
    }
}

int A2LCalSingleData::GetIndex()
{
    return m_Index;
}

A2L_DATA *A2LCalSingleData::GetData()
{
    return m_Data;
}

void A2LCalSingleData::SetData(A2L_DATA *par_Data)
{
    m_Data = par_Data;
}

int A2LCalSingleData::GetLinkNo()
{
    return m_LinkNo;
}

int A2LCalSingleData::GetNamePixelWidth()
{
    return m_NamePixelWidth;
}

void A2LCalSingleData::CalcNamePixelWidth(QFontMetrics *par_FontMetrics)
{
    m_NamePixelWidth = par_FontMetrics->boundingRect(m_CharacteristicName).width();
}

int A2LCalSingleData::GetValuePixelWidth()
{
    return m_ValuePixelWidth;
}

void A2LCalSingleData::CalcValuePixelWidth(QFontMetrics *par_FontMetrics)
{
    m_ValuePixelWidth = par_FontMetrics->boundingRect(m_ValueStr).width();
}

int A2LCalSingleData::GetUnitPixelWidth()
{
    return m_UnitPixelWidth;
}

void A2LCalSingleData::CalcUnitPixelWidth(QFontMetrics *par_FontMetrics)
{
    m_UnitPixelWidth = par_FontMetrics->boundingRect(m_ValueStr).width();
}

bool A2LCalSingleData::CheckExistance(int par_LinkNo)
{
    m_LinkNo = par_LinkNo;
    if (m_LinkNo > 0) {
        if (m_Index < 0) {
            m_Index = A2LGetIndexFromLink(m_LinkNo,
                                          QStringToConstChar(m_CharacteristicName),
                                          A2L_LABEL_TYPE_SINGLE_VALUE_CALIBRATION);
            if (Debug_fh != nullptr) {fprintf(Debug_fh, "        m_Index = %i\n", m_Index); fflush(Debug_fh);}
            if (m_Index >= 0) {
                if (A2LGetCharacteristicAxisInfos(m_LinkNo, m_Index, 0,
                                                   &(m_Info)) != 0) {
                     m_Index = -1;
                     m_Exists = false;
                 } else {
                     CalcRawMinMaxValues();
                     m_Exists = true;
                 }
            } else {
                m_Exists = true;
            }
       } else {
            m_Exists = true;
       }
    } else {
        m_Exists = false;
    }
    if (m_DisplayType == -1) {
        if (m_Info.m_ConvType > 0) {
            m_DisplayType = 3;  // default is physical display type if a cnversion exists
        } else {
            m_DisplayType = 0;  // else raw
        }
    }
    return m_Exists;
}


int A2LCalSingleData::GetType()
{
    return GetLinkDataType(m_Data);
}

bool A2LCalSingleData::IsPhysical()
{
    return m_PhysicalFlag;
}

void A2LCalSingleData::SetPhysical(bool par_Physical)
{
    m_PhysicalFlag = par_Physical;
}


int A2LCalSingleData::GetDataType()
{
    A2L_SINGLE_VALUE *Element = GetLinkSingleValueData(m_Data);
    if (Element == nullptr) return 0;
    return Element->TargetType;
}


const char *A2LCalSingleData::GetMapUnit()
{
    if (m_Info.m_Unit == nullptr) return "";
    return m_Info.m_Unit;
}

const char *A2LCalSingleData::GetMapConversion()
{
    if (m_Info.m_Conversion == nullptr) return "";
    return m_Info.m_Conversion;
}

A2L_SINGLE_VALUE *A2LCalSingleData::GetValue()
{
    if (m_Data == nullptr) return nullptr;
    return GetLinkSingleValueData(m_Data);
}

double A2LCalSingleData::GetRawValue()
{
    A2L_SINGLE_VALUE *Value = GetValue();
    if (Value == nullptr) return 0.0;
    return ConvertRawValueToDouble(Value);
}

double A2LCalSingleData::GetDoubleValue()
{
    A2L_SINGLE_VALUE *ValuePtr = GetValue();
    if (ValuePtr == nullptr) return 0.0;
    double Value = GetRawValue();
    return Conv(Value, m_Info);
}


QString A2LCalSingleData::ConvertToString(double Value, CHARACTERISTIC_AXIS_INFO &AxisInfo)
{
    switch (AxisInfo.m_ConvType) {
    default:
    case 0: // no conversion
        {
            A2L_SINGLE_VALUE Help;
            Help.Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
            Help.Value.Double = Value;
            return ConvertValueToString(&Help);
        }
    case 1: // formula
        {
            A2L_SINGLE_VALUE Help;
            Help.Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
            Help.Value.Double = Conv(Value, AxisInfo);
            return ConvertValueToString(&Help);
        }
    case 2: // text replace
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
    return QString();
}

QString A2LCalSingleData::GetValueString()
{
    A2L_SINGLE_VALUE *ValuePtr = GetValue();
    if (ValuePtr == nullptr) return QString();
    double Value = GetRawValue();
    if (m_PhysicalFlag) {
        A2L_SINGLE_VALUE Help;
        Help.Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
        return ConvertToString(Value, m_Info);
    } else {
        return ConvertValueToString(ValuePtr);
    }
}

QString A2LCalSingleData::ConvertValueToString(A2L_SINGLE_VALUE *par_Value)
{
    QString Ret;
    char Help[32];
    switch (par_Value->Type) {
    case A2L_ELEM_TYPE_INT:
        sprintf(Help, "%" PRIi64 " (i)", par_Value->Value.Int);
        Ret = QString(Help);
        break;
    case A2L_ELEM_TYPE_UINT:
        sprintf(Help, "%" PRIu64 " (u)", par_Value->Value.Uint);
        Ret = QString(Help);
        break;
    case A2L_ELEM_TYPE_DOUBLE:
        sprintf(Help, "%f (f)", par_Value->Value.Double);
        Ret = QString(Help);
        break;
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        sprintf(Help, "%f (p)", par_Value->Value.Double);
        Ret = QString(Help);
        break;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
        sprintf(Help, "\"%s\" (s)", par_Value->Value.String);
        Ret = QString(Help);
        break;
    case A2L_ELEM_TYPE_ERROR:
    default:
        Ret = QString("error");
        break;
    }
    return Ret;
}


double A2LCalSingleData::Conv (double par_Value, CHARACTERISTIC_AXIS_INFO &par_AxisInfo)
{
    double Ret;
    if (m_PhysicalFlag) {
        if (par_AxisInfo.m_ConvType == 1) {
            if ((par_AxisInfo.m_Conversion != nullptr) && (strlen (par_AxisInfo.m_Conversion) > 0)) {
                if (direct_solve_equation_err_state_replace_value (par_AxisInfo.m_Conversion, par_Value, &Ret)) {
                    ThrowError (1, "remove the X axis conversation \"%s\" from the label \"%s\"",
                           par_AxisInfo.m_Conversion, QStringToConstChar(m_CharacteristicName));
                    m_PhysicalFlag = false;  // switch back to raw!
                    Ret = par_Value;   // Fehler in Umrechnung
                }
            } else {
                Ret = par_Value;   // keine Umrechnung
            }
        } else {
            Ret = par_Value;   // keine Umrechnung
        }
    } else {
        Ret = par_Value;   // keine Umrechnung
    }
    return Ret;
}



void A2LCalSingleData::SetValue (A2L_SINGLE_VALUE *par_Value, union BB_VARI par_ToValue, enum BB_DATA_TYPES par_Type)
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
    // also update string buffer
    ToString(true);
}

/*
// Divider muss positiv sein!
double A2LCalSingleData::DivideButMinumumDecByOne (double Value, int Type, double Divider)
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
double A2LCalSingleData::MutiplyButMinumumIncByOne (double Value, int Type, double Factor)
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

double A2LCalSingleData::AddButMinumumIncOrDecByOne (double Value, int Type, double Add)
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

void A2LCalSingleData::ResetAxisInfo(CHARACTERISTIC_AXIS_INFO *AxisInfo, bool Free)
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


double A2LCalSingleData::GetMaxMinusMin ()
{
    if (m_PhysicalFlag) {
        return m_Info.m_Max - m_Info.m_Min;
    } else {
        return m_Info.m_RawMax - m_Info.m_RawMin;
    }
}


int A2LCalSingleData::ChangeValueInsideExternProcess(int op, const QVariant par_Value, int par_GetDataChannelNo, int par_Row, bool par_Readback)
{
    A2L_SINGLE_VALUE *ValuePtr = GetValue();
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
    INDEX_DATA_BLOCK *idb = GetIndexDataBlock(1);
    idb->Data->Index = m_Index;
    idb->Data->Data = m_Data;
    idb->Data->Flags = 0;
    idb->Data->User = par_Row;
    int Ret = A2LGetDataFromLinkReq(par_GetDataChannelNo, 1, idb);
    if (Ret) {
        //NotifyDataChangedToSyncObject(m_LinkNo, m_Index, m_Data);
    }
    if (par_Readback) {
        //Update();
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

void A2LCalSingleData::CalcRawMinMaxValues()
{
    CalcRawMinMaxValuesOneAxis(&m_Info, "");
}

/*
int A2LCalSingleData::Update()
{
    const char *Error;
    if ((m_LinkNo < 0) || (m_Index < 0)) return -1;
    if (Debug_fh != nullptr) {fprintf(Debug_fh, "Update()"); fflush(Debug_fh);}
    m_Data = (A2L_DATA*)A2LGetDataFromLink(m_LinkNo, m_Index, m_Data, 0, &Error);
    if (Debug_fh != nullptr) {fprintf(Debug_fh, " m_Data = %p\n", m_Data); fflush(Debug_fh);}
    if (m_Data == nullptr) {
        return -1;
    }
    return 0;
}
*/

void A2LCalSingleData::SetToNotExiting()
{
    m_Exists = false;
    m_HasChanged = false;
    m_LinkNo = -1;
    m_Index = -1;
    m_ValueStr.clear();

    // m_NamePixelWidth;
    // m_ValuePixelWidth;
    // m_UnitPixelWidth;

    if (m_Info.m_Conversion != nullptr) {
        my_free(m_Info.m_Conversion);
        m_Info.m_Conversion = nullptr;
    }
    if (m_Info.m_Unit != nullptr) {
        my_free(m_Info.m_Unit);
        m_Info.m_Unit = nullptr;
    }
}


void A2LCalSingleData::SetToExiting()
{
    m_Exists = true;
    m_HasChanged = true;
}

bool A2LCalSingleData::Exists()
{
    return m_Exists;
}

QString A2LCalSingleData::GetUnit()
{
    return QString(m_Info.m_Unit);
}

QString A2LCalSingleData::GetValueStr()
{
    return m_ValueStr;
}

int A2LCalSingleData::GetDisplayType()
{
    return m_DisplayType;
}

bool A2LCalSingleData::SetValue(const QVariant &par_Value, int par_GetDataChannelNo, int par_Row)
{
    return ChangeValueInsideExternProcess('I', par_Value, par_GetDataChannelNo, par_Row, true);
}

void A2LCalSingleData::SetValueStr(QString par_String)
{
    m_ValueStr = par_String;
}

void A2LCalSingleData::SetDisplayType(int par_DisplayType)
{
    m_DisplayType = par_DisplayType;
}


CHARACTERISTIC_AXIS_INFO *A2LCalSingleData::GetInfos()
{
    return &m_Info;
}

double A2LCalSingleData::GetDecIncOffset()
{
    return m_DecIncOffset;
}

void A2LCalSingleData::SetDecIncOffset(double par_DecIncOffset)
{
    m_DecIncOffset = par_DecIncOffset;
}

bool A2LCalSingleData::NotifyCheckDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data)
{
    if ((par_LinkNo == m_LinkNo) && (par_Index == m_Index)) {
        if (par_Data != m_Data) {  // not my own notify?
            return true;
        }
    }
    return false;
}

void A2LCalSingleData::Clear()
{
    if (m_Data != nullptr) {
        FreeA2lData(m_Data);
        //free(m_Data);  // not my_free !!!
        m_Data = nullptr;
    }
    Reset(true);
}

static double ConvertToDouble(A2L_SINGLE_VALUE *par_Value)
{
    switch (par_Value->Type) {
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        return par_Value->Value.Double;
    case A2L_ELEM_TYPE_UINT:
        return par_Value->Value.Uint;
    case A2L_ELEM_TYPE_INT:
        return par_Value->Value.Int;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    case A2L_ELEM_TYPE_ERROR:
    default:
        return 0.0;
    }
}

static int64_t ConvertToInt64(A2L_SINGLE_VALUE *par_Value)
{
    switch (par_Value->Type) {
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        return (int64_t)par_Value->Value.Double;
    case A2L_ELEM_TYPE_UINT:
        return (int64_t)par_Value->Value.Uint;
    case A2L_ELEM_TYPE_INT:
        return par_Value->Value.Int;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    case A2L_ELEM_TYPE_ERROR:
    default:
        return 0;
    }
}

static QString ConvertTextReplace(A2L_SINGLE_VALUE *par_Value, char *par_Conversion)
{
    char Help[256];
    int Color;
    if (convert_value2textreplace (ConvertToInt64(par_Value), par_Conversion,
                                   Help, sizeof(Help), &Color) == 0) {
        return QString(Help);
    } else {
        return QString("conversion error");
    }
}

static int ConvertFormula(A2L_SINGLE_VALUE *par_Value, char *par_Conversion, double *ret_Value)
{
    double Help = ConvertToDouble(par_Value);
    if (direct_solve_equation_err_state_replace_value (par_Conversion, Help, ret_Value)) {
        *ret_Value = Help;   // keine Umrechnung
        return -1;
    }
    return 0;
}


static QString ConvertPhys(A2L_SINGLE_VALUE *par_Value, char *par_Conversion, int par_ConversionType, int par_Precision)
{
    double Help;
    switch (par_ConversionType) {
    default:
    case 0:  // no conversion
        return QString("no conversion defined");
    case 1:  // formula
        if (ConvertFormula(par_Value, par_Conversion, &Help)) {
            return QString("conversion error");
        } else {
            return QString().number(Help, 'f', par_Precision);
        }
    case 2:
        return ConvertTextReplace(par_Value, par_Conversion);
    }
}


static QString ConvetDoubleToString (double par_Value)
{
    char String[128];
    int Prec = 15;
    while (1) {
        sprintf (String, "%.*g", Prec, par_Value);
        if ((Prec++) == 18 || (par_Value == strtod (String, NULL))) break;
    }
    return QString(String);
}

int A2LCalSingleData::ToString(bool par_UpdateAlways)
{
    if (m_Data != nullptr) {
        A2L_SINGLE_VALUE* ValuePtr = GetLinkSingleValueData(m_Data);
        if (Debug_fh != nullptr) {fprintf(Debug_fh, "ToString() ValuePtr = %p m_Exists = %i m_HasChanged = %i\n", ValuePtr, m_Exists, m_HasChanged); fflush(Debug_fh);}
        if ((ValuePtr != nullptr) && m_Exists) {
            if (par_UpdateAlways || m_HasChanged) {
                m_HasChanged = false;
                switch(m_DisplayType) {
                case 0:  // Raw
                    switch (ValuePtr->Type) {
                    case A2L_ELEM_TYPE_DOUBLE:
                        m_ValueStr = ConvetDoubleToString(ValuePtr->Value.Double);
                        break;
                    case A2L_ELEM_TYPE_UINT:
                        m_ValueStr = QString().number(ValuePtr->Value.Uint, 10);
                        break;
                    case A2L_ELEM_TYPE_INT:
                        m_ValueStr = QString().number(ValuePtr->Value.Int, 10);
                        break;
                    case A2L_ELEM_TYPE_PHYS_DOUBLE:
                    case A2L_ELEM_TYPE_TEXT_REPLACE:
                    case A2L_ELEM_TYPE_ERROR:
                    //default:
                        m_ValueStr = QString("data type error");
                        break;
                    }
                    break;
                case 1: // Hex
                    switch (ValuePtr->Type) {
                    case A2L_ELEM_TYPE_DOUBLE:
                        m_ValueStr = ConvetDoubleToString(ValuePtr->Value.Double);
                        break;
                    case A2L_ELEM_TYPE_UINT:
                        m_ValueStr = QString("0x");
                        m_ValueStr.append(QString().number(ValuePtr->Value.Uint, 16));
                        break;
                    case A2L_ELEM_TYPE_INT:
                        if (ValuePtr->Value.Int >= 0) {
                            m_ValueStr = QString("0x");
                            m_ValueStr.append(QString().number(ValuePtr->Value.Int, 16));
                        } else {
                            m_ValueStr = QString("-0x");
                            m_ValueStr.append(QString().number(-ValuePtr->Value.Int, 16));
                        }
                        break;
                    case A2L_ELEM_TYPE_PHYS_DOUBLE:
                    case A2L_ELEM_TYPE_TEXT_REPLACE:
                    case A2L_ELEM_TYPE_ERROR:
                    //default:
                        m_ValueStr = QString("data type error");
                        break;
                    }
                    break;
                case 2: // Binaery
                    switch (ValuePtr->Type) {
                    case A2L_ELEM_TYPE_DOUBLE:
                        m_ValueStr = ConvetDoubleToString(ValuePtr->Value.Double);
                        break;
                    case A2L_ELEM_TYPE_UINT:
                        m_ValueStr = QString("0b");
                        m_ValueStr.append(QString().number(ValuePtr->Value.Uint, 2));
                        break;
                    case A2L_ELEM_TYPE_INT:
                        if (ValuePtr->Value.Int >= 0) {
                            m_ValueStr = QString("0b");
                            m_ValueStr.append(QString().number(ValuePtr->Value.Int, 2));
                        } else {
                            m_ValueStr = QString("-0b");
                            m_ValueStr.append(QString().number(-ValuePtr->Value.Int, 2));
                        }
                        break;
                    case A2L_ELEM_TYPE_PHYS_DOUBLE:
                    case A2L_ELEM_TYPE_TEXT_REPLACE:
                    case A2L_ELEM_TYPE_ERROR:
                    //default:
                        m_ValueStr = QString("data type error");
                        break;
                    }
                    break;
                case 3: // Phys
                     m_ValueStr = ConvertPhys(ValuePtr,
                                              m_Info.m_Conversion,
                                              m_Info.m_ConvType,
                                              m_Info.m_FormatLayout); //m_Precision);
                    break;
                default:
                    m_ValueStr = QString("unknown display type (%1)").arg(m_DisplayType);
                    break;
                }
                return 1;
            }
        } else {
            m_ValueStr = QString ("doesn't exist");
        }
    }
    return 0;
}


bool A2LCalSingleData::GetMinMax(double *ret_Min, double *ret_Max)
{
    *ret_Min = DBL_MIN;
    *ret_Max = DBL_MAX;
    int Type = GetLinkDataType(m_Data);
    if (Type == 1) {  // only single values
        A2L_SINGLE_VALUE* ValuePtr = GetLinkSingleValueData(m_Data);
        if (ValuePtr != nullptr) {
            if ((ValuePtr->Flags & (A2L_VALUE_FLAG_READ_ONLY | A2L_VALUE_FLAG_ONLY_VIRTUAL)) == 0) {
                get_datatype_min_max_value (ValuePtr->TargetType, ret_Min, ret_Max);
                *ret_Min = m_Info.m_RawMin;
                *ret_Max = m_Info.m_RawMax;
                return true;
            }
        }
    }
    return false;
}

int A2LCalSingleData::GetAddress(uint64_t *ret_Address)
{
    if (m_Index < 0) return -1;
    return A2LGetMeasurementOrCharacteristicAddress(m_LinkNo, m_Index, ret_Address);
}

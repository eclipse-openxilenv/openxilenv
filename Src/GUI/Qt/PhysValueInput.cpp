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


#include "PhysValueInput.h"
#include <QResizeEvent>
#include <QToolTip>
#include <QApplication>

//#define DEBUG_EVENT

#ifdef DEBUG_EVENT
#include <QDebug>
#endif

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "EquationParser.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "BlackboardConversion.h"
#include "TextReplace.h"
#include "ThrowError.h"
}

PhysValueInput::PhysValueInput(QWidget *parent, int par_Vid, bool par_Raw, bool par_Phys, enum DisplayTypeRawValue par_DisplayRawValue) : QWidget(parent)
{
    setObjectName("PhysValueInput");
    sizePolicy().setControlType(QSizePolicy::GroupBox);
    m_DisplayRawValue = par_DisplayRawValue;
    m_ShowRaw = par_Raw;
    m_ShowPhys = par_Phys;
    m_ConvType = BB_CONV_NONE;

    m_AssignedVid = 0;
    m_ConvString = nullptr;

    m_RowValue = nullptr;
    m_PhysValue = nullptr;
    m_EnumValue = nullptr;
    m_Model = nullptr;

    m_WithPlusMinus = false;
    m_WithConfig = false;

    m_Vid = 0;
    m_BlackboardVariableName = nullptr;

    m_RawPhysValueChangedPingPong = false;

    m_Model = nullptr;

    m_Header = false;


    m_DataType = DATA_TYPE_DOUBLE;
    m_RawIntValue = 0;
    m_RawUIntValue = 0;
    m_RawDoubleValue = 0.0;
    m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;

    if (par_Vid > 0) {
        SetBlackboardVariableId(par_Vid);
        m_ShowRaw = par_Raw;
        m_ShowPhys = par_Phys;
    }
    m_SwapValueBetweeRaWPhys = false;
    ChangeStructureElem();
    m_SwapValueBetweeRaWPhys = true;

    setFocusPolicy(Qt::WheelFocus);

    installEventFilter(this);
    m_WideningSignal = false;

    m_PlusMinusIncrement = 1.0;
    m_StepType = ValueInput::LINEAR_STEP;
}

PhysValueInput::~PhysValueInput()
{
    if (m_BlackboardVariableName != nullptr) my_free(m_BlackboardVariableName);
    if (m_ConvString != nullptr) my_free(m_ConvString);
    if (m_Model != nullptr) delete(m_Model);
}

int PhysValueInput::DispayTypeToBase(PhysValueInput::DisplayTypeRawValue par_DisplayType)
{
    switch (par_DisplayType) {
    //default:
    case DISPLAY_RAW_VALUE_DECIMAL:
        return 10;
    case DISPLAY_RAW_VALUE_HEXAMAL:
        return 16;
    case DISPLAY_RAW_VALUE_BINARY:
        return 2;
    }
    return 10;
}

void PhysValueInput::SetDisplayPhysValue(bool par_State)
{
     m_ShowPhys = par_State;
     ChangeStructureElem();
     ChangeStructurePos(size());
}

void PhysValueInput::SetDisplayRawValue(bool par_State)
{
    m_ShowRaw = par_State;
    ChangeStructureElem();
    ChangeStructurePos(size());
}

void PhysValueInput::SetEnumString(const char *par_EnumString)
{
    if (m_ConvString != nullptr) my_free(m_ConvString);
    size_t Len = strlen(par_EnumString) + 1;
    m_ConvString = static_cast<char*>(my_malloc (Len));
    MEMCPY(m_ConvString, par_EnumString, Len);
    m_ConvType = BB_CONV_TEXTREP;
    ChangeStructureElem();
    ChangeStructurePos(size());
}

void PhysValueInput::SetBlackboardVariableId(int par_Vid, bool par_ChangeRawPhys)
{
    int ConvType;
    bool SaveShowRaw = m_ShowRaw;
    bool SaveShowPhys = m_ShowPhys;
    enum BB_CONV_TYPES SaveConvType = m_ConvType;
    m_Vid = par_Vid;
    ConvType = get_bbvari_conversiontype(m_Vid);
    switch(ConvType) {
    case BB_CONV_FORMULA:
    case BB_CONV_FACTOFF:
    case BB_CONV_OFFFACT:
    case BB_CONV_TAB_INTP:
    case BB_CONV_TAB_NOINTP:
    case BB_CONV_RAT_FUNC:
        if (par_ChangeRawPhys) {
            m_ShowRaw = true;
            m_ShowPhys = true;
        }
        m_ConvType = (enum BB_CONV_TYPES)ConvType;
        break;
    case BB_CONV_TEXTREP:
        if (par_ChangeRawPhys) {
            m_ShowRaw = true;
            m_ShowPhys = true;
        }
        m_ConvType = BB_CONV_TEXTREP;
        FillComboBox();
        break;
    default:
        if (par_ChangeRawPhys) {
            m_ShowRaw = true;
            m_ShowPhys = false;
        }
        m_ConvType = BB_CONV_NONE;
        break;
    }
    if (par_ChangeRawPhys) {
        if ((SaveShowRaw != m_ShowRaw) ||
            (SaveShowPhys != m_ShowPhys) ||
            (SaveConvType != m_ConvType)) {
            ChangeStructureElem();
        }
    }
    bool m_MinMaxCheck;
    union BB_VARI MinRawValue;
    union BB_VARI MaxRawValue;

    int BbVariType = get_bbvaritype(m_Vid);

    if (get_datatype_min_max_union (BbVariType, &MinRawValue, &MaxRawValue) == 0) {
        m_MinMaxCheck = true;
        if (m_RowValue != nullptr) {
            m_RowValue->SetMinMaxValue(MinRawValue, MaxRawValue, static_cast<enum BB_DATA_TYPES>(BbVariType));
        }
    } else {
        m_MinMaxCheck = false;
    }

    switch (BbVariType) {
    case BB_BYTE:
        m_RawIntValue = read_bbvari_byte(m_Vid);
        SetRawValue (m_RawIntValue, DispayTypeToBase(m_DisplayRawValue));
        break;
    case BB_WORD:
        m_RawIntValue = read_bbvari_word(m_Vid);
        SetRawValue (m_RawIntValue, DispayTypeToBase(m_DisplayRawValue));
        break;
    case BB_DWORD:
        m_RawIntValue = read_bbvari_dword(m_Vid);
        SetRawValue (m_RawIntValue, DispayTypeToBase(m_DisplayRawValue));
        break;
    case BB_QWORD:
        m_RawIntValue = read_bbvari_qword(m_Vid);
        SetRawValue (m_RawIntValue, DispayTypeToBase(m_DisplayRawValue));
        break;
    case BB_UBYTE:
        m_RawUIntValue = read_bbvari_ubyte(m_Vid);
        SetRawValue (m_RawUIntValue, DispayTypeToBase(m_DisplayRawValue));
        break;
    case BB_UWORD:
        m_RawUIntValue = read_bbvari_uword(m_Vid);
        SetRawValue (m_RawUIntValue, DispayTypeToBase(m_DisplayRawValue));
        break;
    case BB_UDWORD:
        m_RawUIntValue = read_bbvari_udword(m_Vid);
        SetRawValue (m_RawUIntValue, DispayTypeToBase(m_DisplayRawValue));
        break;
    case BB_UQWORD:
        m_RawUIntValue = read_bbvari_uqword(m_Vid);
        SetRawValue (m_RawUIntValue, DispayTypeToBase(m_DisplayRawValue));
        break;
    case BB_FLOAT:
    case BB_DOUBLE:
        m_RawDoubleValue = read_bbvari_convert_double(m_Vid);
        SetRawValue (m_RawDoubleValue);
        break;
    }
}

void PhysValueInput::SetBlackboardVariableName(char *par_VariableName)
{
    if (m_BlackboardVariableName != nullptr) my_free(m_BlackboardVariableName);
    size_t Len = strlen(par_VariableName) + 1;
    m_BlackboardVariableName = static_cast<char*>(my_malloc (Len));
    MEMCPY(m_BlackboardVariableName, par_VariableName, Len);
}

void PhysValueInput::SetConersionTypeAndString(BB_CONV_TYPES par_ConvType, const char *par_ConvString)
{
    if (m_ConvString != nullptr) my_free(m_ConvString);
    size_t Len = strlen(par_ConvString) + 1;
    m_ConvString = static_cast<char*>(my_malloc (Len));
    MEMCPY(m_ConvString, par_ConvString, Len);
    m_ConvType = par_ConvType;
    ChangeStructureElem();
    ChangeStructurePos(size());
}

void PhysValueInput::SetFormulaString(const char *par_Formula)
{
    if (m_ConvString != nullptr) my_free(m_ConvString);
    size_t Len = strlen(par_Formula) + 1;
    m_ConvString = static_cast<char*>(my_malloc (Len));
    MEMCPY(m_ConvString, par_Formula, Len);
    m_ConvType = BB_CONV_FORMULA;
}

void PhysValueInput::SetRawValue(int par_Value, int par_Base)
{
    m_DataType = DATA_TYPE_INT;
    m_RawIntValue = par_Value;
    switch (par_Base) {
    case 10:
    default:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;
        break;
    case 16:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_HEXAMAL;
        break;
    case 2:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_BINARY;
        break;
    }
    if (m_RowValue != nullptr) m_RowValue->SetValue(par_Value, par_Base);
    if (m_PhysValue != nullptr) {
        // convert
        double PhysValue = ConvertRawToPhys(static_cast<double>(par_Value));
        m_PhysValue->SetValue(PhysValue);
    }
}

void PhysValueInput::SetRawValue(unsigned int par_Value, int par_Base)
{
    m_DataType = DATA_TYPE_UINT;
    m_RawUIntValue = par_Value;
    switch (par_Base) {
    case 10:
    default:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;
        break;
    case 16:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_HEXAMAL;
        break;
    case 2:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_BINARY;
        break;
    }
    if (m_RowValue != nullptr) m_RowValue->SetValue(par_Value, par_Base);
    if (m_PhysValue != nullptr) {
        // convert
        double PhysValue = ConvertRawToPhys(static_cast<double>(par_Value));
        m_PhysValue->SetValue(PhysValue);
    }
}

void PhysValueInput::SetRawValue(uint64_t par_Value, int par_Base)
{
    m_DataType = DATA_TYPE_UINT;
    m_RawUIntValue = par_Value;
    switch (par_Base) {
    case 10:
    default:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;
        break;
    case 16:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_HEXAMAL;
        break;
    case 2:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_BINARY;
        break;
    }
    if (m_RowValue != nullptr) m_RowValue->SetValue(par_Value, par_Base);
    if (m_PhysValue != nullptr) {
        // convert
        double PhysValue = ConvertRawToPhys(static_cast<double>(par_Value));
        m_PhysValue->SetValue(PhysValue);
    }
}

void PhysValueInput::SetRawValue(int64_t par_Value, int par_Base)
{
    m_DataType = DATA_TYPE_UINT;
    m_RawUIntValue = static_cast<uint64_t>(par_Value);
    switch (par_Base) {
    case 10:
    default:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;
        break;
    case 16:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_HEXAMAL;
        break;
    case 2:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_BINARY;
        break;
    }
    if (m_RowValue != nullptr) m_RowValue->SetValue(par_Value, par_Base);
    if (m_PhysValue != nullptr) {
        // convert
        double PhysValue = ConvertRawToPhys(static_cast<double>(par_Value));
        m_PhysValue->SetValue(PhysValue);
    }
}

void PhysValueInput::SetRawValue(double par_Value)
{
    m_DataType = DATA_TYPE_DOUBLE;
    m_RawDoubleValue = par_Value;
    m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;
    if (m_RowValue != nullptr) m_RowValue->SetValue(par_Value);
    if (m_PhysValue != nullptr) {
        // convert
        double PhysValue = ConvertRawToPhys(par_Value);
        m_PhysValue->SetValue(PhysValue);
    }
}

void PhysValueInput::SetRawValue(union BB_VARI par_Value, enum BB_DATA_TYPES par_Type, int par_Base, bool par_ChangeDataType)
{
    int64_t IntValue = 0;
    uint64_t UIntValue = 0;
    double DoubleValue = 0.0;
    switch (par_Base) {
    case 10:
    default:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;
        break;
    case 16:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_HEXAMAL;
        break;
    case 2:
        m_DisplayRawValue = DISPLAY_RAW_VALUE_BINARY;
        break;
    }
    switch (par_Type) {
    case BB_BYTE:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_INT;
        DoubleValue = IntValue = par_Value.b;
        UIntValue = static_cast<uint64_t>(IntValue);
        break;
    case BB_WORD:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_INT;
        DoubleValue = IntValue = par_Value.w;
        UIntValue = static_cast<uint64_t>(IntValue);
        break;
    case BB_DWORD:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_INT;
        DoubleValue = IntValue = par_Value.dw;
        UIntValue = static_cast<uint64_t>(IntValue);
        break;
    case BB_QWORD:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_INT;
        DoubleValue = IntValue = par_Value.qw;
        UIntValue = static_cast<uint64_t>(IntValue);
        break;
    case BB_UBYTE:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_UINT;
        DoubleValue = UIntValue = par_Value.ub;
        IntValue = static_cast<int64_t>(UIntValue);
        break;
    case BB_UWORD:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_UINT;
        DoubleValue = UIntValue = par_Value.uw;
        IntValue = static_cast<int64_t>(UIntValue);
        break;
    case BB_UDWORD:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_UINT;
        DoubleValue = UIntValue = par_Value.udw;
        IntValue = static_cast<int64_t>(UIntValue);
        break;
    case BB_UQWORD:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_UINT;
        DoubleValue = UIntValue = par_Value.uqw;
        IntValue = static_cast<int64_t>(UIntValue);
        break;
    case BB_FLOAT:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_DOUBLE;
        DoubleValue = static_cast<double>(par_Value.f);
        break;
    case BB_DOUBLE:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_DOUBLE;
        DoubleValue = par_Value.d;
        break;
    default:
        if (par_ChangeDataType) m_DataType = DATA_TYPE_DOUBLE;
        DoubleValue = 0.0;
        break;
    }
    if (m_RowValue != nullptr) {
        switch (m_DataType) {
        case DATA_TYPE_DOUBLE:
            m_RowValue->SetValue(DoubleValue);
            break;
        case DATA_TYPE_INT:
            m_RowValue->SetValue(IntValue, par_Base);
            break;
        case DATA_TYPE_UINT:
            m_RowValue->SetValue(UIntValue, par_Base);
            break;
        }
    }
    if (m_PhysValue != nullptr) {
        // convert
        double PhysValue = ConvertRawToPhys(DoubleValue);
        m_PhysValue->SetValue(PhysValue);
    }
}

void PhysValueInput::SetPhysValue(double par_PhysValue)
{
    m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;
    if (m_RowValue != nullptr) {
        // convert
        double PhysValue;
        double RawValue = ConvertPhysToRaw(par_PhysValue, &PhysValue);
        m_DataType = DATA_TYPE_DOUBLE;
        m_RawDoubleValue = RawValue;
        m_RowValue->SetValue(RawValue);
    }
    if (m_PhysValue != nullptr) m_PhysValue->SetValue(par_PhysValue);
}

void PhysValueInput::SetEnumValue(char *par_EnumValue)
{
    m_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL;
    if (m_EnumValue != nullptr) m_EnumValue->setCurrentText(QString(par_EnumValue));
}

double PhysValueInput::GetDoubleRawValue(bool *ret_Ok)
{
    if (m_RowValue != nullptr) {
        return m_RowValue->GetDoubleValue(ret_Ok);
    }
    if (m_PhysValue != nullptr) {
        double PhysValue = m_PhysValue->GetDoubleValue(ret_Ok);
        double RawValue;
        double RealPhysValue;
        if (m_Vid > 0) {
            if (get_raw_value_for_phys_value (m_Vid, PhysValue, &RawValue, &RealPhysValue)) {
                ThrowError (1, "cannot convert physical to raw value");
                if (ret_Ok != nullptr) *ret_Ok = false;
                return PhysValue;
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return RawValue;
            }
        } else if (m_ConvString != nullptr) {
            bool Ok;
            RawValue = ConvertPhysToRaw(PhysValue, &RealPhysValue, &Ok);
            if (!Ok) {
                if (ret_Ok != nullptr) *ret_Ok = false;
                return PhysValue;
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return RawValue;
            }
        } else {
            if (ret_Ok != nullptr) *ret_Ok = false;
            return 0.0;
        }
    }
    if (m_EnumValue != nullptr) {
        return static_cast<double>(m_EnumValue->GetFromValue(ret_Ok));
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0.0;
}

int PhysValueInput::GetIntRawValue(bool *ret_Ok)
{
    if (m_RowValue != nullptr) {
        return m_RowValue->GetIntValue(ret_Ok);
    }
    if (m_PhysValue != nullptr) {
        double PhysValue = m_PhysValue->GetDoubleValue(ret_Ok);
        double RawValue;
        double RealPhysValue;
        if (m_Vid > 0) {
            if (get_raw_value_for_phys_value (m_Vid, PhysValue, &RawValue, &RealPhysValue)) {
                ThrowError (1, "cannot convert physical to raw value");
                if (ret_Ok != nullptr) *ret_Ok = false;
                return static_cast<int>(PhysValue);
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return static_cast<int>(RawValue);
            }
        } else if (m_ConvString != nullptr) {
            bool Ok;
            RawValue = ConvertPhysToRaw(PhysValue, &RealPhysValue, &Ok);
            if (!Ok) {
                if (ret_Ok != nullptr) *ret_Ok = false;
                return static_cast<int>(PhysValue);
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return static_cast<int>(RawValue);
            }
        } else {
            if (ret_Ok != nullptr) *ret_Ok = false;
            return 0.0;
        }
    }
    if (m_EnumValue != nullptr) {
        return m_EnumValue->GetFromValue(ret_Ok);
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0;
}

unsigned int PhysValueInput::GetUIntRawValue(bool *ret_Ok)
{
    if (m_RowValue != nullptr) {
        return m_RowValue->GetUIntValue(ret_Ok);
    }
    if (m_PhysValue != nullptr) {
        double PhysValue = m_PhysValue->GetDoubleValue(ret_Ok);
        double RawValue;
        double RealPhysValue;
        if (m_Vid > 0) {
            if (get_raw_value_for_phys_value (m_Vid, PhysValue, &RawValue, &RealPhysValue)) {
                ThrowError (1, "cannot convert physical to raw value");
                if (ret_Ok != nullptr) *ret_Ok = false;
                return static_cast<unsigned int>(PhysValue);
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return static_cast<unsigned int>(RawValue);
            }
        } else if (m_ConvString != nullptr) {
            bool Ok;
            RawValue = ConvertPhysToRaw(PhysValue, &RealPhysValue, &Ok);
            if (!Ok) {
                if (ret_Ok != nullptr) *ret_Ok = false;
                return static_cast<unsigned int>(PhysValue);
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return static_cast<unsigned int>(RawValue);
            }
        } else {
            if (ret_Ok != nullptr) *ret_Ok = false;
            return 0.0;
        }
    }
    if (m_EnumValue != nullptr) {
        return static_cast<unsigned int>(m_EnumValue->GetFromValue(ret_Ok));
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0;
}

unsigned long long PhysValueInput::GetUInt64RawValue(bool *ret_Ok)
{
    if (m_RowValue != nullptr) {
        return m_RowValue->GetUInt64Value(ret_Ok);
    }
    if (m_PhysValue != nullptr) {
        double PhysValue = m_PhysValue->GetDoubleValue(ret_Ok);
        double RawValue;
        double RealPhysValue;
        if (m_Vid > 0) {
            if (get_raw_value_for_phys_value (m_Vid, PhysValue, &RawValue, &RealPhysValue)) {
                ThrowError (1, "cannot convert physical to raw value");
                if (ret_Ok != nullptr) *ret_Ok = false;
                return static_cast<unsigned int>(PhysValue);
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return static_cast<unsigned int>(RawValue);
            }
        } else if (m_ConvString != nullptr) {
            bool Ok;
            RawValue = ConvertPhysToRaw(PhysValue, &RealPhysValue, &Ok);
            if (!Ok) {
                if (ret_Ok != nullptr) *ret_Ok = false;
                return static_cast<unsigned long long>(PhysValue);
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return static_cast<unsigned long long>(RawValue);
            }
        } else {
            if (ret_Ok != nullptr) *ret_Ok = false;
            return 0.0;
        }
    }
    if (m_EnumValue != nullptr) {
        return static_cast<unsigned long long>(m_EnumValue->GetFromValue(ret_Ok));
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0;
}

long long PhysValueInput::GetInt64RawValue(bool *ret_Ok)
{
    if (m_RowValue != nullptr) {
        return m_RowValue->GetInt64Value(ret_Ok);
    }
    if (m_PhysValue != nullptr) {
        double PhysValue = m_PhysValue->GetDoubleValue(ret_Ok);
        double RawValue;
        double RealPhysValue;
        if (m_Vid > 0) {
            if (get_raw_value_for_phys_value (m_Vid, PhysValue, &RawValue, &RealPhysValue)) {
                ThrowError (1, "cannot convert physical to raw value");
                if (ret_Ok != nullptr) *ret_Ok = false;
                return static_cast<unsigned int>(PhysValue);
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return static_cast<unsigned int>(RawValue);
            }
        } else if (m_ConvString != nullptr) {
            bool Ok;
            RawValue = ConvertPhysToRaw(PhysValue, &RealPhysValue, &Ok);
            if (!Ok) {
                if (ret_Ok != nullptr) *ret_Ok = false;
                return static_cast<long long>(PhysValue);
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return static_cast<long long>(RawValue);
            }
        } else {
            if (ret_Ok != nullptr) *ret_Ok = false;
            return 0.0;
        }
    }
    if (m_EnumValue != nullptr) {
        return static_cast<long long>(m_EnumValue->GetFromValue(ret_Ok));
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0;
}

enum BB_DATA_TYPES PhysValueInput::GetUnionRawValue(union BB_VARI *ret_Value, bool *ret_Ok)
{
    if (m_RowValue != nullptr) {
        return m_RowValue->GetUnionValue(ret_Value, ret_Ok);
    }
    if (m_PhysValue != nullptr) {
        double Ret = m_PhysValue->GetDoubleValue(ret_Ok);
        // convert
        ret_Value->d = Ret;
        return BB_DOUBLE;
    }
    if (m_EnumValue != nullptr) {
        // todo
        return BB_UNKNOWN;
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return BB_UNKNOWN;
}

double PhysValueInput::GetDoublePhysValue(bool *ret_Ok)
{
    if (m_PhysValue != nullptr) {
        double Ret = m_PhysValue->GetDoubleValue(ret_Ok);
        // convert
        return static_cast<unsigned long long>(Ret);
    }
    if (m_RowValue != nullptr) {
        double Ret = m_RowValue->GetDoubleValue(ret_Ok);
        // convert
        return Ret;
    }
    if (m_EnumValue != nullptr) {
        // todo
        return 0.0;
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0;
}

QString PhysValueInput::GetStringEnumValue()
{
    if (m_EnumValue != nullptr) {
        return m_EnumValue->currentText();
    }
    return QString();
}

void PhysValueInput::SetRawMinMaxValue(double par_MinValue, double par_MaxValue)
{
    if (m_RowValue != nullptr) m_RowValue->SetMinMaxValue(par_MinValue, par_MaxValue);
    if (m_PhysValue != nullptr) {
        if (m_ConvType != BB_CONV_TEXTREP) {
            double MinPhysValue = ConvertRawToPhys(par_MinValue);
            double MaxPhysValue = ConvertRawToPhys(par_MaxValue);
            m_PhysValue->SetMinMaxValue(MinPhysValue, MaxPhysValue);
        }
    }
}

void PhysValueInput::SetRawOnlyInt(bool par_OnlyInt)
{
    m_RawOnlyInt = par_OnlyInt;
    if (m_RowValue != nullptr) {
        m_RowValue->SetOnlyInteger(par_OnlyInt);
    }
}

void PhysValueInput::resizeEvent(QResizeEvent * event)
{
    ChangeStructurePos(event->size());
}

bool PhysValueInput::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            QObject *Parent = parent();
            #ifdef DEBUG_EVENT
            qDebug() << "PhysValueInput::eventFilter Qt::Key_Tab or Qt::Key_Backtab" << "postEvent to" << Parent->objectName();
            #endif
            QApplication::postEvent(Parent, new QKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers()));
            // Filter this event because the editor will be closed anyway
            return true;
        }
    } else if(event->type() == QEvent::FocusOut) {
        QFocusEvent* focusEvent = static_cast<QFocusEvent *>(event);
        QObject *Parent = parent();
        #ifdef DEBUG_EVENT
        qDebug() << "PhysValueInput::eventFilter QEvent::FocusOut" << "postEvent to" << Parent->objectName();
        #endif
        QApplication::postEvent(Parent, new QFocusEvent(focusEvent->type(), focusEvent->reason()));

        // Don't filter because focus can be changed internally in editor
        return false;
    } else if(event->type() == QEvent::FocusIn) {
        //qDebug() << "PhysValueInput::eventFilter(QEvent::FocusIn)";
    }
    return QWidget::eventFilter(obj, event);
}

void PhysValueInput::ChangeStructureElem()
{    
    if (m_ShowRaw) {
        if (m_RowValue == nullptr) {
            m_RowValue = new ValueInput(this);
            m_RowValue->SetWideningSignalEnable(m_WideningSignal);
            setFocusProxy(m_RowValue);
            if (m_EnumValue != nullptr) setTabOrder(m_RowValue, m_EnumValue);
            if (m_PhysValue != nullptr) setTabOrder(m_RowValue, m_PhysValue);
            bool Ret = connect (m_RowValue, SIGNAL(ValueChanged(double, bool)), this, SLOT(RawValueChanged(double, bool)));
            if (!Ret) {
                printf ("connect error\n");
            }
            Ret = connect (m_RowValue, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
            if (!Ret) {
                printf ("connect error\n");
            }
            Ret = connect (m_RowValue, SIGNAL(ShouldWideningSignal(int,int)), this, SLOT(ShouldWideningSlot(int,int)));
            if (!Ret) {
                printf ("connect error\n");
            }
            m_RowValue->CheckResizeSignalShouldSent();  // after connect!
            if (m_SwapValueBetweeRaWPhys) {
                if (m_PhysValue != nullptr) {
                    bool Ok;
                    double PhysValue = m_PhysValue->GetDoubleValue(&Ok);
                    double RawValue = ConvertPhysToRaw(PhysValue, &PhysValue, &Ok);
                    m_DataType = DATA_TYPE_DOUBLE;
                    m_RawDoubleValue = RawValue;
                    m_RowValue->SetValue(RawValue);
                } else {
                    switch (m_DataType) {
                    case DATA_TYPE_INT:
                        if (m_DisplayRawValue == DISPLAY_RAW_VALUE_HEXAMAL) {
                            m_RowValue->SetValue(m_RawIntValue, 16);
                        } else if (m_DisplayRawValue == DISPLAY_RAW_VALUE_BINARY) {
                            m_RowValue->SetValue(m_RawIntValue, 2);
                        } else {
                            m_RowValue->SetValue(m_RawIntValue);
                        }
                        break;
                    case DATA_TYPE_UINT:
                        if (m_DisplayRawValue == DISPLAY_RAW_VALUE_HEXAMAL) {
                            m_RowValue->SetValue(m_RawUIntValue, 16);
                        } else if (m_DisplayRawValue == DISPLAY_RAW_VALUE_BINARY) {
                            m_RowValue->SetValue(m_RawUIntValue, 2);
                        } else {
                            m_RowValue->SetValue(m_RawUIntValue);
                        }
                        break;
                    case DATA_TYPE_DOUBLE:
                    //default:
                        m_RowValue->SetValue(m_RawDoubleValue);
                        break;
                    }
                }
                m_RowValue->SetStepType(m_StepType);
                m_RowValue->show();
            }
        }
    } else {
        if (m_RowValue != nullptr) {
            delete m_RowValue;
            m_RowValue = nullptr;
        }
    }
    if (m_ShowPhys) {
        if (m_ConvType == BB_CONV_TEXTREP) {
            if (m_EnumValue == nullptr) {
                m_EnumValue = new EnumComboBox(this);
                m_EnumValue->SetWideningSignalEnable(m_WideningSignal);
                setFocusProxy(m_EnumValue);
                if (m_RowValue != nullptr) setTabOrder(m_RowValue, m_EnumValue);
                FillComboBox();  // before connect!
                bool Ret = connect (m_EnumValue, SIGNAL(currentIndexChanged(int)), this, SLOT(EmunValueChanged(int)));
                if (!Ret) {
                    printf ("connect error\n");
                }
                Ret = connect (m_EnumValue, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
                if (!Ret) {
                    printf ("connect error\n");
                }
                Ret = connect (m_EnumValue, SIGNAL(ShouldWideningSignal(int,int)), this, SLOT(ShouldWideningSlot(int,int)));
                if (!Ret) {
                    printf ("connect error\n");
                }
                m_EnumValue->CheckResizeSignalShouldSent();  // after connect!
                if (m_SwapValueBetweeRaWPhys) {
                    if (m_RowValue != nullptr) {
                        bool Ok;
                        double RawValue = m_RowValue->GetDoubleValue(&Ok);
                        m_DataType = DATA_TYPE_DOUBLE;
                        m_RawDoubleValue = RawValue;
                        m_EnumValue->SetValue(static_cast<int>(RawValue));
                    } else {
                        double RawValue;
                        switch (m_DataType) {
                        case DATA_TYPE_INT:
                            RawValue = m_RawIntValue;
                            break;
                        case DATA_TYPE_UINT:
                            RawValue = m_RawUIntValue;
                            break;
                        case DATA_TYPE_DOUBLE:
                        //default:
                            RawValue = m_RawDoubleValue;
                            break;
                        }
                        m_EnumValue->SetValue(static_cast<int>(RawValue));
                    }
                }
                m_EnumValue->show();
            }
            if (m_PhysValue != nullptr) {
                delete m_PhysValue;
                m_PhysValue = nullptr;
            }
        } else {
            if (m_PhysValue == nullptr) {
                m_PhysValue = new ValueInput(this);
                m_PhysValue->SetWideningSignalEnable(m_WideningSignal);
                setFocusProxy(m_PhysValue);
                if (m_RowValue != nullptr) setTabOrder(m_RowValue, m_PhysValue);
                bool Ret = connect (m_PhysValue, SIGNAL(ValueChanged(double, bool)), this, SLOT(PhysValueChanged(double, bool)));
                if (!Ret) {
                    printf ("connect error\n");
                }
                Ret = connect (m_PhysValue, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
                if (!Ret) {
                    printf ("connect error\n");
                }
                Ret = connect (m_PhysValue, SIGNAL(ShouldWideningSignal(int,int)), this, SLOT(ShouldWideningSlot(int,int)));
                if (!Ret) {
                    printf ("connect error\n");
                }
                m_PhysValue->CheckResizeSignalShouldSent();  // after connect!
                if (m_SwapValueBetweeRaWPhys) {
                    if (m_RowValue != nullptr) {
                        bool Ok;
                        double RawValue = m_RowValue->GetDoubleValue(&Ok);
                        double PhysValue = ConvertRawToPhys(RawValue, &Ok);
                        m_DataType = DATA_TYPE_DOUBLE;
                        m_RawDoubleValue = RawValue;
                        m_PhysValue->SetValue(PhysValue);
                    } else {
                        double RawValue;
                        switch (m_DataType) {
                        case DATA_TYPE_INT:
                            RawValue = m_RawIntValue;
                            break;
                        case DATA_TYPE_UINT:
                            RawValue = m_RawUIntValue;
                            break;
                        case DATA_TYPE_DOUBLE:
                            RawValue = m_RawDoubleValue;
                            break;
                        default:
                            RawValue = 0.0;
                            break;
                        }
                        bool Ok;
                        double PhysValue = ConvertRawToPhys(RawValue, &Ok);
                        m_PhysValue->SetValue(PhysValue);
                    }
                }
                m_PhysValue->SetStepType(m_StepType);
                m_PhysValue->show();
            }
            if (m_EnumValue != nullptr) {
                delete m_EnumValue;
                m_EnumValue = nullptr;
            }
        }
    } else {
        if (m_EnumValue != nullptr) {
            if (m_Model != nullptr) {
                delete m_Model;
                m_Model = nullptr;
            }
            delete m_EnumValue;
            m_EnumValue = nullptr;
        }
        if (m_PhysValue != nullptr) {
            delete m_PhysValue;
            m_PhysValue = nullptr;
        }
    }
    if (m_RowValue != nullptr) m_RowValue->SetConfigButtonEnable(m_WithConfig);
    else if (m_PhysValue != nullptr) m_PhysValue->SetConfigButtonEnable(m_WithConfig);
    if (m_RowValue != nullptr) m_RowValue->SetPlusMinusButtonEnable(m_WithPlusMinus);
    if (m_PhysValue != nullptr) m_PhysValue->SetPlusMinusButtonEnable(m_WithPlusMinus);

    if (m_Header) {
        if (m_RowValue != nullptr) {
            m_RowValue->SetHeader("raw");
        }
        if (m_PhysValue != nullptr) {
            m_PhysValue->SetHeader("physical");
        }
        if (m_EnumValue != nullptr) {
            m_EnumValue->SetHeader("text replace");
        }
    } else {
        // TODO:  switchoff header!
    }
}


void PhysValueInput::ChangeStructurePos(QSize Size)
{
    int Width = Size.width();
    int Height = Size.height();
    if (Width < Height) Width = Height;
    if (m_ShowRaw) {
        if (m_ShowPhys) {
            if (m_ConvType == BB_CONV_TEXTREP) {
                if (m_RowValue != nullptr) {
                    m_RowValue->resize(Width/3, Height);
                    m_RowValue->move(0, 0);
                }
                if (m_EnumValue != nullptr) {
                    m_EnumValue->resize(2*Width/3 - Height/3, Height);
                    m_EnumValue->move(Width/3 + Height/3, 0);
                }
            } else {
                if (m_RowValue != nullptr) {
                    m_RowValue->resize(Width/2 - Height/3, Height);
                    m_RowValue->move(0, 0);
                }
                if (m_PhysValue != nullptr) {
                    m_PhysValue->resize(Width/2, Height);
                    m_PhysValue->move(Width/2, 0);
                }
            }
        } else {
            if (m_RowValue != nullptr) {
                m_RowValue->resize(Width, Height);
                m_RowValue->move(0, 0);
            }
        }
    } else if (m_ShowPhys) {
        if (m_ConvType == BB_CONV_TEXTREP) {
            if (m_EnumValue != nullptr) {
                m_EnumValue->resize(Width, Height);
                m_EnumValue->move(0, 0);
            }
        } else {
            if (m_PhysValue != nullptr) {
                m_PhysValue->resize(Width, Height);
                m_PhysValue->move(0, 0);
            }
        }
    }
}

double PhysValueInput::ConvertRawToPhys(double par_RawValue, bool *ret_Ok)
{
    if (m_Vid > 0) {
        double Ret;
        int State = get_phys_value_for_raw_value (m_Vid, par_RawValue, &Ret);
        if (ret_Ok != nullptr) *ret_Ok = (State == 0);
        return Ret;
    }
    if (m_ConvString != nullptr) {
        double PhysValue;
        char *ErrString;
        if (Conv_RawToPhysFromString(m_ConvString, m_ConvType, m_BlackboardVariableName, par_RawValue, &PhysValue, &ErrString)) {
            ThrowError (1, "cannot convert raw to physical value because the conversion \"%s\" has error \"%s\"", m_ConvString, ErrString);
            FreeErrStringBuffer (&ErrString);
            if (ret_Ok != nullptr) *ret_Ok = false;
            return par_RawValue;
        } else {
            if (ret_Ok != nullptr) *ret_Ok = true;
            return PhysValue;
        }
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0.0;
}

double PhysValueInput::ConvertPhysToRaw(double par_PhysValue, double *ret_PhysValue, bool *ret_Ok)
{
    if (m_Vid > 0) {
        double RawValue;
        if (get_raw_value_for_phys_value (m_Vid, par_PhysValue, &RawValue, ret_PhysValue)) {
            if (ret_Ok != nullptr) *ret_Ok = false;
            return par_PhysValue;
        } else {
            if (ret_Ok != nullptr) *ret_Ok = true;
            return RawValue;
        }
    } else {
        if ((m_ConvString != nullptr)) {
            double RawValue;
            char *ErrString;
            if (Conv_PhysToRawFromString(m_ConvString, m_ConvType, m_BlackboardVariableName, par_PhysValue, &RawValue, ret_PhysValue, &ErrString)) {
                ThrowError (1, "cannot convert physical to raw value because the conversion \"%s\" has error \"%s\"", m_ConvString, ErrString);
                FreeErrStringBuffer (&ErrString);
                if (ret_Ok != nullptr) *ret_Ok = false;
                return par_PhysValue;
            } else {
                if (ret_Ok != nullptr) *ret_Ok = true;
                return RawValue;
            }
        }
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0.0;
}

void PhysValueInput::FillComboBox()
{
    if (m_EnumValue != nullptr) {
        if (m_ConvString == nullptr) {
            if (m_Vid > 0) {
                int Len = 0;
                while ((Len = -get_bbvari_conversion(m_Vid, m_ConvString, Len)) > 0) {
                    m_ConvString = static_cast<char*>(my_realloc (m_ConvString, static_cast<size_t>(Len)));
                }
            }
        }
        if (m_ConvString != nullptr) {
            if (m_Model == nullptr) {
                m_Model = new EnumComboBoxModel(m_ConvString, m_EnumValue);
                m_EnumValue->setModel(m_Model);
            }
        }
    }
}

QString PhysValueInput::DoubleToString(double par_Value)
{
    char Help[256];
    int Prec = 15;
    while (1) {
        PrintFormatToString (Help, sizeof(Help), "%.*g", Prec, par_Value);
        if ((Prec++) == 18 || (par_Value == strtod(Help, nullptr))) break;
    }
    return QString(Help);
}

void PhysValueInput::EnableAutoScaling(void)
{
    m_AutoScalingActive = true;
    if (m_RowValue != nullptr) m_RowValue->EnableAutoScaling();
    if (m_PhysValue != nullptr) m_PhysValue->EnableAutoScaling();
    if (m_EnumValue != nullptr) m_EnumValue->EnableAutoScaling();
}

void PhysValueInput::DisableAutoScaling(void)
{
    m_AutoScalingActive = false;
    if (m_RowValue != nullptr) m_RowValue->DisableAutoScaling();
    if (m_PhysValue != nullptr) m_PhysValue->DisableAutoScaling();
    if (m_EnumValue != nullptr) m_EnumValue->DisableAutoScaling();
}

bool PhysValueInput::GetConfigButtonEnable()
{
    return m_WithConfig;
}

void PhysValueInput::SetConfigButtonEnable(bool par_State)
{
    m_WithConfig = par_State;
    ChangeStructureElem();
    ChangeStructurePos(size());
}

bool PhysValueInput::GetPlusMinusButtonEnable()
{
    return m_WithPlusMinus;
}

void PhysValueInput::SetPlusMinusButtonEnable(bool par_State)
{
    m_WithPlusMinus = par_State;
    ChangeStructureElem();
    ChangeStructurePos(size());
}

double PhysValueInput::GetPlusMinusIncrement()
{
    return m_PlusMinusIncrement;
}

void PhysValueInput::SetPlusMinusIncrement(double par_Increment)
{
    m_PlusMinusIncrement = par_Increment;
    if (m_RowValue != nullptr) m_RowValue->SetPlusMinusIncrement(par_Increment);
    if (m_PhysValue != nullptr) m_PhysValue->SetPlusMinusIncrement(par_Increment);
}

void PhysValueInput::SetStepLinear(bool par_Linear)
{
    if (m_RowValue != nullptr) m_RowValue->SetStepLinear(par_Linear);
    if (m_PhysValue != nullptr) m_PhysValue->SetStepLinear(par_Linear);
    if (par_Linear) {
        m_StepType = ValueInput::LINEAR_STEP;
    } else {
        m_StepType = ValueInput::PERCENTAGE_STEP;
    }
}

void PhysValueInput::SetStepPercentage(bool par_Percentage)
{
    if (m_RowValue != nullptr) m_RowValue->SetStepPercentage(par_Percentage);
    if (m_PhysValue != nullptr) m_PhysValue->SetStepPercentage(par_Percentage);
    if (par_Percentage) {
        m_StepType = ValueInput::PERCENTAGE_STEP;
    } else {
        m_StepType = ValueInput::LINEAR_STEP;
    }
}

void PhysValueInput::SetDispayFormat(int par_Base)
{
    bool Ok;
    union BB_VARI Value;
    enum BB_DATA_TYPES Type = GetUnionRawValue(&Value, &Ok);
    switch (par_Base) {
    case 2:
    case 10:
    case 16:
        SetRawValue(Value, Type, par_Base, false);  // don't change the data type
        break;
    }
}

void PhysValueInput::SetHeaderState(bool pat_State)
{
    m_Header = pat_State;
    ChangeStructureElem();
    ChangeStructurePos(size());
}


QSize PhysValueInput::sizeHint () const
{
    QSize Size = QWidget::sizeHint();
    Size.setWidth(100);
    Size.setHeight(40);
    return Size;
}

QSize PhysValueInput::minimumSizeHint() const
{
    QSize Size = QWidget::minimumSizeHint();
    Size.setWidth(100);
    Size.setHeight(40);
    return Size;
}

void PhysValueInput::setFocus(Qt::FocusReason reason)
{
    if (m_PhysValue != nullptr) m_PhysValue->setFocus(reason);
    else if (m_EnumValue != nullptr) m_EnumValue->setFocus(reason);
    else if (m_RowValue != nullptr) m_RowValue->setFocus(reason);
}


bool PhysValueInput::hasFocus() const
{
    bool Ret = false;
    if (m_PhysValue != nullptr) Ret = Ret || m_PhysValue->hasFocus();
    else if (m_EnumValue != nullptr) Ret = Ret || m_EnumValue->hasFocus();
    else if (m_RowValue != nullptr) Ret = Ret || m_RowValue->hasFocus();
    return Ret;
}

void PhysValueInput::SetWideningSignalEnable(bool par_Enable)
{
    m_WideningSignal = par_Enable;
    if (m_PhysValue != nullptr) m_PhysValue->SetWideningSignalEnable(par_Enable);
    if (m_RowValue != nullptr) m_RowValue->SetWideningSignalEnable(par_Enable);
    if (m_EnumValue != nullptr) m_EnumValue->SetWideningSignalEnable(par_Enable);
}

void PhysValueInput::PhysValueChanged(double par_Value, bool par_OutOfRange)
{
    Q_UNUSED(par_OutOfRange);
     if (m_ShowRaw && (m_RowValue != nullptr)) {
         bool Ok;
         double PhysValue;
         double RawValue = ConvertPhysToRaw(par_Value, &PhysValue, &Ok);
         if (m_PhysValue != nullptr) m_PhysValue->SetValueInvalid(!Ok);
         if (Ok) {
             if (!m_RawPhysValueChangedPingPong) {
                 m_RawPhysValueChangedPingPong = true;
                 m_RowValue->SetValue(RawValue);
                 m_RawPhysValueChangedPingPong = false;
             }
         }
     }
     emit ValueChanged();
}

void PhysValueInput::RawValueChanged(double par_Value, bool par_OutOfRange)
{
    Q_UNUSED(par_OutOfRange);
    if (m_ShowPhys) {
        if (m_PhysValue != nullptr) {
            bool Ok;
            double PhysValue = ConvertRawToPhys(par_Value, &Ok);
            if (m_RowValue != nullptr) m_RowValue->SetValueInvalid(!Ok);
            if (Ok) {
                if (!m_RawPhysValueChangedPingPong) {
                    m_RawPhysValueChangedPingPong = true;
                    m_PhysValue->SetValue(PhysValue);
                    m_RawPhysValueChangedPingPong = false;
                }
            }
        } else if (m_EnumValue != nullptr) {
            if (m_ConvString != nullptr) {
                char Enum[1024];
                int Color;
                if (convert_value2textreplace (static_cast<int32_t>(par_Value), m_ConvString, Enum, sizeof(Enum), &Color) == 0) {
                    if (!m_RawPhysValueChangedPingPong) {
                        m_RawPhysValueChangedPingPong = true;
                        m_EnumValue->setCurrentText(QString(Enum));
                        m_RawPhysValueChangedPingPong = false;
                    }
                }
            }
        }
    }
    emit ValueChanged();
}

void PhysValueInput::EmunValueChanged(int Index)
{
    double RawValue = m_Model->GetFromValue(Index);
    if (!m_RawPhysValueChangedPingPong) {
        m_RawPhysValueChangedPingPong = true;
        if (m_RowValue != nullptr) m_RowValue->SetValue(RawValue);
        m_RawPhysValueChangedPingPong = false;
    }
    emit ValueChanged();
}

void PhysValueInput::editingFinishedSlot()
{
    emit editingFinished();
}

void PhysValueInput::ShouldWideningSlot(int Widening, int Heightening)
{
    if (m_WideningSignal) {
        // If 2 elements visable than expand 2*
        if ((m_RowValue != nullptr) && ((m_PhysValue != nullptr) || (m_EnumValue != nullptr))) Widening *= 2;
        emit ShouldWideningSignal(Widening, Heightening);
    }
}


EnumComboBoxModel::EnumComboBoxModel(char *par_EnumString, QObject *parent) : QAbstractListModel(parent)
{
    int Index = 0;
    char Enum[1024];
    int64_t From;
    int64_t To;
    int32_t Color;
    int Row = 0;
    char ErrMsgBuffer[1024];

    if (par_EnumString != nullptr) {
        while (1) {
            int Ret = GetNextEnumFromEnumListErrMsg (Index, par_EnumString,
                                                     Enum, sizeof(Enum), &From, &To, &Color, ErrMsgBuffer, sizeof (ErrMsgBuffer));
            if (Ret >= 0) {
                EnumComboBoxModelItem NewItem;
                NewItem.Color = Color;
                NewItem.From = From;
                NewItem.To = To;
                NewItem.m_Enum = QString(Enum);
                NewItem.Valid = true;
                m_EnumList.append(NewItem);
            }
            if (Ret == -1) {
                // End of the text replace string reached
                break;
            } else if (Ret == -2) {
                ThrowError (1, "text replace string\n"
                          "%s\n"
                          "has error(s): %s", par_EnumString, ErrMsgBuffer);
                break;
            }
            Row++;
            Index++;
        }
    }
    EnumComboBoxModelItem NewItem;
    NewItem.Color = -1;
    NewItem.From = 0;
    NewItem.To = 0;
    NewItem.m_Enum = QString("out of range");
    NewItem.Valid = false;
    m_EnumList.append(NewItem);
}

int EnumComboBoxModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_EnumList.size();
}

QVariant EnumComboBoxModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::BackgroundRole) {
       int Color = m_EnumList.at(index.row()).Color;
       if (Color == -1) return QVariant();
       else return QColor(Color & 0x000000FF, (Color & 0x0000FF00) >> 8, (Color & 0x00FF0000) >> 16); // Sh. Win Colors 0x00bbggrr
    }
    if (role == Qt::DisplayRole) {
        return m_EnumList.at(index.row()).m_Enum;
    } else {
        return QVariant();
    }
}

int EnumComboBoxModel::GetFromValue(int Index)
{
    return static_cast<int>(m_EnumList.at(Index).From);
}

int EnumComboBoxModel::GetToValue(int Index)
{
    return static_cast<int>(m_EnumList.at(Index).To);
}

QString EnumComboBoxModel::GetEnumString(int Index)
{
    return m_EnumList.at(Index).m_Enum;
}

bool EnumComboBoxModel::IsValid(int Index)
{
    return m_EnumList.at(Index).Valid;
}

int EnumComboBoxModel::GetMaxEnumWidth(QFontMetrics par_FontMetrics)
{
    int Max = 0;
    foreach (EnumComboBoxModelItem Item, m_EnumList) {
        int Width = par_FontMetrics.boundingRect(Item.m_Enum).width();
        if (Width > Max) Max = Width;
    }
    return Max;
}


EnumComboBox::EnumComboBox(QWidget *parent) : QWidget(parent)
{
    setObjectName("EnumComboBox");
    sizePolicy().setControlType(QSizePolicy::GroupBox);
    m_AutoScalingActive = true;
    m_ComboBox = new EComboBox(this);
    m_Header = nullptr;
    connect (m_ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChangedSlot(int)));
    connect (m_ComboBox, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
    setFocusPolicy(Qt::WheelFocus);
    setFocusProxy(m_ComboBox);
    installEventFilter(this);
    m_WideningSignal = false;
}

void EnumComboBox::EnableAutoScaling()
{
    m_AutoScalingActive = true;
}

void EnumComboBox::DisableAutoScaling()
{
    m_AutoScalingActive = false;
}

void EnumComboBox::SetValue(int par_Value)
{
    EnumComboBoxModel *Model = static_cast<EnumComboBoxModel*>(model());
    for (int x = 0; x < Model->rowCount(); x++) {
        if (Model->IsValid(x)) {
            if ((Model->GetFromValue(x) >= par_Value) && (Model->GetToValue(x) <= par_Value)) {
                setCurrentText(Model->GetEnumString(x));
            }
        }
    }
    CheckResizeSignalShouldSent();
}

int EnumComboBox::GetFromValue(bool *ret_Ok)
{
    EnumComboBoxModel *Model = static_cast<EnumComboBoxModel*>(model());
    QString Enum = currentText();
    for (int x = 0; x < Model->rowCount(); x++) {
        if (Enum.compare(Model->GetEnumString(x)) == 0) {
            if (ret_Ok != nullptr) *ret_Ok = true;
            return Model->GetFromValue(x);
        }
    }
    if (ret_Ok != nullptr) *ret_Ok = false;
    return 0;
}

void EnumComboBox::SetHeader(QString par_Header)
{
    if (m_Header != nullptr) m_Header->setText(par_Header);
    else {
        m_Header = new QLabel(par_Header, this);
        m_Header->show();
    }
}

void EnumComboBox::setCurrentText(const QString &par_EnumValue)
{
    m_ComboBox->setCurrentText(par_EnumValue);
}

QString EnumComboBox::currentText()
{
    return m_ComboBox->currentText();
}

void EnumComboBox::setModel(EnumComboBoxModel *par_Model)
{
    m_ComboBox->setModel(par_Model);
}

EnumComboBoxModel *EnumComboBox::model()
{
    return static_cast<EnumComboBoxModel*>(m_ComboBox->model());
}

bool EnumComboBox::hasFocus() const
{
    bool Ret = false;
    if (m_ComboBox != nullptr) Ret = m_ComboBox->hasFocus();
    return Ret;
}

void EnumComboBox::SetWideningSignalEnable(bool par_Enable)
{
    m_WideningSignal = par_Enable;
}

void EnumComboBox::CheckResizeSignalShouldSent()
{
    if (m_WideningSignal) {
        EnumComboBoxModel *Model = model();
        int NeededWith = Model->GetMaxEnumWidth(m_ComboBox->fontMetrics());
        int Widening = 0;
        int Heightening = 0;
        if (NeededWith > width()) {
            Widening = NeededWith - width();
        }
        int NeededHight = m_ComboBox->fontMetrics().height();
        NeededHight = NeededHight + NeededHight/8 + 12;
        int Hight = height();
        if (NeededHight > Hight) {
            Heightening = NeededHight - height();
        }
        if ((Widening > 0) || (Heightening > 0)) {
            emit ShouldWideningSignal(Widening, Heightening);
        }
    }
}

void EnumComboBox::currentIndexChangedSlot(int Index)
{
    CheckResizeSignalShouldSent();
    emit currentIndexChanged(Index);  // transfer
}

void EnumComboBox::editingFinishedSlot()
{
    emit editingFinished();  // only transfer the signal
}

void EnumComboBox::resizeEvent(QResizeEvent *event)
{
    ChangeStructurePos(event->size());
    if (m_AutoScalingActive) {
        int WinHeight =  event->size().height();
        if (m_Header != nullptr) {
            WinHeight -= 13;
        }
        int FontSize = WinHeight - (2 * WinHeight / 4) - 3;
        QFont Font = m_ComboBox->font();
        Font.setPointSize (FontSize);
        m_ComboBox->setFont (Font);
    }
}

bool EnumComboBox::eventFilter(QObject *obj, QEvent *event)
{
    if((event->type() == QEvent::KeyPress)) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            QObject *Parent = parent();
            #ifdef DEBUG_EVENT
            qDebug() << "EnumComboBox::eventFilter Qt::Key_Tab or Qt::Key_Backtab" << "postEvent to" << Parent->objectName();
            #endif
            QApplication::postEvent(Parent, new QKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers()));
            // Filter this event because the editor will be closed anyway
            return true;
        }
    } else if(event->type() == QEvent::FocusOut) {
        QFocusEvent* focusEvent = static_cast<QFocusEvent *>(event);
        QObject *Parent = parent();
        #ifdef DEBUG_EVENT
        qDebug() << "EnumComboBox::eventFilter QEvent::FocusOut" << "postEvent to" << Parent->objectName();
        #endif
        QApplication::postEvent(Parent, new QFocusEvent(focusEvent->type(), focusEvent->reason()));

        // Don't filter because focus can be changed internally in editor
        return false;
    }
    return QWidget::eventFilter(obj, event);

}

void EnumComboBox::ChangeStructurePos(QSize Size)
{
    int Width = Size.width();
    int Height = Size.height();
    int LineHeight;
    int HeaderHeight;
    if (m_Header != nullptr) {
        HeaderHeight = 13;
        LineHeight = Height - HeaderHeight;
    } else {
        HeaderHeight = 0;
        LineHeight = Height;
    }
    if (Width < Height) Width = Height;
    if (m_Header != nullptr) {
        m_Header->resize(Width, HeaderHeight);
        m_Header->move(0, 0);
    }
    m_ComboBox->resize(Width, LineHeight);
    m_ComboBox->move(0, HeaderHeight);
}



EComboBox::EComboBox(QWidget *parent) : QComboBox(parent)
{
    installEventFilter(this);
}

bool EComboBox::eventFilter(QObject *obj, QEvent *event)
{
    if((event->type() == QEvent::KeyPress)) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            QObject *Parent = parent();
            #ifdef DEBUG_EVENT
            qDebug() << "EComboBox::eventFilter Qt::Key_Tab or Qt::Key_Backtab" << "postEvent to" << Parent->objectName();
            #endif
            QApplication::postEvent(Parent, new QKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers()));
            // Filter this event because the editor will be closed anyway
            return true;
        }
    } else if(event->type() == QEvent::FocusOut) {
        QFocusEvent* focusEvent = static_cast<QFocusEvent *>(event);
        QObject *Parent = parent();
        #ifdef DEBUG_EVENT
        qDebug() << "EComboBox::eventFilter QEvent::FocusOut" << "postEvent to" << Parent->objectName();
        #endif
        QApplication::postEvent(Parent, new QFocusEvent(focusEvent->type(), focusEvent->reason()));

        // This signal don't exists inside a combobox
        // emulate it
        emit editingFinished();
        // Don't filter because focus can be changed internally in editor
        return false;
    }
    return QComboBox::eventFilter(obj, event);
}

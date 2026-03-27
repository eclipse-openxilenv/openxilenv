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


#include <math.h>
#include "TextValueInput.h"
#include <QResizeEvent>
extern "C" {
#include "PrintFormatToString.h"
#include "ThrowError.h"
}

TextValueInput::TextValueInput(QWidget* parent) : QLineEdit (parent)
{
    m_MinMaxCheck = false;
    m_OutOfRange = false;
    m_AutoScalingActive = true;

    QRegularExpression rx("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$|"   // Double
                                  "[-+]?0[xX][0-9a-fA-F]+|"   // Hex
                                  "[-+]?(0|[1-9][0-9]*)$");  // Int

    m_Validator.setRegularExpression(rx);

    setValidator(&m_Validator);
    setAlignment(Qt::AlignRight);
    bool Ret = connect (this, SIGNAL(textChanged(const QString &)), this, SLOT(CheckValueRangesSlot (const QString &)));
    if (!Ret) {
        ThrowError (1, "connect in TextValueInput::TextValueInput");
    }

    m_PaletteSave = palette();
    m_PaletteInvalidValueRange.setColor (QPalette::Base, Qt::black);
    m_PaletteInvalidValueRange.setColor (QPalette::Text, Qt::white);

    setFocusPolicy (Qt::StrongFocus);
}

TextValueInput::~TextValueInput()
{

}

void TextValueInput::resizeEvent(QResizeEvent * event)
{
    if (m_AutoScalingActive) {
        m_WinHeight =  event->size().height();
        m_FontSize = m_WinHeight - (m_WinHeight / 3) - 3;
        QFont Font = font();
        Font.setPointSize (m_FontSize);
        setFont (Font);
    }
}

void TextValueInput::SetValue (int par_Value, int par_Base)
{
    if (par_Base == 16) {
        if (par_Value < 0) {
            unsigned int Help = static_cast<unsigned int>(-(par_Value + 1));
            Help--;
            setText(QString("-0x") + QString().number(Help, par_Base));
        } else {
            setText(QString("0x") + QString().number(par_Value, par_Base));
        }
    } else if (par_Base == 10) {
        setText(QString().number(par_Value, par_Base));
    } else {
        setText(QString("only base 10 or 16 allowed"));
    }
}

void TextValueInput::SetValue (unsigned int par_Value, int par_Base)
{
    if (par_Base == 16) {
        setText(QString("0x") + QString().number(par_Value, par_Base));
    } else if (par_Base == 10) {
        setText(QString().number(par_Value, par_Base));
    } else {
        setText(QString("only base 10 or 16 allowed"));
    }
}

void TextValueInput::SetValue (uint64_t par_Value, int par_Base)
{
    if (par_Base == 16) {
        setText(QString("0x") + QString().number(par_Value, par_Base));
    } else if (par_Base == 10) {
        setText(QString().number(par_Value, par_Base));
    } else {
        setText(QString("only base 10 or 16 allowed"));
    }
}

void TextValueInput::SetValue(int64_t par_Value, int par_Base)
{
    if (par_Base == 16) {
        if (par_Value < 0) {
            uint64_t Help = static_cast<uint64_t>(-(par_Value + 1));
            Help--;
            setText(QString("-0x") + QString().number(Help, par_Base));
        } else {
            setText(QString("0x") + QString().number(par_Value, par_Base));
        }
    } else if (par_Base == 10) {
        setText(QString().number(par_Value, par_Base));
    } else {
        setText(QString("only base 10 or 16 allowed"));
    }
}

void TextValueInput::SetValue (double par_Value)
{
    char Help[256];
    int Prec = 15;
    while (1) {
        PrintFormatToString (Help, sizeof(Help), "%.*g", Prec, par_Value);
        if ((Prec++) == 18 || (par_Value == strtod(Help, nullptr))) break;
    }
    setText(QString(Help));
}

void TextValueInput::SetValue(FloatOrInt64 par_Value, int par_Type)
{
    switch (par_Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        SetValue(par_Value.qw);
        break;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        SetValue(par_Value.uqw);
        break;
    case FLOAT_OR_INT_64_TYPE_F64:
        SetValue(par_Value.d);
        break;
    default:
    case FLOAT_OR_INT_64_TYPE_INVALID:
        SetValue(0.0);
        break;
    }
}

void TextValueInput::SetMinMaxValue(BB_VARI par_MinValue, union BB_VARI par_MaxValue, enum BB_DATA_TYPES par_Type)
{
    m_MinMaxCheck = true;
    m_MinValue = par_MinValue;
    m_MaxValue = par_MaxValue;
    m_Type = par_Type;
}


double TextValueInput::GetDoubleValue(bool *ret_Ok)
{
    bool Ok = false;
    double DoubleValue = 0.0;
    QString Text = text();
    if (Text.length()) {
        DoubleValue = Text.toDouble(&Ok);
        if (!Ok) {
            DoubleValue = Text.toInt(&Ok);
            if (!Ok) {
                DoubleValue = Text.toInt(&Ok, 16);
                if (!Ok) {
                    DoubleValue = 0.0;
                }
            }
        }
    } else {
        DoubleValue = 0.0;
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return DoubleValue;
}

int TextValueInput::GetIntValue(bool *ret_Ok)
{
    bool Ok = false;
    int IntValue = 0;
    QString Text = text();
    if (Text.length()) {
        IntValue = Text.toInt(&Ok);
        if (!Ok) {
            IntValue = Text.toInt(&Ok, 16);
            if (!Ok) {
                IntValue = static_cast<int>(Text.toDouble(&Ok));
                if (!Ok) {
                    IntValue = 0;
                }
            }
        }
    } else {
        IntValue = 0;
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return IntValue;
}

unsigned int TextValueInput::GetUIntValue(bool *ret_Ok)
{
    bool Ok = false;
    unsigned int IntValue = 0;
    QString Text = text();
    if (Text.length()) {
        IntValue = Text.toUInt(&Ok);
        if (!Ok) {
            IntValue = Text.toUInt(&Ok, 16);
            if (!Ok) {
                IntValue = static_cast<unsigned int>(Text.toDouble(&Ok));
                if (!Ok) {
                    IntValue = 0;
                }
            }
        }
    } else {
        IntValue = 0;
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return IntValue;
}

uint64_t TextValueInput::GetUInt64Value(bool *ret_Ok)
{
    bool Ok = false;
    unsigned long long  IntValue = 0;
    QString Text = text();
    if (Text.length()) {
        IntValue = Text.toULongLong(&Ok);
        if (!Ok) {
            IntValue = Text.toULongLong(&Ok, 16);
            if (!Ok) {
                IntValue = static_cast<unsigned long long>(Text.toDouble(&Ok));
                if (!Ok) {
                    IntValue = 0;
                }
            }
        }
    } else {
        IntValue = 0;
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return IntValue;
}

enum BB_DATA_TYPES TextValueInput::GetUnionValue(union BB_VARI *ret_Value, bool *ret_Ok)
{
    enum BB_DATA_TYPES Ret;
    bool Ok = false;
    QString Text = text();
    if (Text.length()) {
        unsigned long long UIntValue = Text.toULongLong(&Ok);
        if (Ok) {
            ret_Value->uqw = UIntValue;
            Ret = BB_UQWORD;
        } else {
            UIntValue = Text.toULongLong(&Ok, 16);   // Hex without 0x
            if (Ok) {
                ret_Value->uqw = UIntValue;
                Ret = BB_UQWORD;
            } else {
                long long IntValue = Text.toLongLong(&Ok);
                if (Ok) {
                    ret_Value->qw = IntValue;
                    Ret = BB_QWORD;
                } else {
                    IntValue = Text.toLongLong(&Ok, 16);   // Hex without 0x
                    if (Ok) {
                        ret_Value->qw = IntValue;
                        Ret = BB_QWORD;
                    } else {
                        double DoubleValue = Text.toDouble(&Ok);
                        if (Ok) {
                            ret_Value->d = DoubleValue;
                            Ret = BB_DOUBLE;
                        } else {
                            ret_Value->d = 0.0;
                            Ret = BB_DOUBLE;
                        }
                    }
                }
            }
        }
    } else {
         ret_Value->d = 0.0;
         Ret = BB_DOUBLE;
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return Ret;
}

int TextValueInput::GetFloatOrInt64Value(FloatOrInt64 *ret_Value, bool *ret_Ok)
{
    int Ret;
    union BB_VARI Value;
    switch (GetUnionValue(&Value, ret_Ok)) {
    case BB_QWORD:
        ret_Value->qw = Value.qw;
        Ret = FLOAT_OR_INT_64_TYPE_INT64;
        break;
    case BB_UQWORD:
        ret_Value->uqw = Value.uqw;
        Ret = FLOAT_OR_INT_64_TYPE_UINT64;
        break;
    case BB_DOUBLE:
        ret_Value->uqw = Value.uqw;
        Ret = FLOAT_OR_INT_64_TYPE_F64;
        break;
    default:
        Ret = FLOAT_OR_INT_64_TYPE_INVALID;
        if (ret_Ok != nullptr) *ret_Ok = false;
        break;
    }
    return Ret;
}


void TextValueInput::CheckValueRangesSlot (const QString &par_Text)
{
    Q_UNUSED(par_Text)
    if (m_MinMaxCheck) {
        bool Ok;
        bool OutOfRange;
        union BB_VARI Value;
        enum BB_DATA_TYPES Type = GetUnionValue(&Value, &Ok);
        if (Ok) {
            switch (Type) {
            default:
            case BB_DOUBLE:
                switch (m_Type) {
                default:
                case BB_DOUBLE:
                    OutOfRange = (Value.d < m_MinValue.d) || (Value.d > m_MaxValue.d);
                    break;
                case BB_QWORD:
                    OutOfRange = (round(Value.d) < static_cast<double>(m_MinValue.qw)) || (round(Value.d) > static_cast<double>(m_MaxValue.qw));
                    break;
                case BB_UQWORD:
                    OutOfRange = (round(Value.d) < static_cast<double>(m_MinValue.uqw)) || (round(Value.d) > static_cast<double>(m_MaxValue.uqw));
                    break;
                }
                break;
            case BB_QWORD:
                switch (m_Type) {
                default:
                case BB_DOUBLE:
                    OutOfRange = (Value.qw < static_cast<int64_t>(round(m_MinValue.d))) || (Value.qw > static_cast<int64_t>(round(m_MaxValue.d)));
                    break;
                case BB_QWORD:
                    OutOfRange = (Value.qw < m_MinValue.qw) || (Value.qw > m_MaxValue.qw);
                    break;
                case BB_UQWORD:
                    if (m_MinValue.uqw > INT64_MAX) {
                        OutOfRange = true;
                    } else if (m_MaxValue.uqw < INT64_MAX) {
                        OutOfRange = (Value.qw < static_cast<int64_t>(m_MinValue.uqw)) || (Value.qw > static_cast<int64_t>(m_MaxValue.uqw));
                    } else {
                        OutOfRange = (Value.qw < static_cast<int64_t>(m_MinValue.uqw));
                    }
                    break;
                }
                break;
            case BB_UQWORD:
                switch (m_Type) {
                default:
                case BB_DOUBLE:
                    OutOfRange = (Value.uqw < static_cast<uint64_t>(round(m_MinValue.d))) || (Value.uqw > static_cast<uint64_t>(round(m_MaxValue.d)));
                    break;
                case BB_QWORD:
                    if (m_MaxValue.qw < 0) {
                        OutOfRange = true;
                    } else {
                        if (m_MinValue.qw < 0) {
                            OutOfRange = (Value.uqw > static_cast<uint64_t>(m_MaxValue.qw));
                        } else {
                            OutOfRange = (Value.uqw < static_cast<uint64_t>(m_MinValue.qw)) || (Value.uqw > static_cast<uint64_t>(m_MaxValue.qw));
                        }
                    }
                    break;
                case BB_UQWORD:
                    OutOfRange = (Value.uqw < m_MinValue.uqw) || (Value.uqw > m_MaxValue.uqw);
                    break;
                }
                break;
            }
        } else {
            OutOfRange = true;
        }
        if (OutOfRange && !m_OutOfRange) {
            setPalette (m_PaletteInvalidValueRange);
            m_OutOfRange = true;
        } else if (!OutOfRange && m_OutOfRange) {
            setPalette (m_PaletteSave);
            m_OutOfRange = false;
        }
    }
}

void TextValueInput::SetMinMaxValue (double par_MinValue, double par_MaxValue)
{
    m_MinMaxCheck = true;
    m_MinValue.d = par_MinValue;
    m_MaxValue.d = par_MaxValue;
    m_Type = BB_DOUBLE;
}

void TextValueInput::EnableAutoScaling(void)
{
    m_AutoScalingActive = true;
}

void TextValueInput::DisableAutoScaling(void)
{
    m_AutoScalingActive = false;
}

QSize TextValueInput::sizeHint () const
{
    QSize Size = QLineEdit::sizeHint();
    return Size;
}

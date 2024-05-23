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
#include "ValueInput.h"
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QApplication>

//#define DEBUG_EVENT

#ifdef DEBUG_EVENT
#include <QDebug>
#endif

#include <limits.h>

ValueInput::ValueInput(QWidget* parent) : QWidget (parent)
{
    setObjectName("ValueInput");
    sizePolicy().setControlType(QSizePolicy::GroupBox);
    m_MinMaxCheck = false;
    m_Type = BB_DOUBLE;
    m_OutOfRange = false;
    m_AutoScalingActive = true;

    m_LineEdit = new MyLineEdit(this);

    SetOnlyInteger(false);

    m_WithPlusMinus = false;
    m_WithConfig = false;

    m_Header = nullptr;
    m_ButtonPlus = nullptr;
    m_ButtonMinus = nullptr;
    m_ButtonConfig = nullptr;
    m_PlusMinusIncrement = 1.0;
    m_StepType = LINEAR_STEP;

   // m_LineEdit->setValidator(&m_Validator);
    m_LineEdit->setAlignment(Qt::AlignRight);
    bool Ret = connect (m_LineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(CheckValueRangesSlot (const QString &)));
    if (!Ret) {
        printf ("connect error\n");
    }
    Ret = connect (m_LineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
    if (!Ret) {
        printf ("connect error\n");
    }


    m_PaletteSave = m_LineEdit->palette();
    m_PaletteInvalidValueRange.setColor (QPalette::Base, Qt::black);
    m_PaletteInvalidValueRange.setColor (QPalette::Text, Qt::white);

    //m_LineEdit->setFocusPolicy (Qt::StrongFocus);
    setFocusPolicy(Qt::WheelFocus);
    setFocusProxy(m_LineEdit);

    installEventFilter(this);

    m_WideningSignal = false;
}

ValueInput::~ValueInput()
{

}

void ValueInput::resizeEvent(QResizeEvent * event)
{
    ChangeStructurePos(event->size());
    if (m_AutoScalingActive) {
        m_WinHeight =  event->size().height();
        if (m_Header != nullptr) {
            m_WinHeight -= 13;
        }
        m_FontSize = m_WinHeight - (m_WinHeight / 3) - 3;
        QFont Font = m_LineEdit->font();
        Font.setPointSize (m_FontSize);
        m_LineEdit->setFont (Font);
    }
}

bool ValueInput::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            QObject *Parent = parent();
            #ifdef DEBUG_EVENT
            qDebug() << "ValueInput::eventFilter Qt::Key_Tab or Qt::Key_Backtab" << "postEvent to" << Parent->objectName();
            #endif
            QApplication::postEvent(Parent, new QKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers()));
            // Filter this event because the editor will be closed anyway
            return true;
        }
    } else if(event->type() == QEvent::FocusOut) {
        QFocusEvent* focusEvent = static_cast<QFocusEvent *>(event);
        QObject *Parent = parent();
        #ifdef DEBUG_EVENT
        qDebug() << "ValueInput::eventFilter QEvent::FocusOut" << "postEvent to" << Parent->objectName();
        #endif
        QApplication::postEvent(Parent, new QFocusEvent(focusEvent->type(), focusEvent->reason()));

        // Don't filter because focus can be changed internally in editor
        return false;
    }
    return QWidget::eventFilter(obj, event);
}


void ValueInput::ChangeStructureElem()
{
    if (m_WithPlusMinus) {
        if (m_ButtonPlus == nullptr) {
            m_ButtonPlus = new QPushButton("+", this);
            connect (m_ButtonPlus, SIGNAL(clicked()), this, SLOT(IncrementValue()));
            m_ButtonPlus->setFocusPolicy(Qt::NoFocus);
            m_ButtonPlus->show();
        }
        if (m_ButtonMinus == nullptr) {
            m_ButtonMinus = new QPushButton("-", this);
            m_ButtonMinus->setFocusPolicy(Qt::NoFocus);
            connect (m_ButtonMinus, SIGNAL(clicked()), this, SLOT(DecrementValue()));
            m_ButtonMinus->show();
        }
    } else {
        if (m_ButtonPlus != nullptr) {
            delete m_ButtonPlus;
            m_ButtonPlus = nullptr;

        }
        if (m_ButtonMinus != nullptr) {
            delete m_ButtonMinus;
            m_ButtonMinus = nullptr;
        }
    }
    if (m_WithConfig) {
        if (m_ButtonConfig == nullptr) {
            m_ButtonConfig = new QPushButton("C", this);
            m_ButtonConfig->setFocusPolicy(Qt::NoFocus);
            m_ButtonConfig->show();
        }
    } else {
        if (m_ButtonConfig != nullptr) {
            delete m_ButtonConfig;
            m_ButtonConfig = nullptr;
        }
    }
}


void ValueInput::ChangeStructurePos(QSize Size)
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
    if (m_WithPlusMinus) {
        if (m_WithConfig) {
            if (m_Header != nullptr) {
                m_Header->resize(Width - Height, HeaderHeight);
                m_Header->move(Height/2, 0);
            }
            m_LineEdit->resize(Width - Height, LineHeight);
            m_LineEdit->move(Height/2, HeaderHeight);
            m_ButtonConfig->resize(Height/2, Height);
            m_ButtonConfig->move(0,0);
            m_ButtonPlus->resize(Height/2, Height/2);
            m_ButtonPlus->move(Width - Height/2, 0);
            m_ButtonMinus->resize(Height/2, Height/2);
            m_ButtonMinus->move(Width - Height/2, Height/2);
        } else {
            if (m_Header != nullptr) {
                m_Header->resize(Width - Height/2, HeaderHeight);
                m_Header->move(0, 0);
            }
            m_LineEdit->resize(Width - Height/2, LineHeight);
            m_LineEdit->move(0, HeaderHeight);
            m_ButtonPlus->resize(Height/2, Height/2);
            m_ButtonPlus->move(Width - Height/2, 0);
            m_ButtonMinus->resize(Height/2, Height/2);
            m_ButtonMinus->move(Width - Height/2, Height/2);
        }
    } else {
        if (m_WithConfig) {
            if (m_Header != nullptr) {
                m_Header->resize(Width - Height/2, HeaderHeight);
                m_Header->move(Height/2, 0);
            }
            m_LineEdit->resize(Width - Height/2, LineHeight);
            m_LineEdit->move(Height/2, HeaderHeight);
            m_ButtonConfig->resize(Height/2, Height);
            m_ButtonConfig->move(0,0);
        } else {
            if (m_Header != nullptr) {
                m_Header->resize(Width, HeaderHeight);
                m_Header->move(0, 0);
            }
            m_LineEdit->resize(Width, Height);
            m_LineEdit->move(0, HeaderHeight);
        }
    }
}

void ValueInput::SetValue (int par_Value, int par_Base)
{
    union BB_VARI Out;
    if (m_MinMaxCheck) {
        union BB_VARI In;
        In.qw = par_Value;
        MinMaxRangeCheck(In, BB_QWORD, &Out);
    } else {
        Out.qw = par_Value;
    }
    char HelpString[128];
    bbvari_to_string (BB_QWORD,
                      Out,
                      par_Base,
                      HelpString, sizeof(HelpString));
    m_LineEdit->setText(QString(HelpString));
    CheckResizeSignalShouldSent();
}

void ValueInput::SetValue (unsigned int par_Value, int par_Base)
{
    union BB_VARI Out;
    if (m_MinMaxCheck) {
        union BB_VARI In;
        In.uqw = par_Value;
        MinMaxRangeCheck(In, BB_UQWORD, &Out);
    } else {
        Out.uqw = par_Value;
    }
    char HelpString[128];
    bbvari_to_string (BB_UQWORD,
                      Out,
                      par_Base,
                      HelpString, sizeof(HelpString));
    m_LineEdit->setText(QString(HelpString));
    CheckResizeSignalShouldSent();
}

void ValueInput::SetValue (int64_t par_Value, int par_Base)
{
    union BB_VARI Out;
    if (m_MinMaxCheck) {
        union BB_VARI In;
        In.qw = par_Value;
        MinMaxRangeCheck(In, BB_QWORD, &Out);
    } else {
        Out.qw = par_Value;
    }
    char HelpString[128];
    bbvari_to_string (BB_QWORD,
                      Out,
                      par_Base,
                      HelpString, sizeof(HelpString));
    m_LineEdit->setText(QString(HelpString));
    CheckResizeSignalShouldSent();
}

void ValueInput::SetValue (uint64_t par_Value, int par_Base)
{
    union BB_VARI Out;
    if (m_MinMaxCheck) {
        union BB_VARI In;
        In.uqw = par_Value;
        MinMaxRangeCheck(In, BB_UQWORD, &Out);
    } else {
        Out.uqw = par_Value;
    }
    char HelpString[128];
    bbvari_to_string (BB_UQWORD,
                      Out,
                      par_Base,
                      HelpString, sizeof(HelpString));
    m_LineEdit->setText(QString(HelpString));
    CheckResizeSignalShouldSent();
}

void ValueInput::SetIntValue (double par_Value)
{
    int Base = 10;
    par_Value = round(par_Value);
    QString Text = m_LineEdit->text();
    if (Text.startsWith(QString("0x")) || Text.startsWith(QString("-0x")) ||
        Text.startsWith(QString("0X")) || Text.startsWith(QString("-0X"))) {
        Base = 16;
    } else if (Text.startsWith(QString("0b")) || Text.startsWith(QString("-0b")) ||
               Text.startsWith(QString("0B")) || Text.startsWith(QString("-0B"))) {
        Base = 2;
    }
    if (par_Value < 0.0) {
        if (par_Value >= INT64_MIN) {
            SetValue (static_cast<int64_t>(par_Value), Base);
        } else {
            SetValue (par_Value);  // value to big for a 64 bit signed integer
        }
    } else {
        if (par_Value <= UINT64_MAX) {
            SetValue (static_cast<uint64_t>(par_Value), Base);
        } else {
            SetValue (par_Value);  // value to big for a 64 bit unsigned integer
        }
    }
    CheckResizeSignalShouldSent();
}

void ValueInput::SetValue (double par_Value)
{
    union BB_VARI Out;
    if (m_MinMaxCheck) {
        union BB_VARI In;
        In.d = par_Value;
        MinMaxRangeCheck(In, BB_DOUBLE, &Out);
    } else {
        Out.d = par_Value;
    }
    if (m_OnlyInteger) {
        SetIntValue (par_Value);
    } else {
        char HelpString[128];
        bbvari_to_string (BB_DOUBLE,
                          Out,
                          10,
                          HelpString, sizeof(HelpString));
        if (QString(HelpString).contains('.') ||
                    QString(HelpString).contains('e') ||
                    (par_Value < (double)INT64_MIN) ||
                    (par_Value > (double)INT64_MAX)) {
            m_LineEdit->setText(QString(HelpString));
        } else {
            bool save_MinMaxCheck = m_MinMaxCheck;
            m_MinMaxCheck = false;
            SetIntValue (par_Value);
            m_MinMaxCheck = save_MinMaxCheck;
        }
    }
    CheckResizeSignalShouldSent();
}

void ValueInput::SetValue(union FloatOrInt64 par_Value, int par_Type)
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


double ValueInput::GetDoubleValue(bool *ret_Ok)
{
    bool Ok = false;
    double DoubleValue = 0.0;
    QString Text = m_LineEdit->text();
    if (Text.length()) {
        if (Text.startsWith(QString("0b"))  || Text.startsWith(QString("0B"))) {
            Text.remove(0, 2);
            DoubleValue = Text.toULongLong(&Ok, 2);
        } else if (Text.startsWith(QString("+0b")) || Text.startsWith(QString("+0B"))) {
            Text.remove(0, 3);
            DoubleValue = Text.toULongLong(&Ok, 2);
        } else if (Text.startsWith(QString("-0b")) || Text.startsWith(QString("-0B"))) {
            Text.remove(0, 3);
            DoubleValue = Text.toULongLong(&Ok, 2);
            if (DoubleValue > 0x8000000000000000ULL) {
                DoubleValue = 0.0;
                Ok = false;
            } else {
                DoubleValue = -DoubleValue;
            }
        } else if (Text.startsWith(QString("0x")) || Text.startsWith(QString("0X"))) {
            Text.remove(0, 2);
            DoubleValue = Text.toULongLong(&Ok, 16);
        } else if (Text.startsWith(QString("+0x")) || Text.startsWith(QString("+0X"))) {
            Text.remove(0, 3);
            DoubleValue = Text.toULongLong(&Ok, 16);
        } else if (Text.startsWith(QString("-0x")) || Text.startsWith(QString("-0X"))) {
            Text.remove(0, 3);
            DoubleValue = Text.toULongLong(&Ok, 16);
            if (DoubleValue > 0x8000000000000000ULL) {
                DoubleValue = 0.0;
                Ok = false;
            } else {
                DoubleValue = -DoubleValue;
            }
        } else {
            DoubleValue = Text.toDouble(&Ok);
            if (!Ok) {
                DoubleValue = Text.toULongLong(&Ok);
                if (!Ok) {
                    DoubleValue = Text.toLongLong(&Ok);
                    if (!Ok) {
                        DoubleValue = 0.0;
                    }
                }
            }
        }
    } else {
        DoubleValue = 0.0;
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    if (m_MinMaxCheck) {
        union BB_VARI In, Out;
        In.d = DoubleValue;
        MinMaxRangeCheck(In, BB_DOUBLE, &Out);
        DoubleValue = Out.d;
    }
    return DoubleValue;
}

int ValueInput::GetIntValue(bool *ret_Ok)
{
    bool Ok = false;
    int IntValue = 0;
    QString Text = m_LineEdit->text();
    if (Text.length()) {
        if (Text.startsWith(QString("0b"))  || Text.startsWith(QString("0B"))) {
            Text.remove(0, 2);
            unsigned int HelpValue = Text.toUInt(&Ok, 2);
            if (HelpValue > 0x7FFFFFFF) {
                IntValue = 0;
                Ok = false;
            } else {
                IntValue = static_cast<int>(HelpValue);
            }
        } else if (Text.startsWith(QString("+0b")) || Text.startsWith(QString("+0B"))) {
            Text.remove(0, 3);
            unsigned int HelpValue = Text.toUInt(&Ok, 2);
            if (HelpValue > 0x7FFFFFFF) {
                IntValue = 0;
                Ok = false;
            } else {
                IntValue = static_cast<int>(HelpValue);
            }
        } else if (Text.startsWith(QString("-0b")) || Text.startsWith(QString("-0B"))) {
            Text.remove(0, 3);
            unsigned int HelpValue = Text.toUInt(&Ok, 2);
            if (HelpValue > 0x80000000) {
                IntValue = 0;
                Ok = false;
            } else if (HelpValue == 0x80000000) {
                IntValue = INT_MIN;
            } else {
                IntValue = -static_cast<int>(HelpValue);
            }
        } else if (Text.startsWith(QString("0x")) || Text.startsWith(QString("0X"))) {
            Text.remove(0, 2);
            unsigned int HelpValue = Text.toUInt(&Ok, 16);
            if (HelpValue > 0x7FFFFFFF) {
                IntValue = 0;
                Ok = false;
            } else {
                IntValue = static_cast<int>(HelpValue);
            }
        } else if (Text.startsWith(QString("+0x")) || Text.startsWith(QString("+0X"))) {
            Text.remove(0, 3);
            unsigned int HelpValue = Text.toUInt(&Ok, 16);
            if (HelpValue > 0x7FFFFFFF) {
                IntValue = 0;
                Ok = false;
            } else {
                IntValue = static_cast<int>(HelpValue);
            }
        } else if (Text.startsWith(QString("-0x")) || Text.startsWith(QString("-0X"))) {
            Text.remove(0, 3);
            unsigned int HelpValue = Text.toUInt(&Ok, 16);
            if (HelpValue > 0x80000000) {
                IntValue = 0;
                Ok = false;
            } else if (HelpValue == 0x80000000) {
                IntValue = INT_MIN;
            } else {
                IntValue = -static_cast<int>(HelpValue);
            }
        } else {
            IntValue = static_cast<int>(Text.toDouble(&Ok));
            if (!Ok) {
                IntValue = static_cast<int>(Text.toUInt(&Ok));
                if (!Ok) {
                    IntValue = Text.toInt(&Ok);
                    if (!Ok) {
                        IntValue = 0;
                    }
                }
            }
        }
    } else {
        IntValue = 0;
    }
    if (m_MinMaxCheck) {
        union BB_VARI In, Out;
        In.qw = IntValue;
        MinMaxRangeCheck(In, BB_QWORD, &Out);
        IntValue = static_cast<int>(Out.qw);
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return IntValue;
}

unsigned int ValueInput::GetUIntValue(bool *ret_Ok)
{
    bool Ok = false;
    unsigned int IntValue = 0;
    QString Text = m_LineEdit->text();
    if (Text.length()) {
        if (Text.startsWith(QString("0b"))  || Text.startsWith(QString("0B"))) {
            Text.remove(0, 2);
            IntValue = Text.toUInt(&Ok, 2);
        } else if (Text.startsWith(QString("+0b")) || Text.startsWith(QString("+0B"))) {
            Text.remove(0, 3);
            IntValue = Text.toUInt(&Ok, 2);
        } else if (Text.startsWith(QString("0x")) || Text.startsWith(QString("0X"))) {
            Text.remove(0, 2);
            IntValue = Text.toUInt(&Ok, 16);
        } else if (Text.startsWith(QString("+0x")) || Text.startsWith(QString("+0X"))) {
            Text.remove(0, 3);
            IntValue = Text.toUInt(&Ok, 16);
        } else {
            IntValue = static_cast<unsigned int>(Text.toDouble(&Ok));
            if (!Ok) {
                IntValue = Text.toUInt(&Ok);
                if (!Ok) {
                    IntValue = static_cast<unsigned int>(Text.toInt(&Ok));
                    if (!Ok) {
                        IntValue = 0;
                    }
                }
            }
        }
    } else {
        IntValue = 0;
    }
    if (m_MinMaxCheck) {
        union BB_VARI In, Out;
        In.uqw = IntValue;
        MinMaxRangeCheck(In, BB_UQWORD, &Out);
        IntValue = static_cast<unsigned int>(Out.uqw);
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return IntValue;
}

unsigned long long ValueInput::GetUInt64Value(bool *ret_Ok)
{
    bool Ok = false;
    unsigned long long IntValue = 0;
    QString Text = m_LineEdit->text();
    if (Text.length()) {
        if (Text.startsWith(QString("0b"))  || Text.startsWith(QString("0B"))) {
            Text.remove(0, 2);
            IntValue = Text.toULongLong(&Ok, 2);
        } else if (Text.startsWith(QString("+0b")) || Text.startsWith(QString("+0B"))) {
            Text.remove(0, 3);
            IntValue = Text.toULongLong(&Ok, 2);
        } else if (Text.startsWith(QString("0x")) || Text.startsWith(QString("0X"))) {
            Text.remove(0, 2);
            IntValue = Text.toULongLong(&Ok, 16);
        } else if (Text.startsWith(QString("+0x")) || Text.startsWith(QString("+0X"))) {
            Text.remove(0, 3);
            IntValue = Text.toULongLong(&Ok, 16);
        } else {
            IntValue = static_cast<unsigned long long>(Text.toDouble(&Ok));
            if (!Ok) {
                IntValue = Text.toUInt(&Ok);
                if (!Ok) {
                    IntValue = static_cast<unsigned long long>(Text.toInt(&Ok));
                    if (!Ok) {
                        IntValue = 0;
                    }
                }
            }
        }
    } else {
        IntValue = 0;
    }
    if (m_MinMaxCheck) {
        union BB_VARI In, Out;
        In.uqw = IntValue;
        MinMaxRangeCheck(In, BB_UQWORD, &Out);
        IntValue = Out.uqw;
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return IntValue;
}


BB_DATA_TYPES ValueInput::GetUnionValue(BB_VARI *ret_Value, bool *ret_Ok)
{
    enum BB_DATA_TYPES Ret;
    bool Ok = false;
    QString Text = m_LineEdit->text();
    if (Text.length()) {
        unsigned long long UIntValue;
        long long IntValue;
        if (Text.startsWith(QString("0b"))  || Text.startsWith(QString("0B"))) {
            Text.remove(0, 2);
            ret_Value->uqw = Text.toULongLong(&Ok, 2);
            Ret = BB_UQWORD;
        } else if (Text.startsWith(QString("+0b")) || Text.startsWith(QString("+0B"))) {
            Text.remove(0, 3);
            ret_Value->uqw = Text.toULongLong(&Ok, 2);
            Ret = BB_UQWORD;
        } else if (Text.startsWith(QString("0x")) || Text.startsWith(QString("0X"))) {
            Text.remove(0, 2);
            ret_Value->uqw = Text.toULongLong(&Ok, 16);
            Ret = BB_UQWORD;
        } else if (Text.startsWith(QString("+0x")) || Text.startsWith(QString("+0X"))) {
            Text.remove(0, 3);
            ret_Value->uqw = Text.toULongLong(&Ok, 16);
            Ret = BB_UQWORD;
        } else if (Text.startsWith(QString("-0b")) || Text.startsWith(QString("-0B"))) {
            Text.remove(0, 3);
            UIntValue = Text.toULongLong(&Ok, 2);
            ret_Value->qw = -static_cast<int64_t>(UIntValue);
            Ret = BB_QWORD;
        } else if (Text.startsWith(QString("-0x")) || Text.startsWith(QString("-0X"))) {
            Text.remove(0, 3);
            UIntValue = Text.toULongLong(&Ok, 16);
            ret_Value->qw = -static_cast<int64_t>(UIntValue);
            Ret = BB_QWORD;
        } else {
            UIntValue = Text.toULongLong(&Ok);
            if (Ok) {
                ret_Value->uqw = UIntValue;
                Ret = BB_UQWORD;
            } else {
                IntValue = Text.toLongLong(&Ok);
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
    } else {
         ret_Value->d = 0.0;
         Ret = BB_DOUBLE;
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return Ret;
}

int ValueInput::GetFloatOrInt64Value(union FloatOrInt64 *ret_Value, bool *ret_Ok)
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

long long ValueInput::GetInt64Value(bool *ret_Ok)
{
    bool Ok = false;
    long long IntValue = 0;
    QString Text = m_LineEdit->text();
    if (Text.length()) {
        if (Text.startsWith(QString("0b"))  || Text.startsWith(QString("0B"))) {
            Text.remove(0, 2);
            unsigned long long HelpValue = Text.toULongLong(&Ok, 2);
            if (HelpValue > 0x7FFFFFFFFFFFFFFF) {
                IntValue = 0;
                Ok = false;
            } else {
                IntValue = static_cast<long long>(HelpValue);
            }
        } else if (Text.startsWith(QString("+0b")) || Text.startsWith(QString("+0B"))) {
            Text.remove(0, 3);
            unsigned long long HelpValue = Text.toULongLong(&Ok, 2);
            if (HelpValue > 0x7FFFFFFFFFFFFFFF) {
                IntValue = 0;
                Ok = false;
            } else {
                IntValue = static_cast<long long>(HelpValue);
            }
        } else if (Text.startsWith(QString("-0b")) || Text.startsWith(QString("-0B"))) {
            Text.remove(0, 3);
            unsigned long long HelpValue = Text.toULongLong(&Ok, 2);
            if (HelpValue > 0x8000000000000000) {
                IntValue = 0;
                Ok = false;
            } else if (HelpValue == 0x8000000000000000) {
                IntValue = LLONG_MIN;
            } else {
                IntValue = -static_cast<long long>(HelpValue);
            }
        } else if (Text.startsWith(QString("0x")) || Text.startsWith(QString("0X"))) {
            Text.remove(0, 2);
            unsigned long long HelpValue = Text.toULongLong(&Ok, 16);
            if (HelpValue > 0x7FFFFFFFFFFFFFFF) {
                IntValue = 0;
                Ok = false;
            } else {
                IntValue = static_cast<long long>(HelpValue);
            }
        } else if (Text.startsWith(QString("+0x")) || Text.startsWith(QString("+0X"))) {
            Text.remove(0, 3);
            unsigned long long HelpValue = Text.toULongLong(&Ok, 16);
            if (HelpValue > 0x7FFFFFFFFFFFFFFF) {
                IntValue = 0;
                Ok = false;
            } else {
                IntValue = static_cast<long long>(HelpValue);
            }
        } else if (Text.startsWith(QString("-0x")) || Text.startsWith(QString("-0X"))) {
            Text.remove(0, 3);
            unsigned long long HelpValue = Text.toULongLong(&Ok, 16);
            if (HelpValue > 0x8000000000000000) {
                IntValue = 0;
                Ok = false;
            } else if (HelpValue == 0x8000000000000000) {
                IntValue = LLONG_MIN;
            } else {
                IntValue = -static_cast<long long>(HelpValue);
            }
        } else {
            IntValue = static_cast<long long>(Text.toDouble(&Ok));
            if (!Ok) {
                IntValue = Text.toUInt(&Ok);
                if (!Ok) {
                    IntValue = Text.toInt(&Ok);
                    if (!Ok) {
                        IntValue = 0;
                    }
                }
            }
        }
    } else {
        IntValue = 0;
    }
    if (m_MinMaxCheck) {
        union BB_VARI In, Out;
        In.qw = IntValue;
        MinMaxRangeCheck(In, BB_QWORD, &Out);
        IntValue = static_cast<long long>(Out.uqw);
    }
    if (ret_Ok != nullptr) *ret_Ok = Ok;
    return IntValue;
}



void ValueInput::CheckValueRangesSlot (const QString &par_Text)
{
    Q_UNUSED(par_Text)
    if (m_MinMaxCheck) {
        bool Ok;
        bool OutOfRange;
        m_MinMaxCheck = false;   // shortly switch off
        union BB_VARI Value;
        enum BB_DATA_TYPES par_Type = GetUnionValue(&Value, &Ok);
        m_MinMaxCheck = true;
        if (Ok) {
            OutOfRange = MinMaxRangeCheck(Value, par_Type, nullptr);
        } else {
            OutOfRange = true;
        }
        if (OutOfRange && !m_OutOfRange) {
            m_LineEdit->setPalette (m_PaletteInvalidValueRange);
            m_OutOfRange = true;
        } else if (!OutOfRange && m_OutOfRange) {
            m_LineEdit->setPalette (m_PaletteSave);
            m_OutOfRange = false;
        }
    }
    CheckResizeSignalShouldSent();
    emit ValueChanged(GetDoubleValue(), m_OutOfRange);
}

void ValueInput::IncrementValue()
{
    AddOffset(m_PlusMinusIncrement);
}

void ValueInput::DecrementValue()
{
    AddOffset(-m_PlusMinusIncrement);
}

void ValueInput::editingFinishedSlot()
{
    emit editingFinished();
}

bool ValueInput::MinMaxRangeCheck(union BB_VARI par_In, enum BB_DATA_TYPES par_Type, union BB_VARI *ret_Out)
{
    bool OutOfRange = false;
    switch (par_Type) {
    default:
    case BB_DOUBLE:
        switch (m_Type) {
        default:
        case BB_DOUBLE:
            if (par_In.d < m_MinValue.d) {
                par_In.d = m_MinValue.d;
                OutOfRange = true;
            } else if (par_In.d > m_MaxValue.d) {
                par_In.d = m_MaxValue.d;
                OutOfRange = true;
            }
            break;
        case BB_QWORD:
            if (round(par_In.d) < static_cast<double>(m_MinValue.qw)) {
                par_In.d = static_cast<double>(m_MinValue.qw);
                OutOfRange = true;
            } else if (round(par_In.d) > static_cast<double>(m_MaxValue.qw)) {
                par_In.d = static_cast<double>(m_MaxValue.qw);
                OutOfRange = true;
            }
            break;
        case BB_UQWORD:
            if (round(par_In.d) < static_cast<double>(m_MinValue.uqw)) {
                par_In.d = static_cast<double>(m_MinValue.uqw);
                OutOfRange = true;
            } else if (round(par_In.d) > static_cast<double>(m_MaxValue.uqw)) {
                par_In.d = static_cast<double>(m_MaxValue.uqw);
                OutOfRange = true;
            }
            break;
        }
        break;
    case BB_QWORD:
        switch (m_Type) {
        default:
        case BB_DOUBLE:
            if ((m_MinValue.d >= INT64_MIN) &&
                (par_In.qw < static_cast<int64_t>(round(m_MinValue.d)))) {
                par_In.qw = static_cast<int64_t>(round(m_MinValue.d));
                OutOfRange = true;
            } else if ((m_MaxValue.d <= INT64_MAX) &&
                       (par_In.qw > static_cast<int64_t>(round(m_MaxValue.d)))) {
                par_In.qw = static_cast<int64_t>(round(m_MaxValue.d));
                OutOfRange = true;
            }
            break;
        case BB_QWORD:
            if (par_In.qw < m_MinValue.qw) {
                par_In.qw = m_MinValue.qw;
                OutOfRange = true;
            } else if (par_In.qw > m_MaxValue.qw) {
                par_In.qw = m_MaxValue.qw;
                OutOfRange = true;
            }
            break;
        case BB_UQWORD:
            if (m_MinValue.uqw > INT64_MAX) {
                par_In.qw = INT64_MAX;
                OutOfRange = true;
            } else if (par_In.qw < static_cast<int64_t>(m_MinValue.uqw)) {
                par_In.qw = static_cast<int64_t>(m_MinValue.uqw);
                OutOfRange = true;
            } else if ((m_MaxValue.uqw <= INT64_MAX) && (par_In.qw > static_cast<int64_t>(m_MaxValue.uqw))) {
                par_In.qw = static_cast<int64_t>(m_MaxValue.uqw);
                OutOfRange = true;
            }
            break;
        }
        break;
    case BB_UQWORD:
        switch (m_Type) {
        default:
        case BB_DOUBLE:
            if ((m_MinValue.d >= 0.0) &&
                (par_In.uqw < static_cast<uint64_t>(round(m_MinValue.d)))) {
                par_In.uqw = static_cast<uint64_t>(round(m_MinValue.d));
                OutOfRange = true;
            } else if ((m_MaxValue.d <= UINT64_MAX) &&
                       (par_In.uqw > static_cast<uint64_t>(round(m_MaxValue.d)))) {
                par_In.uqw = static_cast<uint64_t>(round(m_MaxValue.d));
                OutOfRange = true;
            }
            break;
        case BB_QWORD:
            if (m_MaxValue.qw < 0) {
                par_In.uqw = 0;
                OutOfRange = true;
            } else {
                if (m_MinValue.qw < 0) {
                    if (par_In.uqw > static_cast<uint64_t>(m_MaxValue.qw)) {
                        par_In.uqw = static_cast<uint64_t>(m_MaxValue.qw);
                        OutOfRange = true;
                    }
                } else {
                    if (par_In.uqw < static_cast<uint64_t>(m_MinValue.qw)) {
                        par_In.uqw = static_cast<uint64_t>(m_MinValue.qw);
                        OutOfRange = true;
                    } else if (par_In.uqw > static_cast<uint64_t>(m_MaxValue.qw)) {
                        par_In.uqw = static_cast<uint64_t>(m_MaxValue.qw);
                        OutOfRange = true;
                    }
                }
            }
            break;
        case BB_UQWORD:
            if (par_In.uqw < m_MinValue.uqw) {
                par_In.uqw = m_MinValue.uqw;
                OutOfRange = true;
            } else if (par_In.uqw > m_MaxValue.uqw) {
                par_In.uqw = m_MaxValue.uqw;
                OutOfRange = true;
            }
            break;
        }
        break;
    }
    if (ret_Out != nullptr) *ret_Out = par_In;
    return OutOfRange;
}

int ValueInput::BaseOfText()
{
    int Base = 10;
    QString Text = m_LineEdit->text();
    if (Text.startsWith(QString("0x")) || Text.startsWith(QString("-0x")) ||
        Text.startsWith(QString("0X")) || Text.startsWith(QString("-0X"))) {
        Base = 16;
    } else if (Text.startsWith(QString("0b")) || Text.startsWith(QString("-0b")) ||
               Text.startsWith(QString("0B")) || Text.startsWith(QString("-0B"))) {
        Base = 2;
    }
    return Base;
}

void ValueInput::AddOffset(double par_Offset)
{
    switch(m_StepType) {
    case PERCENTAGE_STEP:
        {
            double Value = GetDoubleValue();
            Value += Value * par_Offset;
            SetValue(Value);
            break;
        }
    case LINEAR_STEP:
    //default:
        if (abs(round(par_Offset) - par_Offset) > 0.000001) {
            double Value = GetDoubleValue();
            Value += Value * par_Offset;
            SetValue(Value);
        } else {
            int Base = BaseOfText();
            bool Ok;
            union BB_VARI Value;
            enum BB_DATA_TYPES Type = GetUnionValue(&Value, &Ok);
            switch (Type) {
            case BB_QWORD:
                if ((par_Offset > 0) && ((Value.qw + static_cast<int64_t>(round(par_Offset))) < Value.qw)) {
                    Value.qw = INT64_MAX;  // Intercept overflow
                } else if ((par_Offset < 0) && ((Value.qw + static_cast<int64_t>(round(par_Offset))) > Value.qw)) {
                    Value.qw = INT64_MIN;  // Intercept underflow
                } else  {
                    Value.qw += static_cast<int64_t>(round(par_Offset));
                }
                SetValue(Value.qw, Base);
                break;
            case BB_UQWORD:
                if (par_Offset > 0) {
                    if ((Value.uqw + static_cast<uint64_t>(round(par_Offset))) < Value.uqw) {
                        Value.uqw = UINT64_MAX;
                    } else {
                        Value.uqw += static_cast<uint64_t>(round(par_Offset));
                    }
                } else {
                    if ((Value.uqw - static_cast<uint64_t>(round(-par_Offset))) > Value.uqw) {
                        if (Value.uqw > INT64_MAX) {
                            Value.uqw = 0;
                        } else {
                            // The value became now negative
                            Value.qw -= static_cast<uint64_t>(round(-par_Offset));
                            SetValue(Value.qw);
                            break;
                        }
                    } else {
                        Value.uqw -= static_cast<uint64_t>(round(-par_Offset));
                    }
                }
                SetValue(Value.uqw, Base);
                break;
            case BB_DOUBLE:
                Value.d += par_Offset;
                SetValue(Value.d);
                break;
            default:
                //ThrowError (1, "this should never happen %s(%i)", __FILE__, __LINE__);
                break;
            }
        }
        break;
    }

}

void ValueInput::SetMinMaxValue (double par_MinValue, double par_MaxValue)
{
    m_MinMaxCheck = true;
    m_MinValue.d = par_MinValue;
    m_MaxValue.d = par_MaxValue;
    m_Type = BB_DOUBLE;
}

void ValueInput::SetMinMaxValue(BB_VARI par_MinValue, BB_VARI par_MaxValue, BB_DATA_TYPES par_Type)
{
    m_MinMaxCheck = true;
    ConvertTo64BitDataType(par_MinValue, par_Type, &m_MinValue, &m_Type);
    ConvertTo64BitDataType(par_MaxValue, par_Type, &m_MaxValue, &m_Type);
}

void ValueInput::EnableAutoScaling(void)
{
    m_AutoScalingActive = true;
}

void ValueInput::DisableAutoScaling(void)
{
    m_AutoScalingActive = false;
}

bool ValueInput::GetConfigButtonEnable()
{
    return m_WithConfig;
}

void ValueInput::SetConfigButtonEnable(bool par_State)
{
    m_WithConfig = par_State;
    ChangeStructureElem();
    ChangeStructurePos(size());
}

bool ValueInput::GetPlusMinusButtonEnable()
{
    return m_WithPlusMinus;
}

void ValueInput::SetPlusMinusButtonEnable(bool par_State)
{
    m_WithPlusMinus = par_State;
    ChangeStructureElem();
    ChangeStructurePos(size());
}

double ValueInput::GetPlusMinusIncrement()
{
    return m_PlusMinusIncrement;
}

void ValueInput::SetPlusMinusIncrement(double par_Increment)
{
    m_PlusMinusIncrement = par_Increment;
}

void ValueInput::SetStepLinear(bool par_Linear)
{
    if (par_Linear) {
        m_StepType = LINEAR_STEP;
    } else {
        m_StepType = PERCENTAGE_STEP;
    }
}

void ValueInput::SetStepPercentage(bool par_Percentage)
{
    if (par_Percentage) {
        m_StepType = PERCENTAGE_STEP;
    } else {
        m_StepType = LINEAR_STEP;
    }
}

void ValueInput::SetStepType(ValueInput::StepType par_StepType)
{
    m_StepType = par_StepType;
}

void ValueInput::SetOnlyInteger(bool par_OnlyInteger)
{
    m_OnlyInteger = par_OnlyInteger;
    if (m_OnlyInteger) {
        m_Validator.setRegularExpression(QRegularExpression("[-+]?0[xX][0-9a-fA-F]+|"   // Hex
                                      "[-+]?0b[0-1]+|"            // Binary
                                      "[-+]?(0|[1-9][0-9]*)$"));  // Int
    } else {
        m_Validator.setRegularExpression(QRegularExpression("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$|"   // Double
                                      "[-+]?0[xX][0-9a-fA-F]+|"   // Hex
                                      "[-+]?0b[0-1]+|"            // Binary
                                      "[-+]?(0|[1-9][0-9]*)$"));  // Int
    }
    m_LineEdit->setValidator(&m_Validator);
}

void ValueInput::SetHeader(QString par_Header)
{
    if (m_Header != nullptr) m_Header->setText(par_Header);
    else {
        m_Header = new QLabel(par_Header, this);
        m_Header->show();
    }
}

QSize ValueInput::sizeHint () const
{
    QSize Size = QWidget::sizeHint();
    return Size;
}

void ValueInput::setFocus(Qt::FocusReason reason)
{
    if (m_LineEdit != nullptr) m_LineEdit->setFocus(reason);
}

bool ValueInput::hasFocus() const
{
    bool Ret = false;
    if (m_LineEdit != nullptr) Ret = Ret || m_LineEdit->hasFocus();
    return Ret;
}

QString ValueInput::text()
{
    return m_LineEdit->text();
}

void ValueInput::setText(QString &par_String)
{
    m_LineEdit->setText(par_String);
}

void ValueInput::SetWideningSignalEnable(bool par_Enable)
{
    m_WideningSignal = par_Enable;
}

void ValueInput::SetValueInvalid(bool par_Invalid)
{
    m_OutOfRange = par_Invalid;
    if (m_OutOfRange) {
        m_LineEdit->setPalette (m_PaletteInvalidValueRange);
    } else {
        m_LineEdit->setPalette (m_PaletteSave);
    }
    update();
}

void ValueInput::CheckResizeSignalShouldSent()
{
    if (m_WideningSignal) {
        int NeededWith = m_LineEdit->fontMetrics().boundingRect(m_LineEdit->text()).width() + m_LineEdit->height()/2;
        int Widening = 0;
        int Heightening = 0;
        if (NeededWith > width()) {
            Widening = NeededWith - m_LineEdit->width();
        }
        int NeededHight = m_LineEdit->fontMetrics().height();
        if (NeededHight < 12) NeededHight = 12;
        if (NeededHight > height()) {
            Heightening = NeededHight - height();
        }
        if ((Widening > 0) || (Heightening > 0)) {
            emit ShouldWideningSignal(Widening, Heightening);
        }
    }
}


MyLineEdit::MyLineEdit(QWidget *parent)  : QLineEdit(parent)
{
    setFocusPolicy(Qt::WheelFocus);
    installEventFilter(this);
}

MyLineEdit::MyLineEdit(const QString &name, QWidget *parent) : QLineEdit(name, parent)
{
    setFocusPolicy(Qt::WheelFocus);
    installEventFilter(this);
}


bool MyLineEdit::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            QObject *Parent = parent();
            #ifdef DEBUG_EVENT
            qDebug() << "MyLineEdit::eventFilter Qt::Key_Tab or Qt::Key_Backtab" << "postEvent to" << Parent->objectName();
            #endif
            QApplication::postEvent(Parent, new QKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers()));
            // Filter this event because the editor will be closed anyway
            return true;
        }
    } else if(event->type() == QEvent::FocusOut) {
        QFocusEvent* focusEvent = static_cast<QFocusEvent *>(event);
        QObject *Parent = parent();
        #ifdef DEBUG_EVENT
        qDebug() << "MyLineEdit::eventFilter QEvent::FocusOut" << "postEvent to" << Parent->objectName();
        #endif
        QApplication::postEvent(Parent, new QFocusEvent(focusEvent->type(), focusEvent->reason()));

        // Don't filter because focus can be changed internally in editor
        return false;
    }
    return QLineEdit::eventFilter(obj, event);
}

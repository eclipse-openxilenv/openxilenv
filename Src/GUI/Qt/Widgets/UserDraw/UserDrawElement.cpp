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


#include "UserDrawElement.h"
#include "UserDrawParser.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

extern "C" {
#include "Blackboard.h"
#include "ExecutionStack.h"
#include "EquationParser.h"
#include "ThrowError.h"
#include "MyMemory.h"
}

UserDrawElement::UserDrawElement(UserDrawElement *par_Parent)
{
    m_Parent = par_Parent;
    ResetToDefault();
}

UserDrawElement::~UserDrawElement()
{

}

bool UserDrawElement::Init()
{
    return true;
}

void UserDrawElement::ResetToDefault()
{
    m_Properties.clear();
    m_Visible.SetFixed(1.0);
    m_Properties.Add(QString("visible"), QString());
    m_LineColor.SetFixed(0,0,0);
    m_Properties.Add(QString("color"), QString());
    m_LineThickness.SetFixed(1);
    m_Properties.Add(QString("thickness"), QString());
    m_FillColorIsSet = false;
    m_FillColor.SetFixed(0,0,0);
    m_Properties.Add(QString("fill"), QString());
    m_X.SetFixed(0.0);
    m_Properties.Add(QString("x"), QString());
    m_Y.SetFixed(0.0);
    m_Properties.Add(QString("y"), QString());
    m_Scale.SetFixed(1.0);
    m_Properties.Add(QString("scale"), QString());
    m_Rot.SetFixed(0.0);
    m_Properties.Add(QString("rot"), QString());
}

void UserDrawElement::SetDefaultParameterString()
{
    m_ParameterLine = QString("()");
}

/*int UserDrawElement::Parse(QString &Line)
{
    Q_UNUSED(Line);
    return 0;
}*/

bool UserDrawElement::AddChild(UserDrawElement *par_Child, int par_Pos)
{
    Q_UNUSED(par_Child);
    Q_UNUSED(par_Pos);
    return false;
}

void UserDrawElement::DeleteChild(int par_Pos)
{
    Q_UNUSED(par_Pos);
}

int UserDrawElement::ChildNumber()
{
    if (m_Parent)
        return m_Parent->ChildNumber(this); // .indexOf(const_cast<CalibrationTreeItem*>(this));

    return 0;
}

int UserDrawElement::ChildNumber(UserDrawElement *par_Child)
{
    Q_UNUSED(par_Child);
    return 0;
}

bool UserDrawElement::IsGroup()
{
    return false;
}

bool UserDrawElement::MousePressEvent(QMouseEvent *event, DrawParameter &par_Draw)
{
    Q_UNUSED(event);
    Q_UNUSED(par_Draw);
    return false;
}

bool UserDrawElement::ParseOneBaseParameter(QString &Key, QString &ValueString)
{
    if (!Key.compare("x", Qt::CaseInsensitive)) {
        m_X.Parse(ValueString);
        m_Properties.Add("x", ValueString);
    } else if (!Key.compare("y", Qt::CaseInsensitive)) {
        m_Y.Parse(ValueString);
        m_Properties.Add("y", ValueString);
    } else if (!Key.compare("scale", Qt::CaseInsensitive) ||
               !Key.compare("s", Qt::CaseInsensitive)) {
        m_Scale.Parse(ValueString);
        m_Properties.Add("scale", ValueString);
    } else if (!Key.compare("rot", Qt::CaseInsensitive) ||
               !Key.compare("r", Qt::CaseInsensitive)) {
        m_Rot.Parse(ValueString);
        m_Properties.Add("rot", ValueString);
    } else if (!Key.compare("visible", Qt::CaseInsensitive) ||
               !Key.compare("v", Qt::CaseInsensitive)) {
        m_Visible.Parse(ValueString);
        m_Properties.Add("visible", ValueString);
    } else if (!Key.compare("thickness", Qt::CaseInsensitive) ||
               !Key.compare("t", Qt::CaseInsensitive)) {
        m_LineThickness.Parse(ValueString);
        m_Properties.Add("thickness", ValueString);
    } else if (!Key.compare("color", Qt::CaseInsensitive) ||
               !Key.compare("c", Qt::CaseInsensitive)) {
        m_LineColor.Parse(ValueString);
        m_Properties.Add("color", ValueString);
    } else if (!Key.compare("fill", Qt::CaseInsensitive) ||
               !Key.compare("f", Qt::CaseInsensitive)) {
        m_FillColor.Parse(ValueString);
        m_Properties.Add("fill", ValueString);
        m_FillColorIsSet = true;
    } else {
        return false;
    }
    return true;
}

void UserDrawElement::SetColor(QPainter *par_Painter)
{
    double Red = m_LineColor.m_Red.Get();
    double Green = m_LineColor.m_Green.Get();
    double Blue = m_LineColor.m_Blue.Get();
    int IRed = (int)((Red < 255.0) ? Red : 255.0);
    int IGreen = (int)((Green < 255.0) ? Green : 255.0);
    int IBlue = (int)((Blue < 255.0) ? Blue : 255.0);
    QColor Color(IRed, IGreen, IBlue);
    QPen Pen(Color);
    Pen.setWidth(m_LineThickness.Get());
    par_Painter->setPen(Pen);
}

void UserDrawElement::SetFillColor(QPainter *par_Painter)
{
    if (m_FillColorIsSet) {
        double Red = m_FillColor.m_Red.Get();
        double Green = m_FillColor.m_Green.Get();
        double Blue = m_FillColor.m_Blue.Get();
        int IRed = (int)((Red < 255.0) ? Red : 255.0);
        int IGreen = (int)((Green < 255.0) ? Green : 255.0);
        int IBlue = (int)((Blue < 255.0) ? Blue : 255.0);
        QColor Color(IRed, IGreen, IBlue);
        QBrush Brush = QBrush (Color, Qt::SolidPattern);
        par_Painter->setBrush (Brush);
    }
}

void UserDrawElement::Translate(double *px, double *py, TranslateParameter &Parameter, enum TranslateType par_Type) //, bool par_Scale)
{
/*    double x = *px;
    double y = *py;
    double xr = Parameter.m_Cos * x + Parameter.m_Sin * y;
    double xs = (xr * Parameter.m_Scale + Parameter.m_PosX);
    double yr = Parameter.m_Sin * -x + Parameter.m_Cos * y;
    double ys = (yr * Parameter.m_Scale + Parameter.m_PosY);
    // same with parent
    x = xs + Parameter.m_Draw.m_ParentX;
    y = ys + Parameter.m_Draw.m_ParentY;
    xr = Parameter.m_Draw.m_ParentRotCos * x + Parameter.m_Draw.m_ParentRotSin * y;
    xs = (xr * Parameter.m_Draw.m_ParentScale + Parameter.m_Draw.m_ParentX);
    yr = Parameter.m_Draw.m_ParentRotSin * -x + Parameter.m_Draw.m_ParentRotCos * y;
    ys = (yr * Parameter.m_Draw.m_ParentScale + Parameter.m_Draw.m_ParentY);
    // scale to window size
    if ((par_Type & 0x6) ==  0x6) {
        *px = xs * Parameter.m_Draw.m_XScale;
        *py = Parameter.m_Draw.m_YScale - ys * Parameter.m_Draw.m_YScale;
    } else {
        *px = xs;
        *py = ys;
    }*/
    double x = *px;
    double y = *py;
    double xr;
    double xs;
    double yr;
    double ys;

    if ((par_Type & TranslateOnlyItself) ==  TranslateOnlyItself) {   // translate itself
        xr = Parameter.m_Cos * x + Parameter.m_Sin * y;
        xs = (xr * Parameter.m_Scale + Parameter.m_PosX);
        yr = Parameter.m_Sin * -x + Parameter.m_Cos * y;
        ys = (yr * Parameter.m_Scale + Parameter.m_PosY);
        x = xs;
        y = ys;
    }
    if ((par_Type & TranslateOnlyParent) ==  TranslateOnlyParent) {   // translate parent
        //x = x + Parameter.m_Draw.m_ParentX;
        //y = y + Parameter.m_Draw.m_ParentY;
        xr = Parameter.m_Draw.m_ParentRotCos * x + Parameter.m_Draw.m_ParentRotSin * y;
        xs = (xr * Parameter.m_Draw.m_ParentScale + Parameter.m_Draw.m_ParentX);
        yr = Parameter.m_Draw.m_ParentRotSin * -x + Parameter.m_Draw.m_ParentRotCos * y;
        ys = (yr * Parameter.m_Draw.m_ParentScale + Parameter.m_Draw.m_ParentY);
        x = xs;
        y = ys;
    }
    // scale to window size
    if ((par_Type & TranslateOnyToScreen) ==  TranslateOnyToScreen) {
        x = x * Parameter.m_Draw.m_XScale;
        y = Parameter.m_Draw.m_YScale - y * Parameter.m_Draw.m_YScale;
    }
    *px = x;
    *py = y;
}

int UserDrawElement::GetChildCount()
{
    return 0;
}

UserDrawElement *UserDrawElement::GetChild(int par_Index)
{
    Q_UNUSED(par_Index);
    return nullptr;
}

void UserDrawElement::WriteToINI(QString &par_WindowName, QString &par_Entry, int par_Fd)
{
    QString Line = GetTypeString().append(m_ParameterLine);
    ScQt_IniFileDataBaseWriteString(par_WindowName, par_Entry, Line, par_Fd);
    for (int x = 0; x < GetChildCount(); x++) {
        UserDrawElement *Child = GetChild(x);
        QString ChildEntry = par_Entry;
        char Help[32];
        sprintf(Help, "_%i", x);
        ChildEntry.append(QString(Help));
        Child->WriteToINI(par_WindowName, ChildEntry, par_Fd);
    }
}

bool UserDrawElement::ParseParameterString()
{
    UserDrawParser Parser;
    return Parser.ParseParameters(m_ParameterLine, 0, this);
}

bool UserDrawElement::InitParameter(int par_Pos, QString &par_ParameterString)
{
    ResetToDefault();
    if (par_Pos < 0) SetDefaultParameterString();
    else m_ParameterLine = par_ParameterString.mid(par_Pos);
    return ParseParameterString();
}

BaseValue::BaseValue(QString &par_Value, bool par_OneTimeCalc)
{
    Parse(par_Value, par_OneTimeCalc);
}

BaseValue::BaseValue()
{
    m_IsFixedValue = true;
    m_Value.m_Fixed = 0.0;
}

BaseValue::BaseValue(const BaseValue &par)
{
    m_IsFixedValue = par.m_IsFixedValue;
    m_AtachCounterIdx = par.m_AtachCounterIdx;
    if (m_IsFixedValue) {
        m_Value.m_Fixed = par.m_Value.m_Fixed;
    } else {
        m_Value.m_ExecStack = par.m_Value.m_ExecStack;
        m_AttachCounters[m_AtachCounterIdx] += 1;
    }
}

BaseValue::~BaseValue()
{
    if (!m_IsFixedValue) {
        m_AttachCounters[m_AtachCounterIdx] -= 1;
        if (m_AttachCounters[m_AtachCounterIdx] == 0) {
            if (m_Value.m_ExecStack != nullptr) {
                remove_exec_stack(m_Value.m_ExecStack);
                m_Value.m_ExecStack = nullptr;
            }
        }
    }
}

int BaseValue::Parse(QString &par_Value, bool par_OneTimeCalc)
{
    char *ErrString;
    //m_ValueString = par_Value;
    if (par_OneTimeCalc) {
        m_IsFixedValue = 1;
        if (direct_solve_equation_err_string(QStringToConstChar(par_Value), &m_Value.m_Fixed, &ErrString)) {
            ThrowError (1, "cannot calculate formula \"%s\"\n(%s)", QStringToConstChar(par_Value), ErrString);
            FreeErrStringBuffer (&ErrString);
            m_Value.m_Fixed = 0.0;
            return -1;
        }
    } else {
        union BB_VARI BBValue;
        if (string_to_bbvari (BB_DOUBLE, &BBValue, QStringToConstChar(par_Value)) == 0)  {
            m_Value.m_Fixed = BBValue.d;
            m_IsFixedValue = 1;
        } else {
            m_IsFixedValue = 0;  // first all are equations
            m_Value.m_ExecStack = solve_equation_err_string(QStringToConstChar(par_Value), &ErrString);
            if (m_Value.m_ExecStack == nullptr) {
                ThrowError (1, "cannot parse formula \"%s\"\n(%s)", QStringToConstChar(par_Value), ErrString);
                FreeErrStringBuffer (&ErrString);
                m_IsFixedValue = 1;
                m_Value.m_Fixed = 0.0;
                return -1;
            }
            if (!m_IsFixedValue) {
                m_AtachCounterIdx = GetNewAttachCounter();
            }
        }
    }
    return 0;
}

void BaseValue::SetFixed(double par_Value)
{
    if (!m_IsFixedValue) {
        m_AttachCounters[m_AtachCounterIdx] -= 1;
        if (m_AttachCounters[m_AtachCounterIdx] == 0) {
            if (m_Value.m_ExecStack != nullptr) {
                remove_exec_stack(m_Value.m_ExecStack);
                m_Value.m_ExecStack = nullptr;
            }
        }
    }
    m_IsFixedValue = true;
    m_Value.m_Fixed = par_Value;
}

double BaseValue::Get()
{
    if (m_IsFixedValue) {
        return m_Value.m_Fixed;
    } else {
        return execute_stack(m_Value.m_ExecStack);
    }
}

int BaseValue::GetNewAttachCounter()
{
    while (1) {
        for (int x = 0; x < m_NumOfAttachCounter; x++) {
            if (m_AttachCounters[x] <= 0) {
                m_AttachCounters[x] = 1;
                return x;
            }
        }
        m_NumOfAttachCounter += 64;
        m_AttachCounters = (int*)my_realloc(m_AttachCounters, sizeof(int) * m_NumOfAttachCounter);
        for (int x = m_NumOfAttachCounter-64; x  < m_NumOfAttachCounter; x++) {
            m_AttachCounters[x] = 0;
        }
    }
}

int BaseValue::m_NumOfAttachCounter;
int *BaseValue::m_AttachCounters;

ColorValue::ColorValue(QString &par_Value, bool par_OneTimeCalc)
{
    Parse(par_Value, par_OneTimeCalc);
}

ColorValue::ColorValue()
{

}

int ColorValue::Parse(QString &par_Value, bool par_OneTimeCalc)
{
    QStringList List = par_Value.split(':');
    if (List.size() == 3) {
        QString Value = List.at(0);
        if (!m_Red.Parse(Value, par_OneTimeCalc)) {
            Value = List.at(1);
            if (!m_Green.Parse(Value, par_OneTimeCalc)) {
                Value = List.at(2);
                if (!m_Blue.Parse(Value, par_OneTimeCalc)) {
                    return 0;
                }
            }
        }
    }
    ThrowError (1, "wrong color definition \"%s\" sample: 0xAB:0x12:0x1A", QStringToConstChar(par_Value));
    return -1;
}

void ColorValue::SetFixed(double par_Red, double par_Green, double par_Blue)
{
    m_Red.SetFixed(par_Red);
    m_Green.SetFixed(par_Green);
    m_Blue.SetFixed(par_Blue);
}

int ColorValue::GetInt()
{
    double Red = m_Red.Get();
    double Green = m_Green.Get();
    double Blue = m_Blue.Get();
    int RedInt = (Red < 255.0) ? (int)Red : 255;
    int GreenInt = (Green < 255.0) ? (int)Green : 255;
    int BlueInt = (Blue < 255.0) ? (int)Blue : 255;
    return (RedInt | (GreenInt << 8) | (BlueInt) << 16);
}

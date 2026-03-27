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


#include "UserControlElement.h"
#include "UserControlParser.h"
#include "QtIniFile.h"

extern "C" {
#include "PrintFormatToString.h"
#include "Blackboard.h"
#include "ExecutionStack.h"
#include "EquationParser.h"
#include "ThrowError.h"
#include "MyMemory.h"
}

UserControlElement::UserControlElement(UserControlElement *par_Parent)
{
    m_Parent = par_Parent;
    ResetToDefault();
}

UserControlElement::~UserControlElement()
{

}

bool UserControlElement::Init()
{
    return true;
}

void UserControlElement::ResetToDefault()
{
    m_Properties.clear();
    m_Color = 0;
    m_Properties.Add(QString("color"), QString());
    m_X = 0;
    m_Properties.Add(QString("x"), QString("0"));
    m_Y = 0;
    m_Properties.Add(QString("y"), QString("0"));
    m_Width = 1;
    m_Properties.Add(QString("width"), QString("1"));
    m_Height = 1;
    m_Properties.Add(QString("height"), QString("1"));

}

void UserControlElement::SetDefaultParameterString()
{
    m_ParameterLine = QString("(x=0,y=0,width=1,height=1)");
}


bool UserControlElement::AddChild(UserControlElement *par_Child, int par_Pos)
{
    Q_UNUSED(par_Child);
    Q_UNUSED(par_Pos);
    return false;
}

void UserControlElement::DeleteChild(int par_Pos)
{
    Q_UNUSED(par_Pos);
}

void UserControlElement::ChildHasChanged(UserControlElement *par_Child)
{
    Q_UNUSED(par_Child);
}

int UserControlElement::ChildNumber()
{
    if (m_Parent)
        return m_Parent->ChildNumber(this); // .indexOf(const_cast<CalibrationTreeItem*>(this));

    return 0;
}

int UserControlElement::ChildNumber(UserControlElement *par_Child)
{
    Q_UNUSED(par_Child);
    return 0;
}

bool UserControlElement::IsGroup()
{
    return false;
}

bool UserControlElement::ParseOneBaseParameter(QString &Key, QString &ValueString)
{
    if (!Key.compare("x", Qt::CaseInsensitive)) {
        m_X = ValueString.toUInt();
        m_Properties.Add("x", ValueString);
    } else if (!Key.compare("y", Qt::CaseInsensitive)) {
        m_Y = ValueString.toUInt();
        m_Properties.Add("y", ValueString);
    } else if (!Key.compare("width", Qt::CaseInsensitive)) {
        m_Width = ValueString.toUInt();
        m_Properties.Add("width", ValueString);
    } else if (!Key.compare("height", Qt::CaseInsensitive)) {
        m_Height = ValueString.toUInt();
        m_Properties.Add("height", ValueString);
    } else if (!Key.compare("color", Qt::CaseInsensitive) ||
               !Key.compare("c", Qt::CaseInsensitive)) {
        m_Color = ValueString.toUInt();
        m_Properties.Add("color", ValueString);
    } else {
        return false;
    }
    return true;
}


int UserControlElement::GetChildCount()
{
    return 0;
}

UserControlElement *UserControlElement::GetChild(int par_Index)
{
    Q_UNUSED(par_Index);
    return nullptr;
}

void UserControlElement::WriteToINI(QString &par_WindowName, QString &par_Entry, int par_Fd)
{
    QString Line = GetTypeString().append(m_ParameterLine);
    ScQt_IniFileDataBaseWriteString(par_WindowName, par_Entry, Line, par_Fd);
    for (int x = 0; x < GetChildCount(); x++) {
        UserControlElement *Child = GetChild(x);
        QString ChildEntry = par_Entry;
        char Help[32];
        PrintFormatToString (Help, sizeof(Help),"_%i", x);
        ChildEntry.append(QString(Help));
        Child->WriteToINI(par_WindowName, ChildEntry, par_Fd);
    }
}

void UserControlElement::ScriptStateChanged(int par_State)
{
    for (int x = 0; x < GetChildCount(); x++) {
        UserControlElement *Child = GetChild(x);
        Child->ScriptStateChanged(par_State);
    }
}


bool UserControlElement::ParseParameterString()
{
    UserControlParser Parser;
    return Parser.ParseParameters(m_ParameterLine, 0, this);
}

bool UserControlElement::InitParameter(int par_Pos, QString &par_ParameterString)
{
    ResetToDefault();
    if (par_Pos < 0) SetDefaultParameterString();
    else m_ParameterLine = par_ParameterString.mid(par_Pos);
    return ParseParameterString();
}


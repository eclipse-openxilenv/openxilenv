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


#include "UserControlButton.h"
#include "QtIniFile.h"
#include <QStaticText>
#include "InterfaceToScript.h"
#include "StringHelpers.h"

extern "C" {
#include "Config.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "EnvironmentVariables.h"
#include "EquationParser.h"
}

UserControlButton::UserControlButton(int par_Pos, QString &par_ParameterString, UserControlElement *par_Parent) : UserControlElement(par_Parent)
{
    m_PushButton = new QPushButton();
    connect (m_PushButton, SIGNAL(clicked()), this, SLOT(ButtonPressed()));
    InitParameter(par_Pos, par_ParameterString);
}

UserControlButton::~UserControlButton()
{
    delete m_PushButton;
}

void UserControlButton::ResetToDefault()
{
    UserControlElement::ResetToDefault();
    m_Text.clear();
    m_Properties.Add(QString("text"), QString());
    m_Script.clear();
    m_Properties.Add(QString("script"), QString(""));
    m_Equation.clear();
    m_Properties.Add(QString("equation"), QString(""));
}

void UserControlButton::SetDefaultParameterString()
{
    m_ParameterLine = QString("(x=0,y=0,width=1,height=1,text=button)");
}

bool UserControlButton::ParseOneParameter(QString &par_Value)
{
    int SeparatorPos = par_Value.indexOf('=', 0);
    if (SeparatorPos > 0) {
        // do not use split() because the right side can include = characters
        QString Key = par_Value.left(SeparatorPos).trimmed();
        QString ValueString = par_Value.right(par_Value.size() - (SeparatorPos + 1)).trimmed();
        if (!Key.compare("text", Qt::CaseInsensitive)) {
            m_Text = ValueString;
            m_PushButton->setText(m_Text);
            m_Properties.Add(QString("text"), ValueString);
        } else if (!Key.compare("script", Qt::CaseInsensitive)) {
            m_Script = ValueString;
            m_Properties.Add(QString("script"), ValueString);
        } else if (!Key.compare("equation", Qt::CaseInsensitive)) {
            m_Equation = ValueString;
            m_Properties.Add(QString("equation"), ValueString);
        } else {
            return ParseOneBaseParameter(Key, ValueString);
        }
        return true;
    }
    return false;
}

bool UserControlButton::Init()
{
    return true;
}

QString UserControlButton::GetTypeString()
{
    return QString("Button");
}

QWidget *UserControlButton::GetWidget()
{
    return m_PushButton;
}

void UserControlButton::ScriptStateChanged(int par_State)
{
    m_PushButton->setDisabled(par_State);
}

int UserControlButton::Is(QString &par_Line)
{
    if (par_Line.startsWith("Button", Qt::CaseInsensitive)) {
        return 6;
    } else {
        return 0;
    }
}

void UserControlButton::ButtonPressed()
{
    if (!m_Script.isEmpty()) {
        if (StartScript(QStringToConstChar(m_Script))) {
            ThrowError(1, "cannot start script \"%s\". an other script is running", QStringToConstChar(m_Script));
        }
    }
    if (!m_Equation.isEmpty()) {
        direct_solve_equation(QStringToConstChar(m_Equation));
    }
}


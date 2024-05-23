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


#include <QPainter>
#include "UserControlRoot.h"

#include "UserControlWidget.h"

bool UserControlRoot::ParseOneParameter(QString &par_Value)
{
    Q_UNUSED(par_Value)
    return true;
}

bool UserControlRoot::AddChild(UserControlElement *par_Child, int par_Pos)
{
    par_Child->m_Parent = this;
    if ((par_Pos < 0) || (par_Pos >= m_Childs.count())) m_Childs.append(par_Child);
    else m_Childs.insert(par_Pos, par_Child);
    //m_Layout->addWidget(par_Child->GetWidget(),par_Child->GetX(), par_Child->GetY(), par_Child->GetWidth(), par_Child->GetHeight());
    m_RootWidget->AddWidget(par_Child->GetWidget(),par_Child->GetX(), par_Child->GetY(), par_Child->GetWidth(), par_Child->GetHeight());
    return true;
}

void UserControlRoot::DeleteChild(int par_Pos)
{
    UserControlElement *Elem = m_Childs.at(par_Pos);
    m_RootWidget->RemoveWidget(Elem->GetWidget());
    m_Childs.removeAt(par_Pos);
    delete Elem;
}

void UserControlRoot::ChildHasChanged(UserControlElement *par_Child)
{
    m_RootWidget->RemoveWidget(par_Child->GetWidget());
    m_RootWidget->AddWidget(par_Child->GetWidget(),par_Child->GetX(), par_Child->GetY(), par_Child->GetWidth(), par_Child->GetHeight());
}

QString UserControlRoot::GetTypeString()
{
    return QString("Root");
}

int UserControlRoot::GetChildCount()
{
    return m_Childs.count();
}

UserControlElement *UserControlRoot::GetChild(int par_Index)
{
    return m_Childs.at(par_Index);
}

UserControlRoot::UserControlRoot(int par_Pos, QString &par_ParameterString, UserControlElement *par_Parent) : UserControlElement(par_Parent)
{
    m_RootWidget = nullptr;
    m_Layout = nullptr;
    InitParameter(par_Pos, par_ParameterString);
}

UserControlRoot::~UserControlRoot()
{
    foreach(UserControlElement *Elem, m_Childs) {
        delete Elem;
    }
    if (m_Layout != nullptr) delete m_Layout;
}

void UserControlRoot::ResetToDefault()
{
    UserControlElement::ResetToDefault();
}

void UserControlRoot::SetDefaultParameterString()
{
    m_ParameterLine = QString("()");
}

bool UserControlRoot::IsGroup()
{
    return true;
}

int UserControlRoot::ChildNumber(UserControlElement *par_Child)
{
    return m_Childs.indexOf(par_Child);
}


int UserControlRoot::Is(QString &par_Line)
{
    if (par_Line.startsWith("Root", Qt::CaseInsensitive)) {
        return 4;
    } else {
        return 0;
    }
}

QWidget *UserControlRoot::GetWidget()
{
    return m_RootWidget;
}

void UserControlRoot::SetWidget(UserControlWidget *par_Widget)
{
    if (m_Layout != nullptr) delete m_Layout;
    m_RootWidget = par_Widget;
    m_Layout = new QGridLayout();
    m_RootWidget->setLayout(m_Layout);
}



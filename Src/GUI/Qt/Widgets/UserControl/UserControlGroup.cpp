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


#include "UserControlGroup.h"

UserControlGroup::UserControlGroup(int par_Pos, QString &par_ParameterString, UserControlElement *par_Parent) : UserControlElement(par_Parent)
{
    //if (par_Parent == nullptr) m_GroupBox = new QGroupBox();
    //else m_GroupBox = new QGroupBox(par_Parent->GetWidget());
    m_GroupBox = new QGroupBox();
    m_Layout = new QGridLayout();
    m_GroupBox->setLayout(m_Layout);
    InitParameter(par_Pos, par_ParameterString);
}

UserControlGroup::~UserControlGroup()
{
    foreach(UserControlElement* Child, m_Childs) {
        delete Child;
    }
    delete m_Layout;
    delete m_GroupBox;
}

void UserControlGroup::ResetToDefault()
{
    UserControlElement::ResetToDefault();
}

bool UserControlGroup::AddChild(UserControlElement *par_Child, int par_Pos)
{
    par_Child->m_Parent = this;
    if ((par_Pos < 0) || (par_Pos >= m_Childs.count())) m_Childs.append(par_Child);
    else m_Childs.insert(par_Pos, par_Child);
    m_Layout->addWidget(par_Child->GetWidget(),par_Child->GetY(), par_Child->GetX(), par_Child->GetHeight(), par_Child->GetWidth());
    return true;
}

void UserControlGroup::DeleteChild(int par_Pos)
{
    UserControlElement *Elem = m_Childs.at(par_Pos);
    m_Layout->removeWidget(Elem->GetWidget());
    m_Childs.removeAt(par_Pos);
    delete Elem;
}

void UserControlGroup::ChildHasChanged(UserControlElement *par_Child)
{
    m_Layout->removeWidget(par_Child->GetWidget());
    m_Layout->addWidget(par_Child->GetWidget(),par_Child->GetY(), par_Child->GetX(), par_Child->GetHeight(), par_Child->GetWidth());
}

bool UserControlGroup::IsGroup()
{
    return true;
}

int UserControlGroup::ChildNumber(UserControlElement *par_Child)
{
    return m_Childs.indexOf(par_Child);
}

bool UserControlGroup::ParseOneParameter(QString &par_Value)
{
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        if (!Key.compare("text", Qt::CaseInsensitive)) {
            m_Text = ValueString;
            m_GroupBox->setTitle(m_Text);
            m_Properties.Add(QString("text"), ValueString);
        } else {
            return ParseOneBaseParameter(Key, ValueString);
        }
        return true;
    }
    return false;
}

int UserControlGroup::GetChildCount()
{
    return m_Childs.count();
}

UserControlElement *UserControlGroup::GetChild(int par_Index)
{
    return m_Childs.at(par_Index);
}

QString UserControlGroup::GetTypeString()
{
    return QString("Group");
}

QWidget *UserControlGroup::GetWidget()
{
    return m_GroupBox;
}

int UserControlGroup::Is(QString &par_Line)
{
    if (par_Line.startsWith("Group", Qt::CaseInsensitive)) {
        return 5;
    } else {
        return 0;
    }
}

void UserControlGroup::SetDefaultParameterString()
{
    m_ParameterLine = QString("(x=0,y=0,width=1,height=1,text=group)");
}

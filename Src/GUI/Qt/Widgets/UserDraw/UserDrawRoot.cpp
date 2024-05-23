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
#include "UserDrawRoot.h"


bool UserDrawRoot::ParseOneParameter(QString &par_Value)
{
    Q_UNUSED(par_Value)
    return true;
}


bool UserDrawRoot::AddChild(UserDrawElement *par_Item, int par_Pos)
{
    par_Item->m_Parent = this;
    if ((par_Pos < 0) || (par_Pos >= m_Childs.count())) m_Childs.append(par_Item);
    else m_Childs.insert(par_Pos, par_Item);
    return true;
}

void UserDrawRoot::DeleteChild(int par_Pos)
{
    m_Childs.removeAt(par_Pos);
}


void UserDrawRoot::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    if (m_FixedRatio) {
        if (par_Draw.m_XScale < par_Draw.m_YScale) par_Draw.m_YScale = par_Draw.m_XScale;
        else if (par_Draw.m_YScale < par_Draw.m_XScale) par_Draw.m_XScale = par_Draw.m_YScale;
    }
    if (m_Antialiasing) {
        par_Painter->setRenderHint(QPainter::Antialiasing);
    }

    foreach(UserDrawElement *Elem, m_Childs) {
        Elem->Paint(par_Painter, par_Images, par_Draw);
    }
}

bool UserDrawRoot::MousePressEvent(QMouseEvent *event, DrawParameter &par_Draw)
{
    foreach(UserDrawElement *Elem, m_Childs) {
        if (Elem->MousePressEvent(event, par_Draw)) {
            return true;
        }
    }
    return false;
}

QString UserDrawRoot::GetTypeString()
{
    return QString("Root");
}

int UserDrawRoot::GetChildCount()
{
    return m_Childs.count();
}

UserDrawElement *UserDrawRoot::GetChild(int par_Index)
{
    return m_Childs.at(par_Index);
}

UserDrawRoot::UserDrawRoot(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawRoot::~UserDrawRoot()
{
    foreach(UserDrawElement *Elem, m_Childs) {
        delete Elem;
    }
}

void UserDrawRoot::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
    m_FixedRatio = false;
    m_Antialiasing = false;
}

void UserDrawRoot::SetDefaultParameterString()
{
    m_ParameterLine = QString("()");
}

bool UserDrawRoot::IsGroup()
{
    return true;
}

int UserDrawRoot::ChildNumber(UserDrawElement *par_Child)
{
    return m_Childs.indexOf(par_Child);
}


int UserDrawRoot::Is(QString &par_Line)
{
    if (par_Line.startsWith("Root", Qt::CaseInsensitive)) {
        return 4;
    } else {
        return 0;
    }
}



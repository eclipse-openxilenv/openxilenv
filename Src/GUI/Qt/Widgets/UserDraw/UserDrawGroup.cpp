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


#include "UserDrawGroup.h"

UserDrawGroup::UserDrawGroup(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawGroup::~UserDrawGroup()
{
    foreach(UserDrawElement* Child, m_Childs) {
        delete Child;
    }
}

void UserDrawGroup::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
}

bool UserDrawGroup::AddChild(UserDrawElement *par_Child, int par_Pos)
{
    par_Child->m_Parent = this;
    if ((par_Pos < 0) || (par_Pos >= m_Childs.count())) m_Childs.append(par_Child);
    else m_Childs.insert(par_Pos, par_Child);
    return 0;
}

void UserDrawGroup::DeleteChild(int par_Pos)
{
    m_Childs.removeAt(par_Pos);
}

bool UserDrawGroup::IsGroup()
{
    return true;
}

int UserDrawGroup::ChildNumber(UserDrawElement *par_Child)
{
    return m_Childs.indexOf(par_Child);
}

bool UserDrawGroup::ParseOneParameter(QString &par_Value)
{
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        return ParseOneBaseParameter(Key, ValueString);
    }
    return false;
}

void UserDrawGroup::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    if (m_Visible.Get() >= 0.5) {
        // TODO: DrawParameter D = ParentDraw(par_Draw);
        double Rot = m_Rot.Get();
        double Scale = m_Scale.Get();
        TranslateParameter P(m_X.Get(),
                             m_Y.Get(),
                             Scale,
                             Rot,
                             par_Draw);
        double x = 0.0;
        double y = 0.0;
        Translate(&x, &y, P, TranslateParentAndItself);
        DrawParameter D(x, y, Scale, Rot + par_Draw.m_ParentRot, par_Draw.m_XScale, par_Draw.m_YScale, par_Draw.m_UpdateType);
        foreach(UserDrawElement* Child, m_Childs) {
            Child->Paint(par_Painter, par_Images, D);
        }
    }
}

int UserDrawGroup::GetChildCount()
{
    return m_Childs.count();
}

UserDrawElement *UserDrawGroup::GetChild(int par_Index)
{
    return m_Childs.at(par_Index);
}

QString UserDrawGroup::GetTypeString()
{
    return QString("Group");
}

int UserDrawGroup::Is(QString &par_Line)
{
    if (par_Line.startsWith("Group", Qt::CaseInsensitive)) {
        return 5;
    } else {
        return 0;
    }
}

void UserDrawGroup::SetDefaultParameterString()
{
    m_ParameterLine = QString("(x=0,y=0)");
}

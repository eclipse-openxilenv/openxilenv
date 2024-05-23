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
#include "UserDrawLayer.h"

/*bool UserDrawLayer::Parse(QString &par_Line)
{
    QStringList List = par_Line.split(',');
    if (List.size() == 2) {
        QString ValueString = List.at(0).trimmed();
        if (!ValueString.compare("UpdateFast")) {
            m_Update = UpdateFast;
        } else if (!ValueString.compare("UpdateMedium")) {
            m_Update = UpdateMedium;
        } else if (!ValueString.compare("UpdateSlow")) {
            m_Update = UpdateSlow;
        } else if (!ValueString.compare("UpdateNone")) {
            m_Update = UpdateNone;
        } else if (!ValueString.compare("UpdateEvent")) {
            m_Update = UpdateEvent;
        } else {
            m_Update = UpdateFast;
        }
        ValueString = List.at(1).trimmed();
        if (!ValueString.compare("Transparency")) {
            m_Transparency = true;
        } else {
            m_Transparency = false;
        }
    }
    return true;
}*/

bool UserDrawLayer::ParseOneParameter(QString &par_Value)
{
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        if (!Key.compare("update", Qt::CaseInsensitive)) {
            if (!ValueString.compare("Fast")) {
                m_Update = UpdateFast;
            } else if (!ValueString.compare("Medium")) {
                m_Update = UpdateMedium;
            } else if (!ValueString.compare("Slow")) {
                m_Update = UpdateSlow;
            } else if (!ValueString.compare("None")) {
                m_Update = UpdateNone;
            } else if (!ValueString.compare("Event")) {
                m_Update = UpdateEvent;
            }
            m_Properties.Add(QString("update"), ValueString);
        } else if (!Key.compare("transparent", Qt::CaseInsensitive)) {
            if (!ValueString.compare("yes")) {
                m_Transparency = true;
            } else {
                m_Transparency = false;
            }
            m_Properties.Add(QString("transparent"), ValueString);
        }
        return ParseOneBaseParameter(Key, ValueString);
    }
    return false;
}

bool UserDrawLayer::IsGroup()
{
    return true;
}

int UserDrawLayer::ChildNumber(UserDrawElement *par_Child)
{
    return m_Childs.indexOf(par_Child);
}

bool UserDrawLayer::AddChild(UserDrawElement *par_Item, int par_Pos)
{
    par_Item->m_Parent = this;
    if ((par_Pos < 0) || (par_Pos >= m_Childs.count())) m_Childs.append(par_Item);
    else m_Childs.insert(par_Pos, par_Item);
    return true;
}

void UserDrawLayer::DeleteChild(int par_Pos)
{
    m_Childs.removeAt(par_Pos);
}


void UserDrawLayer::ResizeImage(int par_SizeX, int par_SizeY)
{
    if (m_Image == nullptr) {
        CreateImage(par_SizeX, par_SizeY);
    } else if ((m_Image->width() != par_SizeX) ||
               (m_Image->height() != par_SizeY))  {
        delete m_Image;
        CreateImage(par_SizeX, par_SizeY);
    }
}

QImage *UserDrawLayer::Get()
{
    return m_Image;
}

void UserDrawLayer::DrawImage(QPainter *par_Painter)
{
    par_Painter->drawImage(0,0, *m_Image);
}

void UserDrawLayer::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    if ((m_Image == nullptr) ||
        (m_Image->width() != par_Draw.m_XScale) ||
        (m_Image->height() != par_Draw.m_YScale)) {
        ResizeImage(par_Draw.m_XScale, par_Draw.m_YScale);
    }

    if ((par_Draw.m_UpdateType == DrawParameter::UpdateTypeInit) ||
        ((par_Draw.m_UpdateType == DrawParameter::UpdateTypeCyclic) && (m_Update <= UpdateSlow))) {
        if (m_Transparency) {
            m_Image->fill(Qt::transparent);
        } else {
            m_Image->fill(Qt::white);
        }
        QPainter Painter(m_Image);
        if (m_Transparency) {
            Painter.setBackgroundMode(Qt::TransparentMode);
        } else {
            Painter.setBackgroundMode(Qt::OpaqueMode);
        }
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
        foreach(UserDrawElement *Elem, m_Childs) {
            Elem->Paint(&Painter, par_Images, D);
        }
    }
    par_Painter->drawImage(0,0, *m_Image);
}

int UserDrawLayer::GetChildCount()
{
    return m_Childs.count();
}

UserDrawElement *UserDrawLayer::GetChild(int par_Index)
{
    return m_Childs.at(par_Index);
}

QString UserDrawLayer::GetTypeString()
{
    return QString("Layer");
}

UserDrawLayer::UserDrawLayer(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    m_Image = nullptr;  // this must be done before ResetToDefault()
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawLayer::~UserDrawLayer()
{
    foreach(UserDrawElement *Elem, m_Childs) {
        delete Elem;
    }
}

void UserDrawLayer::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
    m_Update = UpdateFast;
    m_Transparency = false;
    QStringList Enums;
    Enums.append(QString("Fast"));
    Enums.append(QString("Medium"));
    Enums.append(QString("Slow"));
    Enums.append(QString("None"));
    Enums.append(QString("Event"));
    m_Properties.Add(QString("update"), QString(), UserDrawPropertiesListElem::UserDrawPropertyEnumType, Enums);
    Enums.clear();
    Enums.append(QString("no"));
    Enums.append(QString("yes"));
    m_Properties.Add(QString("transparent"), QString(), UserDrawPropertiesListElem::UserDrawPropertyEnumType, Enums);
    m_Update = UpdateCyclic;
    m_Transparency = false;
    if (m_Image != nullptr) {
        delete m_Image;
        m_Image = nullptr;
    }
}

bool UserDrawLayer::CreateImage(int par_SizeX, int par_SizeY)
{
    if (m_Transparency) {
        m_Image = new QImage(par_SizeX, par_SizeY, QImage::Format_ARGB32);  // transparency
    } else {
        m_Image = new QImage(par_SizeX, par_SizeY, QImage::Format_RGB32);
    }
    return true;
}

int UserDrawLayer::Is(QString &par_Line)
{
    if (par_Line.startsWith("Layer", Qt::CaseInsensitive)) {
        return 5;
    } else {
        return 0;
    }
}

void UserDrawLayer::SetDefaultParameterString()
{
    m_ParameterLine = QString("(update=Fast,transparent=no)");
}

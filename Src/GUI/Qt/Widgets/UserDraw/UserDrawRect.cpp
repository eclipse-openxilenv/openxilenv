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


#include "UserDrawRect.h"

UserDrawRect::UserDrawRect(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawRect::~UserDrawRect()
{
}

void UserDrawRect::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
    m_Width.SetFixed(0.5);
    m_Properties.Add(QString("width"), QString());
    m_Height.SetFixed(0.5);
    m_Properties.Add(QString("height"), QString());
}

bool UserDrawRect::ParseOneParameter(QString &par_Value)
{
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        if (!Key.compare("width", Qt::CaseInsensitive) ||
            !Key.compare("w", Qt::CaseInsensitive)) {
            m_Width.Parse(ValueString);
            m_Properties.Add("width", ValueString);
        } else if (!Key.compare("height", Qt::CaseInsensitive) ||
                   !Key.compare("h", Qt::CaseInsensitive)) {
            m_Height.Parse(ValueString);
            m_Properties.Add("height", ValueString);
        } else {
            return ParseOneBaseParameter(Key, ValueString);
        }
        return true;
    }
    return false;
}

void UserDrawRect::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    Q_UNUSED(par_Images)
    if (m_Visible.Get() >= 0.5) {
        SetColor(par_Painter);
        SetFillColor(par_Painter);
        TranslateParameter P(m_X.Get(),
                             m_Y.Get(),
                             m_Scale.Get(),
                             m_Rot.Get(),
                             par_Draw);
        QPointF Points[4];

        double w = m_Width.Get();
        double h = m_Height.Get();

        double x = 0.0;
        double y = 0.0;
        Translate(&x, &y, P);
        Points[0].setX(x);
        Points[0].setY(y);
        x = w;
        y = 0.0;
        Translate(&x, &y, P);
        Points[1].setX(x);
        Points[1].setY(y);
        x = w;
        y = h;
        Translate(&x, &y, P);
        Points[2].setX(x);
        Points[2].setY(y);
        x = 0.0;
        y = h;
        Translate(&x, &y, P);
        Points[3].setX(x);
        Points[3].setY(y);

        par_Painter->drawPolygon(Points, 4);
    }
}

QString UserDrawRect::GetTypeString()
{
    return QString("Rect");
}

int UserDrawRect::Is(QString &par_Line)
{
    if (par_Line.startsWith("Rect", Qt::CaseInsensitive)) {
        return 4;
    } else {
        return 0;
    }
}

void UserDrawRect::SetDefaultParameterString()
{
    m_ParameterLine = QString("(width=0.5,height=0.5)");
}

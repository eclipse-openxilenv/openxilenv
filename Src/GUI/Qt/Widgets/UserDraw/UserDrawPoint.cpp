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


#include "UserDrawPoint.h"
#include "QtIniFile.h"
extern "C" {
#include "Config.h"
#include "ThrowError.h"
}

UserDrawPoint::UserDrawPoint(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawPoint::~UserDrawPoint()
{
}

void UserDrawPoint::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
    //m_Properties.Add(QString("radius"), QString());
}

void UserDrawPoint::SetDefaultParameterString()
{
    m_ParameterLine = QString("()");
}

bool UserDrawPoint::ParseOneParameter(QString &par_Value)
{
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        /*if (!Key.compare("radius", Qt::CaseInsensitive) ||
            !Key.compare("r", Qt::CaseInsensitive)) {
        } else {*/
            return ParseOneBaseParameter(Key, ValueString);
        //}
        //return true;
    }
    return false;
}

void UserDrawPoint::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    Q_UNUSED(par_Images)
    if (m_Visible.Get() >= 0.5) {
        SetColor(par_Painter);
        SetFillColor(par_Painter);
        double Scale = m_Scale.Get();
        double x = m_X.Get();
        double y = m_Y.Get();
        TranslateParameter P(x,
                             y,
                             Scale,
                             0.0,
                             par_Draw);
        x = y = 0.0;
        Translate(&x, &y, P, TranslateAllToScreen);
        QPointF Point(x, y);
        par_Painter->drawPoint(Point);
    }
}

bool UserDrawPoint::Init()
{
    return true;
}

QString UserDrawPoint::GetTypeString()
{
    return QString("Point");
}

int UserDrawPoint::Is(QString &par_Line)
{
    if (par_Line.startsWith("Point", Qt::CaseInsensitive)) {
        return 5;
    } else {
        return 0;
    }
}


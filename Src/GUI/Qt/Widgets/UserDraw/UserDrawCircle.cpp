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


#include "UserDrawCircle.h"
#include "QtIniFile.h"
extern "C" {
#include "Config.h"
#include "ThrowError.h"
}

UserDrawCircle::UserDrawCircle(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawCircle::~UserDrawCircle()
{
}

void UserDrawCircle::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
    m_RotationType = RotationTypeNone;
    m_CenterType = CenterTypeCenter;
    m_Radius.SetFixed(0.5);
    m_Properties.Add(QString("radius"), QString("0.5"));
}

void UserDrawCircle::SetDefaultParameterString()
{
    m_ParameterLine = QString("(radius=0.5)");
}

bool UserDrawCircle::ParseOneParameter(QString &par_Value)
{
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        if (!Key.compare("radius", Qt::CaseInsensitive) ||
            !Key.compare("r", Qt::CaseInsensitive)) {
            m_Radius.Parse(ValueString);
            m_Properties.Add(QString("radius"), ValueString);
        } else if (!Key.compare("RotPoint", Qt::CaseInsensitive)) {
            if (!ValueString.compare("Center", Qt::CaseInsensitive)) {
                m_RotationType = RotationTypeCenter;
            } else {
                m_RotationType = RotationTypeNone;
                if (ValueString.startsWith('(') && ValueString.endsWith(')')) {
                    // remove brackets
                    ValueString.remove(par_Value.size() - 1, 1);
                    ValueString.remove(0, 1);
                    ValueString = ValueString.trimmed();
                    QStringList RotList = ValueString.split(',');
                    if (RotList.size() >= 2) {
                        ValueString = RotList.at(0).trimmed();
                        m_RotXPos.Parse(ValueString);
                        ValueString = RotList.at(0).trimmed();
                        m_RotYPos.Parse(ValueString);
                        m_RotationType = RotationTypePos;
                    }
                }
            }
            m_Properties.Add(QString("RotPoint"), ValueString);
        } else {
            return ParseOneBaseParameter(Key, ValueString);
        }
        return true;
    }
    return false;
}

void UserDrawCircle::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    Q_UNUSED(par_Images)
    if (m_Visible.Get() >= 0.5) {
        SetColor(par_Painter);
        SetFillColor(par_Painter);
        double Scale = m_Scale.Get();
        double x = m_X.Get();
        double y = m_Y.Get();
        double Rot = m_Rot.Get();
        double Radius = m_Radius.Get();
        TranslateParameter P(x,
                             y,
                             Scale,
                             Rot,
                             par_Draw);
        /*switch (m_RotationType) {
        case RotationTypeNone:
            //break;
        case RotationTypeCenter:
            x = -0.5;
            y = 0.5;
            break;
        case RotationTypePos:
            x = -0.5+m_RotXPos.Get() / Scale;
            y = 0.5-m_RotYPos.Get() / Scale;
            break;
        }*/

        double x1 = - Radius;
        double y1 = - Radius;
        Translate(&x1, &y1, P, TranslateAllToScreen);
        double x2 = Radius;
        double y2 = Radius;
        Translate(&x2, &y2, P, TranslateAllToScreen);

        double w, h;
        if (x1 > x2) {
            double xx = x2;
            x2 = x1;
            x1 = xx;
        }
        w = x2 - x1;
        if (y1 > y2) {
            double yy = y2;
            y2 = y1;
            y1 = yy;
        }
        h = y2 - y1;

        QRectF Rectangle(x1, y1, w, h);
        par_Painter->drawEllipse(Rectangle);
    }
}

bool UserDrawCircle::Init()
{
    return true;
}

QString UserDrawCircle::GetTypeString()
{
    return QString("Circle");
}

int UserDrawCircle::Is(QString &par_Line)
{
    if (par_Line.startsWith("Circle", Qt::CaseInsensitive)) {
        return 6;
    } else {
        return 0;
    }
}


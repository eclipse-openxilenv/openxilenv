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


#include "UserDrawText.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

#include <QStaticText>
extern "C" {
#include "Config.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "EnvironmentVariables.h"
}

UserDrawText::UserDrawText(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    InitParameter(par_Pos, par_ParameterString);
    //m_Buffer = (char*)my_malloc (1024);
}

UserDrawText::~UserDrawText()
{
    //if (m_Buffer != nullptr) my_free(m_Buffer);
}

void UserDrawText::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
    //m_Size.SetFixed(16.0);
    m_RotationType = RotationTypeNone;
    m_CenterType = CenterTypeCenter;
    m_Properties.Add(QString("text"), QString());
}

void UserDrawText::SetDefaultParameterString()
{
    m_ParameterLine = QString("(x=0,y=0,scale=0.1,text=text)");
}

bool UserDrawText::ParseOneParameter(QString &par_Value)
{
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        if (!Key.compare("text", Qt::CaseInsensitive)) {
            m_Text = ValueString;
            m_Properties.Add(QString("text"), ValueString);
        // } else if (!Key.compare("size", Qt::CaseInsensitive)) {
        //    m_Size.Parse(ValueString);
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

void UserDrawText::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    Q_UNUSED(par_Images)
    if (m_Visible.Get() >= 0.5) {
        QStaticText Text;
        if (SearchAndReplaceEnvironmentStrings(QStringToConstChar(m_Text), m_Buffer, 1024 + 0x40000000) > 0) {  // no error messages
            Text.setText(QString(m_Buffer));
        } else {
            Text.setText(QString("cannot evaluate string"));
        }


        double Scale = m_Scale.Get();
        double x = m_X.Get();
        double y = m_Y.Get();
        double Rot = m_Rot.Get();
        TranslateParameter P(x,
                             y,
                             Scale,
                             Rot,
                             par_Draw);

        QFont Font = par_Painter->font();
        //Font.setPointSizeF(par_Draw.m_YScale); //m_Size.Get());
        Font.setPixelSize(par_Draw.m_YScale); //m_Size.Get());
        par_Painter->setFont(Font);

        QRect Rect = par_Painter->fontMetrics().boundingRect(Text.text());
        int Width = Rect.width();
        int Height = Rect.height();
        switch (m_RotationType) {
        case RotationTypeNone:
            //break;
        case RotationTypeCenter:
            x = 0.0 - Width * 0.5 / par_Draw.m_YScale;
            y = 0.0 + Height * 0.5 / par_Draw.m_YScale;
            break;
        case RotationTypePos:
            x = -0.5+m_RotXPos.Get() / Scale;
            y = 0.5-m_RotYPos.Get() / Scale;
            break;
        }

        // obere Bildecke
        //y = 1.0; //Scale * par_Draw.m_ParentScale;
        //x = 0;
        Translate(&x, &y, P, TranslateAllToScreen);
        //Center
        //double cy = 0.5;
        //double cx = -0.5;
        //Translate(&cx, &cy, P, TranslateAllToScreen);

        double xs = (par_Draw.m_XScale / (double)Height) *Scale * par_Draw.m_ParentScale;
        double ys = (par_Draw.m_YScale / (double)Height) *Scale * par_Draw.m_ParentScale;

        QTransform TScale;
        TScale.scale(xs, ys);
        QTransform TRotade;
        TRotade.rotateRadians((Rot + par_Draw.m_ParentRot));
        QTransform TTranslate;
        TTranslate.translate(x, y);
        QTransform Transform;
        Transform =  TRotade * TScale * TTranslate;
        par_Painter->setTransform(Transform);

        SetColor(par_Painter);

        par_Painter->drawStaticText(0,0, Text);

        par_Painter->resetTransform();
    }
}

bool UserDrawText::Init()
{
    return true;
}

QString UserDrawText::GetTypeString()
{
    return QString("Text");
}

int UserDrawText::Is(QString &par_Line)
{
    if (par_Line.startsWith("Text", Qt::CaseInsensitive)) {
        return 4;
    } else {
        return 0;
    }
}


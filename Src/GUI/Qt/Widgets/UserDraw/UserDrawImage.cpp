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


#include "UserDrawImage.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

extern "C" {
#include "Config.h"
#include "ThrowError.h"
}

UserDrawImage::UserDrawImage(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawImage::~UserDrawImage()
{
    if (m_Image != nullptr) {
        delete m_Image;
    }
}

void UserDrawImage::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
    m_RotationType = RotationTypeNone;
    m_CenterType = CenterTypeCenter;
    m_Properties.Add(QString("width"), QString());
    m_Properties.Add(QString("height"), QString());
    m_Properties.Add(QString("nr"), QString());
    m_Properties.Add(QString("image"), QString());
    m_Properties.Add(QString("file"), QString());
}

void UserDrawImage::SetDefaultParameterString()
{
    m_ParameterLine = QString("(x=0,y=0,scale=1.0)");
}

bool UserDrawImage::ParseOneParameter(QString &par_Value)
{
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        if (!Key.compare("width", Qt::CaseInsensitive) ||
            !Key.compare("w", Qt::CaseInsensitive)) {
            m_Width.Parse(ValueString);
            m_Properties.Add(QString("width"), ValueString);
        } else if (!Key.compare("height", Qt::CaseInsensitive) ||
                   !Key.compare("h", Qt::CaseInsensitive)) {
            m_Height.Parse(ValueString);
            m_Properties.Add(QString("height"), ValueString);
        } else if (!Key.compare("image", Qt::CaseInsensitive) ||
                   !Key.compare("i", Qt::CaseInsensitive)) {
            m_ImageFileOrName = ValueString;
            m_ImageType = IMAGE_TYPE_NAME;
            m_Properties.Add(QString("image"), ValueString);
        } else if (!Key.compare("nr", Qt::CaseInsensitive) ||
                   !Key.compare("n", Qt::CaseInsensitive)) {
            m_ImageNr.Parse(ValueString);
            m_ImageType = IMAGE_TYPE_NUMBER;
            m_Properties.Add(QString("nr"), ValueString);
        } else if (!Key.compare("file", Qt::CaseInsensitive) ||
                   !Key.compare("f", Qt::CaseInsensitive)) {
            m_ImageFileOrName = ValueString;
            m_ImageType = IMAGE_TYPE_FILE;
            m_Properties.Add(QString("file"), ValueString);
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


void UserDrawImage::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    if (m_Visible.Get() >= 0.5) {
        QImage Image;
        switch(m_ImageType) {
        case IMAGE_TYPE_NUMBER:
            for (int x = 0; x < par_Images->count(); x++) {
                UserDrawImageItem *Item = par_Images->at(x);
                if (Item->m_Number == (int)m_ImageNr.Get()) {
                    Image = Item->m_Image;
                }
            }
            break;
        case IMAGE_TYPE_NAME:
            for (int x = 0; x < par_Images->count(); x++) {
                UserDrawImageItem *Item = par_Images->at(x);
                if (Item->m_Name.compare(m_ImageFileOrName) == 0) {
                    Image = Item->m_Image;
                }
            }
            break;
        case IMAGE_TYPE_FILE:
            if (m_Image != nullptr) {
                Image = *m_Image;
            }
            break;
        }

        int w = Image.width();
        int h = Image.height();
        double Scale = m_Scale.Get();
        double x = m_X.Get();
        double y = m_Y.Get();
        double Rot = m_Rot.Get();
        TranslateParameter P(x,
                             y,
                             Scale,
                             Rot,
                             par_Draw);
        switch (m_RotationType) {
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
        }

        // obere Bildecke
        //y = 1.0; //Scale * par_Draw.m_ParentScale;
        //x = 0;
        Translate(&x, &y, P, TranslateAllToScreen);
        //Center
        //double cy = 0.5;
        //double cx = -0.5;
        //Translate(&cx, &cy, P, TranslateAllToScreen);

        double xs = (par_Draw.m_XScale / (double)w) * Scale * par_Draw.m_ParentScale;
        double ys = (par_Draw.m_YScale / (double)h) * Scale * par_Draw.m_ParentScale;

        QTransform TScale;
        TScale.scale(xs, ys);
        QTransform TRotade;
        TRotade.rotateRadians((Rot + par_Draw.m_ParentRot));;
        QTransform TTranslate;
        TTranslate.translate(x, y);

        QTransform Transform;
        Transform =  TRotade * TScale * TTranslate;
        //Transform.scale(xs, ys);
        //Transform.rotateRadians((Rot + par_Draw.m_ParentRot));
        //Transform.translate(x, y);
        //par_Painter->drawImage(x,y, *m_Image);
        par_Painter->setTransform(Transform);
        par_Painter->drawImage(0,0, Image);
        par_Painter->resetTransform();
        //par_Painter->restore();

        /*par_Painter->setPen(Qt::red);
        par_Painter->drawPoint(x, y);
        par_Painter->setPen(Qt::blue);
        par_Painter->drawPoint(cx, cy);*/
    }
}

bool UserDrawImage::Init()
{
    LastImageNr = -1;
    switch(m_ImageType) {
    case IMAGE_TYPE_NUMBER:
    case IMAGE_TYPE_NAME:
    case IMAGE_TYPE_FILE:
        m_Image = new QImage(m_ImageFileOrName);
        if (m_Image == nullptr) {
            ThrowError (1, "cannot load image file \"%s\"", QStringToConstChar(m_ImageFileOrName));
        }
        break;
    }

    return true;
}

QString UserDrawImage::GetTypeString()
{
    return QString("Image");
}

int UserDrawImage::Is(QString &par_Line)
{
    if (par_Line.startsWith("Image", Qt::CaseInsensitive)) {
        return 5;
    } else {
        return 0;
    }
}


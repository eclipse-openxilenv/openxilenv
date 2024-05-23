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


#include "UserDrawButton.h"
#include "GetEventPos.h"
#include "StringHelpers.h"

#include <QStaticText>

extern "C" {
#include "ThrowError.h"
}
UserDrawButton::UserDrawButton(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawButton::~UserDrawButton()
{
}

void UserDrawButton::ResetToDefault()
{
    UserDrawElement::ResetToDefault();
    m_Width.SetFixed(0.5);
    m_Properties.Add(QString("width"), QString());
    m_Height.SetFixed(0.5);
    m_Properties.Add(QString("height"), QString());
    m_Text.clear();
    m_Properties.Add(QString("text"), QString());
    m_Script.clear();
    m_Properties.Add(QString("script"), QString());
    m_Properties.Add(QString("nr"), QString());
    m_Properties.Add(QString("image"), QString());
    m_Properties.Add(QString("file"), QString());
}

bool UserDrawButton::ParseOneParameter(QString &par_Value)
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
        } else if (!Key.compare("text", Qt::CaseInsensitive)) {
           m_Text = ValueString.trimmed();
           m_Properties.Add(QString("text"), ValueString);
        } else if (!Key.compare("script", Qt::CaseInsensitive)) {
           m_Script = ValueString.trimmed();
           m_Properties.Add(QString("script"), ValueString);
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

#if 0
void UserDrawButton::PaintText(QPainter *par_Painter, double par_Hight, double par_Width, DrawParameter &par_Draw)
{
    QStaticText Text;
    /*if ((m_Buffer != nullptr) &&
        (SearchAndReplaceEnvironmentStrings(QStringToConstChar(m_Text), m_Buffer, 1024 + 0x40000000) > 0)) {  // no error messages
        Text.setText(QString(m_Buffer));
    } else {
        Text.setText(QString("cannot evaluate string"));
    }*/

    Text.setText(m_Text);

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
    Font.setPixelSize(par_Draw.m_YScale * par_Hight); //m_Size.Get());
    par_Painter->setFont(Font);

    QRect Rect = par_Painter->fontMetrics().boundingRect(Text.text());
    int Width = Rect.width();
    int Height = Rect.height();
    /*switch (m_RotationType) {
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
    }*/

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
#endif
#if 0
void UserDrawButton::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    Q_UNUSED(par_Images)
    if (m_Visible.Get() >= 0.5) {
        //PaintText(par_Painter, m_Height.Get(), m_Width.Get(), par_Draw);
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
#endif
void UserDrawButton::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
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

bool UserDrawButton::Init()
{
    LastImageNr = -1;
    switch(m_ImageType) {
    case IMAGE_TYPE_NUMBER:
    case IMAGE_TYPE_NAME:
        m_Image = nullptr;
        break;
    case IMAGE_TYPE_FILE:
        m_Image = new QImage(m_ImageFileOrName);
        if (m_Image == nullptr) {
            ThrowError (1, "cannot load image file \"%s\"", QStringToConstChar(m_ImageFileOrName));
        } else {
            QPainter p(m_Image);
            //if (!p.begin(m_Image)) return false;
             p.setPen(QPen(Qt::red));
             p.setFont(QFont("Times", 12, QFont::Bold));
             p.drawText(m_Image->rect(), Qt::AlignCenter, "Text");
             p.end();
        }
        break;
    }

    return true;
}


static double sign(QPointF &p1, QPointF &p2, QPointF &p3)
{
  return (p1.x() - p3.x()) * (p2.y() - p3.y()) - (p2.x() - p3.x()) * (p1.y() - p3.y());
}

static bool PointInTriangle(QPointF &pt, QPointF &v1, QPointF &v2, QPointF &v3)
{
  bool b1, b2, b3;

  b1 = sign(pt, v1, v2) < 0.0;
  b2 = sign(pt, v2, v3) < 0.0;
  b3 = sign(pt, v3, v1) < 0.0;

  return ((b1 == b2) && (b2 == b3));
}

static bool PointInRectangle(QPointF &pt, QPointF *Rect)
{
    if (PointInTriangle(pt, Rect[0], Rect[1], Rect[2])) {
        return true;
    } else if (PointInTriangle(pt, Rect[2], Rect[3], Rect[0])) {
        return true;
    }
    return false;
}

bool UserDrawButton::MousePressEvent(QMouseEvent *event, DrawParameter &par_Draw)
{
    if (m_Visible.Get() >= 0.5) {
        TranslateParameter P(m_X.Get(),
                             m_Y.Get(),
                             m_Scale.Get(),
                             m_Rot.Get(),
                             par_Draw);
        QPointF p, Points[4];

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

        p.setX(GetEventXPos(event));
        p.setY(GetEventYPos(event));
        bool Ret = PointInRectangle(p, Points);
        return Ret;

    }
    return false;
}

QString UserDrawButton::GetTypeString()
{
    return QString("Button");
}

int UserDrawButton::Is(QString &par_Line)
{
    if (par_Line.startsWith("Button", Qt::CaseInsensitive)) {
        return 6;
    } else {
        return 0;
    }
}

void UserDrawButton::SetDefaultParameterString()
{
    m_ParameterLine = QString("(width=0.5,height=0.5)");
}

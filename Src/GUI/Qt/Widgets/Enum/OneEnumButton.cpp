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


#include "OneEnumButton.h"
#include "OneEnumVariable.h"
#include <QPainter>

OneEnumButton::OneEnumButton(QString label, QColor color, double par_StartValue, double par_EndValue, OneEnumVariable *par_Variable ,QWidget *parent) :
    QPushButton(parent)
{
    buttonLabel = label;
    buttonColor = color;
    m_StartValue = par_StartValue;
    m_EndValue = par_EndValue;

    QColor stdColor(255,255,255);
    setCheckable(true);
    setStyleSheet(QString("QPushButton:checked { background-color: QLinearGradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgb(%1, %2, %3), stop:1 rgb(%4, %5, %6));"
                          "border-style:outset; border-width:2px; border-radius:10px; border-color:beige; min-width:3em; min-height:1em; padding 2px; }"
                          "QPushButton:!checked { background-color: QLinearGradient(x1:0, y1:1, x2:0, y2:0, stop:0 rgb(%7, %8, %9), stop:1 rgb(%10, %11, %12));"
                          "border-style:outset; border-width:2px; border-radius:10px; border-color:beige; min-width:3em; min-height:1em; padding 6px; } ").arg(QString().number(qRed(stdColor.rgb()))).arg(QString().number(qGreen(stdColor.rgb()))).arg(QString().number(qBlue(stdColor.rgb()))).arg(QString().number(qRed(stdColor.darker().rgb()))).arg(QString().number(qGreen(stdColor.darker().rgb()))).arg(QString().number(qBlue(stdColor.darker().rgb()))).arg(QString().number(qRed(stdColor.lighter().rgb()))).arg(QString().number(qGreen(stdColor.lighter().rgb()))).arg(QString().number(qBlue(stdColor.lighter().rgb()))).arg(QString().number(qRed(stdColor.lighter(170).rgb()))).arg(QString().number(qGreen(stdColor.lighter(170).rgb()))).arg(QString().number(qBlue(stdColor.lighter(170).rgb()))));

    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    setMaximumSize(65535, 65535);
    connect(this, SIGNAL(clicked()), this, SLOT(blackboardValueChange()));
    connect(this, SIGNAL(blackboardValueChange(double)), par_Variable, SLOT(blackboardValueChange(double)));

}

OneEnumButton::~OneEnumButton()
{

}

void OneEnumButton::CyclicUpdate(double par_Value)
{
    if ((par_Value >= m_StartValue) && (par_Value <= m_EndValue)) {
        setChecked(true);
    } else {
        setChecked(false);
    }
}

void OneEnumButton::blackboardValueChange()
{
    emit blackboardValueChange(m_StartValue);
}

void OneEnumButton::paintEvent(QPaintEvent *paint)
{
     QPushButton::paintEvent(paint);
     QPainter p(this);
     p.save();
     int xdim = this->width();
     int ydim = this->height();
     p.drawText(QRect(20, 0, xdim - 20, ydim), Qt::AlignLeft | Qt::AlignVCenter, buttonLabel);
     p.setBrush(buttonColor);
     p.drawEllipse(QPoint(10,ydim/2),6,6);
     p.restore();
}

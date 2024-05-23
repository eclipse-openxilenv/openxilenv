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


#include "OscilloscopeLineWidthWidget.h"
#include <QPainter>

OscilloscopeLineWidthWidget::OscilloscopeLineWidthWidget(QWidget *parent) : QWidget(parent)
{
    m_LineWidth = 1;
    this->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
}

OscilloscopeLineWidthWidget::~OscilloscopeLineWidthWidget()
{

}

void OscilloscopeLineWidthWidget::SetLineWidthAndColor(int par_LineWidth, QColor &par_Color)
{
    m_LineWidth = par_LineWidth;
    m_Color = par_Color;
    update();
}

QSize OscilloscopeLineWidthWidget::sizeHint() const
{
    return QSize (40, 20);
}

void OscilloscopeLineWidthWidget::setLineWidth(int arg_lineWidth)
{
    m_LineWidth = arg_lineWidth;
    update();
}

void OscilloscopeLineWidthWidget::setLineColor(QColor arg_lineColor)
{
    m_Color = arg_lineColor;
    update();
}

void OscilloscopeLineWidthWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QPen Pen;
    Pen.setWidth (m_LineWidth);
    Pen.setColor (m_Color);
    painter.setPen (Pen);
    int w = width();
    int y = height() / 2;
    painter.drawLine (3, y, w - 6, y);
}


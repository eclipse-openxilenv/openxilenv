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


#include "LampsGraphicsView.h"

CustomLampsGraphicsView::CustomLampsGraphicsView(QWidget* parent) : QWidget(parent)
{
    m_newFlagShouldBeRemovedByCancel = false;

    m_foregroundColor = QColor(255,0,0);
    m_backgroundColor = QColor(0,0,0);

    m_fromRow = 0;
    m_fromColumn = 0;
    m_columnSpan = 1;
    m_rowSpan = 1;

    setAutoFillBackground(true);
}

CustomLampsGraphicsView::~CustomLampsGraphicsView()
{
}

void CustomLampsGraphicsView::setGraphicsItemGroup(QList<QPolygonF> *arg_foregroundPolygons, QList<QPolygonF> *arg_backgroundPolygons)
{
    if ((arg_foregroundPolygons != nullptr) && (arg_backgroundPolygons != nullptr)) {
        m_foregroundItems = *arg_foregroundPolygons;  // make a copy
        m_backgroundItems = *arg_backgroundPolygons;
    } else {
        m_foregroundItems.clear();
        m_backgroundItems.clear();
    }
    resizeItems(this->width(), this->height());
    update();
}

void CustomLampsGraphicsView::clearGraphicsItemGroup()
{
    m_foregroundItems.clear();
    m_backgroundItems.clear();
    m_scaledForegroundItems.clear();
    m_scaledBackgroundItems.clear();
}

void CustomLampsGraphicsView::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e)
    resizeItems(this->width(), this->height());
}

void CustomLampsGraphicsView::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    emit openDialog();
}

void CustomLampsGraphicsView::contextMenuEvent(QContextMenuEvent *e)
{
    emit customContextMenuRequested(mapToParent(e->pos()));
}

void CustomLampsGraphicsView::resizeItems(int arg_width, int arg_height)
{
    double x_f = static_cast<double>(arg_width);
    double y_f = static_cast<double>(arg_height);
    m_scaledForegroundItems.clear();
    foreach (QPolygonF loc_polygon, m_foregroundItems) {
        QPolygon loc_scaledPolygon;
        foreach (QPointF loc_pointF, loc_polygon) {
            QPoint loc_point(static_cast<int>(loc_pointF.rx() * x_f), static_cast<int>((1.0 - loc_pointF.ry()) * y_f));
            loc_scaledPolygon.append(loc_point);
        }
        m_scaledForegroundItems.append(loc_scaledPolygon);
    }
    m_scaledBackgroundItems.clear();
    foreach (QPolygonF loc_polygon, m_backgroundItems) {
        QPolygon loc_scaledPolygon;
        foreach (QPointF loc_pointF, loc_polygon) {
            QPoint loc_point(static_cast<int>(loc_pointF.rx() * x_f), static_cast<int>((1.0 - loc_pointF.ry()) * y_f));
            loc_scaledPolygon.append(loc_point);
        }
        m_scaledBackgroundItems.append(loc_scaledPolygon);
    }
}

void CustomLampsGraphicsView::updateForegroundColor(QColor arg_color)
{
    m_foregroundColor = arg_color;
    update();
}

void CustomLampsGraphicsView::updateBackgroundColor(QColor arg_color)
{
    QPalette loc_pal;
    loc_pal.setColor(QPalette::Window, arg_color);
    setPalette(loc_pal);
    m_backgroundColor = arg_color;
    update();
}

QColor CustomLampsGraphicsView::backgroundColor()
{
    return m_backgroundColor;
}

void CustomLampsGraphicsView::setStancilName(QString arg_stancilName)
{
    m_stancilName = arg_stancilName;
}

QString CustomLampsGraphicsView::stancilName() {
    return m_stancilName;
}

void CustomLampsGraphicsView::setToolTip(QString arg_variableName)
{
    QWidget::setToolTip(arg_variableName);
}

QString CustomLampsGraphicsView::variableName()
{
    return m_variableName;
}

void CustomLampsGraphicsView::setVarialeName(QString arg_variableName)
{
    m_variableName = arg_variableName;
    setToolTip (m_variableName);
}

void CustomLampsGraphicsView::setPositionAndSize(int arg_fromRow, int arg_fromColumn, int arg_rowSpan, int arg_columnSpan)
{
    m_fromRow = arg_fromRow;
    m_fromColumn = arg_fromColumn;
    m_columnSpan = arg_columnSpan;
    m_rowSpan = arg_rowSpan;

    emit ResetLayout();
}


static QColor BetweenTwoColors (QColor arg_color1, QColor arg_color2, double arg_percentage)
{
    double a1 = arg_color1.alphaF();
    double r1 = arg_color1.redF();
    double g1 = arg_color1.greenF();
    double b1 = arg_color1.blueF();
    double a2 = arg_color2.alphaF();
    double r2 = arg_color2.redF();
    double g2 = arg_color2.greenF();
    double b2 = arg_color2.blueF();

    double a3 = (a1 + (a2 - a1) * arg_percentage);
    double r3 = (r1 + (r2 - r1) * arg_percentage);
    double g3 = (g1 + (g2 - g1) * arg_percentage);
    double b3 = (b1 + (b2 - b1) * arg_percentage);
    QColor loc_ret;
    loc_ret.setRgbF(r3, g3, b3, a3);
    return loc_ret;
}


#define COLOR_LIMIT_VALUE  10
QColor CustomLampsGraphicsView::GetMixedColor(QColor c1, QColor c2)
{
    return BetweenTwoColors(c1, c2, 0.5);
}

void CustomLampsGraphicsView::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    int w = width();
    int h = height();
    QPainter loc_painter(this);

    QColor loc_mixed_color = GetMixedColor(m_foregroundColor, m_backgroundColor);
    QPen loc_pen;
    loc_pen.setColor(loc_mixed_color);
    loc_pen.setWidth((w + h) / 100);
    loc_painter.setPen(loc_pen);

    loc_painter.drawRect(0, 0, w-1, h-1);

    QBrush foregroundBrush = QBrush (m_foregroundColor);
    loc_painter.setBrush (foregroundBrush);
    foreach (QPolygon loc_polygon, m_scaledForegroundItems) {
        loc_painter.drawPolygon(loc_polygon);
    }
    QBrush backgroundBrush = QBrush (m_backgroundColor);
    loc_painter.setBrush (backgroundBrush);
    foreach (QPolygon loc_polygon, m_scaledBackgroundItems) {
        loc_painter.drawPolygon(loc_polygon);
    }
}

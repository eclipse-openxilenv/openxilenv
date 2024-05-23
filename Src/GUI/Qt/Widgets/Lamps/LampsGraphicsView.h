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


#ifndef CUSTOMLAMPSGRAPHICSVIEW_H
#define CUSTOMLAMPSGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsItemGroup>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QGraphicsScene>
#include <QList>
#include <QGraphicsPolygonItem>
#include <QGridLayout>

class CustomLampsGraphicsView : public QWidget
{
    Q_OBJECT
public:
    CustomLampsGraphicsView(QWidget *parent = nullptr);
    ~CustomLampsGraphicsView();
    void setGraphicsItemGroup(QList<QPolygonF> *arg_foregroundPolygons, QList<QPolygonF> *arg_backgroundPolygons);
    void clearGraphicsItemGroup();
    void updateForegroundColor(QColor arg_color);
    void updateBackgroundColor(QColor arg_color);
    QColor backgroundColor();
    void setStancilName(QString arg_stancilName);
    QString stancilName();
    void setToolTip(QString arg_variableName);
    QString variableName();
    void setVarialeName(QString arg_variableName);
    void setPositionAndSize(int arg_fromRow, int arg_fromColumn, int arg_rowSpan, int arg_columnSpan);
    int getFromColumn() { return m_fromColumn; }
    int getFromRow() { return m_fromRow; }
    int getColumnSpan() { return m_columnSpan; }
    int getRowSpan() { return m_rowSpan; }

    static QColor GetMixedColor(QColor c1, QColor c2);

    void setShouldBeRemovedByCancel() { m_newFlagShouldBeRemovedByCancel = true; }
    void resetShouldBeRemovedByCancel() { m_newFlagShouldBeRemovedByCancel = false; }
    bool shouldBeRemovedByCancel() { return m_newFlagShouldBeRemovedByCancel; }

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void contextMenuEvent(QContextMenuEvent *e);

signals:
    void openDialog();
    void ResetLayout();

private:
    void resizeItems(int arg_width, int arg_height);
    QString m_variableName;
    QList<QPolygonF> m_foregroundItems;
    QList<QPolygonF> m_backgroundItems;
    QList<QPolygon> m_scaledForegroundItems;
    QList<QPolygon> m_scaledBackgroundItems;
    QString m_stancilName;
    QColor m_foregroundColor;
    QColor m_backgroundColor;
    int m_fromRow;
    int m_fromColumn;
    int m_columnSpan;
    int m_rowSpan;

    bool m_newFlagShouldBeRemovedByCancel;
};

#endif // CUSTOMLAMPSGRAPHICSVIEW_H

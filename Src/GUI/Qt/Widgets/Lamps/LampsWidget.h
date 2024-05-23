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


#ifndef LAMPSWIDGET_H
#define LAMPSWIDGET_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsPolygonItem>
#include <QPolygonF>
#include <QGridLayout>
#include <QList>
#include <QMenu>

#include "LampsGraphicsView.h"
#include "MdiWindowWidget.h"
#include "LampsDialogFrame.h"

class LampsWidget : public MdiWindowWidget
{
    Q_OBJECT
    Q_INTERFACES(InterfaceWidgetElement)
public:
    explicit LampsWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
    ~LampsWidget() Q_DECL_OVERRIDE;
    void setWindowName(QString name);
    int getControlLampsCount();
    void setSelectedLamp(CustomLampsGraphicsView* lamp);
    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

protected:
    void mouseDoubleClickEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *arg_event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *arg_event) Q_DECL_OVERRIDE;

private:
    void customStancilFromIni(ControlLampModel *arg_model);

signals:
    void removeTopHintFlagControlPanel();
    void setTopHintFlagControlPanel();

public slots:
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;
    void openContextMenu(QPoint p);
    void OwnOpenDialog(int arg_LampNumber);
    void openDialog() Q_DECL_OVERRIDE;
    void deleteLamp(int arg_LampNumber);
    void hoveredLamp(int arg_LampNumber);
    void addNewLamp();
    void deleteExistingLamp();
    void connectNewViewToSlots(CustomLampsGraphicsView* arg_newView);
    void disconnectViewFromSlots(CustomLampsGraphicsView* arg_view);
    void backgroundColorChange(QColor arg_color);
    virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
    virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;

    void ResetLayout();
    void cancelNewLamp(CustomLampsGraphicsView* arg_newLamp);

    void ImExportAllLamps();

private:
    CustomLampsGraphicsView *m_selectedLamp;
    QList<CustomLampsGraphicsView*> m_allLamps;
    QGridLayout *m_layout;
    QColor m_sceneBackgroundColor;
};

#endif // LAMPSWIDGET_H

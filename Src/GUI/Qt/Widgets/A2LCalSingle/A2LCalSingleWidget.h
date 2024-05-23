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


#ifndef A2LCALSINGLEWIDGET_H
#define A2LCALSINGLEWIDGET_H

#include "MdiWindowWidget.h"
#include "A2LCalSingleTableView.h"
#include <QSortFilterProxyModel>
#include "A2LCalSingleModel.h"
#include "TextValueInput.h"
#include "A2LCalSingleDelegateEditor.h"
#include <QWidgetAction>
#include <QRadioButton>
#include <QButtonGroup>

class A2LCalSingleWidget : public MdiWindowWidget {
    Q_OBJECT
public:
    explicit A2LCalSingleWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *par_parent = nullptr) ;
    ~A2LCalSingleWidget() Q_DECL_OVERRIDE;

    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

    int NotifiyStartStopProcess (QString par_ProcessName, int par_Flags, int par_Action);
    void NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data);
    int NotifiyGetDataFromLinkAck(void* par_IndexData, int par_FetchDataChannelNo);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    //void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;

private:
    void AddCharacteristicToModel(QString arg_variableName, int arg_Row = -1, int arg_displayType = -1);

private slots:
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;
    void ShowUnitColumn();
    void ShowDisplayTypeColumn();
    void HideUnitColumn();
    void HideDisplayTypeColumn();
    void CharacteristicProperties();
    void customContextMenu(QPoint arg_point);
    void changeFont(QFont arg_newFont) Q_DECL_OVERRIDE;
    void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;
    void ConfigSlot();
    void UpdateSlot();

private:
    void ConfigDialog();

    int m_LinkNo;

    int m_icon;
    A2LCalSingleTableView *m_tableViewVariables;
    A2LCalSingleDelegateEditor *m_DelegateEditor;
    A2LCalSingleModel *m_Model;
    QPoint m_startDragPosition;
    QColor m_BackgroundColor;

    QAction *m_ConfigAct;
    QAction *m_UpdateAct;
    QAction *m_ShowUnitColumnAct;
    QAction *m_ShowDisplayTypeColumnAct;
    QAction *m_HideUnitColumnAct;
    QAction *m_HideDisplayTypeColumnAct;
    QAction *m_CharacteristicPropertiesAct;

    bool m_ShowUnitColumn;
    bool m_ShowDisplayTypeColumn;

    QModelIndex m_CurrentSelectedIndex;
};

#endif // A2LCALSINGLEWIDGET_H

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


#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H

#include "MdiWindowWidget.h"
#include "TextTableView.h"
#include <QSortFilterProxyModel>
#include "TextTableModel.h"
#include "TextValueInput.h"
#include "BlackboardObserver.h"
#include <QWidgetAction>
#include <QRadioButton>
#include <QButtonGroup>

class TextWidget : public MdiWindowWidget {
    Q_OBJECT
public:
    explicit TextWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *par_parent = nullptr) ;
    ~TextWidget() Q_DECL_OVERRIDE;

    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    void addBlackboardVariableToModel(QString arg_variableName, int arg_Row = -1, int arg_displayType = -1);
    enum WhatType {SAVE_THIS_TO_SCRIPT, SAVE_ALL_TO_SCRIPT, SAVE_THIS_TO_SCRIPT_ATOMIC, SAVE_ALL_TO_SCRIPT_ATOMIC,
                   SAVE_THIS_TO_EQU, SAVE_ALL_TO_EQU, SAVE_THIS_TO_TEMP_BUFFER, SAVE_ALL_TO_TEMP_BUFFER};
    void SaveToFile(enum WhatType par_Type);
    void SaveToFile(enum WhatType par_Type, QTextStream &Stream);

private slots:
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;
    void blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag);
    void openDialog() Q_DECL_OVERRIDE;
    void ShowUnitColumn();
    void ShowDisplayTypeColumn();
    void HideUnitColumn();
    void HideDisplayTypeColumn();
    void BlackboardInfos();
    void SaveThisToScriptAct();
    void SaveAllToScriptAct();
    void SaveThisToScriptAtomicAct();
    void SaveAllToScriptAtomicAct();
    void SaveThisToEquAct();
    void SaveAllToEquAct();
    void SaveThisToSnapshotAct();
    void SaveAllToSnapshotAct();
    void LoadFromEquAct();
    void LoadFromSnapshotAct();
    void customContextMenu(QPoint arg_point);
    void changeFont(QFont arg_newFont) Q_DECL_OVERRIDE;
    void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;

private:
    int m_icon;
    TextTableView *m_tableViewVariables;
    TextTableModel *m_dataModel;
    QPoint m_startDragPosition;
    BlackboardObserverConnection m_ObserverConnection;
    QColor m_BackgroundColor;

    QAction *m_ConfigAct;
    QAction *m_ShowUnitColumnAct;
    QAction *m_ShowDisplayTypeColumnAct;
    QAction *m_HideUnitColumnAct;
    QAction *m_HideDisplayTypeColumnAct;
    QAction *m_BlackboardInfosAct;
    QAction *m_SaveThisToScriptAct;
    QAction *m_SaveAllToScriptAct;
    QAction *m_SaveThisToScriptAtomicAct;
    QAction *m_SaveAllToScriptAtomicAct;
    QAction *m_SaveThisToEquAct;
    QAction *m_SaveAllToEquAct;
    QAction *m_SaveThisToSnapshotAct;
    QAction *m_SaveAllToSnapshotAct;
    QAction *m_LoadFromEquAct;
    QAction *m_LoadFromSnapshotAct;

    bool m_ShowUnitColumn;
    bool m_ShowDisplayTypeColumn;

    QString m_SnapshotBuffer;
};

#endif // TEXTWIDGET_H

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


#ifndef CALLIBRATIONTREEVIEW_H
#define CALLIBRATIONTREEVIEW_H

#include <QWidget>
#include <QTreeView>
#include <QAction>
#include "CalibrationTreeDelegateEditor.h"

extern "C" {
    #include "DebugInfos.h"

    struct PROCESS_DEBUG_DATA_STRUCT;
}

class CalibrationTreeWidget;

class CalibrationTreeView : public QTreeView
{
    Q_OBJECT

public:
    CalibrationTreeView(QWidget *parent, CalibrationTreeWidget *par_CalibrationTreeWidget = nullptr);
    ~CalibrationTreeView() Q_DECL_OVERRIDE;

    QModelIndexList GetSelectedIndexes();

    void FitColumnSizes (void);

    QString GetSelectedLabel ();
    QList<QString>GetSelectedLabels ();

    void UpdateVisableValue();

protected:
    void contextMenuEvent (QContextMenuEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
private:
    void createActions (void);

    CalibrationTreeWidget *m_CalibrationTreeWidget;

    QAction *m_ConfigDialogAct;
    QAction *m_ReloadValuesAct;
    QAction *m_ReferenceAct;
    QAction *m_ReferenceUserNameAct;
    QAction *m_DeReferenceAct;
    QAction *m_ChangeAllSelectedValuesAct;
    QAction *m_ListAllReferencesAct;

    CalibrationTreeDelegateEditor *m_DelegateEditor;

};

#endif // CALLIBRATIONTREEVIEW_H

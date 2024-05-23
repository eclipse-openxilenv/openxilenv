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


#ifndef CANMESSAGETREEVIEW_H
#define CANMESSAGETREEVIEW_H

#include <QTreeView>
#include <QAction>

class CANMessageWindowWidget;

class CANMessageTreeView: public QTreeView
{
    Q_OBJECT

public:
    CANMessageTreeView(CANMessageWindowWidget *par_CANMessageWindowWidget, QWidget *parent = nullptr);
    ~CANMessageTreeView() Q_DECL_OVERRIDE;
    void FitColumnSizes (void);
    void GetColumnsWidth (int par_Column);

protected:
    void contextMenuEvent (QContextMenuEvent *event) Q_DECL_OVERRIDE;

private:
    void createActions (void);

    QAction *m_ConfigDialogAct;
    QAction *m_ClearAct;

    CANMessageWindowWidget *m_CANMessageWindowWidget;

};

#endif // CANMESSAGETREEVIEW_H

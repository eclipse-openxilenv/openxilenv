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


#ifndef INSERTCANERRORDIALOG_H
#define INSERTCANERRORDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QAction>
#include "CanConfigDialog.h"

namespace Ui {
class InsertCANErrorDialog;
}

class InsertCANErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InsertCANErrorDialog(QWidget *parent = nullptr);
    ~InsertCANErrorDialog();

private slots:
    void on_StartPushButton_clicked();

    void on_StopPushButton_clicked();

    void on_ClosePushButton_clicked();

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void CyclicTimeout();

    void CanTreeContextMenuRequestedSlot(const QPoint &pos);
    void SearchVariableSlot();
    void SelectSignalSlot(QModelIndex &SignalIndex);

    void on_StartPushButtonSize_clicked();

    void on_StopPushButtonSize_clicked();

    void on_StartPushButtonInterrupt_clicked();

    void on_StopPushButtonInterrupt_clicked();

private:
    bool GetAll();
    void FillHistory();
    void SaveHistory();

    Ui::InsertCANErrorDialog *ui;

    CanConfigBase *m_CanConfig;
    CanConfigTreeModel *m_CanConfigTreeModel;

    int m_CanBitErrorVid;

    int m_Channel;
    int m_Id;
    int m_StartBit;
    int m_BitSize;
    int m_ByteOrder;
    uint64_t m_BitValue;
    uint32_t m_Cycles;
    int m_Size;

    QTimer *m_Timer;
    QAction *m_SearchVariableAct;

};

#endif // INSERTCANERRORDIALOG_H

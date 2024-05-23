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


#ifndef CONFIGURESCHEDULERSDIALOG_H
#define CONFIGURESCHEDULERSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QListWidgetItem>
#include <Dialog.h>

extern "C" {
#include "Scheduler.h"
#include "SchedBarrier.h"
#include "IniDataBase.h"
}

namespace Ui {
class ConfigureSchedulersDialog;
}

class ConfigureSchedulersDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ConfigureSchedulersDialog(QWidget *parent = nullptr);
    ~ConfigureSchedulersDialog();

public slots:
    void accept();

private slots:
    void on_AddSchedulerPushButton_clicked();

    void on_DeleteSchedulerPushButton_clicked();

    void on_SchedulersListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_EditBarrierPushButton_clicked();

    void on_AddBeforeBarrierPushButton_clicked();

    void on_DeleteBeforeBarrierPushButton_clicked();

    void on_AddBeforeBarrierWaitPushButton_clicked();

    void on_DeleteBeforeBarrierWaitPushButton_clicked();

    void on_AddBehindBarrierPushButton_clicked();

    void on_DeleteBehindBarrierPushButton_clicked();

    void on_AddBehindBarrierWaitPushButton_clicked();

    void on_DeleteBehindBarrierWaitPushButton_clicked();

private:

    Ui::ConfigureSchedulersDialog *ui;

    class DialogData {
    public:
        DialogData()
        {
            m_SchedulerPrio = 0;
            m_Changed = 0;
        }
        QString m_SchedulerName;
        QString m_BarriersBeforeOnlySignal;
        QString m_BarriersBeforeSignalAndWait;
        QString m_BarriersBehindOnlySignal;
        QString m_BarriersBehindSignalAndWait;
        int m_SchedulerPrio;
        int m_Changed;
    };

    QMap <QString, struct DialogData> m_SchedulerInfos;
    QMap <QString, struct DialogData> m_SchedulerInfosSave;

    void SaveAll();
    void InsertAllBarriers();
    void ReadAllSchedulerInfos2Struct();
    struct DialogData ReadSchedulerInfosFromIni2Struct (QString &par_Scheduler);
    void SetSchedulerInfosFromStruct2Dialog (QString &par_SelectedSchduler);
    struct DialogData GetSchedulerInfosFromDialog2Struct(QString &par_Scheduler);
    int CheckChanges (QString &SelectedScheduler);
    bool StructContainsSchedulerName (QString par_SchedulerName);
    void StoreSchedulerInfosFromStruct2Ini (QString &par_SchedulerName);
    void DeleteSchedulerInfosFromIni (QString par_Scheduler);

    void UpdateSchedulerListView();
};

#endif // CONFIGURESCHEDULERSDIALOG_H

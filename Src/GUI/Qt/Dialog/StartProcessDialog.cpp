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


#include "StartProcessDialog.h"
#include "ui_StartProcessDialog.h"

#include "FileDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "InternalProcessList.h"
#include "Scheduler.h"
#include "MainValues.h"
#include "RemoteMasterScheduler.h"
#include "FileExtensions.h"
}

StartProcessDialog::StartProcessDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::StartProcessDialog)
{
    ui->setupUi(this);

    for (int i = 0; all_known_tasks[i] != nullptr; i++)  {
        if (get_pid_by_name (all_known_tasks[i]->name) == UNKNOWN_PROCESS) {
            ui->AvailableInternalProcessesListWidget->addItem(QString (all_known_tasks[i]->name));
        }
    }
    if (s_main_ini_val.ConnectToRemoteMaster) {
        char ProcessName[512];
        int Index = 0;
        while ((Index = rm_GetNextRealtimeProcess(Index, 0, ProcessName, sizeof (ProcessName))) > 0) {
            ui->AvailableInternalProcessesListWidget->addItem(QString (ProcessName));
        }
    }

    ui->StartPushButton->setEnabled(false);
}

StartProcessDialog::~StartProcessDialog()
{
    delete ui;
}

void StartProcessDialog::on_CancelPushButton_clicked()
{
    this->done(0);
}

void StartProcessDialog::on_StartExternalPRocessPushButton_clicked()
{
    QString ExecutableFile = FileDialog::getOpenFileName (this, QString ("Select file"),
                                                           QString(),
                                                           QString (EXTERN_PROC_EXT));
    if (!ExecutableFile.isEmpty()) {
        activate_extern_process (QStringToConstChar(ExecutableFile), -1, nullptr);
    }
    this->done(0);
}

void StartProcessDialog::on_StartPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableInternalProcessesListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString ProcessName = Item->data(Qt::DisplayRole).toString();
        start_process (QStringToConstChar(ProcessName));
    }
    this->done(0);
}

void StartProcessDialog::on_AvailableInternalProcessesListWidget_itemSelectionChanged()
{
    if (ui->AvailableInternalProcessesListWidget->selectedItems().size() >= 1) {
        ui->StartPushButton->setEnabled(true);
    }
}

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


#include "StopProcessDialog.h"
#include "ui_StopProcessDialog.h"

#include "StringHelpers.h"

extern "C" {
#include "Scheduler.h"
#include "MainValues.h"
#include "RemoteMasterScheduler.h"
}

StopProcessDialog::StopProcessDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::StopProcessDialog)
{
    ui->setupUi(this);

    // Add all running processes
    char *pName;
    char LongProcessName[MAX_PATH];
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((pName = read_next_process_name (Buffer)) != nullptr) {
        QString Name(pName);
        QListWidgetItem *Item = new QListWidgetItem(Name,ui->RunningProcessesListWidget);
        GetProcessLongName(get_pid_by_name (pName), LongProcessName);
        Item->setToolTip(QString(LongProcessName));
    }
    close_read_next_process_name(Buffer);
}

StopProcessDialog::~StopProcessDialog()
{
    delete ui;
}

void StopProcessDialog::accept()
{
    QList<QListWidgetItem*> SelectedItems = ui->RunningProcessesListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString ProcessName = Item->data(Qt::DisplayRole).toString();
        terminate_process (get_pid_by_name (QStringToConstChar(ProcessName)));
    }
    QDialog::accept();
}

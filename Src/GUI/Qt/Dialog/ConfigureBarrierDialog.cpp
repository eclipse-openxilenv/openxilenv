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


#include "ConfigureBarrierDialog.h"
#include "ui_ConfigureBarrierDialog.h"

#include "AddNewProcessSchedulerBarrier.h"
#include "StringHelpers.h"

extern "C" {
    #include "Scheduler.h"
    #include "SchedBarrier.h"
    #include "MyMemory.h"
    #include "ThrowError.h"
    #include "Files.h"
    #include "IniDataBase.h"
}

ConfigureBarrierDialog::ConfigureBarrierDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::ConfigureBarrierDialog)
{
    ui->setupUi(this);

    char Entry[64];
    char BarrierName[INI_MAX_LINE_LENGTH];
    for (int b = 0; ; b++) {
        sprintf (Entry, "Barrier_%i", b);
        if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", BarrierName, sizeof (BarrierName),
                                      GetMainFileDescriptor()) <= 0) {
            break;
        }
        ui->BarriersListWidget->addItem (CharToQString(BarrierName));
        m_BarriersSave.append(BarrierName);
    }
}

ConfigureBarrierDialog::~ConfigureBarrierDialog()
{
    delete ui;
}

void ConfigureBarrierDialog::on_AddBarrierPushButton_clicked()
{
    AddNewProcessSchedulerBarrier Dlg (QString ("barrier"), true);
    if (Dlg.exec() == QDialog::Accepted) {
        QString NewBarrier = Dlg.GetNew();
        if (!NewBarrier.isEmpty()) {
            if (ui->BarriersListWidget->findItems (NewBarrier, Qt::MatchFixedString).isEmpty()) {
                if (Dlg.GetWaitIfAlone()) NewBarrier.append(",WaitIfAlone");
                ui->BarriersListWidget->addItem (QString (NewBarrier));
            } else {
                ThrowError (1, "barrier name \"%s\" already used", QStringToConstChar(NewBarrier));
            }
        }
    }

}

void ConfigureBarrierDialog::on_DeleteBarrierPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BarriersListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureBarrierDialog::accept()
{
    // erst mal alle Barrieren loeschen die es nicht mehr gibt
    foreach (QString Item, m_BarriersSave) {
        if (ui->BarriersListWidget->findItems (Item, Qt::MatchFixedString).isEmpty()) {
            // ist nicht mehr in der Liste -> loeschen
            DeleteBarrierFromIni (QStringToConstChar(Item));
        }
    }
    // dann die neuen hinzufuegen
    QList<QListWidgetItem*> AllItems = ui->BarriersListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    foreach (QListWidgetItem* Item, AllItems) {
        QString NewBarrier = Item->data(Qt::DisplayRole).toString();
        bool Found = false;
        foreach (QString Item, m_BarriersSave) {
            if (!NewBarrier.compare (Item)) {
                Found = true;
            }
        }
        // Wenn nicht in der gespeicherrten Liste dann anlegen
        if (!Found) {
            AddNewBarrierToIni (QStringToConstChar(NewBarrier));
        }
    }
    QDialog::accept();
}


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


#include "WritesSctionToExeDialog.h"
#include "ui_WritesSctionToExeDialog.h"
#include "StringHelpers.h"

#include <QMessageBox>

extern "C"
{
    #include "StringMaxChar.h"
    #include "Scheduler.h"
    #include "DebugInfos.h"
    #include "ThrowError.h"
    #include "DebugInfoDB.h"
    #include "UniqueNumber.h"
}

writesectiontoexedialog::writesectiontoexedialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::writesectiontoexedialog)
{
    ui->setupUi(this);

    char *Name;
    bool ProcessRunning  = false;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((Name = read_next_process_name (Buffer)) != nullptr) {
        if (!IsInternalProcess (Name)) {
            ProcessRunning = true;
            ui->listWidgetProcesses->addItem(QString(Name));
        }
    }
    close_read_next_process_name(Buffer);

}

writesectiontoexedialog::~writesectiontoexedialog()
{
    delete ui;
}

void writesectiontoexedialog::accept()
{
    int x;
    char Section[256];
    int pid;
    char ProcessName[MAX_PATH];

    if(ui->listWidgetSelectedSection->count() == 0)
    {
        QMessageBox msgWarnBox;
        msgWarnBox.setIcon(QMessageBox::Warning);
        msgWarnBox.setText(QString("No section selected!"));
        msgWarnBox.setInformativeText("Please add a section!");
        msgWarnBox.exec();
        return;
    }
    STRING_COPY_TO_ARRAY(ProcessName, QStringToConstChar(ui->listWidgetProcesses->currentItem()->text()));

    pid = get_pid_by_name (ProcessName);
    if (pid <= 0) {
        ThrowError (1, "process \"%s\" not running", ProcessName);
        return;
    }
    for (x = 0; x < ui->listWidgetSelectedSection->count(); x++) {
        STRING_COPY_TO_ARRAY(Section, QStringToConstChar(ui->listWidgetSelectedSection->item(x)->text()));
        if (scm_write_section_to_exe (pid, Section)) {
            ThrowError (1, "cannot write section \"%s\" to \"%s\"", Section, ProcessName);
        }
    }
    QDialog::accept();
}

void writesectiontoexedialog::on_pushButtonAdd_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetAvailableSection->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString SectionName = Item->data(Qt::DisplayRole).toString();
        if (ui->listWidgetSelectedSection->findItems(SectionName, Qt::MatchFixedString).isEmpty()) {
            ui->listWidgetSelectedSection->addItem(SectionName);
        }
    }
}

void writesectiontoexedialog::on_pushButtonDel_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetSelectedSection->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void writesectiontoexedialog::on_listWidgetProcesses_itemClicked(QListWidgetItem *item)
{
    char ProcessName[MAX_PATH];
    STRING_COPY_TO_ARRAY(ProcessName, QStringToConstChar(item->text()));
    ui->listWidgetSelectedSection->clear();
    ui->listWidgetAvailableSection->clear();
    AddSectionToListBox(ProcessName);
}


void writesectiontoexedialog::AddSectionToListBox (char *ProcessName)
{
    PROCESS_APPL_DATA *pappldata;
    int x;

    if (ProcessName != nullptr)
    {
        int UniqueId;
        int Pid;
        UniqueId = AquireUniqueNumber();
        pappldata = ConnectToProcessDebugInfos (UniqueId,
                                                ProcessName, &Pid,
                                                nullptr, nullptr, nullptr,
                                                true);
        if (pappldata != nullptr)
        {
            DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
            DEBUG_INFOS_DATA *AssociatedDebugInfos;

            AssociatedProcess = pappldata->AssociatedProcess;
            if (AssociatedProcess != nullptr) {
                AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
                if (AssociatedDebugInfos != nullptr) {
                    if ((AssociatedDebugInfos->SectionSizeOfRawData != nullptr) ||
                        (AssociatedDebugInfos->SectionVirtualSize == nullptr) ||
                        (AssociatedDebugInfos->SectionVirtualAddress == nullptr) ||
                        (AssociatedDebugInfos->SectionNames == nullptr) ||
                        (AssociatedDebugInfos->NumOfSections > 0))
                    {
                        for (x = 0; x < AssociatedDebugInfos->NumOfSections; x++)
                        {
                            if (strcmp (AssociatedDebugInfos->SectionNames[x], ".data") != 0)
                            {
                                    ui->listWidgetAvailableSection->addItem(AssociatedDebugInfos->SectionNames[x]);
                            }

                        }
                    }
                }
            }
        }
        RemoveConnectFromProcessDebugInfos(UniqueId);
        FreeUniqueNumber(UniqueId);
    }
}

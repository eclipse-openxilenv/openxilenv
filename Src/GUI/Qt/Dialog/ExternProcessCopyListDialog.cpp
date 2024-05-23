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


#include "ExternProcessCopyListDialog.h"
#include "ui_ExternProcessCopyListDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "Scheduler.h"
#include "ScBbCopyLists.h"
#include "Blackboard.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "ExtProcessReferences.h"
#include "GetNextStructEntry.h"
#include "ExtProcessReferences.h"
#include "UniqueNumber.h"
#include "DebugInfoAccessExpression.h"
#include "Wildcards.h"
}

ExternProcessCopyListDialog::ExternProcessCopyListDialog(QString par_ProcessName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExternProcessCopyListDialog)
{
    ui->setupUi(this);
    // alles Prozesse durchlaufen
    char *Name;
    bool ProcessRunning  = false;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((Name = read_next_process_name (Buffer)) != nullptr) {
        if (!IsInternalProcess (Name)) {
            ui->ProcessComboBox->addItem (CharToQString(Name));
            if (!Compare2ProcessNames (Name, QStringToConstChar(par_ProcessName))) {
                ui->ProcessComboBox->setCurrentText(Name);
                ProcessRunning = true;
            }
        }
    }
    close_read_next_process_name(Buffer);

    ui->CopyListComboBox->addItem("BB <-> Pipe");   // 0
    ui->CopyListComboBox->addItem("BB -> Pipe");    // 1
    ui->CopyListComboBox->addItem("BB <- Pipe");    // 2

    ui->FilterLineEdit->setText("*");
    ui->AddressFromLineEdit->SetValue(static_cast<uint64_t>(0ULL), 16);
    ui->AddressToLineEdit->SetValue(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL), 16);
    ui->AddressFromLineEdit->setMinimumWidth(200);
    ui->AddressToLineEdit->setMinimumWidth(200);
    QStringList Horizontal;
    Horizontal << "Index" << "Name" << "Vid" << "Address" << "TypeBB" << "TypePipe";

    ui->tableWidget->setHorizontalHeaderLabels(Horizontal);
    if (ProcessRunning) {
        FillTable ();
    }
}

ExternProcessCopyListDialog::~ExternProcessCopyListDialog()
{
    delete ui;
}

void ExternProcessCopyListDialog::FillTable(QString par_ProcessName, int par_ListType, QString par_Filter, unsigned long long par_AddressFrom, unsigned long long par_AddressTo)
{
    ui->tableWidget->setRowCount(0);

    int Pid = get_pid_by_name(QStringToConstChar(par_ProcessName));
    if (Pid > 0) {
        unsigned long long *Addresses;
        int *Vids;
        short *TypeBB, *TypePipe;
        unsigned int SizeOf = 0;
        int Size = GetCopyListForProcess (Pid, par_ListType, &Addresses, &Vids, &TypeBB, &TypePipe);
        if (Size >= 0) {
            int Index = 0;
            for (int x = 0; x < Size; x++) {
                if ((Addresses[x] >= par_AddressFrom) && (Addresses[x] <= par_AddressTo)) {
                    char NameC[BBVARI_NAME_SIZE];
                    if (GetBlackboardVariableName(Vids[x], NameC, sizeof(NameC)) == 0) {
                        if (Compare2StringsWithWildcards(NameC, QStringToConstChar(par_Filter)) == 0) {
                            QString Name(NameC);
                            QTableWidgetItem *Item;
                            ui->tableWidget->insertRow(Index);
                            Item = new QTableWidgetItem(QString().number(x));
                            ui->tableWidget->setItem(Index, 0, Item);
                            Item = new QTableWidgetItem(Name);
                            ui->tableWidget->setItem(Index, 1, Item);
                            Item = new QTableWidgetItem(QString().number(Vids[x]));
                            ui->tableWidget->setItem(Index, 2, Item);
                            Item = new QTableWidgetItem(QString().number(Addresses[x], 16));
                            ui->tableWidget->setItem(Index, 3, Item);
                            Item = new QTableWidgetItem(QString(GetDataTypeName(TypeBB[x])));
                            ui->tableWidget->setItem(Index, 4, Item);
                            Item = new QTableWidgetItem(QString(GetDataTypeName(TypePipe[x])));
                            ui->tableWidget->setItem(Index, 5, Item);
                            SizeOf += GetDataTypeByteSize(TypePipe[x]);
                            Index++;
                        }
                    }
                }
            }
            my_free (Addresses);
            my_free (Vids);
            my_free (TypeBB);
            my_free (TypePipe);
        }
        ui->SizeOfLineEdit->setText(QString().number(SizeOf));
    }
}

void ExternProcessCopyListDialog::FillTable()
{
    unsigned long long par_AddressFrom = ui->AddressFromLineEdit->GetUInt64Value();
    unsigned long long par_AddressTo = ui->AddressToLineEdit->GetUInt64Value();
    int ListType = ui->CopyListComboBox->currentIndex();
    FillTable(ui->ProcessComboBox->currentText(), ListType, ui->FilterLineEdit->text(), par_AddressFrom, par_AddressTo);
}

void ExternProcessCopyListDialog::on_Filter1PushButton_clicked()
{
    FillTable();
}

void ExternProcessCopyListDialog::on_Filter2pushButton_clicked()
{
    FillTable();
}

void ExternProcessCopyListDialog::on_ProcessComboBox_currentIndexChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    FillTable();
}


void ExternProcessCopyListDialog::on_CopyListComboBox_currentIndexChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    FillTable();
}

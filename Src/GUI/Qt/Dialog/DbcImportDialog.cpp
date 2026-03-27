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


#include "DbcImportDialog.h"
#include "ui_DbcImportDialog.h"

#include "StringHelpers.h"
#include "QtIniFile.h"
#include "FileDialog.h"

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "CanDataBase.h"
#include "ImportDbc.h"
#include "FileExtensions.h"
}

DbcImportDialog::DbcImportDialog(QString &par_Filename, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DbcImportDialog)
{
    ui->setupUi(this);
    char *MemberNames, *p;

    ui->UserDefinedLineEdit->setText("new imported variante");
    ui->SameAsDbcFileNameRadioButton->setChecked(true);

    m_Filename = par_Filename;
    DbcImportDialogFillComboboxWithVariante();

    if (CANDB_GetAllBusMembers (QStringToConstChar(m_Filename), &MemberNames) >= 1) {
        p = MemberNames;
        while (*p != 0) {
            ui->AvailableBusMemberListWidget->addItem(QString(p));
            p = p + strlen (p) + 1;
        }
        CANDB_FreeMembersList (MemberNames);
    }
    on_TransferSettingsCheckBox_toggled(false);
}

DbcImportDialog::~DbcImportDialog()
{
    delete ui;
}

char *DbcImportDialog::GetTransmitBusMembersList()
{
    int Size = 2;
    char *Ret = static_cast<char*>(my_malloc (static_cast<size_t>(Size)));
    Ret[0] = Ret[1] = 0;
    int Pos = 0;
    for (int r = 0; r < ui->TramsmitBusMemberListWidget->count(); r++) {
        QString MemberName = ui->TramsmitBusMemberListWidget->item(r)->data(Qt::DisplayRole).toString();
        int Len = MemberName.size() + 1;
        Size += Len;
        Ret = static_cast<char*>(my_realloc (Ret, static_cast<size_t>(Size)));
        if (Ret == nullptr) return nullptr;
        StringCopyMaxCharTruncate (Ret + Pos, QStringToConstChar(MemberName), Size);
        Pos += Len;
    }
    Ret[Pos] = 0;  // terminating 0 char
    return  Ret;
}

char *DbcImportDialog::GetReceiveBusMembersList()
{
    int Size = 2;
    char *Ret = static_cast<char*>(my_malloc (static_cast<size_t>(Size)));
    Ret[0] = Ret[1] = 0;
    int Pos = 0;
    for (int r = 0; r < ui->ReceiveBusMemberListWidget->count(); r++) {
        QString MemberName = ui->ReceiveBusMemberListWidget->item(r)->data(Qt::DisplayRole).toString();
        int Len = MemberName.size() + 1;
        Size += Len;
        Ret = static_cast<char*>(my_realloc (Ret, static_cast<size_t>(Size)));
        if (Ret == nullptr) return nullptr;
        StringCopyMaxCharTruncate (Ret + Pos, QStringToConstChar(MemberName), Size);
        Pos += Len;
    }
    Ret[Pos] = 0;  // terminating 0 char
    return  Ret;
}

bool DbcImportDialog::ExtendSignalNameWithObject()
{
    return ui->ExtendSignalNameWithObjectCheckBox->isChecked();
}

QString DbcImportDialog::AdditionalSignalPrefix()
{
    if (ui->AdditionalSignalPrefixCheckBox->isChecked()) {
        return ui->AdditionalSignalPrefixLineEdit->text();
    } else {
        return QString();
    }
}

QString DbcImportDialog::AdditionalSignalPostfix()
{
    if (ui->AdditionalSignalPostfixCheckBox->isChecked()) {
        return ui->AdditionalSignalPostfixLineEdit->text();
    } else {
        return QString();
    }
}

int DbcImportDialog::TransferSettingsVarianteIndex()
{
    if (ui->TransferSettingsCheckBox->isChecked()) {
        return ui->TransferSettingsVarianteComboBox->currentData(Qt::UserRole).toInt();
    } else {
        return -1;
    }
}

bool DbcImportDialog::ObjectAdditionalEquations()
{
    return ui->ObjectAdditionalEquationsCheckBox->isChecked();
}

bool DbcImportDialog::ObjectInitData()
{
    return ui->ObjectInitDataCheckBox->isChecked();
}

bool DbcImportDialog::SignalInitValues()
{
    return ui->SignalInitValuesCheckBox->isChecked();
}

bool DbcImportDialog::SignalDatatType()
{
    return ui->SignalDatatTypeCheckBox->isChecked();
}

bool DbcImportDialog::SignalEquations()
{
    return ui->SignalEquationsCheckBox->isChecked();
}

bool DbcImportDialog::SortSignals()
{
    return ui->SortSignalsCheckBox->isChecked();
}

bool DbcImportDialog::ExportToFile()
{
    return ui->ExportToFileCheckBox->isChecked();
}

QString DbcImportDialog::ExportToFileName()
{
    return ui->ExportToFileLineEdit->text();
}

QString DbcImportDialog::VariantName()
{
    if (ui->UserDefinedRadioButton->isChecked()) {
        return ui->UserDefinedLineEdit->text();
    } else {
        int StartPos = m_Filename.lastIndexOf("/");
        if (StartPos < 0) {
            StartPos = m_Filename.lastIndexOf("\\");
        }
        int EndPos = m_Filename.lastIndexOf(".");
        if ((StartPos >= 0) && (EndPos >= 0)) {
            StartPos++;
            return m_Filename.mid(StartPos, EndPos - StartPos);
        } else {
            return QString("cannot extract name");
        }
    }
}

bool DbcImportDialog::ObjectNameDoublePoint()
{
    return ui->ObjectNameWithDoublepointCheckBox->isChecked();
}

int DbcImportDialog::DbcImportDialogFillComboboxWithVariante()
{
    int i, x;
    int Fd = ScQt_GetMainFileDescriptor();

    ui->TransferSettingsVarianteComboBox->clear();
    x = ScQt_IniFileDataBaseReadInt ("CAN/Global", "can_varianten_count", 0, Fd);
    for (i = 0; i < x; i++) {
        QString Section = QString("CAN/Variante_%1").arg(i);
        QString Line = ScQt_IniFileDataBaseReadString (Section, "name", "", Fd);
        QVariant Data(i);
        ui->TransferSettingsVarianteComboBox->addItem(Line, Data);
    }
    return x;
}

void DbcImportDialog::accept()
{
    char *TxMembers = GetTransmitBusMembersList();
    char *RxMembers = GetReceiveBusMembersList();
    CANDB_Import (QStringToConstChar(m_Filename),
                  (ExportToFile()) ? ExportToFileName().toLatin1().data() : NULL,
                  VariantName().toLatin1().data(),
                  TxMembers, RxMembers,
                  QStringToConstChar(AdditionalSignalPrefix()),
                  QStringToConstChar(AdditionalSignalPostfix()),
                  (TransferSettingsVarianteIndex() >= 0),
                  TransferSettingsVarianteIndex(),
                  ObjectAdditionalEquations(),
                  SortSignals(),
                  SignalDatatType(),
                  SignalEquations(),
                  SignalInitValues(),
                  ObjectInitData(),
                  ExtendSignalNameWithObject(),
                 ObjectNameDoublePoint());
    CANDB_FreeMembersList (TxMembers);
    CANDB_FreeMembersList (RxMembers);

    QDialog::accept();
}

void DbcImportDialog::on_AddTransmitBusMemberPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBusMemberListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BusMember = Item->data(Qt::DisplayRole).toString();
        if (ui->TramsmitBusMemberListWidget->findItems(BusMember, Qt::MatchFixedString).isEmpty()) {
            ui->TramsmitBusMemberListWidget->addItem(BusMember);
            delete Item;
        }
    }
}

void DbcImportDialog::on_RemoveTransmitBusMemberPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->TramsmitBusMemberListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BusMember = Item->data(Qt::DisplayRole).toString();
        if (ui->AvailableBusMemberListWidget->findItems(BusMember, Qt::MatchFixedString).isEmpty()) {
            ui->AvailableBusMemberListWidget->addItem(BusMember);
            delete Item;
        }
    }
}

void DbcImportDialog::on_AddReceivetBusMemberPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBusMemberListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BusMember = Item->data(Qt::DisplayRole).toString();
        if (ui->ReceiveBusMemberListWidget->findItems(BusMember, Qt::MatchFixedString).isEmpty()) {
            ui->ReceiveBusMemberListWidget->addItem(BusMember);
            delete Item;
        }
    }
}

void DbcImportDialog::on_RemoveReceiveBusMemberPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->ReceiveBusMemberListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BusMember = Item->data(Qt::DisplayRole).toString();
        if (ui->AvailableBusMemberListWidget->findItems(BusMember, Qt::MatchFixedString).isEmpty()) {
            ui->AvailableBusMemberListWidget->addItem(BusMember);
            delete Item;
        }
    }
}

void DbcImportDialog::on_TransferSettingsCheckBox_toggled(bool checked)
{
    ui->TransferSettingsVarianteComboBox->setEnabled(checked);
    ui->ObjectAdditionalEquationsCheckBox->setEnabled(checked);
    ui->ObjectInitDataCheckBox->setEnabled(checked);
    ui->SignalInitValuesCheckBox->setEnabled(checked);
    ui->SignalDatatTypeCheckBox->setEnabled(checked);
    ui->SignalEquationsCheckBox->setEnabled(checked);
    ui->SortSignalsCheckBox->setEnabled(checked);
}

void DbcImportDialog::on_FilePushButton_clicked()
{
    QString Filename = FileDialog::getSaveFileName(this,  QString("Export CAN variant to"), QString(), QString(CAN_EXT));
    if (!Filename.isEmpty()) {
        ui->ExportToFileLineEdit->setText(Filename);
    }
}


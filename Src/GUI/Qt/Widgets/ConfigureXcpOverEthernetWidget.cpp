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


#include "ConfigureXcpOverEthernetWidget.h"
#include "ui_ConfigureXcpOverEthernetWidget.h"

#include "FileDialog.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

extern "C"
{
    #include "MainValues.h"
    #include "MyMemory.h"
    #include "StringMaxChar.h"
    #include "ThrowError.h"
    #include "FileExtensions.h"
    #include "Files.h"
}

int ConfigureXCPOverEthernetWidget::Connection = 0;

ConfigureXCPOverEthernetWidget::ConfigureXCPOverEthernetWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConfigureXCPOverEthernetWidget)
{
    ui->setupUi(this);

    CurrentConnection = Connection;
    initData();

    on_checkBoxEnableConnection_clicked(ui->checkBoxEnableConnection->isChecked());

    Connection++;
}

ConfigureXCPOverEthernetWidget::~ConfigureXCPOverEthernetWidget()
{
    Connection--;
    delete ui;
}

void ConfigureXCPOverEthernetWidget::initData()
{
    int Len;
    int Fd = ScQt_GetMainFileDescriptor();

    QString Section = QString("XCP over Ethernet Configuration for Target %1").arg(Connection);
    ui->checkBoxEnableConnection->setChecked(ScQt_IniFileDataBaseReadYesNo(Section, "Enable", 0, Fd) != 0);
    QString Txt = ScQt_IniFileDataBaseReadString(Section, "Identification", "", Fd);
    ui->lineEditIdentification->setText(Txt);
    Txt = ScQt_IniFileDataBaseReadString(Section, "AssociatedProcessName", "", Fd);
    ui->lineEditAssociatedProcess->setText(Txt);
    Txt = ScQt_IniFileDataBaseReadString(Section, "CalibrationDataSegmentName", "", Fd);
    ui->lineEditDataSegmentName->setText(Txt);
    Txt = ScQt_IniFileDataBaseReadString(Section, "CodeSegmentName", "", Fd);
    ui->lineEditCodeSegmentName->setText(Txt);
    ui->lineEditPort->setText(QString("%1").arg(ScQt_IniFileDataBaseReadUInt(Section, "Port", 1802, Fd)));

    ui->checkBoxAddressTransationForDlls->setChecked(ScQt_IniFileDataBaseReadYesNo(Section, "AddressTranslationForProcessesInsideDll", 0, Fd));

    ui->checkBoxDebugFile->setChecked(ScQt_IniFileDataBaseReadYesNo(Section, "DebugFile", 0, Fd) != 0);
    Txt = ScQt_IniFileDataBaseReadString(Section, "DebugFileName", "", Fd);
    ui->lineEditDebugFile->setText(Txt);
    FillListBoxes();
}

void ConfigureXCPOverEthernetWidget::FillListBoxes()
{
    int Index;
    int Fd = ScQt_GetMainFileDescriptor();

    QString Section = QString("XCP over Ethernet Configuration for Target %1").arg(CurrentConnection);
    Index = 0;
    ui->listWidgetMeasurementTasks->clear();
    while (1) {
        QString Entry = QString("daq_%1").arg(Index);
        QString Txt = ScQt_IniFileDataBaseReadString (Section, Entry, "", Fd);
        if (Txt.isEmpty()) {
            break;
        }
        ui->listWidgetMeasurementTasks->addItem(Txt);
        Index++;
    }
}

void ConfigureXCPOverEthernetWidget::on_pushButtonCut_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetMeasurementTasks->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        tmpLabelName = Item->data(Qt::DisplayRole).toString();
        delete Item;
    }
}

void ConfigureXCPOverEthernetWidget::on_pushButtonCopy_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetMeasurementTasks->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        tmpLabelName = Item->data(Qt::DisplayRole).toString();
    }
}

void ConfigureXCPOverEthernetWidget::on_pushButtonInsertBefore_clicked()
{
    if (!tmpLabelName.isEmpty()) {
        ui->listWidgetMeasurementTasks->insertItem(ui->listWidgetMeasurementTasks->currentRow(), tmpLabelName);
    }
}

void ConfigureXCPOverEthernetWidget::on_pushButtonInsertBehind_clicked()
{
    if (!tmpLabelName.isEmpty()) {
        ui->listWidgetMeasurementTasks->insertItem(ui->listWidgetMeasurementTasks->currentRow()+1, tmpLabelName);
    }
}

void ConfigureXCPOverEthernetWidget::on_pushButtonImport_clicked()
{
    char txt[INI_MAX_LINE_LENGTH];
    FILE *fh;
    int Len;

    QString fileName = FileDialog::getOpenFileName(this, QString ("Import"), QString(), QString (XCP_EXT));

    if(!fileName.isEmpty()) {
        STRING_COPY_TO_ARRAY(txt, QStringToConstChar(fileName));
        if ((fh = open_file (txt, "rt")) == nullptr) {
            ThrowError (1, "cannot open file %s", txt);
            return;
        }
        while (fgets (txt, sizeof (txt), fh) != nullptr) {
            char *p;
            for (p = txt; (*p != 0) && (*p != '\n'); p++);
            *p = 0;
            ui->listWidgetMeasurementTasks->addItem(txt);
        }
    }
}

void ConfigureXCPOverEthernetWidget::on_pushButtonExport_clicked()
{
    char Txt[INI_MAX_LINE_LENGTH];
    FILE *fh;

    QString FileName = FileDialog::getSaveFileName(this, QString ("Save as"), QString(), QString (XCP_EXT));
    StringCopyMaxCharTruncate(Txt, QStringToConstChar(FileName), sizeof(Txt));
    if ((fh = open_file (Txt, "wt")) == nullptr) {
        ThrowError (1, "cannot open file %s", Txt);
        return;
    }

    for (int x = 0; x < ui->listWidgetMeasurementTasks->count(); x++) {
        STRING_APPEND_TO_ARRAY(Txt, QStringToConstChar(ui->listWidgetMeasurementTasks->item(x)->text()));
        fprintf(fh, "%s\n", Txt);
    }
    close_file (fh);
}

void ConfigureXCPOverEthernetWidget::on_pushButtonAddBefore_clicked()
{
    QString StringToAdd, Timestamp, Name, Cycle, Prio;
    Timestamp = QString("%1").arg(ui->comboBoxTimestamp->currentData(Qt::DisplayRole).toString());
    Name = QString("%1").arg(ui->lineEditName->text());
    Cycle = QString("%1").arg(ui->lineEditCycle->text());
    Prio = QString("%1").arg(ui->lineEditPrio->text());
    StringToAdd = QString("%1,%2,%3,%4").arg(Timestamp, Cycle, Prio, Name);
    ui->listWidgetMeasurementTasks->insertItem(ui->listWidgetMeasurementTasks->currentRow(), StringToAdd);
}

void ConfigureXCPOverEthernetWidget::on_pushButtonAddBehind_clicked()
{
    QString StringToAdd, Timestamp, Name, Cycle, Prio;
    Timestamp = QString("%1").arg(ui->comboBoxTimestamp->currentData(Qt::DisplayRole).toString());
    Name = QString("%1").arg(ui->lineEditName->text());
    Cycle = QString("%1").arg(ui->lineEditCycle->text());
    Prio = QString("%1").arg(ui->lineEditPrio->text());
    StringToAdd = QString("%1,%2,%3,%4").arg(Timestamp, Cycle, Prio, Name);
    ui->listWidgetMeasurementTasks->insertItem(ui->listWidgetMeasurementTasks->currentRow()+1, StringToAdd);
}

void ConfigureXCPOverEthernetWidget::on_pushButtonChange_clicked()
{
    if (ui->listWidgetMeasurementTasks->selectedItems().isEmpty()) {
        ThrowError (1, "Please select a label.");
        return;
    }
    QString StringToAdd, DataType, Name, Cycle, Prio;
    int curRow = ui->listWidgetMeasurementTasks->currentRow();
    DataType = QString("%1").arg(ui->comboBoxTimestamp->currentData(Qt::DisplayRole).toString());
    Name = QString("%1").arg(ui->lineEditName->text());
    Cycle = QString("%1").arg(ui->lineEditCycle->text());
    Prio = QString("%1").arg(ui->lineEditPrio->text());
    StringToAdd = QString("%1,%2,%3,%4").arg(DataType, Cycle, Prio, Name);
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetMeasurementTasks->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
    ui->listWidgetMeasurementTasks->insertItem(curRow, StringToAdd);
    ui->listWidgetMeasurementTasks->setCurrentItem(ui->listWidgetMeasurementTasks->currentItem(), QItemSelectionModel::Deselect);
}

void ConfigureXCPOverEthernetWidget::on_listWidgetMeasurementTasks_itemClicked(QListWidgetItem *item)
{
    QString itemString = item->data(Qt::DisplayRole).toString();
    QStringList tmpList = itemString.split(",");
    tmpList.count();
    ui->comboBoxTimestamp->setCurrentText(tmpList.at(0));
    ui->lineEditCycle->setText(tmpList.at(1));
    ui->lineEditPrio->setText(tmpList.at(2));
    ui->lineEditName->setText(tmpList.at(3));
}

void ConfigureXCPOverEthernetWidget::saveXCPConfigData()
{
    int x;
    int Fd = ScQt_GetMainFileDescriptor();

    QString Section = QString("XCP over Ethernet Configuration for Target %1").arg(CurrentConnection);
    if(ui->checkBoxEnableConnection->isChecked())
    {
        ScQt_IniFileDataBaseWriteYesNo(Section, "Enable", 1, Fd);
    } else {
        ScQt_IniFileDataBaseWriteYesNo(Section, "Enable", 0, Fd);
    }
    ScQt_IniFileDataBaseWriteString(Section, "Identification", QStringToConstChar(ui->lineEditIdentification->text()), Fd);
    ScQt_IniFileDataBaseWriteString(Section, "AssociatedProcessName", QStringToConstChar(ui->lineEditAssociatedProcess->text()), Fd);
    ScQt_IniFileDataBaseWriteString(Section, "CalibrationDataSegmentName", QStringToConstChar(ui->lineEditDataSegmentName->text()), Fd);
    ScQt_IniFileDataBaseWriteString(Section, "CodeSegmentName", QStringToConstChar(ui->lineEditCodeSegmentName->text()), Fd);
    ScQt_IniFileDataBaseWriteUInt(Section, "Port", ui->lineEditPort->text().toUInt(), Fd);

    ScQt_IniFileDataBaseWriteYesNo(Section, "AddressTranslationForProcessesInsideDll", ui->checkBoxAddressTransationForDlls->isChecked(), Fd);

    if(ui->checkBoxDebugFile->isChecked())
    {
        ScQt_IniFileDataBaseWriteYesNo(Section, "DebugFile", 1, Fd);
    } else {
        ScQt_IniFileDataBaseWriteYesNo(Section, "DebugFile", 0, Fd);
    }
    ScQt_IniFileDataBaseWriteString(Section, "DebugFileName", QStringToConstChar(ui->lineEditDebugFile->text()), Fd);


    for (x = 0; x < ui->listWidgetMeasurementTasks->count(); x++) {
        QString Entry = QString("daq_%1").arg(x);
        QString Line = ui->listWidgetMeasurementTasks->item(x)->text();
        ScQt_IniFileDataBaseWriteString (Section, Entry, Line, Fd);
    }
    for ( ; ; x++) {
        QString Entry = QString("daq_%1").arg(x);
        if (ScQt_IniFileDataBaseReadString(Section, Entry, "", Fd).isEmpty()) {
            break;
        }
        ScQt_IniFileDataBaseWriteString (Section, Entry, nullptr, Fd);
    }
}

void ConfigureXCPOverEthernetWidget::on_checkBoxEnableConnection_clicked(bool checked)
{
    ui->checkBoxDebugFile->setEnabled(checked);
    ui->lineEditIdentification->setEnabled(checked);
    ui->lineEditAssociatedProcess->setEnabled(checked);
    ui->lineEditDataSegmentName->setEnabled(checked);
    ui->lineEditCodeSegmentName->setEnabled(checked);
    ui->lineEditPort->setEnabled(checked);
    ui->checkBoxAddressTransationForDlls->setEnabled(checked);
    ui->lineEditDebugFile->setEnabled(checked);
    ui->listWidgetMeasurementTasks->setEnabled(checked);
    ui->pushButtonCut->setEnabled(checked);
    ui->pushButtonCopy->setEnabled(checked);
    ui->pushButtonInsertBefore->setEnabled(checked);
    ui->pushButtonInsertBehind->setEnabled(checked);
    ui->pushButtonImport->setEnabled(checked);
    ui->pushButtonExport->setEnabled(checked);
    ui->pushButtonAddBefore->setEnabled(checked);
    ui->pushButtonAddBehind->setEnabled(checked);
    ui->pushButtonChange->setEnabled(checked);
    ui->lineEditCycle->setEnabled(checked);
    ui->lineEditPrio->setEnabled(checked);
    ui->lineEditName->setEnabled(checked);
    ui->comboBoxTimestamp->setEnabled(checked);
}

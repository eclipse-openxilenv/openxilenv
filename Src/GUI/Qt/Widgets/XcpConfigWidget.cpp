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


#include <stdint.h>
#include "XcpConfigWidget.h"
#include "ui_XcpConfigWidget.h"

#include "StringHelpers.h"
#include "QtIniFile.h"

#include <QListWidget>

extern "C"
{
    #include "StringMaxChar.h"
    #include "Message.h"
    #include "Scheduler.h"
    #include "Blackboard.h"
    #include "BlackboardAccess.h"
    #include "Files.h"
    #include "ThrowError.h"
    #include "MyMemory.h"
    #include "LoadSaveToFile.h"
    #include "EquationParser.h"
    #include "CcpMessages.h"
    #include "CcpControl.h"
    #include "FileExtensions.h"
    #include "ReadCanCfg.h"
}

#include <QFileDialog>
#include "FileDialog.h"
#include "MainWinowSyncWithOtherThreads.h"

int XCPConfigWidget::Connection = 0;

XCPConfigWidget::XCPConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::XCPConfigWidget),
    xcpconfig_data(new XCPCFGDLG_SHEET_DATA)
{
    ui->setupUi(this);

    for (int c = 0; c < MAX_CAN_CHANNELS; c++) {
        ui->comboBoxChannel->addItem(QString().number(c));
    }
    ui->pushButtonInsertBefore->setEnabled(false);
    ui->pushButtonInsertBehind->setEnabled(false);

    connect(ui->checkBoxEnableCalibration, SIGNAL(toggled(bool)), ui->checkBoxMoveROMRAM, SLOT(setEnabled(bool)));
    connect(ui->checkBoxEnableCalibration, SIGNAL(toggled(bool)), ui->checkBoxSetCalibration, SLOT(setEnabled(bool)));
    connect(ui->checkBoxEnableCalibration, SIGNAL(toggled(bool)), ui->checkBoxReadParam, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMoveROMRAM, SIGNAL(toggled(bool)), ui->lineEditCalibrationROMSeg, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMoveROMRAM, SIGNAL(toggled(bool)), ui->lineEditCalibrationRAMSeg, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMoveROMRAM, SIGNAL(toggled(bool)), ui->lineEditCalibrationRAMpage, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMoveROMRAM, SIGNAL(toggled(bool)), ui->lineEditCalibrationROMpage, SLOT(setEnabled(bool)));
    connect(ui->checkBoxUnit, SIGNAL(toggled(bool)), ui->lineEditUnit, SLOT(setEnabled(bool)));
    connect(ui->checkBoxConversion, SIGNAL(toggled(bool)), ui->lineEditConversion, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMinMax, SIGNAL(toggled(bool)), ui->lineEditMaxValue, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMinMax, SIGNAL(toggled(bool)), ui->lineEditMinValue, SLOT(setEnabled(bool)));

    tmpCopyBuffer.clear();
    curConnection = Connection;

    initData();

    ui->comboBoxByteOrder->setCurrentText(xcpconfig_data->ByteOrderString);
    ui->comboBoxChannel->setCurrentIndex(xcpconfig_data->Channel);
    ui->comboBoxDebug->setCurrentIndex(xcpconfig_data->DebugFlag);
    ui->lineEditCRO_ID->setText(QString("0x%1").arg(xcpconfig_data->CRO_ID, 0 ,16));
    ui->lineEditCRM_ID->setText(QString("0x%1").arg(xcpconfig_data->CRM_ID, 0, 16));
    ui->lineEditDTO_ID->setText(QString("0x%1").arg(xcpconfig_data->DTO_ID, 0, 16));
    ui->lineEditCommandTimeout->setText(QString().number(xcpconfig_data->Timeout));
    ui->lineEditEventChannelNo->setText(QString().number(xcpconfig_data->EventChannel));
    ui->lineEditPrescaler->setText(QString().number(xcpconfig_data->Prescaler));
    ui->checkBoxExtIdentiefier->setChecked(xcpconfig_data->ExtIds != 0);
    ui->checkBoxEnableCalibration->setChecked(xcpconfig_data->CalibrationEnable != 0);
    ui->checkBoxMoveROMRAM->setChecked(xcpconfig_data->MoveROM2RAM != 0);
    ui->checkBoxSetCalibration->setChecked(xcpconfig_data->SelCalPage != 0);
    ui->checkBoxReadParam->setChecked(xcpconfig_data->ReadParameterAfterCalib != 0);
    ui->lineEditSeedAndKey->setText(xcpconfig_data->SeedKeyDll);
    ui->lineEditCalibrationROMSeg->setText(QString("0x%1").arg(xcpconfig_data->CalibROMSegment, 0, 16));
    ui->lineEditCalibrationROMpage->setText(QString("0x%1").arg(xcpconfig_data->CalibROMPageNo, 0, 16));
    ui->lineEditCalibrationRAMSeg->setText(QString("0x%1").arg(xcpconfig_data->CalibRAMSegment, 0, 16));
    ui->lineEditCalibrationRAMpage->setText(QString("0x%1").arg(xcpconfig_data->CalibRAMPageNo, 0, 16));
    ui->checkBoxUnit->setChecked(xcpconfig_data->UseUnit != 0);
    ui->checkBoxConversion->setChecked(xcpconfig_data->UseConversion != 0);
    ui->checkBoxMinMax->setChecked(xcpconfig_data->UseMinMax != 0);

    if(ui->checkBoxEnableCalibration->isChecked()) {
        ui->checkBoxMoveROMRAM->setEnabled(true);
        ui->checkBoxSetCalibration->setEnabled(true);
        ui->checkBoxReadParam->setEnabled(true);
        ui->lineEditCalibrationROMSeg->setEnabled(true);
        ui->lineEditCalibrationRAMSeg->setEnabled(true);
        ui->lineEditCalibrationRAMpage->setEnabled(true);
        ui->lineEditCalibrationROMpage->setEnabled(true);
    } else {
        ui->checkBoxMoveROMRAM->setEnabled(false);
        ui->checkBoxSetCalibration->setEnabled(false);
        ui->checkBoxReadParam->setEnabled(false);
        ui->lineEditCalibrationROMSeg->setEnabled(false);
        ui->lineEditCalibrationRAMSeg->setEnabled(false);
        ui->lineEditCalibrationRAMpage->setEnabled(false);
        ui->lineEditCalibrationROMpage->setEnabled(false);
    }

    if (ui->checkBoxConversion->isChecked()) {
        ui->lineEditConversion->setEnabled(true);
    } else {
        ui->lineEditConversion->setEnabled(false);
    }

    if (ui->checkBoxUnit->isChecked()) {
        ui->lineEditUnit->setEnabled(true);
    } else {
        ui->lineEditUnit->setEnabled(false);
    }

    if (ui->checkBoxMinMax->isChecked()) {
        ui->lineEditMaxValue->setEnabled(true);
        ui->lineEditMinValue->setEnabled(true);
    } else {
        ui->lineEditMaxValue->setEnabled(false);
        ui->lineEditMinValue->setEnabled(false);
    }

    Connection++;
}

XCPConfigWidget::~XCPConfigWidget()
{
    Connection--;
    delete xcpconfig_data;
    delete ui;
}

void XCPConfigWidget::initData()
{
    int Fd = ScQt_GetMainFileDescriptor();

    QString Section = QString("XCP Configuration for Target %1").arg(Connection);
    xcpconfig_data->CalOrMesListBoxVisible = 0;
    xcpconfig_data->DebugFlag = ScQt_IniFileDataBaseReadYesNo (Section, "DebugMessages", 0, Fd);
    xcpconfig_data->ByteOrderString =ScQt_IniFileDataBaseReadString (Section, "ByteOrder", "lsb_first", Fd);
    xcpconfig_data->Channel = ScQt_IniFileDataBaseReadInt (Section, "Channel", 0, Fd);
    xcpconfig_data->Timeout = ScQt_IniFileDataBaseReadInt (Section, "Timeout", 1000, Fd);
    xcpconfig_data->SeedKeyDll = ScQt_IniFileDataBaseReadString (Section, "SeedKeyDll", "", Fd);
    xcpconfig_data->SeedAndKeyFlag = ScQt_IniFileDataBaseReadInt (Section, "SeedKeyFlag", 0, Fd);
    xcpconfig_data->CRO_ID = ScQt_IniFileDataBaseReadUInt (Section, "CRO_ID", 0, Fd);
    xcpconfig_data->CRM_ID = ScQt_IniFileDataBaseReadUInt (Section, "CRM_ID", 0, Fd);
    xcpconfig_data->DTO_ID = ScQt_IniFileDataBaseReadUInt (Section, "DTO_ID", 0, Fd);
    xcpconfig_data->ExtIds = ScQt_IniFileDataBaseReadInt (Section, "ExtIds", 0, Fd);
    xcpconfig_data->Prescaler = ScQt_IniFileDataBaseReadInt (Section, "Prescaler", 1, Fd);
    xcpconfig_data->EventChannel = ScQt_IniFileDataBaseReadInt (Section, "EventChannel", 0, Fd);
    xcpconfig_data->OnlyBytePointerInDAQ = ScQt_IniFileDataBaseReadYesNo (Section, "OnlyBytePointerInDAQ", 0, Fd);
    xcpconfig_data->CalibrationEnable = ScQt_IniFileDataBaseReadYesNo (Section, "CalibrationEnable", 1, Fd);
    xcpconfig_data->MoveROM2RAM = ScQt_IniFileDataBaseReadYesNo (Section, "MoveROM2RAM", 1, Fd);
    xcpconfig_data->SelCalPage = ScQt_IniFileDataBaseReadYesNo (Section, "SelCalPage", 1, Fd);
    xcpconfig_data->CalibROMSegment = ScQt_IniFileDataBaseReadUInt (Section, "CalibROMSegment", 0x0, Fd);
    xcpconfig_data->CalibROMPageNo = ScQt_IniFileDataBaseReadUInt (Section, "CalibROMPageNo", 0x0, Fd);
    xcpconfig_data->CalibRAMSegment = ScQt_IniFileDataBaseReadUInt (Section, "CalibRAMSegment", 0x0, Fd);
    xcpconfig_data->CalibRAMPageNo = ScQt_IniFileDataBaseReadUInt (Section, "CalibRAMPageNo", 0x0, Fd);

    int Size = ScQt_IniFileDataBaseGetSectionNumberOfEntrys(Section, Fd);
    uint64_t uic = 0;
    uint32_t ui100 = 0;
    uint32_t ui100l = 0;
    int ProcBar;
    if (Size > 1000) {
        ProcBar = OpenProgressBarFromOtherThread ("load CCP");
    } else {
        ProcBar = -1;
    }

    for (xcpconfig_data->VarListCount = 0; ; xcpconfig_data->VarListCount++) {
        QString Entry = QString("v%1").arg(xcpconfig_data->VarListCount);
        QString Line = ScQt_IniFileDataBaseReadString (Section, Entry, "", Fd);
        if (Line.isEmpty()) {
            break;
        }
        ui->listWidgetOfMeasurements->addItem(Line);
        ui100 = (uint32_t)((uic * 100LLU) / (uint32_t)Size);
        uic++;
        if ((ProcBar >= 0) && (ui100 > ui100l)) SetProgressBarFromOtherThread(ProcBar, ui100);
    }
    xcpconfig_data->VarListCountOld = xcpconfig_data->VarListCount;

    for (xcpconfig_data->CalListCount = 0; ; xcpconfig_data->CalListCount++) {
        QString Entry = QString("p%1").arg(xcpconfig_data->CalListCount);
        QString Line = ScQt_IniFileDataBaseReadString (Section, Entry, "",Fd);
        if (Line.isEmpty()) {
            break;
        }
        ui->listWidgetOfCalibrations->addItem(Line);
        ui100 = (uint32_t)((uic * 100LLU) / (uint32_t)Size);
        uic++;
        if ((ProcBar >= 0) && (ui100 > ui100l)) SetProgressBarFromOtherThread(ProcBar, ui100);
    }
    xcpconfig_data->CalListCountOld = xcpconfig_data->CalListCount;
    if (ProcBar >= 0) CloseProgressBarFromOtherThread(ProcBar);

    xcpconfig_data->ReadParameterAfterCalib = ScQt_IniFileDataBaseReadInt (Section, "ReadParameterAfterCalib", 0, Fd);
    xcpconfig_data->UseUnit = ScQt_IniFileDataBaseReadYesNo (Section, "UseUnit", 0, Fd);
    xcpconfig_data->UseConversion = ScQt_IniFileDataBaseReadYesNo (Section, "UseConversion", 0, Fd);
    xcpconfig_data->UseMinMax = ScQt_IniFileDataBaseReadYesNo (Section, "UseMinMax", 0, Fd);

    xcpconfig_data->LoadedFlag = 1;
}

void XCPConfigWidget::on_pushButtonChange_clicked()
{
    QListWidget *ListWidget;

    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }

    if (ListWidget->selectedItems().isEmpty()) {
        ThrowError (1, "Please select a label.");
        return;
    }
    QString Line = BuildLine();

    ListWidget->currentItem()->setText(Line);
}

void XCPConfigWidget::on_pushButtonAddBehind_clicked()
{
    QListWidget *ListWidget;

    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }

    int curRow = ListWidget->currentRow();

    QString Line = BuildLine();

    ListWidget->insertItem(curRow + 1, Line);
    ListWidget->setCurrentItem(ListWidget->currentItem(), QItemSelectionModel::Deselect);
}

void XCPConfigWidget::on_pushButtonAddBefore_clicked()
{
    QListWidget *ListWidget;

    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }

    int curRow = ListWidget->currentRow();

    QString Line = BuildLine();

    ListWidget->insertItem(curRow, Line);
    ListWidget->setCurrentItem(ListWidget->currentItem(), QItemSelectionModel::Deselect);
}

void XCPConfigWidget::on_pushButtonExport_clicked()
{
    QListWidget *ListWidget;
    char txt[INI_MAX_LINE_LENGTH];
    FILE *fh;

    QString fileName = FileDialog::getSaveFileName(this, QString ("Save as"), QString(), QString (XCP_EXT));
    txt[0] = 0;
    STRING_COPY_TO_ARRAY(txt, QStringToConstChar(fileName));
    if ((fh = open_file (txt, "wt")) == nullptr) {
        ThrowError (1, "cannot open file %s", txt);
        return;
    }
    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }
    for (int x = 0; x < ListWidget->count(); x++) {
        STRING_COPY_TO_ARRAY(txt, QStringToConstChar(ListWidget->item(x)->text()));
        fprintf (fh, "%s\n", txt);
    }

    close_file (fh);
}

void XCPConfigWidget::on_pushButtonImport_clicked()
{
    char txt[INI_MAX_LINE_LENGTH];
    FILE *fh;

    QString fileName = FileDialog::getOpenFileName(this, "import xcp file", QString(), XCP_EXT);

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
            switch (ui->tabWidget->currentIndex()) {
            case 0:  // Measurement
                ui->listWidgetOfMeasurements->addItem(txt);
                break;
            case 1:  // Calibration
                ui->listWidgetOfCalibrations->addItem(txt);
                break;
            }
        }
        close_file (fh);
    }
}

void XCPConfigWidget::on_pushButtonInsertBehind_clicked()
{
    QListWidget *ListWidget;
    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }
    int Row = ListWidget->currentRow() + 1;
    foreach (QString Line, tmpCopyBuffer) {
        ListWidget->insertItem(Row, Line);
    }
}

void XCPConfigWidget::on_pushButtonInsertBefore_clicked()
{
    QListWidget *ListWidget;
    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }
    int Row = ListWidget->currentRow();
    foreach (QString Line, tmpCopyBuffer) {
        ListWidget->insertItem(Row, Line);
    }
}

void XCPConfigWidget::on_pushButtonCopy_clicked()
{
    QModelIndexList selectedIndexes;
    QListWidget *ListWidget;
    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }
    selectedIndexes = ListWidget->selectionModel()->selectedIndexes();
    std::sort(selectedIndexes.begin(),selectedIndexes.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
    tmpCopyBuffer.clear();
    foreach (QModelIndex Index, selectedIndexes) {
        tmpCopyBuffer.append(Index.data(Qt::DisplayRole).toString());
    }
    ui->pushButtonInsertBefore->setEnabled(true);
    ui->pushButtonInsertBehind->setEnabled(true);
}

void XCPConfigWidget::on_pushButtonCut_clicked()
{
    QModelIndexList selectedIndexes;
    QListWidget *ListWidget;
    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }
    selectedIndexes = ListWidget->selectionModel()->selectedIndexes();
    std::sort(selectedIndexes.begin(),selectedIndexes.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
    tmpCopyBuffer.clear();
    foreach (QModelIndex Index, selectedIndexes) {
        tmpCopyBuffer.append(Index.data(Qt::DisplayRole).toString());
        ListWidget->model()->removeRow(Index.row()); // remove each row
    }
    ui->pushButtonInsertBefore->setEnabled(true);
    ui->pushButtonInsertBehind->setEnabled(true);
}

void XCPConfigWidget::saveXCPConfigData()
{
    int x;
    int Fd = ScQt_GetMainFileDescriptor();

    QString Section = QString("XCP Configuration for Target %1").arg(curConnection);
    xcpconfig_data->DebugFlag = !ui->comboBoxDebug->currentText().compare("Yes", Qt::CaseInsensitive);
    ScQt_IniFileDataBaseWriteYesNo (Section, "DebugMessages", xcpconfig_data->DebugFlag, Fd);
    xcpconfig_data->ByteOrderString = ui->comboBoxByteOrder->currentText();
    ScQt_IniFileDataBaseWriteString (Section, "ByteOrder", xcpconfig_data->ByteOrderString, Fd);
    xcpconfig_data->Channel = ui->comboBoxChannel->currentIndex();
    ScQt_IniFileDataBaseWriteInt (Section, "Channel", xcpconfig_data->Channel, Fd);
    xcpconfig_data->Timeout = ui->lineEditCommandTimeout->text().toInt();
    ScQt_IniFileDataBaseWriteInt (Section, "Timeout", xcpconfig_data->Timeout, Fd);
    xcpconfig_data->SeedKeyDll = ui->lineEditSeedAndKey->text();
    ScQt_IniFileDataBaseWriteString (Section, "SeedKeyDll", xcpconfig_data->SeedKeyDll, Fd);
    xcpconfig_data->CRO_ID = ui->lineEditCRO_ID->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CRO_ID", xcpconfig_data->CRO_ID, Fd);
    xcpconfig_data->CRM_ID = ui->lineEditCRM_ID->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CRM_ID", xcpconfig_data->CRM_ID, Fd);
    xcpconfig_data->DTO_ID = ui->lineEditDTO_ID->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "DTO_ID", xcpconfig_data->DTO_ID, Fd);
    xcpconfig_data->Prescaler = ui->lineEditPrescaler->text().toInt();
    ScQt_IniFileDataBaseWriteInt (Section, "Prescaler", xcpconfig_data->Prescaler, Fd);
    xcpconfig_data->ExtIds = (ui->checkBoxExtIdentiefier->isChecked()) ? 1 : 0;
    ScQt_IniFileDataBaseWriteInt (Section, "ExtIds", xcpconfig_data->ExtIds, Fd);
    xcpconfig_data->EventChannel = ui->lineEditEventChannelNo->text().toInt();
    ScQt_IniFileDataBaseWriteInt (Section, "EventChannel", xcpconfig_data->EventChannel, Fd);

    ScQt_IniFileDataBaseWriteYesNo (Section, "OnlyBytePointerInDAQ", xcpconfig_data->OnlyBytePointerInDAQ, Fd);
    if (ui->checkBoxEnableCalibration->isChecked()) xcpconfig_data->CalibrationEnable = 1;
    else xcpconfig_data->CalibrationEnable = 0;
    ScQt_IniFileDataBaseWriteYesNo (Section, "CalibrationEnable", xcpconfig_data->CalibrationEnable, Fd);
    if (ui->checkBoxMoveROMRAM->isChecked()) xcpconfig_data->MoveROM2RAM = 1;
    else xcpconfig_data->MoveROM2RAM = 0;
    ScQt_IniFileDataBaseWriteYesNo (Section, "MoveROM2RAM", xcpconfig_data->MoveROM2RAM, Fd);
    if (ui->checkBoxSetCalibration->isChecked()) xcpconfig_data->SelCalPage = 1;
    else xcpconfig_data->SelCalPage = 0;
    ScQt_IniFileDataBaseWriteYesNo (Section, "SelCalPage", xcpconfig_data->SelCalPage, Fd);
    if (ui->checkBoxReadParam->isChecked()) xcpconfig_data->ReadParameterAfterCalib = 1;
    else xcpconfig_data->ReadParameterAfterCalib = 0;
    ScQt_IniFileDataBaseWriteInt (Section, "ReadParameterAfterCalib", xcpconfig_data->ReadParameterAfterCalib, Fd);
    xcpconfig_data->CalibROMSegment = ui->lineEditCalibrationROMSeg->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CalibROMSegment", xcpconfig_data->CalibROMSegment, Fd);
    xcpconfig_data->CalibROMPageNo = ui->lineEditCalibrationROMpage->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CalibROMPageNo", xcpconfig_data->CalibROMPageNo, Fd);
    xcpconfig_data->CalibRAMSegment = ui->lineEditCalibrationRAMSeg->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CalibRAMSegment", xcpconfig_data->CalibRAMSegment, Fd);
    xcpconfig_data->CalibRAMPageNo = ui->lineEditCalibrationRAMpage->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CalibRAMPageNo", xcpconfig_data->CalibRAMPageNo, Fd);

    int VarSize = ui->listWidgetOfMeasurements->count();
    int CalSize = ui->listWidgetOfCalibrations->count();
    int SumSize = VarSize + CalSize;
    uint64_t uic = 0;
    uint32_t ui100 = 0;
    uint32_t ui100l = 0;
    int ProcBar;
    if (SumSize > 1000) {
        ProcBar = OpenProgressBarFromOtherThread ("store CCP");
    } else {
        ProcBar = -1;
    }
    for (x = 0; x < VarSize; x++) {
        QString Entry = QString("v%1").arg(x);
        QString Line = ui->listWidgetOfMeasurements->item(x)->text();
        ScQt_IniFileDataBaseWriteString (Section, Entry, Line, Fd);
        ui100 = (uint32_t)((uic * 100LLU) / (uint32_t)SumSize);
        uic++;
        if ((ProcBar >= 0) && (ui100 > ui100l)) SetProgressBarFromOtherThread(ProcBar, ui100);
    }
    for ( ; x < xcpconfig_data->VarListCountOld; x++) {
        QString Entry = QString("v%1").arg(x);
        ScQt_IniFileDataBaseWriteString (Section, Entry, nullptr, Fd);
    }
    for (x = 0; x < CalSize; x++) {
        QString Entry = QString("p%1").arg(x);
        QString Line = ui->listWidgetOfCalibrations->item(x)->text();
        ScQt_IniFileDataBaseWriteString (Section, Entry, Line, Fd);
        ui100 = (uint32_t)((uic * 100LLU) / (uint32_t)SumSize);
        uic++;
        if ((ProcBar >= 0) && (ui100 > ui100l)) SetProgressBarFromOtherThread(ProcBar, ui100);
    }
    for ( ; x < xcpconfig_data->CalListCountOld; x++) {
        QString Entry = QString("p%1").arg(x);
        ScQt_IniFileDataBaseWriteString (Section, Entry, nullptr, Fd);
    }
    if (ProcBar >= 0) CloseProgressBarFromOtherThread(ProcBar);

    xcpconfig_data->UseUnit = ui->checkBoxUnit->isChecked();
    xcpconfig_data->UseConversion = ui->checkBoxConversion->isChecked();
    xcpconfig_data->UseMinMax = ui->checkBoxMinMax->isChecked();
    ScQt_IniFileDataBaseWriteYesNo (Section, "UseUnit", xcpconfig_data->UseUnit, Fd);
    ScQt_IniFileDataBaseWriteYesNo (Section, "UseConversion", xcpconfig_data->UseConversion, Fd);
    ScQt_IniFileDataBaseWriteYesNo (Section, "UseMinMax", xcpconfig_data->UseMinMax, Fd);
}

int XCPConfigWidget::FillListBoxes(int CalibrationOrMeasurement)
{
    int Ret = 0;
    int index;
    int Fd = ScQt_GetMainFileDescriptor();

    QString Section = QString("XCP Configuration for Target %1").arg(curConnection);
    index = 0;
    QListWidget *ListWidget;
    switch (ui->tabWidget->currentIndex()) {
    default:
    case 0:  // Measurement
        ListWidget = ui->listWidgetOfMeasurements;
        break;
    case 1:  // Calibration
        ListWidget = ui->listWidgetOfCalibrations;
        break;
    }
    ListWidget->clear();
    while (1) {
        QString Entry;
        if (CalibrationOrMeasurement) {
            Entry = QString("p%1").arg(index);;
        } else {
            Entry = QString("v%1").arg(index);
        }
        QString Line = ScQt_IniFileDataBaseReadString(Section, Entry, "", Fd);
        if (Line.isEmpty()) {
            break;
        }
        ListWidget->addItem(Line);
        Ret++;
        index++;
    }

    return Ret;
}

void XCPConfigWidget::listWidgetListOfLabels_itemClicked(QListWidgetItem *item)
{
    QString itemString = item->data(Qt::DisplayRole).toString();
    QStringList tmpList = itemString.split(",");
    ui->comboBoxDataType->setCurrentText(tmpList.at(0));
    ui->lineEditName->setText(tmpList.at(1));
    ui->lineEditAddress->setText(tmpList.at(2));
    if (tmpList.count() > 3) {
        ui->checkBoxUnit->setChecked(true);
        ui->lineEditUnit->setText(tmpList.at(3));
        if  (tmpList.count() > 4) {
            ui->checkBoxConversion->setChecked(true);
            ui->lineEditConversion->setText(tmpList.at(4));
            if  (tmpList.count() > 5) {
                ui->checkBoxMinMax->setChecked(true);
                ui->lineEditMinValue->setText(tmpList.at(5));
                ui->lineEditMaxValue->setText(tmpList.at(6));
            } else ui->checkBoxMinMax->setChecked(false);
        } else ui->checkBoxConversion->setChecked(false);
    } else ui->checkBoxUnit->setChecked(false);

}

QString XCPConfigWidget::BuildLine()
{
    bool UnitFlag = ui->checkBoxUnit->isChecked();
    bool ConversionFlag = ui->checkBoxConversion->isChecked();
    bool MinMaxFlag = ui->checkBoxMinMax->isChecked();

    QString Line = ui->comboBoxDataType->currentText();
    Line.append(",");
    Line.append(ui->lineEditName->text().trimmed());
    Line.append(",");
    Line.append(ui->lineEditAddress->text().trimmed());
    if (UnitFlag) {
       Line.append(",");
       Line.append(ui->lineEditUnit->text().trimmed());
    } else if (ConversionFlag || MinMaxFlag) {
        Line.append(",");
    }
    if (ConversionFlag) {
       Line.append(",");
       Line.append(ui->lineEditConversion->text().trimmed());
    } else if (MinMaxFlag) {
        Line.append(",");
    }
    if (MinMaxFlag) {
       Line.append(",");
       Line.append(ui->lineEditMinValue->text().trimmed());
       Line.append(",");
       Line.append(ui->lineEditMaxValue->text().trimmed());
    }
    return Line;
}

void XCPConfigWidget::on_listWidgetOfMeasurements_itemClicked(QListWidgetItem *item)
{
    listWidgetListOfLabels_itemClicked(item);
}

void XCPConfigWidget::on_listWidgetOfCalibrations_itemClicked(QListWidgetItem *item)
{
    listWidgetListOfLabels_itemClicked(item);
}

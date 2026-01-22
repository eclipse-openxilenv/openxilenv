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
#include "CcpConfigWidget.h"
#include "ui_CcpConfigWidget.h"
#include "StringHelpers.h"
#include "MainWinowSyncWithOtherThreads.h"
#include "FileDialog.h"
#include "QtIniFile.h"

extern "C"
{
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
    #include "StringMaxChar.h"
}

int CCPConfigWidget::Connection = 0;

CCPConfigWidget::CCPConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CCPConfigWidget),
    ccpconfig_data(new CCPCFGDLG_SHEET_DATA)
{
    ui->setupUi(this);

    for (int c = 0; c < MAX_CAN_CHANNELS; c++) {
        ui->comboBoxChannel->addItem(QString().number(c));
    }
    ui->pushButtonInsertBefore->setEnabled(false);
    ui->pushButtonInsertBehind->setEnabled(false);

    connect(ui->checkBoxEnableSeedAndKey, SIGNAL(toggled(bool)), ui->lineEditDAQDLL, SLOT(setEnabled(bool)));
    connect(ui->checkBoxEnableSeedAndKey, SIGNAL(toggled(bool)), ui->lineEditCALDLL, SLOT(setEnabled(bool)));
    connect(ui->checkBoxEnableCalibration, SIGNAL(toggled(bool)), ui->checkBoxMoveROMRAM, SLOT(setEnabled(bool)));
    connect(ui->checkBoxEnableCalibration, SIGNAL(toggled(bool)), ui->checkBoxSetCalibration, SLOT(setEnabled(bool)));
    connect(ui->checkBoxEnableCalibration, SIGNAL(toggled(bool)), ui->checkBoxReadParam, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMoveROMRAM, SIGNAL(toggled(bool)), ui->lineEditCalibrationROMAdr, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMoveROMRAM, SIGNAL(toggled(bool)), ui->lineEditCalibrationRAMAdr, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMoveROMRAM, SIGNAL(toggled(bool)), ui->lineEditCalibrationROMRAMSize, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMoveROMRAM, SIGNAL(toggled(bool)), ui->checkBoxSetCalibration, SLOT(setChecked(bool)));
    connect(ui->checkBoxUnit, SIGNAL(toggled(bool)), ui->lineEditUnit, SLOT(setEnabled(bool)));
    connect(ui->checkBoxConversion, SIGNAL(toggled(bool)), ui->lineEditConversion, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMinMax, SIGNAL(toggled(bool)), ui->lineEditMaxValue, SLOT(setEnabled(bool)));
    connect(ui->checkBoxMinMax, SIGNAL(toggled(bool)), ui->lineEditMinValue, SLOT(setEnabled(bool)));

    tmpCopyBuffer.clear();

    curConnection = Connection;
    initccpcfgdata();

    ui->comboBoxProtocolVersion->setCurrentText(TranslateCCPVersionNumber2String(ccpconfig_data->CCPVersion));
    ui->comboBoxByteOrder->setCurrentText(ccpconfig_data->ByteOrderString);
    ui->comboBoxChannel->setCurrentIndex(ccpconfig_data->Channel);
    ui->comboBoxDebug->setCurrentIndex(ccpconfig_data->DebugFlag);
    ui->lineEditCRO_ID->setText(QString("0x%1").arg(ccpconfig_data->CRO_ID, 0 ,16));
    ui->lineEditCRM_ID->setText(QString("0x%1").arg(ccpconfig_data->CRM_ID, 0, 16));
    ui->lineEditDTO_ID->setText(QString("0x%1").arg(ccpconfig_data->DTO_ID, 0, 16));
    ui->lineEditCommandTimeout->setText(QString().number(ccpconfig_data->Timeout));
    ui->lineEditStationAddress->setText(QString("0x%1").arg(ccpconfig_data->StationAddress, 0, 16));
    ui->lineEditEventChannelNo->setText(QString().number(ccpconfig_data->EventChannel));
    ui->lineEditPrescaler->setText(QString().number(ccpconfig_data->Prescaler));
    ui->checkBoxExtIdentiefier->setChecked(ccpconfig_data->ExtIds != 0);
    ui->checkBoxEnableSeedAndKey->setChecked(ccpconfig_data->SeedAndKeyFlag != 0);
    ui->checkBoxEnableCalibration->setChecked(ccpconfig_data->CalibrationEnable != 0);
    ui->checkBoxMoveROMRAM->setChecked(ccpconfig_data->MoveROM2RAM != 0);
    ui->checkBoxSetCalibration->setChecked(ccpconfig_data->SelCalPage != 0);
    ui->checkBoxReadParam->setChecked(ccpconfig_data->ReadParameterAfterCalib != 0);
    ui->lineEditCalibrationRAMAdr->setText(QString("0x%1").arg(ccpconfig_data->CalibRAMStartAddr, 0, 16));
    ui->lineEditCalibrationROMAdr->setText(QString("0x%1").arg(ccpconfig_data->CalibROMStartAddr, 0, 16));
    ui->lineEditCalibrationROMRAMSize->setText(QString("0x%1").arg(ccpconfig_data->CalibROMRAMSize, 0, 16));
    ui->checkBoxUnit->setChecked(ccpconfig_data->UseUnit != 0);
    ui->checkBoxConversion->setChecked(ccpconfig_data->UseConversion != 0);
    ui->checkBoxMinMax->setChecked(ccpconfig_data->UseMinMax != 0);

    ui->lineEditDAQDLL->setText(ccpconfig_data->SeedKeyDll);
    ui->lineEditCALDLL->setText(ccpconfig_data->SeedKeyDllCal);

    if (ui->checkBoxEnableSeedAndKey->isChecked()) {
        ui->lineEditDAQDLL->setEnabled(true);
        ui->lineEditCALDLL->setEnabled(true);
    } else {
        ui->lineEditDAQDLL->setEnabled(false);
        ui->lineEditCALDLL->setEnabled(false);
    }

    if(ui->checkBoxEnableCalibration->isChecked()) {
        ui->checkBoxMoveROMRAM->setEnabled(true);
        ui->checkBoxSetCalibration->setEnabled(true);
        ui->checkBoxReadParam->setEnabled(true);
        ui->lineEditCalibrationROMAdr->setEnabled(true);
        ui->lineEditCalibrationRAMAdr->setEnabled(true);
        ui->lineEditCalibrationROMRAMSize->setEnabled(true);
    } else {
        ui->checkBoxMoveROMRAM->setEnabled(false);
        ui->checkBoxSetCalibration->setEnabled(false);
        ui->checkBoxReadParam->setEnabled(false);
        ui->lineEditCalibrationROMAdr->setEnabled(false);
        ui->lineEditCalibrationRAMAdr->setEnabled(false);
        ui->lineEditCalibrationROMRAMSize->setEnabled(false);
    }

    if (ui->checkBoxConversion->isChecked())
    {
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

CCPConfigWidget::~CCPConfigWidget()
{
    Connection--;
    delete ccpconfig_data;
    delete ui;
}

int CCPConfigWidget::TranslateCCPVersionString2Number(QString &VersionString)
{
    if (!VersionString.compare("2.0")) {
        return 0x200;
    } else if (!VersionString.compare("2.1")) {
        return 0x201;
    } else {
        return 0x201;
    }
}

void CCPConfigWidget::on_pushButtonCut_clicked()
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

void CCPConfigWidget::on_pushButtonCopy_clicked()
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
    ui->pushButtonInsertBehind->setEnabled(true);}

void CCPConfigWidget::on_pushButtonInsertBefore_clicked()
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

void CCPConfigWidget::on_pushButtonInsertBehind_clicked()
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

void CCPConfigWidget::on_pushButtonImport_clicked()
{
    char Text[INI_MAX_LINE_LENGTH];
    FILE *fh;

    QString FileName = FileDialog::getOpenFileName(this, "import ccp file", QString(), CCP_EXT);

    if(!FileName.isEmpty()) {
        StringCopyMaxCharTruncate(Text, QStringToConstChar(FileName), sizeof(Text));
        if ((fh = open_file(Text, "rt")) == nullptr) {
            ThrowError (1, "cannot open file %s", Text);
            return;
        }
        while (fgets (Text, sizeof(Text), fh) != nullptr) {
            char *p;
            for (p = Text; (*p != 0) && (*p != '\n'); p++);
            *p = 0;
            switch (ui->tabWidget->currentIndex()) {
            case 0:  // Measurement
                ui->listWidgetOfMeasurements->addItem(Text);
                break;
            case 1:  // Calibration
                ui->listWidgetOfCalibrations->addItem(Text);
                break;
            }
        }
        close_file (fh);
    }
}

void CCPConfigWidget::on_pushButtonExport_clicked()
{
    QListWidget *ListWidget;
    char Text[INI_MAX_LINE_LENGTH];
    FILE *fh;

    QString FileName = FileDialog::getSaveFileName(this, QString ("Save as"), QString(), QString (CCP_EXT));
    StringCopyMaxCharTruncate(Text, QStringToConstChar(FileName), sizeof(Text));
    if ((fh = open_file (Text, "wt")) == nullptr) {
        ThrowError (1, "cannot open file %s", Text);
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
        STRING_COPY_TO_ARRAY(Text, QStringToConstChar(ListWidget->item(x)->text()));
        fprintf (fh, "%s\n", Text);
    }
    close_file (fh);
}

void CCPConfigWidget::on_pushButtonAddBefore_clicked()
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


void CCPConfigWidget::on_pushButtonAddBehind_clicked()
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

void CCPConfigWidget::on_pushButtonChange_clicked()
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

QString CCPConfigWidget::TranslateCCPVersionNumber2String (int number)
{
    if (number == 0x200)
    {
        return "2.0";
    } else if (number == 0x201) {
        return "2.1";
    } else if (number == 0x10000201) {
        return "2.1 ESV1";
    } else if (number == 0x20000201) {
        return "2.1 S_STATUS";
    } else {
        return "2.1";
    }
}

void CCPConfigWidget::initccpcfgdata()
{
    int Fd = ScQt_GetMainFileDescriptor();
    QString Entry;
    QString Section = QString("CCP Configuration for Target %1").arg(Connection);

    ccpconfig_data->DebugFlag = ScQt_IniFileDataBaseReadYesNo (Section, "Debug", 0, Fd);
    ccpconfig_data->CCPVersionString = ScQt_IniFileDataBaseReadString (Section, "Protocol Version", "2.1", Fd);
    ccpconfig_data->CCPVersion = TranslateCCPVersionString2Number (ccpconfig_data->CCPVersionString);
    ccpconfig_data->ByteOrderString = ScQt_IniFileDataBaseReadString (Section, "ByteOrder", "lsb_first", Fd);
    ccpconfig_data->Channel = ScQt_IniFileDataBaseReadInt (Section, "Channel", 0, Fd);
    ccpconfig_data->Timeout = ScQt_IniFileDataBaseReadInt (Section, "Timeout", 1000, Fd);
    ccpconfig_data->SeedKeyDll = ScQt_IniFileDataBaseReadString (Section, "SeedKeyDll", "", Fd);
    ccpconfig_data->SeedAndKeyFlag = ScQt_IniFileDataBaseReadInt (Section, "SeedKeyFlag", 0, Fd);
    ccpconfig_data->SeedKeyDllCal = ScQt_IniFileDataBaseReadString (Section, "SeedKeyDllCal", "", Fd);
    ccpconfig_data->CRO_ID = ScQt_IniFileDataBaseReadUInt (Section, "CRO_ID", 0, Fd);
    ccpconfig_data->CRM_ID = ScQt_IniFileDataBaseReadUInt (Section, "CRM_ID", 0, Fd);
    ccpconfig_data->DTO_ID = ScQt_IniFileDataBaseReadUInt (Section, "DTO_ID", 0, Fd);
    ccpconfig_data->ExtIds = ScQt_IniFileDataBaseReadYesNo(Section, "ExtIds", 0, Fd);
    ccpconfig_data->Prescaler = ScQt_IniFileDataBaseReadInt (Section, "Prescaler", 1, Fd);
    ccpconfig_data->StationAddress = ScQt_IniFileDataBaseReadUInt (Section, "StationAddress", 0x39, Fd);
    ccpconfig_data->EventChannel = ScQt_IniFileDataBaseReadInt (Section, "EventChannel", 0, Fd);
    ccpconfig_data->OnlyBytePointerInDAQ = ScQt_IniFileDataBaseReadYesNo (Section, "OnlyBytePointerInDAQ", 0, Fd);
    ccpconfig_data->CalibrationEnable = ScQt_IniFileDataBaseReadYesNo (Section, "CalibrationEnable", 0, Fd);
    ccpconfig_data->MoveROM2RAM = ScQt_IniFileDataBaseReadYesNo (Section, "MoveROM2RAM", 0, Fd);
    ccpconfig_data->SelCalPage = ScQt_IniFileDataBaseReadYesNo (Section, "SelCalPage", 0, Fd);
    ccpconfig_data->CalibROMStartAddr = ScQt_IniFileDataBaseReadUInt (Section, "CalibROMStartAddr", 0x0, Fd);
    ccpconfig_data->CalibRAMStartAddr = ScQt_IniFileDataBaseReadUInt (Section, "CalibRAMStartAddr", 0x0, Fd);
    ccpconfig_data->CalibROMRAMSize = ScQt_IniFileDataBaseReadUInt (Section, "CalibROMRAMSize", 0x0, Fd);
    ccpconfig_data->ReadParameterAfterCalib = ScQt_IniFileDataBaseReadInt (Section, "ReadParameterAfterCalib", 0, Fd);

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

    for (ccpconfig_data->VarListCount = 0; ; ccpconfig_data->VarListCount++) {
        Entry = QString("v%1").arg(ccpconfig_data->VarListCount);
        QString Line = ScQt_IniFileDataBaseReadString (Section, Entry, "", Fd);
        if(Line.isEmpty()) {
            break;
        }
        ui->listWidgetOfMeasurements->addItem(Line);
        ui100 = (uint32_t)((uic * 100LLU) / (uint32_t)Size);
        uic++;
        if ((ProcBar >= 0) && (ui100 > ui100l)) SetProgressBarFromOtherThread(ProcBar, ui100);
    }
    ccpconfig_data->VarListCountOld = ccpconfig_data->VarListCount;
    for (ccpconfig_data->CalListCount = 0; ; ccpconfig_data->CalListCount++) {
        Entry = QString("p%1").arg(ccpconfig_data->CalListCount);
        QString Line = ScQt_IniFileDataBaseReadString (Section, Entry, "", Fd);
        if (Line.isEmpty()) {
            break;
        }
        ui->listWidgetOfCalibrations->addItem(Line);
        ui100 = (uint32_t)((uic * 100LLU) / (uint32_t)Size);
        uic++;
        if ((ProcBar >= 0) && (ui100 > ui100l)) SetProgressBarFromOtherThread(ProcBar, ui100);
    }
    ccpconfig_data->CalListCountOld = ccpconfig_data->CalListCount;
    if (ProcBar >= 0) CloseProgressBarFromOtherThread(ProcBar);

    ccpconfig_data->UseMinMax = ScQt_IniFileDataBaseReadYesNo (Section, "UseMinMax", 0, Fd);

    ccpconfig_data->UseUnit = ScQt_IniFileDataBaseReadYesNo (Section, "UseUnit", 0, Fd);
    ccpconfig_data->UseConversion = ScQt_IniFileDataBaseReadYesNo (Section, "UseConversion", 0, Fd);
    ccpconfig_data->UseMinMax = ScQt_IniFileDataBaseReadYesNo (Section, "UseMinMax", 0, Fd);

    ccpconfig_data->LoadedFlag = 1;
}


void CCPConfigWidget::listWidgetListOfLabels_itemClicked(QListWidgetItem *item)
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

QString CCPConfigWidget::BuildLine()
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

void CCPConfigWidget::on_listWidgetOfMeasurements_itemClicked(QListWidgetItem *item)
{
    listWidgetListOfLabels_itemClicked(item);
}

void CCPConfigWidget::on_listWidgetOfCalibrations_itemClicked(QListWidgetItem *item)
{
    listWidgetListOfLabels_itemClicked(item);
}


void CCPConfigWidget::saveCCPConfigData()
{
    QString Section;
    QString Entry;
    int x;
    int Fd = ScQt_GetMainFileDescriptor();

    Section = QString("CCP Configuration for Target %1").arg(curConnection);
    ccpconfig_data->DebugFlag = !ui->comboBoxDebug->currentText().compare("Yes", Qt::CaseInsensitive);
    ScQt_IniFileDataBaseWriteYesNo (Section, "Debug", ccpconfig_data->DebugFlag, Fd);
    ccpconfig_data->CCPVersionString = ui->comboBoxProtocolVersion->currentText();
    ScQt_IniFileDataBaseWriteString (Section, "Protocol Version", ccpconfig_data->CCPVersionString, Fd);
    ccpconfig_data->ByteOrderString, ui->comboBoxByteOrder->currentText();
    ScQt_IniFileDataBaseWriteString (Section, "ByteOrder", ccpconfig_data->ByteOrderString, Fd);
    ccpconfig_data->Channel = ui->comboBoxChannel->currentIndex();
    ScQt_IniFileDataBaseWriteInt (Section, "Channel", ccpconfig_data->Channel, Fd);
    ccpconfig_data->Timeout = ui->lineEditCommandTimeout->text().toInt();
    ScQt_IniFileDataBaseWriteInt (Section, "Timeout", ccpconfig_data->Timeout, Fd);
    ccpconfig_data->SeedKeyDll =ui->lineEditDAQDLL->text();
    ScQt_IniFileDataBaseWriteString (Section, "SeedKeyDll", ccpconfig_data->SeedKeyDll, Fd);
    if (ui->checkBoxEnableSeedAndKey->isChecked()) ccpconfig_data->SeedAndKeyFlag = 1;
    else ccpconfig_data->SeedAndKeyFlag = 0;
    ScQt_IniFileDataBaseWriteInt (Section, "SeedKeyFlag", ccpconfig_data->SeedAndKeyFlag, Fd);
    ccpconfig_data->SeedKeyDllCal = ui->lineEditCALDLL->text();
    ScQt_IniFileDataBaseWriteString (Section, "SeedKeyDllCal", ccpconfig_data->SeedKeyDllCal, Fd);
    ccpconfig_data->CRO_ID = ui->lineEditCRO_ID->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CRO_ID", ccpconfig_data->CRO_ID, Fd);
    ccpconfig_data->CRM_ID = ui->lineEditCRM_ID->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CRM_ID", ccpconfig_data->CRM_ID, Fd);
    ccpconfig_data->DTO_ID = ui->lineEditDTO_ID->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "DTO_ID", ccpconfig_data->DTO_ID, Fd);
    ccpconfig_data->Prescaler = ui->lineEditPrescaler->text().toInt();
    ScQt_IniFileDataBaseWriteInt (Section, "Prescaler", ccpconfig_data->Prescaler, Fd);
    ccpconfig_data->StationAddress = ui->lineEditStationAddress->text().toUInt(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "StationAddress", ccpconfig_data->StationAddress, Fd);
    ccpconfig_data->ExtIds = (ui->checkBoxExtIdentiefier->isChecked()) ? 1 : 0;
    ScQt_IniFileDataBaseWriteInt (Section, "ExtIds", ccpconfig_data->ExtIds, Fd);
    ccpconfig_data->EventChannel = ui->lineEditEventChannelNo->text().toInt();
    ScQt_IniFileDataBaseWriteInt (Section, "EventChannel", ccpconfig_data->EventChannel, Fd);

    ScQt_IniFileDataBaseWriteYesNo (Section, "OnlyBytePointerInDAQ", ccpconfig_data->OnlyBytePointerInDAQ, Fd);
    if (ui->checkBoxEnableCalibration->isChecked()) ccpconfig_data->CalibrationEnable = 1;
    else ccpconfig_data->CalibrationEnable = 0;
    ScQt_IniFileDataBaseWriteYesNo (Section, "CalibrationEnable", ccpconfig_data->CalibrationEnable, Fd);
    if (ui->checkBoxMoveROMRAM->isChecked()) ccpconfig_data->MoveROM2RAM = 1;
    else ccpconfig_data->MoveROM2RAM = 0;
    ScQt_IniFileDataBaseWriteYesNo (Section, "MoveROM2RAM", ccpconfig_data->MoveROM2RAM, Fd);
    if (ui->checkBoxSetCalibration->isChecked()) ccpconfig_data->SelCalPage = 1;
    else ccpconfig_data->SelCalPage = 0;
    ScQt_IniFileDataBaseWriteYesNo (Section, "SelCalPage", ccpconfig_data->SelCalPage, Fd);
    if (ui->checkBoxReadParam->isChecked()) ccpconfig_data->ReadParameterAfterCalib = 1;
    else ccpconfig_data->ReadParameterAfterCalib = 0;
    ScQt_IniFileDataBaseWriteInt (Section, "ReadParameterAfterCalib", ccpconfig_data->ReadParameterAfterCalib, Fd);
    ccpconfig_data->CalibROMStartAddr = ui->lineEditCalibrationROMAdr->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CalibROMStartAddr", ccpconfig_data->CalibROMStartAddr, Fd);
    ccpconfig_data->CalibRAMStartAddr = ui->lineEditCalibrationRAMAdr->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CalibRAMStartAddr", ccpconfig_data->CalibRAMStartAddr, Fd);
    ccpconfig_data->CalibROMRAMSize = ui->lineEditCalibrationROMRAMSize->text().toULong(nullptr,16);
    ScQt_IniFileDataBaseWriteUIntHex (Section, "CalibROMRAMSize", ccpconfig_data->CalibROMRAMSize, Fd);

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
        Entry = QString("v%1").arg(x);
        QString Line = ui->listWidgetOfMeasurements->item(x)->text();
        ScQt_IniFileDataBaseWriteString (Section, Entry, Line, Fd);
        ui100 = (uint32_t)((uic * 100LLU) / (uint32_t)SumSize);
        uic++;
        if ((ProcBar >= 0) && (ui100 > ui100l)) SetProgressBarFromOtherThread(ProcBar, ui100);
    }
    // delete old trys from list if the list was longer
    for ( ; x < ccpconfig_data->VarListCountOld; x++) {
        Entry = QString("v%1").arg(x);
        ScQt_IniFileDataBaseWriteString (Section, Entry, nullptr, Fd);
    }
    for (x = 0; x < CalSize; x++) {
        Entry = QString("p%1").arg(x);
        QString Line = ui->listWidgetOfCalibrations->item(x)->text();
        ScQt_IniFileDataBaseWriteString (Section, Entry, Line, Fd);
        ui100 = (uint32_t)((uic * 100LLU) / (uint32_t)SumSize);
        uic++;
        if ((ProcBar >= 0) && (ui100 > ui100l)) SetProgressBarFromOtherThread(ProcBar, ui100);
    }
    // delete old trys from list if the list was longer
    for ( ; x < ccpconfig_data->CalListCountOld; x++) {
        Entry = QString("p%1").arg(x);
        ScQt_IniFileDataBaseWriteString (Section, Entry, nullptr, Fd);
    }
    if (ProcBar >= 0) CloseProgressBarFromOtherThread(ProcBar);

    ccpconfig_data->UseUnit = ui->checkBoxUnit->isChecked();
    ccpconfig_data->UseConversion = ui->checkBoxConversion->isChecked();
    ccpconfig_data->UseMinMax = ui->checkBoxMinMax->isChecked();
    ScQt_IniFileDataBaseWriteYesNo (Section, "UseUnit", ccpconfig_data->UseUnit, Fd);
    ScQt_IniFileDataBaseWriteYesNo (Section, "UseConversion", ccpconfig_data->UseConversion, Fd);
    ScQt_IniFileDataBaseWriteYesNo (Section, "UseMinMax", ccpconfig_data->UseMinMax, Fd);
}

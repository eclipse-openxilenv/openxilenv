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


#include "CanMessageWindowConfigDialog.h"
#include "ui_CanMessageWindowConfigDialog.h"
#include "QtIniFile.h"
#include "WindowNameAlreadyInUse.h"
#include "FileDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "ThrowError.h"
#include "MyMemory.h"
#include "FileExtensions.h"
}

CANMessageWindowConfigDialog::CANMessageWindowConfigDialog(QString par_WindowTitle,
                                                           int par_AcceptanceMaskRows,
                                                           CAN_ACCEPT_MASK *par_AcceptanceMask,
                                                           bool par_DecodeFlag,
                                                           bool par_DisplayColumnCounterFlag,
                                                           bool par_DisplayColumnTimeAbsoluteFlag,
                                                           bool par_DisplayColumnTimeDiffFlag,
                                                           bool par_DisplayColumnTimeDiffMinMaxFlag,
                                                           bool par_Record2Disk,
                                                           QString &par_RecorderFileName,
                                                           bool par_TriggerActiv,
                                                           QString &par_TriggerEquation,
                                                           QWidget *parent) : Dialog(parent),
    ui(new Ui::CANMessageWindowConfigDialog)
{
    ui->setupUi(this);

    connect(ui->RecorderTriggerCheckBox, SIGNAL(clicked(bool)), ui->RecorderTriggerEquationLineEdit, SLOT(setEnabled(bool)));

    m_WindowTitle = par_WindowTitle;
    ui->WindowTitleLineEdit->setText(par_WindowTitle);
    QStringList AcceptanceColumnNameList;
    AcceptanceColumnNameList << "Channel" << "From identifier" << "To identifier" << "Normal" << "Ext" << "FD" << "BRS" << "J1939MP";
    ui->AcceptanceMaskTableWidget->setColumnCount(AcceptanceColumnNameList.count());
    ui->AcceptanceMaskTableWidget->setHorizontalHeaderLabels (AcceptanceColumnNameList);
    for (int Column = 0; Column < ui->AcceptanceMaskTableWidget->columnCount(); Column++) {
        ui->AcceptanceMaskTableWidget->resizeColumnToContents(Column);
    }
    for (int Row = 0; (Row < par_AcceptanceMaskRows) && (par_AcceptanceMask[Row].Channel >= 0); Row++) {
        ui->AcceptanceMaskTableWidget->insertRow(Row);
        QTableWidgetItem *Item;
        Item = new QTableWidgetItem(QString ().number(par_AcceptanceMask[Row].Channel));
        ui->AcceptanceMaskTableWidget->setItem (Row, 0, Item);
        Item = new QTableWidgetItem(QString ("0x").append(QString ().number(par_AcceptanceMask[Row].Start, 16)));
        ui->AcceptanceMaskTableWidget->setItem (Row, 1, Item);
        Item = new QTableWidgetItem(QString ("0x").append(QString ().number(par_AcceptanceMask[Row].Stop, 16)));
        ui->AcceptanceMaskTableWidget->setItem (Row, 2, Item);
        // Normal
        Item = new QTableWidgetItem();
        if ((par_AcceptanceMask[Row].Flags & 0x100) == 0) Item->setCheckState(Qt::Checked);
        else Item->setCheckState(Qt::Unchecked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 3, Item);
        // Ext
        Item = new QTableWidgetItem();
        if ((par_AcceptanceMask[Row].Flags & 0x1) == 0) Item->setCheckState(Qt::Checked);
        else Item->setCheckState(Qt::Unchecked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 4, Item);
        // FD
        Item = new QTableWidgetItem();
        if ((par_AcceptanceMask[Row].Flags & 0x2) == 0) Item->setCheckState(Qt::Checked);
        else Item->setCheckState(Qt::Unchecked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 5, Item);
        // BTS
        Item = new QTableWidgetItem();
        if ((par_AcceptanceMask[Row].Flags & 0x4) == 0) Item->setCheckState(Qt::Checked);
        else Item->setCheckState(Qt::Unchecked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 6, Item);
        // J1939MP
        Item = new QTableWidgetItem();
        if ((par_AcceptanceMask[Row].Flags & 0x200) != 0) Item->setCheckState(Qt::Checked);
        else Item->setCheckState(Qt::Unchecked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 7, Item);
    }
    ui->DecodeCheckBox->setChecked(par_DecodeFlag);
    ui->MessageCounterCheckBox->setChecked(par_DisplayColumnCounterFlag);
    ui->MessageTimeAbsoluteCheckBox->setChecked(par_DisplayColumnTimeAbsoluteFlag);
    ui->MessageTimeDifferenceCheckBox->setChecked(par_DisplayColumnTimeDiffFlag);
    ui->MessageTimeMinMaxDifferenceCheckBox->setChecked(par_DisplayColumnTimeDiffMinMaxFlag);

    ui->RecorderOnCheckBox->setChecked(par_Record2Disk);
    ui->RecorderFilenameLineEdit->setText(par_RecorderFileName);
    ui->RecorderTriggerCheckBox->setChecked(par_TriggerActiv);
    ui->RecorderTriggerEquationLineEdit->setText(par_TriggerEquation);

    if(!ui->RecorderTriggerCheckBox->isChecked()) {
        ui->RecorderTriggerEquationLineEdit->setDisabled(true);
    } else {
        ui->RecorderTriggerEquationLineEdit->setDisabled(false);
    }
}

CANMessageWindowConfigDialog::~CANMessageWindowConfigDialog()
{
    delete ui;
}

QString CANMessageWindowConfigDialog::GetWindowTitle()
{
    return ui->WindowTitleLineEdit->text();
}

bool CANMessageWindowConfigDialog::WindowTitleChanged()
{
    if (m_WindowTitle.compare(ui->WindowTitleLineEdit->text())) {
        return true;
    } else {
        return false;
    }
}


int CANMessageWindowConfigDialog::GetAcceptanceMask(CAN_ACCEPT_MASK *AcceptanceMask)
{
    int Ret = ui->AcceptanceMaskTableWidget->rowCount();
    if (Ret > (MAX_CAN_ACCEPTANCE_MASK_ELEMENTS - 1)) {
        ThrowError (1, "only %i accepance masks allowed", MAX_CAN_ACCEPTANCE_MASK_ELEMENTS);
        Ret = MAX_CAN_ACCEPTANCE_MASK_ELEMENTS - 1;
    }
    int Row;
    for (Row = 0; Row < Ret; Row++) {
        bool Ok;
        AcceptanceMask[Row].Channel = ui->AcceptanceMaskTableWidget->item(Row, 0)->data(Qt::DisplayRole).toInt();
        AcceptanceMask[Row].Start = ui->AcceptanceMaskTableWidget->item(Row, 1)->data(Qt::DisplayRole).toString().toULong(&Ok, 0);
        AcceptanceMask[Row].Stop = ui->AcceptanceMaskTableWidget->item(Row, 2)->data(Qt::DisplayRole).toString().toULong(&Ok, 0);
        AcceptanceMask[Row].Flags = ((ui->AcceptanceMaskTableWidget->item(Row, 3)->checkState() == Qt::Checked) ? 0 : 0x100) |
                                    ((ui->AcceptanceMaskTableWidget->item(Row, 4)->checkState() == Qt::Checked) ? 0 : 0x1) |
                                    ((ui->AcceptanceMaskTableWidget->item(Row, 5)->checkState() == Qt::Checked) ? 0 : 0x2) |
                                    ((ui->AcceptanceMaskTableWidget->item(Row, 6)->checkState() == Qt::Checked) ? 0 : 0x4) |
                                    ((ui->AcceptanceMaskTableWidget->item(Row, 7)->checkState() == Qt::Checked) ? 0x200 : 0);
    }
    AcceptanceMask[Row].Channel = -1;
    AcceptanceMask[Row].Start = 0;
    AcceptanceMask[Row].Stop = 0;
    AcceptanceMask[Row].Flags = 0;
    return Ret+1;
}

bool CANMessageWindowConfigDialog::GetDecodeFlag()
{
    return ui->DecodeCheckBox->isChecked();
}

void CANMessageWindowConfigDialog::GetDisplayFlags (bool *ret_DisplayColumnCounterFlag,
                                                    bool *ret_DisplayColumnTimeAbsoluteFlag,
                                                    bool *ret_DisplayColumnTimeDiffFlag,
                                                    bool *ret_DisplayColumnTimeDiffMinMaxFlag)
{
    *ret_DisplayColumnCounterFlag = ui->MessageCounterCheckBox->isChecked();
    *ret_DisplayColumnTimeAbsoluteFlag = ui->MessageTimeAbsoluteCheckBox->isChecked();
    *ret_DisplayColumnTimeDiffFlag = ui->MessageTimeDifferenceCheckBox->isChecked();
    *ret_DisplayColumnTimeDiffMinMaxFlag = ui->MessageTimeMinMaxDifferenceCheckBox->isChecked();
}

QString CANMessageWindowConfigDialog::GetRecorderFilename (bool *ret_Record2Disk)
{
    *ret_Record2Disk = ui->RecorderOnCheckBox->isChecked();
    return ui->RecorderFilenameLineEdit->text();
}


QString CANMessageWindowConfigDialog::GetTrigger(bool *ret_TriggerActiv)
{
    *ret_TriggerActiv = ui->RecorderTriggerCheckBox->isChecked();
    return ui->RecorderTriggerEquationLineEdit->text();
}

void CANMessageWindowConfigDialog::on_NewPushButton_clicked()
{
    int Row = ui->AcceptanceMaskTableWidget->rowCount();
    if (Row < (MAX_CAN_ACCEPTANCE_MASK_ELEMENTS - 1)) {
        ui->AcceptanceMaskTableWidget->insertRow(Row);
        QTableWidgetItem *Item;
        Item = new QTableWidgetItem(QString ().number(0));
        ui->AcceptanceMaskTableWidget->setItem (Row, 0, Item);
        Item = new QTableWidgetItem(QString ("0x").append(QString ().number(0)));
        ui->AcceptanceMaskTableWidget->setItem (Row, 1, Item);
        Item = new QTableWidgetItem(QString ("0x").append(QString ().number(0x1FFFFFFF, 16)));
        ui->AcceptanceMaskTableWidget->setItem (Row, 2, Item);
        // Normal
        Item = new QTableWidgetItem();
        Item->setCheckState(Qt::Checked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 3, Item);
        // Ext
        Item = new QTableWidgetItem();
        Item->setCheckState(Qt::Checked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 4, Item);
        // FD
        Item = new QTableWidgetItem();
        Item->setCheckState(Qt::Checked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 5, Item);
        // BTS
        Item = new QTableWidgetItem();
        Item->setCheckState(Qt::Checked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 6, Item);
        // J1939MP
        Item = new QTableWidgetItem();
        Item->setCheckState(Qt::Unchecked);
        ui->AcceptanceMaskTableWidget->setItem (Row, 7, Item);
    } else {
        ThrowError (1, "max %i acceptance mask entrys allowed", MAX_CAN_ACCEPTANCE_MASK_ELEMENTS);
    }
}

void CANMessageWindowConfigDialog::on_DeletePushButton_clicked()
{
    int Row = ui->AcceptanceMaskTableWidget->currentRow();
    if ((Row >= 0) && (Row < ui->AcceptanceMaskTableWidget->rowCount())) {
        ui->AcceptanceMaskTableWidget->removeRow(Row);
    }
}

void CANMessageWindowConfigDialog::accept()
{
    QString NewWindowTitle = ui->WindowTitleLineEdit->text().trimmed();

    if (NewWindowTitle.compare (m_WindowTitle)) {
        if (NewWindowTitle.length() >= 1) {
            if(!WindowNameAlreadyInUse(NewWindowTitle)) {
                if (IsAValidWindowSectionName(NewWindowTitle)) {
                    QDialog::accept();
                } else {
                    ThrowError (1, "Inside window name only ascii characters and no []\\ are allowed \"%s\"",
                               QStringToConstChar(NewWindowTitle));
                }
            } else {
                ThrowError (1, "window name \"%s\" already in use", QStringToConstChar(NewWindowTitle));
            }
        } else {
            ThrowError (1, "window name must have one or more character \"%s\"", QStringToConstChar(NewWindowTitle));
        }
    } else {
        QDialog::accept();
    }
}


void CANMessageWindowConfigDialog::on_RecorderFilePushButton_clicked()
{
    QString sFileName = FileDialog::getSaveFileName(this, QString ("CAN logging file"), QString(), QString (TXT_EXT));
    if(sFileName.isEmpty()) return;
    ui->RecorderFilenameLineEdit->setText(sFileName);
}

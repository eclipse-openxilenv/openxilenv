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


#include "ConfigCalibrationTreeViewDialog.h"
#include "ui_ConfigCalibrationTreeViewDialog.h"

#include "StringHelpers.h"

extern "C" {
#include "StringMaxChar.h"
#include "Scheduler.h"
#include "ThrowError.h"
}


ConfigCalibrationTreeViewDialog::ConfigCalibrationTreeViewDialog(QString &par_WindowName,
                                                                   char *pProcessName,
                                                                   INCLUDE_EXCLUDE_FILTER *par_IncExcFilter,
                                                                   int par_ShowValue,
                                                                   int par_ShowAddresse,
                                                                   int par_ShowDataType,
                                                                   int par_HexOrDec,
                                                                   QWidget *parent) : Dialog(parent),
    ui(new Ui::ConfigCalibrationTreeViewDialog)
{
    ui->setupUi(this);

    ui->WindowNameLineEdit->setText (par_WindowName);
    // loop through all Processes
    char *Name;
    bool ProcessRunning  = false;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((Name = read_next_process_name (Buffer)) != nullptr) {
        if (!IsInternalProcess (Name)) {
            if (!Compare2ProcessNames (Name, pProcessName)) {
                ProcessRunning = true;
            }
            ui->ProcessComboBox->addItem (QString (Name));
        }
    }
    close_read_next_process_name(Buffer);
    if (!ProcessRunning) {
        if (strlen (pProcessName) == 0) {
            ui->ProcessComboBox->addItem (QString (""));
            ui->ProcessComboBox->setCurrentText (QString (""));
        } else {
            ui->ProcessComboBox->addItem (QString (pProcessName).append (QString (" (not running)")));
            ui->ProcessComboBox->setCurrentText (QString (pProcessName).append (QString (" (not running)")));
        }
    } else {
        char ShortProcessName[MAX_PATH];
        TruncatePathFromProcessName (ShortProcessName, pProcessName, sizeof(ShortProcessName));
        ui->ProcessComboBox->setCurrentText (QString (ShortProcessName));
    }
    ui->Filter->SetFilter (par_IncExcFilter);

    ui->ViewValuesCheckBox->setCheckState ((par_ShowValue) ? Qt::Checked : Qt::Unchecked);
    ui->ViewAddressCheckBox->setCheckState ((par_ShowAddresse) ? Qt::Checked : Qt::Unchecked);
    ui->ViewDataTypeCheckBox->setCheckState ((par_ShowDataType) ? Qt::Checked : Qt::Unchecked);

    ui->HexadecimalRadioButton->setChecked (par_HexOrDec == 1);
    ui->DecimalRadioButton->setChecked (par_HexOrDec == 0);
}

ConfigCalibrationTreeViewDialog::~ConfigCalibrationTreeViewDialog()
{
    delete ui;
}

QString ConfigCalibrationTreeViewDialog::GetWindowName ()
{
    return ui->WindowNameLineEdit->text();
}

void ConfigCalibrationTreeViewDialog::GetProcessName (char *ret_Processname, int par_Maxc)
{
    StringCopyMaxCharTruncate (ret_Processname, QStringToConstChar(ui->ProcessComboBox->currentText().trimmed()), par_Maxc);
}

INCLUDE_EXCLUDE_FILTER *ConfigCalibrationTreeViewDialog::GetFilter ()
{
    return ui->Filter->GetFilter();
}

int ConfigCalibrationTreeViewDialog::GetShowValue()
{
    if (ui->ViewValuesCheckBox->checkState() == Qt::Checked) return 1;
    else return 0;
}

int ConfigCalibrationTreeViewDialog::GetShowAddress()
{
    if (ui->ViewAddressCheckBox->checkState() == Qt::Checked) return 1;
    else return 0;
}

int ConfigCalibrationTreeViewDialog::GetShowDataType()
{
    if (ui->ViewDataTypeCheckBox->checkState() == Qt::Checked) return 1;
    else return 0;
}

int ConfigCalibrationTreeViewDialog::GetHexOrDec()
{
    if (ui->HexadecimalRadioButton->isChecked()) return 1;
    else return 0;
}

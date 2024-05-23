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


#include "ExportReferenceListDialog.h"
#include "ui_ExportReferenceListDialog.h"

#include "FileDialog.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

extern "C"
{
    #include "MainValues.h"
    #include "Scheduler.h"
    #include "FileExtensions.h"
    #include "ThrowError.h"
    #include "ExtProcessReferences.h"
}

ExportReferenceListDialog::ExportReferenceListDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::ExportReferenceListDialog)
{
    ui->setupUi(this);

    QString SCRefLFileName;
    QString CurProcess;

    // Read process name from INI file
    CurProcess = ScQt_IniFileDataBaseReadString("BasicCalibrationSettings", "ApplProcessSCRefL",
                                                "", ScQt_GetMainFileDescriptor());
    // Read process name from INI file
    SCRefLFileName = ScQt_IniFileDataBaseReadString ("BasicCalibrationSettings", "SCRefL_Format",
                                                     "indexed", ScQt_GetMainFileDescriptor());
    if (!SCRefLFileName.compare("not indexed") ||
        !SCRefLFileName.compare("notindexed") ||
        !SCRefLFileName.compare("not_indexed") ||
        s_main_ini_val.SaveRefListInNewFormatAsDefault) {
        ui->radioButtonNew->setChecked(true);
    } else  {
        ui->radioButtonOld->setChecked(true);
    }

    // Fill process list
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
    // Select last used process
    QListWidgetItem *ProcessItem = new QListWidgetItem;
    ProcessItem->setText(CurProcess);
    ui->listWidgetProcesses->setCurrentItem(ProcessItem);

    // If there is only one process inside select it
    if (ui->listWidgetProcesses->count() == 1) {
        ui->listWidgetProcesses->row(nullptr);
    }
}

ExportReferenceListDialog::~ExportReferenceListDialog()
{
    delete ui;
}

void ExportReferenceListDialog::accept()
{
    QString SCRefLFileName;
    QString CurProcess;
    char Help[MAX_PATH];
    int idx;
    int OutputFormat;  // 0 -> old (indexed), 1 -> new (not indexed)
    int Fd = ScQt_GetMainFileDescriptor();

    if (ui->lineEditSCRefL->text().isEmpty())
    {
        ThrowError(MESSAGE_STOP, "No File selected!");
        return;
    }

    if (ui->radioButtonNew->isChecked()) {
        OutputFormat = 1; // new format
        ScQt_IniFileDataBaseWriteString("BasicCalibrationSettings",
                                        "SCRefL_Format", "not indexed", Fd);
    }else {
        OutputFormat = 0;  // old format
        ScQt_IniFileDataBaseWriteString("BasicCalibrationSettings",
                                         "SCRefL_Format", "indexed", Fd);
    }

    SCRefLFileName = ui->lineEditSCRefL->text();
    if (ui->listWidgetProcesses->count() == 0) {
        ThrowError(MESSAGE_STOP, "No Process selected !");
        return;
    } else {
        idx = ui->listWidgetProcesses->currentIndex().row();
        CurProcess = ui->listWidgetProcesses->item(idx)->text();
    }

    ScQt_IniFileDataBaseWriteString("BasicCalibrationSettings",
                                    "ApplProcessSCRefL", CurProcess, Fd);

    ScQt_IniFileDataBaseWriteString("BasicCalibrationSettings",
                                    "SCRefLReadFileName", SCRefLFileName, Fd);

    ExportVariableReferenceList(get_pid_by_name(QStringToConstChar(CurProcess)), QStringToConstChar(SCRefLFileName), OutputFormat);
    QDialog::accept();

}

void ExportReferenceListDialog::on_pushButtonSCRefL_clicked()
{
    QString FileName = FileDialog::getSaveFileName(this, QString ("Save SCRefL file"), QString(), QString (SCREFL_EXT));
    if(!FileName.isEmpty()) {
        ui->lineEditSCRefL->setText(FileName);
    }
}

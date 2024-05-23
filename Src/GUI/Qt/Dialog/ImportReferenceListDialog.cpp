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


#include "ImportReferenceListDialog.h"
#include "ui_ImportReferenceListDialog.h"

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

importscreflistdialog::importscreflistdialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::ImportReferenceListDialog)
{
    ui->setupUi(this);

    QString SCRefLFileName;
    QString CurProcess;

    // Read process name from INI file
    CurProcess = ScQt_IniFileDataBaseReadString ("BasicCalibrationSettings", "ApplProcessSCRefL",
                                                 "", ScQt_GetMainFileDescriptor());
    SCRefLFileName = ScQt_IniFileDataBaseReadString ("BasicCalibrationSettings", "SCRefL_Format",
                                                     "indexed", ScQt_GetMainFileDescriptor());

    char *ProcessName;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2);
    while ((ProcessName = read_next_process_name(Buffer)) != nullptr) {
        if(!IsInternalProcess(ProcessName)) {
            ui->listWidgetProcesses->addItem(ProcessName);
        }
    }
    close_read_next_process_name(Buffer);
    QListWidgetItem *ProcessItem = new QListWidgetItem;
    ProcessItem->setText(CurProcess);
    ui->listWidgetProcesses->setCurrentItem(ProcessItem);

    // If there are only one process select it
    if (ui->listWidgetProcesses->count() == 1) {
        ui->listWidgetProcesses->row(nullptr);
    }
}

importscreflistdialog::~importscreflistdialog()
{
    delete ui;
}

void importscreflistdialog::on_pushButtonSCRefL_clicked()
{
    QString fileName = FileDialog::getOpenFileName(this, QString ("Open SCRefL file"), QString(), QString (SCREFL_EXT));
    if(!fileName.isEmpty())
    {
        ui->lineEditSCRefL->setText(fileName);
    }
}

void importscreflistdialog::accept()
{
    QString SCRefLFileName;
    QString CurProcess;
    int idx;

    SCRefLFileName = ui->lineEditSCRefL->text();
    if (access(QStringToConstChar(SCRefLFileName), 0)) {
        ThrowError(MESSAGE_STOP, "No SCRefL-File selected !");
        return;
    }
    if (ui->listWidgetProcesses->count() == 0) {
        ThrowError(MESSAGE_STOP, "No Process selected !");
        return;
    } else {
        idx = ui->listWidgetProcesses->currentIndex().row();
        CurProcess = ui->listWidgetProcesses->item(idx)->text();
    }

    ScQt_IniFileDataBaseWriteString("BasicCalibrationSettings",
                                    "ApplProcessSCRefL", CurProcess, ScQt_GetMainFileDescriptor());

    ScQt_IniFileDataBaseWriteString("BasicCalibrationSettings",
                                    "SCRefLReadFileName", SCRefLFileName, ScQt_GetMainFileDescriptor());
    ImportVariableReferenceList(get_pid_by_name(QStringToConstChar(CurProcess)), QStringToConstChar(SCRefLFileName));
    QDialog::accept();
}

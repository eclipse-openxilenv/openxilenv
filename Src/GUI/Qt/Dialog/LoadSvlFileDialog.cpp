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


#include "LoadSvlFileDialog.h"
#include "ui_LoadSvlFileDialog.h"

#include "FileDialog.h"
#include "StringHelpers.h"

extern "C"
{
    #include "LoadSaveToFile.h"
    #include "IniDataBase.h"
    #include "Scheduler.h"
    #include "ThrowError.h"
    #include "FileExtensions.h"
}

loadsvlfiledialog::loadsvlfiledialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::loadsvlfiledialog)
{
    ui->setupUi(this);

    char *pname;
    char SvlFileName[MAX_PATH], CurProcess[MAX_PATH];

    // Process name from the INI file
    IniFileDataBaseReadString("BasicCalibrationSettings", "ApplProcessSvl",
                  "", CurProcess, sizeof(CurProcess), GetMainFileDescriptor());

    // List of all external processes
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((pname = read_next_process_name(Buffer)) != nullptr) {
        if (!IsInternalProcess (pname)) {
            QListWidgetItem *ProcessItem = new QListWidgetItem(pname);
            ui->listWidgetProcesses->addItem(ProcessItem);
            if (!Compare2ProcessNames(pname, CurProcess)) {
                ui->listWidgetProcesses->setCurrentItem(ProcessItem);
            }
        }
    }
    close_read_next_process_name(Buffer);

    // If ther is only one process select this
    if (ui->listWidgetProcesses->count() == 1) {
        ui->listWidgetProcesses->row(nullptr);
    }

    IniFileDataBaseReadString("BasicCalibrationSettings", "SvlReadFileName",
                  "*.svl", SvlFileName, sizeof(SvlFileName), GetMainFileDescriptor());

    ui->lineEditSVL->setText(SvlFileName);
}

loadsvlfiledialog::~loadsvlfiledialog()
{
    delete ui;
}

void loadsvlfiledialog::accept()
{
    char SvlFileName[MAX_PATH], CurProcess[MAX_PATH];
    int idx;

    strcpy(SvlFileName, QStringToConstChar(ui->lineEditSVL->text()));
    if (!file_exists(SvlFileName)) {
        ThrowError(MESSAGE_STOP, "No SVL-File selected !");
        return;
    }

    if (ui->listWidgetProcesses->count() == 0)
    {
        ThrowError(MESSAGE_STOP, "No Process selected !");
        return;
    }
    else {
        idx = ui->listWidgetProcesses->currentIndex().row();
        strcpy(CurProcess, QStringToConstChar(ui->listWidgetProcesses->item(idx)->text()));
    }

    IniFileDataBaseWriteString("BasicCalibrationSettings",
                       "ApplProcessSvl", CurProcess, GetMainFileDescriptor());

    IniFileDataBaseWriteString("BasicCalibrationSettings",
                     "SvlReadFileName", SvlFileName, GetMainFileDescriptor());

    int Err = WriteSVLToProcess(SvlFileName, CurProcess);
    if (Err > 0)
    {
        ThrowError(MESSAGE_STOP, "Load of SVL file was not complete successfull (%i incorrect lines or unknown labels)", Err);
    }
    QDialog::accept();
}

void loadsvlfiledialog::on_pushButtonSVL_clicked()
{
    QString fileName = FileDialog::getOpenFileName(this, QString ("Open SVL file"), QString(), QString (SVL_EXT));
    if(!fileName.isEmpty())
    {
        ui->lineEditSVL->setText(fileName);
    }
}

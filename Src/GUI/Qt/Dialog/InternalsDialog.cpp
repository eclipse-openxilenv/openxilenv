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


#include "InternalsDialog.h"
#include "ui_InternalsDialog.h"

#include <BlackboardInfoDialog.h>
#include "StringHelpers.h"

extern "C"
{
    #include "StringMaxChar.h"
    #include "PrintFormatToString.h"
    #include "MainValues.h"
    #include "Scheduler.h"
    #include "MyMemory.h"
    #include "Config.h"
    #include "Files.h"
    #include "IniDataBase.h"
    #include "RunTimeMeasurement.h"
}

InternalsDialog::InternalsDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::InternalsDialog)
{
    ui->setupUi(this);

    char openFiles[280];
    int openFilesItems;
    char *pname;

    openFilesItems = get_count_open_files_infos();

    for (int x = 0; x < openFilesItems; x++) {
        STRING_COPY_TO_ARRAY(openFiles, get_open_files_infos(x));
        ui->listWidgetOpenFiles->addItem(QString(openFiles));
    }
    char IniFileName[MAX_PATH];
    IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), IniFileName, sizeof(IniFileName));
    ui->lineEditINIFile->setText(QString(IniFileName));
#ifdef _WIN32
    ui->lineEditCommandLine->setText(QString(GetCommandLine()));
#else
#endif

    FillOpenIniFile();

    ui->listWidgetProcess->addItem("all");
    // walk through all prozesses
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2);
    while ((pname = read_next_process_name(Buffer)) != nullptr) {
        ui->listWidgetProcess->addItem(QString(pname));
    }
    close_read_next_process_name(Buffer);

    // select first process
    ui->listWidgetProcess->item(0)->setSelected(true);


    get_memory_infos(0);


#ifdef FUNCTION_RUNTIME
    {
        int id = 0;
        char line[1024];
        while (1) {
            id = GetRuntimeMeassurement (id, line);
            if (id == 0) break;
            ui->listWidgetFunctionRuntime->addItem(QString(line));
        }
    }
#endif
}

InternalsDialog::~InternalsDialog()
{
    delete ui;
}

void InternalsDialog::get_memory_infos(int pid)
{
    struct memory_cblock *cblock, *nextblock;
    long total_mem = 0;
    char buffer[1024];
    FILE *fh = nullptr;
    ui->listWidgetMemory->clear();

    for (cblock = memory_block_list; cblock != nullptr; cblock = nextblock) {
        nextblock = cblock->next;
        if ((pid == 0) || (cblock->pid == pid)) {
            PrintFormatToString (buffer, sizeof(buffer), "(%i) %p[%u] %s[%i]", cblock->pid, cblock->block, cblock->size, cblock->file, cblock->line);
            if (fh != nullptr) fprintf (fh, "(%i) %p[%u] %s[%i]\n", cblock->pid, cblock->block, cblock->size, cblock->file, cblock->line);
            ui->listWidgetMemory->addItem(QString(buffer));
            total_mem += cblock->size;
        }
    }
    ui->lineEditTotalBytes->setText(QString("%1").arg(total_mem));
    if (fh != nullptr) fclose (fh);
}

void InternalsDialog::on_listWidgetProcess_currentRowChanged(int currentRow)
{
    int pid;
    char pName[MAX_PATH];
    STRING_COPY_TO_ARRAY(pName, QStringToConstChar(ui->listWidgetProcess->item(currentRow)->text()));
    if(!strcmp("all", pName)) pid = 0;
    else pid = get_pid_by_name(pName);
    get_memory_infos(pid);
}

void InternalsDialog::on_pushButtonBBInfos_clicked()
{
    BlackboardInfoDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void InternalsDialog::on_pushButtonClose_clicked()
{
    QDialog::accept();
}

void InternalsDialog::FillOpenIniFile()
{
    char Filename[MAX_PATH];
    // We know the file descriptors are in the rang 256...(256+15)
    for (int x = 256; x < (256+16); x++) {
        if (IniFileDataBaseGetFileNameByDescriptor(x, Filename, sizeof(Filename))) {
            STRING_COPY_TO_ARRAY(Filename, "not used");
        }
        ui->listWidgetOpenIniFiles->addItem(QString(Filename));
    }
}

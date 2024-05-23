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


#include "LoadXcpConfigDialog.h"
#include "ui_LoadXcpConfigDialog.h"

#include <QMessageBox>
#include "FileDialog.h"
#include "StringHelpers.h"

extern"C"
{
    #include "XcpControl.h"
    #include "ThrowError.h"
    #include "FileExtensions.h"
}

LoadXCPConfigDialog::LoadXCPConfigDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::LoadXCPConfigDialog)
{
    ui->setupUi(this);
    ui->radioButtonCon0->setChecked(true);
}

LoadXCPConfigDialog::~LoadXCPConfigDialog()
{
    delete ui;
}

void LoadXCPConfigDialog::accept()
{
    char fileName[MAX_PATH];
    int i;
    if (ui->lineEditBrowse->text().isEmpty())
    {
        QMessageBox msgWarnBox;
        msgWarnBox.setIcon(QMessageBox::Warning);
        msgWarnBox.setText(QString("No file selected!"));
        msgWarnBox.setInformativeText("Please select a file!");
        msgWarnBox.exec();
        return;
    }
    if (ui->radioButtonCon0->isChecked())
    {
        i = 0;
    } else if (ui->radioButtonCon1->isChecked()) {
        i = 1;
    } else if (ui->radioButtonCon2->isChecked()) {
        i = 2;
    } else if (ui->radioButtonCon3->isChecked()) {
        i = 3;
    } else {
        ThrowError(1, "Unhandled Error");
        return;
    }
    strcpy(fileName, QStringToConstChar(ui->lineEditBrowse->text()));
    LoadConfig_XCP(i, fileName);
    QDialog::accept();
}

void LoadXCPConfigDialog::on_pushButtonBrowse_clicked()
{
    ui->lineEditBrowse->setText(FileDialog::getOpenFileName(this, QString ("Open XCP file"), QString(), QString (XCP_EXT)));
}

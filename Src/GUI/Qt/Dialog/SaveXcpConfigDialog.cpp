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


#include "SaveXcpConfigDialog.h"
#include "ui_SaveXcpConfigDialog.h"

#include <QMessageBox>
#include "FileDialog.h"
#include "StringHelpers.h"

extern"C"
{
    #include "StringMaxChar.h"
    #include "XcpControl.h"
    #include "ThrowError.h"
    #include "FileExtensions.h"
}

SaveXCPConfigDialog::SaveXCPConfigDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::SaveXCPConfigDialog)
{
    ui->setupUi(this);
    ui->radioButtonCon0->setChecked(true);
}

SaveXCPConfigDialog::~SaveXCPConfigDialog()
{
    delete ui;
}

void SaveXCPConfigDialog::accept()
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
    STRING_COPY_TO_ARRAY(fileName, QStringToConstChar(ui->lineEditBrowse->text()));
    SaveConfig_XCP(i, fileName);
    QDialog::accept();
}

void SaveXCPConfigDialog::on_pushButtonBrowse_clicked()
{
    ui->lineEditBrowse->setText(FileDialog::getSaveFileName(this, QString ("Save XCP file"), QString(), QString (XCP_EXT)));
}

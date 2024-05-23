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


#include "A2LCalMapConfigDlg.h"
#include "ui_A2LCalMapConfigDlg.h"

#include "WindowNameAlreadyInUse.h"


extern "C" {
#include "Scheduler.h"
#include "ThrowError.h"
#include "A2LLink.h"
}

A2LCalMapConfigDlg::A2LCalMapConfigDlg (QString &par_WindowTitle,
                                        QString &par_ProcessName,
                                        QStringList &par_Characteristics,
                                        QWidget *parent) : Dialog(parent),
    ui(new Ui::A2LCalMapConfigDlg)
{
    ui->setupUi(this);
    ui->WindowTitleLineEdit->setText(par_WindowTitle);
    ui->A2LSelect->SetFilerVisable(true);
    ui->A2LSelect->SetReferenceButtonsVisable(false);
    ui->A2LSelect->SetVisable(true, A2L_LABEL_TYPE_VAL_BLK_CALIBRATION  |
                                    A2L_LABEL_TYPE_CURVE_CALIBRATION    |
                                    A2L_LABEL_TYPE_MAP_CALIBRATION);
    ui->A2LSelect->SetProcess(par_ProcessName);  // this must call behind SetVisable
    ui->A2LSelect->SetSelected(par_Characteristics);  // this must call behind SetVisable
    ui->WindowTitleLineEdit->setText (par_WindowTitle);
}

A2LCalMapConfigDlg::~A2LCalMapConfigDlg()
{
    delete ui;
}

QString A2LCalMapConfigDlg::GetWindowTitle()
{
    return ui->WindowTitleLineEdit->text();
}

QStringList A2LCalMapConfigDlg::GetCharacteristics()
{
    return ui->A2LSelect->GetSelected();
}

QString A2LCalMapConfigDlg::GetProcess()
{
    return ui->A2LSelect->GetProcess();
}




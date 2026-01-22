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


#include "EditRationalFunctionDialog.h"
#include "ui_EditRationalFunctionDialog.h"


EditRationalFunctionDialog::EditRationalFunctionDialog (QString &par_RarameterString,
                                                       QWidget *parent) : Dialog(parent),
    ui(new Ui::EditRationalFunctionDialog)
{
    ui->setupUi(this);
    QStringList Paramters = par_RarameterString.split(':');
    if (Paramters.count() == 6) {
        ui->ParameterA->SetValue(Paramters.at(0).toDouble());
        ui->ParameterB->SetValue(Paramters.at(1).toDouble());
        ui->ParameterC->SetValue(Paramters.at(2).toDouble());
        ui->ParameterD->SetValue(Paramters.at(3).toDouble());
        ui->ParameterE->SetValue(Paramters.at(4).toDouble());
        ui->ParameterF->SetValue(Paramters.at(5).toDouble());
    }
}

EditRationalFunctionDialog::~EditRationalFunctionDialog()
{
    delete ui;
}

void EditRationalFunctionDialog::accept()
{
    QDialog::accept();
}

QString EditRationalFunctionDialog::GetModifiedRationalFunctionString()
{
    QString Ret;
    Ret = ui->ParameterA->text();
    Ret.append(":");
    Ret.append(ui->ParameterB->text());
    Ret.append(":");
    Ret.append(ui->ParameterC->text());
    Ret.append(":");
    Ret.append(ui->ParameterD->text());
    Ret.append(":");
    Ret.append(ui->ParameterE->text());
    Ret.append(":");
    Ret.append(ui->ParameterF->text());
    return Ret;
}




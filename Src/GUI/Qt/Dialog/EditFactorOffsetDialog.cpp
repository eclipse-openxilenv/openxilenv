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


#include "EditFactorOffsetDialog.h"
#include "ui_EditFactorOffsetDialog.h"


EditFactorOffsetDialog::EditFactorOffsetDialog (QString &par_RarameterString, enum TypeEnum par_Type,
                                                       QWidget *parent) : Dialog(parent),
    ui(new Ui::EditFactorOffsetDialog)
{
    ui->setupUi(this);
    if (par_Type == FACTOR_OFFSET_TYPE) {
        ui->FormulaLabel->setText("ax + b");
    } else {
        ui->FormulaLabel->setText("a * (x + b)");
    }
    QStringList Paramters = par_RarameterString.split(':');
    if (Paramters.count() == 2) {
        ui->ParameterA->SetValue(Paramters.at(0).toDouble());
        ui->ParameterB->SetValue(Paramters.at(1).toDouble());
    }
}

EditFactorOffsetDialog::~EditFactorOffsetDialog()
{
    delete ui;
}

void EditFactorOffsetDialog::accept()
{
    QDialog::accept();
}

QString EditFactorOffsetDialog::GetModifiedFactorOffsetString()
{
    QString Ret;
    Ret = ui->ParameterA->text();
    Ret.append(":");
    Ret.append(ui->ParameterB->text());
    return Ret;
}




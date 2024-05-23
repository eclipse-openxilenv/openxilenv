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


#include "AddNewProcessSchedulerBarrier.h"
#include "ui_AddNewProcessSchedulerBarrier.h"

#include <QRegularExpressionValidator>

AddNewProcessSchedulerBarrier::AddNewProcessSchedulerBarrier(QString WhatToAdd, bool WaitIfAloneVisable, QWidget *parent) : Dialog(parent),
    ui(new Ui::AddNewProcessSchedulerBarrier)
{
    ui->setupUi(this);
    this->setWindowTitle (QString ("Add new %1").arg (WhatToAdd));
    ui->label->setText(QString ("Add new %1").arg(WhatToAdd));
    QRegularExpressionValidator *Validator = new QRegularExpressionValidator;
    Validator->setRegularExpression(QRegularExpression("^\\S+$"));       // match strings without whitespace
    ui->lineEdit->setValidator(Validator);
    if (!WaitIfAloneVisable) {
        ui->checkBox->hide();
    }
}

AddNewProcessSchedulerBarrier::~AddNewProcessSchedulerBarrier()
{
    delete ui;
}

QString AddNewProcessSchedulerBarrier::GetNew()
{
    return ui->lineEdit->text();
}

bool AddNewProcessSchedulerBarrier::GetWaitIfAlone()
{
    return ui->checkBox->isChecked();
}

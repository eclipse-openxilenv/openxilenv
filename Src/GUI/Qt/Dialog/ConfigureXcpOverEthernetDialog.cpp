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


#include "ConfigureXcpOverEthernetDialog.h"
#include "ui_ConfigureXcpOverEthernetDialog.h"

ConfigureXCPOverEthernetDialog::ConfigureXCPOverEthernetDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::ConfigureXCPOverEthernetDialog)
{
    ui->setupUi(this);
}

ConfigureXCPOverEthernetDialog::~ConfigureXCPOverEthernetDialog()
{
    delete ui;
}

void ConfigureXCPOverEthernetDialog::on_buttonBox_accepted()
{
    ui->tabConnection_0->saveXCPConfigData();
    ui->tabConnection_1->saveXCPConfigData();
    ui->tabConnection_2->saveXCPConfigData();
    ui->tabConnection_3->saveXCPConfigData();

    ui->tabConnection_0->~ConfigureXCPOverEthernetWidget();
    ui->tabConnection_1->~ConfigureXCPOverEthernetWidget();
    ui->tabConnection_2->~ConfigureXCPOverEthernetWidget();
    ui->tabConnection_3->~ConfigureXCPOverEthernetWidget();

    QDialog::accept();
}

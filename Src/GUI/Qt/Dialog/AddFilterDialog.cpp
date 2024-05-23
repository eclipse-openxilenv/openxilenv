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


#include "AddFilterDialog.h"
#include "ui_AddFilterDialog.h"

#include <GlobalSectionFilterDialog.h>

addFilterDialog::addFilterDialog(bool includeExclude, QWidget *parent) : Dialog(parent),
    includeExclude(includeExclude),
    ui(new Ui::addFilterDialog)
{
    ui->setupUi(this);
    if (!(ui->lineEditFilter->text().isEmpty()))
    {
        filterName = ui->lineEditFilter->text();
    } else {
        filterName = "";
    }
}

addFilterDialog::~addFilterDialog()
{
    delete ui;
}

void addFilterDialog::accept()
{
    if (!(ui->lineEditFilter->text().isEmpty()))
    {
        filterName = ui->lineEditFilter->text();
    } else return;
    QDialog::accept();
}

QString addFilterDialog::getFilterName()
{
    return filterName;
}

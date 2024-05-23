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


#include "CanConfigJ1939AdditionalsDialog.h"
#include "ui_CanConfigJ1939AdditionalsDialog.h"

#include "CanConfigDialog.h"

CanConfigJ1939AdditionalsDialog::CanConfigJ1939AdditionalsDialog(CanConfigObject *par_CanObject, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CanConfigJ1939AdditionalsDialog)
{
    ui->setupUi(this);
    m_CanObject = par_CanObject;
    ui->VariableDlcCheckBox->setChecked(par_CanObject->m_VariableDlc);
    if (par_CanObject->m_VariableDlc) {
        ui->VariableDlcLineEdit->setText(par_CanObject->m_VariableDlcVariableName);
    } else {
        ui->VariableDlcLineEdit->setEnabled(false);
    }
    ui->VariableDlclInitValueLineEdit->setText(QString().number(par_CanObject->GetSize()));
    switch (par_CanObject->m_DestAddressType) {
    case CanConfigObject::Fixed:
        ui->DestAddrFixedRadioButton->setChecked(true);
        ui->DestAddrFixedLineEdit->SetValue(par_CanObject->m_FixedDestAddress, 16);
        ui->DestAddrVariableLineEdit->setEnabled((false));
        ui->DestAddrVariableInitValueLineEdit->setEnabled(false);
        break;
    case CanConfigObject::Variable:
        ui->DestAddrVariableRadioButton->setChecked(true);
        ui->DestAddrFixedLineEdit->setEnabled(false);
        ui->DestAddrVariableLineEdit->setText(par_CanObject->m_VariableDestAddressVariableName);
        ui->DestAddrVariableInitValueLineEdit->SetValue(par_CanObject->m_VariableDestAddressInitValue, 16);
        break;
    }
}


void CanConfigJ1939AdditionalsDialog::accept()
{
    m_CanObject->m_VariableDlc = ui->VariableDlcCheckBox->isChecked();
    if (m_CanObject->m_VariableDlc) {
        m_CanObject->m_VariableDlcVariableName = ui->VariableDlcLineEdit->text();
    }
    if (ui->DestAddrFixedRadioButton->isChecked()) {
        m_CanObject->m_DestAddressType = CanConfigObject::Fixed;
        m_CanObject->m_FixedDestAddress = ui->DestAddrFixedLineEdit->GetUIntValue();
        m_CanObject->m_VariableDestAddressVariableName.clear();
        m_CanObject->m_VariableDestAddressInitValue = 0;
    } else {
        m_CanObject->m_DestAddressType = CanConfigObject::Variable;
        m_CanObject->m_VariableDestAddressVariableName = ui->DestAddrVariableLineEdit->text();
        m_CanObject->m_VariableDestAddressInitValue = ui->DestAddrVariableInitValueLineEdit->GetUIntValue();
        m_CanObject->m_FixedDestAddress = 0;
    }
    QDialog::accept();
}


CanConfigJ1939AdditionalsDialog::~CanConfigJ1939AdditionalsDialog()
{
    delete ui;
}


void CanConfigJ1939AdditionalsDialog::on_VariableDlcCheckBox_toggled(bool checked)
{
    ui->VariableDlcLineEdit->setEnabled(checked);
}

void CanConfigJ1939AdditionalsDialog::on_DestAddrFixedRadioButton_toggled(bool checked)
{
    ui->DestAddrFixedLineEdit->setEnabled(checked);
}

void CanConfigJ1939AdditionalsDialog::on_DestAddrVariableRadioButton_toggled(bool checked)
{
    ui->DestAddrVariableLineEdit->setEnabled((checked));
    ui->DestAddrVariableInitValueLineEdit->setEnabled(checked);
}

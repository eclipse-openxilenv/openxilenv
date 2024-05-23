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


#include "ConfigSignalConversionDialog.h"
#include "ui_ConfigSignalConversionDialog.h"

ConfigSignalConversionDialog::ConfigSignalConversionDialog(QString par_SignalType, QString par_Unit, QWidget *parent) : Dialog(parent),
    ui(new Ui::ConfigSignalConversionDialog)
{
    ui->setupUi(this);
    setWindowTitle("Configure " + par_SignalType + " conversion");
    ui->ConversionGroupBox->setTitle(par_SignalType + QString(" conversion"));
    ui->RangeGroupBox->setTitle(par_SignalType + QString(" range"));
    ui->MinTypeNameLabel->setText(par_SignalType + QString(" min"));
    ui->MaxTypeNameLabel->setText(par_SignalType + QString(" max"));
    ui->MinTypeUnitLabel->setText(par_Unit);
    ui->MaxTypeUnitLabel->setText(par_Unit);
}

ConfigSignalConversionDialog::~ConfigSignalConversionDialog()
{
    delete ui;
}

void ConfigSignalConversionDialog::SetFacOff(double par_Factor, double par_Offset)
{
    ui->FacOffRadioButton->setChecked(true);
    ui->FactorValueInput->SetValue(par_Factor);
    ui->OffsetValueInput->SetValue(par_Offset);
}

void ConfigSignalConversionDialog::SetCurce(QString &par_Curve)
{
    ui->CurveRadioButton->setChecked(true);
    ui->CurveLineEdit->setText(par_Curve);
}

void ConfigSignalConversionDialog::SetEquation(QString &par_Equation)
{
    ui->EquationRadioButton->setChecked(true);
    ui->EquationLineEdit->setText(par_Equation);
}

void ConfigSignalConversionDialog::SetMinMax(double par_Min, double par_Max)
{
    ui->MinValueInput->SetValue(par_Min);
    ui->MaxValueInput->SetValue(par_Max);
}

int ConfigSignalConversionDialog::GetConversionType()
{
    if (ui->FacOffRadioButton->isChecked()) return 0;
    if (ui->EquationRadioButton->isChecked()) return 1;
    if (ui->CurveRadioButton->isChecked()) return 2;
    return 0;
}

double ConfigSignalConversionDialog::GetMin()
{
    return ui->MinValueInput->GetDoubleValue();
}

double ConfigSignalConversionDialog::GetMax()
{
    return ui->MaxValueInput->GetDoubleValue();
}

double ConfigSignalConversionDialog::GetFactor()
{
    return ui->FactorValueInput->GetDoubleValue();
}

double ConfigSignalConversionDialog::GetOffset()
{
    return ui->OffsetValueInput->GetDoubleValue();
}

QString ConfigSignalConversionDialog::GetCurve()
{
    return ui->CurveLineEdit->text();
}

QString ConfigSignalConversionDialog::GetEquation()
{
    return ui->EquationLineEdit->text();
}

void ConfigSignalConversionDialog::on_FacOffRadioButton_toggled(bool checked)
{
    ui->FactorValueInput->setEnabled(checked);
    ui->OffsetValueInput->setEnabled(checked);
}

void ConfigSignalConversionDialog::on_CurveRadioButton_toggled(bool checked)
{
    ui->CurveLineEdit->setEnabled(checked);
}

void ConfigSignalConversionDialog::on_EquationRadioButton_toggled(bool checked)
{
    ui->EquationLineEdit->setEnabled(checked);
}

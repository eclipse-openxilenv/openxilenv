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


#include "TextWindowChangeValueDialog.h"
#include "ui_TextWindowChangeValueDialog.h"

extern "C" {
#include "Blackboard.h"
}

TextWindowChangeValueDialog::TextWindowChangeValueDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::TextWindowChangeValueDialog)
{
    ui->setupUi(this);
    ui->d_ValueInput->SetPlusMinusButtonEnable(true);
    ui->d_ValueInput->SetHeaderState(true);

    ui->d_ValueInput->setFocus();

    ui->d_ValueInput->SetWideningSignalEnable(true);
    connect (ui->d_ValueInput, SIGNAL(ShouldWideningSignal(int,int)), this, SLOT(ShouldWideningSlot(int,int)));
}

TextWindowChangeValueDialog::~TextWindowChangeValueDialog()
{
    delete ui;
}

void TextWindowChangeValueDialog::setVariableName(QString arg_name)
{
    ui->d_labelVariableName->setText(arg_name);
}

QString TextWindowChangeValueDialog::variableName()
{
    return ui->d_labelVariableName->text();
}

void TextWindowChangeValueDialog::setDisplaytype(TextWindowChangeValueDialog::Displaytype arg_displaytype)
{
    switch (arg_displaytype) {
    case Displaytype::binary:
        ui->d_radioButtonBinary->setChecked(true);
        ui->d_ValueInput->SetDisplayRawValue(true);
        ui->d_ValueInput->SetDisplayPhysValue(false);
        ui->d_ValueInput->SetDispayFormat(2);
        break;
    case Displaytype::decimal:
    //default:
        ui->d_radioButtonDecimal->setChecked(true);
        ui->d_ValueInput->SetDisplayRawValue(true);
        ui->d_ValueInput->SetDisplayPhysValue(false);
        ui->d_ValueInput->SetDispayFormat(10);
        break;
    case Displaytype::hexadecimal:
        ui->d_radioButtonHexadecimal->setChecked(true);
        ui->d_ValueInput->SetDisplayRawValue(true);
        ui->d_ValueInput->SetDisplayPhysValue(false);
        ui->d_ValueInput->SetDispayFormat(16);
        break;
    case Displaytype::physically:
        ui->d_radioButtonPhysically->setChecked(true);
        ui->d_ValueInput->SetDisplayRawValue(true);
        ui->d_ValueInput->SetDisplayPhysValue(true);
        break;
    }
}

TextWindowChangeValueDialog::Displaytype TextWindowChangeValueDialog::displaytype()
{
    if(ui->d_radioButtonBinary->isChecked()) {
        return Displaytype::binary;
    }
    if(ui->d_radioButtonDecimal->isChecked()) {
        return Displaytype::decimal;
    }
    if(ui->d_radioButtonHexadecimal->isChecked()) {
        return Displaytype::hexadecimal;
    }
    if(ui->d_radioButtonPhysically->isChecked()) {
        return Displaytype::physically;
    }
    return Displaytype::decimal;
}

void TextWindowChangeValueDialog::setLinear(bool arg_linear)
{
    if(arg_linear) {
        ui->d_radioButtonLinear->setChecked(true);
        ui->d_ValueInput->SetStepLinear(true);
    } else {
        ui->d_radioButtonPerCent->setChecked(true);
        ui->d_ValueInput->SetStepPercentage(true);
    }
}

bool TextWindowChangeValueDialog::isLinear()
{
    if(ui->d_radioButtonLinear->isChecked()) {
        return true;
    } else {
        return false;
    }
}

void TextWindowChangeValueDialog::setStep(double arg_step)
{
    ui->d_lineEditStep->SetRawValue(arg_step);
    ui->d_ValueInput->SetPlusMinusIncrement(arg_step);
}

double TextWindowChangeValueDialog::step()
{
    return ui->d_lineEditStep->GetDoubleRawValue();
}

void TextWindowChangeValueDialog::setValue(double arg_value)
{
    ui->d_ValueInput->SetRawValue(arg_value);
}

void TextWindowChangeValueDialog::setValue(union BB_VARI arg_value, enum BB_DATA_TYPES par_Type, int par_Base)
{
    ui->d_ValueInput->SetRawValue(arg_value, par_Type, par_Base);
}

double TextWindowChangeValueDialog::value()
{
    return ui->d_ValueInput->GetDoubleRawValue();
}

BB_DATA_TYPES TextWindowChangeValueDialog::value(BB_VARI *ret_Value)
{
    bool Ok;
    return ui->d_ValueInput->GetUnionRawValue(ret_Value, &Ok);
}

void TextWindowChangeValueDialog::SetVid(int par_Vid)
{
    int ConvType = get_bbvari_conversiontype(par_Vid);
    // If no equation or enum than...
    if ((ConvType != BB_CONV_FORMULA) && (ConvType != BB_CONV_TEXTREP)) {
        ui->d_radioButtonPhysically->setEnabled(false);
    }
    ui->d_ValueInput->SetBlackboardVariableId(par_Vid);

}

void TextWindowChangeValueDialog::on_d_radioButtonDecimal_toggled(bool checked)
{
    if (checked) {
        ui->d_ValueInput->SetDisplayRawValue(true);
        ui->d_ValueInput->SetDisplayPhysValue(false);
        ui->d_ValueInput->SetDispayFormat(10);
    }
}

void TextWindowChangeValueDialog::on_d_radioButtonHexadecimal_toggled(bool checked)
{
    if (checked) {
        ui->d_ValueInput->SetDisplayRawValue(true);
        ui->d_ValueInput->SetDisplayPhysValue(false);
        ui->d_ValueInput->SetDispayFormat(16);
    }
}

void TextWindowChangeValueDialog::on_d_radioButtonBinary_toggled(bool checked)
{
    if (checked) {
        ui->d_ValueInput->SetDisplayRawValue(true);
        ui->d_ValueInput->SetDisplayPhysValue(false);
        ui->d_ValueInput->SetDispayFormat(2);
    }
}

void TextWindowChangeValueDialog::on_d_radioButtonPhysically_toggled(bool checked)
{
    if (checked) {
        ui->d_ValueInput->SetDisplayRawValue(true);
        ui->d_ValueInput->SetDisplayPhysValue(true);
    }
}

void TextWindowChangeValueDialog::on_d_lineEditStep_ValueChanged()
{
    double Step = ui->d_lineEditStep->GetDoubleRawValue();
    ui->d_ValueInput->SetPlusMinusIncrement(Step);
}

void TextWindowChangeValueDialog::on_d_radioButtonLinear_toggled(bool checked)
{
    if (checked) {
        ui->d_ValueInput->SetStepLinear(true);
    }
}

void TextWindowChangeValueDialog::on_d_radioButtonPerCent_toggled(bool checked)
{
    if (checked) {
        ui->d_ValueInput->SetStepPercentage(true);
    }
}

void TextWindowChangeValueDialog::ShouldWideningSlot(int par_Widening, int par_Heightening)
{
    resize(width() + par_Widening, height() + par_Heightening);
}

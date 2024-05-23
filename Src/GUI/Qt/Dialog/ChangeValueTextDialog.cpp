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


#include "ChangeValueTextDialog.h"
#include "ui_ChangeValueTextDialog.h"
extern "C" {
#include "ThrowError.h"
}

ChangeValueTextDialog::ChangeValueTextDialog(QWidget *parent, QString par_Header, int par_DataType, bool par_PhysicalFlag, const char *par_Converion) : Dialog(parent),
    ui(new Ui::ChangeValueTextDialog)
{
    ui->setupUi(this);
    if (par_Header.size()) ui->label->setText(par_Header);
    ui->ValueEdit->setFocus();
    ui->ValueEdit->SetDisplayRawValue(true);
    if ((par_DataType >= BB_BYTE) && (par_DataType <= BB_UDWORD)) {
        ui->ValueEdit->SetRawOnlyInt(true);
    }
    ui->ValueEdit->SetHeaderState(true);
    if (par_PhysicalFlag) {
        ui->ValueEdit->SetDisplayPhysValue(true);
    }
    ui->ValueEdit->SetPlusMinusButtonEnable(true);
    ui->ValueEdit->SetStepLinear(true);
    ui->ValueEdit->SetPlusMinusIncrement(1.0);
    if (par_Converion != nullptr) {
        ui->ValueEdit->SetFormulaString(par_Converion);
    }
}

ChangeValueTextDialog::~ChangeValueTextDialog()
{
    delete ui;
}

void ChangeValueTextDialog::SetValue (int32_t par_Value, int par_Base)
{
    ui->ValueEdit->SetRawValue (par_Value, par_Base);
}

void ChangeValueTextDialog::SetValue (uint32_t par_Value, int par_Base)
{
    ui->ValueEdit->SetRawValue (par_Value, par_Base);
}

void ChangeValueTextDialog::SetValue(int64_t par_Value, int par_Base)
{
    ui->ValueEdit->SetRawValue (par_Value, par_Base);
}

void ChangeValueTextDialog::SetValue(uint64_t par_Value, int par_Base)
{
    ui->ValueEdit->SetRawValue (par_Value, par_Base);
}

void ChangeValueTextDialog::SetValue (double par_Value)
{
    ui->ValueEdit->SetRawValue (par_Value);
}

double ChangeValueTextDialog::GetDoubleValue()
{
    return ui->ValueEdit->GetDoubleRawValue();
}

long long ChangeValueTextDialog::GetInt64Value()
{
     return ui->ValueEdit->GetInt64RawValue();
}

unsigned long long ChangeValueTextDialog::GetUInt64Value()
{
    return ui->ValueEdit->GetUInt64RawValue();
}

int ChangeValueTextDialog::GetIntValue()
{
    return ui->ValueEdit->GetIntRawValue();
}

void ChangeValueTextDialog::SetMinMaxValues (double par_MinValue, double par_MaxValue)
{
    ui->ValueEdit->SetRawMinMaxValue(par_MinValue, par_MaxValue);
}

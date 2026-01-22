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


#include <inttypes.h>

#include "A2LCalSinglePropertiesDlg.h"
#include "ui_A2LCalSinglePropertiesDlg.h"
#include "StringHelpers.h"

extern "C" {
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
}

static void InitConTypeComboBox(QComboBox *par_ComboBox)
{
    par_ComboBox->addItem ("none");  // BB_CONV_NONE
    par_ComboBox->addItem ("formula");  // BB_CONV_FORMULA
    par_ComboBox->addItem ("text replace");  // BB_CONV_TEXTREP
    par_ComboBox->addItem ("# * factor + offset");  // BB_CONV_FACTOFF
    par_ComboBox->addItem ("factor * (# + offset)");  // BB_CONV_OFFFACT
    par_ComboBox->addItem ("table with interpolation");  // BB_CONV_TAB_INTP
    par_ComboBox->addItem ("table without interpolation");  // BB_CONV_TAB_NOINTP
    par_ComboBox->addItem ("fractional rational function");  // BB_CONV_RAT_FUNC
    par_ComboBox->addItem ("reference");  // BB_CONV_REF
}

A2LCalSinglePropertiesDlg::A2LCalSinglePropertiesDlg(QString par_Label,
                                                     uint64_t par_Address,
                                                     CHARACTERISTIC_AXIS_INFO *par_Info,
                                                     QWidget *parent) : Dialog(parent),
    ui(new Ui::A2LCalSinglePropertiesDlg)
{
    ui->setupUi(this);

    InitConTypeComboBox(ui->ConvTypeComboBox);
    ui->LabelLineEdit->setText(par_Label);
    char Help[32];
    PrintFormatToString (Help, sizeof(Help), "0x%" PRIX64 "", par_Address);
    ui->AddressLineEdit->setText(QString(Help));
    ui->UnitLineEdit->setText(par_Info->m_Unit);
    ui->RawMinLineEdit->SetValue(par_Info->m_RawMin);
    ui->RawMaxLineEdit->SetValue(par_Info->m_RawMax);
    ui->PhysMinLineEdit->SetValue(par_Info->m_Min);
    ui->PhysMaxLineEdit->SetValue(par_Info->m_Max);
    ui->ConvTypeComboBox->setCurrentIndex(par_Info->m_ConvType);
    ui->ConversionLineEdit->setText(par_Info->m_Conversion);
    ui->FormatLayoutLineEdit->SetValue(par_Info->m_FormatLayout);
    ui->FormatLengthLineEdit->SetValue(par_Info->m_FormatLength);
}

A2LCalSinglePropertiesDlg::~A2LCalSinglePropertiesDlg()
{
    delete ui;
}

static char *ReallocCopy(char *DstPtrQString, QString par_String)
{
    char *Ret = StringRealloc(DstPtrQString, QStringToConstChar(par_String));
    return Ret;
}

void A2LCalSinglePropertiesDlg::Get(CHARACTERISTIC_AXIS_INFO *ret_Info)
{
    ret_Info->m_Unit = ReallocCopy(ret_Info->m_Unit, ui->UnitLineEdit->text());
    ret_Info->m_Min = ui->PhysMinLineEdit->GetDoubleValue();
    ret_Info->m_Max = ui->PhysMaxLineEdit->GetDoubleValue();
    ret_Info->m_RawMin = ui->RawMinLineEdit->GetDoubleValue();
    ret_Info->m_RawMax = ui->RawMaxLineEdit->GetDoubleValue();
    ret_Info->m_ConvType = ui->ConvTypeComboBox->currentIndex();
    ret_Info->m_Conversion = ReallocCopy(ret_Info->m_Conversion, ui->ConversionLineEdit->text());
    ret_Info->m_FormatLayout = ui->FormatLayoutLineEdit->GetUIntValue();
    ret_Info->m_FormatLength = ui->FormatLengthLineEdit->GetUIntValue();
}

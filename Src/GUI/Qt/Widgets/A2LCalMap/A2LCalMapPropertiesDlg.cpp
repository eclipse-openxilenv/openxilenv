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

#include "A2LCalMapPropertiesDlg.h"
#include "ui_A2LCalMapPropertiesDlg.h"

#include "StringHelpers.h"


extern "C" {
#include "MyMemory.h"
}

static void InitConTypeComboBox(QComboBox *par_ComboBox)
{
    par_ComboBox->addItem("none");
    par_ComboBox->addItem("formula");
    par_ComboBox->addItem("text replace");
}

A2LCalMapPropertiesDlg::A2LCalMapPropertiesDlg(QString par_Label,
                                               int par_Type, uint64_t par_Address,
                                               CHARACTERISTIC_AXIS_INFO *par_XAxisInfo,
                                               CHARACTERISTIC_AXIS_INFO *par_YAxisInfo,
                                               CHARACTERISTIC_AXIS_INFO *par_ZAxisInfo,
                                               QWidget *parent) : Dialog(parent),
    ui(new Ui::A2LCalMapPropertiesDlg)
{
    ui->setupUi(this);
    m_Type = par_Type;

    InitConTypeComboBox(ui->XConvTypeComboBox);
    InitConTypeComboBox(ui->YConvTypeComboBox);
    InitConTypeComboBox(ui->MapConvTypeComboBox);
    ui->LabelLineEdit->setText(par_Label);
    char Help[32];
    sprintf(Help, "0x%" PRIX64 "", par_Address);
    ui->AddressLineEdit->setText(QString(Help));
    // X axis
    ui->XInputSignalLineEdit->setText(par_XAxisInfo->m_Input);
    ui->XUnitLineEdit->setText(par_XAxisInfo->m_Unit);
    ui->XRawMinLineEdit->SetValue(par_XAxisInfo->m_RawMin);
    ui->XRawMaxLineEdit->SetValue(par_XAxisInfo->m_RawMax);
    ui->XPhysMinLineEdit->SetValue(par_XAxisInfo->m_Min);
    ui->XPhysMaxLineEdit->SetValue(par_XAxisInfo->m_Max);
    ui->XConvTypeComboBox->setCurrentIndex(par_XAxisInfo->m_ConvType);
    ui->XConversionLineEdit->setText(par_XAxisInfo->m_Conversion);
    ui->XFormatLayoutLineEdit->SetValue(par_XAxisInfo->m_FormatLayout);
    ui->XFormatLengthLineEdit->SetValue(par_XAxisInfo->m_FormatLength);
    // Y axis
    if (m_Type >= 2) { // 2 -> Map
        ui->YInputSignalLineEdit->setText(par_YAxisInfo->m_Input);
        ui->YUnitLineEdit->setText(par_YAxisInfo->m_Unit);
        ui->YRawMinLineEdit->SetValue(par_YAxisInfo->m_RawMin);
        ui->YRawMaxLineEdit->SetValue(par_YAxisInfo->m_RawMax);
        ui->YPhysMinLineEdit->SetValue(par_YAxisInfo->m_Min);
        ui->YPhysMaxLineEdit->SetValue(par_YAxisInfo->m_Max);
        ui->YConvTypeComboBox->setCurrentIndex(par_YAxisInfo->m_ConvType);
        ui->YConversionLineEdit->setText(par_YAxisInfo->m_Conversion);
        ui->YFormatLayoutLineEdit->SetValue(par_YAxisInfo->m_FormatLayout);
        ui->YFormatLengthLineEdit->SetValue(par_YAxisInfo->m_FormatLength);
    } else {
        ui->YAxisGroupBox->setDisabled(true);
    }
    // Map
    ui->MapUnitLineEdit->setText(par_ZAxisInfo->m_Unit);
    ui->MapRawMinLineEdit->SetValue(par_ZAxisInfo->m_RawMin);
    ui->MapRawMaxLineEdit->SetValue(par_ZAxisInfo->m_RawMax);
    ui->MapPhysMinLineEdit->SetValue(par_ZAxisInfo->m_Min);
    ui->MapPhysMaxLineEdit->SetValue(par_ZAxisInfo->m_Max);
    ui->MapConvTypeComboBox->setCurrentIndex(par_ZAxisInfo->m_ConvType);
    ui->MapConversionLineEdit->setText(par_ZAxisInfo->m_Conversion);
    ui->MapFormatLayoutLineEdit->SetValue(par_ZAxisInfo->m_FormatLayout);
    ui->MapFormatLengthLineEdit->SetValue(par_ZAxisInfo->m_FormatLength);
}

A2LCalMapPropertiesDlg::~A2LCalMapPropertiesDlg()
{
    delete ui;
}

static char *ReallocCopy(char *DstPtrQString, QString par_String)
{
    char *Ret = (char*)my_realloc(DstPtrQString, strlen(QStringToConstChar(par_String)) + 1);
    if (Ret != nullptr) strcpy(Ret, QStringToConstChar(par_String));
    return Ret;
}

void A2LCalMapPropertiesDlg::Get(CHARACTERISTIC_AXIS_INFO *ret_XAxisInfo,
                                 CHARACTERISTIC_AXIS_INFO *ret_YAxisInfo,
                                 CHARACTERISTIC_AXIS_INFO *ret_ZAxisInfo)
{
    // X axis
    ret_XAxisInfo->m_Unit = ReallocCopy(ret_XAxisInfo->m_Unit, ui->XUnitLineEdit->text());
    ret_XAxisInfo->m_Input = ReallocCopy(ret_XAxisInfo->m_Input, ui->XInputSignalLineEdit->text());
    ret_XAxisInfo->m_Min = ui->XPhysMinLineEdit->GetDoubleValue();
    ret_XAxisInfo->m_Max = ui->XPhysMaxLineEdit->GetDoubleValue();
    ret_XAxisInfo->m_RawMin = ui->XRawMinLineEdit->GetDoubleValue();
    ret_XAxisInfo->m_RawMax = ui->XRawMaxLineEdit->GetDoubleValue();
    ret_XAxisInfo->m_ConvType = ui->XConvTypeComboBox->currentIndex();
    ret_XAxisInfo->m_Conversion = ReallocCopy(ret_XAxisInfo->m_Conversion, ui->XConversionLineEdit->text());
    ret_XAxisInfo->m_FormatLayout = ui->XFormatLayoutLineEdit->GetUIntValue();
    ret_XAxisInfo->m_FormatLength = ui->XFormatLengthLineEdit->GetUIntValue();
    // Y axis
    if (m_Type >= 2) { // 2 -> Map
        ret_YAxisInfo->m_Unit = ReallocCopy(ret_YAxisInfo->m_Unit, ui->YUnitLineEdit->text());
        ret_YAxisInfo->m_Input = ReallocCopy(ret_YAxisInfo->m_Input, ui->YInputSignalLineEdit->text());
        ret_YAxisInfo->m_Min = ui->YPhysMinLineEdit->GetDoubleValue();
        ret_YAxisInfo->m_Max = ui->YPhysMaxLineEdit->GetDoubleValue();
        ret_YAxisInfo->m_RawMin = ui->YRawMinLineEdit->GetDoubleValue();
        ret_YAxisInfo->m_RawMax = ui->YRawMaxLineEdit->GetDoubleValue();
        ret_YAxisInfo->m_ConvType = ui->YConvTypeComboBox->currentIndex();
        ret_YAxisInfo->m_Conversion = ReallocCopy(ret_YAxisInfo->m_Conversion, ui->YConversionLineEdit->text());
        ret_YAxisInfo->m_FormatLayout = ui->YFormatLayoutLineEdit->GetUIntValue();
        ret_YAxisInfo->m_FormatLength = ui->YFormatLengthLineEdit->GetUIntValue();
    }
    // Map
    ret_ZAxisInfo->m_Unit = ReallocCopy(ret_ZAxisInfo->m_Unit, ui->MapUnitLineEdit->text());
    ret_ZAxisInfo->m_Min = ui->MapPhysMinLineEdit->GetDoubleValue();
    ret_ZAxisInfo->m_Max = ui->MapPhysMaxLineEdit->GetDoubleValue();
    ret_ZAxisInfo->m_RawMin = ui->MapRawMinLineEdit->GetDoubleValue();
    ret_ZAxisInfo->m_RawMax = ui->MapRawMaxLineEdit->GetDoubleValue();
    ret_ZAxisInfo->m_ConvType = ui->MapConvTypeComboBox->currentIndex();
    ret_ZAxisInfo->m_Conversion = ReallocCopy(ret_ZAxisInfo->m_Conversion, ui->MapConversionLineEdit->text());
    ret_ZAxisInfo->m_FormatLayout = ui->MapFormatLayoutLineEdit->GetUIntValue();
    ret_ZAxisInfo->m_FormatLength = ui->MapFormatLengthLineEdit->GetUIntValue();
}

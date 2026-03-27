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


#include "BlackboardInternalDialog.h"
#include "ui_BlackboardInternalDialog.h"

extern "C" {
#include "IniDataBase.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#include "ThrowError.h"
}

BlackboardInternalDialog::BlackboardInternalDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::BlackboardInternalDialog)
{
    ui->setupUi(this);

    bool Ret = connect(ui->FilterGroupBox, SIGNAL(FilterClicked()), this, SLOT(FilterSlot ()));
    if (!Ret) {
        ThrowError (1, "connect");
    }
    m_IncExcFilter = BuildIncludeExcludeFilterFromIni ("BasicSettings", "BlackboardInternalDialog", GetMainFileDescriptor());
    if (m_IncExcFilter != nullptr) {
        ui->FilterGroupBox->SetFilter(m_IncExcFilter);
    }
    FilterSlot();
}

BlackboardInternalDialog::~BlackboardInternalDialog()
{
    delete ui;
    if (m_IncExcFilter != nullptr) {
        SaveIncludeExcludeListsToIni (m_IncExcFilter, "BasicSettings", "BlackboardInternalDialog", GetMainFileDescriptor());
        FreeIncludeExcludeFilter (m_IncExcFilter);
    }
    m_IncExcFilter = nullptr;
}


void BlackboardInternalDialog::FillVarianleList()
{
    ui->VariableListWidget->clear();
    bool OnlyUsedFlag = ui->OnlyVisableCheckBox->isChecked();

    for (int BlackboardIndex = 0; BlackboardIndex < blackboard_infos.Size; BlackboardIndex++) {
        if (blackboard[BlackboardIndex].Vid > 0) {
            if ((OnlyUsedFlag && (blackboard[BlackboardIndex].Type != BB_UNKNOWN_WAIT)) ||
                !OnlyUsedFlag) {
                if (blackboard[BlackboardIndex].pAdditionalInfos != nullptr) {
                    if (blackboard[BlackboardIndex].pAdditionalInfos->Name != nullptr) {
                        if ((m_IncExcFilter == nullptr) || CheckIncludeExcludeFilter (blackboard[BlackboardIndex].pAdditionalInfos->Name, m_IncExcFilter)) {
                            ui->VariableListWidget->addItem (QString (blackboard[BlackboardIndex].pAdditionalInfos->Name));
                        }
                    }
                }
            }
        }
    }
}

void BlackboardInternalDialog::FilterSlot()
{
    // New filter
    FreeIncludeExcludeFilter (m_IncExcFilter);
    m_IncExcFilter = ui->FilterGroupBox->GetFilter();

    FillVarianleList();
}


void BlackboardInternalDialog::on_VariableListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (current != nullptr) {
        QString VariableName = current->data(Qt::DisplayRole).toString();
        if (!VariableName.isEmpty()) {
            for (int BlackboardIndex = 0; BlackboardIndex < blackboard_infos.Size; BlackboardIndex++) {
                if (blackboard[BlackboardIndex].Vid > 0) {
                    if (blackboard[BlackboardIndex].pAdditionalInfos != nullptr) {
                        if (blackboard[BlackboardIndex].pAdditionalInfos->Name != nullptr) {
                            if (!VariableName.compare(blackboard[BlackboardIndex].pAdditionalInfos->Name)) {
                                // Basic Infos
                                ui->NameLineEdit->setText(VariableName);
                                if (blackboard[BlackboardIndex].pAdditionalInfos->DisplayName != nullptr) {
                                    ui->DisplayNameLineEdit->setText(QString (blackboard[BlackboardIndex].pAdditionalInfos->DisplayName));
                                } else {
                                    ui->DisplayNameLineEdit->setText(QString ("NULL"));
                                }
                                ui->ValueLineEdit->SetValue(read_bbvari_convert_double(blackboard[BlackboardIndex].Vid));
                                ui->VidLineEdit->setText(QString("0x").number(blackboard[BlackboardIndex].Vid, 16));
                                ui->DataTypeLineEdit->setText(QString(GetDataTypeName (blackboard[BlackboardIndex].Type)));
                                if (blackboard[BlackboardIndex].pAdditionalInfos->Unit != nullptr) {
                                    ui->UnitLineEdit->setText(QString(blackboard[BlackboardIndex].pAdditionalInfos->Unit));
                                } else {
                                    ui->UnitLineEdit->setText(QString("NULL"));
                                }
                                if (blackboard[BlackboardIndex].pAdditionalInfos->Comment != nullptr) {
                                    ui->CommentPlainTextEdit->setPlainText(QString (blackboard[BlackboardIndex].pAdditionalInfos->Comment));
                                } else {
                                    ui->CommentPlainTextEdit->setPlainText(QString ("NULL"));
                                }
                                // Display
                                ui->MinLineEdit->SetValue(blackboard[BlackboardIndex].pAdditionalInfos->Min);
                                ui->MaxLineEdit->SetValue(blackboard[BlackboardIndex].pAdditionalInfos->Max);
                                ui->PrecLineEdit->setText(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->Prec));
                                ui->WidthLineEdit->setText(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->Prec));
                                ui->StepTypeLineEdit->setText(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->StepType));
                                ui->StepLineEdit->SetValue(blackboard[BlackboardIndex].pAdditionalInfos->Step);
                                ui->ColorLineEdit->setText(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->RgbColor, 16));
                                ui->ConversionTypeLineEdit->setText(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->Conversion.Type));
                                switch (blackboard[BlackboardIndex].pAdditionalInfos->Conversion.Type) {
                                //default:
                                case  BB_CONV_NONE:
                                    break;
                                case BB_CONV_FORMULA:
                                    ui->ConversionLineEdit->setText(QString(blackboard[BlackboardIndex].pAdditionalInfos->Conversion.Conv.Formula.FormulaString));
                                    break;
                                case BB_CONV_TEXTREP:
                                    ui->ConversionLineEdit->setText(QString(blackboard[BlackboardIndex].pAdditionalInfos->Conversion.Conv.TextReplace.EnumString));
                                    break;
                                case BB_CONV_FACTOFF:
                                case BB_CONV_REF:
                                    break;
                                }
                                // Attach counter
                                ui->AttachCounterLineEdit->setText(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->AttachCount));
                                ui->UnknownWaitAttachCounterLineEdit->setText(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->UnknownWaitAttachCount));

                                char ProcessName[MAX_PATH];
                                ui->ProcessAttachCounteTableWidget->clear();
                                QStringList Vertical;
                                QStringList Horizontal;
                                Horizontal << "Pid" << "Attach counter" << "Unknown wait attach counter" << "Access flags" << "WR flags" << "WR enable Flags" << "RangeControl";
                                ui->ProcessAttachCounteTableWidget->setHorizontalHeaderLabels (Horizontal);
                                for (int x = 0; x < 64; x++) {
                                    if (GetProcessShortName(blackboard_infos.pid_access_masks[x], ProcessName, sizeof(ProcessName)) == 0) {
                                        Vertical.append(QString (ProcessName));
                                    } else {
                                        Vertical.append(QString ("unused"));
                                    }
                                    ui->ProcessAttachCounteTableWidget->setItem(x, 0, new QTableWidgetItem(QString().number(blackboard_infos.pid_access_masks[x])));
                                    ui->ProcessAttachCounteTableWidget->setItem(x, 1, new QTableWidgetItem(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->ProcessAttachCounts[x])));
                                    ui->ProcessAttachCounteTableWidget->setItem(x, 2, new QTableWidgetItem(QString().number(blackboard[BlackboardIndex].pAdditionalInfos->ProcessUnknownWaitAttachCounts[x])));
                                    if ((blackboard[BlackboardIndex].AccessFlags & (1ULL << x)) != 0) {
                                        ui->ProcessAttachCounteTableWidget->setItem(x, 3, new QTableWidgetItem(QString("X")));
                                    } else {
                                        ui->ProcessAttachCounteTableWidget->setItem(x, 3, new QTableWidgetItem(QString("-")));
                                    }
                                    if ((blackboard[BlackboardIndex].WrFlags & (1ULL << x)) != 0) {
                                        ui->ProcessAttachCounteTableWidget->setItem(x, 4, new QTableWidgetItem(QString("X")));
                                    } else {
                                        ui->ProcessAttachCounteTableWidget->setItem(x, 4, new QTableWidgetItem(QString("-")));
                                    }
                                    if ((blackboard[BlackboardIndex].WrEnableFlags & (1ULL << x)) != 0) {
                                        ui->ProcessAttachCounteTableWidget->setItem(x, 5, new QTableWidgetItem(QString("X")));
                                    } else {
                                        ui->ProcessAttachCounteTableWidget->setItem(x, 5, new QTableWidgetItem(QString("-")));
                                    }
                                    if ((blackboard[BlackboardIndex].RangeControlFlag & (1ULL << x)) != 0) {
                                        ui->ProcessAttachCounteTableWidget->setItem(x, 6, new QTableWidgetItem(QString("X")));
                                    } else {
                                        ui->ProcessAttachCounteTableWidget->setItem(x, 6, new QTableWidgetItem(QString("-")));
                                    }
                                }
                                ui->ProcessAttachCounteTableWidget->setVerticalHeaderLabels (Vertical);
                            }
                        }
                    }
                }
            }
        }
    }
}

void BlackboardInternalDialog::on_OnlyVisableCheckBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    FilterSlot();
}

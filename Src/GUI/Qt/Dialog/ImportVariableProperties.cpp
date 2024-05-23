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


#include "ImportVariableProperties.h"
#include "ui_ImportVariableProperties.h"

#include "FileDialog.h"
#include "StringHelpers.h"

extern "C"
{
    #include "IniDataBase.h"
    #include "Blackboard.h"
    #include "Wildcards.h"
    #include "FileExtensions.h"
    #include "ThrowError.h"
    #include "ImExportVarProperties.h"
}

importvariableproperties::importvariableproperties(QWidget *parent) : Dialog(parent),
    ui(new Ui::importvariableproperties)
{
    ui->setupUi(this);
    ui->lineEditFilter->setText("*");
    ui->checkBoxUnit->setChecked(true);
    ui->checkBoxValue->setChecked(true);
    ui->checkBoxConversion->setChecked(true);
    ui->checkBoxDisplay->setChecked(true);
    ui->checkBoxStepType->setChecked(true);
    ui->checkBoxColor->setChecked(true);

    ui->checkBoxExistingOnly->setChecked(true);
}

importvariableproperties::~importvariableproperties()
{
    delete ui;
}

void importvariableproperties::on_pushButtonRight_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetAvailableVariable->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems)
    {
        QString VariableName = Item->data(Qt::DisplayRole).toString();
        if (ui->listWidgetImportVariable->findItems(VariableName, Qt::MatchFixedString).isEmpty())
        {
            ui->listWidgetImportVariable->addItem(VariableName);
        }
    }
}

void importvariableproperties::on_pushButtonLeft_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetImportVariable->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void importvariableproperties::on_pushButtonSelectAllAvailable_clicked()
{
    ui->listWidgetAvailableVariable->selectAll();
}

void importvariableproperties::on_pushButtonSelectAllImport_clicked()
{
    ui->listWidgetImportVariable->selectAll();
}

void importvariableproperties::on_buttonBox_accepted()
{
    int ImportUnit;
    int ImportMinMax;
    int ImportConvertion;
    int ImportDisplay;
    int ImportStep;
    int ImportColor;

    QString file = ui->lineEditImportFile->text();
    if (file.isEmpty())
    {
        ThrowError(1, "No file is selected!");
        return;
    }
    file.replace("/","\\");
    char FileName[MAX_PATH];
    strcpy(FileName, QStringToConstChar(file));
    int Fd = IniFileDataBaseOpen(FileName);
    if (Fd <= 0) {
        ThrowError (1, "cannot read \"%s\" file", FileName);
        goto __ERROR;
    }

    ui->checkBoxUnit->isChecked() ? ImportUnit = 1 : ImportUnit = 0;
    ui->checkBoxValue->isChecked() ? ImportMinMax = 1 : ImportMinMax = 0;
    ui->checkBoxConversion->isChecked() ? ImportConvertion = 1 : ImportConvertion = 0;
    ui->checkBoxDisplay->isChecked() ? ImportDisplay = 1 : ImportDisplay = 0;
    ui->checkBoxStepType->isChecked() ? ImportStep = 1 : ImportStep = 0;
    ui->checkBoxColor->isChecked() ? ImportColor = 1 : ImportColor = 0;
    if (ui->listWidgetImportVariable->count() == 0) {
        ThrowError(1, "No variables selected!");
        goto __ERROR;
    }
    for (int i = 0; i < ui->listWidgetImportVariable->count(); i++) {
        char VarName[BBVARI_NAME_SIZE];
        strcpy(VarName, QStringToConstChar(ui->listWidgetImportVariable->item(i)->text()));
        ImportOneVariablePropertiesFlags(Fd,
                                         VarName,
                                         ImportUnit,
                                         ImportMinMax,
                                         ImportConvertion,
                                         ImportDisplay,
                                         ImportStep,
                                         ImportColor);
    }
    QDialog::accept();
__ERROR:
    if (Fd > 0) IniFileDataBaseClose(Fd);
}

void importvariableproperties::on_pushButtonFilter_clicked()
{
    QString file = ui->lineEditImportFile->text();
    if (!file.isEmpty()) {
        FillListBox (QStringToConstChar(ui->lineEditImportFile->text()),
                     ui->checkBoxExistingOnly->isChecked(),
                     QStringToConstChar(ui->lineEditFilter->text()));
    }
}

void importvariableproperties::on_pushButtonFile_clicked()
{
    QString file = FileDialog::getOpenFileName(this, "import variable properties", QString(), INI_EXT);
    file.replace("/","\\");
    if(!file.isEmpty())
    {
        ui->lineEditImportFile->setText(file);
        FillListBox(QStringToConstChar(file),
                    ui->checkBoxExistingOnly->isChecked(),
                    QStringToConstChar(ui->lineEditFilter->text()));
    }
}

int importvariableproperties::FillListBox (const char *File, int OnlyExistingVariablesFlag, const char *Filter)
{
    char Variname[INI_MAX_ENTRYNAME_LENGTH];
    int EntryIdx;

    ui->listWidgetAvailableVariable->clear();

    if (File == nullptr) return 0;
    if (strlen (File) == 0) return 0;
    int Fd = IniFileDataBaseOpen(File);
    if (Fd <= 0) {
        ThrowError (1, "cannot read file \"%s\"", File);
        return 0;
    }

    EntryIdx = 0;
    while ((EntryIdx = IniFileDataBaseGetNextEntryName(EntryIdx, "Variables", Variname, sizeof(Variname), Fd)) >= 0) {
        if (OnlyExistingVariablesFlag) {
            if (get_bbvari_attachcount (get_bbvarivid_by_name (Variname)) > 0) {
                if (!Compare2StringsWithWildcards(Variname, Filter)) {
                    ui->listWidgetAvailableVariable->addItem(Variname);
                }
            }
        } else {
            if (!Compare2StringsWithWildcards(Variname, Filter)) {
                QListWidgetItem *Item = new QListWidgetItem(Variname);
                if (get_bbvari_attachcount (get_bbvarivid_by_name (Variname)) == 0) {
                    Item->setBackground(Qt::lightGray);
                }
                ui->listWidgetAvailableVariable->addItem(Item);
            }
        }
    }
    IniFileDataBaseClose(Fd);

    return 0;
}


void importvariableproperties::on_checkBoxExistingOnly_toggled(bool checked)
{
    FillListBox(QStringToConstChar(ui->lineEditImportFile->text()),
                checked,
                QStringToConstChar(ui->lineEditFilter->text()));
}

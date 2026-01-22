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


#include "ExportVariablePropertiesDialog.h"
#include "ui_ExportVariablePropertiesDialog.h"

#include "FileDialog.h"
#include "StringHelpers.h"

extern "C"
{
    #include "StringMaxChar.h"
    #include "IniDataBase.h"
    #include "Blackboard.h"
    #include "Wildcards.h"
    #include "FileExtensions.h"
    #include "ThrowError.h"
}

exportvariableproperties::exportvariableproperties(QWidget *parent) : Dialog(parent),
    ui(new Ui::exportvariableproperties)
{
    ui->setupUi(this);
    char Filter[BBVARI_NAME_SIZE];
    ui->lineEditFilter->setText("*");
    STRING_COPY_TO_ARRAY(Filter, QStringToConstChar(ui->lineEditFilter->text()));
    ui->checkBoxExistingOnly->setChecked(true);
    FillListBox (GetMainFileDescriptor(), 1, Filter);

}

exportvariableproperties::~exportvariableproperties()
{
    delete ui;
}

int exportvariableproperties::FillListBox (int par_Fd, int OnlyExistingVariablesFlag, char *Filter)
{
    char Variname[INI_MAX_ENTRYNAME_LENGTH];
    int EntryIdx;

    ui->listWidgetAvailableVariable->clear();

    EntryIdx = 0;
    while ((EntryIdx = IniFileDataBaseGetNextEntryName (EntryIdx, "Variables", Variname, sizeof(Variname), par_Fd)) >= 0) {
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
    return 0;
}

void exportvariableproperties::on_pushButtonFile_clicked()
{
    QString fileName = FileDialog::getSaveFileName(this, INI_EXT, QString(), INI_EXT);
    fileName.replace("/","\\");
    if(!fileName.isEmpty()) {
        ui->lineEditExportFile->setText(fileName);
    }
}

void exportvariableproperties::on_pushButtonFilter_clicked()
{
    char Filter[BBVARI_NAME_SIZE];
    int OnlyExistingVariablesFlag;
    ui->checkBoxExistingOnly->isChecked() ? OnlyExistingVariablesFlag = 1 : OnlyExistingVariablesFlag = 0;
    STRING_COPY_TO_ARRAY(Filter, QStringToConstChar(ui->lineEditFilter->text()));
    FillListBox (GetMainFileDescriptor(), OnlyExistingVariablesFlag, Filter);
}

void exportvariableproperties::on_pushButtonRight_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetAvailableVariable->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems)
    {
        QString VariableName = Item->data(Qt::DisplayRole).toString();
        if (ui->listWidgetExportVariable->findItems(VariableName, Qt::MatchFixedString).isEmpty())
        {
            ui->listWidgetExportVariable->addItem(VariableName);
        }
    }
}

void exportvariableproperties::on_pushButtonLeft_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetExportVariable->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void exportvariableproperties::on_pushButtonSelectAllAvailable_clicked()
{
    ui->listWidgetAvailableVariable->selectAll();
}

void exportvariableproperties::on_pushButtonSelectAllExport_clicked()
{
    ui->listWidgetExportVariable->selectAll();
}

void exportvariableproperties::on_buttonBox_accepted()
{
    QString file = ui->lineEditExportFile->text();
    if (!file.isEmpty()) {
        // tranfer variables
        file.replace("/", "\\");
        char FileName[MAX_PATH];
        STRING_COPY_TO_ARRAY(FileName, QStringToConstChar(file));
        int Fd = IniFileDataBaseCreateAndOpenNewIniFile(FileName);
        if (Fd > 0) {
            CopyVariablePropertiesBetweenTwoIniFiles (GetMainFileDescriptor(), Fd);
            if (IniFileDataBaseSave(Fd, FileName, 2)) {
                ThrowError (1, "cannot write to file \"%s\"", FileName);
            } else {
                QDialog::accept();
            }
            IniFileDataBaseClose(Fd);
        } else {
            ThrowError(1, "cannot export to file \"%s\"", FileName);
        }
    } else {
        ThrowError (1, "you must select a file export to first");
    }
}

int exportvariableproperties::CopyVariablePropertiesBetweenTwoIniFiles (int SrcIniFile, int DstIniFile)
{
    int i;
    char VarName[BBVARI_NAME_SIZE];
    char Properties[INI_MAX_LINE_LENGTH];

    for (i = 0; i < ui->listWidgetExportVariable->count(); i++) {
        STRING_COPY_TO_ARRAY(VarName, QStringToConstChar(ui->listWidgetExportVariable->item(i)->text()));
        IniFileDataBaseReadString ("Variables", VarName, "", Properties, sizeof (Properties), SrcIniFile);
        IniFileDataBaseWriteString ("Variables", VarName, Properties, DstIniFile);
    }
    return 0;
}

void exportvariableproperties::on_checkBoxExistingOnly_toggled(bool checked)
{
    Q_UNUSED(checked);
    char Filter[BBVARI_NAME_SIZE];
    STRING_COPY_TO_ARRAY(Filter, QStringToConstChar(ui->lineEditFilter->text()));
    FillListBox (GetMainFileDescriptor(), ui->checkBoxExistingOnly->isChecked(), Filter);
}

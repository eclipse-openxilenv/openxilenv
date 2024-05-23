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


#include "UserDefinedEnvironmentVariablesDialog.h"
#include "ui_UserDefinedEnvironmentVariablesDialog.h"

#include <QDialog>
#include "FileDialog.h"
#include "StringHelpers.h"

extern "C"
{
    #include "IniDataBase.h"
    #include "FileExtensions.h"
    #include "ThrowError.h"
}

UserDefinedEnvironmentVariableDialog::UserDefinedEnvironmentVariableDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::UserDefinedEnvironmentVariableDialog)
{
    ui->setupUi(this);

    FillListBox (GetMainFileDescriptor(), 1);
}

UserDefinedEnvironmentVariableDialog::~UserDefinedEnvironmentVariableDialog()
{
    delete ui;
}

void UserDefinedEnvironmentVariableDialog::on_buttonBox_accepted()
{
    SaveListBox(GetMainFileDescriptor(), 0, 1);
    QDialog::accept();
}

void UserDefinedEnvironmentVariableDialog::on_pushButtonImport_clicked()
{
    QString FileName = FileDialog::getOpenFileName(this, INI_EXT, QString(), INI_EXT);
    if (!FileName.isEmpty()) {
        int Fd = IniFileDataBaseOpen(QStringToConstChar(FileName));
        if (Fd > 0) {
            FillListBox(Fd, 0);
            IniFileDataBaseClose(Fd);
        }
    }
}

void UserDefinedEnvironmentVariableDialog::on_pushButtonExport_clicked()
{
    QString FileName = FileDialog::getSaveFileName(this, INI_EXT, QString(), INI_EXT);
    if (!FileName.isEmpty()) {
        if (ui->listWidgetEnvironmentVariables->selectedItems().count() == 0) {
            ThrowError(1, "No item selected!");
            return;
        }
        int Fd = IniFileDataBaseOpen(QStringToConstChar(FileName));
        if (Fd <= 0) {
            Fd = IniFileDataBaseCreateAndOpenNewIniFile(QStringToConstChar(FileName));
        }
        if (Fd > 0) {
            SaveListBox (Fd, 1, 0);
            IniFileDataBaseSave(Fd, nullptr, 2);
        }
    }
}

void UserDefinedEnvironmentVariableDialog::on_pushButtonDelete_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetEnvironmentVariables->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

int UserDefinedEnvironmentVariableDialog::SaveListBox (int par_Fd, int OnlySelectedFlag, int DeleteOldEntrysFlag)
{
    int x, Count;
    char *p, Line[INI_MAX_LINE_LENGTH];

    if (DeleteOldEntrysFlag) IniFileDataBaseWriteString ("UserDefinedEnvironmentVariables", nullptr, nullptr, par_Fd);
    Count = ui->listWidgetEnvironmentVariables->count();
    for (x = 0; x < Count; x++) {
        if (!OnlySelectedFlag || ui->listWidgetEnvironmentVariables->item(x)->isSelected()) {
            // First selected element
            strcpy(Line, QStringToConstChar(ui->listWidgetEnvironmentVariables->item(x)->text()));
            p = Line;
            while (*p != '=') p++;
            *(p-1) = 0;
            p += 2;  // skip "= "
            RemoveWhitespacesOnlyAtBeginingAndEnd (Line);
            RemoveWhitespacesOnlyAtBeginingAndEnd (p);
            IniFileDataBaseWriteString ("UserDefinedEnvironmentVariables", Line, p, par_Fd);
        }
    }
    return 0;
}

void UserDefinedEnvironmentVariableDialog::RemoveWhitespacesOnlyAtBeginingAndEnd (char *p)
{
    char *d = p;
    char *s = p;
    while (isascii (*s) && isspace (*s)) s++;
    while (*s != 0) *d++ = *s++;
    d--;
    while (isascii (*d) && isspace (*d) && (d > p)) d--;
    d++;
    *d = 0;
}

int UserDefinedEnvironmentVariableDialog::FillListBox (int par_Fd, int ResetFlag)
{
    char VarName[INI_MAX_ENTRYNAME_LENGTH];
    char VarValue[INI_MAX_LINE_LENGTH];
    int EntryIdx;
    size_t Len;

    if (ResetFlag) {
        ui->listWidgetEnvironmentVariables->clear();
    }
    EntryIdx = 0;
    while ((EntryIdx = IniFileDataBaseGetNextEntryName(EntryIdx, "UserDefinedEnvironmentVariables", VarName, sizeof(VarName), par_Fd)) >= 0) {
        strcpy (VarValue, VarName);
        strcat (VarValue, " = ");
        Len = strlen (VarValue);
        IniFileDataBaseReadString ("UserDefinedEnvironmentVariables", VarName, "", VarValue + Len, static_cast<DWORD>(sizeof (VarValue) - Len), par_Fd);
        ui->listWidgetEnvironmentVariables->addItem(VarValue);
    }
    return 0;
}

void UserDefinedEnvironmentVariableDialog::on_pushButtonChange_clicked()
{
    char Line[INI_MAX_LINE_LENGTH];
    char LineValue[INI_MAX_LINE_LENGTH];

    QList<QListWidgetItem*> list = ui->listWidgetEnvironmentVariables->selectedItems();

    if(list.isEmpty()) {
        ThrowError(1, "No variable selected!");
        return;
    }

    strcpy(Line, QStringToConstChar(ui->lineEditName->text()));
    RemoveWhitespacesOnlyAtBeginingAndEnd (Line);
    strcat (Line, " = ");
    strcpy(LineValue, QStringToConstChar(ui->lineEditValue->text()));
    RemoveWhitespacesOnlyAtBeginingAndEnd (LineValue);
    strcat(Line, LineValue);
    QListWidgetItem *SelectedItem = ui->listWidgetEnvironmentVariables->currentItem();
    delete SelectedItem;
    ui->listWidgetEnvironmentVariables->addItem(Line);
}

void UserDefinedEnvironmentVariableDialog::on_pushButtonAdd_clicked()
{
    char Line[INI_MAX_LINE_LENGTH];
    char LineValue[INI_MAX_LINE_LENGTH];

    strcpy(Line, QStringToConstChar(ui->lineEditName->text()));
    RemoveWhitespacesOnlyAtBeginingAndEnd (Line);
    strcat (Line, " = ");
    strcpy(LineValue, QStringToConstChar(ui->lineEditValue->text()));
    RemoveWhitespacesOnlyAtBeginingAndEnd (LineValue);
    strcat(Line, LineValue);
    ui->listWidgetEnvironmentVariables->addItem(Line);
}

void UserDefinedEnvironmentVariableDialog::on_listWidgetEnvironmentVariables_itemClicked(QListWidgetItem *item)
{
    char *p, Line[INI_MAX_LINE_LENGTH];

    strcpy(Line, QStringToConstChar(item->text()));
    p = Line;
    while (*p != '=') p++;
    *(p-1) = 0;
    p += 2;  // skip "= "
    RemoveWhitespacesOnlyAtBeginingAndEnd(Line);
    RemoveWhitespacesOnlyAtBeginingAndEnd(p);
    ui->lineEditName->setText(Line);
    ui->lineEditValue->setText(p);
}

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


#include "UserDefinedBlackboardVariablesDialog.h"
#include "ui_UserDefinedBlackboardVariablesDialog.h"

#include "StringHelpers.h"

extern "C"
{
    #include "ThrowError.h"
    #include "LoadSaveToFile.h"
    #include "IniDataBase.h"
    #include "Wildcards.h"
    #include "Files.h"
    #include "Blackboard.h"
    #include "BlackboardAccess.h"
}

UserDefinedBBVariableDialog::UserDefinedBBVariableDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::UserDefinedBBVariableDialog),
    rowCount(0)
{
    ui->setupUi(this);
    QHeaderView *Header = ui->tableWidget->horizontalHeader();
    Header->setSectionResizeMode(0, QHeaderView::Stretch);
    Header->setSectionResizeMode(1, QHeaderView::Interactive);
    Header->setSectionResizeMode(2, QHeaderView::Interactive);
    Header->setSectionResizeMode(3, QHeaderView::Interactive);
    FillTableView();
}

UserDefinedBBVariableDialog::~UserDefinedBBVariableDialog()
{
    delete ui;
}

void UserDefinedBBVariableDialog::FillTableView()
{
    QTableWidgetItem *item;
    char *type, *value, *unit;
    char VarName[INI_MAX_ENTRYNAME_LENGTH];
    char txt[512*100];
    int EntryIdx;
    int Fd = GetMainFileDescriptor();

    EntryIdx = 0;
    while ((EntryIdx = IniFileDataBaseGetNextEntryName(EntryIdx, "UserDefinedBBVariables", VarName, sizeof(VarName), Fd)) >= 0) {
        increment_rowCount();
        IniFileDataBaseReadString ("UserDefinedBBVariables", VarName, "", txt, sizeof (txt), Fd);
        StringCommaSeparate (txt, &type, &value, &unit, nullptr);
        ui->tableWidget->setRowCount(rowCount);
        QComboBox *comboBox = new QComboBox();
        initComboBox(comboBox);
        item = new QTableWidgetItem(QString(VarName));
        ui->tableWidget->setItem(rowCount-1, 0, item);
        int index = QString(type).toInt();
        if (index < 8 && index >= 0) comboBox->setCurrentIndex(index);
        ui->tableWidget->setCellWidget(rowCount-1, 1, comboBox);
        item = new QTableWidgetItem(QString(value));
        ui->tableWidget->setItem(rowCount-1, 2, item);
        item = new QTableWidgetItem(QString(unit));
        ui->tableWidget->setItem(rowCount-1, 3, item);
    }
}

void UserDefinedBBVariableDialog::initComboBox(QComboBox *par_comboBox)
{
    QStringList *itemsList = new QStringList();
    itemsList->append("BYTE");    // Index 0
    itemsList->append("UBYTE");   // Index 1
    itemsList->append("WORD");    // Index 2
    itemsList->append("UWORD");   // Index 3
    itemsList->append("DWORD");   // Index 4
    itemsList->append("UDWORD");  // Index 5
    itemsList->append("QWORD");   // Index 6
    itemsList->append("UQWORD");  // Index 7
    itemsList->append("FLOAT");   // Index 8
    itemsList->append("DOUBLE");  // Index 9
    par_comboBox->addItems(*itemsList);
}

int UserDefinedBBVariableDialog::get_rowCount()
{
    return rowCount;
}

void UserDefinedBBVariableDialog::set_rowCount(int par_rowCount)
{
    rowCount = par_rowCount;
}

void UserDefinedBBVariableDialog::increment_rowCount()
{
    rowCount++;
}

void UserDefinedBBVariableDialog::decrement_rowCount()
{
    rowCount--;
}

void UserDefinedBBVariableDialog::on_pushButtonNew_clicked()
{
    QTableWidgetItem *item;
    QComboBox *comboBox = new QComboBox();
    initComboBox(comboBox);
    increment_rowCount();
    ui->tableWidget->setRowCount(rowCount);
    item = new QTableWidgetItem(QString(""));
    ui->tableWidget->setItem(rowCount-1, 0, item);
    ui->tableWidget->setCellWidget(rowCount-1, 1, comboBox);
    item = new QTableWidgetItem(QString(""));
    ui->tableWidget->setItem(rowCount-1, 2, item);
    item = new QTableWidgetItem(QString(""));
    ui->tableWidget->setItem(rowCount-1, 3, item);
}

void UserDefinedBBVariableDialog::on_pushButtonDelete_clicked()
{
    if (ui->tableWidget->rowCount() == 0) return; // No items in the table
    QModelIndexList Selected = ui->tableWidget->selectionModel()->selectedRows();
    // Must be sorted climb down
    std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
    foreach (QModelIndex Index, Selected) {
        if (Index.column() == 0) {
            ui->tableWidget->removeRow(Index.row());
            decrement_rowCount();
        }
    }
}

void UserDefinedBBVariableDialog::accept()
{
    int Fd = GetMainFileDescriptor();

    // first check if all variable names are valid
    for(int x = 0; x < ui->tableWidget->rowCount(); x++) {
        QString Name = ui->tableWidget->item(x, 0)->text();
        if (!IsValidVariableName(QStringToConstChar(Name))) {
            ThrowError(1, "variable name \"%s\" not valid", QStringToConstChar(Name));
            ui->tableWidget->setCurrentCell(x, 0);
            return;
        }
        bool Ok;
        QString ValueStr = ui->tableWidget->item(x, 2)->text();
        if (!ValueStr.isEmpty()) {
            ValueStr.toDouble(&Ok);
            if (!Ok) {
                ThrowError(1, "not a value \"%s\"", QStringToConstChar(ValueStr));
                ui->tableWidget->setCurrentCell(x, 2);
                return;
            }
        }
        for(int y = 0; y < ui->tableWidget->rowCount(); y++) {
            if (y != x) {
                if (!ui->tableWidget->item(y, 0)->text().compare(Name)) {
                    ThrowError(1, "variable name \"%s\" are already in use", QStringToConstChar(Name));
                    ui->tableWidget->setCurrentCell(y, 0);
                    return;
                }
            }
        }
    }
    // now find out which should be deleted
    QStringList ShouldRemove;
    char *type, *value, *unit;
    char VarName[INI_MAX_ENTRYNAME_LENGTH];
    char txt[512*100];
    int EntryIdx = 0;
    while ((EntryIdx = IniFileDataBaseGetNextEntryName (EntryIdx, "UserDefinedBBVariables", VarName, sizeof(VarName), Fd)) >= 0) {
        IniFileDataBaseReadString ("UserDefinedBBVariables", VarName, "", txt, sizeof (txt), Fd);
        StringCommaSeparate (txt, &type, &value, &unit, nullptr);
        bool Found = false;
        bool TypeHasChanged = false;
        for (int x = 0; x < ui->tableWidget->rowCount(); x++) {
            QString Name = ui->tableWidget->item(x, 0)->text();
            if (!Name.compare(VarName)) {
                int TypeNo = QString(type).toInt();
                QComboBox *ComboBox = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(x, 1));
                if (ComboBox->currentIndex() == TypeNo) {
                    Found = true;
                } else {
                    TypeHasChanged = true;
                }
            }
        }
        if (!Found) {
            // we should delete this variable (was removd or type has changed)
            int Vid = get_bbvarivid_by_name(VarName);
            if (remove_bbvari(Vid) && TypeHasChanged) {
                ThrowError(1, "cannot change data type of variable \"%s\"", VarName);
            }
            ShouldRemove.append(VarName);
        }
    }
    foreach(QString Name, ShouldRemove) {
        IniFileDataBaseWriteString("UserDefinedBBVariables", QStringToConstChar(Name), NULL, Fd);
    }

    for (int x = 0; x < ui->tableWidget->rowCount(); x++) {
        QString Name = ui->tableWidget->item(x, 0)->text();
        strncpy(VarName, QStringToConstChar(Name), sizeof(VarName));
        VarName[sizeof(VarName)-1] = 0;
        // Get value
        QString ValueStr = ui->tableWidget->item(x, 2)->text();
        // Get unit (unit is optional)
        QString Unit = ui->tableWidget->item(x, 3)->text();
        // Get datatype
        QComboBox *ComboBox = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(x, 1));
        int TypeNo = ComboBox->currentIndex();
        int Vid;
        if (IniFileDataBaseReadString("UserDefinedBBVariables", VarName, "", txt, sizeof (txt), Fd) == 0) {
            switch (TypeNo) {
            case 0:
                Vid = add_bbvari(VarName, BB_BYTE, QStringToConstChar(Unit));
                break;
            case 1:
                Vid = add_bbvari(VarName, BB_UBYTE, QStringToConstChar(Unit));
                break;
            case 2:
                Vid = add_bbvari(VarName, BB_WORD, QStringToConstChar(Unit));
                break;
            case 3:
                Vid = add_bbvari(VarName, BB_UWORD, QStringToConstChar(Unit));
                break;
            case 4:
                Vid = add_bbvari(VarName, BB_DWORD, QStringToConstChar(Unit));
                break;
            case 5:
                Vid = add_bbvari(VarName, BB_UDWORD, QStringToConstChar(Unit));
                break;
            case 6:
                Vid = add_bbvari(VarName, BB_QWORD, QStringToConstChar(Unit));
                break;
            case 7:
                Vid = add_bbvari(VarName, BB_UQWORD, QStringToConstChar(Unit));
                break;
            case 8:
                Vid = add_bbvari(VarName, BB_FLOAT, QStringToConstChar(Unit));
                break;
            case 9:
                Vid = add_bbvari(VarName, BB_DOUBLE, QStringToConstChar(Unit));
                break;
            default:
                Vid = add_bbvari(VarName, BB_UNKNOWN, QStringToConstChar(Unit));
                break;
            }
        } else {
            Vid = get_bbvarivid_by_name(VarName);
        }
        set_bbvari_unit(Vid, QStringToConstChar(Unit));
        bool Ok;
        double Value = ValueStr.toDouble(&Ok);
        if (Ok) {
            write_bbvari_minmax_check(Vid, Value);
        }
        sprintf(txt, "%i,%s,%s", TypeNo, QStringToConstChar(ValueStr), QStringToConstChar(Unit));
        IniFileDataBaseWriteString("UserDefinedBBVariables", VarName, txt, Fd);
    }
    QDialog::accept();
}

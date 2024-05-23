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


#include "HotkeysDialog.h"
#include "ui_HotkeysDialog.h"

#include <MainWindow.h>
#include <QStandardItemModel>
#include <QStringList>

HotkeysDialog::HotkeysDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::hotkeysdialog)
{
    ui->setupUi(this);

    QStringList AcceptanceColumnNameList;
        AcceptanceColumnNameList << "Hotkey" << "Function" << "Formula";

        QString *string;

        ui->tableWidgetShortcuts->setColumnCount(AcceptanceColumnNameList.count());
        ui->tableWidgetShortcuts->setHorizontalHeaderLabels(AcceptanceColumnNameList);
        ui->tableWidgetShortcuts->setRowCount(MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringList()->count());

        for (int Row = 0; Row < ui->tableWidgetShortcuts->rowCount() ; Row++) {
            QTableWidgetItem *Item;

            string = MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringListItem(Row, 0);
            Item = new QTableWidgetItem(*string);
            ui->tableWidgetShortcuts->setItem (Row, 0, Item);

            string = MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringListItem(Row, 1);
            Item = new QTableWidgetItem(*string);
            ui->tableWidgetShortcuts->setItem (Row, 1, Item);

            string = MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringListItem(Row, 2);
            Item = new QTableWidgetItem(*string);
            ui->tableWidgetShortcuts->setItem (Row, 2, Item);
        }
}

HotkeysDialog::~HotkeysDialog()
{
    delete ui;
}

void HotkeysDialog::on_pushButtonNew_clicked()
{
    NewHotkeyDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
    refresh();
}

void HotkeysDialog::on_pushButtonClose_clicked()
{
    MainWindow::PointerToMainWindow->get_hotkeyHandler()->saveHotkeys();
    QDialog::accept();
}

void HotkeysDialog::on_pushButtonEdit_clicked()
{
    QString hotkey, function, formula;
    if(ui->tableWidgetShortcuts->rowCount() == 0) return;
    int i = ui->tableWidgetShortcuts->currentRow();
    hotkey = ui->tableWidgetShortcuts->item(i, 0)->text();
    function = ui->tableWidgetShortcuts->item(i, 1)->text();
    formula = ui->tableWidgetShortcuts->item(i, 2)->text();

    NewHotkeyDialog Dlg(hotkey, function, formula);

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
    refresh();
}

void HotkeysDialog::on_pushButtonDelete_clicked()
{
    if(ui->tableWidgetShortcuts->rowCount() == 0) return;
    int i = ui->tableWidgetShortcuts->currentRow();
    ui->tableWidgetShortcuts->removeRow(i);
    MainWindow::PointerToMainWindow->get_hotkeyHandler()->deleteshortcutStringListItem(i);
    refresh();
}
 void HotkeysDialog::refresh()
 {
     QString *string;

     ui->tableWidgetShortcuts->setRowCount(MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringList()->count());

     for (int Row = 0; Row < ui->tableWidgetShortcuts->rowCount() ; Row++) {
         QTableWidgetItem *Item;

         string = MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringListItem(Row, 0);
         Item = new QTableWidgetItem(*string);
         ui->tableWidgetShortcuts->setItem (Row, 0, Item);

         string = MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringListItem(Row, 1);
         Item = new QTableWidgetItem(*string);
         ui->tableWidgetShortcuts->setItem (Row, 1, Item);

         string = MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringListItem(Row, 2);
         Item = new QTableWidgetItem(*string);
         ui->tableWidgetShortcuts->setItem (Row, 2, Item);
     }
 }


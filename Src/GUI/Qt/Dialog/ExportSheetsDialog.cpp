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


#include "ExportSheetsDialog.h"
#include "ui_ExportSheetsDialog.h"

#include "FileDialog.h"
#include "MainWindow.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

#include <QInputDialog>

extern "C"
{
    #include "FileExtensions.h"
    #include "ThrowError.h"
    #include "StringMaxChar.h"
}

ExportSheetsDialog::ExportSheetsDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::ExportSheetsDialog)
{
    ui->setupUi(this);
    SheetsList = MainWindow::PointerToMainWindow->GetRefToAllSheets();
    foreach (QString Sheetname, SheetsList->keys())
    {
        Sheets *sheet = SheetsList->value(Sheetname);
        ui->listWidgetINI->addItem(sheet->GetName());
    }
}

ExportSheetsDialog::~ExportSheetsDialog()
{
    delete ui;
}

void ExportSheetsDialog::on_pushButtonFile_clicked()
{
    ui->lineEditFile->setText(FileDialog::getSaveFileName(this, QString ("Save sheet(s) to file"), QString(), QString (DSK_INI_EXT)));
}

void ExportSheetsDialog::on_pushButtonRight_clicked()
{
    if (ui->listWidgetINI->selectedItems().count() == 0) return;
    ui->listWidgetExport->addItem(ui->listWidgetINI->currentItem()->text());
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetINI->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ExportSheetsDialog::on_pushButtonLeft_clicked()
{
    if (ui->listWidgetExport->selectedItems().count() == 0) return;
    ui->listWidgetINI->addItem(ui->listWidgetExport->currentItem()->text());
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetExport->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ExportSheetsDialog::accept()
{
    if(ui->lineEditFile->text().isEmpty()) {
        QMessageBox msgWarnBox;
        msgWarnBox.setIcon(QMessageBox::Warning);
        msgWarnBox.setText("No file selected!");
        msgWarnBox.setInformativeText("Please select a file!");
        msgWarnBox.exec();
        return;
    }

    // The information of the sheet(s) are stored in a cache inside the sheet and the INI section is deleted
    // so we have to write the sections to the INI befor we can use it.
    Sheets::CleanIni();
    foreach(QString loc_key, SheetsList->keys()) {
        SheetsList->value(loc_key)->writeToIni(false);
    }
    QString DstFileName = ui->lineEditFile->text();
    int Fd = ScQt_IniFileDataBaseOpen(DstFileName);
    if (Fd <= 0) {
        Fd = ScQt_IniFileDataBaseCreateAndOpenNewIniFile(DstFileName);
    }
    if (Fd > 0) {
        CopySheets(GetMainFileDescriptor(), Fd);

        Sheets::CleanIni();

        if (ScQt_IniFileDataBaseSave(Fd, DstFileName, 2)) {
            ThrowError (1, "cannot export sheet(s) to file %s", QStringToConstChar(DstFileName));
        }
        QDialog::accept();
    } else {
        ThrowError(1, "cannot export to \"%s\"", QStringToConstChar(DstFileName));
    }
}


void ExportSheetsDialog::CopySheets(int par_SrcFd, int par_DstFd)
{
    QString SrcSection;
    QString DstSection;

    // Loop through all sheets inside the source
    for (int x = 0; x < 10000; x++) {
        SrcSection = QString("GUI/OpenWindowsForSheet%1").arg(x);
        QString SrcSheetName = ScQt_IniFileDataBaseReadString(SrcSection, "SheetName", "", par_SrcFd);
        if (SrcSheetName.isEmpty()) {
            if (x == 0) {
                SrcSheetName = QString("Default");
            } else {
                break;
            }
        }
        // Loop through all selcted sheets
        for (int i = 0; i < ui->listWidgetExport->count(); i++) {
            // Check if the sheet is selected to import
            if (!SrcSheetName.compare(ui->listWidgetExport->item(i)->text())) {
                QString DstSheetName = SrcSheetName;
                bool CopyFlag = true;
                // Loop through all sheets inside the destination
                for (int d = 0; d < 10000; d++) {
                    QString DstSection = QString("GUI/OpenWindowsForSheet%1").arg(d);
                    QString SheetName = ScQt_IniFileDataBaseReadString (DstSection, "SheetName", "", par_DstFd);
                    if (SheetName.isEmpty()) {
                        if (d == 0) {
                            SheetName = QString("Default");
                        } else {
                            break;
                        }
                    }
                    // Check if the sheet name already exists
                    if (!SheetName.compare(DstSheetName)) {
                        // Rename the sheet
                        QMessageBox msgInfBox;
                        msgInfBox.setIcon(QMessageBox::Information);
                        msgInfBox.setText(QString("Sheetname '%1' already exists!").arg(DstSheetName));
                        msgInfBox.setInformativeText("Do you want to rename the imported Sheet (yes) or ignore it (no)?");
                        QPushButton *yesButton = msgInfBox.addButton(tr("Yes"), QMessageBox::ActionRole);
                        QPushButton *noButton = msgInfBox.addButton(tr("No"),QMessageBox::ActionRole);

                        msgInfBox.exec();

                        if (msgInfBox.clickedButton() == yesButton) {
                            bool Check;
                            QString NewSheetName;
                            do {
                                NewSheetName = QInputDialog::getText(this, tr("Change Sheetname"),
                                                                     tr("New Sheetname:"), QLineEdit::Normal,
                                                                     DstSheetName, &Check);
                            } while (NewSheetName.isEmpty());
                            DstSheetName = NewSheetName;
                        } else if (msgInfBox.clickedButton() == noButton) {
                            CopyFlag = false;  // do not import this sheet
                            break; // for(;;)
                        }
                    }
                }

                if (CopyFlag) {
                    // this will change th name of the sheet(s) and window(s) if necessary inside the destination INI file
                    // and will copy the sheet section and all refrenced windows to the destionation INI file.
                    for (int d = 0; d < 10000; d++) {
                        DstSection = QString("GUI/OpenWindowsForSheet%1").arg(d);
                        if (!ScQt_IniFileDataBaseLookIfSectionExist(DstSection, par_DstFd)) {
                            break;
                        }
                    }

                    CopyOneSheet(SrcSection, par_SrcFd,
                                 DstSection, par_DstFd, DstSheetName, false);

                }
            }
        }
    }
}


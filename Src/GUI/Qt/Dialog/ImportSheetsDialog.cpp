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


#include "ImportSheetsDialog.h"
#include "ui_ImportSheetsDialog.h"

#include <QInputDialog>
#include "MainWindow.h"
#include "FileDialog.h"
#include "WindowNameAlreadyInUse.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

extern "C"
{
    #include "FileExtensions.h"
    #include "ThrowError.h"
    #include "StringMaxChar.h"
    #include "PrintFormatToString.h"
    #include "WindowIniHelper.h"
}

ImportSheetsDialog::ImportSheetsDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::ImportSheetsDialog)
{
    ui->setupUi(this);
    ui->lineEditFile->installEventFilter(this);
}

ImportSheetsDialog::~ImportSheetsDialog()
{
    delete ui;
}

void ImportSheetsDialog::on_pushButtonFile_clicked()
{
    QString sFileName = FileDialog::getOpenFileName(this, QString ("Import sheet(s) from file"), QString(), QString (DSK_INI_EXT));
    if(sFileName.isEmpty()) return;
    ui->lineEditFile->setText(sFileName);
    fillListBox();
}

void ImportSheetsDialog::on_pushButtonRight_clicked()
{
    if (ui->listWidgetINI->selectedItems().count() == 0) return;
    ui->listWidgetImport->addItem(ui->listWidgetINI->currentItem()->text());
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetINI->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ImportSheetsDialog::on_pushButtonLeft_clicked()
{
    if (ui->listWidgetImport->selectedItems().count() == 0) return;
    ui->listWidgetINI->addItem(ui->listWidgetImport->currentItem()->text());
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetImport->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ImportSheetsDialog::accept()
{
    QString SrcFileName;
    QString SrcName;
    QString DstName;
    QString SrcSection;
    QString DstSection;
    QList<Sheets*> Sheets = MainWindow::PointerToMainWindow->GetAllSheets();
    int SheetPos = Sheets.count();

    if(ui->lineEditFile->text().isEmpty()) {
        QMessageBox msgInfBox;
        msgInfBox.setIcon(QMessageBox::Information);
        msgInfBox.setText("No sheet selected!");
        msgInfBox.exec();
        return;
    }

    if(ui->listWidgetImport->count() == 0) {
        QMessageBox msgInfBox;
        msgInfBox.setIcon(QMessageBox::Information);
        msgInfBox.setText("No sheet selected!");
        msgInfBox.exec();
        return;
    }

    SrcFileName = ui->lineEditFile->text();
    int Fd = ScQt_IniFileDataBaseOpen(SrcFileName);
    if (Fd > 0) {
        // Loop through all sheets inside the source
        for (int x = 0; x < 10000; x++) {
            SrcSection = QString("GUI/OpenWindowsForSheet%1").arg(x);
            SrcName = ScQt_IniFileDataBaseReadString (SrcSection, "SheetName", "",  Fd);
            if (SrcName.isEmpty()) {
                if (x == 0) {
                    SrcName = QString("Default");
                } else {
                    break;
                }
            }
            // Loop through all selcted sheets
            for (int i = 0; i < ui->listWidgetImport->count(); i++) {
                // Check if the sheet is selected to import
                if (!SrcName.compare(ui->listWidgetImport->item(i)->text())) {
                    bool CopyFlag = true;
                    DstName = SrcName;
                    for(int c = 0; c < Sheets.count(); c++) {
                        // Check if the sheet name already exists
                        if (!DstName.compare(Sheets.at(c)->GetName())) {
                            // Rename the sheet
                            QMessageBox msgInfBox;
                            msgInfBox.setIcon(QMessageBox::Information);
                            msgInfBox.setText(QString("Sheetname '%1' already exists!").arg(DstName));
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
                                                                         DstName, &Check);
                                } while (NewSheetName.isEmpty());
                                DstName = NewSheetName;
                                c = 0; // check the new name again
                            } else if (msgInfBox.clickedButton() == noButton) {
                                c = Sheets.count();  // do not import this sheet
                                CopyFlag = false;
                            }
                        }
                    }

                    if (CopyFlag) {
                        // this will change th name of the sheet(s) and window(s) if necessary inside the source INI file
                        // and will copy all refrenced windows to the destionation INI file.
                        // after the sheets are loaded
                        // The information is than stored in a cache inside the sheet and the INI section is deleted
                        DstSection = QString("GUI/OpenWindowsForSheet%1").arg(SheetPos);

                        CopyOneSheet(SrcSection, Fd,
                                     DstSection, ScQt_GetMainFileDescriptor(), DstName, true);

                        MainWindow::PointerToMainWindow->ReadOneSheetFromIni(DstName, DstSection);

                        SheetPos++;
                    }
                }
            }
        }
        ScQt_IniFileDataBaseClose(Fd);
    }
    QDialog::accept();
}


void ImportSheetsDialog::on_lineEditFile_editingFinished()
{
    fillListBox();
}

// Reimplentierte eventFilter methode
bool ImportSheetsDialog::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent*>(event);
        switch (key->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            fillListBox();
            ui->listWidgetINI->setFocus();
            break;
        default:
            return QObject::eventFilter(target, event);
        }
        return true;
    } else {
        return  QObject::eventFilter(target, event);;
    }
}

void ImportSheetsDialog::fillListBox()
{
    ui->listWidgetINI->clear();
    if(ui->lineEditFile->text().isEmpty()) return;
    QString FileName = ui->lineEditFile->text();
    int Fd = ScQt_IniFileDataBaseOpen(FileName);
    if (Fd > 0) {
        for (int i = 0; i < 10000; i++) {
            QString Section = QString("GUI/OpenWindowsForSheet%1").arg(i);
            QString Name = ScQt_IniFileDataBaseReadString (Section, "SheetName", "", Fd);
            if (Name.isEmpty()) {
                break;
            }
            ui->listWidgetINI->addItem(Name);
        }
        ScQt_IniFileDataBaseClose(Fd);
    } else {
        ThrowError(1, "canot import from  \"%s\"", QStringToConstChar(FileName));
    }
}

static QString RmoveNumberInsideBracktsAtTheEnd(QString &par_Text)
{
    QString Ret = par_Text;
    int p = Ret.size();
    if (p > 3) { // at last 3 characters long
        p--;
        if (Ret.at(p) == ')') {  // last character is a ')'
            p--;  // jump over ')'
            if ((Ret.at(p) >= '0')  && (Ret.at(p) <= '9')) {
                while ((Ret.at(p) >= '0')  && (Ret.at(p) <= '9') && (p > 0)) p--;  // jump over a number
                if (Ret.at(p) == '(') {  // jump over '('
                    while ((p > 0) && (Ret.at(p - 1) == ' ')) p--;  // rmove whitespaces
                    if (p > 0) { // there are one or more charactrs left
                        // now w are sure we have found somthing like "(123)" at the end of the string
                        Ret.truncate(p);    // cutoff the "(123)"
                    }
                }
            }
        }
    }
    return Ret;
}

void CopyOneSheet(QString &SrcSection, int par_SrcFd,
                  QString &DstSection, int par_DstFd,
                  QString &DstSheetName, bool ImportFlag)
{
    QString Entry;
    QString DstWindowName;

    // Copy the sheet list
    ScQt_IniFileDataBaseCopySection(par_DstFd, par_SrcFd, DstSection, SrcSection);
    // the name of a sheet can be changed
    ScQt_IniFileDataBaseWriteString(DstSection, "SheetName", DstSheetName, par_DstFd);
    for (int x = 0; x < 10000; x++) {
        Entry = QString("W%1").arg(x);
        QString SrcWindowName = ScQt_IniFileDataBaseReadString(SrcSection, Entry, "", par_SrcFd);
        if (SrcWindowName.isEmpty()) {
            break;
        }
        QString SrcWindowSectionPath = QString("GUI/Widgets/").append(SrcWindowName);
        if (ScQt_IniFileDataBaseLookIfSectionExist(SrcWindowSectionPath, par_SrcFd)) {
            if ((ImportFlag && WindowNameAlreadyInUse(SrcWindowName)) ||  // Window already exist in destination
                (!ImportFlag && ScQt_IniFileDataBaseLookIfSectionExist(SrcWindowSectionPath, par_DstFd))) {  // Section exist
                QString SrcNameTruncated = RmoveNumberInsideBracktsAtTheEnd(SrcWindowName);
                QString DstWindowSectionPath;
                for (int i = 1; i < 10000; i++) {
                    DstWindowName = SrcNameTruncated;
                    DstWindowName.append(QString(" (%1)").arg(i));
                    DstWindowSectionPath = QString("GUI/Widgets/").append(DstWindowName);
                    if (ImportFlag) {
                        if(!WindowNameAlreadyInUse(DstWindowName)) {  // Window already exist in destination
                            break;
                        }
                    } else {
                        if (!ScQt_IniFileDataBaseLookIfSectionExist(DstWindowSectionPath, par_DstFd)) {  // Section exist
                            break;
                        }
                    }
                }
                ScQt_IniFileDataBaseCopySection(par_DstFd, par_SrcFd, DstWindowSectionPath, SrcWindowSectionPath);
            } else {
                DstWindowName = SrcWindowName;
                ScQt_IniFileDataBaseCopySectionSameName(par_DstFd, par_SrcFd, SrcWindowSectionPath);
            }
            // change the name inside the "open window for sheet x" list (this can be changed)
            ScQt_IniFileDataBaseWriteString(DstSection, Entry, DstWindowName, par_DstFd);
            // and add it to the "GUI/All xxxxx Windows" list
            AddWindowToCorrespondAllList(QStringToConstChar(DstWindowName), QStringToConstChar(DstWindowName), par_DstFd);
        }
    }
}

void AddWindowToCorrespondAllList(const char *WidgetName, const char *SectionName, int DstFile)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[INI_MAX_LINE_LENGTH];
    if (IniFileDataBaseReadString (SectionName, "type", "", Line, sizeof(Line), DstFile) > 0) {
        char *AllListSection;
        if (GetSection_AllWindowsOfType(Line, &AllListSection) == 0) {
            for (int x = 0; x < 10000; x++) {
                PrintFormatToString (Entry, sizeof(Entry), "W%i", x);
                if (IniFileDataBaseReadString (AllListSection, Entry, "", Line, sizeof(Line), DstFile) == 0) {
                    // we are at the end of the list now, than add the new one
                    ScQt_IniFileDataBaseWriteString (AllListSection, Entry, WidgetName, DstFile);
                    break;
                }
            }
        }
    }
}

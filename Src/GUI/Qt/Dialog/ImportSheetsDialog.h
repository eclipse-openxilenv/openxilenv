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


#ifndef IMPORTSHEETSDIALOG_H
#define IMPORTSHEETSDIALOG_H

#include <QDialog>
#include "Sheets.h"
#include "Dialog.h"

namespace Ui {
class ImportSheetsDialog;
}

class ImportSheetsDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ImportSheetsDialog(QWidget *parent = nullptr);
    ~ImportSheetsDialog();

public slots:
    void accept();

private slots:
    void on_pushButtonFile_clicked();

    void on_pushButtonRight_clicked();

    void on_pushButtonLeft_clicked();

    void on_lineEditFile_editingFinished();

private:
    Ui::ImportSheetsDialog *ui;
    bool eventFilter(QObject *target, QEvent *event);
    void fillListBox();
};

// this will also used inside ExportSheetsDialog
void CopyOneSheet(QString &SrcSection, int par_SrcFd,
                  QString &DstSection, int par_DstFd,
                  QString &DstSheetName, bool ImportFlag);
void AddWindowToCorrespondAllList(const char *WidgetName, const char *SectionName, int DstFile);


#endif // IMPORTSHEETSDIALOG_H

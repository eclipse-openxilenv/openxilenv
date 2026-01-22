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


#ifndef DBCIMPORTDIALOG_H
#define DBCIMPORTDIALOG_H

#include "Dialog.h"

namespace Ui {
class DbcImportDialog;
}

class DbcImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DbcImportDialog(QString &par_Filename, QWidget *parent = nullptr);
    ~DbcImportDialog();
    char *GetTransmitBusMembersList();
    char *GetReceiveBusMembersList();

    QString AdditionalSignalPrefix();
    QString AdditionalSignalPostfix();

    int TransferSettingsVarianteIndex();
    bool ExtendSignalNameWithObject();
    bool ObjectAdditionalEquations();
    bool ObjectInitData();
    bool SignalInitValues();
    bool SignalDatatType();
    bool SignalEquations();
    bool SortSignals();
    bool ExportToFile();
    QString ExportToFileName();
    QString VariantName();
    bool ObjectNameDoublePoint();

    int DbcImportDialogFillComboboxWithVariante();

public slots:
    void accept();

private slots:
    void on_AddTransmitBusMemberPushButton_clicked();

    void on_RemoveTransmitBusMemberPushButton_clicked();

    void on_AddReceivetBusMemberPushButton_clicked();

    void on_RemoveReceiveBusMemberPushButton_clicked();

    void on_TransferSettingsCheckBox_toggled(bool checked);

    void on_FilePushButton_clicked();

private:
    Ui::DbcImportDialog *ui;
    QString m_Filename;
};

#endif // DBCIMPORTDIALOG_H

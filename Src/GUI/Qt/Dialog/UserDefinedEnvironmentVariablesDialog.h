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


#ifndef USERDEFINEDENVIRONMENTVARIABLEDIALOG_H
#define USERDEFINEDENVIRONMENTVARIABLEDIALOG_H

#include <QDialog>
#include <Dialog.h>
#include <QListWidgetItem>

namespace Ui {
class UserDefinedEnvironmentVariableDialog;
}

class UserDefinedEnvironmentVariableDialog : public Dialog
{
    Q_OBJECT

public:
    explicit UserDefinedEnvironmentVariableDialog(QWidget *parent = nullptr);
    ~UserDefinedEnvironmentVariableDialog();

private slots:
    void on_buttonBox_accepted();

    void on_pushButtonImport_clicked();

    void on_pushButtonExport_clicked();

    void on_pushButtonDelete_clicked();

    void on_pushButtonChange_clicked();

    void on_pushButtonAdd_clicked();

    void on_listWidgetEnvironmentVariables_itemClicked(QListWidgetItem *item);

private:
    Ui::UserDefinedEnvironmentVariableDialog *ui;
    int SaveListBox (int par_Fd, int OnlySelectedFlag, int DeleteOldEntrysFlag);
    void RemoveWhitespacesOnlyAtBeginingAndEnd (char *p);
    int FillListBox (int par_Fd, int ResetFlag);
};

#endif // USERDEFINEDENVIRONMENTVARIABLEDIALOG_H

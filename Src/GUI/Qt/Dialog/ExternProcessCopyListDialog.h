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


#ifndef EXTERNPROCESSCOPYLISTDIALOG_H
#define EXTERNPROCESSCOPYLISTDIALOG_H

#include <QDialog>

namespace Ui {
class ExternProcessCopyListDialog;
}

class ExternProcessCopyListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExternProcessCopyListDialog(QString par_ProcessName, QWidget *parent = nullptr);
    ~ExternProcessCopyListDialog();

private slots:
    void on_Filter1PushButton_clicked();

    void on_Filter2pushButton_clicked();

    void on_ProcessComboBox_currentIndexChanged(const QString &arg1);

    void on_CopyListComboBox_currentIndexChanged(const QString &arg1);

private:
    Ui::ExternProcessCopyListDialog *ui;

    void FillTable(QString par_ProcessName, int par_ListType, QString par_Filter, unsigned long long par_AddressFrom, unsigned long long par_AddressTo);
    void FillTable();

};

#endif // EXTERNPROCESSCOPYLISTDIALOG_H

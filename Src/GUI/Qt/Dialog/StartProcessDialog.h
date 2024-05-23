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


#ifndef STARTPROCESSDIALOG_H
#define STARTPROCESSDIALOG_H

#include <QDialog>
#include <Dialog.h>

namespace Ui {
class StartProcessDialog;
}

class StartProcessDialog : public Dialog
{
    Q_OBJECT

public:
    explicit StartProcessDialog(QWidget *parent = nullptr);
    ~StartProcessDialog();

private slots:
    void on_CancelPushButton_clicked();

    void on_StartExternalPRocessPushButton_clicked();

    void on_StartPushButton_clicked();

    void on_AvailableInternalProcessesListWidget_itemSelectionChanged();

private:
    Ui::StartProcessDialog *ui;
};

#endif // STARTPROCESSDIALOG_H

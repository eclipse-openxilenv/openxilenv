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


#ifndef LOADXCPCONFIGDIALOG_H
#define LOADXCPCONFIGDIALOG_H

#include <Dialog.h>

namespace Ui {
class LoadXCPConfigDialog;
}

class LoadXCPConfigDialog : public Dialog
{
    Q_OBJECT

public:
    explicit LoadXCPConfigDialog(QWidget *parent = nullptr);
    ~LoadXCPConfigDialog();

private slots:
    void on_pushButtonBrowse_clicked();

private:
    Ui::LoadXCPConfigDialog *ui;
    void accept();
};

#endif // LOADXCPCONFIGDIALOG_H

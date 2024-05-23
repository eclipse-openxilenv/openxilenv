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


#ifndef USERDEFINEDBBVARIABLEDIALOG_H
#define USERDEFINEDBBVARIABLEDIALOG_H

#include <QStringList>
#include <QComboBox>
#include "Dialog.h"

namespace Ui {
class UserDefinedBBVariableDialog;
}

class UserDefinedBBVariableDialog : public Dialog
{
    Q_OBJECT

public:
    explicit UserDefinedBBVariableDialog(QWidget *parent = nullptr);
    ~UserDefinedBBVariableDialog();
    int get_rowCount();
    void set_rowCount(int par_rowCount);
    void increment_rowCount();
    void decrement_rowCount();

public slots:
    void accept();

private slots:
    void on_pushButtonNew_clicked();
    void on_pushButtonDelete_clicked();

private:
    Ui::UserDefinedBBVariableDialog *ui;
    int rowCount;
    void FillTableView();
    void initComboBox(QComboBox *par_comboBox);
};

#endif // USERDEFINEDBBVARIABLEDIALOG_H

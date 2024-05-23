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


#ifndef LOADVALUESDIALOG_H
#define LOADVALUESDIALOG_H

#include <QDialog>
#include <Dialog.h>

extern "C"
{
    #include "IniDataBase.h"
}

namespace Ui {
class LoadValuesDialog;
}

class LoadValuesDialog : public Dialog
{
    Q_OBJECT

public:
    explicit LoadValuesDialog(QWidget *parent = nullptr);
    ~LoadValuesDialog();

private slots:
    void on_pushButtonFileSVL_clicked();

    void on_pushButtonFileSymbol_clicked();

    void on_pushButtonFileBinary_clicked();

public slots:
    void accept();

private:
    int LittleBigEndian;
    char GpLabelAddressStr[MAX_PATH];
    Ui::LoadValuesDialog *ui;
};

#endif // LOADVALUESDIALOG_H

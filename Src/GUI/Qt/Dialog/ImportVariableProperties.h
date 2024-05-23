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


#ifndef IMPORTVARIABLEPROPERTIES_H
#define IMPORTVARIABLEPROPERTIES_H

#include <QDialog>
#include <Dialog.h>


namespace Ui {
class importvariableproperties;
}

class importvariableproperties : public Dialog
{
    Q_OBJECT

public:
    explicit importvariableproperties(QWidget *parent = nullptr);
    ~importvariableproperties();

private slots:
    void on_pushButtonRight_clicked();

    void on_pushButtonLeft_clicked();

    void on_pushButtonSelectAllAvailable_clicked();

    void on_pushButtonSelectAllImport_clicked();

    void on_buttonBox_accepted();

    void on_pushButtonFilter_clicked();

    void on_pushButtonFile_clicked();

    void on_checkBoxExistingOnly_toggled(bool checked);

private:
    Ui::importvariableproperties *ui;
    int FillListBox (const char *File, int OnlyExistingVariablesFlag, const char *Filter);
};

#endif // IMPORTVARIABLEPROPERTIES_H

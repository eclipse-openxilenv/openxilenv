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


#ifndef CANCONFIGJ1939ADDITIONALSDIALOG_H
#define CANCONFIGJ1939ADDITIONALSDIALOG_H

#include <QDialog>

class CanConfigObject;

namespace Ui {
class CanConfigJ1939AdditionalsDialog;
}

class CanConfigJ1939AdditionalsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CanConfigJ1939AdditionalsDialog(CanConfigObject *par_CanObject, QWidget *parent = nullptr);
    ~CanConfigJ1939AdditionalsDialog();

public slots:
    void accept();

private slots:
    void on_VariableDlcCheckBox_toggled(bool checked);

    void on_DestAddrFixedRadioButton_toggled(bool checked);

    void on_DestAddrVariableRadioButton_toggled(bool checked);

private:
    Ui::CanConfigJ1939AdditionalsDialog *ui;

    CanConfigObject *m_CanObject;
};

#endif // CANCONFIGJ1939ADDITIONALSDIALOG_H

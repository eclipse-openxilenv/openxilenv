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


#ifndef USERDRAWCONFIGXYPLOTELEMENTDIALOG_H
#define USERDRAWCONFIGXYPLOTELEMENTDIALOG_H

#include "Dialog.h"

namespace Ui {
class UserDrawConfigXYPlotElementDialog;
}

class UserDrawConfigXYPlotElementDialog : public Dialog
{
    Q_OBJECT
public:
    UserDrawConfigXYPlotElementDialog(QString &par_Parameter, QWidget *parent = nullptr);
    ~UserDrawConfigXYPlotElementDialog();
    QString Get();

public slots:

private slots:
    void on_SetXPushButton_clicked();

    void on_SetYPushButton_clicked();

    void on_ColorPushButton_clicked();

    void on_ColorLineEdit_editingFinished();

private:
    bool Parse();
    void SetColorView (QColor color);
    Ui::UserDrawConfigXYPlotElementDialog *ui;

    QString m_Parameter;
};

#endif // USERDRAWCONFIGXYPLOTELEMENTDIALOG_H

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


#ifndef CONFIGUREBARRIERDIALOG_H
#define CONFIGUREBARRIERDIALOG_H

#include <QDialog>
#include <Dialog.h>

namespace Ui {
class ConfigureBarrierDialog;
}

class ConfigureBarrierDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ConfigureBarrierDialog(QWidget *parent = nullptr);
    ~ConfigureBarrierDialog();

public slots:
    void accept();

private slots:
    void on_AddBarrierPushButton_clicked();

    void on_DeleteBarrierPushButton_clicked();

private:
    Ui::ConfigureBarrierDialog *ui;

    QList <QString> m_BarriersSave;
};

#endif // CONFIGUREBARRIERDIALOG_H

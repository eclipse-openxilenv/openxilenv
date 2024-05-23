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


#ifndef IMEXPORTCONTROLLAMPSDIALOG_H
#define IMEXPORTCONTROLLAMPSDIALOG_H

#include <QDialog>
#include "Dialog.h"
#include "LampsModel.h"

namespace Ui {
class ImExportControlLampsDialog;
}

class ImExportControlLampsDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ImExportControlLampsDialog(QWidget *parent = nullptr);
    ~ImExportControlLampsDialog();

public slots:
    void accept();
    void reject();

private slots:
    void on_ChoicePushButton_clicked();

    void on_ImportMovePushButton_clicked();

    void on_ExportMovePushButton_clicked();

    void on_ImportCopyPushButton_clicked();

    void on_ExportCopyPushButton_clicked();

    void on_DeletePushButton_clicked();

    void on_ExternalListView_clicked(const QModelIndex &index);

    void on_InternalListView_clicked(const QModelIndex &index);

    void on_NewPushButton_clicked();

private:
    void EnableDisableButtons(bool par_Enable);
    QString AvoidDuplicatedNames(ControlLampModel *arg_model, QString &arg_name);

    Ui::ImExportControlLampsDialog *ui;

    QString m_Filename;
    int m_Fd;
    ControlLampModel *m_InternalModel;
    ControlLampModel *m_ExternalModel;
};

#endif // IMEXPORTCONTROLLAMPSDIALOG_H

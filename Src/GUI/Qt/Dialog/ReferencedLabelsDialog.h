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


#ifndef REFERENCEDLABELSDIALOG_H
#define REFERENCEDLABELSDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <Dialog.h>

extern "C" {
#include "DebugInfos.h"
}


namespace Ui {
class ReferencedLabelsDialog;
}

class ReferencedLabelsDialogModel : public QStandardItemModel
{
public:
    ReferencedLabelsDialogModel(QObject* parent = nullptr)
        : QStandardItemModel(parent)
    {}

    Qt::ItemFlags flags(const QModelIndex &index) const {
        if(index.column() < 1)
            return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
        else
            return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;
    }
};

class ReferencedLabelsDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ReferencedLabelsDialog(QString par_ProcessName, QWidget *parent = 0);
    ~ReferencedLabelsDialog();


private slots:
    void accept();

    void on_ProcessComboBox_currentIndexChanged(const QString &arg_ProcessName);

    void on_DereferenceLabelPushButton_clicked();

private:
    void CreateModel ();
    void FillModel (const QString &par_Process);
    void WriteModelToIni (const QString &par_Process);
    int UnreferenceOneVariable (int Pid, DEBUG_INFOS_ASSOCIATED_CONNECTION *DebugInfos, char *VariName, char *DisplayName);
    int RenameReferenceOneVariable (int Pid, DEBUG_INFOS_ASSOCIATED_CONNECTION *DebugInfos, const char *VariName,
                                   const char *OldDisplayName, const char *NewDisplayName);

    Ui::ReferencedLabelsDialog *ui;

    ReferencedLabelsDialogModel *m_Model;

    QString m_ProcessName;
};

#endif // REFERENCEDLABELSDIALOG_H

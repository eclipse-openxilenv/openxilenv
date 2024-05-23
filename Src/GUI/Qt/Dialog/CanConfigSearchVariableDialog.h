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


#ifndef CANCONFIGSEARCHVARIABLEDIALOG_H
#define CANCONFIGSEARCHVARIABLEDIALOG_H

#include <QDialog>
#include <QModelIndex>

//#include "CanConfigDialog.h"

class CanConfigTreeModel;

namespace Ui {
class CanConfigSearchVariableDialog;
}

class CanConfigSearchVariableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CanConfigSearchVariableDialog(CanConfigTreeModel *par_Model, QModelIndex par_BaseIndex, QWidget *parent = nullptr);
    ~CanConfigSearchVariableDialog();

signals:
    void SelectSignal(QModelIndex &par_Signal);

private slots:
    void on_SeachPushButton_clicked();

    void on_ClosePushButton_clicked();

private:
    Ui::CanConfigSearchVariableDialog *ui;
    QModelIndex m_BaseIndex;
    QModelIndex m_SerchPos;
    CanConfigTreeModel *m_Model;
};

#endif // CANCONFIGSEARCHVARIABLEDIALOG_H

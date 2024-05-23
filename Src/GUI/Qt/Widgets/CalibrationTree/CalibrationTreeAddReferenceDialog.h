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


#ifndef CALLIBRATIONTREEADDREFERENCEDIALOG_H
#define CALLIBRATIONTREEADDREFERENCEDIALOG_H

#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
class CalibrationTreeAddReferenceDialog;
}

class CalibrationTreeAddReferenceDialogModel : public QStandardItemModel
{
public:
    CalibrationTreeAddReferenceDialogModel(QObject* parent = nullptr)
        : QStandardItemModel(parent)
    {}

    Qt::ItemFlags flags(const QModelIndex &index) const {
        if(index.column() < 1)
            return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
        else
            return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;
    }
};

class CalibrationTreeAddReferenceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationTreeAddReferenceDialog(QStringList &par_Labels, QWidget *parent = nullptr);
    ~CalibrationTreeAddReferenceDialog();

    QStringList GetRenamings();
private:
    Ui::CalibrationTreeAddReferenceDialog *ui;

    CalibrationTreeAddReferenceDialogModel *m_Model;

};

#endif // CALLIBRATIONTREEADDREFERENCEDIALOG_H

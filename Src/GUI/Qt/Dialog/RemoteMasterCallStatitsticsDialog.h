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


#ifndef REMOTEMASTERCALLSTATITSTICSDIALOG_H
#define REMOTEMASTERCALLSTATITSTICSDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>

namespace Ui {
class RemoteMasterCallStatitsticsDialog;
}

class RemoteMasterCallStatitsticsModel : public QAbstractTableModel
{
public:
    RemoteMasterCallStatitsticsModel();
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
private:
    uint64_t m_Freq;
    double m_Convesion;
};

class RemoteMasterCallStatitsticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteMasterCallStatitsticsDialog(QWidget *parent = nullptr);
    ~RemoteMasterCallStatitsticsDialog();

private slots:
    void on_ResetPushButton_clicked();

private:
    Ui::RemoteMasterCallStatitsticsDialog *ui;
    RemoteMasterCallStatitsticsModel *m_Model;
};

#endif // REMOTEMASTERCALLSTATITSTICSDIALOG_H

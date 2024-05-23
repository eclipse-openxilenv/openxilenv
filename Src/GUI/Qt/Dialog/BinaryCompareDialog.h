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


#ifndef BINARYCOMPAREDIALOG_H
#define BINARYCOMPAREDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>
#include "Dialog.h"

class BinaryCompareModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    BinaryCompareModel();
    ~BinaryCompareModel() Q_DECL_OVERRIDE;

    void SetDispayBuffer (QByteArray *arg_DispBuffer);
    void SetRefBuffer (QByteArray *arg_RefBuffer);

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void beginUpdate();
    void endUpdate();

private:
    QByteArray *m_DispBuffer;
    QByteArray *m_RefBuffer;
};


namespace Ui {
class BinaryCompareDialog;
}

class BinaryCompareDialog : public Dialog
{
    Q_OBJECT

public:
    explicit BinaryCompareDialog(QWidget *parent = nullptr);
    ~BinaryCompareDialog();

private slots:
    void on_File1OpenPushButton_clicked();

    void on_File2OpenPushButton_clicked();

    void on_UpPushButton_clicked();

    void on_DownPushButton_clicked();

private:
    void LoadFiles();
    Ui::BinaryCompareDialog *ui;

    void LoadFile(QString &par_Filename, int par_Number);
    BinaryCompareModel m_File1Model;
    BinaryCompareModel m_File2Model;
    QByteArray m_File1Image;
    QByteArray m_File2Image;
};

#endif // BINARYCOMPAREDIALOG_H

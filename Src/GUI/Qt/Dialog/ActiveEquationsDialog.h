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


#ifndef ACTIVEEQUATIONSDIALOG_H
#define ACTIVEEQUATIONSDIALOG_H

#include <QDialog>
#include <QAbstractItemModel>

extern "C" {
#include "EquationList.h"
#include "Wildcards.h"
}

namespace Ui {
class ActiveEquationsDialog;
}

class ActiveEquationsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ActiveEquationsModel(QObject *parent = nullptr);
    ~ActiveEquationsModel() Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void UpdateSnapShot(INCLUDE_EXCLUDE_FILTER *par_Filter, int par_TypeMask);

private:
    int m_NumberOfActiveEquations;
    ACTIVE_EQUATION_ELEM *m_ActiveEquationSnapShot;
    int m_SizeOfActiveEquationSnapShot;
    int m_StringBufferSize;
    char *m_StringBuffer;

    QPixmap *m_AnalogPixmap;
    QPixmap *m_BbPixmap;
    QPixmap *m_BeforePixmap;
    QPixmap *m_BehindPixmap;
    QPixmap *m_CanPixmap;
    QPixmap *m_GlobalPixmap;
    QPixmap *m_RampePixmap;
    QPixmap *m_UnknownPixmap;
    QPixmap *m_WaitUntilPixmap;
};


class ActiveEquationsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActiveEquationsDialog(QWidget *parent = nullptr);
    ~ActiveEquationsDialog();

    static void Open (QWidget *Parent);

private slots:
    void on_RefreshPushButton_clicked();

    void on_Filter_clicked();

    void on_ClosePushButton_clicked();

private:
    void closeEvent(QCloseEvent *event);

    int GetTypeMask();

    void SetTypeMask(int par_Mask);

    Ui::ActiveEquationsDialog *ui;

    ActiveEquationsModel *m_Model;
    INCLUDE_EXCLUDE_FILTER *m_Filter;
};




#endif // ACTIVEEQUATIONSDIALOG_H

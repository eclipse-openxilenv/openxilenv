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


#ifndef BLACKBOARDACCESSDIALOG_H
#define BLACKBOARDACCESSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QListWidgetItem>
#include <QSortFilterProxyModel>
#include <Dialog.h>

namespace Ui {
class BlackboardAccessDialog;
}

class OnProcessVaraible {
public:
    int Pid;
    QStringList Enable;
    QStringList EnableSave;
    QStringList Disable;
    QStringList DisableSave;
};

class ProcessesVariable {
public:
    QMap <QString, OnProcessVaraible*> m_Data;
};

class BlackboardAccessModel : public QAbstractListModel
{
    Q_OBJECT
public:
    BlackboardAccessModel(OnProcessVaraible *par_ProcessesVariable, bool par_EnabledOrDisabled, QObject *arg_parent = nullptr);
    ~BlackboardAccessModel() Q_DECL_OVERRIDE;

public:
    virtual int rowCount(const QModelIndex& arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex& arg_index, int arg_role) const Q_DECL_OVERRIDE;
    void SelectProcess(OnProcessVaraible *par_SelectedOnProcessVaraible);

    void AddVariables(QStringList &par_Variables);
    void RemoveVariables(QStringList &par_Variables);

public slots:

private:
    OnProcessVaraible *m_SelectedOnProcessVaraible;
    bool m_EnabledOrDisabled;
};


class BlackboardAccessDialog : public Dialog
{
    Q_OBJECT

public:
    explicit BlackboardAccessDialog(QWidget *parent = nullptr);
    ~BlackboardAccessDialog();

public slots:
    void accept();

private slots:
    void on_ProcessListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_ToDisablePushButton_clicked();

    void on_ToEnablePushButton_clicked();

    void on_SelectAllEnablePushButton_clicked();

    void on_SelectAllDisablePushButton_clicked();

    void SetEnableFilter(QString par_Filter);
    void SetDisableFilter(QString par_Filter);

private:
    Ui::BlackboardAccessDialog *ui;

    ProcessesVariable m_Data;

    BlackboardAccessModel *m_EnableModel;
    BlackboardAccessModel *m_DisableModel;

    QSortFilterProxyModel *m_EnableProxiModel;
    QSortFilterProxyModel *m_DisableProxiModel;

    void NewProcessSelected (QString par_ProcessName);
};

#endif // BLACKBOARDACCESSDIALOG_H

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


#include "BlackboardAccessDialog.h"
#include "ui_BlackboardAccessDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "MainValues.h"
#include "Blackboard.h"
#include "Scheduler.h"
#include "Wildcards.h"
}


BlackboardAccessDialog::BlackboardAccessDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::BlackboardAccessDialog)
{
    ui->setupUi(this);

    connect(ui->EnableFilterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(SetEnableFilter(QString)));
    connect(ui->DisableFilterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(SetDisableFilter(QString)));

    m_EnableModel = new BlackboardAccessModel(nullptr, true, this);
    m_DisableModel = new BlackboardAccessModel(nullptr, false, this);

    m_EnableProxiModel = new QSortFilterProxyModel(this);
    m_DisableProxiModel = new QSortFilterProxyModel(this);

    m_EnableProxiModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_EnableProxiModel->sort(0);
    m_DisableProxiModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_DisableProxiModel->sort(0);


    m_EnableProxiModel->setSourceModel(m_EnableModel);
    m_DisableProxiModel->setSourceModel(m_DisableModel);

    ui->EnableListView->setModel(m_EnableProxiModel);
    ui->DisableListView->setModel(m_DisableProxiModel);

    // First add all running processes
    char *pName;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((pName = read_next_process_name (Buffer)) != nullptr) {
        QString Name = CharToQString(pName);
        ui->ProcessListWidget->addItem (Name);
    }
    close_read_next_process_name(Buffer);

}

BlackboardAccessDialog::~BlackboardAccessDialog()
{
    delete ui;
    delete m_EnableProxiModel;
    delete m_DisableProxiModel;
    delete m_EnableModel;
    delete m_DisableModel;

    QMapIterator <QString, OnProcessVaraible*> i(m_Data.m_Data);
    while(i.hasNext()) {
        i.next();
        OnProcessVaraible* v = i.value();
        delete v;
    }
}


void BlackboardAccessDialog::NewProcessSelected (QString par_ProcessName)
{
    char szVarName[BBVARI_NAME_SIZE];
    int index;

    // If not loaded than load it now
    if (!m_Data.m_Data.contains (par_ProcessName)) {
        OnProcessVaraible *opv = new OnProcessVaraible;
        // determine the prozess-ID
        if ((opv->Pid = get_pid_by_name (QStringToConstChar(par_ProcessName))) > 0) {
            // loop through all variables
            index = 0;
            while ((index = ReadNextVariableProcessAccess (index, opv->Pid, 1, szVarName, sizeof(szVarName))) > 0) {
                opv->Enable.append (CharToQString(szVarName));
            }
            opv->EnableSave = opv->Enable;
            // loop through all variables
            index = 0;
            while ((index = ReadNextVariableProcessAccess (index, opv->Pid, 2, szVarName, sizeof(szVarName))) > 0) {
                opv->Disable.append (CharToQString(szVarName));
            }
            opv->DisableSave = opv->Disable;
            m_Data.m_Data.insert (par_ProcessName, opv);
            m_EnableModel->SelectProcess(opv);
            m_DisableModel->SelectProcess(opv);
        }
    } else {
        OnProcessVaraible *opv = m_Data.m_Data.value(par_ProcessName);
        m_EnableModel->SelectProcess(opv);
        m_DisableModel->SelectProcess(opv);
    }
}


void BlackboardAccessDialog::on_ProcessListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    if (current != nullptr) {
        QString ProcessName = current->data(Qt::DisplayRole).toString();
        NewProcessSelected (ProcessName);
    }
}

void BlackboardAccessDialog::on_ToDisablePushButton_clicked()
{
    QModelIndexList Selected = ui->EnableListView->selectionModel()->selectedIndexes();
    QStringList MoveList;
    foreach (QModelIndex Index, Selected) {
        QString Name = m_EnableProxiModel->data(Index, Qt::DisplayRole).toString();
        MoveList.append(Name);
    }
    m_EnableModel->RemoveVariables(MoveList);
    m_DisableModel->AddVariables(MoveList);
}

void BlackboardAccessDialog::on_ToEnablePushButton_clicked()
{
    QModelIndexList Selected = ui->DisableListView->selectionModel()->selectedIndexes();
    QStringList MoveList;
    foreach (QModelIndex Index, Selected) {
        QString Name = m_DisableProxiModel->data(Index, Qt::DisplayRole).toString();
        MoveList.append(Name);
    }
    m_DisableModel->RemoveVariables(MoveList);
    m_EnableModel->AddVariables(MoveList);
}

void BlackboardAccessDialog::on_SelectAllEnablePushButton_clicked()
{
    ui->EnableListView->selectAll();
}

void BlackboardAccessDialog::on_SelectAllDisablePushButton_clicked()
{
    ui->DisableListView->selectAll();
}

void BlackboardAccessDialog::SetEnableFilter(QString par_Filter)
{
     m_EnableProxiModel->setFilterWildcard(par_Filter);
}

void BlackboardAccessDialog::SetDisableFilter(QString par_Filter)
{
     m_DisableProxiModel->setFilterWildcard(par_Filter);
}

void BlackboardAccessDialog::accept()
{
    // First store the selected process
    QList<QListWidgetItem*> SelProc = ui->ProcessListWidget->selectedItems();
    foreach (OnProcessVaraible *ProcessItem, m_Data.m_Data) {
        foreach (QString Variable, ProcessItem->Enable) {
            if (!ProcessItem->EnableSave.contains (Variable) &&
                !ProcessItem->Disable.contains (Variable) &&
                ProcessItem->DisableSave.contains (Variable)) {  // Was moved from disable to the enable list
                enable_bbvari_access (ProcessItem->Pid, get_bbvarivid_by_name (QStringToConstChar(Variable)));
            }
        }
        foreach (QString Variable, ProcessItem->Disable) {
            if (!ProcessItem->DisableSave.contains (Variable) &&
                !ProcessItem->Enable.contains (Variable) &&
                ProcessItem->EnableSave.contains (Variable)) {  // Was moved from enable to the disable list
                disable_bbvari_access (ProcessItem->Pid, get_bbvarivid_by_name (QStringToConstChar(Variable)));
            }
        }
    }
    QDialog::accept();
}


BlackboardAccessModel::BlackboardAccessModel(OnProcessVaraible *par_ProcessesVariable, bool par_EnabledOrDisabled, QObject *arg_parent) :
    QAbstractListModel(arg_parent)
{
    m_SelectedOnProcessVaraible = par_ProcessesVariable;
    m_EnabledOrDisabled = par_EnabledOrDisabled;
}

BlackboardAccessModel::~BlackboardAccessModel()
{

}

int BlackboardAccessModel::rowCount(const QModelIndex &arg_parent) const
{
    Q_UNUSED(arg_parent);
    if (m_SelectedOnProcessVaraible == nullptr) {
        return 0;
    } else {
        if (m_EnabledOrDisabled) {
            return m_SelectedOnProcessVaraible->Enable.size();
        } else {
            return m_SelectedOnProcessVaraible->Disable.size();
        }
    }
}

QVariant BlackboardAccessModel::data(const QModelIndex &arg_index, int arg_role) const
{
    if(arg_index.isValid()) {
        switch(arg_role) {
        case Qt::DisplayRole:
            if (m_SelectedOnProcessVaraible != nullptr) {
                if (m_EnabledOrDisabled) {
                    return m_SelectedOnProcessVaraible->Enable.at(arg_index.row());
                } else {
                    return m_SelectedOnProcessVaraible->Disable.at(arg_index.row());
                }
            }
            break;
        }
    }
    return QVariant();
}

void BlackboardAccessModel::SelectProcess(OnProcessVaraible *par_SelectedOnProcessVaraible)
{
    beginResetModel();
    m_SelectedOnProcessVaraible = par_SelectedOnProcessVaraible;
    endResetModel();
}

void BlackboardAccessModel::AddVariables(QStringList &par_Variables)
{
    if (m_SelectedOnProcessVaraible != nullptr) {
        beginResetModel();
        if (m_EnabledOrDisabled) {
            m_SelectedOnProcessVaraible->Enable.append(par_Variables);
        } else {
            m_SelectedOnProcessVaraible->Disable.append(par_Variables);
        }
        endResetModel();
    }
}

void BlackboardAccessModel::RemoveVariables(QStringList &par_Variables)
{
    if (m_SelectedOnProcessVaraible != nullptr) {
        beginResetModel();
        if (m_EnabledOrDisabled) {
            foreach(QString Name, par_Variables) {
                m_SelectedOnProcessVaraible->Enable.removeAll(Name);
            }
        } else {
            foreach(QString Name, par_Variables) {
                m_SelectedOnProcessVaraible->Disable.removeAll(Name);
            }
        }
        endResetModel();
    }
}


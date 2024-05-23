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


#ifndef CANCONFIGJ1939MULTIPGDIALOG_H
#define CANCONFIGJ1939MULTIPGDIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>

namespace Ui {
class CanConfigJ1939MultiPGDialog;
}


class CheckableSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CheckableSortFilterProxyModel(QObject *parent = nullptr);
    ~CheckableSortFilterProxyModel() Q_DECL_OVERRIDE;
    void SetCheckedObjectIds(QList<int> &par_CheckedObjectIds);

    virtual QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex &arg_index, const QVariant &arg_value, int arg_role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const Q_DECL_OVERRIDE;

    void setCheckedState(int par_ObjectId, bool arg_checked);
    void setCheckedState(QModelIndex par_Index, bool arg_checked);

    Qt::CheckState currentCheckState(QModelIndex par_Index) const;

    void SetOnly_C_PG(bool par_Flag, int par_WriteOrRead) { m_Only_C_PG = par_Flag; m_WriteOrRead = par_WriteOrRead; }

    QList<int> Get_C_PGs();
    void Set_C_PGs(QList<int> &par_PGs);

signals:
    void checkedNodesChanged();
    void checkedStateChanged(int par_ObjectID, bool arg_state);

protected:
 bool filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const Q_DECL_OVERRIDE;
 //bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;


private:
    QList<int> m_CheckedCanObjects;
    bool m_Only_C_PG;
    int m_WriteOrRead;  // 0 -> write 1 -> read
};

class CanConfigTreeModel;

class CanConfigJ1939MultiPGDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CanConfigJ1939MultiPGDialog(CanConfigTreeModel *par_Model, QModelIndex &par_Index, int par_WriteOrRead,
                                         QList<int> &par_C_PGs, QWidget *parent = nullptr);
    ~CanConfigJ1939MultiPGDialog();
    QList<int> Get_C_PGs();

private:
    Ui::CanConfigJ1939MultiPGDialog *ui;
    CheckableSortFilterProxyModel *m_ProxyModel;
};

#endif // CANCONFIGJ1939MULTIPGDIALOG_H

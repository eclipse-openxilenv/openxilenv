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


#ifndef CHECKABLESORTFILTERPROXYMODEL_H
#define CHECKABLESORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

extern "C" {
#include "Blackboard.h"
//#include "Wildcards.h"
}

class CheckableSortFilterProxyModel2 : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CheckableSortFilterProxyModel2(QObject *parent = nullptr);
    ~CheckableSortFilterProxyModel2() Q_DECL_OVERRIDE;
    void SetCheckedVariableNames(QStringList &par_CheckedVariableNames);

    virtual QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex &arg_index, const QVariant &arg_value, int arg_role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const Q_DECL_OVERRIDE;

    void setCheckedState(QString arg_VariableName, bool arg_checked);
    void setCheckedState(QModelIndex arg_proxyModelIndex, bool arg_checked);

    Qt::CheckState currentCheckState(QModelIndex arg_modelIndex) const;

    void resetToDefault(void);

    void ResetProxyModel();

    void setMultiSelect(bool par_MultiSelect) {m_multiSelect = par_MultiSelect;}

    void SetCheckable(bool par_Checkable);

    void SetFilterConversionType(enum BB_CONV_TYPES par_ConversionType);
signals:
    void checkedNodesChanged();
    void checkedStateChanged(QString arg_name, bool arg_state);

public slots:
    void SetOnlyVisable(bool par_OnlyVisable);
    void SetFilter(const QString &par_Filter);

protected:
 bool filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const Q_DECL_OVERRIDE;
 bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;


private:
    bool m_Checkable;
    QStringList m_CheckedVariableNames;
    enum BB_CONV_TYPES m_ConversionType;
    bool m_OnlyVisable;
    bool m_multiSelect;
    QString m_Filter;
    char m_OldFilter[512];
};

#endif // CHECKABLESORTFILTERPROXYMODEL_H

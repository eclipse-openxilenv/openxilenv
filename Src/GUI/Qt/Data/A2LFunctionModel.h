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


#ifndef A2LFUNCTIONMODEL_H
#define A2LFUNCTIONMODEL_H

#include <QString>
#include <QAbstractListModel>
#include <QStringListModel>
#include <QSortFilterProxyModel>

extern "C" {
#include "Wildcards.h"
}


class A2LFunctionElement
{
public:
    int m_LinkNr;
    int m_Index;
    QString m_Name;
    int operator==(const A2LFunctionElement &x) const;
    int operator<(const A2LFunctionElement &x) const;
    int operator>(const A2LFunctionElement &x) const;
};


class A2LFunctionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    A2LFunctionModel(QObject *arg_parent = nullptr);
    ~A2LFunctionModel() Q_DECL_OVERRIDE;
    void InitForProcess(QStringList &par_Processes);

public:
    virtual int rowCount(const QModelIndex& arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex& arg_index, int arg_role) const Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex& arg_index) const Q_DECL_OVERRIDE;

    QModelIndex indexOfVariable(const QString &arg_Name);
    void AddVariableByName(const QString &arg_Name);
    bool DeleteVariableByName(const QString &arg_Name);
    bool ExistVariableWithName(const QString &arg_Name);

    void CopyElements(QList<A2LFunctionElement> *par_ElementsToCopy);

public slots:

private:
    QList <A2LFunctionElement> m_Elements;
};

#endif // BLACKBOARDVARIABLEMODEL_H

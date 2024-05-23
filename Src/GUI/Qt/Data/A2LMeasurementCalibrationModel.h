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


#ifndef A2LMEASUREMENTCALIBRATIONMODEL_H
#define A2LMEASUREMENTCALIBRATIONMODEL_H

#include <QString>
#include <QAbstractListModel>
#include <QStringListModel>
#include <QSortFilterProxyModel>

extern "C" {
#include "Wildcards.h"
}


class A2LVariableElemente
{
public:
    int m_LinkNr;
    int m_Index;
    int m_Pid;
    bool m_IsReferenced;
    QString m_Name;
    bool operator==(const A2LVariableElemente &x);
    bool operator<(const A2LVariableElemente &x);
    bool operator>(const A2LVariableElemente &x);
};


class A2LMeasurementCalibrationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    A2LMeasurementCalibrationModel(QObject *arg_parent = nullptr);
    ~A2LMeasurementCalibrationModel() Q_DECL_OVERRIDE;
    void SetProcessesAndTypes(QStringList &par_Processes, int par_Types, QStringList &par_Functions, int par_ReferencedFlags);
    int GetIndexPidLink(QModelIndex &par_Index, int *ret_Pid, int *ret_Link) const;
    void ReferenceChanged(int par_Row);

    virtual int rowCount(const QModelIndex& arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex& arg_index, int arg_role) const Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex& arg_index) const Q_DECL_OVERRIDE;

    QModelIndex indexOfVariable(const QString &arg_Name);
    void AddVariableByName(const QString &arg_Name);
    bool DeleteVariableByName(const QString &arg_Name);
    bool ExistVariableWithName(const QString &arg_Name);

    void CopyElements(QList<A2LVariableElemente> *par_ElementsToCopy);

public slots:

private:
   void FreeList();
    QList <A2LVariableElemente*> m_Elements;
};

#endif // BLACKBOARDVARIABLEMODEL_H

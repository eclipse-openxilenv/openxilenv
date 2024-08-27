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


#ifndef BLACKBOARDVARIABLEMODEL_H
#define BLACKBOARDVARIABLEMODEL_H

#include <QAbstractListModel>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QIcon>
#include "BlackboardObserver.h"

extern "C" {
#include "Blackboard.h"
#include "Wildcards.h"
}

class VariableElemente
{
public:
    int m_Vid;
    enum BB_DATA_TYPES m_DataType;
    enum BB_CONV_TYPES m_ConversionType;
    QString m_Name;
    bool operator==(const VariableElemente &x) const;
    bool operator<(const VariableElemente &x) const;
    bool operator>(const VariableElemente &x) const;
};

class BlackboardVariableModel : public QAbstractListModel
{
    Q_OBJECT
public:
    BlackboardVariableModel(QObject *arg_parent = nullptr);
    ~BlackboardVariableModel() Q_DECL_OVERRIDE;

public:
    virtual int rowCount(const QModelIndex& arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex& arg_index, int arg_role) const Q_DECL_OVERRIDE;
    virtual QStringList mimeTypes() const Q_DECL_OVERRIDE;
    virtual QMimeData*mimeData(const QModelIndexList& arg_indexes) const Q_DECL_OVERRIDE;
    virtual Qt::DropActions supportedDragActions() const Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex& arg_index) const Q_DECL_OVERRIDE;

    QModelIndex indexOfVariable(const QString &arg_Name);
    void AddVariableByName(const QString &arg_Name);
    bool DeleteVariableByName(const QString &arg_Name);
    bool ExistVariableWithName(const QString &arg_Name);

    void CopyElements(QList<VariableElemente> *par_ElementsToCopy);
    BlackboardVariableModel *MakeASnapshotCopy();

    void InsertVid(int arg_vid);
    void RemoveVid(int arg_vid);
    void ChangeDataType(int arg_vid);
    void ChangeConvertionType(int arg_vid);

public slots:
    void blackboardVariableConfigChanged(int vid, unsigned int flags);

    void change_connection_to_observer(bool Connect);

private:
    bool m_ConnectedToObserver;
    QList <VariableElemente> m_Elements;
    BlackboardObserverConnection m_ObserverConnection;
    QIcon m_GhostIcon;
};

#endif // BLACKBOARDVARIABLEMODEL_H

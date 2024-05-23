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


#ifndef CALLIBRATIONTREEMODEL_H
#define CALLIBRATIONTREEMODEL_H

#include <QAbstractItemModel>
#include <QVariant>
#include "CalibrationTreeItem.h"

extern "C" {
    #include "DebugInfoDB.h"
    #include "Wildcards.h"
    #include "SectionFilter.h"  // wegen SECTION_ADDR_RANGES_FILTER
}


class CalibrationTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    CalibrationTreeModel (int par_Pid, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_DebugInfos,
                           int par_ConstOnly,
                           int par_ShowValues,
                           int par_HexOrDec,
                           QObject *par_Parent = nullptr);
    ~CalibrationTreeModel() Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &par_Index, int par_Role) const Q_DECL_OVERRIDE;
    QVariant headerData(int par_Section, Qt::Orientation par_Orientation,
                        int par_Role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int par_Row, int par_Column,
                      const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &par_Index) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &par_Index, const QVariant &par_Value,
                 int par_Role = Qt::EditRole) Q_DECL_OVERRIDE;
    bool setHeaderData(int par_Section, Qt::Orientation par_Orientation,
                       const QVariant &par_Value, int par_Role = Qt::EditRole) Q_DECL_OVERRIDE;

    void Reset (int par_Pid, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_DebugInfos);
    void InvalidCachedValues (void);

    int GetPid (void);

    void SetShowValues (bool par_ShowValues);

    int GetHexOrDecViewFlag();
    void SetHexOrDecViewFlag (int par_HexOrDecViewFlag);

    void AddDelayedRead(CalibrationTreeItem *par_Item, const QModelIndex &par_Index) const;

    void SetAddressInvalid(const QModelIndex &par_Index);

    void SetIncludeExcludeFilter(INCLUDE_EXCLUDE_FILTER *par_IncExcFilter, SECTION_ADDR_RANGES_FILTER *par_GlobalSectionFilter) { m_IncExcFilter = par_IncExcFilter; m_GlobalSectionFilter = par_GlobalSectionFilter; }

    INCLUDE_EXCLUDE_FILTER *GetIncludeExcludeFilter() { return m_IncExcFilter; }
    SECTION_ADDR_RANGES_FILTER *GetGlobalSectionFilter() { return m_GlobalSectionFilter; }

public slots:
    void ExecuteDelayedReads();

private:   
    
    CalibrationTreeItem *GetItem(const QModelIndex &index) const;
    
    int m_Pid;
    DEBUG_INFOS_ASSOCIATED_CONNECTION *m_DebugInfos;
    
    //char *m_Filter;
    int m_ConstOnly;
    int m_ShowValues;
    int m_HexOrDec;
    INCLUDE_EXCLUDE_FILTER *m_IncExcFilter;
    SECTION_ADDR_RANGES_FILTER *m_GlobalSectionFilter;

    CalibrationTreeItem *m_RootItem;

    class DelayedReadListElem {
    public:
        QModelIndex ModelIndex;
        CalibrationTreeItem *Item;
        bool Changed;
        inline bool operator==(const DelayedReadListElem &o) const { return Item == o.Item; }
    };

    QList<DelayedReadListElem> m_DelayedReadItems;
    bool m_ExecuteDelayedReadsRunning;

};


#endif // CALLIBRATIONTREEMODEL_H

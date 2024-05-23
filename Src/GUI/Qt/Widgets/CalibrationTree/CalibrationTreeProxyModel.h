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


#ifndef CALLIBRATIONTREEPROXYMODEL_H
#define CALLIBRATIONTREEPROXYMODEL_H

#include <QSortFilterProxyModel>


extern "C" {
    #include "DebugInfoDB.h"
    #include "Wildcards.h"
    #include "SectionFilter.h"  // wegen SECTION_ADDR_RANGES_FILTER
}



class CalibrationTreeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    CalibrationTreeProxyModel(INCLUDE_EXCLUDE_FILTER *par_IncExcFilter, SECTION_ADDR_RANGES_FILTER *par_GlobalSectionFilter, QObject *parent = nullptr);
    ~CalibrationTreeProxyModel() Q_DECL_OVERRIDE;

    void ChangeFilter (INCLUDE_EXCLUDE_FILTER *par_IncExcFilter, SECTION_ADDR_RANGES_FILTER *par_GlobalSectionFilter);

protected:
    bool filterAcceptsRow (int sourceRow, const QModelIndex &sourceParent) const Q_DECL_OVERRIDE;
    bool lessThan (const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;
    int CompareNames (const QString& s1, const QString& s2) const;
private:
    INCLUDE_EXCLUDE_FILTER *m_IncExcFilter;
    SECTION_ADDR_RANGES_FILTER *m_GlobalSectionFilter;
};

#endif // CALLIBRATIONTREEPROXYMODEL_H

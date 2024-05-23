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


#include "CalibrationTreeProxyModel.h"
#include "StringHelpers.h"

extern "C" {
#include "ThrowError.h"
}

CalibrationTreeProxyModel::CalibrationTreeProxyModel(INCLUDE_EXCLUDE_FILTER *par_IncExcFilter, SECTION_ADDR_RANGES_FILTER *par_GlobalSectionFilter, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_IncExcFilter = par_IncExcFilter;
    m_GlobalSectionFilter = par_GlobalSectionFilter;
}

CalibrationTreeProxyModel::~CalibrationTreeProxyModel()
{
}

void CalibrationTreeProxyModel::ChangeFilter (INCLUDE_EXCLUDE_FILTER *par_IncExcFilter, SECTION_ADDR_RANGES_FILTER *par_GlobalSectionFilter)
{
    m_IncExcFilter = par_IncExcFilter;
    m_GlobalSectionFilter = par_GlobalSectionFilter;
}

bool CalibrationTreeProxyModel::filterAcceptsRow (int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_IncExcFilter == nullptr) return true;  // No filter
    if (sourceParent.isValid()) return true;
    QModelIndex Index = sourceModel()->index(sourceRow, 0, sourceParent);
    int Column = Index.column();
    if (Column == 0) {
        // Only label filter no structure elemente (if label there are no parent)
        QModelIndex IndexParent = Index.parent();
        if (!IndexParent.isValid()) {
            QVariant Variant = sourceModel()->data (Index, Qt::UserRole + 2); // Addresse
            bool ValidAddr = false;
            uint64_t Address = 0;
            if (Variant.isValid()) {
                Address = Variant.toUInt(&ValidAddr);
            }
            Variant = sourceModel()->data (Index, Qt::DisplayRole);
            if (Variant.isValid()) {
                QString Name = Variant.toString();
                if (!ValidAddr || CheckAddressRangeFilter (Address, m_GlobalSectionFilter)) {
                    if (CheckIncludeExcludeFilter (QStringToConstChar(Name), m_IncExcFilter)) {
                        return true;
                    }
                }
            }
        } else {
            return true;
        }
    }
    return false;
}


int CalibrationTreeProxyModel::CompareNames (const QString& s1, const QString& s2) const
{
    QString S1Upper = s1.toUpper();
    QString S2Upper = s2.toUpper();

    return S1Upper < S2Upper;
}



bool CalibrationTreeProxyModel::lessThan (const QModelIndex &left, const QModelIndex &right) const
{
    QModelIndex IndexParent = left.parent();
    if (!IndexParent.isValid()) {  // Parent is invalid only if it is a label
        QVariant leftData = sourceModel()->data(left);
        QVariant rightData = sourceModel()->data(right);

        if (leftData.isValid() && rightData.isValid() &&
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            (leftData.typeId() == QVariant::String) &&
            (rightData.typeId() == QVariant::String)) {
#else
            (leftData.type() == QVariant::String) &&
            (rightData.type() == QVariant::String)) {
#endif
            QString leftString = leftData.toString().toUpper();
            QString rightString = rightData.toString().toUpper();

            return rightString < leftString;
        }
    }
    return false;  // Only labels will be sorted
}

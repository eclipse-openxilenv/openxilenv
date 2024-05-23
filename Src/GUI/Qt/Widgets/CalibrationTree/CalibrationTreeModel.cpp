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


#include "CalibrationTreeModel.h"

extern "C" {
#include "Blackboard.h"
#include "Scheduler.h"
#include "ThrowError.h"
}
#include<QDebug>

CalibrationTreeModel::CalibrationTreeModel (int par_Pid,
                                            DEBUG_INFOS_ASSOCIATED_CONNECTION *par_DebugInfos,
                                            int par_ConstOnly,
                                            int par_ShowValues,
                                            int par_HexOrDec,
                                            QObject *par_Parent)
    : QAbstractItemModel(par_Parent)
{
    m_IncExcFilter = nullptr;
    m_GlobalSectionFilter = nullptr;
    m_Pid = par_Pid;
    m_DebugInfos = par_DebugInfos;
    m_ConstOnly = par_ConstOnly;
    m_ShowValues = par_ShowValues;
    m_HexOrDec = par_HexOrDec;

    m_ExecuteDelayedReadsRunning = false;

    m_RootItem = new CalibrationTreeItem (this, m_DebugInfos, -1, 0, false, 0, true, false, QString ("label"), QString ("type"));
}

CalibrationTreeModel::~CalibrationTreeModel()
{

    delete m_RootItem;
}

void CalibrationTreeModel::SetShowValues (bool par_ShowValues)
{
    m_ShowValues = par_ShowValues;
}

int CalibrationTreeModel::columnCount (const QModelIndex & par_Parent) const
{
    Q_UNUSED(par_Parent)
    return 4;  // always 4
}

QVariant CalibrationTreeModel::data (const QModelIndex &par_Index, int par_Role) const
{
    if (!par_Index.isValid())
        return QVariant();

    CalibrationTreeItem *Item = GetItem (par_Index);

    switch (par_Role) {
    case Qt::UserRole + 1:  // TypeNr
        {
            int TypeNr = Item->GetTypeNr();
            return QVariant (TypeNr);
        }
        //break;
    case Qt::UserRole + 2:  // Address
        {
            uint64_t Address = Item->GetAddress();
            return QVariant (static_cast<long long>(Address));
        }
        //break;
    case Qt::UserRole + 3:  // reset the cached flag
        Item->ClearCache();
        return QVariant ();
        //break;
    case Qt::UserRole + 8:  // trigger delayed read
        if (Item->IsValidBaseElement()) {
            AddDelayedRead(Item, par_Index);
        }
        return QVariant ();
        //break;
    case Qt::UserRole + 4:  // Complete label
        return QVariant (Item->GetLabelCompleteName ());
        //break;
    case Qt::UserRole + 5:  // Display typet: 0 -> Integer decimal with sign
                            //                1 -> Integer hexadecimal with sign
                            //                2 -> Integer decimal without sign
                            //                3 -> Integer hexadezimal without sign
                            //                4 -> Floatingpoint
        {
            int TypeNr = Item->GetTypeNr();
            switch (get_base_type_bb_type_ex (TypeNr, m_DebugInfos)) {
            case BB_BYTE:
            case BB_WORD:
            case BB_DWORD:
            case BB_QWORD:
                return (m_HexOrDec) ? 1 : 0;
                //break;
            case BB_UBYTE:
            case BB_UWORD:
            case BB_UDWORD:
            case BB_UQWORD:
                return (m_HexOrDec) ? 3 : 2;
                //break;
            case BB_FLOAT:
            case BB_DOUBLE:
                return 4;
                //break;
            default:
                return -1;
                //break;
            }
        }
        //break;
    case Qt::UserRole + 6:  // give back value as double
        return Item->GetValueAsDouble();
        //break;
    case Qt::UserRole + 7:  // BlackboardTypeNr or -1 if it is not a base type
        return get_base_type_bb_type_ex (Item->GetTypeNr(), m_DebugInfos);
        //break;
    case Qt::DisplayRole:
    case Qt::EditRole:
        // column:
        // 0 -> Name
        // 1 -> Datentyp
        // 2 -> Value
        switch (par_Index.column()) {
        case 0:
            return Item->GetLabelName ();
            //break;
        case 1:
            {
                bool DelayedReadFlag;
                QVariant Ret = Item->GetLabelValueString (&DelayedReadFlag);
                if (DelayedReadFlag) {
                    AddDelayedRead(Item, par_Index);
                }
                return Ret;
            }
            //break;
        case 2:
            return Item->GetLabelAddressString ();
            //break;
        case 3:
            return Item->GetLabelDataType ();
            //break;
        }
        break;
    case Qt::TextAlignmentRole:
        if ((par_Index.column() == 1) ||    // Value column
            (par_Index.column() == 2)) {    // Address column
            return Qt::AlignRight;
        } else {
            return Qt::AlignLeft;
        }
        //break;
    case Qt::DecorationRole:   // Icons
        if (par_Index.column() == 0) {
            return Item->GetIcon();
        }
        return QVariant();
    default:
        return QVariant();
        //break;
    }

    return QVariant();
}

Qt::ItemFlags CalibrationTreeModel::flags (const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    if (index.column() == 1) {  // Value column is editable
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    } else {
        return QAbstractItemModel::flags(index);
    }
}

CalibrationTreeItem *CalibrationTreeModel::GetItem (const QModelIndex &index) const
{
    if (index.isValid()) {
        CalibrationTreeItem *item = static_cast<CalibrationTreeItem*>(index.internalPointer());
        if (item) {
            return item;
        }
    }
    return m_RootItem;
}

QVariant CalibrationTreeModel::headerData (int par_Section, Qt::Orientation par_Orientation,
                               int par_Role) const
{
    if (par_Orientation == Qt::Horizontal && par_Role == Qt::DisplayRole) {
        switch (par_Section) {
        case 0:
            return QString ("label");
        case 1:
            return QString ("value");
        case 2:
            return QString ("address");
        case 3:
            return QString ("type");
        }
    }
    return QVariant();
}

QModelIndex CalibrationTreeModel::index (int par_Row, int par_Column, const QModelIndex &par_Parent) const
{
    if (par_Parent.isValid() && par_Parent.column() != 0) {
        return QModelIndex();
    }
    CalibrationTreeItem *parentItem = GetItem(par_Parent);

    CalibrationTreeItem *childItem = parentItem->child(par_Row);
    if (childItem) {
        return createIndex (par_Row, par_Column, childItem);
    } else {
        return QModelIndex();
    }
}

QModelIndex CalibrationTreeModel::parent (const QModelIndex &par_Index) const
{
    if (!par_Index.isValid()) {
        return QModelIndex();
    }
    CalibrationTreeItem *childItem = GetItem(par_Index);
    CalibrationTreeItem *parentItem = childItem->parent();

    if (parentItem == m_RootItem) {
        return QModelIndex();
    }
    return createIndex (parentItem->childNumber(), 0, parentItem);
}

int CalibrationTreeModel::rowCount(const QModelIndex &par_Parent) const
{
    if (m_Pid <= 0) {
        return 0;  // if process not running no expanding
    }
    CalibrationTreeItem *parentItem = GetItem (par_Parent);

    return parentItem->childCount();
}

bool CalibrationTreeModel::setData(const QModelIndex &par_Index, const QVariant &par_Value, int par_Role)
{
    if (par_Role != Qt::EditRole) {
        return false;
    }
    CalibrationTreeItem *item = GetItem (par_Index);
    bool result = item->setData (par_Index.column(), par_Value);

    if (result) {
        emit dataChanged (par_Index, par_Index);
    }
    return result;
}

bool CalibrationTreeModel::setHeaderData (int par_Section, Qt::Orientation par_Orientation,
                                           const QVariant &par_Value, int par_Role)
{
    if (par_Role != Qt::EditRole || par_Orientation != Qt::Horizontal) {
        return false;
    }
    bool result = m_RootItem->setData (par_Section, par_Value);

    if (result) {
        emit headerDataChanged (par_Orientation, par_Section, par_Section);
    }
    return result;
}

void CalibrationTreeModel::InvalidCachedValues (void)
{
    m_RootItem->InvalidCachedValues();
}

void CalibrationTreeModel::Reset (int par_Pid, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_DebugInfos)
{
    beginResetModel();
    delete m_RootItem;
    m_Pid = par_Pid;
    m_DebugInfos = par_DebugInfos;
    m_RootItem = new CalibrationTreeItem (this, m_DebugInfos, -1, 0, false, 0, true, false, QString ("label"), QString ("type"));
    endResetModel();

    m_DelayedReadItems.clear();
}

int CalibrationTreeModel::GetPid (void)
{
    return m_Pid;
}

int CalibrationTreeModel::GetHexOrDecViewFlag()
{
    return m_HexOrDec;
}

void CalibrationTreeModel::SetHexOrDecViewFlag (int par_HexOrDecViewFlag)
{
    m_HexOrDec = par_HexOrDecViewFlag;
}

void CalibrationTreeModel::AddDelayedRead(CalibrationTreeItem *par_Item, const QModelIndex &par_Index) const
{
    DelayedReadListElem Elem;
    Elem.ModelIndex = par_Index;
    Elem.Item = par_Item;
    if (!m_DelayedReadItems.contains (Elem)) {
        (const_cast<CalibrationTreeModel*>(this))->m_DelayedReadItems.append(Elem);
    }
}

void CalibrationTreeModel::SetAddressInvalid(const QModelIndex &par_Index)
{
    CalibrationTreeItem *item = GetItem (par_Index);
    item->SetAddressInvalid();
}

void CalibrationTreeModel::ExecuteDelayedReads()
{
    if (!m_ExecuteDelayedReadsRunning) {
        m_ExecuteDelayedReadsRunning = true;
        if (!m_DelayedReadItems.empty()) {
            // Short wait (10ms) and no error message
            if (WaitUntilProcessIsNotActiveAndThanLockIt (m_Pid, 10, ERROR_BEHAVIOR_NO_ERROR_MESSAGE, "", __FILE__, __LINE__) == 0) {
                bool ErrMsgFlag = false;
                int Index = 0;
                for(Index = 0; Index < m_DelayedReadItems.size(); Index++) {
                    DelayedReadListElem Elem =m_DelayedReadItems.at(Index);
                    switch (Elem.Item->DelayedGetLabelValueString(ErrMsgFlag)) {
                    case 0:    // OK
                        Elem.Changed = true;
                        break;
                    case -1:   // only this element will be ignored
                        Elem.Changed = false;
                        SetAddressInvalid(Elem.ModelIndex);
                        break;
                    case -2:   // ignore all elements
                    default:
                        Elem.Changed = false;
                        SetAddressInvalid(Elem.ModelIndex);
                        ErrMsgFlag = false;   // no more error messages
                        break;
                    }
                    m_DelayedReadItems.replace(Index, Elem);
                }
                UnLockProcess (m_Pid);
                foreach (DelayedReadListElem Elem, m_DelayedReadItems) {
                    if (Elem.Changed) {
                        emit this->dataChanged (Elem.ModelIndex, Elem.ModelIndex);
                    }
                }
                m_DelayedReadItems.clear();
            }
        }
        m_ExecuteDelayedReadsRunning = false;
    } else {
        ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
    }
}


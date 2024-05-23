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


#ifndef CALLIBRATIONTREEITEM_H
#define CALLIBRATIONTREEITEM_H

#include <stdint.h>

#include <QList>
#include <QVariant>
#include <QIcon>

extern "C" {
#include "DebugInfos.h"
#include "SharedDataTypes.h"
}

class CalibrationTreeModel;

class CalibrationTreeItem
{
public:
    explicit CalibrationTreeItem (CalibrationTreeModel *par_Model, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_DebugInfos,
                                   int32_t par_What, int32_t par_TypeNr,
                                   bool par_IsBasisClass, uint64_t par_Address,  bool par_AddressIsResolved, bool par_AddressIsInvalid,
                                   QString par_Label, QString par_DataType, CalibrationTreeItem *par_Parent = nullptr);
    ~CalibrationTreeItem();

    QVariant GetLabelName () const;
    QVariant GetLabelDataType () const;
    QVariant GetLabelValueString (bool *ret_DelayedReadFlag);
    QVariant GetLabelAddressString ();
    int DelayedGetLabelValueString (bool ErrMsgFlag);

    uint64_t GetAddress();
    int32_t GetTypeNr();
    void ClearCache ();
    QString GetLabelCompleteName ();

    CalibrationTreeItem *child(int number);
    int childCount();
    int columnCount() const;
    QVariant data(int column);
    CalibrationTreeItem *parent();
    int childNumber() const;
    bool setData(int column, const QVariant &value);

    void InvalidCachedValues (void);

    double GetValueAsDouble();

    uint64_t ResolveAddress(bool *ret_Ok);

    bool IsValidBaseElement();

    QVariant &GetIcon();

    void SetAddressInvalid();

private:
    QString GetLabelCompleteNameWithSeparator ();

    void ExpandChilds ();
    void ExpandPointer ();
    void ExpandStructure ();
    void ExpandArray ();
    void ExpandRoot ();

    CalibrationTreeModel *m_Model;
    int m_What;
    int32_t m_TypeNr;
    uint64_t m_Address;
    bool m_AddressIsResolved;
    bool m_AddressIsInvalid;
    uint64_t m_PointsToAddress;
    bool m_PointsToAddressIsResolved;
    bool m_IsBasisClass;
    union BB_VARI m_ValueCache;
    enum BB_DATA_TYPES m_TypeCache;
    bool m_ValueCachedFlag;

    DEBUG_INFOS_ASSOCIATED_CONNECTION *m_DebugInfos;

    QString m_Label;
    QString m_DataType;

    QList<CalibrationTreeItem*> m_ChildItems;
    bool m_IsChildsExpanded;
    CalibrationTreeItem *m_ParentItem;

    static int sm_ObjectCounter;
    static QVariant sm_BasisDataTypeIcon;
    static QVariant sm_StructTypeIcon;
    static QVariant sm_ArrayTypeIcon;
    static QVariant sm_PointerTypeIcon;
    static QVariant sm_BaseClassTypeIcon;
    static QVariant sm_ErrTypeIcon;
};

#endif // CALLIBRATIONTREEITEM_H

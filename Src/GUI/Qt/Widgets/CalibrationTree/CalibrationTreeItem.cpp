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


#include "CalibrationTreeItem.h"
#include "CalibrationTreeModel.h"

#include "StringHelpers.h"

#include <QStringList>
#include <QPixmap>
#include <QIcon>

#include "inttypes.h"

extern "C" {
#include "MyMemory.h"
#include "Blackboard.h"
#include "ReadWriteValue.h"
#include "ThrowError.h"
#include "Scheduler.h"
#include "DebugInfoDB.h"
}

int CalibrationTreeItem::sm_ObjectCounter;
QVariant CalibrationTreeItem::sm_BasisDataTypeIcon;
QVariant CalibrationTreeItem::sm_StructTypeIcon;
QVariant CalibrationTreeItem::sm_ArrayTypeIcon;
QVariant CalibrationTreeItem::sm_PointerTypeIcon;
QVariant CalibrationTreeItem::sm_BaseClassTypeIcon;
QVariant CalibrationTreeItem::sm_ErrTypeIcon;

CalibrationTreeItem::CalibrationTreeItem (CalibrationTreeModel *par_Model, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_DebugInfos,
                                          int par_What, int32_t par_TypeNr, bool par_IsBasisClass,
                                          uint64_t par_Address, bool par_AddressIsResolved, bool par_AddressIsInvalid,
                                          QString par_Label, QString par_DataType, CalibrationTreeItem *par_Parent)
{
    m_ValueCachedFlag = false;
    m_Model = par_Model;
    m_DebugInfos = par_DebugInfos;
    m_What = par_What;
    m_TypeNr = par_TypeNr;
    m_IsBasisClass = par_IsBasisClass;
    m_Address = par_Address;
    m_AddressIsResolved = par_AddressIsResolved;
    m_AddressIsInvalid = par_AddressIsInvalid;
    m_PointsToAddressIsResolved = false;
    m_PointsToAddress = 0x0;
    m_Label = par_Label;
    m_DataType = par_DataType;
    m_ParentItem = par_Parent;
    m_IsChildsExpanded = false;

    sm_ObjectCounter++;
}

CalibrationTreeItem::~CalibrationTreeItem()
{
    qDeleteAll(m_ChildItems);
    sm_ObjectCounter--;
}


void CalibrationTreeItem::ExpandPointer ()
{
    const char *name;
    char TypeBuffer[1024];
    int32_t points_to_typenr;
    int points_to_what, expand;
    char struct_structname[2*BBVARI_NAME_SIZE];
    int selected_image;

    int Pid = m_Model->GetPid();

    m_PointsToAddress = 0;
    m_PointsToAddressIsResolved = ReadBytesFromExternProcessStopScheduler (Pid, m_Address, sizeof (m_PointsToAddress), &m_PointsToAddress, 0, false) == sizeof (m_PointsToAddress);

    if (get_points_to (&points_to_typenr, &points_to_what, m_TypeNr, m_DebugInfos)) {

        if (get_pointer_type_string (m_TypeNr, 1, TypeBuffer, sizeof (TypeBuffer), m_DebugInfos) > 0) {
            name = TypeBuffer;
        } else {
            name = "";
        }

        sprintf (struct_structname, "[0]");
        expand = 0;
        switch (points_to_what) {
        case 1:   // Base data type
            if (name[0] == '*') {
                if (strcmp (&name[1], "void")) {
                    expand = 4;
                }
                selected_image = 4;
            } else {
                expand = 1;
                selected_image = 1;
            }
            break;
        case 2:   // Structure
            expand = 2;
            selected_image = 2;
            break;
        case 3:   // Array
            expand = 3;
            selected_image = 3;
            break;
        case 4:  // Pointer
            expand = 4;
            selected_image = 4;
            break;
        default:
            selected_image = 0;
            break;
        }

        if ((expand == 4) || (expand == 1)) {

            // Add the item to the tree
            QString StructName = QString (struct_structname);
            QString StructType = QString (name);
            CalibrationTreeItem *NewChild = new CalibrationTreeItem (m_Model, m_DebugInfos, points_to_what, points_to_typenr, 0,
                                                                       m_PointsToAddress, m_PointsToAddressIsResolved, false,
                                                                       StructName, StructType, this);
            if (NewChild == nullptr) {
                return;
            }
            m_ChildItems.append (NewChild);
        } else {
            if (points_to_typenr != -1) {
                int32_t  m_TypeNrSave = m_TypeNr;
                uint64_t  m_AddressSave = m_Address;
                bool m_AddressIsResolvedSave = m_AddressIsResolved;
                m_TypeNr = points_to_typenr;
                m_Address = m_PointsToAddress; //Address;
                m_AddressIsResolved = m_PointsToAddressIsResolved;
                if (expand == 2) {
                    ExpandStructure();
                } if (expand == 3) {  // Pointer
                    ExpandArray();
                } if (expand == 4) {  // Pointer
                    ;
                }
                m_TypeNr = m_TypeNrSave;
                m_Address = m_AddressSave;
                m_AddressIsResolved = m_AddressIsResolvedSave;
            }
        }
    } /* end while */
}

void CalibrationTreeItem::ExpandStructure ()
{
    const char *Name, *TypeName;
   int32_t typenr, pos;
    int what, expand;
    int flag = 1;
    int selected_image;
    uint64_t address;
    uint32_t address_offset;
    int bclass;

    while ((Name = get_next_struct_entry (&typenr, &address_offset, &bclass, m_TypeNr, flag, &pos, m_DebugInfos)) != nullptr) {
        flag = 0;
        address = m_Address + address_offset;

        if ((TypeName = get_struct_entry_typestr (&typenr, pos, &what, m_DebugInfos)) == nullptr) {
            TypeName = "unknown)";
            what = 5;
        }
        expand = 0;
        switch (what) {
        case 1:   // Base data type
            if ((TypeName != nullptr ) && (TypeName[0] == '*')) {
                if (strcmp (&TypeName[1], "void")) {
                    expand = 4;
                }
            } else {
                selected_image = 1;
            }
            break;
        case 2:   // Structure
            expand = 2;
            if (bclass) selected_image = 5;
            else selected_image = 2;
            break;
        case 3:   // Array
            expand = 3;
            selected_image = 3;
            break;
        case 4:  // Pointer
            expand = 4;
            selected_image = 4;
            break;
        default:
            selected_image = 0;
        }

        // Add the item to the tree
        QString StructNameString = QString (Name);
        QString StructTypeString = QString (TypeName);
        CalibrationTreeItem *NewChild = new CalibrationTreeItem (m_Model, m_DebugInfos, what, typenr, bclass,
                                                                   address, m_AddressIsResolved, false, StructNameString, StructTypeString, this);
        if (NewChild == nullptr) {
            return;
        }
        m_ChildItems.append (NewChild);

    } /* end while */

}


void CalibrationTreeItem::ExpandArray ()
{
    const char *TypeName;
    char TypeBuffer[1024];
   int32_t arrayelem_typenr;
    int arrayelem_of_what, expand;
    char arrayelemname [BBVARI_NAME_SIZE];
    int selected_image;
    int32_t array_size, array_elements, x;
    uint64_t address;

    if (get_array_elem_type_and_size (&arrayelem_typenr, &arrayelem_of_what,
                                      &array_size, &array_elements, m_TypeNr, m_DebugInfos)) {

        if (get_array_type_string (m_TypeNr, 1, TypeBuffer, sizeof (TypeBuffer), m_DebugInfos) > 0) {
            TypeName = TypeBuffer;
        } else {
            TypeName = "";
        }

        for (x = 0; x < array_elements; x++) {
            address = m_Address + static_cast<uint64_t>(x * (array_size / array_elements));

            sprintf (arrayelemname, "[%i]", x);

            expand = 0;
            switch (arrayelem_of_what) {
            case 1:   // Base data type
                if ((TypeName != nullptr) && (TypeName[0] == '*')) {
                    if (strcmp (&TypeName[1], "void")) {
                        expand = 4;
                    }
                    selected_image = 4;
                } else {
                    selected_image = 1;
                }
                break;
            case 2:   // Structure
                expand = 2;
                selected_image = 2;
                break;
            case 3:   // Array
                expand = 3;
                selected_image = 3;
                break;
            case 4:  // Pointer
                expand = 4;
                selected_image = 4;
                break;
            default:
                selected_image = 0;
            }

            // Add the item to the tree
            QString StructName = QString (arrayelemname);
            QString StructType = QString (TypeName);
            CalibrationTreeItem *NewChild = new CalibrationTreeItem (m_Model, m_DebugInfos, arrayelem_of_what, arrayelem_typenr, false,
                                                                       address, m_AddressIsResolved, false, StructName, StructType, this);
            if (NewChild == nullptr) {
                return;
            }
            m_ChildItems.append (NewChild);
        }
    }
}

void CalibrationTreeItem::ExpandRoot ()
{
    char *Label;
    char *Type;
    int selected_image;
    int32_t typenr;
    int expand;
    uint64_t address;
    int what;
    int32_t idx;
    int32_t idx_sort;
    SECTION_ADDR_RANGES_FILTER *GlobalSectionFilter = m_Model->GetGlobalSectionFilter();
    INCLUDE_EXCLUDE_FILTER *IncludeEcludeFilter = m_Model->GetIncludeExcludeFilter();
    //int cycle =0;
    //FILE *dbg_fh;
    //static int count;

    if (m_DebugInfos == nullptr) return;

    //dbg_fh = open_file ("c:\\tmp\\tv_label_list.txt", "wt");
#if 0
    // nur neu fllen falls ein Prozess fr diese Debuginfos existiert
    if (m_DebugInfos != nullptr) {
        if (!m_DebugInfos->associated_exe_file) goto __OUT;
        // Globaler Section-Filter laden
        GlobalSectionFilter = BuildGlobalAddrRangeFilter (m_DebugInfos);
    }
#endif
    //idx = 0L;
    idx_sort = 0;
    while ((idx = get_next_sorted_label_index(&idx_sort, m_DebugInfos)) >= 0) {
        Label = get_next_label_ex (&idx, &address, &typenr, m_DebugInfos);
        if (Label == nullptr) {
            ThrowError (1, "Internal error %s(%i)", __FILE__, __LINE__);
        }
    //while ((Label = get_next_label_ex (&idx, &address, &typenr, m_DebugInfos)) != nullptr) {
        if (!CheckAddressRangeFilter (address, GlobalSectionFilter)) continue;

        // Include-Exclude-Label-Filter
        if (IncludeEcludeFilter != nullptr)
            if (!CheckIncludeExcludeFilter (Label, IncludeEcludeFilter)) continue;

        /*if (!strcmp("XYZ::ABC::im_namespace", Label)) {
            printf ("stop");
        }*/

        Type = get_typestr_by_typenr (&typenr, &what, m_DebugInfos);
        if (Type == nullptr) {
            continue;
        }

        expand = 0;
        switch (what) {
        case 1:   // Base data type
            if ((Type != nullptr) && (Type[0] == '*')) {
                if (strcmp (&Type[1], "void")) {
                    expand = 4;
                }
                selected_image = 4;
            } else {
                selected_image = 1;
            }
            break;
        case 2:   // Structure
            expand = 2;
            selected_image = 2;
            break;
        case 3:  // Array
            expand = 3;
            selected_image = 3;
            break;
        case 4:  // Pointer
            expand = 4;
            selected_image = 4;
            break;
        default:    // Unknown
            selected_image = 0;
        }

        // Add the item to the tree
        QString StructName = QString (Label);
        QString StructType = QString (Type);
        CalibrationTreeItem *NewChild = new CalibrationTreeItem (m_Model, m_DebugInfos, what, typenr, false,
                                                                   address, true, false,
                                                                   StructName, StructType, this);
        if (NewChild == nullptr) {
            return;
        }
        m_ChildItems.append (NewChild);

    } /* end while */
}


void CalibrationTreeItem::ExpandChilds ()
{
    if (m_IsChildsExpanded == false) {
        switch (m_What) {
        case -1:  // Root
            ExpandRoot ();
            break;
        case 0:   // Label
            break;
        case 1:   // Base data type
            if (m_TypeNr > 100) {
                ExpandPointer ();
            }
            break;
        case 2:   // Structure
            ExpandStructure ();
            break;
        case 3:   // Array
            ExpandArray ();
            break;
        case 4:  // Pointer
            ExpandPointer ();
            break;
        case MODIFIER_ELEM:
            //printf ("modifier");
            break;
        default:
            break;
        }
        m_IsChildsExpanded = true;
    }
}

QVariant CalibrationTreeItem::GetLabelName () const
{
    return m_Label;
}

QVariant CalibrationTreeItem::GetLabelDataType () const
{
    return m_DataType;
}

void CalibrationTreeItem::ClearCache ()
{
    m_ValueCachedFlag = false;
}


QVariant CalibrationTreeItem::GetLabelValueString (bool *ret_DelayedReadFlag)
{
    if ((m_What != 1) ||
        ((m_Address == 0) && m_AddressIsResolved) ||
        m_AddressIsInvalid ||
        (get_base_type_bb_type_ex (m_TypeNr, m_DebugInfos) < 0)) {
        if (ret_DelayedReadFlag != nullptr) {
            *ret_DelayedReadFlag = false;
        }
        if (m_What != 1) {
            return QString ("");
        } else {
            return QString ("invalid"); //QVariant ();  // Only base data type with an address have a valid value
        }
    }    
    if (!m_ValueCachedFlag) {
        if (ret_DelayedReadFlag != nullptr) {
            *ret_DelayedReadFlag = true;
            return QVariant ();
        } else {
            int Pid = m_Model->GetPid();
            if (Pid > 0) {
                union BB_VARI Value;
                int Type = BB_INVALID;
                Value.uqw = 0;
                bool Error = true;
                if (WaitUntilProcessIsNotActiveAndThanLockIt (Pid, 5000, ERROR_BEHAVIOR_NO_ERROR_MESSAGE, "", __FILE__, __LINE__) == 0) {
                    if (!m_AddressIsResolved) {
                        // The addresse could not be determined -> try it now
                        bool Ok;
                        uint64_t BaseAddress = m_ParentItem->ResolveAddress(&Ok);
                        if (Ok) {
                            m_Address = BaseAddress + m_Address;
                            Error = false;
                        } else {
                            Error = true;
                        }
                    }
                    if (Error == false) {
                        if (ReadValueFromExternProcessViaAddress (m_Address, m_DebugInfos, Pid, m_TypeNr, &Value, &Type, nullptr)) {
                            Error = true;
                        } else {
                            Error = false;
                        }
                    }
                    UnLockProcess (Pid);
                }

                if (Error) {
                    switch (ThrowError (ERROR_OK_CANCEL_CANCELALL, "cannot read value (extern process not running or halted in debugger)")) {
                    default:
                    case IDCANCEL:
                        break;
                    case IDCANCELALL:
                        break;
                    }
                    return QVariant ();
                } else {
                    m_ValueCachedFlag = true;
                    m_ValueCache = Value;
                    m_TypeCache = static_cast<enum BB_DATA_TYPES>(Type);
                }
            } else {
                return QVariant ();
            }
        }

    }
    if (ret_DelayedReadFlag != nullptr) {
        *ret_DelayedReadFlag = false;
    }
    enum BB_DATA_TYPES Type = static_cast<enum BB_DATA_TYPES>(get_base_type_bb_type_ex (m_TypeNr, m_DebugInfos));
    if (m_Model->GetHexOrDecViewFlag()) {
        char Help[256];
        bbvari_to_string(Type, m_ValueCache, 16, Help, sizeof(Help));
        QString ValueString (Help);
        return ValueString;
    } else {
        char Help[256];
        bbvari_to_string(Type, m_ValueCache, 10, Help, sizeof(Help));
        QString ValueString (Help);
        return ValueString;
    }
}

QVariant CalibrationTreeItem::GetLabelAddressString ()
{
    char Help[256];
    sprintf (Help, "0x%08" PRIX64, m_Address);
    QString ValueString (Help);
    return ValueString;
}


int CalibrationTreeItem::DelayedGetLabelValueString(bool ErrMsgFlag)
{
    int Pid = m_Model->GetPid();
    if (Pid > 0) {
        union BB_VARI Value;
        int Type;
        int Pid = m_Model->GetPid();
        if (!m_AddressIsResolved) {
            bool Ok;
            uint64_t BaseAddress = ResolveAddress(&Ok);
            if (Ok) {
                m_Address = BaseAddress + m_Address;
                m_AddressIsResolved = true;
            } else {
                return false;
            }
        }

        if (ReadValueFromExternProcessViaAddress (m_Address, m_DebugInfos, Pid,
                                                  m_TypeNr, &Value, &Type, nullptr) != 0) {
            if (ErrMsgFlag) {
                char Processname[MAX_PATH];
                QString CompleteName;
                if (get_name_by_pid(Pid,Processname) == 0) {
                    CompleteName = GetLabelCompleteName();
                } else {
                    Processname[0] = 0;
                }
                switch (ThrowError (ERROR_OK_CANCEL_CANCELALL, "cannot read value of label \"%s\" from external process \"%s\" (extern process not running or halted in debugger)",
                               QStringToConstChar(CompleteName), Processname)) {
                default:
                case IDCANCEL:
                    return -1;
                case IDCANCELALL:
                    return -2;
                }
            } else {
                return -1;
            }
        } else {
            m_ValueCachedFlag = true;
            m_ValueCache = Value;
            m_TypeCache = static_cast<enum BB_DATA_TYPES>(Type);
        }
    } else {
        return -2;  // Process unknown (like cancel all)
    }
    return 0;
}

uint64_t CalibrationTreeItem::GetAddress ()
{
    return m_Address;
}

int32_t CalibrationTreeItem::GetTypeNr ()
{
    return m_TypeNr;
}


QString CalibrationTreeItem::GetLabelCompleteName ()
{
    CalibrationTreeItem *Parent = parent();
    if (Parent != nullptr) {
        QString Name = Parent->GetLabelCompleteNameWithSeparator ();
        if (!m_Label.isNull()) {
            Name.append(m_Label);
        }
        return Name;
    } else {
        return m_Label;
    }
}

QString CalibrationTreeItem::GetLabelCompleteNameWithSeparator ()
{
    CalibrationTreeItem *Parent = parent();
    if (Parent != nullptr) {
        QString Name = Parent->GetLabelCompleteNameWithSeparator ();
        Name.append(m_Label);
        switch (m_What) {
        case 2:  // Structure
            Name.append(".");
            break;
        case 3:  // Array
            break;
        case 4: // Pointer
            break;
        }
        return Name;
    }
    return QString();
}


CalibrationTreeItem *CalibrationTreeItem::child(int number)
{
    if (m_IsChildsExpanded == false) {
        ExpandChilds ();
    }
    return m_ChildItems.value(number);
}

int CalibrationTreeItem::childCount()
{
    int Ret;
    if (m_IsChildsExpanded == false) {
        switch (m_What) {
        case 1:
            return 0; // Base data type have no childs!
        case 4:       // Don't expand pointer but count the elements
            int32_t points_to_typenr;
            int points_to_what;
            if (get_points_to (&points_to_typenr, &points_to_what,
                               m_TypeNr, m_DebugInfos)) {
                switch (points_to_what) {
                case 0:
                case 1:
                    return 1;
                case 2:  // Structure
                    return get_structure_element_count (points_to_typenr, m_DebugInfos);
                case 3: // Array
                    return get_array_element_count (points_to_typenr, m_DebugInfos);
                case 4:  // Pointer
                    return 1;
                }
            }
            break;
        case 3:
            return get_array_element_count (m_TypeNr, m_DebugInfos);
        case 2:
            return get_structure_element_count (m_TypeNr, m_DebugInfos);
        case MODIFIER_ELEM:
            return 1;
        default:
            ExpandChilds ();
        }
    }
    Ret = m_ChildItems.count();
    return Ret;
}

int CalibrationTreeItem::childNumber() const
{
    if (m_ParentItem)
        return m_ParentItem->m_ChildItems.indexOf(const_cast<CalibrationTreeItem*>(this));

    return 0;
}

int CalibrationTreeItem::columnCount() const
{
    return 3;
}

QVariant CalibrationTreeItem::data(int column)
{
    switch (column) {
    case 0:
        return GetLabelName ();
    case 1:  // Value
        return GetLabelValueString (nullptr);
    case 2:   // Address
        return GetLabelAddressString ();
    case 3:  // Type
        return GetLabelDataType ();
    default:
        return QVariant ();
    }
}


CalibrationTreeItem *CalibrationTreeItem::parent()
{
    return m_ParentItem;
}


bool CalibrationTreeItem::setData(int column, const QVariant &value)

{
    if (column == 1) {  // only values can be written
        bool Converted = false;
        QString ValueString = value.toString();
        union BB_VARI Value;
        Converted = (string_to_bbvari(m_TypeCache, &Value, QStringToConstChar(ValueString)) == 0);
        if (Converted) {
            int Pid = m_Model->GetPid();

            WriteValuesToExternProcessViaAddressStopScheduler (m_DebugInfos, Pid, 1,
                                                               &m_Address, &m_TypeNr,
                                                               &Value, 5000, true);
            m_ValueCachedFlag = false;  // Read value during next refreshing
            return true;
        }
    }
    return false;
}

void CalibrationTreeItem::InvalidCachedValues (void)
{
    m_ValueCachedFlag = false;
    for (int i = 0; i < m_ChildItems.size(); ++i) {
        m_ChildItems.at(i)->InvalidCachedValues ();
    }
}

double CalibrationTreeItem::GetValueAsDouble()
{
    double Ret;
    bbvari_to_double(m_TypeCache, m_ValueCache, &Ret);
    return Ret;
}

uint64_t CalibrationTreeItem::ResolveAddress(bool *ret_Ok)
{
    if (!m_AddressIsResolved) {
        uint64_t BaseAddress = m_ParentItem->ResolveAddress(ret_Ok);
        if (*ret_Ok) {
            m_Address = BaseAddress + m_Address;   // m_Adress is only the offset
        } else {
            return 0x0;
        }
    }
    switch (m_What) {
    case 4: // Pointer: return the  content of the pointer
        if (m_PointsToAddressIsResolved) {
            *ret_Ok = true;
            return m_PointsToAddress;
        } else {
            int Pid = m_Model->GetPid();
            do {
                if (scm_read_bytes (m_Address, Pid, static_cast<char*>(static_cast<void*>(&m_PointsToAddress)), sizeof (m_PointsToAddress)) == sizeof (m_PointsToAddress)) {
                    m_PointsToAddressIsResolved = true;
                } else {
                    m_PointsToAddressIsResolved = false;
               }
                if (!m_PointsToAddressIsResolved) {
                    switch (ThrowError (ERROR_OKCANCEL, "cannot read pointer value (extern process not running or halted in debugger). continue waiting (OK) or ignore (Cancel)")) {
                    default:
                    case IDCANCEL:
                        *ret_Ok = false;
                        return 0x0;
                    case IDOK:
                        break;
                    }
                }
            } while (!m_PointsToAddressIsResolved);
        }
        *ret_Ok = true;
        return m_PointsToAddress;
    default:
        *ret_Ok = true;
        return m_PointsToAddress;
    }
}

bool CalibrationTreeItem::IsValidBaseElement()
{
    if ((m_What != 1) ||
        ((m_Address == 0) && m_AddressIsResolved) ||
        (get_base_type_bb_type_ex (m_TypeNr, m_DebugInfos) < 0)) {
        return false;
    }
    return true;
}

QVariant &CalibrationTreeItem::GetIcon()
{
    switch (m_What) {
    case 1:  // Base data type
        if (sm_BasisDataTypeIcon.isNull()) {
            QString Name = QString (":/Icons/CalTreeDataTypeBase.png");
            sm_BasisDataTypeIcon = QIcon(Name);
        }
        return sm_BasisDataTypeIcon;
    case 2: // Structure
        if (m_IsBasisClass) {
            if (sm_BaseClassTypeIcon.isNull()) {
                QString Name = QString (":/Icons/CalTreeDataTypeBaseClass.png");
                sm_BaseClassTypeIcon = QIcon(Name);
            }
            return sm_BaseClassTypeIcon;
        } else {
            if (sm_StructTypeIcon.isNull()) {
                QString Name = QString (":/Icons/CalTreeDataTypeStruct.png");
                sm_StructTypeIcon = QIcon(Name);
            }
            return sm_StructTypeIcon;
        }
    case 3: // Array
        if (sm_ArrayTypeIcon.isNull()) {
            QString Name = QString (":/Icons/CalTreeDataTypeArray.png");
            sm_ArrayTypeIcon = QIcon(Name);
        }
        return sm_ArrayTypeIcon;
    case 4: // Structure
        if (sm_PointerTypeIcon.isNull()) {
            QString Name = QString (":/Icons/CalTreeDataTypePointer.png");
            sm_PointerTypeIcon = QIcon(Name);
        }
        return sm_PointerTypeIcon;
    }
    if (sm_ErrTypeIcon.isNull()) {
        QString Name = QString (":/Icons/CalTreeDataTypeErr.png");
        sm_ErrTypeIcon = QIcon(Name);
    }
    return sm_ErrTypeIcon;
}

void CalibrationTreeItem::SetAddressInvalid()
{
    m_AddressIsInvalid = true;
}

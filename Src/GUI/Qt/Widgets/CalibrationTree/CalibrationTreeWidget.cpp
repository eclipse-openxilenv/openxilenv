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

#include <stdint.h>
#include <QInputDialog>
#include <QHeaderView>

#include "CalibrationTreeWidget.h"
#include "ChangeValueTextDialog.h"

#include "ConfigCalibrationTreeViewDialog.h"
#include "ReferencedLabelsDialog.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

#include "CalibrationTreeAddReferenceDialog.h"

extern "C" {
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardConvertFromTo.h"
#include "ThrowError.h"
#include "ReadWriteValue.h"
#include "ExtProcessReferences.h"
#include "UniqueNumber.h"
#include "IniDataBase.h"
}


void CalibrationTreeWidgetSyncObject::NotifiyStartStopProcessFromSchedulerThread (int par_UniqueId, void *par_User, int par_Flags, int par_Action)
{
    emit NotifiyStartStopProcessFromSchedulerSignal (par_UniqueId, par_User, par_Flags, par_Action);
}

void CalibrationTreeWidgetSyncObject::AddCalibrationTreeWidget(CalibrationTreeWidget *par_CalibrationTreeWidget)
{
    m_ListOfAllCalibrationTreeWidget.append(par_CalibrationTreeWidget);
}

void CalibrationTreeWidgetSyncObject::RemoveCalibrationTreeWidget(CalibrationTreeWidget *par_CalibrationTreeWidget)
{
     m_ListOfAllCalibrationTreeWidget.removeAll(par_CalibrationTreeWidget);
}

void CalibrationTreeWidgetSyncObject::NotifiyStartStopProcessToWidgetThreadSlot (int par_UniqueId, void *par_User, int par_Flags, int par_Action)
{
    // Now we are inside the GUI thread
    int (*LoadUnloadCallBackFunc)(void *par_User, int par_Flags, int par_Action);
    void *User;

    // If ow the unique Id is valid and th widget still exists
    if (GetConnectionToProcessDebugInfos (par_UniqueId, &LoadUnloadCallBackFunc, &User, 1) == 0) {
        if (par_User != nullptr) {
            CalibrationTreeWidget *Widget;
            Widget = static_cast<CalibrationTreeWidget*>(par_User);
            if (this->m_ListOfAllCalibrationTreeWidget.contains(Widget)) {
                Widget->NotifiyStartStopProcess (par_Flags, par_Action);
            }
        }
    }
}


static CalibrationTreeWidgetSyncObject *SyncObject;


// Remork this function will be called from the scheduler thread
// And will be synchronized with signal/slot
extern "C" {
static int GlobalNotifiyCalibrationTreeWidgetFuncSchedThread (int par_UniqueId, void *par_User, int par_Flags, int par_Action)
{
    if (SyncObject != nullptr) {
        SyncObject->NotifiyStartStopProcessFromSchedulerThread (par_UniqueId, par_User, par_Flags, par_Action);
    }
    return 0;
}
}

CalibrationTreeWidget::CalibrationTreeWidget(QString par_WindowTitle, MdiSubWindow *par_SubWindow, MdiWindowType *par_Type,  QWidget *parent)
    : MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent)
{
    // If SyncObject don't exist build it one time inside the GUI thread
    if (SyncObject == nullptr) {
        SyncObject = new CalibrationTreeWidgetSyncObject();
        bool Ret = connect (SyncObject, SIGNAL(NotifiyStartStopProcessFromSchedulerSignal(int, void*, int, int)),
                            SyncObject, SLOT(NotifiyStartStopProcessToWidgetThreadSlot(int, void*, int, int)));
        if (!Ret) {
            ThrowError (1, "connect error");
        }

    }
    m_UniqueNumber = AquireUniqueNumber ();
    m_DebugInfos = nullptr;
    m_Pid = -1;
    m_IncExcFilter = nullptr;
    m_GlobalSectionFilter = nullptr;
    m_View = new CalibrationTreeView (this, this);
    m_View->setSortingEnabled(true);
    m_View->QAbstractScrollArea::setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    readFromIni();

    if (strlen (m_ProcessName)) {
            m_DebugInfos = ConnectToProcessDebugInfos (m_UniqueNumber,
                                                       m_ProcessName,
                                                       &m_Pid,
                                                       nullptr,
                                                       GlobalNotifiyCalibrationTreeWidgetFuncSchedThread,
                                                       static_cast<void*>(this),
                                                       0);
            m_GlobalSectionFilter = BuildGlobalAddrRangeFilter (m_DebugInfos);

    }

    m_Model = new CalibrationTreeModel (m_Pid,
                                         m_DebugInfos,
                                         m_ConstOnly,
                                         m_ShowValues,
                                         m_HexOrDec);
    m_Model->SetIncludeExcludeFilter(m_IncExcFilter, m_GlobalSectionFilter);
    m_View->setModel(m_Model);
    ShowHideColums();
    m_View->header()->setMinimumSectionSize(10);
    m_View->header()->setStretchLastSection(true);
    m_View->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_View->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_View->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_View->header()->setSectionResizeMode(3, QHeaderView::Interactive);

    m_FirstResize = true;
    SyncObject->AddCalibrationTreeWidget(this);
}

CalibrationTreeWidget::~CalibrationTreeWidget()
{
    SyncObject->RemoveCalibrationTreeWidget(this);
    delete (m_View);
    delete (m_Model);
    if (m_DebugInfos != nullptr) {
        RemoveConnectFromProcessDebugInfos (m_UniqueNumber);
    }
    FreeUniqueNumber (m_UniqueNumber);
    m_GlobalSectionFilter = DeleteAddrRangeFilter(m_GlobalSectionFilter);
    if (m_IncExcFilter != nullptr) FreeIncludeExcludeFilter (m_IncExcFilter);
}


void CalibrationTreeWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_View->resize (width(), height());
    if (m_FirstResize) {
        m_FirstResize = false;
        SetColumnWidth();
    }
}

void CalibrationTreeWidget::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()){
    case Qt::Key_F5:
        ReloadValuesSlot();
        break;
    default:
        MdiWindowWidget::keyPressEvent(event);
        break;
    }
}


void CalibrationTreeWidget::CyclicUpdate()
{
    // only if process is running
    if (m_Pid > 0) {
        m_Model->ExecuteDelayedReads();
    }
}


bool CalibrationTreeWidget::writeToIni()
{
    char txt[INI_MAX_LINE_LENGTH];
    QString SectionPath = GetIniSectionPath();
    int Fd = ScQt_GetMainFileDescriptor();

    ScQt_IniFileDataBaseWriteString (SectionPath, "type", "CalibrationTree", Fd);
    ScQt_IniFileDataBaseWriteString (SectionPath, "process", m_ProcessName, Fd);
    ScQt_IniFileDataBaseWriteString (SectionPath, "filter", m_Filter, Fd);
    sprintf (txt, "%i", m_ConstOnly);
    ScQt_IniFileDataBaseWriteString (SectionPath, "const only", txt, Fd);
    sprintf (txt, "%i", m_ShowValues);
    ScQt_IniFileDataBaseWriteString (SectionPath, "show values", txt, Fd);
    sprintf (txt, "%i", m_ShowAddress);
    ScQt_IniFileDataBaseWriteString (SectionPath, "show address", txt, Fd);
    sprintf (txt, "%i", m_ShowType);
    ScQt_IniFileDataBaseWriteString (SectionPath, "show type", txt, Fd);
    sprintf (txt, "%i", m_HexOrDec);
    ScQt_IniFileDataBaseWriteString(SectionPath, "hex or dec", txt, Fd);

    sprintf (txt, "%i", m_View->columnWidth(0));
    ScQt_IniFileDataBaseWriteString(SectionPath, "column width label", txt, Fd);
    sprintf (txt, "%i", m_View->columnWidth(1));
    ScQt_IniFileDataBaseWriteString(SectionPath, "column width value", txt, Fd);
    sprintf (txt, "%i", m_View->columnWidth(2));
    ScQt_IniFileDataBaseWriteString(SectionPath, "column width address", txt, Fd);
    sprintf (txt, "%i", m_View->columnWidth(3));
    ScQt_IniFileDataBaseWriteString(SectionPath, "column width type", txt, Fd);
    if (m_IncExcFilter != nullptr) {
        SaveIncludeExcludeListsToIni (m_IncExcFilter, QStringToConstChar(SectionPath), "", Fd);
    }

    return true;
}

bool CalibrationTreeWidget::readFromIni()
{
    QString SectionPath = GetIniSectionPath();
    int Fd = GetMainFileDescriptor();

    ScQt_IniFileDataBaseReadString (SectionPath, "process", "", m_ProcessName, sizeof (m_ProcessName), Fd);
    ScQt_IniFileDataBaseReadString (SectionPath, "filter", "*", m_Filter, sizeof (m_Filter), Fd);
    m_ConstOnly = ScQt_IniFileDataBaseReadInt (SectionPath, "const only", 0, Fd);

    m_ShowValues = ScQt_IniFileDataBaseReadInt (SectionPath, "show values", 1, Fd);
    m_ShowAddress = ScQt_IniFileDataBaseReadInt (SectionPath, "show address", 0, Fd);
    m_ShowType = ScQt_IniFileDataBaseReadInt (SectionPath, "show type", 0, Fd);
    m_HexOrDec = ScQt_IniFileDataBaseReadInt(SectionPath, "hex or dec", 0, Fd);

    m_ColumnWidthLabel = ScQt_IniFileDataBaseReadInt(SectionPath, "column width label", -1, Fd);
    m_ColumnWidthValue = ScQt_IniFileDataBaseReadInt(SectionPath, "column width value", -1, Fd);
    m_ColumnWidthAddress = ScQt_IniFileDataBaseReadInt(SectionPath, "column width address", -1, Fd);
    m_ColumnWidthType = ScQt_IniFileDataBaseReadInt(SectionPath, "column width type", -1, Fd);

    m_IncExcFilter = BuildIncludeExcludeFilterFromIni (QStringToConstChar(SectionPath), "", Fd);

    return true;
}

void CalibrationTreeWidget::OpenConfigDialog (void)
{
    char WindowName[1024];
    char ProcessName[MAX_PATH];
    QString WindowTitle = GetWindowTitle ();

    ConfigCalibrationTreeViewDialog Dlg (WindowTitle,
                                          m_ProcessName,
                                          m_IncExcFilter,
                                          m_ShowValues,
                                          m_ShowAddress,
                                          m_ShowType,
                                          m_HexOrDec);
    if (Dlg.exec() == QDialog::Accepted) {

        if (WindowTitle.compare (Dlg.GetWindowName())) {
            RenameWindowTo (Dlg.GetWindowName());
        }
        bool ProcessNameChanged;
        bool FilterChanged;
        bool ShowValuesChanged;
        bool ShowAddressChanged;
        bool ShowTypeChanged;
        int NewShowValues = Dlg.GetShowValue();
        int NewShowAddress = Dlg.GetShowAddress();
        int NewShowType = Dlg.GetShowDataType();
        ShowValuesChanged = (m_ShowValues != NewShowValues);
        ShowAddressChanged = (m_ShowAddress != NewShowAddress);
        ShowTypeChanged = (m_ShowType != NewShowType);
        m_ShowValues = NewShowValues;
        m_ShowAddress = NewShowAddress;
        m_ShowType = NewShowType;

        m_Model->SetShowValues (m_ShowValues);
        m_HexOrDec = Dlg.GetHexOrDec();
        m_Model->SetHexOrDecViewFlag (m_HexOrDec);
        char NewProcessName[MAX_PATH];
        Dlg.GetProcessName (NewProcessName);
        INCLUDE_EXCLUDE_FILTER *NewIncExcFilter = Dlg.GetFilter();
        FilterChanged = CompareIncludeExcludeFilter (m_IncExcFilter, NewIncExcFilter) != 0;
        ProcessNameChanged = Compare2ProcessNames (m_ProcessName, NewProcessName) != 0;

        if (FilterChanged) {
            FreeIncludeExcludeFilter (m_IncExcFilter);
            m_IncExcFilter = NewIncExcFilter;
        } else {
            FreeIncludeExcludeFilter (NewIncExcFilter);
        }

        if (ProcessNameChanged) {
            // Process name has changed
            strcpy (m_ProcessName, NewProcessName);
            if (m_DebugInfos != nullptr) {
                RemoveConnectFromProcessDebugInfos (m_UniqueNumber);
            }
            m_Pid = get_pid_by_name (m_ProcessName);
            m_DebugInfos = ConnectToProcessDebugInfos (m_UniqueNumber,
                                                       m_ProcessName,
                                                       &m_Pid,
                                                       nullptr,
                                                       GlobalNotifiyCalibrationTreeWidgetFuncSchedThread,
                                                       static_cast<void*>(this),
                                                       0);
            if (m_GlobalSectionFilter != nullptr) {
                DeleteAddrRangeFilter(m_GlobalSectionFilter);
            }
            m_GlobalSectionFilter = BuildGlobalAddrRangeFilter (m_DebugInfos);
            m_Model->SetIncludeExcludeFilter(m_IncExcFilter, m_GlobalSectionFilter);
            ShowHideColums();
            m_Model->Reset (m_Pid, m_DebugInfos);
            m_View->FitColumnSizes();
         } else {
            if (ShowValuesChanged || ShowAddressChanged || ShowTypeChanged) {
                ShowHideColums();
            }
            if (FilterChanged) {
                m_Model->SetIncludeExcludeFilter(m_IncExcFilter, m_GlobalSectionFilter);
                m_Model->Reset (m_Pid, m_DebugInfos);
            }
         }
        writeToIni();  // save this direct to the INI file
    }
}


int CalibrationTreeWidget::NotifiyStartStopProcess (int par_Flags, int par_Action)
{
    Q_UNUSED(par_Flags)
    if (par_Action == DEBUG_INFOS_ACTION_PROCESS_TERMINATED) {
        m_Pid = -1;
        m_Model->Reset(-1, m_DebugInfos);  // Process is invalid
    } else if (par_Action == DEBUG_INFOS_ACTION_PROCESS_STARTED) {
        m_Pid = get_pid_by_name (m_ProcessName);
        m_Model->Reset (m_Pid, m_DebugInfos);  // Process is now valid again
        if (m_GlobalSectionFilter != nullptr) {
            DeleteAddrRangeFilter(m_GlobalSectionFilter);
        }
        m_GlobalSectionFilter = BuildGlobalAddrRangeFilter (m_DebugInfos);

        m_View->FitColumnSizes();
    }
    return 0;
}

int CalibrationTreeWidget::GetPid()
{
    return m_Pid;
}

CustomDialogFrame* CalibrationTreeWidget::dialogSettings(QWidget* arg_parent)
{
    Q_UNUSED(arg_parent)
    // NOTE: required by inheritance but not used
    return nullptr;
}

void CalibrationTreeWidget::ShowHideColums()
{
    m_View->setColumnHidden(1, (m_ShowValues == 0));
    m_View->setColumnHidden(2, (m_ShowAddress == 0));
    m_View->setColumnHidden(3, (m_ShowType == 0));
}

void CalibrationTreeWidget::SetColumnWidth()
{
    if (m_ColumnWidthLabel < 0) m_ColumnWidthLabel = 200;
    if (m_ColumnWidthValue < 0) {
        int Width = m_View->width();
        m_ColumnWidthValue = Width - m_ColumnWidthLabel - 4;
    }
    if (m_ColumnWidthAddress < 0) m_ColumnWidthAddress = 100;
    if (m_ColumnWidthType < 0) m_ColumnWidthType = 100;
    m_View->header()->resizeSection(0, m_ColumnWidthLabel);
    if (m_ShowValues != 0) m_View->header()->resizeSection(1, m_ColumnWidthValue);
    if (m_ShowAddress != 0) m_View->header()->resizeSection(2, m_ColumnWidthAddress);
    if (m_ShowType != 0) m_View->header()->resizeSection(3, m_ColumnWidthType);
}

void CalibrationTreeWidget::ConfigDialogSlot (void)
{
    OpenConfigDialog();
}

void CalibrationTreeWidget::ReloadValuesSlot (void)
{
    m_Model->InvalidCachedValues();
    m_View->UpdateVisableValue();
}

void CalibrationTreeWidget::ReferenceLabelSlot (void)
{
    QList<QModelIndex> Selected = m_View->GetSelectedIndexes();
    for (int i = 0; i < Selected.size(); ++i) {
        QModelIndex ModelIndex = Selected.at(i);
        uint64_t Address = ModelIndex.data(Qt::UserRole + 2).toULongLong();  // Address
        int32_t TypeNr = ModelIndex.data(Qt::UserRole + 1).toInt();  // TypeNr
        QString Label = ModelIndex.data(Qt::UserRole + 4).toString();       // complete label name
        if (!Label.isEmpty() && (Address != 0)) {
            if (WaitUntilProcessIsNotActiveAndThanLockIt (m_Pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot reference variable", __FILE__, __LINE__) == 0) {
                if (scm_ref_vari_lock_flag (Address, m_Pid, QStringToConstChar(Label),
                                            get_base_type_bb_type_ex (TypeNr, m_DebugInfos), 0x3,  0)) {
                    UnLockProcess (m_Pid);
                    ThrowError (1, "cannot reference \"%s\" variable (extern process not running or halted in debugger)", QStringToConstChar(Label));
                } else {
                    AddVarToProcessRefList (m_Pid, QStringToConstChar(Label), Address, get_base_type_bb_type_ex (TypeNr, m_DebugInfos));
                    write_referenced_vari_ini (m_Pid, QStringToConstChar(Label), QStringToConstChar(Label));
                    UnLockProcess (m_Pid);
                }
            }
        }
    }
}

void CalibrationTreeWidget::ReferenceLabelUserNameSlot (void)
{
    QList<QModelIndex> Selected = m_View->GetSelectedIndexes();
    QStringList LabelList;
    QList<uint64_t> AddressList;
    QList<int32_t> TypeNrList;

    for (int i = 0; i < Selected.size(); ++i) {
        QModelIndex ModelIndex = Selected.at(i);
        uint64_t Address = ModelIndex.data(Qt::UserRole + 2).toULongLong();
        int32_t TypeNr = ModelIndex.data(Qt::UserRole + 1).toInt();
        AddressList.append(Address);
        TypeNrList.append(TypeNr);
        QString Label = ModelIndex.data(Qt::UserRole + 4).toString();       // complete label name
        if (!Label.isEmpty() && (Address != 0)) {
            LabelList.append(Label);
        }
    }
    CalibrationTreeAddReferenceDialog *Dlg = new CalibrationTreeAddReferenceDialog(LabelList, this);
    if (Dlg->exec() == QDialog::Accepted) {
        QStringList RenamingLabelList = Dlg->GetRenamings();
        if (WaitUntilProcessIsNotActiveAndThanLockIt (m_Pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot reference variable", __FILE__, __LINE__) == 0) {
            int ErrCounter = 0;
            for (int x = 0; x < RenamingLabelList.size(); x++) {
                uint64_t Address = AddressList.at(x);
                int32_t TypeNr = TypeNrList.at(x);
                QString Label = RenamingLabelList.at(x);
                if (scm_ref_vari_lock_flag (Address, m_Pid, QStringToConstChar(Label),
                                            get_base_type_bb_type_ex (TypeNr, m_DebugInfos), 0x3, 0)) {
                    ErrCounter++;
                } else {
                    AddVarToProcessRefList (m_Pid, QStringToConstChar(Label), Address, get_base_type_bb_type_ex (TypeNr, m_DebugInfos));
                    //                                Label name                         Dispaly name
                    write_referenced_vari_ini (m_Pid, QStringToConstChar(LabelList.at(x)), QStringToConstChar(Label));
                }
            }
            UnLockProcess (m_Pid);
            if (ErrCounter) {
                ThrowError (1, "cannot reference %i variable (extern process not running or halted in debugger)", ErrCounter);
            }
        }
    }
    delete Dlg;
}

void CalibrationTreeWidget::DeReferenceLabelSlot()
{
    QList<QModelIndex> Selected = m_View->GetSelectedIndexes();
    for (int i = 0; i < Selected.size(); ++i) {
        QModelIndex ModelIndex = Selected.at(i);
        uint64_t Address = ModelIndex.data(Qt::UserRole + 2).toULongLong();
        int32_t TypeNr = ModelIndex.data(Qt::UserRole + 1).toInt();
        QString Label = ModelIndex.data(Qt::UserRole + 4).toString();       // complete label name
        if (!Label.isEmpty() && (Address != 0)) {
            if (WaitUntilProcessIsNotActiveAndThanLockIt (m_Pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot dereference variable", __FILE__, __LINE__) == 0) {
                char Name[BBVARI_NAME_SIZE+100];
                if (GetDisplayNameByLabelName (m_Pid, QStringToConstChar(Label), Name, BBVARI_NAME_SIZE) != 0) {
                    // If there is no display name than use label name
                    strncpy (Name, QStringToConstChar(Label), BBVARI_NAME_SIZE);
                    Name[BBVARI_NAME_SIZE-1] = 0;
                }
                ConvertLabelAsapCombatible (Name, sizeof(Name), 0);   // replace :: with ._. if necessary

                if (scm_unref_vari_lock_flag (Address, m_Pid, Name,
                                              get_base_type_bb_type_ex (TypeNr, m_DebugInfos), 0)) {
                    UnLockProcess (m_Pid);
                    ThrowError (1, "cannot dereference \"%s\" variable (extern process not running or halted in debugger)", Name);
                } else {
                    DelVarFromProcessRefList (m_Pid, QStringToConstChar(Label));
                    remove_referenced_vari_ini (m_Pid, QStringToConstChar(Label));
                    UnLockProcess (m_Pid);
                }
            }
        }
    }
}

void CalibrationTreeWidget::ChangeValuesAllSelectedLabelsSlot (void)
{
    uint64_t *Addresses;
    int32_t *TypeNrs;
    union BB_VARI *Values;
    QList<QModelIndex> Selected = m_View->GetSelectedIndexes();
    union BB_VARI Value;
    int DisplayMode = 0;
    union BB_VARI PreselectedValue;
    int PreselectedBBdType = BB_INVALID;

    int Count = Selected.size();

    if (Count > 0) {
        Addresses = static_cast<uint64_t*>(alloca (static_cast<size_t>(Count) * sizeof (uint64_t)));
        TypeNrs = static_cast<int32_t*>(alloca (static_cast<size_t>(Count) * sizeof (int32_t)));
        Values = static_cast<union BB_VARI*>(alloca (static_cast<size_t>(Count) * sizeof (union BB_VARI)));
        if ((Addresses != nullptr) && (TypeNrs != nullptr) && (Values != nullptr)) {
            int y = 0;
            Value.uqw = 0;
            for (int i = 0; i < Count; ++i) {
                QModelIndex ModelIndex = Selected.at(i);
                if (ModelIndex.isValid()) {
                    QVariant TypeNrVariant = ModelIndex.data (Qt::UserRole + 1); // +1 -> Type nummer
                    if (TypeNrVariant.canConvert<int>()) {
                        int TypeNr = TypeNrVariant.toInt();
                        int BBType = get_base_type_bb_type_ex (TypeNr, m_DebugInfos);
                        if (BBType >= 0) {  // only bas e data types
                            QVariant AddressVariant = ModelIndex.data (Qt::UserRole + 2); // +2 -> Address
                            if (AddressVariant.canConvert<unsigned int>()) {
                                if (y == 0) {
                                    // First selected element defines the preselected value
                                    DisplayMode = ModelIndex.data (Qt::UserRole + 5).toInt();  // display type: 0 -> Integer dezimal, 1 -> Integer Hexadezimal, 2 -> Floatingpoint
                                    QString ValueString = ModelIndex.data (Qt::DisplayRole).toString();
                                    string_to_bbvari(static_cast<enum BB_DATA_TYPES>(BBType), &PreselectedValue, QStringToConstChar(ValueString));
                                    PreselectedBBdType = BBType;
                                }
                                Addresses[y] = AddressVariant.toULongLong();
                                TypeNrs[y] =TypeNr;
                                Values[y] = Value;
                                ModelIndex.data (Qt::UserRole + 3); // +3 -> this will delete the cached flag
                                y++;
                            }
                        }
                    }
                }
            }
            if (y > 0) {
                ChangeValueTextDialog Dialog;
                union BB_VARI ConvertedValue;
                switch (DisplayMode) {
                case 0:  // Dezimal Signed
                    sc_convert_from_to (PreselectedBBdType, &PreselectedValue, BB_QWORD, &ConvertedValue);
                    Dialog.SetValue (ConvertedValue.qw, 10);
                    break;
                case 1: // Hex Signed
                    sc_convert_from_to (PreselectedBBdType, &PreselectedValue, BB_QWORD, &ConvertedValue);
                    Dialog.SetValue (ConvertedValue.qw, 16);
                    break;
                case 2:  // Dezimal Unigned
                    sc_convert_from_to (PreselectedBBdType, &PreselectedValue, BB_UQWORD, &ConvertedValue);
                    Dialog.SetValue (ConvertedValue.uqw, 10);
                    break;
                case 3:  // Hex Unsigned
                    sc_convert_from_to (PreselectedBBdType, &PreselectedValue, BB_UQWORD, &ConvertedValue);
                    Dialog.SetValue (ConvertedValue.uqw, 16);
                    break;
                case 4:  // Double/Float
                    sc_convert_from_to (PreselectedBBdType, &PreselectedValue, BB_DOUBLE, &ConvertedValue);
                    Dialog.SetValue (ConvertedValue.d);
                    break;
                }
                if (Dialog.exec() == QDialog::Accepted) {
                    int BBType;
                    switch (DisplayMode) {
                    case 0:
                    case 1:
                        Value.qw = Dialog.GetInt64Value();
                        BBType = BB_QWORD;
                        break;
                    case 2:
                    case 3:
                        Value.uqw = Dialog.GetUInt64Value();
                        BBType = BB_UQWORD;
                        break;
                    case 4:
                    default:
                        Value.d = Dialog.GetDoubleValue();
                        BBType = BB_DOUBLE;
                        break;
                    }
                    for (int i = 0; i < y; i++) {
                        sc_convert_from_to (BBType, &Value, get_base_type_bb_type_ex (TypeNrs[i], m_DebugInfos), &Values[i]);
                    }
                    WriteValuesToExternProcessViaAddressStopScheduler (m_DebugInfos, m_Pid, y,
                                                                       Addresses, TypeNrs, Values,
                                                                       5000, true);
                    // At the end delete the affected cached flags
                    for (int i = 0; i < Count; ++i) {
                        QModelIndex ModelIndex = Selected.at(i);
                        if (ModelIndex.isValid()) {
                            ModelIndex.data (Qt::UserRole + 3); // +3 -> this will delete the cached flag
                        }
                    }
                }
            }
        }
    }
}

void CalibrationTreeWidget::ListAllReferencedLabelSlot (void)
{
    ReferencedLabelsDialog Dlg (QString (m_ProcessName), this);
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void CalibrationTreeWidget::changeColor(QColor arg_color)
{
    Q_UNUSED(arg_color)
    // NOTE: required by inheritance but not used
}

void CalibrationTreeWidget::changeFont(QFont arg_font)
{
    Q_UNUSED(arg_font)
    // NOTE: required by inheritance but not used
}

void CalibrationTreeWidget::changeWindowName(QString arg_name)
{
    Q_UNUSED(arg_name)
    // NOTE: required by inheritance but not used
}

void CalibrationTreeWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    Q_UNUSED(arg_variable)
    Q_UNUSED(arg_visible)
    // NOTE: required by inheritance but not used
}

void CalibrationTreeWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_variables)
    Q_UNUSED(arg_visible)
    // NOTE: required by inheritance but not used
}

void CalibrationTreeWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables);

}


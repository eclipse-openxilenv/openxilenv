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


#include "ConfigureProcessDialog.h"
#include "ui_ConfigureProcessDialog.h"

#include "AddNewProcessSchedulerBarrier.h"
#include "ConfigureBarrierDialog.h"
#include "ConfigureSchedulersDialog.h"
#include "FileDialog.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

extern "C" {
#include "PrintFormatToString.h"
#include "StringMaxChar.h"
#include "Scheduler.h"
#include "ThrowError.h"
#include "Files.h"
#include "MainValues.h"
#include "RemoteMasterScheduler.h"
#include "A2LLink.h"
#include "EnvironmentVariables.h"
}

#define NOT_RUNNING_PROCESS_POSTFIX   "  (not running)"

ConfigureProcessDialog::ConfigureProcessDialog(QString par_SelectedProcess, QWidget *parent) : Dialog(parent),
    ui(new Ui::ConfigureProcessDialog)
{
    Q_UNUSED(par_SelectedProcess);
    ui->setupUi(this);
    ui->ProcessStateComboBox->addItem(QString("active"));  // PROCESS_AKTIV         0
    ui->ProcessStateComboBox->addItem(QString("sleep"));   // PROCESS_SLEEP         1
    ui->ProcessStateComboBox->addItem(QString("no sleep able"));   //PROCESS_NONSLEEP      2
    ui->ProcessStateComboBox->addItem(QString("reference"));   // EX_PROCESS_REFERENCE  3
    ui->ProcessStateComboBox->addItem(QString("init"));   // EX_PROCESS_INIT       4
    ui->ProcessStateComboBox->addItem(QString("terminate"));   // EX_PROCESS_TERMINATE  5
    ui->ProcessStateComboBox->addItem(QString("login"));   //             6
    ui->ProcessStateComboBox->addItem(QString(""));   // WIN_PROCESS           7
    ui->ProcessStateComboBox->addItem(QString(""));   // RT_PROCESS_INIT       8
    ui->ProcessStateComboBox->addItem(QString(""));   // RT_PROCESS_TERMINATE  9
    ui->ProcessStateComboBox->addItem(QString("go to terminate"));   // EX_PROCESS_WAIT_FOR_END_OF_CYCLE_TO_GO_TO_TERM   10

    ui->ProcessTypeComboBox->addItem(QString("internal"));
    ui->ProcessTypeComboBox->addItem(QString("external"));
    ui->ProcessTypeComboBox->addItem(QString("realtime"));

    if (!s_main_ini_val.ConnectToRemoteMaster) {
        ui->ResetPushButton->setDisabled(true);
        ui->ReloadPushButton->setDisabled(true);
    }

    ReadAllProcessesInfos(false);
    InsertAllSchedulerAndBarrier();

    UpdateProcessListView();
}

ConfigureProcessDialog::~ConfigureProcessDialog()
{
    delete ui;
}


struct ConfigureProcessDialog::DialogData ConfigureProcessDialog::ReadProcessInfos(QString &SelectedProcess)
{
    struct DialogData Ret;
    MEMSET (&Ret, 0, sizeof (Ret));
    int Prio;
    int Cycles;
    int Delay;
    bool PrioCyclesDelaysValid = false;

    int Pid = get_pid_by_name(QStringToConstChar(SelectedProcess));

    Ret.m_Pid = Pid;

    if (Pid > 0) {
        int Type;
        int MessageSize;
        char AssignedScheduler[MAX_PATH];
        uint64_t bb_access_mask;
        int State;
        uint64_t CyclesCounter;
        uint64_t CyclesTime;
        uint64_t CyclesTimeMax;
        uint64_t CyclesTimeMin;
        if (get_process_info_ex (Pid, nullptr, 0, &Type, &Prio, &Cycles, &Delay, &MessageSize,
                                 AssignedScheduler, sizeof(AssignedScheduler), &bb_access_mask,
                                 &State, &CyclesCounter, &CyclesTime, &CyclesTimeMax, &CyclesTimeMin) == 0) {
            Ret.m_IsRunning = 1;
            Ret.m_State = State;
            Ret.m_Type = Type;
            Ret.m_Mask = bb_access_mask;
            Ret.m_CycleCount = CyclesCounter;
            Ret.m_CycleTime = CyclesTime;
            Ret.m_CycleTimeMax = CyclesTimeMax;
            Ret.m_CycleTimeMin = CyclesTimeMin;
            PrioCyclesDelaysValid = true;
        } else {
            PrioCyclesDelaysValid = false;
        }

        if (get_process_info_internal_debug (Pid,
                                             &Ret.m_hPipe,

                                             &Ret.m_SocketOrPipe,

                                             &Ret.m_IsStartedInDebugger,  // This is 1 if prozess was started inside a debugger
                                             &Ret.m_MachineType,          // If 0 -> 32Bit Windows 1 -> 64Bit Windows, 2 -> 32Bit Linux, 3 -> 64Bit Linux

                                             &Ret.m_UseDebugInterfaceForWrite,
                                             &Ret.m_UseDebugInterfaceForRead,

                                             &Ret.m_wait_on,              // This is set if somthing waiting on this external process


                                             &Ret.m_WorkingFlag,   // Process are inside its cyclic/init/terminate/reference function
                                             &Ret.m_LockedFlag,    // Process should not be inside its cyclic/init/terminate/reference function and the scheduler have to wait
                                             &Ret.m_SchedWaitForUnlocking,  // Scheduler waiting for calling cyclic/init/terminate/reference of the process
                                             &Ret.m_WaitForEndOfCycleFlag,  // Somebody is waiting for the end of an external process cyclic/init/terminate/reference function

                                             &Ret.m_LockedByThreadId,
                                             Ret.m_SrcFileName,
                                             sizeof(Ret.m_SrcFileName),
                                             &Ret.m_SrcLineNr,
                                             Ret.m_DllName,     // This is the name of the DLL if the process lives inside a DLL otherwise this is NULL
                                             sizeof(Ret.m_DllName),
                                             Ret.m_InsideExecutableName,
                                             sizeof(Ret.m_InsideExecutableName),
                                             &Ret.m_NumberInsideExecutable,


                                             &Ret.m_ExecutableBaseAddress,
                                             &Ret.m_DllBaseAddress,

                                             &Ret.m_ExternProcessComunicationProtocolVersion) == 0) {
            ;  // do nothing
        }
    }

    unsigned int RangeControlFlags;
    if (read_extprocinfos_from_ini (GetMainFileDescriptor(),
                                    QStringToConstChar(SelectedProcess),
                                    &Ret.m_Priority,
                                    &Ret.m_TimeSteps,
                                    &Ret.m_Delay,
                                    &Ret.m_Timeout,
                                    Ret.m_RangeErrorCounter, sizeof(Ret.m_RangeErrorCounter),
                                    Ret.m_RangeControl, sizeof( Ret.m_RangeControl),
                                    &RangeControlFlags,
                                    Ret.m_BBPrefix, sizeof(Ret.m_BBPrefix),
                                    Ret.m_SchedulerName, sizeof(Ret.m_SchedulerName),
                                    Ret.m_BarriersBeforeOnlySignal, sizeof(Ret.m_BarriersBeforeOnlySignal),
                                    Ret.m_BarriersBeforeSignalAndWait, sizeof(Ret.m_BarriersBeforeSignalAndWait),
                                    Ret.m_BarriersBehindOnlySignal, sizeof(Ret.m_BarriersBehindOnlySignal),
                                    Ret.m_BarriersBehindSignalAndWait, sizeof(Ret.m_BarriersBehindSignalAndWait),
                                    Ret.m_BarriersLoopOutBeforeOnlySignal, sizeof(Ret.m_BarriersLoopOutBeforeOnlySignal),
                                    Ret.m_BarriersLoopOutBeforeSignalAndWait, sizeof(Ret.m_BarriersLoopOutBeforeSignalAndWait),
                                    Ret.m_BarriersLoopOutBehindOnlySignal, sizeof(Ret.m_BarriersLoopOutBehindOnlySignal),
                                    Ret.m_BarriersLoopOutBehindSignalAndWait, sizeof(Ret.m_BarriersLoopOutBehindSignalAndWait)) != 0) {
        ;  // nix
    }
    if (PrioCyclesDelaysValid) {
        // If ther is no INI entry use the information from the TCB
        Ret.m_Priority = Prio;
        Ret.m_TimeSteps = Cycles;
        Ret.m_Delay = Delay;
    }
    #define CHECK_RANGE_CONTROL_FLAG(f) ((RangeControlFlags & (f)) == (f))
    Ret.m_RangeControlBeforeActiveFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_BEFORE_ACTIVE_FLAG);
    Ret.m_RangeControlBehindActiveFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_BEHIND_ACTIVE_FLAG);
    Ret.m_RangeControlBeforeDeactiveAtStartup = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_BEFORE_INIT_AS_DEACTIVE);
    Ret.m_RangeControlBehindDeactiveAtStartup = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE);
    Ret.m_RangeControlStopSchedFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_STOP_SCHEDULER_FLAG);
    Ret.m_RangeControlOutput = RangeControlFlags & RANGE_CONTROL_OUTPUT_MASK;              // Messagebox, Write to File, ...
    Ret.m_RangeErrorCounterFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_COUNTER_VARIABLE_FLAG);
    Ret.m_RangeControlVarFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_CONTROL_VARIABLE_FLAG);
    Ret.m_RangeControlPhysFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_PHYSICAL_FLAG);
    Ret.m_RangeControlLimitValuesFlag = CHECK_RANGE_CONTROL_FLAG (RANGE_CONTROL_LIMIT_VALUES_FLAG);

    GetBeforeProcessEquationFileName (QStringToConstChar(SelectedProcess), Ret.m_EquationBefore);
    GetBehindProcessEquationFileName (QStringToConstChar(SelectedProcess), Ret.m_EquationBehind);
    GetSVLFileLoadedBeforeInitProcessFileName (QStringToConstChar(SelectedProcess), Ret.m_SvlFile, sizeof(Ret.m_SvlFile));
    GetA2LFileAssociatedProcessFileName(QStringToConstChar(SelectedProcess), Ret.m_A2LFile, sizeof(Ret.m_A2LFile), &Ret.m_UpdateA2LFileFlags);

    return Ret;
}

struct ConfigureProcessDialog::DialogData ConfigureProcessDialog::GetProcessInfos(QString &SelectedProcess)
{
    struct DialogData Ret = m_ProcessInfos.value (SelectedProcess);
    Ret.m_State = ui->ProcessStateComboBox->currentIndex();
    Ret.m_TimeSteps = ui->ProcessRepetionCycleLineEdit->text().toInt();
    Ret.m_Delay = ui->ProcessDelayLineEdit->text().toInt();
    Ret.m_Priority = ui->ProcessOrderLineEdit->text().toInt();
    QString EquationBefore = ui->EquationBeforeLineEdit->text().trimmed();
    EquationBefore.truncate(sizeof (Ret.m_EquationBefore)-1);
    STRING_COPY_TO_ARRAY (Ret.m_EquationBefore, QStringToConstChar(EquationBefore));
    QString EquationBehind = ui->EquationBehindLineEdit->text().trimmed();
    EquationBehind.truncate(sizeof (Ret.m_EquationBehind)-1);
    STRING_COPY_TO_ARRAY (Ret.m_EquationBehind, QStringToConstChar(EquationBehind));
    QString SvlFile = ui->SVLLineEdit->text().trimmed();
    SvlFile.truncate(sizeof (Ret.m_SvlFile)-1);
    STRING_COPY_TO_ARRAY (Ret.m_SvlFile, QStringToConstChar(SvlFile));
    QString A2LFile = ui->A2LLineEdit->text().trimmed();
    A2LFile.truncate(sizeof (Ret.m_A2LFile)-1);
    STRING_COPY_TO_ARRAY (Ret.m_A2LFile, QStringToConstChar(A2LFile));
    Ret.m_UpdateA2LFileFlags = 0;
    if (ui->UpdateA2LCheckBox->isChecked()) Ret.m_UpdateA2LFileFlags |= A2L_LINK_UPDATE_FLAG;
    if (ui->UpdateDLLA2LCheckBox->isChecked()) Ret.m_UpdateA2LFileFlags |= A2L_LINK_ADDRESS_TRANSLATION_DLL_FLAG;
    if (ui->UpdateMultiDLLA2LCheckBox->isChecked()) Ret.m_UpdateA2LFileFlags |= A2L_LINK_ADDRESS_TRANSLATION_MULTI_DLL_FLAG;
    if (ui->IgnoreModCommonAlignmentsCheckBox->isChecked()) Ret.m_UpdateA2LFileFlags |= A2L_LINK_IGNORE_MOD_COMMON_ALIGNMENTS_FLAG;
    if (ui->IgnoreRecordLayoutAlignmentsCheckBox->isChecked()) Ret.m_UpdateA2LFileFlags |= A2L_LINK_IGNORE_RECORD_LAYOUT_ALIGNMENTS_FLAG;
    if (ui->RememberReferencedCheckBox->isChecked()) Ret.m_UpdateA2LFileFlags |= A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG;
    if (ui->IgnoreReadOnlyCheckBox->isChecked()) Ret.m_UpdateA2LFileFlags |= A2L_LINK_IGNORE_READ_ONLY_FLAG;
    if (ui->DefaultAlignmentCheckBox->isChecked()) Ret.m_UpdateA2LFileFlags |= A2L_LINK_DEFAUT_ALIGNMENT_FLAG;

    Ret.m_RangeErrorCounterFlag = ui->RangeControlErrorCounterCheckBox->isChecked();
    Ret.m_RangeControlBeforeActiveFlag = ui->RangeControlBeforeProceesCheckBox->isChecked();
    Ret.m_RangeControlBehindActiveFlag = ui->RangeControlBehindProceesCheckBox->isChecked();
    Ret.m_RangeControlBeforeDeactiveAtStartup = ui->RangeControlBeforeDefaultCheckBox->isChecked();
    Ret.m_RangeControlBehindDeactiveAtStartup = ui->RangeControlBehindDefaultCheckBox->isChecked();
    Ret.m_RangeControlVarFlag = ui->RangeControlVariableCheckBox->isChecked();
    Ret.m_RangeControlPhysFlag = ui->RangeControlPhysicalCheckBox->isChecked();
    Ret.m_RangeControlStopSchedFlag = ui->RangeControlStopSchedulerCheckBox->isChecked();
    Ret.m_RangeControlLimitValuesFlag = ui->RangeControlLimitValuesProceesCheckBox->isChecked();
    MEMSET (Ret.m_RangeControl, 0, sizeof (Ret.m_RangeControl));
    if (Ret.m_RangeControlVarFlag) {
        strncpy (Ret.m_RangeControl, QStringToConstChar(ui->RangeControlVariableLineEdit->text()), sizeof (Ret.m_RangeControl) - 1);
    }
    MEMSET (Ret.m_RangeErrorCounter, 0, sizeof (Ret.m_RangeErrorCounter));
    if (Ret.m_RangeErrorCounterFlag) {
        strncpy (Ret.m_RangeErrorCounter, QStringToConstChar(ui->RangeControlErrorCounterLineEdit->text()), sizeof (Ret.m_RangeErrorCounter) - 1);
    }

    if (ui->RangeControlNoOutputRadioButton->isChecked()) {
        Ret.m_RangeControlOutput = 0;
    } else if (ui->RangeControlErrorMessageBoxRadioButton->isChecked()) {
        Ret.m_RangeControlOutput = 1;
    } else if (ui->RangeControlWriteToFileRadioButton->isChecked()) {
        Ret.m_RangeControlOutput = 2;
    }
    QString SchedulerName = ui->SchedulerComboBox->currentText();
    if (!SchedulerName.isEmpty()) {
         SchedulerName.truncate(sizeof (Ret.m_SchedulerName));
         STRING_COPY_TO_ARRAY (Ret.m_SchedulerName, QStringToConstChar(SchedulerName));
    } else {
        STRING_COPY_TO_ARRAY (Ret.m_SchedulerName, "");
    }
    QList<QListWidgetItem*> AllItems;
    int Count;
    AllItems = ui->BeforeBarrierListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersBeforeOnlySignal[0] = 0;
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) STRING_APPEND_TO_ARRAY (Ret.m_BarriersBeforeOnlySignal, ";");
        STRING_APPEND_TO_ARRAY (Ret.m_BarriersBeforeOnlySignal, QStringToConstChar(Item->data(Qt::DisplayRole).toString()));
    }

    AllItems = ui->BeforeBarrierWaitListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersBeforeSignalAndWait[0] = 0;
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) STRING_APPEND_TO_ARRAY (Ret.m_BarriersBeforeSignalAndWait, ";");
        STRING_APPEND_TO_ARRAY (Ret.m_BarriersBeforeSignalAndWait, QStringToConstChar(Item->data(Qt::DisplayRole).toString()));
    }

    AllItems = ui->BehindBarrierListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersBehindOnlySignal[0] = 0;
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) STRING_APPEND_TO_ARRAY (Ret.m_BarriersBehindOnlySignal, ";");
        STRING_APPEND_TO_ARRAY (Ret.m_BarriersBehindOnlySignal, QStringToConstChar(Item->data(Qt::DisplayRole).toString()));
    }

    AllItems = ui->BehindBarrierWaitListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersBehindSignalAndWait[0] = 0;
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) STRING_APPEND_TO_ARRAY (Ret.m_BarriersBehindSignalAndWait, ";");
        STRING_APPEND_TO_ARRAY (Ret.m_BarriersBehindSignalAndWait, QStringToConstChar(Item->data(Qt::DisplayRole).toString()));
    }
// Loop out
    AllItems = ui->BeforeBarrierLoopOutListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersLoopOutBeforeOnlySignal[0] = 0;
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) STRING_APPEND_TO_ARRAY (Ret.m_BarriersLoopOutBeforeOnlySignal, ";");
        STRING_APPEND_TO_ARRAY (Ret.m_BarriersLoopOutBeforeOnlySignal, QStringToConstChar(Item->data(Qt::DisplayRole).toString()));
    }

    AllItems = ui->BeforeBarrierLoopOutWaitListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersLoopOutBeforeSignalAndWait[0] = 0;
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) STRING_APPEND_TO_ARRAY (Ret.m_BarriersLoopOutBeforeSignalAndWait, ";");
        STRING_APPEND_TO_ARRAY (Ret.m_BarriersLoopOutBeforeSignalAndWait, QStringToConstChar(Item->data(Qt::DisplayRole).toString()));
    }

    AllItems = ui->BehindBarrierLoopOutListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersLoopOutBehindOnlySignal[0] = 0;
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) STRING_APPEND_TO_ARRAY (Ret.m_BarriersLoopOutBehindOnlySignal, ";");
        STRING_APPEND_TO_ARRAY (Ret.m_BarriersLoopOutBehindOnlySignal, QStringToConstChar(Item->data(Qt::DisplayRole).toString()));
    }

    AllItems = ui->BehindBarrierLoopOutWaitListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersLoopOutBehindSignalAndWait[0] = 0;
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) STRING_APPEND_TO_ARRAY (Ret.m_BarriersLoopOutBehindSignalAndWait, ";");
        STRING_APPEND_TO_ARRAY (Ret.m_BarriersLoopOutBehindSignalAndWait, QStringToConstChar(Item->data(Qt::DisplayRole).toString()));
    }
    return Ret;
}


void ConfigureProcessDialog::UpdateProcessListView()
{
    char LongProcessName[MAX_PATH];

    ui->ProceessListWidget->clear();
    QMap<QString, struct ConfigureProcessDialog::DialogData>::iterator i;

    for (i = m_ProcessInfos.begin(); i != m_ProcessInfos.end(); ++i) {
        QString ProcessName(i.key());
        QListWidgetItem *Item = new QListWidgetItem(ProcessName,ui->ProceessListWidget);
        GetProcessLongName(get_pid_by_name (QStringToConstChar(i.key())), LongProcessName, sizeof(LongProcessName));
        Item->setToolTip(QString(LongProcessName));
    }
}

static int StringCompareBegin (const char *String, const char *BeginWith)
{
    size_t Len = strlen (BeginWith);
    return strncmp (String, BeginWith, Len);
}

void ConfigureProcessDialog::ReadAllProcessesInfos(bool par_WithNonRunning)
{
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    ui->ProceessListWidget->clear();

    // First insert all running processes
    char *pName;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name ( 2 | 0x100 | 0x200);
    while ((pName = read_next_process_name (Buffer)) != nullptr) {
        QString Name = CharToQString(pName);
        ui->ProceessListWidget->addItem (Name);

        struct ConfigureProcessDialog::DialogData Data = ConfigureProcessDialog::ReadProcessInfos(Name);
        m_ProcessInfos.insert (Name, Data);
    }
    close_read_next_process_name(Buffer);

    // Than this from the INI-File (but not running)
    if (par_WithNonRunning) {
        int EntryIdx = 0;
        while ((EntryIdx = IniFileDataBaseGetNextEntryName (EntryIdx, "Scheduler", Entry, sizeof (Entry), GetMainFileDescriptor())) >= 0) {
            // Only use the entrys for the processs
            if (StringCompareBegin (Entry, "Equation before") &&
                StringCompareBegin (Entry, "Equation behind") &&
                StringCompareBegin (Entry, "BBPrefixForProcess") &&
                StringCompareBegin (Entry, "SchedulerForProcess") &&
                StringCompareBegin (Entry, "BarriersBeforeOnlySignalFor") &&
                StringCompareBegin (Entry, "BarriersBeforeSignalAndWaitFor") &&
                StringCompareBegin (Entry, "BarriersBehindOnlySignalFor") &&
                StringCompareBegin (Entry, "BarriersBehindSignalAndWaitFor") &&
                StringCompareBegin (Entry, "BarriersLoopOutBeforeOnlySignalFor") &&
                StringCompareBegin (Entry, "BarriersLoopOutBeforeSignalAndWaitFor") &&
                StringCompareBegin (Entry, "BarriersLoopOutBehindOnlySignalFor") &&
                StringCompareBegin (Entry, "BarriersLoopOutBehindSignalAndWaitFor") &&
                StringCompareBegin (Entry, "SVL before") &&
                StringCompareBegin (Entry, "A2L associated") &&
                StringCompareBegin (Entry, "Scheduler_") &&
                StringCompareBegin (Entry, "PriorityForScheduler") &&
                strcmp (Entry, "Period") &&
                strcmp (Entry, "LoginTimeout") &&
                strcmp (Entry, "Login Timeout") &&
                strcmp (Entry, "SyncWithFlexray")) {
                QString Name(Entry);
                if (ui->ProceessListWidget->findItems (Name, Qt::MatchFixedString).isEmpty()) {
                    m_ProcessInfos.insert (Name, ConfigureProcessDialog::ReadProcessInfos(Name));
                    ui->ProceessListWidget->addItem (Name.append(NOT_RUNNING_PROCESS_POSTFIX));
                }
            }
        }
    }
    // Make a copy to find out the changes inside accept()
    m_ProcessInfosSave = m_ProcessInfos;

    ui->ProceessListWidget->setCurrentRow(0);  // select the first process
}

void ConfigureProcessDialog::InsertAllSchedulerAndBarrier()
{
    char Entry[64];
    char SchedulerName[INI_MAX_LINE_LENGTH];

    ui->SchedulerComboBox->clear();
    // The first scheduler will exist always with index 0
    ui->SchedulerComboBox->addItem (QString ("Scheduler"));

    for (int s = 1; ; s++) {
        PrintFormatToString (Entry, sizeof(Entry), "Scheduler_%i", s);
        if (IniFileDataBaseReadString (SCHED_INI_SECTION, Entry, "", SchedulerName, sizeof (SchedulerName), GetMainFileDescriptor()) <= 0) {
            break;
        }
        ui->SchedulerComboBox->addItem(QString(SchedulerName));
    }
    char Line[INI_MAX_LINE_LENGTH];
    ui->AvailableBeforeBarrierListWidget->clear();
    ui->AvailableBehindBarrierListWidget->clear();
    ui->AvailableBeforeBarrierLoopOutListWidget->clear();
    ui->AvailableBehindBarrierLoopOutListWidget->clear();
    for (int b = 0; ; b++) {
        PrintFormatToString (Entry, sizeof(Entry), "Barrier_%i", b);
        if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", Line, sizeof (Line), GetMainFileDescriptor()) <= 0) {
            break;
        }
        char *BarrierName;
        char *WaitIfAlone;
        int WaitIfAloneFlag = 0;
        if (StringCommaSeparate (Line, &BarrierName, &WaitIfAlone, NULL) >= 2) {
            if (!strcmpi (WaitIfAlone, "WaitIfAlone")) {
                WaitIfAloneFlag = 1;
            }
        } else {
            BarrierName = Line;
        }
        ui->AvailableBeforeBarrierListWidget->addItem(QString(BarrierName));
        ui->AvailableBehindBarrierListWidget->addItem(QString(BarrierName));
        ui->AvailableBeforeBarrierLoopOutListWidget->addItem(QString(BarrierName));
        ui->AvailableBehindBarrierLoopOutListWidget->addItem(QString(BarrierName));
    }
}



static void FillBarrierListWidget (char *String, QListWidget *ListWidget)
{
    char *p, *pp;

    ListWidget->clear();
    pp = p = String;
    if ((p != nullptr) && (*p != 0)) {
        while (1) {
            if (*p == ';') {
                *p = 0;
                p++;
                ListWidget->addItem (QString (pp));
                pp = p;
            } else if (*p == 0) {
                ListWidget->addItem (QString (pp));
                break;
            }
            p++;
        }
    }
}

void ConfigureProcessDialog::SetProcessInfos (QString &SelectedProcess)
{
    if (!SelectedProcess.isEmpty()) {
        struct DialogData Data = m_ProcessInfos.value(SelectedProcess);
        ui->ProcessIdLineEdit->setText (QString ("%1").arg (Data.m_Pid));
        ui->ProcessOrderLineEdit->setText (QString ("%1").arg (Data.m_Priority));
        ui->ProcessRepetionCycleLineEdit->setText (QString ("%1").arg (Data.m_TimeSteps));
        ui->ProcessDelayLineEdit->setText (QString ("%1").arg (Data.m_Delay));
        switch (Data.m_State) {
        case PROCESS_AKTIV:
            ui->ProcessStateComboBox->setCurrentIndex(0);
            break;
        case PROCESS_SLEEP:
            ui->ProcessStateComboBox->setCurrentIndex(1);
            break;
        case PROCESS_NONSLEEP:
            ui->ProcessStateComboBox->setCurrentIndex(2);
            break;
        case EX_PROCESS_REFERENCE:
            ui->ProcessStateComboBox->setCurrentIndex(3);
            break;
        case EX_PROCESS_INIT:
            ui->ProcessStateComboBox->setCurrentIndex(4);
            break;
        case EX_PROCESS_TERMINATE:
            ui->ProcessStateComboBox->setCurrentIndex(5);
            break;
        case EX_PROCSS_LOGIN:
            ui->ProcessStateComboBox->setCurrentIndex(6);
            break;
        case WIN_PROCESS:
            ui->ProcessStateComboBox->setCurrentIndex(7);
            break;
        case RT_PROCESS_INIT:
            ui->ProcessStateComboBox->setCurrentIndex(8);
            break;
        case RT_PROCESS_TERMINATE:
            ui->ProcessStateComboBox->setCurrentIndex(9);
            break;
        case EX_PROCESS_WAIT_FOR_END_OF_CYCLE_TO_GO_TO_TERM:
            ui->ProcessStateComboBox->setCurrentIndex(10);
            break;
        }

        switch (Data.m_Type) {
        case 0:  // internal process
        default:
            ui->RealTimeGroupBox->setDisabled(true);
            ui->ProcessTypeComboBox->setCurrentIndex(0);
            break;
        case 2: // external process
            ui->RealTimeGroupBox->setDisabled(true);
            ui->ProcessTypeComboBox->setCurrentIndex(1);
            break;
        case 1:
        case 3:  // internal realtime process  (only with remote master active)
            ui->RealTimeGroupBox->setDisabled(false);
            ui->ProcessTypeComboBox->setCurrentIndex(2);
            break;
        }

        ui->CycleCounterLineEdit->setText (QString ().number(Data.m_CycleCount));
        ui->BlackboardMaskLineEdit->setText (QString ().number(Data.m_Mask, 16));

        char LongProcessName[MAX_PATH];
        STRING_COPY_TO_ARRAY (LongProcessName, "");
        GetProcessLongName (Data.m_Pid, LongProcessName, sizeof(LongProcessName));
        ui->LongProcessNameLineEdit->setText (CharToQString(LongProcessName));

        if (!Data.m_IsRunning) {
            ui->ProcessIdLineEdit->setText (QString ("unknown"));
            ui->ProcessOrderLineEdit->setText (QString ("%1").arg (Data.m_Priority));
            ui->ProcessRepetionCycleLineEdit->setText(QString ("%1").arg (Data.m_TimeSteps));
            ui->ProcessDelayLineEdit->setText (QString ("%1").arg (Data.m_Delay));
        }

        ui->RangeControlBeforeProceesCheckBox->setChecked (Data.m_RangeControlBeforeActiveFlag);
        ui->RangeControlBeforeDefaultCheckBox->setEnabled (Data.m_RangeControlBeforeActiveFlag);
        ui->RangeControlBeforeDefaultCheckBox->setChecked (Data.m_RangeControlBeforeDeactiveAtStartup);
        ui->RangeControlBehindProceesCheckBox->setChecked (Data.m_RangeControlBehindActiveFlag);
        ui->RangeControlBehindDefaultCheckBox->setEnabled (Data.m_RangeControlBehindActiveFlag);
        ui->RangeControlBehindDefaultCheckBox->setChecked (Data.m_RangeControlBehindDeactiveAtStartup);
        ui->RangeControlErrorCounterCheckBox->setChecked (Data.m_RangeErrorCounterFlag);
        ui->RangeControlVariableCheckBox->setChecked (Data.m_RangeControlVarFlag);
        ui->RangeControlPhysicalCheckBox->setChecked (Data.m_RangeControlPhysFlag);
        ui->RangeControlStopSchedulerCheckBox->setChecked (Data.m_RangeControlStopSchedFlag);
        ui->RangeControlLimitValuesProceesCheckBox->setChecked (Data.m_RangeControlLimitValuesFlag);
        if (Data.m_RangeControlVarFlag) {
            ui->RangeControlVariableLineEdit->setText(QString(Data.m_RangeControl));
        } else {
            ui->RangeControlVariableLineEdit->clear();
        }
        if (Data.m_RangeErrorCounterFlag) {
            ui->RangeControlErrorCounterLineEdit->setText(QString(Data.m_RangeErrorCounter));
        } else {
            ui->RangeControlErrorCounterLineEdit->clear();
        }
        ui->RangeControlNoOutputRadioButton->setChecked((Data.m_RangeControlOutput & 0xF) == 0);
        ui->RangeControlErrorMessageBoxRadioButton->setChecked((Data.m_RangeControlOutput & 0xF) == 1);
        ui->RangeControlWriteToFileRadioButton->setChecked((Data.m_RangeControlOutput & 0xF) == 2);

        ui->EquationBeforeLineEdit->setText (QString(Data.m_EquationBefore));
        ui->EquationBehindLineEdit->setText (QString(Data.m_EquationBehind));

        ui->SVLLineEdit->setText(QString(Data.m_SvlFile));
        ui->A2LLineEdit->setText(QString(Data.m_A2LFile));
        ui->UpdateA2LCheckBox->setChecked((Data.m_UpdateA2LFileFlags & A2L_LINK_UPDATE_FLAG) != 0);
        ui->UpdateDLLA2LCheckBox->setChecked((Data.m_UpdateA2LFileFlags & A2L_LINK_ADDRESS_TRANSLATION_DLL_FLAG) != 0);
        ui->UpdateMultiDLLA2LCheckBox->setChecked((Data.m_UpdateA2LFileFlags & A2L_LINK_ADDRESS_TRANSLATION_MULTI_DLL_FLAG) != 0);
        ui->IgnoreModCommonAlignmentsCheckBox->setChecked((Data.m_UpdateA2LFileFlags & A2L_LINK_IGNORE_MOD_COMMON_ALIGNMENTS_FLAG) != 0);
        ui->IgnoreRecordLayoutAlignmentsCheckBox->setChecked((Data.m_UpdateA2LFileFlags & A2L_LINK_IGNORE_RECORD_LAYOUT_ALIGNMENTS_FLAG) != 0);
        ui->RememberReferencedCheckBox->setChecked((Data.m_UpdateA2LFileFlags & A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG) != 0);
        ui->IgnoreReadOnlyCheckBox->setChecked((Data.m_UpdateA2LFileFlags & A2L_LINK_IGNORE_READ_ONLY_FLAG) != 0);
        ui->DefaultAlignmentCheckBox->setChecked((Data.m_UpdateA2LFileFlags & A2L_LINK_DEFAUT_ALIGNMENT_FLAG) != 0);

        if (strlen (Data.m_BBPrefix)) {
            ui->PrefixCheckBox->setCheckState(Qt::Checked);
            ui->PrefixLineEdit->setText(QString(Data.m_BBPrefix));
        } else {
            ui->PrefixCheckBox->setCheckState(Qt::Unchecked);
            ui->PrefixLineEdit->setText(QString(""));
        }
        ui->SchedulerComboBox->setCurrentText(QString(Data.m_SchedulerName));

        FillBarrierListWidget (Data.m_BarriersBeforeOnlySignal, ui->BeforeBarrierListWidget);
        FillBarrierListWidget (Data.m_BarriersBeforeSignalAndWait, ui->BeforeBarrierWaitListWidget);
        FillBarrierListWidget (Data.m_BarriersBehindOnlySignal, ui->BehindBarrierListWidget);
        FillBarrierListWidget (Data.m_BarriersBehindSignalAndWait, ui->BehindBarrierWaitListWidget);
        FillBarrierListWidget (Data.m_BarriersLoopOutBeforeOnlySignal, ui->BeforeBarrierLoopOutListWidget);
        FillBarrierListWidget (Data.m_BarriersLoopOutBeforeSignalAndWait, ui->BeforeBarrierLoopOutWaitListWidget);
        FillBarrierListWidget (Data.m_BarriersLoopOutBehindOnlySignal, ui->BehindBarrierLoopOutListWidget);
        FillBarrierListWidget (Data.m_BarriersLoopOutBehindSignalAndWait, ui->BehindBarrierLoopOutWaitListWidget);

        ui->CurrentLineEdit->setText(QString().number(Data.m_CycleTime));
        ui->MaximalLineEdit->setText(QString().number(Data.m_CycleTimeMax));
        ui->MinimalLineEdit->setText(QString().number(Data.m_CycleTimeMin));

        // Internals:
        if (Data.m_Type == 2)  {  // Only with external processes
            ui->ProcessTabWidget->setTabEnabled(4, true);
            QString Com;
            switch (Data.m_SocketOrPipe) {
            case 0:
                Com = QString("Pipe: ");
                break;
            case 1:
                Com = QString("Socket: ");
                break;
            default:
                Com = QString("unknown: ");
                break;
            }
            Com.append(QString().setNum((uint64_t)Data.m_hPipe, 16));

            switch (Data.m_MachineType) {
            case 0:
                Com.append(QString(" (32Bit Windows)"));
                break;
            case 1:
                Com.append(QString(" (64Bit Windows)"));
                break;
            case 2:
                Com.append(QString(" (32Bit Linux)"));
                break;
            case 3:
                Com.append(QString(" (64Bit Linux)"));
                break;
            default:
                Com.append(QString(" (unknown)"));
                break;
            }
            Com.append(QString("   protocol version: "));
            Com.append(QString().setNum(Data.m_ExternProcessComunicationProtocolVersion));
            ui->CommunicationLabel->setText(Com);

            ui->IsStartedInsideDebuggerCheckBox->setChecked(Data.m_IsStartedInDebugger);
            ui->UseDebugInterfaceForWriteCheckBox->setChecked(Data.m_UseDebugInterfaceForWrite);
            ui->UseDebugInterfaceForReadCheckBox->setChecked(Data.m_UseDebugInterfaceForRead);

            ui->DllNameLineEdit->setText(Data.m_DllName);
            ui->InsideExeNameLineEdit->setText(Data.m_InsideExecutableName);
            ui->NumberInsideExeLineEdit->setText(QString().setNum(Data.m_NumberInsideExecutable, 10));
            ui->ExeBaseAddressLineEdit->setText(QString().setNum(Data.m_ExecutableBaseAddress, 16));
            ui->DllBaseAddressLineEdit->setText(QString().setNum(Data.m_DllBaseAddress, 16));

            ui->WaitOnCheckBox->setChecked(Data.m_wait_on);
            ui->WorkingFlagCheckBox->setChecked(Data.m_WorkingFlag);
            ui->SchedWaitForUnlockingCheckBox->setChecked(Data.m_SchedWaitForUnlocking);
            ui->WaitForEndOfCycleFlagCheckBox->setChecked(Data.m_WaitForEndOfCycleFlag);
            ui->LockedFlagCheckBox->setChecked(Data.m_LockedFlag);
            ui->LockedByThreadIdLineEdit->setText(QString().setNum(Data.m_LockedByThreadId, 16));
            ui->SrcFileNameLineEdit->setText(Data.m_SrcFileName);
            ui->SrcLineNrLineEdit->setText(QString().setNum(Data.m_SrcLineNr, 10));
        } else {
            ui->ProcessTabWidget->setTabEnabled(4, false);
        }
    }
}

static int CheckFileExist(const char *par_Filename)
{
    char Filename[1024];
    SearchAndReplaceEnvironmentStrings(par_Filename, Filename, (int)sizeof(Filename));
    if (access(Filename, 0) == 0) {
        return 1;
    } else {
        return 0;
    }
}

void ConfigureProcessDialog::StoreProcessInfos (QString &par_ProcessName, struct DialogData &par_Data)
{
    if (!par_ProcessName.isEmpty()) {
        //struct DialogData Data = GetProcessInfos (par_ProcessName);
        if (par_Data.m_Changed) {
            if (par_Data.m_IsRunning) {
                // Direct use if process is running
                switch (par_Data.m_State) {
                case PROCESS_AKTIV:
                    break;
                case PROCESS_SLEEP:
                    break;
                case PROCESS_NONSLEEP:
                    break;
                }
                // TODO
            }

            write_extprocinfos_to_ini (GetMainFileDescriptor(),
                                       QStringToConstChar(par_ProcessName),
                                       par_Data.m_Priority,
                                       par_Data.m_TimeSteps,
                                       par_Data.m_Delay,
                                       par_Data.m_Timeout,
                                       par_Data.m_RangeControlBeforeActiveFlag + (par_Data.m_RangeControlBeforeDeactiveAtStartup << 1),
                                       par_Data.m_RangeControlBehindActiveFlag + (par_Data.m_RangeControlBehindDeactiveAtStartup << 1),
                                       par_Data.m_RangeControlStopSchedFlag,
                                       par_Data.m_RangeControlOutput,
                                       par_Data.m_RangeErrorCounterFlag,
                                       par_Data.m_RangeErrorCounter,
                                       par_Data.m_RangeControlVarFlag,
                                       par_Data.m_RangeControl,
                                       par_Data.m_RangeControlPhysFlag,
                                       par_Data.m_RangeControlLimitValuesFlag,
                                       par_Data.m_BBPrefix,
                                       par_Data.m_SchedulerName,
                                       par_Data.m_BarriersBeforeOnlySignal,
                                       par_Data.m_BarriersBeforeSignalAndWait,
                                       par_Data.m_BarriersBehindOnlySignal,
                                       par_Data.m_BarriersBehindSignalAndWait,
                                       par_Data.m_BarriersLoopOutBeforeOnlySignal,
                                       par_Data.m_BarriersLoopOutBeforeSignalAndWait,
                                       par_Data.m_BarriersLoopOutBehindOnlySignal,
                                       par_Data.m_BarriersLoopOutBehindSignalAndWait);
            SetBeforeProcessEquationFileName (QStringToConstChar(par_ProcessName), par_Data.m_EquationBefore);
            SetBehindProcessEquationFileName (QStringToConstChar(par_ProcessName), par_Data.m_EquationBehind);
            SetSVLFileLoadedBeforeInitProcessFileName (QStringToConstChar(par_ProcessName), par_Data.m_SvlFile, 1);
            int Pid = get_pid_by_name(QStringToConstChar(par_ProcessName));
            bool LoadA2L = false;
            if (Pid > 0) {
                // process is running
                char OldA2LFile[512+64];
                int OldUpdateA2LFileFlags;
                if (GetA2LFileAssociatedProcessFileName(QStringToConstChar(par_ProcessName), OldA2LFile, sizeof(OldA2LFile), &OldUpdateA2LFileFlags) > 0) {
#ifdef _WIN32
                    if (strcmpi(OldA2LFile, par_Data.m_A2LFile) ||
#else
                    if (strcmp(OldA2LFile, par_Data.m_A2LFile) ||
#endif
                        (OldUpdateA2LFileFlags != par_Data.m_UpdateA2LFileFlags)) {
                        // has changed -> load or reload
                        int LinkNo = A2LGetLinkToExternProcess(QStringToConstChar(par_ProcessName));
                        if (LinkNo >= 0) {
                            // has a loaded A2L file -> unload this
                            A2LCloseLink(LinkNo, QStringToConstChar(par_ProcessName));
                        }
                        LoadA2L = true;
                    }
                } else {
                    LoadA2L = true;
                }
            }
            SetA2LFileAssociatedToProcessFileName(QStringToConstChar(par_ProcessName), par_Data.m_A2LFile,
                                                  par_Data.m_UpdateA2LFileFlags, 1);
            if (LoadA2L) {
                if (CheckFileExist(par_Data.m_A2LFile)) {
                    if (A2LSetupLinkToExternProcess(par_Data.m_A2LFile, QStringToConstChar(par_ProcessName),
                                                    par_Data.m_UpdateA2LFileFlags)) {
                        ThrowError(1, "cannot load A2L file %s", par_Data.m_A2LFile);
                    }
                }
            }
        }
    }
}

void ConfigureProcessDialog::DeleteProcessInfos (QString par_ProcessName)
{
    SetBeforeProcessEquationFileName (QStringToConstChar(par_ProcessName), nullptr);
    SetBehindProcessEquationFileName (QStringToConstChar(par_ProcessName), nullptr);
    SetSVLFileLoadedBeforeInitProcessFileName (QStringToConstChar(par_ProcessName), nullptr, 1);
    IniFileDataBaseWriteString(SCHED_INI_SECTION, QStringToConstChar(par_ProcessName), nullptr, GetMainFileDescriptor());
    RemoveProcessOrSchedulerFromAllBarrierIniConfig (QStringToConstChar(par_ProcessName), 1);
}


bool ConfigureProcessDialog::ContainsProcessName (QString par_ProcessName)
{
    QMap<QString, struct ConfigureProcessDialog::DialogData>::iterator i;

    for (i = m_ProcessInfos.begin(); i != m_ProcessInfos.end(); ++i) {
        QString ProcessName(i.key());
        if (ProcessName.endsWith(NOT_RUNNING_PROCESS_POSTFIX)) {
            ProcessName.truncate(ProcessName.lastIndexOf(NOT_RUNNING_PROCESS_POSTFIX));
        }
        if (!ProcessName.compare (par_ProcessName, Qt::CaseInsensitive)) return true;
    }
    return false;
}

void ConfigureProcessDialog::on_AddProcessPushButton_clicked()
{
    AddNewProcessSchedulerBarrier Dlg (QString ("process"));
    if (Dlg.exec() == QDialog::Accepted) {
        QString NewProcess = Dlg.GetNew();
        if (!NewProcess.isEmpty()) {
            if (!ContainsProcessName (NewProcess)) {
                struct DialogData New;
                MEMSET (&New, 0, sizeof (New));
                New.m_Changed = 1;  // Mark process immediately as changed otherwise it will not stored
                m_ProcessInfos.insert (NewProcess, New);
                UpdateProcessListView();
            } else {
                ThrowError (1, "process name \"%s\" already used", QStringToConstChar(NewProcess));
            }
        }
    }
}

void ConfigureProcessDialog::on_EditBarriersPushButton_clicked()
{
    ConfigureBarrierDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        InsertAllSchedulerAndBarrier();
    }
}


int ConfigureProcessDialog::CheckChanges (QString &SelectedProcess)
{
    if (!SelectedProcess.isEmpty() &&
        m_ProcessInfos.contains (SelectedProcess)) {   // Process still have to exist
        struct DialogData Prev = GetProcessInfos (SelectedProcess);
        struct DialogData Old = m_ProcessInfos.value (SelectedProcess);
        if (memcmp (&Old, &Prev, sizeof (Prev))) {
            // There have been changed something
            Prev.m_Changed = 1;
            m_ProcessInfos.insert (SelectedProcess, Prev);  // overwrite
            return 1;
        }
    }
    return 0;
}

void ConfigureProcessDialog::on_ProceessListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (previous != nullptr) {
        // Store the data of the previos selcted processes
        QString PreviousSelectedProcess = previous->data(Qt::DisplayRole).toString();
        if (PreviousSelectedProcess.endsWith(NOT_RUNNING_PROCESS_POSTFIX)) {
            PreviousSelectedProcess.truncate(PreviousSelectedProcess.lastIndexOf(NOT_RUNNING_PROCESS_POSTFIX));
        }
        if (!PreviousSelectedProcess.isEmpty()) {
            CheckChanges (PreviousSelectedProcess);
        }
    }
    if (current != nullptr) {
        QString NewSelectedProcess = current->data(Qt::DisplayRole).toString();
        if (NewSelectedProcess.endsWith(NOT_RUNNING_PROCESS_POSTFIX)) {
            NewSelectedProcess.truncate(NewSelectedProcess.lastIndexOf(NOT_RUNNING_PROCESS_POSTFIX));
        }
        if (!NewSelectedProcess.isEmpty()) {
            SetProcessInfos (NewSelectedProcess);
        }
    }
}


void ConfigureProcessDialog::SaveAll()
{
    QMap<QString, struct ConfigureProcessDialog::DialogData>::iterator i;
    QMap<QString, struct ConfigureProcessDialog::DialogData>::iterator j;

    // First delete all prozesse which has been deleted
    for (j = m_ProcessInfosSave.begin(); j != m_ProcessInfosSave.end(); ++j) {
        int Found = 0;
        QString ProcessNameJ(j.key());
        for (i = m_ProcessInfos.begin(); i != m_ProcessInfos.end(); ++i) {
            QString ProcessNameI(i.key());
            if (!ProcessNameI.compare(ProcessNameJ)) {
                Found = 1;
                break;
            }
        }
        if (!Found) {
            DeleteProcessInfos (j.key());
        }
    }
    for (i = m_ProcessInfos.begin(); i != m_ProcessInfos.end(); ++i) {
        QString ProcessName(i.key());
        if (ProcessName.endsWith(NOT_RUNNING_PROCESS_POSTFIX)) {
            ProcessName.truncate(ProcessName.lastIndexOf(NOT_RUNNING_PROCESS_POSTFIX));
        }
        StoreProcessInfos(ProcessName, i.value());
    }
}

void ConfigureProcessDialog::accept()
{
    QString SelectedProcess;

    QListWidgetItem *SelectedItem = ui->ProceessListWidget->currentItem();
    if (SelectedItem != nullptr) {
        QString SelectedProcess = SelectedItem->data(Qt::DisplayRole).toString();
        if (!SelectedProcess.isEmpty()) {
            CheckChanges (SelectedProcess);
        }
    }
    SaveAll();
    QDialog::accept();
}

void ConfigureProcessDialog::on_DeletePushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->ProceessListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString ProcessName = Item->data(Qt::DisplayRole).toString();
        m_ProcessInfos.remove(ProcessName);
    }
    UpdateProcessListView ();
}

void ConfigureProcessDialog::on_EquationBeforePushButton_clicked()
{
    QString EquationBeforeFile = ui->EquationBeforeLineEdit->text();
    EquationBeforeFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                                       EquationBeforeFile,
                                                       QString("TRI files (*.TRI)"));
    if (!EquationBeforeFile.isEmpty()) {
        ui->EquationBeforeLineEdit->setText(EquationBeforeFile);
    }
}

void ConfigureProcessDialog::on_EquationBehindPushButton_clicked()
{
    QString EquationBehindFile = ui->EquationBehindLineEdit->text();
    EquationBehindFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                                       EquationBehindFile,
                                                       QString("TRI files (*.TRI)"));
    if (!EquationBehindFile.isEmpty()) {
        ui->EquationBehindLineEdit->setText(EquationBehindFile);
    }
}

void ConfigureProcessDialog::on_SVLFilePushButton_clicked()
{
    QString SVLFile = ui->SVLLineEdit->text();
    SVLFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                                       SVLFile,
                                                       QString("SVL files (*.SVL)"));
    if (!SVLFile.isEmpty()) {
        ui->SVLLineEdit->setText(SVLFile);
    }
}

void ConfigureProcessDialog::on_A2LPushButton_clicked()
{
    QString A2LFile = ui->A2LLineEdit->text();
    A2LFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                                       A2LFile,
                                                       QString("A2L files (*.A2L)"));
    if (!A2LFile.isEmpty()) {
        ui->A2LLineEdit->setText(A2LFile);
    }
}

void ConfigureProcessDialog::on_EditSchedulersPushButton_clicked()
{
    ConfigureSchedulersDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        InsertAllSchedulerAndBarrier();
    }
}


void ConfigureProcessDialog::on_AddBeforeBarrierPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBeforeBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BeforeBarrierListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BeforeBarrierListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureProcessDialog::on_DeleteBeforeBarrierPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BeforeBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureProcessDialog::on_AddBeforeBarrierWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBeforeBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BeforeBarrierWaitListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BeforeBarrierWaitListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureProcessDialog::on_DeleteBeforeBarrierWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BeforeBarrierWaitListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureProcessDialog::on_AddBarrierBehindPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBehindBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BehindBarrierListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BehindBarrierListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureProcessDialog::on_DeleteBehindBarrierPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BehindBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureProcessDialog::on_AddBehindBarrierWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBehindBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BehindBarrierWaitListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BehindBarrierWaitListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureProcessDialog::on_DeleteBehindBarrierWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BehindBarrierWaitListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}




void ConfigureProcessDialog::on_AddBeforeBarrierLoopOutWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBeforeBarrierLoopOutListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BeforeBarrierLoopOutWaitListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BeforeBarrierLoopOutWaitListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureProcessDialog::on_DeleteBeforeBarrierLoopOutWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BeforeBarrierLoopOutWaitListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureProcessDialog::on_AddBeforeBarrierLoopOutPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBeforeBarrierLoopOutListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BeforeBarrierLoopOutListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BeforeBarrierLoopOutListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureProcessDialog::on_DeleteBeforeBarrierLoopOutPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BeforeBarrierLoopOutListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureProcessDialog::on_AddBarrierLoopOutBehindPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBehindBarrierLoopOutListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BehindBarrierLoopOutListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BehindBarrierLoopOutListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureProcessDialog::on_DeleteBehindBarrierLoopOutPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BehindBarrierLoopOutListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureProcessDialog::on_AddBehindBarrierLoopOutWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBehindBarrierLoopOutListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BehindBarrierLoopOutWaitListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BehindBarrierLoopOutWaitListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureProcessDialog::on_DeleteBehindBarrierLoopOutWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BehindBarrierLoopOutWaitListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureProcessDialog::on_RangeControlBeforeProceesCheckBox_stateChanged(int arg1)
{
    ui->RangeControlBeforeDefaultCheckBox->setEnabled(arg1);
}


void ConfigureProcessDialog::on_RangeControlBehindProceesCheckBox_stateChanged(int arg1)
{
    ui->RangeControlBehindDefaultCheckBox->setEnabled(arg1);
}

void ConfigureProcessDialog::on_ResetPushButton_clicked()
{
    QListWidgetItem *SelectedItem = ui->ProceessListWidget->currentItem();
    if (SelectedItem != nullptr) {
        QString SelectedProcess = SelectedItem->data(Qt::DisplayRole).toString();
        if (!SelectedProcess.isEmpty() &&
            m_ProcessInfos.contains (SelectedProcess)) {   // Process must be still exist
            struct DialogData Old = m_ProcessInfos.value (SelectedProcess);
            rm_ResetProcessMinMaxRuntimeMeasurement(Old.m_Pid);
            struct DialogData New = ReadProcessInfos (SelectedProcess);
            m_ProcessInfos.insert (SelectedProcess, New);  // overwrite
            ui->CurrentLineEdit->setText(QString().number(New.m_CycleTime));
            ui->MaximalLineEdit->setText(QString().number(New.m_CycleTimeMax));
            ui->MinimalLineEdit->setText(QString().number(New.m_CycleTimeMin));
        }
    }
}

void ConfigureProcessDialog::on_ReloadPushButton_clicked()
{
    QListWidgetItem *SelectedItem = ui->ProceessListWidget->currentItem();
    if (SelectedItem != nullptr) {
        QString SelectedProcess = SelectedItem->data(Qt::DisplayRole).toString();
        if (!SelectedProcess.isEmpty() &&
            m_ProcessInfos.contains (SelectedProcess)) {   // Process must be still exist
            struct DialogData New = ReadProcessInfos (SelectedProcess);
            m_ProcessInfos.insert (SelectedProcess, New);  // overwrite
            ui->CurrentLineEdit->setText(QString().number(New.m_CycleTime));
            ui->MaximalLineEdit->setText(QString().number(New.m_CycleTimeMax));
            ui->MinimalLineEdit->setText(QString().number(New.m_CycleTimeMin));
        }
    }
}

void ConfigureProcessDialog::on_ShowConfigOfNotRunningProcessesCheckBox_stateChanged(int arg1)
{
    ReadAllProcessesInfos(arg1 > 0);
}

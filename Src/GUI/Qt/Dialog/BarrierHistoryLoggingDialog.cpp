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


#include "BarrierHistoryLoggingDialog.h"
#include "ui_BarrierHistoryLoggingDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "SchedBarrier.h"
#include "MyMemory.h"
}
BarrierHistoryLoggingDialog::BarrierHistoryLoggingDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::BarrierHistoryLoggingDialog)
{
    ui->setupUi(this);
    InitHistory();
    InitCurrentState();
    UpdateBarrierStates();
}

void BarrierHistoryLoggingDialog::UpdateBarrierStates ()
{
    int State;
    unsigned int SchedulerMask;
    unsigned int ProcessMask;
    int Switch;
    unsigned int SchedulerFlags_0;
    unsigned int ProcessFlags_0;
    unsigned int SchedulerFlags_1;
    unsigned int ProcessFlags_1;
    int ProcesseOrSchedulerCount;
    int WaitOnCounter;
    unsigned int AddNewProcessMasksBefore;
    unsigned int AddNewProcessMasksBehind;
    unsigned int AddNewSchedulerMasks;
    unsigned int RemoveProcessMasks;
    unsigned int RemoveSchedulerMasks;
    unsigned int BarriersLoggingCounter;

    for (int Column = 0; Column < ui->CurrentStateTableWidget->columnCount(); Column++) {
        if (GetBarierInfos (Column,
                            &State, &SchedulerMask, &ProcessMask, &Switch,
                            &SchedulerFlags_0, &ProcessFlags_0,
                            &SchedulerFlags_1, &ProcessFlags_1,
                            &ProcesseOrSchedulerCount,
                            &WaitOnCounter,
                            &AddNewProcessMasksBefore,
                            &AddNewProcessMasksBehind,
                            &AddNewSchedulerMasks,
                            &RemoveProcessMasks,
                            &RemoveSchedulerMasks,
                            &BarriersLoggingCounter) == 0) {
            QTableWidgetItem *Item;
            Item = new QTableWidgetItem(QString ().number(State));
            ui->CurrentStateTableWidget->setItem (0, Column, Item);
            Item = new QTableWidgetItem(QString ().number(SchedulerMask, 16));
            ui->CurrentStateTableWidget->setItem (1, Column, Item);
            Item = new QTableWidgetItem(QString ().number(ProcessMask, 16));
            ui->CurrentStateTableWidget->setItem (2, Column, Item);
            Item = new QTableWidgetItem(QString ().number(Switch));
            ui->CurrentStateTableWidget->setItem (3, Column, Item);
            Item = new QTableWidgetItem(QString ().number(SchedulerFlags_0, 16));
            ui->CurrentStateTableWidget->setItem (4, Column, Item);
            Item = new QTableWidgetItem(QString ().number(ProcessFlags_0, 16));
            ui->CurrentStateTableWidget->setItem (5, Column, Item);
            Item = new QTableWidgetItem(QString ().number(SchedulerFlags_1, 16));
            ui->CurrentStateTableWidget->setItem (6, Column, Item);
            Item = new QTableWidgetItem(QString ().number(ProcessFlags_1, 16));
            ui->CurrentStateTableWidget->setItem (7, Column, Item);
            Item = new QTableWidgetItem(QString ().number(ProcesseOrSchedulerCount));
            ui->CurrentStateTableWidget->setItem (8, Column, Item);
            Item = new QTableWidgetItem(QString ().number(WaitOnCounter));
            ui->CurrentStateTableWidget->setItem (9, Column, Item);
            Item = new QTableWidgetItem(QString ().number(AddNewProcessMasksBefore, 16));
            ui->CurrentStateTableWidget->setItem (10, Column, Item);
            Item = new QTableWidgetItem(QString ().number(AddNewProcessMasksBehind, 16));
            ui->CurrentStateTableWidget->setItem (11, Column, Item);
            Item = new QTableWidgetItem(QString ().number(AddNewSchedulerMasks, 16));
            ui->CurrentStateTableWidget->setItem (12, Column, Item);
            Item = new QTableWidgetItem(QString ().number(RemoveProcessMasks, 16));
            ui->CurrentStateTableWidget->setItem (13, Column, Item);
            Item = new QTableWidgetItem(QString ().number(RemoveSchedulerMasks, 16));
            ui->CurrentStateTableWidget->setItem (14, Column, Item);
            Item = new QTableWidgetItem(QString ().number(BarriersLoggingCounter));
            ui->CurrentStateTableWidget->setItem (15, Column, Item);
        }

        for (int Row = 16; Row < ui->CurrentStateTableWidget->rowCount(); Row++) {
            QString BarrierName = ui->CurrentStateTableWidget->horizontalHeaderItem(Column)->data(Qt::DisplayRole).toString();
            QString ProcessOrSchedulerName = ui->CurrentStateTableWidget->verticalHeaderItem(Row)->data(Qt::DisplayRole).toString();
            QTableWidgetItem *Item;
            switch (GetBarrierState (QStringToConstChar(BarrierName), QStringToConstChar(ProcessOrSchedulerName))) {
            case 0:
                Item = new QTableWidgetItem(QString ("not waiting"));
                ui->CurrentStateTableWidget->setItem (Row, Column, Item);
                break;
            case 1:
                Item = new QTableWidgetItem(QString ("waiting"));
                ui->CurrentStateTableWidget->setItem (Row, Column, Item);
                break;
            default:
                Item = new QTableWidgetItem(QString ("-"));
                ui->CurrentStateTableWidget->setItem (Row, Column, Item);
                break;
            }
        }
    }
}

void BarrierHistoryLoggingDialog::InitCurrentState()
{
    char *BarrierNameString = GetAllBarrierString();
    QStringList BarrierList = CharToQString(BarrierNameString).split (";");
    FreeAllBarrierString (BarrierNameString);
    ui->CurrentStateTableWidget->setColumnCount(BarrierList.count());
    ui->CurrentStateTableWidget->setHorizontalHeaderLabels (BarrierList);

    QStringList SchedulerProcessList;
    SchedulerProcessList << "State" << "SchedulerMask" << "ProcessMask" << "Switch"
                         << "SchedulerFlags_0" << "ProcessFlags_0"
                         << "SchedulerFlags_1" << "ProcessFlags_1"
                         << "ProcesseOrSchedulerCount"
                         << "WaitOnCounter"
                         << "AddNewProcessMasksBefore"
                         << "AddNewProcessMasksBehind"
                         << "AddNewSchedulerMasks"
                         << "RemoveProcessMasks"
                         << "RemoveSchedulerMasks"
                         << "BarriersLoggingCounter";
    char *SchedulerNameString = GetNameOfAllSchedulers();
    SchedulerProcessList.append(CharToQString(SchedulerNameString).split (";"));
    my_free (SchedulerNameString);
    char *ProcessesNameString = GetAllProcessNamesSemicolonSeparated ();
    SchedulerProcessList.append(CharToQString(ProcessesNameString).split (";"));
    my_free (ProcessesNameString);

    ui->CurrentStateTableWidget->setRowCount(SchedulerProcessList.count());
    ui->CurrentStateTableWidget->setVerticalHeaderLabels (SchedulerProcessList);
}

void BarrierHistoryLoggingDialog::InitHistory()
{
    QStringList LabelList;
    LabelList << "Action"
              << "Barrier"
              << "Process/Scheduler name"
              << "Line"
              << "SchedulerMask"
              << "ProcessMask"
              << "Switch"
              << "Ib_0_SchedulerFlags"
              << "Ib_0_ProcessFlags"
              << "Ib_1_SchedulerFlags"
              << "Ib_1_ProcessFlags"

              << "ProcesseOrSchedulerCount"
              << "AddNewProcessMasksBefore"
              << "AddNewProcessMasksBehind"
              << "AddNewSchedulerMasks"
              << "RemoveProcessMasks"
              << "RemoveSchedulerMasks"

              << "Counter";

    ui->HistoryTableWidget->setColumnCount(LabelList.size());
    ui->HistoryTableWidget->setRowCount(GetBarrierLoggingSize ());
    ui->HistoryTableWidget->setHorizontalHeaderLabels (LabelList);

    BARRIER_LOGGING_ACTION Action;
    char BarrierName[MAX_PATH];
    BARRIER_LOGGING_TYPE ProcessOrSched;
    char ProcOrSchedName[MAX_PATH];
    int LineNr;

    // Snapshot
    unsigned int SchedulerMask;
    unsigned int ProcessMask;
    int Switch;  // 0 oder 1
    unsigned int Ib_0_SchedulerFlags;
    unsigned int Ib_0_ProcessFlags;
    unsigned int Ib_1_SchedulerFlags;
    unsigned int Ib_1_ProcessFlags;

    int ProcesseOrSchedulerCount;
    unsigned int AddNewProcessMasksBefore;
    unsigned int AddNewProcessMasksBehind;
    unsigned int AddNewSchedulerMasks;
    unsigned int RemoveProcessMasks;
    unsigned int RemoveSchedulerMasks;

    unsigned int Counter;

    int Row = 0;

    int Index = -1;
    while ((Index = GetNextBarrierLoggingEntry (Index,
                                                BarrierName,
                                                &Action,
                                                &ProcessOrSched,
                                                ProcOrSchedName,
                                                &LineNr,

                                                // Snapshot
                                                &SchedulerMask,
                                                &ProcessMask,
                                                &Switch,
                                                &Ib_0_SchedulerFlags,
                                                &Ib_0_ProcessFlags,
                                                &Ib_1_SchedulerFlags,
                                                &Ib_1_ProcessFlags,

                                                &ProcesseOrSchedulerCount,
                                                &AddNewProcessMasksBefore,
                                                &AddNewProcessMasksBehind,
                                                &AddNewSchedulerMasks,
                                                &RemoveProcessMasks,
                                                &RemoveSchedulerMasks,

                                                &Counter)) >= 0) {

        QString ActionString;
        switch (Action) {
        //default:
        case BARRIER_LOGGING_UNDEF_ACTION:
            ActionString = "undef";
            break;
        case BARRIER_LOGGING_EMERGENCY_CLEANUP_ACTION:
            ActionString = "emergency cleanup killed process";
            break;
        case BARRIER_LOGGING_ADD_BEFORE_PROCESS_ACTION:
            ActionString = "add before process";
            break;
        case BARRIER_LOGGING_REMOVE_BEFORE_PROCESS_ACTION:
            ActionString = "remove before process";
            break;
        case BARRIER_LOGGING_GOTO_SLEEP_BEFORE_PROCESS_ACTION:
            ActionString = "goto sleep before process";
            break;
        case BARRIER_LOGGING_WAKE_ALL_BEFORE_PROCESS_ACTION:
            ActionString = "wake all before process";
            break;
        case BARRIER_LOGGING_WALK_THROUGH_BEFORE_PROCESS_ACTION:
            ActionString = "walk through before process";
            break;

        case BARRIER_LOGGING_ADD_BEHIND_PROCESS_ACTION:
            ActionString = "add behind process";
            break;
        case BARRIER_LOGGING_REMOVE_BEHIND_PROCESS_ACTION:
            ActionString = "remove behind process";
            break;
        case BARRIER_LOGGING_GOTO_SLEEP_BEHIND_PROCESS_ACTION:
            ActionString = "goto sleep behind process";
            break;
        case BARRIER_LOGGING_WAKE_ALL_BEHIND_PROCESS_ACTION:
            ActionString = "wake all behind process";
            break;
        case BARRIER_LOGGING_WALK_THROUGH_BEHIND_PROCESS_ACTION:
            ActionString = "walk through behind process";
            break;

        case BARRIER_LOGGING_ADD_BEFORE_SCHEDULER_ACTION:
            ActionString = "add before scheduler";
            break;
        case BARRIER_LOGGING_REMOVE_BEFORE_SCHEDULER_ACTION:
            ActionString = "remove before scheduler";
            break;
        case BARRIER_LOGGING_GOTO_SLEEP_BEFORE_SCHEDULER_ACTION:
            ActionString = "goto sleep before scheduler";
            break;
        case BARRIER_LOGGING_WAKE_ALL_BEFORE_SCHEDULER_ACTION:
            ActionString = "wake all before scheduler";
            break;
        case BARRIER_LOGGING_WALK_THROUGH_BEFORE_SCHEDULER_ACTION:
            ActionString = "walk through before scheduler";
            break;

        case BARRIER_LOGGING_ADD_BEHIND_SCHEDULER_ACTION:
            ActionString = "add behind scheduler";
            break;
        case BARRIER_LOGGING_REMOVE_BEHIND_SCHEDULER_ACTION:
            ActionString = "remove behind scheduler";
            break;
        case BARRIER_LOGGING_GOTO_SLEEP_BEHIND_SCHEDULER_ACTION:
            ActionString = "goto sleep behind scheduler";
            break;
        case BARRIER_LOGGING_WAKE_ALL_BEHIND_SCHEDULER_ACTION:
            ActionString = "wake all behind scheduler";
            break;
        case BARRIER_LOGGING_WALK_THROUGH_BEHIND_SCHEDULER_ACTION:
            ActionString = "walk through behind scheduler";
            break;

        case BARRIER_LOGGING_ADD_BEFORE_LOOPOUT_ACTION:
            ActionString = "add before loopout";
            break;
        case BARRIER_LOGGING_REMOVE_BEFORE_LOOPOUT_ACTION:
            ActionString = "remove before loopout";
            break;
        case BARRIER_LOGGING_GOTO_SLEEP_BEFORE_LOOPOUT_ACTION:
            ActionString = "goto sleep before loopout";
            break;
        case BARRIER_LOGGING_WAKE_ALL_BEFORE_LOOPOUT_ACTION:
            ActionString = "wake all before loopout";
            break;
        case BARRIER_LOGGING_WALK_THROUGH_BEFORE_LOOPOUT_ACTION:
            ActionString = "walk through before loopout";
            break;

        case BARRIER_LOGGING_ADD_BEHIND_LOOPOUT_ACTION:
            ActionString = "add behind loopout";
            break;
        case BARRIER_LOGGING_REMOVE_BEHIND_LOOPOUT_ACTION:
            ActionString = "remove behind loopout";
            break;
        case BARRIER_LOGGING_GOTO_SLEEP_BEHIND_LOOPOUT_ACTION:
            ActionString = "goto sleep behind loopout";
            break;
        case BARRIER_LOGGING_WAKE_ALL_BEHIND_LOOPOUT_ACTION:
            ActionString = "wake all behind loopout";
            break;
        case BARRIER_LOGGING_WALK_THROUGH_BEHIND_LOOPOUT_ACTION:
            ActionString = "walk through behind loopout";
            break;
        }
        QTableWidgetItem *newItem = new QTableWidgetItem(ActionString);
        ui->HistoryTableWidget->setItem(Row, 0, newItem);
        newItem = new QTableWidgetItem(QString(BarrierName));
        ui->HistoryTableWidget->setItem(Row, 1, newItem);
        newItem = new QTableWidgetItem(QString(ProcOrSchedName));
        ui->HistoryTableWidget->setItem(Row, 2, newItem);
        newItem = new QTableWidgetItem(QString().number(LineNr));
        ui->HistoryTableWidget->setItem(Row, 3, newItem);

        newItem = new QTableWidgetItem(QString().number(SchedulerMask, 16));
        ui->HistoryTableWidget->setItem(Row, 4, newItem);
        newItem = new QTableWidgetItem(QString().number(ProcessMask, 16));
        ui->HistoryTableWidget->setItem(Row, 5, newItem);

        newItem = new QTableWidgetItem(QString().number(Switch));
        ui->HistoryTableWidget->setItem(Row, 6, newItem);

        newItem = new QTableWidgetItem(QString().number(Ib_0_SchedulerFlags, 16));
        ui->HistoryTableWidget->setItem(Row, 7, newItem);
        newItem = new QTableWidgetItem(QString().number(Ib_0_ProcessFlags, 16));
        ui->HistoryTableWidget->setItem(Row, 8, newItem);
        newItem = new QTableWidgetItem(QString().number(Ib_1_SchedulerFlags, 16));
        ui->HistoryTableWidget->setItem(Row, 9, newItem);
        newItem = new QTableWidgetItem(QString().number(Ib_1_ProcessFlags, 16));
        ui->HistoryTableWidget->setItem(Row, 10, newItem);

        newItem = new QTableWidgetItem(QString().number(ProcesseOrSchedulerCount));
        ui->HistoryTableWidget->setItem(Row, 11, newItem);

        newItem = new QTableWidgetItem(QString().number(AddNewProcessMasksBefore, 16));
        ui->HistoryTableWidget->setItem(Row, 12, newItem);
        newItem = new QTableWidgetItem(QString().number(AddNewProcessMasksBehind, 16));
        ui->HistoryTableWidget->setItem(Row, 13, newItem);
        newItem = new QTableWidgetItem(QString().number(AddNewSchedulerMasks, 16));
        ui->HistoryTableWidget->setItem(Row, 14, newItem);
        newItem = new QTableWidgetItem(QString().number(RemoveProcessMasks, 16));
        ui->HistoryTableWidget->setItem(Row, 15, newItem);
        newItem = new QTableWidgetItem(QString().number(RemoveSchedulerMasks, 16));
        ui->HistoryTableWidget->setItem(Row, 16, newItem);

        newItem = new QTableWidgetItem(QString().number(Counter));
        ui->HistoryTableWidget->setItem(Row, 17, newItem);

        Row++;
    }
}


BarrierHistoryLoggingDialog::~BarrierHistoryLoggingDialog()
{
    delete ui;
}

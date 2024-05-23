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


#include "LoadedDebugInfosDialog.h"
#include "ui_LoadedDebugInfosDialog.h"

extern "C" {
#include "DebugInfoDB.h"
}

LoadedDebugInfosDialog::LoadedDebugInfosDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::LoadedDebugInfosDialog)
{
    ui->setupUi(this);


    DEBUG_INFOS_ASSOCIATED_CONNECTION *DebugInfosAssociatedConnections;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *DebugInfosAssociatedProcesses;
    DEBUG_INFOS_DATA *DebugInfosDatas;

    GetInternalDebugInfoPointers (&DebugInfosAssociatedConnections,
                                  &DebugInfosAssociatedProcesses,
                                  &DebugInfosDatas);


    int x, Row;
    QStringList LabelList;
    LabelList << "Index" << "Used by unique id" << "LoadUnloadCallBackFuncsSchedThread" << "LoadUnloadCallBackFuncParUsers" << "Associated process index";
    ui->ConnectionsTableWidget->setColumnCount(LabelList.size());
    ui->ConnectionsTableWidget->setHorizontalHeaderLabels (LabelList);

    Row = 0;
    for (x = 0; x < DEBUG_INFOS_MAX_CONNECTIONS; x++) {
        if (DebugInfosAssociatedConnections[x].UsedFlag) {
            char Help[32];
            ui->ConnectionsTableWidget->insertRow(Row);
            QTableWidgetItem *Item;
            Item = new QTableWidgetItem(QString ().number(x));
            ui->ConnectionsTableWidget->setItem (Row, 0, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosAssociatedConnections[x].UsedByUniqueId));
            ui->ConnectionsTableWidget->setItem (Row, 1, Item);
            sprintf(Help, "0x%p", DebugInfosAssociatedConnections[x].LoadUnloadCallBackFuncsSchedThread);
            Item = new QTableWidgetItem(QString (Help));
            ui->ConnectionsTableWidget->setItem (Row, 2, Item);
            sprintf(Help, "0x%p", DebugInfosAssociatedConnections[x].LoadUnloadCallBackFuncParUsers);
            Item = new QTableWidgetItem(QString (Help));
            ui->ConnectionsTableWidget->setItem (Row, 3, Item);

            int64_t ProcessNumber = DebugInfosAssociatedConnections[x].AssociatedProcess - DebugInfosAssociatedProcesses;
            if ((ProcessNumber < 0) && (ProcessNumber >= DEBUG_INFOS_MAX_PROCESSES)) {
                ProcessNumber = -1;
            }
            Item = new QTableWidgetItem(QString ().number(ProcessNumber));
            ui->ConnectionsTableWidget->setItem (Row, 4, Item);
            Row++;
        }
    }
    LabelList.clear();
    LabelList << "Index" << "Attach counter" << "Process name" << "Pid" << "Base address" << "Executable name" << "is realtime" << "machine type" << "Associated debug infos index";
    ui->ProcessesTableWidget->setColumnCount(LabelList.size());
    ui->ProcessesTableWidget->setHorizontalHeaderLabels (LabelList);

    Row = 0;
    for (x = 0; x < DEBUG_INFOS_MAX_PROCESSES; x++) {
        if (DebugInfosAssociatedProcesses[x].UsedFlag) {
            ui->ProcessesTableWidget->insertRow(Row);
            QTableWidgetItem *Item;
            Item = new QTableWidgetItem(QString ().number(x));
            ui->ProcessesTableWidget->setItem (Row, 0, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosAssociatedProcesses[x].AttachCounter));
            ui->ProcessesTableWidget->setItem (Row, 1, Item);
            Item = new QTableWidgetItem(QString (DebugInfosAssociatedProcesses[x].ProcessName));
            ui->ProcessesTableWidget->setItem (Row, 2, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosAssociatedProcesses[x].Pid));
            ui->ProcessesTableWidget->setItem (Row, 3, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosAssociatedProcesses[x].BaseAddress, 16));
            ui->ProcessesTableWidget->setItem (Row, 4, Item);
            Item = new QTableWidgetItem(QString (DebugInfosAssociatedProcesses[x].ExeOrDllName));
            ui->ProcessesTableWidget->setItem (Row, 5, Item);

            Item = new QTableWidgetItem(QString ().number(DebugInfosAssociatedProcesses[x].IsRealtimeProcess));
            ui->ProcessesTableWidget->setItem (Row, 6, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosAssociatedProcesses[x].MachineType));
            ui->ProcessesTableWidget->setItem (Row, 7, Item);

            int64_t DebugInfosNumber = DebugInfosAssociatedProcesses[x].AssociatedDebugInfos - DebugInfosDatas;
            if ((DebugInfosNumber < 0) || (DebugInfosNumber >= DEBUG_INFOS_MAX_DEBUG_INFOS)) {
                DebugInfosNumber = -1;
            }
            Item = new QTableWidgetItem(QString ().number(DebugInfosNumber));
            ui->ProcessesTableWidget->setItem (Row, 8, Item);
            Row++;
        }
    }

    LabelList.clear();
    LabelList << "Index" << "Attach counter" << "Try to load flag" << "Loaded flag" << "Executable file name" << "Debug info file name" << "label list entry count" << "Absolute or relative address" << "Type";
    ui->DebugInfosTableWidget->setColumnCount(LabelList.size());
    ui->DebugInfosTableWidget->setHorizontalHeaderLabels (LabelList);

    Row = 0;
    for (x = 0; x < DEBUG_INFOS_MAX_PROCESSES; x++) {
        if (DebugInfosDatas[x].UsedFlag) {
            ui->DebugInfosTableWidget->insertRow(Row);
            QTableWidgetItem *Item;
            Item = new QTableWidgetItem(QString ().number(x));
            ui->DebugInfosTableWidget->setItem (Row, 0, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosDatas[x].AttachCounter));
            ui->DebugInfosTableWidget->setItem (Row, 1, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosDatas[x].TryToLoadFlag));
            ui->DebugInfosTableWidget->setItem (Row, 2, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosDatas[x].LoadedFlag));
            ui->DebugInfosTableWidget->setItem (Row, 3, Item);
            Item = new QTableWidgetItem(QString (DebugInfosDatas[x].ExecutableFileName));
            ui->DebugInfosTableWidget->setItem (Row, 4, Item);
            Item = new QTableWidgetItem(QString (DebugInfosDatas[x].DebugInfoFileName));
            ui->DebugInfosTableWidget->setItem (Row, 5, Item);
            Item = new QTableWidgetItem(QString ().number(DebugInfosDatas[x].label_list_entrys));
            ui->DebugInfosTableWidget->setItem (Row, 6, Item);
            switch (DebugInfosDatas[x].AbsoluteOrRelativeAddress) {
            default:
            case DEBUGINFO_ADDRESS_ABSOLUTE:
                Item = new QTableWidgetItem(QString ("absolute"));
                break;
            case DEBUGINFO_ADDRESS_RELATIVE:
                Item = new QTableWidgetItem(QString ("relative"));
                break;
            }
            ui->DebugInfosTableWidget->setItem (Row, 7, Item);
            switch (DebugInfosDatas[x].DebugInfoType) {
            case DEBUGINFOTYPE_VISUAL6:
                Item = new QTableWidgetItem(QString ("VisualC 6.0"));
                break;
            case DEBUGINFOTYPE_VISUALNET_2003:
                Item = new QTableWidgetItem(QString ("VisualC 2003"));
                break;
            case DEBUGINFOTYPE_VISUALNET_2008:
                Item = new QTableWidgetItem(QString ("VisualC 2008"));
                break;
            case DEBUGINFOTYPE_VISUALNET_2010:
                Item = new QTableWidgetItem(QString ("VisualC 2010"));
                break;
            case DEBUGINFOTYPE_VISUALNET_2012:
                Item = new QTableWidgetItem(QString ("VisualC 2012"));
                break;
            case DEBUGINFOTYPE_VISUALNET_2015:
                Item = new QTableWidgetItem(QString ("VisualC 2015"));
                break;
            case DEBUGINFOTYPE_VISUALNET_2017:
                Item = new QTableWidgetItem(QString ("VisualC 2017"));
                break;
            case DEBUGINFOTYPE_VISUALNET_2019:
                Item = new QTableWidgetItem(QString ("VisualC 2019"));
                break;
            case DEBUGINFOTYPE_DWARF:
                Item = new QTableWidgetItem(QString ("Dwarf (gcc)"));
                break;
            }
            ui->DebugInfosTableWidget->setItem (Row, 8, Item);
            Row++;
        }
    }
}

LoadedDebugInfosDialog::~LoadedDebugInfosDialog()
{
    delete ui;
}

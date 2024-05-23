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


#ifndef CONFIGUREPROCESSDIALOG_H
#define CONFIGUREPROCESSDIALOG_H

#include "Dialog.h"
#include <QMap>
#include <QListWidgetItem>
extern "C" {
#include "IniDataBase.h"
#include "Scheduler.h"
#include "SchedBarrier.h"
}

namespace Ui {
class ConfigureProcessDialog;
}

class ConfigureProcessDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ConfigureProcessDialog(QString par_SelectedProcess, QWidget *parent = nullptr);
    ~ConfigureProcessDialog();

public slots:
    void accept();

private slots:
    void on_AddProcessPushButton_clicked();

    void on_EditBarriersPushButton_clicked();

    void on_ProceessListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_DeletePushButton_clicked();

    void on_EquationBeforePushButton_clicked();

    void on_EquationBehindPushButton_clicked();

    void on_SVLFilePushButton_clicked();

    void on_EditSchedulersPushButton_clicked();

    void on_AddBeforeBarrierPushButton_clicked();

    void on_DeleteBeforeBarrierPushButton_clicked();

    void on_AddBeforeBarrierWaitPushButton_clicked();

    void on_DeleteBeforeBarrierWaitPushButton_clicked();

    void on_AddBarrierBehindPushButton_clicked();

    void on_DeleteBehindBarrierPushButton_clicked();

    void on_AddBehindBarrierWaitPushButton_clicked();

    void on_DeleteBehindBarrierWaitPushButton_clicked();

    void on_AddBeforeBarrierLoopOutWaitPushButton_clicked();

    void on_DeleteBeforeBarrierLoopOutWaitPushButton_clicked();

    void on_AddBeforeBarrierLoopOutPushButton_clicked();

    void on_DeleteBeforeBarrierLoopOutPushButton_clicked();

    void on_AddBarrierLoopOutBehindPushButton_clicked();

    void on_DeleteBehindBarrierLoopOutPushButton_clicked();

    void on_AddBehindBarrierLoopOutWaitPushButton_clicked();

    void on_DeleteBehindBarrierLoopOutWaitPushButton_clicked();

    void on_RangeControlBeforeProceesCheckBox_stateChanged(int arg1);


    void on_RangeControlBehindProceesCheckBox_stateChanged(int arg1);

    void on_ResetPushButton_clicked();

    void on_ReloadPushButton_clicked();

    void on_ShowConfigOfNotRunningProcessesCheckBox_stateChanged(int arg1);

    void on_A2LPushButton_clicked();

private:
    struct DialogData {
        int m_IsRunning;
        int m_Pid;
        int m_State;
        int m_Type;  // 0 -> internal process, 1 -> external process, 2 -> realtime process (remote master)
        int m_Priority;
        int m_TimeSteps;
        int m_Delay;
        int m_Timeout;
        uint64_t m_Mask;
        uint64_t m_CycleCount;

        char m_SchedulerName[MAX_SCHEDULER_NAME_LENGTH];
        char m_BarriersBeforeOnlySignal[INI_MAX_LINE_LENGTH];
        char m_BarriersBeforeSignalAndWait[INI_MAX_LINE_LENGTH];
        char m_BarriersBehindOnlySignal[INI_MAX_LINE_LENGTH];
        char m_BarriersBehindSignalAndWait[INI_MAX_LINE_LENGTH];

        char m_BarriersLoopOutBeforeOnlySignal[INI_MAX_LINE_LENGTH];
        char m_BarriersLoopOutBeforeSignalAndWait[INI_MAX_LINE_LENGTH];
        char m_BarriersLoopOutBehindOnlySignal[INI_MAX_LINE_LENGTH];
        char m_BarriersLoopOutBehindSignalAndWait[INI_MAX_LINE_LENGTH];

       int m_RangeControlBeforeActiveFlag;
       int m_RangeControlBehindActiveFlag;
       int m_RangeControlBeforeDeactiveAtStartup;
       int m_RangeControlBehindDeactiveAtStartup;
       int m_RangeControlStopSchedFlag;
       int m_RangeControlOutput;
       int m_RangeErrorCounterFlag;
       int m_RangeControlVarFlag;
       int m_RangeControlPhysFlag;
       int m_RangeControlLimitValuesFlag;

        // Time measurement only avaiable with activ remote master
        uint64_t m_CycleTime;
        uint64_t m_CycleTimeMax;
        uint64_t m_CycleTimeMin;

        char m_RangeErrorCounter[512];
        char m_RangeControl[512];
        char m_BBPrefix[512];
        char m_EquationBefore[512];
        char m_EquationBehind[512];
        char m_SvlFile[512];
        char m_A2LFile[512];

        int m_UpdateA2LFileFlags;

        // Debug Sheet
        HANDLE m_hPipe;

        int m_SocketOrPipe;

        int m_IsStartedInDebugger;  // Is the extern process started inside a debugger
        int m_MachineType;          // If 0 -> 32Bit Windows 1 -> 64Bit Windows, 2 -> 32Bit Linux, 3 -> 64Bit Linux

        int m_UseDebugInterfaceForWrite;
        int m_UseDebugInterfaceForRead;

        int m_wait_on;                     // Is set if wait for returning from a call inside the external process

        int m_WorkingFlag;   // Process are inside his cyclic/init/terminate/reference function
        int m_LockedFlag;    // Process cannot enter his cyclic/init/terminate/reference function he must wait
        int m_SchedWaitForUnlocking;  // Scheduler wait till he are allowed to enter cyclic/init/terminate/reference function
        int m_WaitForEndOfCycleFlag;  // Somebody is waiting that the the external process has finished it cyclic/init/terminate/reference function

        DWORD m_LockedByThreadId;
        char m_SrcFileName[MAX_PATH];
        int m_SrcLineNr;
        char m_DllName[MAX_PATH];     // If the external process lives inside a DLL this contains the DLL name otherwise this is a NULL pointer
        char m_InsideExecutableName[MAX_PATH];
        int m_NumberInsideExecutable;

        uint64_t m_ExecutableBaseAddress;
        uint64_t m_DllBaseAddress;

        int m_ExternProcessComunicationProtocolVersion;

        int m_Changed;
    };

    struct DialogData ReadProcessInfos(QString &SelectedProcess);
    struct DialogData GetProcessInfos(QString &SelectedProcess);
    void SetProcessInfos(QString &SelectedProcess);
    void StoreProcessInfos(QString &par_ProcessName, struct DialogData &par_Data);
    void DeleteProcessInfos (QString par_ProcessName);
    void ReadAllProcessesInfos(bool par_WithNonRunning);
    void InsertAllSchedulerAndBarrier();
    int CheckChanges (QString &SelectedProcess);
    void SaveAll();
    void UpdateProcessListView();
    bool ContainsProcessName (QString par_ProcessName);

    Ui::ConfigureProcessDialog *ui;

    QMap <QString, struct DialogData> m_ProcessInfos;
    QMap <QString, struct DialogData> m_ProcessInfosSave;
};

#endif // CONFIGUREPROCESSDIALOG_H

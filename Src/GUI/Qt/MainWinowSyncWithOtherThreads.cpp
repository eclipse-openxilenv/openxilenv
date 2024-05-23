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


#include "MainWinowSyncWithOtherThreads.h"
#include "MainWindow.h"
#include "StringHelpers.h"
#include "Platform.h"
extern "C" {
#include "Scheduler.h"
#include "MyMemory.h"
#include "ThrowError.h"
}

#define UNUSED(x) (void)(x)

#define my_GetCurrentThreadId()  (static_cast<int>(GetCurrentThreadId()))

MainWinowSyncWithOtherThreads::MainWinowSyncWithOtherThreads()
{
    Init();
}

void MainWinowSyncWithOtherThreads::Init()
{
    m_GUIThreadId = my_GetCurrentThreadId();
    InitializeCriticalSection (&m_CriticalSection);
    InitializeConditionVariable(&m_ConditionVariable);
    memset(&m_Processbars, 0, sizeof (m_Processbars));
}

void MainWinowSyncWithOtherThreads::SchedulerStateChanged(int par_State)
{
    emit SchedulerStateChangedSignal(my_GetCurrentThreadId(), par_State);
}

void MainWinowSyncWithOtherThreads::SchedulerStateChangedAck(int par_ThreadId)
{
    UNUSED(par_ThreadId);
}

int MainWinowSyncWithOtherThreads::OpenWindowByNameFromOtherThread(char *par_WindowName, bool par_PosFlag, int par_XPos, int par_YPos, bool par_SizeFlag, int par_Width, int par_Hight)
{
    EnterCriticalSection (&m_CriticalSection);
    check("OpenWindowByNameFromOtherThread");
    emit OpenWindowByNameSignal(my_GetCurrentThreadId(), par_WindowName, par_PosFlag, par_XPos, par_YPos, par_SizeFlag, par_Width, par_Hight);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::OpenWindowByNameFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::IsWindowOpenFromOtherThread(char *par_WindowName)
{
    EnterCriticalSection (&m_CriticalSection);
    check("IsWindowOpenFromOtherThread");
    emit IsWindowOpenSignal(my_GetCurrentThreadId(), par_WindowName);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::IsWindowOpenFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

void MainWinowSyncWithOtherThreads::SaveAllConfigToIniDataBaseFromOtherThread()
{
    EnterCriticalSection (&m_CriticalSection);
    check("SaveAllConfigToIniDataBaseFromOtherThread");
    emit SaveAllConfigToIniDataBaseSignal(my_GetCurrentThreadId());
    WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
}

void MainWinowSyncWithOtherThreads::SaveAllConfigToIniDataBaseFromOtherThreadAck(int par_ThreadId)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, 0);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::CloseWindowByNameFromOtherThread(char *par_WindowName)
{
    EnterCriticalSection (&m_CriticalSection);
    check("CloseWindowByNameFromOtherThread");
    emit CloseWindowByNameSignal(my_GetCurrentThreadId(), par_WindowName);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::CloseWindowByNameFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::SelectSheetFromOtherThread(char *par_SheetName)
{
    EnterCriticalSection (&m_CriticalSection);
    check("SelectSheetFromOtherThread");
    emit SelectSheetSignal(my_GetCurrentThreadId(), par_SheetName);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::SelectSheetFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::AddSheetFromOtherThread(char *par_SheetName)
{
    EnterCriticalSection (&m_CriticalSection);
    check("AddSheetFromOtherThread");
    emit AddSheetSignal(my_GetCurrentThreadId(), par_SheetName);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::AddSheetFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::DeleteSheetFromOtherThread(char *par_SheetName)
{
    EnterCriticalSection (&m_CriticalSection);
    check("DeleteSheetFromOtherThread");
    emit DeleteSheetSignal(my_GetCurrentThreadId(), par_SheetName);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::DeleteSheetFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::RenameSheetFromOtherThread(char *par_OldSheetName, char *par_NewSheetName)
{
    EnterCriticalSection (&m_CriticalSection);
    check("RenameSheetFromOtherThread");
    emit RenameSheetSignal(my_GetCurrentThreadId(), par_OldSheetName, par_NewSheetName);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::RenameSheetFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::ImportHotkeyFromOtherThread(char *par_Filename)
{
    EnterCriticalSection (&m_CriticalSection);
    check("ImportHotkeyFromOtherThread");
    QString Filename(par_Filename);
    emit ImportHotkeySignal(my_GetCurrentThreadId(), Filename);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::ImportHotkeyFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::ExportHotkeyFromOtherThread(char *par_Filename)
{
    EnterCriticalSection (&m_CriticalSection);
    check("ExportHotkeyFromOtherThread");
    QString Filename(par_Filename);
    emit ExportHotkeySignal(my_GetCurrentThreadId(), Filename);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::ExportHotkeyFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

void MainWinowSyncWithOtherThreads::SetNotFasterThanRealtimeState(int par_State)
{
    emit SetNotFasterThanRealtimeStateSignal( par_State);
}

void MainWinowSyncWithOtherThreads::SetSuppressDisplayNonExistValuesState(int par_State)
{
    emit SetSuppressDisplayNonExistValuesStateSignal(par_State);
}

void MainWinowSyncWithOtherThreads::DeactivateBreakpointFromOtherThread()
{
    emit DeactivateBreakpointSignal();
}

void MainWinowSyncWithOtherThreads::AddNewScriptErrorMessageFromOtherThread(int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message)
{
    EnterCriticalSection (&m_CriticalSection);
    check("AddNewScriptErrorMessageFromOtherThread");
    emit AddNewScriptErrorMessageSignal(my_GetCurrentThreadId(), par_Level, par_LineNr, par_Filename, par_Message);
    WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
}

void MainWinowSyncWithOtherThreads::AddNewScriptErrorMessageFromOtherThreadAck(int par_ThreadId)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, 0);
    LeaveCriticalSection (&m_CriticalSection);
}

#define SAME_THREAD_PROCESSBARS
int MainWinowSyncWithOtherThreads::OpenProgressBarFromOtherThread(const char *par_ProgressName)
{
    int Ret;

    EnterCriticalSection (&m_CriticalSection);

    Ret = AllocProcessbarId();

    if (Ret > 0) {
        QString loc_ProgressName(par_ProgressName);
        if (MainWindow::IsMainWindowOpen()) {
            if (!IsGuiThread()) {
                emit OpenProgressBarSignal(Ret, my_GetCurrentThreadId(), loc_ProgressName);
            }
#ifdef SAME_THREAD_PROCESSBARS
            else {
                MainWindow *mw = MainWindow::GetPtrToMainWindow();
                if (mw != nullptr) {
                    mw->OpenProgressBarSlot(Ret, my_GetCurrentThreadId(), par_ProgressName);
                } else {
                    FreeProcessbarId(Ret);
                    Ret = -1;
                }
            }
#endif
        } else {
            FreeProcessbarId(Ret);
            Ret = -1;
        }
    }
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::SetProgressBarFromOtherThread(int par_ProgressBarID, int par_Value)
{
    EnterCriticalSection (&m_CriticalSection);

    if (MainWindow::IsMainWindowOpen()) {
        bool GuiThread = (IsGuiThread() != 0);
        if (!GuiThread) {
            emit SetProgressBarSignal(my_GetCurrentThreadId(), par_ProgressBarID, par_Value, GuiThread);
        }
#ifdef SAME_THREAD_PROCESSBARS
        else {
            MainWindow *mw = MainWindow::GetPtrToMainWindow();
            if (mw != nullptr) {
                mw->SetProgressBarSlot(my_GetCurrentThreadId(), par_ProgressBarID, par_Value, true);
            }
        }
#endif
    }
    LeaveCriticalSection (&m_CriticalSection);
}

void MainWinowSyncWithOtherThreads::CloseProgressBarFromOtherThread(int par_ProgressBarID)
{
    EnterCriticalSection (&m_CriticalSection);

    FreeProcessbarId(par_ProgressBarID);

    if (MainWindow::IsMainWindowOpen()) {
        if (!IsGuiThread()) {
            emit CloseProgressBarSignal(my_GetCurrentThreadId(), par_ProgressBarID);
        }
#ifdef SAME_THREAD_PROCESSBARS
        else {
            MainWindow *mw = MainWindow::GetPtrToMainWindow();
            if (mw != nullptr) {
                mw->CloseProgressBarSlot(my_GetCurrentThreadId(), par_ProgressBarID);
            }
        }
#endif
    }
    LeaveCriticalSection (&m_CriticalSection);
}


// Script commands: CREATE_DLG/ADD_DLG_ITEM/SHOW_DLG
int MainWinowSyncWithOtherThreads::ScriptCreateDialogFromOtherThread(char *par_Headline)
{
    EnterCriticalSection (&m_CriticalSection);
    check("ScriptCreateDialogFromOtherThread");
    QString loc_Headline =  CharToQString(par_Headline);
    emit ScriptCreateDialogSignal(my_GetCurrentThreadId(), loc_Headline);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::ScriptCreateDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::ScriptAddDialogItemFromOtherThread(char *par_Element, char *par_Label)
{
    EnterCriticalSection (&m_CriticalSection);
    check("ScriptCreateDialogFromOtherThread");
    QString loc_Element = CharToQString(par_Element);
    QString loc_Label = CharToQString(par_Label);
    emit ScriptAddDialogItemSignal(my_GetCurrentThreadId(), loc_Element, loc_Label);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::ScriptAddDialogItemFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::ScriptShowDialogFromOtherThread()
{
    EnterCriticalSection (&m_CriticalSection);
    check("ScriptCreateDialogFromOtherThread");
    emit ScriptShowDialogSignal(my_GetCurrentThreadId());
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::ScriptShowDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::ScriptDestroyDialogFromOtherThread()
{
    EnterCriticalSection (&m_CriticalSection);
    check("ScriptCreateDialogFromOtherThread");
    emit ScriptDestroyDialogSignal(my_GetCurrentThreadId());
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::ScriptDestroyDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

int MainWinowSyncWithOtherThreads::IsScriptDialogClosedFromOtherThread()
{
    EnterCriticalSection (&m_CriticalSection);
    check("ScriptCreateDialogFromOtherThread");
    emit IsScriptDialogClosedSignal(my_GetCurrentThreadId());
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::IsScriptDialogClosedFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

void MainWinowSyncWithOtherThreads::StopOscilloscopeFromOtherThread(void *par_OscilloscopeWidget, uint64_t par_Time)
{
    emit StopOscilloscopeSignal(par_OscilloscopeWidget, par_Time);
}

int MainWinowSyncWithOtherThreads::RemoteProcedureCallRequestFromOtherThread(void *par_Connection, const void *par_In, void *ret_Out)
{
    EnterCriticalSection (&m_CriticalSection);
    check("RemoteProcedureCallRequestFromOtherThread");
    emit RemoteProcedureCallRequestSignal(my_GetCurrentThreadId(), par_Connection, par_In, ret_Out);
    int Ret = WaitCurrentThread(INFINITE);
    LeaveCriticalSection (&m_CriticalSection);
    return Ret;
}

void MainWinowSyncWithOtherThreads::RemoteProcedureCallRequestFromOtherThreaddAck(int par_ThreadId, int par_RetrunValue)
{
    EnterCriticalSection (&m_CriticalSection);
    WakeThread(par_ThreadId, par_RetrunValue);
    LeaveCriticalSection (&m_CriticalSection);
}

void MainWinowSyncWithOtherThreads::check(const char *par_Name)
{
    int CurrentThreadId = my_GetCurrentThreadId();
    for (int x = 0; x < 32; x++) {
        if ((m_Wait[x].ThreadId == CurrentThreadId) &&
            (m_Wait[x].Wait == true)) {
            FILE *fh = fopen("c:\\tmp\\check.txt", "at");
            if (fh != nullptr) {
                fprintf (fh, "check(\"%s\")\n", par_Name);
                fclose(fh);
            }
        }
    }
}

int MainWinowSyncWithOtherThreads::WaitCurrentThread(unsigned long par_Timeout)
{
    if (m_GUIThreadId != my_GetCurrentThreadId()) {
        for (int x = 0; x < 32; x++) {
            if (m_Wait[x].ThreadId <= 0) {
                m_Wait[x].ThreadId = my_GetCurrentThreadId();
                m_Wait[x].Wait = true;
                unsigned long TimeoutCounter = par_Timeout;
                while (m_Wait[x].Wait) {  // Condition variables are subject to spurious wakeups
                    SleepConditionVariableCS (&m_ConditionVariable, &m_CriticalSection, 100);
                    TimeoutCounter--;
                    if (TimeoutCounter <= 0) {
                        m_Wait[x].Wait = false;
                        m_Wait[x].ThreadId = -1;
                        return -1;
                    }
                }
                int Ret = m_Wait[x].ReturnValue;
                m_Wait[x].ThreadId = -1;
                return Ret;
            }
        }
        return -1;
    } else {
        return  m_ReturnValueSameThread;
    }
}

void MainWinowSyncWithOtherThreads::WakeThread(int par_ThreadId, int par_ReturnValue)
{
    if (m_GUIThreadId != par_ThreadId) {
        for (int x = 0; x < 32; x++) {
            if (m_Wait[x].ThreadId == par_ThreadId) {
                m_Wait[x].ReturnValue = par_ReturnValue;
                m_Wait[x].Wait = false;
                WakeAllConditionVariable(&m_ConditionVariable);
            }
        }
    } else {
        m_ReturnValueSameThread = par_ReturnValue;
    }

}

int MainWinowSyncWithOtherThreads::AllocProcessbarId()
{
    int x;
    int Ret = -1;
    for (x = 0; x < PROCESS_BAR_MAX_ENTRYS; x++) {
        if (m_Processbars.Elem[x].Id == 0)  {
            m_Processbars.EmementCount++;
            if (m_Processbars.EmementCount > 0x7FFF) m_Processbars.EmementCount  = 1;
            Ret = m_Processbars.Elem[x].Id = (m_Processbars.EmementCount << 8) + x;
            break;
        }
    }
    return Ret;
}

void MainWinowSyncWithOtherThreads::FreeProcessbarId(int par_Id)
{
    int x;
    for (x = 0; x < PROCESS_BAR_MAX_ENTRYS; x++) {
        if (m_Processbars.Elem[x].Id == par_Id)  {
            m_Processbars.Elem[x].Id = 0;
        }
    }
}

MainWinowSyncWithOtherThreads MainWinowSyncWithOtherThreadsInstance;

extern "C" {
int OpenWindowByNameFromOtherThread (char *par_WindowName, int par_PosFlag, int par_XPos, int par_YPos, int par_SizeFlag, int par_Width, int par_Hight)
{
    return MainWinowSyncWithOtherThreadsInstance.OpenWindowByNameFromOtherThread (par_WindowName, par_PosFlag, par_XPos, par_YPos, par_SizeFlag, par_Width, par_Hight);
}

void OpenWindowByNameFromOtherThreadAck (int par_ThreadId,int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.OpenWindowByNameFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

int IsWindowOpenFromOtherThread (char *par_WindowName)
{
    return MainWinowSyncWithOtherThreadsInstance.IsWindowOpenFromOtherThread(par_WindowName);
}

void IsWindowOpenFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
    return MainWinowSyncWithOtherThreadsInstance.IsWindowOpenFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

void SaveAllConfigToIniDataBaseFromOtherThread (void)
{
    MainWinowSyncWithOtherThreadsInstance.SaveAllConfigToIniDataBaseFromOtherThread();
}

void SaveAllConfigToIniDataBaseFromOtherThreadAck (int par_ThreadId)
{
    MainWinowSyncWithOtherThreadsInstance.SaveAllConfigToIniDataBaseFromOtherThreadAck(par_ThreadId);
}

int CloseWindowByNameFromOtherThread (char *par_WindowName)
{
    return MainWinowSyncWithOtherThreadsInstance.CloseWindowByNameFromOtherThread(par_WindowName);
}

void CloseWindowByNameFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.CloseWindowByNameFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

int SelectSheetFromOtherThread (char *par_SheetName)
{
    return MainWinowSyncWithOtherThreadsInstance.SelectSheetFromOtherThread(par_SheetName);
}

void SelectSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.SelectSheetFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

int AddSheetFromOtherThread (char *par_SheetName)
{
    return MainWinowSyncWithOtherThreadsInstance.AddSheetFromOtherThread(par_SheetName);
}

void AddSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.AddSheetFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

int DeleteSheetFromOtherThread (char *par_SheetName)
{
    return MainWinowSyncWithOtherThreadsInstance.DeleteSheetFromOtherThread(par_SheetName);
}

void DeleteSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.DeleteSheetFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

int RenameSheetFromOtherThread (char *par_OldSheetName, char *par_NewSheetName)
{
    return MainWinowSyncWithOtherThreadsInstance.RenameSheetFromOtherThread (par_OldSheetName, par_NewSheetName);
}

void RenameSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.RenameSheetFromOtherThreadAck (par_ThreadId, par_RetrunValue);
}

void AddNewScriptErrorMessageFromOtherThread (int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message)
{
    MainWinowSyncWithOtherThreadsInstance.AddNewScriptErrorMessageFromOtherThread (par_Level, par_LineNr, par_Filename, par_Message);
}

void AddNewScriptErrorMessageFromOtherThreadAck (int par_ThreadId)
{
    MainWinowSyncWithOtherThreadsInstance.AddNewScriptErrorMessageFromOtherThreadAck (par_ThreadId);
}

void SchedulerStateChanged(int par_State)
{
    MainWinowSyncWithOtherThreadsInstance.SchedulerStateChanged(par_State);
}

void SchedulerStateChangedAck(int par_ThreadId)
{
    MainWinowSyncWithOtherThreadsInstance.SchedulerStateChangedAck(par_ThreadId);
}

int ImportHotkeyFromOtherThread(char *par_Filename)
{
    return MainWinowSyncWithOtherThreadsInstance.ImportHotkeyFromOtherThread(par_Filename);
}

void ImportHotkeyFromOtherThreadAck(int par_ThreadId, int par_ReturnValue)
{
    MainWinowSyncWithOtherThreadsInstance.ImportHotkeyFromOtherThreadAck(par_ThreadId, par_ReturnValue);
}

int ExportHotkeyFromOtherThread(char *par_Filename)
{
    return MainWinowSyncWithOtherThreadsInstance.ExportHotkeyFromOtherThread(par_Filename);
}

void ExportHotkeyFromOtherThreadAck(int par_ThreadId, int par_ReturnValue)
{
    MainWinowSyncWithOtherThreadsInstance.ExportHotkeyFromOtherThreadAck(par_ThreadId, par_ReturnValue);
}

void SetNotFasterThanRealtimeState(int par_State)
{
    MainWinowSyncWithOtherThreadsInstance.SetNotFasterThanRealtimeState(par_State);
}

void SetSuppressDisplayNonExistValuesState(int par_State)
{
    MainWinowSyncWithOtherThreadsInstance.SetSuppressDisplayNonExistValuesState(par_State);
}

void DeactivateBreakpointFromOtherThread()
{
    MainWinowSyncWithOtherThreadsInstance.DeactivateBreakpointFromOtherThread();
}

static FILE *debug_fh;


int OpenProgressBarFromOtherThread(const char *par_ProgressName)
{
    int ret;
    ret = MainWinowSyncWithOtherThreadsInstance.OpenProgressBarFromOtherThread(par_ProgressName);
    if (debug_fh != nullptr) {
        fprintf (debug_fh,"return %i\n", ret);
    }
    return ret;
}


void SetProgressBarFromOtherThread(int par_ProgressBarID, int par_Value)
{
    MainWinowSyncWithOtherThreadsInstance.SetProgressBarFromOtherThread(par_ProgressBarID, par_Value);
}

void CloseProgressBarFromOtherThread(int par_ProgressBarID)
{
    MainWinowSyncWithOtherThreadsInstance.CloseProgressBarFromOtherThread(par_ProgressBarID);
}


// Script commands: CREATE_DLG/ADD_DLG_ITEM/SHOW_DLG
int ScriptCreateDialogFromOtherThread(char *par_Headline)
{
    return MainWinowSyncWithOtherThreadsInstance.ScriptCreateDialogFromOtherThread(par_Headline);
}

void ScriptCreateDialogFromOtherThreadAck(int par_ThreadId, int par_ReturnValue)
{
    MainWinowSyncWithOtherThreadsInstance.ScriptCreateDialogFromOtherThreadAck(par_ThreadId, par_ReturnValue);
}

int ScriptAddDialogItemFromOtherThread(char *par_Element, char *par_Label)
{
    return MainWinowSyncWithOtherThreadsInstance.ScriptAddDialogItemFromOtherThread(par_Element, par_Label);
}

void ScriptAddDialogItemFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.ScriptAddDialogItemFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

int ScriptShowDialogFromOtherThread()
{
    return MainWinowSyncWithOtherThreadsInstance.ScriptShowDialogFromOtherThread();
}

void ScriptShowDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.ScriptShowDialogFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

int ScriptDestroyDialogFromOtherThread()
{
    return MainWinowSyncWithOtherThreadsInstance.ScriptDestroyDialogFromOtherThread();
}

void ScriptDestroyDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.ScriptDestroyDialogFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

int IsScriptDialogClosedFromOtherThread()
{
    return MainWinowSyncWithOtherThreadsInstance.IsScriptDialogClosedFromOtherThread();
}

void IsScriptDialogClosedFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.IsScriptDialogClosedFromOtherThreadAck(par_ThreadId, par_RetrunValue);
}

void StopOscilloscopeFromOtherThread(void* par_OscilloscopeWidget, uint64_t par_Time)
{
    MainWinowSyncWithOtherThreadsInstance.StopOscilloscopeFromOtherThread(par_OscilloscopeWidget, par_Time);
}
}

int RemoteProcedureCallRequestFromOtherThread(void *par_Connection, const void *par_In, void *ret_Out)
{
    return MainWinowSyncWithOtherThreadsInstance.RemoteProcedureCallRequestFromOtherThread(par_Connection, par_In, ret_Out);
}

void RemoteProcedureCallRequestFromOtherThreaddAck(int par_ThreadId, int par_RetrunValue)
{
    MainWinowSyncWithOtherThreadsInstance.RemoteProcedureCallRequestFromOtherThreaddAck(par_ThreadId, par_RetrunValue);
}

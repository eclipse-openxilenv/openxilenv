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


#ifndef MAINWINOWSYNCWITHOTHERTHREADS_H
#define MAINWINOWSYNCWITHOTHERTHREADS_H

#ifdef __cplusplus
#include <QObject>

#include "Platform.h"

// TODO: the class SignalsFromOtherThread from MainWindow.h should be move to this file!

class MainWinowSyncWithOtherThreads : public QObject
{
    Q_OBJECT

public:
    MainWinowSyncWithOtherThreads();

    void Init(void);

    void SchedulerStateChanged(int par_State);
    void SchedulerStateChangedAck(int par_ThreadId);

    // OpenWindowByName (WindowName)             ->
    int OpenWindowByNameFromOtherThread (char *par_WindowName, bool par_PosFlag, int par_XPos, int par_YPos, bool par_SizeFlag, int par_Width, int par_Hight);
    void OpenWindowByNameFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
    // get_mdi_window_hwnd_by_name (WindowName)  ->
    int IsWindowOpenFromOtherThread (char *par_WindowName);
    void IsWindowOpenFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);
    // save_all_infos2ini                        ->
    void SaveAllConfigToIniDataBaseFromOtherThread (void);
    void SaveAllConfigToIniDataBaseFromOtherThreadAck (int par_RetrunValue);
    // CloseWindowByName (char *WindowName)      ->
    int CloseWindowByNameFromOtherThread (char *par_WindowName);
    void CloseWindowByNameFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);

    int SelectSheetFromOtherThread (char *par_SheetName);
    void SelectSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
    int AddSheetFromOtherThread (char *par_SheetName);
    void AddSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
    int DeleteSheetFromOtherThread (char *par_SheetName);
    void DeleteSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
    int RenameSheetFromOtherThread (char *par_OldSheetName, char *par_NewSheetName);
    void RenameSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);

    int ImportHotkeyFromOtherThread (char *par_Filename);
    void ImportHotkeyFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
    int ExportHotkeyFromOtherThread (char *par_Filename);
    void ExportHotkeyFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);

    void SetNotFasterThanRealtimeState(int par_State);
    void SetSuppressDisplayNonExistValuesState(int par_State);
    void DeactivateBreakpointFromOtherThread();

    void AddNewScriptErrorMessageFromOtherThread (int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message);
    void AddNewScriptErrorMessageFromOtherThreadAck (int par_ThreadId);

    int OpenProgressBarFromOtherThread(const char *par_ProgressName);
    void SetProgressBarFromOtherThread(int par_ProgressBarID, int par_Value);
    void CloseProgressBarFromOtherThread(int par_ProgressBarID);

    // Script commands: CREATE_DLG/ADD_DLG_ITEM/SHOW_DLG
    int ScriptCreateDialogFromOtherThread(char* par_Headline);
    void ScriptCreateDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);
    int ScriptAddDialogItemFromOtherThread(char* par_Element, char* par_Label);
    void ScriptAddDialogItemFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);
    int ScriptShowDialogFromOtherThread(void);
    void ScriptShowDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);
    int ScriptDestroyDialogFromOtherThread(void);
    void ScriptDestroyDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);
    int IsScriptDialogClosedFromOtherThread(void);
    void IsScriptDialogClosedFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);

    void StopOscilloscopeFromOtherThread(void *par_OscilloscopeWidget, uint64_t par_Time);

    // Socket basierte remote API
    int RemoteProcedureCallRequestFromOtherThread(void *par_Connection, const void *par_In, void *ret_Out);
    void RemoteProcedureCallRequestFromOtherThreaddAck(int par_ThreadId, int par_RetrunValue);

signals:
    void SchedulerStateChangedSignal(int par_ThreadId, int par_State);

    void OpenWindowByNameSignal(int par_ThreadId, char *par_WindowName, bool par_PosFlag, int par_XPos, int par_YPos, bool par_SizeFlag, int par_Width, int par_Height);
    void IsWindowOpenSignal (int par_ThreadId, char *par_WindowName);
    void SaveAllConfigToIniDataBaseSignal (int par_ThreadId);
    void CloseWindowByNameSignal (int par_ThreadId, char *par_WindowName);

    void SelectSheetSignal (int par_ThreadId, char *par_SheetName);
    void AddSheetSignal (int par_ThreadId, char *par_SheetName);
    void DeleteSheetSignal (int par_ThreadId, char *par_SheetName);
    void RenameSheetSignal (int par_ThreadId, char *par_OldSheetName, char *par_NewSheetName);

    void ImportHotkeySignal(int par_ThreadId, QString Filename);
    void ExportHotkeySignal(int par_ThreadId, QString Filename);

    void SetNotFasterThanRealtimeStateSignal(int par_State);
    void SetSuppressDisplayNonExistValuesStateSignal(int par_State);
    void DeactivateBreakpointSignal();

    void AddNewScriptErrorMessageSignal (int par_ThreadId, int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message);

    void OpenProgressBarSignal(int par_Id, int par_ThreadId, QString par_ProgressName);
    void SetProgressBarSignal(int par_ThreadId, int par_ProgressBarID, int par_Value, bool par_CalledFromGuiThread);
    void CloseProgressBarSignal(int par_ThreadId, int par_ProgressBarID);

    // Script commands: CREATE_DLG/ADD_DLG_ITEM/SHOW_DLG
    void ScriptCreateDialogSignal(int par_ThreadId, QString par_Headline);
    void ScriptAddDialogItemSignal(int par_ThreadId, QString par_Element, QString par_Label);
    void ScriptShowDialogSignal(int par_ThreadId);
    void ScriptDestroyDialogSignal(int par_ThreadId);
    void IsScriptDialogClosedSignal(int par_ThreadId);

    void StopOscilloscopeSignal(void *par_OscilloscopeWidget, uint64_t pat_Time);

    // Socket based remote API
    void RemoteProcedureCallRequestSignal(int par_ThreadId, void *par_Connection, const void *par_In, void *ret_Out);

private:
    void check(const char *par_Name);

    CRITICAL_SECTION m_CriticalSection;
    CONDITION_VARIABLE m_ConditionVariable;

    struct {
        int ThreadId;
        bool Wait;
        int ReturnValue;
    } m_Wait[32];   // max 32 threads

    int WaitCurrentThread(unsigned long par_Timeout);
    void WakeThread(int par_ThreadId, int par_ReturnValue);

    int m_GUIThreadId;
    int m_ReturnValueSameThread;

#define PROCESS_BAR_MAX_ENTRYS 16
    struct {
        int EmementCount;
        struct ELEM {
            int Id;
        };
        struct ELEM Elem[PROCESS_BAR_MAX_ENTRYS];
    } m_Processbars;

    int AllocProcessbarId();
    void FreeProcessbarId(int par_Id);
};

extern MainWinowSyncWithOtherThreads MainWinowSyncWithOtherThreadsInstance;

extern "C" {
#endif

void SchedulerStateChanged(int par_State);
void SchedulerStateChangedAck(int par_ThreadId);

int OpenWindowByNameFromOtherThread (char *par_WindowName, int par_PosFlag, int par_XPos, int par_YPos, int par_SizeFlag, int par_Width, int par_Hight);
void OpenWindowByNameFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
int IsWindowOpenFromOtherThread (char *par_WindowName);
void IsWindowOpenFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
void SaveAllConfigToIniDataBaseFromOtherThread (void);
void SaveAllConfigToIniDataBaseFromOtherThreadAck (int par_ThreadId);
int CloseWindowByNameFromOtherThread (char *par_WindowName);
void CloseWindowByNameFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);

int SelectSheetFromOtherThread (char *par_SheetName);
void SelectSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
int AddSheetFromOtherThread (char *par_SheetName);
void AddSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
int DeleteSheetFromOtherThread (char *par_SheetName);
void DeleteSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);
int RenameSheetFromOtherThread (char *par_OldSheetName, char *par_NewSheetName);
void RenameSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue);

int ImportHotkeyFromOtherThread(char *par_Filename);
void ImportHotkeyFromOtherThreadAck(int par_ThreadId, int par_ReturnValue);
int ExportHotkeyFromOtherThread(char *par_Filename);
void ExportHotkeyFromOtherThreadAck(int par_ThreadId, int par_ReturnValue);

void SetNotFasterThanRealtimeState(int par_State);
void SetSuppressDisplayNonExistValuesState(int par_State);
void DeactivateBreakpointFromOtherThread(void);

void AddNewScriptErrorMessageFromOtherThread (int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message);
void AddNewScriptErrorMessageFromOtherThreadAck (int par_ThreadId);

int OpenProgressBarFromOtherThread(const char *par_ProgressName);
void SetProgressBarFromOtherThread(int par_ProgressBarID, int par_Value);
void CloseProgressBarFromOtherThread(int par_ProgressBarID);

// Script commands: CREATE_DLG/ADD_DLG_ITEM/SHOW_DLG
int ScriptCreateDialogFromOtherThread(char* par_Headline);
void ScriptCreateDialogFromOtherThreadAck(int par_ThreadId, int par_ReturnValue);
int ScriptAddDialogItemFromOtherThread(char* par_Element, char* par_Label);
void ScriptAddDialogItemFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);
int ScriptShowDialogFromOtherThread(void);
void ScriptShowDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);
int ScriptDestroyDialogFromOtherThread(void);
void ScriptDestroyDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);
int IsScriptDialogClosedFromOtherThread(void);
void IsScriptDialogClosedFromOtherThreadAck(int par_ThreadId, int par_RetrunValue);

void StopOscilloscopeFromOtherThread(void* par_OscilloscopeWidget, uint64_t par_Time);

// Socket based remote API
int RemoteProcedureCallRequestFromOtherThread(void *par_Connection, const void *par_In, void *ret_Out);
void RemoteProcedureCallRequestFromOtherThreaddAck(int par_ThreadId, int par_RetrunValue);

#ifdef __cplusplus
}
#endif

#endif // MAINWINOWSYNCWITHOTHERTHREADS_H

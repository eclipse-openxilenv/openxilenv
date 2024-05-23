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
#include "ScriptErrorDialog.h"
#include "MainWindow.h"
#include "Platform.h"
extern "C" {
#include "Scheduler.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "RpcSocketServer.h"
}

extern "C" {
int OpenWindowByNameFromOtherThread (char *par_WindowName, int par_PosFlag, int par_XPos, int par_YPos, int par_SizeFlag, int par_Width, int par_Hight)
{
    return 0;
}

void OpenWindowByNameFromOtherThreadAck (int par_ThreadId,int par_RetrunValue)
{
}

int IsWindowOpenFromOtherThread (char *par_WindowName)
{
    return 0;
}

void IsWindowOpenFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
}

void SaveAllConfigToIniDataBaseFromOtherThread (void)
{
}

void SaveAllConfigToIniDataBaseFromOtherThreadAck (int par_ThreadId)
{
}

int CloseWindowByNameFromOtherThread (char *par_WindowName)
{
    return 0;
}

void CloseWindowByNameFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
}

int SelectSheetFromOtherThread (char *par_SheetName)
{
    return 0;
}

void SelectSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
}

int AddSheetFromOtherThread (char *par_SheetName)
{
    return 0;
}

void AddSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
}

int DeleteSheetFromOtherThread (char *par_SheetName)
{
    return 0;
}

void DeleteSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
}

int RenameSheetFromOtherThread (char *par_OldSheetName, char *par_NewSheetName)
{
    return 0;
}

void RenameSheetFromOtherThreadAck (int par_ThreadId, int par_RetrunValue)
{
}

void AddNewScriptErrorMessageFromOtherThread (int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message)
{
    if ((par_Filename != NULL) && (par_Message != NULL)) {
        const char *LevelString;
        switch (par_Level) {
        case 0:
            LevelString = "Info:";
            break;
        case 1:
            LevelString = "Warning:";
            break;
        case 2:
            LevelString = "Error:";
            break;
        default:
            LevelString = "Unknown:";
            break;
        }
        printf("Script> %s (%i) \"%s\"\n", par_Filename, par_LineNr, par_Message);
    }
}

void AddNewScriptErrorMessageFromOtherThreadAck (int par_ThreadId)
{
}

void SchedulerStateChanged(int par_State)
{
}

void SchedulerStateChangedAck(int par_ThreadId)
{
}

int ImportHotkeyFromOtherThread(char *par_Filename)
{
    return 0;
}

void ImportHotkeyFromOtherThreadAck(int par_ThreadId, int par_ReturnValue)
{
}

int ExportHotkeyFromOtherThread(char *par_Filename)
{
    return 0;
}

void ExportHotkeyFromOtherThreadAck(int par_ThreadId, int par_ReturnValue)
{
}

void SetNotFasterThanRealtimeState(int par_State)
{
}

void SetSuppressDisplayNonExistValuesState(int par_State)
{
}

void DeactivateBreakpointFromOtherThread()
{
}

int OpenProgressBarFromOtherThread(const char *par_ProgressName)
{
    return 123;
}


void SetProgressBarFromOtherThread(int par_ProgressBarID, int par_Value)
{
}

void CloseProgressBarFromOtherThread(int par_ProgressBarID)
{
}


// Script-Commands: CREATE_DLG/ADD_DLG_ITEM/SHOW_DLG
int ScriptCreateDialogFromOtherThread(char *par_Headline)
{
    return 0;
}

void ScriptCreateDialogFromOtherThreadAck(int par_ThreadId, int par_ReturnValue)
{
}

int ScriptAddDialogItemFromOtherThread(char *par_Element, char *par_Label)
{
    return 0;
}

void ScriptAddDialogItemFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
}

int ScriptShowDialogFromOtherThread()
{
    return 0;
}

void ScriptShowDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
}

int ScriptDestroyDialogFromOtherThread()
{
    return 0;
}

void ScriptDestroyDialogFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
}

int IsScriptDialogClosedFromOtherThread()
{
    return 0;
}

void IsScriptDialogClosedFromOtherThreadAck(int par_ThreadId, int par_RetrunValue)
{
}

void StopOscilloscopeFromOtherThread(void* par_OscilloscopeWidget, uint64_t par_Time)
{
}


}

int RemoteProcedureCallRequestFromOtherThread(void *par_Connection, const void *par_In, void *ret_Out)
{
    int Ret = RemoteProcedureDecodeMessage((RPC_CONNECTION*)par_Connection, (RPC_API_BASE_MESSAGE*)par_In, (RPC_API_BASE_MESSAGE_ACK*)ret_Out);
    return Ret;
}

void RemoteProcedureCallRequestFromOtherThreaddAck(int par_ThreadId, int par_RetrunValue)
{
}

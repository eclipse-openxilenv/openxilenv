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


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>

#include "Config.h"

#include "ThrowError.h"
#include "Files.h"
#include "ImExportDskFile.h"
#include "MainWinowSyncWithOtherThreads.h"
#include "UtilsWindow.h"

#include "RpcSocketServer.h"
#include "RpcFuncGui.h"

#define UNUSED(x) (void)(x)

static int RPCFunc_LoadDesktop(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_LOAD_DESKTOP_MESSAGE *In = (RPC_API_LOAD_DESKTOP_MESSAGE*)par_DataIn;
    RPC_API_LOAD_DESKTOP_MESSAGE_ACK *Out = (RPC_API_LOAD_DESKTOP_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_LOAD_DESKTOP_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOAD_DESKTOP_MESSAGE_ACK);

    Out->Header.ReturnValue = script_command_load_desktop_file ((char*)In + In->OffsetFile);

    return Out->Header.StructSize;
}

static int RPCFunc_SaveDesktop(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SAVE_DESKTOP_MESSAGE *In = (RPC_API_SAVE_DESKTOP_MESSAGE*)par_DataIn;
    RPC_API_SAVE_DESKTOP_MESSAGE_ACK *Out = (RPC_API_SAVE_DESKTOP_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SAVE_DESKTOP_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SAVE_DESKTOP_MESSAGE_ACK);

    Out->Header.ReturnValue = script_command_save_desktop_file ((char*)In + In->OffsetFile);

    return Out->Header.StructSize;
}

static int RPCFunc_ClearDesktop(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    //RPC_API_CLEAR_DESKTOP_MESSAGE *In = (RPC_API_CLEAR_DESKTOP_MESSAGE*)par_DataIn;
    RPC_API_CLEAR_DESKTOP_MESSAGE_ACK *Out = (RPC_API_CLEAR_DESKTOP_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_CLEAR_DESKTOP_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CLEAR_DESKTOP_MESSAGE_ACK);

    Out->Header.ReturnValue = 0;
    script_command_clear_desktop ();

    return Out->Header.StructSize;
}


static int RPCFunc_CreateDialog(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_CREATE_DIALOG_MESSAGE *In = (RPC_API_CREATE_DIALOG_MESSAGE*)par_DataIn;
    RPC_API_CREATE_DIALOG_MESSAGE_ACK *Out = (RPC_API_CREATE_DIALOG_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_CREATE_DIALOG_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CREATE_DIALOG_MESSAGE_ACK);

    Out->Header.ReturnValue = ScriptCreateDialogFromOtherThread((char*)In + In->OffsetDialogName);
    Out->Header.ReturnValue = 0; // will return always 0
    return Out->Header.StructSize;
}

static int RPCFunc_AddDialogItem(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_ADD_DIALOG_ITEM_MESSAGE *In = (RPC_API_ADD_DIALOG_ITEM_MESSAGE*)par_DataIn;
    RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK *Out = (RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK);

    Out->Header.ReturnValue = ScriptAddDialogItemFromOtherThread ((char*)In + In->OffsetDescription, (char*)In + In->OffsetVariName);
    Out->Header.ReturnValue = 0; // will return always 0
    return Out->Header.StructSize;
}

static int RPCFunc_ShowDialog(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    //RPC_API_SHOW_DIALOG_MESSAGE *In = (RPC_API_SHOW_DIALOG_MESSAGE*)par_DataIn;
    RPC_API_SHOW_DIALOG_MESSAGE_ACK *Out = (RPC_API_SHOW_DIALOG_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SHOW_DIALOG_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SHOW_DIALOG_MESSAGE_ACK);

    ScriptShowDialogFromOtherThread();
    Out->Header.ReturnValue = 0; // will return always 0
    return Out->Header.StructSize;
}

static int RPCFunc_IsDialogClosed(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    UNUSED(par_DataIn);
    //RPC_API_IS_DIALOG_CLOSED_MESSAGE *In = (RPC_API_IS_DIALOG_CLOSED_MESSAGE*)par_DataIn;
    RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK *Out = (RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK);

    Out->Header.ReturnValue = IsScriptDialogClosedFromOtherThread();
    return Out->Header.StructSize;
}


static int RPCFunc_SelectSheet(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SELECT_SHEET_MESSAGE *In = (RPC_API_SELECT_SHEET_MESSAGE*)par_DataIn;
    RPC_API_SELECT_SHEET_MESSAGE_ACK *Out = (RPC_API_SELECT_SHEET_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_SELECT_SHEET_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SELECT_SHEET_MESSAGE_ACK);

    Out->Header.ReturnValue = SelectSheetFromOtherThread ((char*)In + In->OffsetSheetName);
    return Out->Header.StructSize;
}

static int RPCFunc_AddSheet(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_ADD_SHEET_MESSAGE *In = (RPC_API_ADD_SHEET_MESSAGE*)par_DataIn;
    RPC_API_ADD_SHEET_MESSAGE_ACK *Out = (RPC_API_ADD_SHEET_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_ADD_SHEET_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ADD_SHEET_MESSAGE_ACK);

    Out->Header.ReturnValue = AddSheetFromOtherThread ((char*)In + In->OffsetSheetName);
    return Out->Header.StructSize;
}

static int RPCFunc_DeleteSheet(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_DELETE_SHEET_MESSAGE *In = (RPC_API_DELETE_SHEET_MESSAGE*)par_DataIn;
    RPC_API_DELETE_SHEET_MESSAGE_ACK *Out = (RPC_API_DELETE_SHEET_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_DELETE_SHEET_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DELETE_SHEET_MESSAGE_ACK);

    Out->Header.ReturnValue = DeleteSheetFromOtherThread ((char*)In + In->OffsetSheetName);
    return Out->Header.StructSize;
}

static int RPCFunc_RenameSheet(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_RENAME_SHEET_MESSAGE *In = (RPC_API_RENAME_SHEET_MESSAGE*)par_DataIn;
    RPC_API_RENAME_SHEET_MESSAGE_ACK *Out = (RPC_API_RENAME_SHEET_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_RENAME_SHEET_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_RENAME_SHEET_MESSAGE_ACK);

    Out->Header.ReturnValue = RenameSheetFromOtherThread ((char*)In + In->OffsetOldSheetName, (char*)In + In->OffsetNewSheetName);
    return Out->Header.StructSize;
}

static int RPCFunc_OpenWindow(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_OPEN_WINDOW_MESSAGE *In = (RPC_API_OPEN_WINDOW_MESSAGE*)par_DataIn;
    RPC_API_OPEN_WINDOW_MESSAGE_ACK *Out = (RPC_API_OPEN_WINDOW_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_OPEN_WINDOW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_OPEN_WINDOW_MESSAGE_ACK);

    Out->Header.ReturnValue = OpenWindowByFilter ((char*)In + In->OffsetWindowName);
    return Out->Header.StructSize;
}

static int RPCFunc_CloseWindow(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_CLOSE_WINDOW_MESSAGE *In = (RPC_API_CLOSE_WINDOW_MESSAGE*)par_DataIn;
    RPC_API_CLOSE_WINDOW_MESSAGE_ACK *Out = (RPC_API_CLOSE_WINDOW_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_CLOSE_WINDOW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CLOSE_WINDOW_MESSAGE_ACK);

    Out->Header.ReturnValue = CloseWindowByFilter ((char*)In + In->OffsetWindowName);
    return Out->Header.StructSize;
}

static int RPCFunc_DeleteWindow(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_DELETE_WINDOW_MESSAGE *In = (RPC_API_DELETE_WINDOW_MESSAGE*)par_DataIn;
    RPC_API_DELETE_WINDOW_MESSAGE_ACK *Out = (RPC_API_DELETE_WINDOW_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_DELETE_WINDOW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DELETE_WINDOW_MESSAGE_ACK);

    Out->Header.ReturnValue = DeleteWindowByFilter ((char*)In + In->OffsetWindowName);
    return Out->Header.StructSize;
}

static int RPCFunc_ImportWindow(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_IMPORT_WINDOW_MESSAGE *In = (RPC_API_IMPORT_WINDOW_MESSAGE*)par_DataIn;
    RPC_API_IMPORT_WINDOW_MESSAGE_ACK *Out = (RPC_API_IMPORT_WINDOW_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_IMPORT_WINDOW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_IMPORT_WINDOW_MESSAGE_ACK);

    Out->Header.ReturnValue = ImportWindowByFilter ((char*)In + In->OffsetWindowName, (char*)In + In->OffsetFileName);
    return Out->Header.StructSize;
}

static int RPCFunc_ExportWindow(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_EXPORT_WINDOW_MESSAGE *In = (RPC_API_EXPORT_WINDOW_MESSAGE*)par_DataIn;
    RPC_API_EXPORT_WINDOW_MESSAGE_ACK *Out = (RPC_API_EXPORT_WINDOW_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_EXPORT_WINDOW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_EXPORT_WINDOW_MESSAGE_ACK);

    Out->Header.ReturnValue = ExportWindowByFilter ((char*)In + In->OffsetSheetName, (char*)In + In->OffsetWindowName, (char*)In + In->OffsetFileName);
    return Out->Header.StructSize;
}


int AddGuiFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOAD_DESKTOP_CMD, 1, RPCFunc_LoadDesktop, sizeof(RPC_API_LOAD_DESKTOP_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_LOAD_DESKTOP_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOAD_DESKTOP_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SAVE_DESKTOP_CMD, 1, RPCFunc_SaveDesktop, sizeof(RPC_API_SAVE_DESKTOP_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SAVE_DESKTOP_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SAVE_DESKTOP_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CLEAR_DESKTOP_CMD, 1, RPCFunc_ClearDesktop, sizeof(RPC_API_CLEAR_DESKTOP_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_CLEAR_DESKTOP_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CLEAR_DESKTOP_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CREATE_DIALOG_CMD, 1, RPCFunc_CreateDialog, sizeof(RPC_API_CREATE_DIALOG_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_CREATE_DIALOG_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CREATE_DIALOG_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ADD_DIALOG_ITEM_CMD, 1, RPCFunc_AddDialogItem, sizeof(RPC_API_ADD_DIALOG_ITEM_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ADD_DIALOG_ITEM_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SHOW_DIALOG_CMD, 1, RPCFunc_ShowDialog, sizeof(RPC_API_SHOW_DIALOG_MESSAGE), sizeof(RPC_API_SHOW_DIALOG_MESSAGE), STRINGIZE(RPC_API_SHOW_DIALOG_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SHOW_DIALOG_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_IS_DIALOG_CLOSED_CMD, 1, RPCFunc_IsDialogClosed, sizeof(RPC_API_IS_DIALOG_CLOSED_MESSAGE), sizeof(RPC_API_IS_DIALOG_CLOSED_MESSAGE), STRINGIZE(RPC_API_IS_DIALOG_CLOSED_MESSAGE_MEMBERS), STRINGIZE(RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK_MEMBERS));

    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SELECT_SHEET_CMD, 1, RPCFunc_SelectSheet, sizeof(RPC_API_SELECT_SHEET_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SELECT_SHEET_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SELECT_SHEET_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ADD_SHEET_CMD, 1, RPCFunc_AddSheet, sizeof(RPC_API_ADD_SHEET_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ADD_SHEET_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ADD_SHEET_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DELETE_SHEET_CMD, 1, RPCFunc_DeleteSheet, sizeof(RPC_API_DELETE_SHEET_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_DELETE_SHEET_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DELETE_SHEET_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_RENAME_SHEET_CMD, 1, RPCFunc_RenameSheet, sizeof(RPC_API_RENAME_SHEET_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_RENAME_SHEET_MESSAGE_MEMBERS), STRINGIZE(RPC_API_RENAME_SHEET_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_OPEN_WINDOW_CMD, 1, RPCFunc_OpenWindow, sizeof(RPC_API_OPEN_WINDOW_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_OPEN_WINDOW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_OPEN_WINDOW_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CLOSE_WINDOW_CMD, 1, RPCFunc_CloseWindow, sizeof(RPC_API_CLOSE_WINDOW_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_CLOSE_WINDOW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CLOSE_WINDOW_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DELETE_WINDOW_CMD, 1, RPCFunc_DeleteWindow, sizeof(RPC_API_DELETE_WINDOW_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_DELETE_WINDOW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DELETE_WINDOW_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_IMPORT_WINDOW_CMD, 1, RPCFunc_ImportWindow, sizeof(RPC_API_IMPORT_WINDOW_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_IMPORT_WINDOW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_IMPORT_WINDOW_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_EXPORT_WINDOW_CMD, 1, RPCFunc_ExportWindow, sizeof(RPC_API_EXPORT_WINDOW_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_EXPORT_WINDOW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_EXPORT_WINDOW_MESSAGE_ACK_MEMBERS));
    return 0;
}

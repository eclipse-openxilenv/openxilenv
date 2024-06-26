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


#ifndef RPCFUNCGUI_H
#define RPCFUNCGUI_H

#include <stdint.h>

#include "RpcFuncBase.h"

#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct on
#endif
#endif
#pragma pack(push,1)

#define GUI_CMD_OFFSET  80

#define RPC_API_LOAD_DESKTOP_CMD         (GUI_CMD_OFFSET+0)
typedef struct {
#define RPC_API_LOAD_DESKTOP_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetFile;\
    char Data[1];  // > 1Byte!
    RPC_API_LOAD_DESKTOP_MESSAGE_MEMBERS
} RPC_API_LOAD_DESKTOP_MESSAGE;

typedef struct {
#define RPC_API_LOAD_DESKTOP_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_LOAD_DESKTOP_MESSAGE_ACK_MEMBERS
} RPC_API_LOAD_DESKTOP_MESSAGE_ACK;

#define RPC_API_SAVE_DESKTOP_CMD         (GUI_CMD_OFFSET+1)
typedef struct {
#define RPC_API_SAVE_DESKTOP_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetFile;\
    char Data[1];  // > 1Byte!
    RPC_API_SAVE_DESKTOP_MESSAGE_MEMBERS
} RPC_API_SAVE_DESKTOP_MESSAGE;

typedef struct {
#define RPC_API_SAVE_DESKTOP_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_SAVE_DESKTOP_MESSAGE_ACK_MEMBERS
} RPC_API_SAVE_DESKTOP_MESSAGE_ACK;

#define RPC_API_CLEAR_DESKTOP_CMD         (GUI_CMD_OFFSET+2)
typedef struct {
#define RPC_API_CLEAR_DESKTOP_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_CLEAR_DESKTOP_MESSAGE_MEMBERS
} RPC_API_CLEAR_DESKTOP_MESSAGE;

typedef struct {
#define RPC_API_CLEAR_DESKTOP_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_CLEAR_DESKTOP_MESSAGE_ACK_MEMBERS
} RPC_API_CLEAR_DESKTOP_MESSAGE_ACK;

#define RPC_API_CREATE_DIALOG_CMD         (GUI_CMD_OFFSET+3)
typedef struct {
#define RPC_API_CREATE_DIALOG_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetDialogName;\
    char Data[1];  // > 1Byte!
    RPC_API_CREATE_DIALOG_MESSAGE_MEMBERS
} RPC_API_CREATE_DIALOG_MESSAGE;

typedef struct {
#define RPC_API_CREATE_DIALOG_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_CREATE_DIALOG_MESSAGE_ACK_MEMBERS
} RPC_API_CREATE_DIALOG_MESSAGE_ACK;

#define RPC_API_ADD_DIALOG_ITEM_CMD         (GUI_CMD_OFFSET+4)
typedef struct {
#define RPC_API_ADD_DIALOG_ITEM_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetDescription;\
    int32_t OffsetVariName;\
    char Data[1];  // > 1Byte!
    RPC_API_ADD_DIALOG_ITEM_MESSAGE_MEMBERS
} RPC_API_ADD_DIALOG_ITEM_MESSAGE;

typedef struct {
#define RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK_MEMBERS
} RPC_API_ADD_DIALOG_ITEM_MESSAGE_ACK;

#define RPC_API_SHOW_DIALOG_CMD         (GUI_CMD_OFFSET+5)
typedef struct {
#define RPC_API_SHOW_DIALOG_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_SHOW_DIALOG_MESSAGE_MEMBERS
} RPC_API_SHOW_DIALOG_MESSAGE;

typedef struct {
#define RPC_API_SHOW_DIALOG_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_SHOW_DIALOG_MESSAGE_ACK_MEMBERS
} RPC_API_SHOW_DIALOG_MESSAGE_ACK;

#define RPC_API_IS_DIALOG_CLOSED_CMD         (GUI_CMD_OFFSET+6)
typedef struct {
#define RPC_API_IS_DIALOG_CLOSED_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_IS_DIALOG_CLOSED_MESSAGE_MEMBERS
} RPC_API_IS_DIALOG_CLOSED_MESSAGE;

typedef struct {
#define RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK_MEMBERS
} RPC_API_IS_DIALOG_CLOSED_MESSAGE_ACK;

#define RPC_API_SELECT_SHEET_CMD         (GUI_CMD_OFFSET+10)
typedef struct {
#define RPC_API_SELECT_SHEET_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSheetName;\
    char Data[1];  // > 1Byte!
    RPC_API_SELECT_SHEET_MESSAGE_MEMBERS
} RPC_API_SELECT_SHEET_MESSAGE;

typedef struct {
#define RPC_API_SELECT_SHEET_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_SELECT_SHEET_MESSAGE_ACK_MEMBERS
} RPC_API_SELECT_SHEET_MESSAGE_ACK;

#define RPC_API_ADD_SHEET_CMD         (GUI_CMD_OFFSET+11)
typedef struct {
#define RPC_API_ADD_SHEET_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSheetName;\
    char Data[1];  // > 1Byte!
    RPC_API_ADD_SHEET_MESSAGE_MEMBERS
} RPC_API_ADD_SHEET_MESSAGE;

typedef struct {
#define RPC_API_ADD_SHEET_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_ADD_SHEET_MESSAGE_ACK_MEMBERS
} RPC_API_ADD_SHEET_MESSAGE_ACK;

#define RPC_API_DELETE_SHEET_CMD         (GUI_CMD_OFFSET+12)
typedef struct {
#define RPC_API_DELETE_SHEET_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSheetName;\
    char Data[1];  // > 1Byte!
    RPC_API_DELETE_SHEET_MESSAGE_MEMBERS
} RPC_API_DELETE_SHEET_MESSAGE;

typedef struct {
#define RPC_API_DELETE_SHEET_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_DELETE_SHEET_MESSAGE_ACK_MEMBERS
} RPC_API_DELETE_SHEET_MESSAGE_ACK;

#define RPC_API_RENAME_SHEET_CMD         (GUI_CMD_OFFSET+13)
typedef struct {
#define RPC_API_RENAME_SHEET_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetOldSheetName;\
    int32_t OffsetNewSheetName;\
    char Data[1];  // > 1Byte!
    RPC_API_RENAME_SHEET_MESSAGE_MEMBERS
} RPC_API_RENAME_SHEET_MESSAGE;

typedef struct {
#define RPC_API_RENAME_SHEET_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_RENAME_SHEET_MESSAGE_ACK_MEMBERS
} RPC_API_RENAME_SHEET_MESSAGE_ACK;

#define RPC_API_OPEN_WINDOW_CMD         (GUI_CMD_OFFSET+14)
typedef struct {
#define RPC_API_OPEN_WINDOW_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetWindowName;\
    char Data[1];  // > 1Byte!
    RPC_API_OPEN_WINDOW_MESSAGE_MEMBERS
} RPC_API_OPEN_WINDOW_MESSAGE;

typedef struct {
#define RPC_API_OPEN_WINDOW_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_OPEN_WINDOW_MESSAGE_ACK_MEMBERS
} RPC_API_OPEN_WINDOW_MESSAGE_ACK;

#define RPC_API_CLOSE_WINDOW_CMD         (GUI_CMD_OFFSET+15)
typedef struct {
#define RPC_API_CLOSE_WINDOW_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetWindowName;\
    char Data[1];  // > 1Byte!
    RPC_API_CLOSE_WINDOW_MESSAGE_MEMBERS
} RPC_API_CLOSE_WINDOW_MESSAGE;

typedef struct {
#define RPC_API_CLOSE_WINDOW_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_CLOSE_WINDOW_MESSAGE_ACK_MEMBERS
} RPC_API_CLOSE_WINDOW_MESSAGE_ACK;

#define RPC_API_DELETE_WINDOW_CMD         (GUI_CMD_OFFSET+16)
typedef struct {
#define RPC_API_DELETE_WINDOW_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetWindowName;\
    char Data[1];  // > 1Byte!
    RPC_API_DELETE_WINDOW_MESSAGE_MEMBERS
} RPC_API_DELETE_WINDOW_MESSAGE;

typedef struct {
#define RPC_API_DELETE_WINDOW_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_DELETE_WINDOW_MESSAGE_ACK_MEMBERS
} RPC_API_DELETE_WINDOW_MESSAGE_ACK;

#define RPC_API_IMPORT_WINDOW_CMD         (GUI_CMD_OFFSET+17)
typedef struct {
#define RPC_API_IMPORT_WINDOW_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetWindowName;\
    int32_t OffsetFileName;\
    char Data[1];  // > 1Byte!
    RPC_API_IMPORT_WINDOW_MESSAGE_MEMBERS
} RPC_API_IMPORT_WINDOW_MESSAGE;

typedef struct {
#define RPC_API_IMPORT_WINDOW_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_IMPORT_WINDOW_MESSAGE_ACK_MEMBERS
} RPC_API_IMPORT_WINDOW_MESSAGE_ACK;

#define RPC_API_EXPORT_WINDOW_CMD         (GUI_CMD_OFFSET+18)
typedef struct {
#define RPC_API_EXPORT_WINDOW_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSheetName;\
    int32_t OffsetWindowName;\
    int32_t OffsetFileName;\
    char Data[1];  // > 1Byte!
    RPC_API_EXPORT_WINDOW_MESSAGE_MEMBERS
} RPC_API_EXPORT_WINDOW_MESSAGE;

typedef struct {
#define RPC_API_EXPORT_WINDOW_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_EXPORT_WINDOW_MESSAGE_ACK_MEMBERS
} RPC_API_EXPORT_WINDOW_MESSAGE_ACK;

#pragma pack(pop)
#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct reset
#endif
#endif

int AddGuiFunctionToTable(void);


#endif // RPCFUNCGUI_H

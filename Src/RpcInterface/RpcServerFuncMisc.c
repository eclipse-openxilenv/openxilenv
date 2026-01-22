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
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Files.h"
#include "EnvironmentVariables.h"
#include "ScriptMessageFile.h"
#include "ScriptChangeSettings.h"

#include "RpcSocketServer.h"
#include "RpcFuncMisc.h"

#define UNUSED(x) (void)(x)

static int RPCFunc_CreateFileWithContent(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    FILE *fh;
    char Path[MAX_PATH];
    RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE *In = (RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE*)par_DataIn;
    RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK *Out = (RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK);
    SearchAndReplaceEnvironmentStrings ((char*)In + In->OffsetFilename, Path, sizeof (Path));

    fh = fopen (Path, "wt");
    if (fh == NULL) {
        Out->Header.ReturnValue = -1;
    } else {
        char *Content = (char*)In + In->OffsetContent;
        if (fputs (Content, fh) < 0) {
            Out->Header.ReturnValue = -1;
        }
        fclose (fh);
    }
    return Out->Header.StructSize;
}


static int RPCFunc_GetEnvironmentVariable(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE *In = (RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE*)par_DataIn;
    RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK *Out = (RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK);
    Out->OffsetVariableValue = sizeof(RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK) - 1;

    if (strlen ((char*)In + In->OffsetVariableName) >= 2048) {
        Out->Header.ReturnValue = -1;
        return sizeof (RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK);
    } else  {
        char Help[4096+2];
        STRING_COPY_TO_ARRAY(Help, "%");
        STRING_APPEND_TO_ARRAY(Help,(char*)In + In->OffsetVariableName);
        STRING_APPEND_TO_ARRAY(Help, "%");
        Out->Header.ReturnValue = SearchAndReplaceEnvironmentStrings (Help, Out->Data, 4096);
        if (Out->Header.ReturnValue >= 0) {
            if (!strcmp (Help, Out->Data)) {
                Out->Data[0] = 0;
                Out->Header.ReturnValue = -1;
            } else {;
                Out->Header.StructSize += (int32_t)strlen(Out->Data);
                Out->Header.ReturnValue = 0;
            }
        } else {
            Out->Header.ReturnValue= -1;
            Out->Data[0] = 0;
        }
    }
    return Out->Header.StructSize;
}

static int RPCFunc_SetEnvironmentVariable(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE *In = (RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE*)par_DataIn;
    RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK *Out = (RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK);

    Out->Header.ReturnValue = SetUserEnvironmentVariable((char*)In + In->OffsetVariableName, (char*)In + In->OffsetVariableValue);

    return Out->Header.StructSize;
}

static int RPCFunc_ChangeSettings(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_CHANGE_SETTINGS_MESSAGE *In = (RPC_API_SET_CHANGE_SETTINGS_MESSAGE*)par_DataIn;
    RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK *Out = (RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK);

    Out->Header.ReturnValue = ScriptChangeBasicSettings (CHANGE_SETTINGS_COMMAND_SET, 0, (char*)In + In->OffseSettingName, (char*)In + In->OffsetSettingValue);

    return Out->Header.StructSize;
}

static int RPCFunc_OutText(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_TEXT_OUT_MESSAGE *In = (RPC_API_TEXT_OUT_MESSAGE*)par_DataIn;
    RPC_API_TEXT_OUT_MESSAGE_ACK *Out = (RPC_API_TEXT_OUT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_TEXT_OUT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_TEXT_OUT_MESSAGE_ACK);

    AddScriptMessage ((char*)In + In->OffsetText);

    Out->Header.ReturnValue = 0;

    return Out->Header.StructSize;
}

static int RPCFunc_ErrorOutText(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_ERROR_TEXT_OUT_MESSAGE *In = (RPC_API_ERROR_TEXT_OUT_MESSAGE*)par_DataIn;
    RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK *Out = (RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK);

    Out->Header.ReturnValue = ThrowError (In->ErrorLevel, (char*)In + In->OffsetText);

    return Out->Header.StructSize;
}


static int RPCFunc_CreateFile(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_CREATE_FILE_MESSAGE *In = (RPC_API_CREATE_FILE_MESSAGE*)par_DataIn;
    RPC_API_CREATE_FILE_MESSAGE_ACK *Out = (RPC_API_CREATE_FILE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_CREATE_FILE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CREATE_FILE_MESSAGE_ACK);
#ifdef _WIN32
    Out->Handle = (uint64_t)CreateFile ((char*)In + In->OffsetFilename, In->dwDesiredAccess, In->dwShareMode, NULL, In->dwCreationDisposition, In->dwFlagsAndAttributes, NULL);
#else
    {
        int flags = 0;
        mode_t mode = 0;
        if ((In->dwDesiredAccess & 0x40000000U) == 0x40000000U) { // GENERIC_WRITE
            if ((In->dwDesiredAccess & 0x80000000U) == 0x80000000U) { // GENERIC_READ
                flags |= O_RDWR;
            } else {
                flags |= O_WRONLY;
            }
        }
        if ((In->dwDesiredAccess & 0x00000004U) == 0x00000004U) { // FILE_APPEND_DATA
            flags |= O_APPEND;
        }
        if ((In->dwCreationDisposition & 0x00000002U) == 0x00000002U) {  // CREATE_ALWAYS
            flags |= O_CREAT;
        }

        Out->Handle = (uint64_t)open ((char*)In + In->OffsetFilename, flags, mode);
    }
#endif
    return Out->Header.StructSize;
}

static int RPCFunc_CloseHandle(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_CLOSE_HANDLE_MESSAGE *In = (RPC_API_CLOSE_HANDLE_MESSAGE*)par_DataIn;
    RPC_API_CLOSE_HANDLE_MESSAGE_ACK *Out = (RPC_API_CLOSE_HANDLE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_CLOSE_HANDLE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_CLOSE_HANDLE_MESSAGE_ACK);
#ifdef _WIN32
    CloseHandle((HANDLE)In->Handle);
#else
    close((HANDLE)In->Handle);
#endif
    return Out->Header.StructSize;
}

static int RPCFunc_ReadFile(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_READ_FILE_MESSAGE *In = (RPC_API_READ_FILE_MESSAGE*)par_DataIn;
    RPC_API_READ_FILE_MESSAGE_ACK *Out = (RPC_API_READ_FILE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_READ_FILE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_READ_FILE_MESSAGE_ACK);
    Out->Offset_uint8_NumberOfBytesRead_Buffer = sizeof(RPC_API_READ_FILE_MESSAGE_ACK) - 1;
#ifdef _WIN32
    Out->Header.ReturnValue = ReadFile((HANDLE)In->Handle, (char*)Out + Out->Offset_uint8_NumberOfBytesRead_Buffer, In->nNumberOfBytesToRead, (LPDWORD)&(Out->NumberOfBytesRead), NULL);
#else
    Out->NumberOfBytesRead = read((HANDLE)In->Handle, (char*)Out + Out->Offset_uint8_NumberOfBytesRead_Buffer, In->nNumberOfBytesToRead);
    if (Out->NumberOfBytesRead <= 0) {
        Out->Header.ReturnValue = 1;
        Out->NumberOfBytesRead = 0;
    }
#endif
    Out->Header.StructSize += Out->NumberOfBytesRead;
    return Out->Header.StructSize;
}

static int RPCFunc_WriteFile(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_WRITE_FILE_MESSAGE *In = (RPC_API_WRITE_FILE_MESSAGE*)par_DataIn;
    RPC_API_WRITE_FILE_MESSAGE_ACK *Out = (RPC_API_WRITE_FILE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_WRITE_FILE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_WRITE_FILE_MESSAGE_ACK);
#ifdef _WIN32
    Out->Header.ReturnValue = WriteFile((HANDLE)In->Handle, (char*)In + In->Offset_uint8_nNumberOfBytesToWrite_Buffer, In->nNumberOfBytesToWrite, (LPDWORD)&(Out->NumberOfBytesWritten), NULL);
#else
    Out->NumberOfBytesWritten = write((HANDLE)In->Handle, (char*)In + In->Offset_uint8_nNumberOfBytesToWrite_Buffer, In->nNumberOfBytesToWrite);
    if (Out->NumberOfBytesWritten <= 0) {
        Out->Header.ReturnValue = 1;
        Out->NumberOfBytesWritten = 0;
    }
#endif
    return Out->Header.StructSize;
}


int AddMiscFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CREATE_FILE_WITH_CONTENT_CMD, 0, RPCFunc_CreateFileWithContent, sizeof(RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_ENVIRONMENT_VARIABLE_CMD, 0, RPCFunc_GetEnvironmentVariable, sizeof(RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_ENVIRONMENT_VARIABLE_CMD, 0, RPCFunc_SetEnvironmentVariable, sizeof(RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_TEXT_OUT_CMD, 1, RPCFunc_OutText, sizeof(RPC_API_TEXT_OUT_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_TEXT_OUT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_TEXT_OUT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ERROR_TEXT_OUT_CMD, 1, RPCFunc_ErrorOutText, sizeof(RPC_API_ERROR_TEXT_OUT_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ERROR_TEXT_OUT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CHANGE_SETTINGS_CMD, 1, RPCFunc_ChangeSettings, sizeof(RPC_API_SET_CHANGE_SETTINGS_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_CHANGE_SETTINGS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CREATE_FILE_CMD, 0, RPCFunc_CreateFile, sizeof(RPC_API_CREATE_FILE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_CREATE_FILE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CREATE_FILE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_CLOSE_HANDLE_CMD, 0, RPCFunc_CloseHandle, sizeof(RPC_API_CLOSE_HANDLE_MESSAGE), sizeof(RPC_API_CLOSE_HANDLE_MESSAGE), STRINGIZE(RPC_API_CLOSE_HANDLE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_CLOSE_HANDLE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_READ_FILE_CMD, 0, RPCFunc_ReadFile, sizeof(RPC_API_READ_FILE_MESSAGE), sizeof(RPC_API_READ_FILE_MESSAGE), STRINGIZE(RPC_API_READ_FILE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_READ_FILE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_WRITE_FILE_CMD, 0, RPCFunc_WriteFile, sizeof(RPC_API_WRITE_FILE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_WRITE_FILE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_WRITE_FILE_MESSAGE_ACK_MEMBERS));
    return 0;
}

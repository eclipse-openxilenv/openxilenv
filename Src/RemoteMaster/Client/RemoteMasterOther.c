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


#include <stdint.h>
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "StructRM.h"
#include "RemoteMasterNet.h"
#include "EnvironmentVariables.h"
#include "RemoteMasterOther.h"
#include "MainValues.h"
#include "Platform.h"

int rm_Init (int par_PreAllocMemorySize, int par_PidForSchedulersHelperProcess, uint32_t *ret_Version, int32_t *ret_PatchVersion)
{
    RM_INIT_REQ *Req;
    RM_INIT_ACK Ack;
    int x, SizeOfStruct;
    uint32_t Offset;

    SizeOfStruct = (int)sizeof(RM_INIT_REQ);
    for (x = 0; x < 32; x++) {
        if (s_main_ini_val.ConfigurablePrefix[x] != NULL) {
            SizeOfStruct += strlen(s_main_ini_val.ConfigurablePrefix[x]) + 1;
        }
    }
    Req = (RM_INIT_REQ*)my_malloc((size_t)SizeOfStruct);

    Req->PreAllocMemorySize = par_PreAllocMemorySize;
    Req->PidForSchedulersHelperProcess = par_PidForSchedulersHelperProcess;
    Offset = sizeof(RM_INIT_REQ);
    for (x = 0; x < 32; x++) {
        if (s_main_ini_val.ConfigurablePrefix[x] != NULL) {
            int Len = strlen(s_main_ini_val.ConfigurablePrefix[x]) + 1;
            Req->OffsetConfigurablePrefix[x] = Offset;
            StringCopyMaxCharTruncate((char*)Req + Offset, s_main_ini_val.ConfigurablePrefix[x], Len);
            Offset += Len;
        } else {
            Req->OffsetConfigurablePrefix[x] = 0;
        }
    }

    TransactRemoteMaster (RM_INIT_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    my_free(Req);
    *ret_Version = Ack.Version;
    *ret_PatchVersion = Ack.PatchVersion;
    return Ack.Ret;
}


int rm_Terminate (void)
{
    RM_TERMINATE_REQ Req;
    RM_TERMINATE_ACK Ack;

    TransactRemoteMaster (RM_TERMINATE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));

    return Ack.Ret;
}

void rm_KillThread (void)
{
    RM_KILL_THREAD_REQ Req;

    SentToRemoteMaster (&Req, RM_KILL_THREAD_CMD, sizeof(Req));
#ifdef _WIN32
    closesocket(RemoteMasterGetSocket());
#else
    close(RemoteMasterGetSocket());
#endif
}

void rm_Kill (void)
{
    RM_KILL_REQ Req;

    CloseAllOtherSocketsToRemoteMaster();
    SentToRemoteMaster (&Req, RM_KILL_CMD, sizeof(Req));
#ifdef _WIN32
    closesocket(RemoteMasterGetSocket());
#else
    close(RemoteMasterGetSocket());
#endif
}

int rm_Ping (int Value)
{
    RM_PING_REQ Req;
    RM_PING_ACK Ack;

    Req.Value = Value;
    TransactRemoteMaster (RM_TERMINATE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));

    return Ack.Ret;
}

#define FILE_BUFFER_SIZE 1024
#define BUFFER_SIZE (2*1024)

int rm_CopyFile(char *par_FileNameSrc, char *par_FileNameDst)
{
    RM_COPY_FILE_START_REQ *Req;
    RM_COPY_FILE_START_ACK *Ack;
    uint32_t Counter;
    HANDLE hFileExe = INVALID_HANDLE_VALUE;
    void *FileBuffer = NULL;
    int Ret = -1;
    int LenFileNameDst;
    int SizeOfStruct;
    int32_t FileHandle;
    char FilenameSolvedEnvVars[2048];

    SearchAndReplaceEnvironmentStrings(par_FileNameSrc, FilenameSolvedEnvVars, sizeof(FilenameSolvedEnvVars));
    Req = my_malloc(BUFFER_SIZE);
    Ack = my_malloc(BUFFER_SIZE);
    if ((Req == NULL) || (Ack == NULL)) goto __ERROR;

    hFileExe = INVALID_HANDLE_VALUE;
    FileBuffer = my_malloc(FILE_BUFFER_SIZE);
    if (FileBuffer == NULL) goto __ERROR;

    hFileExe = CreateFile (FilenameSolvedEnvVars,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL_INT_OR_PTR);
    if (hFileExe == (HANDLE)HFILE_ERROR) {
        ThrowError (1, "Unable to open %s file\n", FilenameSolvedEnvVars);
        goto __ERROR;
    }

    LenFileNameDst = (int)strlen (par_FileNameDst) + 1;
    SizeOfStruct = (int)sizeof (RM_COPY_FILE_START_REQ) + LenFileNameDst;
    Req->OffsetFileName = sizeof(RM_COPY_FILE_START_REQ);
    StringCopyMaxCharTruncate((char*)Req + Req->OffsetFileName, par_FileNameDst, LenFileNameDst);

    TransactRemoteMaster (RM_COPY_FILE_START_CMD, Req, SizeOfStruct, Ack, BUFFER_SIZE);
    if (Ack->Ret) {
        ThrowError (1, "cannot open \"%s\" on target (%s)", par_FileNameDst, (char*)Ack + Ack->OffsetErrorString);
        goto __ERROR;
    } else {
        DWORD NumberOfBytesRead;
        FileHandle = Ack->FileHandle;
        Counter = 0;
        while (1) {
            RM_COPY_FILE_NEXT_REQ *Req2 = (RM_COPY_FILE_NEXT_REQ*)Req;
            RM_COPY_FILE_NEXT_ACK *Ack2 = (RM_COPY_FILE_NEXT_ACK*)Ack;
            if (!ReadFile (hFileExe, FileBuffer, FILE_BUFFER_SIZE, &NumberOfBytesRead, NULL)) {
                //DWORD Err =GetLastError();
                CloseHandle (hFileExe);
                ThrowError (1, "cannot read from executable file %s", FilenameSolvedEnvVars);
                goto __ERROR;
            }
            if (NumberOfBytesRead == 0) break; // End of file
            SizeOfStruct = (int)sizeof (RM_COPY_FILE_NEXT_REQ) + (int)NumberOfBytesRead;
            Req2->OffsetData = sizeof(RM_COPY_FILE_NEXT_REQ);
            MEMCPY((char*)Req2 + Req2->OffsetData, FileBuffer, NumberOfBytesRead);
            Req2->FileHandle = FileHandle;
            Req2->BlockSize = NumberOfBytesRead;
            TransactRemoteMaster (RM_COPY_FILE_NEXT_CMD, Req2, SizeOfStruct, Ack2, BUFFER_SIZE);

            if (Ack2->Ret) {
                ThrowError (1, "cannot copy part %i of \"%s\"  (%s)", Counter, par_FileNameDst, (char*)Ack2 + Ack2->OffsetErrorString);
                goto __ERROR;
            }
            Counter++;
        }
        {
            RM_COPY_FILE_END_REQ *Req3 = (RM_COPY_FILE_END_REQ*)Req;
            RM_COPY_FILE_END_ACK *Ack3 = (RM_COPY_FILE_END_ACK*)Ack;
            Req3->FileHandle = FileHandle;
            TransactRemoteMaster (RM_COPY_FILE_END_CMD, Req3, SizeOfStruct, Ack3, BUFFER_SIZE);
            if (Ack3->Ret) {
                ThrowError (1, "cannot end copy \"%s\" (%s)", par_FileNameDst, (char*)Ack3 + Ack3->OffsetErrorString);
                goto __ERROR;
            }

        }
    }
    Ret = 0; // alles durchlaufen
__ERROR:
    if (Req != NULL) my_free (Req);
    if (Ack != NULL) my_free (Ack);
    if (FileBuffer != NULL)  my_free(FileBuffer);
    if (hFileExe != INVALID_HANDLE_VALUE) CloseHandle(hFileExe);
    return Ret;
}


int rm_ReadBytes (int par_Pid, uint64_t par_Address, int par_Size, void *ret_Data)
{
    RM_READ_DATA_BYTES_REQ Req;
    RM_READ_DATA_BYTES_ACK *Ack;
    int SizeOfStruct;

    Req.Pid = par_Pid;
    Req.Size = (uint32_t)par_Size;
    Req.Address = par_Address;

    SizeOfStruct = (int)sizeof(RM_READ_DATA_BYTES_ACK) + par_Size;
    Ack = (RM_READ_DATA_BYTES_ACK*)_alloca((size_t)SizeOfStruct);
    TransactRemoteMaster (RM_READ_DATA_BYTES_CMD, &Req, sizeof(Req), Ack, SizeOfStruct);
    if (Ack->Ret > 0) {
        MEMCPY(ret_Data, (char*)Ack + Ack->OffsetData, (size_t)par_Size);
    }
    return Ack->Ret;
}

int rm_WriteBytes(int par_Pid, uint64_t par_Address, int par_Size, void *par_Data)
{
    RM_WRITE_DATA_BYTES_REQ *Req;
    RM_WRITE_DATA_BYTES_ACK Ack;
    int SizeOfStruct;

    SizeOfStruct = (int)sizeof(RM_WRITE_DATA_BYTES_REQ) + par_Size;
    Req = (RM_WRITE_DATA_BYTES_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Pid = par_Pid;
    Req->Size = (uint32_t)par_Size;
    Req->Address = par_Address;
    Req->OffsetData = sizeof(RM_WRITE_DATA_BYTES_REQ);
    MEMCPY((char*)Req + Req->OffsetData, par_Data, (size_t)par_Size);

    TransactRemoteMaster (RM_WRITE_DATA_BYTES_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    return Ack.Ret;
}

int rm_ReferenceVariable(int par_Pid, uint64_t par_Address, const char *par_Name, int par_Type, int par_Dir)
{
    RM_REFERENCE_VARIABLE_REQ *Req;
    RM_REFERENCE_VARIABLE_ACK Ack;
    int SizeOfStruct;
    int Len;

    Len = strlen(par_Name) + 1;
    SizeOfStruct = (int)sizeof(RM_REFERENCE_VARIABLE_REQ) + Len;
    Req = (RM_REFERENCE_VARIABLE_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Pid = par_Pid;
    Req->Address = par_Address;
    Req->OffsetName = sizeof(RM_REFERENCE_VARIABLE_REQ);
    StringCopyMaxCharTruncate((char*)Req + Req->OffsetName, par_Name, Len);
    Req->Type = par_Type;
    Req->Dir = par_Dir;

    TransactRemoteMaster (RM_REFERENCE_VARIABLE_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    return Ack.Ret;
}

int rm_DeReferenceVariable(int par_Pid, uint64_t par_Address, const char *par_Name, int par_Type)
{
    RM_DEREFERENCE_VARIABLE_REQ *Req;
    RM_DEREFERENCE_VARIABLE_ACK Ack;
    int SizeOfStruct;
    int Len;

    Len = strlen(par_Name) + 1;
    SizeOfStruct = (int)sizeof(RM_DEREFERENCE_VARIABLE_REQ) + Len;
    Req = (RM_DEREFERENCE_VARIABLE_REQ*)_alloca((size_t)SizeOfStruct);
    Req->Pid = par_Pid;
    Req->Address = par_Address;
    Req->OffsetName = sizeof(RM_DEREFERENCE_VARIABLE_REQ);
    StringCopyMaxCharTruncate((char*)Req + Req->OffsetName, par_Name, Len);
    Req->Type = par_Type;

    TransactRemoteMaster (RM_DEREFERENCE_VARIABLE_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    return Ack.Ret;
}


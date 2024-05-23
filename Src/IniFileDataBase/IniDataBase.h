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


#pragma once

#include "Platform.h"

// max. line length inside INI file
#define INI_MAX_LINE_LENGTH  4096
#define INI_MAX_LONGLINE_LENGTH  131071
#define INI_MAX_SECTION_LENGTH  1024
#define INI_MAX_ENTRYNAME_LENGTH  1024

// This should be static
int GetFileDescriptorByName(const char *par_Filename);
void SetMainFileDescriptor(int par_FileDescriptor);
int GetMainFileDescriptor(void);
void SetMainIniFilename(const char *par_FileName);

int IniFileDataBaseInit(void);
void IniFileDataBaseTerminate(void);
int IniFileDataBaseOpen(const char *par_Filename);
int IniFileDataBaseClose(int par_FileDescriptor);
#define INIFILE_DATABAE_OPERATION_WRITE_ONLY  0
#define INIFILE_DATABAE_OPERATION_RENAME      1
#define INIFILE_DATABAE_OPERATION_REMOVE      2
int IniFileDataBaseSave(int par_FileDescriptor, const char *par_DstFileName, int par_Operation);
int IniFileDataBaseGetFileNameByDescriptor(int par_FileDescriptor, char *ret_Name, int par_MaxSize);

int IniFileDataBaseWriteString (const char* section, const char* par_Entry, const char* txt, int par_FileDescriptor);
int IniFileDataBaseWriteInt (const char* par_Section, const char* par_Entry, int Value, int par_FileDescriptor);
int IniFileDataBaseWriteUInt (const char* par_Section, const char* par_Entry, unsigned int Value,int par_FileDescriptor);
int IniFileDataBaseWriteUIntHex (const char* par_Section, const char* par_Entry, unsigned int Value, int par_FileDescriptor);
int IniFileDataBaseWriteFloat (const char* par_Section, const char* par_Entry, double Value, int par_FileDescriptor);
int IniFileDataBaseWriteYesNo (const char* par_Section, const char* par_Entry, int Flag, int par_FileDescriptor);
int IniFileDataBaseWriteSection (const char* section, const char* txt, int par_FileDescriptor);

int IniFileDataBaseReadString (const char* section, const char* par_Entry, const char* deftxt,
                                 char* txt, int nsize, int par_FileDescriptor);
char *IniFileDataBaseReadStringBufferNoDef (const char* par_Section, const char* par_Entry, int par_FileDescriptor);
char *IniFileDataBaseReadStringBuffer (const char* par_Section, const char* par_Entry, const char* par_DefaulText,
                                       int par_FileDescriptor);
void IniFileDataBaseReadStringBufferFree (char *par_Buffer);

int IniFileDataBaseReadInt (const char* section, const char* par_Entry, int par_DefaultValue, int par_FileDescriptor);
unsigned int IniFileDataBaseReadUInt (const char* section, const char* par_Entry, unsigned int par_DefaultValue, int par_FileDescriptor);
double IniFileDataBaseReadFloat (const char* par_Section, const char* par_Entry, double par_DefaultValue, int par_FileDescriptor);
int IniFileDataBaseReadYesNo (const char* section, const char* par_Entry, int par_DefaultValue, int par_FileDescriptor);
int IniFileDataBaseReadSection (const char* section, char* txt, int nsize, int par_FileDescriptor);
char *IniFileDataBaseReadSectionBuffer (const char* par_Section, int par_FileDescriptor);
void IniFileDataBaseReadSectionBufferFree (char *par_Buffer);

int IniFileDataBaseWriteByteImage(const char* par_Section, const char* par_EntryPrefix, int par_Type, void *par_Image, int par_Len, int par_FileDescriptor);

// If ret_buffer_size <= 0 and ret_buffer == NULL than only the needed buffer size will returned
// If ret_buffer_size <= 0 and ret_buffer != NULL than the buffer will be allocated, This buffer must be give free with DBFreeByteImageBuffer
int IniFileDataBaseReadByteImage (const char* par_Section, const char* par_Entry_prefix, int *ret_type,
                                    void *ret_buffer, int ret_buffer_size, int par_FileDescriptor);
void IniFileDataBaseFreeByteImageBuffer (void *par_Buffer);

int IniFileDataBaseGetNextSectionName (int par_Index, char *ret_Section, int par_MaxLen, int par_FileDescriptor);
int IniFileDataBaseGetNextEntryName (int par_Index, const char *par_Section, char *ret_Entry, int par_MaxLen,
                                    int par_FileDescriptor);
int IniFileDataBaseCopySection (int par_DstFileDescriptor, int par_SrcFileDescriptor,
                               const char *par_DstSection, const char *par_SrcSection);
int IniFileDataBaseCopySectionSameName (int par_DstFileDescriptor, int par_SrcFileDescriptor, const char *par_Section);
int IniFileDataBaseCopySectionSameFile (int par_FileDescriptor, const char *par_DstSection, const char *par_SrcSection);
int IniFileDataBaseRenameFile (const char *par_NewName, int par_FileDescriptor);
int IniFileDataBaseFindNextSectionNameRegExp (int par_Index, const char *par_Filter, int par_CaseSensetive,
                                             char *ret_Name, int par_MaxLen, int par_FileDescriptor);
int IniFileDataBaseLookIfSectionExist (const char *Section, int par_FileDescriptor);
int IniFileDataBaseGetSectionNumberOfEntrys (const char *par_Section, int par_FileDescriptor);

void IniFileDataBaseEnterCriticalSectionUser(const char *par_File, int par_Line);
void IniFileDataBaseLeaveCriticalSectionUser(void);

int CreateNewEmptyIniFile(const char *par_Name);
int IniFileDataBaseCreateAndOpenNewIniFile(const char *par_Name);

int IsAValidSectionName(const char *par_Name);

int SaveAllInfosToIniDataBase (void);

int set_ini_path (const char *sz_inipath);


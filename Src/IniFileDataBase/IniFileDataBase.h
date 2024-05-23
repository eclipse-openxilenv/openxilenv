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

#define INIFILE ini_path
extern char ini_path[];

#define CONST_CHAR_POINTER const char*
#define CHAR_POINTER char*

int init_ini_db (void);
void terminate_ini_db (void);
int read_ini2db (char *filename);
int write_db2ini (char *SrcFileName, char *DstFileName, int op);
BOOL DBWritePrivateProfileString (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, CONST_CHAR_POINTER txt, CONST_CHAR_POINTER filename);
BOOL DBWritePrivateProfileInt (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, int Value, CONST_CHAR_POINTER filename);
BOOL DBWritePrivateProfileUInt (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, unsigned int Value, CONST_CHAR_POINTER filename);
BOOL DBWritePrivateProfileUIntHex (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, unsigned int Value, CONST_CHAR_POINTER filename);
BOOL DBWritePrivateProfileFloat (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, double Value, CONST_CHAR_POINTER filename);
BOOL DBWritePrivateProfileYesNo (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, int Flag, CONST_CHAR_POINTER filename);
BOOL DBWritePrivateProfileSection (CONST_CHAR_POINTER section, CONST_CHAR_POINTER txt, CONST_CHAR_POINTER filename);
DWORD DBGetPrivateProfileString (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, CONST_CHAR_POINTER deftxt,
                                 CHAR_POINTER txt, DWORD nsize, CONST_CHAR_POINTER filename);
char *DBGetPrivateProfileStringBufferNoDef (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, CONST_CHAR_POINTER filename);
char *DBGetPrivateProfileStringBuffer (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, CONST_CHAR_POINTER deftxt,
                                       CONST_CHAR_POINTER filename);
void DBGetPrivateProfileStringBufferFree (char *buffer);

int DBGetPrivateProfileInt (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, int defvalue, CONST_CHAR_POINTER filename);
unsigned int DBGetPrivateProfileUInt (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, unsigned int defvalue, CONST_CHAR_POINTER filename);
double DBGetPrivateProfileFloat (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, double defvalue, CONST_CHAR_POINTER filename);
int DBGetPrivateProfileYesNo (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, int defvalue, CONST_CHAR_POINTER filename);
DWORD DBGetPrivateProfileSection (CONST_CHAR_POINTER section, CHAR_POINTER txt, DWORD nsize, CONST_CHAR_POINTER filename);

BOOL DBWritePrivateProfileByteImage (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry_prefix, int type, void *image, int len, CONST_CHAR_POINTER filename);

// If ret_buffer_size <= 0 and ret_buffer == NULL than only the needed buffer size will returned
// If ret_buffer_size <= 0 and ret_buffer != NULL than the buffer will be allocated, This buffer must be give free with DBFreeByteImageBuffer
DWORD DBGetPrivateProfileByteImage (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry_prefix, int *ret_type,
                                    void *ret_buffer, DWORD ret_buffer_size, CONST_CHAR_POINTER filename);
void DBFreeByteImageBuffer (void *buffer);

int GetNextSectionName (int idx, char *section, int maxc);
int GetNextEntryName (int EntryIdx, const char *section_in, char *entry, int maxc, const char *filename);
int copy_ini_section (const char *dest_file, const char *src_file, const char *dest_section, const char *src_section);
int copy_ini_section_same_name (const char *dest_file, const char *src_file, const char *section);
int fast_copy_ini_section_same_file (const char *filename, const char *dst_section, const char *src_section);
int rename_section (char *old_name, char *new_name, char *filename);
int RegisterINIFileGetFullPath (char *name, int size);
int UnRegisterINIFile (char *name);
int IsRegisterINIFile (char *name);
int RenameRegisterINIFile (char *DstName, char *SrcName);
int CompareINIFile (char *Name1, char *Name2);
int DBFindNextSectionNameRegExp (int idx, char *filter, int CaseSensetive, char *name, int maxc, CONST_CHAR_POINTER filename);
int LookIfSectionExist (const char *Section, const char *filename);
int DBGetSectionNumberOfEntrys (const char *section_in, const char *filename);

int CreateNewEmptyIniFile(const char *par_Name);
int CreateAndOpenNewIniFile(char *par_Name, int par_MaxSize);

int save_all_infos2ini (void);

void EnterIniDBCriticalSectionUser(const char *par_File, int par_Line);
void LeaveIniDBCriticalSectionUser(void);


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

#include "MainValues.h"

#include "IniDataBase.h"
#include "IniFileDataBase.h"



int init_ini_db ()
{
    return IniFileDataBaseInit();
}

void terminate_ini_db (void)
{
    IniFileDataBaseTerminate();
}

int read_ini2db (char *filename)
{
    return (IniFileDataBaseOpen(filename) > 0) ? 0 : -1;
}

int write_db2ini (char *SrcFileName, char *DstFileName, int op)
{
    return IniFileDataBaseSave(GetFileDescriptorByName(SrcFileName), DstFileName, op);
}

BOOL DBWritePrivateProfileString (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, CONST_CHAR_POINTER txt, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseWriteString(section, entry, txt, GetFileDescriptorByName(filename));
}

BOOL DBWritePrivateProfileInt (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, int Value, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseWriteInt(section_in, entry, Value, GetFileDescriptorByName(filename));
}

BOOL DBWritePrivateProfileUInt (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, unsigned int Value, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseWriteUInt(section_in, entry, Value, GetFileDescriptorByName(filename));
}

BOOL DBWritePrivateProfileUIntHex (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, unsigned int Value, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseWriteUIntHex(section_in, entry, Value, GetFileDescriptorByName(filename));
}

BOOL DBWritePrivateProfileFloat (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, double Value, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseWriteFloat(section_in, entry, Value, GetFileDescriptorByName(filename));
}

BOOL DBWritePrivateProfileYesNo (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, int Flag, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseWriteYesNo(section_in, entry, Flag, GetFileDescriptorByName(filename));
}

BOOL DBWritePrivateProfileSection (CONST_CHAR_POINTER section, CONST_CHAR_POINTER txt, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseWriteSection(section, txt, GetFileDescriptorByName(filename));
}

DWORD DBGetPrivateProfileString (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, CONST_CHAR_POINTER deftxt,
                                 CHAR_POINTER txt, DWORD nsize, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadString(section, entry, deftxt, txt, nsize, GetFileDescriptorByName(filename));
}

char *DBGetPrivateProfileStringBufferNoDef (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadStringBufferNoDef(section_in, entry, GetFileDescriptorByName(filename));
}

char *DBGetPrivateProfileStringBuffer (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, CONST_CHAR_POINTER deftxt,
                                       CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadStringBuffer(section_in, entry, deftxt, GetFileDescriptorByName(filename));
}

void DBGetPrivateProfileStringBufferFree (char *buffer)
{
    IniFileDataBaseReadStringBufferFree(buffer);
}

int DBGetPrivateProfileInt (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, int defvalue, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadInt(section, entry, defvalue, GetFileDescriptorByName(filename));
}

unsigned int DBGetPrivateProfileUInt (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, unsigned int defvalue, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadUInt(section, entry, defvalue, GetFileDescriptorByName(filename));
}

double DBGetPrivateProfileFloat (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry, double defvalue, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadFloat(section_in, entry, defvalue, GetFileDescriptorByName(filename));
}

int DBGetPrivateProfileYesNo (CONST_CHAR_POINTER section, CONST_CHAR_POINTER entry, int defvalue, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadYesNo(section, entry, defvalue, GetFileDescriptorByName(filename));
}

DWORD DBGetPrivateProfileSection (CONST_CHAR_POINTER section, CHAR_POINTER txt, DWORD nsize, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadSection(section, txt, nsize, GetFileDescriptorByName(filename));
}


BOOL DBWritePrivateProfileByteImage (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry_prefix, int type, void *image, int len, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseWriteByteImage(section_in, entry_prefix, type, image, len, GetFileDescriptorByName(filename));
}

// If ret_buffer_size <= 0 and ret_buffer == NULL than only the needed buffer size will returned
// If ret_buffer_size <= 0 and ret_buffer != NULL than the buffer will be allocated, This buffer must be give free with DBFreeByteImageBuffer
DWORD DBGetPrivateProfileByteImage (CONST_CHAR_POINTER section_in, CONST_CHAR_POINTER entry_prefix, int *ret_type,
                                    void *ret_buffer, DWORD ret_buffer_size, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseReadByteImage(section_in, entry_prefix, ret_type, ret_buffer, ret_buffer_size, GetFileDescriptorByName(filename));
}

void DBFreeByteImageBuffer (void *buffer)
{
    IniFileDataBaseFreeByteImageBuffer(buffer);
}

int GetNextEntryName (int EntryIdx, const char *section_in, char *entry, int maxc, const char *filename)
{
    return IniFileDataBaseGetNextEntryName(EntryIdx, section_in, entry, maxc, GetFileDescriptorByName(filename));
}

int copy_ini_section (const char *dest_file, const char *src_file, const char *dest_section, const char *src_section)
{
    return IniFileDataBaseCopySection(GetFileDescriptorByName(dest_file),
                                      GetFileDescriptorByName(src_file),
                                      dest_section, src_section);
}
int copy_ini_section_same_name (const char *dest_file, const char *src_file, const char *section)
{
    return IniFileDataBaseCopySectionSameName(GetFileDescriptorByName(dest_file),
                                              GetFileDescriptorByName(src_file),
                                              section);
}

int RegisterINIFileGetFullPath (char *name, int size)
{
    return 0;
}

int UnRegisterINIFile (char *name)
{
    return IniFileDataBaseClose(GetFileDescriptorByName(name));
}

int DBFindNextSectionNameRegExp (int idx, char *filter, int CaseSensetive, char *name, int maxc, CONST_CHAR_POINTER filename)
{
    return IniFileDataBaseFindNextSectionNameRegExp(idx, filter, CaseSensetive, name, maxc, GetFileDescriptorByName(filename));
}
int LookIfSectionExist (const char *Section, const char *filename)
{
    return IniFileDataBaseLookIfSectionExist(Section, GetFileDescriptorByName(filename));
}

int DBGetSectionNumberOfEntrys (const char *section_in, const char *filename)
{
    return IniFileDataBaseGetSectionNumberOfEntrys(section_in, GetFileDescriptorByName(filename));
}


int save_all_infos2ini (void)
{
    WriteBasicConfigurationToIni(&s_main_ini_val);
    return 0;
}

void EnterIniDBCriticalSectionUser(const char *par_File, int par_Line)
{
    IniFileDataBaseEnterCriticalSectionUser(par_File, par_Line);
}

void LeaveIniDBCriticalSectionUser(void)
{
    IniFileDataBaseLeaveCriticalSectionUser();
}

int CreateAndOpenNewIniFile(char *par_Name, int par_MaxSize)
{
    int FileDescriptor = IniFileDataBaseCreateAndOpenNewIniFile(par_Name);
    if (FileDescriptor > 0) {
        if (IniFileDataBaseGetFileNameByDescriptor(FileDescriptor, par_Name, par_MaxSize)) {
            return -1;
        }
        return 0;
    }
    return -1;
}

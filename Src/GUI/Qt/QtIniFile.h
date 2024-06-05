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


#ifndef QTINIFILE_H
#define QTINIFILE_H

#include <QString>
#include <QImage>

extern "C" {
#include "IniDataBase.h"
}

int ScQt_IniFileDataBaseOpen(QString &par_Filename);
int ScQt_IniFileDataBaseClose(int par_FileDescriptor);
#define INIFILE_DATABAE_OPERATION_WRITE_ONLY  0
#define INIFILE_DATABAE_OPERATION_RENAME      1
#define INIFILE_DATABAE_OPERATION_REMOVE      2
int ScQt_IniFileDataBaseSave(int par_FileDescriptor, QString &par_DstFileName, int par_Operation);
int ScQt_IniFileDataBaseCreateAndOpenNewIniFile(QString &par_Name);

int ScQt_GetMainFileDescriptor(void);
void ScQt_SetMainFileDescriptor(int par_Fd);

// Write
bool ScQt_IniFileDataBaseWriteString (const char *section, const char *entry, QString &txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteString (const char *section, QString &entry, const char *txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteString (const char *section, const char *entry, const char *txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteString (QString &section, QString &entry, QString &txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteString (QString &section, const char *entry, QString &txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteString (QString &section, QString &entry, const char *txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteString (QString &section, const char *entry, const char *txt, int par_Fd);

bool ScQt_IniFileDataBaseWriteInt (QString &section_in, QString &entry, int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteInt (QString &section_in, const char *entry, int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteInt (const char *section_in, QString &entry, int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteInt (const char *section_in, const char *entry, int Value, int par_Fd);

bool ScQt_IniFileDataBaseWriteUInt (QString &section_in, QString &entry, unsigned int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteUInt (QString &section_in, const char *entry, unsigned int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteUInt (const char *section_in, QString &entry, unsigned int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteUInt (const char *section_in, const char *&entry, unsigned int Value, int par_Fd);

bool ScQt_IniFileDataBaseWriteUIntHex (QString &section_in, QString &entry, unsigned int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteUIntHex (QString &section_in, const char *entry, unsigned int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteUIntHex (const char *section_in, QString &entry, unsigned int Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteUIntHex (const char *section_in, const char *entry, unsigned int Value, int par_Fd);

bool ScQt_IniFileDataBaseWriteFloat (QString &section_in, QString &entry, double Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteFloat (QString &section_in, const char *entry, double Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteFloat (const char *section_in, QString &entry, double Value, int par_Fd);
bool ScQt_IniFileDataBaseWriteFloat (const char *&section_in,const char *&entry, double Value, int par_Fd);

bool ScQt_IniFileDataBaseWriteYesNo (QString &section_in, QString &entry, int Flag, int par_Fd);
bool ScQt_IniFileDataBaseWriteYesNo (QString &section_in, const char *entry, int Flag, int par_Fd);
bool ScQt_IniFileDataBaseWriteYesNo (const char *section_in, QString &entry, int Flag, int par_Fd);
bool ScQt_IniFileDataBaseWriteYesNo (const char *section_in, const char *entry, int Flag, int par_Fd);

bool ScQt_IniFileDataBaseWriteSection (QString &section, QString &txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteSection (QString &section, const char *txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteSection (const char *section, QString &txt, int par_Fd);
bool ScQt_IniFileDataBaseWriteSection (const char *section, const char *txt, int par_Fd);

// Read
QString ScQt_IniFileDataBaseReadString(QString &section_in, QString &entry, const char *deftxt, int par_Fd);
QString ScQt_IniFileDataBaseReadString(QString &section_in, const char *entry, const char *deftxt, int par_Fd);
QString ScQt_IniFileDataBaseReadString(const char *section_in, QString &entry, const char *deftxt, int par_Fd);
QString ScQt_IniFileDataBaseReadString(const char *section_in, const char *entry, const char *deftxt, int par_Fd);
int ScQt_IniFileDataBaseReadString(QString &section_in, const char *entry, const char *deftxt, char *txt, int maxc, int par_Fd);

int ScQt_IniFileDataBaseReadInt (QString &section, QString &entry, int defvalue, int par_Fd);
int ScQt_IniFileDataBaseReadInt (QString &section, const char *entry, int defvalue, int par_Fd);
int ScQt_IniFileDataBaseReadInt (const char *section, QString &entry, int defvalue, int par_Fd);
int ScQt_IniFileDataBaseReadInt (const char *section, const char *entry, int defvalue, int par_Fd);

unsigned int ScQt_IniFileDataBaseReadUInt (QString &section, QString &entry, unsigned int defvalue, int par_Fd);
unsigned int ScQt_IniFileDataBaseReadUInt (QString &section, const char *entry, unsigned int defvalue, int par_Fd);
unsigned int ScQt_IniFileDataBaseReadUInt (const char *&section, QString &entry, unsigned int defvalue, int par_Fd);
unsigned int ScQt_IniFileDataBaseReadUInt (const char *section, const char *entry, unsigned int defvalue, int par_Fd);

double ScQt_IniFileDataBaseReadFloat (QString &section_in, QString &entry, double defvalue, int par_Fd);
double ScQt_IniFileDataBaseReadFloat (QString &section_in, const char *entry, double defvalue, int par_Fd);
double ScQt_IniFileDataBaseReadFloat (const char *section_in, QString &entry, double defvalue, int par_Fd);
double ScQt_IniFileDataBaseReadFloat (const char *section_in, const char *&entry, double defvalue, int par_Fd);

int ScQt_IniFileDataBaseReadYesNo (QString &section, QString &entry, int defvalue, int par_Fd);
int ScQt_IniFileDataBaseReadYesNo (QString &section, const char *entry, int defvalue, int par_Fd);
int ScQt_IniFileDataBaseReadYesNo (const char *section, QString &entry, int defvalue, int par_Fd);
int ScQt_IniFileDataBaseReadYesNo (const char *section, const char *entry, int defvalue, int par_Fd);

bool ScQt_IniFileDataBaseWriteByteImage (QString &section_in, QString &entry_prefix, int type, void *image, int len, int par_Fd);

// If par_BufferSize <= 0 and ret_Buffer == NULL than only the needed buffer size will returned
// If par_BufferSize <= 0 and ret_Buffer != NULL than the buffer will be allocated, This buffer must be give free with ScQt_IniFileDataBaseFreeByteImageBuffer
int ScQt_IniFileDataBaseReadByteImage (QString &section_in, QString &entry_prefix, int *ret_type,
                                         void *ret_buffer, int ret_buffer_size, int par_Fd);
void ScQt_IniFileDataBaseFreeByteImageBuffer (void *buffer);


bool ScQt_IniFileDataBaseDeleteEntry(QString &section, QString &entry, int par_Fd);
bool ScQt_IniFileDataBaseDeleteSection(QString &section, int par_Fd);

QImage *ScQt_IniFileDataBaseReadImage(QString &section, QString &entry, int par_Fd);
bool ScQt_IniFileDataBaseWriteImage(QString &section, QString &entry, QString &image_filename, int par_Fd);

bool ScQt_IniFileDataBaseLookIfSectionExist(QString &par_Section, int par_Fd);
int ScQt_IniFileDataBaseFindNextSectionNameRegExp(int par_Index, QString &par_Filter, int par_CaseSensetive,
                                                  QString *ret_Name, int par_FileDescriptor);
int ScQt_IniFileDataBaseGetSectionNumberOfEntrys(QString &par_Section, int par_Fd);

int ScQt_IniFileDataBaseCopySection(int par_DstFileDescriptor, int par_SrcFileDescriptor,
                                    QString &par_DstSection, QString &par_SrcSection);
int ScQt_IniFileDataBaseCopySectionSameName(int par_DstFileDescriptor, int par_SrcFileDescriptor, QString &par_Section);

bool IsAValidWindowSectionName(QString &par_WindowName);

#endif // QTINIFILE_H

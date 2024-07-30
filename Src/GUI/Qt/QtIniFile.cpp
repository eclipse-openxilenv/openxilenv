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


#include "QtIniFile.h"
#include "StringHelpers.h"

#include <QFile>
#include <QDataStream>

extern "C" {
#include "IniDataBase.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "Files.h"
}

int ScQt_IniFileDataBaseOpen(QString &par_Filename)
{
    return IniFileDataBaseOpen(QStringToConstChar(par_Filename));
}

int ScQt_IniFileDataBaseOpenNoFilterPossible(QString &par_Filename)
{
    return IniFileDataBaseOpenNoFilterPossible(QStringToConstChar(par_Filename));
}

int ScQt_IniFileDataBaseClose(int par_FileDescriptor)
{
    return IniFileDataBaseClose(par_FileDescriptor);
}

#define INIFILE_DATABAE_OPERATION_WRITE_ONLY  0
#define INIFILE_DATABAE_OPERATION_RENAME      1
#define INIFILE_DATABAE_OPERATION_REMOVE      2
int ScQt_IniFileDataBaseSave(int par_FileDescriptor, QString &par_DstFileName, int par_Operation)
{
    return IniFileDataBaseSave(par_FileDescriptor, QStringToConstChar(par_DstFileName), par_Operation);
}


int ScQt_IniFileDataBaseCreateAndOpenNewIniFile(QString &par_Name)
{
    return IniFileDataBaseCreateAndOpenNewIniFile(QStringToConstChar(par_Name));
}

int ScQt_GetMainFileDescriptor(void)
{
    return GetMainFileDescriptor();
}

void ScQt_SetMainFileDescriptor(int par_Fd)
{
    SetMainFileDescriptor(par_Fd);
}

// Write
bool ScQt_IniFileDataBaseWriteString (QString &section, QString &entry, QString &txt, int par_Fd)
{
    return IniFileDataBaseWriteString(QStringToConstChar(section), QStringToConstChar(entry), QStringToConstChar(txt), par_Fd);
}
bool ScQt_IniFileDataBaseWriteString (QString &section, const char *entry, QString &txt, int par_Fd)
{
    return IniFileDataBaseWriteString(QStringToConstChar(section), entry, QStringToConstChar(txt), par_Fd);
}
bool ScQt_IniFileDataBaseWriteString(QString &section, QString &entry, const char *txt, int par_Fd)
{
    return IniFileDataBaseWriteString(QStringToConstChar(section), QStringToConstChar(entry), txt, par_Fd);
}
bool ScQt_IniFileDataBaseWriteString (QString &section, const char *entry, const char *txt, int par_Fd)
{
    return IniFileDataBaseWriteString(QStringToConstChar(section), entry, txt, par_Fd);
}
bool ScQt_IniFileDataBaseWriteString (const char *section, const char *entry, QString &txt, int par_Fd)
{
    return IniFileDataBaseWriteString(section, entry, QStringToConstChar(txt), par_Fd);
}
bool ScQt_IniFileDataBaseWriteString (const char *section, QString &entry, const char *txt, int par_Fd)
{
    return IniFileDataBaseWriteString(section, QStringToConstChar(entry), txt, par_Fd);
}
bool ScQt_IniFileDataBaseWriteString (const char *section, const char *entry, const char *txt, int par_Fd)
{
    return IniFileDataBaseWriteString(section, entry, txt, par_Fd);
}

bool ScQt_IniFileDataBaseWriteInt (QString &section_in, QString &entry, int Value, int par_Fd)
{
    return IniFileDataBaseWriteInt(QStringToConstChar(section_in), QStringToConstChar(entry), Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteInt (QString &section_in, const char *entry, int Value, int par_Fd)
{
    return IniFileDataBaseWriteInt(QStringToConstChar(section_in), entry, Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteInt (const char *section_in, QString &entry, int Value, int par_Fd)
{
    return IniFileDataBaseWriteInt(section_in, QStringToConstChar(entry), Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteInt (const char *section_in, const char *entry, int Value, int par_Fd)
{
    return IniFileDataBaseWriteInt(section_in, entry, Value, par_Fd);
}

bool ScQt_IniFileDataBaseWriteUInt (QString &section_in, QString &entry, unsigned int Value, int par_Fd)
{
    return IniFileDataBaseWriteUInt(QStringToConstChar(section_in), QStringToConstChar(entry), Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteUInt (QString &section_in, const char *entry, unsigned int Value, int par_Fd)
{
    return IniFileDataBaseWriteUInt(QStringToConstChar(section_in), entry, Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteUInt (const char *section_in, QString &entry, unsigned int Value, int par_Fd)
{
    return IniFileDataBaseWriteUInt(section_in, QStringToConstChar(entry), Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteUInt (const char *section_in, const char *&entry, unsigned int Value, int par_Fd)
{
    return IniFileDataBaseWriteUInt(section_in, entry, Value, par_Fd);
}


bool ScQt_IniFileDataBaseWriteUIntHex (QString &section_in, QString &entry, unsigned int Value, int par_Fd)
{
    return IniFileDataBaseWriteUIntHex(QStringToConstChar(section_in), QStringToConstChar(entry), Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteUIntHex (QString &section_in, const char *entry, unsigned int Value, int par_Fd)
{
    return IniFileDataBaseWriteUIntHex(QStringToConstChar(section_in), entry, Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteUIntHex (const char *section_in, QString &entry, unsigned int Value, int par_Fd)
{
    return IniFileDataBaseWriteUIntHex(section_in, QStringToConstChar(entry), Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteUIntHex (const char *section_in, const char *entry, unsigned int Value, int par_Fd)
{
    return IniFileDataBaseWriteUIntHex(section_in, entry, Value, par_Fd);
}

bool ScQt_IniFileDataBaseWriteFloat (QString &section_in, QString &entry, double Value, int par_Fd)
{
    return IniFileDataBaseWriteFloat(QStringToConstChar(section_in), QStringToConstChar(entry), Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteFloat (QString &section_in, const char *entry, double Value, int par_Fd)
{
    return IniFileDataBaseWriteFloat(QStringToConstChar(section_in), entry, Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteFloat (const char *section_in, QString &entry, double Value, int par_Fd)
{
    return IniFileDataBaseWriteFloat(section_in, QStringToConstChar(entry), Value, par_Fd);
}
bool ScQt_IniFileDataBaseWriteFloat (const char *&section_in,const char *&entry, double Value, int par_Fd)
{
    return IniFileDataBaseWriteFloat(section_in, entry, Value, par_Fd);
}

bool ScQt_IniFileDataBaseWriteYesNo (QString &section_in, QString &entry, int Flag, int par_Fd)
{
    return IniFileDataBaseWriteYesNo(QStringToConstChar(section_in), QStringToConstChar(entry), Flag, par_Fd);
}
bool ScQt_IniFileDataBaseWriteYesNo (QString &section_in, const char *entry, int Flag, int par_Fd)
{
    return IniFileDataBaseWriteYesNo(QStringToConstChar(section_in), entry, Flag, par_Fd);
}
bool ScQt_IniFileDataBaseWriteYesNo (const char *section_in, QString &entry, int Flag, int par_Fd)
{
    return IniFileDataBaseWriteYesNo(section_in, QStringToConstChar(entry), Flag, par_Fd);
}
bool ScQt_IniFileDataBaseWriteYesNo (const char *section_in, const char *entry, int Flag, int par_Fd)
{
    return IniFileDataBaseWriteYesNo(section_in, entry, Flag, par_Fd);
}


bool ScQt_IniFileDataBaseWriteSection (QString &section, QString &txt, int par_Fd)
{
    return IniFileDataBaseWriteSection(QStringToConstChar(section), QStringToConstChar(txt), par_Fd);
}
bool ScQt_IniFileDataBaseWriteSection (QString &section, const char *txt, int par_Fd)
{
    return IniFileDataBaseWriteSection(QStringToConstChar(section), txt, par_Fd);
}
bool ScQt_IniFileDataBaseWriteSection (const char *section, QString &txt, int par_Fd)
{
    return IniFileDataBaseWriteSection(section, QStringToConstChar(txt), par_Fd);
}
bool ScQt_IniFileDataBaseWriteSection (const char *section, const char *txt, int par_Fd)
{
    return IniFileDataBaseWriteSection(section, txt, par_Fd);
}

// Read
QString ScQt_IniFileDataBaseReadString(QString &section_in, QString &entry, const char *deftxt, int par_Fd)
{
    char *Buffer;
    Buffer = IniFileDataBaseReadStringBufferNoDef(QStringToConstChar(section_in), QStringToConstChar(entry), par_Fd);
    QString Ret;
    if (Buffer != nullptr) {
        Ret = QString().fromLatin1(Buffer);
        IniFileDataBaseReadStringBufferFree(Buffer);
    } else {
        Ret = QString(deftxt);
    }
    return Ret;
}
QString ScQt_IniFileDataBaseReadString(QString &section_in, const char *entry, const char *deftxt, int par_Fd)
{
    char *Buffer;
    Buffer = IniFileDataBaseReadStringBufferNoDef(QStringToConstChar(section_in), entry, par_Fd);
    QString Ret;
    if (Buffer != nullptr) {
        Ret = QString().fromLatin1(Buffer);
        IniFileDataBaseReadStringBufferFree(Buffer);
    } else {
        Ret = QString(deftxt);
    }
    return Ret;
}
QString ScQt_IniFileDataBaseReadString(const char *section_in, QString &entry, const char *deftxt, int par_Fd)
{
    char *Buffer;
    Buffer = IniFileDataBaseReadStringBufferNoDef(section_in, QStringToConstChar(entry), par_Fd);
    QString Ret;
    if (Buffer != nullptr) {
        Ret = QString().fromLatin1(Buffer);
        IniFileDataBaseReadStringBufferFree(Buffer);
    } else {
        Ret = QString(deftxt);
    }
    return Ret;
}
QString ScQt_IniFileDataBaseReadString(const char *section_in, const char *entry, const char *deftxt, int par_Fd)
{
    char *Buffer;
    Buffer = IniFileDataBaseReadStringBufferNoDef(section_in, entry, par_Fd);
    QString Ret;
    if (Buffer != nullptr) {
        Ret = QString().fromLatin1(Buffer);
        IniFileDataBaseReadStringBufferFree(Buffer);
    } else {
        Ret = QString(deftxt);
    }
    return Ret;
}
int ScQt_IniFileDataBaseReadString(QString &section_in, const char *entry, const char *deftxt, char *txt, int nsize, int par_Fd)
{
    return IniFileDataBaseReadString(QStringToConstChar(section_in), entry, deftxt, txt, nsize, par_Fd);
}


int ScQt_IniFileDataBaseReadInt (QString &section, QString &entry, int defvalue, int par_Fd)
{
    return IniFileDataBaseReadInt(QStringToConstChar(section), QStringToConstChar(entry), defvalue, par_Fd);
}
int ScQt_IniFileDataBaseReadInt (QString &section, const char *entry, int defvalue, int par_Fd)
{
    return IniFileDataBaseReadInt(QStringToConstChar(section), entry, defvalue, par_Fd);
}
int ScQt_IniFileDataBaseReadInt (const char *section, QString &entry, int defvalue, int par_Fd)
{
    return IniFileDataBaseReadInt(section, QStringToConstChar(entry), defvalue, par_Fd);
}
int ScQt_IniFileDataBaseReadInt (const char *section, const char *entry, int defvalue, int par_Fd)
{
    return IniFileDataBaseReadInt(section, entry, defvalue, par_Fd);
}

unsigned int ScQt_IniFileDataBaseReadUInt (QString &section, QString &entry, unsigned int defvalue, int par_Fd)
{
    return IniFileDataBaseReadUInt(QStringToConstChar(section), QStringToConstChar(entry), defvalue, par_Fd);
}
unsigned int ScQt_IniFileDataBaseReadUInt (QString &section, const char *entry, unsigned int defvalue, int par_Fd)
{
    return IniFileDataBaseReadUInt(QStringToConstChar(section), entry, defvalue, par_Fd);
}
unsigned int ScQt_IniFileDataBaseReadUInt (const char *&section, QString &entry, unsigned int defvalue, int par_Fd)
{
    return IniFileDataBaseReadUInt(section, QStringToConstChar(entry), defvalue, par_Fd);
}
unsigned int ScQt_IniFileDataBaseReadUInt (const char *section, const char *entry, unsigned int defvalue, int par_Fd)
{
    return IniFileDataBaseReadUInt(section, entry, defvalue, par_Fd);
}

double ScQt_IniFileDataBaseReadFloat (QString &section_in, QString &entry, double defvalue, int par_Fd)
{
    return IniFileDataBaseReadFloat(QStringToConstChar(section_in), QStringToConstChar(entry), defvalue, par_Fd);
}
double ScQt_IniFileDataBaseReadFloat (QString &section_in, const char *entry, double defvalue, int par_Fd)
{
    return IniFileDataBaseReadFloat(QStringToConstChar(section_in), entry, defvalue, par_Fd);
}
double ScQt_IniFileDataBaseReadFloat (const char *section_in, QString &entry, double defvalue, int par_Fd)
{
    return IniFileDataBaseReadFloat(section_in, QStringToConstChar(entry), defvalue, par_Fd);
}
double ScQt_IniFileDataBaseReadFloat (const char *section_in, const char *&entry, double defvalue, int par_Fd)
{
    return IniFileDataBaseReadFloat(section_in, entry, defvalue, par_Fd);
}

int ScQt_IniFileDataBaseReadYesNo (QString &section, QString &entry, int defvalue, int par_Fd)
{
    return IniFileDataBaseReadYesNo(QStringToConstChar(section), QStringToConstChar(entry), defvalue, par_Fd);
}
int ScQt_IniFileDataBaseReadYesNo (QString &section, const char *entry, int defvalue, int par_Fd)
{
    return IniFileDataBaseReadYesNo(QStringToConstChar(section), entry, defvalue, par_Fd);
}
int ScQt_IniFileDataBaseReadYesNo (const char *section, QString &entry, int defvalue, int par_Fd)
{
    return IniFileDataBaseReadYesNo(section, QStringToConstChar(entry), defvalue, par_Fd);
}
int ScQt_IniFileDataBaseReadYesNo (const char *section, const char *entry, int defvalue, int par_Fd)
{
    return IniFileDataBaseReadYesNo(section, entry, defvalue, par_Fd);
}


bool ScQt_IniFileDataBaseWriteByteImage(QString &section_in, QString &entry_prefix, int type, void *image, int len, int par_Fd)
{
    if (!IniFileDataBaseWriteByteImage(QStringToConstChar(section_in), QStringToConstChar(entry_prefix), type, image, len, par_Fd)) {
        return false;
    }
    return true;
}

int ScQt_IniFileDataBaseReadByteImage(QString &section_in, QString &entry_prefix, int *ret_type, void *ret_buffer, int ret_buffer_size, int par_Fd)
{
    return IniFileDataBaseReadByteImage(QStringToConstChar(section_in), QStringToConstChar(entry_prefix), ret_type, ret_buffer, ret_buffer_size, par_Fd);
}

void ScQt_IniFileDataBaseFreeByteImageBuffer(void *buffer)
{
    IniFileDataBaseFreeByteImageBuffer(buffer);
}



bool ScQt_IniFileDataBaseDeleteEntry(QString &section, QString &entry, int par_Fd)
{
    return IniFileDataBaseWriteString(QStringToConstChar(section), QStringToConstChar(entry), nullptr, par_Fd);
}

bool ScQt_IniFileDataBaseDeleteSection(QString &section, int par_Fd)
{
    return IniFileDataBaseWriteString(QStringToConstChar(section), nullptr, nullptr, par_Fd);
}


QImage *ScQt_IniFileDataBaseReadImage(QString &section, QString &entry, int par_Fd)
{
    QImage *Ret = nullptr;
    uchar *ImageBuffer;
    int Len;
    int Type;
    Len = IniFileDataBaseReadByteImage (QStringToConstChar(section), QStringToConstChar(entry), &Type,
                                        (void *)&ImageBuffer, 0, par_Fd);
    if (Len > 0) {
        Ret = new QImage();
        if (!Ret->loadFromData(ImageBuffer, Len)) {
            delete Ret;
            Ret = nullptr;
        }
        IniFileDataBaseFreeByteImageBuffer(ImageBuffer);
    }
    return Ret;
}

bool ScQt_IniFileDataBaseWriteImage(QString &section, QString &entry, QString &image_filename, int par_Fd)
{
    char *ImageBuffer;
    int Len;

    QFile File(image_filename);

    if (!File.open(QIODevice::ReadOnly)) {
        return false;
    }
    LogFileAccess (QStringToConstChar(image_filename));
    Len = File.size();
    ImageBuffer = (char*)my_malloc(Len);
    if (ImageBuffer == nullptr) {
        ThrowError (1, "cannot allocate %i bytes for image \"%s\" buffer", Len, QStringToConstChar(image_filename));
        File.close();
        return false;
    }
    QDataStream Out(&File);
    if (Out.readRawData(ImageBuffer, Len) != Len) {
        ThrowError (1, "cannot read %i bytes for image \"%s\"", Len, QStringToConstChar(image_filename));
        File.close();
    }
    if (!IniFileDataBaseWriteByteImage (QStringToConstChar(section), QStringToConstChar(entry), 1000, ImageBuffer, Len, par_Fd)) {
        my_free(ImageBuffer);
        File.close();
        return false;
    }
    my_free(ImageBuffer);
    File.close();
    return true;
}

bool IsAValidWindowSectionName(QString &par_WindowName)
{
    int Len = par_WindowName.size();
    if (Len >= (INI_MAX_SECTION_LENGTH - strlen("GUI\\Widgets"))) {
        return false;
    }
    for(int x = 0; x < Len; x++) {
    QChar Char = par_WindowName.at(x);
        if ((Char < ' ') || (Char > '~') ||  // only ascii characters are allowed
            (Char == '[') || (Char == '[') || (Char == '\\')) {   // and no []/ characters
            return false;
        }
    }
    return true;
}

bool ScQt_IniFileDataBaseLookIfSectionExist(QString &par_Section, int par_Fd)
{
    return IniFileDataBaseLookIfSectionExist(QStringToConstChar(par_Section), par_Fd);
}

int ScQt_IniFileDataBaseFindNextSectionNameRegExp(int par_Index, QString &par_Filter, int par_CaseSensetive,
                                                  QString *ret_Name, int par_FileDescriptor)
{
    int Ret;
    char Section[INI_MAX_SECTION_LENGTH];

    Ret = IniFileDataBaseFindNextSectionNameRegExp(par_Index, QStringToConstChar(par_Filter), par_CaseSensetive,
                                                   Section, sizeof(Section), par_FileDescriptor);
    if (Ret >= 0) {
        *ret_Name = CharToQString(Section);
    }
    return Ret;
}

int ScQt_IniFileDataBaseGetSectionNumberOfEntrys(QString &par_Section, int par_Fd)
{
    return IniFileDataBaseGetSectionNumberOfEntrys(QStringToConstChar(par_Section), par_Fd);
}

int ScQt_IniFileDataBaseCopySection (int par_DstFileDescriptor, int par_SrcFileDescriptor,
                                     QString &par_DstSection, QString &par_SrcSection)
{
    return IniFileDataBaseCopySection(par_DstFileDescriptor, par_SrcFileDescriptor,
                                      QStringToConstChar(par_DstSection), QStringToConstChar(par_SrcSection));
}

int ScQt_IniFileDataBaseCopySectionSameName (int par_DstFileDescriptor, int par_SrcFileDescriptor, QString &par_Section)
{
    return IniFileDataBaseCopySectionSameName (par_DstFileDescriptor, par_SrcFileDescriptor, QStringToConstChar(par_Section));
}


#if 0
int QStringToIniFileString(QString *par_String, char *ret_String, int max_Len)
{
    int Count = 0;
    char *d = ret_String;
    int Size = par_String->size();
    for (int x = 0; x < Size; x++) {
        char16_t Char = par_String->at(x).unicode();
        if ((Char >= ' ') && (Char <= '~') &&
            (Char != '[') && (Char != '[') && (Char != '\\')) {
            Count += 5;
            if (Count < max_Len) {
                *d = (char)Char;
                d++;
            }
        } else {   // all other chars will be replaced with "\xxxx" (xx will be the 16bit hex value of the char)
            unsigned char n;
            Count += 5;
            if (Count < max_Len) {
                uint8_t n;
                *d++ = '\\';
                n = Char >> 12;
                if (n < 10) {
                    *d = '0' + n;
                } else {
                    *d = ('a' - 10) + n;
                }
                n = (Char >> 8) & 0xF;
                if (n < 10) {
                    *d = '0' + n;
                } else {
                    *d = ('a' - 10) + n;
                }
                n = (Char >> 4) & 0xF;
                if (n < 10) {
                    *d = '0' + n;
                } else {
                    *d = ('a' - 10) + n;
                }
                d++;
                n = Char & 0xF;
                if (n < 10) {
                    *d = '0' + n;
                } else {
                    *d = ('a' - 10) + n;
                }
                d++;
            }
        }
    }
    Count++;
    *d = 0;
    return Count;
}
#endif

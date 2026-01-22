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
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#include <time.h>
#endif
#include "Config.h"
#include "Platform.h"
#include "Files.h"
#include "ReadFromBlackboardPipe.h"
#include "TraceRecorder.h"
#include "StimulusPlayer.h"
#include "Blackboard.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "EnvironmentVariables.h"
#include "PrintFormatToString.h"
#include "ThrowError.h"
#include "StimulusReadFile.h"
#include "MdfStructs.h"
#include "BlackboardConvertFromTo.h"
#include "StimulusReadMdfFile.h"


int IsMdfFormat(const char *par_Filename, int *ret_Version)
{
    MY_FILE_HANDLE fh;
    MDF_IDBLOCK MdfIdBlock;
    int ReadResult;
    char Filename[MAX_PATH];

    SearchAndReplaceEnvironmentStrings(par_Filename, Filename, sizeof(Filename));

    fh = my_open(Filename);
    if (fh == MY_INVALID_HANDLE_VALUE) {
        return 0;
    }
    LogFileAccess (Filename);
    ReadResult = my_read (fh, &MdfIdBlock, sizeof (MdfIdBlock));
    if (ReadResult != sizeof (MdfIdBlock)) {
        my_close (fh);
        return 0;
    }
    if (strncmp("MDF     ", MdfIdBlock.FileIdentifier, 8)) {
        my_close (fh);
        return 0;
    }
    if ((MdfIdBlock.VersionNumber < 300) || (MdfIdBlock.VersionNumber > 330) ||
        !(!strncmp("3.00 ", MdfIdBlock.FormatTdentifier, 5) ||
          !strncmp("3.10 ", MdfIdBlock.FormatTdentifier, 5) ||
          !strncmp("3.20 ", MdfIdBlock.FormatTdentifier, 5) ||
          !strncmp("3.30 ", MdfIdBlock.FormatTdentifier, 5))) {
        my_close (fh);
        return 0;
    }
    if (ret_Version != NULL) *ret_Version = MdfIdBlock.VersionNumber;

    if (MdfIdBlock.DefaultByteOrder != 0) {
        ThrowError (1, "%s file are not in little endian (lsb first order)\n", Filename);
        my_close (fh);
        return 0;
    }
    my_close (fh);
    return 1;
}

static char *ReadString(MY_FILE_HANDLE fh, uint32_t par_Offset)
{
    char *Ret = NULL;
    int ReadResult;
    uint64_t OldPos;
    MDF_TXBLOCK MdfTxBlock;

    OldPos = my_ftell(fh);
    my_lseek(fh, par_Offset);
    ReadResult = my_read (fh, &MdfTxBlock, sizeof (MdfTxBlock));
    if (ReadResult == sizeof (MdfTxBlock)) {
        if (!strncmp("TX", MdfTxBlock.BlockTypeIdentifier, 2)) {
            Ret = my_malloc(MdfTxBlock.BlockSize - sizeof(MdfTxBlock));
            if (Ret != NULL) {
                ReadResult = my_read (fh, Ret, MdfTxBlock.BlockSize - sizeof(MdfTxBlock));
                if (ReadResult != (int)MdfTxBlock.BlockSize - (int)sizeof(MdfTxBlock)) {
                    my_free(Ret);
                    Ret = NULL;
                }
            }
        }
    }
    my_lseek(fh, OldPos);
    return Ret;
}

uint64_t ConvertDateTimeStringToTimeStamp(char *par_Date, char *par_Time)
{
    char *p;
#ifdef _WIN32
    SYSTEMTIME st;
    FILETIME ft;

    MEMSET(&st, 0, sizeof(st));
    p = par_Date;
    st.wDay = (WORD)strtoul(p, &p, 10);
    if (*p++ != ':') return 0;
    st.wMonth = (WORD)strtoul(p, &p, 10);
    if (*p++ != ':') return 0;
    st.wYear = (WORD)strtoul(p, &p, 10);
    if (*p != 0) return 0;
    p = par_Time;
    st.wHour = (WORD)strtoul(p, &p, 10);
    if (*p++ != ':') return 0;
    st.wMinute = (WORD)strtoul(p, &p, 10);
    if (*p++ != ':') return 0;
    st.wSecond = (WORD)strtoul(p, &p, 10);
    if (*p != 0) return 0;
    SystemTimeToFileTime(&st, &ft);

    return ((uint64_t)ft.dwHighDateTime << 32) + (uint64_t)ft.dwLowDateTime;
#else
    struct tm tm_time;
    time_t time_t_time;

    MEMSET(&tm_time, 0, sizeof(tm_time));
    p = par_Date;
    tm_time.tm_mday = (int)strtoul(p, &p, 10);
    if (*p++ != ':') return 0;
    tm_time.tm_mon = (int)strtoul(p, &p, 10);
    if (*p++ != ':') return 0;
    tm_time.tm_year = (int)strtoul(p, &p, 10);
    if (*p != 0) return 0;
    p = par_Time;
    tm_time.tm_hour = (int)strtoul(p, &p, 10);
    if (*p++ != ':') return 0;
    tm_time.tm_min = (int)strtoul(p, &p, 10);
    if (*p++ != ':') return 0;
    tm_time.tm_sec = (int)strtoul(p, &p, 10);
    if (*p != 0) return 0;
    time_t_time = mktime (&tm_time);
    return (uint64_t)time_t_time;
#endif
}

static int IsValidDataType(int par_DataType, int par_BitSize)
{
    switch (par_DataType) {
    case 0: //    0 = unsigned integer
        if (par_BitSize <= 64) return 1;
        else return 0;
    case 1: //    1 = signed integer (twos complement)
        if (par_BitSize <= 64) return 1;
        else return 0;
    case 2: //    2 = IEEE 754 floating-point format FLOAT (4 bytes)
        if (par_BitSize == 32) return 1;
        else return 0;
    case 3: //    3 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
        if (par_BitSize == 64) return 1;
        else return 0;
    default:
    //    Default Byte order from IDBLOCK
    //    4 = VAX floating-point format (F_Float)
    //    5 = VAX floating-point format (G_Float)
    //    6 = VAX floating-point format (D_Float)
    //    obsolete
    //    7 = String (NULL terminated)
    //    8 = Byte Array (max. 8191 Bytes, constant record length!)
    //    9 = unsigned integer
    //    10 = signed integer (twos complement)
    //    11 = IEEE 754 floating-point format FLOAT (4 bytes)
    //    12 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
    //    Big Endian Byte order
    //    13 = unsigned integer
    //    14 = signed integer (twos complement)
    //    15 = IEEE 754 floating-point format FLOAT (4 bytes)
    //    16 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
        return 0;
    }
}

static int IsInsideList(const char *par_List, const char *par_Variable)
{
    if ((par_List != NULL) && (par_Variable != NULL)) {
        const char *p = strstr(par_List, par_Variable);
        if (p != NULL) {
            p += strlen(par_Variable);
            if ((*p == ';') || (*p == 0)) {
                return 1;
            }
        }
    }
    return 0;
}

/* This will read all variable names from a stimulus files  (with ; separation) */
char *MdfReadStimulHeaderVariabeles (const char *par_Filename)
{
    int VariableCount;
    char *Ret = NULL;
    int LenOfRet = 0;
    int Pos;

    MY_FILE_HANDLE fh;
    MDF_IDBLOCK MdfIdBlock;
    MDF_HDBLOCK MdfHdBlock;
    MDF_DGBLOCK MdfDgBlock;
    MDF_CGBLOCK MdfCgBlock;
    MDF_CNBLOCK MdfCnBlock;
    uint32_t DataGroupBlockOffset;
    uint32_t ChannelGroupBlockOffset;
    uint32_t ChannelBlockOffset;
    int ReadResult;
    char Filename[MAX_PATH];

    SearchAndReplaceEnvironmentStrings(par_Filename, Filename, sizeof(Filename));

    fh = my_open(Filename);
    if (fh == MY_INVALID_HANDLE_VALUE) {
        return NULL;
    }
    LogFileAccess (Filename);
    ReadResult = my_read (fh, &MdfIdBlock, sizeof (MdfIdBlock));
    if (ReadResult != sizeof (MdfIdBlock)) {
        my_close (fh);
        return NULL;
    }
    if (strncmp("MDF     ", MdfIdBlock.FileIdentifier, 8)) {
        my_close (fh);
        return NULL;
    }
    ReadResult = my_read (fh, &MdfHdBlock, sizeof (MdfHdBlock));
    if ((ReadResult != sizeof (MdfHdBlock)) && (MdfHdBlock.BlockSize != sizeof (MdfHdBlock))) {
        my_close (fh);
        return NULL;
    }
    if (strncmp("HD", MdfHdBlock.BlockTypeIdentifier, 2)) {
        my_close (fh);
        return NULL;
    }
    if (MdfHdBlock.FirstDataGroupBlock == 0) {  // There must be exist at least one
        my_close (fh);
        return NULL;
    }

    VariableCount = 0;
    DataGroupBlockOffset = MdfHdBlock.FirstDataGroupBlock;
    while (DataGroupBlockOffset) {
        my_lseek(fh, DataGroupBlockOffset);
        ReadResult = my_read (fh, &MdfDgBlock, sizeof (MdfDgBlock));
        if ((ReadResult != sizeof (MdfDgBlock)) && (MdfDgBlock.BlockSize != sizeof (MdfDgBlock))) {
            my_close (fh);
            return NULL;
        }
        if (strncmp("DG", MdfDgBlock.BlockTypeIdentifier, 2)) {
            my_close (fh);
            return NULL;
        }
        if (MdfDgBlock.DataBlock != 0) {
            ChannelGroupBlockOffset = MdfDgBlock.FirstChannelGroupBlock;
            while (ChannelGroupBlockOffset) {
                my_lseek(fh, ChannelGroupBlockOffset);
                ReadResult = my_read (fh, &MdfCgBlock, sizeof (MdfCgBlock));
                if ((ReadResult != sizeof (MdfCgBlock)) && (MdfCgBlock.BlockSize != sizeof (MdfCgBlock))) {
                    my_close (fh);
                    return NULL;
                }
                if (strncmp("CG", MdfCgBlock.BlockTypeIdentifier, 2)) {
                    my_close (fh);
                    return NULL;
                }

                ChannelBlockOffset = MdfCgBlock.FirstChannelBlock;
                while (ChannelBlockOffset) {
                    my_lseek(fh, ChannelBlockOffset);
                    ReadResult = my_read (fh, &MdfCnBlock, sizeof (MdfCnBlock));
                    if ((ReadResult != sizeof (MdfCnBlock)) && (MdfCnBlock.BlockSize != sizeof (MdfCnBlock))) {
                        my_close (fh);
                        return NULL;
                    }
                    if (strncmp("CN", MdfCnBlock.BlockTypeIdentifier, 2)) {
                        my_close (fh);
                        return NULL;
                    }

                    if (MdfCnBlock.ChannelType == 0) {
                        if (IsValidDataType(MdfCnBlock.SignalDataType, MdfCnBlock.NumberOfBits)) {
                            char *SignalName;
                            if (MdfCnBlock.LongSignalName != 0) {
                                // SignalName have to give free with my_free
                                SignalName = ReadString(fh, MdfCnBlock.LongSignalName);
                            } else {
                                SignalName = MdfCnBlock.ShortSignalName;
                            }

                            if (!IsInsideList(Ret, SignalName)) {
                                Pos = LenOfRet;
                                if (VariableCount) {
                                    LenOfRet += 1;   // ';'
                                }
                                LenOfRet += (int)strlen (SignalName);
                                Ret = my_realloc (Ret, LenOfRet + 1);
                                if (Ret == NULL) {
                                    ThrowError (1, "upps out of memory");
                                    return NULL;
                                }
                                if (VariableCount) {
                                    StringCopyMaxCharTruncate (Ret + Pos, ";", (LenOfRet + 1) - Pos);
                                    Pos += 1;
                                }
                                StringCopyMaxCharTruncate (Ret + Pos, SignalName, (LenOfRet + 1) - Pos);
                                VariableCount++;
                            }

                            if (SignalName != MdfCnBlock.ShortSignalName) {
                                my_free(SignalName);
                            }
                        }
                    } else {
                    }
                    ChannelBlockOffset = MdfCnBlock.NextChannelBlock;
                }
                ChannelGroupBlockOffset = MdfCgBlock.NextChannelGroupBlock;
            }
        }
        DataGroupBlockOffset = MdfDgBlock.NextDataGroupBlock;
    }
    my_close (fh);
    return Ret;    /* all OK */
}

static void CleanCache(MDF_STIMULI_FILE *par_Mdf)
{
    if (par_Mdf->Cache.DataBlocks != NULL) {
        int x;
        for (x = 0; x < par_Mdf->Cache.NumberOfDataBlocks; x++) {
            if (par_Mdf->Cache.DataBlocks[x].BlockData != NULL) {
                my_free (par_Mdf->Cache.DataBlocks[x].BlockData);
            }
            if (par_Mdf->Cache.DataBlocks[x].Records != NULL) {
                int y;
                for (y = 0; y < par_Mdf->Cache.DataBlocks[x].NumberOfChannelGroups; y++) {
                    ;
                }
                my_free (par_Mdf->Cache.DataBlocks[x].Records);
            }
        }
        my_free (par_Mdf->Cache.DataBlocks);
    }

}

static void ErrorCleanCache(MDF_STIMULI_FILE *par_Mdf, const char *par_Error)
{
    ThrowError (1, "inside MDF cache: \"%s\"", par_Error);
    CleanCache(par_Mdf);
}

static int AddDataBlockToCache(MDF_STIMULI_FILE *par_Mdf, uint32_t par_OffsetInsideFile, uint16_t par_NumberOfRecordIDs)
{
    par_Mdf->Cache.NumberOfDataBlocks++;
    par_Mdf->Cache.DataBlocks = (MDF_DATA_BLOCK_CACHE*)my_realloc(par_Mdf->Cache.DataBlocks, par_Mdf->Cache.NumberOfDataBlocks * sizeof (MDF_DATA_BLOCK_CACHE));
    if (par_Mdf->Cache.DataBlocks == NULL) {
        ErrorCleanCache(par_Mdf, "out of memory");
        return -1;
    }
    MEMSET(&(par_Mdf->Cache.DataBlocks[par_Mdf->Cache.NumberOfDataBlocks - 1]), 0, sizeof (MDF_DATA_BLOCK_CACHE));
    par_Mdf->Cache.DataBlocks[par_Mdf->Cache.NumberOfDataBlocks - 1].OffsetInsideFile = par_OffsetInsideFile;
    par_Mdf->Cache.DataBlocks[par_Mdf->Cache.NumberOfDataBlocks - 1].CurrentFilePos = par_OffsetInsideFile;
    par_Mdf->Cache.DataBlocks[par_Mdf->Cache.NumberOfDataBlocks - 1].EndCacheFilePos = par_OffsetInsideFile;
    par_Mdf->Cache.DataBlocks[par_Mdf->Cache.NumberOfDataBlocks - 1].StartCacheFilePos = par_OffsetInsideFile;
    par_Mdf->Cache.DataBlocks[par_Mdf->Cache.NumberOfDataBlocks - 1].NumberOfRecordIDs = par_NumberOfRecordIDs;
    return par_Mdf->Cache.NumberOfDataBlocks - 1;
}

static int AddChannelGroupToCache(MDF_STIMULI_FILE *par_Mdf, int par_BlockNumber, int par_ChannelId, int par_SizeOfDataRecord, uint32_t par_NumberOfRecords)
{
    MDF_DATA_BLOCK_CACHE *DataBlock;
    if (par_BlockNumber >= par_Mdf->Cache.NumberOfDataBlocks) {
        ErrorCleanCache(par_Mdf, "par_BlockNumber >= Mdf->Cache.NumberOfDataBlocks");
        return -1;
    }
    DataBlock = &(par_Mdf->Cache.DataBlocks[par_BlockNumber]);
    DataBlock->NumberOfChannelGroups++;
    DataBlock->Records = (MDF_RECORD_CACHE*)my_realloc(DataBlock->Records, DataBlock->NumberOfChannelGroups * sizeof(MDF_RECORD_CACHE));
    if (DataBlock->Records == NULL) {
        ErrorCleanCache(par_Mdf, "out of memory");
        return -1;
    }
    MEMSET(&DataBlock->Records[DataBlock->NumberOfChannelGroups - 1], 0, sizeof(MDF_RECORD_CACHE));
    DataBlock->Records[DataBlock->NumberOfChannelGroups - 1].Id = par_ChannelId;
    DataBlock->Records[DataBlock->NumberOfChannelGroups - 1].SizeOfOneRecord = par_SizeOfDataRecord;
    DataBlock->Records[DataBlock->NumberOfChannelGroups - 1].NumberOfRecords = par_NumberOfRecords;
    return DataBlock->NumberOfChannelGroups - 1;
}

static int TranslateChannelIdToIndex(MDF_DATA_BLOCK_CACHE *par_DataBlock, int par_Id)
{
    int x;
    for (x = 0; x < par_DataBlock->NumberOfChannelGroups; x++) {
        if (par_DataBlock->Records[x].Id == par_Id) {
            return x;
        }
    }
    ThrowError (1, "cannot translate record ID=%i too an index", par_Id);
    return -1;
}

static int AddVariableToCache(MDF_STIMULI_FILE *par_Mdf, int par_BlockNumber, int par_ChannelId, int par_Vid,
                              int par_DataType, int par_BbDataType, int par_NumberOfBits, int par_StartBitOffset,
                              int par_TimeVarFlag, double par_TimeFac, double par_TimeOff)
{
    MDF_DATA_BLOCK_CACHE *DataBlock;
    MDF_RECORD_CACHE *Record;
    int Idx;

    if (par_BlockNumber >= par_Mdf->Cache.NumberOfDataBlocks) {
        ErrorCleanCache(par_Mdf, "par_BlockNumber >= Mdf->Cache.NumberOfDataBlocks");
        return -1;
    }
    DataBlock = &(par_Mdf->Cache.DataBlocks[par_BlockNumber]);
    Idx = TranslateChannelIdToIndex(DataBlock, par_ChannelId);
    if (Idx >= DataBlock->NumberOfChannelGroups) {
        ErrorCleanCache(par_Mdf, "Idx >= DataBlock->NumberOfRecords");
        return -1;
    }
    Record = &(DataBlock->Records[Idx]);
    if (par_TimeVarFlag) {
        Record->TimeVariableDataType = par_DataType;
        Record->TimeVariableStartBitOffset = par_StartBitOffset;
        Record->TimeVariableNumberOfBits = par_NumberOfBits;
        Record->TimeVariableFactor = par_TimeFac;
        Record->TimeVariableOffset = par_TimeOff;
        if ((Record->TimeVariableFactor == 1.0e-9) &&
            (Record->TimeVariableOffset == 0.0)) {
            Record->TimeAreInNanoSec = 1;
        }
    } else {
        Record->NumberOfEntrys++;
        Record->Entrys = (MDF_RECORD_ENTY*)my_realloc(Record->Entrys, Record->NumberOfEntrys * sizeof(MDF_RECORD_ENTY));
        if (Record->Entrys == NULL) {
            ErrorCleanCache(par_Mdf, "out of memory");
            return -1;
        }
        MEMSET(&Record->Entrys[Record->NumberOfEntrys - 1], 0, sizeof(MDF_RECORD_ENTY));
        Record->Entrys[Record->NumberOfEntrys - 1].Vid = par_Vid;
        Record->Entrys[Record->NumberOfEntrys - 1].DataType = par_DataType;
        Record->Entrys[Record->NumberOfEntrys - 1].BbDataType = par_BbDataType;
        Record->Entrys[Record->NumberOfEntrys - 1].NumberOfBits = par_NumberOfBits;
        Record->Entrys[Record->NumberOfEntrys - 1].StartBitOffset = par_StartBitOffset;
    }
    return DataBlock->NumberOfChannelGroups - 1;
}

static void DoubleToString(double par_Value, char *ret_String, int par_Maxc)
{
    int Prec = 15;
    while (1) {
        PrintFormatToString (ret_String, par_Maxc, "%.*g", Prec, par_Value);
        if ((Prec++) == 18 || (par_Value == strtod (ret_String, NULL))) break;
    }
}

static int MdfReadTextRangeTable(MDF_STIMULI_FILE *Mdf, int par_Vid, uint32_t par_Size)
{
    int x;
    int ReadResult;
    char *Conversion;
    MDF_COMPU_VTAB_RANGE *Elements;
    char LowerRangeString[32];
    char UpperRangeString[32];
    int PosInConversion = 0;
    int Ret = -1;
    int NumberOfElements = par_Size / sizeof(MDF_COMPU_VTAB_RANGE);

    Conversion = my_malloc(BBVARI_CONVERSION_SIZE);
    Elements = (MDF_COMPU_VTAB_RANGE*)my_malloc(sizeof (*Elements) * NumberOfElements);
    if ((Conversion == NULL) || (Elements == NULL)) {
        return -1;
    }

    ReadResult = my_read (Mdf->fh, Elements, sizeof (*Elements) * NumberOfElements);
    if (ReadResult != (int)sizeof (*Elements) * NumberOfElements) {
        goto ERROR_OUT;
    }
    for (x = 1; x < NumberOfElements; x++) {
        MDF_TXBLOCK MdfTxBlock;

        my_lseek(Mdf->fh, Elements[x].PointerToTx);
        ReadResult = my_read (Mdf->fh, &MdfTxBlock, sizeof (MdfTxBlock));
        if (ReadResult != sizeof (MdfTxBlock)) {
            goto ERROR_OUT;
        }
        if (strncmp("TX", MdfTxBlock.BlockTypeIdentifier, 2)) {
            goto ERROR_OUT;
        }
        DoubleToString(Elements[x].LowerRange, LowerRangeString, sizeof(LowerRangeString));
        DoubleToString(Elements[x].UpperRange, UpperRangeString, sizeof(UpperRangeString));

        if (x > 0) { // for(x=1...
            // Check if it fits into
            if ((PosInConversion + MdfTxBlock.BlockSize - sizeof(MdfTxBlock) + strlen(LowerRangeString) + strlen(UpperRangeString) + 8) < BBVARI_CONVERSION_SIZE) {
                if (x != 1) {
                    StringCopyMaxCharTruncate(Conversion + PosInConversion, " ", BBVARI_CONVERSION_SIZE - PosInConversion);
                    PosInConversion++;
                }
                StringCopyMaxCharTruncate(Conversion + PosInConversion, LowerRangeString, BBVARI_CONVERSION_SIZE - PosInConversion);
                PosInConversion += (int)strlen(LowerRangeString);
                StringCopyMaxCharTruncate(Conversion + PosInConversion, " ", BBVARI_CONVERSION_SIZE - PosInConversion);
                PosInConversion++;
                StringCopyMaxCharTruncate(Conversion + PosInConversion, UpperRangeString, BBVARI_CONVERSION_SIZE - PosInConversion);
                PosInConversion += (int)strlen(UpperRangeString);
                StringCopyMaxCharTruncate(Conversion + PosInConversion, " \"", BBVARI_CONVERSION_SIZE - PosInConversion);
                PosInConversion += 2;
                ReadResult = my_read (Mdf->fh, Conversion + PosInConversion, MdfTxBlock.BlockSize - sizeof(MdfTxBlock));
                if (ReadResult != (int)MdfTxBlock.BlockSize - (int)sizeof(MdfTxBlock)) {
                    goto ERROR_OUT;
                }
                PosInConversion += ReadResult - 1;
                StringCopyMaxCharTruncate(Conversion + PosInConversion, "\";", BBVARI_CONVERSION_SIZE - PosInConversion);
                PosInConversion += 2;
            } else {
                break;
            }
        }
    }
    Conversion[PosInConversion] = 0;
    Ret =set_bbvari_conversion(par_Vid, 2, Conversion);
ERROR_OUT:
    if (Conversion != NULL) my_free (Conversion);
    if (Elements != NULL) my_free (Elements);
    return Ret;
}

static int MdfReadTextValueTable(MDF_STIMULI_FILE *Mdf, int par_Vid, uint32_t par_Size)
{
    int x;
    int ReadResult;
    char *Conversion;
    MDF_COMPU_VTAB *Elements;
    char ValueString[32];
    int PosInConversion = 0;
    int Ret = -1;
    int NumberOfElements = par_Size / sizeof(MDF_COMPU_VTAB);

    Conversion = my_malloc(BBVARI_CONVERSION_SIZE);
    Elements = (MDF_COMPU_VTAB*)my_malloc(sizeof (*Elements) * NumberOfElements);
    if ((Conversion == NULL) || (Elements == NULL)) {
        return -1;
    }
    ReadResult = my_read (Mdf->fh, Elements, sizeof (*Elements) * NumberOfElements);
    if (ReadResult != (int)sizeof (*Elements) * NumberOfElements) {
        goto ERROR_OUT;
    }
    for (x = 0; x < NumberOfElements; x++) {
        DoubleToString(Elements[x].Value, ValueString, sizeof(ValueString));
        //  Check if it fits into
        if ((PosInConversion + strlen(Elements[x].AssignedText) + strlen(ValueString) + strlen(ValueString) + 8) < BBVARI_CONVERSION_SIZE) {
            if (x != 0) {
                StringCopyMaxCharTruncate(Conversion + PosInConversion, " ", BBVARI_CONVERSION_SIZE - PosInConversion);
                PosInConversion++;
            }
            StringCopyMaxCharTruncate(Conversion + PosInConversion, ValueString, BBVARI_CONVERSION_SIZE - PosInConversion);
            PosInConversion += (int)strlen(ValueString);
            StringCopyMaxCharTruncate(Conversion + PosInConversion, " ", BBVARI_CONVERSION_SIZE - PosInConversion);
            PosInConversion++;
            StringCopyMaxCharTruncate(Conversion + PosInConversion, ValueString, BBVARI_CONVERSION_SIZE - PosInConversion);
            PosInConversion += (int)strlen(ValueString);
            StringCopyMaxCharTruncate(Conversion + PosInConversion, " \"", BBVARI_CONVERSION_SIZE - PosInConversion);
            PosInConversion += 2;
            StringCopyMaxCharTruncate(Conversion + PosInConversion, Elements[x].AssignedText, BBVARI_CONVERSION_SIZE - PosInConversion);
            PosInConversion += (int)strlen(Elements[x].AssignedText);
            StringCopyMaxCharTruncate(Conversion + PosInConversion, "\";", BBVARI_CONVERSION_SIZE - PosInConversion);
            PosInConversion += 2;
        } else {
            break;
        }
    }
    Conversion[PosInConversion] = 0;
    Ret =set_bbvari_conversion(par_Vid, 2, Conversion);
ERROR_OUT:
    if (Conversion != NULL) my_free (Conversion);
    if (Elements != NULL) my_free (Elements);
    return Ret;
}

static int MdfGetConversion (MDF_STIMULI_FILE *Mdf, uint32_t par_ConversionOffset, int par_Vid, double *ret_Fac, double *ret_Off)
{
    int ReadResult;
    MDF_CCBLOCK MdfCcBlock;

    my_lseek(Mdf->fh, par_ConversionOffset);
    ReadResult = my_read (Mdf->fh, &MdfCcBlock, sizeof (MdfCcBlock));
    if ((ReadResult != sizeof (MdfCcBlock)) && (MdfCcBlock.BlockSize < sizeof (MdfCcBlock))) {
        return -1;
    }
    if (strncmp("CC", MdfCcBlock.BlockTypeIdentifier, 2)) {
        return -1;
    }
    if (MdfCcBlock.PhysicalValueRangeValidFlag) {
        if (par_Vid > 0) {
            set_bbvari_max(par_Vid, MdfCcBlock.MaximumPhysicalSignalValue);
            set_bbvari_min(par_Vid, MdfCcBlock.MinimumPhysicalSignalValue);
        }
    }
    if  (par_Vid > 0) {
        set_bbvari_unit(par_Vid, MdfCcBlock.PhysicalUnit);
    }
    switch (MdfCcBlock.ConversionType) {
    case 0:     //     0 = parametric, linear
        {
            double P[2];
            char Conversion[64];
            if (MdfCcBlock.BlockSize >= (sizeof (MdfCcBlock) + 2 * sizeof(double))) {
                ReadResult = my_read (Mdf->fh, P, sizeof (P));
                if (ReadResult != sizeof (P)) {
                    return -1;
                }
                if  (par_Vid > 0) {
                    PrintFormatToString (Conversion, sizeof(Conversion), "%.18g:%.18g", P[1], P[0]);
                    set_bbvari_conversion(par_Vid, BB_CONV_FACTOFF, Conversion);
                } else {
                    if (ret_Fac != NULL) *ret_Fac = P[1];
                    if (ret_Off != NULL) *ret_Off = P[0];
                }
            }
            break;
        }
    case 1:     //     1 = tabular with interpolation
    case 2:     //     2 = tabular
        if (par_Vid > 0) {
            if (MdfCcBlock.BlockSize >= (sizeof (MdfCcBlock) + 6 * sizeof(double))) {
                int Size = (MdfCcBlock.BlockSize - sizeof(MdfCcBlock)) / (2 * sizeof(double));
                int ConvStringSize = 64 * Size;
                char *Conversion = my_malloc(ConvStringSize);
                double *P = my_malloc(sizeof(double) * 2 * Size);
                if ((Conversion != NULL) && (P != NULL)) {
                    int x;
                    char *p;
                    ReadResult = my_read (Mdf->fh, P, Size * 2 * sizeof(double));
                    if (ReadResult !=  Size * 2 * sizeof(double)) {
                        my_free(Conversion);
                        my_free(P);
                        return -1;
                    }
                    p = Conversion;
                    for (x = 0; x < Size; x++) {
                        p += PrintFormatToString (p, ConvStringSize - (p - Conversion), (x == 0) ? "%.18g/%.18g" : ":%.18g/%.18g", P[x * 2 + 1], P[x * 2]);  // phys/raw
                    }
                    set_bbvari_conversion(par_Vid, (MdfCcBlock.ConversionType == 1) ? BB_CONV_TAB_INTP : BB_CONV_TAB_NOINTP, Conversion);
                }
            }
            break;
        }
    case 9:     //     9 = rational function
         {
            double P[6];
            char Conversion[256];
            if (MdfCcBlock.BlockSize >= (sizeof (MdfCcBlock) + 6 * sizeof(double))) {
                ReadResult = my_read (Mdf->fh, P, sizeof (P));
                if (ReadResult != sizeof (P)) {
                    return -1;
                }
                if  (par_Vid > 0) {
                    if ((P[0] == 0.0) &&   // a == 0
                        (P[1] != 0.0) &&   // b != 0
                        (P[3] == 0.0) &&   // d == 0
                        (P[4] == 0.0)) {   // e == 0
                        // it is a linear conversion
                        PrintFormatToString (Conversion, sizeof(Conversion), "%.18g:%.18g",
                                P[5] / P[1], P[2] / P[1]); // f/b, c/b
                        set_bbvari_conversion(par_Vid, BB_CONV_FACTOFF, Conversion);
                    } else {
                        PrintFormatToString (Conversion, sizeof(Conversion), "%.18g:%.18g:%.18g:%.18g:%.18g:%.18g",
                                P[0], P[1], P[2], P[3], P[4], P[5]);
                        set_bbvari_conversion(par_Vid, BB_CONV_RAT_FUNC, Conversion);
                    }
                }
            }
            break;
        }
    case 11:    //     11 = ASAM-MCD2 Text Table, (COMPU_VTAB)
        return MdfReadTextValueTable(Mdf,  par_Vid, MdfCcBlock.BlockSize - sizeof(MdfCcBlock));
    case 12:    //     12 = ASAM-MCD2 Text Range Table (COMPU_VTAB_RANGE)
        return MdfReadTextRangeTable(Mdf,  par_Vid, MdfCcBlock.BlockSize - sizeof(MdfCcBlock));
    case 65535: //     65535 = 1:1 conversion formula (Int = Phys)
        if  (par_Vid > 0) {
            set_bbvari_conversion(par_Vid, 0, "");
        }
        break;
    default:
        return -1;
    }

    return 0;
}

static void MdfSetTheEndOfAllDataGroupBlocks(MDF_STIMULI_FILE *par_Mdf)
{
    int b, c;
    uint32_t Size;
    for (b = 0; b < par_Mdf->Cache.NumberOfDataBlocks; b++) {
        MDF_DATA_BLOCK_CACHE *DataBlock = &(par_Mdf->Cache.DataBlocks[b]);
        Size = 0;
        for (c = 0; c < DataBlock->NumberOfChannelGroups; c++) {
            Size += (DataBlock->Records[c].SizeOfOneRecord + DataBlock->NumberOfRecordIDs) * DataBlock->Records[c].NumberOfRecords;
        }
        DataBlock->SizeOfBlock = Size;
        DataBlock->EndOfInsideFile = DataBlock->OffsetInsideFile + Size;
    }
}

static int MdfTranslateDatatypeToBlackboardType (int par_DataType, int par_BitSize)
{
    switch(par_DataType) {
    case 0: //    0 = unsigned integer
        if (par_BitSize <= 8) {
            return BB_UNKNOWN_UBYTE;
        } else if (par_BitSize <= 16) {
            return BB_UNKNOWN_UWORD;
        } else if (par_BitSize <= 32) {
            return BB_UNKNOWN_UDWORD;
        } else if (par_BitSize <= 64) {
            return BB_UNKNOWN_UQWORD;
        }
        break;
    case 1: //    1 = signed integer (twos complement)
        if (par_BitSize <= 8) {
            return BB_UNKNOWN_BYTE;
        } else if (par_BitSize <= 16) {
            return BB_UNKNOWN_WORD;
        } else if (par_BitSize <= 32) {
            return BB_UNKNOWN_WORD;
        } else if (par_BitSize <= 64) {
            return BB_UNKNOWN_QWORD;
        }
        break;
    case 2: //    2 = IEEE 754 floating-point format FLOAT (4 bytes)
        return BB_UNKNOWN_FLOAT;
    case 3: //    3 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
        return BB_UNKNOWN_DOUBLE;
    default:
    //    Default Byte order from IDBLOCK
    //    4 = VAX floating-point format (F_Float)
    //    5 = VAX floating-point format (G_Float)
    //    6 = VAX floating-point format (D_Float)
    //    obsolete
    //    7 = String (NULL terminated)
    //    8 = Byte Array (max. 8191 Bytes, constant record length!)
    //    9 = unsigned integer
    //    10 = signed integer (twos complement)
    //    11 = IEEE 754 floating-point format FLOAT (4 bytes)
    //    12 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
    //    Big Endian Byte order
    //    13 = unsigned integer
    //    14 = signed integer (twos complement)
    //    15 = IEEE 754 floating-point format FLOAT (4 bytes)
    //    16 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
        break;
    }
    return BB_INVALID;
}

STIMULI_FILE *MdfOpenAndReadStimuliHeader (const char *par_Filename, const char *par_Variables)
{
    int VariableCount;
    MDF_IDBLOCK MdfIdBlock;
    MDF_HDBLOCK MdfHdBlock;
    MDF_DGBLOCK MdfDgBlock;
    MDF_CGBLOCK MdfCgBlock;
    MDF_CNBLOCK MdfCnBlock;
    uint32_t DataGroupBlockOffset;
    uint32_t ChannelGroupBlockOffset;
    uint32_t ChannelBlockOffset;
    int ReadResult;
    STIMULI_FILE *Ret = NULL;
    MDF_STIMULI_FILE *Mdf = NULL;
    const char *vlp;
    int BlockNumber = 0;
    char DateString[11];
    char TimeString[9];
    char Filename[MAX_PATH];

    SearchAndReplaceEnvironmentStrings(par_Filename, Filename, sizeof(Filename));

    Ret = (STIMULI_FILE*)my_calloc(1, sizeof(STIMULI_FILE));
    if (Ret == NULL) {
        ThrowError(1, "out of memmory");
        goto __ERROR;
    }
    Mdf = (MDF_STIMULI_FILE*)my_calloc(1, sizeof(MDF_STIMULI_FILE));
    if (Mdf == NULL) {
        ThrowError(1, "out of memmory");
        goto __ERROR;
    }
    Mdf->fh = MY_INVALID_HANDLE_VALUE;
    Ret->File = (void*)Mdf;
    Ret->FileType = MDF_FILE;

    Mdf->fh = my_open(Filename);
    if (Mdf->fh == MY_INVALID_HANDLE_VALUE) {
        ThrowError (1, "Unable to open %s file\n", Filename);
        goto __ERROR;
    }

    Mdf->FirstTimeStamp = 0xFFFFFFFFFFFFFFFFULL;
    Mdf->SizeOfFile = (uint32_t)my_get_file_size(Mdf->fh);

    ReadResult = my_read (Mdf->fh, &MdfIdBlock, sizeof (MdfIdBlock));
    if (ReadResult != sizeof (MdfIdBlock)) {
        goto __ERROR;
    }
    if (strncmp("MDF     ", MdfIdBlock.FileIdentifier, 8)) {
        goto __ERROR;
    }
    ReadResult = my_read (Mdf->fh, &MdfHdBlock, sizeof (MdfHdBlock));
    if ((ReadResult != sizeof (MdfHdBlock)) && (MdfHdBlock.BlockSize != sizeof (MdfHdBlock))) {
        goto __ERROR;
    }
    if (strncmp("HD", MdfHdBlock.BlockTypeIdentifier, 2)) {
        goto __ERROR;
    }
    if (MdfHdBlock.FirstDataGroupBlock == 0) {  // There must be at least one.
        goto __ERROR;
    }

    // Start time
    MEMCPY (TimeString, MdfHdBlock.TimeOfRecording, 8);
    TimeString[8] = 0;
    MEMCPY (DateString, MdfHdBlock.DateOfRecording, 10);
    DateString[10] = 0;

    Mdf->StartTimeStamp = ConvertDateTimeStringToTimeStamp(DateString, TimeString);

    VariableCount = 0;
    DataGroupBlockOffset = MdfHdBlock.FirstDataGroupBlock;
    while (DataGroupBlockOffset) {
        my_lseek(Mdf->fh, DataGroupBlockOffset);
        ReadResult = my_read (Mdf->fh, &MdfDgBlock, sizeof (MdfDgBlock));
        if ((ReadResult != sizeof (MdfDgBlock)) && (MdfDgBlock.BlockSize != sizeof (MdfDgBlock))) {
            goto __ERROR;
        }
        if (strncmp("DG", MdfDgBlock.BlockTypeIdentifier, 2)) {
            goto __ERROR;
        }
        if (MdfDgBlock.DataBlock != 0) {
            // Add new data block
            AddDataBlockToCache(Mdf, MdfDgBlock.DataBlock, MdfDgBlock.NumberOfRecordIDs);

            ChannelGroupBlockOffset = MdfDgBlock.FirstChannelGroupBlock;
            while (ChannelGroupBlockOffset) {
                my_lseek(Mdf->fh, ChannelGroupBlockOffset);
                ReadResult = my_read (Mdf->fh, &MdfCgBlock, sizeof (MdfCgBlock));
                if ((ReadResult != sizeof (MdfCgBlock)) && (MdfCgBlock.BlockSize != sizeof (MdfCgBlock))) {
                    goto __ERROR;
                }
                if (strncmp("CG", MdfCgBlock.BlockTypeIdentifier, 2)) {
                    goto __ERROR;
                }

                // Add new channel block
                AddChannelGroupToCache(Mdf, BlockNumber, MdfCgBlock.RecordID, MdfCgBlock.SizeOfDataRecord, MdfCgBlock.NumberOfRecords);

                if (MdfCgBlock.SizeOfDataRecord > Mdf->Cache.DataBlocks[BlockNumber].LargesRecord) {
                    Mdf->Cache.DataBlocks[BlockNumber].LargesRecord = MdfCgBlock.SizeOfDataRecord;
                }

                ChannelBlockOffset = MdfCgBlock.FirstChannelBlock;
                while (ChannelBlockOffset) {
                    my_lseek(Mdf->fh, ChannelBlockOffset);
                    ReadResult = my_read (Mdf->fh, &MdfCnBlock, sizeof (MdfCnBlock));
                    if ((ReadResult != sizeof (MdfCnBlock)) && (MdfCnBlock.BlockSize != sizeof (MdfCnBlock))) {
                        goto __ERROR;
                    }
                    if (strncmp("CN", MdfCnBlock.BlockTypeIdentifier, 2)) {
                        goto __ERROR;
                    }
                    if (MdfCnBlock.ChannelType == 0) {
                        if (IsValidDataType(MdfCnBlock.SignalDataType, MdfCnBlock.NumberOfBits)) {
                            char *SignalName;
                            if (MdfCnBlock.LongSignalName != 0) {
                                // SignalName have to give free with my_free
                                SignalName = ReadString(Mdf->fh, MdfCnBlock.LongSignalName);
                            } else {
                                SignalName = MdfCnBlock.ShortSignalName;
                            }

                            // Look if variable schould be used
                            if (par_Variables != NULL) {
                                Mdf->vids = my_realloc(Mdf->vids, (VariableCount + 1) * sizeof (int));
                                Mdf->dtypes = my_realloc(Mdf->dtypes, (VariableCount + 1) * sizeof (int));
                                if ((Mdf->vids == NULL) || (Mdf->dtypes == NULL)) {
                                    goto __ERROR;
                                }
                                Mdf->vids[VariableCount] = -1;
                                vlp = par_Variables;
                                while (*vlp != '\0') {
                                    if (!strcmp (vlp, SignalName)) {
                                        int BbDataType = MdfTranslateDatatypeToBlackboardType(MdfCnBlock.SignalDataType, MdfCnBlock.NumberOfBits);
                                        Mdf->vids[VariableCount] = add_bbvari (SignalName, BbDataType, NULL);
                                        Mdf->dtypes[VariableCount] = get_bbvaritype (Mdf->vids[VariableCount]);
                                        // Add new variable
                                        AddVariableToCache(Mdf, BlockNumber, MdfCgBlock.RecordID, Mdf->vids[VariableCount], MdfCnBlock.SignalDataType, Mdf->dtypes[VariableCount],
                                                           MdfCnBlock.NumberOfBits, MdfCnBlock.StartBitOffset, 0, 0.0, 0.0);
                                        if (MdfCnBlock.ConversionFormula != 0) {
                                            MdfGetConversion(Mdf, MdfCnBlock.ConversionFormula, Mdf->vids[VariableCount], NULL, NULL);
                                        }
                                        VariableCount++;
                                        break;
                                    }
                                    while (*vlp != '\0') vlp++;
                                    vlp++;
                                }
                            } else {
                                // All variables
                                Mdf->vids[VariableCount] = add_bbvari (SignalName, BB_UNKNOWN_DOUBLE, NULL);
                                Mdf->dtypes[VariableCount] = get_bbvaritype (Mdf->vids[VariableCount]);
                                // Add new variable
                                AddVariableToCache(Mdf, BlockNumber, MdfCgBlock.RecordID, Mdf->vids[VariableCount], MdfCnBlock.SignalDataType, Mdf->dtypes[VariableCount],
                                                   MdfCnBlock.NumberOfBits, MdfCnBlock.StartBitOffset, 0, 0.0, 0.0);
                                if (MdfCnBlock.ConversionFormula != 0) {
                                    MdfGetConversion(Mdf, MdfCnBlock.ConversionFormula, Mdf->vids[VariableCount], NULL, NULL);
                                }
                                VariableCount++;
                            }
                        }
                    } else {
                        double TimeVariableFactor = 1.0;
                        double TimeVariableOffset = 0.0;
                        if (MdfCnBlock.ConversionFormula != 0) {
                            MdfGetConversion(Mdf, MdfCnBlock.ConversionFormula, 0, &TimeVariableFactor, &TimeVariableOffset);
                        }
                        AddVariableToCache(Mdf, BlockNumber, MdfCgBlock.RecordID, 0, MdfCnBlock.SignalDataType, 0,
                                           MdfCnBlock.NumberOfBits, MdfCnBlock.StartBitOffset, 1, TimeVariableFactor, TimeVariableOffset);
                    }
                    ChannelBlockOffset = MdfCnBlock.NextChannelBlock;
                }
                ChannelGroupBlockOffset = MdfCgBlock.NextChannelGroupBlock;
            }
            BlockNumber++;
        }
        DataGroupBlockOffset = MdfDgBlock.NextDataGroupBlock;
    }

    MdfSetTheEndOfAllDataGroupBlocks(Mdf);

    Mdf->variable_count = VariableCount;
    return Ret;
__ERROR:
    if (Ret != NULL) my_free(Ret);
    if (Mdf != NULL) {
        CleanCache(Mdf);
        if (Mdf->fh != MY_INVALID_HANDLE_VALUE) my_close(Mdf->fh);
        if (Mdf->vids != NULL) my_free(Mdf->vids);
        if (Mdf->dtypes != NULL) my_free(Mdf->dtypes);
        my_free(Mdf);
    }
    return NULL;
}


int MdfGetNumberOfStimuliVariables(STIMULI_FILE *par_File)
{
    MDF_STIMULI_FILE *Mdf = (MDF_STIMULI_FILE*)par_File->File;
    return (Mdf->variable_count);

}


static int UpdateCache(MDF_STIMULI_FILE *par_Mdf, MDF_DATA_BLOCK_CACHE *DataBlock)
{
    uint32_t Rest, SizeOfRead;
    int Readed;

    my_lseek(par_Mdf->fh, DataBlock->EndCacheFilePos);
    if (DataBlock->BlockData == NULL) {
        DataBlock->BlockData = my_malloc (DataBlock->LargesRecord * 32);
        if (DataBlock->BlockData == NULL) {
            return -1;
        }
    }
    SizeOfRead = DataBlock->LargesRecord * 32;
    // don't read behind the file end
    if ((DataBlock->CurrentFilePos + SizeOfRead) > par_Mdf->SizeOfFile) {
        SizeOfRead = par_Mdf->SizeOfFile - DataBlock->CurrentFilePos;
    }
    if (DataBlock->EndCacheFilePos > DataBlock->CurrentFilePos) {
        Rest = DataBlock->EndCacheFilePos - DataBlock->CurrentFilePos;
        MEMCPY(DataBlock->BlockData, DataBlock->BlockData + (DataBlock->CurrentFilePos - DataBlock->StartCacheFilePos), Rest);
        SizeOfRead -= Rest;
    } else {
        Rest = 0;
    }

    Readed = my_read(par_Mdf->fh, DataBlock->BlockData + Rest, SizeOfRead);

    DataBlock->StartCacheFilePos = DataBlock->EndCacheFilePos - Rest;
    DataBlock->EndCacheFilePos = DataBlock->StartCacheFilePos + Rest + Readed;
    return 0;
}

__inline static int ConvWithSign (uint64_t value, int size, int sign, union FloatOrInt64 *ret_value)
{
    if (sign) {
        if ((0x1ULL << (size - 1)) & value) { // Are the sign bit set
            if (size < 64) {
                ret_value->qw = (int64_t)(value - (0x1ULL << size));
            } else {
                ret_value->qw = (int64_t)value;
            }
        } else ret_value->qw = (int64_t)value;
        return FLOAT_OR_INT_64_TYPE_INT64;
    } else {
        ret_value->uqw = value;
        return FLOAT_OR_INT_64_TYPE_UINT64;
    }
}

#ifdef __GNUC__
#define _byteswap_ulong __builtin_bswap32
#define _byteswap_uint64 __builtin_bswap64
#endif

__inline static uint64_t GetValue (unsigned char *par_Data, int par_ByteOrder, int par_StartBit, int par_BitSize, int par_RecordSize)
{
    uint32_t PosByte;
    uint32_t PosBit;
    uint64_t Ret;

    if (par_ByteOrder == 1) {  // 1 -> MSB first
        int32_t PosByte_m8;
        uint32_t IStartBit = (uint32_t)((par_RecordSize << 3) - par_StartBit);
        PosBit = IStartBit & 0x7;
        PosByte = IStartBit >> 3;
        PosByte_m8 = (int32_t)PosByte - 8;
        Ret = _byteswap_uint64 (*(uint64_t *)(void*)(par_Data + PosByte_m8)) << PosBit;
        Ret |= (uint64_t)par_Data[PosByte] >> (8 - PosBit);
    } else {  // 0 -> LSB first
        PosBit = par_StartBit & 0x7;
        PosByte = (uint32_t)(par_StartBit >> 3);
        Ret = *(uint64_t *)(void*)(par_Data + PosByte) >> PosBit;
        Ret |= ((uint64_t)par_Data[PosByte + 8] << (8 - PosBit)) << 56;
   }
    if (par_BitSize == 64) {
        return Ret;
    } else {
        return Ret &= ~(0xFFFFFFFFFFFFFFFFULL << par_BitSize);
    }
}

static int ConvertValue(unsigned char* par_Data, int par_DataType, int par_StartBit, int par_BitSize, int par_RecordSize, union FloatOrInt64 *ret_Value)
{
    uint64_t HelpU64;
    switch(par_DataType) {
    case 0: //    0 = unsigned integer
        HelpU64 = GetValue(par_Data, 0, par_StartBit, par_BitSize, par_RecordSize);
        return ConvWithSign(HelpU64, par_BitSize, 0, ret_Value);
        break;
    case 1: //    1 = signed integer (twos complement)
        HelpU64 = GetValue(par_Data, 0, par_StartBit, par_BitSize, par_RecordSize);
        return ConvWithSign(HelpU64, par_BitSize, 1, ret_Value);
        break;
    case 2: //    2 = IEEE 754 floating-point format FLOAT (4 bytes)
        HelpU64 = GetValue(par_Data, 0, par_StartBit, par_BitSize, par_RecordSize);
        ret_Value->d = *(float*)&HelpU64;
        return FLOAT_OR_INT_64_TYPE_F64;
        break;
    case 3: //    3 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
        HelpU64 = GetValue(par_Data, 0, par_StartBit, par_BitSize, par_RecordSize);
        ret_Value->d = *(double*)&HelpU64;
        return FLOAT_OR_INT_64_TYPE_F64;
        break;
    default:
    //    Default Byte order from IDBLOCK
    //    4 = VAX floating-point format (F_Float)
    //    5 = VAX floating-point format (G_Float)
    //    6 = VAX floating-point format (D_Float)
    //    obsolete
    //    7 = String (NULL terminated)
    //    8 = Byte Array (max. 8191 Bytes, constant record length!)
    //    9 = unsigned integer
    //    10 = signed integer (twos complement)
    //    11 = IEEE 754 floating-point format FLOAT (4 bytes)
    //    12 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
    //    Big Endian Byte order
    //    13 = unsigned integer
    //    14 = signed integer (twos complement)
    //    15 = IEEE 754 floating-point format FLOAT (4 bytes)
    //    16 = IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
        return FLOAT_OR_INT_64_TYPE_INVALID;
        break;
    }
}

static uint64_t ConvertTimeVariableToUInt64(MDF_RECORD_CACHE *par_Record, unsigned char *par_Cache, uint32_t CachePos)
{
    union FloatOrInt64 Value;
    switch(ConvertValue(par_Cache + CachePos, par_Record->TimeVariableDataType, par_Record->TimeVariableStartBitOffset,
                         par_Record->TimeVariableNumberOfBits, par_Record->SizeOfOneRecord, &Value)) {
    case FLOAT_OR_INT_64_TYPE_F64:
        if (par_Record->TimeAreInNanoSec) {
            return (uint64_t)(Value.d); // this are already nano seconds
        } else {
            return (uint64_t)((Value.d * par_Record->TimeVariableFactor + par_Record->TimeVariableOffset) * TIMERCLKFRQ);
        }
    case FLOAT_OR_INT_64_TYPE_UINT64:
        if (par_Record->TimeAreInNanoSec) {
            return Value.uqw;
        } else {
            return (uint64_t)((Value.uqw * par_Record->TimeVariableFactor + par_Record->TimeVariableOffset) * TIMERCLKFRQ);
        }
    case FLOAT_OR_INT_64_TYPE_INT64:
        if (par_Record->TimeAreInNanoSec) {
            return (uint64_t)Value.qw;
        } else {
            return (uint64_t)((Value.qw * par_Record->TimeVariableFactor + par_Record->TimeVariableOffset) * TIMERCLKFRQ);
        }
    default:
        return 0;
    }
}


static int GetTimeOfCurrentRecord(MDF_STIMULI_FILE *par_Mdf, MDF_DATA_BLOCK_CACHE *par_DataBlock, uint64_t *ret_t)
{
    int Idx;
    uint16_t Id;
    uint32_t CachePos;
    MDF_RECORD_CACHE *Record;

    if (par_DataBlock->CurrentFilePos >= par_DataBlock->EndOfInsideFile) {
        return STIMULI_END_OF_FILE;
    }
    if (par_DataBlock->EndCacheFilePos < par_Mdf->SizeOfFile) {
        if ((par_DataBlock->CurrentFilePos + par_DataBlock->LargesRecord + 2 * sizeof(uint16_t)) >= par_DataBlock->EndCacheFilePos) {
            UpdateCache(par_Mdf, par_DataBlock);
        }
    }
    CachePos = par_DataBlock->CurrentFilePos - par_DataBlock->StartCacheFilePos;
    if (par_DataBlock->NumberOfRecordIDs == 0) {
        Idx = 0;
    } else {
        unsigned char *p = par_DataBlock->BlockData + CachePos;
        Id = *p;
        Idx = TranslateChannelIdToIndex(par_DataBlock, Id);
        CachePos += sizeof(uint8_t);
    }
    Record = &(par_DataBlock->Records[Idx]);

    *ret_t = ConvertTimeVariableToUInt64(Record, par_DataBlock->BlockData, CachePos);

    return 0;
}

static int AddCurrentRecordToSampleSlot(MDF_DATA_BLOCK_CACHE *par_DataBlock,
                                        VARI_IN_PIPE *pipevari_list, int par_PipePos)
{
    int Idx;
    MDF_RECORD_CACHE *Record;
    MDF_RECORD_ENTY *Entry;
    int v;
    union FloatOrInt64 Value;
    int Type;
    uint32_t CachePos;
    int Pos = par_PipePos;

    CachePos = par_DataBlock->CurrentFilePos - par_DataBlock->StartCacheFilePos;
    if (par_DataBlock->NumberOfRecordIDs == 0) {
        Idx = 0;
    } else {
        Idx = TranslateChannelIdToIndex(par_DataBlock, *(uint8_t*)(par_DataBlock->BlockData + CachePos));
        CachePos += sizeof(uint8_t);
    }
    Record = &(par_DataBlock->Records[Idx]);
    for (v = 0; v < Record->NumberOfEntrys; v++) {
        Entry = &(Record->Entrys[v]);
        Type = ConvertValue(par_DataBlock->BlockData + CachePos, Entry->DataType, Entry->StartBitOffset,
                            Entry->NumberOfBits, Record->SizeOfOneRecord, &Value);
        sc_convert_from_FloatOrInt64_to_BB_VARI(Type, Value, Entry->BbDataType, &(pipevari_list[Pos].value));
        pipevari_list[Pos].vid = Entry->Vid;
        pipevari_list[Pos].type = Entry->BbDataType;
        Pos++;
    }
    par_DataBlock->CurrentFilePos += Record->SizeOfOneRecord + (sizeof (uint8_t) * par_DataBlock->NumberOfRecordIDs);
    return Pos;
}

int MdfReadOneStimuliTimeSlot(STIMULI_FILE *par_File, VARI_IN_PIPE *pipevari_list, uint64_t *ret_t)
{
    MDF_DATA_BLOCK_CACHE *DataBlock;
    MDF_DATA_BLOCK_CACHE *EarliestDataBlock = NULL;
    int b;
    int ValueCount = 0;
    uint64_t Time;
    uint64_t EarliestTime = 0xFFFFFFFFFFFFFFFFULL;
    int RecordState;


    MDF_STIMULI_FILE *Mdf = (MDF_STIMULI_FILE*)par_File->File;

    for (b = 0; b < Mdf->Cache.NumberOfDataBlocks; b++) {
        DataBlock = &(Mdf->Cache.DataBlocks[b]);

        RecordState = GetTimeOfCurrentRecord(Mdf, DataBlock, &Time);
        if (RecordState == 0) {
            if (Time < EarliestTime) {
                EarliestTime = Time;
                EarliestDataBlock = DataBlock;
            }
        }
    }
    if (EarliestDataBlock == NULL) {
        return STIMULI_END_OF_FILE;   // All records inside all data blocks are read
    } else {
        ValueCount = AddCurrentRecordToSampleSlot(EarliestDataBlock, pipevari_list, 0);
        if (EarliestTime < Mdf->FirstTimeStamp) {
            Mdf->FirstTimeStamp = EarliestTime;
        }
        *ret_t = EarliestTime - Mdf->FirstTimeStamp;
        return ValueCount;
    }
}

void MdfCloseStimuliFile(STIMULI_FILE *par_File)
{
    if (par_File != NULL) {
        MDF_STIMULI_FILE *Mdf = (MDF_STIMULI_FILE*)par_File->File;
        if (Mdf != NULL) {
            if (Mdf->fh != MY_INVALID_HANDLE_VALUE) my_close(Mdf->fh);
            if (Mdf->vids != NULL) my_free(Mdf->vids);
            if (Mdf->dtypes != NULL) my_free(Mdf->dtypes);
            my_free(Mdf);
        }
        my_free(par_File);
    }
}

int *MdfGetStimuliVariableIds(STIMULI_FILE *par_File)
{
    MDF_STIMULI_FILE *Mdf = (MDF_STIMULI_FILE*)par_File->File;
    return (Mdf->vids);
}

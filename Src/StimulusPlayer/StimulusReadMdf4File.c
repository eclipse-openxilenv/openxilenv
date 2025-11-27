/* ---------- read_mdf.c ------------------------------------------------- *\
 *                                                                         *
 *  Programm: Softcar                                                      *
 *                                                                         *
 *  Autor : Bieber         Abt.: TE-P          Datum: 27.11.2020           *
 *                                                                         *
 *  Funktion : Stimuli-File-Converter fuer dem HD-Player                   *
 *             fuer das MDF File Format                                    *
 *                                                                         *
\* ------------------------------------------------------------------------*/

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
#include "ThrowError.h"
#include "StimulusReadFile.h"
#include "Mdf4Structs.h"
#include "BlackboardConvertFromTo.h"
#include "StimulusReadMdf4File.h"

#include "zlib.h"

// If DebugOut exists write some debug infos to c:\\temp\\debug_mdf4.txt
//static FILE *DebugOut;
#define DebugOut NULL
//#define FFLUSH_DEBUGOUT fflush(DebugOut)
#define FFLUSH_DEBUGOUT


int IsMdf4Format(const char *par_Filename, int *ret_Version)
{
    MY_FILE_HANDLE fh;
    MDF4_IDBLOCK MdfIdBlock;
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
    if ((MdfIdBlock.VersionNumber < 400) || (MdfIdBlock.VersionNumber > 430) ||
        !(!strncmp("4.10 ", MdfIdBlock.FormatTdentifier, 5) ||
        !strncmp("4.20 ", MdfIdBlock.FormatTdentifier, 5) ||
        !strncmp("4.30 ", MdfIdBlock.FormatTdentifier, 5))) {
        //ThrowError (1, "%s file has version %s softcar supports only version 4.10...4.30\n", Filename, MdfIdBlock.FormatTdentifier);
        my_close (fh);
        return 0;
    }
    if (ret_Version != NULL) *ret_Version = MdfIdBlock.VersionNumber;

    if (MdfIdBlock.DefaultByteOrder != 0) {
        ThrowError (1, "%s file are not in little endian (intel order) softcar supports only this\n", Filename);
        my_close (fh);
        return 0;
    }
    my_close (fh);
    return 1;
}

static char *ReadString(MY_FILE_HANDLE fh, uint64_t par_Offset, char *Buffer, int *BufferSize)
{
    char *Ret = NULL;
    int ReadResult;
    uint64_t OldPos;
    MDF4_BLOCK_HEADER BlockHeader;

    OldPos = my_ftell(fh);
    my_lseek(fh, par_Offset);

    ReadResult = my_read (fh, &BlockHeader, sizeof (BlockHeader));
    if (ReadResult != sizeof (BlockHeader)) return NULL;
    if (strncmp("##TX", BlockHeader.BlockTypeIdentifier, 4)) return NULL;

    if (BlockHeader.BlockLength > (uint64_t)*BufferSize) {
        *BufferSize = (int)BlockHeader.BlockLength;
    }
    Ret = my_realloc(Buffer, *BufferSize);
    if (Ret == NULL) {
       *BufferSize = 0;
        return NULL;
    }
    ReadResult = my_read (fh, Ret, (uint32_t)BlockHeader.BlockLength);
    if (ReadResult != (int)BlockHeader.BlockLength) {
        my_free(Ret);
        return NULL;
    }
    my_lseek(fh, OldPos);
    return Ret;
}

static int IsValidDataType(int par_DataType, int par_BitSize)
{
    switch (par_DataType) {
    case 0: //    0 = unsigned integer little endian
    case 1: //    1 = unsigned integer big endian
        if (par_BitSize <= 64) return 1;
        else return 0;
    case 2: //    2 = signed integer little endian
    case 3: //    3 = signed integer big endian
        if (par_BitSize <= 64) return 1;
        else return 0;
    case 4: //    4 = floating-point format little endian
    case 5: //    5 = floating-point format big endian
        if ((par_BitSize == 64) || (par_BitSize == 32)) return 1;
        else return 0;
    default:
        return 0;
    }
}

static uint64_t *ReadMdf4BlockLinkList(MY_FILE_HANDLE *fh, uint64_t *LinkList, int *RetSizeOfLinkListBuffer, int SizeOfLinkList)
{
    int ReadResult;
    if (SizeOfLinkList > *RetSizeOfLinkListBuffer) {
        *RetSizeOfLinkListBuffer = SizeOfLinkList;
        LinkList = (uint64_t*)my_malloc (sizeof(uint64_t) * *RetSizeOfLinkListBuffer);
        if (LinkList == NULL) return NULL;
    }
    ReadResult = my_read (fh, LinkList, sizeof (uint64_t) * SizeOfLinkList);
    if (ReadResult != sizeof (uint64_t) * SizeOfLinkList) return NULL;
    return LinkList;
}

static double *ReadMdf4BlockParList(MY_FILE_HANDLE *fh, double *ParList, int *RetSizeOfParListBuffer, int SizeOfParList)
{
    int ReadResult;
    if (SizeOfParList > *RetSizeOfParListBuffer) {
        *RetSizeOfParListBuffer = SizeOfParList;
        ParList = (double*)my_malloc (sizeof(double) * *RetSizeOfParListBuffer);
        if (ParList == NULL) return NULL;
    }
    ReadResult = my_read (fh, ParList, sizeof (uint64_t) * SizeOfParList);
    if (ReadResult != sizeof (double) * SizeOfParList) return NULL;
    return ParList;
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

/* liest Variablenname eines Stimuli-Files  */
char *Mdf4ReadStimulHeaderVariabeles (const char *par_Filename)
{
    int VariableCount;
    char *Ret = NULL;
    int LenOfRet = 0;
    int Pos;

    MY_FILE_HANDLE fh;
    MDF4_IDBLOCK MdfIdBlock;
    MDF4_HDBLOCK MdfHdBlock;
    MDF4_BLOCK_HEADER BlockHeader;
    uint64_t DataGroupBlockOffset;
    uint64_t ChannelGroupBlockOffset;
    uint64_t ChannelBlockOffset;
    int ReadResult;
    uint64_t *LinkList = NULL;
    int LinkListSize = 0;
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
    ReadResult = my_read (fh, &BlockHeader, sizeof (BlockHeader));
    if (ReadResult != sizeof (BlockHeader)) goto __ERROUT;
    if (strncmp("##HD", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROUT;

    LinkList = ReadMdf4BlockLinkList(fh, LinkList, &LinkListSize, (uint32_t)BlockHeader.LinkCount);
    if (LinkList == NULL) goto __ERROUT;


    ReadResult = my_read (fh, &MdfHdBlock, sizeof (MdfHdBlock));
    if (ReadResult != sizeof (MdfHdBlock)) goto __ERROUT;

    DataGroupBlockOffset = LinkList[0];

    VariableCount = 0;
    while (DataGroupBlockOffset) {
        MDF4_DGBLOCK MdfDgBlock;
        my_lseek(fh, DataGroupBlockOffset);

        ReadResult = my_read (fh, &BlockHeader, sizeof (BlockHeader));
        if (ReadResult != sizeof (BlockHeader)) goto __ERROUT;
        if (strncmp("##DG", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROUT;
        LinkList = ReadMdf4BlockLinkList(fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
        if (LinkList == NULL) goto __ERROUT;
        ReadResult = my_read (fh, &MdfDgBlock, sizeof (MdfDgBlock));
        if (ReadResult != sizeof (MdfDgBlock)) goto __ERROUT;

        DataGroupBlockOffset = LinkList[0];  // 0 -> Next
        ChannelGroupBlockOffset = LinkList[1];   // 1 -> first CG
        while (ChannelGroupBlockOffset) {
            MDF4_CGBLOCK MdfCgBlock;
            my_lseek(fh, ChannelGroupBlockOffset);

            ReadResult = my_read (fh, &BlockHeader, sizeof (BlockHeader));
            if (ReadResult != sizeof (BlockHeader)) goto __ERROUT;
            if (strncmp("##CG", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROUT;
            LinkList = ReadMdf4BlockLinkList(fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
            if (LinkList == NULL) goto __ERROUT;
            ReadResult = my_read (fh, &MdfCgBlock, sizeof (MdfCgBlock));
            if (ReadResult != sizeof (MdfCgBlock)) goto __ERROUT;

            ChannelGroupBlockOffset = LinkList[0];  // 0 -> Next
            ChannelBlockOffset = LinkList[1];   // 1 -> first CN
            while (ChannelBlockOffset) {
                MDF4_CNBLOCK MdfCnBlock;
                my_lseek(fh, ChannelBlockOffset);

                ReadResult = my_read (fh, &BlockHeader, sizeof (BlockHeader));
                if (ReadResult != sizeof (BlockHeader)) goto __ERROUT;
                if (strncmp("##CN", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROUT;
                LinkList = ReadMdf4BlockLinkList(fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
                if (LinkList == NULL) goto __ERROUT;
                ReadResult = my_read (fh, &MdfCnBlock, sizeof (MdfCnBlock));
                if (ReadResult != sizeof (MdfCnBlock)) goto __ERROUT;

                ChannelBlockOffset = LinkList[0];  // 0 -> Next

                if (MdfCnBlock.Type == 0) {
                    if (IsValidDataType(MdfCnBlock.DataType, MdfCnBlock.BitCount)) {
                        char *SignalName = NULL;
                        int SizeOfSignalName = 0;
                        // SignalName muss mit my_free wieder freigegeben werden
                        SignalName = ReadString(fh, LinkList[2], SignalName, &SizeOfSignalName);
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
                                strcpy (Ret + Pos, ";");
                                Pos += 1;
                            }
                            strcpy (Ret + Pos, SignalName);
                            Pos += (int)strlen (SignalName);
                            VariableCount++;
                        }
                        my_free(SignalName);
                    }
                } else if (MdfCnBlock.Type == 0) {
                    // printf("time channel");
                } else {
                    // printf("unknown");
                }
            }
        }
    }
__ERROUT:
    if (LinkList != NULL) my_free(LinkList);
    my_close (fh);
    return Ret;    /* alles OK */
}

static void CleanCache(MDF4_STIMULI_FILE *par_Mdf)
{
    if (par_Mdf->Cache.DataGroups != NULL) {
        int x;
        for (x = 0; x < par_Mdf->Cache.NumberOfDataGroups; x++) {
            if (par_Mdf->Cache.DataGroups[x].BlockData != NULL) {
                my_free (par_Mdf->Cache.DataGroups[x].BlockData);
            }
            if (par_Mdf->Cache.DataGroups[x].Records != NULL) {
                int y;
                for (y = 0; y < par_Mdf->Cache.DataGroups[x].NumberOfChannelGroups; y++) {
                    ;
                }
                my_free (par_Mdf->Cache.DataGroups[x].Records);
            }
        }
        my_free (par_Mdf->Cache.DataGroups);
    }

}

static void ErrorCleanCache(MDF4_STIMULI_FILE *par_Mdf, const char *par_Error)
{
    ThrowError (1, "inside MDF cache: \"%s\"", par_Error);
    CleanCache(par_Mdf);
}

static int AddDataGroupToCache(MDF4_STIMULI_FILE *par_Mdf, uint8_t par_SizeOfRecordID)
{
    par_Mdf->Cache.NumberOfDataGroups++;
    par_Mdf->Cache.DataGroups = (MDF4_DATA_GROUP_CACHE*)my_realloc(par_Mdf->Cache.DataGroups, par_Mdf->Cache.NumberOfDataGroups * sizeof (MDF4_DATA_GROUP_CACHE));
    if (par_Mdf->Cache.DataGroups == NULL) {
        ErrorCleanCache(par_Mdf, "out of memory");
        return -1;
    }
    memset(&(par_Mdf->Cache.DataGroups[par_Mdf->Cache.NumberOfDataGroups - 1]), 0, sizeof (MDF4_DATA_GROUP_CACHE));
    par_Mdf->Cache.DataGroups[par_Mdf->Cache.NumberOfDataGroups - 1].SizeOfRecordID = par_SizeOfRecordID;
    par_Mdf->Cache.DataGroups[par_Mdf->Cache.NumberOfDataGroups - 1].CurrentBlock = -1;

    if (DebugOut != NULL) { fprintf (DebugOut, "Add data group to cache: %i, %i\n", par_Mdf->Cache.NumberOfDataGroups - 1,  (int)par_SizeOfRecordID); FFLUSH_DEBUGOUT; }

    return par_Mdf->Cache.NumberOfDataGroups - 1;
}


static int AddDataBlockToDataGroupCache(MDF4_STIMULI_FILE *par_Mdf, int par_IndexOfBlockGroup, uint64_t par_OffsetInsideFile,
                                        uint64_t par_SizeOfBlock,  uint64_t par_SizeOfDeflatedBlock,
                                        int par_InflateType, uint32_t par_Parameter)
{
    if (par_IndexOfBlockGroup < par_Mdf->Cache.NumberOfDataGroups) {
        MDF4_DATA_GROUP_CACHE *DataGroup = &par_Mdf->Cache.DataGroups[par_IndexOfBlockGroup];
        DataGroup->NoOfBlocks++;
        DataGroup->Blocks = (MDF4_DATA_BLOCK_CACHE*)my_realloc(DataGroup->Blocks, DataGroup->NoOfBlocks * sizeof (MDF4_DATA_BLOCK_CACHE));
        if (DataGroup->Blocks == NULL) {
            ErrorCleanCache(par_Mdf, "out of memory");
            return -1;
        }
        memset(&(DataGroup->Blocks[DataGroup->NoOfBlocks - 1]), 0, sizeof (MDF4_DATA_BLOCK_CACHE));
        DataGroup->Blocks[DataGroup->NoOfBlocks - 1].OffsetInsideFile = par_OffsetInsideFile;
        DataGroup->Blocks[DataGroup->NoOfBlocks - 1].SizeOfBlock = par_SizeOfBlock;
        DataGroup->Blocks[DataGroup->NoOfBlocks - 1].SizeOfDeflatedBlock = par_SizeOfDeflatedBlock;
        DataGroup->Blocks[DataGroup->NoOfBlocks - 1].InflateType = par_InflateType;
        DataGroup->Blocks[DataGroup->NoOfBlocks - 1].Parameter = par_Parameter;
        if (par_SizeOfBlock > DataGroup->LargestBlockSize) {
            DataGroup->LargestBlockSize = par_SizeOfBlock;
        }
        if (par_SizeOfDeflatedBlock > DataGroup->LargestDeflatedBlockSize) {
            DataGroup->LargestDeflatedBlockSize = par_SizeOfDeflatedBlock;
        }
        if (par_InflateType == 1) {
            DataGroup->HaveTransposedData = 1;
        }
        if (DebugOut != NULL) { fprintf (DebugOut, "  Add data block to group %i: offset=%llx, size=%llx\n", DataGroup->NoOfBlocks - 1, par_OffsetInsideFile, par_SizeOfBlock); FFLUSH_DEBUGOUT; }
        return DataGroup->NoOfBlocks - 1;
    } else {
        return -1;
    }
}

static int AddChannelGroupToCache(MDF4_STIMULI_FILE *par_Mdf, int par_BlockNumber, uint64_t par_ChannelId, int par_SizeOfDataRecord, uint32_t par_NumberOfRecords, char *par_Name)
{
    MDF4_DATA_GROUP_CACHE *DataBlock;
    if (par_BlockNumber >= par_Mdf->Cache.NumberOfDataGroups) {
        ErrorCleanCache(par_Mdf, "par_BlockNumber >= Mdf->Cache.NumberOfDataBlocks");
        return -1;
    }
    DataBlock = &(par_Mdf->Cache.DataGroups[par_BlockNumber]);
    DataBlock->NumberOfChannelGroups++;
    DataBlock->Records = (MDF4_RECORD_CACHE*)my_realloc(DataBlock->Records, DataBlock->NumberOfChannelGroups * sizeof(MDF4_RECORD_CACHE));
    if (DataBlock->Records == NULL) {
        ErrorCleanCache(par_Mdf, "out of memory");
        return -1;
    }
    memset(&DataBlock->Records[DataBlock->NumberOfChannelGroups - 1], 0, sizeof(MDF4_RECORD_CACHE));
    DataBlock->Records[DataBlock->NumberOfChannelGroups - 1].Id = par_ChannelId;
    DataBlock->Records[DataBlock->NumberOfChannelGroups - 1].SizeOfOneRecord = par_SizeOfDataRecord;
    DataBlock->Records[DataBlock->NumberOfChannelGroups - 1].NumberOfRecords = par_NumberOfRecords;
    DataBlock->Records[DataBlock->NumberOfChannelGroups - 1].Name = par_Name;
    if (DebugOut != NULL) { fprintf (DebugOut, "  Add channel group to block group %i: Id=%llu, size=%i, noof=%u, %s\n", par_BlockNumber, par_ChannelId, par_SizeOfDataRecord, par_NumberOfRecords, par_Name); FFLUSH_DEBUGOUT; }
    return DataBlock->NumberOfChannelGroups - 1;
}

static int TranslateChannelIdToIndex(MDF4_DATA_GROUP_CACHE *par_DataBlock, uint64_t par_Id)
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

static int AddVariableToCache(MDF4_STIMULI_FILE *par_Mdf, int par_BlockNumber, uint64_t par_ChannelId, int par_Vid,
                              int par_DataType, int par_BbDataType, int par_NumberOfBits, int par_StartBitOffset,
                              int par_TimeVarFlag, double par_TimeFac, double par_TimeOff, const char *par_Name)
{
    MDF4_DATA_GROUP_CACHE *DataBlock;
    MDF4_RECORD_CACHE *Record;
    int Idx;

    //check_memory_corrupted(__LINE__, __FILE__);
    if (par_BlockNumber >= par_Mdf->Cache.NumberOfDataGroups) {
        ErrorCleanCache(par_Mdf, "par_BlockNumber >= Mdf->Cache.NumberOfDataBlocks");
        return -1;
    }
    DataBlock = &(par_Mdf->Cache.DataGroups[par_BlockNumber]);
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
        if (DebugOut != NULL) { fprintf (DebugOut, "    Add time signal to block group %i: Id=%llu\n", par_BlockNumber, par_ChannelId); FFLUSH_DEBUGOUT; }
    } else {
        DataBlock->NoOfAllSignals++;
        Record->NumberOfEntrys++;
        Record->Entrys = (MDF4_RECORD_ENTRY*)my_realloc(Record->Entrys, Record->NumberOfEntrys * sizeof(MDF4_RECORD_ENTRY));
        if (Record->Entrys == NULL) {
            ErrorCleanCache(par_Mdf, "out of memory");
            return -1;
        }
        memset(&Record->Entrys[Record->NumberOfEntrys - 1], 0, sizeof(MDF4_RECORD_ENTRY));
        Record->Entrys[Record->NumberOfEntrys - 1].Vid = par_Vid;
        Record->Entrys[Record->NumberOfEntrys - 1].DataType = par_DataType;
        Record->Entrys[Record->NumberOfEntrys - 1].BbDataType = par_BbDataType;
        Record->Entrys[Record->NumberOfEntrys - 1].NumberOfBits = par_NumberOfBits;
        Record->Entrys[Record->NumberOfEntrys - 1].StartBitOffset = par_StartBitOffset;
        if (DebugOut != NULL) { fprintf (DebugOut, "    Add signal \"%s\" to block group %i: Id=%llu\n", par_Name, par_BlockNumber, par_ChannelId); FFLUSH_DEBUGOUT; }
    }
    //check_memory_corrupted(__LINE__, __FILE__);
    return DataBlock->NumberOfChannelGroups - 1;
}

static void DoubleToString(double par_Value, char *ret_String)
{
    int Prec = 15;
    while (1) {
        sprintf (ret_String, "%.*g", Prec, par_Value);
        if ((Prec++) == 18 || (par_Value == strtod (ret_String, NULL))) break;
    }
}

static int MdfReadTextRangeTable(MDF4_STIMULI_FILE *Mdf, int par_Vid, int ParListSize, double *ParList,
                                 int LinkListSize, uint64_t *LinkList)
{
    int x;
    int ReadResult;
    char *Conversion;
    char LowerRangeString[32];
    char UpperRangeString[32];
    MDF4_BLOCK_HEADER BlockHeader;
    int PosInConversion = 0;
    int Ret = 0;

    if ((ParListSize * 2) == (LinkListSize - 5)) {
        return -1;
    }

    Conversion = my_malloc(BBVARI_CONVERSION_SIZE);
    if (Conversion == NULL) {
        return -1;
    }

    for (x = 0; x < (LinkListSize - 5); x++) {
        int Len;
        DoubleToString(ParList[x * 2], LowerRangeString);
        DoubleToString(ParList[x * 2] + 1, UpperRangeString);
        Len = (int)strlen(LowerRangeString);
        strcpy(Conversion + PosInConversion, LowerRangeString);
        PosInConversion += Len;
        Conversion[PosInConversion] = ' ';
        PosInConversion++;
        Len = (int)strlen(UpperRangeString);
        strcpy(Conversion + PosInConversion, UpperRangeString);
        PosInConversion += Len;
        Conversion[PosInConversion] = ' ';
        PosInConversion++;
        Conversion[PosInConversion] = '\"';
        PosInConversion++;
        my_lseek(Mdf->fh, LinkList[x + 4]);
        ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
        if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
        if (strncmp("##TX", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROR;
        ReadResult = my_read (Mdf->fh, Conversion + PosInConversion, (uint32_t)(BlockHeader.BlockLength - sizeof(BlockHeader)));
        if (ReadResult != (int)BlockHeader.BlockLength - (int)sizeof(BlockHeader)) goto __ERROR;
        PosInConversion += ReadResult;
        while ((PosInConversion > 0) &&
               (Conversion[PosInConversion - 1] == 0)) PosInConversion--;
        Conversion[PosInConversion] = '\"';
        PosInConversion++;
        Conversion[PosInConversion] = ';';
        PosInConversion++;
    }

    Conversion[PosInConversion] = 0;
    Ret = set_bbvari_conversion(par_Vid, 2, Conversion);
__ERROR:
    if (Conversion != NULL) my_free (Conversion);
    return Ret;
}

static int MdfReadTextValueTable(MDF4_STIMULI_FILE *Mdf, int par_Vid, int ParListSize, double *ParList,
                                 int LinkListSize, uint64_t *LinkList)
{
    int x;
    int ReadResult;
    char *Conversion;
    char ValueString[32];
    MDF4_BLOCK_HEADER BlockHeader;
    int PosInConversion = 0;
    int Ret = 0;

    if (ParListSize != (LinkListSize - 5)) {
        return -1;
    }

    Conversion = my_malloc(BBVARI_CONVERSION_SIZE);
    if (Conversion == NULL) {
        return -1;
    }

    for (x = 0; x < (LinkListSize - 5); x++) {
        int Len;
        DoubleToString(ParList[x], ValueString);
        Len = (int)strlen(ValueString);
        strcpy(Conversion + PosInConversion, ValueString);
        PosInConversion += Len;
        Conversion[PosInConversion] = ' ';
        PosInConversion++;
        strcpy(Conversion + PosInConversion, ValueString);
        PosInConversion += Len;
        Conversion[PosInConversion] = ' ';
        PosInConversion++;
        Conversion[PosInConversion] = '\"';
        PosInConversion++;
        my_lseek(Mdf->fh, LinkList[x + 4]);
        ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
        if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
        if (strncmp("##TX", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROR;
        ReadResult = my_read (Mdf->fh, Conversion + PosInConversion, (uint32_t)(BlockHeader.BlockLength - sizeof(BlockHeader)));
        if (ReadResult != (int)BlockHeader.BlockLength - (int)sizeof(BlockHeader)) goto __ERROR;
        PosInConversion += ReadResult;
        while ((PosInConversion > 0) &&
               (Conversion[PosInConversion - 1] == 0)) PosInConversion--;
        Conversion[PosInConversion] = '\"';
        PosInConversion++;
        Conversion[PosInConversion] = ';';
        PosInConversion++;
    }

    Conversion[PosInConversion] = 0;
    Ret = set_bbvari_conversion(par_Vid, 2, Conversion);
__ERROR:
    if (Conversion != NULL) my_free (Conversion);
    return Ret;
}

static int MdfGetConversion (MDF4_STIMULI_FILE *Mdf, uint64_t par_ConversionOffset, int par_Vid, double *ret_Fac, double *ret_Off)
{
    int ReadResult;
    MDF4_BLOCK_HEADER BlockHeader;
    MDF4_CCBLOCK MdfCcBlock;
    uint64_t *LinkList = NULL;
    int LinkListSize = 0;
    double *ParList = NULL;
    int ParListSize = 0;

    my_lseek(Mdf->fh, par_ConversionOffset);
    ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
    if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
    if (strncmp("##CC", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROR;
    LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
    if (LinkList == NULL) goto __ERROR;
    ReadResult = my_read (Mdf->fh, &MdfCcBlock, sizeof (MdfCcBlock));
    if (ReadResult != sizeof (MdfCcBlock)) goto __ERROR;

    if ((par_Vid > 0) &&
        ((MdfCcBlock.Flags & 0x8) == 0x8)) {   // Ranges are valid
        set_bbvari_min(par_Vid, MdfCcBlock.RangeMin);
        set_bbvari_max(par_Vid, MdfCcBlock.RangeMax);
    }
    if ((par_Vid > 0) &&
        ((MdfCcBlock.Flags & 0x4) == 0x4)) {   // Precision are valid
        set_bbvari_format(par_Vid, 20, MdfCcBlock.Precision);
    }
    if  ((par_Vid > 0) &&
         (LinkList[1] > 0)) {
        // LinkList[1] == Unit
        char *Unit = NULL;
        int LenOfUnit = 0;
        Unit = ReadString(Mdf->fh, LinkList[1], Unit, &LenOfUnit);
        if (Unit != NULL) {
            set_bbvari_unit(par_Vid, Unit);
            my_free(Unit);
        }
    }

    ParList = ReadMdf4BlockParList(Mdf->fh, ParList, &ParListSize,
                                   MdfCcBlock.NoOfValues);

    switch(MdfCcBlock.Type) {
    case 0:  // no conversion
        break;
    case 1:  // linear
        if (MdfCcBlock.NoOfValues == 2) {
            char Help[64];
            if  (par_Vid > 0) {
                sprintf (Help, "$*%g+%g", ParList[1], ParList[0]);
                set_bbvari_conversion(par_Vid, 1, Help);
            } else {
                if (ret_Fac != NULL) *ret_Fac = ParList[1];
                if (ret_Off != NULL) *ret_Off = ParList[0];
            }
        }
        break;
    case 2: // rational
   // case 3: // algebraic
        if (MdfCcBlock.NoOfValues == 6) {
            if  (par_Vid > 0) {
                char Help[256];
                sprintf (Help, "(%g*$*$+%g*$+%g)/(%g*$*$+%g*$+%g)",
                         ParList[0], ParList[1], ParList[2],
                         ParList[3], ParList[4], ParList[5]);
                set_bbvari_conversion(par_Vid, 1, Help);
            }
        }
        break;
    case 4: // Value to value interpolation
    case 5: // Value to value
    case 6: // Value range to value
        // not implemented
        goto __ERROR;
    case 7: // Value to text
        if (MdfReadTextValueTable(Mdf, par_Vid, MdfCcBlock.NoOfValues, ParList, (int)BlockHeader.LinkCount, LinkList)) {
            goto __ERROR;
        }
        break;
    case 8:
        if (MdfReadTextRangeTable(Mdf, par_Vid, MdfCcBlock.NoOfValues, ParList, (int)BlockHeader.LinkCount, LinkList)) {
            goto __ERROR;
        }
        break;
    default:
        // all others are not implemented
        goto __ERROR;
    }
    return 0;
__ERROR:
    return -1;
}

static int MdfTranslateDatatypeToBlackboardType (int par_DataType, int par_BitSize)
{
    switch(par_DataType) {
    case 0: //    0 = unsigned integer little endian
    case 1: //    1 = unsigned integer big endian
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
    case 2: //    2 = signed integer little endian
    case 3: //    3 = signed integer big endian
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
    case 4: //    4 = IEEE 754 floating-point format little endian
    case 5: //    5 = IEEE 754 floating-point format big endian
        if (par_BitSize <= 32) {
            return BB_UNKNOWN_FLOAT;
        } else if (par_BitSize <= 64) {
            return BB_UNKNOWN_DOUBLE;
        }
        break;
    default:
        break;
    }
    return BB_INVALID;
}


int LinkedListOfDataBlocks(MDF4_STIMULI_FILE *Mdf, int par_IndexOfDataGroup, uint64_t *par_LinkList, int par_NoOf)
{
    MDF4_BLOCK_HEADER BlockHeader;
    int ReadResult;
    int x;
    int Ret = 0;

    for (x = 0; x < (int)par_NoOf; x++) {
        uint64_t DataFilePosition;
        uint64_t DataFileBlockSize;
        my_lseek(Mdf->fh, par_LinkList[x]);
        ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
        if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
        if (!strncmp("##DZ", BlockHeader.BlockTypeIdentifier, 4)) {
            MDF4_DZBLOCK MdfDzBlock;
            // Have no link list!
            ReadResult = my_read (Mdf->fh, &MdfDzBlock, sizeof (MdfDzBlock));
            if (ReadResult != sizeof (MdfDzBlock)) goto __ERROR;
            DataFilePosition = my_ftell(Mdf->fh);
            // add new data block to cache
            AddDataBlockToDataGroupCache(Mdf, par_IndexOfDataGroup, DataFilePosition,
                                         MdfDzBlock.OrigDataLength, MdfDzBlock.DataLength,
                                         MdfDzBlock.Type, MdfDzBlock.Parameter);
        } else if (!strncmp("##DT", BlockHeader.BlockTypeIdentifier, 4)) {
            DataFilePosition = my_ftell(Mdf->fh);
            DataFileBlockSize = BlockHeader.BlockLength - sizeof(BlockHeader);
            AddDataBlockToDataGroupCache(Mdf, par_IndexOfDataGroup, DataFilePosition, DataFileBlockSize,
                                         0, -1, 0);
        } else if (!strncmp("##DL", BlockHeader.BlockTypeIdentifier, 4)) {
            MDF4_DLBLOCK MdfDlBlock;
            uint64_t *LinkList = NULL;
            int LinkListSize = 0;
            LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
            if (LinkList == NULL) goto __ERROR;
            ReadResult = my_read (Mdf->fh, &MdfDlBlock, sizeof (MdfDlBlock));
            if (ReadResult != sizeof (MdfDlBlock)) goto __ERROR;
            Ret = LinkedListOfDataBlocks(Mdf, par_IndexOfDataGroup, LinkList + 1, MdfDlBlock.NoOfBlocks);
            my_free(LinkList);
            if (Ret) break;
        } else {
            ThrowError(1, "not expected data block \"%.4s inside block list\"", BlockHeader.BlockTypeIdentifier);
            Ret = -1;
            goto __ERROR;
        }
    }
__ERROR:
    return Ret;
}


STIMULI_FILE *Mdf4OpenAndReadStimuliHeader (const char *par_Filename, const char *par_Variables)
{
    int VariableCount;
    MDF4_IDBLOCK MdfIdBlock;
    MDF4_HDBLOCK MdfHdBlock;
    uint64_t *LinkList = NULL;
    int LinkListSize = 0;

    uint64_t DataGroupBlockOffset;
    uint64_t ChannelGroupBlockOffset;
    uint64_t ChannelBlockOffset;
    int ReadResult;
    const char *vlp;
    int BlockNumber = 0;

    MDF4_BLOCK_HEADER BlockHeader;
    STIMULI_FILE *Ret = NULL;
    MDF4_STIMULI_FILE *Mdf = NULL;
    char Filename[MAX_PATH];

    SearchAndReplaceEnvironmentStrings(par_Filename, Filename, sizeof(Filename));

    Ret = (STIMULI_FILE*)my_calloc(1, sizeof(STIMULI_FILE));
    if (Ret == NULL) {
        ThrowError(1, "out of memmory");
        goto __ERROR;
    }
    Mdf = (MDF4_STIMULI_FILE*)my_calloc(1, sizeof(MDF4_STIMULI_FILE));
    if (Mdf == NULL) {
        ThrowError(1, "out of memmory");
        goto __ERROR;
    }
    Ret->File = (void*)Mdf;
    Ret->FileType = MDF4_FILE;

    Mdf->fh = my_open(Filename);
    if (Mdf->fh == MY_INVALID_HANDLE_VALUE) {
        ThrowError (1, "Unable to open %s file\n", Filename);
        goto __ERROR;
    }
    //LogFileAccess (Filename);
#ifndef DebugOut
    DebugOut = fopen ("c:\\temp\\debug_mdf4.txt", "wt");
#endif

    Mdf->FirstTimeStamp = 0xFFFFFFFFFFFFFFFFULL;
    Mdf->SizeOfFile = (uint32_t)my_get_file_size(Mdf->fh);


    ReadResult = my_read (Mdf->fh, &MdfIdBlock, sizeof (MdfIdBlock));
    if (ReadResult != sizeof (MdfIdBlock)) {
        my_close (Mdf->fh);
        return NULL;
    }
    if (strncmp("MDF     ", MdfIdBlock.FileIdentifier, 8)) {
        my_close (Mdf->fh);
        return NULL;
    }
    ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
    if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
    if (strncmp("##HD", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROR;

    LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
    if (LinkList == NULL) goto __ERROR;


    ReadResult = my_read (Mdf->fh, &MdfHdBlock, sizeof (MdfHdBlock));
    if (ReadResult != sizeof (MdfHdBlock)) goto __ERROR;

    DataGroupBlockOffset = LinkList[0];

    // now read the file history block
    /*if (LinkList[1] > 0) {
        MDF4_FHBLOCK MdfFhBlock;
        my_lseek(Mdf->fh, LinkList[1]);
        ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
        if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
        if (strncmp("##FH", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROR;
        LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
        if (LinkList == NULL) goto __ERROR;
        ReadResult = my_read (Mdf->fh, &MdfFhBlock, sizeof (MdfFhBlock));
        if (ReadResult != sizeof (MdfFhBlock)) goto __ERROR;
    }*/

    VariableCount = 0;
    while (DataGroupBlockOffset) {
        MDF4_DGBLOCK MdfDgBlock;
        uint64_t DataFilePosition;
        uint64_t DataFileBlockSize;
        int IndexOfDataGroup;
        my_lseek(Mdf->fh, DataGroupBlockOffset);

        ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
        if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
        if (strncmp("##DG", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROR;
        LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
        if (LinkList == NULL) goto __ERROR;
        ReadResult = my_read (Mdf->fh, &MdfDgBlock, sizeof (MdfDgBlock));
        if (ReadResult != sizeof (MdfDgBlock)) goto __ERROR;

        DataGroupBlockOffset = LinkList[0];  // 0 -> Next
        ChannelGroupBlockOffset = LinkList[1];   // 1 -> first CG

        if (LinkList[2] > 0) {
            my_lseek(Mdf->fh, LinkList[2]); // 2 -> Index data
            ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
            if (ReadResult != sizeof (BlockHeader)) goto __ERROR;

            // add new data group to cache
            IndexOfDataGroup = AddDataGroupToCache(Mdf, MdfDgBlock.RecIdSize);

            if (!strncmp("##DT", BlockHeader.BlockTypeIdentifier, 4)) {
                // Have no link list!
                DataFilePosition = my_ftell(Mdf->fh);
                DataFileBlockSize = BlockHeader.BlockLength - sizeof(BlockHeader);

                // add new data block to cache
                AddDataBlockToDataGroupCache(Mdf, IndexOfDataGroup, DataFilePosition, DataFileBlockSize,
                                             0, -1, 0);
           } else if (!strncmp("##DZ", BlockHeader.BlockTypeIdentifier, 4)) {
                MDF4_DZBLOCK MdfDzBlock;
                // Have no link list!
                ReadResult = my_read (Mdf->fh, &MdfDzBlock, sizeof (MdfDzBlock));
                if (ReadResult != sizeof (MdfDzBlock)) goto __ERROR;
                DataFilePosition = my_ftell(Mdf->fh);
                // add new data block to cache
                AddDataBlockToDataGroupCache(Mdf, IndexOfDataGroup, DataFilePosition,
                                             MdfDzBlock.OrigDataLength, MdfDzBlock.DataLength,
                                             MdfDzBlock.Type, MdfDzBlock.Parameter);
            } else if (!strncmp("##HL", BlockHeader.BlockTypeIdentifier, 4)) {
                MDF4_HLBLOCK MdfHlBlock;
                LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
                ReadResult = my_read (Mdf->fh, &MdfHlBlock, sizeof (MdfHlBlock));
                if (ReadResult != sizeof (MdfHlBlock)) goto __ERROR;

                LinkedListOfDataBlocks(Mdf, IndexOfDataGroup, LinkList, BlockHeader.LinkCount);

            } else if (!strncmp("##DL", BlockHeader.BlockTypeIdentifier, 4)) {
                MDF4_DLBLOCK MdfDlBlock;
                LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
                if (LinkList == NULL) goto __ERROR;
                ReadResult = my_read (Mdf->fh, &MdfDlBlock, sizeof (MdfDlBlock));
                if (ReadResult != sizeof (MdfDlBlock)) goto __ERROR;

                if (MdfDlBlock.NoOfBlocks != (BlockHeader.LinkCount - 1)) {
                    ThrowError(1, "MdfDlBlock.NoOfBlocks != (BlockHeader.LinkCount - 1)");
                }
                LinkedListOfDataBlocks(Mdf, IndexOfDataGroup, LinkList + 1, MdfDlBlock.NoOfBlocks);

            } else {
                ThrowError(1, "not expected data block \"%.4s\"", BlockHeader.BlockTypeIdentifier);
                goto __ERROR;
            }
            while (ChannelGroupBlockOffset) {
                MDF4_CGBLOCK MdfCgBlock;
                char *Name;
                int NameLen;
                my_lseek(Mdf->fh, ChannelGroupBlockOffset);

                ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
                if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
                if (strncmp("##CG", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROR;
                LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
                if (LinkList == NULL) goto __ERROR;
                ReadResult = my_read (Mdf->fh, &MdfCgBlock, sizeof (MdfCgBlock));
                if (ReadResult != sizeof (MdfCgBlock)) goto __ERROR;

                ChannelGroupBlockOffset = LinkList[0];  // 0 -> Next
                ChannelBlockOffset = LinkList[1];   // 1 -> first CN
                NameLen = 0;
                Name = ReadString(Mdf->fh, LinkList[2], NULL, &NameLen);

                // add new channel block to cache
                if (AddChannelGroupToCache(Mdf, BlockNumber, MdfCgBlock.RecordId, MdfCgBlock.NoOfDataBytes, (uint32_t)MdfCgBlock.NoOfSamples, Name) < 0) {
                    goto __ERROR;
                }

                if (MdfCgBlock.NoOfDataBytes > Mdf->Cache.DataGroups[BlockNumber].LargesRecord) {
                    Mdf->Cache.DataGroups[BlockNumber].LargesRecord = MdfCgBlock.NoOfDataBytes;
                }

                while (ChannelBlockOffset) {
                    MDF4_CNBLOCK MdfCnBlock;
                    my_lseek(Mdf->fh, ChannelBlockOffset);

                    ReadResult = my_read (Mdf->fh, &BlockHeader, sizeof (BlockHeader));
                    if (ReadResult != sizeof (BlockHeader)) goto __ERROR;
                    if (strncmp("##CN", BlockHeader.BlockTypeIdentifier, 4)) goto __ERROR;
                    LinkList = ReadMdf4BlockLinkList(Mdf->fh, LinkList, &LinkListSize, (int)BlockHeader.LinkCount);
                    if (LinkList == NULL) goto __ERROR;
                    ReadResult = my_read (Mdf->fh, &MdfCnBlock, sizeof (MdfCnBlock));
                    if (ReadResult != sizeof (MdfCnBlock)) goto __ERROR;

                    ChannelBlockOffset = LinkList[0];  // 0 -> Next

                    if (MdfCnBlock.Type == 0) {
                        if (IsValidDataType(MdfCnBlock.DataType, MdfCnBlock.BitCount)) {
                            char *SignalName = NULL;
                            int SizeOfSignalName = 0;
                            // SignalName muss mit my_free wieder freigegeben werden
                            SignalName = ReadString(Mdf->fh, LinkList[2], SignalName, &SizeOfSignalName);

                            // schaue ob Variable eingespielt werden soll
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
                                        int BbDataType = MdfTranslateDatatypeToBlackboardType(MdfCnBlock.DataType, MdfCnBlock.BitCount);
                                        Mdf->vids[VariableCount] = add_bbvari (SignalName, BbDataType, NULL);
                                        Mdf->dtypes[VariableCount] = get_bbvaritype (Mdf->vids[VariableCount]);
                                        // Neue Variable hinzufuegen
                                        AddVariableToCache(Mdf, BlockNumber, MdfCgBlock.RecordId, Mdf->vids[VariableCount], MdfCnBlock.DataType, Mdf->dtypes[VariableCount],
                                                           MdfCnBlock.BitCount, MdfCnBlock.BitOffset + MdfCnBlock.ByteOffset * 8, 0, 0.0, 0.0, SignalName);
                                        if (LinkList[4] != 0) {  // CC == 4
                                            MdfGetConversion(Mdf, LinkList[4], Mdf->vids[VariableCount], NULL, NULL);   // CC == 4
                                        }
                                        VariableCount++;
                                        break;
                                    }
                                    while (*vlp != '\0') vlp++;
                                    vlp++;
                                }
                            } else {
                                // Alle Variable
                                Mdf->vids[VariableCount] = add_bbvari (SignalName, BB_UNKNOWN_DOUBLE, NULL);
                                Mdf->dtypes[VariableCount] = get_bbvaritype (Mdf->vids[VariableCount]);
                                // Neue Variable hinzufuegen
                                AddVariableToCache(Mdf, BlockNumber, MdfCgBlock.RecordId, Mdf->vids[VariableCount], MdfCnBlock.DataType, Mdf->dtypes[VariableCount],
                                                   MdfCnBlock.BitCount, MdfCnBlock.BitOffset + MdfCnBlock.ByteOffset * 8, 0, 0.0, 0.0, SignalName);
                                if (LinkList[4] != 0) {
                                    MdfGetConversion(Mdf, LinkList[4], Mdf->vids[VariableCount], NULL, NULL);
                                }
                                VariableCount++;
                            }
                            my_free(SignalName);
                            VariableCount++;
                        }
                    } else if (MdfCnBlock.Type == 1) {
                        // variable length signal data (VLSD)
                        printf ("variable length signal are not supported");
                    } else if ((MdfCnBlock.Type == 2) && (MdfCnBlock.SyncType == 1)) {  // 2 -> Master time axis
                        double TimeVariableFactor = 1.0;
                        double TimeVariableOffset = 0.0;
                        if (LinkList[4] != 0) {  // CC == 4
                            MdfGetConversion(Mdf, LinkList[4], 0, &TimeVariableFactor, &TimeVariableOffset);
                        }
                        AddVariableToCache(Mdf, BlockNumber, MdfCgBlock.RecordId, 0, MdfCnBlock.DataType, 0,
                                           MdfCnBlock.BitCount, MdfCnBlock.BitOffset + MdfCnBlock.ByteOffset * 8, 1,
                                           TimeVariableFactor, TimeVariableOffset, "time");
                        //printf("time channel");
                    } else {
                        //printf("unknown");
                    }
                }
            }
            BlockNumber++;
        }
    }
    Mdf->variable_count = VariableCount;
    return Ret;
__ERROR:
    ThrowError(1, "cannot read from file \"%s\"", Filename);
    if (Ret != NULL) my_free(Ret);
    if (Mdf != NULL) {
        CleanCache(Mdf);
        if (Mdf->fh != NULL) my_close(Mdf->fh);
        if (Mdf->vids != NULL) my_free(Mdf->vids);
        if (Mdf->dtypes != NULL) my_free(Mdf->dtypes);
        my_free(Mdf);
    }
    return NULL;
}

int Mdf4GetNumberOfStimuliVariables(STIMULI_FILE *par_File)
{
    MDF4_STIMULI_FILE *Mdf = (MDF4_STIMULI_FILE*)par_File->File;
    return (Mdf->variable_count);

}

static void InverseTranspose(unsigned char *par_In, unsigned char *ret_Out, int par_Size, int par_RecordSize)
{
    int RowCount = par_Size / par_RecordSize;
    for (int Row = 0; Row < RowCount; Row++) {
        for (int Column = 0; Column < par_RecordSize; Column++) {
            int Dst = Row * par_RecordSize + Column;
            int Src = Column * RowCount + Row;
            ret_Out[Dst] = par_In[Src];
        }
    }
}

static int UpdateCache(MDF4_STIMULI_FILE *par_Mdf, MDF4_DATA_GROUP_CACHE *DataBlock)
{
    int Readed;
    unsigned char *Data = NULL;

    if (DataBlock->BlockData == NULL) {
        DataBlock->BlockData = my_malloc (DataBlock->LargestBlockSize + DataBlock->LargesRecord);
        if (DataBlock->BlockData == NULL) {
            ThrowError(1, "cannot alloc memory");
            return -1;
        }
        Data = DataBlock->BlockData;
        DataBlock->CurrentBlock = 0;
        DataBlock->StartCacheFilePos = DataBlock->Blocks[DataBlock->CurrentBlock].OffsetInsideFile;
        DataBlock->CurrentFilePos = DataBlock->StartCacheFilePos;
        DataBlock->SizeOfBlock = DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfBlock;
        DataBlock->EndCacheFilePos = DataBlock->StartCacheFilePos + DataBlock->SizeOfBlock;
        // if we have compressed parts also setup a buffer for the copressed data
        if (DataBlock->LargestDeflatedBlockSize > 0) {
            DataBlock->BlockDeflatedData = my_malloc (DataBlock->LargestDeflatedBlockSize);
            if (DataBlock->BlockDeflatedData == NULL) {
                ThrowError(1, "cannot alloc memory");
                return -1;
            }
            if (DataBlock->HaveTransposedData) {
                DataBlock->BlockDeflatedAndTransposedData = my_malloc (DataBlock->LargestBlockSize);
                if (DataBlock->BlockDeflatedAndTransposedData == NULL) {
                    ThrowError(1, "cannot alloc memory");
                    return -1;
                }
            }
        }
    } else {
        // copy the left bytes to the beginning of the cache
        if (DataBlock->CurrentBlock < DataBlock->NoOfBlocks) {
            uint64_t Left = DataBlock->EndCacheFilePos - DataBlock->CurrentFilePos;
            if (Left > 0) {
                memcpy (DataBlock->BlockData,
                        DataBlock->BlockData + (DataBlock->CurrentFilePos - DataBlock->StartCacheFilePos),
                        Left);
            }
            Data = DataBlock->BlockData + Left;
            DataBlock->StartCacheFilePos = DataBlock->Blocks[DataBlock->CurrentBlock].OffsetInsideFile - Left;
            DataBlock->CurrentFilePos = DataBlock->StartCacheFilePos;
            DataBlock->SizeOfBlock = DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfBlock + Left;
            DataBlock->EndCacheFilePos = DataBlock->StartCacheFilePos + DataBlock->SizeOfBlock;
        }
    }
    // Switch to next block if there are one
    if ((Data != NULL) && (DataBlock->CurrentBlock < DataBlock->NoOfBlocks)) {
        if (DebugOut != NULL) { fprintf (DebugOut, "Update cache %i: offset=%llx, size=%llx\n",
                                         (int)(DataBlock - par_Mdf->Cache.DataGroups),
                                         DataBlock->Blocks[DataBlock->CurrentBlock].OffsetInsideFile, DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfBlock); FFLUSH_DEBUGOUT; }

        my_lseek(par_Mdf->fh, DataBlock->Blocks[DataBlock->CurrentBlock].OffsetInsideFile);
        if (DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfDeflatedBlock > 0) {
            if (DataBlock->Blocks[DataBlock->CurrentBlock].InflateType == 0) {
                z_stream s;
                int Ret;
                memset(&s, 0, sizeof(s));
                Ret = inflateInit(&s);
                if (Ret != Z_OK) {
                    ThrowError(1, "cannot inflate");
                    return -1;
                }
                // it is a compressed data block
                Readed = my_read(par_Mdf->fh, DataBlock->BlockDeflatedData, (uint32_t)DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfDeflatedBlock);
                // now we should decompress the data
                s.avail_in = DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfDeflatedBlock;
                s.next_in = DataBlock->BlockDeflatedData;
                s.avail_out = DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfBlock;
                s.next_out = Data;
                Ret = inflate(&s, Z_NO_FLUSH);
                if (Ret != Z_STREAM_END) {
                    inflateEnd(&s);
                    ThrowError(1, "cannot inflate");
                    return -1;
                }
                inflateEnd(&s);
            } else if (DataBlock->Blocks[DataBlock->CurrentBlock].InflateType == 1) {
                z_stream s;
                int Ret;
                memset(&s, 0, sizeof(s));
                Ret = inflateInit(&s);
                if (Ret != Z_OK) {
                    ThrowError(1, "cannot inflate");
                    return -1;
                }
                // it is a compressed data block
                Readed = my_read(par_Mdf->fh, DataBlock->BlockDeflatedData, (uint32_t)DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfDeflatedBlock);
                // now we should decompress the data
                s.avail_in = DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfDeflatedBlock;
                s.next_in = DataBlock->BlockDeflatedData;
                s.avail_out = DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfBlock;
                s.next_out = DataBlock->BlockDeflatedAndTransposedData;
                Ret = inflate(&s, Z_NO_FLUSH);
                if (Ret != Z_STREAM_END) {
                    inflateEnd(&s);
                    ThrowError(1, "cannot inflate");
                    return -1;
                }
                inflateEnd(&s);
                InverseTranspose(DataBlock->BlockDeflatedAndTransposedData, Data, DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfBlock,
                                 DataBlock->Blocks[DataBlock->CurrentBlock].Parameter);
            } else {
                ThrowError(1, "unknown compression type %i", DataBlock->Blocks[DataBlock->CurrentBlock].InflateType);
                return -1;
            }
        } else {
            Readed = my_read(par_Mdf->fh, Data, (uint32_t)DataBlock->Blocks[DataBlock->CurrentBlock].SizeOfBlock);
        }
        DataBlock->CurrentBlock++;
        if (Readed == (int)DataBlock->SizeOfBlock) {
            return 0;
        } else {
            return -1;
        }
    } else {
        DataBlock->CurrentBlock++;
        return -1;
    }
}

__inline static int ConvWithSign (uint64_t value, int size, int sign, union FloatOrInt64 *ret_value)
{
    if (sign) {
        if ((0x1ULL << (size - 1)) & value) { // ist Vorzeichenbit gesetzt?
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

    if (par_ByteOrder == 1) {  // 1 -> Motorola
        int32_t PosByte_m8;
        uint32_t IStartBit = (uint32_t)((par_RecordSize << 3) - par_StartBit);
        PosBit = IStartBit & 0x7;
        PosByte = IStartBit >> 3;
        PosByte_m8 = (int32_t)PosByte - 8;
        Ret = _byteswap_uint64 (*(uint64_t *)(void*)(par_Data + PosByte_m8)) << PosBit;
        Ret |= (uint64_t)par_Data[PosByte] >> (8 - PosBit);
    } else {  // 0 -> Intel
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

static int ConvertValue(unsigned char* par_Data, int par_DataType, int par_StartBit, int par_BitSize, int par_RecordSize,
                        union FloatOrInt64 *ret_Value)
{
    uint64_t HelpU64;
    switch(par_DataType) {
    case 0: //    0 = unsigned integer little endian
        HelpU64 = GetValue(par_Data, 0, par_StartBit, par_BitSize, par_RecordSize);
        return ConvWithSign(HelpU64, par_BitSize, 0, ret_Value);
        break;
    case 1: //    0 = unsigned integer big endian
        HelpU64 = GetValue(par_Data, 1, par_StartBit, par_BitSize, par_RecordSize);
        return ConvWithSign(HelpU64, par_BitSize, 0, ret_Value);
        break;
    case 2: //    2 = signed integer (twos complement) little endian
        HelpU64 = GetValue(par_Data, 0, par_StartBit, par_BitSize, par_RecordSize);
        return ConvWithSign(HelpU64, par_BitSize, 1, ret_Value);
        break;
    case 3: //    3 = signed integer (twos complement) big endian
        HelpU64 = GetValue(par_Data, 1, par_StartBit, par_BitSize, par_RecordSize);
        return ConvWithSign(HelpU64, par_BitSize, 1, ret_Value);
        break;
    case 4: //    4 = IEEE 754 floating-point format little endian
        HelpU64 = GetValue(par_Data, 0, par_StartBit, par_BitSize, par_RecordSize);
        if (par_BitSize == 32) {
            ret_Value->d = *(float*)&HelpU64;
            return FLOAT_OR_INT_64_TYPE_F64;
        } else if (par_BitSize == 64) {
            ret_Value->d = *(double*)&HelpU64;
            return FLOAT_OR_INT_64_TYPE_F64;
        } else {
            return FLOAT_OR_INT_64_TYPE_INVALID;
        }
        break;
    case 5: //    5 = IEEE 754 floating-point format big endian
        HelpU64 = GetValue(par_Data, 1, par_StartBit, par_BitSize, par_RecordSize);
        if (par_BitSize == 32) {
            ret_Value->d = *(float*)&HelpU64;
            return FLOAT_OR_INT_64_TYPE_F64;
        } else if (par_BitSize == 64) {
            ret_Value->d = *(double*)&HelpU64;
            return FLOAT_OR_INT_64_TYPE_F64;
        } else {
            return FLOAT_OR_INT_64_TYPE_INVALID;
        }
        break;
    default:
        return FLOAT_OR_INT_64_TYPE_INVALID;
        break;
    }
}

static uint64_t ConvertTimeVariableToUInt64(MDF4_RECORD_CACHE *par_Record, unsigned char *par_Cache, uint64_t CachePos)
{
    union FloatOrInt64 Value;
    switch(ConvertValue(par_Cache + CachePos, par_Record->TimeVariableDataType, par_Record->TimeVariableStartBitOffset,
                        par_Record->TimeVariableNumberOfBits, par_Record->SizeOfOneRecord, &Value)) {
    case FLOAT_OR_INT_64_TYPE_F64:
        if (par_Record->TimeAreInNanoSec) {
            return (uint64_t)(Value.d); // sind schon Nanosekunden
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

static int GetTimeOfCurrentRecord(MDF4_STIMULI_FILE *par_Mdf, MDF4_DATA_GROUP_CACHE *par_DataBlock, uint64_t *ret_t)
{
    int Idx;
    uint16_t Id;
    uint64_t CachePos;
    MDF4_RECORD_CACHE *Record;

    if (par_DataBlock->NoOfAllSignals == 0) {
        return STIMULI_END_OF_FILE;   // no signal selcted from this block
    }

    if (par_DataBlock->CurrentBlock > par_DataBlock->NoOfBlocks) {
        return STIMULI_END_OF_FILE;
    }
    if ((par_DataBlock->CurrentFilePos + par_DataBlock->LargesRecord) >= par_DataBlock->EndCacheFilePos) {
        UpdateCache(par_Mdf, par_DataBlock);
    }
    CachePos = par_DataBlock->CurrentFilePos - par_DataBlock->StartCacheFilePos;
    if (par_DataBlock->SizeOfRecordID == 0) {
        Idx = 0;
    } else {
        unsigned char *p = par_DataBlock->BlockData + CachePos;
        Id = *p;
        Idx = TranslateChannelIdToIndex(par_DataBlock, Id);
        CachePos += sizeof(uint8_t);
    }
    Record = &(par_DataBlock->Records[Idx]);

    *ret_t = ConvertTimeVariableToUInt64(Record, par_DataBlock->BlockData, CachePos);

    /*if (DebugOut != NULL) { fprintf (DebugOut, "Time %i: cache pos=%llu ret=%llu\n",
                                     (int)(par_DataBlock - par_Mdf->Cache.DataGroups), CachePos, *ret_t);
                                     FFLUSH_DEBUGOUT; }
*/
    return 0;
}

static int AddCurrentRecordToSampleSlot(MDF4_DATA_GROUP_CACHE *par_DataBlock,
                                        VARI_IN_PIPE *pipevari_list, int par_PipePos)
{
    int Idx;
    MDF4_RECORD_CACHE *Record;
    MDF4_RECORD_ENTRY *Entry;
    int v;
    union FloatOrInt64 Value;
    int Type;
    uint64_t CachePos;
    int Pos = par_PipePos;

    CachePos = par_DataBlock->CurrentFilePos - par_DataBlock->StartCacheFilePos;
    if (par_DataBlock->SizeOfRecordID == 0) {
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
    par_DataBlock->CurrentFilePos += Record->SizeOfOneRecord + (sizeof (uint8_t) * par_DataBlock->SizeOfRecordID);
    return Pos;
}

int Mdf4ReadOneStimuliTimeSlot(STIMULI_FILE *par_File, VARI_IN_PIPE *pipevari_list, uint64_t *ret_t)
{
    MDF4_DATA_GROUP_CACHE *DataBlock;
    MDF4_DATA_GROUP_CACHE *EarliestDataBlock = NULL;
    int b, bb;
    int ValueCount = 0;
    uint64_t Time;
    uint64_t EarliestTime = 0xFFFFFFFFFFFFFFFFULL;
    int RecordState;
    // static FILE *fh;

    MDF4_STIMULI_FILE *Mdf = (MDF4_STIMULI_FILE*)par_File->File;

    for (b = 0; b < Mdf->Cache.NumberOfDataGroups; b++) {
        DataBlock = &(Mdf->Cache.DataGroups[b]);

        RecordState = GetTimeOfCurrentRecord(Mdf, DataBlock, &Time);
        if (RecordState == 0) {
            if (Time < EarliestTime) {
                EarliestTime = Time;
                EarliestDataBlock = DataBlock;
                bb = b;
            }
        }
    }

    /*if (fh == NULL) {
        fh = fopen("c:\\temp\\timestamps.txt", "wt");
    }
    fprintf (fh, "%i: %llu\n", bb, EarliestTime);
    fflush(fh);*/
    if (EarliestDataBlock == NULL) {
        return STIMULI_END_OF_FILE;   // alle Records in allen DataBlocks gelesen
    } else {
        ValueCount = AddCurrentRecordToSampleSlot(EarliestDataBlock, pipevari_list, 0);
        if (EarliestTime < Mdf->FirstTimeStamp) {
            Mdf->FirstTimeStamp = EarliestTime;
        }
        *ret_t = EarliestTime - Mdf->FirstTimeStamp;
        return ValueCount;
    }
}

static void FreeDataStruct(MDF4_STIMULI_FILE *par_Mdf)
{
    if (par_Mdf != NULL) {
        int x;
        if (par_Mdf->fh != NULL) my_close(par_Mdf->fh);
        if (par_Mdf->vids != NULL) my_free(par_Mdf->vids);
        if (par_Mdf->dtypes != NULL) my_free(par_Mdf->dtypes);
        for (x = 0; x < par_Mdf->Cache.NumberOfDataGroups; x++) {
            if (par_Mdf->Cache.DataGroups[x].Records != NULL) {
                int r;
                for (r = 0; r < par_Mdf->Cache.DataGroups[x].NumberOfChannelGroups; r++) {
                    if (par_Mdf->Cache.DataGroups[x].Records[r].Name != NULL) {
                        my_free(par_Mdf->Cache.DataGroups[x].Records[r].Name);
                    }
                }
                my_free(par_Mdf->Cache.DataGroups[x].Records);
            }
            if (par_Mdf->Cache.DataGroups[x].BlockData != NULL) my_free(par_Mdf->Cache.DataGroups[x].BlockData);
            if (par_Mdf->Cache.DataGroups[x].BlockDeflatedData != NULL) my_free(par_Mdf->Cache.DataGroups[x].BlockDeflatedData);
            if (par_Mdf->Cache.DataGroups[x].BlockDeflatedAndTransposedData != NULL) my_free(par_Mdf->Cache.DataGroups[x].BlockDeflatedAndTransposedData);
            if (par_Mdf->Cache.DataGroups[x].Blocks) my_free(par_Mdf->Cache.DataGroups[x].Blocks);
        }
        my_free(par_Mdf->Cache.DataGroups);
        my_free(par_Mdf);
    }
}

void Mdf4CloseStimuliFile(STIMULI_FILE *par_File)
{
    if (par_File != NULL) {
        MDF4_STIMULI_FILE *Mdf = (MDF4_STIMULI_FILE*)par_File->File;
        FreeDataStruct(Mdf);
#ifndef DebugOut
        if (DebugOut != NULL) fclose(DebugOut);
#endif

    }
}

int *Mdf4GetStimuliVariableIds(STIMULI_FILE *par_File)
{
    MDF4_STIMULI_FILE *Mdf = (MDF4_STIMULI_FILE*)par_File->File;
    return (Mdf->vids);
}

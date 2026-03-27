/*
 * Copyright 2024 ZF Friedrichshafen AG
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

#include "Platform.h"
#include <stdio.h>

#include "Config.h"
#include "MyMemory.h"
#include "Files.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "Scheduler.h"
#include "BlackboardConversion.h"
#include "TraceRecorder.h"
#include "ThrowError.h"
#include "TextReplace.h"
#include "Mdf4Structs.h"
#include "TraceWriteMdf4File.h"

#define UNUSED(x) (void)(x)

static int64_t GetFilePosMultipleOf8 (FILE *par_File)
{
    int64_t Ret =  _ftelli64 (par_File);
    int Mod = Ret & 0x7;
    if (Mod != 0) {
        int ToAdd = 8 - Mod;
        int64_t Help = 0;
        if (fwrite(&Help, 1, ToAdd, par_File) != ToAdd) {
            Ret = -1;
        } else {
            Ret += ToAdd;
        }
    }
    return Ret;
}

static int WriteLinkToPos (FILE *fh, uint64_t Link, int64_t FilePos)
{
    int64_t Help;
    Help = _ftelli64 (fh);
    _fseeki64 (fh, FilePos, SEEK_SET);
    fwrite (&Link, sizeof (uint64_t), 1, fh);
    _fseeki64 (fh, Help, SEEK_SET);
    return 0;
}

static char *GetNumber (char *Zahl, double *pValue)
{
    char *s = Zahl;
    char *ret;

    while (isspace(*s)) s++;
    if ((s[0] == '0') && (s[1] == 'x')) {  // ist eine Hex-Zahl
        unsigned long long h = strtoull (s, &ret, 16);
        *pValue = (double)h;
    } else {
        *pValue = strtod (s, &ret);
    }
    if (ret == s) return NULL;
    else return ret;
}

static int ConvertFormulaToFacOff (char *Formula, double *pFac, double *pOff)
{
    char *s = Formula;

    *pFac = 1.0;
    *pOff = 0.0;
    while (isspace(*s)) s++;
    if ((*s == '#') || (*s == '$')) {  // erst kommt die Variable
        s++;
        while (isspace(*s)) s++;
        switch (*s) {  // OP
        case '*':
            s++;
            if ((s = GetNumber (s, pFac)) == NULL) return 0;
            while (isspace(*s)) s++;
            switch (*s) {  // OP
            case '+':
                s++;
                if ((s = GetNumber (s, pOff)) == NULL) return 0;
                break;
            case '-':
                s++;
                if ((s = GetNumber (s, pOff)) == NULL) return 0;
                *pOff *= -1.0;
                break;
            case 0:
                break;
            default:
                return 0;  // kann nicht konvertiert werden
            }
            break;
        case '+':
            s++;
            if ((s = GetNumber (s, pOff)) == NULL) return 0;
            break;
        case '-':
            s++;
            if ((s = GetNumber (s, pOff)) == NULL) return 0;
            *pOff *= -1.0;
            break;
        case 0:  // nur ein # ist auch OK
            break;
        default:
            return 0;  // kann nicht konvertiert werden
        }
    } else {
        if ((s = GetNumber (s, pFac)) == NULL) return 0;
        while (isspace(*s)) s++;
        switch (*s) {  // OP
        case '*':
            s++;
            while (isspace(*s)) s++;
            if ((*s != '#') && (*s != '$'))  return 0;
            s++;
            while (isspace(*s)) s++;
            switch (*s) {  // OP
            case '+':
                s++;
                if ((s = GetNumber (s, pOff)) == NULL) return 0;
                break;
            case '-':
                s++;
                if ((s = GetNumber (s, pOff)) == NULL) return 0;
                *pOff = -*pOff;
                break;
            case 0:
                break;
            default:
                return 0;  // kann nicht konvertiert werden
            }
            break;
        case '+':
            s++;
            *pOff = *pFac;
            while (isspace(*s)) s++;
            if ((*s == '#') || (*s == '$'))  {
                s++;
                while (isspace(*s)) s++;
                switch (*s) {
                case '*':
                    s++;
                    if ((s = GetNumber (s, pFac)) == NULL) return 0;
                    break;
                case 0:
                    break;
                }
            } else {
                if ((s = GetNumber (s, pOff)) == NULL) return 0;
            }
            break;
        case '-':
            s++;
            *pOff = *pFac;
            while (isspace(*s)) s++;
            if ((*s == '#') || (*s == '$'))  {
                s++;
                while (isspace(*s)) s++;
                switch (*s) {
                case '*':
                    s++;
                    if ((s = GetNumber (s, pFac)) == NULL) return 0;
                    *pFac = -*pFac;
                    break;
                case 0:
                    *pFac = -1.0;
                    break;
                }
            } else {
                if ((s = GetNumber (s, pOff)) == NULL) return 0;
            }
            break;
        case 0:  // nur ein # ist auch OK
            break;
        default:
            return 0;  // kann nicht konvertiert werden
        }
    }
    while (isspace(*s)) s++;
    return (*s == 0);
}


static int WriteMdf4BlockLinkList(FILE *fh, uint64_t *LinkList, int SizeOfLinkList)
{
    int WriteResult;
    WriteResult = (int)fwrite (LinkList, sizeof (uint64_t), SizeOfLinkList, fh);
    if (WriteResult != SizeOfLinkList) return -1;
    return 0;
}

static int WriteMdf4BlockHeader(FILE *fh, const char *BlockName, uint64_t *LinkList, int SizeOfLinkList, uint64_t AdditionalSize)
{
    MDF4_BLOCK_HEADER BlockHeader;
    int WriteResult;

    BlockHeader.Reserved = 0;
    BlockHeader.LinkCount = SizeOfLinkList;
    BlockHeader.BlockLength = sizeof (BlockHeader) + SizeOfLinkList * sizeof(uint64_t) + AdditionalSize;
    strncpy(BlockHeader.BlockTypeIdentifier, BlockName, 4);
    WriteResult = (int)fwrite (&BlockHeader, sizeof (BlockHeader), 1, fh);
    if (WriteResult != 1) return -1;

    return WriteMdf4BlockLinkList(fh, LinkList, SizeOfLinkList);
}

static uint64_t WriteString(FILE *fh, const char *par_String)
{
    uint64_t Ret =  GetFilePosMultipleOf8(fh);

    int SringLen = (int)strlen(par_String) + 1;
    if (WriteMdf4BlockHeader(fh, "##TX", NULL, 0, SringLen)) {
        Ret = 0;
        goto __ERROUT;
    }
    if ((int)fwrite (par_String, 1, SringLen, fh) != SringLen) {
        Ret = 0;
        goto __ERROUT;
    }
__ERROUT:
    return Ret;
}


static uint64_t WriteConversion(FILE *fh, int par_Vid, int par_RawPhysFlag, enum BB_CONV_TYPES par_ConversionType, char *Conversion)
{
    BB_VARIABLE_CONVERSION Conv;
    //double Fac, Off;
    MDF4_CCBLOCK MdfCcBlock ={0};
    double *DynParList = NULL;
    uint64_t *DynLinkList = NULL;
    double StackParList[6];   // BB_CONV_RAT_FUNC will fit inside
    uint64_t StackLinkList[5];
    double *ParList = StackParList;
    uint64_t *LinkList = StackLinkList;
    int SizeOfParList = 0;
    int SizeOfLinkList = 4;
    int Size;
    uint64_t Ret = 0;

    MdfCcBlock.Flags = 0;

    StackLinkList[0] = 0;
    StackLinkList[1] = 0;
    StackLinkList[2] = 0;
    StackLinkList[3] = 0;
    // If formula defind but variable already physical converted don't write a
    // formula into the MDA file
    if ((par_ConversionType == 1) && (par_RawPhysFlag == 1)) {
        par_ConversionType = 0;
    }
    switch (par_ConversionType) {
    default:
    case BB_CONV_NONE:  // no conversion
        MdfCcBlock.Type = 0; //  1:1 conversion formula (Int = Phys)
        SizeOfParList = 0;
        MdfCcBlock.NoOfValues = 0;
        break;
    case BB_CONV_FORMULA:
        get_bbvari_conversion (par_Vid, Conversion, BBVARI_CONVERSION_SIZE);
        if (ConvertFormulaToFacOff (Conversion, &Conv.Conv.FactorOffset.Factor, &Conv.Conv.FactorOffset.Offset)) {
            MdfCcBlock.Type = 1; // CCBLOCK  Linear Function with 2 Parameters
            SizeOfParList = 2;
            MdfCcBlock.NoOfValues = SizeOfParList;
            ParList[0] = Conv.Conv.FactorOffset.Offset;
            ParList[1] = Conv.Conv.FactorOffset.Factor;
        } else {
            char *s;
            MdfCcBlock.Type = 3; // Text formula
            MdfCcBlock.NoOfReferences = 1;
            // replace # or $ with X
            s = Conversion;
            while (*s != 0) {
                if ((*s == '#') || (*s == '$')) {
                    *s = 'X';
                }
                s++;
            }
            SizeOfLinkList = 5;
            SizeOfParList = 0;
            MdfCcBlock.NoOfValues = SizeOfParList;

            if ((LinkList[4] = WriteString(fh, Conversion)) == 0) {
                Ret = 0;
                goto __ERROUT;
            }
        }
        break;
    case BB_CONV_TEXTREP:  // ENUM
        get_bbvari_conversion (par_Vid, Conversion, BBVARI_CONVERSION_SIZE);
        Size = GetEnumListSize(Conversion);
        if (Size > 0) {
            int x, Pos = 0;
            SizeOfParList = Size * 2;
            MdfCcBlock.NoOfValues = SizeOfParList;
            MdfCcBlock.NoOfReferences = Size + 1;
            SizeOfLinkList = 4 + Size + 1;  // we have 4 links + one link for each text replace + one NULL link
            ParList = DynParList = (double*)my_malloc(sizeof(double) * SizeOfParList);
            LinkList = DynLinkList = (uint64_t*)my_malloc(sizeof(uint64_t) * SizeOfLinkList);
            for (x = 0; x < 4; x++) {
                LinkList[x] = StackLinkList[x];
            }
            for (x = 0; x < Size; x++) {
                char EnumText[512];
                int64_t From, To;
                Pos = GetNextEnumFromEnumList_Pos (Pos, Conversion, EnumText, sizeof(EnumText), &From, &To, NULL);
                if (Pos < 0) {
                    break;
                }
                LinkList[4 + x] = WriteString(fh, EnumText);
                ParList[x * 2] = (double)From;
                ParList[x * 2 + 1] = (double)To;
            }
            LinkList[4 + Size] = 0; // List must end with a NULL reference
            MdfCcBlock.Type = 8;  // ASAM-MCD2 Text Range Table (COMPU_VTAB_RANGE)
        } else {
            MdfCcBlock.Type = 0; //  1:1 conversion formula (Int = Phys)
        }
        break;
    case BB_CONV_FACTOFF:
    case BB_CONV_OFFFACT:
        get_bbvari_conversion (par_Vid, Conversion, BBVARI_CONVERSION_SIZE);
        if (par_ConversionType == BB_CONV_FACTOFF) {
            Conv_ParseFactorOffsetString(Conversion, &Conv);
        } else {
            Conv_ParseOffsetFactorString(Conversion, &Conv);
            Conv.Conv.FactorOffset.Offset *= Conv.Conv.FactorOffset.Factor;
        }
        MdfCcBlock.Type = 1; // CCBLOCK  Linear Function with 2 Parameters
        SizeOfParList = 2;
        MdfCcBlock.NoOfValues = SizeOfParList;
        ParList[0] = Conv.Conv.RatFunc.b;
        ParList[1] = Conv.Conv.RatFunc.a;
        break;
    case BB_CONV_TAB_INTP:   // Table with interpolation
    case BB_CONV_TAB_NOINTP:   // Table without interpolation
        get_bbvari_conversion (par_Vid, Conversion, BBVARI_CONVERSION_SIZE);
        Conv_ParseTableInterpolString(Conversion, &Conv);
        SizeOfParList = 2 * Conv.Conv.Table.Size;
        MdfCcBlock.NoOfValues = SizeOfParList;
        ParList = DynParList = (double*)my_malloc(sizeof(double) * SizeOfParList);
        for (int x = 0; x < Conv.Conv.Table.Size; x++) {
            ParList[x*2] = Conv.Conv.Table.Values[x].Raw;
            ParList[x*2+1] = Conv.Conv.Table.Values[x].Phys;
        }
        if (par_ConversionType == BB_CONV_TAB_INTP) MdfCcBlock.Type = 4; // tabular with interpolation
        else MdfCcBlock.Type = 5; // tabular without interpolation
        break;
    case BB_CONV_RAT_FUNC:   // Rational function
        get_bbvari_conversion (par_Vid, Conversion, BBVARI_CONVERSION_SIZE);
        Conv_ParseRationalFunctionString(Conversion, &Conv);
        MdfCcBlock.Type = 2; // CCBLOCK Rational Function with 6 Parameters
        SizeOfParList = 6;
        MdfCcBlock.NoOfValues = SizeOfParList;
        ParList[0] = Conv.Conv.RatFunc.a;
        ParList[1] = Conv.Conv.RatFunc.b;
        ParList[2] = Conv.Conv.RatFunc.c;
        ParList[3] = Conv.Conv.RatFunc.d;
        ParList[4] = Conv.Conv.RatFunc.e;
        ParList[5] = Conv.Conv.RatFunc.f;
        break;

    }

    if (par_Vid > 0) {
        MdfCcBlock.Flags |= 0x2;  // Ranges are valid
        MdfCcBlock.RangeMin = get_bbvari_min(par_Vid);
        MdfCcBlock.RangeMax = get_bbvari_max(par_Vid);
    }
    if (par_Vid > 0) {
        MdfCcBlock.Flags |= 0x1; // Precision are valid
        MdfCcBlock.Precision = get_bbvari_format_prec(par_Vid);
    }
    if  (par_Vid > 0) {
        char Unit[64];
        get_bbvari_unit(par_Vid, Unit, sizeof(Unit));
        LinkList[1] = WriteString(fh, Unit);
    }
    //MdfCcBlock.NoOfValues = SizeOfParList;
    Ret = GetFilePosMultipleOf8(fh);
    WriteMdf4BlockHeader(fh, "##CC", LinkList, SizeOfLinkList, sizeof(MdfCcBlock) + SizeOfParList * sizeof(double));

    // at last write the block itself
    if (fwrite (&MdfCcBlock, sizeof (MdfCcBlock), 1, fh) != 1) {
        Ret = 0;
        goto __ERROUT;
    }
    if (SizeOfParList > 0) {
        if (fwrite (ParList, sizeof (double), SizeOfParList, fh) != SizeOfParList) {
            Ret = 0;
            goto __ERROUT;
        }
    }
__ERROUT:
    if (DynParList != NULL) my_free(DynParList);
    if (DynLinkList != NULL) my_free(DynLinkList);
    return Ret;
}

uint64_t WriteOneChannel(FILE *fh, int par_SignalType, int par_Vid, int par_RawPhysFlag, char *ConversionBuffer,
                         uint64_t par_PosNextChannel, int *RecordSize)
{
    MDF4_CNBLOCK MdfCnBlock;
    uint64_t LinkList[8];
    uint64_t Ret = 0;
    char SignalName[512];
    enum BB_DATA_TYPES DataType;
    enum BB_CONV_TYPES ConversionType;

    if (par_SignalType == 1) { // Time line
        MdfCnBlock.Type = 2;
        MdfCnBlock.SyncType = 1;
        STRING_COPY_TO_ARRAY(SignalName, "t");
        ConversionType = 0;  // 1:1
        DataType = BB_DOUBLE;
    } else {
        MdfCnBlock.Type = 0;
        MdfCnBlock.SyncType = 0;  // Signal
        GetBlackboardVariableNameAndTypes(par_Vid, SignalName, sizeof(SignalName), &DataType, &ConversionType);
    }

    LinkList[0] = par_PosNextChannel;  // link to the next channel
    LinkList[1] = 0;  // Cx
    LinkList[2] = WriteString(fh, SignalName); // Name
    LinkList[3] = 0;  // Si
    LinkList[4] = WriteConversion(fh, par_Vid, par_RawPhysFlag, ConversionType, ConversionBuffer);  // Cc
    LinkList[5] = 0;
    LinkList[6] = 0;
    LinkList[7] = 0;

    MdfCnBlock.Reserved = 0;
    MdfCnBlock.Flags = 0;
    Ret = GetFilePosMultipleOf8(fh);  // file position where to write the link to the next channel

    if (par_RawPhysFlag) {
        MdfCnBlock.BitCount = 64;
        MdfCnBlock.DataType = 4;  // IEEE 754 floating-point format DOUBLE
    } else {
        switch (DataType) {
        case BB_BYTE:
            MdfCnBlock.BitCount = 8;
            MdfCnBlock.DataType = 2; // signed integer
            break;
        case BB_UBYTE:
            MdfCnBlock.BitCount = 8;
            MdfCnBlock.DataType = 0; // unsigned integer
            break;
        case BB_WORD:
            MdfCnBlock.BitCount = 16;
            MdfCnBlock.DataType = 2; // signed integer
            break;
        case BB_UWORD:
            MdfCnBlock.BitCount = 16;
            MdfCnBlock.DataType = 0;  // unsigned integer
            break;
        case BB_DWORD:
            MdfCnBlock.BitCount = 32;
            MdfCnBlock.DataType = 2; // signed integer
            break;
        case BB_UDWORD:
            MdfCnBlock.BitCount = 32;
            MdfCnBlock.DataType = 0; // unsigned integer
            break;
        case BB_QWORD:
            MdfCnBlock.BitCount = 64;
            MdfCnBlock.DataType = 2; // signed integer
            break;
        case BB_UQWORD:
            MdfCnBlock.BitCount = 64;
            MdfCnBlock.DataType = 0; // unsigned integer
            break;
        case BB_FLOAT:
            MdfCnBlock.BitCount = 32;
            MdfCnBlock.DataType = 4;  // IEEE 754 floating-point format FLOAT (4 bytes)
            break;
        case BB_DOUBLE:
            MdfCnBlock.BitCount = 64;
            MdfCnBlock.DataType = 4;  // IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
            break;
        default:
            Ret = 0;
            goto __ERROUT;
        }
    }

    MdfCnBlock.ByteOffset = *RecordSize;
    MdfCnBlock.BitOffset = 0;

    MdfCnBlock.LimitExtMax = 0.0;
    MdfCnBlock.LimitExtMin = 0.0;
    MdfCnBlock.LimitMax = 0.0;
    MdfCnBlock.LimitMin = 0.0;
    MdfCnBlock.RangeMax = 0.0;
    MdfCnBlock.RangeMin = 0.0;

    // add signal bit size to record byte size
    *RecordSize += MdfCnBlock.BitCount / 8;

    WriteMdf4BlockHeader(fh, "##CN", LinkList, 8, sizeof(MdfCnBlock) );
    if (fwrite (&MdfCnBlock, sizeof (MdfCnBlock), 1, fh) != 1) {
        Ret = 0;
        goto __ERROUT;
    }
__ERROUT:
    return Ret;
}

#define MDF_STRUCT_OFFSET(Struct, Member) \
    ((long)((char*)&Struct.Member - (char*)&Struct))

static uint64_t FPos_LinkToTheFirstDL;
static uint64_t FPos_NoOfSamples;
static uint64_t FPos_NextDataGroupBlock;

static char *DataBuffer;
static int DataBufferSize;  // >= 128KByte
static int RecordSize;      // must be fit n times into DataBufferSize
static int DataBufferPos;   // sizeof(MDF4_BLOCK_HEADER)...DataBufferSize-1

static uint64_t *DataBufferLinkList;
static int DataBufferLinkListSize;
static int DataBufferLinkListPos;

static int AllocDataBuffer(void)
{
    int Modulo;
    DataBufferSize = 4 * 1024 * 1024;  // 4M byte buffer
    if (RecordSize > 0) {
        Modulo = DataBufferSize % RecordSize;
        if (Modulo > 0) {
            DataBufferSize += RecordSize - Modulo;
        }
    }
    DataBufferSize += sizeof(MDF4_BLOCK_HEADER);  // need space for the header
    DataBuffer = my_malloc(DataBufferSize);
    if (DataBuffer == NULL) return -1;
    ((MDF4_BLOCK_HEADER*)DataBuffer)->Reserved = 0;
    ((MDF4_BLOCK_HEADER*)DataBuffer)->LinkCount = 0;
    ((MDF4_BLOCK_HEADER*)DataBuffer)->BlockLength = 0; // this must be updated if block will be flushed to disk
    strncpy(((MDF4_BLOCK_HEADER*)DataBuffer)->BlockTypeIdentifier, "##DT", 4);
    DataBufferPos = sizeof(MDF4_BLOCK_HEADER);

    DataBufferLinkListSize = 16;
    DataBufferLinkListPos = 1;  // the first is the next ##DL inside the linked list.
    DataBufferLinkList = (uint64_t*)my_malloc(DataBufferLinkListSize * sizeof(uint64_t));
    if (DataBufferLinkList == NULL) return -1;
    DataBufferLinkList[0] = 0; // we have never a next ##DL
    return 0;
}

static int FlushDataBuffer(FILE *fh)
{
    if ((DataBufferLinkListPos + 1) >= DataBufferLinkListSize) {
        DataBufferLinkListSize += DataBufferLinkListSize / 4 + 32;
        DataBufferLinkList = (uint64_t*)my_realloc(DataBufferLinkList, DataBufferLinkListSize * sizeof(uint64_t));
        if (DataBufferLinkList == NULL) {
            ThrowError(1, "out of memory");
            return -1;
        }
    }
    uint64_t FilePos = _ftelli64(fh);
    uint64_t Offset = (8 - (FilePos & 0x7)) & 0x7;  // must be multiple of 8
    if (Offset > 0) {
        char Filler[8] = {0};
        if (fwrite (Filler, Offset, 1, fh) != 1) {
            return -1;
        }
        FilePos += Offset;
    }
    DataBufferLinkList[DataBufferLinkListPos] = FilePos;
    DataBufferLinkListPos++;
    ((MDF4_BLOCK_HEADER*)DataBuffer)->BlockLength = DataBufferPos; // this must be updated if block will be flushed to disk
    if (fwrite (DataBuffer, DataBufferPos, 1, fh) != 1) {
        return -1;
    }
    DataBufferPos = sizeof(MDF4_BLOCK_HEADER);
    return 0;
}

static int CheckFitIntoBuffer(FILE *fh)
{
    if ((DataBufferSize - DataBufferPos) < RecordSize) {
       return FlushDataBuffer(fh);
    }
    return 0;
}

static int SaveBlockLinkList(FILE *fh)
{
    MDF4_DLBLOCK MdfDlBlock;

    WriteLinkToPos(fh, GetFilePosMultipleOf8(fh), FPos_LinkToTheFirstDL);

    WriteMdf4BlockHeader(fh, "##DL", DataBufferLinkList, DataBufferLinkListPos, sizeof(MdfDlBlock));
    MdfDlBlock.Flags = 1; // EQUAL_LENGTH
    MdfDlBlock.Reserved[0] = 0;
    MdfDlBlock.Reserved[1] = 0;
    MdfDlBlock.Reserved[2] = 0;
    MdfDlBlock.NoOfBlocks = DataBufferLinkListPos - 1; // nummber of blocks are one smaller as the linked list
    MdfDlBlock.EqualLength = DataBufferSize - sizeof(MDF4_BLOCK_HEADER); // EQUAL_LENGTH flag

    if (fwrite (&MdfDlBlock, sizeof (MdfDlBlock), 1, fh) != 1) {
        return -1;
    }
    return 0;
}

static void FreeDataBuffer(void)
{
    DataBufferSize = 0;
    my_free(DataBuffer);
    DataBuffer = NULL;
    DataBufferLinkListSize = 0;
    DataBufferLinkListPos = 0;
    my_free(DataBufferLinkList);
    DataBuffer = NULL;
}

#define CLOSE_FILE_FREE_BUFFERS  \
{ \
    fclose (*pfile); \
    if (Conversion != NULL) my_free (Conversion);\
    if (Conversion2 != NULL) my_free (Conversion2);\
}

#define xstr(s) str(s)
#define str(s) #s

int64_t WriteFileHistory(FILE *par_File)
{
    int64_t Ret;
    MDF4_FHBLOCK MdfFhBlock;
    uint64_t LinkList[2];
    char *History = "<FHcomment xmlns='http://www.asam.net/mdf/v4'><TX>File was created.</TX><tool_id>OpenXilEnv</tool_id><tool_vendor>none</tool_vendor><tool_version>"
                    xstr(XILENV_VERSION) "." xstr(XILENV_MINOR_VERSION) "." xstr(XILENV_PATCH_VERSION)
                    "</tool_version><user_name>unknown</user_name><common_properties><e name=\"binaries_version\"> buildin mdf4 writer </e></common_properties></FHcomment>";
    int Len =  strlen(History) + 1;

    LinkList[0] = 0;  // no next one
    LinkList[1] = GetFilePosMultipleOf8(par_File);

    if(WriteMdf4BlockHeader(par_File, "##MD", NULL, 0, Len)) {
        goto __ERROUT;
    }
    if (fwrite(History, 1, Len, par_File) != Len) {
        goto __ERROUT;
    }

    Ret = GetFilePosMultipleOf8(par_File);
    if(WriteMdf4BlockHeader(par_File, "##FH", LinkList, 2, sizeof(MdfFhBlock))) {
        goto __ERROUT;
    }
    MdfFhBlock.TimeStamp.Time = GetSimulatedStartTimeInNanoSecond() + GetSimulatedTimeInNanoSecond();
    MdfFhBlock.TimeStamp.TimeZoneOffset = 0;
    MdfFhBlock.TimeStamp.DstOffset = 0;
    MdfFhBlock.TimeStamp.flags = 0;
    MdfFhBlock.Reserved[0] = MdfFhBlock.Reserved[1] = MdfFhBlock.Reserved[2] = 0;
    if (fwrite (&MdfFhBlock, sizeof (MdfFhBlock), 1, par_File) != 1) {
        goto __ERROUT;
    }
    return Ret;
__ERROUT:
    return 0;
}

int OpenWriteMdf4Head (START_MESSAGE_DATA hdrec_data,
                       int32_t *vids, char *dec_phys_flags, FILE **pfile)
{
    int Channel, x;
    MDF4_IDBLOCK MdfIdBlock;
    MDF4_HDBLOCK MdfHdBlock;
    MDF4_DGBLOCK MdfDgBlock;
    MDF4_CGBLOCK MdfCgBlock;
    char *ConversionBuffer;

    uint64_t FPos_LinkToFileHistory;
    uint64_t FPos_LinkToFirstDataGroup;
    uint64_t FPos_FirstChannelBlock;
    uint64_t PosNextChannel;
    uint64_t LinkList[16];
    int Ret = 0;

    conv_rec_time_steps = hdrec_data.RecTimeSteps;

    if ((*pfile = OpenFile4WriteWithPrefix (hdrec_data.Filename, "wb")) == NULL) {
        ThrowError (1, "cannot open \"%s\"", hdrec_data.Filename);
        return CANNOT_OPEN_RECFILE;
    }

    ConversionBuffer = my_malloc (BBVARI_CONVERSION_SIZE);
    if (ConversionBuffer == NULL) {
        goto __ERROUT;
    }

    MEMSET(&MdfIdBlock, 0, sizeof(MdfIdBlock));
    MEMCPY (MdfIdBlock.FileIdentifier, "MDF     ", 8);
    StringCopyMaxCharTruncate (MdfIdBlock.FormatTdentifier, "4.10 ", sizeof(MdfIdBlock.FormatTdentifier));
    StringCopyMaxCharTruncate (MdfIdBlock.ProgramIdentifier, "XilEnv", sizeof(MdfIdBlock.ProgramIdentifier));
    MdfIdBlock.DefaultByteOrder = 0;    // 0 = Little Endian (Intel)
    MdfIdBlock.DefaultFloatFormat = 0;  // 0 = Floating-point format compliant with IEEE 754 standard
    MdfIdBlock.VersionNumber = 410;     // 410 for this version
    MdfIdBlock.Reserved1 = 0;
    MdfIdBlock.Reserved2[0] = 0;
    MdfIdBlock.Reserved2[1] = 0;
    for (x = 0; x < 30; x++) {
        MdfIdBlock.Reserved3[x] = 0;
    }

    if (fwrite (&MdfIdBlock, sizeof (MdfIdBlock), 1, *pfile) != 1) {
        Ret = -1;
        goto __ERROUT;
    }

    FPos_LinkToFirstDataGroup = _ftelli64 (*pfile) + sizeof(MDF4_BLOCK_HEADER); // Link to first data group
    FPos_LinkToFileHistory = FPos_LinkToFirstDataGroup + 1 * sizeof(uint64_t);
    LinkList[0] = 0; // Link to first data group (will be written later) // _ftelli64 (*pfile) + sizeof(MDF4_BLOCK_HEADER) + 6 * sizeof(uint64_t) + sizeof(MdfHdBlock);

    // Link to first data group
    LinkList[1] = 0;  // Link to file history block (will be written later)
    LinkList[2] = 0;
    LinkList[3] = 0;  // Attachment
    LinkList[4] = 0;  // Event
    LinkList[5] = 0;  // XML string

    WriteMdf4BlockHeader(*pfile, "##HD", LinkList, 6, sizeof(MdfHdBlock));
    MdfHdBlock.TimeStamp.Time = GetSimulatedStartTimeInNanoSecond() + GetSimulatedTimeInNanoSecond();
    MdfHdBlock.TimeStamp.TimeZoneOffset = 0;
    MdfHdBlock.TimeStamp.DstOffset = 0;
    MdfHdBlock.TimeStamp.flags = 0;
    MdfHdBlock.Flags = 0;
    MdfHdBlock.TimeClass = 0;
    MdfHdBlock.Reserved = 0;
    MdfHdBlock.StartAngle = 0;
    MdfHdBlock.StartDistance = 0;
    if (fwrite (&MdfHdBlock, sizeof (MdfHdBlock), 1, *pfile) != 1) {
        Ret = -1;
        goto __ERROUT;
    }

    WriteLinkToPos(*pfile, WriteFileHistory(*pfile), FPos_LinkToFileHistory);
    WriteLinkToPos(*pfile, GetFilePosMultipleOf8(*pfile), FPos_LinkToFirstDataGroup);

    // we use only one data group block
    MdfDgBlock.Reserved[0] = MdfDgBlock.Reserved[1] = MdfDgBlock.Reserved[2] = MdfDgBlock.Reserved[3] = MdfDgBlock.Reserved[4] = MdfDgBlock.Reserved[5] = MdfDgBlock.Reserved[6] = 0;
    MdfDgBlock.RecIdSize = 0;

    FPos_NextDataGroupBlock = _ftelli64 (*pfile) + sizeof(MDF4_BLOCK_HEADER) + 0 * sizeof(uint64_t);
    FPos_FirstChannelBlock = _ftelli64 (*pfile) + sizeof(MDF4_BLOCK_HEADER) + 1 * sizeof(uint64_t);
    FPos_LinkToTheFirstDL = _ftelli64 (*pfile) + sizeof(MDF4_BLOCK_HEADER) + 2 * sizeof(uint64_t);
    LinkList[0] = 0;  // next ##DG (no next only one)
    LinkList[1] = 0;  // Link to the first channel list (Linkt to one "##CG") this will be written later. We have only one ##
    LinkList[2] = 0;  // Link to the first data block list (Linkt to one "##DL") this will be written at the end of the recording
    LinkList[3] = 0;  // Attachment

    WriteMdf4BlockHeader(*pfile, "##DG", LinkList, 4, sizeof(MdfDgBlock));
    if (fwrite (&MdfDgBlock, sizeof (MdfDgBlock), 1, *pfile) != 1) {
        Ret = -1;
        goto __ERROUT;
    }

    PosNextChannel = 0;
    RecordSize = 0;
    // now the measurement channels
    for (Channel = 0; Channel < vids[Channel] > 0; Channel++) {
        PosNextChannel = WriteOneChannel(*pfile, 0, vids[Channel],
                                         (dec_phys_flags != NULL) && dec_phys_flags[Channel],
                                         ConversionBuffer,
                                         PosNextChannel,
                                         &RecordSize);
        if (PosNextChannel == 0) {
            goto __ERROUT;
        }
    }

    // now write the time channel
    PosNextChannel = WriteOneChannel(*pfile, 1, -1, 0, ConversionBuffer, PosNextChannel, &RecordSize);
    if (PosNextChannel == 0) {
        goto __ERROUT;
    }

    // we use only one channel group block
    MdfCgBlock.Reserved = 0;
    MdfCgBlock.RecordId = 0;
    MdfCgBlock.Flags = 0;
    MdfCgBlock.NoOfSamples = 0;
    MdfCgBlock.PathSeparator = '\\';
    MdfCgBlock.NoOfInvalidBytes = 0;
    MdfCgBlock.NoOfDataBytes = RecordSize;

    LinkList[0] = 0;  // Link to the next channel group (always 0 we have only one CG)
    LinkList[1] = PosNextChannel;  // Link to first channel group (will be written later)
    LinkList[2] = WriteString(*pfile, "Signals channel group");
    LinkList[3] = 0;  // Attachment
    LinkList[4] = 0;
    LinkList[5] = 0;

    WriteLinkToPos(*pfile, GetFilePosMultipleOf8(*pfile), FPos_FirstChannelBlock);

    WriteMdf4BlockHeader(*pfile, "##CG", LinkList, 6, sizeof(MdfCgBlock));

    FPos_NoOfSamples = _ftelli64 (*pfile) + (char*)&MdfCgBlock.NoOfSamples - (char*)&MdfCgBlock;

    if (fwrite (&MdfCgBlock, sizeof (MdfCgBlock), 1, *pfile) != 1) {
        Ret = -1;
        goto __ERROUT;
    }

    fflush(*pfile);
    AllocDataBuffer();

__ERROUT:
    if (ConversionBuffer != NULL) my_free (ConversionBuffer);
    return Ret;
}


int WriteRingbuffMdf4 (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count)
{
    int i;

    if (CheckFitIntoBuffer(file)) {
        return EOF;
    }
    if (rpvari_count >= 2) {  // first is the counter second is the timestamp
        for (i = 2; (i < rpvari_count); i++) {
            switch (stamp->EntryList[i].type) {
            case BB_BYTE:
            case BB_UBYTE:
                DataBuffer[DataBufferPos] = stamp->EntryList[i].value.ub;
                DataBufferPos++;
                break;
            case BB_WORD:
            case BB_UWORD:
                *(uint16_t*)(DataBuffer + DataBufferPos) = stamp->EntryList[i].value.uw;
                DataBufferPos += 2;
                break;
            case BB_DWORD:
            case BB_UDWORD:
            case BB_FLOAT:
                *(uint32_t*)(DataBuffer + DataBufferPos) = stamp->EntryList[i].value.udw;
                DataBufferPos += 4;
                break;
            case BB_QWORD:
            case BB_UQWORD:
            case BB_DOUBLE:
                *(uint64_t*)(DataBuffer + DataBufferPos) = stamp->EntryList[i].value.uqw;
                DataBufferPos += 8;
                break;
            }
        }
        // the time stamp is the last entry inside the record
        *(uint64_t*)(DataBuffer + DataBufferPos) = stamp->EntryList[1].value.uqw;
        DataBufferPos += 8;
    }
    return 0;
}


typedef struct {
    uint64_t Timestamp;
    char *Comment;
} RECORDER_COMMENT_ELEM;

static RECORDER_COMMENT_ELEM *Comments;
static int SizeOfComments;
static int NoOfComments;
static int LongestComment;
static int SizeOfSDBlock;

static int WriteAllCommentsToMdfFile(FILE *fh, uint64_t RecorderStartTime)
{
    int x;
    MDF4_DGBLOCK MdfDgBlock;
    MDF4_CGBLOCK MdfCgBlock;
    MDF4_CNBLOCK MdfCnBlock;
    MDF4_DLBLOCK MdfDlBlock;
    uint64_t LinkList[8];
    uint64_t PosNextChannel, PosChannelGroup, PosDL;
    uint64_t FilePos;
    uint64_t *SdBlocksFilePos;
    int RecordSize;
    int Ret = -1;

    SdBlocksFilePos = (uint64_t*)my_malloc((NoOfComments + 1) * sizeof(uint64_t));
    if(SdBlocksFilePos == NULL) {
        goto __ERROUT;
    }
    SdBlocksFilePos[0] = 0;
    // Add for each text entry one data "##SD" block
    for(x = 0; x < NoOfComments; x++) {
        uint32_t Len = strlen(Comments[x].Comment) + 1;
        SdBlocksFilePos[x + 1] =  _ftelli64(fh);
        if(WriteMdf4BlockHeader(fh, "##SD", LinkList, 0, 4 + Len)) {
            goto __ERROUT;
        }
        if (fwrite(&Len, sizeof (Len), 1, fh) != 1) {
            goto __ERROUT;
        }
        if (fwrite(Comments[x].Comment, 1, Len, fh) != Len) {
            goto __ERROUT;
        }
    }
    // Add one "##DL" block with links to all "##SD" blocks
    PosDL = GetFilePosMultipleOf8(fh);
    WriteMdf4BlockHeader(fh, "##DL", SdBlocksFilePos, NoOfComments + 1, sizeof(MdfDlBlock));
    MdfDlBlock.Flags = 0;
    MdfDlBlock.Reserved[0] = 0;
    MdfDlBlock.Reserved[1] = 0;
    MdfDlBlock.Reserved[2] = 0;
    MdfDlBlock.NoOfBlocks = NoOfComments; // nummber of blocks are one smaller as the linked list
    MdfDlBlock.EqualLength = 0;

    if (fwrite (&MdfDlBlock, sizeof (MdfDlBlock), 1, fh) != 1) {
        return -1;
    }


    // first write the time channel
    RecordSize = 0;
    PosNextChannel = WriteOneChannel(fh, 1, -1, 0, NULL, 0, &RecordSize);
    if (PosNextChannel == 0) {
        goto __ERROUT;
    }

    // than write one comment text channel
    LinkList[0] = PosNextChannel;  // link to the next channel
    LinkList[1] = 0;  // Cx
    LinkList[2] = WriteString(fh, "Comment"); // Name
    LinkList[3] = 0;  // Si
    LinkList[4] = 0;  // Cc
    LinkList[5] = PosDL;  // "##DL"
    LinkList[6] = 0;
    LinkList[7] = 0;

    MEMSET(&MdfCnBlock, 0,  sizeof(MdfCnBlock));
    MdfCnBlock.Type = 1;      // VLSD channel
    MdfCnBlock.SyncType = 0;  // Signal

    MdfCnBlock.Reserved = 0;
    MdfCnBlock.Flags = 4;

    MdfCnBlock.BitCount = 8 * sizeof(uint64_t); //LongestComment;
    MdfCnBlock.DataType = 6; // 6 = ASCII string, 7 = String UTF-8

    MdfCnBlock.ByteOffset = 0;
    MdfCnBlock.BitOffset = 0;

    MdfCnBlock.LimitMax = 255;
    MdfCnBlock.LimitMin = 0;
    MdfCnBlock.Precision = 255;
    PosNextChannel = GetFilePosMultipleOf8 (fh);
    WriteMdf4BlockHeader(fh, "##CN", LinkList, 8, sizeof(MdfCnBlock) );
    if (fwrite (&MdfCnBlock, sizeof (MdfCnBlock), 1, fh) != 1) {
        goto __ERROUT;
    }
    RecordSize += sizeof(uint64_t);

    // we use only one channel group block
    MdfCgBlock.Reserved = 0;
    MdfCgBlock.RecordId = 0;
    MdfCgBlock.Flags = 0;
    MdfCgBlock.NoOfSamples = NoOfComments;
    MdfCgBlock.PathSeparator = '\\';
    MdfCgBlock.NoOfInvalidBytes = 0;
    MdfCgBlock.NoOfDataBytes = RecordSize;  // VLSD + timestamp //RecordSize;

    LinkList[0] = 0;  // Link to the next  channel group (always 0 we have only one CG)
    LinkList[1] = PosNextChannel;  // Link to first channel group (will be written later)
    LinkList[2] = WriteString(fh, "Comments channel group");
    LinkList[3] = 0;  // Attachment
    LinkList[4] = 0;
    LinkList[5] = 0;
    LinkList[6] = 0;
    LinkList[7] = 0;

    PosChannelGroup = GetFilePosMultipleOf8(fh);
    WriteMdf4BlockHeader(fh, "##CG", LinkList, 8, sizeof(MdfCgBlock));
    if (fwrite (&MdfCgBlock, sizeof (MdfCgBlock), 1, fh) != 1) {
        goto __ERROUT;
    }

    FilePos = _ftelli64 (fh);
    // we have a next data group
    WriteLinkToPos (fh, FilePos, FPos_NextDataGroupBlock);

    LinkList[0] = 0;  // next ##DG (no next only one)
    LinkList[1] = PosChannelGroup;  // Link to the first channel list (Linkt to one "##CG")
    LinkList[2] = FilePos + sizeof(MDF4_BLOCK_HEADER) + 4 * sizeof(uint64_t) + sizeof(MdfDgBlock);  // Link to the first data block (Linkt to one "##DT")
    LinkList[3] = 0;  // Attachment

    MdfDgBlock.Reserved[0] = MdfDgBlock.Reserved[1] = MdfDgBlock.Reserved[2] = MdfDgBlock.Reserved[3] = MdfDgBlock.Reserved[4] = MdfDgBlock.Reserved[5] = MdfDgBlock.Reserved[6] = 0;
    MdfDgBlock.RecIdSize = 0;

    WriteMdf4BlockHeader(fh, "##DG", LinkList, 4, sizeof(MdfDgBlock));
    if (fwrite (&MdfDgBlock, sizeof (MdfDgBlock), 1, fh) != 1) {
        goto __ERROUT;
    }

    WriteMdf4BlockHeader(fh, "##DT", LinkList, 0, RecordSize * NoOfComments);

    // Add the timeline and VLSD
    uint64_t Vlsd = 0;
    for (x = 0; x < NoOfComments; x++) {
        double Time = (double)(Comments[x].Timestamp - RecorderStartTime) / TIMERCLKFRQ;
        if (fwrite (&Time, sizeof (Time), 1, fh) != 1) {
            goto __ERROUT;
        }
        if ((int)fwrite (&Vlsd, sizeof(Vlsd), 1, fh) != 1) {
            goto __ERROUT;
        }
        //Vlsd += 4 + strlen(Comments[x].Comment) + 1;
    }
    Ret = 0;
__ERROUT:
    if(SdBlocksFilePos != NULL) {
        my_free(SdBlocksFilePos);
    }
    for (x = 0; x < NoOfComments; x++) {
        if (Comments[x].Comment != NULL) my_free (Comments[x].Comment);
    }
    NoOfComments = 0;
    SizeOfComments = 0;
    SizeOfSDBlock = 0;
    my_free(Comments);
    Comments = NULL;
    return Ret;
}

int TailMdf4File (FILE *fh, uint32_t Samples, uint64_t RecorderStartTime)
{
    FlushDataBuffer(fh);
    SaveBlockLinkList(fh);
    FreeDataBuffer();

    WriteLinkToPos(fh, Samples, FPos_NoOfSamples);

    // are there any comments to store?
    if (NoOfComments > 0) {
        if (WriteAllCommentsToMdfFile(fh, RecorderStartTime)) {
            ThrowError(1, "cannot store comments to MDF file");
        }
    }
    close_file (fh);
    return 0;
}

int WriteCommentToMdf4 (FILE *File, uint64_t Timestamp, const char *Comment)
{
    UNUSED(File);
    int CommentLen = (int)strlen(Comment) + 1;
    // we only collect all comments, this will be written if the MDF file will be closed
    if (NoOfComments >= SizeOfComments) {
        SizeOfComments += 64  + SizeOfComments / 4;
        Comments = (RECORDER_COMMENT_ELEM*)my_realloc(Comments, SizeOfComments * sizeof(RECORDER_COMMENT_ELEM));
        if (Comments == NULL) {
            NoOfComments = 0;
            SizeOfComments = 0;
            ThrowError(1, "out of memory");
            return -1;
        }
    }
    Comments[NoOfComments].Timestamp = Timestamp;
    SizeOfSDBlock += 4 + CommentLen;
    if (CommentLen > LongestComment) LongestComment = CommentLen;
    Comments[NoOfComments].Comment = my_malloc(CommentLen);
    if (Comments[NoOfComments].Comment  == NULL) {
        NoOfComments = 0;
        SizeOfComments = 0;
        ThrowError(1, "out of memory");
        return -1;
    }
    MEMCPY(Comments[NoOfComments].Comment, Comment, CommentLen);
    NoOfComments++;
    return 0;
}


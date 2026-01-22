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


#include "Platform.h"
#include <stdio.h>

#include "Config.h"
#include "MyMemory.h"
#include "Files.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ConfigurablePrefix.h"
#include "BlackboardConversion.h"
#include "TraceRecorder.h"
#include "ThrowError.h"
#include "TextReplace.h"
#include "MdfStructs.h"
#include "TraceWriteMdfFile.h"


static int WriteLinkToHere (FILE *fh, int32_t FilePos)
{
    int Help;
    Help = ftell (fh);
    fseek (fh, FilePos, SEEK_SET);
    fwrite (&Help, sizeof (uint32_t), 1, fh);
    fseek (fh, Help, SEEK_SET);
    return 0;
}

static int WriteUINT16ToPos (FILE *fh, uint16_t Value, int32_t FilePos)
{
    int Help;
    Help = ftell (fh);
    fseek (fh, FilePos, SEEK_SET);
    fwrite (&Value, sizeof (uint16_t), 1, fh);
    fseek (fh, Help, SEEK_SET);
    return 0;
}

static int WriteUINT32ToPos (FILE *fh, uint32_t Value, int32_t FilePos)
{
    int Help;
    Help = ftell (fh);
    fseek (fh, FilePos, SEEK_SET);
    fwrite (&Value, sizeof (uint32_t), 1, fh);
    fseek (fh, Help, SEEK_SET);
    return 0;
}

static char *GetZahl (char *Zahl, double *pValue)
{
    char *s = Zahl;
    char *ret;

    while (isspace(*s)) s++;
    if ((s[0] == '0') && (s[1] == 'x')) {  // is it a hex value
        uint64_t h = strtoull (s, &ret, 16);
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
    if ((*s == '#') || (*s == '$')) {  // first is the variable
        s++;
        while (isspace(*s)) s++;
        switch (*s) {  // OP
        case '*':
            s++;
            if ((s = GetZahl (s, pFac)) == NULL) return 0;
            while (isspace(*s)) s++;
            switch (*s) {  // OP
            case '+':
                s++;
                if ((s = GetZahl (s, pOff)) == NULL) return 0;
                break;
            case '-':
                s++;
                if ((s = GetZahl (s, pOff)) == NULL) return 0;
                *pOff *= -1.0;
                break;
            case 0:
                break;
            default:
                return 0;  // can not convert
            }
            break;
        case '+':
            s++;
            if ((s = GetZahl (s, pOff)) == NULL) return 0;
            break;
        case '-':
            s++;
            if ((s = GetZahl (s, pOff)) == NULL) return 0;
            *pOff *= -1.0;
            break;
        case 0:  // only one # is also OK
            break;
        default:
            return 0;  // can not convert
        }
    } else {
        if ((s = GetZahl (s, pFac)) == NULL) return 0;
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
                if ((s = GetZahl (s, pOff)) == NULL) return 0;
                break;
            case '-':
                s++;
                if ((s = GetZahl (s, pOff)) == NULL) return 0;
                *pOff = -*pOff;
                break;
            case 0:
                break;
            default:
                return 0;  // can not convert
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
                    if ((s = GetZahl (s, pFac)) == NULL) return 0;
                    break;
                case 0:
                    break;
                }
            } else {
                if ((s = GetZahl (s, pOff)) == NULL) return 0;
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
                    if ((s = GetZahl (s, pFac)) == NULL) return 0;
                    *pFac = -*pFac;
                    break;
                case 0:
                    *pFac = -1.0;
                    break;
                }
            } else {
                if ((s = GetZahl (s, pOff)) == NULL) return 0;
            }
            break;
        case 0:  // only one # is also OK
            break;
        default:
            return 0;  // can not convert
        }
    }
    while (isspace(*s)) s++;
    return (*s == 0);
}

#define MDF_STRUCT_OFFSET(Struct, Member) \
    ((int32_t)((char*)&Struct.Member - (char*)&Struct))

static int32_t FPos_NumberOfRecords;
static int32_t FPos_NextDataGroupBlock;
static int32_t FPos_NumberOfDataGroups;

#define CLOSE_FILE_FREE_BUFFERS  \
{ \
    fclose (*pfile); \
    if (Conversion != NULL) my_free (Conversion);\
    if (Conversion2 != NULL) my_free (Conversion2);\
}


int OpenWriteMdfHead (START_MESSAGE_DATA hdrec_data,
                      int32_t *vids, char *dec_phys_flags, FILE **pfile)
{
    int *vids_point;
    int ChannelCount, Channel, x;
    enum BB_CONV_TYPES ConvType;
    MDF_IDBLOCK MdfIdBlock;
    MDF_HDBLOCK MdfHdBlock;
    MDF_DGBLOCK MdfDgBlock;
    MDF_CGBLOCK MdfCgBlock;
    MDF_CNBLOCK MdfCnBlock;
    MDF_TXBLOCK MdfTxBlock;
    char Vorher;
    MDF_CCBLOCK MdfCcBlock;
    char Nachher;
    char SignalName[BBVARI_NAME_SIZE];
    char *Conversion;
    char *Conversion2;
    BB_VARIABLE_CONVERSION Conv;
    int BitOffsetInRecord;

    int32_t FPos_DataBlock;
    int32_t FPos_NextChannelBlock;
    int32_t FPos_ConversionFormula;
    int32_t FPos_LongSignalName;
    int32_t FPos_ConversionDataSize;
    int32_t FPos_SizeOfDataRecord;
    int32_t FPos_BlockSize;

    int32_t FilePos = 0;
    int32_t FilePosSave;

#ifdef _WIN32
    SYSTEMTIME Time;
#else
    time_t Time;
#endif
    char TimeString[16];
    char DateString[16];

    Vorher = Nachher = (char)0xAA;
    conv_rec_time_steps = hdrec_data.RecTimeSteps;

    if ((*pfile = OpenFile4WriteWithPrefix (hdrec_data.Filename, "wb")) == NULL) {
        ThrowError (1, "cannot open \"%s\"", hdrec_data.Filename);
        return CANNOT_OPEN_RECFILE;
    }

    Conversion = my_malloc (BBVARI_CONVERSION_SIZE);
    Conversion2 = my_malloc (BBVARI_CONVERSION_SIZE);
    if ((Conversion == NULL) || (Conversion == NULL)) {
        CLOSE_FILE_FREE_BUFFERS
        return CANNOT_OPEN_RECFILE;
    }

    MEMCPY (MdfIdBlock.FileIdentifier, "MDF     ", 8);
    StringCopyMaxCharTruncate (MdfIdBlock.FormatTdentifier, "3.10 ", sizeof(MdfIdBlock.FormatTdentifier));
    StringCopyMaxCharTruncate (MdfIdBlock.ProgramIdentifier, GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), sizeof(MdfIdBlock.ProgramIdentifier));
    MdfIdBlock.DefaultByteOrder = 0;    // 0 = Little Endian
    MdfIdBlock.DefaultFloatFormat = 0;  // 0 = Floating-point format IEEE 754
    MdfIdBlock.VersionNumber = 310;     // 310 for this version
    MdfIdBlock.Reserved1 = 0;
    MdfIdBlock.Reserved2[0] = 0;
    MdfIdBlock.Reserved2[1] = 0;
    for (x = 0; x < 30; x++) {
        MdfIdBlock.Reserved3[x] = 0;
    }

    if (fwrite (&MdfIdBlock, sizeof (MdfIdBlock), 1, *pfile) != 1) {
        CLOSE_FILE_FREE_BUFFERS
        return -1;
    }
    FilePos += sizeof (MdfIdBlock);

    MEMCPY (MdfHdBlock.BlockTypeIdentifier, "HD", 2);
    MdfHdBlock.BlockSize = sizeof (MdfHdBlock); 
    MdfHdBlock.FirstDataGroupBlock = ftell (*pfile) + (int32_t)sizeof (MdfHdBlock);
    MdfHdBlock.MeasurementFileComment = 0;
    MdfHdBlock.ProgramBlock = 0;
    MdfHdBlock.NumberOfDataGroups = 1;

    // can be incremented to 2 if there are any user comments to store
    FPos_NumberOfDataGroups = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfHdBlock, NumberOfDataGroups);

#ifdef _WIN32
    GetLocalTime (&Time);
    GetTimeFormat (LOCALE_USER_DEFAULT, // locale
                   0,                   // options
                   &Time,               // time
                   "hh':'mm':'ss",      // time format string
                   TimeString,         // formatted string buffer
                   sizeof (TimeString));  // size of string buffer
    GetDateFormat (LOCALE_USER_DEFAULT, // locale
                   0,                   // options
                   &Time,               // time
                   "dd':'MM':'yyyy",    // date format
                   DateString,         // formatted string buffer
                   sizeof (DateString)); // size of string buffer
#else
    struct tm LocalTime, *pLocalTime;
    Time = time(NULL);
    pLocalTime = localtime_r(&Time, &LocalTime);
    PrintFormatToString (TimeString, sizeof(TimeString), "%02u:%02u:%02u", (unsigned int)LocalTime.tm_hour, (unsigned int)LocalTime.tm_min, (unsigned int)LocalTime.tm_sec);
    PrintFormatToString (DateString, sizeof(DateString), "%02u.%02u.%04u", (unsigned int)LocalTime.tm_mday, (unsigned int)LocalTime.tm_mon, (unsigned int)LocalTime.tm_year + 1900);
#endif

    MEMCPY (MdfHdBlock.DateOfRecording, DateString, 10);   // Date at which the recording was started in "DD:MM:YYYY" format
    MEMCPY (MdfHdBlock.TimeOfRecording, TimeString, 8);   //Time at which the recording was started in "HH:MM:SS" format
    STRING_COPY_TO_ARRAY (MdfHdBlock.AuthorName, "unknown author");
    STRING_COPY_TO_ARRAY (MdfHdBlock.OrganizationName, "unknown organization");
    STRING_COPY_TO_ARRAY (MdfHdBlock.ProjectName, "unknown project");
    STRING_COPY_TO_ARRAY (MdfHdBlock.MeasurementObject, "unknown measurement object");

    if (fwrite (&MdfHdBlock, sizeof (MdfHdBlock), 1, *pfile) != 1) {
        CLOSE_FILE_FREE_BUFFERS
        return -1;
    }
    FilePos += sizeof (MdfHdBlock);

    ChannelCount = 0;
    for (vids_point = vids; *vids_point > 0; vids_point++) {
        ChannelCount++;
    }

    MEMCPY (MdfDgBlock.BlockTypeIdentifier, "DG", 2);
    MdfDgBlock.BlockSize = sizeof (MdfDgBlock); 
    MdfDgBlock.NextDataGroupBlock = 0;       // Pointer to next data group block (DGBLOCK) (NIL allowed)
    MdfDgBlock.FirstChannelGroupBlock = ftell (*pfile) + (int32_t)sizeof (MdfDgBlock);   // Pointer to first channel group block (CGBLOCK) (NIL allowed)
    MdfDgBlock.TriggerBlock = 0;
    MdfDgBlock.DataBlock = 0x7FFFFFFF;      // this will be written later
    FPos_DataBlock = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfDgBlock, DataBlock);

    MdfDgBlock.NumberOfChannelGroups = 1;  // Number of channel groups (redundant information)
    MdfDgBlock.NumberOfRecordIDs = 0;      // Number of record IDs in the data block
                                           // 0 = data records without record ID
                                           // 1 = record ID (UINT8) before each data record
                                           // 2 = record ID (UINT8) before and after each data record
    MdfDgBlock.Reserved = 0;

    FPos_NextDataGroupBlock = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfDgBlock, NextDataGroupBlock);

    if (fwrite (&MdfDgBlock, sizeof (MdfDgBlock), 1, *pfile) != 1) {
        CLOSE_FILE_FREE_BUFFERS
        return -1;
    }
    FilePos += sizeof (MdfDgBlock);

    MEMCPY (MdfCgBlock.BlockTypeIdentifier, "CG", 2);
    MdfCgBlock.BlockSize = sizeof (MdfCgBlock); 
    MdfCgBlock.NextChannelGroupBlock = 0;
    MdfCgBlock.FirstChannelBlock = (uint32_t)(ftell (*pfile) + (int32_t)sizeof (MdfCgBlock));
    MdfCgBlock.ChannelGroupComment = 0;
    MdfCgBlock.RecordID = 0;             // Record ID, i.e. value of the identifier for a record if the DGBLOCK defines a number of record IDs > 0
    MdfCgBlock.NumberOfChannels = (unsigned short)(ChannelCount + 1);    // Number of channels (redundant information)
    MdfCgBlock.SizeOfDataRecord = 0xFFFF;      // this will be written later
    FPos_SizeOfDataRecord = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfCgBlock, SizeOfDataRecord);
    MdfCgBlock.NumberOfRecords = 100;      // this will be written later
    FPos_NumberOfRecords = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfCgBlock, NumberOfRecords);

    if (fwrite (&MdfCgBlock, sizeof (MdfCgBlock), 1, *pfile) != 1) {
        CLOSE_FILE_FREE_BUFFERS
        return -1;
    }
    FilePos += sizeof (MdfCgBlock);

    BitOffsetInRecord = 0;

    // Time-Channel
    MEMSET (&MdfCnBlock, 0, sizeof (MdfCnBlock));
    MEMCPY (MdfCnBlock.BlockTypeIdentifier, "CN", 2);
    MdfCnBlock.BlockSize = sizeof (MdfCnBlock); 

    MdfCnBlock.NextChannelBlock = (uint32_t)(ftell (*pfile) + (int32_t)sizeof (MdfCnBlock));

    MdfCnBlock.ConversionFormula = 0;
    MdfCnBlock.SourceDependingExtensions = 0; 
    MdfCnBlock.DependencyBlock = 0;  
    MdfCnBlock.ChannelComment = 0; 
    MdfCnBlock.ChannelType = 1;     //    1 = time channel
    StringCopyMaxCharTruncate (MdfCnBlock.ShortSignalName, "t", 31);
    MdfCnBlock.ShortSignalName[31] = 0;
    STRING_COPY_TO_ARRAY (MdfCnBlock.SignalDescription, "");
    //  Start offset in bits to determine the first bit of the signal in the data record.
    MdfCnBlock.StartBitOffset = (uint16_t)(BitOffsetInRecord);
    //  Number of bits used to encode the value of this signal in a data record
    MdfCnBlock.NumberOfBits = 64;
    MdfCnBlock.SignalDataType = 3;  // IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
    BitOffsetInRecord += MdfCnBlock.NumberOfBits;

    MdfCnBlock.ValueRangeValidFlag = 0;
    MdfCnBlock.MinimumSignalValue = 0.0;
    MdfCnBlock.MaximumSignalValue = 0.0;
    //  Sampling rate for a virtual time channel
    MdfCnBlock.SamplingRate = 0.0; //hdrec_data.SamplePeriod;

    MdfCnBlock.LongSignalName = 0;
    MdfCnBlock.SignalDisplayName = 0;
    MdfCnBlock.AdditionalByteOffset = 0;

    if (fwrite (&MdfCnBlock, sizeof (MdfCnBlock), 1, *pfile) != 1) {
        CLOSE_FILE_FREE_BUFFERS
        return -1;
    }
    FilePos += sizeof (MdfCnBlock);

    // measurement signals
    for (Channel = 0; Channel < ChannelCount; Channel++) {
        MEMSET (&MdfCnBlock, 0, sizeof (MdfCnBlock));
        MEMCPY (MdfCnBlock.BlockTypeIdentifier, "CN", 2);
        MdfCnBlock.BlockSize = sizeof (MdfCnBlock); 

        MdfCnBlock.NextChannelBlock = 0x0;    // maybe this will be written later
        FPos_NextChannelBlock = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfCnBlock, NextChannelBlock);
    
        MdfCnBlock.ConversionFormula = 0x7FFFFFF;    // this will be written later
        FPos_ConversionFormula = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfCnBlock, ConversionFormula);

        MdfCnBlock.SourceDependingExtensions = 0; 
        MdfCnBlock.DependencyBlock = 0;  
        MdfCnBlock.ChannelComment = 0; 
        MdfCnBlock.ChannelType = 0;   //    0 = data channel
                                      //    1 = time channel
        // Not more than 31 characters allowed
        GetBlackboardVariableName (vids[Channel], SignalName, sizeof(SignalName));
        StringCopyMaxCharTruncate (MdfCnBlock.ShortSignalName, SignalName, 31);
        MdfCnBlock.ShortSignalName[31] = 0;
        STRING_COPY_TO_ARRAY (MdfCnBlock.SignalDescription, "");
        //  Start offset in bits to determine the first bit of the signal in the data record.
        MdfCnBlock.StartBitOffset = (uint16_t)(BitOffsetInRecord);
        //  Number of bits used to encode the value of this signal in a data record
        if ((dec_phys_flags != NULL) && (dec_phys_flags[Channel])) {
            MdfCnBlock.NumberOfBits = 64;
            MdfCnBlock.SignalDataType = 3;  // IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
        } else {
            switch (get_bbvaritype (vids[Channel])) {
            case BB_BYTE:
                MdfCnBlock.NumberOfBits = 8;
                MdfCnBlock.SignalDataType = 1; // signed integer
                break;
            case BB_UBYTE:
                MdfCnBlock.NumberOfBits = 8;
                MdfCnBlock.SignalDataType = 0; // unsigned integer
                break;
            case BB_WORD:
                MdfCnBlock.NumberOfBits = 16;
                MdfCnBlock.SignalDataType = 1; // signed integer
                break;
            case BB_UWORD:
                MdfCnBlock.NumberOfBits = 16;
                MdfCnBlock.SignalDataType = 0; // unsigned integer
                break;
            case BB_DWORD:
                MdfCnBlock.NumberOfBits = 32;
                MdfCnBlock.SignalDataType = 1; // signed integer
                break;
            case BB_UDWORD: 
                MdfCnBlock.NumberOfBits = 32;
                MdfCnBlock.SignalDataType = 0; // unsigned integer
                break;
            case BB_QWORD:
                MdfCnBlock.NumberOfBits = 64;
                MdfCnBlock.SignalDataType = 1; // signed integer
                break;
            case BB_UQWORD:
                MdfCnBlock.NumberOfBits = 64;
                MdfCnBlock.SignalDataType = 0; // unsigned integer
                break;
            case BB_FLOAT: 
                MdfCnBlock.NumberOfBits = 32;
                MdfCnBlock.SignalDataType = 2;  // IEEE 754 floating-point format FLOAT (4 bytes)
                break;
            case BB_DOUBLE:
                MdfCnBlock.NumberOfBits = 64;
                MdfCnBlock.SignalDataType = 3;  // IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
                break;
            default:
                CLOSE_FILE_FREE_BUFFERS
                return -1;
                //break;
            }
        }
        BitOffsetInRecord += MdfCnBlock.NumberOfBits;
        if((BitOffsetInRecord + MdfCnBlock.NumberOfBits) >= 0x10000) {
            CLOSE_FILE_FREE_BUFFERS
                ThrowError (1, "Try to record to many variables to MDF3 file \"%s\", max 65472 bits in one sample are allowed", hdrec_data);
            return -1;
        }

        MdfCnBlock.ValueRangeValidFlag = 1; 
        MdfCnBlock.MinimumSignalValue = get_bbvari_min (vids[Channel]); 
        MdfCnBlock.MaximumSignalValue = get_bbvari_max (vids[Channel]);
        //  Sampling rate for a virtual time channel
        MdfCnBlock.SamplingRate = 0.0; //hdrec_data.SamplePeriod;

        MdfCnBlock.LongSignalName = 0x7FFFFFF;    // this will be written later
        FPos_LongSignalName = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfCnBlock, LongSignalName);
        MdfCnBlock.SignalDisplayName = 0;
        MdfCnBlock.AdditionalByteOffset = 0;

        if (fwrite (&MdfCnBlock, sizeof (MdfCnBlock), 1, *pfile) != 1) {
            CLOSE_FILE_FREE_BUFFERS
            return -1;
        }
        FilePos += sizeof (MdfCnBlock);
        // Long signal name
        WriteLinkToHere (*pfile, FPos_LongSignalName);
        MEMCPY (MdfTxBlock.BlockTypeIdentifier, "TX", 2);
        MdfTxBlock.BlockSize = (uint16_t)(sizeof (MdfTxBlock) + strlen (SignalName) + 1);
        if (fwrite (&MdfTxBlock, sizeof (MdfTxBlock), 1, *pfile) != 1) {
            CLOSE_FILE_FREE_BUFFERS
            return -1;
        }
        FilePos += sizeof (MdfTxBlock);
        if (fwrite (SignalName, strlen (SignalName) + 1, 1, *pfile) != 1) {
            CLOSE_FILE_FREE_BUFFERS
            return -1;
        }
        FilePos += (int32_t)(strlen (SignalName) + 1);

        FilePosSave = ftell (*pfile);
        // Conversion formula
        WriteLinkToHere (*pfile, FPos_ConversionFormula);
        MEMSET(&MdfCcBlock, 0, sizeof(MdfCcBlock));
        MEMCPY (MdfCcBlock.BlockTypeIdentifier, "CC", 2);
        MdfCcBlock.BlockSize = sizeof (MdfCcBlock);
        FPos_BlockSize = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfCcBlock, BlockSize);
        MdfCcBlock.PhysicalValueRangeValidFlag = 1;
        MdfCcBlock.MinimumPhysicalSignalValue = MdfCnBlock.MinimumSignalValue;
        MdfCcBlock.MaximumPhysicalSignalValue = MdfCnBlock.MaximumSignalValue;
        get_bbvari_unit (vids[Channel], MdfCcBlock.PhysicalUnit, 19);
        MdfCcBlock.ConversionDataSize = 0;    // this will be written later
        FPos_ConversionDataSize = ftell (*pfile) + MDF_STRUCT_OFFSET (MdfCcBlock, ConversionDataSize);

        ConvType = get_bbvari_conversiontype (vids[Channel]);
        // If formula defind but variable already physical converted don't write a
        // formula into the MDA file
        if ((ConvType == BB_CONV_FORMULA) && (dec_phys_flags != NULL) && (dec_phys_flags[Channel] == 1)) {
            ConvType = BB_CONV_NONE;
        }
        switch (ConvType) {
        default:
        case BB_CONV_NONE:  // no conversion
            MdfCcBlock.ConversionType = 65535; //  1:1 conversion formula (Int = Phys)
            break;
        case BB_CONV_FORMULA:  // Formula
            get_bbvari_conversion (vids[Channel], Conversion, BBVARI_CONVERSION_SIZE);
            if (ConvertFormulaToFacOff (Conversion, &Conv.Conv.FactorOffset.Factor, &Conv.Conv.FactorOffset.Offset)) {
                // is it a linear function than switch to factor offset conversion
                ConvType = BB_CONV_FACTOFF;
                MdfCcBlock.ConversionType = 0; // CCBLOCK  Linear Function with 2 Parameters
            } else {
                MdfCcBlock.ConversionType = 10; // ASAM-MCD2 Text formula
            }
            break;
        case BB_CONV_TEXTREP:  // ENUM
            get_bbvari_conversion (vids[Channel], Conversion, BBVARI_CONVERSION_SIZE);
            MdfCcBlock.ConversionType = 12;  // ASAM-MCD2 Text Range Table (COMPU_VTAB_RANGE)
            break;
        case BB_CONV_FACTOFF:   // Linear with factor and offset
        case BB_CONV_OFFFACT:   // Linear with offset and factor
            get_bbvari_conversion (vids[Channel], Conversion, BBVARI_CONVERSION_SIZE);
            if (ConvType == BB_CONV_FACTOFF) {
                Conv_ParseFactorOffsetString(Conversion, &Conv);
            } else {
                Conv_ParseOffsetFactorString(Conversion, &Conv);
                Conv.Conv.FactorOffset.Offset *= Conv.Conv.FactorOffset.Factor;
            }
            MdfCcBlock.ConversionType = 0; // CCBLOCK Linear function with 2 Parameters
            break;
        case BB_CONV_TAB_INTP:   // Table with interpolation
            get_bbvari_conversion (vids[Channel], Conversion, BBVARI_CONVERSION_SIZE);
            Conv_ParseTableInterpolString(Conversion, &Conv);
            MdfCcBlock.ConversionType = 1; // tabular with interpolation
            break;
        case BB_CONV_TAB_NOINTP:   // Table without interpolation
            get_bbvari_conversion (vids[Channel], Conversion, BBVARI_CONVERSION_SIZE);
            Conv_ParseTableNoInterpolString(Conversion, &Conv);
            MdfCcBlock.ConversionType = 2; // tabular without interpolation
            break;
        case BB_CONV_RAT_FUNC:   // Rational function
            get_bbvari_conversion (vids[Channel], Conversion, BBVARI_CONVERSION_SIZE);
            Conv_ParseRationalFunctionString(Conversion, &Conv);
            MdfCcBlock.ConversionType = 9; // CCBLOCK Rational Function with 6 Parameters
            break;
        }

        if (fwrite (&MdfCcBlock, sizeof (MdfCcBlock), 1, *pfile) != 1) {
            CLOSE_FILE_FREE_BUFFERS
            return -1;
        }
        FilePos += sizeof (MdfCcBlock);

        switch (ConvType) {
        default:
        case BB_CONV_NONE:  // no conversion
            WriteUINT16ToPos (*pfile, 0, FPos_ConversionDataSize);
            WriteUINT16ToPos (*pfile, (uint16_t)sizeof(MDF_CCBLOCK), FPos_BlockSize);
            break;
        case BB_CONV_FORMULA:  // Formula
            {
                char *s, *d;
                // # replace wit X1
                s = Conversion;
                d = Conversion2;
                MEMSET (Conversion2, 0, BBVARI_CONVERSION_SIZE);
                while (*s != 0) {
                    if ((*s == '#') || (*s == '$')) {
                        *d++ = 'X';
                        s++;
                    } else {
                        *d++ = *s++;
                    }
                }
                *d = 0;
                if (fwrite (Conversion2, strlen (Conversion2) + 1, 1, *pfile) != 1) {
                    CLOSE_FILE_FREE_BUFFERS
                    return -1;
                }
                FilePos += (int32_t)(strlen (Conversion2) + 1);
                WriteUINT16ToPos (*pfile, (uint16_t)strlen (Conversion2), FPos_ConversionDataSize);
                WriteUINT16ToPos (*pfile, (uint16_t)(sizeof(MDF_CCBLOCK) + strlen (Conversion2) + 1), FPos_BlockSize);
            }
            break;
        case BB_CONV_TEXTREP:  // ENUM
            {
                int x;
                int Pos = 0;
                int Offset = 0;
                int Size = GetEnumListSize(Conversion);

                for (x = -1; x < Size; x++) {
                    char EnumText[512];
                    int64_t From, To;
                    MDF_COMPU_VTAB_RANGE TabElem;

                    if (x == -1) {  // First enty is "Undefined" we will ignore this
                        From = To = 0;
                        STRING_COPY_TO_ARRAY (EnumText, "Undefined");
                    } else {
                        Pos = GetNextEnumFromEnumList_Pos (Pos, Conversion, EnumText, sizeof(EnumText), &From, &To, NULL);
                        if (Pos < 0) {
                            break;
                        }
                    }
                    TabElem.LowerRange = (double)From;
                    TabElem.UpperRange = (double)To;
                    TabElem.PointerToTx = FilePosSave + sizeof(MDF_CCBLOCK) + (Size + 1) * sizeof(MDF_COMPU_VTAB_RANGE) + Offset;

                    Offset += (int)(sizeof(MDF_TXBLOCK) + strlen(EnumText) + 1);

                    if (fwrite (&TabElem, sizeof (TabElem), 1, *pfile) != 1) {
                        CLOSE_FILE_FREE_BUFFERS
                        return -1;
                    }
                }
                Pos = 0;
                for (x = -1; x < Size; x++) {
                    char EnumText[512];
                    int64_t From, To;
                    MDF_TXBLOCK Tx;

                    if (x == -1) {  // First enty is "Undefined" we will ignore this
                        From = To = 0;
                        STRING_COPY_TO_ARRAY (EnumText, "Undefined");
                    } else {
                        Pos = GetNextEnumFromEnumList_Pos (Pos, Conversion, EnumText, sizeof(EnumText), &From, &To, NULL);
                        if (Pos < 0) {
                            break;
                        }
                    }
                    MEMCPY (Tx.BlockTypeIdentifier, "TX", 2);
                    Tx.BlockSize = (uint16_t)(sizeof(MDF_TXBLOCK) +strlen(EnumText) + 1);

                    if (fwrite (&Tx, sizeof (Tx), 1, *pfile) != 1) {
                        CLOSE_FILE_FREE_BUFFERS
                        return -1;
                    }
                    if (fwrite (EnumText, strlen (EnumText) + 1, 1, *pfile) != 1) {
                        CLOSE_FILE_FREE_BUFFERS
                        return -1;
                    }
                }
                WriteUINT16ToPos (*pfile, (uint16_t)(Size + 1), FPos_ConversionDataSize);
                WriteUINT16ToPos (*pfile, (uint16_t)(sizeof(MDF_CCBLOCK) + (Size + 1) * sizeof(MDF_COMPU_VTAB_RANGE)), FPos_BlockSize);
            }
            break;
        case BB_CONV_FACTOFF:   // Linear with factor and offset
        case BB_CONV_OFFFACT:   // Linear with offset and factor
            if ((fwrite (&Conv.Conv.FactorOffset.Offset, sizeof(double), 1, *pfile) != 1) ||
                (fwrite (&Conv.Conv.FactorOffset.Factor, sizeof(double), 1, *pfile) != 1)) {
                CLOSE_FILE_FREE_BUFFERS
                    return -1;

            }
            FilePos += 2 * sizeof(double);
            WriteUINT16ToPos (*pfile, 2, FPos_ConversionDataSize);
            WriteUINT16ToPos (*pfile, (uint16_t)(sizeof(MDF_CCBLOCK) + 2 * sizeof (double)), FPos_BlockSize);
            break;
        case BB_CONV_TAB_NOINTP:   // Table without interpolation
        case BB_CONV_TAB_INTP:   // Table with interpolation
            for (x = 0; x < Conv.Conv.Table.Size; x++) {
                if ((fwrite (&Conv.Conv.Table.Values[x].Raw, sizeof(double), 1, *pfile) != 1) ||
                    (fwrite (&Conv.Conv.Table.Values[x].Phys, sizeof(double), 1, *pfile) != 1)) {
                    CLOSE_FILE_FREE_BUFFERS
                    return -1;

                }
            }
            FilePos += Conv.Conv.Table.Size * 2 * sizeof(double);
            WriteUINT16ToPos (*pfile, Conv.Conv.Table.Size, FPos_ConversionDataSize);
            WriteUINT16ToPos (*pfile, (uint16_t)(sizeof(MDF_CCBLOCK) + 2 * sizeof (double) * Conv.Conv.Table.Size), FPos_BlockSize);
            break;
        case BB_CONV_RAT_FUNC:   // Rational function
            if ((fwrite (&Conv.Conv.RatFunc.a, sizeof(double), 1, *pfile) != 1) ||
                (fwrite (&Conv.Conv.RatFunc.b, sizeof(double), 1, *pfile) != 1) ||
                (fwrite (&Conv.Conv.RatFunc.c, sizeof(double), 1, *pfile) != 1) ||
                (fwrite (&Conv.Conv.RatFunc.d, sizeof(double), 1, *pfile) != 1) ||
                (fwrite (&Conv.Conv.RatFunc.e, sizeof(double), 1, *pfile) != 1) ||
                (fwrite (&Conv.Conv.RatFunc.f, sizeof(double), 1, *pfile) != 1)) {
                CLOSE_FILE_FREE_BUFFERS
                return -1;
            }
            FilePos += 6 * sizeof(double);
            WriteUINT16ToPos (*pfile, 6, FPos_ConversionDataSize);
            WriteUINT16ToPos (*pfile, (uint16_t)(sizeof(MDF_CCBLOCK) + 6 * sizeof (double)), FPos_BlockSize);
            break;
        }

        // Pointer to next channel description if there are one more
        if (Channel < (ChannelCount - 1)) WriteLinkToHere (*pfile, FPos_NextChannelBlock);
    }

    WriteUINT16ToPos (*pfile, (uint16_t)(BitOffsetInRecord / 8), FPos_SizeOfDataRecord);

    // Now followed by the data
    WriteLinkToHere (*pfile, FPos_DataBlock);
    if (Conversion != NULL) my_free (Conversion);
    if (Conversion2 != NULL) my_free (Conversion2);
    return 0;
}


int WriteRingbuffMdf (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count)
{
    int i;
    int status = 0;

    for (i = 1; ((i < rpvari_count) && !status); i++) {
        switch (stamp->EntryList[i].type) {
        case BB_BYTE:
            status |= !fwrite (&stamp->EntryList[i].value.b, sizeof (char), 1, file);
            break;
        case BB_UBYTE:
            status |= !fwrite (&stamp->EntryList[i].value.ub, sizeof (unsigned char), 1, file);
            break;
        case BB_WORD:
            status |= !fwrite (&stamp->EntryList[i].value.w, sizeof (int16_t), 1, file);
            break;
        case BB_UWORD:
            status |= !fwrite (&stamp->EntryList[i].value.uw, sizeof (uint16_t), 1, file);
            break;
        case BB_DWORD:
            status |= !fwrite (&stamp->EntryList[i].value.dw, sizeof (int32_t), 1, file);
            break;
        case BB_UDWORD:
            status |= !fwrite (&stamp->EntryList[i].value.udw, sizeof (uint32_t), 1, file);
            break;
        case BB_QWORD:
            status |= !fwrite (&stamp->EntryList[i].value.qw, sizeof (int64_t), 1, file);
            break;
        case BB_UQWORD:
            status |= !fwrite (&stamp->EntryList[i].value.uqw, sizeof (uint64_t), 1, file);
            break;
        case BB_FLOAT:
            status |= !fwrite (&stamp->EntryList[i].value.f, sizeof (float), 1, file);
            break;
        case BB_DOUBLE:
            status |= !fwrite (&stamp->EntryList[i].value.d, sizeof (double), 1, file);
            break;
        }
    }
    return ((status) ? EOF : 0);
}

typedef struct {
    uint64_t Timestamp;
    char Comment[2048/8];
} RECORDER_COMMENT_ELEM;

static RECORDER_COMMENT_ELEM *Comments;
static int SizeOfComments;
static int NoOfComments;


static int WriteAllCommentsToMdfFile(FILE *fh, uint64_t RecorderStartTime)
{
    int x;
    MDF_DGBLOCK MdfDgBlock;
    MDF_CGBLOCK MdfCgBlock;
    MDF_CNBLOCK MdfCnBlock;
    long FPos_DataBlock;
    long FilePos;
    int BitOffsetInRecord;
    int Ret = -1;

    FilePos = ftell (fh);
    // we have a next channel group
    WriteUINT16ToPos (fh, 2, FPos_NumberOfDataGroups);
    WriteUINT32ToPos (fh, FilePos, FPos_NextDataGroupBlock);

    MEMCPY (MdfDgBlock.BlockTypeIdentifier, "DG", 2);
    MdfDgBlock.BlockSize = sizeof (MdfDgBlock);
    MdfDgBlock.NextDataGroupBlock = 0;       // Pointer to next data group block (DGBLOCK) (NIL allowed)
    MdfDgBlock.FirstChannelGroupBlock = ftell (fh) + (int32_t)sizeof (MdfDgBlock);   // Pointer to first channel group block (CGBLOCK) (NIL allowed)
    MdfDgBlock.TriggerBlock = 0;
    MdfDgBlock.DataBlock = 0x7FFFFFFF;      // wird spaeter geschrieben
    FPos_DataBlock = ftell (fh) + MDF_STRUCT_OFFSET (MdfDgBlock, DataBlock);

    MdfDgBlock.NumberOfChannelGroups = 1;  // Number of channel groups (redundant information)
    MdfDgBlock.NumberOfRecordIDs = 0;      // Number of record IDs in the data block
        // 0 = data records without record ID
        // 1 = record ID (UINT8) before each data record
        // 2 = record ID (UINT8) before and after each data record
    MdfDgBlock.Reserved = 0;

    if (fwrite (&MdfDgBlock, sizeof (MdfDgBlock), 1, fh) != 1) {
        goto __ERROR;
    }
    FilePos += sizeof (MdfDgBlock);

    MEMCPY (MdfCgBlock.BlockTypeIdentifier, "CG", 2);
    MdfCgBlock.BlockSize = sizeof (MdfCgBlock);
    MdfCgBlock.NextChannelGroupBlock = 0;
    MdfCgBlock.FirstChannelBlock = (uint32_t)(ftell (fh) + (long)sizeof (MdfCgBlock));
    MdfCgBlock.ChannelGroupComment = 0;
    MdfCgBlock.RecordID = 0;             // Record ID, i.e. value of the identifier for a record if the DGBLOCK defines a number of record IDs > 0
    MdfCgBlock.NumberOfChannels = (unsigned short)(1 + 1);    // time + comment
    MdfCgBlock.SizeOfDataRecord = sizeof(double) + 2048 / 8;  // time + comment
    MdfCgBlock.NumberOfRecords = NoOfComments;

    if (fwrite (&MdfCgBlock, sizeof (MdfCgBlock), 1, fh) != 1) {
        goto __ERROR;
    }
    FilePos += sizeof (MdfCgBlock);

    BitOffsetInRecord = 0;

    // Time-Channel

    MEMSET (&MdfCnBlock, 0, sizeof (MdfCnBlock));
    MEMCPY (MdfCnBlock.BlockTypeIdentifier, "CN", 2);
    MdfCnBlock.BlockSize = sizeof (MdfCnBlock);

    MdfCnBlock.NextChannelBlock = (uint32_t)(ftell (fh) + (long)sizeof (MdfCnBlock));

    MdfCnBlock.ConversionFormula = 0;
    MdfCnBlock.SourceDependingExtensions = 0;
    MdfCnBlock.DependencyBlock = 0;
    MdfCnBlock.ChannelComment = 0;
    MdfCnBlock.ChannelType = 1;     //    1 = time channel for all signals of this group (in each channel group, exactly one channel must be defined as time channel)
    strncpy (MdfCnBlock.ShortSignalName, "t", 31);
    MdfCnBlock.ShortSignalName[31] = 0;
    STRING_COPY_TO_ARRAY (MdfCnBlock.SignalDescription, "");
    //  Start offset in bits to determine the first bit of the signal in the data record.
    //  The start offset is a combination of a "Byte offset" and a "Bit offset".
    //  There can be an "additional Byte offset" (see below) which must be added to the Byte offset.
    //  The (total) Byte offset is applied to the plain record data, i.e. without record ID,
    //  and points to the first Byte that contains bits of the signal value.
    //  The bit offset is used to determine the LSB within the Bytes for the signal value.
    MdfCnBlock.StartBitOffset = (uint16_t)(BitOffsetInRecord);
    //  Number of bits used to encode the value of this signal in a data record
    MdfCnBlock.NumberOfBits = 64;
    MdfCnBlock.SignalDataType = 3;  // IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
    BitOffsetInRecord += MdfCnBlock.NumberOfBits;

    MdfCnBlock.ValueRangeValidFlag = 0;
    MdfCnBlock.MinimumSignalValue = 0.0;
    MdfCnBlock.MaximumSignalValue = 0.0;
    //  Sampling rate for a virtual time channel. Unit [s]
    MdfCnBlock.SamplingRate = 0.0;
    MdfCnBlock.LongSignalName = 0;
    MdfCnBlock.SignalDisplayName = 0;
    MdfCnBlock.AdditionalByteOffset = 0;

    if (fwrite (&MdfCnBlock, sizeof (MdfCnBlock), 1, fh) != 1) {
        goto __ERROR;
    }
    FilePos += sizeof (MdfCnBlock);

    // Comments

    MEMSET (&MdfCnBlock, 0, sizeof (MdfCnBlock));
    MEMCPY (MdfCnBlock.BlockTypeIdentifier, "CN", 2);
    MdfCnBlock.BlockSize = sizeof (MdfCnBlock);

    MdfCnBlock.NextChannelBlock = 0; // no more blocks (uint32_t)(ftell (fh) + (long)sizeof (MdfCnBlock));

    MdfCnBlock.ConversionFormula = 0;
    MdfCnBlock.SourceDependingExtensions = 0;
    MdfCnBlock.DependencyBlock = 0;
    MdfCnBlock.ChannelComment = 0;
    MdfCnBlock.ChannelType = 0;     //    1 = time channel for all signals of this group (in each channel group, exactly one channel must be defined as time channel)
    strncpy (MdfCnBlock.ShortSignalName, "Comment", 31);
    MdfCnBlock.ShortSignalName[31] = 0;
    STRING_COPY_TO_ARRAY (MdfCnBlock.SignalDescription, "Comments entered by the user during measurement");
    //  Start offset in bits to determine the first bit of the signal in the data record.
    //  The start offset is a combination of a "Byte offset" and a "Bit offset".
    //  There can be an "additional Byte offset" (see below) which must be added to the Byte offset.
    //  The (total) Byte offset is applied to the plain record data, i.e. without record ID,
    //  and points to the first Byte that contains bits of the signal value.
    //  The bit offset is used to determine the LSB within the Bytes for the signal value.
    MdfCnBlock.StartBitOffset = (uint16_t)(BitOffsetInRecord);
    //  Number of bits used to encode the value of this signal in a data record
    MdfCnBlock.NumberOfBits = 2048;
    MdfCnBlock.SignalDataType = 7;  // IEEE 754 floating-point format DOUBLE (8 / 10 bytes)
    BitOffsetInRecord += MdfCnBlock.NumberOfBits;
    MdfCnBlock.ValueRangeValidFlag = 0;
    MdfCnBlock.MinimumSignalValue = 0.0;
    MdfCnBlock.MaximumSignalValue = 0.0;
    //  Sampling rate for a virtual time channel. Unit [s]
    MdfCnBlock.SamplingRate = 0.0;
    MdfCnBlock.LongSignalName = 0;
    MdfCnBlock.SignalDisplayName = 0;
    MdfCnBlock.AdditionalByteOffset = 0;

    if (fwrite (&MdfCnBlock, sizeof (MdfCnBlock), 1, fh) != 1) {
        goto __ERROR;
    }
    FilePos += sizeof (MdfCnBlock);

    FilePos = ftell (fh);
    WriteUINT32ToPos (fh, FilePos, FPos_DataBlock);

    // Add the data
    for (x = 0; x < NoOfComments; x++) {
        double Time = (double)(Comments[x].Timestamp - RecorderStartTime) / TIMERCLKFRQ;
        if (fwrite (&Time, sizeof (Time), 1, fh) != 1) {
            goto __ERROR;
        }
        if (fwrite (Comments[x].Comment, sizeof (Comments[x].Comment), 1, fh) != 1) {
            goto __ERROR;
        }
    }
    Ret = 0;
__ERROR:
    NoOfComments = 0;
    SizeOfComments = 0;
    my_free(Comments);
    Comments = NULL;
    return Ret;
}


int TailMdfFile (FILE *fh, uint32_t Samples, uint64_t RecorderStartTime)
{
    WriteUINT32ToPos (fh, Samples, FPos_NumberOfRecords);
    // are there any comments to store?
    if (NoOfComments > 0) {
        if (WriteAllCommentsToMdfFile(fh, RecorderStartTime)) {
            ThrowError(1, "cannot store comments to MDF file");
        }
    }
    close_file (fh);
    return 0;
}


int WriteCommentToMdf (FILE *File, uint64_t Timestamp, const char *Comment)
{
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
    MEMSET(Comments[NoOfComments].Comment, 0, sizeof(Comments[NoOfComments].Comment));
    strncpy(Comments[NoOfComments].Comment, Comment, sizeof(Comments[NoOfComments].Comment) - 1);
    NoOfComments++;
    return 0;
}


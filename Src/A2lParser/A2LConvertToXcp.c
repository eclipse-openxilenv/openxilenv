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


#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "StringMaxChar.h"
#include "IniDataBase.h"
#include "Blackboard.h"
#include "A2LAccess.h"
#include "A2LLink.h"
#include "A2LConvertToXcp.h"

#define MAX_LINE_LEN       (128*1024)
#define MAX_CONVERSION_LEN (128*1024)

int A2LConvertToXcpOrCpp(const char *par_A2LFile, const char *par_XcpOrCppFile, int par_XcpOrCpp, int par_Flags, char *ErrString, int MaxErrSize)
{
    ASAP2_DATABASE *Database;
    int LineNr;
    int Ret = -1;

    Database = LoadAsapFile (par_A2LFile, 0, ErrString, MaxErrSize, &LineNr);
    if (Database == NULL) {
        return -1;
    } else {
        int Fd = IniFileDataBaseCreateAndOpenNewIniFile(par_XcpOrCppFile);
        if (Fd > 0) {
            const char *Section;
            char Name[512];
            char Entry[16];
            char *Line;
            int Type;
            uint64_t Address;
            int ConvType;
            char *Conv;
            char Unit[64];
            int FormatLength, FormatLayout;
            int XDim, YDim, ZDim;
            double Min, Max;
            int IsWritable;

            int Idx, v, p;

            if (par_XcpOrCpp) Section = "XCP Configuration";
            else Section = "CCP Configuration for Target";

            Line = (char*)my_malloc(MAX_LINE_LEN);
            Conv = (char*)my_malloc(MAX_CONVERSION_LEN);
            if ((Line == NULL) || (Conv == NULL)) {
                StringCopyMaxCharTruncate (ErrString, "out of memory", MaxErrSize);
                Ret = -1;
            } else {
                // first delete all variable and parameter
                for (v = 0; ; v++) {
                    PrintFormatToString (Entry, sizeof(Entry), "v%i", v);
                    if (IniFileDataBaseReadString(Section, Entry, "", Line, 128*1024, Fd) > 0) {
                        IniFileDataBaseWriteString(Section, Entry, NULL, Fd);
                    } else break;
                }
                for (p = 0; ; p++) {
                    PrintFormatToString (Entry, sizeof(Entry), "p%i", p);
                    if (IniFileDataBaseReadString(Section, Entry, "", Line, 128*1024, Fd) > 0) {
                        IniFileDataBaseWriteString(Section, Entry, NULL, Fd);
                    } else break;
                }
                // Write the new measurements
                Idx = -1;
                v = p = 0;
                while ((Idx = GetNextMeasurement(Database, Idx, "*", A2L_LABEL_TYPE_MEASUREMENT, Name, sizeof(Name))) >= 0)  {
                    if (GetMeasurementInfos (Database, Idx,
                                             Name, sizeof(Name),
                                             &Type, &Address,
                                             &ConvType, Conv, sizeof(Conv),
                                             Unit, sizeof(Unit),
                                             &FormatLength, &FormatLayout,
                                             &XDim, &YDim, &ZDim,
                                             &Min, &Max, &IsWritable) == 0) {
                        if((XDim <= 1) && (YDim <= 1) && (ZDim <= 1)) {
                            //v0=DWORD,FDRawTxStd_Sig2.Value,0x7000AEAC,"-",#,-2147483648,2147483647
                            if (ConvType != 1) {
                                StringCopyMaxCharTruncate(Conv, "#", MAX_CONVERSION_LEN);
                            }
                            PrintFormatToString (Line, sizeof(Line), "%s,%s,0x%" PRIX64 ",%s,%s,%g,%g",
                                     GetDataTypeName(Type), Name, Address, Unit, Conv, Min, Max);
                            PrintFormatToString (Entry, sizeof(Entry), "v%i", v);
                            IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                            v++;
                            if (IsWritable &&
                                ((par_Flags & A2LCONVERT2XCP_WRITABLE_MEASUREMENTS_AS_PARAMETER) == A2LCONVERT2XCP_WRITABLE_MEASUREMENTS_AS_PARAMETER)) {
                                PrintFormatToString (Entry, sizeof(Entry), "p%i", p);
                                IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                                p++;
                            }
                        } else if ((YDim <= 1) && (ZDim <= 1)) {  // one dimensional array
                            int i;
                            for (i = 0; i < XDim; i++) {
                                PrintFormatToString (Line, sizeof(Line), "%s,%s[%i],0x%" PRIX64 ",%s,%s,%g,%g",
                                         GetDataTypeName(Type), Name, i, Address + GetDataTypeByteSize(Type) * i, Unit, Conv, Min, Max);
                                PrintFormatToString (Entry, sizeof(Entry), "v%i", v);
                                IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                                v++;
                                if (IsWritable &&
                                    ((par_Flags & A2LCONVERT2XCP_WRITABLE_MEASUREMENTS_AS_PARAMETER) == A2LCONVERT2XCP_WRITABLE_MEASUREMENTS_AS_PARAMETER)) {
                                    PrintFormatToString (Entry, sizeof(Entry), "p%i", p);
                                    IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                                    p++;
                                }
                            }
                        } else if (ZDim <= 1) { // two dimensional array
                            int i, j;
                            for (j = 0; j < YDim; j++) {
                                for (i = 0; i < XDim; i++) {
                                    PrintFormatToString (Line, sizeof(Line), "%s,%s[%i][%i],0x%" PRIX64 ",%s,%s,%g,%g",
                                             GetDataTypeName(Type), Name, i, j, Address + GetDataTypeByteSize(Type) * (j + XDim + i), Unit, Conv, Min, Max);
                                    PrintFormatToString (Entry, sizeof(Entry), "v%i", v);
                                    IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                                    v++;
                                    if (IsWritable &&
                                        ((par_Flags & A2LCONVERT2XCP_WRITABLE_MEASUREMENTS_AS_PARAMETER) == A2LCONVERT2XCP_WRITABLE_MEASUREMENTS_AS_PARAMETER)) {
                                        PrintFormatToString (Entry, sizeof(Entry), "p%i", p);
                                        IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                                        p++;
                                    }
                                }
                            }
                        }
                    }
                }
                // be sure no old entries follow
                for (int i = v; i < (v+10); i++) {
                    PrintFormatToString (Entry, sizeof(Entry), "v%i", i);
                    IniFileDataBaseWriteString(Section, Entry, NULL, Fd);

                }
                // Write the new characteristics
                Idx = -1;
                while ((Idx = GetNextCharacteristic(Database, Idx, "*", A2L_LABEL_TYPE_SINGLE_VALUE_CALIBRATION, Name, sizeof(Name))) >= 0)  {
                    if (GetValueCharacteristicInfos(Database, Idx,
                                                    Name, sizeof(Name),
                                                    &Type, &Address,
                                                    &ConvType, Conv, sizeof(Conv),
                                                    Unit, sizeof(Unit),
                                                    &FormatLength, &FormatLayout,
                                                    &XDim, &YDim, &ZDim,
                                                    &Min, &Max, &IsWritable) == 0) {
                        if((XDim <= 1) && (YDim <= 1) && (ZDim <= 1)) {
                            //v0=DWORD,FDRawTxStd_Sig2.Value,0x7000AEAC,"-",#,-2147483648,2147483647
                            if (ConvType != 1) {
                                StringCopyMaxCharTruncate(Conv, "#", MAX_CONVERSION_LEN);
                            }
                            PrintFormatToString (Entry, sizeof(Entry), "p%i", p);
                            PrintFormatToString (Line, sizeof(Line), "%s,%s,0x%" PRIX64 ",%s,%s,%g,%g",
                                     GetDataTypeName(Type), Name, Address, Unit, Conv, Min, Max);
                            IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                            p++;
                        } else if ((YDim <= 1) && (ZDim <= 1)) {  // one dimensional array
                            int i;
                            for (i = 0; i < XDim; i++) {
                                PrintFormatToString (Entry, sizeof(Entry), "p%i", p);
                                PrintFormatToString (Line, sizeof(Line), "%s,%s[%i],0x%" PRIX64 ",%s,%s,%g,%g",
                                         GetDataTypeName(Type), Name, i, Address + GetDataTypeByteSize(Type) * i, Unit, Conv, Min, Max);
                                IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                                p++;
                            }
                        } else if (ZDim <= 1) { // two dimensional array
                            int i, j;
                            for (j = 0; j < YDim; j++) {
                                for (i = 0; i < XDim; i++) {
                                    PrintFormatToString (Entry, sizeof(Entry), "p%i", p);
                                    PrintFormatToString (Line, sizeof(Line), "%s,%s[%i][%i],0x%" PRIX64 ",%s,%s,%g,%g",
                                             GetDataTypeName(Type), Name, i, j, Address + GetDataTypeByteSize(Type) * (j + XDim + i), Unit, Conv, Min, Max);
                                    IniFileDataBaseWriteString(Section, Entry, Line, Fd);
                                    p++;
                                }
                            }
                        }
                    }
                }
                // be sure no old entries follow
                for (int i = p; i < (p+10); i++) {
                    PrintFormatToString (Entry, sizeof(Entry), "p%i", i);
                    IniFileDataBaseWriteString(Section, Entry, NULL, Fd);

                }
                my_free(Line);
                my_free(Conv);
            }
            Ret = IniFileDataBaseSave(Fd, NULL, 2);
        }
        FreeAsap2Database(Database);
    }
   return Ret;
}

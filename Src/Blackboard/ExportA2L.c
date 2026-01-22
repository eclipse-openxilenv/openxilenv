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

#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Files.h"
#include "ThrowError.h"
#include "Blackboard.h"
#include "TextReplace.h"

// convert a blackboard data type to an ASAP data type
static int ConvertDataType (int bbtype, char *asaptype, int maxc)
{
    switch (bbtype) {
    case BB_BYTE:
        StringCopyMaxCharTruncate (asaptype, "SBYTE", maxc);
        break;
    case BB_UBYTE:
        StringCopyMaxCharTruncate (asaptype, "UBYTE", maxc);
        break;
    case BB_WORD:
        StringCopyMaxCharTruncate (asaptype, "SWORD", maxc);
        break;
    case BB_UWORD:
        StringCopyMaxCharTruncate (asaptype, "UWORD", maxc);
        break;
    case BB_DWORD:
        StringCopyMaxCharTruncate (asaptype, "SLONG", maxc);
        break;
    case BB_UDWORD:
        StringCopyMaxCharTruncate (asaptype, "ULONG", maxc);
        break;
    case BB_FLOAT:
    case BB_DOUBLE:
        StringCopyMaxCharTruncate (asaptype, "FLOAT32_IEEE", maxc);
        break;
    default:
        return -1;
    }
    return 0;
}

int ExportA2lFile (const char *fname)
{
    FILE *fh;
    int x, vid_index;
    char *convstr;
    char  *s, *d;
    char txt[1024];
    int count;
    double Factor, Offset;

    if ((fh = open_file (fname, "w+b")) == NULL) {
        ThrowError (1, "cannot open file %s", fname);
        return -1;
    }

    convstr = my_malloc (BBVARI_CONVERSION_SIZE*4);
    if (convstr == NULL) {
        fclose (fh);
        return -1;
    }

    // Head of A2L file
    fprintf (fh, "ASAP2_VERSION 1 31\r\n"
                 "/begin PROJECT exporting \"\"\r\n"
                 "  /begin MODULE ROM1 \"%s\"\r\n\r\n"
                 "    /begin MOD_COMMON \"\"\r\n"
                 "      DEPOSIT ABSOLUTE\r\n"
                 "      BYTE_ORDER MSB_LAST\r\n"
                 "    /end MOD_COMMON\r\n\r\n",
                 fname);

    for (vid_index = 0; vid_index < get_blackboardsize(); vid_index++) {
        if ((blackboard[vid_index].Vid > 0) &&
            (blackboard[vid_index].Type < BB_UNKNOWN)) {
            // export conversion
            switch (blackboard[vid_index].pAdditionalInfos->Conversion.Type) {
            default:
            case BB_CONV_NONE:
                fprintf (fh, "    /begin COMPU_METHOD %s_conv\r\n"
                             "      \"\"\r\n"
                             "      RAT_FUNC\r\n"
                             "      \"%%%i.%i\"\r\n"
                             "      \"%s\"\r\n"
                             "      COEFFS 0 1 0 0 0 1\r\n"
                             "    /end COMPU_METHOD\r\n",
                             blackboard[vid_index].pAdditionalInfos->Name,
                             (int)blackboard[vid_index].pAdditionalInfos->Width,
                             (int)blackboard[vid_index].pAdditionalInfos->Prec,
                             blackboard[vid_index].pAdditionalInfos->Unit);
                break;
            case BB_CONV_FORMULA:
                s = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString;
                d = convstr;
                while (*s != '\0') {
                    if ((*s == '#') || (*s == '$')) *d = 'X';
                    else *d = *s;
                    s++; d++;
                }
                *d = '\0';
                fprintf (fh, "    /begin COMPU_METHOD %s_conv\r\n"
                             "      \"\"\r\n"
                             "      FORM\r\n"
                             "      \"%%%i.%i\"\r\n"
                             "      \"%s\"\r\n"
                             "      /begin FORMULA \"%s\" /end FORMULA\r\n"
                             "    /end COMPU_METHOD\r\n",
                             blackboard[vid_index].pAdditionalInfos->Name,
                             (int)blackboard[vid_index].pAdditionalInfos->Width,
                             (int)blackboard[vid_index].pAdditionalInfos->Prec,
                             blackboard[vid_index].pAdditionalInfos->Unit,
                             convstr);
                break;
            case BB_CONV_TEXTREP:
                fprintf (fh, "    /begin COMPU_METHOD %s_conv\r\n"
                             "      \"\"\r\n"
                             "      TAB_VERB\r\n"
                             "      \"%%%i.%i\"\r\n"
                             "      \"%s\"\r\n"
                             "      COMPU_TAB_REF %s_enum\r\n"
                             "    /end COMPU_METHOD\r\n",
                             blackboard[vid_index].pAdditionalInfos->Name,
                             (int)blackboard[vid_index].pAdditionalInfos->Width,
                             (int)blackboard[vid_index].pAdditionalInfos->Prec,
                             blackboard[vid_index].pAdditionalInfos->Unit,
                             blackboard[vid_index].pAdditionalInfos->Name);
                fprintf (fh, "\r\n");
                fprintf (fh, "    /begin COMPU_VTAB %s_enum\r\n"
                             "      \"\"\r\n"
                             "      TAB_VERB\r\n",
                             blackboard[vid_index].pAdditionalInfos->Name);
                if ((count = textreplace2asap (blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString, convstr)) < 0) {
                    ThrowError (1, "cannot transform conversion %s of label %s",
                           blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString,
                           blackboard[vid_index].pAdditionalInfos->Name);
                    count = 0;
                    convstr[0] = 0;

                }
                fprintf (fh, "      %i\r\n", count);
                fprintf (fh, "%s", convstr);
                fprintf (fh, "    /end COMPU_VTAB\r\n");
                break;
            case BB_CONV_TAB_INTP:
            case BB_CONV_TAB_NOINTP:
                fprintf (fh, "    /begin COMPU_METHOD %s_conv\r\n"
                            "      \"\"\r\n"
                            "      %s\r\n"
                            "      \"%%%i.%i\"\r\n"
                            "      \"%s\"\r\n"
                            "      COMPU_TAB_REF %s_enum\r\n"
                            "    /end COMPU_METHOD\r\n",
                        blackboard[vid_index].pAdditionalInfos->Name,
                        (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_TAB_INTP) ? "TAB_INTP" : "TAB_NOINTP",
                        (int)blackboard[vid_index].pAdditionalInfos->Width,
                        (int)blackboard[vid_index].pAdditionalInfos->Prec,
                        blackboard[vid_index].pAdditionalInfos->Unit,
                        blackboard[vid_index].pAdditionalInfos->Name);
                fprintf (fh, "\r\n");
                fprintf (fh, "    /begin COMPU_TAB %s_enum\r\n"
                            "      \"\"\r\n"
                            "      %s\r\n",
                            "      %i\r\n",
                        blackboard[vid_index].pAdditionalInfos->Name,
                        (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_TAB_INTP) ? "TAB_INTP" : "TAB_NOINTP",
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Size);
                for (x = 0; x < blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Size; x++) {
                    fprintf (fh, "      %.18g %.18g\r\n",
                             blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values->Phys,
                             blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values->Raw);
                }
                fprintf (fh, "    /end COMPU_TAB\r\n");
                break;
            case BB_CONV_FACTOFF:
            case BB_CONV_OFFFACT:
                if (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_FACTOFF) {
                    Factor = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor;
                    Offset = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Offset;
                } else {
                    Factor = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor;
                    Offset = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Offset *
                             blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor;
                }
                fprintf (fh, "    /begin COMPU_METHOD %s_conv\r\n"
                            "      \"\"\r\n"
                            "      LINEAR\r\n"
                            "      \"%%%i.%i\"\r\n"
                            "      \"%s\"\r\n"
                            "      COEFFS_LINEAR %.18g %.18g\r\n"
                            "    /end COMPU_METHOD\r\n",
                        blackboard[vid_index].pAdditionalInfos->Name,
                        (int)blackboard[vid_index].pAdditionalInfos->Width,
                        (int)blackboard[vid_index].pAdditionalInfos->Prec,
                        blackboard[vid_index].pAdditionalInfos->Unit,
                        Factor, Offset);
                break;
            case BB_CONV_RAT_FUNC:
                fprintf (fh, "    /begin COMPU_METHOD %s_conv\r\n"
                            "      \"\"\r\n"
                            "      RAT_FUNC\r\n"
                            "      \"%%%i.%i\"\r\n"
                            "      \"%s\"\r\n"
                            "      COEFFS %.18g %.18g %.18g %.18g %.18g %.18g\r\n"
                            "    /end COMPU_METHOD\r\n",
                        blackboard[vid_index].pAdditionalInfos->Name,
                        (int)blackboard[vid_index].pAdditionalInfos->Width,
                        (int)blackboard[vid_index].pAdditionalInfos->Prec,
                        blackboard[vid_index].pAdditionalInfos->Unit,
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.a,
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.b,
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.c,
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.d,
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.e,
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.f);
                break;
            }

            if (!ConvertDataType (blackboard[vid_index].Type, txt, sizeof(txt))) {
                fprintf (fh, "\r\n");
                fprintf (fh, "    /begin MEASUREMENT %s\r\n",
                         blackboard[vid_index].pAdditionalInfos->Name);
                fprintf (fh, "      \"%s\"\r\n",
                         blackboard[vid_index].pAdditionalInfos->Name); // Beschreibung
                fprintf (fh, "      %s\r\n", txt);   // Datentyp
                fprintf (fh, "      %s_conv\r\n",
                          blackboard[vid_index].pAdditionalInfos->Name); // Umrechnung
                fprintf (fh, "      1\r\n");
                fprintf (fh, "      100\r\n");
                fprintf (fh, "      %.16g\r\n", blackboard[vid_index].pAdditionalInfos->Min);  // min
                fprintf (fh, "      %.16g\r\n", blackboard[vid_index].pAdditionalInfos->Max);  // max
                fprintf (fh, "      ECU_ADDRESS 0x0\r\n");
                fprintf (fh, "    /end MEASUREMENT\r\n");
                fprintf (fh, "\r\n");
            }
        }
    }

    my_free (convstr);

    fprintf (fh, "  /end MODULE\r\n");
    fprintf (fh, "/end PROJECT\r\n");
    close_file (fh);
    return 0;
}

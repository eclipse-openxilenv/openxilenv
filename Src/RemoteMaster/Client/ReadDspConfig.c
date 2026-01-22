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
#include "IniFileDataBase.h"
#include "Blackboard.h"
#include "Scheduler.h"
#include "Files.h"
#include "Message.h"
#include "EquationList.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ExecutionStack.h"
#include "Dsp56301ConfigStruct.h"
#include "ReadFlexcardDigIOConfig.h"

#include "ReadDspConfig.h"

#define UNUSED(x) (void)(x)

typedef struct {
    // Umrechnung
    int ConvType;
    double Factor;
    double Offset;
    double OutMin;    // oder Fg bei ADC
    double OutMax;    // oder D bei ADC
    char ConvString[2048];
} CONFIG_CONV_DLG_STRUCT;

typedef struct {
    char *DialogName;
    char *ConvName;
    char *MinName;
    char *MinUnit;
    char *MaxName;
    char *MaxUnit;
    CONFIG_CONV_DLG_STRUCT *pConfConv;
} CONV_DLG_STRUCT;

typedef struct {
    int Card;
    int InOut;   // 0 -> In, 1 -> Out
    int OnOff;
    char SignalName[BBVARI_NAME_SIZE];
    char Unit[BBVARI_UNIT_SIZE];
    char DataType[16];
    char SignalType[8];
    // PT2 Filter fr ADC
    double Fg;
    double D;
    // Umrechnung
    CONFIG_CONV_DLG_STRUCT Conv;
    // DAC Amplitude fr DDS
    int AmplType;   // 0 -> Fixed 1 -> Variable
    double AmplFixedValue;
    char AmplSignalName[BBVARI_NAME_SIZE];
    char AmplUnit[BBVARI_UNIT_SIZE];
    char AmplDataType[16];
    int AmplConvType;
    double AmplFactor;
    double AmplOffset;
    char AmplConvString[2048];

    int OffsetType;   // 0 -> Fixed 1 -> Variable
    double OffsetFixedValue;
    char OffsetSignalName[BBVARI_NAME_SIZE];
    char OffsetUnit[BBVARI_UNIT_SIZE];
    char OffsetDataType[16];
    int OffsetConvType;
    double OffsetFactor;
    double OffsetOffset;
    char OffsetConvString[2048];

    int WaveForm;    // 0..3
} CONFIG_ONE_ANALOG_CHANNEL_DLG_STRUCT;

typedef struct {
    int FixedOrVariable;
    double FixedValue;
    char SignalName[BBVARI_NAME_SIZE];
    char Unit[BBVARI_UNIT_SIZE];
    char DataType[16];
    // Umrechnung
    CONFIG_CONV_DLG_STRUCT Conv;
} FRQ_PWM_PHASE_CONFIG;

typedef struct {
    int OnOff;
    FRQ_PWM_PHASE_CONFIG Frq;
    FRQ_PWM_PHASE_CONFIG Pwm;
    FRQ_PWM_PHASE_CONFIG Phase;
    int PinNrA;
    int PinNrB;
    int PinBitMaskA;     // unntig
    int PinBitMaskB;     // unntig
    int CounterOnOff;
    char CounterName[BBVARI_NAME_SIZE];
} CONFIG_ONE_CHANNEL_DLG_STRUCT;


int GetMaxNumberOfDsps(void)
{
    return __MAX_DSPS;
}

static char *GetNextCommaSeperatedWord (char *Line, char *Word, int Size)
{
    int i = 0;
    char *s = Line;
    char *d = Word;

    while (isascii(*s) && isspace (*s)) s++;
    while (1) {
        if ((*s == ',') || (*s == 0)) {
            if (d != Word) while (isspace (*(d-1))) d--;
            *d = 0;
            while (isascii(*s) && isspace (*s)) s++;
            if (*s != 0) s++;
            return s;
        } else {
            if (i < (Size - 1)) {
                *d++ = *s++;
                i++;
            } else {
                s++;
                *d = 0;
            }
        }
    }
}


static int LoadOnePeriode (int DspNr, int i, int *OnePeriode)
{
    int x, y;
    char section[64];
    char entry[32];
    char line[512];
    char help[32];
    char *p;

    PrintFormatToString (section, sizeof(section), "DSP56K_DA_Channel_OnePeriode_%i_%i", DspNr, i);
    for (x = 0; x < 1024/16; x++) {
        PrintFormatToString (entry, sizeof(entry), "x%i", x);
        if (DBGetPrivateProfileString (section, entry, "", line, sizeof (line), INIFILE) > 0) {
            p = line;
            for (y = 0; y < 16; y++) {
                p = GetNextCommaSeperatedWord (p, help, 32);
                OnePeriode[x*16+y] = atol (help);
            }
        } else break;
    }
    return 0;
}

#if 0
static int SendOnePeriodeToDSP (int Nr, int Card)
{
    int *OnePeriode;

    OnePeriode = my_calloc (1024, sizeof (int));
    if (OnePeriode == NULL) return -1;
    LoadOnePeriode (Card, Nr, OnePeriode);
    write_message (get_pid_by_name ("DSP56301"),
                   0xDAD0 + Card,
                   1024 * sizeof (int), (char*)OnePeriode);
    return 0;
}
#endif

// InOut = 0 -> Output (DA)
// InOut = 1 -> Input (AD)
static void GetSectionName (int Card, int InOut, char *Section)
{
    if (InOut) {
        if (Card) PrintFormatToString (Section, sizeof(Section), "DSP56K_AD_Channel_Config Card(%i)", Card);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_AD_Channel_Config");
    } else {
        if (Card) PrintFormatToString (Section, sizeof(Section), "DSP56K_DA_Channel_Config Card(%i)", Card);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DA_Channel_Config");
    }
}

static void CheckMinMaxVoltage (double *pMin, double *pMax)
{
    if (*pMin < 0.0) *pMin = 0.0;
    if (*pMax > 10.0) *pMax = 10.0;
    if (*pMin > *pMax) *pMin = *pMax;
}

static void CheckFgAndD (double *pFg, double *pD)
{
    if ((*pFg != 0.0) && (*pFg < 100.0)) *pFg = 100.0;
    if (*pFg > 1500.0) *pFg = 1500.0;
    if (*pD < 0.7) *pD = 0.7;
    if (*pD > 2.0) *pD = 2.0;
}

static int ConvertDataTypeStringToInt (char *par_DataTypeString)
{
    return GetDataTypebyName (par_DataTypeString);
}

const char *ConvertDataTypeIntToString (int par_DataTypeInt)
{
    const char *pDataType;
    switch (par_DataTypeInt) {
    case BB_BYTE:
        pDataType = "BYTE";
        break;
    case BB_UBYTE:
        pDataType = "UBYTE";
        break;
    case BB_WORD:
        pDataType = "WORD";
        break;
    case BB_UWORD:
        pDataType = "UWORD";
        break;
    case BB_DWORD:
        pDataType = "DWORD";
        break;
    case BB_UDWORD:
        pDataType = "UDWORD";
        break;
    case BB_FLOAT:
        pDataType = "FLOAT";
        break;
    case BB_DOUBLE:
        pDataType = "DOUBLE";
        break;
    default:
        pDataType = "UNKNOWN_DOUBLE";
        break;
    }
    return pDataType;
}

static void SetDefaultValues (CONFIG_ONE_ANALOG_CHANNEL_DLG_STRUCT *pCoc)
{
    pCoc->OnOff = 0;
    pCoc->SignalName[0] = 0;
    pCoc->Unit[0] = 0;
    pCoc->DataType[0] = 0;
    pCoc->SignalType[0] = 0;
    pCoc->Conv.Factor = 1.0;
    pCoc->Conv.Offset = 0.0;
    pCoc->AmplFixedValue = 1.0;
    pCoc->Conv.OutMin = 0.0;
    pCoc->Conv.OutMax = 10.0;
    pCoc->Fg = 500.0;
    pCoc->D = 1.4;
    CheckMinMaxVoltage (&pCoc->Conv.OutMin, &pCoc->Conv.OutMax);
    CheckFgAndD (&pCoc->Fg, &pCoc->D);
    pCoc->Conv.ConvType = 0;
    pCoc->Conv.ConvString[0] = 0;
    pCoc->WaveForm = 0;
    pCoc->AmplType = 0;
    pCoc->AmplSignalName[0] = 0;
    pCoc->AmplUnit[0] = 0;
    pCoc->AmplDataType[0] = 0;
    pCoc->AmplConvType = 0;
    pCoc->AmplFactor = 1.0;
    pCoc->AmplOffset = 0.0;
    pCoc->AmplConvString[0] = 0;
    pCoc->OffsetType = 0;
    pCoc->OffsetFixedValue = 0.0;
    pCoc->OffsetSignalName[0] = 0;
    pCoc->OffsetUnit[0] = 0;
    pCoc->OffsetDataType[0] = 0;
    pCoc->OffsetConvType = 0;
    pCoc->OffsetFactor = 1.0;
    pCoc->OffsetOffset = 0.0;
    pCoc->OffsetConvString[0] = 0;
    // gltiger Datentyp-String
    STRING_COPY_TO_ARRAY (pCoc->DataType, ConvertDataTypeIntToString (ConvertDataTypeStringToInt (pCoc->DataType)));
    STRING_COPY_TO_ARRAY (pCoc->AmplDataType, ConvertDataTypeIntToString (ConvertDataTypeStringToInt (pCoc->AmplDataType)));
    STRING_COPY_TO_ARRAY (pCoc->OffsetDataType, ConvertDataTypeIntToString (ConvertDataTypeStringToInt (pCoc->OffsetDataType)));
}

static int GetActiveChannelCount (int Card, int InOut)
{
    char Section[INI_MAX_SECTION_LENGTH];
    GetSectionName (Card, InOut, Section);
    // Anzahl der aktiven Kanle zurckgeben
    return DBGetPrivateProfileInt (Section, "ActiveChannels", 16, INIFILE);
}


static int ReadOneChannelFromIni (CONFIG_ONE_ANALOG_CHANNEL_DLG_STRUCT *pCoc, int Card, int Channel, int InOut)
{
    char Section[INI_MAX_SECTION_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[INI_MAX_LINE_LENGTH];
    char Help[32];
    char *p;

    MEMSET (pCoc, 0, sizeof (CONFIG_ONE_ANALOG_CHANNEL_DLG_STRUCT));
    pCoc->Card = Card;
    pCoc->InOut = InOut;
    GetSectionName (Card, InOut, Section);

    PrintFormatToString (entry, sizeof(entry), "Channel_%i", Channel);
    if (DBGetPrivateProfileString (Section, Entry, "", Line, sizeof (Line), INIFILE) > 0) {
        p = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
        pCoc->OnOff = atol (Help);
        p = GetNextCommaSeperatedWord (p, pCoc->SignalName, sizeof (pCoc->SignalName));
        p = GetNextCommaSeperatedWord (p, pCoc->Unit, sizeof (pCoc->Unit));
        p = GetNextCommaSeperatedWord (p, pCoc->DataType, sizeof (pCoc->DataType));
        p = GetNextCommaSeperatedWord (p, pCoc->SignalType, sizeof (pCoc->SignalType));
        // Faktor
        p = GetNextCommaSeperatedWord (p, Help, 32);
        pCoc->Conv.Factor = atof (Help);
        // Offset
        p = GetNextCommaSeperatedWord (p, Help, 32);
        pCoc->Conv.Offset = atof (Help);
        // Amplitude Fixed
        p = GetNextCommaSeperatedWord (p, Help, 32);
        pCoc->AmplFixedValue = atof (Help);
        // Signal min  (ignored)
        p = GetNextCommaSeperatedWord (p, Help, 32);
        // Out min
        p = GetNextCommaSeperatedWord (p, Help, 32);
        pCoc->Conv.OutMin = atof (Help);
        // Signal max (ignored)
        p = GetNextCommaSeperatedWord (p, Help, 32);
        // Out max
        p = GetNextCommaSeperatedWord (p, Help, 32);
        pCoc->Conv.OutMax = atof (Help);
        // NC (ignored)
        p = GetNextCommaSeperatedWord (p, Help, 32);
        // Fg
        p = GetNextCommaSeperatedWord (p, Help, 32);
        pCoc->Fg = atof (Help);
        // D
        GetNextCommaSeperatedWord (p, Help, 32);
        pCoc->D = atof (Help);

        CheckMinMaxVoltage (&pCoc->Conv.OutMin, &pCoc->Conv.OutMax);
        CheckFgAndD (&pCoc->Fg, &pCoc->D);

        // Entry_?_conv = UmrechnungTyp, Umrechnugnsstring
        PrintFormatToString (entry, sizeof(entry), "Channel_%i_conv", Channel);
        if (DBGetPrivateProfileString (Section, Entry, "", Line, sizeof (Line), INIFILE) > 0) {
            // UmrechnungTyp 0-> Faktor/Offset, 1 -> Curve, 2 -> Equation
            p = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
            pCoc->Conv.ConvType = atol (Help);
            p = GetNextCommaSeperatedWord (p, pCoc->Conv.ConvString, sizeof (pCoc->Conv.ConvString));
        } else pCoc->Conv.ConvType = 0;
        // Entry_?_dds = Waveform, AmplFixedOrVari,AmplSignalName,AmplUnit,AmplDatentyp,AmplUmrechnungTyp,AmplUmrechnugnsstring
        PrintFormatToString (entry, sizeof(entry), "Channel_%i_dds", Channel);
        if (DBGetPrivateProfileString (Section, Entry, "", Line, sizeof (Line), INIFILE) > 0) {
            // WaveForm
            p = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
            pCoc->WaveForm = atol (Help);
            if (pCoc->WaveForm < 0) pCoc->WaveForm = 0;
            if (pCoc->WaveForm > 3) pCoc->WaveForm = 3;
            // Amplitude Fixed Or Variable
            p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
            pCoc->AmplType = atol (Help);
            // Amplitude SignalName
            p = GetNextCommaSeperatedWord (p, pCoc->AmplSignalName, sizeof (pCoc->AmplSignalName));
            p = GetNextCommaSeperatedWord (p, pCoc->AmplUnit, sizeof (pCoc->AmplUnit));
            p = GetNextCommaSeperatedWord (p, pCoc->AmplDataType, sizeof (pCoc->AmplDataType));
            // Amplitude UmrechnungTyp
            p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
            pCoc->AmplConvType = atol (Help);
            p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
            pCoc->AmplFactor = atof (Help);
            p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
            pCoc->AmplOffset = atof (Help);
            // Amplitude Umrechnugnsstring
            p = GetNextCommaSeperatedWord (p, pCoc->AmplConvString, sizeof (pCoc->AmplConvString));
        }
        // Entry_?_dds = OffsetFixedOrVari,OffsetSignalName,OffsetUnit,OffsetDatentyp,OffsetUmrechnungTyp,OffsetUmrechnugnsstring
        PrintFormatToString (entry, sizeof(entry), "Channel_%i_dds2", Channel);
        if (DBGetPrivateProfileString (Section, Entry, "", Line, sizeof (Line), INIFILE) > 0) {
            // Offset Fixed Or Variable
            p = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
            pCoc->OffsetType = atol (Help);
            // Offset Fix Value
            p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
            pCoc->OffsetFixedValue = atof (Help);
            // Offset SignalName
            p = GetNextCommaSeperatedWord (p, pCoc->OffsetSignalName, sizeof (pCoc->OffsetSignalName));
            p = GetNextCommaSeperatedWord (p, pCoc->OffsetUnit, sizeof (pCoc->OffsetUnit));
            p = GetNextCommaSeperatedWord (p, pCoc->OffsetDataType, sizeof (pCoc->OffsetDataType));
            // Offset UmrechnungTyp
            p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
            pCoc->OffsetConvType = atol (Help);
            p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
            pCoc->OffsetFactor = atof (Help);
            p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
            pCoc->OffsetOffset = atof (Help);
            // Offset Umrechnugnsstring
            p = GetNextCommaSeperatedWord (p, pCoc->OffsetConvString, sizeof (pCoc->OffsetConvString));
        }
    } else {
        SetDefaultValues (pCoc);
    }
    // gltiger Datentyp-String
    STRING_COPY_TO_ARRAY (pCoc->DataType, ConvertDataTypeIntToString (ConvertDataTypeStringToInt (pCoc->DataType)));
    STRING_COPY_TO_ARRAY (pCoc->AmplDataType, ConvertDataTypeIntToString (ConvertDataTypeStringToInt (pCoc->AmplDataType)));
    STRING_COPY_TO_ARRAY (pCoc->OffsetDataType, ConvertDataTypeIntToString (ConvertDataTypeStringToInt (pCoc->OffsetDataType)));

    // Anzahl der aktiven Kanle zurckgeben
    return 0;
}


static int AddDataToDaCfg (DSP_DA_CONFIG **pCfg, char *Data, int Size)
{
    int NewSize;
    int Ret;

    // Gre der Struktur anpassen?
    if ((int)((*pCfg)->DataPos + Size) >= (*pCfg)->DataSize) {
        NewSize = (*pCfg)->DataSize + 1024;
        (*pCfg) = (DSP_DA_CONFIG*)my_realloc ((*pCfg), (size_t)NewSize + sizeof(DSP_DA_CONFIG));
        (*pCfg)->DataSize = NewSize;
    }
    MEMCPY (&(*pCfg)->Data[(*pCfg)->DataPos], Data, (size_t)Size);
    Ret = (*pCfg)->DataPos;
    (*pCfg)->DataPos += Size;
    return Ret;
}

static int AddEquToDaCfg (DSP_DA_CONFIG **pCfg, char *EquStr, struct EXEC_STACK_ELEM *ExecStack, int Size)
{
    int NewSize;
    int Ret;

    // Gre der Struktur anpassen?
    if ((int)((*pCfg)->DataPos + Size) >= (*pCfg)->DataSize) {
        NewSize = (*pCfg)->DataSize + 1024;
        (*pCfg) = (DSP_DA_CONFIG*)my_realloc ((*pCfg), (size_t)NewSize + sizeof(DSP_DA_CONFIG));
        (*pCfg)->DataSize = NewSize;
    }
    MEMCPY (&(*pCfg)->Data[(*pCfg)->DataPos], ExecStack, (size_t)Size);
    Ret = (*pCfg)->DataPos;
    (*pCfg)->DataPos += Size;
    attach_exec_stack ((struct EXEC_STACK_ELEM *)&((*pCfg)->Data[Ret]));
    RegisterEquation (0, EquStr, (struct EXEC_STACK_ELEM *)&((*pCfg)->Data[Ret]), "", EQU_TYPE_ANALOG);

    return Ret;
}

DSP_DA_CONFIG *ReadAnalogOutConfigForOneDacDSP (int Card)
{
    DSP_DA_CONFIG *pCfg;
    int Channel;
    int Period;
    int Size;
    int Index;
    CONFIG_ONE_ANALOG_CHANNEL_DLG_STRUCT Cocds;
    int UseCANData;   // nur als Dummy
    struct EXEC_STACK_ELEM *ExecStack;


    pCfg = (DSP_DA_CONFIG*)my_calloc (1, sizeof (DSP_DA_CONFIG));
    if (pCfg == NULL) return NULL;
    pCfg->Status = 1;   // Gltige Config.
    pCfg->CardType = 0; // Todo
    for (Channel = 0; Channel < 16; Channel++) {
        ReadOneChannelFromIni (&Cocds, Card, Channel, 0);
        pCfg->Output[Channel].Flag = Cocds.OnOff;
        if (pCfg->Output[Channel].Flag) {
            pCfg->Output[Channel].Vid = add_bbvari (Cocds.SignalName,
                                                    ConvertDataTypeStringToInt (Cocds.DataType),
                                                    Cocds.Unit);
            if (!strcmp (Cocds.SignalType, "FRQ")) pCfg->Output[Channel].SignalType = 1;
            else pCfg->Output[Channel].SignalType = 0;
            pCfg->Output[Channel].ConvType = Cocds.Conv.ConvType;
            switch (pCfg->Output[Channel].ConvType) {
            case 0:
            default:
                pCfg->Output[Channel].Conv.FacOff.Factor = Cocds.Conv.Factor;
                pCfg->Output[Channel].Conv.FacOff.Offset = Cocds.Conv.Offset;
                break;
            case 1:  // Curve
                if (TranslateString2Curve (Cocds.Conv.ConvString, sizeof (Cocds.Conv.ConvString), &Size)) {
                    pCfg->Output[Channel].Flag = 0;   // Kanal ausschalten
                } else {
                     // Achtung Cfg kann sich ndern!!
                     Index = AddDataToDaCfg (&pCfg, Cocds.Conv.ConvString, Size);
                     pCfg->Output[Channel].Conv.CurveIdx = Index;
                }

                break;
            case 2:   // Formula
                if ((ExecStack = TranslateString2ByteCode (Cocds.Conv.ConvString, sizeof (Cocds.Conv.ConvString), &Size, &UseCANData)) == NULL) {  // Fehler
                    pCfg->Output[Channel].Flag = 0;   // Kanal ausschalten
                } else {
                     // Achtung Cfg kann sich ndern!!
                     Index = AddEquToDaCfg (&pCfg, Cocds.Conv.ConvString, ExecStack, Size);
                     remove_exec_stack (ExecStack);
                     pCfg->Output[Channel].Conv.EquIdx = Index;
                }
                break;
            }
            pCfg->Output[Channel].OutMin = Cocds.Conv.OutMin;
            pCfg->Output[Channel].OutMax = Cocds.Conv.OutMax;

            // Amplitude
            pCfg->Output[Channel].AmplType = Cocds.AmplType;   // 0 -> Fixed 1 -> Variable
            switch (pCfg->Output[Channel].AmplType) {
            case 0:      // Fixed Amplitude   (0...1)
            default:
                pCfg->Output[Channel].Ampl.Fix = Cocds.AmplFixedValue;
                break;
            case 1:
                pCfg->Output[Channel].Ampl.Variable.Vid = add_bbvari (Cocds.AmplSignalName,
                                                                      ConvertDataTypeStringToInt (Cocds.AmplDataType),
                                                                      Cocds.AmplUnit);
                pCfg->Output[Channel].Ampl.Variable.ConvType = Cocds.AmplConvType;
                switch (pCfg->Output[Channel].Ampl.Variable.ConvType) {
                case 0:
                default:
                    pCfg->Output[Channel].Ampl.Variable.Conv.FacOff.Factor = Cocds.Conv.Factor;
                    pCfg->Output[Channel].Ampl.Variable.Conv.FacOff.Offset = Cocds.Conv.Offset;
                    break;
                case 1:
                    if (TranslateString2Curve (Cocds.AmplConvString, sizeof (Cocds.AmplConvString), &Size)) {
                        pCfg->Output[Channel].Flag = 0;   // Kanal ausschalten
                    } else {
                        // Achtung Cfg kann sich ndern!!
                        Index = AddDataToDaCfg (&pCfg, Cocds.AmplConvString, Size);
                        pCfg->Output[Channel].Ampl.Variable.Conv.CurveIdx = Index;
                    }

                    break;
                case 2:
                    if ((ExecStack = TranslateString2ByteCode (Cocds.AmplConvString, sizeof (Cocds.AmplConvString), &Size, &UseCANData)) == NULL) {  // Fehler
                        pCfg->Output[Channel].Flag = 0;   // Kanal ausschalten
                    } else {
                        // Achtung Cfg kann sich ndern!!
                        Index = AddEquToDaCfg (&pCfg, Cocds.AmplConvString, ExecStack, Size);
                        remove_exec_stack (ExecStack);
                        pCfg->Output[Channel].Ampl.Variable.Conv.EquIdx = Index;
                    }
                    break;
                }
                break;
            }

            // Spannuangs-Offset
            pCfg->Output[Channel].OffsetType = Cocds.OffsetType;   // 0 -> Fixed 1 -> Variable
            switch (pCfg->Output[Channel].OffsetType) {
            case 0:      // Fixed Offset   (0...10V)
            default:
                pCfg->Output[Channel].Offset.Fix = Cocds.OffsetFixedValue;
                break;
            case 1:
                pCfg->Output[Channel].Offset.Variable.Vid = add_bbvari (Cocds.OffsetSignalName,
                                                                      ConvertDataTypeStringToInt (Cocds.OffsetDataType),
                                                                      Cocds.OffsetUnit);
                pCfg->Output[Channel].Offset.Variable.ConvType = Cocds.OffsetConvType;
                switch (pCfg->Output[Channel].Offset.Variable.ConvType) {
                case 0:
                default:
                    pCfg->Output[Channel].Offset.Variable.Conv.FacOff.Factor = Cocds.Conv.Factor;
                    pCfg->Output[Channel].Offset.Variable.Conv.FacOff.Offset = Cocds.Conv.Offset;
                    break;
                case 1:
                    if (TranslateString2Curve (Cocds.OffsetConvString, sizeof (Cocds.OffsetConvString), &Size)) {
                        pCfg->Output[Channel].Flag = 0;   // Kanal ausschalten
                    } else {
                        // Achtung Cfg kann sich ndern!!
                        Index = AddDataToDaCfg (&pCfg, Cocds.OffsetConvString, Size);
                        pCfg->Output[Channel].Offset.Variable.Conv.CurveIdx = Index;
                    }

                    break;
                case 2:
                    if ((ExecStack = TranslateString2ByteCode (Cocds.OffsetConvString, sizeof (Cocds.OffsetConvString), &Size, &UseCANData)) == NULL) {  // Fehler
                        pCfg->Output[Channel].Flag = 0;   // Kanal ausschalten
                    } else {
                        // Achtung Cfg kann sich ndern!!
                        Index = AddEquToDaCfg (&pCfg, Cocds.OffsetConvString, ExecStack, Size);
                        remove_exec_stack (ExecStack);
                        pCfg->Output[Channel].Offset.Variable.Conv.EquIdx = Index;
                    }
                    break;
                }
                break;
            }

            pCfg->Output[Channel].OutMin = Cocds.Conv.OutMin;
            pCfg->Output[Channel].OutMax = Cocds.Conv.OutMax;
            pCfg->Output[Channel].WaveFormAddr = 0x00800000 + (Cocds.WaveForm << 10);
        }
    }

    for (Period = 0; Period < 4; Period++) {
        LoadOnePeriode (Card, Period, pCfg->Dds4Periods.OnePeriod[Period]);
    }
    return pCfg;
}

void DeleteAnalogOutConfigForOneDacDSP (DSP_DA_CONFIG *pCfg)
{
    int Channel;
    char *Help;

    for (Channel = 0; Channel < 16; Channel++) {
        if (pCfg->Output[Channel].Flag) {
            if (pCfg->Output[Channel].Vid > 0) remove_bbvari (pCfg->Output[Channel].Vid);
            if (pCfg->Output[Channel].ConvType == 2) {   // Equation
                Help = &pCfg->Data[pCfg->Output[Channel].Conv.EquIdx];
                detach_exec_stack ((struct EXEC_STACK_ELEM*)Help);
            }
            if (pCfg->Output[Channel].AmplType == 1) {
                if (pCfg->Output[Channel].Ampl.Variable.Vid > 0) remove_bbvari (pCfg->Output[Channel].Ampl.Variable.Vid);
                if (pCfg->Output[Channel].Ampl.Variable.ConvType == 2) {  // Equation
                    Help = &pCfg->Data[pCfg->Output[Channel].Ampl.Variable.Conv.EquIdx];
                    detach_exec_stack ((struct EXEC_STACK_ELEM*)Help);
                }
            }
            if (pCfg->Output[Channel].OffsetType == 1) {
                if (pCfg->Output[Channel].Offset.Variable.Vid > 0) remove_bbvari (pCfg->Output[Channel].Offset.Variable.Vid);
                if (pCfg->Output[Channel].Offset.Variable.ConvType == 2) {  // Equation
                    Help = &pCfg->Data[pCfg->Output[Channel].Offset.Variable.Conv.EquIdx];
                    detach_exec_stack ((struct EXEC_STACK_ELEM*)Help);
                }
            }
        }
    }
}


int AddDataToAdCfg (DSP_AD_CONFIG **pCfg, char *Data, int Size)
{
    int NewSize;
    int Ret;

    // Gre der Struktur anpassen?
    if ((int)((*pCfg)->DataPos + Size) >= (*pCfg)->DataSize) {
        NewSize = (*pCfg)->DataSize + 1024;
        (*pCfg) = (DSP_AD_CONFIG*)my_realloc ((*pCfg), (size_t)NewSize + sizeof(DSP_AD_CONFIG));
        (*pCfg)->DataSize = NewSize;
    }
    MEMCPY (&(*pCfg)->Data[(*pCfg)->DataPos], Data, (size_t)Size);
    Ret = (*pCfg)->DataPos;
    (*pCfg)->DataPos += Size;

    return Ret;
}

int AddEquToAdCfg (DSP_AD_CONFIG **pCfg, char *EquStr, struct EXEC_STACK_ELEM *ExecStack, int Size)
{
    int NewSize;
    int Ret;

    // Gre der Struktur anpassen?
    if ((int)((*pCfg)->DataPos + Size) >= (*pCfg)->DataSize) {
        NewSize = (*pCfg)->DataSize + 1024;
        (*pCfg) = (DSP_AD_CONFIG*)my_realloc ((*pCfg), (size_t)NewSize + sizeof(DSP_AD_CONFIG));
        (*pCfg)->DataSize = NewSize;
    }
    MEMCPY (&(*pCfg)->Data[(*pCfg)->DataPos], ExecStack, (size_t)Size);
    Ret = (*pCfg)->DataPos;
    (*pCfg)->DataPos += Size;
    attach_exec_stack ((struct EXEC_STACK_ELEM *)&((*pCfg)->Data[Ret]));
    RegisterEquation (0, EquStr, (struct EXEC_STACK_ELEM *)&((*pCfg)->Data[Ret]), "", EQU_TYPE_ANALOG);

    return Ret;
}



DSP_AD_CONFIG *ReadAnalogInConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg, int Card)
{
    int Channel;
    int Size;
    int Index;
    CONFIG_ONE_ANALOG_CHANNEL_DLG_STRUCT Cocds;
    int UseCANData;   // nur als Dummy
    struct EXEC_STACK_ELEM *ExecStack;

    if (pCfg == NULL) {
        pCfg = (DSP_AD_CONFIG*)my_calloc (1, sizeof (DSP_AD_CONFIG));
        if (pCfg == NULL) return NULL;
        pCfg->Status = 1;   // Gltige Config.
        pCfg->CardType = 0; // Todo
    }
    pCfg->ActiveChannels = GetActiveChannelCount (Card, 1);

    // erst mal die ADC Kanle einlesen
    for (Channel = 0; Channel < 16; Channel++) {
        ReadOneChannelFromIni (&Cocds, Card, Channel, 1);
        pCfg->Input[Channel].Flag = Cocds.OnOff;
        if (pCfg->Input[Channel].Flag) {
            pCfg->Input[Channel].Vid = add_bbvari (Cocds.SignalName,
                                                   ConvertDataTypeStringToInt (Cocds.DataType),
                                                   Cocds.Unit);
            pCfg->Input[Channel].Fg = Cocds.Fg;
            pCfg->Input[Channel].D = Cocds.D;
            pCfg->Input[Channel].ConvType = Cocds.Conv.ConvType;
            switch (pCfg->Input[Channel].ConvType) {
            case 0:
            default:
                pCfg->Input[Channel].Conv.FacOff.Factor = Cocds.Conv.Factor;
                pCfg->Input[Channel].Conv.FacOff.Offset = Cocds.Conv.Offset;
                break;
            case 1:
                if (TranslateString2Curve (Cocds.Conv.ConvString, sizeof (Cocds.Conv.ConvString), &Size)) {
                    pCfg->Input[Channel].Flag = 0;   // Kanal ausschalten
                } else {
                    // Achtung Cfg kann sich ndern!!
                    Index = AddDataToAdCfg (&pCfg, Cocds.Conv.ConvString, Size);
                    pCfg->Input[Channel].Conv.CurveIdx = Index;
                }

                break;
            case 2:
                if ((ExecStack = TranslateString2ByteCode (Cocds.Conv.ConvString, sizeof (Cocds.Conv.ConvString), &Size, &UseCANData)) == NULL) {  // Fehler
                    pCfg->Input[Channel].Flag = 0;   // Kanal ausschalten
                } else {
                     // Achtung Cfg kann sich ndern!!
                     Index = AddDataToAdCfg (&pCfg, Cocds.Conv.ConvString, Size);
                     remove_exec_stack (ExecStack);
                     pCfg->Input[Channel].Conv.EquIdx = Index;

                }
                break;
            }
        }
    }
    return pCfg;
}

void DeleteAnalogInConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg)
{
    int Channel;
    char *Help;

    for (Channel = 0; Channel < 16; Channel++) {
        if (pCfg->Input[Channel].Flag) {
            if (pCfg->Input[Channel].Vid > 0) remove_bbvari (pCfg->Input[Channel].Vid);
            if (pCfg->Input[Channel].ConvType == 2) {   // Equation
                Help = &pCfg->Data[pCfg->Input[Channel].Conv.EquIdx];
                detach_exec_stack ((struct EXEC_STACK_ELEM*)Help);
            }
        }
    }
    // my_free (pCfg);
}

int SaveAnalogSignalConfiguration (char *Filename)
{
    char Section[256];
    FILE *fh;
    int x;

    // File erstellen oder vorhandes File ueberschreiben
    if ((fh = open_file (Filename, "wt")) == NULL) return -1;
    close_file (fh);
    RegisterINIFileGetFullPath (Filename, MAX_PATH);

    // Frequenz/PWM-Ausgnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }

    // Digitale-Ausgnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_DigOut_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DigOut_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }

    // Digitale-Eingnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_DigIn_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DigIn_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }

    // Analoge-Ausgnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_DA_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DA_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }

    // Analoge-Eingnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_AD_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_AD_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }

    // Frequenz/PWM-Eingnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Input_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Input_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }

    // Digitale-Ausgnge der Flexcard
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "Flexcard_Dig_IO_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "Flexcard_Dig_IO_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }

    // Digitale-Eingnge der Flexcard
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "Flexcard_FrqPwm_Input_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "Flexcard_FrqPwm_Input_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }

    // Digitale-Eingnge der Flexcard
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "Flexcard_FrqPwm_Input_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "Flexcard_FrqPwm_Input_Channel_Config");
        if (LookIfSectionExist (Section, INIFILE)) {
            copy_ini_section (Filename, INIFILE, Section, Section);
        } else break;
    }


    write_db2ini (Filename, Filename, 2);
    UnRegisterINIFile (Filename);
    return 0;
}

int LoadAnalogSignalConfiguration (char *Filename)
{
    char Section[256];
    int x;

    RegisterINIFileGetFullPath (Filename, MAX_PATH);
    if (read_ini2db (Filename)) {
        UnRegisterINIFile (Filename);
        return -1;
    }

    // LOAD_ANAOG_CFG locken, damit dabei nicht ein laden der Config gleichzeit stattfindet.
    EnterIniDBCriticalSectionUser(__FILE__, __LINE__);
    // Frequenz/PWM-Ausgnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Channel_Config");
        if (LookIfSectionExist (Section, Filename)) {
            copy_ini_section (INIFILE, Filename, Section, Section);
        } else break;
    }

    // Digitale-Ausgnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_DigOut_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DigOut_Channel_Config");
        if (LookIfSectionExist (Section, Filename)) {
            copy_ini_section (INIFILE, Filename, Section, Section);
        } else break;
    }

    // Digitale-Eingnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_DigIn_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DigIn_Channel_Config");
        if (LookIfSectionExist (Section, Filename)) {
            copy_ini_section (INIFILE, Filename, Section, Section);
        } else break;
    }

    // Analoge-Ausgnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_DA_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DA_Channel_Config");
        if (LookIfSectionExist (Section, Filename)) {
            copy_ini_section (INIFILE, Filename, Section, Section);
        } else break;
    }

    // Analoge-Eingnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_AD_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_AD_Channel_Config");
        if (LookIfSectionExist (Section, Filename)) {
            copy_ini_section (INIFILE, Filename, Section, Section);
        } else break;
    }

    // Frequenz/PWM-Eingnge
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Input_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Input_Channel_Config");
        if (LookIfSectionExist (Section, Filename)) {
            copy_ini_section (INIFILE, Filename, Section, Section);
        } else break;
    }

    // Digitale-Ausgnge der Flexcard
    for (x = 0; x < 4; x++) {
        if (x) PrintFormatToString (Section, sizeof(Section), "Flexcard_Dig_IO_Channel_Config Card(%i)", x);
        else PrintFormatToString (Section, sizeof(Section), "Flexcard_Dig_IO_Channel_Config");
        if (LookIfSectionExist (Section, Filename)) {
            copy_ini_section (INIFILE, Filename, Section, Section);
        } else break;
    }

    // Digitale-Eingnge der Flexcard
    for (x = 0; x < 4; x++) {
         if (x) PrintFormatToString (Section, sizeof(Section), "Flexcard_Dig_IO_Channel_Config Card(%i)", x);
         else PrintFormatToString (Section, sizeof(Section), "Flexcard_Dig_IO_Channel_Config");
         if (LookIfSectionExist (Section, Filename)) {
             copy_ini_section (INIFILE, Filename, Section, Section);
         } else break;
     }

     // Digitale-Eingnge der Flexcard
     for (x = 0; x < 4; x++) {
         if (x) PrintFormatToString (Section, sizeof(Section), "Flexcard_FrqPwm_Input_Channel_Config Card(%i)", x);
         else PrintFormatToString (Section, sizeof(Section), "Flexcard_FrqPwm_Input_Channel_Config");
         if (LookIfSectionExist (Section, Filename)) {
             copy_ini_section (INIFILE, Filename, Section, Section);
         } else break;
     }
     LeaveIniDBCriticalSectionUser();


    // DSPs neu laden !!!!
    DSP56KLoadAllConfigs ();
    // und Flexcard digital IO
    FlexcardDigIoConfigHasChangend ();
    UnRegisterINIFile (Filename);
    return 0;
}

static int ReadFrqOrPwmOrPhase (DSP_AD_CONFIG **pCfg, FRQ_PWM_PHASE_CONFIG *DlgStruct, DSP_DIG_FRQ_OR_PWM_OR_PHASE *CfgStruct)
{
    int Size;
    int Ret = 0;
    int UseCANData;   // nur als Dummy
    struct EXEC_STACK_ELEM *ExecStack;

    CfgStruct->FixedOrVariable = DlgStruct->FixedOrVariable;
    CfgStruct->FixedValue = DlgStruct->FixedValue;
    if (CfgStruct->FixedOrVariable) {
        CfgStruct->Vid = add_bbvari (DlgStruct->SignalName,
                                    ConvertDataTypeStringToInt (DlgStruct->DataType),
                                    DlgStruct->Unit);
        CfgStruct->ConvType = DlgStruct->Conv.ConvType;
        switch (CfgStruct->ConvType) {
        case 0:
        default:
            CfgStruct->Conv.FacOff.Factor = DlgStruct->Conv.Factor;
            CfgStruct->Conv.FacOff.Offset = DlgStruct->Conv.Offset;
            break;
        case 1:
            if (TranslateString2Curve (DlgStruct->Conv.ConvString, sizeof (DlgStruct->Conv.ConvString), &Size)) {
                Ret |= 1;   // Kanal ausschalten
            } else {
                int Offset;
                int Index;
                // Achtung Cfg kann sich ndern!!
                Offset = (int)((char*)CfgStruct - (char*)(*pCfg));
                Index = AddDataToAdCfg (pCfg, DlgStruct->Conv.ConvString, Size);
                CfgStruct = (DSP_DIG_FRQ_OR_PWM_OR_PHASE*)(void*)((char*)(*pCfg) + Offset);
                CfgStruct->Conv.CurveIdx = Index;
            }

            break;
        case 2:
            if ((ExecStack = TranslateString2ByteCode (DlgStruct->Conv.ConvString, sizeof (DlgStruct->Conv.ConvString), &Size, &UseCANData)) == NULL)  {  // Fehler
                Ret |= 1;   // Kanal ausschalten
            } else {
                int Offset;
                int Index;
                // Achtung Cfg kann sich ndern!!
                Offset = (int)((char*)CfgStruct - (char*)(*pCfg));
                Index = AddEquToAdCfg (pCfg, DlgStruct->Conv.ConvString, ExecStack, Size);
                remove_exec_stack (ExecStack);
                CfgStruct = (DSP_DIG_FRQ_OR_PWM_OR_PHASE*)(void*)((char*)(*pCfg) + Offset);
                CfgStruct->Conv.EquIdx = Index;
            }
            break;
        }
    } else {
        CfgStruct->Vid = 0;
    }
    return Ret;
}

static char *ReadFrqPhaseOrPwm (char *p, FRQ_PWM_PHASE_CONFIG *pFppc, char *Section, char *Entry, char *Postfix)
{
    char ConvEntry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[INI_MAX_LINE_LENGTH];
    char Help[32];
    char *pp;

    p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
    pFppc->FixedOrVariable = atol (Help);
    p = GetNextCommaSeperatedWord (p, pFppc->SignalName, sizeof (pFppc->SignalName));
    if (!pFppc->FixedOrVariable) {
        pFppc->FixedValue = atof (pFppc->SignalName);
        pFppc->SignalName[0] = 0;
    } else {
        pFppc->FixedValue = 0.0;
    }
    p = GetNextCommaSeperatedWord (p, pFppc->Unit, sizeof (pFppc->Unit));
    p = GetNextCommaSeperatedWord (p, pFppc->DataType, sizeof (pFppc->DataType));
    p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
    pFppc->Conv.Factor = atof (Help);
    p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
    pFppc->Conv.Offset = atof (Help);
    // Signal min und max  (ignored)
    p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
    p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
    p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
    pFppc->Conv.OutMin = atof (Help);
    p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
    pFppc->Conv.OutMax = atof (Help);

    // Entry_?_frq_conv = UmrechnungTyp, Umrechnugnsstring
    // Entry_?_pwm_conv = UmrechnungTyp, Umrechnugnsstring
    // Entry_?_phase_conv = UmrechnungTyp, Umrechnugnsstring
    PrintFormatToString (ConvEntry, sizeof(ConvEntry), "%s_%s_conv", Entry, Postfix);
    if (DBGetPrivateProfileString (Section, ConvEntry, "", Line, sizeof (Line), INIFILE) > 0) {
        // UmrechnungTyp 0-> Faktor/Offset, 1 -> Curve, 2 -> Equation
        pp = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
        pFppc->Conv.ConvType = atol (Help);
        pp = GetNextCommaSeperatedWord (pp, pFppc->Conv.ConvString, sizeof (pFppc->Conv.ConvString));
    } else {
        pFppc->Conv.ConvType = 0;
        pFppc->Conv.ConvString[0] = 0;
    }

    return p;
}

static int ReadOneFrqChannelFromIni (CONFIG_ONE_CHANNEL_DLG_STRUCT *pCoc, int Card, int Channel) //, int InOut)
{
    char Section[INI_MAX_SECTION_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[INI_MAX_LINE_LENGTH];
    char Help[32];
    char *p;

    if (Card) PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Channel_Config Card(%i)", Card);
    else PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Channel_Config");

    PrintFormatToString (entry, sizeof(entry), "Channel_%i", Channel);
    if (DBGetPrivateProfileString (Section, Entry, "", Line, sizeof (Line), INIFILE) > 0) {
        p = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
        pCoc->OnOff = atol (Help);
        p = ReadFrqPhaseOrPwm (p, &pCoc->Frq, Section, Entry, "frq");
        p = ReadFrqPhaseOrPwm (p, &pCoc->Pwm, Section, Entry, "pwm");
        p = ReadFrqPhaseOrPwm (p, &pCoc->Phase, Section, Entry, "phase");

        p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
        pCoc->PinNrA = atol (Help);
        if (pCoc->PinNrA == 0) pCoc->PinNrA = (Channel * 2) + 1;   // default Wert (abwrtskop.)
        p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
        pCoc->PinNrB = atol (Help);
        if (pCoc->PinNrB == 0) pCoc->PinNrB = (Channel * 2) + 2;  // default Wert (abwrtskop.)
        //BuildPinBitMasks (pCoc);
        p = GetNextCommaSeperatedWord (p, Help, sizeof (Help));
        pCoc->CounterOnOff = atol (Help);
        p = GetNextCommaSeperatedWord (p, pCoc->CounterName, sizeof (pCoc->CounterName));
    } else {
        // falls neue Zeile PWM = 50% da sonst bei PWM = 0 Verwirrung kein Signal sichtbar
        pCoc->Pwm.FixedValue = 50.0;
    }
    return 0;
}

static void BuildPinBitMasks (DSP_DIG_FRQ_AND_PWM_AND_PHASE *sp)
{
    // -1 Pin wird nicht verwendet
    // 0 default Pin (abwrts kombatibel)
    // 1...16 Pin Nummer
    if (sp->PinNrA <= -1) sp->PinBitMaskA = 0x0000;
    else sp->PinBitMaskA = 1 << (sp->PinNrA - 1);
    if (sp->PinNrB <= -1) sp->PinBitMaskB = 0x0000;
    else sp->PinBitMaskB = 1 << (sp->PinNrB - 1);
}

DSP_AD_CONFIG *ReadFrqPwmPhaseConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg, int Card)
{
    int Channel;
    CONFIG_ONE_CHANNEL_DLG_STRUCT Cocds;

    if (pCfg == NULL) {
        pCfg = (DSP_AD_CONFIG*)my_calloc (1, sizeof (DSP_AD_CONFIG));
        if (pCfg == NULL) return NULL;
        pCfg->Status = 1;   // Gltige Config.
        pCfg->CardType = 0; // Todo
    }

    // dann die Frequenz, PWM, Phase
    for (Channel = 0; Channel < 4; Channel++) {
        ReadOneFrqChannelFromIni (&Cocds, Card, Channel);
        pCfg->DigFrqPwmPhase[Channel].OnOff = Cocds.OnOff;
        if (pCfg->DigFrqPwmPhase[Channel].OnOff) {
            if (ReadFrqOrPwmOrPhase (&pCfg, &Cocds.Frq, &pCfg->DigFrqPwmPhase[Channel].Frq)) {
                pCfg->DigFrqPwmPhase[Channel].OnOff = 0;
            }
            if (ReadFrqOrPwmOrPhase (&pCfg, &Cocds.Pwm, &pCfg->DigFrqPwmPhase[Channel].Pwm)) {
                pCfg->DigFrqPwmPhase[Channel].OnOff = 0;
            }
            if (ReadFrqOrPwmOrPhase (&pCfg, &Cocds.Phase, &pCfg->DigFrqPwmPhase[Channel].Phase)) {
                pCfg->DigFrqPwmPhase[Channel].OnOff = 0;
            }
        }
        pCfg->DigFrqPwmPhase[Channel].PinNrA = Cocds.PinNrA;
        pCfg->DigFrqPwmPhase[Channel].PinNrB = Cocds.PinNrB;

        BuildPinBitMasks (&pCfg->DigFrqPwmPhase[Channel]);
        pCfg->DigFrqPwmPhase[Channel].CounterOnOff = Cocds.CounterOnOff;
        if (pCfg->DigFrqPwmPhase[Channel].CounterOnOff) {
            pCfg->DigFrqPwmPhase[Channel].CounterVid = add_bbvari (Cocds.CounterName,
                                                                   BB_DWORD,
                                                                   "x");
        }
    }
    return pCfg;
}

static int LastCardNr;
static int LastAdcCardNr;
static int LastDacCardNr;
static DSP_DA_CONFIG *pLastDacConfig;
static DSP_AD_CONFIG *pLastAdcConfig;

static __FOUND_DSPS FoundDsps;


int GetNumberOfDaDsps (void)
{
    return FoundDsps.DacDspCount;
}

int GetNumberOfAdDsps (void)
{
    return FoundDsps.AdcDspCount;
}


void DeleteDigInConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg)
{
    int Channel;

    for (Channel = 0; Channel < 8; Channel++) {
        if (pCfg->DigInput[Channel].OnOff) {
            if (pCfg->DigInput[Channel].Vid > 0) remove_bbvari (pCfg->DigInput[Channel].Vid);
        }
    }
}

void DeleteDigOutConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg)
{
    int Channel;

    for (Channel = 0; Channel < 16; Channel++) {
        if (pCfg->DigOutput[Channel].OnOff) {
            if (pCfg->DigOutput[Channel].Vid > 0) remove_bbvari (pCfg->DigOutput[Channel].Vid);
        }
    }
}

void DeleteFrqPwmPhaseConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg)
{
    int Channel;
    char *Help;

    // Dann alle Frequenz, PWM, Phase BB-Variable und Equations lschen
    for (Channel = 0; Channel < 4; Channel++) {
        if (pCfg->DigFrqPwmPhase[Channel].OnOff) {
            if (pCfg->DigFrqPwmPhase[Channel].Frq.FixedOrVariable) {
                if (pCfg->DigFrqPwmPhase[Channel].Frq.Vid > 0) remove_bbvari (pCfg->DigFrqPwmPhase[Channel].Frq.Vid);
                if (pCfg->DigFrqPwmPhase[Channel].Frq.ConvType == 2) {   // Equation
                    Help = &pCfg->Data[pCfg->DigFrqPwmPhase[Channel].Frq.Conv.EquIdx];
                    detach_exec_stack ((struct EXEC_STACK_ELEM*)Help);
                }
            }
            if (pCfg->DigFrqPwmPhase[Channel].Pwm.FixedOrVariable) {
                if (pCfg->DigFrqPwmPhase[Channel].Pwm.Vid > 0) remove_bbvari (pCfg->DigFrqPwmPhase[Channel].Pwm.Vid);
                if (pCfg->DigFrqPwmPhase[Channel].Pwm.ConvType == 2) {   // Equation
                    Help = &pCfg->Data[pCfg->DigFrqPwmPhase[Channel].Pwm.Conv.EquIdx];
                    detach_exec_stack ((struct EXEC_STACK_ELEM*)Help);
                }
            }
            if (pCfg->DigFrqPwmPhase[Channel].Phase.FixedOrVariable) {
                if (pCfg->DigFrqPwmPhase[Channel].Phase.Vid > 0) remove_bbvari (pCfg->DigFrqPwmPhase[Channel].Phase.Vid);
                if (pCfg->DigFrqPwmPhase[Channel].Phase.ConvType == 2) {   // Equation
                    Help = &pCfg->Data[pCfg->DigFrqPwmPhase[Channel].Phase.Conv.EquIdx];
                    detach_exec_stack ((struct EXEC_STACK_ELEM*)Help);
                }
            }
        }
    }
}

void DeleteFrqPwmInputConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg)
{
    int Channel;
    char *Help;

    // Dann alle Frequenz, PWM, Phase BB-Variable und Equations lschen
    for (Channel = 0; Channel < 1; Channel++) {
        if (pCfg->DigFrqPwmInput[Channel].OffFrqPwmFlag) {
            if (pCfg->DigFrqPwmInput[Channel].Signal.Vid > 0) remove_bbvari (pCfg->DigFrqPwmInput[Channel].Signal.Vid);
            if (pCfg->DigFrqPwmInput[Channel].Signal.ConvType == 2) {   // Equation
                Help = &pCfg->Data[pCfg->DigFrqPwmInput[Channel].Signal.Conv.EquIdx];
                detach_exec_stack ((struct EXEC_STACK_ELEM*)Help);
            }
        }
    }
}

typedef struct {
    int Card;
    int InOut;   // 0 -> In, 1 -> Out
    int OnOff;
    char SignalName[BBVARI_NAME_SIZE];
    char Unit[BBVARI_UNIT_SIZE];
    char DataType[16];
    // Umrechnung
    CONFIG_CONV_DLG_STRUCT Conv;
} CONFIG_ONE_CHANNEL_DIG_IN_STRUCT;

static int ReadOneDigChannelFromIni (CONFIG_ONE_CHANNEL_DIG_IN_STRUCT *pCoc, int Card, int Channel, int InOut)
{
    char Section[INI_MAX_SECTION_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[INI_MAX_LINE_LENGTH];
    char Help[32];
    char *p;

    pCoc->Card = Card;
    pCoc->InOut = InOut;
    if (InOut) {
        if (Card) PrintFormatToString (Section, sizeof(Section), "DSP56K_DigIn_Channel_Config Card(%i)", Card);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DigIn_Channel_Config");
    } else {
        if (Card) PrintFormatToString (Section, sizeof(Section), "DSP56K_DigOut_Channel_Config Card(%i)", Card);
        else PrintFormatToString (Section, sizeof(Section), "DSP56K_DigOut_Channel_Config");
    }

    PrintFormatToString (entry, sizeof(entry), "Channel_%i", Channel);
    if (DBGetPrivateProfileString (Section, Entry, "", Line, sizeof (Line), INIFILE) > 0) {
        p = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
        pCoc->OnOff = atol (Help);
        p = GetNextCommaSeperatedWord (p, pCoc->SignalName, sizeof (pCoc->SignalName));
        p = GetNextCommaSeperatedWord (p, pCoc->Unit, sizeof (pCoc->Unit));
        p = GetNextCommaSeperatedWord (p, pCoc->DataType, sizeof (pCoc->DataType));
        // Entry_?_conv = UmrechnungTyp, Umrechnugnsstring
        PrintFormatToString (entry, sizeof(entry), "Channel_%i_conv", Channel);
        if (DBGetPrivateProfileString (Section, Entry, "", Line, sizeof (Line), INIFILE) > 0) {
            // UmrechnungTyp 0-> Faktor/Offset, 1 -> Curve, 2 -> Equation
            p = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
            pCoc->Conv.ConvType = atol (Help);
            p = GetNextCommaSeperatedWord (p, pCoc->Conv.ConvString, sizeof (pCoc->Conv.ConvString));
        }
    } else {
        pCoc->OnOff = 0;
    }
    // gltiger Datentyp-String
    STRING_COPY_TO_ARRAY (pCoc->DataType, ConvertDataTypeIntToString (ConvertDataTypeStringToInt (pCoc->DataType)));

    return 0;
}

DSP_AD_CONFIG *ReadDigInConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg, int Card)
{
    int Channel;
    CONFIG_ONE_CHANNEL_DIG_IN_STRUCT Cocds;

    if (pCfg == NULL) {
        pCfg = (DSP_AD_CONFIG*)my_calloc (1, sizeof (DSP_AD_CONFIG));
        if (pCfg == NULL) return NULL;
        pCfg->Status = 1;   // Gltige Config.
        pCfg->CardType = 0; // Todo
    }

    for (Channel = 0; Channel < 8; Channel++) {
        ReadOneDigChannelFromIni (&Cocds, Card, Channel, 1);  // 1->IN
        pCfg->DigInput[Channel].OnOff = Cocds.OnOff;
        if (pCfg->DigInput[Channel].OnOff) {
            pCfg->DigInput[Channel].Vid = add_bbvari (Cocds.SignalName,
                                                      ConvertDataTypeStringToInt (Cocds.DataType),
                                                      Cocds.Unit);
        }
    }
    return pCfg;
}

DSP_AD_CONFIG *ReadDigOutConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg, int Card)
{
    int Channel;
    CONFIG_ONE_CHANNEL_DIG_IN_STRUCT Cocds;

    if (pCfg == NULL) {
        pCfg = (DSP_AD_CONFIG*)my_calloc (1, sizeof (DSP_AD_CONFIG));
        if (pCfg == NULL) return NULL;
        pCfg->Status = 1;   // Gltige Config.
        pCfg->CardType = 0; // Todo
    }

    for (Channel = 0; Channel < 16; Channel++) {
        ReadOneDigChannelFromIni (&Cocds, Card, Channel, 0);  // 0->OUT
        pCfg->DigOutput[Channel].OnOff = Cocds.OnOff;
        if (pCfg->DigOutput[Channel].OnOff) {
            pCfg->DigOutput[Channel].Vid = add_bbvari (Cocds.SignalName,
                                                       ConvertDataTypeStringToInt (Cocds.DataType),
                                                       Cocds.Unit);
        }
    }
    return pCfg;
}

typedef struct {
    int OffFrqPwmFlag;
// 0 -> Off
// 1 -> Frequenz-Messung
// 2 -> Periodendauer
// 3 -> Puls-Breite (High-Time)
// 4 -> Puls-Breite (Low-Time)
    FRQ_PWM_PHASE_CONFIG Signal;
    double MinPeriodeTime;
} CONFIG_FRQ_PWM_IN_DLG_STRUCT;

int ReadFrqPwmInputFromIni (CONFIG_FRQ_PWM_IN_DLG_STRUCT *pCoc, int Card, int Channel)
{
    char Section[INI_MAX_SECTION_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[INI_MAX_LINE_LENGTH];
    char Help[32];
    char *p;

    if (Card) PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Input_Channel_Config Card(%i)", Card);
    else PrintFormatToString (Section, sizeof(Section), "DSP56K_FrqPwm_Input_Channel_Config");

    PrintFormatToString (entry, sizeof(entry), "Channel_%i", Channel);
    if (DBGetPrivateProfileString (Section, Entry, "", Line, sizeof (Line), INIFILE) > 0) {
        p = GetNextCommaSeperatedWord (Line, Help, sizeof (Help));
        pCoc->OffFrqPwmFlag = atol (Help);
        p = ReadFrqPhaseOrPwm (p, &pCoc->Signal, Section, Entry, "sig");
        PrintFormatToString (entry, sizeof(entry), "Channel_%i_MinPeriodeTime", Channel);
        pCoc->MinPeriodeTime = DBGetPrivateProfileFloat (Section, Entry, 0.0, INIFILE);
    } else {
        pCoc->OffFrqPwmFlag = 0;
    }
    return 0;
}


DSP_AD_CONFIG *ReadFrqPwmInputConfigForOneAdcDSP (DSP_AD_CONFIG *pCfg, int Card)
{
    int Channel;
    CONFIG_FRQ_PWM_IN_DLG_STRUCT Cocds;

    if (pCfg == NULL) {
        pCfg = (DSP_AD_CONFIG*)my_calloc (1, sizeof (DSP_AD_CONFIG));
        if (pCfg == NULL) return NULL;
        pCfg->Status = 1;   // Gltige Config.
        pCfg->CardType = 0; // Todo
    }

    // dann die Frequenz, PWM, Phase
    for (Channel = 0; Channel < 1; Channel++) {
        ReadFrqPwmInputFromIni (&Cocds, Card, Channel);

        pCfg->DigFrqPwmInput[Channel].OffFrqPwmFlag = Cocds.OffFrqPwmFlag;
        // max Periodendauer berwachung
        switch (Cocds.OffFrqPwmFlag) {
        default:
        case 1: // -> Frequenz-Messung
        case 2: // -> Periodendauer
            pCfg->DigFrqPwmInput[Channel].MinPeriodeTime = 0.0;
            break;
        case 3: // -> Puls-Breite (High-Time)
        case 4: // -> Puls-Breite (Low-Time)
            pCfg->DigFrqPwmInput[Channel].MinPeriodeTime = Cocds.MinPeriodeTime;
            break;
        }

        if (pCfg->DigFrqPwmInput[Channel].OffFrqPwmFlag) {
            if (ReadFrqOrPwmOrPhase (&pCfg, &Cocds.Signal, &pCfg->DigFrqPwmInput[Channel].Signal)) {
                pCfg->DigFrqPwmInput[Channel].OffFrqPwmFlag = 0;
            }
        }
    }
    return pCfg;
}


#define DSP56K_ADC_CONFIG_FLAG  1
#define DSP56K_DAC_CONFIG_FLAG  2

static int DSP56KNextConfig (int par_Mask)
{
    // Das Laden der Config locken, damit dabei nicht eingleichzeit ein LOAD_ANAOG_CFG stattfindet.
    EnterIniDBCriticalSectionUser(__FILE__, __LINE__);
    if (pLastDacConfig != NULL) {
        DeleteAnalogOutConfigForOneDacDSP (pLastDacConfig);
        my_free (pLastDacConfig);
        pLastDacConfig = NULL;
    }
    if (pLastAdcConfig != NULL) {
        DeleteAnalogInConfigForOneAdcDSP (pLastAdcConfig);
        DeleteDigInConfigForOneAdcDSP (pLastAdcConfig);
        DeleteDigOutConfigForOneAdcDSP (pLastAdcConfig);
        DeleteFrqPwmPhaseConfigForOneAdcDSP (pLastAdcConfig);
        DeleteFrqPwmInputConfigForOneAdcDSP (pLastAdcConfig);
        my_free (pLastAdcConfig);
        pLastAdcConfig = NULL;
    }
    while (LastCardNr < FoundDsps.DspCount) {
        switch (FoundDsps.InfoCards[LastCardNr].CardType) {
        case ADC_CARD_TYPE:
            if ((par_Mask & DSP56K_ADC_CONFIG_FLAG) == DSP56K_ADC_CONFIG_FLAG) {
                int FPGAType;
                pLastAdcConfig = ReadAnalogInConfigForOneAdcDSP (NULL, LastAdcCardNr);
                pLastAdcConfig = ReadDigInConfigForOneAdcDSP (pLastAdcConfig, LastAdcCardNr);
                pLastAdcConfig = ReadDigOutConfigForOneAdcDSP (pLastAdcConfig, LastAdcCardNr);
                pLastAdcConfig = ReadFrqPwmPhaseConfigForOneAdcDSP (pLastAdcConfig, LastAdcCardNr);
                pLastAdcConfig = ReadFrqPwmInputConfigForOneAdcDSP (pLastAdcConfig, LastAdcCardNr);
                pLastAdcConfig->LinearAddr = FoundDsps.InfoCards[LastCardNr].LinearAddress;
#ifndef _M_X64
                pLastAdcConfig->LinearAddr_hdw = FoundDsps.InfoCards[LastCardNr].LinearAddress_hdw;
#endif
                pLastAdcConfig->CardNr = LastAdcCardNr;           // 0 ... __MAX_DSPS
                pLastAdcConfig->Channels = FoundDsps.InfoCards[LastCardNr].Channels;
                pLastAdcConfig->Status = FoundDsps.InfoCards[LastCardNr].Status;
                pLastAdcConfig->CardType = 0;  // ToDo

                FPGAType = (FoundDsps.InfoCards[LastCardNr].CardId >> 8) & 0xFF;
                switch (FPGAType) {
                case 0xC2:
                    pLastAdcConfig->FrqInFPGAClock = 6611000.0; // in Hz (neue Karte FPGA $C2 CYCLONE Typ 2 (EP2Q5))
                    break;
                case 0xA1:
                case 0x01:
                    pLastAdcConfig->FrqInFPGAClock = 8263700.0;
                    break;
                default:
                    ThrowError (ERROR_NO_STOP, "unkown FPGA %02X found on AD-DSP card CARD_ID = %06X (asumming it's an old one)", FPGAType,FoundDsps.InfoCards[LastCardNr].CardId);
                    pLastAdcConfig->FrqInFPGAClock = 8263700.0;
                    break;
                }
                pLastAdcConfig->FrqInFPGAClockPeriod = 1.0 / pLastAdcConfig->FrqInFPGAClock;

                // DgbPrintAdcConfigStruct (pLastAdcConfig, 0);
                write_message (get_pid_by_name ("DSP56301"), 0xADC, REAL_SIZEOF (pLastAdcConfig), (char*)pLastAdcConfig);
                LastAdcCardNr++;
                LastCardNr++;
                LeaveIniDBCriticalSectionUser();
                return 1;
            }
            break;
        case DAC_CARD_TYPE:
            if ((par_Mask & DSP56K_DAC_CONFIG_FLAG) == DSP56K_DAC_CONFIG_FLAG) {
                pLastDacConfig = ReadAnalogOutConfigForOneDacDSP (LastDacCardNr);
                pLastDacConfig->LinearAddr = FoundDsps.InfoCards[LastCardNr].LinearAddress;
#ifndef _M_X64
                pLastDacConfig->LinearAddr_hdw = FoundDsps.InfoCards[LastCardNr].LinearAddress_hdw;
#endif
                pLastDacConfig->CardNr = LastDacCardNr;           // 0 ... __MAX_DSPS
                pLastDacConfig->Channels = FoundDsps.InfoCards[LastCardNr].Channels;
                pLastDacConfig->Status = FoundDsps.InfoCards[LastCardNr].Status;
                pLastDacConfig->CardType = 0;  // ToDo
                write_message (get_pid_by_name ("DSP56301"), 0xDAC, REAL_SIZEOF (pLastDacConfig), (char*)pLastDacConfig);
                LastDacCardNr++;
                LastCardNr++;
                LeaveIniDBCriticalSectionUser();
                return 1;
            }
            break;
        default:
            LastCardNr++;
            break;
        }
    }
    LeaveIniDBCriticalSectionUser();
    return 0;    // Es gab keine weiteren DSP-Configs!
}


void DSP56KLoadAllConfigs (void)
{
    LastCardNr = 0;
    LastAdcCardNr = 0;
    LastDacCardNr = 0;
    write_message(get_pid_by_name ("RemoteMasterControl"), 0xADCDAC, sizeof(LastCardNr), (char*)&LastCardNr);
    //DSP56KNextConfig (DSP56K_ADC_CONFIG_FLAG | DSP56K_DAC_CONFIG_FLAG);
}

void DSP56KConfigAck (int Flag)
{
    UNUSED(Flag);
    DSP56KNextConfig (DSP56K_ADC_CONFIG_FLAG | DSP56K_DAC_CONFIG_FLAG);
}

// wird von der GUI-Message-Loop aus aufgerufen wenn dort eine
// DSP56301_INI-Message empfangen wird.
void DSP56KLoadAllConfigsAfterStartDspProcess (void)
{
    MESSAGE_HEAD mhead;
    read_message (&mhead, (char*)&FoundDsps, sizeof (FoundDsps));
    DSP56KLoadAllConfigs ();
}

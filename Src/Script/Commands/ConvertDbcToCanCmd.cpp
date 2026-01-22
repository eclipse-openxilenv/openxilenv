#include <stdio.h>

extern "C" {
#include "MyMemory.h"
#include "IniDataBase.h"
#include "ImportDbc.h"
#include "StringMaxChar.h"

}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cConvertDbcToCanCmd)


int cConvertDbcToCanCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}


static char* BuildMemberList(char *par_SemicolonSeparatedList)
{
    char *Members = (char*)my_malloc(strlen(par_SemicolonSeparatedList) + 2);
    if (Members != NULL) {
        char *d, *s;
        for (d = Members, s = par_SemicolonSeparatedList; *s != 0; d++, s++) {
            if (*s == ';') {
                *d = 0;
            } else {
                *d = *s;
            }
        }
        *d = 0;
    }
    return Members;
}

static int GetVariantIndexByName(char *par_Name)
{
    char section[INI_MAX_SECTION_LENGTH], txt[INI_MAX_LINE_LENGTH];
    int i, Size;

    Size = IniFileDataBaseReadInt ("CAN/Global", "can_varianten_count", 0, GetMainFileDescriptor());
    for (i = 0; i < Size; i++) {
        sprintf (section, "CAN/Variante_%i", i);
        IniFileDataBaseReadString (section, "name", "", txt, sizeof (txt), GetMainFileDescriptor());
        if (!strcmp(txt, par_Name)) {
            return i;
        }
    }
    return -1;
}

int cConvertDbcToCanCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    char InputDbcFile[MAX_PATH];
    char OutputCanFile[MAX_PATH];
    char VariantName[MAX_PATH];
    char SignalPrefix[MAX_PATH];
    char SignalPostfix[MAX_PATH];
    char *TxMembers;
    char *RxMembers;
    int ExtendSignalWithObjectName = 0;
    int ObjectNameDoublePoint = 0;
    int SortSignals = 0;
    char VariantTransferSettings[MAX_PATH];
    int ObjectAdditionalEquations = 0;
    int SignalDataType = 0;
    int SignalEquation = 0;
    int SignalInitValue = 0;
    int ObjectInitData = 0;
    int ParameterOffset = 0;
    int TransferSettingsVarianteIndex = -1;
    int Ret;

    StringCopyMaxCharTruncate(InputDbcFile, par_Parser->GetParameter(0), MAX_PATH);
    StringCopyMaxCharTruncate(OutputCanFile, par_Parser->GetParameter(1), MAX_PATH);
    StringCopyMaxCharTruncate(VariantName, par_Parser->GetParameter(2), MAX_PATH);
    TxMembers = BuildMemberList(par_Parser->GetParameter(3));
    RxMembers = BuildMemberList(par_Parser->GetParameter(4));
    if ((TxMembers != NULL) || (RxMembers != NULL)) {
        if (par_Parser->GetParameterCounter() >= 6) {
            StringCopyMaxCharTruncate(SignalPrefix, par_Parser->GetParameter(5), MAX_PATH);
        } else {
            SignalPrefix[0] = 0;
        }
        if (par_Parser->GetParameterCounter() >= 7) {
            StringCopyMaxCharTruncate(SignalPostfix, par_Parser->GetParameter(6), MAX_PATH);
        } else {
            SignalPostfix[0] = 0;
        }
        if (par_Parser->GetParameterCounter() >= 8) {
            int x;
            for (x = 0; x < 2; x++) {
                if (!stricmp(par_Parser->GetParameter(7 + ParameterOffset), "EXTENED_SIGNAL_WITH_OBJECT_NAME")) {
                    ExtendSignalWithObjectName = 1;
                    ParameterOffset++;
                } else if (!stricmp(par_Parser->GetParameter(7 + ParameterOffset), "OBJECT_NAME_DOUBLEPOINT")) {
                    ObjectNameDoublePoint = 1;
                    ParameterOffset++;
                } else {
                    break;
                }
            }
        }
        if (par_Parser->GetParameterCounter() >= (8 + ParameterOffset)) {
            StringCopyMaxCharTruncate(VariantTransferSettings, par_Parser->GetParameter(7 + ParameterOffset), MAX_PATH);
        } else {
            VariantTransferSettings[0] = 0;
        }
        if (strlen(VariantTransferSettings) > 0) {
            int x;
            for (x = 8 + ParameterOffset; x < par_Parser->GetParameterCounter(); x++) {
                if (!stricmp(par_Parser->GetParameter(x), "OBJECT_ADDITIONAL_EQUATIONS")) {
                    ObjectAdditionalEquations = 1;
                } else if (!stricmp(par_Parser->GetParameter(x), "SIGNAL_DATA_TYPE")) {
                    SignalDataType = 1;
                } else if (!stricmp(par_Parser->GetParameter(x), "SIGNAL_EQUATION")) {
                    SignalEquation = 1;
                } else if (!stricmp(par_Parser->GetParameter(x), "SIGNAL_INIT_VALUE")) {
                    SignalInitValue = 1;
                } else if (!stricmp(par_Parser->GetParameter(x), "OBJECT_INIT_DATA")) {
                    ObjectInitData = 1;
                } else if (!stricmp(par_Parser->GetParameter(x), "SORT_SIGNALS")) {
                    SortSignals = 1;
                } else {
                    par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "wrong parameter \"%s\"",
                                       par_Parser->GetParameter(x));
                    break;
                }
            }
            TransferSettingsVarianteIndex = GetVariantIndexByName(VariantTransferSettings);
        } else {
            TransferSettingsVarianteIndex = -1;
        }

        Ret = CANDB_Import (InputDbcFile,
                            OutputCanFile,
                            VariantName,
                            TxMembers, RxMembers,
                            SignalPrefix,
                            SignalPostfix,
                            TransferSettingsVarianteIndex >= 0,
                            TransferSettingsVarianteIndex,
                            ObjectAdditionalEquations,
                            SortSignals,
                            SignalDataType,
                            SignalEquation,
                            SignalInitValue,
                            ObjectInitData,
                            ExtendSignalWithObjectName,
                            ObjectNameDoublePoint);
        my_free(TxMembers);
        my_free(RxMembers);
    } else {
        Ret = -1;
    }
    return Ret;
}

int cConvertDbcToCanCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cConvertDbcToCanCmd ConvertDbcToCanCmd ("CONVERT_DBC_TO_CAN",
                                               5,
                                               16,
                                               nullptr,
                                               FALSE,
                                               FALSE,
                                               0,
                                               0,
                                               CMD_INSIDE_ATOMIC_ALLOWED);

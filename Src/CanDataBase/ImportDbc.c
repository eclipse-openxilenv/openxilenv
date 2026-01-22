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
#include <stdlib.h>
#include <string.h>
#include "ThrowError.h"
#include "PrintFormatToString.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "Files.h"
#include "MyMemory.h"
#include "CanDataBase.h"
#include "IniDataBase.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#include "EquationParser.h"
#include "ImportDbc.h"

#define MAX_ADDITIONAL_EQUS 100

int CANDB_GetAllBusMembers (const char *FileName, char **MemberNames)
{
    FILE *fh;
    int ret = -1;
    int LineCounter = 0;
    char Word[1024];
    int MemberCounter = 0;
    int MemberListSize = 0;
    int MemberListSizeOld;
    char *p;
    int eol;

    *MemberNames = NULL;
    if ((fh = open_file (FileName, "rt")) == NULL) {
        ThrowError (1, "cannot open file %s", FileName);
        ret = -1;
    } else {
        while (read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter) != END_OF_FILE) {
            if (!strcmp ("BU_:", Word)) {
                do {
                    eol = read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);
                    MemberCounter++;
                    MemberListSizeOld = MemberListSize;
                    MemberListSize += (int)strlen (Word) + 1;
                    *MemberNames = (char*)my_realloc (*MemberNames, MemberListSize+10);
                    if (*MemberNames == NULL) {
                        ThrowError (1, "Out of memory");
                        ret = -1;
                        break;
                    }
                    p = *MemberNames;
                    p += MemberListSizeOld;
                    StringCopyMaxCharTruncate (p, Word, sizeof(Word));
                } while (eol != END_OF_LINE);
                p = *MemberNames;
                p += MemberListSize;
                if (p != NULL) {
                    *p = 0;
                    p++;
                    *p = 0;
                }
                ret = MemberCounter;
                break;
            }
        }
        if (!MemberCounter) ThrowError (1, "no \"BU:\" section found");
    }
    close_file (fh);
    return ret;
}

void CANDB_FreeMembersList (char *MemberNames)
{
    my_free (MemberNames);
}

static int IsInMemberList (char *Member, char *MemberList)
{
    char *p = MemberList;
    
    if (p == NULL) return 0;
    do {
        if (!strcmp (Member, p)) return 1;
        p = p + strlen (p) + 1;
    } while (*p != 0);
    return 0;
}

static void ReplaceKeywords (int Vnr, int Onr, char *Node, const char *In, char *Out, int maxc)
{
    const char *p;
    char *d;
    char *s;

    p = In;
    d = Out;
    maxc--;

    while (*p != 0) {
        if ((p - In) >= maxc) break;
        if ((*p == '#') && (p[1] == '[')) {
            if (!strncmp (p+2, "id", 2) && (p[4] == ']')) {
                char section[INI_MAX_SECTION_LENGTH];
                char help[512];
                PrintFormatToString (section, sizeof(section), "CAN/Variante_%i/Object_%i", Vnr, Onr);
                IniFileDataBaseReadString(section, "id", "", help, sizeof (help), GetMainFileDescriptor());
                if ((int)((p - In) + (int)strlen (help)) >= maxc) break;
                StringCopyMaxCharTruncate (d, help, sizeof(help));
                p += 5;   // #[id]
                d += strlen (d);
            } else if (!strncmp (p+2, "name", 4) && (p[6] == ']')) {
                char section[INI_MAX_SECTION_LENGTH];
                char help[512];
                PrintFormatToString (section, sizeof(section), "CAN/Variante_%i/Object_%i", Vnr, Onr);
                IniFileDataBaseReadString(section, "name", "", help, sizeof (help), GetMainFileDescriptor());
                s = help;
                while (*s != 0) {
                    if (!isalnum (*s) &&
                        (*s != '_') &&
                        (*s != ':') &&
                        (*s != '[') &&
                        (*s != ']')) *s = '_';
                    s++;
                }
                if (((int)(p - In) + (int)strlen (help)) >= maxc) break;
                StringCopyMaxCharTruncate (d, help, sizeof(help));
                p += 7;   // #[name]
                d += strlen (d);
            } else if (!strncmp (p+2, "node", 4) && (p[6] == ']')) {
                char help[1024];
                StringCopyMaxCharTruncate (help, Node, sizeof(help));
                s = help;
                while (*s != 0) {
                    if (!isalnum (*s) &&
                        (*s != '_') &&
                        (*s != ':') &&
                        (*s != '[') &&
                        (*s != ']')) *s = '_';
                    s++;
                }
                if (((int)(p - In) + (int)strlen (help)) >= maxc) break;
                StringCopyMaxCharTruncate (d, help, sizeof(help));
                p += 7;   // #[name]
                d += strlen (d);
            } else *d++ = *p++;
        } else *d++ = *p++;
    }
    *d = 0;
}

static int SearchCanObject (int Vnr, int Id, int ExtFlag)
{
    char Section[INI_MAX_SECTION_LENGTH];
    char Txt[512];
    int c, can_object_count;
    int IdRef , ExtFlagRef;

    PrintFormatToString (Section, sizeof(Section), "CAN/Variante_%i", Vnr);
    can_object_count = IniFileDataBaseReadInt (Section, "can_object_count", 0, GetMainFileDescriptor());
    for (c = 0; c < can_object_count; c++) {
        PrintFormatToString (Section, sizeof(Section), "CAN/Variante_%i/Object_%i", Vnr, c);
        IniFileDataBaseReadString (Section, "id", "", Txt, sizeof (Txt), GetMainFileDescriptor());
        IdRef = strtol (Txt, NULL, 0) ;
        IniFileDataBaseReadString (Section, "extended", "", Txt, sizeof (Txt), GetMainFileDescriptor());
        if (!strcmpi ("yes", Txt)) ExtFlagRef = 1;
        else ExtFlagRef = 0;
        if ((Id == IdRef) && (ExtFlag == ExtFlagRef)) {
            // Object found
            return c;
        }
    }
    // Object not found
    return -1;
}

static int SearchCanSignal (int Vnr, int Onr, char *Name)
{
    char Section[INI_MAX_SECTION_LENGTH];
    char Txt[512];
    int s, signal_count;

    PrintFormatToString (Section, sizeof(Section), "CAN/Variante_%i/Object_%i", Vnr, Onr);
    signal_count = IniFileDataBaseReadInt (Section, "signal_count", 0, GetMainFileDescriptor());
    for (s = 0; s < signal_count; s++) {
        PrintFormatToString (Section, sizeof(Section), "CAN/Variante_%i/Object_%i/Signal_%i", Vnr, Onr, s);
        IniFileDataBaseReadString (Section, "name", "", Txt, sizeof (Txt), GetMainFileDescriptor());
        if (!strcmp (Name, Txt)) {
            return s;
        }
    }
    // Object not found
    return -1;
}


int TransferSettingsFromOtherVariant (int Fd,
                                      int VnrTo,
                                      int VnrFrom,
                                      int ObjectAddEquFlag,
                                      int SortSigFlag,
                                      int SigDTypeFlag,
                                      int SigEquFlag,
                                      int SigInitValueFlag,
                                      int ObjectInitDataFlag)
{
    char Section[INI_MAX_SECTION_LENGTH];
    char Section2[INI_MAX_SECTION_LENGTH];
    char entry[INI_MAX_ENTRYNAME_LENGTH];
    char Txt[INI_MAX_LINE_LENGTH];
    int x, c, s, can_object_count, signal_count = 0;
    int SortCANSigAtTheEnd;
    int Id, ExtFlag;
    int Onr, Snr;


    PrintFormatToString (Section, sizeof(Section), "CAN/Variante_%i", VnrTo);
    can_object_count = IniFileDataBaseReadInt (Section, "can_object_count", 0, Fd);
    for (c = 0; c < can_object_count; c++) {

        PrintFormatToString (Section, sizeof(Section), "CAN/Variante_%i/Object_%i", VnrTo, c);
        IniFileDataBaseReadString (Section, "id", "", Txt, sizeof (Txt), Fd);
        Id = strtol (Txt, NULL, 0) ;
        IniFileDataBaseReadString (Section, "extended", "", Txt, sizeof (Txt), Fd);
        if (!strcmpi ("yes", Txt)) ExtFlag = 1;
        else ExtFlag = 0;

        if ((Onr = SearchCanObject (VnrFrom, Id, ExtFlag)) >= 0) {
            SortCANSigAtTheEnd = -1;
            PrintFormatToString (Section2, sizeof(Section2), "CAN/Variante_%i/Object_%i", VnrFrom, Onr);
            if (ObjectAddEquFlag) {
                // additional blackboard variables in CAN message
                for (x = 0; x < MAX_ADDITIONAL_EQUS; x++) {  // max. 100 Equations
                    PrintFormatToString (entry, sizeof(entry), "additional_variable_%i", x);
                    if (IniFileDataBaseReadString (Section2, entry, "", Txt, sizeof (Txt), GetMainFileDescriptor()) <= 0) {
                        break;
                    }
                    IniFileDataBaseWriteString (Section, entry, Txt, Fd);
                }
                // additional equations before transmit/receive CAN message
                for (x = 0; x < MAX_ADDITIONAL_EQUS; x++) {  // max. 100 Equations
                    PrintFormatToString (entry, sizeof(entry), "equ_before_%i", x);
                    if (IniFileDataBaseReadString (Section2, entry, "", Txt, sizeof (Txt), GetMainFileDescriptor()) <= 0) {
                        break;
                    }
                    IniFileDataBaseWriteString (Section, entry, Txt, Fd);
                }
                // additional equations behind transmit/receive CAN message
                for (x = 0; x < MAX_ADDITIONAL_EQUS; x++) {  // max. 100 Equations
                    PrintFormatToString (entry, sizeof(entry), "equ_behind_%i", x);
                    if (IniFileDataBaseReadString (Section2, entry, "", Txt, sizeof (Txt), GetMainFileDescriptor()) <= 0) {
                        break;
                    }
                    IniFileDataBaseWriteString (Section, entry, Txt, Fd);
                }
            }
            if (ObjectInitDataFlag) {
                IniFileDataBaseReadString (Section2, "InitData", "", Txt, sizeof (Txt), GetMainFileDescriptor());
                IniFileDataBaseWriteString (Section, "InitData", Txt, Fd);
            }

            if (SigDTypeFlag || SigEquFlag || SortSigFlag || SigInitValueFlag) {
                signal_count = IniFileDataBaseReadInt (Section, "signal_count", 0, Fd);
                for (s = 0; s < signal_count; s++) {
                    char Txt[INI_MAX_LINE_LENGTH];
                    PrintFormatToString (Section, sizeof(Section), "CAN/Variante_%i/Object_%i/Signal_%i", VnrTo, c, s);
                    IniFileDataBaseReadString (Section, "name", "", Txt, sizeof (Txt), Fd);
                    if ((Snr = SearchCanSignal (VnrFrom, Onr, Txt)) >= 0) {
                        PrintFormatToString (Section2, sizeof(Section2), "CAN/Variante_%i/Object_%i/Signal_%i", VnrFrom, Onr, Snr);
                        if (SigDTypeFlag) {
                            IniFileDataBaseReadString (Section2, "bbtype", "", Txt, sizeof (Txt), GetMainFileDescriptor());
                            IniFileDataBaseWriteString (Section, "bbtype", Txt, Fd);
                        }
                        if (SigEquFlag) {
                            IniFileDataBaseReadString (Section2, "convtype", "", Txt, sizeof (Txt), GetMainFileDescriptor());
                            if (!strcmp (Txt, "curve") || !strcmp (Txt, "equation")) {
                                IniFileDataBaseWriteString (Section, "convtype", Txt, Fd);
                                IniFileDataBaseReadString (Section2, "convstring", "", Txt, sizeof (Txt), GetMainFileDescriptor());
                                IniFileDataBaseWriteString (Section, "convstring", Txt, Fd);
                            }
                        }
                        if (SigInitValueFlag) {
                            IniFileDataBaseReadString (Section2, "startvalue active", "yes", Txt, sizeof (Txt), GetMainFileDescriptor());
                            IniFileDataBaseWriteString (Section, "startvalue active", Txt, Fd);
                            IniFileDataBaseReadString (Section2, "startvalue", "", Txt, sizeof (Txt), GetMainFileDescriptor());
                            IniFileDataBaseWriteString (Section, "startvalue", Txt, Fd);
                        }
                        if (SortSigFlag) {
                            IniFileDataBaseReadString (Section2, "convtype", "", Txt, sizeof (Txt), GetMainFileDescriptor());
                            if (!strcmp (Txt, "equation")) {
                                int UseCANData = 0;
                                IniFileDataBaseReadString (Section2, "convstring", "", Txt, sizeof (Txt), GetMainFileDescriptor());
                                CheckEquSyntax (Txt, 1, 0, &UseCANData);
                                if (UseCANData) {
                                    SortCANSigAtTheEnd = s;
                                }
                            }
                        }
                    }
                }
            }
            if (SortCANSigAtTheEnd > -1) {
                if (SortCANSigAtTheEnd <= (signal_count - 1)) {
                    CopySignal (VnrTo, 9999, c, 9999, SortCANSigAtTheEnd, 9999, Fd, Fd);
                    for (s = SortCANSigAtTheEnd; s < (signal_count-1); s++) {
                        CopySignal (VnrTo, VnrTo, Onr, Onr, s+1, s, Fd, Fd);
                    }
                    CopySignal (9999, VnrTo, 9999, c, 9999, signal_count-1, Fd, Fd);
                }
            }
        }
    }
    return 0;
}

int GetCanServerCycleStep (void)
{
    PID Pid;
    int Ret = 1;

    if ((Pid = get_pid_by_name ("CANServer")) > 0) {
        if (get_process_info_ex(Pid, NULL, 0, NULL, NULL, &Ret, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL) != 0) {
            Ret = 1;
        }
        if (Ret <= 0) Ret = 1;
    }
    return Ret;
}

static int IsAName(const char *par_Name)
{
    if (par_Name != NULL) {
        if (par_Name[0] != 0) {
            return 1;
        }
    }
    return 0;
}

int CANDB_Import (const char *FileName,
                  char *OutputCanFileName,
                  char *CanVariantName,
                  char *TxMemberList, char *RxMemberList,
                  const char *Prefix, const char *Postfix,
                  int TransferSettingsFlag,
                  int SrcVarianteNr,
                  int ObjectAddEquFlag,
                  int SortSigFlag,
                  int SigDTypeFlag,
                  int SigEquFlag,
                  int SigInitValueFlag,
                  int ObjectInitDataFlag,
                  int ExtendSignalNameWithObjectNameFlag,
                  int ObjectNameDoublePointFlag)
{
    FILE *fh;
    int ret = -1;
    int LineCounter = 1;
    char Word[1024];
    char Node[1024];
    char ObjectName[1024];
    unsigned long Id;
    int ExtId;
    long Size;
    char *Dir;
    char SignalName[512];
    char Unit[64];
    int StartBit;
    int BitSize;
    int MsbOrLsbFirst;
    double Factor;
    double Offset;
    int Sign;
    char *p;
    int Vnr = 0;
    int Onr;
    int MuxStartBit = 0;
    int MuxBitSize = 0;
    int MuxValue = 0;
    int mux_flag;
    int MuxSignal;
    int c;
    long FilePos;
    int mux_flag_in_obj;
    int run;
    unsigned int Cycle_ms;
    unsigned int Cycle;
    unsigned int Delay_ms;
    unsigned int Delay;
    int Vid;
    double abt_per;
    int CANServerCycles;
    char IniFileName[MAX_PATH];
    char VariantNameProposal[MAX_PATH];
    int DstFd = -1;

    if ((fh = open_file (FileName, "rb")) == NULL) {
        ThrowError (1, "cannot open file %s", FileName);
    } else {
        if (IsAName(OutputCanFileName)) {
            StringCopyMaxCharTruncate(IniFileName, OutputCanFileName, sizeof(IniFileName));
            DstFd = IniFileDataBaseCreateAndOpenNewIniFile(IniFileName);
            IniFileDataBaseWriteString ("CAN/Global", "copy_buffer_type", "2", DstFd);
            Vnr = 9999;
        } else {
            DstFd = GetMainFileDescriptor(); // destination is the loaded INI file
            Vnr = -1;
        }
        if (IsAName(CanVariantName)) {
            StringCopyMaxCharTruncate(VariantNameProposal, CanVariantName, sizeof(VariantNameProposal));
        } else {
            StringCopyMaxCharTruncate(VariantNameProposal, "new imported variante", sizeof(VariantNameProposal));
        }
        PrintFormatToString (Word, sizeof(Word), "%s", VariantNameProposal);
        // generate unique names
        c = 0;
        for (;;) {
            c++;
            if (SearchVarianteByName (Word, -1) < 0) break;
            PrintFormatToString (Word, sizeof(Word), "%s (%i)", VariantNameProposal, c);
        }

        Vnr = AddNewVarianteIni (DstFd, Vnr, Word, "imported from DBC", 500, 0.75, 0, 0, 4000, 0.8);
        if (read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter) != END_OF_FILE) {
            for (;;) {
                if (!strcmp ("BO_", Word)) {
                    read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);   // ID
                    Id = strtoul (Word, NULL, 10);
                    if ((Id & 0x80000000UL) == 0x80000000UL) {
                        ExtId = 1;
                        Id &= ~0x80000000UL;
                    } else ExtId = 0;
                    read_next_word_line_counter (fh, ObjectName, sizeof(ObjectName), &LineCounter);   // Name
                    if (!ObjectNameDoublePointFlag) {
                        char *p = ObjectName;
                        while (*p != 0) p++;
                        if (p > ObjectName) {
                            p--;
                            if (*p == ':') *p = 0;  // delete ':' at the end
                        }
                    }
                    read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);   // Size  oder :
                    if (!strcmp (":", Word)) read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);
                    Size = atol (Word);
                    if (Size < 0) Size = 0;
                    //if (Size > 8) J1939Flag = 1;
                    //else J1939Flag = 0;
                    if (Size > 1785) Size = 1785;
                    read_next_word_line_counter (fh, Node, sizeof(Node), &LineCounter);   // Bus member
                    if (IsInMemberList (Node, TxMemberList)) {
                        Dir = "write";
                    } else if (IsInMemberList (Node, RxMemberList)) {
                        Dir = "read";
                    } else Dir = NULL;   // not used

                    if (Dir != NULL) {
                        Onr = AddNewObjectIni (DstFd, Vnr, ObjectName, ObjectName, Id,
                                               Size, Dir, "normal", 0,
                                               0, 0, "", ExtId);
                        // Read signals
                        FilePos = ftell (fh);
                        mux_flag_in_obj = mux_flag = 0;

                        p = ObjectName;
                        while (*p != 0) p++;
                        if (p > ObjectName) {
                            if (*(p-1) == ':') p--;  // If Object name ends with a : delete this
                        }
                        p[0] = '.';
                        p[1] = 0;

                        // Signals will be read twice, because the MUX bits of a MUX signal
                        // must not be infront of the MUX signals
                        for (run = 0; run < 2; run++) {
                            fseek (fh, FilePos, SEEK_SET);
                            for (;;) {
                                if (read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter) == END_OF_FILE) break;
                                if (!strcmp ("SG_", Word)) {
                                    if (ExtendSignalNameWithObjectNameFlag) {
                                        StringCopyMaxCharTruncate (SignalName, ObjectName, sizeof(SignalName));
                                    } else {
                                        SignalName[0] = 0;
                                    }
                                    read_next_word_line_counter (fh, SignalName + strlen (SignalName) , (int)sizeof(SignalName) - (int)strlen (SignalName), &LineCounter);   // Signal-Nmae
                                    MuxSignal = 0;
                                    read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);   // ':' or 'M' oder 'm??'
                                    if (!strcmp ("M", Word)) {   // MUX signals MUX code bit pos/size
                                        read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);
                                        mux_flag = 1;
                                    } else if (Word[0] == 'm') {  // MUX signal
                                        if (run && !mux_flag_in_obj) {
                                            ThrowError (1, "there is an mux signal %s but no mux bit definition %s (%i)", FileName, LineCounter);
                                        }
                                        MuxSignal = 1;
                                        MuxValue = atol(&Word[1]);
                                        read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);
                                    } else mux_flag = 0;

                                    read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);   // Startbit|Bitsize@msb_first oder lsb_first+
                                    StartBit = strtol(Word, &p, 10);
                                    if (*p != '|') {
                                        ThrowError (1, "Syntax error in \"%s\" expect a '|' (%s [%i])", Word, FileName, LineCounter);
                                        break;
                                    } else p++;
                                    BitSize = strtol(p, &p, 10);
                                    if (*p != '@') {
                                        ThrowError (1, "Syntax error in \"%s\" expect a '@' (%s [%i])", Word, FileName, LineCounter);
                                        break;
                                    } else p++;
                                    MsbOrLsbFirst = strtol(p, &p, 10);
                                    if (*p == '+') {
                                        Sign = 0;
                                    } else if (*p == '-') {
                                        Sign = 1;
                                    } else {
                                        ThrowError (1, "Syntax error in \"%s\" expect a '+' or a '-' (%s [%i])", Word, FileName, LineCounter);
                                        break;
                                    }
                                    read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);   // (Factor,offset)
                                    p = Word;
                                    if (*p != '(') {
                                        ThrowError (1, "Syntax error in \"%s\" expect a '(' (%s [%i])", Word, FileName, LineCounter);
                                        break;
                                    } else p++;
                                    Factor = strtod(p, &p);
                                    if (*p != ',') {
                                        ThrowError (1, "Syntax error in \"%s\" expect a ',' (%s [%i])", Word, FileName, LineCounter);
                                        break;
                                    } else p++;
                                    Offset = strtod(p, &p);
                                    if (*p != ')') {
                                        ThrowError (1, "Syntax error in \"%s\" expect a ')' (%s [%i])", Word, FileName, LineCounter);
                                        break;
                                    }
                                    // Convert factor and offset
                                    if (!strcmp (Dir, "write")) {
                                        // CAN = BB / FAKTOR   -   OFFSET / FAKTOR
                                        if (Factor == 0.0) { 
                                            ThrowError (1, "cannot convert cersion because factor is zero (%s [%i])", FileName, LineCounter);
                                        } else {
                                            Offset = - Offset / Factor;
                                            Factor = 1 / Factor;
                                        }
                                    }
                                    read_next_word_line_counter (fh, Unit, sizeof(Unit), &LineCounter);   // [min|max]
                                    read_next_word_line_counter (fh, Unit, sizeof(Unit), &LineCounter);   // "Unit"
                                    do {
                                        c = fgetc (fh);
                                    } while ((c != EOF) && (c != '\n'));

                                    // IF msb_first the StartBit must be convert
                                    if (MsbOrLsbFirst == 0) {
                                        int TempStartBit = ((StartBit / 8) * 8) + (7 - (StartBit % 8));
                                        StartBit = (Size * 8) - BitSize - TempStartBit;
                                    }

                                    if (mux_flag) {
                                        // Only store Mux infos because it is nat a real signal
                                        MuxStartBit = StartBit;
                                        MuxBitSize = BitSize;
                                        mux_flag_in_obj = 1;
                                        mux_flag = 0;
                                    } else if (run) {   // Write signals inside the twiche run
                                        char Help[512];
                                        char Help2[512];
                                        StringCopyMaxCharTruncate (Help, SignalName, sizeof(Help));
                                        ReplaceKeywords (Vnr, Onr, Node, Prefix, Help2, sizeof(Help2));
                                        StringCopyMaxCharTruncate (SignalName, Help2, sizeof(Help2));
                                        STRING_APPEND_TO_ARRAY (SignalName, Help);
                                        ReplaceKeywords (Vnr, Onr, Node, Postfix, Help2, sizeof(Help2));
                                        STRING_APPEND_TO_ARRAY (SignalName, Help2);

                                        if (MuxSignal) {
                                            AddNewSignalIni (DstFd, Vnr, Onr, SignalName, SignalName, Unit,
                                                             Factor, Offset, StartBit, BitSize,
                                                             (MsbOrLsbFirst) ? "lsb_first" : "msb_first", 0.0, MuxStartBit,
                                                             MuxBitSize, MuxValue, "mux", "BB_UNKNOWN_DOUBLE",
                                                             (Sign) ? "signed" : "unsigned");
                                        } else {
                                            AddNewSignalIni (DstFd, Vnr, Onr, SignalName, SignalName, Unit,
                                                             Factor, Offset, StartBit, BitSize,
                                                             (MsbOrLsbFirst) ? "lsb_first" : "msb_first", 0.0, 0,
                                                             0, 0, "normal", "BB_UNKNOWN_DOUBLE",
                                                             (Sign) ? "signed" : "unsigned");
                                        }
                                    }
                                } else {
                                    break;  // for (;;)
                                }
                            }
                        }
                    }
                } else {
                    if (read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter) == END_OF_FILE) break; 
                }
            } 
        }
        // Read cycle time
        Vid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME), BB_UNKNOWN, NULL);
        abt_per = read_bbvari_double (Vid);
        CANServerCycles = GetCanServerCycleStep ();
        remove_bbvari (Vid);
        fseek (fh, 0, SEEK_SET);
        if (read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter) != END_OF_FILE) {
            for (;;) {
                if (!strcmp ("BA_", Word)) {
                    int Value;
                    int Type;
                    read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);
                    if (!strcmp ("\"GenMsgCycleTime\"", Word)) {
                        Type = SET_CAN_OBJECT_PROPERY_CYCLE;
                    } else if (!strcmp ("\"GenMsgDelayTime\"", Word)) {
                        Type = SET_CAN_OBJECT_PROPERY_DELAY;
                    } else if (!strcmp ("\"VFrameFormat\"", Word)) {
                        Type = SET_CAN_OBJECT_PROPERY_FRAMEFORMAT;
                    } else {
                        Type = SET_CAN_OBJECT_PROPERY_NOTHING;
                    }
                    if (Type != SET_CAN_OBJECT_PROPERY_NOTHING) {
                        read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);
                        if (!strcmp ("BO_", Word)) {
                            read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);   // ID
                            Id = strtoul (Word, NULL, 10);
                            if ((Id & 0x80000000UL) == 0x80000000UL) {
                                ExtId = 1;
                                Id &= ~0x80000000UL;
                            } else ExtId = 0;
                            read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter);   // cycle time or delay in ms or FrameFormat
                            Value = strtoul (Word, NULL, 10);
                            if ((Type == SET_CAN_OBJECT_PROPERY_CYCLE) ||
                                (Type == SET_CAN_OBJECT_PROPERY_DELAY) ||
                                (Type == SET_CAN_OBJECT_PROPERY_STARTDELAY)) {
                                Value = (unsigned long)((double)Value / 1000.0 / abt_per / (double)CANServerCycles);
                            }
                            SetCanObjectPropertyById (DstFd, Vnr, Id, ExtId, Type, Value);
                        }
                    }
                } else {
                    if (read_next_word_line_counter (fh, Word, sizeof(Word), &LineCounter) == END_OF_FILE) break;
                }
            }
        }
        // Should something should be takeover from the old variant
        if (TransferSettingsFlag) {
             TransferSettingsFromOtherVariant (DstFd, Vnr, SrcVarianteNr,
                                               ObjectAddEquFlag,
                                               SortSigFlag,
                                               SigDTypeFlag,
                                               SigEquFlag,
                                               SigInitValueFlag,
                                               ObjectInitDataFlag);
        }
        if (IsAName(OutputCanFileName)) {
            if (IniFileDataBaseSave (DstFd, NULL, INIFILE_DATABAE_OPERATION_REMOVE)) {
                ThrowError (1, "cannot export CAN-DB to file %s", IniFileName);
            }
        }
        close_file (fh);
    }
    return 0;
}

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


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Platform.h"
#include <malloc.h>
#include "Config.h"
#include "WindowIniHelper.h"
#include "IniDataBase.h"
#include "ReadCanCfg.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Wildcards.h"
#include "MainValues.h"
#include "Message.h"
#include "Scheduler.h"
#include "Files.h"
#include "BlackboardAccess.h"
#include "EquationParser.h"
#include "CanDataBase.h"

#include "VirtualNetwork.h"
#include "VirtualCanDriver.h"

#define UNUSED(x) (void)(x)

//static char buffer[1024 * 128];
static int LoadingCANConfigActiveFlag = 0;


static uint64_t SwapBytes (uint64_t v, int size)
{
    int x;
    uint64_t ret = 0;
    char *s, *d;

    s = (char*)&v;
    d = (char*)&ret + size -1;
    for (x = 0; x < size; x++) {
        *d-- = *s++;
    }
    return ret;
}

int GetCanControllerCountFromIni (char *par_Section, int par_Fd)
{
    int can_controller_count;
    int can_controller_count_ext;
    int ret;

    can_controller_count_ext = IniFileDataBaseReadInt (par_Section, "can_controller_count_ext", 0, par_Fd);
    can_controller_count = IniFileDataBaseReadInt (par_Section, "can_controller_count", 0, par_Fd);

    if (can_controller_count_ext >= 4) ret = can_controller_count_ext;
    else ret = can_controller_count;

    if (ret > MAX_CAN_CHANNELS) ret = MAX_CAN_CHANNELS;
    return ret;
}

static void SetCanControllerCountFromIni (char *par_Section, int par_Fd, int par_CanControllerCount)
{
    int can_controller_count;
    int can_controller_count_ext;

    if (par_CanControllerCount > MAX_CAN_CHANNELS) par_CanControllerCount = MAX_CAN_CHANNELS;

    if (par_CanControllerCount < 4) can_controller_count = par_CanControllerCount;
    else can_controller_count = 4;
    can_controller_count_ext = par_CanControllerCount;
    IniFileDataBaseWriteInt(par_Section, "can_controller_count", can_controller_count, par_Fd);
    IniFileDataBaseWriteInt(par_Section, "can_controller_count_ext", can_controller_count_ext, par_Fd);
}

static int DeleteVariante (int vnr);
static int CopyObject (int vnr, int new_vnr, int onr, int new_onr, int DstFile, int SrcFile);
int CopyVariante (int vnr, int new_vnr, int DstFile, int SrcFile)
{
    char SrcSection[INI_MAX_SECTION_LENGTH], DstSection[INI_MAX_SECTION_LENGTH];
    int x, can_object_count;

    DeleteVariante (new_vnr);
    sprintf (SrcSection, "CAN/Variante_%i", vnr);
    sprintf (DstSection, "CAN/Variante_%i", new_vnr);
    IniFileDataBaseCopySection(DstFile, SrcFile, DstSection, SrcSection);
    can_object_count = IniFileDataBaseReadInt(SrcSection, "can_object_count", 0, SrcFile);
    for (x = 0; x < can_object_count; x++) {
        CopyObject (vnr, new_vnr, x, x, DstFile, SrcFile);
    }
    return 0;
}

int AppendVariante (int base_vnr, int append_vnr, int DstFile, int SrcFile)
{
    char base_section[INI_MAX_SECTION_LENGTH], append_section[INI_MAX_SECTION_LENGTH];
    int x, base_can_object_count, append_can_object_count;

    sprintf (base_section, "CAN/Variante_%i", base_vnr);
    sprintf (append_section, "CAN/Variante_%i", append_vnr);
    base_can_object_count = IniFileDataBaseReadInt (base_section, "can_object_count", 0, DstFile);
    append_can_object_count = IniFileDataBaseReadInt (append_section, "can_object_count", 0, SrcFile);
    for (x = 0; x < append_can_object_count; x++) {
        CopyObject (append_vnr, base_vnr, x, base_can_object_count + x, DstFile, SrcFile);
    }
    IniFileDataBaseWriteInt(base_section, "can_object_count", base_can_object_count + append_can_object_count, DstFile);
    return (base_can_object_count + append_can_object_count);
}

int AddNewVarianteIni (char *name, char *desc, int brate)
{
    char section[INI_MAX_SECTION_LENGTH];
    int can_varianten_count;
    char txt[32];

    sprintf (section, "CAN/Global");
    can_varianten_count = IniFileDataBaseReadInt (section, "can_varianten_count", 0, GetMainFileDescriptor());
    sprintf (section, "CAN/Variante_%i", can_varianten_count);
    IniFileDataBaseWriteString (section, "name", name, GetMainFileDescriptor());
    IniFileDataBaseWriteString (section, "desc", desc, GetMainFileDescriptor());
    sprintf (txt, "%i", brate);
    IniFileDataBaseWriteString (section, "baud rate", txt, GetMainFileDescriptor());
    IniFileDataBaseWriteString (section, "can_object_count", "0", GetMainFileDescriptor());
    sprintf (txt, "%i", can_varianten_count+1);
    sprintf (section, "CAN/Global");
    IniFileDataBaseWriteString (section, "can_varianten_count", txt, GetMainFileDescriptor());
    
    return can_varianten_count;
}

static int DeleteCanObject (int vnr, int onr);
static int DeleteVariante (int vnr)
{
    char section[INI_MAX_SECTION_LENGTH];
    int x, can_object_count;

    sprintf (section, "CAN/Variante_%i", vnr);
    //ThrowError (1, "Delete Variante %s", section);
    can_object_count = IniFileDataBaseReadInt (section, "can_object_count", 0, GetMainFileDescriptor());
    for (x = 0; x < can_object_count; x++) {
        DeleteCanObject (vnr, x);
    }
    IniFileDataBaseWriteString (section, NULL, NULL, GetMainFileDescriptor());
    return 0;
}

int SearchVarianteByName (char *name, int NotIdx)
{
    char section[INI_MAX_SECTION_LENGTH], txt[INI_MAX_LINE_LENGTH];
    int i, x;
    
    x = IniFileDataBaseReadInt ("CAN/Global", "can_varianten_count", 0, GetMainFileDescriptor());
    for (i = 0; i < x; i++) {
        if (i == NotIdx) continue;
        sprintf (section, "CAN/Variante_%i", i);
        IniFileDataBaseReadString (section, "name", "", txt, sizeof (txt), GetMainFileDescriptor());
        if (!strcmp (txt, name)) return i;    
    }
    return -1;
}

static int CopyObject (int vnr, int new_vnr, int onr, int new_onr, int DstFile, int SrcFile)
{
    char SrcSection[INI_MAX_SECTION_LENGTH], DstSection[INI_MAX_SECTION_LENGTH];
    int x, signal_count;

    DeleteCanObject (new_vnr, new_onr);
    sprintf (SrcSection, "CAN/Variante_%i/Object_%i", vnr, onr);
    sprintf (DstSection, "CAN/Variante_%i/Object_%i", new_vnr, new_onr);
    IniFileDataBaseCopySection(DstFile, SrcFile, DstSection, SrcSection);
    signal_count = IniFileDataBaseReadInt(SrcSection, "signal_count", 0, SrcFile);
    for (x = 0; x < signal_count; x++) {
        CopySignal (vnr, new_vnr, onr, new_onr, x, x, DstFile, SrcFile);
    }
    return 0;
}

int AddNewObjectIni (int vnr, char *name, char *desc,uint32_t id,
                     int size, char *dir, char *type, int mux_startbit,
                     int mux_bitsize, int mux_value, char *azg_type, int ext)
{
    UNUSED(azg_type);
    char section[INI_MAX_SECTION_LENGTH];
    int object_count;
    char txt[32];
    int Fd = GetMainFileDescriptor();

    sprintf (section, "CAN/Variante_%i", vnr);
    object_count = IniFileDataBaseReadInt (section, "can_object_count", 0, Fd);
    sprintf (section, "CAN/Variante_%i/Object_%i", vnr, object_count);
    IniFileDataBaseWriteString (section, "name", name, Fd);
    IniFileDataBaseWriteString (section, "desc", desc, Fd);
    sprintf (txt, "0x%X", id);
    IniFileDataBaseWriteString (section, "id", txt, Fd);
    sprintf (txt, "%i", size);
    IniFileDataBaseWriteString (section, "size", txt, Fd);
    IniFileDataBaseWriteString (section, "direction", dir, Fd);
    IniFileDataBaseWriteString (section, "type", type, Fd);
    sprintf (txt, "%i", mux_startbit);
    IniFileDataBaseWriteString (section, "mux_startbit", txt, Fd);
    sprintf (txt, "%i", mux_bitsize);
    IniFileDataBaseWriteString (section, "mux_bitsize", txt, Fd);
    sprintf (txt, "%i", mux_value);
    IniFileDataBaseWriteString (section, "mux_value", txt, Fd);
    if (ext) IniFileDataBaseWriteString (section, "extended", "yes", Fd);
    else IniFileDataBaseWriteString (section, "extended", "no", Fd);
    IniFileDataBaseWriteString (section, "signal_count", "0", Fd);

    sprintf (txt, "%i", object_count+1);
    sprintf (section, "CAN/Variante_%i", vnr);
    IniFileDataBaseWriteString (section, "can_object_count", txt, Fd);
    
    return object_count;

}

static int DeleteSignal (int vnr, int onr, int snr);
static int DeleteCanObject (int vnr, int onr)
{
    char section[INI_MAX_SECTION_LENGTH];
    int x, signal_count;

    sprintf (section, "CAN/Variante_%i/Object_%i", vnr, onr);
    signal_count = IniFileDataBaseReadInt (section, "signal_count", 0, GetMainFileDescriptor());
    for (x = 0; x < signal_count; x++) {
        DeleteSignal (vnr, onr, x);
    }
    IniFileDataBaseWriteString (section, NULL, NULL, GetMainFileDescriptor());
    return 0;
}

int SetCanObjectCycleById (int Vnr, unsigned int Id, int ExtFlag, unsigned int Cycle)
{
    char section[INI_MAX_SECTION_LENGTH];
    char txt[32];
    int x, can_object_count;
    unsigned int IdRef;
    int ExtFlagRef;
    int Fd = GetMainFileDescriptor();

    sprintf (section, "CAN/Variante_%i", Vnr);
    can_object_count = IniFileDataBaseReadInt (section, "can_object_count", 0, Fd);
    for (x = 0; x < can_object_count; x++) {
        sprintf (section, "CAN/Variante_%i/Object_%i", Vnr, x);
        if (IniFileDataBaseReadString (section, "id", "", txt, sizeof (txt), Fd)) {
            IdRef = strtoul (txt, NULL, 0);
            IniFileDataBaseReadString (section, "extended", "", txt, sizeof (txt), Fd);
            if (!strcmpi ("yes", txt) ) ExtFlagRef = 1;
            else ExtFlagRef = 0;
            if ((Id == IdRef) && (ExtFlag == ExtFlagRef)) {
                if (Cycle < 1) Cycle = 1;
                sprintf (txt, "%i", Cycle);
                IniFileDataBaseWriteString (section, "cycles", txt, Fd);
                return 1;
            }
        }
    }
    return 0;
}

int CopySignal (int vnr, int new_vnr, int onr, int new_onr, int snr, int new_snr, int DstFile, int SrcFile)
{
    char SrcSection[INI_MAX_SECTION_LENGTH], DstSection[INI_MAX_SECTION_LENGTH];

    DeleteSignal (new_vnr, new_onr, new_snr);
    sprintf (SrcSection, "CAN/Variante_%i/Object_%i/Signal_%i", vnr, onr, snr);
    sprintf (DstSection, "CAN/Variante_%i/Object_%i/Signal_%i", new_vnr, new_onr, new_snr);
    IniFileDataBaseCopySection(DstFile, SrcFile, DstSection, SrcSection);
    return 0;
}


int AddNewSignalIni (int vnr, int onr, char *name, char *desc, char *unit,
                     double convert, double offset, int startbit, int bitsize,
                     char *byteorder, double startvalue, int mux_startbit,
                     int mux_bitsize, int mux_value, char *type, char *bbtype, char *sign)
{
    char section[INI_MAX_SECTION_LENGTH];
    int signal_count;
    char txt[32];
    int Fd = GetMainFileDescriptor();

    sprintf (section, "CAN/Variante_%i/Object_%i", vnr, onr);
    signal_count = IniFileDataBaseReadInt (section, "signal_count", 0, Fd);
    sprintf (section, "CAN/Variante_%i/Object_%i/Signal_%i", vnr, onr, signal_count);
    IniFileDataBaseWriteString (section, "name", name, Fd);
    IniFileDataBaseWriteString (section, "desc", desc, Fd);
    IniFileDataBaseWriteString (section, "unit", unit, Fd);
    sprintf (txt, "%f", convert);
    IniFileDataBaseWriteString (section, "convert", txt, Fd);
    sprintf (txt, "%f", offset);
    IniFileDataBaseWriteString (section, "offset", txt, Fd);
    sprintf (txt, "%i", startbit);
    IniFileDataBaseWriteString (section, "startbit", txt, Fd);
    sprintf (txt, "%i", bitsize);
    IniFileDataBaseWriteString (section, "bitsize", txt, Fd);
    IniFileDataBaseWriteString (section, "byteorder", byteorder, Fd);
    sprintf (txt, "%f", startvalue);
    IniFileDataBaseWriteString (section, "startvalue", txt, Fd);
    sprintf (txt, "%i", mux_startbit);
    IniFileDataBaseWriteString (section, "mux_startbit", txt, Fd);
    sprintf (txt, "%i", mux_bitsize);
    IniFileDataBaseWriteString (section, "mux_bitsize", txt, Fd);
    sprintf (txt, "%i", mux_value);
    IniFileDataBaseWriteString (section, "mux_value", txt, Fd);
    IniFileDataBaseWriteString (section, "type", type, Fd);
    IniFileDataBaseWriteString (section, "bbtype", bbtype, Fd);
    IniFileDataBaseWriteString (section, "sign", sign, Fd);

    sprintf (txt, "%i", signal_count+1);
    sprintf (section, "CAN/Variante_%i/Object_%i", vnr, onr);
    IniFileDataBaseWriteString (section, "signal_count", txt, Fd);
    
    return signal_count;
}


static int DeleteSignal (int vnr, int onr, int snr)
{
    char section[INI_MAX_SECTION_LENGTH];
    sprintf (section, "CAN/Variante_%i/Object_%i/Signal_%i", vnr, onr, snr);
    IniFileDataBaseWriteString (section, NULL, NULL, GetMainFileDescriptor());

    return 0;
}

static void MoveSelectedCANVarianteIndex (int vnr, int offset)
{
    int can_var_nr;
    int x, c, channel_count;
    char txt[INI_MAX_LINE_LENGTH];
    char txt2[INI_MAX_LINE_LENGTH];
    char entry[64];
    char *p;

    channel_count = GetCanControllerCountFromIni ("CAN/Global", GetMainFileDescriptor());

    for (c = 0; c < channel_count; c++) {
        sprintf (entry, "can_controller%i_variante", c+1);
        IniFileDataBaseReadString ("CAN/Global", entry, "", txt2, sizeof (txt2), GetMainFileDescriptor());
        p = txt2;
        x = 0;
        txt[0] = 0;
        while (isdigit (*p)) { 
            can_var_nr = strtol (p, &p, 0);
            if (*p == ',') p++;
            if (can_var_nr == vnr) {
                continue;  // delete this
            } else if (can_var_nr > vnr) {
                can_var_nr -= offset;
            }
            if (x) strcat (txt, ",");
            sprintf (txt + strlen (txt), "%i", can_var_nr);
            x++;
        }
        if (strlen (txt) == 0) StringCopyMaxCharTruncate (txt, "-1", sizeof(txt));
        IniFileDataBaseWriteString ("CAN/Global", entry, txt, GetMainFileDescriptor());
	}
}

// Remark: this function will return always the first node
static int SetSelectedCANVariante (int vnr, int channel)
{
    char txt[32];
    char entry[64];
    int channel_count;

    channel_count = GetCanControllerCountFromIni ("CAN/Global", GetMainFileDescriptor());
    if (channel <= channel_count) {
        sprintf (entry, "can_controller%i_variante", channel);
        sprintf (txt, "%i", vnr); 
        IniFileDataBaseWriteString ("CAN/Global", entry, txt, GetMainFileDescriptor());
        return 0;
    }
    return -1;
}


static int AddSelectedCANVariante (int vnr, int channel)
{
    char txt[INI_MAX_LINE_LENGTH];
    char entry[64];
    int channel_count;

    channel_count = GetCanControllerCountFromIni ("CAN/Global", GetMainFileDescriptor());
    if (channel <= channel_count) {
        sprintf (entry, "can_controller%i_variante", channel);
        sprintf (txt, "%i", vnr); 
        IniFileDataBaseReadString ("CAN/Global", entry, "", txt, sizeof (txt), GetMainFileDescriptor());
        if (strtol (txt, NULL, 0) == -1) {
            sprintf (txt, "%i", vnr);
        } else {
            if (strlen (txt)) strcat (txt, ",");
            sprintf (txt + strlen (txt), "%i", vnr);
        }
        IniFileDataBaseWriteString ("CAN/Global", entry, txt, GetMainFileDescriptor());
        return 0;
    }
    return -1;
}


static int GetFirstSelectedCANVariante (int channel)
{
    char entry[64];
    int channel_count;

    channel_count = GetCanControllerCountFromIni ("CAN/Global", GetMainFileDescriptor());
    if (channel <= channel_count) {
        sprintf (entry, "can_controller%i_variante", channel);
        return IniFileDataBaseReadInt ("CAN/Global", entry, -1, GetMainFileDescriptor());
    }
    return -1;
}


static int LoadCANVarianteScriptCommandThreadFunction (void *par)
{
    char txt[32];
    int x, ret = 0;
    char *fname;
    int channel;
    int Fd;

    channel = *((int*)par);
    fname = (char*)par;
    fname += sizeof(int);

    Fd = IniFileDataBaseOpen(fname);
    if (Fd > 0) {
        if (IniFileDataBaseReadInt ("CAN/Global", "copy_buffer_type", 0, Fd) == 2) {
            x = IniFileDataBaseReadInt ("CAN/Global", "can_varianten_count", 0, GetMainFileDescriptor());
            CopyVariante (9999, x, GetMainFileDescriptor(), Fd);
            sprintf (txt, "%i", x+1);
            IniFileDataBaseWriteString ("CAN/Global", "can_varianten_count", txt, GetMainFileDescriptor());
            ret = SetSelectedCANVariante (x, channel);
        } else {
            ret = -1;
        }
    } else {
        ret = -1;
    }
    LoadingCANConfigActiveFlag = 0;
    IniFileDataBaseClose(Fd);
    my_free (par);
#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    ExitThread (ret);
#endif
    return ret;
}


static int AppnedCANVarianteScriptCommandThreadFunction (void *par)
{
    int ret = 0;
    char *fname;
    int channel;
    int vnr;
    int Fd;

    channel = *((int*)par);
    fname = (char*)par;
    fname += sizeof(int);

    // lock APPEND_CAN_VARIANT no other load of configuration at the same time
    IniFileDataBaseEnterCriticalSectionUser(__FILE__, __LINE__);
    vnr = GetFirstSelectedCANVariante (channel);
    if (vnr < 0) {
        ret = vnr;
    } else {
        Fd = IniFileDataBaseOpen(fname);
        if (Fd > 0) {
            if (IniFileDataBaseReadInt ("CAN/Global", "copy_buffer_type", 0, Fd) == 2) {
                AppendVariante (vnr, 9999, GetMainFileDescriptor(), Fd);
            } else {
                ret = -1;
            }
            LoadingCANConfigActiveFlag = 0;
            IniFileDataBaseClose(Fd);
        } else {
            ret = -1;
        }
    }
    my_free (par);
    IniFileDataBaseLeaveCriticalSectionUser();
#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    ExitThread(ret);
#endif
    return ret;
}


static int LoadAndSelectCANNodeScriptCommandThreadFunction (void *par)
{
    int x, ret = 0;
    char *fname;
    int channel;
    char txt[INI_MAX_LINE_LENGTH];
    int Fd;

    channel = *((int*)par);
    fname = (char*)par;
    fname += sizeof(int);

    // lock LOAD_CAN_VARIANT no other load of configuration at the same time
    IniFileDataBaseEnterCriticalSectionUser(__FILE__, __LINE__);
    Fd = IniFileDataBaseOpen(fname);
    if (Fd > 0) {
        if (IniFileDataBaseReadInt ("CAN/Global", "copy_buffer_type", 0, Fd) == 2) {
            x = IniFileDataBaseReadInt ("CAN/Global", "can_varianten_count", 0, GetMainFileDescriptor());
            CopyVariante (9999, x, GetMainFileDescriptor(), Fd);
            sprintf (txt, "%i", x+1);
            IniFileDataBaseWriteString ("CAN/Global", "can_varianten_count", txt, GetMainFileDescriptor());
            ret = AddSelectedCANVariante (x, channel);
        } else {
            ret = -1;
        }
        IniFileDataBaseClose(Fd);
    } else {
        ret = -1;
    }
    IniFileDataBaseLeaveCriticalSectionUser();
    LoadingCANConfigActiveFlag = 0;
#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    ExitThread(ret);
#endif
    return ret;
}

#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
static HANDLE ThreadHandle=0;
#endif

int LoadCANVarianteScriptCommand (char *fname, int channel)
{
    char *par;
#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    DWORD ThreadId;
#endif

    LoadingCANConfigActiveFlag = 1;
    par = my_malloc(MAX_PATH + 1 + sizeof(int));
    if (par == NULL) return -1;
    *((int*)(void*)par) = channel;
    StringCopyMaxCharTruncate (par + sizeof(int), fname, MAX_PATH);

#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    ThreadHandle = CreateThread (NULL, 64*1024,
                                 (LPTHREAD_START_ROUTINE)LoadCANVarianteScriptCommandThreadFunction,
                                 (void*)par, 0, &ThreadId);
#else
    return LoadCANVarianteScriptCommandThreadFunction (par);
#endif

}

void AppendCANVarianteScriptCommand (char *fname, int channel)
{
    char *par;
#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    DWORD ThreadId;
#endif

    LoadingCANConfigActiveFlag = 1;
    par = my_malloc(MAX_PATH + 1 + sizeof(int));
    if (par == NULL) return;
    *((int*)(void*)par) = channel;
    StringCopyMaxCharTruncate (par + sizeof(int), fname, MAX_PATH);

#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    ThreadHandle = CreateThread (NULL, 64*1024,
                                 (LPTHREAD_START_ROUTINE)AppendCANVarianteScriptCommandThreadFunction,
                                 (void*)par, 0, &ThreadId);
#else
    AppnedCANVarianteScriptCommandThreadFunction (par);
#endif

}


void LoadAndSelectCANNodeScriptCommand (char *fname, int channel)
{
    char *par;
#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    DWORD ThreadId;
#endif

    LoadingCANConfigActiveFlag = 1;
    par = my_malloc(MAX_PATH + 1 + sizeof(int));
    if (par == NULL) return;
    *((int*)(void*)par) = channel;
    StringCopyMaxCharTruncate (par + sizeof(int), fname, MAX_PATH);

#ifdef USE_THREAD_IMPORT_CAN_VARIANTE
    ThreadHandle = CreateThread (NULL, 64*1024,
                                 (LPTHREAD_START_ROUTINE)LoadAndSelectCANNodeScriptCommandThreadFunction,
                                 (void*)par, 0, &ThreadId);
#else
    LoadAndSelectCANNodeScriptCommandThreadFunction (par);
#endif

}

/* Delete all CAN variants from the INI file */
int DeleteAllCANVariantesScriptCommand (void)
{
    int x;
    int can_varianten_count;
    char entry[64];
    int Fd =GetMainFileDescriptor();

    // lock DEL_ALL_CAN_VARIANTS no other load of configuration at the same time
    IniFileDataBaseEnterCriticalSectionUser(__FILE__, __LINE__);
    can_varianten_count = IniFileDataBaseReadInt ("CAN/Global", "can_varianten_count", 0, Fd);
    for (x = 0; x < can_varianten_count; x++) {
        DeleteVariante (x);
    }
    for (x = 0; x < 4; x++) {
        sprintf (entry, "can_controller%i_variante", x+1);
        IniFileDataBaseWriteString ("CAN/Global", entry, NULL, Fd);
    }
    IniFileDataBaseWriteString ("CAN/Global", "can_varianten_count", "0", Fd);
    return -1;
}

static int CmpVnrs (const void *a, const void *b)
{
    if (*(const int*)a < *(const int*)b) return -1;
    else if (*(const int*)a == *(const int*)b) return 0;
    else return 1;
}

static int GetVnrs (int Channel, int *Vnrs, int MaxVnrs)
{
    UNUSED(MaxVnrs);
    int vnr_count = 0;
    char *p;
    char entry[64];
    char txt[INI_MAX_LINE_LENGTH];

    sprintf (entry, "can_controller%i_variante", Channel);
    IniFileDataBaseReadString ("CAN/Global", entry, "", txt, sizeof (txt), GetMainFileDescriptor());
    p = txt;
    while (isdigit (*p)) { 
        Vnrs[vnr_count++] = strtol (p, &p, 0);
        if (*p == ',') p++;
        if (vnr_count >= 100) break;
    }
    qsort (Vnrs, (size_t)vnr_count, sizeof (int), CmpVnrs);

    return vnr_count;
}

void DelCANVarianteScriptCommand (int channel)
{
    int x, i;
    int can_varianten_count;
    int vnrs[100];
    int vnr_count;
    int del_idx = 0;
    int Fd = GetMainFileDescriptor();

    can_varianten_count = IniFileDataBaseReadInt ("CAN/Global", "can_varianten_count", 0, Fd);
    vnr_count = GetVnrs (channel, vnrs, 100);
    x = 0;
    for (i = 0; i < can_varianten_count; i++) {
        if (i == vnrs[x]) {  // delete this variant
            DeleteVariante (i); 
            MoveSelectedCANVarianteIndex (i - del_idx, 1);
            del_idx++;
            x++;
        } else {
            if (del_idx) {
                CopyVariante (i, i - del_idx, Fd, Fd);
                DeleteVariante (i); 
            }
        }
    }
    IniFileDataBaseWriteInt ("CAN/Global", "can_varianten_count", can_varianten_count - del_idx, Fd);
}


void DelCANVarianteByNameScriptCommand (char *Name)
{
    int x, i;
    int can_varianten_count;
    int del_idx = 0;
    char section[INI_MAX_SECTION_LENGTH], txt[INI_MAX_LINE_LENGTH];
    int Fd = GetMainFileDescriptor();

    can_varianten_count = IniFileDataBaseReadInt ("CAN/Global", "can_varianten_count", 0, Fd);
    x = 0;
    for (i = 0; i < can_varianten_count; i++) {
        sprintf (section, "CAN/Variante_%i", i);
        IniFileDataBaseReadString (section, "name", "", txt, sizeof (txt), Fd);
        if (!Compare2StringsWithWildcardsCaseSensitive (txt, Name, 1)) {  // Case sensitive
            DeleteVariante (i); 
            MoveSelectedCANVarianteIndex (i - del_idx, 1);
            del_idx++;
            x++;
        } else {
            if (del_idx) {
                CopyVariante (i, i - del_idx, Fd, Fd);
                DeleteVariante (i); 
            }
        }
    }
    IniFileDataBaseWriteInt ("CAN/Global", "can_varianten_count", can_varianten_count - del_idx, Fd);
}

int GetDtypeString (char *Dtype, char **pTxt, char **pDtypstr, char **pName)
{
    char *p;
    size_t Size;

    p = *pTxt;
    Size = strlen (Dtype);
    while ((*p != 0) && isspace (*p)) p++;
    *pDtypstr = p;
    if (!_strnicmp (Dtype, p, Size)) {
        p += Size;
        if ((*p != 0) || isspace(*p)) {
            if (*p != 0) *p++ = 0;
            while ((*p != 0) && isspace (*p)) p++;
            *pName = p;
            while ((*p != 0) && !isspace (*p) && (*p != ';')) p++;
            if (p == *pName) return 0;
            if (*p != 0) *p++ = 0;
            *pTxt = p;
            return 1;
        }
    }
    return 0;
}


static void BuildMasks(uint8_t *par_AndMask, uint8_t *par_OrMask,
                       int par_Startbit, int par_Bitsize, int par_Size, uint64_t par_BitErrValue)
{
    int x, y;
    int Endbit = par_Startbit + par_Bitsize;
    for (x = 0; x < par_Size; x++) {
        par_AndMask[x] = 0xFF;
        par_OrMask[x] = 0x00;
    }
    // look for each bit inside the mask;
    for (x = 0; x < par_Size; x++) {
        for (y = 0; y < 8; y++) {
            int BitPos = (x << 3) + y;
            if ((BitPos >= par_Startbit) && (BitPos < Endbit)) {
                uint8_t Bit = (par_BitErrValue >> (BitPos - par_Startbit)) & 0x1;
                par_OrMask[x] |= Bit << y;
                par_AndMask[x] &= ~(1 << y);
            }
        }
    }
}

// Script-command:
// SET_CAN_ERR (Channel, Id, Startbit, Bitsize, Byteorder, Cycles, Value)
// oder SET_CAN_ERR (Channel, Id, CAN-Signalname, Cycles, Value)
// CLEAR_CAN_ERR 

int ScriptSetCanErr (int Channel, int Id, int Startbit, int Bitsize, const char *Byteorder, uint32_t Cycles, uint64_t BitErrValue)
{
    CAN_BIT_ERROR CanBitError;
    int Ret;

    CanBitError.Id = Id;
    CanBitError.Counter = Cycles;
    CanBitError.Channel = Channel;
    if (Startbit == -1) {    // negative Startbit -> The byte length of the CAN object should be changed
        CanBitError.Command = CHANGE_DATA_LENGTH;
        if (Bitsize < 0) CanBitError.Size = 0;
        else if (Bitsize > 64) CanBitError.Size = 64;
        else CanBitError.Size = Bitsize;
        CanBitError.SizeSave = -1;
    } else if (Startbit == -2) {    // negative Startbit -> The number of transmit cycles should be changed
        CanBitError.Command = SUSPEND_TRANSMITION;
    } else  {
        CanBitError.Command = OVERWRITE_DATA_BYTES;
        if (!strcmpi ("msb_first", Byteorder)) {
            CanBitError.ByteOrder = 1;
        } else {
            CanBitError.ByteOrder = 0;
        }
        BuildMasks(CanBitError.AndMask, CanBitError.OrMask, Startbit, Bitsize, CAN_BIT_ERROR_MAX_SIZE, BitErrValue);
        CanBitError.Size = -1;
    }
    // first send it to the CAN server
    Ret = write_message (get_pid_by_name ("CANServer"), NEW_CANSERVER_SET_BIT_ERR, sizeof (CanBitError), (char*)&CanBitError);
    // than set the filter to th virtual CANs
    Ret = Ret | VirtualCanInsertCanError(0, Channel, Id, Startbit, Bitsize, Byteorder, Cycles, BitErrValue);
    return Ret;
}

int ScriptSetCanErrSignalName (int Channel, int Id, char *Signalname, uint32_t Cycles, uint64_t BitErrValue)
{
    int Startbit;
    int Bitsize;
    char section[INI_MAX_SECTION_LENGTH];
    char entry[INI_MAX_LINE_LENGTH];
    char txt[INI_MAX_LINE_LENGTH];
    char txt2[INI_MAX_LINE_LENGTH];
    char *p;
    int i, i2, i3;
    int vc2, vc3;
    int c, c_start, c_end;
    int id_x;
    int Fd = GetMainFileDescriptor();

    if (Channel < 0) {
        c_start = 0;
        c_end = GetCanControllerCountFromIni ("CAN/Global", Fd);
    } else {
        c_start = Channel;
        c_end = Channel + 1;
    }

    for (c = c_start; c < c_end; c++) {
        sprintf (entry, "can_controller%i_variante", c+1);
        IniFileDataBaseReadString ("CAN/Global", entry, "", txt2, sizeof (txt2), Fd);
        p = txt2;
        while (isdigit (*p)) { 
            i = strtol (p, &p, 0);
            if (*p == ',') p++;
            if (i < 0) return -1;  // Channel not in use
            
            sprintf (section, "CAN/Variante_%i", i);
            vc2 = IniFileDataBaseReadInt (section, "can_object_count", 0, Fd);
            for (i2 = 0; i2 < vc2; i2++) {
                sprintf (section, "CAN/Variante_%i/Object_%i", i, i2);
                // Do not insert read objects into the list
                IniFileDataBaseReadString (section, "direction", "", txt, sizeof (txt), Fd);
                if (!strcmpi ("read", txt)) continue;
                IniFileDataBaseReadString(section, "id", "0x0", txt, sizeof (txt), Fd);
                id_x = strtol (txt, NULL, 0);
                if ((Id < 0) || (Id == id_x)) {
                    vc3 = IniFileDataBaseReadInt (section, "signal_count", 0, Fd);
                    for (i3 = 0; i3 < vc3; i3++) {
                        sprintf (section, "CAN/Variante_%i/Object_%i/Signal_%i", i, i2, i3);
                        if (!IniFileDataBaseReadString(section, "name", "", txt, sizeof (txt), Fd)) continue;
                        if (!strcmp (Signalname, txt)) {
                            IniFileDataBaseReadString (section, "startbit", "0", txt, sizeof (txt), Fd);
                            Startbit = strtol (txt, NULL, 0);
                            IniFileDataBaseReadString (section, "bitsize", "1", txt, sizeof (txt), Fd);
                            Bitsize = strtol (txt, NULL, 0);
                            IniFileDataBaseReadString (section, "byteorder", "lsb_first", txt, sizeof (txt), Fd);
                            return ScriptSetCanErr (c, id_x, Startbit, Bitsize, txt, Cycles, BitErrValue);
                        }
                    }
                }
            }
        }
    }
    return -2;   // Signal name not found
}

int ScriptClearCanErr (void)
{
    int Ret;

    // first send it to the CAN server
    Ret = write_message (get_pid_by_name ("CANServer"), NEW_CANSERVER_RESET_BIT_ERR, 0, NULL);
    // than set the filter to th virtual CANs
    Ret = Ret | VirtualCanResetCanError(0, -1);
    return Ret;
}

int ScriptGetCANTransmitCycleRateOrId  (int Channel, char *ObjectName, int What)
{
    char section[INI_MAX_SECTION_LENGTH];
    char entry[INI_MAX_LINE_LENGTH];
    char txt[INI_MAX_LINE_LENGTH];
    char txt2[INI_MAX_LINE_LENGTH];
    char *p;
    int i, i2;
    int vc2;
    int c, c_start, c_end;
    int ret = -1;
    int Fd = GetMainFileDescriptor();

    if (Channel < 0) {
        c_start = 0;
        c_end = GetCanControllerCountFromIni ("CAN/Global", Fd);
    } else {
        c_start = Channel;
        c_end = Channel + 1;
    }

    for (c = c_start; c < c_end; c++) {
        sprintf (entry, "can_controller%i_variante", c+1);
        IniFileDataBaseReadString ("CAN/Global", entry, "", txt2, sizeof (txt2), Fd);
        p = txt2;
        while (isdigit (*p)) { 
            i = strtol (p, &p, 0);
            if (*p == ',') p++;
            if (i < 0) return -1;  // Channel not in use
            
            sprintf (section, "CAN/Variante_%i", i);
            vc2 = IniFileDataBaseReadInt (section, "can_object_count", 0, Fd);
            for (i2 = 0; i2 < vc2; i2++) {
                sprintf (section, "CAN/Variante_%i/Object_%i", i, i2);
                // Do not insert read objects into the list
                IniFileDataBaseReadString (section, "direction", "", txt, sizeof (txt), Fd);
                // Cycle rate only with transmit objects
                if ((What == 0) && !strcmpi ("read", txt)) continue;
                IniFileDataBaseReadString(section, "name", "", txt, sizeof (txt), Fd);
                if (!strcmp (txt, ObjectName)) {
                    switch (What) {
                    case 1:
                        ret = IniFileDataBaseReadInt (section, "id", -1, Fd);
                        return ret;
                    case 0:
                    default:
                        IniFileDataBaseReadString (section, "cycles", "-1", txt, sizeof (txt), Fd);
                        {
                            double v = direct_solve_equation (txt);
                            ret = (int)v;
                        }
                        return ret;
                    }
                }
            }
        }
    }
    return -1;   // Objekt name not found
}

int ScriptSetCANChannelCount (int ChannelCount)
{
    int Fd = GetMainFileDescriptor();

    if ((ChannelCount < 1) || (ChannelCount > 8)) {
        return -1;
    } else {
        SetCanControllerCountFromIni ("CAN/Global", Fd, ChannelCount);
        return 0;
    }
}

int ScriptSetCANChannelStartupState (int Channel, int StartupState)
{
    char Entry[64];
    int Fd = GetMainFileDescriptor();
    if (GetCanControllerCountFromIni ("CAN/Global", Fd) < Channel) {
        sprintf (Entry, "can_controller%i_startup_state", Channel);
        IniFileDataBaseWriteInt ("CAN/Global", Entry, StartupState, Fd);
        return 0;
    }
    return -1;
}

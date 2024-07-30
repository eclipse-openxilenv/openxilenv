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


#ifndef _CANDB_H
#define _CANDB_H

#include <stdint.h>

int AddNewVarianteIni (char *name, char *desc, int brate);
int AddNewObjectIni (int vnr, char *name, char *desc,uint32_t id,
					 int size, char *dir, char *type, int mux_startbit,
					 int mux_bitsize, int mux_value, char *azg_type, int ext);
int AddNewSignalIni (int vnr, int onr, char *name, char *desc, char *unit,
					 double convert, double offset, int startbit, int bitsize,
					 char *byteorder, double startvalue, int mux_startbit,
					 int mux_bitsize, int mux_value, char *type, char *bbtype, char *sign);
int ImportGlobalIni (int c, int v,uint32_t address, int irq);

int SearchVarianteByName (char *name, int NotIdx);
int DeleteVarianteByName (char *name);
int LoadCANVarianteScriptCommand (char *fname, int channel);
void LoadAndSelectCANNodeScriptCommand (char *fname, int channel);
void AppendCANVarianteScriptCommand (char *fname, int channel);
void DelCANVarianteScriptCommand (int channel);
void DelCANVarianteByNameScriptCommand (char *Name);
int  IsCANVarianteLoaded (void);

int DeleteAllCANVariantesScriptCommand (void);


int ScriptClearCanErr (void);
int ScriptSetCanErrSignalName (int Channel, int Id, char *Signalname,uint32_t Cycles, uint64_t BitErrValue);
int ScriptSetCanErr (int Channel, int Id, int Startbit, int Bitsize, const char *Byteorder, uint32_t Cycles, uint64_t BitErrValue);

int ScriptGetCANTransmitCycleRateOrId  (int Channel, char *ObjectName, int What);

void ReadCANCardInfosFromQueue (void);

int SetCanObjectCycleById (int Vnr, unsigned int Id, int ExtFlag, unsigned int Cycle);


int CopySignal (int vnr, int new_vnr, int onr, int new_onr, int snr, int new_snr, int DstFile, int SrcFile);

int ScriptSetCANChannelCount (int ChannelCount);
int ScriptSetCANChannelStartupState (int Channel, int StartupState);
int ScriptSetAZGChannel (int Channel);

int GetDtypeString (char *Dtype, char **pTxt, char **pDtypstr, char **pName);

int GetCanControllerCountFromIni (char *par_Section, int par_Fd);

int ScriptGetCANTransmitCycleRateOrId  (int Channel, char *ObjectName, int What);

#endif

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


#ifndef _RDWRDATA_H
#define _RDWRDATA_H

#include "SharedDataTypes.h"

int ReadValueFromExternProcessViaAddress (uint64_t address, PROCESS_APPL_DATA *pappldata, int pid,
                                          int32_t typenr, union BB_VARI *ret_Value, int *ret_Type, double *ret_DoubleValue);
int WriteValueToExternProcessViaAddress (uint64_t address, PROCESS_APPL_DATA *pappldata, int pid, int *PidSameExe, int PidSameExeCount,
                                         int32_t typenr, union BB_VARI value);
int ReadStringFromExternProcessViaLabel (PROCESS_APPL_DATA *pappldata, int Pid, char *Dst, int MaxSize, char *Label);
int ReadValueFromExternProcessViaLabel (PROCESS_APPL_DATA *pappldata, int Pid, double *pValue, char *Label);

int WriteValuesToExternProcessViaAddressStopScheduler (PROCESS_APPL_DATA *par_DebugInfos, int par_Pid, int par_Count,
                                                       uint64_t *par_Addresses, int32_t *par_TypeNrs, union BB_VARI *par_Values,
                                                       int par_Timeout, int par_ErrorMessageFlag);

int ReadValuesFromExternProcessViaAddressStopScheduler (PROCESS_APPL_DATA *par_DebugInfos, int par_Pid, int par_Count,
                                                        uint64_t *par_Addresses, int32_t *par_TypeNrs, double **ret_Values,
                                                        int par_Timout, int par_ErrorMessageFlag);

int ReadBytesFromExternProcessStopScheduler (int par_Pid,
                                             uint64_t par_StartAddresse, int par_Count, void *ret_Buffer,
                                             int par_Timeout, int par_ErrorMessageFlag);
int WriteBytesToExternProcessStopScheduler (int par_Pid,
                                            uint64_t par_StartAddresse, int par_Count, void *par_Buffer,
                                            int par_Timeout, int par_ErrorMessageFlag);

#endif

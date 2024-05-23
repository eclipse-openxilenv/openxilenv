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


#pragma once

#include <stdint.h>

#include "Blackboard.h"

void set_client_socket_for_request(int socket);

int ConnectToClinetForRequests(char *Address, int Port);

int rm_read_varinfos_from_ini(int Vid, char *Name, uint32_t ReadReqMask);

int rm_write_varinfos_to_ini(BB_VARIABLE *sp_vari_elem);

void rm_ObserationCallbackFunction(void(*ObserationCallbackFunction) (VID Vid, uint32_t ObservationFlags, uint32_t ObservationData), VID Vid, uint32_t ObservationFlags, uint32_t ObservationData);

void rm_SchedulerCyclicEvent(int SchedulerNr, uint64_t CycleCounter, uint64_t SimulatedTimeSinceStartedInNanoSecond);

int RealtimeProcessStarted(int Pid, char *ProcessName);
int RealtimeProcessStopped(int Pid, char *ProcessName);

int AttachRegisterEquationPid(int Pid, uint64_t UniqueNumber);
int AttachRegisterEquation(uint64_t UniqueNumber);
int DetachRegisterEquationPid(int Pid, uint64_t UniqueNumber);
int DetachRegisterEquation(uint64_t UniqueNumber);


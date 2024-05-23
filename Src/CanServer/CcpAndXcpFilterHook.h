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


#ifndef CCPANDXCPFILTERHOOK_H
#define CCPANDXCPFILTERHOOK_H

#include "Message.h"
#include "CanFifo.h"

void CcpCyclic (void);     // muss in jedem Zyklus aufgerufen werden.
void CcpTerminate (void);  // muss in der Terminate-Funkion des CAN-Servers
                           // aufgerufen werden.
extern int GlobalXcpCcpActiveFlag;      //  != 0 Zeigt an ob CCP aktiv ist.
int CcpMessageFilter (MESSAGE_HEAD *mhead, NEW_CAN_SERVER_CONFIG *csc);
                          // muss im der Messagefilter Routine des CAN-Servers
                          // aufgerufen werden.
int CppCanMessageFilter (NEW_CAN_SERVER_CONFIG *csc, int Channel, uint32_t Id, unsigned char *Data, int Size);

#endif

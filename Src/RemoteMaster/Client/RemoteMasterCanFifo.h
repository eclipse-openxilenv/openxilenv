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

#include "CanFifo.h"

int rm_CreateCanFifos (int Depth);

int rm_DeleteCanFifos (int Handle);

int rm_FlushCanFifo (int Handle, int Flags);

int rm_ReadCanMessageFromFifo2Process (int Handle, CAN_FIFO_ELEM *pCanMessage);

int rm_ReadCanMessagesFromFifo2Process (int Handle, CAN_FIFO_ELEM *pCanMessage, int MaxMessages);

int rm_WriteCanMessagesFromProcess2Fifo (int Handle, CAN_FIFO_ELEM *pCanMessage, int MaxMessages);

int rm_WriteCanMessageFromProcess2Fifo (int Handle, CAN_FIFO_ELEM *pCanMessage);

int rm_ReadCanFdMessageFromFifo2Process (int Handle, CAN_FD_FIFO_ELEM *pCanMessage);

int rm_ReadCanFdMessagesFromFifo2Process (int Handle, CAN_FD_FIFO_ELEM *pCanMessage, int MaxMessages);

int rm_WriteCanFdMessagesFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage, int MaxMessages);

int rm_WriteCanFdMessageFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage);

int rm_SetAcceptMask4CanFifo (int Handle, CAN_ACCEPT_MASK *cam, int Size);




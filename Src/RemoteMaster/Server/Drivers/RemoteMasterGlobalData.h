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


#ifndef SOFTCARRTGLOBALDATA_H
#define SOFTCARRTGLOBALDATA_H

#define __MAX_FLEXCARDS 4
#define __MAX_SOCKETCANS 16

typedef struct {
    struct {
        int Major;
        int Minor;
        int Patch;
    } LinuxKernelVersion;

    struct {
        struct {
            void *BaseAddress;
            int BaseAddressFileDescriptor;
            void *DMAVirtualAddress;
            int DMAVirtualAddressFileDescriptor;
            unsigned int DMAPhysicalAddress;
            unsigned int FirmwareVersion;
            unsigned int HardwareVersion;
            unsigned int FeatureSwitchs;
        } FlexCards[__MAX_FLEXCARDS];
        int FoundFlexCards;
        int FlexCardInterruptFileDescriptor;
        int FlexCardConfigFileDescriptor;
        unsigned char FlexCardCommandHigh;
    } HwInfosFlexcards;

    struct {
        struct {
            int Socket;
        } SocketCAN[__MAX_SOCKETCANS];
        int FoundSocketCANs;
    } HwInfosSocketCANs;
} REMOTE_MASTER_GLOBAL_DATA;

#endif
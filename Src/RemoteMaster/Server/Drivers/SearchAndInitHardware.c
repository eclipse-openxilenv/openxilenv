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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "MapSocketCAN.h"
#include "IrqCpuAffinity.h"

#include "RemoteMasterGlobalData.h"

#include "SearchAndInitHardware.h"


static REMOTE_MASTER_GLOBAL_DATA XilEnvGloabalHardwareData;

int SearchAndInitHardware(void)
{
    char eth_dev_name[1024];

    GetLinuxKernelVersion(&XilEnvGloabalHardwareData.LinuxKernelVersion.Major,
                          &XilEnvGloabalHardwareData.LinuxKernelVersion.Minor,
                          &XilEnvGloabalHardwareData.LinuxKernelVersion.Patch);
 
    if (SocketCANsInit(&XilEnvGloabalHardwareData) > 0) {

    }

    // All interrupts would be processed by CPU 0. Therefore CPU1...n should not have to process any interupts
	ResetAllIrqToCpu0();

    if (GetEthernetDeviceNameForIpAddress(eth_dev_name)) {
        strcpy(eth_dev_name, "enp0s25");
    }

    SetIrqToCpu(eth_dev_name, 1);    // ethernet interface interrupt should be done by CPU 1

     // If we found a Flexcard his interrupt should be done by CPU 2 (there is also the realtime scheduler)
    if (XilEnvGloabalHardwareData.HwInfosFlexcards.FoundFlexCards > 0) {
		SetIrqToCpu("uio_flexcard", 2);

	}
    return 0;
}

REMOTE_MASTER_GLOBAL_DATA *GetHardwareInfos(void)
{
    return &XilEnvGloabalHardwareData;
}

int GetFlexrayChannelCount(void)
{
    return XilEnvGloabalHardwareData.HwInfosFlexcards.FoundFlexCards;
}


int TerminateAllHardware(void)
{
    return 0;
}

int GetLinuxKernelVersion(int *ret_Major, int *ret_Minor, int *ret_Patch)
{
    int fd = open("/proc/version", O_RDONLY | O_SYNC);
    if (fd != -1) {
        char HelpString[256];
        char *p;
        read(fd, HelpString, sizeof(HelpString));
        if (!strncmp(HelpString, "Linux version ", 14)) {
            p = HelpString + 14;
            *ret_Major = strtoul(p, &p, 10);
            if (*p == '.') {
                p++;
                *ret_Minor = strtoul(p, &p, 10);
                if (*p == '.') {
                    p++;
                    *ret_Patch = strtoul(p, &p, 10);
                    return 0;
                }
            }
        }
        close(fd);
    }
    return -1;
}

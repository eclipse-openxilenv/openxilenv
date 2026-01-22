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
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <string.h>

#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "RemoteMasterGlobalData.h"

#include "MapSocketCAN.h"

static int FoundSocketCANChannelCount;
static int CanSockets[__MAX_SOCKETCANS];

int SocketCANsInit(REMOTE_MASTER_GLOBAL_DATA *GloabalData)
{
    int c;
    for (c = 0; c < __MAX_SOCKETCANS; c++) {   // max. 8 Kanaele
#if 1
        char CmdLine[256];
        int Status;

        PrintFormatToString (CmdLine, sizeof(CmdLine), "ip addr ls dev can%i", c);
        Status = system(CmdLine);
        if (Status != 0) break;
        CanSockets[c] = GloabalData->HwInfosSocketCANs.SocketCAN[c].Socket = c;
#else
        char DeviceName[32];
        int Ret;
        int Socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);

        /* Locate the interface you wish to use */
        struct ifreq ifr;
        PrintFormatToString (DeviceName, sizeof(DeviceName), "can%i", c);
        STRING_COPY_TO_ARRAY(ifr.ifr_name, DeviceName);
        Ret = ioctl(Socket, SIOCGIFINDEX, &ifr); /* ifr.ifr_ifindex gets filled
                                        * with that device's index */
        if (Ret != 0) {
            close(Socket);
            break;

        }
                                        /* Select that CAN interface, and bind the socket to it. */
        struct sockaddr_can addr;
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        if (bind(Socket, (struct sockaddr*)&addr, sizeof(addr))) {
            close(Socket);
            break;
        }
        CanSockets[c] = GloabalData->HwInfosSocketCANs.SocketCAN[c].Socket = Socket;
#endif
    }
    FoundSocketCANChannelCount = c;
    GloabalData->HwInfosSocketCANs.FoundSocketCANs = c;
    return c;
}


int GetNumberOfFoundCanChannelsSocketCAN(void)
{
    return FoundSocketCANChannelCount;
}

int GetCanSocketByChannelNumber(int ChannelNumber)
{
    return CanSockets[ChannelNumber];
}

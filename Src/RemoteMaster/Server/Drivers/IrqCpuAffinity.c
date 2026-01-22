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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "StringMaxChar.h"
#include "PrintFormatToString.h"

int ResetAllIrqToCpu0(void)
{
    int i, fh;
    char Path[256];
    char Data[256];
    for (i = 0; i < 64; i++) {   // are this max. 64?
        PrintFormatToString (Path, sizeof(Path), "/proc/irq/%i/smp_affinity", i);
        fh = open(Path, O_RDWR);
        if (fh > 0) {
            PrintFormatToString (Data, sizeof(Data), "1");
            write(fh, Data, strlen(Data));
            close(fh);
        }
    }
    return 0;
}


int SetIrqToCpu(char *par_NameOfDevice, int par_CpuNumber)
{
    int i, fh;
    char Path[256];
    char Data[256];
    for (i = 0; i < 64; i++) {   // are this max. 64?
        PrintFormatToString (Path, sizeof(Path), "/proc/irq/%i/%s", i, par_NameOfDevice);
        fh = open(Path, O_RDONLY);
        if (fh > 0) {
            close(fh);
            PrintFormatToString (Path,sizeof(Path),  "/proc/irq/%i/smp_affinity", i);
            fh = open(Path, O_RDWR);
            if (fh > 0) {
                PrintFormatToString (Data, sizeof(Data), "%i", 1 << par_CpuNumber);
                write(fh, Data, strlen(Data));
                close(fh);
                printf("set irq affinity of device %s to %X\n", par_NameOfDevice, 1 << par_CpuNumber);
            }
        }
    }
    return 0;
}

int GetEthernetDeviceNameForIpAddress(char *ret_name, int maxc)
{
    struct ifaddrs *if_addrs;
    struct ifaddrs *a;
    char address_string[32];

    if_addrs = NULL;
    getifaddrs(&if_addrs);

    for (a = if_addrs; a != NULL; a = a->ifa_next) {
        if (a->ifa_addr == NULL) continue;
        if (a->ifa_addr->sa_family == AF_INET) {  // IP4
            void *tmp = &((struct sockaddr_in*)a->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmp, address_string, sizeof(address_string));
            printf("%s -> %s\n", a->ifa_name, address_string);
            if (strcmp("127.0.0.1", address_string)) {
                StringCopyMaxCharTruncate(ret_name, a->ifa_name, maxc);
                return 0;
            }
        }
    }
    return -1;
}

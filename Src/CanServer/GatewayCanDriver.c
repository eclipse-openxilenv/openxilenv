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
#include <Windows.h>
#include "ThrowError.h"
#include "StringMaxChar.h"
//#include "MainValues.h"
#include "Scheduler.h"

#include "VirtualCanDriver.h"

#include "GatewayCanDriver.h"

#define UNUSED(x) (void)(x)


static int (__stdcall *CanGatewayDevice_open_can_ptr) (NEW_CAN_SERVER_CONFIG *csc, int channel);

static int (__stdcall *CanGatewayDevice_close_can_ptr) (NEW_CAN_SERVER_CONFIG *csc, int channel);

static int (__stdcall *CanGatewayDevice_ReadNextObjectFromQueue_ptr) (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                                                      uint32_t *pid, unsigned char* data,
                                                                      unsigned char* pext, unsigned char* psize,
                                                                      uint64_t *pTimeStamp);

static int (__stdcall *CanGatewayDevice_read_can_ptr) (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos);

static int (__stdcall *CanGatewayDevice_write_can_ptr) (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos);

static int (__stdcall *CanGatewayDevice_WriteObjectToQueue_ptr) (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                                                 uint32_t id, unsigned char* data,
                                                                 unsigned char ext, unsigned char size);

static int (__stdcall *CanGatewayDevice_status_can_ptr) (NEW_CAN_SERVER_CONFIG *csc, int channel);

static int (__stdcall *InitCanGatewayDevice_ptr)(int par_Flags,
                                                 int (*par_virtdev_open_can)(NEW_CAN_SERVER_CONFIG* csc, int channel),
                                                 int (*par_virtdev_close_can)(NEW_CAN_SERVER_CONFIG* csc, int channel),
                                                 int (*par_virtdev_ReadNextObjectFromQueue)(NEW_CAN_SERVER_CONFIG* csc, int channel,
                                                                                            uint32_t* pid, unsigned char* data,
                                                                                            unsigned char* pext, unsigned char* psize,
                                                                                            uint64_t* pTimeStamp),
                                                 int (*par_virtdev_WriteObjectToQueue)(NEW_CAN_SERVER_CONFIG* csc, int channel,
                                                                                       uint32_t id, unsigned char* data,
                                                                                       unsigned char ext, unsigned char size),
                                                 int (*par_ThrowError)(int level, const char* format, ...),
                                                 uint64_t(*par_GetSimulatedTimeSinceStartedInNanoSecond)(void));

static int (__stdcall *GetCanGatewayDeviceCount_ptr)(void);
static int (__stdcall *GetCanGatewayDeviceInfos_ptr)(int par_Channel, char *ret_Name, int par_MaxChars, uint32_t *ret_DriverVersion, uint32_t *ret_DllVersion, uint32_t *ret_InterfaceVersion);
static int (__stdcall *GetCanGatewayDeviceFdSupport_ptr)(int par_Channel);

static void *GetFuncAddress(HMODULE par_Handle, const char *par_FunctionName)
{
    void* Ret = (void *)GetProcAddress(par_Handle, par_FunctionName);
    if (Ret == NULL) {
        ThrowError (1, "Function %s not found inside dll", par_FunctionName);
    }
    return Ret;
}

int InitCanGatewayDevice(int par_Flags)
{

    HMODULE ModuleDll;
    char* p, DllNameWithPath[MAX_PATH];

    if (InitCanGatewayDevice_ptr != NULL) {
        return 0;   // it is already init
    }

    GetModuleFileNameA(NULL, DllNameWithPath, sizeof(DllNameWithPath));
    p = DllNameWithPath;
    while (*p != 0) p++;
    while ((p > DllNameWithPath) && (*p != '\\')) p--;
    StringCopyMaxCharTruncate(p, "\\..\\CanGatewayDevice.dll", sizeof(DllNameWithPath) - (p - DllNameWithPath));
    ModuleDll = LoadLibraryA(DllNameWithPath);
    if (ModuleDll == NULL) {
        char* lpMsgBuf = NULL;
        DWORD dw = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      dw,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&lpMsgBuf,
                      0, NULL);
        ThrowError(1, "cannot load DLL \"%s\": %s", DllNameWithPath, lpMsgBuf);
        LocalFree(lpMsgBuf);
        return -1;
    }
    if ((CanGatewayDevice_open_can_ptr = GetFuncAddress(ModuleDll, "CanGatewayDevice_open_can")) == NULL) return -1;
    if ((CanGatewayDevice_close_can_ptr = GetFuncAddress(ModuleDll, "CanGatewayDevice_close_can")) == NULL) return -1;
    if ((CanGatewayDevice_ReadNextObjectFromQueue_ptr = GetFuncAddress(ModuleDll, "CanGatewayDevice_ReadNextObjectFromQueue")) == NULL) return -1;
    if ((CanGatewayDevice_read_can_ptr = GetFuncAddress(ModuleDll, "CanGatewayDevice_read_can")) == NULL) return -1;
    if ((CanGatewayDevice_write_can_ptr = GetFuncAddress(ModuleDll, "CanGatewayDevice_write_can")) == NULL) return -1;
    if ((CanGatewayDevice_WriteObjectToQueue_ptr = GetFuncAddress(ModuleDll, "CanGatewayDevice_WriteObjectToQueue")) == NULL) return -1;
    if ((CanGatewayDevice_status_can_ptr = GetFuncAddress(ModuleDll, "CanGatewayDevice_status_can")) == NULL) return -1;
    if ((InitCanGatewayDevice_ptr = GetFuncAddress(ModuleDll, "InitCanGatewayDevice")) == NULL) return -1;
    if ((GetCanGatewayDeviceCount_ptr = GetFuncAddress(ModuleDll, "GetCanGatewayDeviceCount")) == NULL) return -1;
    if ((GetCanGatewayDeviceInfos_ptr = GetFuncAddress(ModuleDll, "GetCanGatewayDeviceInfos")) == NULL) return -1;
    if ((GetCanGatewayDeviceFdSupport_ptr = GetFuncAddress(ModuleDll, "GetCanGatewayDeviceFdSupport")) == NULL) return -1;

    return InitCanGatewayDevice_ptr(par_Flags,
                                    virtdev_open_can,
                                    virtdev_close_can,
                                    virtdev_ReadNextObjectFromQueue,
                                    virtdev_WriteObjectToQueue,
                                    ThrowError,
                                    GetSimulatedTimeSinceStartedInNanoSecond);
}

int GetCanGatewayDeviceCount(void)
{
    if (GetCanGatewayDeviceCount_ptr == NULL) return 0;
    return GetCanGatewayDeviceCount_ptr();
}

int GetCanGatewayDeviceInfos(int par_Channel, char *ret_Name, int par_MaxChars, uint32_t *ret_DriverVersion, uint32_t *ret_DllVersion, uint32_t *ret_InterfaceVersion)
{
    if (GetCanGatewayDeviceInfos_ptr == NULL) return -1;
    return GetCanGatewayDeviceInfos_ptr(par_Channel, ret_Name, par_MaxChars, ret_DriverVersion, ret_DllVersion, ret_InterfaceVersion);
}

int GetCanGatewayDeviceFdSupport(int par_Channel)
{
    if (GetCanGatewayDeviceFdSupport_ptr == NULL) return 0;
    return GetCanGatewayDeviceFdSupport_ptr(par_Channel);
}

int CanGatewayDevice_open_can (NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    if (CanGatewayDevice_open_can_ptr == NULL) return -1;
    return CanGatewayDevice_open_can_ptr(csc, channel);
}

int CanGatewayDevice_close_can (NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    if (CanGatewayDevice_close_can_ptr == NULL) return -1;
    return CanGatewayDevice_close_can_ptr(csc, channel);
}

int CanGatewayDevice_ReadNextObjectFromQueue (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                        uint32_t *pid, unsigned char* data,
                                        unsigned char* pext, unsigned char* psize,
                                        uint64_t *pTimeStamp)
{
    if (CanGatewayDevice_ReadNextObjectFromQueue_ptr == NULL) return -1;
    return CanGatewayDevice_ReadNextObjectFromQueue_ptr(csc, channel,
                                                        pid, data,
                                                        pext, psize,
                                                        pTimeStamp);
}

int CanGatewayDevice_read_can (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos)
{
    if (CanGatewayDevice_read_can_ptr == NULL) return -1;
    return CanGatewayDevice_read_can_ptr(csc, channel, o_pos);
}

int CanGatewayDevice_write_can (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos)
{
    if (CanGatewayDevice_write_can_ptr == NULL) return -1;
    return CanGatewayDevice_write_can_ptr(csc, channel, o_pos);
}

int CanGatewayDevice_WriteObjectToQueue (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                   uint32_t id, unsigned char* data,
                                   unsigned char ext, unsigned char size)
{
    if (CanGatewayDevice_WriteObjectToQueue_ptr == NULL) return -1;
    return CanGatewayDevice_WriteObjectToQueue_ptr (csc, channel,
                                               id, data,
                                               ext, size);
}

int CanGatewayDevice_status_can (NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    if (CanGatewayDevice_status_can_ptr == NULL) return -1;
    return CanGatewayDevice_status_can_ptr (csc, channel);
}

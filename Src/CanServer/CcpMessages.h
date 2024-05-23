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


#ifndef __CCPMSG_H
#define __CCPMSG_H

#include <stdint.h>

//Empfangened Messages:
#define CCP_AKTIVATE                19001
#define CCP_DEAKTIVATE              19002
#define CCP_DTO_ACTIVATE_TIMEOUT    19003
#define CCP_CRO                     19004
typedef struct {
    int Channel;
    uint32_t Id;
    int ExtFlag;
    int Size;
    unsigned char Data[8];
} CCP_CRO_CAN_OBJECT;

#define CCP_SET_CRM_AND_DTOS_IDS    19005
#define CPP_DTOS_DATA_STRUCTS       19006
// Sendende Messages:
#define CCP_CRM_ACK                 19007
#define CCP_DTO_TIMEOUT             19008
#define CCP_SET_ACTIVE_CON_NO       19888
#define CCP_RECEIVE_ALL_VARIABLE_FIRST_TIME 19999

#define CCP_START                   19009
#define CCP_STOP                    19010
#define CCP_INFO                    19011
#define CCP_START_ACK               19109
#define CCP_STOP_ACK                19110
#define CCP_INFO_ACK                19111
#define CCP_REMOVE_BBVARIS          19112


#define CCP_MAX_DTO_PACKAGES        64

typedef struct {
    int Channel;
    int ExtIds;
    int ByteOrder;
    uint32_t CRO_id;       // Command
    uint32_t CRM_id;       // Command ACK
    uint32_t DTO_id;      // Daten
    int Timeout;
    int Prescaler;
    int StationAddress;
    int EventChannel;
    int OnlyBytePointerInDAQ;  // noch nicht implementiert!
    int SeedKeyFlag;
#define CCP_NO_SEED_AND_KEY       0
#define CCP_SEED_AND_KEY          1
#define CCP_SEED_AND_KEY_SIM      3    // Wenn Seed&Key-DLL-String "BB"

    char SeedKeyDll[256];
    char SeedKeyDllCal[256];
    int CalibrationEnable;                
    int MoveROM2RAM;
    int SelCalPage;
    uint32_t CalibROMStartAddr;   // zB: EB1 0xA0000
    uint32_t CalibRAMStartAddr;   // zB: EB1 0x220000
    uint32_t CalibROMRAMSize;     // zB: EB1 0x20000
    int ReadParameterAfterCalib;
    int CcpVersion;  
    int XcpOrCcp;     // 1 -> XCP 0 -> CCP
    int DebugMessages;
    struct {
        int SetCalPage;
        unsigned char CalibROMSegment;
        unsigned char CalibROMPageNo;
        unsigned char CalibRAMSegment;
        unsigned char CalibRAMPageNo;
    } Xcp;
    int UseUnitFlag;
    int UseConvFlag;
    int UseMinMaxFlag;
} CCP_CONFIG;

typedef struct {
/* 4 */     int VarCount;          // 0...7
            struct {
/* 1 */         unsigned char ByteOffset;
/* 1 */         unsigned char Size;
/* 2 */         char Filler[2];
/* 4 */         int Type;
/* 4 */         uint32_t Address;
/* 4 */         int Vid;
/* 112 */   } Entrys[7];
/* 4 */     int Timeout;
            char Filler [128-120];
} CCP_DTO_PACKAGE;

typedef struct {
   int DTOPackagesCount;
   CCP_DTO_PACKAGE DTO_Packages[CCP_MAX_DTO_PACKAGES];
   //int Timeouts[CCP_MAX_DTO_PACKAGES];
   char Data[1];       //  Umrechnungen aller in DTOs enthaltenen Variablen
                       //  ist auf jeden Fall groesser als 1 Byte!!
                       //  wird zZ nicht verwendet!
} CCP_VARIABLES_CONFIG;



typedef struct {
    unsigned char major_version;
    unsigned char minor_version;
    char TCU_ID[256];
    unsigned char ResourceAvailabilityMask;
    unsigned char ResourceProtectionMask;
    int DAQ_Size;
    int DAQ_Pid;
} CCP_INFOS;

#endif

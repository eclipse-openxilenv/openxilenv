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


#include "Config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "Message.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "IniDataBase.h"
#include "Files.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "EnvironmentVariables.h"
#include "LoadSaveToFile.h"
#include "ConfigurablePrefix.h"
#include "CcpMessages.h"
#include "XcpControl.h"

#define UNUSED(x) (void)(x)

#define CCP_CTR                  0xAA

#define ALL_PARAM_READ           0
#define MORE_PARAM_READ_PENDING  1


typedef struct _XcpCalParamItem {
    int vid;
    uint32_t Address;
} XcpCalParamItem;


typedef struct {
// This must be set to true after start of a calibration session
// to force a update of all selected parameters
    int XcpUploadInProgress;

// Is it a LSB or MSB first target
    int XcpByteOrder;       // 0 == LSB first 1 == MSB first

// Which protocol version should be used
    int gXcpVersion;

// XCP Seed and the protection state
#define MAX_SEED_LENGHT 255 // should be not larger as 255
#define MAX_KEY_LENGHT 255  // should be not larger as 255
    unsigned char gXcpSeed[MAX_SEED_LENGHT];
    unsigned char gXcpSeedLength;
    unsigned char gXcpSeedRemaining;
    unsigned char gXcpKey[MAX_KEY_LENGHT];
    unsigned char gXcpKeyLength;
    unsigned char gXcpProtectionStatus;
    unsigned char gXcpSeedCounter;

    unsigned char gXcpKeyCounter;

// Priviledge Level fuer Seed & Key
#define PL_CAL  0x01
#define PL_DAQ  0x04
#define PL_STIM 0x08
#define PL_PGM  0x10

// State of the connection
    int gXcpConnected;
#define CONNECTED_MASK              0x00000001
#define MESSUREMENT_RUNNING_MASK    0x00000002
#define CALIBRATION_ACTIVE_MASK     0x00000004
    
    int gXcpCALAviable;
    int gXcpDAQAviable;
    int XcpMaxCTOs;
    int XcpMaxDTOs;

    uint64_t AlivePingTimeout;
    uint64_t AlivePingCounter;

// Base configuration
    CCP_CONFIG XcpConfig;

// This structure store the assembling of the data transmition objects (DTO)
    CCP_VARIABLES_CONFIG dto_conf;

    CCP_INFOS ccp_infos;
    int XcpErrCode;

    VID XCPCommandVid;
    VID XCPCommandSequenceVid;
    VID XCPStatusVid;

    int XcpCalParamListSize;
    int XcpCalParamPos;

    XcpCalParamItem *XcpCalParamList;

    uint64_t TimeoutCounter;

//- XCP state variables ----------------------------------------------
    int XcpCommandSequence;
    int XcpCommandSequenceRequest;
#define XCP_NO_COMMAND_SEQUENCE             0
#define XCP_READ_TCU_INFO                   1
#define XCP_START_MEASUREMENT               2 
#define XCP_STOP_MEASUREMENT                3  
#define XCP_READ_MEMORY                     4 
#define XCP_DOWNLOAD_SEQUENCE               5
#define XCP_UPLOAD_SEQUENCE                 6
#define XCP_START_CALIBRATION               7
#define XCP_STOP_CALIBRATION                8
#define XCP_ALIFE_PING                      9

    int XcpCommand;
#define XCP_NO_COMMAND                          0
#define XCP_GET_ID                              1
#define XCP_GET_ID_WAIT_ACK                     2
#define XCP_UPLOAD                              3
#define XCP_UPLOAD_WAIT_ACK                     4
#define XCP_READ_TCU_INFO_FINISHED              5
#define XCP_CONNECT                             6 
#define XCP_CONNECT_WAIT_ACK                    7 
#define XCP_GET_DAQ_PROCESSOR_INFO              8
#define XCP_GET_DAQ_PROCESSOR_INFO_WAIT_ACK     9 
#define XCP_GET_DAQ_RESOLUTION_INFO            10
#define XCP_GET_DAQ_RESOLUTION_INFO_WAIT_ACK   11
#define XCP_FREE_DAQ                           12
#define XCP_FREE_DAQ_WAIT_ACK                  13
#define XCP_ALLOC_DAQ                          14
#define XCP_ALLOC_DAQ_WAIT_ACK                 15
#define XCP_ALLOC_ODT                          16
#define XCP_ALLOC_ODT_WAIT_ACK                 17
#define XCP_ALLOC_ODT_ENTRY                    18
#define XCP_ALLOC_ODT_ENTRY_WAIT_ACK           19
#define XCP_SET_DAQ_PTR                        20
#define XCP_SET_DAQ_PTR_WAIT_ACK               21
#define XCP_WRITE_DAQ                          22
#define XCP_WRITE_DAQ_WAIT_ACK                 23
#define XCP_SET_DAQ_LIST_MODE                  24
#define XCP_SET_DAQ_LIST_MODE_WAIT_ACK         25
#define XCP_START_STOP_DAQ_LIST                26
#define XCP_START_STOP_DAQ_LIST_WAIT_ACK       27
#define XCP_GET_DAQ_CLOCK                      28
#define XCP_GET_DAQ_CLOCK_WAIT_ACK             29
#define XCP_START_STOP_SYNCH                   30
#define XCP_START_STOP_SYNCH_WAIT_ACK          31
#define XCP_DISCONNECT                         32
#define XCP_DISCONNECT_WAIT_ACK                33
#define XCP_BUILD_DTOS                         34
#define XCP_START_MEASUREMENT_FINISHED         35
#define XCP_STOP_MEASUREMENT_FINISHED          36
#define XCP_SET_MTA                            37
#define XCP_SET_MTA_WAIT_ACK                   38
#define XCP_DATA_DOWNLOAD                      39
#define XCP_DATA_DOWNLOAD_WAIT_ACK             40
#define XCP_START_CALIBRATION_FINISHED         41
#define XCP_STOP_CALIBRATION_FINISHED          42
#define XCP_COPY_CAL_PAGE                      43
#define XCP_COPY_CAL_PAGE_ACK                  44
#define XCP_SET_CAL_PAGE                       45
#define XCP_SET_CAL_PAGE_ACK                   46
#define XCP_WAIT_FIRST_DATA                    47
#define XCP_STOP_DAQ_LIST                      48
#define XCP_STOP_DAQ_LIST_WAIT_ACK             49
#define XCP_STOP_SYNCH                         50
#define XCP_STOP_SYNCH_WAIT_ACK                51
#define XCP_GET_STATUS                         52
#define XCP_GET_STATUS_WAIT_ACK                53
#define XCP_GET_SEED                           54
#define XCP_GET_SEED_WAIT_ACK                  55
#define XCP_UNLOCK                             56
#define XCP_UNLOCK_WAIT_ACK                    57
#define XCP_CONNECTED                          58
#define XCP_COMMAND_STARTPOINT                100

    char **CcpMeasurementLabelList;
    int CcpMeasurementLabelListSize;
    char *CcpMeasurementLabelListBuffer;

    int WaitForAnAckFlag;      // 0 do not expect an ACK
                               // 1 now qait for an ACK
                               // 2 ACK was detected by the message filter
    unsigned char AckCanData[8];

    // Calibration
#define XCP_MAX_DOWNLOAD_SIZE     8
    unsigned char CalData[XCP_MAX_DOWNLOAD_SIZE];  // Max. double
    int CalSize;
    uint32_t CalAddress;
    int CalPos;
    
    // Position of the current downloding parameter
    int UploadXcpCalParamPos;

    int OdtPtr;  // o
    int DaqPtr;  // i
    uint32_t SizeOfTCUString;
    uint32_t TCUStringPointer;

} XCP_CONNECTTION;

static XCP_CONNECTTION XcpConnections[4];




void XCPDownload_CmdScheduler (XCP_CONNECTTION *pCon, int Connection);
void XCPUpload_CmdScheduler (XCP_CONNECTTION *pCon, int Connection);
void XCPWriteCharacteristics (XCP_CONNECTTION *pCon);
int XCPReadCharacteristics (XCP_CONNECTTION *pCon);




#define SWAP_TO_MSB_FIRST(p,n) if (pCon->XcpByteOrder) SwapToMsbFirst(p,n);

static void SwapToMsbFirst (void *p0, unsigned int n)
{
    unsigned char b;
    unsigned char *p = (unsigned char*)p0;
    switch (n) {
    case 2:
        b = p[0]; p[0] = p[1]; p[1] = b;
        break;
    case 4:
        b = p[3]; p[3] = p[0]; p[0] = b;
        b = p[2]; p[2] = p[1]; p[1] = b;
        break;
    case 8:
        b = p[7]; p[7] = p[0]; p[0] = b;
        b = p[6]; p[6] = p[1]; p[1] = b;
        b = p[5]; p[5] = p[2]; p[2] = b;
        b = p[4]; p[4] = p[3]; p[3] = b;
        break;
    }
}


/* XCP-Commands */
#define XCP_PACKET_TYPE_CONNECT                        0xFF
#define XCP_PACKET_TYPE_DISCONNECT                     0xFE
#define XCP_PACKET_TYPE_GET_ID                         0xFA
#define XCP_PACKET_TYPE_UPLOAD                         0xF5
#define XCP_PACKET_TYPE_GET_DAQ_PROCESSOR_INFO         0xDA
#define XCP_PACKET_TYPE_GET_DAQ_RESOLUTION_INFO        0xD9
#define XCP_PACKET_TYPE_FREE_DAQ                       0xD6
#define XCP_PACKET_TYPE_ALLOC_DAQ                      0xD5
#define XCP_PACKET_TYPE_ALLOC_ODT                      0xD4
#define XCP_PACKET_TYPE_ALLOC_ODT_ENTRY                0xD3
#define XCP_PACKET_TYPE_SET_DAQ_PTR                    0xE2
#define XCP_PACKET_TYPE_WRITE_DAQ                      0xE1
#define XCP_PACKET_SET_DAQ_LIST_MODE                   0xE0
#define XCP_PACKET_START_STOP_DAQ_LIST                 0xDE
#define XCP_PACKET_START_STOP_SYNC                     0xDD
#define XCP_PACKET_GET_DAQ_CLOCK                       0xDC
#define XCP_PACKET_SET_MTA                             0xF6
#define XCP_PACKET_DOWNLOAD                            0xF0
#define XCP_PACKET_UPLOAD                              0xF5
#define XCP_PACKET_SET_CAL_PAGE                        0xEB
#define XCP_PACKET_COPY_CAL_PAGE                       0xE4
#define XCP_PACKET_GET_STATUS                          0xFD
#define XCP_PACKET_GET_SEED                            0xF8
#define XCP_PACKET_UNLOCK                              0xF7


#define ERR_CMD_SYNCH         0x00
#define ERR_CMD_BUSY          0x10
#define ERR_DAQ_ACTIVE        0x11
#define ERR_PGM_ACTIVE        0x12
#define ERR_CMD_UNKNOWN       0x20 
#define ERR_CMD_SYNTAX        0x21
#define ERR_OUT_OF_RANGE      0x22
#define ERR_WRITE_PROTECTED   0x23 
#define ERR_ACCESS_DENIED     0x24
#define ERR_ACCESS_LOCKED     0x25 
#define ERR_PAGE_NOT_VALID    0x26 
#define ERR_MODE_NOT_VALID    0x27 
#define ERR_SEGMENT_NOT_VALID 0x28 
#define ERR_SEQUENCE          0x29 
#define ERR_DAQ_CONFIG        0x2A 
#define ERR_MEMORY_OVERFLOW   0x30 
#define ERR_GENERIC           0x31 
#define ERR_VERIFY            0x32

static struct {
    int ErrCode;
    char *ErrText;
} XcpErrCodeTab[] = {
    {-1, "Timeout"},
    {ERR_CMD_SYNCH, "Command processor synchronization"},
    {ERR_CMD_BUSY, "Command was not executed"},
    {ERR_DAQ_ACTIVE, "Command rejected because DAQ is running"},
    {ERR_PGM_ACTIVE, "Command rejected because PGM is running"},
    {ERR_CMD_UNKNOWN, "Unknown command or not implemented optional command"},
    {ERR_CMD_SYNTAX, "Command syntax invalid"},
    {ERR_OUT_OF_RANGE, "Command syntax valid but command parameter(s) out of range"},
    {ERR_WRITE_PROTECTED, "The memory location is write protected"},
    {ERR_ACCESS_DENIED, "The memory location is not accessible"},
    {ERR_ACCESS_LOCKED, "Access denied, Seed & Key is required"},
    {ERR_PAGE_NOT_VALID, "Selected page not available"},
    {ERR_MODE_NOT_VALID, "Selected page mode not available"},
    {ERR_SEGMENT_NOT_VALID, "Selected segment not valid"},
    {ERR_SEQUENCE, "Sequence error"},
    {ERR_DAQ_CONFIG, "DAQ configuration not valid"},
    {ERR_MEMORY_OVERFLOW, "Memory overflow error"},
    {ERR_GENERIC, "Generic error"},
    {ERR_VERIFY, "The slave internal program verify routine detects an error"}};


static char *XcpErrorStr (int ErrCode) 
{
    int x;

    for (x = 0; x < (int)(sizeof (XcpErrCodeTab) / sizeof (XcpErrCodeTab[0])); x++) {
        if (ErrCode == XcpErrCodeTab[x].ErrCode) return XcpErrCodeTab[x].ErrText;
    }
    return "Unknown XCP Errorcode";
}

static void FillCanObject (CCP_CRO_CAN_OBJECT *pCanObj, XCP_CONNECTTION *pCon)
{
    MEMSET (pCanObj->Data, 0, 8);
    pCanObj->Channel = pCon->XcpConfig.Channel;
    pCanObj->Id = pCon->XcpConfig.CRO_id;
    pCanObj->ExtFlag = pCon->XcpConfig.ExtIds;
    pCanObj->Size = 8;
}

static void DebugErrorMessage (CCP_CRO_CAN_OBJECT *pCanObj, char *Text, XCP_CONNECTTION *pCon)
{
    if (pCon->XcpConfig.DebugMessages) {
        ThrowError (INFO_NO_STOP, "%s: (%i/0x%X) %02X %02X %02X %02X  %02X %02X %02X %02X", 
               Text, pCanObj->Channel, pCanObj->Id,
               (int)pCanObj->Data[0], (int)pCanObj->Data[1], (int)pCanObj->Data[2], (int)pCanObj->Data[3], 
               (int)pCanObj->Data[4], (int)pCanObj->Data[5], (int)pCanObj->Data[6], (int)pCanObj->Data[7]);
    }
}

static void TransmitCROMessage (CCP_CRO_CAN_OBJECT *pCanObj)
{
    write_message (get_pid_by_name ("CANServer"), CCP_CRO, sizeof (CCP_CRO_CAN_OBJECT), (char*)pCanObj);
}

/* tansmit functionen for XCP commands */

// Upload a data block unlimited lengths
// MTA have to be set before
static int xcpUpload (int size, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_UPLOAD;
    CanObject.Data[1] = (unsigned char)size;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "UPLOAD", pCon);
    return 1;
}


// Request for identifikation
static int xcpGetId (XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_GET_ID;
    CanObject.Data[1] = 0x01;  // ASAM MC 2 filename without path and extension
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "GET_ID", pCon);
    return 1;
}

// Connect
static int xcpConnect (XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_CONNECT;
    CanObject.Data[1] = 0;    // NORMAL
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "CONNECT", pCon);
    return 1;
}

// Disconnect
int xcpDisconnect (XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_DISCONNECT;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "DISCONNECT", pCon);
    return 1;
}


//------------------------------------------------------------------------------
// DAQ

int xcpGetDaqProcessorInfo (XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_GET_DAQ_PROCESSOR_INFO;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "GET_DAQ_PROCESSOR_INFO", pCon);
    return 1;
}

int xcpGetDaqResolutionInfo (XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_GET_DAQ_RESOLUTION_INFO;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "GET_RESOLUTION_INFO", pCon);
    return 1;
}

int xcpFreeDaq (XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_FREE_DAQ;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "FREE_DAQ", pCon);
    return 1;
}

int xcpAllocDaq (int DaqCount, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_ALLOC_DAQ;
    *(unsigned short*)(void*)&CanObject.Data[2] = (unsigned short)DaqCount;
    SWAP_TO_MSB_FIRST (&CanObject.Data[2], 2);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "ALLOC_DAQ", pCon);
    return 1;
}

int xcpAllocOdt (int DaqNr, int OdtCount, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_ALLOC_ODT;
    *(unsigned short*)(void*)&CanObject.Data[2] = (unsigned short)DaqNr;
    SWAP_TO_MSB_FIRST (&CanObject.Data[2], 2);
    CanObject.Data[4] = (unsigned char)OdtCount;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "ALLOC_ODT", pCon);
    return 1;
}

int xcpAllocOdtEntry (int DaqNr, int OdtNr, int OdtEntryCount, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_ALLOC_ODT_ENTRY;
    *(unsigned short*)(void*)&CanObject.Data[2] = (unsigned short)DaqNr;
    SWAP_TO_MSB_FIRST (&CanObject.Data[2], 2);
    CanObject.Data[4] = (unsigned char)OdtNr;
    CanObject.Data[5] = (unsigned char)OdtEntryCount;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "ALLOC_ODT_ENTRY", pCon);
    return 1;
}

int xcpSetDaqPtr (int DaqNr, int OdtNr, int OdtEntryNr, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_SET_DAQ_PTR;
    *(unsigned short*)(void*)&CanObject.Data[2] = (unsigned short)DaqNr;
    SWAP_TO_MSB_FIRST (&CanObject.Data[2], 2);
    CanObject.Data[4] = (unsigned char)OdtNr;
    CanObject.Data[5] = (unsigned char)OdtEntryNr;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "SET_DAQ_PTR", pCon);
    return 1;
}

int xcpWriteDaq (int BitOffset, int SizeOfElem, int AddrExt, uint32_t Addr, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_TYPE_WRITE_DAQ;
    CanObject.Data[1] = (unsigned char)BitOffset;
    CanObject.Data[2] = (unsigned char)SizeOfElem;
    CanObject.Data[3] = (unsigned char)AddrExt;
    *(uint32_t*)(void*)&CanObject.Data[4] = Addr;
    SWAP_TO_MSB_FIRST (&CanObject.Data[4], 4);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "WRITE_DAQ", pCon);
    return 1;
}

int xcpSetDaqListMode (int Mode, int DaqNr, int EventNr, int Prescaler, int Priority, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_SET_DAQ_LIST_MODE;
    CanObject.Data[1] = (unsigned char)Mode;
    *(uint16_t*)(void*)&CanObject.Data[2] = (uint16_t)DaqNr;
    SWAP_TO_MSB_FIRST (&CanObject.Data[2], 2);
    *(uint16_t*)(void*)&CanObject.Data[4] = (uint16_t)EventNr;
    SWAP_TO_MSB_FIRST (&CanObject.Data[4], 2);
    CanObject.Data[6] = (unsigned char)Prescaler;
    CanObject.Data[7] = (unsigned char)Priority;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "SET_DAQ_LIST_MODE", pCon);
    return 1;
}

int xcpStartStopDaqList (int Mode, int DaqNr, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_START_STOP_DAQ_LIST;
    CanObject.Data[1] = (unsigned char)Mode;
    *(uint16_t*)(void*)&CanObject.Data[2] = (uint16_t)DaqNr;
    SWAP_TO_MSB_FIRST (&CanObject.Data[2], 2);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "START_STOP_DAQ_LIST", pCon);
    return 1;
}

int xcpStartStopSync (int Mode, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_START_STOP_SYNC;
    CanObject.Data[1] = (unsigned char)Mode;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "START_STOP_SYNCH", pCon);
    return 1;
}

// Calibration

int xcpSetMta (uint32_t Address, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_SET_MTA;
    *(uint32_t*)(void*)&CanObject.Data[4] = Address;
    SWAP_TO_MSB_FIRST (&CanObject.Data[4], 4);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "SET_MTA", pCon);
    return 1;
}

static int xcpDownload (const unsigned char *Data, int Size, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_DOWNLOAD;
    CanObject.Data[1] = (unsigned char)Size;
    MEMCPY ((void*)&CanObject.Data[2], (const void*)Data, (size_t)Size);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "DOWNLOAD", pCon);
    return 1;
}

static int xcpCopyCalPage (const unsigned char SrcSegment, const unsigned char SrcPage,
                           const unsigned char DstSegment, const unsigned char DstPage,
                           XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_COPY_CAL_PAGE;
    CanObject.Data[1] = SrcSegment;
    CanObject.Data[2] = SrcPage;
    CanObject.Data[3] = DstSegment;
    CanObject.Data[4] = DstPage;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "COPY_CAL_PAGE", pCon);
    return 1;
}

static int xcpSetCalPage (const unsigned char CalSegment, const unsigned char CalPage,
                           XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_SET_CAL_PAGE;
    CanObject.Data[1] = 3;  // Mode = XCP und ECU
    CanObject.Data[2] = CalSegment;
    CanObject.Data[3] = CalPage;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "SET_CAL_PAGE", pCon);
    return 1;
}

static int xcpGetStatus (XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_GET_STATUS;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "GET_STATUS", pCon);
    return 1;
}

static int xcpGetSeed (unsigned char ResourceProtectionStatus, unsigned char Mode, XCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_GET_SEED;
    CanObject.Data[1] = Mode; 
    CanObject.Data[2] = ResourceProtectionStatus;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "GET_SEED", pCon);
    return 1;
}

static int xcpUnlock (XCP_CONNECTTION *pCon, unsigned char privilege)
{
    UNUSED(privilege);
    CCP_CRO_CAN_OBJECT CanObject;
    int i;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = XCP_PACKET_UNLOCK;
    CanObject.Data[1] = pCon->gXcpKeyLength - pCon->gXcpKeyCounter;
    for (i=2; (i < 8) && (pCon->gXcpKeyCounter < MAX_KEY_LENGHT) && (pCon->gXcpKeyCounter < pCon->gXcpKeyLength); i++, pCon->gXcpKeyCounter++) {
        CanObject.Data[i] = pCon->gXcpKey[pCon->gXcpKeyCounter]; 
    }
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (&CanObject, "UNLOCK", pCon);
    return 1;
}


#ifdef _WIN32
typedef unsigned long (__cdecl* XCP_COMPUTEKEYFROMSEED) (unsigned char privilege,
                                                         unsigned char byteLenSeed,
                                                         unsigned char *seed,
                                                         unsigned char *byteLenKey,
                                                         unsigned char *key);
#endif


static int xcpCalcKey (XCP_CONNECTTION *pCon, unsigned char privilege)
{
#ifdef _WIN32
    HANDLE hLibrary;
    unsigned long Ret;

    hLibrary = LoadLibrary (pCon->XcpConfig.SeedKeyDll);
    if (hLibrary == NULL)  {
        ThrowError (1, "cannot load XCP Seed&Key DLL \"%s\"", pCon->XcpConfig.SeedKeyDll);
    } else  {
        XCP_COMPUTEKEYFROMSEED XCP_ComputeKeyFromSeed;
        XCP_ComputeKeyFromSeed = (XCP_COMPUTEKEYFROMSEED)GetProcAddress (hLibrary, "XCP_ComputeKeyFromSeed");
        if (XCP_ComputeKeyFromSeed == NULL) {
            ThrowError (1, "there is no function XCP_ComputeKeyFromSeed in (%s) DLL", pCon->XcpConfig.SeedKeyDll);
        } else {
            pCon->gXcpKeyLength = (unsigned char)MAX_SEED_LENGHT;
            Ret = XCP_ComputeKeyFromSeed (privilege, pCon->gXcpSeedLength, pCon->gXcpSeed, &(pCon->gXcpKeyLength), pCon->gXcpKey);
            switch (Ret) {
              case 0: // o.k.
                //ThrowError (1, "XCP Seed&Key function XCP_ComputeKeyFromSeed (%s) returned successfully", pCon->XcpConfig.SeedKeyDll);
                break;
              case 1:
                ThrowError (1, "XCP Seed&Key function XCP_ComputeKeyFromSeed (%s) failed:\nthe requested privilege can not be unlocked with this DLL", pCon->XcpConfig.SeedKeyDll);
                return 0;
              case 2:
                ThrowError (1, "XCP Seed&Key function XCP_ComputeKeyFromSeed (%s) failed:\nthe seed length is wrong, key could not be computed", pCon->XcpConfig.SeedKeyDll);
                return 0;
              case 3:
                ThrowError (1, "XCP Seed&Key function XCP_ComputeKeyFromSeed (%s) failed:\nthe space for the key is too small", pCon->XcpConfig.SeedKeyDll);
                return 0;
              default:
                ThrowError (1, "XCP Seed&Key function XCP_ComputeKeyFromSeed (%s) failed:\nunknown error", pCon->XcpConfig.SeedKeyDll);
                return 0;
            }
        }
        FreeLibrary (hLibrary);
    }

    if (pCon->gXcpKeyLength > MAX_KEY_LENGHT) {
        ThrowError (1, "Actually only keys with maximun size of %d bytes allowed! But %d bytes are requested.", MAX_KEY_LENGHT, (int)pCon->gXcpKeyLength);
        return 0;
    }
    pCon->gXcpKeyCounter = 0;
    return 1;
#else
    UNUSED(pCon);
    UNUSED(privilege);
    // TODO
    ThrowError(1, "not implementd yet");
    return -1;
#endif
}




static int XcpReadConfigFromIni (int Connection)
{
    char txt[32];
    char Section[256];
    XCP_CONNECTTION *pCon;
    int Fd = GetMainFileDescriptor();

    pCon = &XcpConnections[Connection];
    PrintFormatToString (Section, sizeof(Section), "XCP Configuration for Target %i", Connection);
    IniFileDataBaseReadString (Section, "DebugMessages", "no", txt, sizeof (txt), Fd);
    if (!strcmpi ("yes", txt)) pCon->XcpConfig.DebugMessages = 1;
    else pCon->XcpConfig.DebugMessages = 0;
    pCon->XcpConfig.Channel = IniFileDataBaseReadInt (Section, "Channel", 0, Fd);
    IniFileDataBaseReadString (Section, "ByteOrder", "", txt, sizeof (txt), Fd);
    if (!strcmpi ("msb_first", txt)) pCon->XcpConfig.ByteOrder = 1;
    else pCon->XcpConfig.ByteOrder = 0;
    pCon->XcpByteOrder = pCon->XcpConfig.ByteOrder;
    IniFileDataBaseReadString (Section, "CRO_ID", "0", txt, sizeof (txt), Fd);
    pCon->XcpConfig.CRO_id = strtoul (txt, NULL, 0);
    IniFileDataBaseReadString (Section, "CRM_ID", "0", txt, sizeof (txt), Fd);
    pCon->XcpConfig.CRM_id = strtoul (txt, NULL, 0);
    IniFileDataBaseReadString (Section, "DTO_ID", "0", txt, sizeof (txt), Fd);
    pCon->XcpConfig.DTO_id = strtoul (txt, NULL, 0);
    pCon->XcpConfig.ExtIds = IniFileDataBaseReadInt (Section, "ExtIds", 0, Fd);
    pCon->XcpConfig.Timeout = IniFileDataBaseReadInt (Section, "Timeout", 500, Fd);  // in ms
    IniFileDataBaseReadString (Section, "StationAddress", "0x0039", txt, sizeof (txt), Fd);
    pCon->XcpConfig.StationAddress = strtol (txt, NULL, 0);
    pCon->XcpConfig.Prescaler = IniFileDataBaseReadInt (Section, "Prescaler", 1, Fd);
    pCon->XcpConfig.EventChannel = IniFileDataBaseReadInt (Section, "EventChannel", 0, Fd);
    IniFileDataBaseReadString (Section, "OnlyBytePointerInDAQ", "no", txt, sizeof(txt), Fd);
    pCon->XcpConfig.OnlyBytePointerInDAQ = !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "SeedKeyDll", "", pCon->XcpConfig.SeedKeyDll, sizeof (pCon->XcpConfig.SeedKeyDll), Fd);
    if (strlen (pCon->XcpConfig.SeedKeyDll)) {  // If there are a DLL Seed&Key are active
        pCon->XcpConfig.SeedKeyFlag = CCP_SEED_AND_KEY;
    } else {
        pCon->XcpConfig.SeedKeyFlag = CCP_NO_SEED_AND_KEY;
    }
    IniFileDataBaseReadString (Section, "CalibrationEnable", "no", txt, sizeof(txt), Fd);
    pCon->XcpConfig.CalibrationEnable = !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "MoveROM2RAM", "no", txt, sizeof(txt), Fd);
    pCon->XcpConfig.MoveROM2RAM =  !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "SelCalPage", "no", txt, sizeof(txt), Fd);
    pCon->XcpConfig.Xcp.SetCalPage =  !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "CalibROMSegment", "0x0", txt, sizeof (txt), Fd);
    pCon->XcpConfig.Xcp.CalibROMSegment = (unsigned char)strtoul (txt, NULL, 0);
    IniFileDataBaseReadString (Section, "CalibROMPageNo", "0x0", txt, sizeof (txt), Fd);
    pCon->XcpConfig.Xcp.CalibROMPageNo = (unsigned char)strtoul (txt, NULL, 0);
    IniFileDataBaseReadString (Section, "CalibRAMSegment", "0x0", txt, sizeof (txt), Fd);
    pCon->XcpConfig.Xcp.CalibRAMSegment = (unsigned char)strtoul (txt, NULL, 0);
    IniFileDataBaseReadString (Section, "CalibRAMPageNo", "0x0", txt, sizeof (txt), Fd);
    pCon->XcpConfig.Xcp.CalibRAMPageNo = (unsigned char)strtoul (txt, NULL, 0);
    pCon->XcpConfig.ReadParameterAfterCalib = IniFileDataBaseReadInt (Section, "ReadParameterAfterCalib", 0, Fd);

    IniFileDataBaseReadString (Section, "UseUnit", "no", txt, sizeof(txt), Fd);
    pCon->XcpConfig.UseUnitFlag = !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "UseConversion", "no", txt, sizeof(txt), Fd);
    pCon->XcpConfig.UseConvFlag = !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "UseMinMax", "no", txt, sizeof(txt), Fd);
    pCon->XcpConfig.UseMinMaxFlag = !strcmp ("yes", txt);

    pCon->XcpConfig.XcpOrCcp = 1; // It is aXCP connection
    write_message (get_pid_by_name ("CANServer"), CCP_SET_ACTIVE_CON_NO, sizeof (int), (char*)&Connection);
    write_message (get_pid_by_name ("CANServer"), CCP_SET_CRM_AND_DTOS_IDS, sizeof (pCon->XcpConfig), (char*)&pCon->XcpConfig);
    return 0;
}


static int get_bbvari_sizeof (char *dtype)
{
    if (!strcmp (dtype, "BYTE"))
        return 1;
    if (!strcmp (dtype, "UBYTE")) 
        return 1;
    if (!strcmp (dtype, "WORD"))
        return 2;
    if (!strcmp (dtype, "UWORD"))
        return 2;
    if (!strcmp (dtype, "DWORD"))
        return 4;
    if (!strcmp (dtype, "UDWORD"))
        return 4;
    if (!strcmp (dtype, "FLOAT"))
        return 4;
    return 0;
}

static int get_bbvari_type (char *dtype)
{
    if (!strcmp (dtype, "BYTE"))
        return BB_BYTE;
    if (!strcmp (dtype, "UBYTE")) 
        return BB_UBYTE;
    if (!strcmp (dtype, "WORD"))
        return BB_WORD;
    if (!strcmp (dtype, "UWORD"))
        return BB_UWORD;
    if (!strcmp (dtype, "DWORD"))
        return BB_DWORD;
    if (!strcmp (dtype, "UDWORD"))
        return BB_UDWORD;
    if (!strcmp (dtype, "FLOAT"))
        return BB_FLOAT;
    return 0;
}

static int DeleteCalibrationList (int Connection)
{
    XCP_CONNECTTION *pCon;
    int x;

    pCon = &XcpConnections[Connection];
    if (pCon->XcpCalParamList != NULL) {
        for (x = 0; x < pCon->XcpCalParamListSize; x++) {
            if (pCon->XcpCalParamList[x].vid > 0) {
                remove_bbvari (pCon->XcpCalParamList[x].vid);
            }
        }
        my_free (pCon->XcpCalParamList);
        pCon->XcpCalParamList = NULL;
        pCon->XcpCalParamListSize = 0;
        return 0;
    }
    return -1;
}

static int BuildCalibrationList (int par_ParamCount, char **par_Params, int par_Connection)
{
    int x, p;
    char Section[256];
    char txt[INI_MAX_LINE_LENGTH];
    char entry[32];
    char *dtype_str;
    char *name;
    char *address_str;
    int dtype;
    int found;
    XCP_CONNECTTION *pCon;
    char *Unit, *Conversion, *MinString, *MaxString;
    int Fd = GetMainFileDescriptor();
    int *ParamIdxs = NULL;
    int Ret = 0;

    pCon = &XcpConnections[par_Connection];
    PrintFormatToString (Section, sizeof(Section), "XCP Configuration for Target %i", par_Connection);
    DeleteCalibrationList (par_Connection);
    pCon->XcpCalParamList = (XcpCalParamItem*)my_malloc ((size_t)par_ParamCount * sizeof (XcpCalParamItem));
    if (pCon->XcpCalParamList == NULL) {
        ThrowError (1, "out of memory");
        return -1;
    }
    ParamIdxs = (int*)my_calloc ((size_t)par_ParamCount, sizeof (int));
    if (ParamIdxs == NULL) {
        ThrowError (1, "out of memory");
        return -1;
    }
    for (x = 0; x < par_ParamCount; x++) {
        ParamIdxs[x] = -1;
    }
    // first search all parameters and store it's index
    for (p = 0; ; p++) {
        PrintFormatToString (entry, sizeof(entry), "p%i", p);
        if (IniFileDataBaseReadString(Section, entry, "", txt, sizeof (txt), Fd) == 0) {
            break;
        }
        if (StringCommaSeparate (txt, &dtype_str, &name, NULL) == 2) {
            for (x = 0; x < par_ParamCount; x++) {
                if (!strcmp (name, par_Params[x])) {
                    ParamIdxs[x] = p;
                    break;
                }
            }
        }
    }
    pCon->XcpCalParamListSize = par_ParamCount;
    pCon->XcpCalParamPos = 0;
    for (x = 0; x < par_ParamCount; x++) {
        found = 0;
        if (ParamIdxs[x] >= 0) {
            p = ParamIdxs[x];
            PrintFormatToString (entry, sizeof(entry), "p%i", p);
            if (IniFileDataBaseReadString(Section, entry, "", txt, sizeof (txt), Fd) == 0) break;
            Unit = Conversion = MinString = MaxString = NULL;
            if (StringCommaSeparate (txt, &dtype_str, &name, &address_str, &Unit, &Conversion, &MinString, &MaxString, NULL) >= 3) {
                if (!strcmp (name, par_Params[x])) {
                    dtype = get_bbvari_type (dtype_str);
                    pCon->XcpCalParamList[x].Address = strtoul (address_str, NULL, 0);
                    pCon->XcpCalParamList[x].vid = add_bbvari (name, dtype,
                                                               (pCon->XcpConfig.UseUnitFlag) ? Unit : NULL);
                    if (pCon->XcpCalParamList[x].vid < 0) {
                        ThrowError (1, "XCP cannot add calibration parameter \"%s\" to blackboard (%i)", name, pCon->XcpCalParamList[x].vid);
                    }
                    if ((pCon->XcpConfig.UseConvFlag) && (Conversion != NULL)) {
                        set_bbvari_conversion (pCon->XcpCalParamList[x].vid, 1, Conversion);
                    }
                    if ((pCon->XcpConfig.UseMinMaxFlag) && (MinString != NULL) && (MaxString != NULL)) {
                        double Min, Max;
                        Min = strtod (MinString, NULL);
                        Max = strtod (MaxString, NULL);
                        set_bbvari_min (pCon->XcpCalParamList[x].vid, Min);
                        set_bbvari_max (pCon->XcpCalParamList[x].vid, Max);
                    }
                    found = 1;
                } 
            } else {
                ThrowError (1, "missing parameter in string %s", txt);
                Ret = -1;
            }
        } else {
            ThrowError (1, "XCP parameter \"%s\" is not configured and cannot calibrate", par_Params[x]);
            pCon->XcpCalParamList[x].Address = 0;
            pCon->XcpCalParamList[x].vid = -1;
            Ret = -1;
        }
    }
    if (ParamIdxs != NULL) my_free(ParamIdxs);
    return Ret;
}

static int DeleteDTOsBBVariable (int Connection)
{
    CCP_VARIABLES_CONFIG *dto_conf;
    CCP_DTO_PACKAGE *Dto;
    int v, DTOIdx;
    XCP_CONNECTTION *pCon;
    
    pCon = &XcpConnections[Connection];
    dto_conf = &pCon->dto_conf;
    for (DTOIdx = 0; DTOIdx < dto_conf->DTOPackagesCount; DTOIdx++) {
        Dto = &(dto_conf->DTO_Packages[DTOIdx]);
        for (v = 0; v < Dto->VarCount; v++) {
            if (Dto->Entrys[v].Vid > 0) remove_bbvari (Dto->Entrys[v].Vid);
            Dto->Entrys[v].Vid = 0;
        }
    }
    return 0;
}


static int BuildDTOs (CCP_VARIABLES_CONFIG *dto_conf, int var_count, char **vars, int max_dto_packages, int Connection,
                      int UseUnitFlag, int UseConvFlag, int UseMinMaxFlag)
{
    char Section[256];
    char txt[INI_MAX_LINE_LENGTH];
    char entry[32];
    char *dtype_str;
    char *name;
    char *address_str;
    int dtype;
    uint32_t address;
    int size;
    int vid;
    int pid, pid_max;   // package id
    CCP_DTO_PACKAGE *pDTOPackage;
    int used_bytes[CCP_MAX_DTO_PACKAGES];     // jeweils 0...7
    int run, v, x;
    char *Unit, *Conversion, *MinString, *MaxString;
    char *VariableFoundArray;
    int Fd = GetMainFileDescriptor();
    int *VarIdxs;

    DeleteDTOsBBVariable (Connection);
    PrintFormatToString (Section, sizeof(Section), "XCP Configuration for Target %i", Connection);
    MEMSET (used_bytes, 0, sizeof(used_bytes));
    // alles nullen
    MEMSET (dto_conf, 0, sizeof (CCP_VARIABLES_CONFIG));

    VarIdxs = (int*)my_calloc ((size_t)var_count, sizeof (int));
    for (x = 0; x < var_count; x++) {
        VarIdxs[x] = -1;
    }
    VariableFoundArray = my_calloc ((size_t)var_count, sizeof (char));
    // first search all signals and store it's index
    for (v = 0; ; v++) {
        PrintFormatToString (entry, sizeof(entry), "v%i", v);
        if (IniFileDataBaseReadString(Section, entry, "", txt, sizeof (txt), Fd) == 0) {
            break;
        }
        if (StringCommaSeparate (txt, &dtype_str, &name, NULL) == 2) {
            for (x = 0; x < var_count; x++) {
                if (!strcmp (name, vars[x])) {
                    VarIdxs[x] = v;
                    break;
                }
            }
        }
    }

    // 3 walkthroughs
    // inide first time running all 4 Byte-Variables
    // inide second time running all 2 Byte-Variables
    // inide third time running all 1 Byte-Variables
    pid_max = 0;
    for (run = 0; run < 3; run++) {
        pid = 0;
        for (x = 0; x < var_count; x++) {
          if (VarIdxs[x] >= 0) {
                v = VarIdxs[x];
                PrintFormatToString (entry, sizeof(entry), "v%i", v);
                if (IniFileDataBaseReadString(Section, entry, "", txt, sizeof (txt), Fd) == 0) {
                    break;
                }
                Unit = Conversion = MinString = MaxString = NULL;
                if (StringCommaSeparate (txt, &dtype_str, &name, &address_str, &Unit, &Conversion, &MinString, &MaxString, NULL) >= 3) {
                    if (!strcmp (name, vars[x])) {
                        dtype = get_bbvari_type (dtype_str);
                        // size = 1,2 or 4
                        size = get_bbvari_sizeof (dtype_str);
                        if (((run == 0) && (size == 4)) ||
                            ((run == 1) && (size == 2)) ||
                            ((run == 2) && (size == 1))) {
                            address = strtoul (address_str, NULL, 0);
                            vid = add_bbvari (name, dtype,
                                              (UseUnitFlag) ? Unit : NULL);
                            if (vid < 0) {
                                ThrowError (1, "XCP cannot add measurement \"%s\" to blackboard (%i)", name, vid);
                            } else {
                                VariableFoundArray[x] = 1;
                            }
                            if ((UseConvFlag) && (Conversion != NULL)) {
                                set_bbvari_conversion (vid, 1, Conversion);
                            }
                            if ((UseMinMaxFlag) && (MinString != NULL) && (MaxString != NULL)) {
                                double Min, Max;
                                Min = strtod (MinString, NULL);
                                Max = strtod (MaxString, NULL);
                                set_bbvari_min (vid, Min);
                                set_bbvari_max (vid, Max);
                            }

                            // into which package fits the variable?
                            while (used_bytes[pid] + size > 7) {    
                                pid++;
                                if (pid > pid_max) pid_max = pid;
                                if (pid >= max_dto_packages) {
                                    ThrowError (1, "too many variables max. dto packages %i", max_dto_packages);
                                    my_free (VariableFoundArray);
                                    my_free (VarIdxs);
                                    return -1;
                                }
                            }
                            pDTOPackage = &(dto_conf->DTO_Packages[pid]);
                            pDTOPackage->Entrys[pDTOPackage->VarCount].ByteOffset = (unsigned char)(used_bytes[pid] + 1);
                            pDTOPackage->Entrys[pDTOPackage->VarCount].Type = dtype;
                            pDTOPackage->Entrys[pDTOPackage->VarCount].Size = (unsigned char)size;
                            pDTOPackage->Entrys[pDTOPackage->VarCount].Address = address;
                            pDTOPackage->Entrys[pDTOPackage->VarCount].Vid = vid;
                            pDTOPackage->VarCount++;
                            used_bytes[pid] += size;
                            break;
                        }
                    }
                } else {
                    ThrowError (1, "missing parameter in string %s", txt);
                    my_free (VariableFoundArray);
                    my_free (VarIdxs);
                    return -1;
                }
            } else {
                ThrowError (1, "cannot find variable \"%s\"", vars[x]);
                my_free (VariableFoundArray);
                my_free (VarIdxs);
                return -1;
            }
        }
    }
    for (x = 0; x < var_count; x++) {
        if (!VariableFoundArray[x]) {
            ThrowError (1, "XCP unknown measurement \"%s\"", vars[x]);
        }
    }
    my_free (VariableFoundArray);
    my_free (VarIdxs);
    dto_conf->DTOPackagesCount = pid_max + 1;
    write_message (get_pid_by_name ("CANServer"), CCP_SET_ACTIVE_CON_NO, sizeof (int), (char*)&Connection);
    write_message (get_pid_by_name ("CANServer"), CPP_DTOS_DATA_STRUCTS, sizeof (CCP_VARIABLES_CONFIG), (char*)dto_conf);
    return 0;
}

/* Timeout observation */
static void SetTimeOut (XCP_CONNECTTION *pCon)
{
    pCon->TimeoutCounter = GetSimulatedTimeInNanoSecond();
    pCon->WaitForAnAckFlag = 1;
}

static int CheckTimeout (unsigned int ms, XCP_CONNECTTION *pCon)
{
    // Without timeout;
    if (pCon->XcpConfig.Timeout == 0) {
        return 0;
    }
    // With timeout
    else {
        int ret = ((GetSimulatedTimeInNanoSecond() - pCon->TimeoutCounter) > (uint64_t)ms * (uint64_t)(TIMERCLKFRQ / 1000));
        if (ret) {
            ThrowError(ERROR_NO_STOP, "XCP: Timeout");
        }
        return ret;
    }
}

#define x__MIN(a,b)  (((a) < (b)) ? (a) : (b))

#define XCP_CHECK_TIMEOUT() if (CheckTimeout ((unsigned)pCon->XcpConfig.Timeout, pCon)) { \
    DeleteDTOsBBVariable (Connection);  \
    DeleteCalibrationList (Connection); \
    write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(Connection), (char*)&Connection); \
    write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(Connection), (char*)&Connection); \
    pCon->XcpCommand = pCon->XcpCommandSequence = XCP_NO_COMMAND; \
    pCon->WaitForAnAckFlag = 0; \
    pCon->gXcpConnected = 0; \
    pCon->XcpErrCode = -1; \
}

#define XCP_CHECK_TIMEOUT_MS(ms) if (CheckTimeout (ms, pCon)) { \
    DeleteDTOsBBVariable (Connection);  \
    DeleteCalibrationList (Connection); \
    write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(Connection), (char*)&Connection); \
    write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(Connection), (char*)&Connection); \
    pCon->XcpCommand = pCon->XcpCommandSequence = XCP_NO_COMMAND; \
    pCon->WaitForAnAckFlag = 0; \
    pCon->gXcpConnected = 0; \
    pCon->XcpErrCode = -1; \
}

#define XCP_STOP_COMMAND_SEQUENCE() { \
    DeleteDTOsBBVariable (Connection);  \
    DeleteCalibrationList (Connection); \
    write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(Connection), (char*)&Connection); \
    write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(Connection), (char*)&Connection); \
    pCon->XcpCommand = pCon->XcpCommandSequence = XCP_NO_COMMAND; \
    pCon->WaitForAnAckFlag = 0; \
    pCon->gXcpConnected = 0; \
    pCon->XcpErrCode = -1; \
}


static int XcpCheckAckMessage (XCP_CONNECTTION *pCon, int ErrInfoFlag)
{
    int Ret = 0;

    if (pCon->WaitForAnAckFlag == 2) {
        if (pCon->AckCanData[0]==0xFF) {   // Ack OK
            Ret = 1;   // ACK
        } else if (pCon->AckCanData[0]==0xFE) {  // negative Ack
            if (ErrInfoFlag) {
                pCon->XcpErrCode = pCon->AckCanData[1];
            }
            ThrowError ( (ErrInfoFlag) ? ERROR_NO_STOP : INFO_NOT_WRITEN_TO_MSG_FILE, "XCP got a negativ response error code (0x%02X) \"%s\"", 
                   (int)pCon->AckCanData[1], XcpErrorStr ((int)pCon->AckCanData[1]));
            Ret = -1;   
        } else {  // all other Acks
            ThrowError (ERROR_NO_STOP, "XCP got an unknown response 0x%02X", (int)pCon->AckCanData[0]);
            Ret = -2;   
        }
        pCon->WaitForAnAckFlag = 0;
    }
    return Ret;
}

static void PrintACKMessage (char *CmdText, XCP_CONNECTTION *pCon)
{
    if (pCon->XcpConfig.DebugMessages) {
        ThrowError (INFO_NO_STOP, "%s_ACK (%i/0x%X): %02X %02X %02X %02X %02X %02X %02X %02X", 
            CmdText, pCon->XcpConfig.Channel, pCon->XcpConfig.CRM_id, 
            (int)pCon->AckCanData[0], (int)pCon->AckCanData[1], (int)pCon->AckCanData[2], (int)pCon->AckCanData[3], 
            (int)pCon->AckCanData[4], (int)pCon->AckCanData[5], (int)pCon->AckCanData[6], (int)pCon->AckCanData[7]);
    }
}



static void ReadECUInfosCommandScheduer (XCP_CONNECTTION *pCon, int Connection)
{
    switch (pCon->XcpCommand) {
    case XCP_COMMAND_STARTPOINT:
        pCon->XcpErrCode = 0;
        pCon->XcpCommand = XCP_CONNECT;
        pCon->OdtPtr = 0;  
        pCon->DaqPtr = 0; 
        pCon->SizeOfTCUString = 0;
        pCon->TCUStringPointer = 0;
        break;
    case XCP_CONNECT:
        write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof (int), (char*)&Connection);
        XcpReadConfigFromIni (Connection);
        if ((pCon->gXcpConnected & CONNECTED_MASK) != CONNECTED_MASK) {   // it is already connected?
            xcpConnect (pCon);
            SetTimeOut (pCon);
            pCon->XcpCommand = XCP_CONNECT_WAIT_ACK;
        } else pCon->XcpCommand = XCP_GET_DAQ_PROCESSOR_INFO;
        break;
    case XCP_CONNECT_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("CONNECT", pCon);
            pCon->gXcpCALAviable = (pCon->AckCanData[1] & 0x01) == 0x01;
            pCon->gXcpDAQAviable = (pCon->AckCanData[1] & 0x04) == 0x04;
            pCon->XcpByteOrder = (pCon->AckCanData[2] & 0x01) == 0x01;
            if (pCon->XcpConfig.DebugMessages) ThrowError (INFO_NO_STOP, "XcpByteOrder = %s", (pCon->XcpByteOrder) ? "msb_first" : "lsb_first");
            pCon->XcpMaxCTOs = pCon->AckCanData[3];
            pCon->XcpMaxDTOs = pCon->AckCanData[4] << 8 | pCon->AckCanData[5];

            pCon->gXcpConnected |= CONNECTED_MASK;
            pCon->XcpCommand = XCP_GET_ID; 
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_GET_ID:
        xcpGetId (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_ID_WAIT_ACK;
        break;
    case XCP_GET_ID_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_ID", pCon);
            pCon->SizeOfTCUString = *(uint32_t*)(void*)&(pCon->AckCanData[4]);
            SWAP_TO_MSB_FIRST (&pCon->SizeOfTCUString, 4);
            if (pCon->SizeOfTCUString > (sizeof (pCon->ccp_infos.TargetIdentifier) - 2)) {
                ThrowError(INFO_NO_STOP, "length of TCU string (%i) is larger than = %i", pCon->SizeOfTCUString, sizeof (pCon->ccp_infos.TargetIdentifier) - 2);
                pCon->SizeOfTCUString = sizeof (pCon->ccp_infos.TargetIdentifier) - 2;
            }
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "length of TCU string = %i", pCon->SizeOfTCUString);
            pCon->TCUStringPointer = 0;
            MEMSET (pCon->ccp_infos.TargetIdentifier, 0, sizeof (pCon->ccp_infos.TargetIdentifier));
            if (pCon->SizeOfTCUString == 0) {
                pCon->XcpCommand = XCP_READ_TCU_INFO_FINISHED;
            } else {
                pCon->XcpCommand = XCP_UPLOAD;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_UPLOAD:
        xcpUpload ((int)x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7), pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_UPLOAD_WAIT_ACK;
        break;
    case XCP_UPLOAD_WAIT_ACK:
        switch (XcpCheckAckMessage (pCon, 0)) {
        case 1:
            PrintACKMessage ("UPLOAD", pCon);
            MEMCPY (&(pCon->ccp_infos.TargetIdentifier[pCon->TCUStringPointer]), &(pCon->AckCanData[1]), x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7));
            pCon->TCUStringPointer += x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7);
            if (pCon->SizeOfTCUString == pCon->TCUStringPointer) {
                if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "TCU string = %s", pCon->ccp_infos.TargetIdentifier);
                pCon->XcpCommand = XCP_READ_TCU_INFO_FINISHED;
            } else {
                pCon->XcpCommand = XCP_UPLOAD;
            }
            break;
        case -1:
            pCon->ccp_infos.TargetIdentifier[0] = 0;
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "no TCU string available");
            pCon->XcpCommand = XCP_READ_TCU_INFO_FINISHED;
            break;
        default:
            break;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_READ_TCU_INFO_FINISHED:
        pCon->XcpCommand = 0;
        pCon->XcpCommandSequence = 0;
        break;
    }

}


static void StartMeasurement_CommandScheduer (XCP_CONNECTTION *pCon, int Connection)
{
    int i;  

    switch (pCon->XcpCommand) {
    case XCP_COMMAND_STARTPOINT:
        pCon->XcpErrCode = 0;
        if ((pCon->gXcpConnected & MESSUREMENT_RUNNING_MASK) == MESSUREMENT_RUNNING_MASK) {  // Messurement continue running
            pCon->XcpCommand = XCP_STOP_DAQ_LIST;
        } else {
            pCon->XcpCommand = XCP_CONNECT;
        }
        break;

    case XCP_STOP_DAQ_LIST:
        xcpStartStopDaqList (2, 0, pCon);   // 2 = select
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_STOP_DAQ_LIST_WAIT_ACK;
        break;
    case XCP_STOP_DAQ_LIST_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("START_STOP_DAQ_LIST", pCon);
            pCon->XcpCommand = XCP_STOP_SYNCH;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_STOP_SYNCH:
        xcpStartStopSync (0, pCon);   // 0 = stop
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_STOP_SYNCH_WAIT_ACK;
        break;
    case XCP_STOP_SYNCH_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("START_STOP_SYNCH", pCon);
            pCon->gXcpConnected &= ~MESSUREMENT_RUNNING_MASK;
            // remove blackboard variable
            DeleteDTOsBBVariable (Connection);
            write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(Connection), (char*)&Connection);
            // Deactivate timout observation which was activated with CCP_DTO_ACTIVATE_TIMEOUT-Message
            write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(Connection), (char*)&Connection);
            write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof(Connection), (char*)&Connection);
            pCon->XcpCommand = XCP_GET_ID;   // no connect neccessary
        }
        XCP_CHECK_TIMEOUT();
        break;

    case XCP_CONNECT:
        write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof (int), (char*)&Connection);
        XcpReadConfigFromIni (Connection);
        xcpConnect (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_CONNECT_WAIT_ACK;
        break;
    case XCP_CONNECT_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("CONNECT", pCon);
            pCon->gXcpCALAviable = (pCon->AckCanData[1] & 0x01) == 0x01;
            pCon->gXcpDAQAviable = (pCon->AckCanData[1] & 0x04) == 0x04;
            pCon->XcpByteOrder = (pCon->AckCanData[2] & 0x01) == 0x01;
            if (pCon->XcpConfig.DebugMessages) ThrowError (INFO_NO_STOP, "XcpByteOrder = %s", (pCon->XcpByteOrder) ? "msb_first" : "lsb_first");
            pCon->XcpMaxCTOs = pCon->AckCanData[3];
            pCon->XcpMaxDTOs = pCon->AckCanData[4] << 8 | pCon->AckCanData[5];

            pCon->gXcpConnected |= CONNECTED_MASK;

            if (pCon->XcpConfig.SeedKeyFlag == CCP_SEED_AND_KEY) {
                pCon->XcpCommand = XCP_GET_STATUS;
            } else {
                pCon->XcpCommand = XCP_GET_ID;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_GET_STATUS:
        xcpGetStatus (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_STATUS_WAIT_ACK;
        break;
    case XCP_GET_STATUS_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_STATUS", pCon);
            pCon->gXcpProtectionStatus = pCon->AckCanData[2];
            if (pCon->gXcpProtectionStatus & PL_DAQ) {
                //DAQ list commands (DIRECTION = DAQ -> STIM not implemented!!!) are protected with SEED & Key mechanism
                pCon->XcpCommand = XCP_GET_SEED;
                pCon->gXcpSeedCounter = 0;
            } else {
                //Nothing is protected regarding measurement
                pCon->XcpCommand = XCP_GET_ID;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_GET_SEED:
        xcpGetSeed (PL_DAQ, pCon->gXcpSeedCounter != 0, pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_SEED_WAIT_ACK;
        break;
    case XCP_GET_SEED_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_SEED", pCon);
            if (pCon->gXcpSeedCounter == 0) { 
                pCon->gXcpSeedLength = pCon->AckCanData[1];  // first ack will include the length Seeds
            } else {
                 pCon->gXcpSeedRemaining = pCon->AckCanData[1];  // following response will contain the left length of the seed
            }
            for (i = 2; (i < 8) && (pCon->gXcpSeedCounter < pCon->gXcpSeedLength) && (pCon->gXcpSeedCounter < MAX_SEED_LENGHT); i++, pCon->gXcpSeedCounter++) {
                pCon->gXcpSeed[pCon->gXcpSeedCounter] = pCon->AckCanData[i];
            }
            if (pCon->gXcpSeedCounter == pCon->gXcpSeedLength) {
                if (xcpCalcKey (pCon, PL_DAQ)) {
                    pCon->XcpCommand = XCP_UNLOCK;
                } else {
                    XCP_STOP_COMMAND_SEQUENCE(); 
                }
            } else {
                pCon->XcpCommand = XCP_GET_SEED;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_UNLOCK:
        xcpUnlock (pCon, PL_DAQ);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_UNLOCK_WAIT_ACK;
        break;
    case XCP_UNLOCK_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_UNLOCK", pCon);
            if (pCon->gXcpKeyCounter == pCon->gXcpKeyLength) {
                pCon->XcpCommand = XCP_GET_ID;
            } else {
                pCon->XcpCommand = XCP_UNLOCK;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_GET_ID:
        xcpGetId (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_ID_WAIT_ACK;
        break;
    case XCP_GET_ID_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_ID", pCon);
            pCon->SizeOfTCUString = *(uint32_t*)(void*)&(pCon->AckCanData[4]);
            SWAP_TO_MSB_FIRST (&pCon->SizeOfTCUString, 4);
            if (pCon->SizeOfTCUString > (sizeof (pCon->ccp_infos.TargetIdentifier) - 2)) {
                ThrowError(INFO_NO_STOP, "length of TCU string (%i) is larger than = %i", pCon->SizeOfTCUString, sizeof (pCon->ccp_infos.TargetIdentifier) - 2);
                pCon->SizeOfTCUString = sizeof (pCon->ccp_infos.TargetIdentifier) - 2;
            }
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "length of TCU string = %i", pCon->SizeOfTCUString);
            pCon->TCUStringPointer = 0;
            MEMSET (pCon->ccp_infos.TargetIdentifier, 0, sizeof (pCon->ccp_infos.TargetIdentifier));
            if (pCon->SizeOfTCUString == 0) {
                pCon->XcpCommand = XCP_GET_DAQ_PROCESSOR_INFO;
            } else {
                pCon->XcpCommand = XCP_UPLOAD;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_UPLOAD:
        xcpUpload ((int)x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7), pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_UPLOAD_WAIT_ACK;
        break;
    case XCP_UPLOAD_WAIT_ACK:
        switch (XcpCheckAckMessage (pCon, 0)) {
        case 1:
            PrintACKMessage ("UPLOAD", pCon);
            MEMCPY (&(pCon->ccp_infos.TargetIdentifier[pCon->TCUStringPointer]), &(pCon->AckCanData[1]), x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7));
            pCon->TCUStringPointer += x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7);
            if (pCon->SizeOfTCUString == pCon->TCUStringPointer) {
                if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "TCU string = %s", pCon->ccp_infos.TargetIdentifier);
                pCon->XcpCommand = XCP_GET_DAQ_PROCESSOR_INFO;
            } else {
                pCon->XcpCommand = XCP_UPLOAD;
            }
            break;
        case -1:
            pCon->ccp_infos.TargetIdentifier[0] = 0;
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "no TCU string available (ignoring)");
            pCon->XcpCommand = XCP_GET_DAQ_PROCESSOR_INFO;
            break;
        default:
            break;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_GET_DAQ_PROCESSOR_INFO:
        xcpGetDaqProcessorInfo (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_DAQ_PROCESSOR_INFO_WAIT_ACK;
        break;
    case XCP_GET_DAQ_PROCESSOR_INFO_WAIT_ACK:
        switch (XcpCheckAckMessage (pCon, 0)) {
        case 1:
            PrintACKMessage ("GET_DAQ_PROCESSOR_INFO", pCon);
            pCon->XcpCommand = XCP_BUILD_DTOS;
            break;
        case -1:
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "no DAQ processor infos available (ignoring)");
            pCon->XcpCommand = XCP_BUILD_DTOS;
            break;
        default:
            break;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_GET_DAQ_RESOLUTION_INFO:
        xcpGetDaqResolutionInfo (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_DAQ_RESOLUTION_INFO_WAIT_ACK;
        break;
    case XCP_GET_DAQ_RESOLUTION_INFO_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 0) == 1) {
            PrintACKMessage ("GET_DAQ_RESOLUTION_INFO", pCon);
            pCon->XcpCommand = XCP_BUILD_DTOS;
        }
        XCP_CHECK_TIMEOUT();
        break;

    case XCP_BUILD_DTOS:
        if (BuildDTOs (&pCon->dto_conf, pCon->CcpMeasurementLabelListSize, pCon->CcpMeasurementLabelList, 255, Connection,
                       pCon->XcpConfig.UseUnitFlag, pCon->XcpConfig.UseConvFlag, pCon->XcpConfig.UseMinMaxFlag)) {
            pCon->XcpCommand = 0;
            pCon->XcpCommandSequence = 0;
        } else {
            pCon->XcpCommand = XCP_FREE_DAQ;
            pCon->OdtPtr = pCon->DaqPtr = 0;
        }
        break;

    case XCP_FREE_DAQ:
        xcpFreeDaq (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_FREE_DAQ_WAIT_ACK;
        break;
    case XCP_FREE_DAQ_WAIT_ACK:
        switch (XcpCheckAckMessage (pCon, 0)) {
        case 1:
            PrintACKMessage ("FREE_DAQ", pCon);
            pCon->XcpCommand = XCP_ALLOC_DAQ;
            break;
        case -1:
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "no free DAQ (ignoring)");
            pCon->XcpCommand = XCP_ALLOC_DAQ;
            break;
        default:
            break;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_ALLOC_DAQ:
        xcpAllocDaq (1, pCon);  // there are only one DAQ liste possible
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_ALLOC_DAQ_WAIT_ACK;
        break;
    case XCP_ALLOC_DAQ_WAIT_ACK:
        switch (XcpCheckAckMessage (pCon, 0)) {
        case 1:
            PrintACKMessage ("ALLOC_DAQ", pCon);
            pCon->XcpCommand = XCP_ALLOC_ODT;
            break;
        case -1:
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "no alloc DAQ (ignoring)");
            pCon->XcpCommand = XCP_ALLOC_ODT;
            break;
        default:
            break;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_ALLOC_ODT:
        xcpAllocOdt (0, pCon->dto_conf.DTOPackagesCount, pCon);     
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_ALLOC_ODT_WAIT_ACK;
        break;
    case XCP_ALLOC_ODT_WAIT_ACK:
        switch (XcpCheckAckMessage (pCon, 0)) {
        case 1:
            PrintACKMessage ("ALLOC_ODT", pCon);
            pCon->XcpCommand = XCP_ALLOC_ODT_ENTRY;
            break;
        case -1:
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "no alloc ODT (ignoring)");
            pCon->XcpCommand = XCP_ALLOC_ODT_ENTRY;
            break;
        default:
            break;
        }
        XCP_CHECK_TIMEOUT();
        break;

    case XCP_ALLOC_ODT_ENTRY:
        xcpAllocOdtEntry (0, pCon->OdtPtr, pCon->dto_conf.DTO_Packages[pCon->OdtPtr].VarCount, pCon);     
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_ALLOC_ODT_ENTRY_WAIT_ACK;
        break;
    case XCP_ALLOC_ODT_ENTRY_WAIT_ACK:
        switch (XcpCheckAckMessage (pCon, 0)) {
        case 1:
            PrintACKMessage ("ALLOC_ODT_ENTRY", pCon);
            pCon->OdtPtr++;
            if (pCon->OdtPtr >= pCon->dto_conf.DTOPackagesCount) {
                pCon->OdtPtr = 0;
                pCon->XcpCommand = XCP_SET_DAQ_PTR;
            } else {
                pCon->XcpCommand = XCP_ALLOC_ODT_ENTRY;
            }
            break;
        case -1:
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "no alloc ODT entry (ignoring)");
            pCon->OdtPtr++;
            if (pCon->OdtPtr >= pCon->dto_conf.DTOPackagesCount) {
                pCon->OdtPtr = 0;
                pCon->XcpCommand = XCP_SET_DAQ_PTR;
            } else {
                pCon->XcpCommand = XCP_ALLOC_ODT_ENTRY;
            }
            break;
        default:
            break;
        }
        XCP_CHECK_TIMEOUT();
        break;

    case XCP_SET_DAQ_PTR:
        xcpSetDaqPtr (0, pCon->OdtPtr, 0, pCon);      
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_SET_DAQ_PTR_WAIT_ACK;
        break;
    case XCP_SET_DAQ_PTR_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1 ) {
            PrintACKMessage ("SET_DAQ_PTR", pCon);
            pCon->XcpCommand = XCP_WRITE_DAQ;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_WRITE_DAQ:
        xcpWriteDaq (0xFF, pCon->dto_conf.DTO_Packages[pCon->OdtPtr].Entrys[pCon->DaqPtr].Size, 0, 
                     pCon->dto_conf.DTO_Packages[pCon->OdtPtr].Entrys[pCon->DaqPtr].Address, pCon);   
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_WRITE_DAQ_WAIT_ACK;
        break;
    case XCP_WRITE_DAQ_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("WRITE_DAQ", pCon);
            pCon->DaqPtr++;
            if (pCon->DaqPtr >= pCon->dto_conf.DTO_Packages[pCon->OdtPtr].VarCount) {
                pCon->DaqPtr = 0;
                pCon->OdtPtr++;
                if (pCon->OdtPtr >= pCon->dto_conf.DTOPackagesCount) {
                    pCon->XcpCommand = XCP_SET_DAQ_LIST_MODE;
                    break;
                } else {
                    pCon->XcpCommand = XCP_SET_DAQ_PTR;
                    break;
                }
            } else {
                pCon->XcpCommand = XCP_WRITE_DAQ;   
            }
        }
        XCP_CHECK_TIMEOUT();
        break;

    case XCP_SET_DAQ_LIST_MODE:
        xcpSetDaqListMode (0, 0, pCon->XcpConfig.EventChannel, pCon->XcpConfig.Prescaler, 0x00, pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_SET_DAQ_LIST_MODE_WAIT_ACK;
        break;
    case XCP_SET_DAQ_LIST_MODE_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("SET_DAQ_LIST_MODE", pCon);
            pCon->XcpCommand = XCP_START_STOP_DAQ_LIST;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_START_STOP_DAQ_LIST:
        xcpStartStopDaqList (2, 0, pCon);   // 2 = select
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_START_STOP_DAQ_LIST_WAIT_ACK;
        break;
    case XCP_START_STOP_DAQ_LIST_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("START_STOP_DAQ_LIST", pCon);
            pCon->XcpCommand = XCP_START_STOP_SYNCH; 
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_START_STOP_SYNCH:
        if ((pCon->gXcpConnected & MESSUREMENT_RUNNING_MASK) == MESSUREMENT_RUNNING_MASK) {   
             xcpStartStopSync (0, pCon);   // 0 = stop
        } else {
             xcpStartStopSync (1, pCon);   // 1 = start
        }
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_START_STOP_SYNCH_WAIT_ACK;
        break;
    case XCP_START_STOP_SYNCH_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("START_STOP_SYNCH", pCon);
            if ((pCon->gXcpConnected & MESSUREMENT_RUNNING_MASK) == MESSUREMENT_RUNNING_MASK) {   
                pCon->gXcpConnected &= ~MESSUREMENT_RUNNING_MASK;
                pCon->XcpCommand = XCP_BUILD_DTOS;
            } else {
                write_message (get_pid_by_name ("CANServer"), CCP_DTO_ACTIVATE_TIMEOUT, sizeof(int), (char*)&Connection);
                pCon->XcpCommand = XCP_WAIT_FIRST_DATA;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_WAIT_FIRST_DATA:
        {
            MESSAGE_HEAD mhead;
            if (test_message (&mhead)) {
                if (mhead.mid == CCP_RECEIVE_ALL_VARIABLE_FIRST_TIME) {
                    remove_message ();
                    pCon->XcpCommand = XCP_START_MEASUREMENT_FINISHED;
                } 
            }
        }
        break;
    case XCP_START_MEASUREMENT_FINISHED:
        pCon->XcpCommand = 0;
        pCon->XcpCommandSequence = 0;
        pCon->gXcpConnected |= MESSUREMENT_RUNNING_MASK;
        break;
    }
}



static void StopMeasurement_CommandScheduer (XCP_CONNECTTION *pCon, int Connection)
{
    switch (pCon->XcpCommand) {
    case XCP_COMMAND_STARTPOINT:
        pCon->XcpErrCode = 0;
        pCon->XcpCommand = XCP_START_STOP_DAQ_LIST;
        break;
    case XCP_START_STOP_DAQ_LIST:
        xcpStartStopDaqList (2, 0, pCon);   // 2 = select
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_START_STOP_DAQ_LIST_WAIT_ACK;
        break;
    case XCP_START_STOP_DAQ_LIST_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("START_STOP_DAQ_LIST", pCon);
            pCon->XcpCommand = XCP_START_STOP_SYNCH;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_START_STOP_SYNCH:
        xcpStartStopSync (0, pCon);   // 0 = stop
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_START_STOP_SYNCH_WAIT_ACK;
        break;
    case XCP_START_STOP_SYNCH_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("START_STOP_SYNCH", pCon);
            pCon->gXcpConnected &= ~MESSUREMENT_RUNNING_MASK;
            // remove blackboard variables
            DeleteDTOsBBVariable (Connection);
            write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(Connection), (char*)&Connection);

            // is calibration active?
            if (pCon->gXcpConnected != CONNECTED_MASK) {
                // Deactivate timout observation which was activated with CCP_DTO_ACTIVATE_TIMEOUT-Message
                write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(int), (char*)&Connection);
                write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof(int), (char*)&Connection);
                // Than jump over disconnect
                pCon->XcpCommand = XCP_STOP_MEASUREMENT_FINISHED;
            } else {
                pCon->XcpCommand = XCP_DISCONNECT;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_DISCONNECT:
        xcpDisconnect (pCon); 
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_DISCONNECT_WAIT_ACK;
        break;
    case XCP_DISCONNECT_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("DISCONNECT", pCon);
            pCon->gXcpConnected = 0;
            pCon->XcpCommand = XCP_STOP_MEASUREMENT_FINISHED;
            write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(int), (char*)&Connection);
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_STOP_MEASUREMENT_FINISHED:
        pCon->XcpCommand = 0;
        pCon->XcpCommandSequence = 0;
        break;
    }
}

static void StartCalibration_CommandScheduer (XCP_CONNECTTION *pCon, int Connection)
{
    int i;  

    switch (pCon->XcpCommand) {
    case XCP_COMMAND_STARTPOINT:
        DeleteCalibrationList (Connection);
        pCon->XcpErrCode = 0;
        pCon->XcpCommand = XCP_CONNECT;
        break;
    case XCP_CONNECT:
        write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof (int), (char*)&Connection);
        XcpReadConfigFromIni (Connection);
        if ((pCon->gXcpConnected & CONNECTED_MASK) != CONNECTED_MASK) {   // is already connected?
            xcpConnect (pCon);
            SetTimeOut (pCon);
            pCon->XcpCommand = XCP_CONNECT_WAIT_ACK;
        } else {
            pCon->XcpCommand = XCP_CONNECTED;
        }
        break;
    case XCP_CONNECT_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("CONNECT", pCon);
            pCon->gXcpCALAviable = (pCon->AckCanData[1] & 0x01) == 0x01;
            pCon->gXcpDAQAviable = (pCon->AckCanData[1] & 0x04) == 0x04;
            pCon->XcpByteOrder = (pCon->AckCanData[2] & 0x01) == 0x01;
            if (pCon->XcpConfig.DebugMessages) ThrowError (INFO_NO_STOP, "XcpByteOrder = %s", (pCon->XcpByteOrder) ? "msb_first" : "lsb_first");
            pCon->XcpMaxCTOs = pCon->AckCanData[3];
            pCon->XcpMaxDTOs = pCon->AckCanData[4] << 8 | pCon->AckCanData[5];

            pCon->gXcpConnected |= CONNECTED_MASK;
            pCon->XcpCommand = XCP_CONNECTED;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_CONNECTED:
        if (pCon->XcpConfig.SeedKeyFlag == CCP_SEED_AND_KEY) {
            pCon->XcpCommand = XCP_GET_STATUS;
        } else {
            pCon->XcpCommand = XCP_GET_ID;
        }
        break;

    case XCP_GET_STATUS:
        xcpGetStatus (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_STATUS_WAIT_ACK;
        break;
    case XCP_GET_STATUS_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_STATUS", pCon);
            pCon->gXcpProtectionStatus = pCon->AckCanData[2];
            if (pCon->gXcpProtectionStatus & PL_CAL) {
                // Calibration commands are protected with SEED & Key mechanism
                pCon->XcpCommand = XCP_GET_SEED;
                pCon->gXcpSeedCounter = 0;
            } else {
                // Nothing is protected regarding measurement
                pCon->XcpCommand = XCP_GET_ID;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_GET_SEED:
        xcpGetSeed (PL_CAL, pCon->gXcpSeedCounter != 0, pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_SEED_WAIT_ACK;
        break;
    case XCP_GET_SEED_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_SEED", pCon);
            if (pCon->gXcpSeedCounter == 0) { 
                pCon->gXcpSeedLength = pCon->AckCanData[1];  // first ack will include the length Seeds
            } else {
                 pCon->gXcpSeedRemaining = pCon->AckCanData[1];  // following response will contain the left length of the seed
            }
            for (i = 2; (i < 8) && (pCon->gXcpSeedCounter < pCon->gXcpSeedLength) && (pCon->gXcpSeedCounter < MAX_SEED_LENGHT); i++, pCon->gXcpSeedCounter++) {
                pCon->gXcpSeed[pCon->gXcpSeedCounter] = pCon->AckCanData[i];
            }
            if (pCon->gXcpSeedCounter == pCon->gXcpSeedLength) {
                if (xcpCalcKey (pCon, PL_CAL)) {
                    pCon->XcpCommand = XCP_UNLOCK;
                } else {
                    XCP_STOP_COMMAND_SEQUENCE(); 
                }
            } else {
                pCon->XcpCommand = XCP_GET_SEED;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_UNLOCK:
        xcpUnlock (pCon, PL_CAL); 
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_UNLOCK_WAIT_ACK;
        break;
    case XCP_UNLOCK_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_UNLOCK", pCon);
            if (pCon->gXcpKeyCounter == pCon->gXcpKeyLength) {
                pCon->XcpCommand = XCP_GET_ID;
            } else {
                pCon->XcpCommand = XCP_UNLOCK;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_GET_ID:
        xcpGetId (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_ID_WAIT_ACK;
        break;
    case XCP_GET_ID_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_ID", pCon);
            pCon->SizeOfTCUString = *(uint32_t*)(void*)&(pCon->AckCanData[4]);
            SWAP_TO_MSB_FIRST (&pCon->SizeOfTCUString, 4);
            if (pCon->SizeOfTCUString > (sizeof (pCon->ccp_infos.TargetIdentifier) - 2)) {
                ThrowError(INFO_NO_STOP, "length of TCU string (%i) is larger than = %i", pCon->SizeOfTCUString, sizeof (pCon->ccp_infos.TargetIdentifier) - 2);
                pCon->SizeOfTCUString = sizeof (pCon->ccp_infos.TargetIdentifier) - 2;
            }
            if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "length of TCU string = %i", pCon->SizeOfTCUString);
            pCon->TCUStringPointer = 0;
            MEMSET (pCon->ccp_infos.TargetIdentifier, 0, sizeof (pCon->ccp_infos.TargetIdentifier));
            if (pCon->SizeOfTCUString == 0) {
                goto __NO_ECU_NAME;
            } else {
                pCon->XcpCommand = XCP_UPLOAD;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_UPLOAD:
        xcpUpload ((int)x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7), pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_UPLOAD_WAIT_ACK;
        break;
    case XCP_UPLOAD_WAIT_ACK:
        {
            int Ack = 0;
            switch (XcpCheckAckMessage (pCon, 0)) {
            case 1:
                PrintACKMessage ("UPLOAD", pCon);
                MEMCPY (&(pCon->ccp_infos.TargetIdentifier[pCon->TCUStringPointer]), &(pCon->AckCanData[1]), x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7));
                pCon->TCUStringPointer += x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 7);
                if (pCon->SizeOfTCUString == pCon->TCUStringPointer) {
                    if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "TCU string = %s", pCon->ccp_infos.TargetIdentifier);
                    Ack = 1;
                } else {
                    pCon->XcpCommand = XCP_UPLOAD;
                }
                break;
            case -1:
                pCon->ccp_infos.TargetIdentifier[0] = 0;
                if (pCon->XcpConfig.DebugMessages) ThrowError(INFO_NO_STOP, "no TCU string available");
                Ack = 1;
                break;
            default:
                break;
            }
            if (Ack) {
__NO_ECU_NAME:
                if (pCon->XcpConfig.MoveROM2RAM) {
                    pCon->XcpCommand = XCP_COPY_CAL_PAGE;
                } else if (pCon->XcpConfig.Xcp.SetCalPage) {
                    pCon->XcpCommand = XCP_SET_CAL_PAGE;
                } else {
                    pCon->XcpCommand = XCP_START_CALIBRATION_FINISHED;
                }
            }
            XCP_CHECK_TIMEOUT();
        }
        break;
    case XCP_COPY_CAL_PAGE:
        xcpCopyCalPage (pCon->XcpConfig.Xcp.CalibROMSegment, pCon->XcpConfig.Xcp.CalibROMPageNo,
                        pCon->XcpConfig.Xcp.CalibRAMSegment, pCon->XcpConfig.Xcp.CalibRAMPageNo,
                        pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_COPY_CAL_PAGE_ACK;
        break;
    case XCP_COPY_CAL_PAGE_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("COPY_CAL_PAGE",pCon);
            if (pCon->XcpConfig.Xcp.SetCalPage) {
                pCon->XcpCommand = XCP_SET_CAL_PAGE;
            } else {
                pCon->XcpCommand = XCP_START_CALIBRATION_FINISHED;
            }
        }
        XCP_CHECK_TIMEOUT_MS (30000);    // 30s
        break;
    case XCP_SET_CAL_PAGE:
        xcpSetCalPage (pCon->XcpConfig.Xcp.CalibRAMSegment, pCon->XcpConfig.Xcp.CalibRAMPageNo, pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_SET_CAL_PAGE_ACK;
        break;
    case XCP_SET_CAL_PAGE_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("SET_CAL_PAGE",pCon);
            pCon->XcpCommand = XCP_START_CALIBRATION_FINISHED;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_START_CALIBRATION_FINISHED:
        BuildCalibrationList (pCon->CcpMeasurementLabelListSize, pCon->CcpMeasurementLabelList, Connection);
        // After connection is established read all parameter from target
        pCon->XcpUploadInProgress = TRUE;
        pCon->gXcpConnected |= CALIBRATION_ACTIVE_MASK;
        pCon->XcpCommand = 0;
        pCon->XcpCommandSequence = 0;
        break;
    }
}


static void StopCalibration_CommandScheduer (XCP_CONNECTTION *pCon, int Connection)
{
    switch (pCon->XcpCommand) {
    case XCP_COMMAND_STARTPOINT:
        pCon->XcpErrCode = 0;
        pCon->gXcpConnected &= ~CALIBRATION_ACTIVE_MASK;
        DeleteCalibrationList (Connection);
        // is meassurement active
        if ((pCon->gXcpConnected) != CONNECTED_MASK) {
            // than jump over disconnect
            pCon->XcpCommand = XCP_STOP_CALIBRATION_FINISHED;
        } else {
            pCon->XcpCommand = XCP_DISCONNECT;
        }
        break;
    case XCP_DISCONNECT:
        xcpDisconnect (pCon); 
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_DISCONNECT_WAIT_ACK;
        break;
    case XCP_DISCONNECT_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("DISCONNECT", pCon);
            pCon->gXcpConnected = 0;
            pCon->XcpCommand = XCP_STOP_CALIBRATION_FINISHED;
            write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(int), (char*)&Connection);
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_STOP_CALIBRATION_FINISHED:
        pCon->XcpCommand = 0;
        pCon->XcpCommandSequence = 0;
        break;
    }
}

static void AlivePing_CommandScheduer (XCP_CONNECTTION *pCon, int Connection)
{
    switch (pCon->XcpCommand) {
    case XCP_GET_STATUS:
        xcpGetStatus (pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_GET_STATUS_WAIT_ACK;
        break;
    case XCP_GET_STATUS_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("GET_STATUS", pCon);
            pCon->XcpCommand = 0;
            pCon->XcpCommandSequence = 0;
        }
        XCP_CHECK_TIMEOUT();
        break;
    }
}


static void SetStatusVariableTextReplaces (XCP_CONNECTTION *pCon)
{
    char *enumstr;

    enumstr = "0 0 \"IDLE\";"
              "1 1 \"CONNECTED\";"
              "3 3 \"MEASUREMENT\";"
              "5 5 \"CALIBRATION\";"
              "7 7 \"CALIBRATION_AND_MEASUREMENT\";";
    set_bbvari_conversion (pCon->XCPStatusVid, BB_CONV_TEXTREP, enumstr);

    enumstr =  "0 0 \"NO_COMMAND\";"
               "1 1 \"GET_ID\";"
               "2 2 \"GET_ID_WAIT_ACK\";"
               "3 3 \"UPLOAD\";"
               "4 4 \"UPLOAD_WAIT_ACK\";"
               "5 5 \"READ_TCU_INFO_FINISHED\";"
               "6 6 \"CONNECT\";"
               "7 7 \"CONNECT_WAIT_ACK\";"
               "8 8 \"GET_DAQ_PROCESSOR_INFO\";"
               "9 9 \"GET_DAQ_PROCESSOR_INFO_WAIT_ACK\";"
               "10 10 \"GET_DAQ_RESOLUTION_INFO\";"
               "11 11 \"GET_DAQ_RESOLUTION_INFO_WAIT_ACK\";"
               "12 12 \"FREE_DAQ\";"
               "13 13 \"FREE_DAQ_WAIT_ACK\";"
               "14 14 \"ALLOC_DAQ\";"
               "15 15 \"ALLOC_DAQ_WAIT_ACK\";"
               "16 16 \"ALLOC_ODT\";"
               "17 17 \"ALLOC_ODT_WAIT_ACK\";"
               "18 18 \"ALLOC_ODT_ENTRY\";"
               "19 19 \"ALLOC_ODT_ENTRY_WAIT_ACK\";"
               "20 20 \"SET_DAQ_PTR\";"
               "21 21 \"SET_DAQ_PTR_WAIT_ACK\";"
               "22 22 \"WRITE_DAQ\";"
               "23 23 \"WRITE_DAQ_WAIT_ACK\";"
               "24 24 \"SET_DAQ_LIST_MODE\";"
               "25 25 \"SET_DAQ_LIST_MODE_WAIT_ACK\";"
               "26 26 \"START_STOP_DAQ_LIST\";"
               "27 27 \"START_STOP_DAQ_LIST_WAIT_ACK\";"
               "28 28 \"GET_DAQ_CLOCK\";"
               "29 29 \"GET_DAQ_CLOCK_WAIT_ACK\";"
               "30 30 \"START_STOP_SYNCH\";"
               "31 31 \"START_STOP_SYNCH_WAIT_ACK\";"
               "32 32 \"DISCONNECT\";"
               "33 33 \"DISCONNECT_WAIT_ACK\";"
               "34 34 \"BUILD_DTOS\";"
               "35 35 \"START_MEASUREMENT_FINISHED\";"
               "36 36 \"STOP_MEASUREMENT_FINISHED\";"
               "37 37 \"SET_MTA\";"
               "38 38 \"SET_MTA_WAIT_ACK\";"
               "39 39 \"DATA_DOWNLOAD\";"
               "40 40 \"DATA_DOWNLOAD_WAIT_ACK\";"
               "41 41 \"START_CALIBRATION_FINISHED\";"
               "42 42 \"STOP_CALIBRATION_FINISHED\";"
               "43 43 \"COPY_CAL_PAGE\";"
               "44 44 \"COPY_CAL_PAGE_ACK\";"
               "45 45 \"SET_CAL_PAGE\";"
               "46 46 \"SET_CAL_PAGE_ACK\";"
               "47 47 \"WAIT_FIRST_DATA\";"
               "48 48 \"STOP_DAQ_LIST\";"
               "49 49 \"STOP_DAQ_LIST_WAIT_ACK\";"
               "50 50 \"STOP_SYNCH\";"
               "51 51 \"STOP_SYNCH_WAIT_ACK\";"
               "52 52 \"GET_STATUS\";"
               "53 53 \"GET_STATUS_WAIT_ACK\";"
               "54 54 \"GET_SEED\";"
               "55 55 \"GET_SEED_WAIT_ACK\";"
               "56 56 \"UNLOCK\";"
               "57 57 \"UNLOCK_WAIT_ACK\";"
               "58 58 \"CONNECTED\";"
               "100 100 \"COMMAND_STARTPOINT\";";



    set_bbvari_conversion (pCon->XCPCommandVid, BB_CONV_TEXTREP, enumstr);

    enumstr = "0 0 \"NO_COMMAND\";"
              "1 1 \"READ_TCU_INFO\";"
              "2 2 \"START_MEASUREMENT\";"
              "3 3 \"STOP_MEASUREMENT\";"
              "4 4 \"READ_MEMORY\";"
              "5 5 \"DOWNLOAD_SEQUENCE\";"
              "6 6 \"UPLOAD_SEQUENCE\";"
              "7 7 \"START_CALIBRATION\";"
              "8 8 \"STOP_CALIBRATION\";";
    set_bbvari_conversion (pCon->XCPCommandSequenceVid, BB_CONV_TEXTREP, enumstr);
}


static void cyclic_xcp_control (void)
{
    MESSAGE_HEAD mhead;
    int x, AckFromConnection;

    // some Messages can receive during command
    if (test_message (&mhead)) {
        switch (mhead.mid) {
        case CCP_CRM_ACK:
            {
                CCP_CRO_CAN_OBJECT CanObject;
                read_message (&mhead, (char*)&CanObject, sizeof (CanObject));
                AckFromConnection = CanObject.Channel;
                if ((AckFromConnection >= 0) && (AckFromConnection < 4)) {
                    if (XcpConnections[AckFromConnection].WaitForAnAckFlag == 1) {
                        XcpConnections[AckFromConnection].WaitForAnAckFlag = 2;
                        MEMCPY (XcpConnections[AckFromConnection].AckCanData, CanObject.Data, 8);
                    }
                }
            }
            break;
        case CCP_DTO_TIMEOUT:
            read_message (&mhead, (char*)&AckFromConnection, sizeof (int));  
            if ((AckFromConnection >= 0) && (AckFromConnection < 4)) {
                write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof (int), (char*)&AckFromConnection);
                write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(int), (char*)&AckFromConnection);
                XcpConnections[AckFromConnection].XcpCommandSequence = 
                XcpConnections[AckFromConnection].XcpCommand = 
                XcpConnections[AckFromConnection].gXcpConnected = 0;
                // remove all blackboard variables
                DeleteCalibrationList (AckFromConnection);
                DeleteDTOsBBVariable (AckFromConnection);
                ThrowError (ERROR_NO_STOP, "Timeout XCP measurement connection %i stopped (receive some time no DTO messages)", AckFromConnection);

            }
            break;
        }
    }
    for (x = 0; x < 4; x++) {
        XCP_CONNECTTION *pCon = &XcpConnections[x];
        switch (pCon->XcpCommandSequence) {
        case XCP_READ_TCU_INFO:
            ReadECUInfosCommandScheduer (pCon, x);
            break;
        case XCP_START_MEASUREMENT:
            StartMeasurement_CommandScheduer (pCon, x);
            break;
        case XCP_STOP_MEASUREMENT:
            StopMeasurement_CommandScheduer (pCon, x);
            break;
        case XCP_READ_MEMORY:
            // not implemented
            pCon->XcpCommandSequence = 0;
            break;
        case XCP_DOWNLOAD_SEQUENCE:
            XCPDownload_CmdScheduler (pCon, x);
            break;
        case XCP_UPLOAD_SEQUENCE:
            XCPUpload_CmdScheduler (pCon, x);
            break;
        case XCP_START_CALIBRATION:
            StartCalibration_CommandScheduer (pCon, x);
            break;
        case XCP_STOP_CALIBRATION:
            StopCalibration_CommandScheduer (pCon, x);
            break;
        case XCP_ALIFE_PING:
            AlivePing_CommandScheduer (pCon, x);
            break;
        case XCP_NO_COMMAND_SEQUENCE:
            // Is there a new command sequence (Start/Stop-XCP(Cal))?
            if (pCon->XcpCommandSequenceRequest) {
                pCon->XcpCommandSequence = pCon->XcpCommandSequenceRequest;
                pCon->XcpCommandSequenceRequest = 0;
                pCon->XcpCommand = XCP_COMMAND_STARTPOINT;
                break;
            }
            if ((pCon->gXcpConnected & CALIBRATION_ACTIVE_MASK) == CALIBRATION_ACTIVE_MASK) { 
                if (pCon->XcpUploadInProgress == TRUE) {
                    if (XCPReadCharacteristics (pCon) == ALL_PARAM_READ) {
                        pCon->XcpUploadInProgress = FALSE;
                    }
                } else XCPWriteCharacteristics (pCon);
                if ((pCon->gXcpConnected & MESSUREMENT_RUNNING_MASK) != MESSUREMENT_RUNNING_MASK) { 
                    if (pCon->XcpCommandSequence == XCP_NO_COMMAND_SEQUENCE) {
                        // Transmit an alive ping if there is no other communication this will check if
                        // the connection to the target is Ok.
                        if (pCon->AlivePingCounter >= pCon->AlivePingTimeout) {
                            pCon->AlivePingCounter = 0;
                            pCon->XcpCommandSequence = XCP_ALIFE_PING;
                            pCon->XcpCommand = XCP_GET_STATUS;
                            AlivePing_CommandScheduer (pCon, x);
                        } else {
                            pCon->AlivePingCounter++;
                        }
                    } else {
                        pCon->AlivePingCounter = 0;
                    }
                }

            }
            break;
        default:
            pCon->XcpCommandSequence = XCP_NO_COMMAND_SEQUENCE;
            ThrowError (1, "Internal error, that should never happen %s(%i)", __FILE__, __LINE__);
            break;
        }
        write_bbvari_udword (pCon->XCPStatusVid, (uint32_t)pCon->gXcpConnected);
        write_bbvari_udword (pCon->XCPCommandVid, (uint32_t)pCon->XcpCommand);
        write_bbvari_udword (pCon->XCPCommandSequenceVid, (uint32_t)pCon->XcpCommandSequence);
    }
}


static int init_xcp_control (void)
{
    int x;
    char txt[BBVARI_NAME_SIZE];
    TASK_CONTROL_BLOCK *pTcb;

    for (x = 0; x < 4; x++) {
        XCP_CONNECTTION *pCon = &XcpConnections[x];
        const char *Prefix = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES);
        pCon->XcpCommandSequence = XCP_NO_COMMAND_SEQUENCE;
        pCon->XcpCommand = XCP_NO_COMMAND;
        pCon->XcpUploadInProgress = 0;
        pCon->XcpByteOrder = 0;
        pCon->gXcpVersion = 0; 

        pCon->gXcpSeed[0] = 0;
        pCon->gXcpSeed[1] = 0;
        pCon->gXcpSeed[2] = 0;
        pCon->gXcpSeed[3] = 0;
        pCon->gXcpSeed[4] = 0;
        pCon->gXcpSeed[5] = 0;
        pCon->gXcpSeedLength = 0;
        pCon->gXcpKey[0] = 0;
        pCon->gXcpKey[1] = 0;
        pCon->gXcpKey[2] = 0;
        pCon->gXcpKey[3] = 0;
        pCon->gXcpKey[4] = 0;
        pCon->gXcpKey[5] = 0;
        pCon->gXcpKeyLength = 0;
        pCon->gXcpProtectionStatus = 0;
        pCon->gXcpConnected = 0;
        pCon->XcpErrCode = 0;

        PrintFormatToString (txt, sizeof(txt), "%s.XCP[%i].Status", Prefix, x);
        pCon->XCPStatusVid = add_bbvari (txt, BB_UDWORD, "");
        PrintFormatToString (txt, sizeof(txt), "%s.XCP[%i].Command", Prefix, x);
        pCon->XCPCommandVid = add_bbvari (txt, BB_UDWORD, "");
        PrintFormatToString (txt, sizeof(txt), "%s.XCP[%i].CommandSequence", Prefix, x);
        pCon->XCPCommandSequenceVid = add_bbvari (txt, BB_UDWORD, "");
        SetStatusVariableTextReplaces (pCon);
        // Send a ping every 200ms if only calibartion is active
        pTcb = GET_TCB ();
        pCon->AlivePingTimeout = 200000 / ((uint64_t)pTcb->time_steps * get_sched_periode_timer_clocks ());
        if (pCon->AlivePingTimeout < 5) pCon->AlivePingTimeout = 5;
        pCon->AlivePingCounter = 0;

    }
    return 0;
}

static void terminate_xcp_control (void)
{
    int x;

    for (x = 0; x < 4; x++) {
        XCP_CONNECTTION *pCon = &XcpConnections[x];
        if (pCon->CcpMeasurementLabelList != NULL) my_free (pCon->CcpMeasurementLabelList);
        pCon->CcpMeasurementLabelList = NULL;
        if (pCon->CcpMeasurementLabelListBuffer != NULL) my_free (pCon->CcpMeasurementLabelListBuffer);
        pCon->CcpMeasurementLabelListBuffer = NULL;
        pCon->CcpMeasurementLabelListSize = 0;

        DeleteCalibrationList (x);
        DeleteDTOsBBVariable (x);

        pCon->XcpCommandSequenceRequest = XCP_NO_COMMAND_SEQUENCE;

        remove_bbvari (pCon->XCPStatusVid);
        remove_bbvari (pCon->XCPCommandVid);
        remove_bbvari (pCon->XCPCommandSequenceVid);
    }
}

TASK_CONTROL_BLOCK xcp_control_tcb
    = INIT_TASK_COTROL_BLOCK("XCP_Control", INTERN_ASYNC, 1, cyclic_xcp_control, init_xcp_control, terminate_xcp_control, 32*1024);



// Interface
int Start_XCP (int ConnectionNo, int CalibOrMeasurment, int Count, char **Variables)
{
    int BufferSize = 10;
    int i;
    char *p;
    XCP_CONNECTTION *pCon;

    // Set text replace
    for (i = 0; i < 4; i++) SetStatusVariableTextReplaces (&XcpConnections[i]);

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) return -1;
    pCon = &XcpConnections[ConnectionNo];

    if (get_pid_by_name ("CANServer") < 0) {
        ThrowError (1, "process \"CANServer\" not running. Start it before using CCP");
        return -1;
    }
    if (pCon->XcpCommandSequenceRequest == 0) {   // No command active
        if (get_pid_by_name ("XCP_Control") < 0) {
            start_process ("XCP_Control");
        }
        // Copy label list
        pCon->CcpMeasurementLabelListSize = Count;
        if (pCon->CcpMeasurementLabelListBuffer != NULL) my_free (pCon->CcpMeasurementLabelListBuffer);
        if (pCon->CcpMeasurementLabelList != NULL) my_free (pCon->CcpMeasurementLabelList);
        for (i = 0; i < Count; i++) BufferSize += (int)strlen (Variables[i]) + 1;
        if ((pCon->CcpMeasurementLabelListBuffer = my_malloc ((size_t)BufferSize)) != NULL) {
            if ((pCon->CcpMeasurementLabelList = my_malloc ((size_t)Count * sizeof (char*))) != NULL) {
                p = pCon->CcpMeasurementLabelListBuffer;
                for (i = 0; i < Count; i++) {
                    StringCopyMaxCharTruncate (p, Variables[i], BufferSize - (p - pCon->CcpMeasurementLabelListBuffer));
                    pCon->CcpMeasurementLabelList[i] = p;
                    p += strlen (p) + 1;
                }
                // start command scheduler
                if (CalibOrMeasurment == START_CALIBRATION) {
                    pCon->XcpCommandSequenceRequest = XCP_START_CALIBRATION;
                } else {
                    pCon->XcpCommandSequenceRequest = XCP_START_MEASUREMENT;
                }
            }
        }
        return 0;
    } else {
        ThrowError (1, "cannot start XCP %s because an other command is runnung", 
            (CalibOrMeasurment == START_CALIBRATION) ? "calibration" : "measurement" );
        return -1;
    }
}

int Stop_XCP (int ConnectionNo, int CalibOrMeasurment)
{
    XCP_CONNECTTION *pCon;

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) return -1;
    pCon = &XcpConnections[ConnectionNo];
    if (get_pid_by_name ("CANServer") < 0) {
        ThrowError (1, "process \"CANServer\" not running. Start it before using CCP");
        return -1;
    }
    if (pCon->XcpCommandSequenceRequest == 0) {   // No command active
        if (CalibOrMeasurment == START_CALIBRATION) {
            pCon->XcpCommandSequenceRequest = XCP_STOP_CALIBRATION;
        } else {
            pCon->XcpCommandSequenceRequest = XCP_STOP_MEASUREMENT;
        }
        return 0;
    } else {
        ThrowError (1, "cannot stop CCP %s because an other command is runnung", 
            (CalibOrMeasurment == START_CALIBRATION) ? "calibration" : "measurement" );
        return -1;
    }
}

int Is_XCP_AllCommandDone (int *pErrorCode)
{
    int x;
    int NegRet = 0;
    XCP_CONNECTTION *pCon;

    for (x = 0; x < 4; x++) {
        pCon = &XcpConnections[x];
        if (pErrorCode != NULL) *pErrorCode |= pCon->XcpErrCode;
        NegRet |= !((pCon->XcpCommandSequenceRequest == 0) && (pCon->XcpCommandSequence == 0) && (pCon->XcpUploadInProgress == FALSE));
    }
    return !NegRet;
}

int Is_XCP_CommandDone (int Connection, int *pErrorCode, char **pErrText)
{
    XCP_CONNECTTION *pCon;

    if ((Connection >= 0) && (Connection < 4)) {
        pCon = &XcpConnections[Connection];
        if (pErrorCode != NULL) *pErrorCode = pCon->XcpErrCode;
        if (pErrText != NULL) *pErrText = XcpErrorStr(pCon->XcpErrCode);
        return ((pCon->XcpCommandSequenceRequest == 0) && (pCon->XcpCommandSequence == 0) && (pCon->XcpUploadInProgress == FALSE));
    } 
    return -1;
}


int RequestECUInfo_XCP (int ConnectionNo)
{
    XCP_CONNECTTION *pCon;

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) {
        ThrowError (1, "Connection number %i too large (0...3)", ConnectionNo);
        return -1;
    }
    pCon = &XcpConnections[ConnectionNo];
    if (get_pid_by_name ("CANServer") < 0) {
        ThrowError (1, "process \"CANServer\" not running. Start it before using XCP");
        return -1;
    }
    if (pCon->XcpCommandSequenceRequest == 0) {   // KNo command active?
        if (get_pid_by_name ("XCP_Control") < 0) {
            start_process ("XCP_Control");
        }
        // Command-Scheduler starten
        pCon->XcpCommandSequenceRequest = XCP_READ_TCU_INFO;
        return 0;
    } else {
        ThrowError (1, "cannot read ECU infos over XCP because an other command is runnung");
        return -1;
    }

}

int GetECUString_XCP (int ConnectionNo, char *String, int maxc)
{
    UNUSED(maxc);
    XCP_CONNECTTION *pCon;
    int Ret;

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) {
        ThrowError (1, "Connection number %i too large (0...3)", ConnectionNo);
        return -1;
    }
    pCon = &XcpConnections[ConnectionNo];
    if (((pCon->gXcpConnected & MESSUREMENT_RUNNING_MASK) == MESSUREMENT_RUNNING_MASK) ||
        ((pCon->gXcpConnected & CALIBRATION_ACTIVE_MASK) == CALIBRATION_ACTIVE_MASK)) {
        pCon->ccp_infos.TargetIdentifier[sizeof(pCon->ccp_infos.TargetIdentifier)-1] = 0;
        PrintFormatToString (String, maxc, "%s", pCon->ccp_infos.TargetIdentifier);
        Ret = 0;
    } else {
        PrintFormatToString (String, maxc, "XCP %i conection not active", ConnectionNo);
        Ret = -1;
    }

    return Ret;
}

int LoadConfig_XCP (int ConnectionNo, char *fname)
{
    int Fd;
    char SectionSrc[256];
    char SectionDst[256];
    char FilenameEnvVarsReplaced[MAX_PATH];

    SearchAndReplaceEnvironmentStrings(fname, FilenameEnvVarsReplaced, sizeof(FilenameEnvVarsReplaced));

    Fd = IniFileDataBaseOpen(FilenameEnvVarsReplaced);
    if (Fd <= 0) {
        return -1;
    }

    PrintFormatToString (SectionSrc, sizeof(SectionSrc), "XCP Configuration");
    PrintFormatToString (SectionDst, sizeof(SectionDst), "XCP Configuration for Target %i", ConnectionNo);

    if (IniFileDataBaseCopySection(GetMainFileDescriptor(), Fd, SectionDst, SectionSrc)) {
        return -1;
    }

    return 0;
}

int SaveConfig_XCP (int ConnectionNo, char *fname)
{
    int Fd;
    char SectionSrc[256];
    char SectionDst[256];

    PrintFormatToString (SectionDst, sizeof(SectionDst), "XCP Configuration");
    PrintFormatToString (SectionSrc, sizeof(SectionSrc), "XCP Configuration for Target %i", ConnectionNo);

    remove (fname);
    Fd = IniFileDataBaseCreateAndOpenNewIniFile(fname);
    if (Fd < 0) {
        return -1;
    }
    IniFileDataBaseCopySection(Fd, GetMainFileDescriptor(), SectionDst, SectionSrc);
    if (IniFileDataBaseSave(Fd, NULL, 2)) {
        ThrowError (1, "cannot save XCP config to file %s", fname);
    }
    return 0;
}


//** CCP Calibration ******************************************************************************************

void StartDownloadCmdSchedule_XCP (uint32_t arg_Address, int arg_Size, void *arg_Data, XCP_CONNECTTION *pCon)
{
    pCon->CalAddress = arg_Address;
    if (arg_Size > XCP_MAX_DOWNLOAD_SIZE) {
        ThrowError (1, "cannot download packages larger than %i Bytes, your package is %i bytes lage", (int)XCP_MAX_DOWNLOAD_SIZE, arg_Size); 
    }
    pCon->CalSize = arg_Size;
    MEMCPY (pCon->CalData, arg_Data, (size_t)arg_Size);
    pCon->XcpCommandSequence = XCP_DOWNLOAD_SEQUENCE;
    pCon->XcpCommand = XCP_SET_MTA;
}


void StartUploadCmdSchedule_XCP (uint32_t arg_Address, int arg_Size, int arg_UploadXcpCalParamPos, XCP_CONNECTTION *pCon)
{
    pCon->CalAddress = arg_Address;
    pCon->CalSize = arg_Size;
    pCon->UploadXcpCalParamPos = arg_UploadXcpCalParamPos;
    pCon->XcpCommandSequence = XCP_UPLOAD_SEQUENCE;
    pCon->XcpCommand = XCP_SET_MTA;
}


void XCPDownload_CmdScheduler (XCP_CONNECTTION *pCon, int Connection)
{
    switch (pCon->XcpCommand)
    {
    case XCP_SET_MTA:
        xcpSetMta (pCon->CalAddress, pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_SET_MTA_WAIT_ACK;
        break;
    case XCP_SET_MTA_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("SET_MTA", pCon);
            pCon->XcpCommand = XCP_DATA_DOWNLOAD;
            pCon->CalPos = 0;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_DATA_DOWNLOAD:
        xcpDownload (&pCon->CalData[pCon->CalPos], x__MIN ((pCon->CalSize - pCon->CalPos), 6), pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_DATA_DOWNLOAD_WAIT_ACK;
        break;
    case XCP_DATA_DOWNLOAD_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("DOWNLOAD", pCon);
            pCon->CalPos += x__MIN ((pCon->CalSize - pCon->CalPos), 6);
            if (pCon->CalPos >= pCon->CalSize) {
                // readback written data
                if (pCon->XcpConfig.ReadParameterAfterCalib) {
                    StartUploadCmdSchedule_XCP (pCon->CalAddress, pCon->CalSize, pCon->XcpCalParamPos, pCon);
                } else {
                    pCon->XcpCommand = 0;
                    pCon->XcpCommandSequence = 0;
                }
            } else {
                pCon->XcpCommand = XCP_DATA_DOWNLOAD;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    }
}



void XCPUpload_CmdScheduler (XCP_CONNECTTION *pCon, int Connection)
{
    union BB_VARI help;
    uint64_t wrflag;

    switch (pCon->XcpCommand)
    {
    case XCP_SET_MTA:
        xcpSetMta (pCon->CalAddress, pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_SET_MTA_WAIT_ACK;
        break;
    case XCP_SET_MTA_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("SET_MTA", pCon);
            pCon->XcpCommand = XCP_UPLOAD;
            pCon->CalPos = 0;
        }
        XCP_CHECK_TIMEOUT();
        break;
    case XCP_UPLOAD:
        xcpUpload (x__MIN ((pCon->CalSize - pCon->CalPos), 7), pCon);
        SetTimeOut (pCon);
        pCon->XcpCommand = XCP_UPLOAD_WAIT_ACK;
        break;
    case XCP_UPLOAD_WAIT_ACK:
        if (XcpCheckAckMessage (pCon, 1) == 1) {
            PrintACKMessage ("UPLOAD", pCon);
            MEMCPY (&(pCon->CalData[pCon->CalPos]), &(pCon->AckCanData[1]), (size_t)x__MIN ((pCon->CalSize - pCon->CalPos), 7));
            pCon->CalPos += x__MIN ((pCon->CalSize - pCon->CalPos), 7);
            if (pCon->CalPos >= pCon->CalSize) {
                pCon->XcpCommand = 0;
                pCon->XcpCommandSequence = 0;
                // Write to the blackboard
                switch (get_bbvaritype(pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid)) {
                case BB_BYTE:
                    help.b = *(signed char*)&pCon->CalData[0]; 
                    write_bbvari_byte (pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, help.b);
                    break;
                case BB_UBYTE:
                    help.ub = *(unsigned char*)&pCon->CalData[0];
                    write_bbvari_ubyte (pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, help.ub);
                    break;
                case BB_WORD:
                    help.w = *(signed short*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST((void*)&help.w, sizeof(help.w));
                    write_bbvari_word (pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, help.w);
                    break;
                case BB_UWORD:
                    help.uw = *(unsigned short*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST((void*)&help.uw, sizeof(help.uw));
                    write_bbvari_uword (pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, help.uw);
                    break;
                case BB_DWORD:
                    help.dw = *(int32_t*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST((void*)&help.dw, sizeof(help.dw));
                    write_bbvari_dword (pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, help.dw);
                    break;
                case BB_UDWORD:
                    help.udw = *(uint32_t*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST((void*)&help.udw, sizeof(help.udw));
                    write_bbvari_udword (pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, help.udw);
                    break;
                case BB_FLOAT:
                    help.f = *(float*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST((void*)&help.f, sizeof(help.f));
                    write_bbvari_float (pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, help.f);
                    break;
                case BB_DOUBLE: // Remark double would be converted to float
                    help.d = *(double*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST((void*)&help.d, sizeof(help.d));
                    write_bbvari_double (pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, help.d);
                    break;
                default:
                    break;
                }
                // Avoid that immediately a download would follow
                get_free_wrflag(get_pid_by_name ("XCP_Control"), &wrflag);
                reset_wrflag(pCon->XcpCalParamList[pCon->UploadXcpCalParamPos].vid, wrflag);
            } else {
                pCon->XcpCommand = XCP_UPLOAD;
            }
        }
        XCP_CHECK_TIMEOUT();
        break;
    }
}


void XCPWriteCharacteristics (XCP_CONNECTTION *pCon)
{
    uint64_t wrflag;
    union BB_VARI help;
    int XcpCalParamPosOld;

    get_free_wrflag(get_pid_by_name ("XCP_Control"), &wrflag);
    XcpCalParamPosOld = pCon->XcpCalParamPos;
    while ((pCon->XcpCalParamList[pCon->XcpCalParamPos].vid < 0) ||  // not a valid blackboard variable
           !test_wrflag(pCon->XcpCalParamList[pCon->XcpCalParamPos].vid, wrflag)) {
        pCon->XcpCalParamPos++;
        if (pCon->XcpCalParamPos >= pCon->XcpCalParamListSize) pCon->XcpCalParamPos = 0;
        if (pCon->XcpCalParamPos == XcpCalParamPosOld) return;  // no write flag set
    }
    reset_wrflag(pCon->XcpCalParamList[pCon->XcpCalParamPos].vid, wrflag);
    switch (get_bbvaritype(pCon->XcpCalParamList[pCon->XcpCalParamPos].vid)) {
    case BB_BYTE:
        help.b = read_bbvari_byte (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid);
        StartDownloadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 1, &help.b, pCon);
        break;
    case BB_UBYTE:
        help.ub = read_bbvari_ubyte (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid);
        StartDownloadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 1, &help.ub, pCon);
        break;
    case BB_WORD:
        help.w = read_bbvari_word (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST((void*)&help.w, sizeof(help.w));
        StartDownloadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 2, &help.w, pCon);
        break;
    case BB_UWORD:
        help.uw = read_bbvari_uword (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST((void*)&help.uw, sizeof(help.uw));
        StartDownloadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 2, &help.uw, pCon);
        break;
    case BB_DWORD:
        help.dw = read_bbvari_dword (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST((void*)&help.dw, sizeof(help.dw));
        StartDownloadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 4, &help.dw, pCon);
        break;
    case BB_UDWORD:
        help.udw = read_bbvari_udword (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST((void*)&help.udw, sizeof(help.udw));
        StartDownloadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 4, &help.udw, pCon);
        break;
    case BB_FLOAT:
        help.f = read_bbvari_float (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST((void*)&help.f, sizeof(help.f));
        StartDownloadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 4, &help.f, pCon);
        break;
    case BB_DOUBLE:  // Remark double would be converted to float
        help.d = read_bbvari_double (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST((void*)&help.d, sizeof(help.d));
        StartDownloadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 8, &help.d, pCon);
        break;
    default:
        break;
    }
}

int XCPReadCharacteristics (XCP_CONNECTTION *pCon)
{
    if (pCon->XcpCalParamPos >= pCon->XcpCalParamListSize) {
        // End of the list, all choosen parameter has been read
        pCon->XcpCalParamPos = 0;
        return ALL_PARAM_READ;
    } else {
        if (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid > 0) {   // valid blackboard variable
            // Next element of the paramteter list
            StartUploadCmdSchedule_XCP (pCon->XcpCalParamList[pCon->XcpCalParamPos].Address, 
                                        bbvari_sizeof(get_bbvaritype (pCon->XcpCalParamList[pCon->XcpCalParamPos].vid)),
                                        pCon->XcpCalParamPos, pCon);
        }
        pCon->XcpCalParamPos++;
        return MORE_PARAM_READ_PENDING;
    }
}


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


#include "Platform.h"
#include <stdio.h>
#include <stdlib.h>
#include "Message.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "IniDataBase.h"
#include "Files.h"
#include "ThrowError.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "MyMemory.h"
#include "EnvironmentVariables.h"
#include "LoadSaveToFile.h"
#include "EquationParser.h"
#include "ConfigurablePrefix.h"
#include "CcpMessages.h"
#include "CcpControl.h"

#define UNUSED(x) (void)(x)

/*  CCP Default Parameters */
#define CCP_MASTER_ADDR          0
#define CCP_CTR                  0xAA


#define ALL_PARAM_READ           0
#define MORE_PARAM_READ_PENDING  1


typedef struct _CcpCalParamItem {
    int vid;
    uint32_t Address;
} CcpCalParamItem;


typedef struct {
    // This must be set to true after start of a calibration session
    // to force a update of all selected parameters
    int CppUploadInProgress;

    // Is it a LSB or MSB first target
    int CcpByteOrder;       // 0 == LSB first 1 == MSB first

    // Which protocol version should be used
    // Only 201 will be supported
    int gCcpVersion;

    // CCP Seed and the protection state
    unsigned char gCcpSeed[4];
    unsigned char gCcpProtectionStatus;

    // Priviledge Level fuer Seed & Key
    #define PL_CAL 0x01
    #define PL_DAQ 0x02
    #define PL_PGM 0x40

    // State of the connection
    int gCcpConnected;
    #define CONNECTED_MASK              0x00000001
    #define MEASUREMENT_RUNNING_MASK    0x00000002
    #define CALIBRATION_ACTIVE_MASK     0x00000004

    int DebugMessages;

    uint64_t AlivePingTimeout;
    uint64_t AlivePingCounter;

    // Base configuration
    CCP_CONFIG CcpConfig;

    // This structure store the assembling of the data transmition objects (DTO)
    CCP_VARIABLES_CONFIG dto_conf;

    CCP_INFOS ccp_infos;
    int CcpErrCode;

    VID CCPVersionVid;
    VID CCPCommandVid;
    VID CCPCommandSequenceVid;
    VID CCPStatusVid;

    // downward compatibility only channel 0
    VID CCPVersionVidOld;
    VID CCPCommandVidOld;
    VID CCPCommandSequenceVidOld;
    VID CCPStatusVidOld;

    int CcpCalParamListSize;
    int CcpCalParamPos;

    CcpCalParamItem *CcpCalParamList;

    //- CCP state variables ----------------------------------------------
    int CcpCommandSequence;
    int CcpCommandSequenceRequest;
    #define CCP_NO_COMMAND_SEQUENCE             0
    #define CCP_READ_TCU_INFO                   1
    #define CCP_START_MEASUREMENT               2 
    #define CCP_STOP_MEASUREMENT                3  
    #define CCP_READ_MEMORY                     4 
    #define CCP_DOWNLOAD_SEQUENCE               5
    #define CCP_UPLOAD_SEQUENCE                 6
    #define CCP_START_CALIBRATION               7
    #define CCP_STOP_CALIBRATION                8
    #define CCP_ALIFE_PING                      9

    int CcpCommand;
    #define CCP_NO_COMMAND                      0
    #define CCP_GET_VERSION                     1
    #define CCP_GET_VERSION_WAIT_ACK            2 
    #define CCP_EXCHANGE_ID                     3
    #define CCP_EXCHANGE_ID_WAIT_ACK            4
    #define CCP_DATA_UPLOAD                     5
    #define CCP_DATA_UPLOAD_WAIT_ACK            6
    #define CCP_READ_TCU_INFO_FINISHED          7

    #define CCP_CONNECT                         8 
    #define CCP_CONNECT_WAIT_ACK                9  
    #define CCP_GET_DAQ_LIST_SIZE              10
    #define CCP_GET_DAQ_LIST_SIZE_WAIT_ACK     11 
    #define CCP_BUILD_DTOS                     12
    #define CCP_SET_DAQ_LIST_PTR               13
    #define CCP_SET_DAQ_LIST_PTR_WAIT_ACK      14 
    #define CCP_WRITE_DAQ_LIST_ENTRY           15
    #define CCP_WRITE_DAQ_LIST_ENTRY_WAIT_ACK  16
    #define CCP_PREPARE_DAQ_LIST               17
    #define CCP_PREPARE_DAQ_LIST_WAIT_ACK      18 
    #define CCP_START_ALL_DAQ_LISTS            19
    #define CCP_START_ALL_DAQ_LISTS_WAIT_ACK   20
    #define CCP_START_MEASUREMENT_FINISHED     21

    #define CCP_STOP_ALL_DAQ_LIST              22
    #define CCP_STOP_ALL_DAQ_LIST_WAIT_ACK     23
    #define CCP_STOP_MEASUREMENT_FINISHED      24
    #define CCP_DISCONNECT                     25
    #define CCP_DISCONNECT_WAIT_ACK            26
    #define CCP_SET_MTA                        27
    #define CCP_SET_MTA_WAIT_ACK               28
    #define CCP_SET_MTA0                       29
    #define CCP_SET_MTA0_WAIT_ACK              30
    #define CCP_SET_MTA1                       31
    #define CCP_SET_MTA1_WAIT_ACK              32
    #define CCP_SET_MTA2                       33
    #define CCP_SET_MTA2_WAIT_ACK              34
    #define CCP_DATA_DOWNLOAD                  35
    #define CCP_DATA_DOWNLOAD_WAIT_ACK         36
    #define CCP_MOVE_MEMORY                    37
    #define CCP_MOVE_MEMORY_WAIT_ACK           38
    #define CCP_SELECT_CAL_PAGE                39
    #define CCP_SELECT_CAL_PAGE_WAIT_ACK       40
    #define CCP_START_CALIBRATION_FINISHED     41
    #define CCP_STOP_CALIBRATION_FINISHED      42
    #define CCP_GET_SEED                       43
    #define CCP_GET_SEED_WAIT_ACK              44
    #define CCP_UNLOCK                         45
    #define CCP_UNLOCK_WAIT_ACK                46
    #define CCP_GET_STATUS_FLAGS               47
    #define CCP_GET_STATUS_FLAGS_ACK           48
    #define CCP_SET_STATUS_FLAGS               49
    #define CCP_SET_STATUS_FLAGS_ACK           50
    #define CCP_MEASUREMENT_STOPPED            51
    #define CCP_GET_SEED_2                     52
    #define CCP_GET_SEED_WAIT_ACK_2            53
    #define CCP_UNLOCK_2                       54
    #define CCP_UNLOCK_WAIT_ACK_2              55
    #define CCP_GET_STATUS_FLAGS_2             56
    #define CCP_GET_STATUS_FLAGS_ACK_2         57
    #define CCP_SET_STATUS_FLAGS_2             58
    #define CCP_SET_STATUS_FLAGS_ACK_2         59
    #define CCP_WAIT_FIRST_DATA                60
    #define CCP_COMMAND_STARTPOINT            100

    char **CcpMeasurementLabelList;
    int CcpMeasurementLabelListSize;
    char *CcpMeasurementLabelListBuffer;

    // Calibration
#define CCP_MAX_DOWNLOAD_SIZE     8
    unsigned char CalData[CCP_MAX_DOWNLOAD_SIZE];  // Max. double
    int CalSize;
    uint32_t CalAddress;
    int CalPos;

// Position of the current downloding parameter
    int UploadCcpCalParamPos;

    int OdtPtr;  // o
    int DaqPtr;  // i
    int SizeOfTCUString;
    int TCUStringPointer;

    int SessionStatus;
    int SessionStatusCounter;
    int DAQSize;
    int DAQPid;


} CCP_CONNECTTION;

static CCP_CONNECTTION CcpConnections[4];


void Download_CmdScheduler (CCP_CONNECTTION *pCon, int Connection);
void Upload_CmdScheduler (CCP_CONNECTTION *pCon, int Connection);
void CCPWriteCharacteristics (CCP_CONNECTTION *pCon);
int CCPReadCharacteristics (CCP_CONNECTTION *pCon);


#define SWAP_TO_MSB_FIRST(p,n) if (pCon->CcpByteOrder) SwapToMsbFirst(p,n);

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


/* CCP-Commands */

/* Basic */
#define CC_CONNECT           0x01
#define CC_SET_MTA           0x02
#define CC_DNLOAD            0x03
#define CC_UPLOAD            0x04
#define CC_TEST              0x05 /* V2.1 */
#define CC_START_STOP        0x06
#define CC_DISCONNECT        0x07
#define CC_START_STOP_ALL    0x08 /* V2.1 */
#define CC_SHORT_UPLOAD      0x0F
#define CC_GET_DAQ_SIZE      0x14
#define CC_SET_DAQ_PTR       0x15
#define CC_WRITE_DAQ         0x16
#define CC_EXCHANGE_ID       0x17
#define CC_GET_CCP_VERSION   0x1B /* V2.1 */
#define CC_DNLOAD6           0x23

/* Optional */
#define CC_GET_CAL_PAGE      0x09
#define CC_SET_S_STATUS      0x0C
#define CC_GET_S_STATUS      0x0D
#define CC_BUILD_CHKSUM      0x0E
#define CC_CLEAR_MEMORY      0x10
#define CC_SET_CAL_PAGE      0x11
#define CC_GET_SEED          0x12
#define CC_UNLOCK            0x13
#define CC_PROGRAM           0x18
#define CC_MOVE_MEMORY       0x19
#define CC_DIAG_SERVICE      0x20
#define CC_ACTION_SERVICE    0x21
#define CC_PROGRAM6          0x22


/* Error messages */
#define CRC_OK                 0x00
#define CRC_DAQ_OVERLOAD       0x01
#define CRC_CMD_BUSY           0x10
#define CRC_DAQ_BUSY           0x11
#define CRC_TIMEOUT            0x12
#define CRC_KEY_REQUEST        0x18
#define CRC_SS_REQUEST         0x19
#define CRC_COLD_START_REQUEST 0x20
#define CRC_CMD_UNKNOWN        0x30
#define CRC_CMD_SYNTAX         0x31
#define CRC_CMD_RANGE          0x32
#define CRC_NO_ACCESS          0x33
#define CRC_OVERLOAD           0x34
#define CRC_LOCKED             0x35
#define CRC_RESOURCE_NAV       0x36

static char *CCPErrorNoToString(int err)
{
    switch (err) {
    case CRC_CMD_BUSY:
        return "CMD Busy";
    case CRC_DAQ_BUSY:
        return "DAQ Busy";
    case CRC_DAQ_OVERLOAD:
        return "DAQ Overload";
    case CRC_KEY_REQUEST:
        return "Key Request";
    case CRC_COLD_START_REQUEST:
        return "Cold Start Request";
    case CRC_TIMEOUT:
        return "Internal Timeout";
    case CRC_CMD_UNKNOWN:
        return "Unknown Command";
    case CRC_CMD_SYNTAX:
        return "Command Syntax";
    case CRC_CMD_RANGE:
        return "Parameter out of Range";
    case CRC_NO_ACCESS:
        return "No Access";
    case CRC_RESOURCE_NAV:
        return "Resource not available";
    case -1:
        return "Timeout";
    }
    return "Unknown CCP Errorcode";
}


static void FillCanObject (CCP_CRO_CAN_OBJECT *pCanObj, CCP_CONNECTTION *pCon)
{
    MEMSET (pCanObj->Data, 0, 8);
    pCanObj->Channel = pCon->CcpConfig.Channel;
    pCanObj->Id = pCon->CcpConfig.CRO_id;
    pCanObj->ExtFlag = pCon->CcpConfig.ExtIds;
    pCanObj->Size = 8;
}

static void DebugErrorMessage (CCP_CONNECTTION *pCon, CCP_CRO_CAN_OBJECT *pCanObj, char *Text)
{
    if (pCon->DebugMessages) {
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

static void PrintACKMessage (CCP_CONNECTTION *pCon, char *CmdText, CCP_CRO_CAN_OBJECT *pCanObject)
{
    int CANChannel;
    if (pCon->DebugMessages) {
        CANChannel = pCanObject->Channel-4;
        if (CANChannel < 0) CANChannel = 0;
        if (CANChannel > 3) CANChannel = 3;
        CANChannel = CcpConnections[CANChannel].CcpConfig.Channel;
        ThrowError (INFO_NO_STOP, "%s_ACK (%i/0x%X): %02X %02X %02X %02X %02X %02X %02X %02X", 
            CmdText, CANChannel, pCanObject->Id, 
            (int)pCanObject->Data[0], (int)pCanObject->Data[1], (int)pCanObject->Data[2], (int)pCanObject->Data[3], 
            (int)pCanObject->Data[4], (int)pCanObject->Data[5], (int)pCanObject->Data[6], (int)pCanObject->Data[7]);
    }
}


/* tansmit functionen for XCP commands */

// Upload a data block unlimited lengths
// MTA have to be set before
static int ccpDataUpload (int size, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_UPLOAD;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)size;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "UPLOAD");
    return 1;
}


// Request for identification
static int ccpExchangeId(CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_EXCHANGE_ID;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = CCP_MASTER_ADDR;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "EXCHANGE_ID");
    return 1;
}



// Request for version
static int ccpGetVersion (CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_GET_CCP_VERSION;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = 2;
    CanObject.Data[3] = 1;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "GET_CCP_VERSION");
    return 1;
}

// Connect
static int ccpConnect (int id, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_CONNECT;
    CanObject.Data[1] = CCP_CTR;
    *(unsigned short*)(void*)&CanObject.Data[2] = (unsigned short)id;
    if (pCon->gCcpVersion < 0x0201) { // From V2.1 always LSB first format
        SWAP_TO_MSB_FIRST (&CanObject.Data[2],2);
    }
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "CONNECT");
    return 1;
}

// Disconnect
int ccpDisconnect (int temporary, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_DISCONNECT;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)(temporary ? 0x00 : 0x01);
    CanObject.Data[3] = 0;
    *(unsigned short*)(void*)&CanObject.Data[4] = (unsigned short)pCon->CcpConfig.StationAddress;
    if (pCon->gCcpVersion < 0x0201) { //  From V2.1 always LSB first format
        SWAP_TO_MSB_FIRST (&CanObject.Data[4],2);
    }
    CanObject.Data[6] = 0;
    CanObject.Data[7] = 0;

    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "DISCONNECT");
    pCon->gCcpConnected = 0;
    return 1;
}

#ifdef _WIN32
typedef unsigned long (__cdecl* ASAP1A_CCP_COMPUTEKEYFROMSEED) (unsigned char *seed,
                                                               unsigned short sizeSeed,
                                                               unsigned char *key,
                                                               unsigned short maxSizeKey,
                                                               unsigned short *sizeKey);
#endif

// Unlock a resource with Seed&Key
int ccpUnlockResource (unsigned char resource, CCP_CONNECTTION *pCon)
{
#ifdef _WIN32
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    if (pCon->gCcpProtectionStatus) { // protected?
        HANDLE hLibrary;
        unsigned short KeySize;
        int x;
        int Ret;
        char *SeedAndKeyDll;

        for (x = 2; x < 8; x++) CanObject.Data[x] = 0;

        if (pCon->CcpConfig.SeedKeyFlag != CCP_SEED_AND_KEY_SIM) {
            switch (resource) {
            case PL_CAL:
                SeedAndKeyDll = pCon->CcpConfig.SeedKeyDllCal;
                break;
            default:
            case PL_DAQ:
                SeedAndKeyDll = pCon->CcpConfig.SeedKeyDll;
                break;
            }

            hLibrary = LoadLibrary (SeedAndKeyDll);
            if (hLibrary == NULL)  {
                ThrowError (1, "cannot load CCP Seed&Key DLL \"%s\"", SeedAndKeyDll);
            } else  {
                ASAP1A_CCP_COMPUTEKEYFROMSEED ASAP1A_CCP_ComputeKeyFromSeed;
                ASAP1A_CCP_ComputeKeyFromSeed = (ASAP1A_CCP_COMPUTEKEYFROMSEED)GetProcAddress (hLibrary, "ASAP1A_CCP_ComputeKeyFromSeed");
                if (ASAP1A_CCP_ComputeKeyFromSeed == NULL) {
                    ThrowError (1, "there is no function ASAP1A_CCP_ComputeKeyFromSeed in (%s) DLL", SeedAndKeyDll);
                } else {
                    Ret = (int)ASAP1A_CCP_ComputeKeyFromSeed (pCon->gCcpSeed, 4, &CanObject.Data[2], 6, &KeySize);
                    if (Ret == 0) {
                        ThrowError (1, "CCP Seed&Key function ASAP1A_CCP_ComputeKeyFromSeed (%s) return value is %i", SeedAndKeyDll, Ret);
                    }
                }
                FreeLibrary (hLibrary);
            }
        }
        // Unlock
        CanObject.Data[0] = CC_UNLOCK;
        CanObject.Data[1] = CCP_CTR;

        TransmitCROMessage (&CanObject);
        DebugErrorMessage (pCon, &CanObject, "UNLOCK");
        return 1;
    } else {
        return 0;
    }
#else
    // TODO
    UNUSED(resource);
    UNUSED(pCon);
    ThrowError (1, "not implemented yet");
    return 0;
#endif

}

// Seed&Key
static int ccpUnlockProtection (unsigned char privilegeLevel, CCP_CONNECTTION *pCon)
{
    if (pCon->gCcpVersion >= 0x0201) {
        // from CCP2.1 each resource must unlock separate
        if (privilegeLevel & PL_CAL) {
            if (!ccpUnlockResource (PL_CAL, pCon)) return 0;
        }
        if (privilegeLevel & PL_DAQ) {
            if (!ccpUnlockResource (PL_DAQ, pCon)) return 0;
        }
        if (privilegeLevel & PL_PGM) {
            if (!ccpUnlockResource (PL_PGM, pCon)) return 0;
        }
    } else {
        if (!ccpUnlockResource (privilegeLevel, pCon)) return 0;
    }
    return 1;
}

// fetch seed
static int ccpGetSeed (unsigned char resource, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_GET_SEED;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = resource;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "GET_SEED");
    return 1;
}

//------------------------------------------------------------------------------
// DAQ

// Get the number of ODTs inside a DAQ-List
int ccpGetDaqListSize(int daq_num, uint32_t daq_id, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_GET_DAQ_SIZE;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)daq_num;
    *(uint32_t*)(void*)&CanObject.Data[4] = daq_id;
    SWAP_TO_MSB_FIRST (&CanObject.Data[4],4);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "GET_DAQ_SIZE");
    return 1;
}

// Set DAQ list pointer
static int ccpSetDaqListPtr (int daq_num, int odt, int idx, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_SET_DAQ_PTR;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)daq_num;
    CanObject.Data[3] = (unsigned char)odt;
    CanObject.Data[4] = (unsigned char)idx;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "SET_DAQ_PTR");
    return 1;
}

// Write to a DAQ list pointer
static int ccpWriteDaqListEntry (unsigned int size, unsigned int ext, unsigned int addr, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_WRITE_DAQ;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)size;
    CanObject.Data[3] = (unsigned char)ext;
    *(unsigned int*)(void*)&CanObject.Data[4] = addr;
    SWAP_TO_MSB_FIRST(&CanObject.Data[4],4);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "WRITE_DAQ");
    return 1;
}

// Prepare DAQ list (V2.1)
static int ccpPrepareDaqList (int daq, int last, int channel, int prescaler, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_START_STOP;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = 2; // Prepare
    CanObject.Data[3] = (unsigned char)daq;
    CanObject.Data[4] = (unsigned char)last;
    CanObject.Data[5] = (unsigned char)channel;
    *(unsigned short*)(void*)&CanObject.Data[6] = (unsigned short)prescaler;
    SWAP_TO_MSB_FIRST(&CanObject.Data[6],2);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "START_STOP");
    return 1;
}


// Start DAQ list (V2.0)
static int ccpStartDaqList_0200 (unsigned char daq, unsigned char last, unsigned char interval, CCP_CONNECTTION *pCon)
{
    UNUSED(interval);
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_START_STOP;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = 1; // Start
    CanObject.Data[3] = daq;
    CanObject.Data[4] = last;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "START_STOP");
    return 1;
}

// Stop DAQ list (V2.0)
static int ccpStopDaqList_0200 (unsigned char daq, unsigned char last, CCP_CONNECTTION *pCon)
{
    UNUSED(last);
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_START_STOP;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = 0; // Stop
    CanObject.Data[3] = daq;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "START_STOP");
    return 1;
}

// Stop DAQ list (V2.1)
static int ccpStopDaqList (unsigned int daq, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_START_STOP;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = 0; // Stop
    CanObject.Data[3] = (unsigned char)daq;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "START_STOP");
    return 1;
}

// Start all DAQ lists
static int ccpStartAllDaqLists (CCP_CONNECTTION *pCon)
{
    // CCP V2.1
    if (pCon->gCcpVersion >= 0x0201) {
        CCP_CRO_CAN_OBJECT CanObject;
        FillCanObject (&CanObject, pCon);
        CanObject.Data[0] = CC_START_STOP_ALL;
        CanObject.Data[1] = CCP_CTR;
        CanObject.Data[2] = 1; // Start
        TransmitCROMessage (&CanObject);
        DebugErrorMessage (pCon, &CanObject, "START_STOP_ALL");
        return 1;
    }
    // Emulation for CCP V2.0
    else {
        return ccpStartDaqList_0200(0, (unsigned char)pCon->dto_conf.DTOPackagesCount, 0x10, pCon);
    }
}

// Stop all DAQ lists
static int ccpStopAllDaqLists (CCP_CONNECTTION *pCon)
{
    // CCP V2.1
    if (pCon->gCcpVersion >= 0x0201) {
        CCP_CRO_CAN_OBJECT CanObject;
        FillCanObject (&CanObject, pCon);
        CanObject.Data[0] = CC_START_STOP_ALL;
        CanObject.Data[1] = CCP_CTR;
        CanObject.Data[2] = 0; // Stop
        TransmitCROMessage (&CanObject);
        DebugErrorMessage (pCon, &CanObject, "START_STOP_ALL");
        return 1;
    }
    // Emulation for CCP V2.0
    else {
        return ccpStopDaqList_0200(0, (unsigned char)pCon->dto_conf.DTOPackagesCount, pCon);
    }
}

static int ccpGetSStatus (CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_GET_S_STATUS;
    CanObject.Data[1] = CCP_CTR;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "GET_S_STATUS");
    return 1;
}


static int ccpSetSStatus (int SessionStatus, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_SET_S_STATUS;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)SessionStatus;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "SET_S_STATUS");
    return 1;
}

static int ccpDownload (const unsigned char *Data, int Size, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_DNLOAD;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)Size;
    MEMCPY ((void*)&CanObject.Data[3], (const void*)Data, (size_t)Size);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "DNLOAD");
    return 1;
}

static int ccpUpload (int Size, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject,pCon);
    CanObject.Data[0] = CC_UPLOAD;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)Size;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "UPLOAD");
    return 1;
}

static int ccpSetMta (int Nr, uint32_t arg_Address, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_SET_MTA;
    CanObject.Data[1] = CCP_CTR;
    CanObject.Data[2] = (unsigned char)Nr;             // 0 oder 1
    CanObject.Data[3] = 0;
    *(uint32_t*)(void*)&CanObject.Data[4] = arg_Address;
    SWAP_TO_MSB_FIRST (&CanObject.Data[4],4);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "SET_MTA");
    return 1;
}

static int ccpMoveMemory (uint32_t arg_Size, CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_MOVE_MEMORY;
    CanObject.Data[1] = CCP_CTR;
    *(uint32_t*)(void*)&CanObject.Data[2] = arg_Size;
    SWAP_TO_MSB_FIRST (&CanObject.Data[2],4);
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "MOVE_MEMORY");
    return 1;
}

static int ccpSelectCalPage (CCP_CONNECTTION *pCon)
{
    CCP_CRO_CAN_OBJECT CanObject;

    FillCanObject (&CanObject, pCon);
    CanObject.Data[0] = CC_SET_CAL_PAGE;
    CanObject.Data[1] = CCP_CTR;
    TransmitCROMessage (&CanObject);
    DebugErrorMessage (pCon, &CanObject, "SET_CAL_PAGE");
    return 1;
}

static int TranslateCCPVersionString2Number (char *VersionString)
{
    if (!strcmp (VersionString, "2.0")) {
        return 0x200;
    } else if (!strcmp (VersionString, "2.1")) {
        return 0x201;
    } else if (!strcmp (VersionString, "2.1 ESV1")) {
        return 0x10000201;
    } else if (!strcmp (VersionString, "2.1 S_STATUS")) {
        return 0x20000201;
    } else {
        return 0x201;
    }
}

static int CcpReadConfigFromIni (int Connection)
{
    char txt[INI_MAX_LINE_LENGTH];
    char Section[256];
    CCP_CONNECTTION *pCon;
    double Erg;
    int Fd = GetMainFileDescriptor();

    pCon = &CcpConnections[Connection];
    PrintFormatToString (Section, sizeof(Section), "CCP Configuration for Target %i", Connection);
    IniFileDataBaseReadString (Section, "Debug", "no", txt, sizeof (txt), Fd);
    if (!strcmp ("yes", txt)) pCon->DebugMessages = 1;
    else pCon->DebugMessages = 0;
    IniFileDataBaseReadString (Section, "Protocol Version", "2.1", txt, sizeof (txt), Fd);
    pCon->CcpConfig.CcpVersion = TranslateCCPVersionString2Number (txt);
    pCon->CcpConfig.Channel = IniFileDataBaseReadInt (Section, "Channel", 0, Fd);
    IniFileDataBaseReadString (Section, "ByteOrder", "", txt, sizeof (txt), Fd);
    if (!strcmpi ("msb_first", txt)) pCon->CcpConfig.ByteOrder = 1;
    else pCon->CcpConfig.ByteOrder = 0;
    pCon->CcpByteOrder = pCon->CcpConfig.ByteOrder;
    IniFileDataBaseReadString (Section, "CRO_ID", "0", txt, sizeof (txt), Fd);
    pCon->CcpConfig.CRO_id = strtoul (txt, NULL, 0);
    IniFileDataBaseReadString (Section, "CRM_ID", "0", txt, sizeof (txt), Fd);
    pCon->CcpConfig.CRM_id = strtoul (txt, NULL, 0);
    IniFileDataBaseReadString (Section, "DTO_ID", "0", txt, sizeof (txt), Fd);
    pCon->CcpConfig.DTO_id = strtoul (txt, NULL, 0);
    pCon->CcpConfig.ExtIds = IniFileDataBaseReadInt (Section, "ExtIds", 0, Fd);
    pCon->CcpConfig.Timeout = IniFileDataBaseReadInt (Section, "Timeout", 500, Fd);  // in ms
    IniFileDataBaseReadString (Section, "StationAddress", "0x0039", txt, sizeof (txt), Fd);
    pCon->CcpConfig.StationAddress = strtol (txt, NULL, 0);
    pCon->CcpConfig.Prescaler = IniFileDataBaseReadInt (Section, "Prescaler", 1, Fd);
    pCon->CcpConfig.EventChannel = IniFileDataBaseReadInt (Section, "EventChannel", 0, Fd);
    IniFileDataBaseReadString (Section, "OnlyBytePointerInDAQ", "no", txt, sizeof(txt), Fd);
    pCon->CcpConfig.OnlyBytePointerInDAQ = !strcmp ("yes", txt);

    // Seed&Key  
    IniFileDataBaseReadString (Section, "SeedKeyDll", "", pCon->CcpConfig.SeedKeyDll, sizeof(pCon->CcpConfig.SeedKeyDll), Fd);
    if (!strcmp (txt, "BB")) {   // Hitorische Altlast wenn in der Seed&Key-Dll "BB" steht wird dieses simuliert
        pCon->CcpConfig.SeedKeyFlag = CCP_SEED_AND_KEY_SIM;
    } else {
        if (IniFileDataBaseReadInt (Section, "SeedKeyFlag", 0, Fd)) {
            pCon->CcpConfig.SeedKeyFlag = CCP_SEED_AND_KEY;
        } else {
            pCon->CcpConfig.SeedKeyFlag = CCP_NO_SEED_AND_KEY;
        }
    }
    if (pCon->CcpConfig.CcpVersion >= 0x201) {
        IniFileDataBaseReadString (Section, "SeedKeyDllCal", "", pCon->CcpConfig.SeedKeyDllCal, sizeof(pCon->CcpConfig.SeedKeyDllCal), Fd);
    } else {
        StringCopyMaxCharTruncate (pCon->CcpConfig.SeedKeyDllCal, pCon->CcpConfig.SeedKeyDll, sizeof(pCon->CcpConfig.SeedKeyDllCal));
    }
    // Achtung default sollte eigentlich "no" und Adresse 0x0 sein
    IniFileDataBaseReadString (Section, "CalibrationEnable", "yes", txt, sizeof(txt), Fd);
    pCon->CcpConfig.CalibrationEnable = !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "MoveROM2RAM", "yes", txt, sizeof(txt), Fd);
    pCon->CcpConfig.MoveROM2RAM =  !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "SelCalPage", "yes", txt, sizeof(txt), Fd);
    pCon->CcpConfig.SelCalPage =  !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "CalibROMStartAddr", "0x0", txt, sizeof (txt), Fd);
    SearchAndReplaceEnvironmentStrings (txt, txt, sizeof(txt));
    if (direct_solve_equation_err_sate (txt, &Erg)) {
        ThrowError (1, "CCP cannot set CalibROMStartAddr = \"%s\"", txt);
        pCon->CcpConfig.CalibROMStartAddr = 0;
    } else {
        pCon->CcpConfig.CalibROMStartAddr = (uint32_t)Erg;
    }
    IniFileDataBaseReadString (Section, "CalibRAMStartAddr", "0x0", txt, sizeof (txt), Fd);
    SearchAndReplaceEnvironmentStrings (txt, txt, sizeof(txt));
    if (direct_solve_equation_err_sate (txt, &Erg)) {
        ThrowError (1, "CCP cannot set CalibRAMStartAddr = \"%s\"", txt);
        pCon->CcpConfig.CalibRAMStartAddr = 0;
    } else {
        pCon->CcpConfig.CalibRAMStartAddr = (uint32_t)Erg;
    }
    IniFileDataBaseReadString (Section, "CalibROMRAMSize", "0x0", txt, sizeof (txt), Fd);
    SearchAndReplaceEnvironmentStrings (txt, txt, sizeof(txt));
    if (direct_solve_equation_err_sate (txt, &Erg)) {
        ThrowError (1, "CCP cannot set CalibROMRAMSize = \"%s\"", txt);
        pCon->CcpConfig.CalibROMRAMSize = 0;
    } else {
        pCon->CcpConfig.CalibROMRAMSize = (uint32_t)Erg;
    }
    pCon->CcpConfig.ReadParameterAfterCalib = IniFileDataBaseReadInt (Section, "ReadParameterAfterCalib", 0, Fd);

    IniFileDataBaseReadString (Section, "UseUnit", "no", txt, sizeof(txt), Fd);
    pCon->CcpConfig.UseUnitFlag = !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "UseConversion", "no", txt, sizeof(txt), Fd);
    pCon->CcpConfig.UseConvFlag = !strcmp ("yes", txt);
    IniFileDataBaseReadString (Section, "UseMinMax", "no", txt, sizeof(txt), Fd);
    pCon->CcpConfig.UseMinMaxFlag = !strcmp ("yes", txt);

    pCon->CcpConfig.XcpOrCcp = 0; // CCP-Verbinung ist immer die Connection Nummern 4...7
    {
        int CcpConnectionNo = Connection + 4;
        write_message (get_pid_by_name ("CANServer"), CCP_SET_ACTIVE_CON_NO, sizeof (int), (char*)&CcpConnectionNo);
    }
    write_message (get_pid_by_name ("CANServer"), CCP_SET_CRM_AND_DTOS_IDS, sizeof (pCon->CcpConfig), (char*)&pCon->CcpConfig);
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
    int x;
    CCP_CONNECTTION *pCon;

    pCon = &CcpConnections[Connection];
    if (pCon->CcpCalParamList != NULL) {
        for (x = 0; x < pCon->CcpCalParamListSize; x++) {
            if (pCon->CcpCalParamList[x].vid > 0) {
                remove_bbvari (pCon->CcpCalParamList[x].vid);
            }
        }
        my_free (pCon->CcpCalParamList);
        pCon->CcpCalParamList = NULL;
        pCon->CcpCalParamListSize = 0;
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
    CCP_CONNECTTION *pCon;
    uint32_t Offset = 0;
    char *Unit, *Conversion, *MinString, *MaxString;
    int Fd = GetMainFileDescriptor();
    int *ParamIdxs = NULL;
    int Ret = 0;

    PrintFormatToString (Section, sizeof(Section), "%%XILENV_CCP%i_PARAM_OFFSET%%", par_Connection);
    SearchAndReplaceEnvironmentStrings (Section, txt, sizeof (txt));
    if (!strcmp (Section, txt) && (par_Connection == 0)) {
        PrintFormatToString (Section, sizeof(Section), "%%XILENV_CCP_PARAM_OFFSET%%");
        SearchAndReplaceEnvironmentStrings (Section, txt, sizeof (txt));
    }
    if (strcmp (Section, txt)) {
        Offset = strtoul (txt, NULL, 0);
    }
    pCon = &CcpConnections[par_Connection];
    PrintFormatToString (Section, sizeof(Section), "CCP Configuration for Target %i", par_Connection);
    DeleteCalibrationList (par_Connection);
    /*if (CcpCalParamList != NULL) my_free (CcpCalParamList);
    CcpCalParamList = NULL;
    CcpCalParamListSize = 0;*/
    pCon->CcpCalParamList = (CcpCalParamItem*)my_malloc ((size_t)par_ParamCount * sizeof (CcpCalParamItem));
    if (pCon->CcpCalParamList == NULL) {
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

    pCon->CcpCalParamListSize = par_ParamCount;
    pCon->CcpCalParamPos = 0;
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
                    pCon->CcpCalParamList[x].Address = Offset + strtoul (address_str, NULL, 0);
                    pCon->CcpCalParamList[x].vid = add_bbvari (name, dtype, 
                                                               (pCon->CcpConfig.UseUnitFlag) ? Unit : NULL);
                    if (pCon->CcpCalParamList[x].vid < 0) {
                        ThrowError (1, "CCP cannot add calibration parameter \"%s\" to blackboard (%i)", name, pCon->CcpCalParamList[x].vid);
                    }
                    if ((pCon->CcpConfig.UseConvFlag) && (Conversion != NULL)) {
                        if (strlen (Conversion)) {
                            set_bbvari_conversion (pCon->CcpCalParamList[x].vid, 1, Conversion);
                        } else {
                            set_bbvari_conversion (pCon->CcpCalParamList[x].vid, 0, Conversion);
                        }
                    }
                    if ((pCon->CcpConfig.UseMinMaxFlag) && (MinString != NULL) && (MaxString != NULL)) {
                        double Min, Max;
                        Min = strtod (MinString, NULL);
                        Max = strtod (MaxString, NULL);
                        set_bbvari_min (pCon->CcpCalParamList[x].vid, Min);
                        set_bbvari_max (pCon->CcpCalParamList[x].vid, Max);
                    }
                    found = 1;
                } 
            } else {
                ThrowError (1, "missing parameter in string %s", txt);
                Ret = -1;
            }
        }
        if (!found) {
            ThrowError (1, "CCP parameter \"%s\" is not configured and cannot calibrate", par_Params[x]);
            pCon->CcpCalParamList[x].Address = 0;
            pCon->CcpCalParamList[x].vid = -1;
            Ret = -1;
        }
    }
    if (ParamIdxs != NULL) my_free(ParamIdxs);
    return Ret;
}

static int DeleteDTOsBBVariable (CCP_VARIABLES_CONFIG *dto_conf)
{
    CCP_DTO_PACKAGE *Dto;
    int v, DTOIdx;

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
    int used_bytes[CCP_MAX_DTO_PACKAGES];
    int run, v, x;
    char *Unit, *Conversion, *MinString, *MaxString;
    char *VariableFoundArray;
    int Fd = GetMainFileDescriptor();
    int *VarIdxs;

    DeleteDTOsBBVariable (dto_conf);
    PrintFormatToString (Section, sizeof(Section), "CCP Configuration for Target %i", Connection);
    MEMSET (used_bytes, 0, sizeof(used_bytes));
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
                                ThrowError (1, "CCP cannot add measurement \"%s\" to blackboard (%i)", name, vid);
                            }  else {
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
            ThrowError (1, "CCP unknown measurement \"%s\"", vars[x]);
        }
    }
    my_free (VariableFoundArray);
    my_free (VarIdxs);
    dto_conf->DTOPackagesCount = pid_max + 1;
    write_message (get_pid_by_name ("CANServer"), CPP_DTOS_DATA_STRUCTS, sizeof (CCP_VARIABLES_CONFIG), (char*)dto_conf);
    return 0;
}

/* Timeout observation */
static DWORD TimeoutCounter;
static int CycleCounterVid;
static double AbtastPeriode;

static void SetTimeOut (void)
{
    if (CycleCounterVid <= 0) {
        int Vid;
        CycleCounterVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CYCLE_COUNTER),
                                      BB_UDWORD, NULL);
        Vid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME), BB_DOUBLE, NULL);
        AbtastPeriode = read_bbvari_double (Vid);
        remove_bbvari (Vid);
    }
    TimeoutCounter = read_bbvari_udword (CycleCounterVid);
}


static int CheckTimeout (unsigned int ms, CCP_CONNECTTION *pCon)
{
    // Without timeout;
    if (pCon->CcpConfig.Timeout == 0) {
        return 0;
    }
    // With Timeout
    else {
        uint32_t t = read_bbvari_udword (CycleCounterVid);

        int ret= ((t - TimeoutCounter) > (uint32_t)((double)ms / 1000.0 / AbtastPeriode));
        if (ret) {
            ThrowError(ERROR_NO_STOP, "CCP: Timeout");
        }
        return ret;
    }
}


#define x__MIN(a,b)  (((a) < (b)) ? (a) : (b))

#define CCP_CHECK_TIMEOUT() if (CheckTimeout ((unsigned)pCon->CcpConfig.Timeout, pCon)) { \
    DeleteDTOsBBVariable (&pCon->dto_conf);  \
    DeleteCalibrationList (Connection); \
    { int CcpConnectionNo = Connection + 4; \
      write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo); \
      write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo); \
    } \
    pCon->CcpCommand = pCon->CcpCommandSequence = CCP_NO_COMMAND; \
    pCon->gCcpConnected = 0;  \
    pCon->CcpErrCode = -1; \
}

#define CCP_CHECK_TIMEOUT_MS(ms) if (CheckTimeout (ms, pCon)) { \
    DeleteDTOsBBVariable (&pCon->dto_conf);  \
    DeleteCalibrationList (Connection); \
    { int CcpConnectionNo = Connection + 4; \
    write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo); \
    write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo); \
    } \
    pCon->CcpCommand = pCon->CcpCommandSequence = CCP_NO_COMMAND; \
    pCon->gCcpConnected = 0; \
    pCon->CcpErrCode = -1; \
}

static int CcpCheckAckMessage (CCP_CRO_CAN_OBJECT *pCanObject, CCP_CONNECTTION *pCon)
{
    MESSAGE_HEAD mhead;

    if (test_message (&mhead) && (mhead.mid == CCP_CRM_ACK)) { 
        read_message (&mhead, (char*)pCanObject, sizeof (CCP_CRO_CAN_OBJECT)); \
        if (pCanObject->Data[0]==0xFF) { // Ack
            if (pCanObject->Data[2]!=CCP_CTR) { // ctr-error
                ThrowError (ERROR_NO_STOP, "CCP: Bad ctr received.(%02X %02X %02X %02X %02X %02X %02X %02X)",
                       (int)pCanObject->Data[0], (int)pCanObject->Data[1], (int)pCanObject->Data[2], (int)pCanObject->Data[3],
                       (int)pCanObject->Data[4], (int)pCanObject->Data[5], (int)pCanObject->Data[6], (int)pCanObject->Data[7] );
                return 0;
            }

            if (pCanObject->Data[1] // error code
                && (pCon->CcpCommand != CCP_GET_VERSION_WAIT_ACK)) { // Ignore protocol error at GetVersion
                ThrowError (ERROR_NO_STOP, "CCP: protocol error: %s! (%02XH)", 
                       CCPErrorNoToString (pCanObject->Data[1]),
                       (int)pCanObject->Data[1]);
                pCon->CcpCommandSequence = pCon->CcpCommand = 0;
                pCon->gCcpConnected = 0;
                pCon->CcpErrCode = (int)pCanObject->Data[1];
                return 0;
            }
            
            return 1;          // receive a ACK

        } else  if (pCanObject->Data[0]==0xFE) {   // Ignore event
            ThrowError (ERROR_NO_STOP, "CCP: received an event (igrnoring) (%02X %02X %02X %02X %02X %02X %02X %02X)",
                   (int)pCanObject->Data[0], (int)pCanObject->Data[1], (int)pCanObject->Data[2], (int)pCanObject->Data[3],
                   (int)pCanObject->Data[4], (int)pCanObject->Data[5], (int)pCanObject->Data[6], (int)pCanObject->Data[7] );
        } // BYTE0 = 0xFE
    }
    return 0;
}



static void ReadECUInfosCommandScheduer (CCP_CONNECTTION *pCon, int Connection)
{
    CCP_CRO_CAN_OBJECT CanObject;

    switch (pCon->CcpCommand) {
    case CCP_COMMAND_STARTPOINT:
        pCon->CcpErrCode = 0;
        pCon->CcpCommand = CCP_CONNECT;
        break;
    case CCP_CONNECT:
        CcpReadConfigFromIni (Connection);
        {
            int CcpConnectionNo = Connection + 4;
            write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
        }
        if ((pCon->gCcpConnected & CONNECTED_MASK) != CONNECTED_MASK) {   // is alread connected?
            pCon->gCcpVersion = pCon->CcpConfig.CcpVersion & 0xFFFF;  // desired CCP version
            ccpConnect (pCon->CcpConfig.StationAddress, pCon);
            SetTimeOut ();
            pCon->CcpCommand = CCP_CONNECT_WAIT_ACK;
        } else pCon->CcpCommand = CCP_GET_VERSION;
        break;
    case CCP_CONNECT_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "CONNECT", &CanObject);
            pCon->gCcpConnected |= CONNECTED_MASK;
            pCon->CcpCommand = CCP_GET_VERSION;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_GET_VERSION:
        ccpGetVersion (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_VERSION_WAIT_ACK;
        break;
    case CCP_GET_VERSION_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "GET_CCP_VERSION", &CanObject);
            pCon->ccp_infos.major_version = CanObject.Data[3];
            pCon->ccp_infos.minor_version = CanObject.Data[4];
            pCon->CcpCommand = CCP_EXCHANGE_ID;
        }
        // If an error happen this is version 2.0, the command doesn't exist in 2.0
        else {
            pCon->ccp_infos.major_version = 0x02;
            pCon->ccp_infos.minor_version = 0x00;
        }
        pCon->gCcpVersion = pCon->ccp_infos.major_version * 256 + pCon->ccp_infos.minor_version;
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_EXCHANGE_ID:
        ccpExchangeId (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_EXCHANGE_ID_WAIT_ACK;
        break;
    case CCP_EXCHANGE_ID_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "EXCHANGE_ID", &CanObject);
            pCon->ccp_infos.ResourceAvailabilityMask = CanObject.Data[5];
            pCon->ccp_infos.ResourceProtectionMask = CanObject.Data[6];
            pCon->SizeOfTCUString = CanObject.Data[3];
            pCon->TCUStringPointer = 0;
            pCon->CcpCommand = CCP_DATA_UPLOAD;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_DATA_UPLOAD:
        ccpDataUpload (x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5), pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_DATA_UPLOAD_WAIT_ACK;
        break;
    case CCP_DATA_UPLOAD_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "UPLOAD", &CanObject);
            MEMCPY (&(pCon->ccp_infos.TargetIdentifier[pCon->TCUStringPointer]), 
                    &(CanObject.Data[3]), 
                    (size_t)x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5));
            pCon->TCUStringPointer += x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5);
            if (pCon->SizeOfTCUString == pCon->TCUStringPointer) {
                pCon->CcpCommand = CCP_GET_DAQ_LIST_SIZE;
            } else {
                pCon->CcpCommand = CCP_DATA_UPLOAD;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_GET_DAQ_LIST_SIZE:
        ccpGetDaqListSize (0, pCon->CcpConfig.DTO_id, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_DAQ_LIST_SIZE_WAIT_ACK;
        break;
    case CCP_GET_DAQ_LIST_SIZE_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "GET_DAQ_SIZE", &CanObject);
            pCon->ccp_infos.DAQ_Size = CanObject.Data[3];
            pCon->ccp_infos.DAQ_Pid = CanObject.Data[4];
            pCon->CcpCommand = CCP_READ_TCU_INFO_FINISHED;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_READ_TCU_INFO_FINISHED:
        pCon->CcpCommand = 0;
        pCon->CcpCommandSequence = 0;
        break;
    }
}


static void StartMeasurement_CommandScheduer (CCP_CONNECTTION *pCon, int Connection)
{
    CCP_CRO_CAN_OBJECT CanObject;

    switch (pCon->CcpCommand) {
    case CCP_COMMAND_STARTPOINT:
        pCon->CcpErrCode = 0;
        CcpReadConfigFromIni (Connection);
        if ((pCon->gCcpConnected & MEASUREMENT_RUNNING_MASK) == MEASUREMENT_RUNNING_MASK) {  // Messurement is running
            pCon->CcpCommand = CCP_STOP_ALL_DAQ_LIST;
        } else {
            pCon->CcpCommand = CCP_CONNECT;
        }
        break;

    case CCP_STOP_ALL_DAQ_LIST:
        if ((pCon->CcpConfig.CcpVersion & 0x10000000) == 0x10000000) {
            ccpStopDaqList (0, pCon);
        } else {
            ccpStopAllDaqLists (pCon);
        }
        SetTimeOut ();
        pCon->CcpCommand = CCP_STOP_ALL_DAQ_LIST_WAIT_ACK;
        break;
    case CCP_STOP_ALL_DAQ_LIST_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject,pCon)) {
            if ((pCon->CcpConfig.CcpVersion & 0x10000000) == 0x10000000) {
                PrintACKMessage (pCon, "START_STOP", &CanObject);
            } else {
                PrintACKMessage (pCon, "START_STOP_ALL", &CanObject);
            }
            pCon->SessionStatusCounter = 0;
            //Set S_STATUS
            if ((pCon->CcpConfig.CcpVersion & 0x20000000) == 0x20000000) {  
                pCon->CcpCommand = CCP_GET_STATUS_FLAGS;
            } else {
                pCon->CcpCommand = CCP_MEASUREMENT_STOPPED;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;

    case CCP_GET_STATUS_FLAGS:
        ccpGetSStatus (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_STATUS_FLAGS_ACK;
        break;
    case CCP_GET_STATUS_FLAGS_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "GET_S_STATUS", &CanObject);
            pCon->SessionStatus = CanObject.Data[3];
            if ((pCon->SessionStatus & 0x02) != 0x02) {
                pCon->CcpCommand = CCP_MEASUREMENT_STOPPED;
            } else {
                if (pCon->SessionStatusCounter++ >= 5) {
                    // After 5 not succesful attempts reset it hard
                    pCon->CcpCommand = CCP_SET_STATUS_FLAGS;
                } else {
                    pCon->CcpCommand = CCP_GET_STATUS_FLAGS;
                }
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_SET_STATUS_FLAGS:
        ccpSetSStatus (pCon->SessionStatus &= ~0x02, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_STATUS_FLAGS_ACK;
        break;
    case CCP_SET_STATUS_FLAGS_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_S_STATUS", &CanObject);
            pCon->CcpCommand = CCP_MEASUREMENT_STOPPED;
        }
        CCP_CHECK_TIMEOUT();
        break;

    case CCP_MEASUREMENT_STOPPED:
        pCon->gCcpConnected &= ~MEASUREMENT_RUNNING_MASK;
        // remove blackboard variables
        DeleteDTOsBBVariable (&pCon->dto_conf);
        {
            int CcpConnectionNo = Connection + 4;
            write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
            //Deactivate timout observation which was activated with CCP_DTO_ACTIVATE_TIMEOUT-Message
            write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
            write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
        }
        // Than jump over disconnect
        pCon->CcpCommand = CCP_CONNECT;
        break;

    case CCP_CONNECT:
        {
            int CcpConnectionNo = Connection + 4;
            write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
        }
        if ((pCon->gCcpConnected & CONNECTED_MASK) != CONNECTED_MASK) {   // is already connected?
            pCon->gCcpVersion = pCon->CcpConfig.CcpVersion & 0xFFFF;  // desired CCP version
            ccpConnect (pCon->CcpConfig.StationAddress, pCon);
            SetTimeOut ();
            pCon->CcpCommand = CCP_CONNECT_WAIT_ACK;
        } else {
            if (pCon->CcpConfig.SeedKeyFlag) {
                pCon->CcpCommand = CCP_GET_SEED;
            } else {
                pCon->CcpCommand = CCP_GET_DAQ_LIST_SIZE;
            }
        }
        break;
    case CCP_CONNECT_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "CONNECT", &CanObject);
            pCon->gCcpConnected |= CONNECTED_MASK;
            pCon->CcpCommand = CCP_GET_VERSION; 
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_GET_VERSION:
        ccpGetVersion (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_VERSION_WAIT_ACK;
        break;
    case CCP_GET_VERSION_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "GET_CCP_VERSION", &CanObject);
            if (CanObject.Data[1] == 0) {
                pCon->ccp_infos.major_version = CanObject.Data[3];
                pCon->ccp_infos.minor_version = CanObject.Data[4];
            }
            // If an error happen this is version 2.0, the command doesn't exist in 2.0
            else {
                pCon->ccp_infos.major_version = 0x02;
                pCon->ccp_infos.minor_version = 0x00;
            }
            pCon->gCcpVersion = pCon->ccp_infos.major_version * 256 + pCon->ccp_infos.minor_version;
            if (pCon->CcpConfig.SeedKeyFlag) {
                pCon->CcpCommand = CCP_GET_SEED;
            } else {
                pCon->CcpCommand = CCP_EXCHANGE_ID;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_GET_SEED:
        ccpGetSeed (PL_DAQ, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_SEED_WAIT_ACK;
        break;
    case CCP_GET_SEED_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            int x;
            PrintACKMessage (pCon, "GET_SEED", &CanObject);
            pCon->gCcpProtectionStatus = CanObject.Data[3];
            for (x = 0; x < 4; x++) pCon->gCcpSeed[x] = CanObject.Data[4 + x];
            if (CanObject.Data[3] == 0) {
                // No Seed & Key neccessary
                pCon->CcpCommand = CCP_EXCHANGE_ID;
            } else {
                pCon->CcpCommand = CCP_UNLOCK;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_UNLOCK:
        if (ccpUnlockProtection (PL_DAQ, pCon)) {
            SetTimeOut ();
            pCon->CcpCommand = CCP_UNLOCK_WAIT_ACK;
        } else {
            pCon->CcpCommand = CCP_EXCHANGE_ID;
        }
        break;
    case CCP_UNLOCK_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "UNLOCK", &CanObject);
            pCon->CcpCommand = CCP_EXCHANGE_ID;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_EXCHANGE_ID:
        ccpExchangeId (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_EXCHANGE_ID_WAIT_ACK;
        break;
    case CCP_EXCHANGE_ID_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "EXCHANGE_ID", &CanObject);
            pCon->ccp_infos.ResourceAvailabilityMask = CanObject.Data[5];
            pCon->ccp_infos.ResourceProtectionMask = CanObject.Data[6];
            pCon->SizeOfTCUString = CanObject.Data[3];
            pCon->TCUStringPointer = 0;
            pCon->CcpCommand = CCP_DATA_UPLOAD;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_DATA_UPLOAD:
        ccpDataUpload (x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5), pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_DATA_UPLOAD_WAIT_ACK;
        break;
    case CCP_DATA_UPLOAD_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "UPLOAD", &CanObject);
            MEMCPY (&(pCon->ccp_infos.TargetIdentifier[pCon->TCUStringPointer]), 
                    &(CanObject.Data[3]), 
                    (size_t)x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5));
            pCon->TCUStringPointer += x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5);
            if (pCon->SizeOfTCUString == pCon->TCUStringPointer) {
                pCon->CcpCommand = CCP_GET_DAQ_LIST_SIZE;
            } else {
                pCon->CcpCommand = CCP_DATA_UPLOAD;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_GET_DAQ_LIST_SIZE:
        ccpGetDaqListSize (0, pCon->CcpConfig.DTO_id, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_DAQ_LIST_SIZE_WAIT_ACK;
        break;
    case CCP_GET_DAQ_LIST_SIZE_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "GET_DAQ_SIZE", &CanObject);
            pCon->DAQSize = CanObject.Data[3];
            pCon->DAQPid = CanObject.Data[4];
            pCon->CcpCommand = CCP_BUILD_DTOS;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_BUILD_DTOS:
        if (BuildDTOs (&pCon->dto_conf, pCon->CcpMeasurementLabelListSize, pCon->CcpMeasurementLabelList, pCon->DAQSize, Connection,
                       pCon->CcpConfig.UseUnitFlag, pCon->CcpConfig.UseConvFlag, pCon->CcpConfig.UseMinMaxFlag)) {
            pCon->CcpCommand = 0;
            pCon->CcpCommandSequence = 0;
        } else {
            pCon->CcpCommand = CCP_SET_DAQ_LIST_PTR;
            pCon->OdtPtr = 0;
            pCon->DaqPtr = 0;
        }
        break;
    case CCP_SET_DAQ_LIST_PTR:
        ccpSetDaqListPtr (0, pCon->OdtPtr, pCon->DaqPtr, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_DAQ_LIST_PTR_WAIT_ACK;
        break;
    case CCP_SET_DAQ_LIST_PTR_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_DAQ_PTR", &CanObject);
            pCon->CcpCommand = CCP_WRITE_DAQ_LIST_ENTRY;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_WRITE_DAQ_LIST_ENTRY:
        ccpWriteDaqListEntry (pCon->dto_conf.DTO_Packages[pCon->OdtPtr].Entrys[pCon->DaqPtr].Size, 0,
                              pCon->dto_conf.DTO_Packages[pCon->OdtPtr].Entrys[pCon->DaqPtr].Address, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_WRITE_DAQ_LIST_ENTRY_WAIT_ACK;
        break;
    case CCP_WRITE_DAQ_LIST_ENTRY_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "WRITE_DAQ", &CanObject);
            pCon->DaqPtr++;
            if (pCon->DaqPtr >= pCon->dto_conf.DTO_Packages[pCon->OdtPtr].VarCount) {
                pCon->DaqPtr = 0;
                pCon->OdtPtr++;
                if (pCon->OdtPtr >= pCon->dto_conf.DTOPackagesCount) {
                    // Set S_STATUS
                    if ((pCon->CcpConfig.CcpVersion & 0x20000000) == 0x20000000) {  
                        pCon->CcpCommand = CCP_GET_STATUS_FLAGS_2;
                    } else {
                        pCon->CcpCommand = CCP_PREPARE_DAQ_LIST;
                    }
                    break;
                }
            }
            pCon->CcpCommand = CCP_SET_DAQ_LIST_PTR;   // weiteres DAQ-Element
        }
        CCP_CHECK_TIMEOUT();
        break;

    case CCP_GET_STATUS_FLAGS_2:
        ccpGetSStatus (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_STATUS_FLAGS_ACK_2;
        break;
    case CCP_GET_STATUS_FLAGS_ACK_2:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "GET_S_STATUS", &CanObject);
            pCon->SessionStatus = CanObject.Data[3];
            if ((pCon->SessionStatus & 0x02) == 0x02) {   // DAQ-Flag is already set
                pCon->CcpCommand = CCP_PREPARE_DAQ_LIST;
            } else {
                pCon->CcpCommand = CCP_SET_STATUS_FLAGS_2;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_SET_STATUS_FLAGS_2:
        ccpSetSStatus (pCon->SessionStatus |= 0x02, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_STATUS_FLAGS_ACK_2;
        break;
    case CCP_SET_STATUS_FLAGS_ACK_2:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_S_STATUS", &CanObject);
            pCon->CcpCommand = CCP_PREPARE_DAQ_LIST;
        }
        CCP_CHECK_TIMEOUT();
        break;

    case CCP_PREPARE_DAQ_LIST:
        // CCP V2.1
        if (pCon->gCcpVersion >= 0x0201) {
          ccpPrepareDaqList (0, pCon->dto_conf.DTOPackagesCount-1, pCon->CcpConfig.EventChannel, pCon->CcpConfig.Prescaler, pCon);
          SetTimeOut ();
          pCon->CcpCommand = CCP_PREPARE_DAQ_LIST_WAIT_ACK;
        }
        // CCP V2.0
        else {
          pCon->CcpCommand = CCP_START_ALL_DAQ_LISTS;
        }
        break;
    case CCP_PREPARE_DAQ_LIST_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "START_STOP", &CanObject);
            pCon->CcpCommand = CCP_START_ALL_DAQ_LISTS;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_START_ALL_DAQ_LISTS:
        ccpStartAllDaqLists (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_START_ALL_DAQ_LISTS_WAIT_ACK;
        break;
    case CCP_START_ALL_DAQ_LISTS_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            int CcpConnectionNo = Connection + 4;
            write_message (get_pid_by_name ("CANServer"), CCP_DTO_ACTIVATE_TIMEOUT, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
            PrintACKMessage (pCon, "START_STOP_ALL", &CanObject);
            pCon->CcpCommand = CCP_WAIT_FIRST_DATA;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_WAIT_FIRST_DATA:
        {
            MESSAGE_HEAD mhead;
            if (test_message (&mhead)) {
                if (mhead.mid == CCP_RECEIVE_ALL_VARIABLE_FIRST_TIME) {
                    remove_message ();
                    // First complete messrement with all  gueltige komplette Messung aller Variables
                    pCon->CcpCommand = CCP_START_MEASUREMENT_FINISHED;
                } 
            }
        }
        break;
    case CCP_START_MEASUREMENT_FINISHED:
        pCon->CcpCommand = 0;
        pCon->CcpCommandSequence = 0;
        pCon->gCcpConnected |= MEASUREMENT_RUNNING_MASK;   // Messurement is running
        break;
    }
}


static void StopMeasurement_CommandScheduer (CCP_CONNECTTION *pCon, int Connection)
{
    CCP_CRO_CAN_OBJECT CanObject;

    switch (pCon->CcpCommand) {
    case CCP_COMMAND_STARTPOINT:
        pCon->CcpErrCode = 0;
        pCon->CcpCommand = CCP_STOP_ALL_DAQ_LIST;
        break;
    case CCP_STOP_ALL_DAQ_LIST:
        if ((pCon->CcpConfig.CcpVersion & 0x10000000) == 0x10000000) {
            ccpStopDaqList (0, pCon);
        } else {
            ccpStopAllDaqLists (pCon);
        }
        SetTimeOut ();
        pCon->CcpCommand = CCP_STOP_ALL_DAQ_LIST_WAIT_ACK;
        break;
    case CCP_STOP_ALL_DAQ_LIST_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            if ((pCon->CcpConfig.CcpVersion & 0x10000000) == 0x10000000) {
                PrintACKMessage (pCon, "START_STOP", &CanObject);
            } else {
                PrintACKMessage (pCon, "START_STOP_ALL", &CanObject);
            }
            pCon->SessionStatusCounter = 0;
            // Set S_STATUS
            if ((pCon->CcpConfig.CcpVersion & 0x20000000) == 0x20000000) {
                pCon->CcpCommand = CCP_GET_STATUS_FLAGS;
            } else {
                pCon->CcpCommand = CCP_MEASUREMENT_STOPPED;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;

    case CCP_GET_STATUS_FLAGS:
        ccpGetSStatus (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_STATUS_FLAGS_ACK;
        break;
    case CCP_GET_STATUS_FLAGS_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "GET_S_STATUS", &CanObject);
            pCon->SessionStatus = CanObject.Data[3];
            if ((pCon->SessionStatus & 0x02) != 0x02) {
                pCon->CcpCommand = CCP_MEASUREMENT_STOPPED;
            } else {
                if (pCon->SessionStatusCounter++ >= 5) {
                    // After 5 not succesful attempts reset it hard
                    pCon->CcpCommand = CCP_SET_STATUS_FLAGS;
                } else {
                    pCon->CcpCommand = CCP_GET_STATUS_FLAGS;
                }
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_SET_STATUS_FLAGS:
        ccpSetSStatus (pCon->SessionStatus &= ~0x02, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_STATUS_FLAGS_ACK;
        break;
    case CCP_SET_STATUS_FLAGS_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_S_STATUS", &CanObject);
            pCon->CcpCommand = CCP_MEASUREMENT_STOPPED;
        }
        CCP_CHECK_TIMEOUT();
        break;

    case CCP_MEASUREMENT_STOPPED:
        pCon->gCcpConnected &= ~MEASUREMENT_RUNNING_MASK;
        // Remove blackboard variables
        DeleteDTOsBBVariable (&pCon->dto_conf);
        {
            int CcpConnectionNo = Connection + 4;
            write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
            // is calibration active
            if (pCon->gCcpConnected != CONNECTED_MASK) {
                //Deactivate timout observation which was activated with CCP_DTO_ACTIVATE_TIMEOUT-Message
                write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
                write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
                // Than jump over disconnect
                pCon->CcpCommand = 0;
                pCon->CcpCommandSequence = 0;
            } else {
                pCon->CcpCommand = CCP_DISCONNECT;
            }
        }
        break;

    case CCP_DISCONNECT:
        ccpDisconnect (0, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_DISCONNECT_WAIT_ACK;
        break;
    case CCP_DISCONNECT_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            int CcpConnectionNo = Connection + 4;
            PrintACKMessage (pCon, "DISCONNECT", &CanObject);
            pCon->gCcpConnected = 0;
            pCon->CcpCommand = CCP_STOP_MEASUREMENT_FINISHED;
            write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_STOP_MEASUREMENT_FINISHED:
        pCon->CcpCommand = 0;
        pCon->CcpCommandSequence = 0;
        break;
    }
}

static void StartCalibration_CommandScheduer (CCP_CONNECTTION *pCon, int Connection)
{
    CCP_CRO_CAN_OBJECT CanObject;

    switch (pCon->CcpCommand) {
    case CCP_COMMAND_STARTPOINT:
        DeleteCalibrationList (Connection);
        pCon->CcpErrCode = 0;
        pCon->CcpCommand = CCP_CONNECT;
        break;
    case CCP_CONNECT:
        {
            int CcpConnectionNo = Connection + 4;
            write_message (get_pid_by_name ("CANServer"), CCP_AKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
        }
        CcpReadConfigFromIni (Connection);
        if ((pCon->gCcpConnected & CONNECTED_MASK) != CONNECTED_MASK) {   // is already connected?
            pCon->gCcpVersion = pCon->CcpConfig.CcpVersion & 0xFFFF;  // desired CCP version
            ccpConnect (pCon->CcpConfig.StationAddress, pCon);
            SetTimeOut ();
            pCon->CcpCommand = CCP_CONNECT_WAIT_ACK;
        } else {
            if (pCon->CcpConfig.SeedKeyFlag) {
                pCon->CcpCommand = CCP_GET_SEED;
            } else {
                if (pCon->CcpConfig.MoveROM2RAM) {  
                    pCon->CcpCommand = CCP_SET_MTA0;
                } else { // Calibration daten are already inside RAM
                    if (pCon->CcpConfig.SelCalPage) {
                        pCon->CcpCommand = CCP_SET_MTA2;
                    } else {
                        pCon->CcpCommand = CCP_START_CALIBRATION_FINISHED;
                    }
                }
            }
        }
        break;
    case CCP_CONNECT_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "CONNECT", &CanObject);
            pCon->gCcpConnected |= CONNECTED_MASK;
            pCon->CcpCommand = CCP_GET_VERSION; 
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_GET_VERSION:
        ccpGetVersion (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_VERSION_WAIT_ACK;
        break;
    case CCP_GET_VERSION_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "GET_CCP_VERSION", &CanObject);
            if (CanObject.Data[1] == 0) {
                pCon->ccp_infos.major_version = CanObject.Data[3];
                pCon->ccp_infos.minor_version = CanObject.Data[4];
            }
            // If an error happen this is version 2.0, the command doesn't exist in 2.0
            else {
                pCon->ccp_infos.major_version = 0x02;
                pCon->ccp_infos.minor_version = 0x00;
            }
            pCon->gCcpVersion = pCon->ccp_infos.major_version * 256 + pCon->ccp_infos.minor_version;
            if (pCon->CcpConfig.SeedKeyFlag) {
                pCon->CcpCommand = CCP_GET_SEED;
            } else {
                pCon->CcpCommand = CCP_EXCHANGE_ID;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_GET_SEED:
        ccpGetSeed (PL_CAL, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_SEED_WAIT_ACK;
        break;
    case CCP_GET_SEED_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            int x;
            PrintACKMessage (pCon, "GET_SEED", &CanObject);
            pCon->gCcpProtectionStatus = CanObject.Data[3];
            for (x = 0; x < 4; x++) pCon->gCcpSeed[x] = CanObject.Data[4 + x];
            if (CanObject.Data[3] == 0) {
                // no Seed&Key neccessary
                pCon->CcpCommand = CCP_GET_SEED_2;
            } else {
                pCon->CcpCommand = CCP_UNLOCK;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_UNLOCK:
        if (ccpUnlockProtection (PL_CAL, pCon)) {
            SetTimeOut ();
            pCon->CcpCommand = CCP_UNLOCK_WAIT_ACK;
        } else {
            pCon->CcpCommand = CCP_GET_SEED_WAIT_ACK_2;
        }
        break;
    case CCP_UNLOCK_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "UNLOCK", &CanObject);
            pCon->CcpCommand = CCP_GET_SEED_2;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_GET_SEED_2:
        ccpGetSeed (PL_DAQ, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_GET_SEED_WAIT_ACK_2;
        break;
    case CCP_GET_SEED_WAIT_ACK_2:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            int x;
            PrintACKMessage (pCon, "GET_SEED", &CanObject);
            pCon->gCcpProtectionStatus = CanObject.Data[3];
            for (x = 0; x < 4; x++) pCon->gCcpSeed[x] = CanObject.Data[4 + x];
            if (CanObject.Data[3] == 0) {
                // no Seed&Key neccessary
                pCon->CcpCommand = CCP_EXCHANGE_ID;
            } else {
                pCon->CcpCommand = CCP_UNLOCK_2;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_UNLOCK_2:
        if (ccpUnlockProtection (PL_DAQ, pCon)) {
            SetTimeOut ();
            pCon->CcpCommand = CCP_UNLOCK_WAIT_ACK_2;
        } else {
            pCon->CcpCommand = CCP_EXCHANGE_ID;
        }
        break;
    case CCP_UNLOCK_WAIT_ACK_2:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "UNLOCK", &CanObject);
            pCon->CcpCommand = CCP_EXCHANGE_ID;
        }
        CCP_CHECK_TIMEOUT();
        break;

    case CCP_EXCHANGE_ID:
        ccpExchangeId (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_EXCHANGE_ID_WAIT_ACK;
        break;
    case CCP_EXCHANGE_ID_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "EXCHANGE_ID", &CanObject);
            pCon->ccp_infos.ResourceAvailabilityMask = CanObject.Data[5];
            pCon->ccp_infos.ResourceProtectionMask = CanObject.Data[6];
            pCon->SizeOfTCUString = CanObject.Data[3];
            pCon->TCUStringPointer = 0;
            pCon->CcpCommand = CCP_DATA_UPLOAD;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_DATA_UPLOAD:
        ccpDataUpload (x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5), pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_DATA_UPLOAD_WAIT_ACK;
        break;
    case CCP_DATA_UPLOAD_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "UPLOAD", &CanObject);
            MEMCPY (&(pCon->ccp_infos.TargetIdentifier[pCon->TCUStringPointer]), 
                    &(CanObject.Data[3]), 
                    (size_t)x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5));
            pCon->TCUStringPointer += x__MIN ((pCon->SizeOfTCUString - pCon->TCUStringPointer), 5);
            if (pCon->SizeOfTCUString == pCon->TCUStringPointer) {
                if (pCon->CcpConfig.MoveROM2RAM) {  
                    pCon->CcpCommand = CCP_SET_MTA0;
                } else { // Calibration data are already inside RAM
                    if (pCon->CcpConfig.SelCalPage) {
                        pCon->CcpCommand = CCP_SET_MTA2;
                    } else {
                        pCon->CcpCommand = CCP_START_CALIBRATION_FINISHED;
                    }
                }
            } else {
                pCon->CcpCommand = CCP_DATA_UPLOAD;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_SET_MTA0:
        ccpSetMta (0, pCon->CcpConfig.CalibROMStartAddr, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_MTA0_WAIT_ACK;
        break;
    case CCP_SET_MTA0_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_MTA", &CanObject);
            pCon->CcpCommand = CCP_SET_MTA1;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_SET_MTA1:
        ccpSetMta (1, pCon->CcpConfig.CalibRAMStartAddr, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_MTA1_WAIT_ACK;
        break;
    case CCP_SET_MTA1_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_MTA", &CanObject);
            pCon->CcpCommand = CCP_MOVE_MEMORY;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_MOVE_MEMORY:
        ccpMoveMemory (pCon->CcpConfig.CalibROMRAMSize, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_MOVE_MEMORY_WAIT_ACK;
        break;
    case CCP_MOVE_MEMORY_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "MOVE_MEMORY", &CanObject);
            pCon->CcpCommand = CCP_SET_MTA2;
        }
        CCP_CHECK_TIMEOUT_MS (30000);    // 30s
        break;
    case CCP_SET_MTA2:
        ccpSetMta (0, pCon->CcpConfig.CalibRAMStartAddr, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_MTA2_WAIT_ACK;
        break;
    case CCP_SET_MTA2_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_MTA", &CanObject);
            pCon->CcpCommand = CCP_SELECT_CAL_PAGE;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_SELECT_CAL_PAGE:
        ccpSelectCalPage (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SELECT_CAL_PAGE_WAIT_ACK;
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_SELECT_CAL_PAGE_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SELECT_CAL_PAGE", &CanObject);
            pCon->CcpCommand = CCP_START_CALIBRATION_FINISHED;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_START_CALIBRATION_FINISHED:
        BuildCalibrationList (pCon->CcpMeasurementLabelListSize, pCon->CcpMeasurementLabelList, Connection);
        // After connection is established read all parameter from target
        pCon->CppUploadInProgress = TRUE;
        pCon->gCcpConnected |= CALIBRATION_ACTIVE_MASK;
        pCon->CcpCommand = 0;
        pCon->CcpCommandSequence = 0;
        break;
    }
}

static void StopCalibration_CommandScheduer (CCP_CONNECTTION *pCon, int Connection)
{
    CCP_CRO_CAN_OBJECT CanObject;

    switch (pCon->CcpCommand) {
    case CCP_COMMAND_STARTPOINT:
        pCon->CcpErrCode = 0;
        pCon->gCcpConnected &= ~CALIBRATION_ACTIVE_MASK;
        DeleteCalibrationList (Connection);
        // is messsurement active?
        if ((pCon->gCcpConnected) != CONNECTED_MASK) {
            // Than jump over disconnect
            pCon->CcpCommand = CCP_STOP_CALIBRATION_FINISHED;
        } else {
            pCon->CcpCommand = CCP_DISCONNECT;
        }
        break;
    case CCP_DISCONNECT:
        ccpDisconnect (0, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_DISCONNECT_WAIT_ACK;
        break;
    case CCP_DISCONNECT_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            int CcpConnectionNo = Connection + 4;
            PrintACKMessage (pCon, "DISCONNECT", &CanObject);
            pCon->gCcpConnected = 0;
            pCon->CcpCommand = CCP_STOP_CALIBRATION_FINISHED;
            write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(CcpConnectionNo), (char*)&CcpConnectionNo);
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_STOP_CALIBRATION_FINISHED:
        pCon->CcpCommand = 0;
        pCon->CcpCommandSequence = 0;
        break;
    }
}

static void AlivePing_CommandScheduer (CCP_CONNECTTION *pCon, int Connection)
{
    CCP_CRO_CAN_OBJECT CanObject;

    switch (pCon->CcpCommand) {
    case CCP_EXCHANGE_ID:
        ccpExchangeId (pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_EXCHANGE_ID_WAIT_ACK;
        break;
    case CCP_EXCHANGE_ID_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "EXCHANGE_ID", &CanObject);
            pCon->CcpCommand = 0;
            pCon->CcpCommandSequence = 0;
            break;
        }
        CCP_CHECK_TIMEOUT();
        break;
    }
}


static void SetStatusVariableTextReplaces (CCP_CONNECTTION *pCon)
{
    int Len;

    static char StatusEnumStr[] = "0 0 \"IDLE\";"
                                  "1 1 \"CONNECTED\";"
                                  "3 3 \"MEASUREMENT\";"
                                  "5 5 \"CALIBRATION\";"
                                  "7 7 \"CALIBRATION_AND_MEASUREMENT\";";


    static char CmdEnumStr[] = "0 0 \"NO_COMMAND\";"
                     "1 1\"GET_VERSION\";"
                     "2 2\"GET_VERSION_WAIT_ACK\";"
                     "3 3\"EXCHANGE_ID\";"         
                     "4 4\"EXCHANGE_ID_WAIT_ACK\";"
                     "5 5\"DATA_UPLOAD\";"
                     "6 6\"DATA_UPLOAD_WAIT_ACK\";"
                     "7 7\"READ_TCU_INFO_FINISHED\";"          
                     "8 8\"CONNECT\";"                          
                     "9 9\"CONNECT_WAIT_ACK\";"                  
                     "10 10\"GET_DAQ_LIST_SIZE\";"              
                     "11 11\"GET_DAQ_LIST_SIZE_WAIT_ACK\";"      
                     "12 12\"BUILD_DTOS\";"                     
                     "13 13\"SET_DAQ_LIST_PTR\";"               
                     "14 14\"SET_DAQ_LIST_PTR_WAIT_ACK\";"       
                     "15 15\"WRITE_DAQ_LIST_ENTRY\";"           
                     "16 16\"WRITE_DAQ_LIST_ENTRY_WAIT_ACK\";"  
                     "17 17\"PREPARE_DAQ_LIST\";"               
                     "18 18\"PREPARE_DAQ_LIST_WAIT_ACK\";"       
                     "19 19\"START_ALL_DAQ_LISTS\";"            
                     "20 20\"START_ALL_DAQ_LISTS_WAIT_ACK\";"   
                     "21 21\"START_MEASUREMENT_FINISHED\";"     
                     "22 22\"STOP_ALL_DAQ_LIST\";"              
                     "23 23\"STOP_ALL_DAQ_LIST_WAIT_ACK\";"     
                     "24 24\"STOP_MEASUREMENT_FINISHED\";"      
                     "25 25\"DISCONNECT\";"
                     "26 26\"DISCONNECT_WAIT_ACK\";"            
                     "27 27\"SET_MTA\";"                        
                     "28 28\"SET_MTA_WAIT_ACK\";"               
                     "29 29\"SET_MTA0\";"                       
                     "30 30\"SET_MTA0_WAIT_ACK\";"              
                     "31 31\"SET_MTA1\";"                       
                     "32 32\"SET_MTA1_WAIT_ACK\";"              
                     "33 33\"SET_MTA2\";"                       
                     "34 34\"SET_MTA2_WAIT_ACK\";"              
                     "35 35\"DATA_DOWNLOAD\";"                  
                     "36 36\"DATA_DOWNLOAD_WAIT_ACK\";"         
                     "37 37\"MOVE_MEMORY\";"                    
                     "38 38\"MOVE_MEMORY_WAIT_ACK\";"
                     "39 39\"SELECT_CAL_PAGE\";"
                     "40 40\"SELECT_CAL_PAGE_WAIT_ACK\";"
                     "41 41\"START_CAL_FINISHED\";"
                     "42 42\"STOP_CAL_FINISHED\";"
                     "43 43\"GET_SEED\";"
                     "44 44\"GET_SEED_WAIT_ACK\";"
                     "45 45\"UNLOCK\";"
                     "46 46\"UNLOCK_WAIT_ACK\";"
                     "47 47\"GET_STATUS_FLAGS\";"
                     "48 48\"GET_STATUS_FLAGS_ACK\";"
                     "49 49\"SET_STATUS_FLAGS\";"
                     "50 50\"SET_STATUS_FLAGS_ACK\";"
                     "51 51\"MESSUREMENT_STOPPED\";" 
                     "52 52\"GET_SEED_2\";"
                     "53 53\"GET_SEED_WAIT_ACK_2\";"
                     "54 54\"UNLOCK_2\";"
                     "55 55\"UNLOCK_WAIT_ACK_2\";"
                     "56 56\"GET_STATUS_FLAGS_2\";"
                     "57 57\"GET_STATUS_FLAGS_ACK_2\";"
                     "58 58\"SET_STATUS_FLAGS_2\";"
                     "59 59\"SET_STATUS_FLAGS_ACK_2\";"
                     "60 60\"WAIT_FIRST_DATA\";"
                     "100 100\"COMMAND_STARTPOINT\";";


    static char CmdSeqEnumStr[] = "0 0 \"NO_COMMAND\";"
                     "1 1 \"READ_TCU_INFO\";"
                     "2 2 \"START_MEASUREMENT\";" 
                     "3 3 \"STOP_MEASUREMENT\";"  
                     "4 4 \"READ_MEMORY\";"
                     "5 5 \"DOWNLOAD_SEQUENCE\";"
                     "6 6 \"UPLOAD_SEQUENCE\";"
                     "7 7 \"START_CALIBRATION\";"
                     "8 8 \"STOP_CALIBRATION\";";

    set_bbvari_conversion (pCon->CCPStatusVid, BB_CONV_TEXTREP, StatusEnumStr);
    Len = (int)strlen (CmdEnumStr) + 1;
    if (Len >= (BBVARI_CONVERSION_SIZE * 2)) {
        ThrowError (1, "enum-string too long (%i >= %i) %s (%i)", Len, (int)(BBVARI_CONVERSION_SIZE * 2), __FILE__, __LINE__); 
    }
    set_bbvari_conversion (pCon->CCPCommandVid, BB_CONV_TEXTREP, CmdEnumStr);

    set_bbvari_conversion (pCon->CCPCommandSequenceVid, BB_CONV_TEXTREP, CmdSeqEnumStr);
}


static void cyclic_ccp_control (void)
{
    MESSAGE_HEAD mhead;
    int x, AckFromConnection;

    // some Messages can receive during command
    if (test_message (&mhead)) {
        switch (mhead.mid) {
        case CCP_CRM_ACK:  
            break;
        case CCP_DTO_TIMEOUT:
            read_message (&mhead, (char*)&AckFromConnection, sizeof (int));  
            if ((AckFromConnection >= 4) && (AckFromConnection < 8)) { // 0...3 are XCP
                write_message (get_pid_by_name ("CANServer"), CCP_DEAKTIVATE, sizeof(AckFromConnection), (char*)&AckFromConnection);
                write_message (get_pid_by_name ("CANServer"), CCP_REMOVE_BBVARIS, sizeof(AckFromConnection), (char*)&AckFromConnection);
                AckFromConnection -= 4;
                CcpConnections[AckFromConnection].CcpCommandSequence = 0;
                CcpConnections[AckFromConnection].CcpCommand = 0; 
                CcpConnections[AckFromConnection].gCcpConnected = 0;
                // remove all blackboard variable
                DeleteCalibrationList (AckFromConnection);
                DeleteDTOsBBVariable (&CcpConnections[AckFromConnection].dto_conf);
                ThrowError (ERROR_NO_STOP, "Timeout CCP measurement connection stopped (receive some time no DTO messages)");
            }
            break;
        }
    }
    for (x = 0; x < 4; x++) {
        CCP_CONNECTTION *pCon = &CcpConnections[x];
        switch (pCon->CcpCommandSequence) {
        case CCP_READ_TCU_INFO:
            ReadECUInfosCommandScheduer (pCon, x);
            break;
        case CCP_START_MEASUREMENT:
            StartMeasurement_CommandScheduer (pCon, x);
            break;
        case CCP_STOP_MEASUREMENT:
            StopMeasurement_CommandScheduer (pCon, x);
            break;
        case CCP_READ_MEMORY:
            // not implemented
            pCon->CcpCommandSequence = 0;
            break;
        case CCP_DOWNLOAD_SEQUENCE:
            Download_CmdScheduler (pCon, x);
            break;
        case CCP_UPLOAD_SEQUENCE:
            Upload_CmdScheduler (pCon, x);
            break;
        case CCP_START_CALIBRATION:
            StartCalibration_CommandScheduer (pCon, x);
            break;
        case CCP_STOP_CALIBRATION:
            StopCalibration_CommandScheduer (pCon, x);
            break;
        case CCP_ALIFE_PING:
            AlivePing_CommandScheduer (pCon, x);
            break;
        case CCP_NO_COMMAND_SEQUENCE:
            // Is there a new command sequence (Start/Stop-CCP(Cal))?
            if (pCon->CcpCommandSequenceRequest) {
                pCon->CcpCommandSequence = pCon->CcpCommandSequenceRequest;
                pCon->CcpCommandSequenceRequest = 0;
                pCon->CcpCommand = CCP_COMMAND_STARTPOINT;
                break;
            }
            if ((pCon->gCcpConnected & CALIBRATION_ACTIVE_MASK) == CALIBRATION_ACTIVE_MASK) { 
                if (pCon->CppUploadInProgress == TRUE) {
                    if (CCPReadCharacteristics (pCon) == ALL_PARAM_READ) {
                        pCon->CppUploadInProgress = FALSE;
                    }
                } else CCPWriteCharacteristics (pCon);
                if ((pCon->gCcpConnected & MEASUREMENT_RUNNING_MASK) != MEASUREMENT_RUNNING_MASK) {
                    if (pCon->CcpCommandSequence == CCP_NO_COMMAND_SEQUENCE) {
                        // Transmit an alive ping if there is no other communication this will check if
                        // the connection to the target is Ok.
                        if (pCon->AlivePingCounter >= pCon->AlivePingTimeout) {
                            pCon->AlivePingCounter = 0;
                            pCon->CcpCommandSequence = CCP_ALIFE_PING;
                            pCon->CcpCommand = CCP_EXCHANGE_ID;
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
            pCon->CcpCommandSequence = CCP_NO_COMMAND_SEQUENCE;
            ThrowError (1, "Internal error, that should never happen %s(%i)", __FILE__, __LINE__);
            break;
        }
        write_bbvari_udword (pCon->CCPVersionVid, (uint32_t)pCon->gCcpVersion);
        write_bbvari_udword (pCon->CCPStatusVid, (uint32_t)pCon->gCcpConnected);
        write_bbvari_udword (pCon->CCPCommandVid, (uint32_t)pCon->CcpCommand);
        write_bbvari_udword (pCon->CCPCommandSequenceVid, (uint32_t)pCon->CcpCommandSequence);
        if (x == 0) {
            write_bbvari_udword (pCon->CCPVersionVidOld, (uint32_t)pCon->gCcpVersion);
            write_bbvari_udword (pCon->CCPStatusVidOld, (uint32_t)pCon->gCcpConnected);
            write_bbvari_udword (pCon->CCPCommandVidOld, (uint32_t)pCon->CcpCommand);
            write_bbvari_udword (pCon->CCPCommandSequenceVidOld, (uint32_t)pCon->CcpCommandSequence);
        }
    }

}

static int init_ccp_control (void)
{
    int x;
    char txt[BBVARI_NAME_SIZE];
    TASK_CONTROL_BLOCK *pTcb;

    for (x = 0; x < 4; x++) {
        CCP_CONNECTTION *pCon = &CcpConnections[x];
        const char *Prefix = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES);
        pCon->CcpCommandSequence = CCP_NO_COMMAND_SEQUENCE;
        pCon->CcpCommand = CCP_NO_COMMAND;

        PrintFormatToString (txt, sizeof(txt), "%s.CCP[%i].Version", Prefix, x);
        pCon->CCPVersionVid = add_bbvari (txt, BB_UDWORD, "");
        PrintFormatToString (txt, sizeof(txt), "%s.CCP[%i].Status", Prefix, x);
        pCon->CCPStatusVid = add_bbvari (txt, BB_UDWORD, "");
        PrintFormatToString (txt, sizeof(txt), "%s.CCP[%i].Command", Prefix, x);
        pCon->CCPCommandVid = add_bbvari (txt, BB_UDWORD, "");
        PrintFormatToString (txt, sizeof(txt), "%s.CCP[%i].CommandSequence", Prefix, x);
        pCon->CCPCommandSequenceVid = add_bbvari (txt, BB_UDWORD, "");
        if (x == 0) { // downward compatible
            PrintFormatToString (txt, sizeof(txt), "%s.CCPVersion", Prefix);
            pCon->CCPVersionVidOld = add_bbvari (txt, BB_UDWORD, "");
            PrintFormatToString (txt, sizeof(txt), "%s.CCPStatus", Prefix);
            pCon->CCPStatusVidOld = add_bbvari (txt, BB_UDWORD, "");
            PrintFormatToString (txt, sizeof(txt), "%s.CCPCommand", Prefix);
            pCon->CCPCommandVidOld = add_bbvari (txt, BB_UDWORD, "");
            PrintFormatToString (txt, sizeof(txt), "%s.CCPCommandSequence", Prefix);
            pCon->CCPCommandSequenceVidOld = add_bbvari (txt, BB_UDWORD, "");
        }
        SetStatusVariableTextReplaces (pCon);
        // Send a ping every 200ms if only calibartion is active
        pTcb = GET_TCB ();
        pCon->AlivePingTimeout = 200000 / ((uint64_t)pTcb->time_steps * get_sched_periode_timer_clocks ());
        if (pCon->AlivePingTimeout < 5) pCon->AlivePingTimeout = 5;
        pCon->AlivePingCounter = 0;
    }
    return 0;
}

static void terminate_ccp_control (void)
{
    int x;

    for (x = 0; x < 4; x++) {
        CCP_CONNECTTION *pCon = &CcpConnections[x];
        if (pCon->CcpMeasurementLabelList != NULL) my_free (pCon->CcpMeasurementLabelList);
        pCon->CcpMeasurementLabelList = NULL;
        if (pCon->CcpMeasurementLabelListBuffer != NULL) my_free (pCon->CcpMeasurementLabelListBuffer);
        pCon->CcpMeasurementLabelListBuffer = NULL;
        pCon->CcpMeasurementLabelListSize = 0;
        DeleteCalibrationList (x);
        DeleteDTOsBBVariable (&pCon->dto_conf);

        remove_bbvari (pCon->CCPVersionVid);
        remove_bbvari (pCon->CCPStatusVid);
        remove_bbvari (pCon->CCPCommandVid);
        remove_bbvari (pCon->CCPCommandSequenceVid);
        if (x == 0) {
            remove_bbvari (pCon->CCPVersionVidOld);
            remove_bbvari (pCon->CCPStatusVidOld);
            remove_bbvari (pCon->CCPCommandVidOld);
            remove_bbvari (pCon->CCPCommandSequenceVidOld);
        }
    }
    for (x = 0; x < 4; x++) {
        CCP_CONNECTTION *pCon = &CcpConnections[x];
        MEMSET (pCon, 0, sizeof (CCP_CONNECTTION));
        pCon->CcpCommandSequence = pCon->CcpCommandSequenceRequest = CCP_NO_COMMAND_SEQUENCE;
        pCon->CcpCommand = CCP_NO_COMMAND;
	}
}

TASK_CONTROL_BLOCK ccp_control_tcb
    = INIT_TASK_COTROL_BLOCK("CCP_Control", INTERN_ASYNC, 1, cyclic_ccp_control, init_ccp_control, terminate_ccp_control, 32*1024);


// Interface
int Start_CCP (int ConnectionNo, int CalibOrMeasurment, int Count, char **Variables)
{
    int BufferSize = 10;
    int i;
    char *p;
    CCP_CONNECTTION *pCon;

    // Set text replace
    for (i = 0; i < 4; i++) SetStatusVariableTextReplaces (&CcpConnections[i]);

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) return -1;
    pCon = &CcpConnections[ConnectionNo];

    if (get_pid_by_name ("CANServer") < 0) {
        ThrowError (1, "process \"CANServer\" not running. Start it before using CCP");
        return -1;
    }
    if (pCon->CcpCommandSequenceRequest == 0) {   // No command active?
        if (get_pid_by_name ("CCP_Control") < 0) {
            start_process ("CCP_Control");
        }
        // Copy label list
        pCon->CcpMeasurementLabelListSize = Count;
        if (pCon->CcpMeasurementLabelListBuffer != NULL) my_free (pCon->CcpMeasurementLabelListBuffer);
        if (pCon->CcpMeasurementLabelList != NULL) my_free (pCon->CcpMeasurementLabelList);
        for (i = 0; i < Count; i++) BufferSize += (int)strlen (Variables[i]) + 1;
        if ((pCon->CcpMeasurementLabelListBuffer = my_malloc (BufferSize)) != NULL) {
            if ((pCon->CcpMeasurementLabelList = my_malloc ((size_t)Count * sizeof (char*))) != NULL) {
                p = pCon->CcpMeasurementLabelListBuffer;
                for (i = 0; i < Count; i++) {
                    StringCopyMaxCharTruncate (p, Variables[i], BufferSize - (p - pCon->CcpMeasurementLabelListBuffer));
                    pCon->CcpMeasurementLabelList[i] = p;
                    p += strlen (p) + 1;
                }
                // start command scheduler
                if (CalibOrMeasurment == START_CALIBRATION) {
                    pCon->CcpCommandSequenceRequest = CCP_START_CALIBRATION;
                } else {
                    pCon->CcpCommandSequenceRequest = CCP_START_MEASUREMENT;
                }
            }
        }
        return 0;
    } else {
        ThrowError (1, "cannot start CCP %s because an other command is runnung", 
            (CalibOrMeasurment == START_CALIBRATION) ? "calibration" : "measurement" );
        return -1;
    }
}

int Stop_CCP (int ConnectionNo, int CalibOrMeasurment)
{
    CCP_CONNECTTION *pCon;

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) return -1;
    pCon = &CcpConnections[ConnectionNo];
    if (get_pid_by_name ("CANServer") < 0) {
        ThrowError (1, "process \"CANServer\" not running. Start it before using CCP");
        return -1;
    }
    if (pCon->CcpCommandSequenceRequest == 0) {   // No command active?
        if (CalibOrMeasurment == START_CALIBRATION) {
            pCon->CcpCommandSequenceRequest = CCP_STOP_CALIBRATION;
        } else {
            pCon->CcpCommandSequenceRequest = CCP_STOP_MEASUREMENT;
        }
        return 0;
    } else {
        ThrowError (1, "cannot stop CCP %s because an other command is runnung", 
            (CalibOrMeasurment == START_CALIBRATION) ? "calibration" : "measurement" );
        return -1;
    }
}

int Is_CCP_AllCommandDone (int *pErrorCode)
{
    int x;
    int NegRet = 0;
    CCP_CONNECTTION *pCon;

    for (x = 0; x < 4; x++) {
        pCon = &CcpConnections[x];
        if (pErrorCode != NULL) *pErrorCode |= pCon->CcpErrCode;
        NegRet |= !((pCon->CcpCommandSequenceRequest == 0) && (pCon->CcpCommandSequence == 0) && (pCon->CppUploadInProgress == FALSE));
    }
    return !NegRet;
}

int Is_CCP_CommandDone (int Connection, int *pErrorCode, char **pErrText)
{
    CCP_CONNECTTION *pCon;

    if ((Connection >= 0) && (Connection < 4)) {
        pCon = &CcpConnections[Connection];
        if (pErrorCode != NULL) *pErrorCode = pCon->CcpErrCode;
        if (pErrText != NULL) *pErrText = CCPErrorNoToString(pCon->CcpErrCode);
        return ((pCon->CcpCommandSequenceRequest == 0) && (pCon->CcpCommandSequence == 0) && (pCon->CppUploadInProgress == FALSE));
    }
    return -1;
}

int RequestECUInfo_CCP (int ConnectionNo)
{
    CCP_CONNECTTION *pCon;

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) {
        ThrowError (1, "Connection number %i too large (0...3)", ConnectionNo);
        return -1;
    }
    pCon = &CcpConnections[ConnectionNo];
    if (get_pid_by_name ("CANServer") < 0) {
        ThrowError (1, "process \"CANServer\" not running. Start it before using CCP");
        return -1;
    }
    if (pCon->CcpCommandSequenceRequest == 0) {   // No command active?
        if (get_pid_by_name ("CCP_Control") < 0) {
            start_process ("CCP_Control");
        }
        // start command scheduler
        pCon->CcpCommandSequenceRequest = CCP_READ_TCU_INFO;
        return 0;
    } else {
        ThrowError (1, "cannot read ECU infos over CCP because an other command is runnung");
        return -1;
    }

}

int GetECUInfos_CCP (int ConnectionNo, char *String, int maxc)
{
    UNUSED(maxc);
    CCP_CONNECTTION *pCon;
    int Ret;

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) {
        ThrowError (1, "Connection number %i too large (0...3)", ConnectionNo);
        return -1;
    }
    pCon = &CcpConnections[ConnectionNo];

    if (((pCon->gCcpConnected & MEASUREMENT_RUNNING_MASK) == MEASUREMENT_RUNNING_MASK) ||
        ((pCon->gCcpConnected & CALIBRATION_ACTIVE_MASK) == CALIBRATION_ACTIVE_MASK)) { 
        pCon->ccp_infos.TargetIdentifier[sizeof(pCon->ccp_infos.TargetIdentifier)-1] = 0;
        PrintFormatToString (String, maxc, "CCP-Version = %i.%i TCU-String = \"%s\" AvailabilityMask = 0x%02X ProtectionMask = 0x%02X "
                "DAQ Size = %i DAQ pid = %i",
                (int)pCon->ccp_infos.major_version,
                (int)pCon->ccp_infos.minor_version,
                pCon->ccp_infos.TargetIdentifier,
                (int)pCon->ccp_infos.ResourceAvailabilityMask,
                (int)pCon->ccp_infos.ResourceProtectionMask,
                pCon->ccp_infos.DAQ_Size,
                pCon->ccp_infos.DAQ_Pid);
        Ret = 0;
    } else {
        PrintFormatToString (String, maxc, "CCP conection not active");
        Ret = -1;
    }
    return Ret;
}

int GetECUString_CCP (int ConnectionNo, char *String, int maxc)
{
    UNUSED(maxc);
    CCP_CONNECTTION *pCon;
    int Ret;

    if ((ConnectionNo < 0) || (ConnectionNo >= 4)) {
        ThrowError (1, "Connection number %i too large (0...3)", ConnectionNo);
        return -1;
    }
    pCon = &CcpConnections[ConnectionNo];

    if (((pCon->gCcpConnected & MEASUREMENT_RUNNING_MASK) == MEASUREMENT_RUNNING_MASK) ||
        ((pCon->gCcpConnected & CALIBRATION_ACTIVE_MASK) == CALIBRATION_ACTIVE_MASK)) { 
        pCon->ccp_infos.TargetIdentifier[sizeof(pCon->ccp_infos.TargetIdentifier)-1] = 0;
        PrintFormatToString (String, maxc, "%s", pCon->ccp_infos.TargetIdentifier);
        Ret = 0;
    } else {
        PrintFormatToString (String, maxc, "CCP conection not active");
        Ret = -1;
    }
    return 0;
}

int LoadConfig_CCP (int ConnectionNo, const char *fname)
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
    PrintFormatToString (SectionSrc, sizeof(SectionSrc), "CCP Configuration for Target");
    PrintFormatToString (SectionDst, sizeof(SectionDst), "CCP Configuration for Target %i", ConnectionNo);

    if (IniFileDataBaseCopySection(GetMainFileDescriptor(), Fd, SectionDst, SectionSrc)) {
        return -1;
    }

    return 0;
}


int SaveConfig_CCP (int ConnectionNo, char *fname)
{
    char SectionSrc[256];
    char SectionDst[256];
    int Fd;

    PrintFormatToString (SectionDst, sizeof(SectionDst), "CCP Configuration for Target");
    PrintFormatToString (SectionSrc, sizeof(SectionSrc), "CCP Configuration for Target %i", ConnectionNo);

    remove (fname);
    Fd = IniFileDataBaseCreateAndOpenNewIniFile(fname);
    if (Fd < 0) {
        return -1;
    }
    IniFileDataBaseCopySection(Fd, GetMainFileDescriptor(), SectionDst, SectionSrc);
    if (IniFileDataBaseSave(Fd, NULL, 2)) {
        ThrowError (1, "cannot save CCP config to file %s", fname);
    }
    return 0;
}


//** CCP Calibration ******************************************************************************************

void StartDownloadCmdScheduler (uint32_t arg_Address, int arg_Size, void *arg_Data, CCP_CONNECTTION *pCon)
{
    pCon->CalAddress = arg_Address;
    if (arg_Size > CCP_MAX_DOWNLOAD_SIZE) {
        ThrowError (1, "cannot download packages larger than %i Bytes, your package is %i bytes lage", (int)CCP_MAX_DOWNLOAD_SIZE, arg_Size); 
    }
    pCon->CalSize = arg_Size;
    MEMCPY (pCon->CalData, arg_Data, (size_t)arg_Size);
    pCon->CcpCommandSequence = CCP_DOWNLOAD_SEQUENCE;
    pCon->CcpCommand = CCP_SET_MTA;
}


void StartUploadCmdScheduler (uint32_t arg_Address, int arg_Size, int arg_UploadCcpCalParamPos, CCP_CONNECTTION *pCon)
{
    pCon->CalAddress = arg_Address;
    pCon->CalSize = arg_Size;
    pCon->UploadCcpCalParamPos = arg_UploadCcpCalParamPos;
    pCon->CcpCommandSequence = CCP_UPLOAD_SEQUENCE;
    pCon->CcpCommand = CCP_SET_MTA;
}


void Download_CmdScheduler (CCP_CONNECTTION *pCon, int Connection)
{
    CCP_CRO_CAN_OBJECT CanObject;

    switch (pCon->CcpCommand)
    {
    case CCP_SET_MTA:
        ccpSetMta (0, pCon->CalAddress, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_MTA_WAIT_ACK;
        break;
    case CCP_SET_MTA_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_MTA", &CanObject);
            pCon->CcpCommand = CCP_DATA_DOWNLOAD;
            pCon->CalPos = 0;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_DATA_DOWNLOAD:
        ccpDownload (&pCon->CalData[pCon->CalPos], x__MIN ((pCon->CalSize - pCon->CalPos), 5), pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_DATA_DOWNLOAD_WAIT_ACK;
        break;
    case CCP_DATA_DOWNLOAD_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "DNLOAD", &CanObject);
            pCon->CalPos += x__MIN ((pCon->CalSize - pCon->CalPos), 5);
            if (pCon->CalPos >= pCon->CalSize) {
                // readback written data
                if (pCon->CcpConfig.ReadParameterAfterCalib) {
                    StartUploadCmdScheduler (pCon->CalAddress, pCon->CalSize, pCon->CcpCalParamPos, pCon);
                } else {
                    pCon->CcpCommand = 0;
                    pCon->CcpCommandSequence = 0;
                }
            } else {
                pCon->CcpCommand = CCP_DATA_DOWNLOAD;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    }
}



void Upload_CmdScheduler (CCP_CONNECTTION *pCon, int Connection)
{
    CCP_CRO_CAN_OBJECT CanObject;
    union BB_VARI help;
    uint64_t wrflag;

    switch (pCon->CcpCommand)
    {
    case CCP_SET_MTA:
        ccpSetMta (0, pCon->CalAddress, pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_SET_MTA_WAIT_ACK;
        break;
    case CCP_SET_MTA_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "SET_MTA", &CanObject);
            pCon->CcpCommand = CCP_DATA_UPLOAD;
            pCon->CalPos = 0;
        }
        CCP_CHECK_TIMEOUT();
        break;
    case CCP_DATA_UPLOAD:
        ccpUpload (x__MIN ((pCon->CalSize - pCon->CalPos), 5), pCon);
        SetTimeOut ();
        pCon->CcpCommand = CCP_DATA_UPLOAD_WAIT_ACK;
        break;
    case CCP_DATA_UPLOAD_WAIT_ACK:
        if (CcpCheckAckMessage (&CanObject, pCon)) {
            PrintACKMessage (pCon, "UPLOAD", &CanObject);
            MEMCPY (&(pCon->CalData[pCon->CalPos]), &(CanObject.Data[3]), (size_t)x__MIN ((pCon->CalSize - pCon->CalPos), 5));
            pCon->CalPos += x__MIN ((pCon->CalSize - pCon->CalPos), 5);
            if (pCon->CalPos >= pCon->CalSize) {
                pCon->CcpCommand = 0;
                pCon->CcpCommandSequence = 0;
                // Write to the blackboard
                switch (get_bbvaritype(pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid)) {
                case BB_BYTE:
                    help.b = *(signed char*)&pCon->CalData[0]; 
                    write_bbvari_byte (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, help.b);
                    break;
                case BB_UBYTE:
                    help.ub = *(unsigned char*)&pCon->CalData[0];
                    write_bbvari_ubyte (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, help.ub);
                    break;
                case BB_WORD:
                    help.w = *(signed short*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST ((void*)&help.w, sizeof(help.w));
                    write_bbvari_word (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, help.w);
                    break;
                case BB_UWORD:
                    help.uw = *(unsigned short*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST ((void*)&help.uw, sizeof(help.uw));
                    write_bbvari_uword (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, help.uw);
                    break;
                case BB_DWORD:
                    help.dw = *(int32_t*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST ((void*)&help.dw, sizeof(help.dw));
                    write_bbvari_dword (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, help.dw);
                    break;
                case BB_UDWORD:
                    help.udw = *(uint32_t*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST ((void*)&help.udw, sizeof(help.udw));
                    write_bbvari_udword (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, help.udw);
                    break;
                case BB_FLOAT:
                    help.f = *(float*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST ((void*)&help.f, sizeof(help.f));
                    write_bbvari_float (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, help.f);
                    break;
                case BB_DOUBLE: // Remark double would be converted to float
                    help.d = *(double*)(void*)&pCon->CalData[0];
                    SWAP_TO_MSB_FIRST ((void*)&help.d, sizeof(help.d));
                    write_bbvari_double (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, help.d);
                    break;
                default:
                    break;
                }
                // Avoid that immediately a download would follow
                get_free_wrflag (get_pid_by_name ("CCP_Control"), &wrflag);
                reset_wrflag (pCon->CcpCalParamList[pCon->UploadCcpCalParamPos].vid, wrflag);
            } else {
                pCon->CcpCommand = CCP_DATA_UPLOAD;
            }
        }
        CCP_CHECK_TIMEOUT();
        break;
    }
}


void CCPWriteCharacteristics (CCP_CONNECTTION *pCon)
{
    uint64_t wrflag;
    union BB_VARI help;
    int CcpCalParamPosOld;

    get_free_wrflag(get_pid_by_name ("CCP_Control"), &wrflag);
    CcpCalParamPosOld = pCon->CcpCalParamPos;
    while ((pCon->CcpCalParamList[pCon->CcpCalParamPos].vid <= 0) ||    // not a valid blackboard variable
           !test_wrflag(pCon->CcpCalParamList[pCon->CcpCalParamPos].vid, wrflag)) {
        pCon->CcpCalParamPos++;
        if (pCon->CcpCalParamPos >= pCon->CcpCalParamListSize) pCon->CcpCalParamPos = 0;
        if (pCon->CcpCalParamPos == CcpCalParamPosOld) return;  // no write flag set
    }
    reset_wrflag(pCon->CcpCalParamList[pCon->CcpCalParamPos].vid, wrflag);
    switch (get_bbvaritype(pCon->CcpCalParamList[pCon->CcpCalParamPos].vid)) {
    case BB_BYTE:
        help.b = read_bbvari_byte (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid);
        StartDownloadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 1, &help.b, pCon);
        break;
    case BB_UBYTE:
        help.ub = read_bbvari_ubyte (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid);
        StartDownloadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 1, &help.ub, pCon);
        break;
    case BB_WORD:
        help.w = read_bbvari_word (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST ((void*)&help.w, sizeof(help.w));
        StartDownloadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 2, &help.w, pCon);
        break;
    case BB_UWORD:
        help.uw = read_bbvari_uword (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST ((void*)&help.uw, sizeof(help.uw));
        StartDownloadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 2, &help.uw, pCon);
        break;
    case BB_DWORD:
        help.dw = read_bbvari_dword (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST ((void*)&help.dw, sizeof(help.dw));
        StartDownloadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 4, &help.dw, pCon);
        break;
    case BB_UDWORD:
        help.udw = read_bbvari_udword (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST ((void*)&help.udw, sizeof(help.udw));
        StartDownloadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 4, &help.udw, pCon);
        break;
    case BB_FLOAT:
        help.f = read_bbvari_float (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST ((void*)&help.f, sizeof(help.f));
        StartDownloadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 4, &help.f, pCon);
        break;
    case BB_DOUBLE: // Remark double would be converted to float
        help.d = read_bbvari_double (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid);
        SWAP_TO_MSB_FIRST ((void*)&help.d, sizeof(help.d));
        StartDownloadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 8, &help.d, pCon);
        break;
    default:
        break;
    }
}

int CCPReadCharacteristics (CCP_CONNECTTION *pCon)
{
    if (pCon->CcpCalParamPos >= pCon->CcpCalParamListSize) {
        // End of the list, all choosen parameter has been read
        pCon->CcpCalParamPos = 0;
        return ALL_PARAM_READ;
    }
    else {
        if (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid > 0) {  // valid blackboard variable
            // Next element of the paramteter list
            StartUploadCmdScheduler (pCon->CcpCalParamList[pCon->CcpCalParamPos].Address, 
                                     bbvari_sizeof(get_bbvaritype (pCon->CcpCalParamList[pCon->CcpCalParamPos].vid)),
                                     pCon->CcpCalParamPos, pCon);
        }
        pCon->CcpCalParamPos++;
        return MORE_PARAM_READ_PENDING;
    }
}


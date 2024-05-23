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


/*! Global Defines for XCP
    *\brief Standard Commands
*/
#define CC_CONNECT          0xFF
#define CC_DISCONNECT       0xFE
#define CC_GET_STATUS       0xFD
#define CC_SYNCH            0xFC


#define CC_GET_COMM_MODE_INFO             0xFB
#define CC_GET_ID                         0xFA
#define CC_SET_REQUEST                    0xF9
#define CC_GET_SEED                       0xF8
#define CC_UNLOCK                         0xF7
#define CC_SET_MTA                        0xF6
#define CC_UPLOAD                         0xF5
#define CC_SHORT_UPLOAD                   0xF4
#define CC_BUILD_CHECKSUM                 0xF3
                                          
#define CC_TRANSPORT_LAYER_CMD            0xF2
#define CC_USER_CMD                       0xF1

/*-------------------------------------------------------------------------*/
/* Calibration Commands*/

#define CC_DOWNLOAD                       0xF0

#define CC_DOWNLOAD_NEXT                  0xEF
#define CC_DOWNLOAD_MAX                   0xEE
#define CC_SHORT_DOWNLOAD                 0xED
#define CC_MODIFY_BITS                    0xEC

/*-------------------------------------------------------------------------*/
/* Page switching Commands (PAG) */

#define CC_SET_CAL_PAGE                   0xEB
#define CC_GET_CAL_PAGE                   0xEA

#define CC_GET_PAG_PROCESSOR_INFO         0xE9
#define CC_GET_SEGMENT_INFO               0xE8
#define CC_GET_PAGE_INFO                  0xE7
#define CC_SET_SEGMENT_MODE               0xE6
#define CC_GET_SEGMENT_MODE               0xE5
#define CC_COPY_CAL_PAGE                  0xE4


/*-------------------------------------------------------------------------*/
/* DATA Acquisition and Stimulation Commands (DAQ/STIM) */
                                          
#define CC_CLEAR_DAQ_LIST                 0xE3
#define CC_SET_DAQ_PTR                    0xE2
#define CC_WRITE_DAQ                      0xE1
#define CC_SET_DAQ_LIST_MODE              0xE0
#define CC_GET_DAQ_LIST_MODE              0xDF
#define CC_START_STOP_DAQ_LIST            0xDE
#define CC_START_STOP_SYNCH               0xDD

#define CC_GET_DAQ_CLOCK                  0xDC
#define CC_READ_DAQ                       0xDB
#define CC_GET_DAQ_PROCESSOR_INFO         0xDA
#define CC_GET_DAQ_RESOLUTION_INFO        0xD9 
#define CC_GET_DAQ_LIST_INFO              0xD8
#define CC_GET_DAQ_EVENT_INFO             0xD7

#define CC_FREE_DAQ                       0xD6
#define CC_ALLOC_DAQ                      0xD5
#define CC_ALLOC_ODT                      0xD4
#define CC_ALLOC_ODT_ENTRY                0xD3


/*-------------------------------------------------------------------------*/
/* Packet Identifiers Slave -> Master */
#define PID_RES                           0xFF   /* response packet        */
#define PID_ERR                           0xFE   /* error packet           */
#define PID_EV                            0xFD   /* event packet           */
#define PID_SERV                          0xFC   /* service request packet */


/*-------------------------------------------------------------------------*/
/* ResourceMask (CONNECT) */

#define RM_CAL_PAG                  0x01
#define RM_DAQ                      0x04
#define RM_STIM                     0x08
#define RM_PGM                      0x10


/*-------------------------------------------------------------------------*/
/* CommModeBasic (CONNECT) */

#define PI_MSB_FIRST                   0x01
#define PI_LSB_FIRST                   0x00

#define CMB_BYTE_ORDER                (0x01u<<0)
#define CMB_ADDRESS_GRANULARITY       (0x03u<<1)
#define CMB_SLAVE_BLOCK_MODE          (0x01u<<6)
#define CMB_OPTIONAL                  (0x01u<<7)

#define CMB_ADDRESS_GRANULARITY_BYTE  (0<<1)
#define CMB_ADDRESS_GRANULARITY_WORD  (1<<1)
#define CMB_ADDRESS_GRANULARITY_DWORD (2<<1)
#define CMB_ADDRESS_GRANULARITY_QWORD (3<<1)

/*-------------------------------------------------------------------------*/
/* Session Status (GET_STATUS and SET_REQUEST) */

#define SS_STORE_CAL_REQ       0x01u 
#define SS_BLOCK_UPLOAD        0x02u /* Internal */
#define SS_STORE_DAQ_REQ       0x04u
#define SS_CLEAR_DAQ_REQ       0x08u
#define SS_ERROR               0x10u /* Internal */
#define SS_CONNECTED           0x20u /* Internal */
#define SS_DAQ                 0x40u
#define SS_RESUME              0x80u


/*-------------------------------------------------------------------------*/
/* Identifier Type (GET_ID) */

#define IDT_ASCII              0
#define IDT_ASAM_NAME          1
#define IDT_ASAM_PATH          2
#define IDT_ASAM_URL           3
#define IDT_ASAM_UPLOAD        4
#define IDT_VECTOR_MAPNAMES    0xDB


/*-------------------------------------------------------------------------*/
/* PAGE_PROPERTIES (GET_PAGE_INFO)*/

#define ECU_ACCESS_TYPE                   0x03
#define XCP_READ_ACCESS_TYPE              0xC0
#define XCP_WRITE_ACCESS_TYPE             0x30

/* ECU_ACCESS_TYPE */
#define ECU_ACCESS_NONE                   (0<<0)
#define ECU_ACCESS_WITHOUT                (1<<0)
#define ECU_ACCESS_WITH                   (2<<0)
#define ECU_ACCESS_DONT_CARE              (3<<0)

/* XCP_READ_ACCESS_TYPE */
#define XCP_READ_ACCESS_NONE              (0<<2)
#define XCP_READ_ACCESS_WITHOUT           (1<<2)
#define XCP_READ_ACCESS_WITH              (2<<2)
#define XCP_READ_ACCESS_DONT_CARE         (3<<2)

/* XCP_WRITE_ACCESS_TYPE */
#define XCP_WRITE_ACCESS_NONE             (0<<4)
#define XCP_WRITE_ACCESS_WITHOUT          (1<<4)
#define XCP_WRITE_ACCESS_WITH             (2<<4)
#define XCP_WRITE_ACCESS_DONT_CARE        (3<<4)

/* Get Page Processor info Response*/
#define PAG_PROPERTY_FREEZE               0x01


/* Calculation of checksum */
#define XCP_ADD_11              0x01
#define XCP_ADD_12              0x02
#define XCP_ADD_14              0x03
#define XCP_ADD_22              0x04
#define XCP_ADD_24              0x05
#define XCP_ADD_44              0x06
#define XCP_CRC_16              0x07
#define XCP_CRC_16_CCITT        0x08
#define XCP_CRC_32              0x09

//Error Responses
#define ERR_CMD_SYNCH           0x00
#define ERR_CMD_UNKNOWN         0x20
#define ERR_OUT_OF_RANGE        0x22
#define ERR_WRITE_PROTECTED     0x23
#define ERR_PAGE_NOT_VALID      0x26
#define ERR_MODE_NOT_VALID      0x27
#define ERR_SEGMENT_NOT_VALID   0x28
#define ERR_SEQUENCE            0x29
#define ERR_MEMORY_OVERFLOW     0x30

/*-------------------------------------------------------------------------*/
/* Event Codes */

#define EVC_RESUME_MODE        0x00
#define EVC_CLEAR_DAQ          0x01
#define EVC_STORE_DAQ          0x02
#define EVC_STORE_CAL          0x03
#define EVC_CMD_PENDING        0x05
#define EVC_DAQ_OVERLOAD       0x06
#define EVC_SESSION_TERMINATED 0x07
#define EVC_USER               0xFE
#define EVC_TRANSPORT          0xFF


/*-------------------------------------------------------------------------*/
/* GET_DAQ_PROCESSOR_INFO */

/* DAQ_PROPERTIES */
#define DAQ_PROPERTY_CONFIG_TYPE          0x01
#define DAQ_PROPERTY_PRESCALER            0x02
#define DAQ_PROPERTY_RESUME               0x04
#define DAQ_PROPERTY_BIT_STIM             0x08
#define DAQ_PROPERTY_TIMESTAMP            0x10
#define DAQ_PROPERTY_NO_PID               0x20
#define DAQ_PROPERTY_OVERLOAD_INDICATION  0xC0

/* DAQ Overload Indication Type */
#define DAQ_OVERLOAD_INDICATION_NONE      (0<<6)
#define DAQ_OVERLOAD_INDICATION_PID       (1<<6)
#define DAQ_OVERLOAD_INDICATION_EVENT     (2<<6)

/* DAQ_KEY_uint8_t */
#define DAQ_OPT_TYPE                      0x0F
#define DAQ_EXT_TYPE                      0x30
#define DAQ_HDR_TYPE                      0xC0

/* DAQ Optimisation Type */
#define DAQ_OPT_DEFAULT                   (0<<0)
#define DAQ_OPT_ODT_16                    (1<<0)
#define DAQ_OPT_ODT_32                    (2<<0)
#define DAQ_OPT_ODT_64                    (3<<0)
#define DAQ_OPT_ALIGNMENT                 (4<<0)
#define DAQ_OPT_MAX_ENTRY_SIZE            (5<<0)

/* DAQ Address Extension Scope */
#define DAQ_EXT_FREE                      (0<<4)
#define DAQ_EXT_ODT                       (1<<4)
#define DAQ_EXT_DAQ                       (3<<4)

/* DAQ Identification Field Type */
#define DAQ_HDR_PID                       (0<<6)
#define DAQ_HDR_ODT_DAQB                  (1<<6)
#define DAQ_HDR_ODT_DAQW                  (2<<6)
#define DAQ_HDR_ODT_FIL_DAQW              (3<<6)
 

/*-------------------------------------------------------------------------*/
/* GET_DAQ_RESOLUTION_INFO */

/* TIMESTAMP_MODE */
#define DAQ_TIMESTAMP_SIZE  0x07
#define DAQ_TIMESTAMP_FIXED 0x08
#define DAQ_TIMESTAMP_UNIT  0xF0

/* DAQ Timestamp Size */
#define DAQ_TIMESTAMP_OFF         (0<<0)
#define DAQ_TIMESTAMP_uint8_t        (1<<0)
#define DAQ_TIMESTAMP_WORD        (2<<0)
#define DAQ_TIMESTAMP_DWORD       (4<<0)

/* DAQ Timestamp Unit */
#define DAQ_TIMESTAMP_UNIT_1NS     0
#define DAQ_TIMESTAMP_UNIT_10NS    1
#define DAQ_TIMESTAMP_UNIT_100NS   2
#define DAQ_TIMESTAMP_UNIT_1US     3
#define DAQ_TIMESTAMP_UNIT_10US    4  
#define DAQ_TIMESTAMP_UNIT_100US   5
#define DAQ_TIMESTAMP_UNIT_1MS     6
#define DAQ_TIMESTAMP_UNIT_10MS    7
#define DAQ_TIMESTAMP_UNIT_100MS   8
#define DAQ_TIMESTAMP_UNIT_1S      9


/*-------------------------------------------------------------------------*/
/* DAQ_LIST_PROPERTIES (GET_DAQ_LIST_INFO) */

#define DAQ_LIST_PREDEFINED           0x01
#define DAQ_LIST_FIXED_EVENT          0x02
#define DAQ_LIST_DIR_DAQ              0x04
#define DAQ_LIST_DIR_STIM             0x08


/*-------------------------------------------------------------------------*/
/* EVENT_PROPERTY (GET_DAQ_EVENT_INFO) */

#define DAQ_EVENT_DIRECTION_DAQ      0x04
#define DAQ_EVENT_DIRECTION_STIM     0x08
#define DAQ_EVENT_DIRECTION_DAQ_STIM 0x0C

/*-------------------------------------------------------------------------*/
/* Content of DAQ Mode */
#define DAQ_STOPPED                   0x00
#define DAQ_STARTED                   0x01
#define DAQ_SELECTED                  0x02

/*-------------------------------------------------------------------------*/
/* Session Status (GET_STATUS and SET_REQUEST) */

#define SS_STORE_CAL_REQ       0x01u 
#define SS_BLOCK_UPLOAD        0x02u /* Internal */
#define SS_STORE_DAQ_REQ       0x04u
#define SS_CLEAR_DAQ_REQ       0x08u
#define SS_ERROR               0x10u /* Internal */
#define SS_CONNECTED           0x20u /* Internal */
#define SS_DAQ                 0x40u
#define SS_RESUME              0x80u


/*****************************************************/
/*****************************************************/
/*****************************************************/
/*! Application Configuration
    *\brief Switches and sizes for the application
*/
//#define XCP_COMMUNICATION_THREAD   1
#define COMMUNICATION_PORT      1802
#define MAX_CTO                 0xFF              //Maximalgroesse der CTO Packages
#define MAX_DTO                 0x1000            //Maximalgroesse der DTO Packages
#define MAX_ODT                 0x20              //Maximum amount of ODTs to a DAQ
#define MAX_ODT_ENTRY_SIZE      0x10              //Maximum size of ODT entry
#define MAXDAQLISTS             10                //Maximum amount Of DAQ Lists
#define MAXEVENTCHANNELS        10                //Maximum amount of event channels
#define MAX_ID_LEN              0xFF              //Maximal length of the identification string

#define MAX_PAGES_FOR_SEGMENTS  2                 //Maximal Anzahl von Pages pro Segment

//States of Transfer Statemachine for responding CTO 
#define XCP_TR_IDLE             0x00              //Transferstate Idle
#define XCP_TR_RESP_CTO         0x01              //CTO Response muss gesendet werden
#define XCP_TR_SEND_MTA_BLOCK   0x02              //Send Block
#define XCP_TR_RESP_DAQ         0x03              //DAQ Response muss gesendet werden


/*****************************************************/
/*Supported resources*/
/* Supported are CAL/PAG und DAQ, aktuell kein STIM*/
const unsigned char projSetResources = RM_CAL_PAG | RM_DAQ; 
/*Basic comm mode settings*/
/*Byte order = LSB first, Granularity = uint8_t,No slave block mode, no additional information*/
const unsigned char projSetBasicCommMode = PI_LSB_FIRST | CMB_ADDRESS_GRANULARITY_BYTE;

/* Versions of protocoll layer and transport layer*/
const unsigned char xcpTransportLayerVersion = 1;
const unsigned char xcpProtocollLayerVersion = 1;

/*DAQ Properties*/
/* Supported are Dynamic DAQ Configuration and Prescaler*/
const unsigned char bscSetDAQ = DAQ_PROPERTY_CONFIG_TYPE | DAQ_PROPERTY_PRESCALER | DAQ_PROPERTY_TIMESTAMP; 

/*DAQ Keybyte*/
/*Supported: Default optimisation:
Absolute DAQ, Absolute ODT Number, adress extension free, Optimisation = default*/
const unsigned char bscSetDAQKeyB = 0x00; 

/*DAQ Resolution info*/
/*Granularity ODT Entry size DAQ*/
const unsigned char bscSetGranODTEntrySizeDaq = 1; /*= Byte*/

/*DAQ Timestamp Mode settings*/
/* Unit = 10ms, Timestamp size = 2*/
const unsigned char bscSetDAQTSMode = (DAQ_TIMESTAMP_UNIT_10MS <<4) | DAQ_TIMESTAMP_WORD |DAQ_TIMESTAMP_FIXED;

/*DAQ Timestamp Ticks*/
const unsigned char bscSetDAQTSTicks = 1;

/*DAQ Event Properties*/
const unsigned char bscSetDaqEventProperties = DAQ_EVENT_DIRECTION_DAQ;  //Actually just DAQ supported




/*! Global Types for XCP
    *\brief XCP over IP Header
*/
#pragma pack(push,2)
typedef struct _XCP_OVER_IP_HEADER
{
  uint16_t    usLen;
  uint16_t    usCounter;
}XCP_OVER_IP_HEADER;


typedef struct xcpDto {
//  unsigned char l;
//#if defined ( kXcpPacketHeaderSize )
//  unsigned char h[kXcpPacketHeaderSize];
//#endif
  unsigned char bData[MAX_DTO];
} tXcpDto;

typedef union 
{
  /* There might be a loss of up to 3 bytes. */
  unsigned char  b[ ((MAX_CTO + 3) & 0xFFC)      ];
  uint16_t w[ ((MAX_CTO + 3) & 0xFFC) / 2  ];
  uint32_t dw[ ((MAX_CTO + 3) & 0xFFC) / 4 ];
} tXcpCto;

typedef struct _XCP_BASIC_REQUEST
{
  uint16_t    usLen;
  uint16_t    usCounter;
  tXcpCto           xcpRequest;
}XCP_BASIC_REQUEST;

typedef struct _XCP_BASIC_RESPONSE
{
  uint16_t    usLen;
  uint16_t    usCounter;
  tXcpCto           xcpResponse;
}XCP_BASIC_RESPONSE;

typedef struct _XCP_DAQ_RESPONSE
{
  uint16_t    usLen;
  uint16_t    usCounter;
  tXcpDto           xcpResponse;
  //unsigned char*    Mta; 
  //uint32_t     MtaTransferred;
}XCP_DAQ_RESPONSE;

#pragma pack(pop)

//Storage of Segment Info
typedef struct _XCP_SEGMENT_INFO
{
  uint32_t     baseAdress;
  uint32_t     Length;
  unsigned char     Number;
  unsigned char     Pages;
}XCP_SEGMENT_INFO;

//ODT Entry Struktur
typedef struct _XCP_ODT_ENTRY
{
  unsigned char     bitOffset;
  unsigned char     sizeOfElement;
  unsigned char     adressEytension;
  uint64_t     adressOfElement;
}XCP_ODT_ENTRY;

//ODT Struktur
typedef struct _XCP_ODT
{
  //ODT Entries
  XCP_ODT_ENTRY*        m_odtBasePtr;           //BasePtr to ODT List
  XCP_ODT_ENTRY*        m_curODTPtr;            //Zeiger auf das aktuell selektierte ODT Element
  uint8_t                  m_odtCount;             //Current MAX Count of ODT List
  uint8_t                  m_AllocatedElements;    //Amount of allocated elements

  //Transfer Field to that ODT
  uint8_t*                 m_DAQTransferField;     //Last DAQ Data List
  uint16_t        m_DAQTransferFieldAllocated;  //Allocated amount of BYTEs
  uint16_t        m_DAQTransferFieldLen;  //Current transfer Field len

}XCP_ODT;

/*! Prototype of a DAQ Element
    *\brief DAQs are used in asyncron communications to respond measuremnts to the server
    *\note DAQs are dynamically defined structures which describe measurements 
    * Also, the structure of ODTs defines the transmission to the server
*/
typedef struct _XCP_DAQ
{
  //ODT References
  XCP_ODT*              m_PtrODTs[MAX_ODT];     //Pointer to DAQ owned ODTs
  uint8_t                  m_allocatedODTs;        //Currently accocated amount of ODTs
  XCP_ODT*              m_ptrCurrentODT;        //Pointer to current ODT

  //DAQ
  uint8_t                  m_ListMode;             //Current DAQ List Mode
  WORD                  m_EventChannel;         //Event Channel Number
  uint8_t                  m_Prescaler;            //Transmission rate prescaler
  uint8_t                  m_Cycle;                //Current Event Cycle
  uint8_t                  m_Prio;                 //DAQ List Priority

  //State Handling
  uint8_t                  m_State;                //Current state of DAQ List
  bool                  m_Sampled;              //Currently sampled
  bool                  m_Error;                //Current error state
}XCP_DAQ;

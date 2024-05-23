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


#ifndef _RDCANCFG_H
#define _RDCANCFG_H

#include "SharedDataTypes.h"

#define MAX_CAN_CHANNELS 16

#define MAX_OBJECTS_ALL_CHANNELS     1024
#define MAX_TX_OBJECTS_ONE_CHANNEL    256
#define MAX_RX_OBJECTS_ONE_CHANNEL    256
#define MAX_OBJECTS_ONE_CHANNEL       256

#define MAX_SIGNALS_ALL_CHANNELS     4096
#define MAX_SIGNALS_ONE_OBJECT        256

typedef  struct {
#ifdef _M_X64
    /* 8 */    uint8_t *Data;
    /* 8 */    uint8_t *OldData;
    /* 8 */    uint8_t *DataPtr;   // ist bei nicht MultiPackage Botschaften immer gleich Data
#else
    /* 4 */    uint8_t *Data;
    /* 4 */    uint8_t *Data_hdw;
    /* 4 */    uint8_t *OldData;
    /* 4 */    uint8_t *OldData_hdw;
    /* 4 */    uint8_t *DataPtr;   // ist bei nicht MultiPackage Botschaften immer gleich Data
    /* 4 */    uint8_t *DataPtr_hdw;   // ist bei nicht MultiPackage Botschaften immer gleich Data
#endif
#ifdef _M_X64
    /* 8 */    int16_t *signals_ptr;
#else
    /* 4 */    int16_t *signals_ptr;
    /* 4 */    int16_t *signals_ptr_hdw;
#endif
    /* 4 */    int32_t vid;
    /* 4 */    int32_t cycles;
    /* 4 */    int32_t delay;
    /* 4 */    int32_t cycles_counter;

    /* 4 */    int32_t EventOrCyclic;    // 0 -> Cyclic 1 -> Event
    /* 4 */    int32_t EventIdx;

    // Equation vor dem Lesen/Schreiben der Variablen (wenn <= 0 keine)
    /* 4 */    int32_t EquationBeforeIdx;
    // Start einer Liste von Indexen, die mit 0 terminiert ist
    // Equation nach dem Lesen/Schreiben der Variablen (wenn <= 0 keine)
    /* 4 */    int32_t EquationBehindIdx;

    /* 4 */    int32_t AdditionalVariablesIdx;

    // wie sollen Bits vorinitiallisiert werden
    /* 4 */    int32_t InitDataIdx;

    /* 4 */    int32_t signal_count;

    /* 4 */    int32_t SignalsArrayIdx;


/* 4 */    uint32_t id;

/* 2 */    int16_t size;       // kann bei Multipackage Botschaften auch > 8 sein
/* 2 */    int16_t max_size;   // normaly the same as size except CAN FD multi PGs

/* 1 */    int8_t ext;         // 0 standard ID's (11bit) 1 extended ID's (29bit)
    #define STANDARD_ID     0
    #define EXTENDED_ID     1
    #define CAN_EXT_MASK    1
    #define CANFD_BTS_MASK  2
    #define CANFD_FDF_MASK  4
/* 1 */    int8_t type;        // 0 normal
    #define NORMAL_OBJECT 0
    #define MUX_OBJECT    1
    #define J1939_OBJECT  3
    #define J1939_22_MULTI_C_PG  4
    #define J1939_22_C_PG    5
/* 1 */    int8_t dir;
    #define READ_OBJECT   0
    #define WRITE_OBJECT  1
    #define WRITE_VARIABLE_ID_OBJECT  2

/* 1 */    int8_t byteorder;
/* 1 */    int8_t flag;

/* 1 */    int8_t empty_filler; //bit_rate_switch;
/* 1 */    int8_t cycle_is_execst;
/* 1 */    int8_t should_be_send;

           union {
               struct {
/* 4 */            uint32_t Mask;
/* 4 */            int32_t Value;
/* 4 */            int32_t next;
/* 4 */            int32_t master;               // -1 Master sonst Index
/* 4 */            int32_t token;
/* 2 */            int16_t BitPos;
/* 1 */            int8_t Size;
/* 1 */            int8_t fill1;
/*23 */        } Mux;
               struct {
/* 4 */            uint32_t C_PGs_ArrayIdx;
/* 4 */            int32_t vid_dlc;   // if <= 0 than fixed
/* 4 */            int32_t ret_dlc;
/* 4 */            int32_t dst_addr_vid;
/* 1 */            uint8_t dst_addr;
/* 1 */            uint8_t status;
               } J1939;
/* 23 */   } Protocol;

           //int8_t fill[256-232];
           int8_t fill[128-120];
}  NEW_CAN_SERVER_OBJECT;     /* 128 Bytes */

typedef struct {
/* 8 */    union FloatOrInt64 conv;
/* 8 */    union FloatOrInt64 offset;
/* 8 */    union FloatOrInt64 start_value;
/* 8 */    uint64_t mask;
/* 4 */    int32_t NameIdx;
/* 4 */    int32_t UnitIdx;
/* 4 */    int32_t type;        // 0 normal
    #define NORMAL_SIGNAL 0
    #define MUX_SIGNAL    1
    #define MUX_BY_SIGNAL 2
/* 4 */    int32_t byteorder;
    #define LSB_FISRT_BYTEORDER  0
    #define MSB_FIRST_BYTEORDER  1
/* 4 */    int32_t bbtype;      // BB_BYTE, BB_UBYTE, ...
/* 4 */    int32_t startbit;
/* 4 */    int32_t bitsize;
/* 4 */    int32_t mux_startbit;   // oder bei type == MUX_BY_SIGNAL ist hier die VID des Multiplexer Signals gespeichert
/* 4 */    int32_t mux_bitsize;
/* 4 */    int32_t mux_value;
/* 4 */    int32_t mux_next;
/* 4 */    int32_t mux_equ_sb_childs; 
/* 4 */    int32_t mux_equ_v_childs; 
/* 4 */    int32_t mux_token; 
/* 4 */    uint32_t mux_mask;
/* 4 */    int32_t vid;
/* 4 */    int32_t start_value_flag;        // wenn 1 dann ist Start-Value ins BB zu uebernehmen wenn 0 dann nicht
/* 4 */    int32_t sign;
/* 4 */    int32_t ConvType;
#define CAN_CONV_NONE       0
#define CAN_CONV_FACTOFF    1
#define CAN_CONV_EQU        2
#define CAN_CONV_CURVE      3
#define CAN_CONV_FACTOFF2   4
#define CAN_CONV_REPLACED   5
/* 4 */    int32_t ConvIdx;
/* 4 */    int32_t float_type;

/* 4 */    int32_t conv_type;
/* 4 */    int32_t offset_type;
/* 4 */    int32_t start_value_type;
//    int8_t fill[128-128];
}  NEW_CAN_SERVER_SIGNAL;      /* 128 Bytes */

typedef struct {
//    int16_t pos;
    int32_t pos;
    uint32_t id;
}  NEW_CAN_HASH_ARRAY_ELEM;


// J1939

#define J1939TP_MP_CHANNELS_MAX 4
#define J1939TP_MP_MAX_SRC_ADDRESSES  4
#define J1939TP_MP_MAX_OBJECTS  16

//*** RX Channel Control types
typedef enum _eJ1939Tp_RxChanCtrlTypes
{
  E_J1939TP_RX_CHAN_CTRL_FREE       = 0u,    // rx channel not used
  E_J1939TP_RX_CHAN_CTRL_RX_PENDING = 1u     // rx channel used for RX message
} eJ1939Tp_RxChanCtrlTypes;

//*** TX Channel Control types
typedef enum _eJ1939Tp_TxChanCtrlTypes
{
  E_J1939TP_TX_CHAN_CTRL_FREE           = 0u,    // tx channel not used
  E_J1939TP_TX_CHAN_CTRL_NEWOBJECT      = 1u,    // tx channel shall start with new object
  E_J1939TP_TX_CHAN_CTRL_WAITFOR_CTS    = 2u,    // tx channel waits for CTS message
  E_J1939TP_TX_CHAN_CTRL_WAITFOR_EOMACK = 3u,    // tx channel waits for EOMACK message
  E_J1939TP_TX_CHAN_CTRL_TRM_STOPPED    = 4u,    // tx channel transmission is stopped
  E_J1939TP_TX_CHAN_CTRL_SEND_DATA      = 5u     // tx channel shall send data
} eJ1939Tp_TxChanCtrlTypes;

//*** structure to control the reception of multipackage messages
typedef struct J1939Tp_MP_RxChannel {
/*8*/    uint64_t Timeout;
/*8*/    uint8_t  *p_data;                  // Pointer to msg-data
/*4*/    uint32_t MsgHandle;                // Message id for callback
/*4*/    eJ1939Tp_RxChanCtrlTypes ChannelCtrl; // channel control
/*4*/    uint32_t RxMsgPgn;                 //  Parameter Group Number of packeted message!
/*2*/    uint16_t NoOfDataBytes;            // total number of transmitted data bytes
/*1*/    uint8_t CMType;                    // connection management type (BAM / RTS CTS)
/*1*/    uint8_t SourceAddr;               // Source Adress
/*1*/    uint8_t DestAddr;                 // destination adress
/*1*/    uint8_t NumOfAllPackages;         // total number of data messages
/*1*/    uint8_t NumOfReceivedPackages;    // number of already received data messages
         uint8_t Fill[64-35];
}  sJ1939Tp_MP_RxChannel;

//*** structure to control the transmission of multipackage messages
typedef struct J1939Tp_MP_TxChannel {
/*8*/    uint64_t Timeout;
/*8*/    uint8_t *p_data;                   // Pointer to msg-data
/*4*/    uint32_t MsgHandle;                // Message id for callback
/*4*/    eJ1939Tp_TxChanCtrlTypes ChannelCtrl;  // status of the TX operation
/*4*/    uint32_t Id;
/*2*/    uint16_t Dlc;
/*1*/    uint8_t CMType;                   // connection management type (BAM / RTS CTS)
/*1*/    uint8_t NumOfPackagesTilNewCTS;   // number of expected receive data messages
/*1*/    uint8_t NumOfTransmittedPackages; // number of already received data messages
/*1*/    uint8_t DestAddr;                 // target address of message
         uint8_t Fill[64-34];
}  sJ1939Tp_MP_TxChannel;

// end J1939

typedef struct NEW_CAN_SERVER_CONFIG_STRUCT {
    int32_t channel_count;    // 1, 2, 3, oder 4
    int32_t channel_type;
    #define PC_I_03_2_527         1
    #define iPC_I_165_PCI_2_527   2
    #define PC_I_03_2_527_IRQ     3
    #define TIN_CAN               4
    int32_t Fill0;
    int32_t dont_use_units_for_bb; // do not use units in signal definitions for blackboard variable descriptions
    int32_t dont_use_init_values_for_existing_variables;

    uint8_t j1939_flag; // ist gesetzt falls auf einem Kanal J1939 aktiv ist. Jeder Kanal hat noch ein eigenes Flag
    uint8_t EnableGatewayDeviceDriver;
    uint8_t Fill1;
    uint8_t Fill2;

    struct {
    /*4*/    int32_t enable;          // Kanal ist aktiv
    /*4*/    int32_t bus_frq;
    /*4*/    int32_t can_fd_enabled;
    /*4*/    int32_t data_baud_rate;
    /*8*/    double sample_point;
    /*8*/    double data_sample_point;
    /*4*/    uint32_t CardTypeNrChannel;  // Byte0 ist Kanal auf Karte, Byte1 Karten Nummer, Byte2 ist Karten-Type (zur Zeit ist Byte2 immer 0)
	/*4*/    int32_t Socket;
#ifdef _M_X64
    /*8*/    int (*read_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, int o_pos);
                    /* Funktionszeiger fuer Schreiben eines CAN-Objektes */
    /*8*/    int (*write_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, int o_pos);
                /* Funktionszeiger zum Oeffnen eines CAN-Karten-Treibers */
    /*8*/    int (*open_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);
               /* Funktionszeiger zum Schliessen des CAN-Karten-Treibers */
    /*8*/    int (*close_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);
               /* Funktionszeiger zum Lessen Status des CAN-Karten-Treibers */
    /*8*/    int (*status_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);

               /* Funktionszeiger zum Lessen Status des CAN-Karten-Treibers */
    /*8*/    int (*queue_read_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel,
                                    uint32_t *pid, unsigned char* data,
                                    unsigned char* pext, unsigned char* psize,
                                    uint64_t *pTimeStamp);
               /* Funktionszeiger zum Lessen Status des CAN-Karten-Treibers */
    /*8*/    int (*queue_write_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int Channel,
                                     uint32_t id, unsigned char* data,
                                     unsigned char ext, unsigned char size);
    /*8*/    int (*info_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, 
                              int Info, int MaxReturnSize, void *Return);
#else
    /*4*/    int (*read_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, int o_pos);
    /*4*/    int (*read_can_hdw) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, int o_pos);
                    /* Funktionszeiger fuer Schreiben eines CAN-Objektes */
    /*4*/    int (*write_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, int o_pos);
    /*4*/    int (*write_can_hdw) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, int o_pos);
                /* Funktionszeiger zum Oeffnen eines CAN-Karten-Treibers */
    /*4*/    int (*open_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);
    /*4*/    int (*open_can_hdw) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);
               /* Funktionszeiger zum Schliessen des CAN-Karten-Treibers */
    /*4*/    int (*close_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);
    /*4*/    int (*close_can_hdw) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);
               /* Funktionszeiger zum Lessen Status des CAN-Karten-Treibers */
    /*4*/    int (*status_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);
    /*4*/    int (*status_can_hdw) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel);

               /* Funktionszeiger zum Lessen Status des CAN-Karten-Treibers */
    /*4*/    int (*queue_read_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel,
                                    uint32_t *pid, unsigned char* data,
                                    unsigned char* pext, unsigned char* psize,
                                    uint64_t *pTimeStamp);
    /*4*/    int (*queue_read_can_hdw) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel,
                                    uint32_t *pid, unsigned char* data,
                                    unsigned char* pext, unsigned char* psize,
                                    uint64_t *pTimeStamp);
               /* Funktionszeiger zum Lessen Status des CAN-Karten-Treibers */
    /*4*/    int (*queue_write_can) (uint32_t CardTypeNrChannel,
                                     uint32_t id, unsigned char* data,
                                     unsigned char ext, unsigned char size);
    /*4*/    int (*queue_write_can_hdw) (uint32_t CardTypeNrChannel,
                                     uint32_t id, unsigned char* data,
                                     unsigned char ext, unsigned char size);
    /*4*/    int (*info_can) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, 
                              int Info, int MaxReturnSize, void *Return);
    /*4*/    int (*info_can_hdw) (struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, 
                              int Info, int MaxReturnSize, void *Return);
#endif
#define CAN_CHANNEL_INFO_MIXED_IDS_ALLOWED  1
#define CAN_CHANNEL_INFO_CAN_CARD_STRING    2
    /*4*/    int32_t object_count;
    /*2*/    int16_t rx_object_count;
    /*2*/    int16_t tx_object_count;
    /*256*2*/int16_t objects[MAX_OBJECTS_ONE_CHANNEL];     // max. 256 Objekte je Kanal
        // zur schnellern Suche der Objekte gibt es sortierte Arrays fuer TX und RX-Objekte.
    /*256*8*/NEW_CAN_HASH_ARRAY_ELEM hash_rx[MAX_RX_OBJECTS_ONE_CHANNEL]; // max. 256 Objekte je Kanal (enthaelt aufsteigend sortierte Liste der RX-Objekte Mux nur Master)
    /*256*8*/NEW_CAN_HASH_ARRAY_ELEM hash_tx[MAX_TX_OBJECTS_ONE_CHANNEL]; // max. 256 Objekte je Kanal (enthaelt aufsteigend sortierte Liste der TX-Objekte Mux nur Master)
    /*4*/    int32_t global_tx_enable_vid;
    /*4*/    int32_t fd_support;
    /*4*/    int32_t j1939_flag;
    /*4*/    uint8_t J1939_src_addr[J1939TP_MP_MAX_SRC_ADDRESSES];

    /*2*/    uint16_t j1939_rx_object_count;
    /*2*/    uint16_t fill1;
    /*2*/    uint32_t fill2;
    /*32*/   uint16_t j1939_rx_objects[J1939TP_MP_MAX_OBJECTS];

    /*64*4*/ sJ1939Tp_MP_RxChannel J1939Tp_MP_RxChannels[J1939TP_MP_CHANNELS_MAX];
    /*64*4*/ sJ1939Tp_MP_TxChannel J1939Tp_MP_TxChannels[J1939TP_MP_CHANNELS_MAX];

    /*4*/    int32_t VirtualNetworkId;
             int8_t filler[8192-5292];
    } channels[MAX_CAN_CHANNELS];            // max. 4 Kanaele    8192Bytes

    NEW_CAN_SERVER_OBJECT objects[MAX_OBJECTS_ALL_CHANNELS];       // max. 512 CAN-Objekte fuer alle Kanal  131072Bytes

    NEW_CAN_SERVER_SIGNAL bb_signals[MAX_SIGNALS_ALL_CHANNELS];       // max. 4096 Signale                    262144Bytes

    // Startadresse der CAN-Objekt Daten-Buffers
    // beinhaltet alle Puffer hintereinander
#ifdef _M_X64
    int8_t *PtrDataBuffer;
#else
    int8_t *PtrDataBuffer;
    int8_t *PtrDataBuffer_hdw;
#endif
    int32_t CanServerPid;
    int32_t SymSize;
    int32_t SymPos;
    char Symbols[1];  // nicht 1 sondern flexibel!!

}  NEW_CAN_SERVER_CONFIG;

#define GET_CAN_SIG_NAME(c, i) (&c->Symbols[c->bb_signals[i].NameIdx])
#define GET_CAN_SIG_UNIT(c, i) (&c->Symbols[c->bb_signals[i].UnitIdx])
#define GET_CAN_SIG_BYTECODE(c, i) (&c->Symbols[c->bb_signals[i].ConvIdx])
#define GET_CAN_EVENT_BYTECODE(c, i) (&c->Symbols[c->objects[i].EventIdx])
#define GET_CAN_CYCLES_BYTECODE(c, i) (&c->Symbols[c->objects[i].cycles])

#define GET_CAN_INIT_DATA_OBJECT(c, i) (&c->Symbols[c->objects[i].InitDataIdx])

#define GET_CAN_J1939_MULTI_C_PG_OBJECT(c, i) ((uint32_t*)(&c->Symbols[c->objects[i].Protocol.J1939.C_PGs_ArrayIdx]))



NEW_CAN_SERVER_CONFIG *LoadCanConfigAndStart (int par_Fd);
NEW_CAN_SERVER_CONFIG *ReadCanConfig (int par_Fd);
int detach_canserver_config (NEW_CAN_SERVER_CONFIG *csc);
int remove_canserver_config (NEW_CAN_SERVER_CONFIG *csc);
int AzgEdicReadConfigAndSend (char *IniFile);
int AzgEdicCheckSymbolSize (int AzgEdic, int Count, char **p);
int AzgEdicCalcSymbolsSize (int AzgEdic, int Count, char **p);
int AzgEdicSortSymbols (int Count, char **p);
int CountEdicCanDataMessage (void);

int TranslateString2Curve (char *txt, int maxc, int *pSize);
struct EXEC_STACK_ELEM *TranslateString2ByteCode (char *equ, int maxc, int *pSize, int *pUseCANData);

#define CAN_BIT_ERROR_MAX_SIZE  64
typedef struct {
    int32_t Command;
#define OVERWRITE_DATA_BYTES    1
#define CHANGE_DATA_LENGTH      2
#define SUSPEND_TRANSMITION     3
    int32_t Channel;
    int32_t Id;
    uint32_t Counter;
    int32_t Size;
    int32_t SizeSave;
    int32_t LastCANObjectPos;
    int32_t ByteOrder;
    uint8_t AndMask[CAN_BIT_ERROR_MAX_SIZE];
    uint8_t OrMask[CAN_BIT_ERROR_MAX_SIZE];
}  CAN_BIT_ERROR;

typedef struct {
    int32_t SizeOfStruct;
    int32_t Channel;
    int32_t Id;
    int32_t UseCANData;
    int32_t IdxOfSignalName;
    int32_t LenOfSignalName;
    int32_t IdxOfExecStack;
    int32_t LenOfExecStack;
    char Data[1];   // This ar normaly more than 1 byte
}  CAN_SET_SIG_CONV;

// If par_Conversion == NULL reset the conversion
// If par_Conversion == NULL and par_SignalName == NULL reset all conversions.

int SendAddOrRemoveReplaceCanSigConvReq (int par_Channel, int par_Id, char *par_SignalName, struct EXEC_STACK_ELEM *par_ExecStack, int par_UseCANData);

#ifndef __GNUC__
#pragma pack ()
#endif

#endif

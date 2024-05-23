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


#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#include "tcb.h"

/* ------------------------------------------------------------ *\
 *    Global defines  					                     	*
\* ------------------------------------------------------------ */

/* Fehler */
#define MESSAGE_SIZE_ERROR  -501
#define MESSAGE_PID_ERROR   -502
#define MESSAGE_OVERFLOW    -503
#define NO_MESSAGE_QUEUE    -504

/* Alle Message-Typen */
/* Message-IDs */

#define ADD_BBVARI_READ_INI           18300
#define BBVARI_WRITE_INI              18301

// Generator
#define RAMPEN_START    1230
#define RAMPEN_STOP     1231
#define RAMPEN_FILENAME 1232
#define RAMPEN_ACK      1233
#define RAMPEN_ANZAHL_STUETZSTELLEN               1234
#define RAMPEN_ZEIT_STUETZSTELLE                  1235
#define RAMPEN_AMPL_STUETZSTELLE                  1236
#define RAMPEN_ZEIT_EXEC_STACK_STUETZSTELLE       1237
#define RAMPEN_AMPL_EXEC_STACK_STUETZSTELLE       1238
#define RAMPEN_VIDS                               1239  

// HD-Play
#define HDPLAY_FILENAME_MESSAGE         5001
#define HDPLAY_START_MESSAGE            5002
#define HDPLAY_STOP_MESSAGE             5003
#define HDPLAY_CONFIG_FILENAME_MESSAGE  5004
#define HDPLAY_TRIGGER_MESSAGE          5005
#define HDPLAY_ACK_MESSAGE              5006
#define HDPLAY_ACK                      5007
// WR-Pipe
#define PLAY_DATA_MESSAGE          4001
#define SEND_VIDS_MESSAGE          4002
#define START_WRPIPE_MESSAGE       4003
#define STOP_WRPIPE_MESSAGE        4004
#define TRIGGER_INFO_MESSAGE       4005
#define WRPIPE_ACK                 4006
#define PLAY_DATA_MESSAGE_PING     4007
#define PLAY_NO_MORE_DATA_MESSAGE  4008
// RD-Pipe
#define RDPIPE_LOGON         301
#define RDPIPE_LOGOFF        302
#define RDPIPE_SEND_TRIGGER  303
#define RDPIPE_SEND_WRFLAG   304
#define RDPIPE_SEND_LENGTH   305
#define RDPIPE_ACK           306
#define RDPIPE_DEC_PHYS_MASK 307
// HD-Rec
#define RECORD_MESSAGE      901
#define INIT_REC_MESSAGE    902
#define START_REC_MESSAGE   903
#define VARI_REC_MESSAGE    904
#define STOP_REC_MESSAGE    905
#define REC_CONFIG_FILENAME_MESSAGE 906
#define HDREC_ACK           907
#define HDREC_COMMENT_MESSAGE  908
// Trigger
#define TRIGGER_FILENAME_MESSAGE  22000
#define TRIGGER_STOP_MESSAGE      22001
#define TRIGGER_ACK               22002
#define TRIGGER_EUATIONCONTER_MESSAGE 22003
#define TRIGGER_EUATIONS_MESSAGE      22004 
#define TRIGGER_DONT_ADD_NOT_EXITING_VARS   22005
#define TRIGGER_START_MESSAGE     22006
#define TRIGGER_ERROR_MESSAGE     22007

#define AZG_READ_ERROR_MEM_ACK        1321

#define NEW_CANSERVER_INIT            1324
#define NEW_CANSERVER_STOP            1325

#define IS_NEW_CANSERVER_RUNNING      0xFFFF0
#define NEW_CANSERVER_SET_BIT_ERR     0xAAAA
#define NEW_CANSERVER_RESET_BIT_ERR   0xAAAB
#define NEW_CANSERVER_SET_SIG_CONV    1326

#define FLEXRAY_SERVER_INIT            2324
#define FLEXRAY_SERVER_STOP            2325

#define IS_FLEXRAY_SERVER_RUNNING      2326
#define FLEXRAY_SERVER_SET_BIT_ERR     2327
#define FLEXRAY_SERVER_SET_SIG_CONV    2328

// RT-File
#define RT_FOPEN                      1316 
#define RT_FOPEN_ACK                  1317 

#define MESSAGE_THROUGH_FIFO_MASK              0x10000000
#define IMPORTANT_MESSAGE_THROUGH_FIFO_MASK    0x30000000

#define MESSAGE_MAX_SIZE         (1024*1024)
#define MESSAGE_QUEUE_MAX_SIZE   (4*1024*1024)
#define MESSAGE_QUEUE_MIN_SIZE   (64*1024)


// begin REMOTE_MASTER

typedef struct {
    int32_t NumOfCards;                  // 1 ... 16
    int32_t NumOfChannels;
    struct {
        int32_t CardType;
#define CAN_CARD_TYPE_UNKNOWN         0
#define CAN_CARD_TYPE_VIRTUAL         5
#define CAN_CARD_TYPE_SOCKETCAN       8
        char CANCardString[64];
        int32_t State;
        int32_t NumOfChannels;
    } Cards[16];     // must be same as MAX_CAN_CHANNELS
} CAN_CARD_INFOS;
#define GET_CAN_CARD_INFOS            1339
#define GET_CAN_CARD_INFOS_ACK        1340

#define NEW_CAN_INI                   18308
#define NEW_CAN_ATTACH_CFG_ACK        18312
#define DSP56301_INI                  18309

#if 0
/ lagacy things
typedef struct {
    int32_t NumOfCards;                  // 1, 2, 3 or 4
    int32_t Fill1;
    struct {
        int32_t CardType;
#define FLEXCARD_TYPE_UNKNOWN         0
#define FLEXCARD_TYPE_PROTOTYPE       1
        char CardString[64];
        int32_t State;
        int32_t NumOfChannels;
        int32_t FirmwareVersion;
        int32_t HardwareVersion;
        int32_t FeatureSwitchs;
    } Cards[4];
} FLEXCARD_INFOS;
#define GET_FLEXCARD_INFOS             2339
#define GET_FLEXCARD_INFOS_ACK         2340

#define FLEXRAY_ATTACH_CFG_ACK         2341

#define FLEXRAY_INI                    2342   

#define FLEXCARD_DIGIO_CFG             2343
#define FLEXCARD_DIGIO_CFG_ACK         2244
#define FLEXCARD_DIGIO_CFG_REQ         2245
#define FLEXCARD_DIGIO_CFG_CHANGED     2246
#endif

// end REMOTE_MASTER


// Message head
typedef struct {
    int32_t pid;         /* Process-ID of the transmitter  */
    int32_t mid;         /* Message-ID (type of the message) */
    uint64_t timestamp;  /* Timestamp of the message (when dispatched) */
    int32_t size;        /* Size of the message  */
} MESSAGE_HEAD;          /* (20 bytes) */

MESSAGE_BUFFER *build_message_queue (int size);

void remove_message_queue (MESSAGE_BUFFER *message_buffer);

int write_message_ts_as (int pid, int mid, uint64_t timestamp, int msize, const char *mblock, int tx_pid);
int write_message_ts (int pid, int mid, uint64_t timestamp, int msize, const char *mblock);
int write_message_as (int pid, int mid, int msize, const char *mblock, int tx_pid);
int write_message (int pid, int mid, int msize, const char *mblock);

int write_important_message_ts_as (int pid, int mid, uint64_t timestamp, int msize, const char *mblock, int tx_pid);
int write_important_message (int pid, int mid, int msize, const char *mblock);

int read_message (MESSAGE_HEAD *mhead, char *message, int maxlen);

int test_message (MESSAGE_HEAD *mhead);

int remove_message (void);


#endif

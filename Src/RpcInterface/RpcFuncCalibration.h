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


#ifndef RPCFUNCCALIBRATION_H
#define RPCFUNCCALIBRATION_H

#include <stdint.h>

#if defined SCRPCDLL_EXPORTS
#include "XilEnvRpc.h"        // if included from RPC DLL/library use this header
#else
#ifndef SC2PY_EXPORTS
#include "Blackboard.h"       // otherwise this
#endif
#endif

#include "RpcFuncBase.h"

#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct on
#endif
#endif
#pragma pack(push,1)

#define CALIBRATION_CMD_OFFSET  300  /* ...339 */


#define RPC_API_LOAD_SVL_CMD         (CALIBRATION_CMD_OFFSET+0)
typedef struct {
#define RPC_API_LOAD_SVL_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSvlFilename;\
    int32_t OffsetProcess;\
    char Data[1];  // > 1Byte!
    RPC_API_LOAD_SVL_MESSAGE_MEMBERS
} RPC_API_LOAD_SVL_MESSAGE;

typedef struct {
#define RPC_API_LOAD_SVL_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_LOAD_SVL_MESSAGE_ACK_MEMBERS
} RPC_API_LOAD_SVL_MESSAGE_ACK;

#define RPC_API_SAVE_SVL_CMD         (CALIBRATION_CMD_OFFSET+1)
typedef struct {
#define RPC_API_SAVE_SVL_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSvlFilename;\
    int32_t OffsetProcess;\
    int32_t OffsetFilter;\
    char Data[1];  // > 1Byte!
    RPC_API_SAVE_SVL_MESSAGE_MEMBERS
} RPC_API_SAVE_SVL_MESSAGE;

typedef struct {
#define RPC_API_SAVE_SVL_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_SAVE_SVL_MESSAGE_ACK_MEMBERS
} RPC_API_SAVE_SVL_MESSAGE_ACK;

#define RPC_API_SAVE_SAL_CMD         (CALIBRATION_CMD_OFFSET+2)
typedef struct {
#define RPC_API_SAVE_SAL_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSalFilename;\
    int32_t OffsetProcess;\
    int32_t OffsetFilter;\
    char Data[1];  // > 1Byte!
    RPC_API_SAVE_SAL_MESSAGE_MEMBERS
} RPC_API_SAVE_SAL_MESSAGE;

typedef struct {
#define RPC_API_SAVE_SAL_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_SAVE_SAL_MESSAGE_ACK_MEMBERS
} RPC_API_SAVE_SAL_MESSAGE_ACK;

#define RPC_API_GET_SYMBOL_RAW_CMD         (CALIBRATION_CMD_OFFSET+5)
typedef struct {
#define RPC_API_GET_SYMBOL_RAW_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSymbol;\
    int32_t OffsetProcess;\
    int32_t Flags;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_SYMBOL_RAW_MESSAGE_MEMBERS
} RPC_API_GET_SYMBOL_RAW_MESSAGE;

typedef struct {
#define RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header; \
    union BB_VARI Value; 
    RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK_MEMBERS
} RPC_API_GET_SYMBOL_RAW_MESSAGE_ACK;

#define RPC_API_SET_SYMBOL_RAW_CMD         (CALIBRATION_CMD_OFFSET+6)
typedef struct {
#define RPC_API_SET_SYMBOL_RAW_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetSymbol;\
    int32_t OffsetProcess;\
    int Flags;\
    int DataType;\
    union BB_VARI Value;\
    char Data[1];  // > 1Byte!
    RPC_API_SET_SYMBOL_RAW_MESSAGE_MEMBERS
} RPC_API_SET_SYMBOL_RAW_MESSAGE;

typedef struct {
#define RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header; \
    union BB_VARI Value; 
    RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK_MEMBERS
} RPC_API_SET_SYMBOL_RAW_MESSAGE_ACK;

// A2L Links

#define RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_CMD         (CALIBRATION_CMD_OFFSET+10)
typedef struct {
#define RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetA2LFileName;\
    int32_t OffsetProcessName;\
    int32_t UpdateFlag;\
    char Data[1];  // > 1Byte!
    RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_MEMBERS
} RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE;

typedef struct {
#define PC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    PC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK_MEMBERS
} RPC_API_SETUP_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK;


#define RPC_API_GET_LINK_TO_EXTERN_PROCESS_CMD         (CALIBRATION_CMD_OFFSET+11)
typedef struct {
#define RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetProcessName;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_MEMBERS
} RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE;

typedef struct {
#define PC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    PC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK_MEMBERS
} RPC_API_GET_LINK_TO_EXTERN_PROCESS_MESSAGE_ACK;


#define RPC_API_GET_INDEX_FROM_LINK_CMD         (CALIBRATION_CMD_OFFSET+20)
typedef struct {
#define RPC_API_GET_INDEX_FROM_LINK_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t LinkNr;\
    int32_t OffsetLabel;\
    int32_t TypeMask;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_INDEX_FROM_LINK_MESSAGE_MEMBERS
} RPC_API_GET_INDEX_FROM_LINK_MESSAGE;

typedef struct {
#define RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
   RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK_MEMBERS
} RPC_API_GET_INDEX_FROM_LINK_MESSAGE_ACK;

#define RPC_API_GET_NEXT_SYMBOL_FROM_LINK_CMD         (CALIBRATION_CMD_OFFSET+21)
typedef struct {
#define RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t LinkNr;\
    int32_t Index;\
    int32_t TypeMask;\
    int32_t OffsetFilter;\
    int32_t MaxChar;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_MEMBERS
} RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE;

typedef struct {
#define RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    int32_t OffsetRetName;\
    char Data[1];  // > 1Byte!
   RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK_MEMBERS
} RPC_API_GET_NEXT_SYMBOL_FROM_LINK_MESSAGE_ACK;

#define RPC_API_GET_DATA_FROM_LINK_CMD         (CALIBRATION_CMD_OFFSET+22)
typedef struct {
#define RPC_API_GET_DATA_FROM_LINK_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t LinkNr;\
    int32_t Index;\
    int32_t PhysFlag;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_DATA_FROM_LINK_MESSAGE_MEMBERS
} RPC_API_GET_DATA_FROM_LINK_MESSAGE;

typedef struct {
#define RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    int32_t OffsetData;\
    int32_t OffsetRetError;\
    char Data[1];  // > 1Byte!
   RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK_MEMBERS
} RPC_API_GET_DATA_FROM_LINK_MESSAGE_ACK;

#define RPC_API_SET_DATA_TO_LINK_CMD         (CALIBRATION_CMD_OFFSET+23)
typedef struct {
#define RPC_API_SET_DATA_TO_LINK_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t LinkNr;\
    int32_t Index;\
    int32_t OffsetData;\
    char Data[1];  // > 1Byte!
    RPC_API_SET_DATA_TO_LINK_MESSAGE_MEMBERS
} RPC_API_SET_DATA_TO_LINK_MESSAGE;

typedef struct {
#define RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    int32_t OffsetRetError;\
    char Data[1];  // > 1Byte!
   RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK_MEMBERS
} RPC_API_SET_DATA_TO_LINK_MESSAGE_ACK;

#define RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_CMD         (CALIBRATION_CMD_OFFSET+24)
typedef struct {
#define RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t LinkNr;\
    int32_t Index;\
    int32_t DirFlags;
    RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_MEMBERS
} RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE;

typedef struct {
#define RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
   RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK_MEMBERS
} RPC_API_REFERENCE_MEASUREMENT_TO_BLACKBOARD_MESSAGE_ACK;

#define RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_CMD         (CALIBRATION_CMD_OFFSET+25)
typedef struct {
#define RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t LinkNr;\
    int32_t Index;
    RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_MEMBERS
} RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE;

typedef struct {
#define RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
   RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK_MEMBERS
} RPC_API_DEREFERENCE_MEASUREMENT_FROM_BLACKBOARD_MESSAGE_ACK;

#pragma pack(pop)
#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct reset
#endif
#endif
int AddCalibrationFunctionToTable(void);


#endif // RPCFUNCCALIBRATION_H

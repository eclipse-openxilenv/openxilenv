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
#include "Platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include "A2LBuffer.h"
#include "A2LTokenizer.h"
#include "A2LParser.h"

//#define DEBUG_PRINTF

#define UNUSED(x) (void)(x)

#define CHECK_SET_FLAG_BIT_POS(f,b)  if (((f) & (1ULL << (b))) == (1ULL << (b))) {ThrowParserError (Parser, __FILE__, __LINE__, "\"%s\" already defined", KeywordTableEntry->KeywordString);} else (f) |= (1ULL << (b));


#define ENUMDEF_BYTE_ORDER "BYTE_ORDER_MSB_LAST 0 BYTE_ORDER_MSB_FIRST 1 ? -1"
#define ENUMDEF_ADDRESS_GRANULARITY "ADDRESS_GRANULARITY_BYTE 1 ADDRESS_GRANULARITY_WORD 2 ADDRESS_GRANULARITY_DWORD 4 ? -1"

#define ENUMDEF_SAMPLE_RATE "SINGLE 1 TRIPLE 3 ? -1"
#define ENUMDEF_SYNC_EDGE "SINGLE 1 DUAL 2 ? -1"
#define ENUMDEF_VAR_OR_FIXED "VARIABLE 0 FIXED 1 ? -1"

#define ENUMDEF_SAMPLE_RATE "SINGLE 1 TRIPLE 3 ? -1"
#define ENUMDEF_SYNC_EDGE "SINGLE 1 DUAL 2 ? -1"
#define ENUMDEF_DAQ_LIST_CAN_ID "VARIABLE 0 FIXED 1 & -1"

int ParseIfDataXcpProtocolLayerCommunicationModeSupportedBlockSlave(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(KeywordTableEntry);
    UNUSED(HasBeginKeyWord);
    return CheckParserError(Parser);
}

int ParseIfDataXcpProtocolLayerCommunicationModeSupportedBlockMaster(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(KeywordTableEntry);
    UNUSED(HasBeginKeyWord);
    ReadUIntFromFile(Parser);  // will be ignored
    ReadUIntFromFile(Parser);  // will be ignored
    return CheckParserError(Parser);
}


#define TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED_BLOCK_SLAVE     1
#define TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED_BLOCK_MASTER    2

static ASAP2_KEYWORDS IfDataXcpProtocolLayerCommunicationModeSupportBlockKeywordTable[] =
{ {0, "SLAVE",         0, TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED_BLOCK_SLAVE,    ParseIfDataXcpProtocolLayerCommunicationModeSupportedBlockSlave, ""},
  {-1, "MASTER",       2, TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED_BLOCK_MASTER,   ParseIfDataXcpProtocolLayerCommunicationModeSupportedBlockMaster, ""},
  {0, NULL,            0, -1,                                                                           NULL, ""} };

int ParseIfDataXcpProtocolLayerCommunicationModeSupportedBlock(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    ASAP2_IF_DATA_XCP_PROTOCOL_LAYER *ProtocolLayer = (ASAP2_IF_DATA_XCP_PROTOCOL_LAYER*)Data;
    if CheckParserError(Parser) return -1;

    // parse all optionale parameters
    while (GetNextOptionalParameter(Parser, IfDataXcpProtocolLayerCommunicationModeSupportBlockKeywordTable, (void*)ProtocolLayer) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
    return CheckParserError(Parser);
}

int ParseIfDataXcpProtocolLayerCommunicationModeSupportedInterleaved(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(KeywordTableEntry);
    UNUSED(HasBeginKeyWord);
    return CheckParserError(Parser);
}


#define TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED_BLOCK          1
#define TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED_INTERLEAVED    2

static ASAP2_KEYWORDS IfDataXcpProtocolLayerCommunicationModeSupportKeywordTable[] =
{ {-1, "BLOCK",         1, TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED_BLOCK,         ParseIfDataXcpProtocolLayerCommunicationModeSupportedBlock, ""},
  {0, "INTERLEAVED",    1, TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED_INTERLEAVED,   ParseIfDataXcpProtocolLayerCommunicationModeSupportedInterleaved, ""},
  {0, NULL,             0, -1,                                                                NULL, ""} };


int ParseIfDataXcpProtocolLayerCommunicationModeSupport(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    ASAP2_IF_DATA_XCP_PROTOCOL_LAYER *ProtocolLayer = (ASAP2_IF_DATA_XCP_PROTOCOL_LAYER*)Data;
    if CheckParserError(Parser) return -1;

    // parse all optionale parameters
    while (GetNextOptionalParameter(Parser, IfDataXcpProtocolLayerCommunicationModeSupportKeywordTable, (void*)ProtocolLayer) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

int ParseIfDataXcpProtocolLayerOptinalCmd(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(KeywordTableEntry);
    UNUSED(HasBeginKeyWord);
    ReadKeyWordFromFile(Parser);  //will be ignored!
    return CheckParserError(Parser);
}

int ParseIfDataXcpProtocolLayerSeedAndKeyDll(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_PROTOCOL_LAYER *ProtocolLayer = (ASAP2_IF_DATA_XCP_PROTOCOL_LAYER*)Data;
    CHECK_SET_FLAG_BIT_POS(ProtocolLayer->OptionalParameter.Flags,
                           OPTPARAM_IF_DATA_XCP_PROTOCOL_LAYER_SEED_AND_KEY_EXTERNAL_FUNCTION);
    ProtocolLayer->OptionalParameter.SeedAndKeyDll = ReadStringFromFile_Buffered(Parser);  
    return CheckParserError(Parser);
}

#define TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_OPTIONAL_CMD                    1
#define TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_SEED_AND_KEY_EXTERNAL_FUNCTION  2
#define TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED    3

static ASAP2_KEYWORDS IfDataXcpProtocolLayerKeywordTable[] =
{ {0, "OPTIONAL_CMD",                     1, TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_OPTIONAL_CMD,                     ParseIfDataXcpProtocolLayerOptinalCmd, ""},
  {0, "SEED_AND_KEY_EXTERNAL_FUNCTION",   1, TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_SEED_AND_KEY_EXTERNAL_FUNCTION,   ParseIfDataXcpProtocolLayerSeedAndKeyDll, ""},
  {-1, "COMMUNICATION_MODE_SUPPORTED",    1, TOKEN_IF_DATA_XCP_PROTOCOL_LAYER_COMMUNICATION_MODE_SUPPORTED,     ParseIfDataXcpProtocolLayerCommunicationModeSupport, ""},
  {0, NULL,                               0, -1,                                                                NULL, ""} };


int ParseIfDataXcpProtocolLayer(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    ASAP2_IF_DATA_XCP *Xcp = (ASAP2_IF_DATA_XCP*)Data;
    if CheckParserError(Parser) return -1;
    CHECK_SET_FLAG_BIT_POS(Xcp->OptionalParameter.Flags,
                           OPTPARAM_IF_DATA_XCP_PROTOCOL_LAYER);

    // 1. parse all fixed parameters
    Xcp->OptionalParameter.ProtocolLayer.ProtocolVersion = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.T1 = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.T2 = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.T3 = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.T4 = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.T5 = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.T6 = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.T7 = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.MAX_CTO = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.MAX_DTO = ReadUIntFromFile(Parser);
    Xcp->OptionalParameter.ProtocolLayer.ByteOrder = ReadEnumFromFile(Parser, ENUMDEF_BYTE_ORDER);
    Xcp->OptionalParameter.ProtocolLayer.AddressGranularity = ReadEnumFromFile(Parser, ENUMDEF_ADDRESS_GRANULARITY);
    if CheckParserError(Parser) return -1;

    // 2. parse all optionale parameters
    while (GetNextOptionalParameter(Parser, IfDataXcpProtocolLayerKeywordTable, (void*)&(Xcp->OptionalParameter.ProtocolLayer)) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}


// XCP_ON_CAN

int ParseIfDataXcpOnCanIdBroadcast(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_ID_BROADCAST);
    XcpOnCan->OptionalParameter.CanIdBroadcast = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanIdMaster(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_ID_MASTER);
    XcpOnCan->OptionalParameter.CanIdMaster = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanIdSlave(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_ID_SLAVE);
    XcpOnCan->OptionalParameter.CanIdSlave = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanBaudrate(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_BAUDRATE);
    XcpOnCan->OptionalParameter.Baudrate = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanSamplePoint(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_SAMPLE_POINT);
    XcpOnCan->OptionalParameter.SamplePoint = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanSampleRate(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_SAMPLE_RATE);
    XcpOnCan->OptionalParameter.SamplePoint = ReadEnumFromFile(Parser, ENUMDEF_SAMPLE_RATE);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanBtlCycles(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_BTL_CYCLES);
    XcpOnCan->OptionalParameter.BtlCycles = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanSjw(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_SJW);
    XcpOnCan->OptionalParameter.Sjw = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanSyncEdge(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_SYNC_EDGE);
    XcpOnCan->OptionalParameter.SyncEdge = ReadEnumFromFile(Parser, ENUMDEF_SYNC_EDGE);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanMaxDlcRequired(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_MAX_DLC_REQUIRED);
    XcpOnCan->OptionalParameter.MaxDlcRequired = 1;
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanMaxBusLoad(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;
    CHECK_SET_FLAG_BIT_POS(XcpOnCan->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_XCP_ON_CAN_MAX_BUS_LOAD);
    XcpOnCan->OptionalParameter.MaxBusLoad = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseIfDataXcpOnCanDaqListCanId(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_XCP_ON_CAN *XcpOnCan = (ASAP2_IF_DATA_XCP_ON_CAN*)Data;

    if (XcpOnCan->OptionalParameter.NumberOfDaqLists >= DAQ_LIST_MAX_NUMBER) {
        ThrowParserError(Parser, __FILE__, __LINE__, "too many DAQ_LIST_CAN_ID entrys only %s allowed", DAQ_LIST_MAX_NUMBER);
        return -1;
    }
    XcpOnCan->OptionalParameter.DaqLists[XcpOnCan->OptionalParameter.NumberOfDaqLists].DaqListNr = ReadUIntFromFile(Parser);
    switch (XcpOnCan->OptionalParameter.DaqLists[XcpOnCan->OptionalParameter.NumberOfDaqLists].VarOrFixed = ReadEnumFromFile(Parser, ENUMDEF_DAQ_LIST_CAN_ID)) {
    case 0:  // VARIABLE
        break;
    case 1:  // FIXED
        XcpOnCan->OptionalParameter.DaqLists[XcpOnCan->OptionalParameter.NumberOfDaqLists].FixedValue = ReadUIntFromFile(Parser);
        break;
    default:
        ThrowParserError(Parser, __FILE__, __LINE__, "only FIXED or VARIABLE are allowed");
        return -1;
    }
    XcpOnCan->OptionalParameter.NumberOfDaqLists++;
    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

#define TOKEN_IF_DATA_XCP_ON_CAN_ID_BROADCAST                    1
#define TOKEN_IF_DATA_XCP_ON_CAN_ID_MASTER                       2
#define TOKEN_IF_DATA_XCP_ON_CAN_ID_SLAVE                        3
#define TOKEN_IF_DATA_XCP_ON_CAN_BAUDRATE                        4
#define TOKEN_IF_DATA_XCP_ON_CAN_SAMPLE_POINT                    5
#define TOKEN_IF_DATA_XCP_ON_CAN_SAMPLE_RATE                     6
#define TOKEN_IF_DATA_XCP_ON_CAN_BTL_CYCLES                      7
#define TOKEN_IF_DATA_XCP_ON_CAN_SJW                             8
#define TOKEN_IF_DATA_XCP_ON_CAN_SYNC_EDGE                       9
#define TOKEN_IF_DATA_XCP_ON_CAN_MAX_DLC_REQUIRED               10
#define TOKEN_IF_DATA_XCP_ON_CAN_MAX_BUS_LOAD                   11
#define TOKEN_IF_DATA_XCP_ON_CAN_DAQ_LIST_CAN_ID                12
#define TOKEN_IF_DATA_XCP_ON_CAN_FD                             13
#define TOKEN_IF_DATA_XCP_ON_CAN_PROTOCOL_LAYER                 14
#define TOKEN_IF_DATA_XCP_ON_CAN_DAQ                            15
#define TOKEN_IF_DATA_XCP_ON_CAN_PAG                            16
#define TOKEN_IF_DATA_XCP_ON_CAN_PGM                            17

static ASAP2_KEYWORDS IfDataXcpOnCanKeywordTable[] =
{ {0, "CAN_ID_BROADCAST",        1, TOKEN_IF_DATA_XCP_ON_CAN_ID_BROADCAST,                 ParseIfDataXcpOnCanIdBroadcast, ""},
  {0, "CAN_ID_MASTER",           1, TOKEN_IF_DATA_XCP_ON_CAN_ID_MASTER,                    ParseIfDataXcpOnCanIdMaster, ""},
  {0, "CAN_ID_SLAVE",            1, TOKEN_IF_DATA_XCP_ON_CAN_ID_SLAVE,                     ParseIfDataXcpOnCanIdSlave, ""},
  {0, "BAUDRATE",                1, TOKEN_IF_DATA_XCP_ON_CAN_BAUDRATE,                     ParseIfDataXcpOnCanBaudrate, ""},
  {0, "SAMPLE_POINT",            1, TOKEN_IF_DATA_XCP_ON_CAN_SAMPLE_POINT,                 ParseIfDataXcpOnCanSamplePoint, ""},
  {0, "SAMPLE_RATE",             1, TOKEN_IF_DATA_XCP_ON_CAN_SAMPLE_RATE,                  ParseIfDataXcpOnCanSampleRate, ""},
  {0, "BTL_CYCLES",              1, TOKEN_IF_DATA_XCP_ON_CAN_BTL_CYCLES,                   ParseIfDataXcpOnCanBtlCycles, ""},
  {0, "SJW",                     1, TOKEN_IF_DATA_XCP_ON_CAN_SJW,                          ParseIfDataXcpOnCanSjw, ""},
  {0, "SYNC_EDGE",               1, TOKEN_IF_DATA_XCP_ON_CAN_SYNC_EDGE,                    ParseIfDataXcpOnCanSyncEdge, ""},
  {0, "MAX_DLC_REQUIRED",        0, TOKEN_IF_DATA_XCP_ON_CAN_MAX_DLC_REQUIRED,             ParseIfDataXcpOnCanMaxDlcRequired, ""},
  {0, "MAX_BUS_LOAD",            1, TOKEN_IF_DATA_XCP_ON_CAN_MAX_BUS_LOAD,                 ParseIfDataXcpOnCanMaxBusLoad, ""},
  {1, "DAQ_LIST_CAN_ID",        -1, TOKEN_IF_DATA_XCP_ON_CAN_DAQ_LIST_CAN_ID,              ParseIfDataXcpOnCanDaqListCanId, ""},
  {1, "CAN_FD",                 -1, TOKEN_IF_DATA_XCP_ON_CAN_FD,                           IgnoreBlock, ""},
  {1, "PROTOCOL_LAYER",          8, TOKEN_IF_DATA_XCP_ON_CAN_PROTOCOL_LAYER,               IgnoreBlock, ""},    // this is normally insied IF_DATA XCP!
  {1, "DAQ",                    -1, TOKEN_IF_DATA_XCP_ON_CAN_DAQ,                          IgnoreBlock, ""},    // this is normally insied IF_DATA XCP!
  {1, "PAG",                    -1, TOKEN_IF_DATA_XCP_ON_CAN_PAG,                          IgnoreBlock, ""},    // this is normally insied IF_DATA XCP!
  {1, "PGM",                    -1, TOKEN_IF_DATA_XCP_ON_CAN_PGM,                          IgnoreBlock, ""},    // this is normally insied IF_DATA XCP!
  {0, NULL,                      0, -1,                                                    NULL, ""} };

int ParseIfDataXcpOnCan(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    ASAP2_IF_DATA_XCP *Xcp = (ASAP2_IF_DATA_XCP*)Data;
    if CheckParserError(Parser) return -1;
    CHECK_SET_FLAG_BIT_POS(Xcp->OptionalParameter.Flags,
                           OPTPARAM_IF_DATA_XCP_ON_CAN);

    // 1. parse all fixed parameters
    Xcp->OptionalParameter.OnCan.Version = ReadUIntFromFile(Parser);
    if CheckParserError(Parser) return -1;

    // 2. parse all optionale parameters
    while (GetNextOptionalParameter(Parser, IfDataXcpOnCanKeywordTable, (void*)&(Xcp->OptionalParameter.OnCan)) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}


#define TOKEN_IF_DATA_XCP_PROTOCOL_LAYER                1
#define TOKEN_IF_DATA_XCP_PAG                           2
#define TOKEN_IF_DATA_XCP_DAQ                           3
#define TOKEN_IF_DATA_XCP_ON_FLX                        4
#define TOKEN_IF_DATA_XCP_ON_CAN                        5
#define TOKEN_IF_DATA_XCP_ON_TCP_IP                     6
#define TOKEN_IF_DATA_XCP_ON_UDP_IP                     7

static ASAP2_KEYWORDS IfDataXcpKeywordTable[] =
{ {1, "PROTOCOL_LAYER",          8, TOKEN_IF_DATA_XCP_PROTOCOL_LAYER,        ParseIfDataXcpProtocolLayer, ""},
  {1, "PAG",                    -1, TOKEN_IF_DATA_XCP_PAG,                   IgnoreBlock, ""},
  {1, "PGM",                    -1, TOKEN_IF_DATA_XCP_PAG,                   IgnoreBlock, ""},
  {1, "DAQ",                    -1, TOKEN_IF_DATA_XCP_DAQ,                   IgnoreBlock, ""},
  {1, "XCP_ON_FLX",             -1, TOKEN_IF_DATA_XCP_ON_FLX,                IgnoreBlock, ""},
  {1, "XCP_ON_CAN",             -1, TOKEN_IF_DATA_XCP_ON_CAN,                ParseIfDataXcpOnCan, ""},
  {1, "XCP_ON_TCP_IP",          -1, TOKEN_IF_DATA_XCP_ON_TCP_IP,             IgnoreBlock, ""},
  {1, "XCP_ON_UDP_IP",          -1, TOKEN_IF_DATA_XCP_ON_UDP_IP,             IgnoreBlock, ""},
  {0, NULL,                  0, -1,                                          NULL, ""} };

int ParseModuleIfDataBlock(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    char *Help = ReadKeyWordFromFile(Parser);
    if (Help == NULL) return -1;
    if (((Parser->Flags & A2L_PARSER_TRY_TO_LOAD_IF_DATA_XCP) == A2L_PARSER_TRY_TO_LOAD_IF_DATA_XCP) &&
        (!strcmp(Help, "XCP"))) {
        ASAP2_MODULE_DATA *Module = (ASAP2_MODULE_DATA*)Data;
        if CheckParserError(Parser) return -1;

        if (Module->IfDataXcpCounter) {
            ThrowParserError(Parser, __FILE__, __LINE__, "only one IF_DATA XCP allowed");
            return -1;
        }
        Module->IfDataXcpCounter++;
        // 2. parse optional parameter
        while (GetNextOptionalParameter(Parser, IfDataXcpKeywordTable, (void*)&(Module->IfDataXcp)) == 1);

        if CheckParserError(Parser) return -1;

        if (CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord)) return -1;
    } else {
        return IgnoreIfDataBlock(Parser);
    }
    return CheckParserError(Parser);
}


#include <stdint.h>
#include "ReadCanCfg.h"
#include "CanServer.h"
#include "Scheduler.h"

#include "J1939MultiPackage.h"

#define CONTROL_BYTE_TP_CM_RTS            0x10
#define CONTROL_BYTE_TP_CM_CTS            0x11
#define CONTROL_BYTE_TP_CM_BAM            0x20
#define CONTROL_BYTE_TP_CM_END_OF_MSG_ACK 0x13
#define CONTROL_BYTE_TP_CM_ABORT          0xFF

#define PDU_FORMAT_1   1
#define PDU_FORMAT_2   2

#define ABORT_REASON_ALREADY_IN_USE     0x01
#define ABORT_REASON_NO_RESOURCE        0x02
#define ABORT_REASON_TIMEOUT            0x03
#define ABORT_REASON_ERROR              0xFE

#define TIMEOUT_T1    750
#define TIMEOUT_T2   1250
#define TIMEOUT_T3   1250
#define TIMEOUT_T4   1050
#define TIMEOUT_Tr    200
#define TIMEOUT_Th    500
#define TIME_BETWEEN_TWO_DATA_PACKAGE_BAM 50
#define TIME_BETWEEN_TWO_DATA_PACKAGE_CTS 10

//#define DEBUG_LOGGING

#ifdef DEBUG_LOGGING
static FILE *FileHandle;

static void DebugLogging(NEW_CAN_SERVER_CONFIG *CanServerConfig, int par_Channel, int par_ObjPos,
                         uint32_t par_Id, const uint8_t *par_Data, const char *par_Text)
{
    if (FileHandle == NULL) {
        FileHandle = fopen("c:\\temp\\j1939.log", "wt");
    }
    if (FileHandle != NULL) {
        uint64_t Cycle = GetCycleCounter64();
        if (par_Data != NULL) {
            int x;
            fprintf (FileHandle, "%llu: %s (%i/%s) 0x%X:  ", Cycle, par_Text, par_Channel, GET_CAN_OBJECT_NAME(CanServerConfig, par_ObjPos), par_Id);
            for (x = 0; x < 8; x++) {
                fprintf (FileHandle, "%02X ", par_Data[x]);
            }
            fprintf (FileHandle, "\n");
        } else {
            int c;
            fprintf (FileHandle, "%llu: %s (%i/%s)\n", Cycle, par_Text, par_Channel, GET_CAN_OBJECT_NAME(CanServerConfig, par_ObjPos));
            for(c = 0; c < CanServerConfig->channel_count; c++) {
                int o;
                fprintf (FileHandle, " Channel %i:\n", c);
                fprintf (FileHandle, "  Current active timeouts:\n");
                uint64_t Timestamp;
                Timestamp = GetSimulatedTimeSinceStartedInNanoSecond();
                for (o = 0; o < CanServerConfig->channels[c].J1939TimeoutObjIdxPos; o++) {
                    int ObjPos = CanServerConfig->channels[c].J1939TimeoutObjIdx[o];
                    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
                    uint64_t Diff = CanServerConfig->channels[c].J1939Timeout[o] - Timestamp;
                    fprintf (FileHandle, "    %llu: %s: 0x%X: (%i)\n", Diff, GET_CAN_OBJECT_NAME(CanServerConfig, ObjPos), Object->id, (int)Object->Protocol.J1939.status);
                }
                fprintf (FileHandle, "  Current active transmits:\n");
                for (o = 0; o < CanServerConfig->channels[c].J1939CurrentTxObjIdxPos; o++) {
                    int ObjPos = CanServerConfig->channels[c].J1939CurrentTxObjIdx[o];
                    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
                    fprintf (FileHandle, "    %s: 0x%X: (%i)\n", GET_CAN_OBJECT_NAME(CanServerConfig, ObjPos), Object->id, (int)Object->Protocol.J1939.status);
                }
                fprintf (FileHandle, "  All j1939 objects:\n");
                for (o = 0; o < CanServerConfig->channels[c].object_count; o++) {
                    int ObjPos = CanServerConfig->channels[c].objects[o];
                    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
                    if (Object->type == J1939_OBJECT) {
                        fprintf (FileHandle, "    %s: 0x%X: (%i)\n", GET_CAN_OBJECT_NAME(CanServerConfig, ObjPos), Object->id, (int)Object->Protocol.J1939.status);
                    }
                }
            }
        }
        fflush(FileHandle);
    }
}
#else
#define DebugLogging(CanServerConfig, par_Channel, par_ObjPos, par_Id, par_Data, par_Text)
#endif

static void AddPngToData(NEW_CAN_SERVER_CONFIG *CanServerConfig, int ObjPos, uint8_t *Data)
{
    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
    uint32_t Pgn = Object->id & 0xFFFF00;
    Data[5] = Pgn >> 8;
    Data[6] = Pgn >> 16;
    Data[7] = Pgn;
}

static int SendRtsOrBamMessage(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel, int ObjPos, uint8_t ControlByte)
{
    uint8_t Data[8];
    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
    Object->Protocol.J1939.number_of_packages =  Object->Protocol.J1939.size / 7U;
    if((Object->Protocol.J1939.size % 7U) > 0) {
        Object->Protocol.J1939.number_of_packages++;
    }
    Data[0] = ControlByte;
    Data[1] = Object->Protocol.J1939.size;
    Data[2] = Object->Protocol.J1939.size >> 8;
    Data[3] = Object->Protocol.J1939.number_of_packages;
    Data[4] = 0xFF;
    AddPngToData(CanServerConfig, ObjPos, Data);
    int Id = (Object->id & 0xFF0000FF) | 0xEC0000 | ((int)Object->Protocol.J1939.dst_addr << 8);
    DebugLogging(CanServerConfig, Channel, ObjPos, Id, Data, "Send RTS");
    return SendCanObjectForOtherProcesses (Channel, Id, 1, 8, Data);
}

static int SendDataPackage(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel, int ObjPos)
{
    uint8_t Data[8];
    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
    Data[0] = Object->Protocol.J1939.current_number_of_package;

    int Src = (int)(Object->Protocol.J1939.current_number_of_package - 1) * 7;
    for (int Dst = 1; (Dst < 8); Src++, Dst++) {
        if (Src < Object->size) {
            Data[Dst] = Object->DataPtr[Src];
        } else {
            Data[Dst] = 0xFF;
        }
    }
    uint32_t Id = (Object->id & 0xFF0000FF) | 0xEB0000 | ((uint32_t)Object->Protocol.J1939.dst_addr << 8);
    DebugLogging(CanServerConfig, Channel, ObjPos, Id, Data, "Send data package");
    return SendCanObjectForOtherProcesses (Channel, Id, 1, 8, Data);
}

static int SendCtsMessage(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel, int ObjPos)
{
    uint8_t Data[8];
    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
    Data[0] = CONTROL_BYTE_TP_CM_CTS;  // CTS (Clear to send)
    Data[1] = Object->Protocol.J1939.number_of_packages;
    Data[2] = Object->Protocol.J1939.current_number_of_package; // TODO start with 1
    Data[3] = 0xFF;
    Data[4] = 0xFF;
    AddPngToData(CanServerConfig, ObjPos, Data);
    int Id = (Object->id & 0xFF0000FF) | 0xEC0000 | ((int)Object->Protocol.J1939.dst_addr << 8);
    // Swap source and destionation address
    Id = (Id & 0xFFFF0000) | ((Id >> 8) & 0xFF) | ((Id << 8) & 0xFF00);
    DebugLogging(CanServerConfig, Channel, ObjPos, Id, Data, "Send CTS");
    return SendCanObjectForOtherProcesses (Channel, Id, 1, 8, Data);
}

static int SendAckMessage(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel, int ObjPos)
{
    uint8_t Data[8];
    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
    Data[0] = CONTROL_BYTE_TP_CM_END_OF_MSG_ACK;  // ACK
    Data[1] = Object->Protocol.J1939.size;
    Data[2] = Object->Protocol.J1939.size >> 8;
    Data[3] = Object->Protocol.J1939.current_number_of_package;
    Data[4] = 0xFF;
    AddPngToData(CanServerConfig, ObjPos, Data);
    int Id = (Object->id & 0xFF0000FF) | 0xEC0000 | ((int)Object->Protocol.J1939.dst_addr << 8);
    // Swap source and destionation address
    Id = (Id & 0xFFFF0000) | ((Id >> 8) & 0xFF) | ((Id << 8) & 0xFF00);
    DebugLogging(CanServerConfig, Channel, ObjPos, Id, Data, "Send ACK");
    return SendCanObjectForOtherProcesses (Channel, Id, 1, 8, Data);
}

static int SendAbortMessage(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel, int ObjPos, uint8_t Reason)
{
    uint8_t Data[8];
    int Ret;
    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
    Data[0] = CONTROL_BYTE_TP_CM_ABORT; // Conection abort
    Data[1] = Reason;
    Data[2] = 0xFF;
    Data[3] = 0xFF;
    Data[4] = 0xFF;
    AddPngToData(CanServerConfig, ObjPos, Data);
    int Id = (Object->id & 0xFF0000FF) | 0xEC0000 | ((int)Object->Protocol.J1939.dst_addr << 8);
    DebugLogging(CanServerConfig, Channel, ObjPos, Id, Data, "Send Abort");
    Ret = SendCanObjectForOtherProcesses (Channel, Id, 1, 8, Data);
#if 0
    // Swap source and destionation address
    Id = (Id & 0xFFFF0000) | ((Id >> 8) & 0xFF) | ((Id << 8) & 0xFF00);
    Ret = Ret | SendCanObjectForOtherProcesses (Channel, Id, 1, 8, Data);
#endif
    return Ret;
}


static int AddJ1939TimeOut(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel, int ObjPos, uint32_t Timeout)
{
    if (CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos < J1939_MAX_SIMULTANEOUS_CONNECTIONS) {
        CanServerConfig->channels[Channel].J1939TimeoutObjIdx[CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos] = ObjPos;
        CanServerConfig->channels[Channel].J1939Timeout[CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos] =
            GetSimulatedTimeSinceStartedInNanoSecond() + (uint64_t)Timeout * 1000000;
        CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos++;
        return 0;
    } else {
        DebugLogging(CanServerConfig, Channel, ObjPos, 0, NULL, "To many timeouts");
        return -1;
    }
}

static int RemoveJ1939TimeOut(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel, int ObjPos)
{
    int x;
    for (x = 0; x < CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos; x++) {
        if (CanServerConfig->channels[Channel].J1939TimeoutObjIdx[x] == ObjPos) {
            for (x++; x < CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos; x++) {
                CanServerConfig->channels[Channel].J1939TimeoutObjIdx[x-1] = CanServerConfig->channels[Channel].J1939TimeoutObjIdx[x];
                CanServerConfig->channels[Channel].J1939Timeout[x-1] = CanServerConfig->channels[Channel].J1939Timeout[x];
            }
            CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos--;
            return 0;
        }
    }
    DebugLogging(CanServerConfig, Channel, ObjPos, 0, NULL, "remove no matching timeout");
    return -1;
}

static int UpdateJ1939TimeOut(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel, int ObjPos, uint32_t NewTimeout)
{
    int x;
    for (x = 0; x < CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos; x++) {
        if (CanServerConfig->channels[Channel].J1939TimeoutObjIdx[x] == ObjPos) {
            CanServerConfig->channels[Channel].J1939Timeout[x] =
                GetSimulatedTimeSinceStartedInNanoSecond() + (uint64_t)NewTimeout * 1000000; // ns
            return 0;
        }
    }
    DebugLogging(CanServerConfig, Channel, ObjPos, 0, NULL, "update no matching timeout");
    return -1;
}

static int J1939RemoveCurrentTxObject(NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, int ObjPos)
{
    int x;
    for (x = 0; x < CanServerConfig->channels[c].J1939CurrentTxObjIdxPos; x++) {
        if (CanServerConfig->channels[c].J1939CurrentTxObjIdx[x] == ObjPos) {
            for (x++; x < CanServerConfig->channels[c].J1939CurrentTxObjIdxPos; x++) {
                CanServerConfig->channels[c].J1939CurrentTxObjIdx[x - 1] = CanServerConfig->channels[c].J1939CurrentTxObjIdx[x];
            }
            CanServerConfig->channels[c].J1939CurrentTxObjIdxPos--;
            return 0;
        }
    }
    return -1;
}

static void StopTransmitingMessage(NEW_CAN_SERVER_CONFIG *CanServerConfig, NEW_CAN_SERVER_OBJECT *Object, int c, int ObjPos)
{
    DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "before StopTransmitingMessage()");
    J1939RemoveCurrentTxObject(CanServerConfig, c, ObjPos);
    RemoveJ1939TimeOut(CanServerConfig, c, ObjPos);
    Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
    DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "after StopTransmitingMessage()");
}

void J1939CheckAllTimeOuts(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel)
{
    int x;
    uint64_t Timestamp;
    Timestamp = GetSimulatedTimeSinceStartedInNanoSecond();
    for (x = 0; x < CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos; x++) {
        if(Timestamp >= CanServerConfig->channels[Channel].J1939Timeout[x]) {
            int ObjPos = CanServerConfig->channels[Channel].J1939TimeoutObjIdx[x];
            NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
            if (Object->type == READ_OBJECT) {
                SendAbortMessage(CanServerConfig, Channel, ObjPos, ABORT_REASON_TIMEOUT); // TODO
                Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
                RemoveJ1939TimeOut(CanServerConfig, Channel, ObjPos);
            } else {
                // it is a tramsmit object timeout
                if (Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_TRANSMIT_DATA) {
                    if (Object->Protocol.J1939.current_number_of_package <= Object->Protocol.J1939.number_of_packages) {
                        // send data package
                        SendDataPackage(CanServerConfig, Channel, ObjPos);
                        if ((Object->Protocol.J1939.current_number_of_package * 7) >= Object->Protocol.J1939.size) {
                            // all data packages was transmitted
                            if (Object->Protocol.J1939.format == PDU_FORMAT_2) {
                                StopTransmitingMessage(CanServerConfig, Object, Channel, ObjPos);
                                HaveTransmittedJ1939TxMultiPackageFrame(CanServerConfig, Channel, ObjPos, Timestamp);
                            } else {
                                // we should wait for an ACK
                                Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_WAIT_FOR_ACK;
                                UpdateJ1939TimeOut(CanServerConfig, Channel, ObjPos, TIMEOUT_T3);
                            }
                        } else {
                            uint32_t Timeout;
                            if (Object->Protocol.J1939.format == PDU_FORMAT_2) {
                                Timeout = TIME_BETWEEN_TWO_DATA_PACKAGE_BAM;
                            } else {
                                Timeout = TIME_BETWEEN_TWO_DATA_PACKAGE_CTS;
                            }
                            UpdateJ1939TimeOut(CanServerConfig, Channel, ObjPos, Timeout);
                            Object->Protocol.J1939.current_number_of_package++;
                        }
                    } else {
                        // this stage should not reached
                        DebugLogging(CanServerConfig, Channel, ObjPos, 0, NULL, "j1939 are in state J1939_MULTI_PACKAGE_WAIT_FOR_TRANSMIT_DATA but "
                                     "current_number_of_package >= number_of_packages");
                    }
                } else { //if (Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_CTS) {
                    // this should be terminat the transmit because of a missing CTS/ACK
                    StopTransmitingMessage(CanServerConfig, Object, Channel, ObjPos);
                }
            }
        }
    }
}

void J1939ClearAllTimeOuts(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel)
{
    int x;
    for (x = 0; x < CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos; x++) {
        int ObjPos = CanServerConfig->channels[Channel].J1939TimeoutObjIdx[x];
        NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
        Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
    }
    CanServerConfig->channels[Channel].J1939TimeoutObjIdxPos = 0;
}

static int J1939AddCurrentTxObject(NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, int ObjPos)
{
    if (CanServerConfig->channels[c].J1939CurrentTxObjIdxPos < J1939_MAX_SIMULTANEOUS_CONNECTIONS) {
        CanServerConfig->channels[c].J1939CurrentTxObjIdx[CanServerConfig->channels[c].J1939CurrentTxObjIdxPos] = ObjPos;
        CanServerConfig->channels[c].J1939CurrentTxObjIdxPos++;
        return 0;
    } else {
        return -1;
    }
}

static int J1939CheckIfResourceIsFree(NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, int DstAddr, int SrcAddr)
{
    int x;
    // check if there is an active transmition with same source/destionation address
    for (x = 0; x < CanServerConfig->channels[c].J1939CurrentTxObjIdxPos; x++) {
        int ObjPos = CanServerConfig->channels[c].J1939CurrentTxObjIdx[x];
        if (((uint8_t)CanServerConfig->objects[ObjPos].id == SrcAddr) &&
            ((uint8_t)CanServerConfig->objects[ObjPos].Protocol.J1939.dst_addr == DstAddr)) {
            DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "Resource blocked by current tx");
            return 0;
        }
    }
    // check if there is an active receive with same source/destionation address
    for (x = 0; x < CanServerConfig->channels[c].J1939TimeoutObjIdxPos; x++) {
        int ObjPos = CanServerConfig->channels[c].J1939TimeoutObjIdx[x];
        if (((uint8_t)CanServerConfig->objects[ObjPos].id == DstAddr) &&
            ((uint8_t)CanServerConfig->objects[ObjPos].Protocol.J1939.dst_addr == SrcAddr)) {
            DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "Resource blocked by current rx");
            return 0;
        }
    }
    return 1;
}

static void FilterCurrentTxAckMessages(NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, uint32_t id, uint8_t *data, int size, uint64_t TimeStamp)
{
    int x;
    for (x = 0; x < CanServerConfig->channels[c].J1939CurrentTxObjIdxPos; x++) {
        int ObjPos = CanServerConfig->channels[c].J1939CurrentTxObjIdx[x];
        NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
        uint32_t RefId1 = (uint32_t)id & 0xFF0000FFUL;
        uint32_t RefId2 = ((uint32_t)Object->id & 0xFF000000UL) + Object->Protocol.J1939.dst_addr;
        if (RefId2 == RefId1) {  // should we check same priority?
            if ((Object->type != J1939_OBJECT) || (Object->size <= 8)) {
                DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "this must be a j1939 multi package object");
            } else {
                if (Object->dir == WRITE_OBJECT) {
                    int MsgPgn = ((uint32_t)data[6] << 16) + ((uint32_t)data[5] << 8);
                    int ObjPgn = Object->id & 0xFFFF00;
                    if (MsgPgn == ObjPgn) {
                        switch(data[0]) {   // Control byte
                        case CONTROL_BYTE_TP_CM_CTS:  // CTS (Clear to send)
                            if ((Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_CTS) ||
                                (Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_TRANSMIT_DATA))  {
                                if (data[1] == 0) { // CTS HOLD
                                    Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_WAIT_FOR_CTS;
                                    UpdateJ1939TimeOut(CanServerConfig, c, ObjPos, TIMEOUT_T4);
                                } else {
                                    Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_WAIT_FOR_TRANSMIT_DATA;
                                    Object->Protocol.J1939.number_of_packages = data[1];
                                    Object->Protocol.J1939.current_number_of_package = data[2];
                                    // Update timeout 0 -> the first data package will be send immediately
                                    UpdateJ1939TimeOut(CanServerConfig, c, ObjPos, 0);
                                }
                            } else if (Object->Protocol.J1939.status != J1939_MULTI_PACKAGE_IDLE) {
                                DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "j1939 transmit multi package object not waiting for a CTS message");
                                SendAbortMessage(CanServerConfig, c, ObjPos, ABORT_REASON_ERROR);
                                StopTransmitingMessage(CanServerConfig, Object, c, ObjPos);
                            }
                            break;
                        case CONTROL_BYTE_TP_CM_END_OF_MSG_ACK:  // End of message Ack
                            if ((Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_CTS) ||
                                (Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_ACK) ||
                                (Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_TRANSMIT_DATA))  {
                                StopTransmitingMessage(CanServerConfig, Object, c, ObjPos);
                                HaveTransmittedJ1939TxMultiPackageFrame(CanServerConfig, c, ObjPos, TimeStamp);
                            } else {
                                DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "j1939 transmit multi package object not waiting for a ACK message");
                                SendAbortMessage(CanServerConfig, c, ObjPos, ABORT_REASON_ERROR);
                                StopTransmitingMessage(CanServerConfig, Object, c, ObjPos);
                            }
                            break;
                        case CONTROL_BYTE_TP_CM_ABORT:  // Conection abort
                            if (Object->Protocol.J1939.status != J1939_MULTI_PACKAGE_IDLE) {
                                DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "j1939 receive multi package object will be aborted");
                                StopTransmitingMessage(CanServerConfig, Object, c, ObjPos);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

static void FilterRxMessages(NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, uint32_t id, uint8_t *data, int size, uint64_t TimeStamp)
{
    uint8_t DstAddr;
    int32_t ObjListIdx;

    DstAddr = id >> 8;
    ObjListIdx = CanServerConfig->channels[c].J1939RxDstAddrObjIdx[DstAddr];
    if (ObjListIdx >= 0) {
        int16_t *ObjPosList = (int16_t*)&CanServerConfig->Symbols[ObjListIdx];
        while(*ObjPosList >= 0) {
            int ObjPos = *ObjPosList;
            NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
            if ((Object->id & 0xFF0000FFUL) == (id & 0xFF0000FFUL)) {  // should we check same priority?
                if ((Object->type != J1939_OBJECT) || (Object->size <= 8)) {
                    DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "this must be a j1939 multi package object");
                } else {
                    if (Object->dir == READ_OBJECT) {
                        uint8_t SrcAddr;
                        int MsgPgn = ((uint32_t)data[6] << 16) + ((uint32_t)data[5] << 8);
                        int ObjPgn = Object->id & 0xFFFF00;
                        if (MsgPgn == ObjPgn) {
                            switch(data[0]) {   // Control byte
                            case CONTROL_BYTE_TP_CM_RTS:  // RTS (Request to send)
                                SrcAddr = id;
                                Object->Protocol.J1939.dst_addr = DstAddr; // is destination address was configured as 0xFF this can changed
                                // switch src and dst address
                                if (!J1939CheckIfResourceIsFree(CanServerConfig, c, SrcAddr, DstAddr)) {
                                    DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "j1939 resource is not free send abort");
                                    SendAbortMessage(CanServerConfig, c, ObjPos, ABORT_REASON_ALREADY_IN_USE);
                                    break;  // switch()
                                }
                                // fall througth
                            case CONTROL_BYTE_TP_CM_BAM:  // BAM
                                if (Object->Protocol.J1939.status != J1939_MULTI_PACKAGE_IDLE) {
                                    DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "j1939 receive multi package object not in idle state. We will throw away the not finished data and start a new reception");
                                    Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
                                    RemoveJ1939TimeOut(CanServerConfig, c, ObjPos);
                                }
                                Object->Protocol.J1939.size = (int)data[1] + ((int)data[2] << 8);
                                Object->Protocol.J1939.number_of_packages = data[3];
                                Object->Protocol.J1939.current_number_of_package = 1; // we start with package 1
                                if (Object->Protocol.J1939.size > Object->size) {
                                    DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "j1939 receive multi package object larger than configured buffer (it will be truncated)");
                                    // but continue
                                }
                                if (data[0] == CONTROL_BYTE_TP_CM_RTS) {
                                    Object->Protocol.J1939.format = PDU_FORMAT_1;
                                    SendCtsMessage(CanServerConfig, c, ObjPos);
                                    AddJ1939TimeOut(CanServerConfig, c, ObjPos, TIMEOUT_T2);
                                } else {   // BAM
                                    Object->Protocol.J1939.format = PDU_FORMAT_2;
                                    AddJ1939TimeOut(CanServerConfig, c, ObjPos, TIMEOUT_T1);
                                }
                                Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_WAIT_FOR_DATA;
                                break;
                            case CONTROL_BYTE_TP_CM_ABORT:   // Conection abort
                                if (Object->Protocol.J1939.status != J1939_MULTI_PACKAGE_IDLE) {
                                    DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "j1939 receive multi package object will be aborted");
                                    Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
                                    RemoveJ1939TimeOut(CanServerConfig, c, ObjPos);
                                }
                                break;
                            default:
                                //DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "receive unknown control byte inside a j1939 multi package object");
                                break;
                            }
                        }
                    }
                }
            }
            ObjPosList++;
        }
    }
}

void J1939MultiPackageCanMessageFilter (NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, uint32_t id, uint8_t *data, int size, uint64_t TimeStamp)
{
    uint8_t DstAddr;
    int32_t ObjListIdx;

    switch (id & 0xFF0000) {
    case 0xEC0000:  // RTS/CTS
        FilterRxMessages(CanServerConfig, c, id, data, size, TimeStamp);
        FilterCurrentTxAckMessages(CanServerConfig, c, id, data, size, TimeStamp);
        break;
    case 0xEB0000:  // DT
        DstAddr = id >> 8;
        ObjListIdx = CanServerConfig->channels[c].J1939RxDstAddrObjIdx[DstAddr];
        if (ObjListIdx >= 0) {
            int16_t *ObjPosList = (int16_t*)&CanServerConfig->Symbols[ObjListIdx];
            while(*ObjPosList >= 0) {
                int Id;
                int ObjPos = *ObjPosList;
                NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
                if ((Object->id & 0xFF0000FFUL) == (id & 0xFF0000FFUL)) {  // should we check same priority?
                    if (Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_DATA) {
                        if (Object->Protocol.J1939.current_number_of_package == data[0]) {
                            if (Object->Protocol.J1939.number_of_packages >= data[0]) {
                                // now we have received a valid data package, copy the data
                                int Dst = (int)(Object->Protocol.J1939.current_number_of_package - 1) * 7;
                                int SrcMax = Object->Protocol.J1939.size - Dst;
                                if (SrcMax > 7) {
                                    SrcMax = 7;
                                }
                                for (int Src = 0; (Dst < Object->size) && (Src < SrcMax); Src++, Dst++) {
                                    Object->DataPtr[Dst] = data[Src + 1];
                                }
                                if (Object->Protocol.J1939.current_number_of_package == Object->Protocol.J1939.number_of_packages) {
                                    if ((Object->Protocol.J1939.current_number_of_package * 7) >= Object->Protocol.J1939.size) {
                                        // we have received the complete message
                                        Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
                                        SendAckMessage(CanServerConfig, c, ObjPos);
                                        RemoveJ1939TimeOut(CanServerConfig, c, ObjPos);
                                        Object->Protocol.J1939.ret_dlc = Object->Protocol.J1939.size;
                                        DecodeJ1939RxMultiPackageFrame(CanServerConfig, c, ObjPos, TimeStamp);
                                    } else {
                                        // wait for an additional CTS
                                        Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_WAIT_FOR_CTS;
                                        UpdateJ1939TimeOut(CanServerConfig, c, ObjPos, TIMEOUT_Th);
                                    }
                                } else {
                                    Object->Protocol.J1939.current_number_of_package++;
                                    UpdateJ1939TimeOut(CanServerConfig, c, ObjPos, TIMEOUT_T1);
                                }
                            } else {
                                // too many data packages
                                Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
                                RemoveJ1939TimeOut(CanServerConfig, c, ObjPos);
                                SendAbortMessage(CanServerConfig, c, ObjPos, ABORT_REASON_ERROR);
                            }
                        } else {
                            // not fitting data packages
                            Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
                            RemoveJ1939TimeOut(CanServerConfig, c, ObjPos);
                            SendAbortMessage(CanServerConfig, c, ObjPos, ABORT_REASON_ERROR);
                        }
                    } else {
                        // Sequence error
                        Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_IDLE;
                        RemoveJ1939TimeOut(CanServerConfig, c, ObjPos);
                        SendAbortMessage(CanServerConfig, c, ObjPos, ABORT_REASON_ERROR);
                    }
                }
                ObjPosList++;
            }
        }
        break;
    default:
        break;
    }
 }

int J1939MultiPackageCanTransmit (NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, int ObjPos)
{
    NEW_CAN_SERVER_OBJECT *Object = &CanServerConfig->objects[ObjPos];
    if ((Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_IDLE) ||
        (Object->Protocol.J1939.status == J1939_MULTI_PACKAGE_WAIT_FOR_RESOURCE)) {
        if (J1939CheckIfResourceIsFree(CanServerConfig, c, Object->Protocol.J1939.dst_addr, (uint8_t)Object->id)) {
            if (Object->Protocol.J1939.dst_addr == 0xFF) {  // it is a BAM
                Object->Protocol.J1939.format = PDU_FORMAT_2;
                Object->Protocol.J1939.current_number_of_package = 1;
                SendRtsOrBamMessage(CanServerConfig, c, ObjPos, CONTROL_BYTE_TP_CM_BAM);
                Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_WAIT_FOR_TRANSMIT_DATA;
                AddJ1939TimeOut(CanServerConfig, c, ObjPos, TIME_BETWEEN_TWO_DATA_PACKAGE_BAM);
                DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "start transmitting BAM");
            } else {
                J1939AddCurrentTxObject(CanServerConfig, c, ObjPos);  // only CTS will be added not the BAM
                Object->Protocol.J1939.format = PDU_FORMAT_1;
                SendRtsOrBamMessage(CanServerConfig, c, ObjPos, CONTROL_BYTE_TP_CM_RTS);
                Object->Protocol.J1939.status = J1939_MULTI_PACKAGE_WAIT_FOR_CTS;
                AddJ1939TimeOut(CanServerConfig, c, ObjPos, TIMEOUT_T2);
                DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "start transmitting CTS");
            }
            return 0;    // transmition started
        } else {
            DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "other connection with same destination or source address are active");
            return -2;   // other connection with same destination or source address are active
        }
    } else {
        DebugLogging(CanServerConfig, c, ObjPos, 0, NULL, "this message is in transmit state");
        return -1; // this message is in transmit state
    }
}


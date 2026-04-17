#ifndef J1939MULTIPACKAGE_H
#define J1939MULTIPACKAGE_H

#include "ReadCanCfg.h"

#define J1939_MULTI_PACKAGE_IDLE                    0
#define J1939_MULTI_PACKAGE_WAIT_FOR_CTS            1
#define J1939_MULTI_PACKAGE_WAIT_FOR_ACK            2
#define J1939_MULTI_PACKAGE_WAIT_FOR_DATA           3
#define J1939_MULTI_PACKAGE_WAIT_FOR_TRANSMIT_DATA  4
#define J1939_MULTI_PACKAGE_WAIT_FOR_RESOURCE       5

void J1939CheckAllTimeOuts(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel);

void J1939ClearAllTimeOuts(NEW_CAN_SERVER_CONFIG *CanServerConfig, int Channel);

void J1939MultiPackageCanMessageFilter(NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, uint32_t id, uint8_t *data, int size, uint64_t TimeStamp);

int J1939MultiPackageCanTransmit (NEW_CAN_SERVER_CONFIG *CanServerConfig, int c, int ObjPos);

#endif

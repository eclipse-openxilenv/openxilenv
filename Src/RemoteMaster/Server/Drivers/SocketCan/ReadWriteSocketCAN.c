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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ReadCanCfg.h"
#include "RealtimeScheduler.h"
#include "ReadWriteSocketCAN.h"

#ifndef CANFD_FDF
#define CANFD_FDF  0x04
#endif

int SocketCAN_open_can(NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    int ChannelOnCardNr;
    int Status;
    int Socket;
    char CmdLine[256];
    char DeviceName[32];

    ChannelOnCardNr = csc->channels[channel].CardTypeNrChannel & 0xFF;

    PrintFormatToString (CmdLine, sizeof(CmdLine), "ip link set down can%i", ChannelOnCardNr);
    Status = system(CmdLine);

    // ip link set can0 txqueuelen 1000
    if (csc->channels[channel].can_fd_enabled) {
        PrintFormatToString (CmdLine, sizeof(CmdLine), "ip link set can%i type can bitrate %i sample-point %f dbitrate %i dsample-point %f fd on",
                ChannelOnCardNr, csc->channels[channel].bus_frq * 1000, csc->channels[channel].sample_point, csc->channels[channel].data_baud_rate * 1000, csc->channels[channel].data_sample_point);
    } else {
        PrintFormatToString (CmdLine, sizeof(CmdLine), "ip link set can%i type can bitrate %i sample-point %f",
                ChannelOnCardNr, csc->channels[channel].bus_frq * 1000, csc->channels[channel].sample_point);
    }
    Status = system(CmdLine);
    if (Status != 0) return -1;

    PrintFormatToString (CmdLine, sizeof(CmdLine), "ip link set up can%i", ChannelOnCardNr);
    Status = system(CmdLine);
    if (Status != 0) return -1;

    Socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    /* Locate the interface you wish to use */
    struct ifreq ifr;
    PrintFormatToString (DeviceName, sizeof(DeviceName), "can%i", ChannelOnCardNr);
    STRING_COPY_TO_ARRAY(ifr.ifr_name, DeviceName);
    Status = ioctl(Socket, SIOCGIFINDEX, &ifr); /* ifr.ifr_ifindex gets filled
                                                 * with that device's index */
    if (Status != 0) {
        close(Socket);
        return -1;
    }

    u_long mode = 1;  // 1 to enable non-blocking socket
    Status = ioctl(Socket, FIONBIO, &mode);

    if (csc->channels[channel].can_fd_enabled) {
        int status;
        int raw_fd_frames = 1; /* 0 = disabled (default), 1 = enabled */

        status = setsockopt(Socket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                            &raw_fd_frames, sizeof(raw_fd_frames));
    }
    /* Select that CAN interface, and bind the socket to it. */
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(Socket, (struct sockaddr*)&addr, sizeof(addr))) {
        close(Socket);
        return -1;
    }
    csc->channels[channel].Socket = Socket;

    return 0;
}

int SocketCAN_close_can(NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    int ChannelOnCardNr;
    int Status;
    char CmdLine[256];    
 
    ChannelOnCardNr = csc->channels[channel].CardTypeNrChannel & 0xFF;

    close(csc->channels[channel].Socket);

    PrintFormatToString (CmdLine, sizeof(CmdLine), "ip link set down can%i", ChannelOnCardNr);
    Status = system(CmdLine);
    if (Status != 0) return -1;
    return 0;
}

int SocketCAN_ReadNextObjectFromQueue(NEW_CAN_SERVER_CONFIG *csc, int channel,
    uint32_t *pid, unsigned char* data,
    unsigned char* pext, unsigned char* psize,
    uint64_t *pTimeStamp)
{
    struct iovec IoVec;
    struct msghdr MsgHdr;
    struct sockaddr_can SocketAddr = {
        .can_family = AF_CAN,
        .can_ifindex = 0,  // any can interface
    };
    char MsgControl[CMSG_SPACE(sizeof(struct timeval)) +
                    CMSG_SPACE(3 * sizeof(struct timespec)) +
                    CMSG_SPACE(sizeof(__u32))];
    struct cmsghdr *CMsgHdr;
    struct timeval TimeValue;
    int ReadBytes;

    MsgHdr.msg_namelen = sizeof(SocketAddr);
    MsgHdr.msg_name = &SocketAddr;
    MsgHdr.msg_iov = &IoVec;
    MsgHdr.msg_iovlen = 1;
    MsgHdr.msg_controllen = sizeof(MsgControl);
    MsgHdr.msg_control = &MsgControl;
    MsgHdr.msg_flags = 0;

    if (csc->channels[channel].can_fd_enabled) {
        struct canfd_frame Frame;

        IoVec.iov_len = sizeof(Frame);
        IoVec.iov_base = &Frame;

__IGNORE_FD:
        ReadBytes = recvmsg(csc->channels[channel].Socket, &MsgHdr, 0);

        if (ReadBytes > 0) {
            if ((size_t)ReadBytes != CANFD_MTU) {
                //ThrowError(1, "recvmsg wrong size %i expectd %i", nbytes, CAN_MTU);
                goto __IGNORE_FD;
            }
            else {
                *pTimeStamp = GetCycleCounter64() * get_sched_periode_timer_clocks64();
                *pid = (uint32_t)(Frame.can_id & 0x1FFFFFFF);
                *pext = (unsigned char)(Frame.can_id >> 31) |
                        (((Frame.flags & CANFD_FDF) == CANFD_FDF) << 2) |
                        (((Frame.flags & CANFD_BRS) == CANFD_BRS) << 1);
                *psize = Frame.len;
                MEMCPY(data, Frame.data, Frame.len);
                return 1;
            }
        } else {
            return 0;
        }
    } else {
        struct can_frame Frame;

        IoVec.iov_len = sizeof(Frame);
        IoVec.iov_base = &Frame;

    __IGNORE:
        ReadBytes = recvmsg(csc->channels[channel].Socket, &MsgHdr, 0);

        if (ReadBytes > 0) {
            if ((size_t)ReadBytes != CAN_MTU) {
                //ThrowError(1, "recvmsg wrong size %i expectd %i", nbytes, CAN_MTU);
                goto __IGNORE;
            } else {
                *pTimeStamp = GetCycleCounter64() * get_sched_periode_timer_clocks64();
                *pid = (uint32_t)(Frame.can_id & 0x1FFFFFFF);
                *pext = (unsigned char)(Frame.can_id >> 31);
                *psize = Frame.can_dlc;
                MEMCPY(data, Frame.data, 8);
                return 1;
            }
        } else {
            return 0;
        }
    }
}

int SocketCAN_read_can(NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos)
{
    return 0;
}


int SocketCAN_write_can(NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos)
{
    if (csc->channels[channel].can_fd_enabled) {
        int ret;
        struct canfd_frame frame;
        frame.can_id = csc->objects[o_pos].id | (uint32_t)csc->objects[o_pos].ext << 31;
        frame.len = (__u8)csc->objects[o_pos].size;
        frame.flags = ((csc->objects[o_pos].ext & 0x02) >> 1) |  // BRS
                      (csc->objects[o_pos].ext & 0x04); // FDF
        MEMCPY(frame.data, csc->objects[o_pos].Data, (size_t)csc->objects[o_pos].size);
        ret = write(csc->channels[channel].Socket, &frame, sizeof(struct canfd_frame));
        if (ret == sizeof(struct canfd_frame)) {
            return 0;
        } else {
            return 1;
        }
    } else {
        struct can_frame frame;
        frame.can_id = csc->objects[o_pos].id | (uint32_t)csc->objects[o_pos].ext << 31;
        frame.can_dlc = (__u8)csc->objects[o_pos].size;
        MEMCPY(frame.data, csc->objects[o_pos].Data, (size_t)csc->objects[o_pos].size);

        if (write(csc->channels[channel].Socket, &frame, sizeof(struct can_frame)) == sizeof(struct can_frame)) {
            return 0;
        } else {
            return 1;
        }
    }
}

// return 0 CAN object was written into the queueand there is place for one ore more additional objects
// return 1 CAN object was written into the queue but there is no place for additional objects
// return -1 CAN object was not written into the queue (full)
int SocketCAN_WriteObjectToQueue(NEW_CAN_SERVER_CONFIG *csc, int channel,
    uint32_t id, unsigned char* data,
    unsigned char ext, unsigned char size)
{
    if (csc->channels[channel].can_fd_enabled) {
        struct canfd_frame frame;
        frame.can_id = id | (uint32_t)ext << 31;
        frame.len = size;
        if (size > 8) {
            frame.flags = CANFD_BRS;
        }
        MEMCPY(frame.data, data, size);

        if (write(csc->channels[channel].Socket, &frame, sizeof(struct canfd_frame)) == sizeof(struct canfd_frame)) {
            return 0;
        } else {
            return 1;
        }
    } else {
        struct can_frame frame;
        frame.can_id = id | (uint32_t)ext << 31;
        frame.can_dlc = size;
        MEMCPY(frame.data, data, size);

        if (write(csc->channels[channel].Socket, &frame, sizeof(struct can_frame)) == sizeof(struct can_frame)) {
            return 0;
        } else {
            return 1;
        }
    }
}

int SocketCAN_status_can(NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    return 0;
}

int SocketCAN_info_can(struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, int Info, int MaxReturnSize, void *Return)
{
    switch (Info) {
    case CAN_CHANNEL_INFO_MIXED_IDS_ALLOWED:
        if (MaxReturnSize >= sizeof(int)) {
            *(int*)Return = 1;                    // card will support mixed 11/29bit Ids
            return sizeof(int);
        } else return -2;
    case CAN_CHANNEL_INFO_CAN_CARD_STRING:
        if (MaxReturnSize >= 15) {
            StringCopyMaxCharTruncate((char*)Return, "Socket CAN", MaxReturnSize);
            return (int)strlen((char*)Return) + 1;
        } else return -2;
    }
    return -1;
}


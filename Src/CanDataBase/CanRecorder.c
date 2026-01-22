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


#include "EquationParser.h"
#include "Files.h"
#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "CanFifo.h"
#include "MainValues.h"
#include "RemoteMasterControlProcess.h"
#include "CanRecorder.h"

struct CAN_RECORDER {
    int m_CanFiFoHandle;
    int m_OnOff;
    CAN_ACCEPT_MASK m_cam[MAX_CAN_ACCEPTANCE_MASK_ELEMENTS+1];   // The last one must be Channel = -1
    int m_cam_count;

    int m_DisplayFormat;
#define DISPLAY_HEX 0
#define DISPLAY_DEC 1
#define DISPLAY_BIN 2

    int m_Record2Disk;
    char m_RecorderFileName[MAX_PATH];
    FILE *m_fh;
    int m_TriggerActiv;
    char m_TriggerEquation[1024];
    int m_Triggered;
    int m_MaxCanMsgRec;

    int m_RefreshCounter;
    double m_AbtastPeriodeInMs;
    int m_DisplayColumnCounterFlag;
    int m_DisplayColumnTimeAbsoluteFlag;
    int m_DisplayColumnTimeDiffFlag;
    int m_DisplayColumnTimeDiffMinMaxFlag;
#ifdef _WIN32
    SYSTEMTIME m_SystemTimeStartCANLogging;
#else
    struct timespec m_SystemTimeStartCANLogging;
#endif
    int m_WaitForFirstMessageFlag;
    double m_TsOffset;
} CAN_RECORDER;

// return value are milliseconds
static double TimestampCalc (struct CAN_RECORDER *par_Rec, CAN_FD_FIFO_ELEM *pCanMessage)
{
    double Ts;

    if (pCanMessage->node) {  // transmitted by oneself
        // transmit messages: the timestampare are the cycle counter
        Ts = (double)(pCanMessage->timestamp) * par_Rec->m_AbtastPeriodeInMs;
    } else {
        if (s_main_ini_val.ConnectToRemoteMaster) {
            uint32_t Cycle;
            uint16_t T4Master, T4;
            switch (GetCanCardType (pCanMessage->channel)) {
            default:
                Ts = (double)(pCanMessage->timestamp) * 0.000001;
                break;
            case 1:   // I_PCI-165
            case 2:   // I_PCI-XC16
                Cycle = (uint32_t)(pCanMessage->timestamp >> 32);
                T4Master = (uint16_t)((uint32_t)(pCanMessage->timestamp) >> 16);
                T4 = (uint16_t)(pCanMessage->timestamp);
                Ts = (double)(Cycle) * par_Rec->m_AbtastPeriodeInMs + (double)((int16_t)(T4 - T4Master)) / 1250.0;
                break;
            case 6: // Flexcard
                 // High DWord ist der UCx, LowDWord ist der FRCx
                Ts = (double)((pCanMessage->timestamp << 32) | (pCanMessage->timestamp >> 32)) / 80000.0;
                break;
            case 8: // SocketCAN
                Ts = (double)(pCanMessage->timestamp) / 1000.0;
                break;
            }
        } else {
            // receiving CAN messages are 1ns resolution
            Ts = (double)(pCanMessage->timestamp) * 0.000001;
        }
    }
    return Ts;
}

static void BuildAbsoluteTimeString (struct CAN_RECORDER *par_Rec, double Ts, char *Txt)
{
    uint32_t Hour;
    uint32_t Minute;
    double T = Ts + par_Rec->m_TsOffset;

    Hour = (uint32_t)(T / 3600000.0);  // 1 hour has 3600s
    T -= (double)(Hour) * 3600000.0;
    Minute = (uint32_t)(T / 60000.0);  // 1 minute has 60s
    T -= (double)(Minute) * 60000.0;
    T *= 0.001;
    PrintFormatToString (Txt, sizeof(Txt), "%02i:%02i:%09.6f", Hour, Minute, T);
}

static void FlushMessageQueue (struct CAN_RECORDER *par_Rec)
{
    CAN_FIFO_ELEM CANMessage[64];
    while (ReadCanMessagesFromFifo2Process (par_Rec->m_CanFiFoHandle, CANMessage, 64) >= 64);
}

int CanRecorderReadAndProcessMessages (struct CAN_RECORDER *par_Rec)
{
    CAN_FD_FIFO_ELEM *pCANMessage, CANMessages[64];
    int i, x;
    int ReadElements;
    double Ts;
    char Txt[64];

    if (par_Rec->m_Record2Disk) {
        if (par_Rec->m_TriggerActiv) {
            if (!par_Rec->m_Triggered) {
                par_Rec->m_Triggered = (int)(direct_solve_equation_no_error_message (par_Rec->m_TriggerEquation)) != 0;
            }
        // wenn keine Trigger aktiv ist gleich mit der Aufzeichnug starten
        } else par_Rec->m_Triggered = 1;

        if (par_Rec->m_Triggered) {   // in File schreiben
            if (par_Rec->m_fh == NULL) {
                if ((par_Rec->m_fh = open_file (par_Rec->m_RecorderFileName, "wt")) == NULL) {
                   par_Rec-> m_Record2Disk = 0;
                    ThrowError (1, "cannot open file %s", par_Rec->m_RecorderFileName);
                    return -1;
                } else {
                    par_Rec->m_WaitForFirstMessageFlag = 1;
                    fprintf (par_Rec->m_fh, "Time\t");
                    //fprintf (m_fh, "Cycle\t");
                    fprintf (par_Rec->m_fh, "Dir\t");
                    fprintf (par_Rec->m_fh, "C\t");
                    fprintf (par_Rec->m_fh, "Id\t");
                    fprintf (par_Rec->m_fh, "Size\t");
                    fprintf (par_Rec->m_fh, "Data[0...7/63]\n");

                    par_Rec->m_CanFiFoHandle = CreateCanFifos (10000, ((uint32_t)-1 << 16) | 0x1); // do not lose any message ((uint32_t)-1 << 16)
                    par_Rec->m_AbtastPeriodeInMs = GetSchedulingPeriode () * 1000.0;
                    SetAcceptMask4CanFifo (par_Rec->m_CanFiFoHandle, par_Rec->m_cam, par_Rec->m_cam_count);
                    FlushMessageQueue (par_Rec);
                    goto __WRITE_A_LINE;
                }
            }
__WRITE_A_LINE:
            do {
                ReadElements = ReadCanFdMessagesFromFifo2Process (par_Rec->m_CanFiFoHandle, CANMessages, 64);   // max 64 CAN-Objekte lesen
                for (x = 0; x < ReadElements; x++) {
                    pCANMessage = &CANMessages[x];
                    Ts = TimestampCalc (par_Rec, pCANMessage);
                    if (par_Rec->m_WaitForFirstMessageFlag) {
#ifdef _WIN32
                        GetLocalTime (&(par_Rec->m_SystemTimeStartCANLogging));
                        par_Rec->m_WaitForFirstMessageFlag = 0;

                        par_Rec->m_TsOffset = (double)(par_Rec->m_SystemTimeStartCANLogging.wHour) * 3600000.0 +
                                     (double)(par_Rec->m_SystemTimeStartCANLogging.wMinute) * 60000.0 +
                                     (double)(par_Rec->m_SystemTimeStartCANLogging.wSecond) * 1000.0 +
                                     (double)(par_Rec->m_SystemTimeStartCANLogging.wMilliseconds);
#else
                        clock_gettime(CLOCK_MONOTONIC, &par_Rec->m_SystemTimeStartCANLogging);
                        par_Rec->m_TsOffset = par_Rec->m_SystemTimeStartCANLogging.tv_sec * 1000.0 +
                                              par_Rec->m_SystemTimeStartCANLogging.tv_nsec / 1000000.0;
#endif
                        par_Rec->m_TsOffset -= Ts;
                    }
                    BuildAbsoluteTimeString (par_Rec, Ts, Txt);
                    fprintf (par_Rec->m_fh, "%s\t", Txt);
                    //fprintf (m_fh, "%.3f\t", Ts);
                    fprintf (par_Rec->m_fh, "%s\t", (pCANMessage->node) ? "->" : "<-");
                    //fprintf (m_fh, "%i\t", (int)CANMessage.timestamp);
                    fprintf (par_Rec->m_fh, "%i\t", (int)(pCANMessage->channel));
                    fprintf (par_Rec->m_fh, "0x%X", (int)(pCANMessage->id));
                    switch (pCANMessage->ext) {
                    case 0:
                        fprintf (par_Rec->m_fh, "n\t");
                        break;
                    case 1:
                        fprintf (par_Rec->m_fh, "e\t");
                        break;
                    case 2:
                        fprintf (par_Rec->m_fh, "bn\t");
                        break;
                    case 3:
                        fprintf (par_Rec->m_fh, "be\t");
                        break;
                    case 4:
                        fprintf (par_Rec->m_fh, "fn\t");
                        break;
                    case 5:
                        fprintf (par_Rec->m_fh, "fe\t");
                        break;
                    case 6:
                        fprintf (par_Rec->m_fh, "fbn\t");
                        break;
                    case 7:
                        fprintf (par_Rec->m_fh, "fbe\t");
                        break;
                    }

                    fprintf (par_Rec->m_fh, "%i\t", (int)(pCANMessage->size));

                    for (i = 0; (i < pCANMessage->size) && (i < 64); i++) {
                        fprintf (par_Rec->m_fh, "0x%02X ", (int)(pCANMessage->data[i]));
                    }
                    fprintf (par_Rec->m_fh, "\n");
                }
            } while (ReadElements == 64);   // repeat as long as received 64 messages
        }  // if (par_Rec->m_Triggered)
    } else if (par_Rec->m_fh != NULL) {
        close_file (par_Rec->m_fh);
        par_Rec->m_fh = NULL;
        DeleteCanFifos (par_Rec->m_CanFiFoHandle);
        return 1; // should be removed if added by remote control or Script
    }
    return 0;
}

static void StringCopy(char *Dst, const char *Src, int MaxC)
{
    int Len = (int)strlen(Src);
    if (Len >= (MaxC-1)) Len = MaxC-1;
    MEMCPY(Dst, Src, Len);
    Dst[Len] = 0;
}


struct CAN_RECORDER *SetupCanRecorder(const char *par_FileName,
                                      const char *par_TriggerEqu,
                                      int par_DisplayColumnCounterFlag,
                                      int par_DisplayColumnTimeAbsoluteFlag,
                                      int par_DisplayColumnTimeDiffFlag,
                                      int par_DisplayColumnTimeDiffMinMaxFlag,
                                      CAN_ACCEPT_MASK *par_AcceptanceMasks,
                                      int par_AcceptanceMaskCount)
{
    struct CAN_RECORDER *Ret = my_calloc(1, sizeof(CAN_RECORDER));
    if (Ret == NULL) return NULL;
    StringCopy(Ret->m_RecorderFileName, par_FileName, sizeof(Ret->m_RecorderFileName));
    if ((par_TriggerEqu != NULL) && (strlen(par_TriggerEqu) > 0)) {
        StringCopy(Ret->m_TriggerEquation, par_TriggerEqu, sizeof(Ret->m_TriggerEquation));
        Ret->m_TriggerActiv = 1;
    } else {
        Ret->m_TriggerActiv = 0;
    }
    if (par_AcceptanceMaskCount > MAX_CAN_ACCEPTANCE_MASK_ELEMENTS) par_AcceptanceMaskCount = MAX_CAN_ACCEPTANCE_MASK_ELEMENTS;
    MEMCPY(Ret->m_cam, par_AcceptanceMasks, sizeof(CAN_ACCEPT_MASK) * par_AcceptanceMaskCount);   // the last must be chanel = -1
    Ret->m_cam[par_AcceptanceMaskCount].Channel = -1;
    Ret->m_cam_count = par_AcceptanceMaskCount;
    Ret->m_DisplayColumnCounterFlag = par_DisplayColumnCounterFlag;
    Ret->m_DisplayColumnTimeAbsoluteFlag = par_DisplayColumnTimeAbsoluteFlag;
    Ret->m_DisplayColumnTimeDiffFlag = par_DisplayColumnTimeDiffFlag;
    Ret->m_DisplayColumnTimeDiffMinMaxFlag = par_DisplayColumnTimeDiffMinMaxFlag;

    Ret->m_WaitForFirstMessageFlag = 0;
    Ret->m_TsOffset = 0.0;

    Ret->m_Record2Disk = 1;

    return Ret;
}


void StopCanRecorder(struct CAN_RECORDER *par_Rec)
{
    par_Rec->m_Record2Disk = 0;
}

void FreeCanRecorder(struct CAN_RECORDER *par_Rec)
{
    my_free(par_Rec);
}

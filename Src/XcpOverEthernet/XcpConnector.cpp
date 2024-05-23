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


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#else
#include "Platform.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

extern "C" {
    #include "MyMemory.h"
    #include "StringMaxChar.h"
    #include "ThrowError.h"
}
#include "XcpOverEthernet.h"
#include "XcpCopyPages.h"
#include "XcpConnector.h"

#define UNUSED(x) (void)(x)

typedef struct {
    const char *CmdName;
    unsigned char CmdNr;
} XCP_CMD_STRING;

static XCP_CMD_STRING XcpCmdStrings[256] =
{{"CONNECT",                0xFF},
 {"DISCONNECT",             0xFE},
 {"GET_STATUS",             0xFD},
 {"SYNCH",                  0xFC},
 {"GET_COMM_MODE_INFO",     0xFB},
 {"GET_ID",                 0xFA},
 {"SET_REQUEST",            0xF9},
 {"GET_SEED",               0xF8},
 {"UNLOCK",                 0xF7},
 {"SET_MTA",                0xF6},
 {"UPLOAD",                 0xF5},
 {"SHORT_UPLOAD",           0xF4},
 {"BUILD_CHECKSUM",         0xF3},
 {"TRANSPORT_LAYER_CMD",    0xF2},
 {"USER_CMD",               0xF1},
 {"DOWNLOAD",               0xF0},
 {"DOWNLOAD_NEXT",          0xEF},
 {"DOWNLOAD_MAX",           0xEE},
 {"SHORT_DOWNLOAD",         0xED},
 {"MODIFY_BITS",            0xEC},
 {"SET_CAL_PAGE",           0xEB},
 {"GET_CAL_PAGE",           0xEA},
 {"GET_PAG_PROCESSOR_INFO", 0xE9},
 {"GET_SEGMENT_INFO",       0xE8},
 {"GET_PAGE_INFO",          0xE7},
 {"SET_SEGMENT_MODE",       0xE6},
 {"GET_SEGMENT_MODE",       0xE5},
 {"COPY_CAL_PAGE",          0xE4},
 {"CLEAR_DAQ_LIST",         0xE3},
 {"SET_DAQ_PTR",            0xE2},
 {"WRITE_DAQ",              0xE1},
 {"SET_DAQ_LIST_MODE",      0xE0},
 {"GET_DAQ_LIST_MODE",      0xDF},
 {"START_STOP_DAQ_LIST",    0xDE},
 {"START_STOP_SYNCH",       0xDD},
 {"GET_DAQ_CLOCK",          0xDC},
 {"READ_DAQ",               0xDB},
 {"GET_DAQ_PROCESSOR_INFO", 0xDA},
 {"GET_DAQ_RESOLUTION_INFO",0xD9},
 {"GET_DAQ_LIST_INFO",      0xD8},
 {"GET_DAQ_EVENT_INFO",     0xD7},
 {"FREE_DAQ",               0xD6},
 {"ALLOC_DAQ",              0xD5},
 {"ALLOC_ODT",              0xD4},
 {"ALLOC_ODT_ENTRY",        0xD3},
 {"PROGRAM_START",          0xD2},
 {"PROGRAM_CLEAR",          0xD1},
 {"PROGRAM",                0xD0},
 {"PROGRAM_RESET",          0xCF},
 {"GET_PGM_PROCESSOR_INFO", 0xCE},
 {"GET_SECTOR_INFO",        0xCD},
 {"PROGRAM_PREPARE",        0xCC},
 {"PROGRAM_FORMAT",         0xCB},
 {"PROGRAM_NEXT",           0xCA},
 {"PROGRAM_MAX",            0xC9},
 {"PROGRAM_VERIFY",         0xC8}};

static XCP_CMD_STRING XcpRspStrings[256] =
{{"PID_RES",                0xFF},
 {"PID_ERR",                0xFE},
 {"PID_EV",                 0xFD},
 {"PID_SERV",               0xFC}
};

const char    swIdentification[] = "XilEnv";
cXCPConnector::cXCPConnector(void)
{
#ifdef _WIN32
    memset(&m_wsa,0,sizeof(m_wsa));
    memset(&m_addr,0,sizeof(m_addr));
#endif
    m_socket = 0;
    m_LoggingFlag = false;
    m_SendState = XCP_TR_IDLE;
    m_ResourceProtectionState = 0;
    m_Port = COMMUNICATION_PORT;

    //Segmant /Page Handling
    //System has Segment 0  = Datasegment with 2 Pages (Work /Reference)
    //and Segment 1 = Code Segment
    m_AmountSegments = 2;
    m_ActivePage = 0;

    //MTA
    m_CurMTA = 0;
    m_CurMTA64 = 0;
    m_CurMTAExtension = 0;
    m_CurMTATransferred = 0;
    m_CurMTAIsNotTragetAddr = 0;

    //Initialize Identification string
    strcpy(m_Identification,swIdentification);

    //Initialize Logfile
    strcpy(m_Logfile,"c:\\temp\\XilEnvXcp.log");
    m_fh = nullptr;

    //Initializes DAQ Memory
    InitDAQ();

    //Initialize Eventlist
    memset(&m_ptrEvents,0,sizeof(m_ptrEvents));
    memset(&m_EventCycleCounters,0,sizeof(m_EventCycleCounters));
    m_AllocatedEvents = 0;

    //Initialize Request/Response
    memset(&m_Request,0,sizeof(m_Request));
    memset(&m_Response,0,sizeof(m_Response));
    m_RspPackageCounter = 0;
    m_ExpectedElements = 0;
    m_ReceivedElements = 0;

    m_DataSegment = nullptr;
    m_CodeSegment = nullptr;
    m_DllAddressTranslation = false;
    m_TerminateCommunicateThreadFlag = false;
}

cXCPConnector::~cXCPConnector(void)
{
    if (m_DataSegment != nullptr) my_free (m_DataSegment);
    m_DataSegment = nullptr;
    if (m_CodeSegment != nullptr) my_free (m_CodeSegment);
    m_CodeSegment = nullptr;
#ifdef XCP_COMMUNICATION_THREAD
    TerminateCommunicateThreadFunction();
#endif
}

void cXCPConnector::Cyclic (void)
{
    int x;

    m_DaqClock++;
    for (x = 0; x < m_AllocatedEvents; x++) {
        m_EventCycleCounters[x]++;
        if (m_EventCycleCounters[x] >= m_ptrEvents[x].timeCycle) {
            XcpEvent (static_cast<unsigned char>(x));
            m_EventCycleCounters[x] = 0;
        }
    }
}

#ifdef XCP_COMMUNICATION_THREAD
int cXCPConnector::CommunicateThreadFunction (cXCPConnector *ThisPtr)
{
#ifdef _WIN32
    HANDLE WaitForHandles[4];

    WaitForHandles[0] = ThisPtr->m_TerminateCommunicateThreadEvent;
    WaitForHandles[1] = CreateEvent (nullptr, FALSE, FALSE, nullptr);
    WaitForHandles[2] = ThisPtr->m_SendEvent;
    WaitForHandles[3] = ThisPtr->m_DoCycleEventHandle;
    WSAEventSelect (ThisPtr->m_socket, WaitForHandles[1], FD_READ|FD_WRITE);

    while (1) {
        switch (WaitForMultipleObjectsEx (4, WaitForHandles, FALSE, 10000000, FALSE)) {
        case WAIT_OBJECT_0 + 3:
            ResetEvent (ThisPtr->m_DoCycleEventHandle);
            ThisPtr->Cyclic ();
            SetEvent (ThisPtr->m_hCycleFinishedEventHandle);
            break;
        case WAIT_OBJECT_0 + 0:
            CloseHandle (WaitForHandles[1]);
            ThisPtr->Terminate ();
            ThisPtr->m_TerminateCommunicateThreadFlag = 0;
            ExitThread (0);
            //return 0;
            //break;
        case WAIT_OBJECT_0 + 2:
            ResetEvent (ThisPtr->m_SendEvent);
        case WAIT_TIMEOUT:
        case WAIT_OBJECT_0 + 1:
            EnterCriticalSection (&ThisPtr->m_CriticalSection);
            ThisPtr->InnerCommunicate();
            LeaveCriticalSection (&ThisPtr->m_CriticalSection);
            break;
        case WAIT_FAILED:
            char *ErrString = GetLastSysErrorString ();
            ThrowError (1, "WaitForMultipleObjectsEx failed %i \"%s\"\n", GetLastError (), ErrString);
            FreeLastSysErrorString (ErrString);
            break;
        }
    }
#else
    return 0;
#endif
}

bool cXCPConnector::StartCommunicateThreadFunction (void)
{
#ifdef _WIN32
    InitializeCriticalSection (&m_CriticalSection);
    m_TerminateCommunicateThreadEvent = CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (m_TerminateCommunicateThreadEvent == nullptr) return false;
    m_SendEvent = CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (m_SendEvent == nullptr) return false;
    m_DoCycleEventHandle = CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (m_DoCycleEventHandle == nullptr) return false;
    m_hCycleFinishedEventHandle = CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (m_hCycleFinishedEventHandle == nullptr) return false;

    m_CommunicateThreadHandle = CreateThread (nullptr, 64*1024,
                                             reinterpret_cast<LPTHREAD_START_ROUTINE>(CommunicateThreadFunction),
                                             static_cast<void*>(this), 0, &m_CommunicateThreadId);
    if (m_CommunicateThreadHandle == nullptr) {
        CloseHandle (m_TerminateCommunicateThreadEvent);
        CloseHandle (m_SendEvent);
        DeleteCriticalSection (&m_CriticalSection);
        return false;
    }
    m_TerminateCommunicateThreadFlag = true;
#else
#endif
    return true;
}

bool cXCPConnector::TerminateCommunicateThreadFunction (void)
{
#ifdef _WIN32
    if (m_TerminateCommunicateThreadFlag) {
        SetEvent (m_TerminateCommunicateThreadEvent);
        WaitForSingleObject (m_CommunicateThreadHandle, 2000);
        while (m_TerminateCommunicateThreadFlag) {
            Sleep(10);
        }
        CloseHandle (m_TerminateCommunicateThreadEvent);
        CloseHandle (m_SendEvent);
        CloseHandle (m_DoCycleEventHandle);
        CloseHandle (m_hCycleFinishedEventHandle);
        DeleteCriticalSection (&m_CriticalSection);
    }
#else
#endif
    return true;
}

#endif

void cXCPConnector::SetIdentificationString(char* cpId)
{
    if (strlen(cpId) > (MAX_ID_LEN -1)) {
        //Given Id too long => cut to accepted len
        memset(m_Identification,0,sizeof(m_Identification));
        MEMCPY(m_Identification,cpId,(sizeof(m_Identification)-1));
    } else {
        strcpy(m_Identification,cpId);
    }
}

void cXCPConnector::SetLogfile(char* cpLogfile)
{
    if (m_fh != nullptr) fclose (m_fh);
    m_fh = nullptr;
    if (strlen(cpLogfile) > (sizeof(m_Logfile)-1)) {
        //Given Id too long => cut to accepted len
        memset(m_Logfile,0,sizeof(m_Logfile));
        MEMCPY(m_Logfile,cpLogfile,(sizeof(m_Logfile)-1));
    } else {
        strcpy(m_Logfile,cpLogfile);
    }
}

int cXCPConnector::SwitchPagesAllSegments(int Page)
{
    if (m_LoggingFlag) WriteLogString("Switch all Segments to page %i", Page);
    for (int s = 0; s < m_CopyPages.get_number_of_sections(); s++) {
        unsigned char NoPages;
        unsigned char A2lSegmentNo;
        unsigned char ExeSegmentNo;
        char *SectionName = m_CopyPages.get_sections_name (s);

        if (m_LoggingFlag) WriteLogString("    is Segment \"%s\" callibationable", SectionName);
        if (IsSegmentCallibationAble(SectionName, &A2lSegmentNo, &ExeSegmentNo, &NoPages) == 1) {
            if (Page >= NoPages) {
                return -1;
            }
            if (m_LoggingFlag) WriteLogString("      Switch Segment \"%s\" to page %i", SectionName, Page);
            if (m_CopyPages.switch_page_of_section (A2lSegmentNo, ExeSegmentNo, m_ActivePage, Page, this)) {
                PrepareErrResponse(ERR_OUT_OF_RANGE);
                return -1;
            }
        }
    }
    if (m_LoggingFlag) WriteLogString("Switch all Segments to page %i done", Page);
    return 0;
}

bool cXCPConnector::Init(int LinkNr, unsigned short Port, char *Identification, char *ProcessName, unsigned char ucAmEvents,XCP_EVENT_DESC* ptrEvents,
                         char *DataSegment, char *CodeSegment, int LoggingActive, char *LogFile)
{
    int iRetCode;

    //Set disconnection
    HandleDisconnect();

    //Get Eventlist and store local
    if (m_ptrEvents) {
      delete [] m_ptrEvents;
    }
    if (m_EventCycleCounters) {
      delete [] m_EventCycleCounters;
    }
    m_LinkNr = LinkNr;
    m_Port = Port;
    strcpy (m_Identification, Identification);
    strcpy (m_ProcessName, ProcessName);

    strcpy(m_Logfile,LogFile);
    m_LoggingFlag = (LoggingActive != 0);

    m_ptrEvents = new XCP_EVENT_DESC[ucAmEvents];
    m_EventCycleCounters = new int[ucAmEvents];
    if ((m_ptrEvents == nullptr) || (m_EventCycleCounters == nullptr)) {
        return false;
    }
    memset(m_ptrEvents,0,ucAmEvents*sizeof(XCP_EVENT_DESC));
    memset(m_EventCycleCounters,0,ucAmEvents*sizeof(int));
    MEMCPY(m_ptrEvents,ptrEvents,ucAmEvents*sizeof(XCP_EVENT_DESC));
    m_AllocatedEvents = ucAmEvents;

    //Initialize Rsp. Package Counter
    m_RspPackageCounter = 0;

    //Handling OK?
    if (m_socket != 0)
    {
      //Failure => Trace and return
      ThrowError (1, "Fehler: Init Methode wird mehrfach aufgerufen!");
      return false;
    }
#ifdef _WIN32
    iRetCode = WSAStartup(MAKEWORD(2,0),&m_wsa);
    if (iRetCode != 0) {
        //Failure => Trace and return
        char *ErrString = GetLastSysErrorString ();
        ThrowError (1, "Fehler: Das Socket Framework konnte nicht initialisiert werden! \r\n Fehler code: %d \"%s\"\n",iRetCode, ErrString);
        FreeLastSysErrorString (ErrString);
        return false;
    }

    m_socket =socket(AF_INET,SOCK_DGRAM,0);
    if(m_socket == INVALID_SOCKET) {
        char *ErrString = GetLastSysErrorString ();
        ThrowError (1, "Fehler: Der Socket konnte nicht erstellt werden!\r\n Fehler code: %d \"%s\"\n",WSAGetLastError(), ErrString);
        FreeLastSysErrorString (ErrString);
        return false;
    }

    m_addr.sin_family=AF_INET;
    m_addr.sin_port=htons(m_Port);
    m_addr.sin_addr.s_addr=ADDR_ANY;
    iRetCode = bind(m_socket,reinterpret_cast<SOCKADDR*>(&m_addr),sizeof(SOCKADDR_IN));
    if(iRetCode == SOCKET_ERROR) {
        char *ErrString = GetLastSysErrorString ();
        ThrowError(1, "Fehler: bind, fehler code: %d \"%s\"\n",WSAGetLastError(), ErrString);
        FreeLastSysErrorString (ErrString);
        return false;
    }
#else
#endif
    //OK, Socket is completely initialized => wait for incomming telegramms
    m_SocketInitialized = true;

    if (DataSegment == nullptr) {
        m_DataSegment = nullptr;
    } else {
        m_DataSegment = static_cast<char*>(my_malloc (strlen (DataSegment) + 1));
        strcpy (m_DataSegment, DataSegment);
    }
    if (CodeSegment == nullptr) {
        m_CodeSegment = nullptr;
    } else {
        m_CodeSegment = static_cast<char*>(my_malloc (strlen (CodeSegment) + 1));
        strcpy (m_CodeSegment, CodeSegment);
    }
    m_LoopSend = 0;
    m_WaitForNextCommand = 0;

#ifdef XCP_COMMUNICATION_THREAD
    return StartCommunicateThreadFunction ();
#else
    return true;
#endif
}

void cXCPConnector::Terminate(void)
{
    //Just compute if socket is still initialized
    if (!m_SocketInitialized) {
        return;
    }

    //Reset statemachine
    m_SendState = XCP_TR_IDLE;

    //Send Termination Event
    HandleSendEvent2Server(EVC_SESSION_TERMINATED);
    Send(false);

#ifdef _WIN32
    //Wait short time till event was sent
    Sleep(200);

    //Close Sockets
    m_SocketInitialized = false;
    shutdown(m_socket,0xFFFF);
    closesocket(m_socket);
    WSACleanup();
#else
#endif
  //Reset DAQ dynamic memory
  ResetDynamicDAQMemory();
}

bool cXCPConnector::ReInit(void)
{
    FreeDAQList ();

    //Just compute if socket is still initialized
    if (!m_SocketInitialized) {
        return false;
    }

    //Reset statemachine
    m_SendState = XCP_TR_IDLE;

    return true;
}

bool cXCPConnector::InnerCommunicate(void)
{
    bool Ret;

    if (!m_SocketInitialized) {
        return false;
    }

    //Receive Data
    Ret = Receive();

    //Send Data
    do {
        if (!Send(true)) {
            return false;
        }
    } while (m_LoopSend);

    return Ret;
}

void cXCPConnector::IncrementCycleCounter(void)
{
#ifdef _WIN32
    SetEvent (m_DoCycleEventHandle);
    WaitForSingleObject (m_hCycleFinishedEventHandle, INFINITE);
    ResetEvent (m_hCycleFinishedEventHandle);
#else
#endif
}

bool cXCPConnector::Communicate(void)
{
    if (!m_SocketInitialized) {
        return false;
    }

    //Increment Timestamp
    m_DaqClock++;

#ifndef XCP_COMMUNICATION_THREAD
    bool Ret = true;
    m_WaitForNextCommand = 0;
    StartTime = GetTickCount();
    do {
        Ret = InnerCommunicate();
        if (m_WaitForNextCommand) {
            if (Ret) {
                StartTime = GetTickCount();
            } else if ((GetTickCount() - StartTime) > 100) {
                m_WaitForNextCommand = 0;
                break;
            }
        }
    } while (m_WaitForNextCommand || Ret);
    return Ret;
#else 
    return true;
#endif
}

void cXCPConnector::XcpEvent(unsigned char ucEvent)
{
    unsigned short  curDAQList = 0;
#ifdef XCP_COMMUNICATION_THREAD
    EnterCriticalSection (&m_CriticalSection);
#endif
    while (curDAQList < m_dynAllocatedDAQs) {
        //Find started DAQs assigned to that event?
        if (m_Daq[curDAQList].m_EventChannel == ucEvent &&
            m_Daq[curDAQList].m_State == DAQ_STARTED) {
            //OK, started DAQ to this event found
            //Sample Data
            PrepareDaqTransmission(curDAQList);

            //Check, if DTO Response has to be sended
            //Increment cycles and compare to prescaler
            m_Daq[curDAQList].m_Cycle++;
            if (m_Daq[curDAQList].m_Cycle >= m_Daq[curDAQList].m_Prescaler) {
                //Respond
                RespondDAQ(curDAQList);

                //Reset Cycle Counter
                m_Daq[curDAQList].m_Cycle = 0;
            }
        }
        curDAQList++;
    }
#ifdef XCP_COMMUNICATION_THREAD
   LeaveCriticalSection (&m_CriticalSection);
#endif
}

void cXCPConnector::RespondDAQ(unsigned short ucDaqList)
{
#ifdef _WIN32
    unsigned char         ucCurODT;
    unsigned char         ucStartODT;
    int                   remoteAddrLen=sizeof(SOCKADDR_IN);
    XCP_DAQ_RESPONSE      CurrentResponse;
    unsigned short        bDataLen;
    int                   iRetCode;

    //Validate given DAQ List
    if (ucDaqList < m_dynAllocatedDAQs) {
        if (m_Daq[ucDaqList].m_Sampled == true) {
            ucStartODT = static_cast<unsigned char>(ucDaqList << 4);

            //Prepare Response for all ODTs of the selected DAQ
            ucCurODT = 0;
            while (ucCurODT < m_Daq[ucDaqList].m_allocatedODTs) {
                //Prepare send data
                CurrentResponse.usCounter = m_RspPackageCounter;
                bDataLen = m_Daq[ucDaqList].m_PtrODTs[ucCurODT]->m_DAQTransferFieldLen;

                //Prepare PID
                CurrentResponse.xcpResponse.bData[0] = ucCurODT+ucStartODT;

                //Timestamp switched on?
                if (m_Daq[ucDaqList].m_ListMode & DAQ_PROPERTY_TIMESTAMP && ucCurODT == 0) {
                    //With Timestamp -> we use fix the 2 Byte timestamp in the first response package (ODT) of this DAQ
                    MEMCPY(&CurrentResponse.xcpResponse.bData[1],&m_DaqClock,sizeof(m_DaqClock));
                    MEMCPY(&CurrentResponse.xcpResponse.bData[1+sizeof(m_DaqClock)],m_Daq[ucDaqList].m_PtrODTs[ucCurODT]->m_DAQTransferField,bDataLen);
                    CurrentResponse.usLen = bDataLen + 1 + sizeof(m_DaqClock);
                } else {
                    //without Timestamp
                    //Copy data
                    MEMCPY(&CurrentResponse.xcpResponse.bData[1],m_Daq[ucDaqList].m_PtrODTs[ucCurODT]->m_DAQTransferField,bDataLen);
                    CurrentResponse.usLen = bDataLen + 1;
                }
                //Send the latest Response
                iRetCode = sendto(m_socket,reinterpret_cast<const char*>(&CurrentResponse),CurrentResponse.usLen + sizeof(XCP_OVER_IP_HEADER),0,
                                  reinterpret_cast<SOCKADDR*>(&m_remoteAddr),remoteAddrLen);
                if (iRetCode == SOCKET_ERROR) {
                    //Fehler beim Senden von Daten
                    ThrowError (1, "Fehler: sendto, fehler code: %d\n",WSAGetLastError());
                    //Delete Msg
                    m_Response.usLen = 0;
                    Terminate();
                    return;
                } else {
                    //Increment DAQ Package Counter
                    m_RspPackageCounter++;
                }

                //Log outgoing Data
                if (m_LoggingFlag) {
                  WriteLog ("XilEnv->Eth", 1,nullptr,false,reinterpret_cast<XCP_DAQ_RESPONSE*>(&CurrentResponse));
                }
                ucCurODT++;
            }
        }
    }
#else
#endif
    return;
}

bool cXCPConnector::Send(bool bLog)
{
#ifdef _WIN32
    int                   iRetCode;
    int                   remoteAddrLen=sizeof(SOCKADDR_IN);
    XCP_BASIC_RESPONSE    CurrentResponse;
    unsigned char         bcurLen = 0;
    bool                  bSend = false;

    //Pre- Send actions
    switch (m_SendState) {
    default:
    case XCP_TR_IDLE:
        break;
    case XCP_TR_RESP_CTO:
        // Construct the request and transmit
        MEMCPY(&CurrentResponse,&m_Response,sizeof(m_Response));
        CurrentResponse.usCounter = m_RspPackageCounter;
        bSend = true;
        break;
    case XCP_TR_SEND_MTA_BLOCK:
        // Construct the request and transmit
        bcurLen = m_Request.xcpRequest.b[1];

        m_WaitForNextCommand = 1;

        if ((static_cast<int>(bcurLen) - static_cast<int>(m_CurMTATransferred)) > (MAX_CTO - 1)) {
            bcurLen =  static_cast<unsigned char>((MAX_CTO - 1));
#ifdef XCP_COMMUNICATION_THREAD
            SetEvent (m_SendEvent);
#else
        m_LoopSend = 1;
#endif

        } else {
            bcurLen =  static_cast<unsigned char>(bcurLen - m_CurMTATransferred);
    #ifndef XCP_COMMUNICATION_THREAD
            m_LoopSend = 0;
#endif
        }
        CurrentResponse.xcpResponse.b[0] = PID_RES;
        CurrentResponse.usLen = bcurLen + 1;

        if (m_CurMTAIsNotTragetAddr) {
            char *m_CurMTA64Char = reinterpret_cast<char*>(m_CurMTA64);
            // It is not a address of an external process
            if ((m_CurMTA64Char >= m_Identification) && (m_CurMTA64Char < (m_Identification + sizeof (m_Identification)))) {
                // It is an identifier string m_CurMTA was set with GET_ID before
                MEMCPY (&CurrentResponse.xcpResponse.b[1],m_CurMTA64Char,bcurLen);
            } else {
                int e;
                for (e = 0; e < m_AllocatedEvents; e++) {
                    if (m_ptrEvents[e].EventName != nullptr) {
                        if ((m_CurMTA64Char >= m_ptrEvents[e].EventName) && (m_CurMTA64Char < (m_ptrEvents[e].EventName + strlen (m_ptrEvents[e].EventName) + 1))) {
                            // It is an event name m_CurMTA was set with GET_DAQ_EVENT_INFO before
                            MEMCPY (&CurrentResponse.xcpResponse.b[1],m_CurMTA64Char,bcurLen);
                            break;
                        }
                    }
                }
                if (e == m_AllocatedEvents) {
                    ThrowError (1, "try to upload data without setting MTA");
                    memset (&CurrentResponse.xcpResponse.b[1],0,bcurLen);
                }
            }
        } else {
            XCPReadMemoryFromExternProcess (m_LinkNr, m_CurMTA, static_cast<void*>(&CurrentResponse.xcpResponse.b[1]), bcurLen);
        }
        CurrentResponse.usCounter = m_RspPackageCounter;
        bSend = true;
        break;
    } // switch

    if (bSend) {
        //Send the latest Response
        iRetCode = sendto(m_socket,reinterpret_cast<const char*>(&CurrentResponse),CurrentResponse.usLen + sizeof(XCP_OVER_IP_HEADER),0,
                          reinterpret_cast<SOCKADDR*>(&m_remoteAddr),remoteAddrLen);
        if (iRetCode == SOCKET_ERROR && bLog) {
            // Error during send
            ThrowError (1, "Fehler: sendto, fehler code: %d\n",WSAGetLastError());
            // delete message
            m_Response.usLen = 0;
            return false;
        }

        //Increment response package counter
        m_RspPackageCounter++;

        // Log outgoing data
        if (m_LoggingFlag && bLog) {
            WriteLog ("XilEnv->Eth", 1,reinterpret_cast<XCP_BASIC_REQUEST*>(&CurrentResponse),false);
        }

        // Post send actions
        switch (m_SendState) {
        case XCP_TR_RESP_CTO: /* Statndard response to CTO Request */
            if (!m_CurMTA) {
                memset(&m_Response,0,sizeof(m_Response));
            }
            m_SendState = XCP_TR_IDLE;
            break;
        case XCP_TR_SEND_MTA_BLOCK:
            //Increment MTA
            m_CurMTA += bcurLen;
            m_CurMTATransferred += (bcurLen);

            bcurLen = m_Request.xcpRequest.b[1];
            if (m_CurMTATransferred >= static_cast<uint32_t>(bcurLen)) {
                // All data sended! => go to idle
                m_SendState = XCP_TR_IDLE;
                memset(&m_Response,0,sizeof(m_Response));
                m_CurMTATransferred = 0;
            }
            break;
        }
    }
#else
#endif
    return true;
}

bool cXCPConnector::Process(unsigned char* ptrTelegram, unsigned short usTelLen)
{
    UNUSED(usTelLen);
    switch (ptrTelegram[0]) {
    case CC_CONNECT:
        /* Incomming request was a connect -> response */
        PrepareConnectResponse();
        break;
    case CC_DISCONNECT:
        // Set all states to disconnection
        if (m_Connected) HandleDisconnect();
        /* Incomming request was a disconnect -> no response */
        break;
    case CC_GET_STATUS:
        if (m_Connected) PrepareCurrentStatus();
        break;
    case CC_GET_ID:
        if (m_Connected) PrepareIdentification();
        break;
    case CC_UPLOAD:
        if (m_Connected) {
            //Response object still exists => upload data
            m_SendState = XCP_TR_SEND_MTA_BLOCK;
        }
        break;
    case CC_SHORT_UPLOAD:
        if (m_Connected) ShortUpload();
        break;
    case CC_SHORT_DOWNLOAD:
        if (m_Connected) ShortDownload();
        break;
    case CC_SYNCH:
        // Server signals timeout => Reset communication
        if (m_Connected) PrepareErrResponse(ERR_CMD_SYNCH);
        break;
    case CC_GET_PAG_PROCESSOR_INFO:
        if (m_Connected) PreparePageProcessorInfo();
        break;
    case CC_GET_SEGMENT_INFO:
        if (m_Connected) PrepareSegmentInfo();
        break;
    case CC_SET_SEGMENT_MODE:
        if (m_Connected) TakeSegmentMode();
        break;
    case CC_GET_PAGE_INFO:
        if (m_Connected) PreparePageInfo();
        break;
    case CC_GET_CAL_PAGE:
        if (m_Connected) PrepareGetCalPage();
        break;
    case CC_SET_CAL_PAGE:
        if (m_Connected) TakeCalibrationPage();
        break;
    case CC_SET_MTA:
        if (m_Connected) TakeCurrentMTA();
        break;
    case CC_BUILD_CHECKSUM:
        if (m_Connected) PrepareChecksum();
        break;
    //Daq
    case CC_GET_DAQ_PROCESSOR_INFO:
        if (m_Connected) PrepareDAQProcessorInfo();
        break;
    case CC_DOWNLOAD:
        if (m_Connected) TakeFirstBlockOfDownloadData();
        break;
    case CC_DOWNLOAD_NEXT:
        if (m_Connected) TakeNextBlockOfDownloadData();
        break;
    case CC_ALLOC_DAQ:
        if (m_Connected) AllocateDAQList();
        break;
    case CC_ALLOC_ODT:
        if (m_Connected) AllocateODT2DAQ();
        break;
    case CC_ALLOC_ODT_ENTRY:
        if (m_Connected) AllocateODTEntry2DAQ();
        break;
    case CC_CLEAR_DAQ_LIST:
        if (m_Connected) ClearDAQList();
        break;
    case CC_FREE_DAQ:
        if (m_Connected) FreeDAQList();
        break;
    case CC_SET_DAQ_PTR:
        if (m_Connected) SetDAQPointer();
        break;
    case CC_WRITE_DAQ:
        if (m_Connected) WriteElement2ODTEntry();
        break;
    case CC_SET_DAQ_LIST_MODE:
        if (m_Connected) SetDAQListMode();
        break;
    case CC_GET_DAQ_LIST_MODE:
        if (m_Connected) HandleGetDAQListMode();
        break;
    case CC_START_STOP_DAQ_LIST:
        if (m_Connected) HandleStartStopDAQList();
        break;
    case CC_START_STOP_SYNCH:
        if (m_Connected) HandleStartStopDAQListSynch();
        break;
    case CC_GET_DAQ_RESOLUTION_INFO:
        if (m_Connected) PrepareDAQResolutionInfo();
        break;
    case CC_GET_DAQ_EVENT_INFO:
        if (m_Connected) PrepareDAQEventInfo();
        break;
    case CC_GET_DAQ_CLOCK:
        if (m_Connected) HandleGetDaqClock();
        break;
    case CC_COPY_CAL_PAGE:
        if (m_Connected) CopyCalPage();
        break;
    case CC_SET_REQUEST:
        if (m_Connected) SetRequest();
        break;
    default:
        if (m_Connected) PrepareErrResponse(ERR_CMD_UNKNOWN);
        break;
    }
    return TRUE;
}

bool cXCPConnector::Receive(void)
{
#ifdef _WIN32
    unsigned long         ulAmount;
    int                   iRetCode;
    unsigned char         Buffer[512];
    int                   remoteAddrLen=sizeof(SOCKADDR_IN);
    XCP_OVER_IP_HEADER    strHeader;
    BOOL                  telReceived = FALSE;
    bool                  Ret = false;    // This is true if at least one packet was received successful

    //Initialize Data
    memset(&strHeader,0,sizeof(strHeader));

    //Receive complete available inputs
    while (!telReceived) {
        if (ioctlsocket(m_socket, FIONREAD, &ulAmount)) {
            ThrowError (1, "Fehler: ioctlsocket, fehler code: %d\n",WSAGetLastError());
            Terminate();
            return 0;
        }
        if (ulAmount == 0) {
            //No input data available
            return Ret;
        }
        //Receive Data
        iRetCode = recvfrom (m_socket, (char*)Buffer, sizeof (Buffer), 0, reinterpret_cast<SOCKADDR*>(&m_remoteAddr), &remoteAddrLen);
        if (iRetCode == SOCKET_ERROR) {
            HandleDisconnect();
            ReInit ();
            return Ret;
        }

        //Check frame
        MEMCPY(&strHeader,Buffer, sizeof(strHeader));
        if (strHeader.usLen <= (iRetCode - static_cast<int>(sizeof(strHeader)))) {
            //Telegream completely received
            //Memorize complete Request
            MEMCPY(&m_Request,Buffer,static_cast<size_t>(iRetCode));

            //Log incomming Data
            if (m_LoggingFlag) {
                WriteLog ("Eth->XilEnv", 1, &m_Request,true);
            }

            //Process information
            Process(&Buffer[4],strHeader.usLen);
            telReceived = TRUE;
            Ret = true;
        }
    }//while
    return Ret;
#else
    return FALSE;
#endif
}

void cXCPConnector::WriteLog (const char *Prefix, int DispCmd, XCP_BASIC_REQUEST *Buffer,bool bRequest,XCP_DAQ_RESPONSE *daqBuffer)
{
#ifdef _WIN32
    const char *CmdString;
    const char *Whitespaces;

    //File already open?
    if (m_fh == nullptr) {
        m_fh = fopen (m_Logfile, "wt");
    }

    if (m_fh != nullptr) {
        if (DispCmd) {
            Whitespaces = "";
            if (bRequest) {
                CmdString = XcpCmdStrings[0xFF - Buffer->xcpRequest.b[0]].CmdName;
            } else {
                if (Buffer) {
                    CmdString = XcpRspStrings[0xFF - Buffer->xcpRequest.b[0]].CmdName;
                } else {
                    //DAQ Response
                    CmdString = "DAQ Response";
                }
            }
            if (CmdString == nullptr) {
                CmdString = "unknown command";
            }
        } else {
            Whitespaces = "    ";
            CmdString = "";
        }

        //Append data
        if (Buffer == nullptr && daqBuffer) {
            //DAQ response
            fprintf (m_fh, "%s%i (%i) %010u %05i %05i %s(%i): %s  len=%i, ctr=%i, Data=[",
                     Whitespaces, m_LinkNr, static_cast<int>(m_Port),
                     static_cast<uint32_t>(GetTickCount()), daqBuffer->usCounter, m_DaqClock, Prefix,
                     static_cast<int>(m_remoteAddr.sin_port), CmdString, static_cast<int>(daqBuffer->usLen), 0/*Buffer->ctr*/);
            for (int x = 0; x < daqBuffer->usLen; x++) {
                if (x == (daqBuffer->usLen-1)) fprintf (m_fh, "%02X", static_cast<int>(daqBuffer->xcpResponse.bData[x]));
                else if ((x%4) == 3) fprintf (m_fh, "%02X  ", static_cast<int>(daqBuffer->xcpResponse.bData[x]));
                else if ((x%8) == 7) fprintf (m_fh, "%02X   ", static_cast<int>(daqBuffer->xcpResponse.bData[x]));
                else fprintf (m_fh, "%02X ", static_cast<int>(daqBuffer->xcpResponse.bData[x]));
            }
            fprintf (m_fh, "]\n");
        } else {
            //CTO Response
            fprintf (m_fh, "%s%i (%i) %010u %s(%i): %s  len=%i, ctr=%i, Data=[",
                     Whitespaces,
                     m_LinkNr,
                     static_cast<int>(m_Port),
                     static_cast<uint32_t>(GetTickCount()),
                     Prefix, static_cast<int>(m_remoteAddr.sin_port), CmdString, static_cast<int>(Buffer->usLen), 0);
            for (int x = 0; x < Buffer->usLen; x++) {
                if (x == (Buffer->usLen-1)) fprintf (m_fh, "%02X", static_cast<int>(Buffer->xcpRequest.b[x]));
                else if ((x%4) == 3) fprintf (m_fh, "%02X  ", static_cast<int>(Buffer->xcpRequest.b[x]));
                else if ((x%8) == 7) fprintf (m_fh, "%02X   ", static_cast<int>(Buffer->xcpRequest.b[x]));
                else fprintf (m_fh, "%02X ", static_cast<int>(Buffer->xcpRequest.b[x]));
            }
            fprintf (m_fh, "]\n");
        }
        fflush (m_fh);
    }
#else
#endif
}

void cXCPConnector::WriteLogString(const char *format, ...)
{
    va_list args;
    char buffer[4096];

    va_start (args, format);
    vsprintf (buffer, format, args);
    va_end (args);

    //File already open?
    if (m_fh == nullptr) {
        m_fh = fopen (m_Logfile, "wt");
    }
    if (m_fh != nullptr) {
        fprintf (m_fh, "#->%s\n", buffer);
    }
}

void cXCPConnector::WriteLogByteBuffer(unsigned char *Buffer, int Size)
{
    if (m_fh != nullptr) {
        fprintf (m_fh, "X->");
        for (int x = 0; x < Size; x++) {
            if (((x%16) == 15) && (x < (Size-1))) fprintf (m_fh, "%02X\n   ", static_cast<int>(Buffer[x]));
            else if ((x%8) == 7) fprintf (m_fh, "%02X  ", static_cast<int>(Buffer[x]));
            else if ((x%4) == 3) fprintf (m_fh, "%02X   ", static_cast<int>(Buffer[x]));
            else fprintf (m_fh, "%02X ", static_cast<int>(Buffer[x]));
        }
        fprintf (m_fh, "]\n");
    }
}

void cXCPConnector::HandleDisconnect()
{
    if (m_Connected) {
        DisconnectFronProcess (m_LinkNr);
    }
    //Reset session status
    m_SessionStatus = 0;
    m_Connected = 0;
    if (m_fh != nullptr) fclose (m_fh);
    m_fh = nullptr;
}

void cXCPConnector::PrepareConnectResponse()
{
    if (!m_Connected) {
        if (ConnectToProcess (m_LinkNr, &m_Pid, m_ProcessName) != 0) {
            return;
        }
        m_CopyPages.SetProcess (m_Pid, m_LinkNr);
        // Delete all page files if there are exist any
        m_CopyPages.delete_all_page_files (); 
        // Than create new one
        for (int s = 0; s < m_CopyPages.get_number_of_sections (); s++)  {
            unsigned char NoPages;
            unsigned char A2lSegmentNo;
            unsigned char ExeSegmentNo;
            if (IsSegmentCallibationAble (m_CopyPages.get_sections_name (s), &A2lSegmentNo, &ExeSegmentNo, &NoPages) == 1) {
                for (int p = 0; p < NoPages; p++) {
                    m_CopyPages.AddPageToCallibationSegment (static_cast<unsigned char>(A2lSegmentNo), static_cast<unsigned char>(p), m_CopyPages.get_sections_size (s), m_CopyPages.get_sections_start_address (s));
                }
            }
        }

        //Set application to connection state
        m_SessionStatus = SS_CONNECTED;
        m_Connected = 1;
    }
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Copy response
    m_Response.usLen = 8;
    m_Response.usCounter = m_Request.usCounter;
  
    //Fuellen des Response
    m_Response.xcpResponse.b[0] = PID_RES;
  
    //supported Resources, see projectspecific configuration
    m_Response.xcpResponse.b[1] = projSetResources;

    //Comm Mode baisc
    //supported Basic Comm settings, see projectspecific configuration
    m_Response.xcpResponse.b[2] = projSetBasicCommMode;

    //MAX CTO and DSTO size in BYTE
    m_Response.xcpResponse.b[3] = MAX_CTO;
    m_Response.xcpResponse.w[2] = MAX_DTO;

    //XCP Protocoll Layer Version number
    m_Response.xcpResponse.b[6] = xcpProtocollLayerVersion;

    //XCP Transport Layer Version number
    m_Response.xcpResponse.b[7] = xcpTransportLayerVersion;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareCurrentStatus()
{
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data
    m_Response.usLen = 6;
    m_Response.usCounter = m_Request.usCounter;

    //Fuellen des Response
    m_Response.xcpResponse.b[0] = PID_RES;

    //Current session status
    m_Response.xcpResponse.b[1] = m_SessionStatus;

    //Current resource protection status
    m_Response.xcpResponse.b[2] = m_ResourceProtectionState;

    //Reserved

    //Session configuration ID

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareIdentification()
{
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data
    m_Response.usLen = 8; //+ strlen(swIdentification)+1;
    m_Response.usCounter = m_Request.usCounter;

    //Fuellen des Response
    m_Response.xcpResponse.b[0] = PID_RES;

    //ASCII Repraesentation
    m_Response.xcpResponse.b[1] = IDT_ASCII;

    //Reserved 2 Bytes

    //length
    m_Response.xcpResponse.dw[1] = static_cast<uint32_t>(strlen(m_Identification));

    //Copy Identification string
    m_CurMTA64 = reinterpret_cast<uint64_t>(m_Identification);
    m_CurMTATransferred = 0;

    m_CurMTAIsNotTragetAddr = 1;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareErrResponse(unsigned char errCode)
{
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data
    m_Response.usLen = 2;
    m_Response.usCounter = m_Request.usCounter;

    //Fuellen des Response
    m_Response.xcpResponse.b[0] = PID_ERR;
    m_Response.xcpResponse.b[1] = errCode;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PreparePageProcessorInfo()
{
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data
    m_Response.usLen = 3;

    //Fuellen des Response
    m_Response.xcpResponse.b[0] = PID_RES;

    //Maximal amount of segments
    m_Response.xcpResponse.b[1] = m_AmountSegments;

    //Page Properties
    m_Response.xcpResponse.b[2] = PAG_PROPERTY_FREEZE;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareSegmentInfo()
{
#ifdef _WIN32
    unsigned char                   SegmentNumber;
    unsigned char                   Mode;
    unsigned char                   SegmentInfo;
    struct _IMAGE_SECTION_HEADER*   ptrSegment;

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Fuellen des Response
    m_Response.xcpResponse.b[0] = PID_RES;

    //Determine segment for Response
    Mode = m_Request.xcpRequest.b[1];
    SegmentInfo = m_Request.xcpRequest.b[3];
    SegmentNumber = m_Request.xcpRequest.b[2];

    if (SegmentNumber > m_CopyPages.get_number_of_sections ()) {
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    ptrSegment = m_CopyPages.get_sections_header (SegmentNumber);
    if (ptrSegment == nullptr) {
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    //Transfer either Base adress or Length, depending of the request
    switch (Mode) {
    case 0:
        //Set data
        m_Response.usLen = 8;
        switch (SegmentInfo) {
        case 0://Adress required
            //Send adress
            m_Response.xcpResponse.dw[1] = ptrSegment->PointerToRawData;
            break;
        case 1:
            //Send Length
            m_Response.xcpResponse.dw[1] = ptrSegment->Misc.VirtualSize;
            break;
        default:
            //Segment Info out of Range
            PrepareErrResponse(ERR_OUT_OF_RANGE);
            break;
        };
        break;
    case 1://Segment info dont care
        //Set data
        //Amount of Pages
        unsigned char ExeSegmentNo;
        if (IsSegmentCallibationAble(SegmentNumber, &ExeSegmentNo,  &(m_Response.xcpResponse.b[1])) < 0) {
            PrepareErrResponse(ERR_OUT_OF_RANGE);
        } else {
            m_Response.usLen = 6;
            //Adress Extension
            m_Response.xcpResponse.b[2] = 0;

            //Max Mapping
            m_Response.xcpResponse.b[3] = 0;

            //Compression Method
            m_Response.xcpResponse.b[4] = 0;

            //Encryption Method
            m_Response.xcpResponse.b[5] = 0;
        }
        break;
    case 2:
        //Set data
        m_Response.usLen = 8;
        //Mapping Info
        m_Response.xcpResponse.dw[1] = 0;//strInfo.baseAdress;
        break;
    default:
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        break;
    }

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
#else
#endif
}

void cXCPConnector::PreparePageInfo()
{
    unsigned char   Segment;
    //unsigned char   Page;

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Get Request Data
    Segment = m_Request.xcpRequest.b[2];
    //Page = m_Request.xcpRequest.b[3];

    //Wenn fuer ein Segment ungleich des aktuellen Segments die Info angefordert wird,
    //so antworte mit einem negativen Response
    if (Segment > m_CopyPages.get_number_of_sections ()) {
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    //Set data
    m_Response.usLen = 3;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    // Wenn Code-Segment dann ist dies nicht schreibbar alle anderen sind auch wenn das Flag IMAGE_SCN_MEM_WRITE
    // nicht gesetzt ist schreibbar!
    unsigned char ExeSegmentNo;
    unsigned char NoPages;
    if (IsSegmentCallibationAble(Segment, &ExeSegmentNo,  &NoPages) == 1) {
        m_Response.xcpResponse.b[1] = ECU_ACCESS_WITH | XCP_READ_ACCESS_WITH | XCP_WRITE_ACCESS_WITH;
    } else {
        m_Response.xcpResponse.b[1] = ECU_ACCESS_WITH | XCP_READ_ACCESS_WITH;
    }

    //Init Segment
    m_Response.xcpResponse.b[2] = Segment;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareGetCalPage()
{
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data
    m_Response.usLen = 4;

    //Fueellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Logical Page number
    m_Response.xcpResponse.b[3] = m_ActivePage;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::TakeCurrentMTA()
{
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Store MTA
    m_CurMTA = TranslateAddress (m_Request.xcpRequest.dw[1]);
    m_CurMTAExtension = m_Request.xcpRequest.b[3];
    m_CurMTAIsNotTragetAddr = 0;  // ist eine TCu interne Adresse

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::ShortUpload()
{
    unsigned int Size;
    uint64_t Address;

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    Size = m_Request.xcpRequest.b[1];
    Address = TranslateAddress(m_Request.xcpRequest.dw[1]);

    if (Size > (MAX_CTO - 1)) {
        m_Response.xcpResponse.b[0] = ERR_OUT_OF_RANGE;
        //Set data for Response
        m_Response.usLen = 1;
    } else {

        XCPReadMemoryFromExternProcess (m_LinkNr, Address, static_cast<void*>(&m_Response.xcpResponse.b[1]), static_cast<int>(Size));

        //Fuellen des Response => Page / Segment Dependent
        m_Response.xcpResponse.b[0] = PID_RES;

        //Set data for Response
        m_Response.usLen = static_cast<unsigned short>(Size + 1);
    }
    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::ShortDownload()
{
    unsigned int Size;
    uint64_t Address;

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    Size = m_Request.xcpRequest.b[1];
    Address = TranslateAddress(m_Request.xcpRequest.dw[1]);

    if (Size > (MAX_CTO - 8)) {
        m_Response.xcpResponse.b[0] = ERR_OUT_OF_RANGE;
        //Set data for Response
        m_Response.usLen = 1;
    } else {
        //Fuellen des Response => Page / Segment Dependent

        if (XCPWriteMemoryToExternProcess (m_LinkNr, Address, reinterpret_cast<char*>(&m_Request.xcpRequest.b[8]), static_cast<int>(Size)) == 0) {
            m_Response.xcpResponse.b[0] = ERR_OUT_OF_RANGE;
            //Set data for Response
            m_Response.usLen = 1;
        }
        //Fuellen des Response => Page / Segment Dependent
        m_Response.xcpResponse.b[0] = PID_RES;

        //Set data for Response
        m_Response.usLen = static_cast<unsigned short>(Size + 1);
    }
    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareChecksum()
{
    uint32_t   ulChecksum;
    unsigned char   ucCalculationType;
    uint32_t   ulBlocksize;

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Get Blocksize
    ulBlocksize = m_Request.xcpRequest.dw[1];

    //Calculate Checksum
    ulChecksum = CalculateChecksumOfCurrentMTA(ulBlocksize,&ucCalculationType);

    //Set data for Response
    m_Response.usLen = 8;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Checksum Type
    m_Response.xcpResponse.b[1] = ucCalculationType;

    //Checksum
    m_Response.xcpResponse.dw[1] = ulChecksum;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

// CRC32 Begin
#define POLYNOMIAL			0x04C11DB7
#define INITIAL_REMAINDER	0xFFFFFFFF
#define FINAL_XOR_VALUE		0xFFFFFFFF

#define WIDTH    (8 * sizeof (uint32_t))
#define TOPBIT   (1U << (WIDTH - 1))

#define REFLECT_DATA(X)			(static_cast<unsigned char>(reflect((X), 8)))
#define REFLECT_REMAINDER(X)	(static_cast<uint32_t>(reflect((X), WIDTH)))

static void Crc32Init (uint32_t *Crc32Table)
{
    uint32_t remainder;
    uint32_t dividend;
    unsigned char bit;

    for (dividend = 0; dividend < 256; ++dividend) {
        remainder = dividend << (WIDTH - 8);
        for (bit = 8; bit > 0; --bit) {
            if (remainder & TOPBIT) {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }
        Crc32Table[dividend] = remainder;
    }
}

static uint32_t reflect(uint32_t data, unsigned char nBits)
{
    uint32_t  reflection = 0x00000000;
    unsigned char  bit;

    for (bit = 0; bit < nBits; ++bit) {
        if (data & 0x01) {
            reflection |= (1 << ((nBits - 1) - bit));
        }
        data = (data >> 1);
    }
    return (reflection);
}


uint32_t Crc32Fast (unsigned char const message[], int nBytes)
{
    uint32_t remainder = INITIAL_REMAINDER;
    unsigned char data;
    int byte;
    uint32_t Crc32Table[256];

    Crc32Init (Crc32Table);

    for (byte = 0; byte < nBytes; ++byte) {
        data = REFLECT_DATA(message[byte]) ^ (remainder >> (WIDTH - 8));
        remainder = Crc32Table[data] ^ (remainder << 8);
    }
    return (REFLECT_REMAINDER(remainder) ^ FINAL_XOR_VALUE);
}
// CRC32 End

uint32_t  cXCPConnector::CalculateChecksumOfCurrentMTA(uint32_t ulBlocksize,
                                                       unsigned char* ptrAlgorithm)
{
    uint32_t  ulChecksum = 0;
    unsigned char* ptrData;

    ptrData = static_cast<unsigned char*>(my_malloc (ulBlocksize));
    if (ptrData != nullptr) {
        XCPReadMemoryFromExternProcess (m_LinkNr, m_CurMTA, ptrData, static_cast<int>(ulBlocksize));
        ulChecksum = Crc32Fast (ptrData, static_cast<int>(ulBlocksize));
        *ptrAlgorithm = XCP_CRC_32;
        m_CurMTA += ulBlocksize;
        my_free (ptrData);
    }
    return ulChecksum;
}


void cXCPConnector::TakeCalibrationPage()
{
#ifdef _WIN32
    unsigned char   Segment,Page,Mode;
    struct _IMAGE_SECTION_HEADER*   ptrSegment;

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Get Segment and Page
    Segment = m_Request.xcpRequest.b[2];
    Page = m_Request.xcpRequest.b[3];

    Mode = m_Request.xcpRequest.b[1];
    if (Mode & 0x80) {
        if (SwitchPagesAllSegments(Page)) {
            PrepareErrResponse(ERR_PAGE_NOT_VALID);
        }
    } else {
        //check Segment adressing
        if (Segment > m_CopyPages.get_number_of_sections ()) {
            //Invalid Page
            PrepareErrResponse(ERR_SEGMENT_NOT_VALID);
            return;
        }

        //Check Pages for Code and Data Segment
        if (Page >= MAX_PAGES_FOR_SEGMENTS) {
            //Invalid Page
            PrepareErrResponse(ERR_PAGE_NOT_VALID);
            return;
        }
        ptrSegment = m_CopyPages.get_sections_header (Segment);
        if (ptrSegment == nullptr) {
            PrepareErrResponse(ERR_OUT_OF_RANGE);
            return;
        }
        //If desired Page != ActivePage => Load from File
        if (m_ActivePage != Page) {
            unsigned char ExeSegmentNo;
            unsigned char NoPages;
            if ((Segment == 0) && (Page == 1)) {
                printf ("debug");
            }
            if (IsSegmentCallibationAble(Segment, &ExeSegmentNo, &NoPages) == 1) {
                if (m_CopyPages.switch_page_of_section (Segment, ExeSegmentNo, m_ActivePage, Page, this)) {
                    PrepareErrResponse(ERR_OUT_OF_RANGE);
                    return;
                }
            } else {
                PrepareErrResponse(ERR_OUT_OF_RANGE);
                return;
            }
        }
    }
    m_ActivePage = Page;

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
#else
#endif
}

void cXCPConnector::PrepareDAQProcessorInfo()
{
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 8;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    /************************************DAQ Properties ******************************************/
    //DAQ Properties, see basic settings
    m_Response.xcpResponse.b[1] = bscSetDAQ;

    //MAX DAQ => Aktuell allocierte DAQ Listen => Nur dynamische
    m_Response.xcpResponse.w[1] = m_dynAllocatedDAQs;

  //Event Channel nicht MAX DAQ sondern die beim Init uebergebene Anzahl.
    m_Response.xcpResponse.w[2] = m_AllocatedEvents; //MAXEVENTCHANNELS;

    //Minimal number of predefined DAQs => No predefined DAQs
    m_Response.xcpResponse.b[6] = 0;

    //DAQ Keybyte, see basic Settings
    m_Response.xcpResponse.b[7] = bscSetDAQKeyB;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}


void cXCPConnector::TakeSegmentMode()
{
    //Get Segment Mode 4 Segment
    m_curSegmentMode = m_Request.xcpRequest.b[1];

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::TakeFirstBlockOfDownloadData()
{
    //Get Segment Mode 4 Segment
    m_ExpectedElements = m_Request.xcpRequest.b[1];
    if (m_ExpectedElements > (MAX_CTO - 2)) {
        m_ReceivedElements = MAX_CTO - 2;
    } else {
        m_ReceivedElements = m_ExpectedElements;
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Take Data => copy to MTA pointer and post- increment
    XCPWriteMemoryToExternProcess (m_LinkNr, m_CurMTA, &m_Request.xcpRequest.b[2], m_ReceivedElements);

    m_CurMTA += m_ReceivedElements;

    if (m_ExpectedElements > (MAX_CTO - 2)) {
        //Download in Block Mode => Next block follows without confirmation
        m_WaitForNextCommand = 1;
    } else {
        //Download Just One block => Finished => Respond
        //Set data for Response
        m_Response.usLen = 1;

        //Fuellen des Response => Page / Segment Dependent
        m_Response.xcpResponse.b[0] = PID_RES;

        //Trigger statemachine
        m_SendState = XCP_TR_RESP_CTO;
        m_WaitForNextCommand = 1;
    }
}

void cXCPConnector::TakeNextBlockOfDownloadData()
{
    unsigned char     currentElements;
    unsigned char     currentReceived;

    //Get Segment Mode 4 Segment
    currentElements = m_Request.xcpRequest.b[1];
    if (m_ExpectedElements <= (MAX_CTO - 2)) {
        currentReceived = currentElements;
    } else {
        currentReceived = MAX_CTO - 2;
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Take Data => copy to MTA pointer and post- increment
    XCPWriteMemoryToExternProcess (m_LinkNr, m_CurMTA, &m_Request.xcpRequest.b[2], currentReceived);

    m_CurMTA += currentReceived;

    //Check, if completely received
    m_ReceivedElements += currentReceived;

    if (m_ExpectedElements > m_ReceivedElements) {
        //Download in Block Mode => Next block follows without confirmation
        m_WaitForNextCommand = 1;
    } else {
        if (m_ExpectedElements == m_ReceivedElements) {
            //Download Finished => Respond
            //Set data for Response
            m_Response.usLen = 1;

            //Fuellen des Response => Page / Segment Dependent
            m_Response.xcpResponse.b[0] = PID_RES;

            //Trigger statemachine
            m_SendState = XCP_TR_RESP_CTO;

            m_WaitForNextCommand = 0;
        } else {
            //Received more than expected => Signal error
            //Download Finished => Respond
            //Set data for Response
            m_Response.usLen = 3;

            //Fuellen des Response => Page / Segment Dependent
            m_Response.xcpResponse.b[0] = PID_RES;
            m_Response.xcpResponse.b[1] = ERR_SEQUENCE;
            m_Response.xcpResponse.b[2] = m_ExpectedElements;

            //Trigger statemachine
            m_SendState = XCP_TR_RESP_CTO;

            m_WaitForNextCommand = 0;
        }
    }
}

/*Allocates a given amount of DAQ Lists*/
void cXCPConnector::AllocateDAQList()
{
    unsigned short uiDaqs2Allocate;

    //Get Request
    uiDaqs2Allocate = m_Request.xcpRequest.w[1];

    //Memory overflow?
    if (uiDaqs2Allocate > MAXDAQLISTS) {
        //Requested amount of DAQ Lists exceeds implementation
        PrepareErrResponse(ERR_MEMORY_OVERFLOW);
        return;
    }

    //Sequence error ?
    if (m_dynAllocatedDAQs > 0) {
        //Free DAQ must have been called prior this request => if not, Respond sequence error
        PrepareErrResponse(ERR_SEQUENCE);
        return;
    }

    //Take allocation request
    m_dynAllocatedDAQs = uiDaqs2Allocate;

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

/*Clears a specific DAQ List DAQ Lists*/
void cXCPConnector::ClearDAQList()
{
    unsigned short    usDAQ2Clear;

    //Get Request
    usDAQ2Clear = m_Request.xcpRequest.w[1];

    //Clear all List elements
    if (usDAQ2Clear > m_dynAllocatedDAQs) {
      //DAQ List# not valid
      PrepareErrResponse(ERR_OUT_OF_RANGE);
      return;
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}


/*Clears all DAQ Lists*/
void cXCPConnector::FreeDAQList()
{
    //Clear all DAQ-ODT List elements
    //Release allocated memory and reset every item
    ResetDynamicDAQMemory();
    m_dynAllocatedDAQs = 0;

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::AllocateODT2DAQ()
{
    unsigned short  daqListNumber;
    unsigned char   daqListODTCount;
    unsigned short  iLoop;

    //Get incomming data
    daqListNumber = m_Request.xcpRequest.w[1];
    daqListODTCount = m_Request.xcpRequest.b[4];

    //Allocate a ODT for a given DAQ
    if (daqListNumber < m_dynAllocatedDAQs) {
        iLoop = 0;
        while (iLoop < daqListODTCount) {
            //Object already existing
            if (m_Daq[daqListNumber].m_PtrODTs[iLoop] == nullptr) {
                m_Daq[daqListNumber].m_PtrODTs[iLoop] = new XCP_ODT;
                memset(m_Daq[daqListNumber].m_PtrODTs[iLoop],0,sizeof(XCP_ODT));
            } else {
                //ODT already exists => Sequence error
                PrepareErrResponse(ERR_SEQUENCE);
                return;
            }
            iLoop++;
        }

        //Finish Settings
        m_Daq[daqListNumber].m_allocatedODTs = daqListODTCount;
        m_Daq[daqListNumber].m_ptrCurrentODT = m_Daq[daqListNumber].m_PtrODTs[0];
    } else {
        //Invalid DAQ List
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}


void cXCPConnector::AllocateODTEntry2DAQ()
{
    unsigned short daqListNumber;
    unsigned char  odtNumber;
    unsigned char  odtEntriesCount;

    //Get incomming data
    daqListNumber = m_Request.xcpRequest.w[1];
    odtNumber = m_Request.xcpRequest.b[4];
    odtEntriesCount = m_Request.xcpRequest.b[5];

    //Access the desired element
    if (daqListNumber < m_dynAllocatedDAQs && odtNumber < m_Daq[daqListNumber].m_allocatedODTs) {
        //Valid Daq List and Valid ODT
        //=> Allocate space for ODT Entries
        m_Daq[daqListNumber].m_PtrODTs[odtNumber]->m_odtBasePtr = new XCP_ODT_ENTRY[odtEntriesCount];
        memset(m_Daq[daqListNumber].m_PtrODTs[odtNumber]->m_odtBasePtr,0,sizeof(XCP_ODT_ENTRY)*odtEntriesCount);
        m_Daq[daqListNumber].m_PtrODTs[odtNumber]->m_AllocatedElements = odtEntriesCount;
        m_Daq[daqListNumber].m_PtrODTs[odtNumber]->m_curODTPtr = m_Daq[daqListNumber].m_PtrODTs[odtNumber]->m_odtBasePtr;
    } else {
        if (daqListNumber >= m_dynAllocatedDAQs) {
            //Invalid DAQ List
            PrepareErrResponse(ERR_OUT_OF_RANGE);
            return;
        } else {
            //DAQ List not existing => Sequence error
            PrepareErrResponse(ERR_SEQUENCE);
            return;
        }
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::SetDAQPointer()
{
    unsigned char  odtNumber;
    unsigned char  odtEntry;

    //Get incomming data
    m_adressedDAQList = m_Request.xcpRequest.w[1];
    odtNumber = m_Request.xcpRequest.b[4];
    odtEntry = m_Request.xcpRequest.b[5];

    //Set DAQ Pointer for communication
    if (m_adressedDAQList < m_dynAllocatedDAQs) { // && m_Daq[m_adressedDAQList].m_PtrODTs) {
        //Adressed daq list already set
        //Now, set ODT
        if (odtNumber < m_Daq[m_adressedDAQList].m_allocatedODTs) {
            //Set ODT Pointer to current
            m_Daq[m_adressedDAQList].m_ptrCurrentODT = m_Daq[m_adressedDAQList].m_PtrODTs[odtNumber];

            //Set Entry Ptr to ODT Entry
            if (odtEntry < m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_AllocatedElements) {
                m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_curODTPtr = m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_odtBasePtr;
                int i = 0;
                while (i < odtEntry) {
                    m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_curODTPtr++;
                    i++;
                }
            } else {
                //Invalid DAQ List
                PrepareErrResponse(ERR_OUT_OF_RANGE);
                return;
            }
        } else {
            //Invalid DAQ List
            PrepareErrResponse(ERR_OUT_OF_RANGE);
            return;
        }
    } else {
        if (m_adressedDAQList >= m_dynAllocatedDAQs) {
            //Invalid DAQ List
            PrepareErrResponse(ERR_OUT_OF_RANGE);
            return;
        } else {
            //DAQ List not existing => Sequence error
            PrepareErrResponse(ERR_SEQUENCE);
            return;
        }
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::WriteElement2ODTEntry()
{
    unsigned char   bitOffset;
    unsigned char   sizeOfElement;
    unsigned char   adrExtension;
    uint64_t   Adress;

    //Get incomming data
    bitOffset = m_Request.xcpRequest.b[1];
    sizeOfElement = m_Request.xcpRequest.b[2];
    adrExtension = m_Request.xcpRequest.b[3];
    Adress = TranslateAddress(m_Request.xcpRequest.dw[1]);

    //Adressed list existing
    if (m_adressedDAQList < m_dynAllocatedDAQs && m_Daq[m_adressedDAQList].m_ptrCurrentODT &&
            m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_curODTPtr) {
        //Writes an element to an ODT Entry
        m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_curODTPtr->bitOffset = bitOffset;
        m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_curODTPtr->adressEytension = adrExtension;
        m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_curODTPtr->adressOfElement = Adress;
        m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_curODTPtr->sizeOfElement = sizeOfElement;
        m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_curODTPtr++;
        m_Daq[m_adressedDAQList].m_ptrCurrentODT->m_odtCount++;

        //XCP Spec: when ladt ODT Entry was written to the ODT, the current Pointer is undefined!
    } else {
        //Invalid DAQ List
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::SetDAQListMode()
{
    unsigned char Mode;
    unsigned short daqListNumber;
    unsigned short eventChannelNumber;
    unsigned char  transmissionRatePrescaler;
    unsigned char  daqListPriority;

    //Get incomming data
    Mode = m_Request.xcpRequest.b[1];
    daqListNumber = m_Request.xcpRequest.w[1];
    eventChannelNumber = m_Request.xcpRequest.w[2];
    transmissionRatePrescaler = m_Request.xcpRequest.b[6];
    daqListPriority = m_Request.xcpRequest.b[7];

    //Writes an element to an ODT Entry
    if (daqListNumber < m_dynAllocatedDAQs) {
        m_Daq[daqListNumber].m_ListMode = Mode;
        m_Daq[daqListNumber].m_EventChannel = eventChannelNumber;
        m_Daq[daqListNumber].m_Prescaler = transmissionRatePrescaler;
        m_Daq[daqListNumber].m_Prio = daqListPriority;
    } else {
        //Invalid DAQ List
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::HandleGetDAQListMode()
{
    unsigned short daqListNumber;

    //Get incomming data
    daqListNumber = m_Request.xcpRequest.w[1];

    //Gets the list mode to a existine DAQ Queue
    if (daqListNumber < m_dynAllocatedDAQs) {
        //Reset response field
        memset(&m_Response,0,sizeof(m_Response));
        m_Response.xcpResponse.b[0] = PID_RES;
        m_Response.usLen = 8;

        m_Response.xcpResponse.b[1] = m_Daq[daqListNumber].m_ListMode;
        m_Response.xcpResponse.w[2] = m_Daq[daqListNumber].m_EventChannel;
        m_Response.xcpResponse.b[6] = m_Daq[daqListNumber].m_Prescaler;
        m_Response.xcpResponse.b[7] = m_Daq[daqListNumber].m_Prio;
    } else {
        //Invalid DAQ List
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

/*Start/Stop specified DAQ*/
void cXCPConnector::HandleStartStopDAQList()
{
    unsigned char Mode;
    unsigned short daqListNumber;

    //Get incomming data
    Mode = m_Request.xcpRequest.b[1];
    daqListNumber = m_Request.xcpRequest.w[1];

    if (daqListNumber < m_dynAllocatedDAQs) {
        //Take Mode for selected DAQ List
        m_Daq[daqListNumber].m_State = Mode;
    } else {
        //Invalid DAQ List
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));

    //Set data for Response
    m_Response.usLen = 2;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Get first PID of the selected List
    //jede DAQ-Liste hat MAX_ODT = 16
    m_Response.xcpResponse.b[1] = static_cast<unsigned char>(daqListNumber << 4);

    //Get Session state depending on Mode
    GetSessionState();

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

/*Start/Stop all DAQ Lists*/
void cXCPConnector::HandleStartStopDAQListSynch()
{
    unsigned char Mode;
    unsigned short daqListNumber = 0;

    //Get incomming data
    Mode = m_Request.xcpRequest.b[1];

    for (daqListNumber = 0; daqListNumber < m_dynAllocatedDAQs; daqListNumber++) {
        switch (Mode) {
        case 0: // Stop all
            m_Daq[daqListNumber].m_State = DAQ_STOPPED;
            break;
        case 1: // start selected
            if (m_Daq[daqListNumber].m_State == DAQ_SELECTED) {
                m_Daq[daqListNumber].m_State = DAQ_STARTED;
            }
            break;
        case 2: // stop selected
            if (m_Daq[daqListNumber].m_State == DAQ_SELECTED) {
                m_Daq[daqListNumber].m_State = DAQ_STOPPED;
            }
            break;

        }
    }
    //If started directly or selected, initialize the transfer buffer
    if (Mode == 0x01 /*start*/ || Mode == 0x02) {
        CreateOrResetDAQTransferBuffers();
    }

    //Get Session state depending on Mode
    GetSessionState();

    //Set data for Response
    m_Response.usLen = 1;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareDAQResolutionInfo()
{
    //No incomming data

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));
    m_Response.usLen = 8;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Granularity ODT Entry size DAQ => Basic settings
    m_Response.xcpResponse.b[1] = bscSetGranODTEntrySizeDaq;

    //MAX ODT Entry size
    m_Response.xcpResponse.b[2] = MAX_ODT_ENTRY_SIZE;

    //Granularity ODT Entry size STIM
    //=> Not supported
    m_Response.xcpResponse.b[3] = bscSetGranODTEntrySizeDaq;

    //MAX ODT Entry size STIM
    //=> Not supported
    m_Response.xcpResponse.b[4] = MAX_ODT_ENTRY_SIZE;

    //Timestamp Mode
    m_Response.xcpResponse.b[5] = bscSetDAQTSMode;

    //Timestamp Ticks
    m_Response.xcpResponse.w[3] = bscSetDAQTSTicks;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareDAQEventInfo()
{
    unsigned short  daqEventChannelNumber;
    XCP_EVENT_DESC*   ptrEvent;
    unsigned char   ucTimeUnit;

    //No incomming data
    daqEventChannelNumber = m_Request.xcpRequest.w[1];

    //Plausible
    if (daqEventChannelNumber >= m_AllocatedEvents) {
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    } else {
        ptrEvent = m_ptrEvents+daqEventChannelNumber;
    }

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));
    m_Response.usLen = 7;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //DAQ Event Properties => All the same (Just DAQ, no STIM supported)
    m_Response.xcpResponse.b[1] = bscSetDaqEventProperties;

    //MAX DAQ Lists
    //All the same: MAX DAQ could be assigned to one event
    m_Response.xcpResponse.b[2] = MAXDAQLISTS;

    //Event Channel Name
    if (ptrEvent->EventName != nullptr) {
        if (strlen (ptrEvent->EventName) > 255) m_Response.xcpResponse.b[3] = 255;
        else m_Response.xcpResponse.b[3] = static_cast<unsigned char>(strlen (ptrEvent->EventName));
        m_CurMTA64 = reinterpret_cast<uint64_t>(ptrEvent->EventName);
        // ist nicht eine Target Adresse sondern eine interne!
        // Der Identification String wird nicht aus dem externen Prozess gelesen!
        m_CurMTAIsNotTragetAddr = 1;
    }

    //Event channel timecycle => Channel specific
    m_Response.xcpResponse.b[4] = ptrEvent->timeCycle;

    //Event channel Time Unit => High nipple = Channel specific, Low Nibble = fixed (2 Byte Timestamp and fixed event)
    ucTimeUnit = (ptrEvent->timebase & 0x0F);
    m_Response.xcpResponse.b[5] = ucTimeUnit;

    //Event channel Prio => Channel specific
    m_Response.xcpResponse.b[6] = ptrEvent->Prio;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::PrepareDaqTransmission(unsigned short usDaq)
{
    unsigned short ptrCurrent = 0;
    unsigned char  bODT = 0;
    unsigned char  iLoop;

    if (usDaq < m_dynAllocatedDAQs) {
        //Valid Daq
        while (bODT < m_Daq[usDaq].m_allocatedODTs) {
            //Prepare Data to all ODT Lists of this DAQ
            if (m_Daq[usDaq].m_PtrODTs[bODT]->m_odtBasePtr) {
                m_Daq[usDaq].m_PtrODTs[bODT]->m_curODTPtr = m_Daq[usDaq].m_PtrODTs[bODT]->m_odtBasePtr;
                iLoop = 0;
                ptrCurrent = 0;
                while (iLoop < m_Daq[usDaq].m_PtrODTs[bODT]->m_AllocatedElements) {
                  XCPReadMemoryFromExternProcess (m_LinkNr,
                                                  m_Daq[usDaq].m_PtrODTs[bODT]->m_curODTPtr->adressOfElement,
                                                  static_cast<void*>(m_Daq[usDaq].m_PtrODTs[bODT]->m_DAQTransferField+ptrCurrent),
                                                  m_Daq[usDaq].m_PtrODTs[bODT]->m_curODTPtr->sizeOfElement);

                  //Increment the current ODT Ptr
                  ptrCurrent += m_Daq[usDaq].m_PtrODTs[bODT]->m_curODTPtr->sizeOfElement;
                  m_Daq[usDaq].m_PtrODTs[bODT]->m_curODTPtr++;
                  iLoop++;
              }
              //Store DAQ Length information
              m_Daq[usDaq].m_PtrODTs[bODT]->m_DAQTransferFieldLen = ptrCurrent;
          }
          bODT++;
        }
        //Set Sampled state flag
        m_Daq[usDaq].m_Sampled = true;
    }
}

void cXCPConnector::InitDAQ()
{
    m_dynAllocatedDAQs = 0;
    m_adressedDAQList = 0;
    m_DaqClock = 0;
    m_DaqPackageCounter = 0;
    memset(m_Daq,0,sizeof(m_Daq));
}

void cXCPConnector::ResetDynamicDAQMemory()
{
    uint8_t    bLoop = 0;
    uint8_t    bODT;

    m_dynAllocatedDAQs = 0;
    m_adressedDAQList = 0;

    while (bLoop < MAXDAQLISTS) {
        //Reset ODT List to DAQ
        bODT = 0;
        while (bODT < MAX_ODT) {
            if (m_Daq[bLoop].m_PtrODTs[bODT]) {
                if (m_Daq[bLoop].m_PtrODTs[bODT]->m_odtBasePtr) {
                    delete [] m_Daq[bLoop].m_PtrODTs[bODT]->m_odtBasePtr;
                }
                if (m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField) {
                    delete [] m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField;
                }
            }
            bODT++;
        }
        //Reset next DAQ
        bLoop++;
    }
    memset(m_Daq,0,sizeof(m_Daq));
}

void cXCPConnector::CreateOrResetDAQTransferBuffers()
{
    uint8_t              bLoop = 0;
    uint8_t              bODT;
    uint8_t              bEntry;
    unsigned short    desMemory;

    while (bLoop < m_dynAllocatedDAQs) {
        //Reset ODT List to DAQ
        bODT = 0;
        while (bODT < m_Daq[bLoop].m_allocatedODTs) {
            if (m_Daq[bLoop].m_PtrODTs[bODT]) {
                //Reset Desired Memory
                desMemory = 0;

                if (m_Daq[bLoop].m_PtrODTs[bODT]->m_odtCount > 0) {
                    bEntry = 0;
                    m_Daq[bLoop].m_PtrODTs[bODT]->m_curODTPtr = m_Daq[bLoop].m_PtrODTs[bODT]->m_odtBasePtr;
                    while (bEntry < m_Daq[bLoop].m_PtrODTs[bODT]->m_odtCount) {
                        desMemory += m_Daq[bLoop].m_PtrODTs[bODT]->m_curODTPtr->sizeOfElement;
                        m_Daq[bLoop].m_PtrODTs[bODT]->m_curODTPtr++;
                        bEntry++;
                    }
                }

                //Transferfield already created
                if (m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField && m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferFieldAllocated < desMemory) {
                    //Dynamischer Speicher existiert, ist aber zu klein
                    delete [] m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField;
                    m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField = new unsigned char[desMemory];
                    m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferFieldAllocated = desMemory;
                    memset(m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField,0,m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferFieldAllocated);
                } else {
                    if (m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField == nullptr) {
                        //Dynamischer Speicher wurde noch nicht allociert
                        m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField = new unsigned char[desMemory];
                        m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferFieldAllocated = desMemory;
                        memset(m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField,0,m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferFieldAllocated);
                    } else {
                        //Field alreadyExisting => Reset Data
                        memset(m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferField,0,m_Daq[bLoop].m_PtrODTs[bODT]->m_DAQTransferFieldAllocated);
                    }
                }
            }
            bODT++;
        }
        bLoop++;
    }
}

void cXCPConnector::GetSessionState()
{
    unsigned char   bLoop = 0;

    while (bLoop < m_dynAllocatedDAQs) {
        if (m_Daq[bLoop].m_State ==DAQ_STARTED) {
            m_SessionStatus |= SS_DAQ;
            return;
        }
        bLoop++;
    }

    //Ruecksetzen des DAQ_Running
    m_SessionStatus = m_SessionStatus & ~SS_DAQ;
    return;
}

void cXCPConnector::HandleGetDaqClock()
{
    //No incomming data

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));
    m_Response.usLen = 8;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_RES;

    //Granularity ODT Entry size DAQ => Basic settings
    m_Response.xcpResponse.dw[1] = m_DaqClock;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}


void cXCPConnector::CopyCalPage (void)
{
    int SrcSegNo, SrcPageNo, DstSegNo, DstPageNo;

    SrcSegNo = m_Request.xcpRequest.b[1];
    SrcPageNo = m_Request.xcpRequest.b[2];
    DstSegNo = m_Request.xcpRequest.b[3];
    DstPageNo = m_Request.xcpRequest.b[4];

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));
    m_Response.usLen = 8;

    if ((SrcSegNo >= m_CopyPages.get_number_of_sections ()) ||
        (DstSegNo >= m_CopyPages.get_number_of_sections ()) ||
        (SrcPageNo >= MAX_PAGES_FOR_SEGMENTS) ||
        (DstPageNo >= MAX_PAGES_FOR_SEGMENTS)) {
        PrepareErrResponse(ERR_OUT_OF_RANGE);
        return;
    }

    if (m_CopyPages.copy_section_page_file (SrcSegNo, SrcPageNo, DstSegNo, DstPageNo)) {
        //Fuellen des Response => Page / Segment Dependent
        m_Response.xcpResponse.b[0] = ERR_WRITE_PROTECTED;
    } else {
        //Fuellen des Response => Page / Segment Dependent
        m_Response.xcpResponse.b[0] = PID_RES;
    }
    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::SetRequest (void)
{
    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));
    m_Response.usLen = 8;
    m_Response.xcpResponse.b[0] = PID_RES;

    if ((m_Request.xcpRequest.b[1] & 1) == 1) {
        // Arbeitsseite in Referenzseite kopieren
        //Schreiben der beiden Pagefiles
        bool found = false;
        for (int s = 0; s < m_CopyPages.get_number_of_sections (); s++)  {
            // alle applizierbaren Sections zurueckschreiben
            unsigned char NoPages;
            unsigned char A2lSegmentNo;
            unsigned char ExeSegmentNo;
            if (IsSegmentCallibationAble (m_CopyPages.get_sections_name (s), &A2lSegmentNo, &ExeSegmentNo, &NoPages) == 1) {
                for (int p = 0; p < MAX_PAGES_FOR_SEGMENTS; p++) {
                    m_CopyPages.switch_page_of_section (A2lSegmentNo, ExeSegmentNo, p, -1, this);  // alle Pages mit aktuellen Daten schreiben
                }
                //int TaskNumber;
                char *SectionName = m_CopyPages.get_sections_short_name (s);
                if (SectionName == nullptr) {
                    //Fuellen des Response => Page / Segment Dependent
                    m_Response.xcpResponse.b[0] = ERR_OUT_OF_RANGE;
                } else {
                    //TaskNumber = m_CopyPages.get_sections_task_number(s);

                    if (write_section_to_exe_file (m_Pid, SectionName)) {
                        //Fuellen des Response => Page / Segment Dependent
                        found = true;
                    } else {
                        //Fuellen des Response => Page / Segment Dependent
                        m_Response.xcpResponse.b[0] = ERR_OUT_OF_RANGE;
                    }
                }
            }
        }
        if (found) {
            m_Response.xcpResponse.b[0] = PID_RES;
        } else {
            m_Response.xcpResponse.b[0] = ERR_SEGMENT_NOT_VALID;
        }
    } else {
        m_Response.xcpResponse.b[0] = ERR_OUT_OF_RANGE;
    }
    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

void cXCPConnector::HandleSendEvent2Server(unsigned char ucEventCode)
{
    //No incomming data

    //Reset response field
    memset(&m_Response,0,sizeof(m_Response));
    m_Response.usLen = 2;

    //Fuellen des Response => Page / Segment Dependent
    m_Response.xcpResponse.b[0] = PID_EV;

    //Append Event Code without information
    m_Response.xcpResponse.b[1] = ucEventCode;

    //Trigger statemachine
    m_SendState = XCP_TR_RESP_CTO;
}

int cXCPConnector::GetSegmentName(char *SegmentDescString, int SegmentNr, char *par_or_ret_SegmentName, int MaxChars, unsigned char *ret_SegmentNo)
{
    char *p, *pp;
    unsigned int s;
    char *pSegmentDescStart;
    char *pSegmentDescEnd;
    char *pSegmentNameStart;
    char *pSegmentNameEnd = nullptr;
    char *EndPtr;
    unsigned int Value = 0;
    int ValueFlag;

    if (SegmentDescString == nullptr) {
        return -1;  // kein Datensegment(e) angegeben
    }
    p = SegmentDescString;
    for (s = 0; *p != 0; s++) {
        while (isspace (*p)) p++;  // fuehrende Leerzechen
        pSegmentNameStart = pSegmentDescStart = p;
        while (1) {
            if ((*p == 0) || (*p == ',')) {
                pSegmentDescEnd = p;
                while (isspace (*(pSegmentDescEnd-1))) pSegmentDescEnd--;  // Leerzechen zwischen Sectionname und ,
                if (pSegmentDescStart == pSegmentDescEnd) {
                    ThrowError (1, "inside section description \"%s\" empty section name", m_DataSegment);
                    return -3;
                }
                pp = pSegmentDescStart;
                ValueFlag = 0;
                while (pp < pSegmentDescEnd) {
                    if (*pp == '=') {
                        pSegmentNameEnd = pp;
                        while (isspace (*(pSegmentNameEnd-1))) pSegmentNameEnd--;  // Leerzechen zwischen Sectionname und =
                        if (pSegmentNameStart == pSegmentNameEnd) {
                            ThrowError (1, "inside section description \"%s\" empty section name", m_DataSegment);
                            return -3;
                        }
                        pp++;
                        Value = strtoul (pp, &EndPtr, 0);
                        if (EndPtr != pSegmentDescEnd) {
                            ThrowError (1, "inside section description \"%s\" invalid section number", m_DataSegment);
                            return -3;
                        }
                        ValueFlag = 1;
                    }
                    pp++;
                }
                if (!ValueFlag) {
                    pSegmentNameEnd = pp;
                    Value = s;   // wenn nichts angegeben durchnummerieren
                }

                if (SegmentNr < 0) {
                    if ((pSegmentNameEnd - pSegmentNameStart) == static_cast<int>(strlen (par_or_ret_SegmentName)) &&
                         !memcmp (par_or_ret_SegmentName, pSegmentNameStart, strlen (par_or_ret_SegmentName))) {
                        *ret_SegmentNo = static_cast<unsigned char>(Value);
                        return 1;
                    }
                } else if (SegmentNr == static_cast<int>(Value)) {
                    // ist als Applikations Segment (Section) in der Init-Methode angegeben worden
                    if ((pSegmentNameEnd - pSegmentNameStart) >= MaxChars) {
                        ThrowError (1, "inside section description \"%s\" section name too long", m_DataSegment);
                        return -3;
                    }
                    MEMCPY (par_or_ret_SegmentName, pSegmentNameStart, static_cast<size_t>(pSegmentNameEnd - pSegmentNameStart));
                    par_or_ret_SegmentName[pSegmentNameEnd - pSegmentNameStart] = 0;
                    *ret_SegmentNo = static_cast<unsigned char>(Value);
                    return 1;
                }
                if (*p == ',') p++;
                break; // while-Schleife
            }
            p++;
        }
    }
    return -2;  // Segment nicht gefunden
}

int cXCPConnector::IsSegmentCallibationAble(unsigned char A2LSegmentNr, unsigned char *ret_ExeSegmentNo, unsigned char *ret_NoPages)
{
    char SegmentName[256];
    int Status;
    unsigned char Help;
    int ExeSegmentNo;

    Status = GetSegmentName (m_DataSegment, A2LSegmentNr, SegmentName, sizeof (SegmentName), &Help);
    switch (Status) {
    case 1:
        *ret_NoPages = 2;  // bei Segmenten mit Applikations-Daten immer 2 Pages
        ExeSegmentNo = m_CopyPages.find_section_by_name (0, SegmentName);
        if ((ExeSegmentNo >= 0) && (ExeSegmentNo <= 255)) {
            *ret_ExeSegmentNo = static_cast<unsigned char>(ExeSegmentNo);
            return 1;   // Datensegment
        } else {
            return -1;  // Fehler
        }
    case -2:  // nicht gefunden dann als Code-Segment suchen
        Status = GetSegmentName (m_CodeSegment, A2LSegmentNr, SegmentName, sizeof (SegmentName), &Help);
        switch (Status) {
        case 1:
            ExeSegmentNo = m_CopyPages.find_section_by_name (0, SegmentName);
            if ((ExeSegmentNo >= 0) && (ExeSegmentNo <= 255)) {
                *ret_ExeSegmentNo =  static_cast<unsigned char>(ExeSegmentNo);
                return 0;   // Code-Segment ist nicht applizierbar
            } else {
                return -1; // Fehler
            }
        }
        break;
    case -1:  // es wurden keine Daten-Segmente angegeben, dann ubernehme 1:1 die Segmentierung aus der EXE
        *ret_ExeSegmentNo = A2LSegmentNr;
        return m_CopyPages.is_sections_callibation_able (A2LSegmentNr);
    default:
        break;  // Fehler
    }
    return -1;
}

int cXCPConnector::IsSegmentCallibationAble (char *SegmentName, unsigned char *ret_A2lSegmentNo, unsigned char *ret_ExeSegmentNo, unsigned char *ret_NoPages)
{
    int Status;
    int ExeSegmentNo;

    Status = GetSegmentName (m_DataSegment, -1, SegmentName, sizeof (SegmentName), ret_A2lSegmentNo);
    switch (Status) {
    case 1:
        *ret_NoPages = MAX_PAGES_FOR_SEGMENTS;  // bei Segmenten mit Applikations-Daten immer 2 Pages
        ExeSegmentNo = m_CopyPages.find_section_by_name (0, SegmentName);
        if ((ExeSegmentNo >= 0) && (ExeSegmentNo <= 255)) {
            *ret_ExeSegmentNo =  static_cast<unsigned char>(ExeSegmentNo);
            return 1;   // Datensegment
        } else {
            return -1;  // Fehler
        }
    case -2:  // nicht gefunden dann als Code-Segment suchen
        Status = GetSegmentName (m_CodeSegment, -1, SegmentName, sizeof (SegmentName), ret_A2lSegmentNo);
        switch (Status) {
        case 1:
            ExeSegmentNo = m_CopyPages.find_section_by_name (0, SegmentName);
            if ((ExeSegmentNo >= 0) && (ExeSegmentNo <= 255)) {
                *ret_ExeSegmentNo =  static_cast<unsigned char>(ExeSegmentNo);
                return 0;   // Code-Segment ist nicht applizierbar
            } else {
                return -1; // Fehler
            }
        }
        break;
    case -1:  // es wurden keine Daten-Segmente angegeben, dann ubernehme 1:1 die Segmentierung aus der EXE
        ExeSegmentNo = m_CopyPages.find_section_by_name (0, SegmentName);
        if ((ExeSegmentNo >= 0) && (ExeSegmentNo <= 255)) {
            *ret_A2lSegmentNo = *ret_ExeSegmentNo =  static_cast<unsigned char>(ExeSegmentNo);
            if (m_CopyPages.is_sections_callibation_able (ExeSegmentNo)) {
                *ret_NoPages = MAX_PAGES_FOR_SEGMENTS;  // bei Segmenten mit Applikations-Daten immer 2 Pages
                return 1;
            } else {
                *ret_NoPages = 0;  // bei allen anderen Segmenten immer 0 Pages
                return 0;
            }
        } else {
            return -1; // Fehler
        }
    default:
        break;  // Fehler
    }
    return -1;
}

uint64_t cXCPConnector::TranslateAddress (uint64_t par_Address)
{
    if (m_DllAddressTranslation) {
        return m_CopyPages.TranslateXCPOverEthernetAddress (par_Address);
    } else {
        return par_Address;
    }
}


void cXCPConnector::ActivateDllAddressTranslation()
{
    m_DllAddressTranslation = true;
}

void cXCPConnector::DeactivateDllAddressTranslation()
{
    m_DllAddressTranslation = false;
}


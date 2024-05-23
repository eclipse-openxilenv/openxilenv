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


#pragma once
#include "Platform.h"
#include "XcpOverEthernetCfg.h"
#include "XcpCopyPages.h"
#include "XcpGlobalDeclarations.h"        //Global Project Declarations

#define XCP_COMMUNICATION_THREAD

class cXCPConnector
{
public:
    cXCPConnector(void);
    ~cXCPConnector(void);

private:
#ifdef _WIN32
    WSADATA               m_wsa;                  //Info about current socket implementation
    SOCKADDR_IN           m_addr;                 //Info about socket
    SOCKET                m_socket;               //Socket
#else
    int                   m_socket;               //Socket
#endif
    bool                  m_SocketInitialized;    //Initialisierungsflags
    bool                  m_LoggingFlag;          //Logging of raw data to File allowed?
    unsigned char         m_SendState;            //Upload Mimik

    XCP_BASIC_REQUEST     m_Request;              //Current Request
    XCP_BASIC_RESPONSE    m_Response;             //Complete Response

#ifdef _WIN32
    SOCKADDR_IN           m_remoteAddr;           //Socket information
#endif
    unsigned short        m_Port;                 //UDP Communication Port

    //Segment information of attached software
    unsigned char         m_AmountSegments;       //Amount of segments
    unsigned char         m_ActivePage;           //Active Page of the Segment
    unsigned char         m_curSegmentMode;       //Current Segment Mode
    char                  *m_DataSegment;      // zB: "mysec"
    char                  *m_CodeSegment;      // zB: ".text"

    //MTA
    uint64_t         m_CurMTA;               //Aktueller MTA des letzten SET_MTA Commands
    uint64_t              m_CurMTA64;             //Aktueller MTA des letzten SET_MTA Commands nur intern (m_Identification, EventName) koennt ejetzt entfallen, da m_CurMTA auch 64bit breit!)
    unsigned char         m_CurMTAExtension;      //Aktuelle MTA Extension of last MTA
    uint32_t         m_CurMTATransferred;    //Aktuelle Transferlaenge der aktuellen Upload Sequenz
    int                   m_CurMTAIsNotTragetAddr;  // ist nicht eine Target Adresse sondern eine interne!
    //Download
    unsigned char         m_ExpectedElements;     //Expected Elements for Download
    unsigned char         m_ReceivedElements;     //Received until now

    //DAQ
    unsigned short        m_dynAllocatedDAQs;     //Amount of dyn. req DAQ Lists
    unsigned short        m_adressedDAQList;      //Currently adressed DAQ List
    unsigned short        m_DaqClock;             //Current DAQ Clock
    uint32_t         m_DaqPackageCounter;    //Counter of response messages
    XCP_DAQ               m_Daq[MAXDAQLISTS];     //Struktur fuer DAQ Transfer
    XCP_EVENT_DESC*       m_ptrEvents;            //Pointer to dynamic array of Events
    unsigned char         m_AllocatedEvents;      //Currently accolated events
    int*                  m_EventCycleCounters;
    //States
    unsigned char         m_ResourceProtectionState;    //Aktueller Protection Status der Resourcen
    unsigned char         m_SessionStatus;              //Aktueller Session Status

    //Identification & Logging
    char                  m_Identification[MAX_PATH]; //Identification of software
    char                  m_Logfile[MAX_PATH];               //Logfile (if enabled)
    FILE *                m_fh;

    //Misc
    unsigned short        m_RspPackageCounter;          //Counter of all Rsp. Packagaes

#ifdef XCP_COMMUNICATION_THREAD
    HANDLE                m_CommunicateThreadHandle;
    DWORD                 m_CommunicateThreadId;
    HANDLE                m_TerminateCommunicateThreadEvent;
    bool                  m_TerminateCommunicateThreadFlag;
    CRITICAL_SECTION      m_CriticalSection;
    HANDLE                m_SendEvent;
#endif
    int                   m_LoopSend;
    int                   m_WaitForNextCommand;

    int                   m_LinkNr;
    int                   m_Pid;
    char                  m_ProcessName[MAX_PATH];

    HANDLE                m_DoCycleEventHandle;
    HANDLE                m_hCycleFinishedEventHandle;

    cCopyPages            m_CopyPages;
    int                   m_Connected;
    bool                  m_DllAddressTranslation;

public:
    bool Init(int LinkNr, unsigned short Port, char *Identification, char *ProcessName, unsigned char ucAmEvents, XCP_EVENT_DESC* ptrEvents,
              char *DataSegment, char *CodeSegment, int LoggingActive, char *LogFile);
    void Terminate(void);
    bool Communicate(void);
    void SetIdentificationString(char*);
    void SetLogfile(char*);
    void XcpEvent(unsigned char);
    void IncrementCycleCounter(void);

    void ActivateDllAddressTranslation();
    void DeactivateDllAddressTranslation();

    void WriteLogString (const char *format, ...);
    void WriteLogByteBuffer (unsigned char *Buffer, int Size);

private:
    void Cyclic (void);

    bool ReInit(void);

    //direct socket Handling
    bool Receive(void);
    bool Send(bool bLog);
    void RespondDAQ(unsigned short);

    void HandleDisconnect();

    //Preparation of Response Data
    void PrepareConnectResponse();
    void PrepareCurrentStatus();
    void PrepareIdentification();
    void PreparePageProcessorInfo();
    void PrepareSegmentInfo();
    void PreparePageInfo();
    void PrepareGetCalPage();
    void PrepareChecksum();
    void PrepareErrResponse(unsigned char errorCode);
    void PrepareDAQProcessorInfo();
    void TakeSegmentMode();
    void TakeFirstBlockOfDownloadData();
    void TakeNextBlockOfDownloadData();
    void AllocateDAQList();
    void ClearDAQList();
    void FreeDAQList();
    void AllocateODT2DAQ();
    void AllocateODTEntry2DAQ();
    void SetDAQPointer();
    void WriteElement2ODTEntry();
    void SetDAQListMode();
    void HandleStartStopDAQList();
    void HandleStartStopDAQListSynch();
    void PrepareDAQResolutionInfo();
    void PrepareDAQEventInfo();
    void PrepareDaqTransmission(unsigned short usDaq);
    void GetSessionState();
    void HandleGetDaqClock();
    void CopyCalPage();
    void SetRequest();
    void HandleGetDAQListMode();
    void HandleSendEvent2Server(unsigned char ucEventCode);
    void ShortUpload();
    void ShortDownload();

    //DAQ Memory Handling
    void InitDAQ();
    void ResetDynamicDAQMemory();
    void CreateOrResetDAQTransferBuffers();

    //Uebernahme von Daten
    void TakeCurrentMTA();
    void TakeCalibrationPage();

    //Logging & Errorhandling
    void WriteLog (const char *Prefix, int DispCmd, XCP_BASIC_REQUEST *Buffer,bool bRequest,XCP_DAQ_RESPONSE *daqBuffer = NULL);

    //Internal Processing
    bool Process(unsigned char *ptrTelegram, unsigned short usTelLen);

    //Checksum
    uint32_t CalculateChecksumOfCurrentMTA(uint32_t ulBlocksize,unsigned char* ptrAlgorithm);

    bool InnerCommunicate(void);

    int GetSegmentName(char *SegmentDescString, int SegmentNr, char *par_or_ret_SegmentName, int MaxChars, unsigned char *ret_SegmentNo);
    int IsSegmentCallibationAble(unsigned char A2LSegmentNr, unsigned char *ret_ExeSegmentNo, unsigned char *ret_NoPages);
    int IsSegmentCallibationAble(char *SegmentName, unsigned char *ret_A2lSegmentNo, unsigned char *ret_ExeSegmentNo, unsigned char *ret_NoPages);

    int SwitchPagesAllSegments(int Page);

#ifdef XCP_COMMUNICATION_THREAD
    //Thread-Methode zur Kommunikation
    static int CommunicateThreadFunction (cXCPConnector *ThisPtr);
    bool StartCommunicateThreadFunction (void);
    bool TerminateCommunicateThreadFunction (void);
#endif
    uint64_t TranslateAddress(uint64_t par_Address);
};

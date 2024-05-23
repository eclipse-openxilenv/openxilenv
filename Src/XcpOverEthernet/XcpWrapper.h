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


#ifndef CXCPWRAPPER_H
#define CXCPWRAPPER_H

#include "XcpOverEthernetCfg.h"

#ifdef __cplusplus

class cXCPWrapper {
public:
    cXCPWrapper(void);
    ~cXCPWrapper(void);

    bool Init (int LinkNr, unsigned short Port, char *Identification, char *ProcessName, unsigned char ucAmEvents, XCP_EVENT_DESC* ptrEvents,
               char *DataSegment, char *CodeSegment, int LoggingActive, char *LogFile);
    void Terminate (void);
    bool Communicate (void);
    void SetIdentificationString (char* cpId);
    void SetLogfile (char* cpLogfile);
    void XcpEvent (unsigned char ucEvent);
    void IncrementCycleCounter(void);
    void ActivateDllAddressTranslation();
    void DeactivateDllAddressTranslation();
private:
    void *XCPConnector;
};

extern  "C" {
#endif // #ifdef __cplusplus

int XCPWrapperCreate (void);
void XCPWrapperDelete (void);
void XCPWrapperSetPort (unsigned long Port);
int XCPWrapperInit (int LinkNr, unsigned short Port, char *Identification, char *ProcessName, unsigned char ucAmEvents, XCP_EVENT_DESC* ptrEvents,
                    char *DataSegment, char *CodeSegment, int LoggingActive, char *LogFile);
void XCPWrapperTerminate (void);
int XCPWrapperCommunicate (void);

void XCPWrapperSetIdentificationString (char* cpId);
void XCPWrapperSetLogfile (char* cpLogfile);
void XCPWrapperXcpEvent (unsigned char ucEvent);
void XCPWrapperTakeCommunicationPort (unsigned short usPort);
void XCPWrapperIncrementCycleCounter(void);

#ifdef __cplusplus
}
#endif // #ifdef __cplusplus


#endif // #ifndef CXCPWRAPPER_H


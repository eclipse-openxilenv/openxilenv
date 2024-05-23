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


#include "Platform.h"
#include <stdio.h>
#include "XcpConnector.h"
#include "XcpWrapper.h"


cXCPWrapper::cXCPWrapper(void)
{
    XCPConnector = static_cast<void*>(new cXCPConnector);
}

cXCPWrapper::~cXCPWrapper(void)
{
    delete (static_cast<cXCPConnector*>(XCPConnector));
}


bool cXCPWrapper::Init(int LinkNr, unsigned short Port, char *Identification, char *ProcessName, unsigned char ucAmEvents, XCP_EVENT_DESC* ptrEvents,
                       char *DataSegment, char *CodeSegment, int LoggingActive, char *LogFile)
{
    return (static_cast<cXCPConnector*>(XCPConnector))->Init(LinkNr, Port, Identification, ProcessName, ucAmEvents, ptrEvents,
                                                DataSegment, CodeSegment, LoggingActive, LogFile);
}

void cXCPWrapper::Terminate(void)
{
    (static_cast<cXCPConnector*>(XCPConnector))->Terminate();
}

bool cXCPWrapper::Communicate(void)
{
    return (static_cast<cXCPConnector*>(XCPConnector))->Communicate();
}

void cXCPWrapper::SetIdentificationString(char* cpId)
{
    (static_cast<cXCPConnector*>(XCPConnector))->SetIdentificationString(cpId);
}

void cXCPWrapper::SetLogfile(char*cpLogfile)
{
    (static_cast<cXCPConnector*>(XCPConnector))->SetLogfile(cpLogfile);
}

void cXCPWrapper::XcpEvent(unsigned char ucEvent)
{
    (static_cast<cXCPConnector*>(XCPConnector))->XcpEvent(ucEvent);
}

void cXCPWrapper::IncrementCycleCounter(void)
{
    (static_cast<cXCPConnector*>(XCPConnector))->IncrementCycleCounter();
}

void cXCPWrapper::ActivateDllAddressTranslation()
{
    (static_cast<cXCPConnector*>(XCPConnector))->ActivateDllAddressTranslation();
}

void cXCPWrapper::DeactivateDllAddressTranslation()
{
    (static_cast<cXCPConnector*>(XCPConnector))->DeactivateDllAddressTranslation();
}


static cXCPConnector *__XCPConnector;

static int __XCPWrapperCreate (void)
{
    __XCPConnector = new cXCPConnector;
    return (__XCPConnector != nullptr);
}

static void __XCPWrapperDelete (void)
{
    if (__XCPConnector != nullptr) {
        delete (static_cast<cXCPConnector*>(__XCPConnector));
        __XCPConnector = nullptr;
    }
}

static bool __XCPWrapperInit (int LinkNr, unsigned short Port, char *Identification, char *ProcessName, unsigned char ucAmEvents,XCP_EVENT_DESC* ptrEvents,
                              char *DataSegment, char *CodeSegment, int LoggingActive, char *LogFile)
{
    if (__XCPConnector == nullptr) return FALSE;
    return __XCPConnector->Init (LinkNr, Port, Identification, ProcessName, ucAmEvents, ptrEvents,
                                 DataSegment, CodeSegment, LoggingActive, LogFile);
}

static void __XCPWrapperTerminate (void)
{
    if (__XCPConnector == nullptr) return;
    __XCPConnector->Terminate();
}

static bool __XCPWrapperCommunicate (void)
{
    if (__XCPConnector == nullptr) return FALSE;
    return __XCPConnector->Communicate();
}

static void __XCPWrapperSetIdentificationString (char* cpId)
{
    if (__XCPConnector == nullptr) return;
    __XCPConnector->SetIdentificationString (cpId);
}

static void __XCPWrapperSetLogfile (char*cpLogfile)
{
    if (__XCPConnector == nullptr) return;
    __XCPConnector->SetLogfile(cpLogfile);
}

static void __XCPWrapperXcpEvent (unsigned char ucEvent)
{
    if (__XCPConnector == nullptr) return;
    __XCPConnector->XcpEvent (ucEvent);
}

static void __XCPWrapperIncrementCycleCounter (void)
{
    if (__XCPConnector == nullptr) return;
    __XCPConnector->IncrementCycleCounter ();
}

extern "C" {

int XCPWrapperCreate (void)
{
    return __XCPWrapperCreate ();
}

void XCPWrapperDelete (void)
{
    __XCPWrapperDelete ();
}


int XCPWrapperInit (int LinkNr, unsigned short Port, char *Identification, char *ProcessName, unsigned char ucAmEvents, XCP_EVENT_DESC* ptrEvents,
                    char *DataSegment, char *CodeSegment, int LoggingActive, char *LogFile)
{
    return __XCPWrapperInit (LinkNr, Port, Identification, ProcessName, ucAmEvents, ptrEvents,
                             DataSegment, CodeSegment, LoggingActive, LogFile);
}

void XCPWrapperTerminate (void)
{
    __XCPWrapperTerminate();
}

int XCPWrapperCommunicate (void)
{
    return __XCPWrapperCommunicate();
}

void XCPWrapperSetIdentificationString (char* cpId)
{
    __XCPWrapperSetIdentificationString (cpId);
}

void XCPWrapperSetLogfile (char* cpLogfile)
{
    __XCPWrapperSetLogfile (cpLogfile);
}

void XCPWrapperXcpEvent (unsigned char ucEvent)
{
    __XCPWrapperXcpEvent (ucEvent);
}

void XCPWrapperIncrementCycleCounter (void)
{
    __XCPWrapperIncrementCycleCounter ();
}

}

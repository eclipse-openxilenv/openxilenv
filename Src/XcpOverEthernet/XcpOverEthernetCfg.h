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


#ifndef XCPOVERETHERNETCFG_H
#define XCPOVERETHERNETCFG_H

#define MAX_XCP_LINKS    4

// This struct exist also inside XilEnvProc.h
typedef struct
{
    unsigned char  timeCycle;                     //Event occures timecycle * timebase
    unsigned char  Prio;                          //Priority of the event
    unsigned char  timebase;                      //timebase on which timecycle depends on
    /* DAQ Timestamp Unit */
    #define DAQ_TIMESTAMP_UNIT_1NS     0
    #define DAQ_TIMESTAMP_UNIT_10NS    1
    #define DAQ_TIMESTAMP_UNIT_100NS   2
    #define DAQ_TIMESTAMP_UNIT_1US     3
    #define DAQ_TIMESTAMP_UNIT_10US    4
    #define DAQ_TIMESTAMP_UNIT_100US   5
    #define DAQ_TIMESTAMP_UNIT_1MS     6
    #define DAQ_TIMESTAMP_UNIT_10MS    7
    #define DAQ_TIMESTAMP_UNIT_100MS   8
    #define DAQ_TIMESTAMP_UNIT_1S      9
    char Fill1;
    int Fill2;
    char *EventName;
} XCP_EVENT_DESC;

#endif // XCPOVERETHERNETCFG_H

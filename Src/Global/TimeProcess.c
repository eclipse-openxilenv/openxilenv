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


#include <time.h>

#include "Config.h"
#include "ConfigurablePrefix.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#ifdef __linux__
#include <time.h>
#endif
#include "TimeProcess.h"

static struct {
    unsigned short Year;   // range [0000...]
    unsigned char Month;   // range [1..12]
    unsigned char Day;     // range [1..31]
    unsigned char Hour;    // range [0..23]
    unsigned char Minute;  // range [0..59]
    unsigned char Second;  // range [0..59]
    unsigned char Weekday; // range [0..6] == [Sunday..Saturday]
    VID VidYear;
    VID VidMonth;
    VID VidDay;
    VID VidHour;
    VID VidMinute;
    VID VidSecond;
    VID VidWeekday;
} TimeStruct;

void CyclicTimeProcess (void)
{
#ifdef __linux__
    time_t SystemTime;
    struct tm *LocalTime;

    SystemTime = time(&SystemTime);

    LocalTime = localtime(&SystemTime) ;

    TimeStruct.Year    = 1900 + LocalTime->tm_year;
    TimeStruct.Month   = (unsigned char)LocalTime->tm_mon + 1;
    TimeStruct.Day     = (unsigned char)LocalTime->tm_mday;
    TimeStruct.Hour    = (unsigned char)LocalTime->tm_hour;
    TimeStruct.Minute  = (unsigned char)LocalTime->tm_min;
    TimeStruct.Second  = (unsigned char)LocalTime->tm_sec;
    TimeStruct.Weekday = (unsigned char)LocalTime->tm_wday;
#else
    FILETIME FileTime;
    uint64_t Help;
    SYSTEMTIME SystemTime;
    Help = ((GetSimulatedStartTimeInNanoSecond() + GetSimulatedTimeInNanoSecond()) / 100);   // FILETIME is 100ns resolution
    FileTime = *(FILETIME*)&Help;
    FileTimeToSystemTime (&FileTime, &SystemTime);

    TimeStruct.Year    = SystemTime.wYear;
    TimeStruct.Month   = (unsigned char)SystemTime.wMonth;
    TimeStruct.Day     = (unsigned char)SystemTime.wDay;
    TimeStruct.Hour    = (unsigned char)SystemTime.wHour;
    TimeStruct.Minute  = (unsigned char)SystemTime.wMinute;
    TimeStruct.Second  = (unsigned char)SystemTime.wSecond;
    TimeStruct.Weekday = (unsigned char)SystemTime.wDayOfWeek;
#endif


    write_bbvari_uword (TimeStruct.VidYear, TimeStruct.Year);
    write_bbvari_ubyte (TimeStruct.VidMonth, TimeStruct.Month);
    write_bbvari_ubyte (TimeStruct.VidDay, TimeStruct.Day);
    write_bbvari_ubyte (TimeStruct.VidHour, TimeStruct.Hour);
    write_bbvari_ubyte (TimeStruct.VidMinute, TimeStruct.Minute);
    write_bbvari_ubyte (TimeStruct.VidSecond, TimeStruct.Second);
    write_bbvari_ubyte (TimeStruct.VidWeekday, TimeStruct.Weekday);
}

int InitTimeProcess (void)
{
    char Name[BBVARI_NAME_SIZE];

    TimeStruct.VidYear = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".Time.Year", Name, sizeof(Name)), BB_UWORD, "");
    TimeStruct.VidMonth = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".Time.Month", Name, sizeof(Name)), BB_UBYTE, "");
    TimeStruct.VidDay = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".Time.Day", Name, sizeof(Name)), BB_UBYTE, "");
    TimeStruct.VidHour = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".Time.Hour", Name, sizeof(Name)), BB_UBYTE, "h");
    TimeStruct.VidMinute = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".Time.Minute", Name, sizeof(Name)), BB_UBYTE, "min");
    TimeStruct.VidSecond = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".Time.Second", Name, sizeof(Name)), BB_UBYTE, "s");
    TimeStruct.VidWeekday = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD, ".Time.Weekday", Name, sizeof(Name)), BB_UBYTE, "");
    return 0;
}

void TerminateTimeProcess (void)
{
    remove_bbvari (TimeStruct.VidYear);
    remove_bbvari (TimeStruct.VidMonth);
    remove_bbvari (TimeStruct.VidDay);
    remove_bbvari (TimeStruct.VidHour);
    remove_bbvari (TimeStruct.VidMinute);
    remove_bbvari (TimeStruct.VidSecond);
    remove_bbvari (TimeStruct.VidWeekday);
}


TASK_CONTROL_BLOCK TimeTcb
= INIT_TASK_COTROL_BLOCK("TimeProcess", INTERN_ASYNC, 1, CyclicTimeProcess, InitTimeProcess, TerminateTimeProcess, 0);



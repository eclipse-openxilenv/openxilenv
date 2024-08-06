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
#include "Platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "Config.h"
#include "Scheduler.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "IniDataBase.h"
#include "EquationParser.h"
#include "BlackboardAccess.h"
#include "Message.h"
#include "CcpControl.h"
#include "XcpControl.h"
#include "MainValues.h"
#include "ScriptChangeSettings.h"
#include "InterfaceToScript.h"

#include "EnvironmentVariables.h"

static CRITICAL_SECTION EnvironmentCriticalSection;


int InitEnvironmentVariables (void)
{
    INIT_CS (&EnvironmentCriticalSection);
    srand ((unsigned)time (NULL));
    return 0;
}


struct USER_ENV_VAR {
    char *Name;
    char *Value;
    int NameLen;
    int ValueLen;
};

static struct USER_ENV_VAR *UserEnvVars;
static int UserEnvVarCounter;

static int SearchUserEnvironmentVariableNoCriticalSection (const char *Name, char *RetValue, int maxc)
{
    int x;

    for (x = 0; x < UserEnvVarCounter; x++) {
        if (UserEnvVars[x].Name != NULL) {
            if (!strcmp (Name, UserEnvVars[x].Name)) {
                if ((RetValue != NULL) && (UserEnvVars[x].Value != NULL)) {
                    strncpy (RetValue, UserEnvVars[x].Value, (size_t)maxc);
                    RetValue[maxc-1] = 0;
                }
                return (x+1);   // Position 1 is first entry
            }
        }
    }
    return 0;  // not found
}

static int SearchUserEnvironmentVariable (char *Name, char *RetValue, int maxc)
{
    int Ret;
    ENTER_CS (&EnvironmentCriticalSection);
    Ret = SearchUserEnvironmentVariableNoCriticalSection (Name, RetValue, maxc);
    LEAVE_CS (&EnvironmentCriticalSection);
    return Ret;
}


static int AddUserEnvironmentVariable (const char *Name, const char *Value)
{
    int pos;

__REPEAT:
    for (pos = 0; pos < UserEnvVarCounter; pos++) {
        if (UserEnvVars[pos].Name == NULL) {
            goto __FOUND;
        }
    }
    UserEnvVarCounter += 10;
    UserEnvVars = (struct USER_ENV_VAR*)my_realloc (UserEnvVars, (size_t)UserEnvVarCounter * sizeof (struct USER_ENV_VAR));
    if (UserEnvVars == NULL) return -1;
    memset (UserEnvVars + UserEnvVarCounter - 10, 0, 10 * sizeof (struct USER_ENV_VAR));
    goto __REPEAT;
__FOUND:
    UserEnvVars[pos].NameLen = (int)strlen (Name) + 1;
    UserEnvVars[pos].ValueLen = (int)strlen (Value) + 1;

    UserEnvVars[pos].Name = my_malloc ((size_t)UserEnvVars[pos].NameLen);
    UserEnvVars[pos].Value = my_malloc ((size_t)UserEnvVars[pos].ValueLen);
    if ((UserEnvVars[pos].Name == NULL) || (UserEnvVars[pos].Value == NULL)) return -1;
    strcpy (UserEnvVars[pos].Name, Name);
    strcpy (UserEnvVars[pos].Value, Value);
    return 0;
}

int SetUserEnvironmentVariable (const char *Name, const char *Value)
{
    int pos;

    ENTER_CS (&EnvironmentCriticalSection);
    // Check if already exists
    if ((pos = SearchUserEnvironmentVariableNoCriticalSection (Name, NULL, 0)) != 0) {
        // overwrite
        pos--;
        my_free (UserEnvVars[pos].Value);
        UserEnvVars[pos].ValueLen = (int)strlen (Value) + 1;
        UserEnvVars[pos].Value = my_malloc (UserEnvVars[pos].ValueLen);
        MEMCPY (UserEnvVars[pos].Value, Value, UserEnvVars[pos].ValueLen);
    } else {
        // add new
        AddUserEnvironmentVariable (Name, Value);
    }
    LEAVE_CS (&EnvironmentCriticalSection);
    return 0;
}


int GetEnvironmentVariableList (char **Buffer, int *SizeOfBuffer,
                                char **RefBuffer, int *SizeOfRefBuffer)
{
    int x;
    int Size;
    char *p, *r;
    int Ret = 0;

    ENTER_CS (&EnvironmentCriticalSection);
    // first calculate needed buffer size
    Size = 1;   // terminating 0 char
    for (x = 0; x < UserEnvVarCounter; x++) {
        if (UserEnvVars[x].Name != NULL) {
            Size += UserEnvVars[x].NameLen + UserEnvVars[x].ValueLen + 2;
        }
    }
    if (*SizeOfBuffer < Size) {
        *SizeOfBuffer = Size;
        *Buffer = my_realloc (*Buffer, (size_t)Size);
        if (*Buffer == NULL) {
            LEAVE_CS (&EnvironmentCriticalSection);
            ThrowError (1, "out of memory");
            return 0;
        }
    }
    p = *Buffer;
    r = *RefBuffer;
    for (x = 0; x < UserEnvVarCounter; x++) {
        if (UserEnvVars[x].Name != NULL) {
            char *ps = p;
            MEMCPY (p, UserEnvVars[x].Name, (size_t)UserEnvVars[x].NameLen);
            p += UserEnvVars[x].NameLen - 1;
            MEMCPY (p, " = ", 3);
            p += 3;
            MEMCPY (p, UserEnvVars[x].Value, (size_t)UserEnvVars[x].ValueLen);
            p += UserEnvVars[x].ValueLen - 1;
            *p = 0;
            p++;
            if (Ret == 0) {
                if (r == NULL) {
                    Ret = 1;
                } else if (strcmp (r, ps)) {
                    Ret = 1;
                } else {
                    r += p - ps;
                }
            }
        }
    }
    *p = 0;
    if (Ret) {
        if (*SizeOfRefBuffer < *SizeOfBuffer) {
            *SizeOfRefBuffer = *SizeOfBuffer;
            *RefBuffer = my_realloc (*RefBuffer, (size_t)*SizeOfRefBuffer);
            if (*RefBuffer == NULL) {
                LEAVE_CS (&EnvironmentCriticalSection);
                ThrowError (1, "out of memory");
                return 0;
            }
        }
        MEMCPY (*RefBuffer, *Buffer, (size_t)Size);
    }
    LEAVE_CS (&EnvironmentCriticalSection);
    return Ret;
}


int SearchAndReplaceEnvironmentStrings (const char *src, char *dest, int maxc)
{
    return SearchAndReplaceEnvironmentStringsExt (src, dest, maxc, NULL, NULL);
}

static int StartWith(const char *par_EqualTo, char *par_String, char **ret_Next)
{
    if (s_main_ini_val.EnableLegacyEnvironmentVariables) {
        if ((par_EqualTo[0] == 'X') && (par_EqualTo[1] == 'I') && (par_EqualTo[2] == 'L') &&
            (par_EqualTo[3] == 'E') && (par_EqualTo[4] == 'N') && (par_EqualTo[5] == 'V') &&
            (par_String[0] == 'S') && (par_String[1] == 'C')) {
            par_String += 2;
            par_EqualTo += 6;
        }
    }
    while(*par_EqualTo == *par_String) {
        par_EqualTo++;
        par_String++;
        if (*par_EqualTo == 0) {
            *ret_Next = par_String;
            return 1;
        }
    }
    return 0;
}

static int EqualWith(const char *par_EqualTo, char *par_String)
{
    if (s_main_ini_val.EnableLegacyEnvironmentVariables) {
        if ((par_EqualTo[0] == 'X') && (par_EqualTo[1] == 'I') && (par_EqualTo[2] == 'L') &&
            (par_EqualTo[3] == 'E') && (par_EqualTo[4] == 'N') && (par_EqualTo[5] == 'V') &&
            (par_String[0] == 'S') && (par_String[1] == 'C')) {
            par_String += 2;
            par_EqualTo += 6;
        }
    }
    while(*par_EqualTo == *par_String) {
        if (*par_EqualTo == 0) {
            if (*par_String == 0) {
                return 1;
            } else {
                return 0;
            }
        }
        par_EqualTo++;
        par_String++;
    }
    return 0;
}


int SearchAndReplaceEnvironmentStringsExt (const char *src, char *dest, int maxc, int *pSolvedEnvVars, int *pNoneSolvedEnvVars)
{
    char *s, *os, *d, *h;
    const char *ch;
    int cc;
    char *src_buf;
    int ErrOutFlag = (maxc & 0x40000000) != 0x40000000;
    int SolvedEnvVarsCounter = 0;
    int NoneSolvedEnvVarsCounter = 0;

    maxc &= 0x3FFFFFFF;

    src_buf = _alloca(strlen (src)+1);
    strcpy (src_buf, src);

    cc = 0;
    s = src_buf;
    d = dest;
    while (*s != 0) {
        if (*s == '%') {    // Environment variable ?
            char *Next;
            char EnvVarName[1024];
            char EnvVarValue[2048];
            int envcc;
            s++;
            os = s-1;          // remember old position
            h = EnvVarName;
            envcc = 0;
            while (*s != '%') {
                envcc++;
                if ((envcc >= (int)sizeof (EnvVarName)) || (*s == 0) || (isascii (*s) && isspace (*s))) {
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                }
                *h++ = *s++;
            }
            *h = 0;
            s++;  // jump over %
            if (EqualWith ("XILENV_EXE_DIR", EnvVarName)) {
                SolvedEnvVarsCounter++;
#ifdef _WIN32
                GetModuleFileName (GetModuleHandle(NULL), EnvVarValue, sizeof (EnvVarValue));
#else
                readlink("/proc/self/exe", EnvVarValue, sizeof (EnvVarValue));
#endif
                h = EnvVarValue;
                while (*h != 0) h++;
                while ((*h != '\\') && (*h != '/') && (h > EnvVarValue)) h--;
                *h = 0;
            } else if (EqualWith ("XILENV_WORK_DIR", EnvVarName)) {    // Work folder from basic settings
                SolvedEnvVarsCounter++;
                strcpy (EnvVarValue, s_main_ini_val.WorkDir);
            } else if (EqualWith ("XILENV_CURRENT_DIR", EnvVarName)) {    // current folder
                SolvedEnvVarsCounter++;
                GetCurrentDirectory (maxc, EnvVarValue);
            } else if (EqualWith("XILENV_SCRIPT_PATH", EnvVarName)) { // Name of the running script
                const char *ScriptFilename;
                ScriptFilename = GetRunningScriptPath ();
                if (ScriptFilename == NULL) { // no script is running
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                }
                strcpy (EnvVarValue, ScriptFilename);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_SCRIPT_DIR", EnvVarName)) {  // Verzeichnis des aktuell laufenden Scriptfiles
                const char *ScriptPath;
                ScriptPath = GetRunningScriptPath ();
                if (ScriptPath == NULL) { // no script is running
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                }
                strcpy (EnvVarValue, ScriptPath);
                h = EnvVarValue;
                while (*h != 0) h++;
                while ((*h != '\\') && (*h != '/') && (h > EnvVarValue)) h--;
                *h = 0;
                SolvedEnvVarsCounter++;
            } else if (EqualWith("XILENV_INI_PATH", EnvVarName)) { // Name of the INI file
                IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), EnvVarValue, sizeof(EnvVarValue));
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_INI_DIR", EnvVarName)) {  // Folder of the current loaded INI file
                IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), EnvVarValue, sizeof(EnvVarValue));
                h = EnvVarValue;
                while (*h != 0) h++;
                while ((*h != '\\') && (*h != '/') && (h > EnvVarValue)) h--;
                *h = 0;
                SolvedEnvVarsCounter++;
            } else if (EqualWith("XILENV_SCRIPT_NAME", EnvVarName)) { // Name of the running Script file
                const char *ScriptFilename;
                ScriptFilename = GetRunningScriptPath ();
                if (ScriptFilename == NULL) { // no script is running
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                }
                ch = ScriptFilename;
                while (*ch != 0) ch++;
                while ((*ch != '\\') && (*ch != '/') && (ch > EnvVarValue)) ch--;
                if ((*ch == '\\') || (*ch == '/')) ch++;
                strcpy (EnvVarValue, ch);   // only file name without path
                SolvedEnvVarsCounter++;
            } else if (EqualWith("XILENV_SCRIPT_LINE", EnvVarName)) { // Name of the running Script file
                int LineNr;
                LineNr = GetRunningScriptLine ();
                if (LineNr >= 0) sprintf (EnvVarValue, "%i", LineNr); 
                else strcpy (EnvVarValue, "unknown");
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP_SWSTAND", EnvVarName)) {    // The name of the software version received by CCP
                GetECUString_CCP (0, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP_INFOS", EnvVarName)) {    // Get complete infos for CCP connection
                GetECUInfos_CCP (0, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP0_SWSTAND", EnvVarName)) {    // The name of the software version received by CCP
                GetECUString_CCP (0, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP0_INFOS", EnvVarName)) {    // Get complete infos for CCP connection
                GetECUInfos_CCP (0, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP1_SWSTAND", EnvVarName)) {    // The name of the software version received by CCP
                GetECUString_CCP (1, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP1_INFOS", EnvVarName)) {    // Get complete infos for CCP connection
                GetECUInfos_CCP (1, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP2_SWSTAND", EnvVarName)) {    // The name of the software version received by CCP
                GetECUString_CCP (2, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP2_INFOS", EnvVarName)) {    // Get complete infos for CCP connection
                GetECUInfos_CCP (2, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP3_SWSTAND", EnvVarName)) {    // The name of the software version received by CCP
                GetECUString_CCP (3, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_CCP3_INFOS", EnvVarName)) {    // Get complete infos for CCP connection
                GetECUInfos_CCP (3, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_XCP0_SWSTAND", EnvVarName)) {    // The name of the software version received by XCP
                GetECUString_XCP (0, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_XCP1_SWSTAND", EnvVarName)) {    // The name of the software version received by XCP
                GetECUString_XCP (1, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_XCP2_SWSTAND", EnvVarName)) {    // The name of the software version received by XCP
                GetECUString_XCP (2, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_XCP3_SWSTAND", EnvVarName)) {    // The name of the software version received by XCP
                GetECUString_XCP (3, EnvVarValue, maxc);
                SolvedEnvVarsCounter++;
            } else if (StartWith ("XILENV_VARIABLE", EnvVarName, &Next)) {  // Value of a blackboard variable
                double erg;
                int FormatChar = 'g';   // If not defined use %g as format spezifier
                char *p = Next;
                char FormatString[16+BBVARI_NAME_SIZE];
                char *f = FormatString;
                int Size = 0;
                char *ErrStr;
                int Vid = -1;
                int Color;

                if (*p == '[') {  // Is there a format spezifier?
                    p++;
                    *f++ = '%';
                    while (*p != ']') {
                        if ((*p == 0) || (Size >= (int)(sizeof (FormatString)-1))){
                            if (ErrOutFlag) ThrowError (1, "Environment variable XILENV_VARIABLE: contains error in format specification field []");
                            s = os;       // it was not an environment variable
                            goto NO_ENV_VAR;
                        }
                        *f = *p;
                        p++;
                        f++;
                        Size++;
                    }
                    *f = 0;
                    FormatChar = *(p-1);
                    p++;
                    if (!strncmp("%enum", FormatString, 5)) {
                        char *VariableName;
                        FormatChar = 1000;
                        f = FormatString+5; // jump over "enum" string
                        while (isascii(*f) && isspace(*f)) {
                            f++;
                        }
                        if (*f != '(') {
                            if (ErrOutFlag) ThrowError (1, "Environment variable XILENV_VARIABLE: contains error in format specification field [], ecpecting a '(' after enum");
                            s = os;       // it was not an environment variable
                            goto NO_ENV_VAR;
                        }
                        f++;  // '('
                        while (isascii(*f) && isspace(*f)) {
                            f++;
                        }
                        VariableName = f;
                        while (*f != ')') {
                            if (*f == 0) {
                                if (ErrOutFlag) ThrowError (1, "Environment variable XILENV_VARIABLE: contains error in format specification field [], ecpecting a ')' after enum");
                                s = os;       // it was not an environment variable
                                goto NO_ENV_VAR;
                            }
                            f++;
                        }
                        while (isascii(*(f-1)) && isspace(*(f-1)) && (f > VariableName)) {
                            f--;
                        }
                        *f = 0;
                        Vid = get_bbvarivid_by_name(VariableName);
                        if (Vid <= 0) {
                            if (ErrOutFlag) ThrowError (1, "Environment variable XILENV_VARIABLE: contains error in format specification field [], variable \"%s\" doesn't exists", VariableName);
                            s = os;       // it was not an environment variable
                            goto NO_ENV_VAR;
                        }
                    }
                } else {
                    strcpy (FormatString, "%g");  //If not defined use %g as format spezifier
                    FormatChar = 'g';
                }
                if (*p != ':') {
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                }
                p++;
                if(direct_solve_equation_err_string (p, &erg, &ErrStr)) {
                    if (ErrOutFlag) ThrowError (1, "in environment variable XILENV_VARIABLE: \"%s\"", ErrStr);
                    FreeErrStringBuffer (&ErrStr);
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                }
                // %[flags] [width] [.precision] type
                switch (FormatChar) {
                case 'c':
                case 'i':
                case 'd':
                    sprintf (EnvVarValue, FormatString, (long)erg);
                    break;
                case 'u':
                case 'o':
                case 'x':
                case 'X':
                    sprintf (EnvVarValue, FormatString, (unsigned long)erg);
                    break;
                case 'e':
                case 'E':
                case 'f':
                case 'g':
                case 'G':
                    sprintf (EnvVarValue, FormatString, erg);
                    break;
                case 1000:
                    {
                        int ret = convert_value_textreplace (Vid, (long)erg, EnvVarValue, BBVARI_ENUM_NAME_SIZE, &Color);
                        if ((ret < 0) &&
                            (ret != -2)) {   // -2 is "out of range", this should not result into an error message
                            if (ErrOutFlag) ThrowError (1, "Environment variable XILENV_VARIABLE: contains error in format specification field [], variable has no enum conversion type");
                            s = os;       // it was not an environment variable
                            goto NO_ENV_VAR;
                        }
                    }
                    break;
                default:
                    if (ErrOutFlag) ThrowError (1, "not valid format specification field [%s] in environment variable XILENV_VARIABLE", FormatChar);
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                    // no break;
                } 
                SolvedEnvVarsCounter++; 
            } else if (StartWith ("XILENV_PROCESS_DIR:", EnvVarName, &Next)) {  // Folder of an external process
                int pid;
                if((pid = get_pid_by_name(Next)) <= 0) {
                    if (ErrOutFlag) ThrowError (1, "Process '%s' in environment variable XILENV_PROCESS_DIR: doesn't exist", Next);
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                }
                get_name_by_pid (pid, EnvVarValue);
                h = EnvVarValue;
                while (*h != 0) h++;
                while ((*h != '\\') && (*h != '/') && (h > EnvVarValue)) h--;
                *h = 0;
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_RANDOM", EnvVarName)) {  // Random value
                sprintf (EnvVarValue, "%d", rand());
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_TIME_STRING", EnvVarName)) {  // Time string
#ifdef _WIN32
                SYSTEMTIME Time;
                GetLocalTime (&Time);
                GetTimeFormat (LOCALE_USER_DEFAULT, // locale
                               0,                   // options
                               &Time,               // time
                               "hh':'mm':'ss",      // time format string
                               EnvVarValue,         // formatted string buffer
                               maxc);               // size of string buffer
#else
                time_t Time;
                struct tm LocalTime, *pLocalTime;
                Time = time(NULL);
                pLocalTime = localtime_r(&Time, &LocalTime);
                sprintf (EnvVarValue, "%02u:%02u:%02u", (unsigned int)LocalTime.tm_hour, (unsigned int)LocalTime.tm_min, (unsigned int)LocalTime.tm_sec);
#endif
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_TIME2_STRING", EnvVarName)) {  // Time string
#ifdef _WIN32
                SYSTEMTIME Time;
                GetLocalTime (&Time);
                GetTimeFormat (LOCALE_USER_DEFAULT, // locale
                               0,                   // options
                               &Time,               // time
                               "hh'.'mm'.'ss",      // time format string
                               EnvVarValue,         // formatted string buffer
                               maxc);               // size of string buffer
#else
                time_t Time;
                struct tm LocalTime, *pLocalTime;
                Time = time(NULL);
                pLocalTime = localtime_r(&Time, &LocalTime);
                sprintf (EnvVarValue, "%02u.%02u.%02u", (unsigned int)LocalTime.tm_hour, (unsigned int)LocalTime.tm_min, (unsigned int)LocalTime.tm_sec);
#endif
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_DATE_STRING", EnvVarName)) {  // Date string
#ifdef _WIN32
                SYSTEMTIME Time;
                GetLocalTime (&Time);
                GetDateFormat (LOCALE_USER_DEFAULT, // locale
                               0,                   // options
                               &Time,               // time
                               "dd'.'MM'.'yyyy",    // date format
                               EnvVarValue,         // formatted string buffer
                               maxc);               // size of string buffer
#else
                time_t Time;
                struct tm LocalTime, *pLocalTime;
                Time = time(NULL);
                pLocalTime = localtime_r(&Time, &LocalTime);
                sprintf (EnvVarValue, "%02u.%02u.%04u", (unsigned int)LocalTime.tm_mday, (unsigned int)LocalTime.tm_mon, (unsigned int)LocalTime.tm_year + 1900);
#endif
                SolvedEnvVarsCounter++;
            } else if (EqualWith ("XILENV_WINDOWS_TICK_COUNTER", EnvVarName)) {
                DWORD TickCount;
                TickCount = GetTickCount();
                sprintf (EnvVarValue, "%u", (uint32_t)TickCount);
                SolvedEnvVarsCounter++;
            } else if (StartWith ("XILENV_BASIC_SETTINGS:", EnvVarName, &Next)) {  // Folder of an external process
                char *RetPointer;
                if (ScriptChangeBasicSettings (CHANGE_SETTINGS_COMMAND_GET, 0, Next, (char*)&RetPointer) == 0) {
                    size_t Len = strlen(RetPointer);
                    if (Len >= sizeof (EnvVarValue)) {
                        Len = sizeof (EnvVarValue) - 1;
                    }
                    MEMCPY (EnvVarValue, RetPointer, Len);
                    my_free (RetPointer); // this will be alloc inside in ScriptChangeBasicSettings
                    EnvVarValue[Len] = 0;
                    SolvedEnvVarsCounter++;
                } else {
                    if (ErrOutFlag) ThrowError (1, "in environment variable XILENV_BASIC_SETTINGS: \"%s\" is a not valid basic setting name", Next);
                    s = os;       // it was not an environment variable
                    goto NO_ENV_VAR;
                }
            } else if (SearchUserEnvironmentVariable (EnvVarName, EnvVarValue, sizeof (EnvVarValue))) {
                SolvedEnvVarsCounter++;
            } else if (IniFileDataBaseReadString ("UserDefinedEnvironmentVariables", EnvVarName, "",
                                                  EnvVarValue, sizeof (EnvVarValue), GetMainFileDescriptor()) > 0) {
                SolvedEnvVarsCounter++;
            } else if (GetEnvironmentVariable(EnvVarName,               // Is it a system environment variable?
                                              EnvVarValue,
                                              sizeof (EnvVarValue))) {
                SolvedEnvVarsCounter++;
            } else {
                NoneSolvedEnvVarsCounter++;   // There is something with %xxxx% but it is not terminable
                s = os;       // it was not an environment variable
                goto NO_ENV_VAR;
            }
            // Copy environment variablen content
            h = EnvVarValue;
            while (*h != 0) {
                cc++;
                if (cc >= maxc) {
                    *d = 0;
                    if (pNoneSolvedEnvVars != NULL) *pNoneSolvedEnvVars = NoneSolvedEnvVarsCounter;
                    if (pSolvedEnvVars != NULL) *pSolvedEnvVars = SolvedEnvVarsCounter;
                    return -1;
                }
                *d++ = *h++;
            }
        } else {
          NO_ENV_VAR:
            cc++;
            if (cc >= maxc) {
                *d = 0;
                if (pNoneSolvedEnvVars != NULL) *pNoneSolvedEnvVars = NoneSolvedEnvVarsCounter;
                if (pSolvedEnvVars != NULL) *pSolvedEnvVars = SolvedEnvVarsCounter;
                return -1;
            }
            *d++ = *s++;
        }
    }
    *d = 0;
    if (pNoneSolvedEnvVars != NULL) *pNoneSolvedEnvVars = NoneSolvedEnvVarsCounter;
    if (pSolvedEnvVars != NULL) *pSolvedEnvVars = SolvedEnvVarsCounter;
    return cc;
}

int CheckIfEnvironmentVariableExist (const char *Name)
{
    char In[MAX_PATH*4];
    char Out[MAX_PATH*4];

    if ((strlen(Name) + 3) > sizeof(In)) return -1;
    if (strstr (Name, "%") != NULL) return -2;
    sprintf (In, "%%%s%%", Name);
    SearchAndReplaceEnvironmentStrings (In, Out, sizeof (Out));
    if (strcmp (In, Out)) {
        return 1;
    } else {
        return 0;
    }
}

int RemoveUserEnvironmentVariable (const char *Name)
{
    int x;

    for (x = 0; x < UserEnvVarCounter; x++) {
        if (UserEnvVars[x].Name != NULL) {
            if (!strcmp (Name, UserEnvVars[x].Name)) {
                my_free (UserEnvVars[x].Value);
                UserEnvVars[x].Value = NULL;
                my_free (UserEnvVars[x].Name);
                UserEnvVars[x].Name = NULL;
                return 0;
            }
        }
    }
    return -1;
}

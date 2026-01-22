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
#include "MyMemory.h"
#include "RunTimeMeasurement.h"

//#define FUNCTION_RUNTIME

#ifdef FUNCTION_RUNTIME
typedef struct {
    const char *FunctionName;
    uint64_t CallCounter;
    uint64_t Start;
    uint64_t End;
    uint64_t Last;
    uint64_t Sum;
} RUNTIME_INFOS;


RUNTIME_INFOS *RuntimeInfos;
int RuntimeInfosCount;
int RuntimeInfosSize;

/*
Beispiel: Aufruf:

int TestFunktion (void)
{
    ...
    static int RunTimeId;

    RunTimeId = BeginRuntimeMeassurement ("TestFunktion", RunTimeId);

    ...

    EndRuntimeMeassurement (RunTimeId);
    return 1;
}
*/

__inline uint64_t GetTimeStamp()
{
    uint64_t retval;
    retval  = __rdtsc();
    return retval;
}



int BeginRuntimeMeassurement (const char *Name, int id)
{
    if (id == 0) {    // erster Aufruf eine Id ermitteln
        if (!RuntimeInfosCount) RuntimeInfosCount++;   // Element 0 muss leer bleiben
        id = RuntimeInfosCount;
        RuntimeInfosCount++;
        if (RuntimeInfosCount >= RuntimeInfosSize ) {
            RuntimeInfosSize += 64;
            RuntimeInfos = (RUNTIME_INFOS*)my_realloc (RuntimeInfos, RuntimeInfosSize * sizeof (RUNTIME_INFOS));
        }
        MEMSET (&(RuntimeInfos[id]), 0, sizeof (RUNTIME_INFOS));
    } 
    RuntimeInfos[id].CallCounter++;
    RuntimeInfos[id].Start = GetTimeStamp();
    RuntimeInfos[id].FunctionName = Name;
    return id;
}


int EndRuntimeMeassurement (int id, unsigned int max)
{
    RuntimeInfos[id].End = GetTimeStamp();
    RuntimeInfos[id].Last = RuntimeInfos[id].End - RuntimeInfos[id].Start;
    RuntimeInfos[id].Sum += RuntimeInfos[id].Last;
    return (max && (RuntimeInfos[id].Last > max)); 
}

static double GetCPUFreqFromRegestry (void)
{
    HKEY hKey;
    DWORD Type;
    DWORD Data;
    DWORD Size;
    double Ret = 1000000000.0;

    if (RegOpenKey (HKEY_LOCAL_MACHINE, //HKEY hKey,
                    "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", //LPCTSTR lpSubKey,
                    &hKey) == ERROR_SUCCESS) {  //PHKEY phkResult
        Size = sizeof (Data); 
        if (RegQueryValueEx (hKey, "~MHz", NULL, &Type, (LPBYTE)&Data, &Size) == ERROR_SUCCESS) {
            Ret = Data * 1000000.0;   // return with unit Hz
        } 
        RegCloseKey (hKey);
    }
    return Ret;
}


int GetRuntimeMeassurement (int id, char *line)
{
	static double CPUClock;
    if (id == 0) {                          // beginning
        id = 1;
    }
	if (CPUClock <= 1.0) {
		CPUClock = GetCPUFreqFromRegestry ();
	}
    if (id >= RuntimeInfosCount) return 0;  // Ende
    PrintFormatToString (line, sizeof(line), "%s  %I64d Call(s)  %fs %fs/call",  //%I64d CPU-Clocks",
             RuntimeInfos[id].FunctionName,
             RuntimeInfos[id].CallCounter,
             (double)RuntimeInfos[id].Sum / CPUClock,
             (double)RuntimeInfos[id].Sum / CPUClock / (double)RuntimeInfos[id].CallCounter);
    return id + 1;
}

#endif

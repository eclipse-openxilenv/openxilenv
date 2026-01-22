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

#include "ThrowError.h"
#include "MyMemory.h"
#include "UniqueNumber.h"

static CRITICAL_SECTION UniqueNumberCriticalSection;


static int GeneratUniqueNuberCounter;
static int *UsedUniqueNumbers;
static int SizeOfUsedUniqueNumbers;
static int CountUsedUniqueNumbers;

void InitUniqueNumbers (void)
{
    InitializeCriticalSection (&UniqueNumberCriticalSection);
    GeneratUniqueNuberCounter = 1;   // One is the first number
}

void TerminateUniqueNumbers (void)
{
    EnterCriticalSection (&UniqueNumberCriticalSection);
    if (UsedUniqueNumbers != NULL) my_free(UsedUniqueNumbers);
    UsedUniqueNumbers = NULL;
    GeneratUniqueNuberCounter = 1;
    LeaveCriticalSection (&UniqueNumberCriticalSection);
}

int AquireUniqueNumber (void)
{
    int x, Ret;

    EnterCriticalSection (&UniqueNumberCriticalSection);
    if ((CountUsedUniqueNumbers + 1) >= SizeOfUsedUniqueNumbers) {
        SizeOfUsedUniqueNumbers += 1024;
        UsedUniqueNumbers = my_realloc (UsedUniqueNumbers, (size_t)SizeOfUsedUniqueNumbers * sizeof (UsedUniqueNumbers[0]));
        if (UsedUniqueNumbers == NULL) {
            ThrowError (1, "Out of memory %s (%i)", __FILE__, __LINE__);
            return INVALID_UNIQUE_NUMBER;
        }
    }
    for (;;) {
        GeneratUniqueNuberCounter++;
        if (GeneratUniqueNuberCounter == MAX_UNIQUE_NUMBER_VALUE) GeneratUniqueNuberCounter = 1;
        for (x = 0; x < CountUsedUniqueNumbers; x++) {
            if (UsedUniqueNumbers[x] == GeneratUniqueNuberCounter) break;
        }
        if (x == CountUsedUniqueNumbers) break;
    }
    UsedUniqueNumbers[CountUsedUniqueNumbers] = GeneratUniqueNuberCounter;
    CountUsedUniqueNumbers++;
    Ret = GeneratUniqueNuberCounter;
    LeaveCriticalSection (&UniqueNumberCriticalSection);
    return Ret;
}


void FreeUniqueNumber (int par_Number)
{
    int x, xx;
    int found = 0;
    EnterCriticalSection (&UniqueNumberCriticalSection);

    for (x = 0; x < CountUsedUniqueNumbers; x++) {
        if (UsedUniqueNumbers[x] == par_Number) {
            for (xx = x + 1; xx < CountUsedUniqueNumbers; xx++) {
                UsedUniqueNumbers[xx - 1] = UsedUniqueNumbers[xx];
            }
            CountUsedUniqueNumbers--;
            found = 1;
            break;
        }
    }
    if (!found) {
        ThrowError (1, "Try to free a unknown unique number %u", par_Number);
    }
    LeaveCriticalSection (&UniqueNumberCriticalSection);
}

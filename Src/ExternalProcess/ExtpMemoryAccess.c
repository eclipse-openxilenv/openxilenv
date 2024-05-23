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
#ifndef _WIN32
#include <sys/mman.h>
#endif
#include "XilEnvExtProc.h"
#include "StringMaxChar.h"
#include "ExtpXcpCopyPages.h"

int XilEnvInternal_EnableWriteAccressToMemory (void *par_Address, size_t par_Size)
{
#ifdef _WIN32
    DWORD OldProtect;
    return (VirtualProtect(par_Address, (int)par_Size, PAGE_READWRITE, &OldProtect) == 0) ? -1 : 0;
#else
    static int PageSize;
    if (PageSize == 0) {
        PageSize = sysconf(_SC_PAGE_SIZE);
    }
    if (PageSize > 0) {
        void *PageAlignedStartAddress;
        int PageAlignedSize;
        size_t PageNoStart = (size_t)par_Address / PageSize;
        size_t PageNoEnd = ((size_t)par_Address + par_Size) / PageSize;
        PageAlignedStartAddress = (void*)(PageNoStart * PageSize);
        PageAlignedSize = (PageNoEnd - PageNoStart + 1) * PageSize;
        int Ret = mprotect(PageAlignedStartAddress, PageAlignedSize, PROT_READ | PROT_WRITE);
        return Ret;
    } else {
        return -1;
    }
#endif
}

int XilEnvInternal_TryAndCatchWriteToMemCopy (void *Dst, void *Src, int Size)
{
    int ShouldChangeProtection;
#if _WIN32
    ShouldChangeProtection = (XilEnvInternal_CheckIfAddessIsInsideAWritableSection (Dst) == 0) |
                             (XilEnvInternal_CheckIfAddessIsInsideAWritableSection ((void*)((char*)Dst + Size)) == 0);
#else
    ShouldChangeProtection = 1;
#endif
    if (ShouldChangeProtection) {   // If it is not inside a writable section make it writable
        if (XilEnvInternal_EnableWriteAccressToMemory(Dst, Size) == 0) {
            MEMCPY (Dst, Src, Size);
            return 0;
        }
    } else {
        MEMCPY (Dst, Src, Size);
        return 0;
    }
    return 1;
}

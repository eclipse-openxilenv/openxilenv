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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "StringMaxChar.h"
#include "RemoteMasterModelInterface.h"
#include "RemoteMasterReadWriteMemory.h"

int CheckAccessAndReadMemory(void *par_Address, int par_Size, void *ret_Data)
{
    if (1) {    // How to heck if access possible?
        MEMCPY(ret_Data, par_Address, par_Size);
        return par_Size;
    } else {
        return -1;
    }
}

int CheckAccessAndWriteMemory(void *par_Address, int par_Size, void *par_Data)
{
    if (1) {    // How to heck if access possible?
        // Look if it matches a referenced variable
        check_bbvari_ref(par_Address, par_Data, par_Size);
        MEMCPY(par_Address, par_Data, par_Size);
        return par_Size;
    }
    else {
        return -1;
    }
}

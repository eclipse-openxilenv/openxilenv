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
#include <malloc.h>

#include "RunTimeMeasurement.h"
#include "Blackboard.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ThrowError.h"
#include "MainValues.h"
#include "Scheduler.h"
#include "DebugInfoDB.h"
#include "DebugInfos.h"
#include "GetNextStructEntry.h"
#include "UniqueNumber.h"


static int GetStructElemByAddr (int32_t parent_typenr,
                                int32_t *pparent_typenr_idx,
                                int it_is_a_bclass,
                                uint64_t parent_address,
                                uint64_t address,              //  Address search for
                                int *pbbtype,
                                char *buffer,
                                int size_of_buffer,
                                PROCESS_APPL_DATA *pappldata);

static int GetArrayElemByAddr (int32_t parent_typenr,
                               int32_t *pparent_typenr_idx,
                               uint64_t parent_address,
                               uint64_t address,              // Address search for
                               int *pbbtype,
                               char *buffer,
                               int size_of_buffer,
                               PROCESS_APPL_DATA *pappldata);


int GetStructEntryByAddress (uint64_t address,             // in  symbol address
                             char *buffer,                 // out return of the expanded label name
                             int size_of_buffer,
                             int *pbbtype,                 // out data type
                             PROCESS_APPL_DATA *pappldata) // in  ...
{
    int what;
    int err = -1;
    int ret;
    char *lname;                    // Name of the base symbols
    int32_t base_typenr;            // Type of the base dymbols
    int32_t base_typenr_idx = -1;   // Index inside the type table of the base symbols or -1 if till now unknown
    int32_t *pbase_typenr_idx;      // Pointer to the index inside the type table of the base symbols
    uint64_t base_addr;

    if ((lname = get_label_by_address_ex (address,   // in
                                          &base_addr,  // out
                                          &base_typenr,   // out
                                          &base_typenr_idx,  // out
                                          &pbase_typenr_idx, // out 
                                          pappldata)) != NULL) {
        StringCopyMaxCharTruncate(buffer, lname, size_of_buffer);   // copy label name to the return buffer
        what = get_what_is_typenr (&base_typenr,       // in out
                                   &base_typenr_idx,   // in out
                                   pappldata);
        *pbase_typenr_idx = base_typenr_idx;  // Update index, at the beginning it is aways -1


        switch (what) {
        case 1:   // It is a base data type
            *pbbtype = get_base_type_bb_type_ex (base_typenr, pappldata);
            if ((*pbbtype < 0) ||              // it is not a base data type -> reject entry
                (address != (base_addr))) {    // or the address don't match
                err = -1;
            } else {
                err = 0;            //  This is the end
            }
            break;
        case 2:   // Structure
            err = GetStructElemByAddr (base_typenr,
                                       &base_typenr_idx,
                                       0,
                                       base_addr,
                                       address,
                                       pbbtype,
                                       buffer,
                                       size_of_buffer,
                                       pappldata);
            break;
        case 3:  // Array
            err = GetArrayElemByAddr (base_typenr,
                                      &base_typenr_idx,
                                      base_addr,
                                      address,
                                      pbbtype,
                                      buffer,
                                      size_of_buffer,
                                      pappldata);
            break;
        case 4:      // Pointer and
        default:     // unknown data types would not be expanded
            err = -1;
        }
    }
    ret = err;
    return ret;
}


static int GetStructElemByAddr (int32_t parent_typenr,
                                int32_t *pparent_typenr_idx,
                                int it_is_a_bclass,
                                uint64_t parent_address,
                                uint64_t address,              // Address search for
                                int *pbbtype,
                                char *buffer,
                                int size_of_buffer,
                                PROCESS_APPL_DATA *pappldata)
{
    char *name;
    int typenr;
    int what;
    int offset;
    int err = -1;
    int have_a_bclass;
    int ret;
    int typenr_idx = -1;
    int field_idx = -1;


    // BEGIN_RUNTIME_MEASSUREMENT ("GetNextStructEntryStruct") geht nicht wegen Rekursion

    if ((name = get_struct_elem_by_offset (parent_typenr,
                                           pparent_typenr_idx,    // in out
                                           (int32_t)(address - parent_address),    // desired offset
                                           &offset,                     // actual offset
                                           &have_a_bclass, 
                                           &typenr,
                                           &field_idx,
                                           pappldata)) != NULL) {
        if (it_is_a_bclass) StringAppendMaxCharTruncate(buffer, "::", size_of_buffer);
        else StringAppendMaxCharTruncate (buffer, ".", size_of_buffer);
        StringAppendMaxCharTruncate (buffer, name, size_of_buffer);
        if ((what = get_what_is_typenr (&typenr, &typenr_idx, pappldata)) < 0) {
            err = -1;
            ret = err;
            goto __OUT;
        }
        switch (what) {
        case 1:   // Base data type
            *pbbtype = get_base_type_bb_type_ex (typenr, pappldata);
            if ((*pbbtype < 0) ||                            // it is not a base data type -> reject entry
                (address != (parent_address + (uint64_t)offset))) {    // or the address don't match
                err = -1;
            } else { 
                err = 0;            //  This is the end
            }
            break;
        case 2:   // Structure
            err = GetStructElemByAddr (typenr,
                                       &typenr_idx,
                                       have_a_bclass,
                                       parent_address + (uint64_t)offset,
                                       address,
                                       pbbtype,
                                       buffer,
                                       size_of_buffer,
                                       pappldata);
            break;
        case 3:  // Array
            err = GetArrayElemByAddr (typenr,
                                       &typenr_idx,
                                      parent_address + (uint64_t)offset,
                                      address,
                                      pbbtype,
                                      buffer,
                                      size_of_buffer,
                                      pappldata);
            break;
        case 4:      // Pointer and
        default:     // unknown data types would not be expanded
            err = -1;
        }
    } else {
        err = -1; // no more structure elements
    }
    ret = err;
__OUT:
    // END_RUNTIME_MEASSUREMENT
    return ret;
}

#define UNUSED(x) (void)(x)

static int GetArrayElemByAddr (int32_t parent_typenr,
                               int32_t *pparent_typenr_idx,
                               uint64_t parent_address,
                               uint64_t address,              // Address search for
                               int *pbbtype,
                               char *buffer,
                               int size_of_buffer,
                               PROCESS_APPL_DATA *pappldata)

{
    UNUSED(pparent_typenr_idx);
    int32_t arrayelem_typenr;
    int32_t arrayelem_typenr_idx = -1;
    int arrayelem_of_what;
    int32_t array_size, array_elements, sizeof_array_element;
    int32_t index;
    int err;
    int ret = -1;

    // BEGIN_RUNTIME_MEASSUREMENT ("GetNextStructEntryArray")
    if (!get_array_elem_type_and_size (&arrayelem_typenr, &arrayelem_of_what,
                                       &array_size, &array_elements, parent_typenr, pappldata)) {
        ThrowError (1, "internal error %s (%i)", __FILE__, __LINE__);
        ret = -1;
        goto __OUT;
    }
    sizeof_array_element = array_size / array_elements;
    if (sizeof_array_element <= 0) {
        ThrowError (1, "internal error %s(%i)", __FILE__, __LINE__);
        ret = -1;
        goto __OUT;
    }
    index = (int32_t)(address - parent_address) / sizeof_array_element;
    PrintFormatToString (buffer + strlen(buffer), size_of_buffer -  strlen(buffer), "[%i]", index);
    switch (arrayelem_of_what) {
    case 1:   // Base data type
        *pbbtype = get_base_type_bb_type_ex (arrayelem_typenr, pappldata);
        if ((*pbbtype < 0) ||                            // it is not a base data type -> reject entry
            (address != (parent_address + (uint64_t)(index * sizeof_array_element)))) {    // or the address don't match
            err = -1;
        } else { 
            err = 0;            //  This is the end
        }
        break;
    case 2:   // Structure
        err = GetStructElemByAddr (arrayelem_typenr,
                                    &arrayelem_typenr_idx,
                                    0,
                                    parent_address + (uint64_t)(index * sizeof_array_element),
                                    address,
                                    pbbtype,
                                    buffer,
                                    size_of_buffer,
                                    pappldata);
        break;
    case 3:  // Array
        err = GetArrayElemByAddr (arrayelem_typenr,
                                  &arrayelem_typenr_idx,
                                  parent_address + (uint64_t)(index * sizeof_array_element),
                                  address,
                                  pbbtype,
                                  buffer,
                                  size_of_buffer,
                                  pappldata);
        break;
    case 4:      // Pointer und
    default:     // unknown data types would not be expanded
        err = -1;
    }
    ret = err;
__OUT:
    // END_RUNTIME_MEASSUREMENT
    return ret;
}


int DbgInfoTranslatePointerToLabel (uint64_t ptr, char *label, int maxc, int *pbbtype, int pid)
{
    PROCESS_APPL_DATA *pappldata = NULL;
    int bbtype;
    char buffer[4096];
    int ret = -1;
    int UniqueId = -1;
    int Pid;

    BEGIN_RUNTIME_MEASSUREMENT ("DbgInfoTranslatePointerToLabel")
    if (get_name_by_pid (pid, buffer, sizeof(buffer))) {
        goto __OUT;
    }

    UniqueId = AquireUniqueNumber();
    pappldata = ConnectToProcessDebugInfos (UniqueId,
                                            buffer,
                                            &Pid,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    if (pappldata == NULL) {
        goto __OUT;
    }
    if (GetStructEntryByAddress (ptr,
                                 buffer,
                                sizeof(buffer),
                                 &bbtype,
                                 pappldata) == -1) {
        ret = ADDRESS_NO_VALID_LABEL;
        goto __OUT;
    }
    *pbbtype = bbtype; 
    ConvertLabelAsapCombatible (buffer, sizeof(buffer), 0);
    StringCopyMaxCharTruncate(label, buffer, (size_t)(maxc-1));
    label[maxc-1] = 0;
    ret = (strlen (buffer) >= BBVARI_NAME_SIZE) ? LABEL_NAME_TOO_LONG : 0;
  __OUT:
    if (pappldata != NULL) {
        RemoveConnectFromProcessDebugInfos (UniqueId);
    }
    if (UniqueId > 0) {
        FreeUniqueNumber (UniqueId);
    }

    END_RUNTIME_MEASSUREMENT
    return ret;
}

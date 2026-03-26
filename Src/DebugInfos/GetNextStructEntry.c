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
#include <string.h>

#include "RunTimeMeasurement.h"
#include "Blackboard.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "MainValues.h"
#include "Scheduler.h"
#include "DebugInfoDB.h"
#include "DebugInfos.h"
#include "UniqueNumber.h"
#include "GetNextStructEntry.h"


static int GetNextStructEntryStruct (int pos,
                                     int it_is_a_bclass,
                                     char *buffer, int size_of_buffer,
                                     uint64_t *paddress,
                                     int *pbbtype,
                                     GET_NEXT_STRUCT_ENTRY_BUFFER *internal_data);

static int GetNextStructEntryArray (int pos,
                                   char *buffer, int size_of_buffer,
                                   uint64_t *paddress,
                                   int *pbbtype,
                                   GET_NEXT_STRUCT_ENTRY_BUFFER *internal_data);



int GetNextStructEntry (PROCESS_APPL_DATA *pappldata, // in  ...
                        char *lname,                  // in  Symbol/label name
                        char *buffer,                 // out Return of the expanded label name
                        int maxc,
                        uint64_t *paddress,           // io  Symbol address
                        int *pbbtype,                 // out data type
                        int FirstElemFlag,            // in
                        int ConvPointerToIntFlag,     // in
                        GET_NEXT_STRUCT_ENTRY_BUFFER *internal_data)    // in/out
{
    char *next_l;
    int32_t next;
    int i, err;
    long pos = 0;
    int ret;

    BEGIN_RUNTIME_MEASSUREMENT ("GetNextStructEntry")
    internal_data->ConvPointerToIntFlag = ConvPointerToIntFlag;
    internal_data->pappldata = pappldata;
    if (FirstElemFlag) {  // new label
        for (i = 0; i < MAX_STRUCT_DEPTH; i++) internal_data->idxs[i] = internal_data->typenrs[i] = 0;
        // search label
        next = 0;
        while ((next_l = get_next_label (&next, pappldata)) != NULL) {
            if (!strcmp (next_l, lname)) {   // label found
                if (!get_type_by_name (&internal_data->typenr, lname, &internal_data->what, pappldata)) {
                    ret = NO_MORE_ENTRYS;
                    goto __OUT;
                }
                internal_data->typenrs[0] = internal_data->typenr;
                break;
            }
        }
        if (internal_data->typenr == 0) {
            ret = ERROR_IN_ENTRY;  // label not found
            goto __OUT;
        }
    }
    StringCopyMaxCharTruncate (buffer, lname, maxc);                  // Copy label name to output buffer

    *paddress =  get_label_adress (lname, pappldata);
    switch (internal_data->what) {
    case 1:   // Base data type
        *pbbtype = get_base_type_bb_type_ex (internal_data->typenr, pappldata);
        if (*pbbtype < 0) {           // It is not a base type reject entry
            goto DEFAULT;
        }
        err = BASIC_TYPE;
        break;
    case 2:   // Structure
        pos++;
        err = GetNextStructEntryStruct (pos,
                                        0,
                                        buffer,
                                        maxc,
                                        paddress,
                                        pbbtype,
                                        internal_data);
        pos--;
        if (err == NO_MORE_ENTRYS) {  // Structure read completed
            internal_data->typenrs[pos] = 0;
            internal_data->idxs[pos] = 0;
        }
        break;
    case 3:  // Array
        pos++;
        err = GetNextStructEntryArray (pos,
                                       buffer,
                                       maxc,
                                       paddress,
                                       pbbtype,
                                       internal_data);
        pos--;
        if (err == NO_MORE_ENTRYS) {  // Array read completed
            internal_data->typenrs[pos] = 0;
            internal_data->idxs[pos] = 0;
        }
        break;
    case 4:      // Pointer
        if (internal_data->ConvPointerToIntFlag) {
            *pbbtype = BB_UDWORD;  // If it is a 64 bit executable this should be BB_UQWORD
            err = BASIC_TYPE;
        } else {
            err = NO_MORE_ENTRYS;
        }
        break;
    default:     // Unknown data types would not be expanded
    DEFAULT:
        err = NO_MORE_ENTRYS;
    }
    ret = err;
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}


int GetNextStructEntryEx (PROCESS_APPL_DATA *pappldata, // in  ...
                          char *lname,                 // in  Symbol/label name
                          int32_t parent_typenr,
                          uint64_t parrent_address,
                          char *buffer,                // out Return of the expanded label name
                          int maxc,
                          uint64_t *paddress,     // io  Symbol address
                          int *pbbtype,                // out data type
                          int flag)                    // in
{
    GET_NEXT_STRUCT_ENTRY_BUFFER *static_data;
    int i, err;
    int pos = 0;
    int ret;

    BEGIN_RUNTIME_MEASSUREMENT ("GetNextStructEntryEx")
    *paddress = parrent_address;
    static_data = (GET_NEXT_STRUCT_ENTRY_BUFFER*)get_return_buffer_func_thread ((void*)GetNextStructEntryEx, sizeof (GET_NEXT_STRUCT_ENTRY_BUFFER));

    static_data->ConvPointerToIntFlag = 0;
    static_data->pappldata = pappldata;
    if (flag) {  // new label
        int32_t Idx = -1;
        for (i = 0; i < MAX_STRUCT_DEPTH; i++) static_data->idxs[i] = static_data->typenrs[i] = 0;
        static_data->what = get_what_is_typenr (&parent_typenr, &Idx, pappldata);
        static_data->typenrs[0] = parent_typenr;
    }
    StringCopyMaxCharTruncate (buffer, lname, maxc);                  // Copy label name to output buffer

    switch (static_data->what) {
    case 1:   // Base data type
        *pbbtype = get_base_type_bb_type_ex (parent_typenr, pappldata);
        if (*pbbtype < 0) {           // It is not a base type reject entry
            goto DEFAULT;
        }
        err = BASIC_TYPE;
        break;
    case 2:   // Structure
        pos++;
        err = GetNextStructEntryStruct (pos,
                                        0,
                                        buffer,
                                        maxc,
                                        paddress,
                                        pbbtype,
                                        static_data);
        pos--;
        if (err == NO_MORE_ENTRYS) {  // Structure read completed
            static_data->typenrs[pos] = 0;
            static_data->idxs[pos] = 0;
        }
        break;
    case 3:  // Array
        pos++;
        err = GetNextStructEntryArray (pos,
                                       buffer,
                                       maxc,
                                       paddress,
                                       pbbtype,
                                       static_data);
        pos--;
        if (err == NO_MORE_ENTRYS) {  // Array read completed
            static_data->typenrs[pos] = 0;
            static_data->idxs[pos] = 0;
        }
        break;
    case 4:      // Pointer
        if (static_data->ConvPointerToIntFlag) {
            *pbbtype = BB_UDWORD;  // If it is a 64 bit executable this should be BB_UQWORD
            err = BASIC_TYPE;
        } else {
            err = NO_MORE_ENTRYS;
        }
        break;
    default:     // Unknown data types would not be expanded
    DEFAULT:
        err = NO_MORE_ENTRYS;
    }
    ret = err;
    END_RUNTIME_MEASSUREMENT
    return ret;
}

static int GetNextStructEntryStruct (int pos,
                                     int it_is_a_bclass,
                                     char *buffer,
                                     int size_of_buffer,
                                     uint64_t *paddress,
                                     int *pbbtype,
                                     GET_NEXT_STRUCT_ENTRY_BUFFER *internal_data)
{
    char *name;
    int32_t typenr;
    int32_t what;
    int flag;
    uint32_t address_offset;
    uint64_t address_save;
    int32_t entry;
    int err;
    char *save;
    int have_a_bclass;
    int ret;

    // BEGIN_RUNTIME_MEASSUREMENT ("GetNextStructEntryStruct")
    address_save = *paddress;
    save = buffer + strlen (buffer);
    entry = internal_data->idxs[pos];         // current entry
  NEXT_STRUCT_ENTRY:
    *paddress = address_save;
    if (entry == 0) flag = 1;  // first entry
    else flag = 0;
    if ((name = get_next_struct_entry (&typenr, &address_offset, &have_a_bclass, internal_data->typenrs[pos-1], flag, &entry, internal_data->pappldata)) != NULL) {
        if (it_is_a_bclass) StringAppendMaxCharTruncate(buffer, "::", size_of_buffer);
        else StringAppendMaxCharTruncate (buffer, ".", size_of_buffer);
        StringAppendMaxCharTruncate (buffer, name, size_of_buffer);
        *paddress = *paddress + address_offset;
        if (get_struct_entry_typestr (&typenr, entry, &what, internal_data->pappldata) == NULL) {
            // ThrowError (1, "sanity check %s %li name = %s, pos = %i, buffer = %s", __FILE__, (long)__LINE__, name, pos, buffer);
            internal_data->typenrs[pos] = typenr;
            internal_data->typenrs[pos] = 0;
            internal_data->idxs[pos] = entry;
            err = ERROR_IN_ENTRY;
            ret = err;
            goto __OUT;
        }
        internal_data->typenrs[pos] = typenr;
        switch (what) {
        case 1:   // Base data type
            *pbbtype = get_base_type_bb_type_ex (typenr, internal_data->pappldata);
            if (*pbbtype < 0) {           // It is not a base type reject entry
                goto DEFAULT;
            }
            err = VALID_ENTRY;            // there are more structure entrys
            internal_data->typenrs[pos] = 0;
            internal_data->idxs[pos] = entry;
            break;
        case 2:   // Structure
            pos++;
            err = GetNextStructEntryStruct (pos,
                                            have_a_bclass,
                                            buffer,
                                            size_of_buffer,
                                            paddress,
                                            pbbtype,
                                            internal_data);
            pos--;
            if (err == NO_MORE_ENTRYS) {  // Sub structure read completed
                internal_data->typenrs[pos] = 0;
                internal_data->idxs[pos] = entry;
                *save = '\0';
                goto NEXT_STRUCT_ENTRY;
            }
            break;
        case 3:  // Array
            pos++;
            err = GetNextStructEntryArray (pos,
                                           buffer,
                                           size_of_buffer,
                                           paddress,
                                           pbbtype,
                                           internal_data);
            pos--;
            if (err == NO_MORE_ENTRYS) {  // Array read completed
                internal_data->typenrs[pos] = 0;
                internal_data->idxs[pos] = entry;
                *save = '\0';
                goto NEXT_STRUCT_ENTRY;
            }
            break;
        case 4:      // Pointer
            if (internal_data->ConvPointerToIntFlag) {
                *pbbtype = BB_UDWORD;  // If it is a 64 bit executable this should be BB_UQWORD
                err = VALID_ENTRY;            // there are more structure entrys
                internal_data->typenrs[pos] = 0;
                internal_data->idxs[pos] = entry;
            } else {
                internal_data->typenrs[pos] = 0;
                internal_data->idxs[pos] = entry;
                *save = '\0';
                goto NEXT_STRUCT_ENTRY;
            }
            break;
        default:     // Unknown data types would not be expanded
        DEFAULT:
            internal_data->typenrs[pos] = 0;
            internal_data->idxs[pos] = entry;
            *save = '\0';
            goto NEXT_STRUCT_ENTRY;
        }
    } else {
        err = NO_MORE_ENTRYS; // kein weiterer Structureelement
        internal_data->idxs[pos] = 0;
        internal_data->typenrs[pos] = 0;
    }
    ret = err;
__OUT:
    // END_RUNTIME_MEASSUREMENT
    return ret;
}



static int GetNextStructEntryArray (int pos,
                                    char *buffer,
                                    int size_of_buffer,
                                    uint64_t *paddress,
                                    int *pbbtype,
                                    GET_NEXT_STRUCT_ENTRY_BUFFER *internal_data)
{
    int32_t arrayelem_typenr;
    int32_t arrayelem_of_what;
    int32_t array_size, array_elements;
    long index;
    int err;
    char *save;
    uint64_t address_save;
    int ret;

    // BEGIN_RUNTIME_MEASSUREMENT ("GetNextStructEntryArray")
    address_save = *paddress;
    save = buffer + strlen (buffer);
    index = internal_data->idxs[pos];       // current entry
    if (!get_array_elem_type_and_size (&arrayelem_typenr, &arrayelem_of_what,
                                       &array_size, &array_elements, internal_data->typenrs[pos-1], internal_data->pappldata)) {
        // This can happen if the array is of unknown data type
        ret = NO_MORE_ENTRYS;
        goto __OUT;
    }
    if (index >= array_elements) {
        internal_data->idxs[pos] = 0;
        internal_data->typenrs[pos] = 0;
        ret = NO_MORE_ENTRYS;             // No more array entrys
        goto __OUT;
    }
    internal_data->typenrs[pos] = arrayelem_typenr;

  NEXT_ARRAY_ELEMENT:
    *paddress = address_save;
    *paddress = *paddress + (uint64_t)(index * (array_size / array_elements));
    PrintFormatToString (buffer + strlen(buffer), size_of_buffer - strlen(buffer), "[%li]", index);
    switch (arrayelem_of_what) {
    case 1:   // Base data type
        *pbbtype = get_base_type_bb_type_ex (arrayelem_typenr, internal_data->pappldata);
        if (*pbbtype < 0) {           // It is not a base type reject entry
             goto DEFAULT;
        }
        err = VALID_ENTRY;            // There are more array elements
        (internal_data->idxs[pos])++;  // next array element index
        internal_data->typenrs[pos] = 0;
        break;
    case 2:   // Structure
        pos++;
        err = GetNextStructEntryStruct (pos,
                                        0,
                                        buffer,
                                        size_of_buffer,
                                        paddress,
                                        pbbtype,
                                        internal_data);
        pos--;
        if (err == NO_MORE_ENTRYS) {  // Structure read completed
            (internal_data->idxs[pos])++;  // next array element index
            if (internal_data->idxs[pos] < array_elements) {
                *save = '\0';
                index = internal_data->idxs[pos];
                goto NEXT_ARRAY_ELEMENT;
            } else {
                internal_data->idxs[pos] = 0;
                internal_data->typenrs[pos] = 0;
            }
        }
        break;
    case 3:  // Array
        pos++;
        err = GetNextStructEntryArray (pos,
                                       buffer,
                                       size_of_buffer,
                                       paddress,
                                       pbbtype,
                                       internal_data);
        pos--;
        if (err == NO_MORE_ENTRYS) {      // Array read completed
            (internal_data->idxs[pos])++;   // next array element index
            if (internal_data->idxs[pos] < array_elements) {
                *save = '\0';
                index = internal_data->idxs[pos];
                goto NEXT_ARRAY_ELEMENT;
            } else {
                internal_data->idxs[pos] = 0;
                internal_data->typenrs[pos] = 0;
            }
        }
        break;
    case 4:      // Pointer
        if (internal_data->ConvPointerToIntFlag) {
            *pbbtype = BB_UDWORD;  // If it is a 64 bit executable this should be BB_UQWORD
            err = VALID_ENTRY;            // There are more array elements
            (internal_data->idxs[pos])++;  // next array element index
            internal_data->typenrs[pos] = 0;
        } else {
            (internal_data->idxs[pos])++;  // next array element index
            if (internal_data->idxs[pos] < array_elements) {
                *save = '\0';
                index = internal_data->idxs[pos];
                goto NEXT_ARRAY_ELEMENT;
            } else {
                internal_data->idxs[pos] = 0;
                internal_data->typenrs[pos] = 0;
            }
            err = NO_MORE_ENTRYS;
        }
        break;
    default:     // Unknown data types would not be expanded
    DEFAULT:
        (internal_data->idxs[pos])++;  // next array element index
        if (internal_data->idxs[pos] < array_elements) {
            *save = '\0';
            index = internal_data->idxs[pos];
            goto NEXT_ARRAY_ELEMENT;
        } else {
            internal_data->idxs[pos] = 0;
            internal_data->typenrs[pos] = 0;
        }
        err = NO_MORE_ENTRYS;
        break;
    }
    ret = err;
__OUT:
    // END_RUNTIME_MEASSUREMENT
    return ret;
}


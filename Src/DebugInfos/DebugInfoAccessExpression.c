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
#include <ctype.h>
#include "Platform.h"

#include "ThrowError.h"
#include "Blackboard.h"
#include "Scheduler.h"
#include "DebugInfoDB.h"
#include "DebugInfos.h"
#include "DebugInfoAccessExpression.h"


static int struct_elem (const char *txt, uint64_t *paddress, int32_t *ptypenr,
                        int32_t typenr, PROCESS_APPL_DATA *pappldata, int pid, int already_locked, int only_base_types);


/***************************************************************************
 *                                                                         *
 *                   -----+-> '.' ---> struct_elem()                       *
 *                        |                                                *
 *                        +-> [idx]-\                                      *
 *                        |         |                                      *
 *                        +-> EOL   |                                      *
 *                        |         |                                      *
 *                        \---------/                                      *
 ***************************************************************************/

static int array_or_pointer_elem (int array_or_ptr, const char *txt, uint64_t *paddress,
                                  int32_t *ptypenr, int32_t index, int32_t typenr, PROCESS_APPL_DATA *pappldata, int pid,
                                  int already_locked, int only_base_types)
{
    char *np;
    const char *p = txt;
    int32_t idx;
    int32_t array_size, array_elements, arrayelem_typenr;
    int arrayelem_of_what;

    if (p == NULL) return 0;
    if (array_or_ptr) {
        // Pointer / reference
        if (already_locked || (WaitUntilProcessIsNotActiveAndThanLockIt (pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot read pointer", __FILE__, __LINE__) == 0)) {
            if (scm_read_bytes (*paddress, pid, (char*)paddress, sizeof (int32_t)) != sizeof (int32_t)) {
                //ThrowError (1, "cannot read pointer value (extern process not running or halted in debugger)");
                if (!already_locked) UnLockProcess (pid);
                return -1;
            }
            if (!already_locked) UnLockProcess (pid);
        }
        if (!get_points_to (&arrayelem_typenr, &arrayelem_of_what, typenr, pappldata)) {
            return -1;
        }
        if (arrayelem_of_what == ARRAY_ELEM) {  // Pointer / reference to an array
            return array_or_pointer_elem (0, txt, paddress, ptypenr, index, arrayelem_typenr, pappldata, pid, already_locked, only_base_types);
        }
        if (get_base_type_bb_type_ex (arrayelem_typenr, pappldata) > -1) {  // Array of base data types
            *paddress += (uint64_t)(get_base_type_size (arrayelem_typenr) * index);
            *ptypenr = arrayelem_typenr;
            return 0;
        }
    } else {
        // Array
        if (!get_array_elem_type_and_size (&arrayelem_typenr, &arrayelem_of_what,
                                           &array_size, &array_elements, typenr, pappldata)) return -1;

        if (index >= array_elements) return -1;
        *paddress = *paddress + (uint64_t)(index * (array_size / array_elements));
    }
    while (1) {
        switch (*p) {
        case '[':           // multidimensional array
            p++;     // Skip [ bracket
            idx = strtol (p, &np, 10);
            if ((np == p) || (*np != ']') || (idx < 0)) {
                return -1;
            } else {
                p = np+1;  // Skip ] bracket
                return array_or_pointer_elem ((arrayelem_of_what != ARRAY_ELEM), p, paddress, ptypenr, idx, arrayelem_typenr, pappldata, pid, already_locked, only_base_types);
            }
        case '.':
            p++;
            return struct_elem (p, paddress, ptypenr, arrayelem_typenr, pappldata, pid, already_locked, only_base_types);
        case '\0':  // EOL
            *ptypenr = arrayelem_typenr;
            return 0;
        default:
            return -1;
        }
    }
}

/***************************************************************************
 *                                                                         *
 *                                 /----------------------\                *
 *                                 |                      |                *
 *                             ----+-> ElemName -+-> '.' -/                *
 *                                               |                         *
 *                                               +-> [idx] -->array_elem() *
 *                                               |                         *
 *                                               \-> EOL                   *
 *                                                                         *
 ***************************************************************************/

static int struct_elem (const char *txt, uint64_t *paddress, int32_t *ptypenr,
                        int32_t typenr, PROCESS_APPL_DATA *pappldata, int pid, int already_locked, int only_base_types)
{
    char entry[BBVARI_NAME_SIZE];
    char *np;
    const char *p = txt;
    char *l = entry;
    int32_t idx;
    int flag;
    int32_t typenr_entry;
    uint32_t address_offset;
    int32_t pos;
    char *next_e;
    int what;
    int bclass;

    if (p == NULL) return -1;
    while (1) {
        switch (*p) {
        case '[':
            p++;    // Skip [ bracket
            idx = strtol (p, &np, 10);
            if ((np == p) || (*np != ']') || (idx < 0)) {
                return -1;
            } else {
                *l = '\0';
                p = np+1;  // Skip ] bracket
                flag = 1;
                while (1) {
                    if ((next_e = get_next_struct_entry (&typenr_entry, &address_offset, &bclass, typenr, flag, &pos, pappldata)) == NULL) return -1;
                    flag = 0;
                    if (!strcmp (next_e, entry)) break;  // found entry
                }
                *paddress = *paddress + address_offset;
                get_struct_entry_typestr (NULL, pos, &what, pappldata);
                return array_or_pointer_elem ((what != ARRAY_ELEM), p, paddress, ptypenr, idx, typenr_entry, pappldata, pid, already_locked, only_base_types);
            }
        case ':':   // Base class will be handled like a sub class
            *l = 0;
            flag = 1;
            while (1) {
                bclass = 0;
                if ((next_e = get_next_struct_entry (&typenr_entry, &address_offset, &bclass, typenr, flag, &pos, pappldata)) == NULL) {
                    break;
                }
                flag = 0;
                if (bclass) {
                    if (!strcmp (next_e, entry)) { // found entry
                        if (*(p+1) == ':') p++;
                        goto IT_IS_A_BASECLASS;
                    }
                }
            }
            goto IT_IS_A_NAMESPACE;
        case '.':
            *l = '\0';
          IT_IS_A_BASECLASS:
            p++;
            flag = 1;
            while (1) {
                if ((next_e = get_next_struct_entry (&typenr_entry, &address_offset, &bclass, typenr, flag, &pos, pappldata)) == NULL) return -1;
                flag = 0;
                if (!strcmp (next_e, entry)) break;  // found entry
            }
            *paddress = *paddress + address_offset;
            return struct_elem (p, paddress, ptypenr, typenr_entry, pappldata, pid, already_locked, only_base_types);
        case '\0':  // EOL
            *l = '\0';
            flag = 1;
            while (1) {
                if ((next_e = get_next_struct_entry (&typenr_entry, &address_offset, &bclass, typenr, flag, &pos, pappldata)) == NULL) return -1;
                flag = 0;
                if (!strcmp (next_e, entry)) break;  // found entry
            }
            *paddress = *paddress + address_offset;
            if (typenr_entry >= 0x1000) {   // is not a base data type
                *ptypenr = typenr_entry;
                // look for modifier (to be on the safe side)
                get_modify_what (ptypenr, NULL, typenr_entry, pappldata);
            } else {
                *ptypenr = typenr_entry;
            }
            return 0;
        default:
          IT_IS_A_NAMESPACE:
            if (l == entry) {
                if (!isalpha (*p) && *p != '_' && *p != '%') {
                    return -1;
                }
            } else {
                if (*p == '{') {
                    int i = 1;
                    while ((*p != '}') || i) {
                        if ((l-entry) < ((int64_t)sizeof(entry)-1)) *l++ = *p++;
                        if (*p == '}') i--;
                        else if (*p == '{') i++;
                    }
                }
                if (!isalnum (*p) && *p != '_' && *p != '}' && *p != ':' && *p != '%' && *p != '$') {
                    return -1;
                }
            }
            if ((l-entry) < ((int64_t)sizeof(entry)-1)) *l++ = *p++;
        }
    }
}



/***************************************************************************
 *                                                                         *
 *  LabelName -+-> '.' --> srtuct_elem()                                   *
 *             |                                                           *
 *             +-> [idx] --> array_elem()                                  *
 *             |                                                           *
 *             \-> EOL                                                     *
 *                                                                         *
 ***************************************************************************/

int appl_label_already_locked (DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata, int pid, const char *lname, uint64_t *paddress, int32_t *ptypenr,
                               int already_locked, int only_base_types)
{
    char label[BBVARI_NAME_SIZE];
    char *np;
    const char *p = lname;
    char *l = label;
    int32_t idx;
    int32_t typenr;
    int what;

    if (lname == NULL) return -1;

    while (1) {
        switch (*p) {
        case '[':
            p++;     // Skip [ bracket
            *l = '\0';
            idx = 0;
            if (SearchLabelByName (label, pappldata) < 0) {
                return -1;
            }
            get_type_by_name (&typenr, label, &what, pappldata);
            if ((what != ARRAY_ELEM) && (what != POINTER_ELEM) && (what != MODIFIER_ELEM)) {
                return -1;
            }
            idx = strtol (p, &np, 10);
            if ((np == p) || (*np != ']') || (idx < 0)) {
                return -1;
            } else {
                p = np+1;   // Skip ] bracket
                *paddress = get_label_adress (label, pappldata);
                return array_or_pointer_elem ((what != ARRAY_ELEM), p, paddress, ptypenr, idx, typenr, pappldata, pid, already_locked, only_base_types);
            }
        case '.':
            *l = '\0';
            p++;   // Skip '.'
            idx = 0;
            if (SearchLabelByName (label, pappldata) < 0) {
                return -1;
            }
            get_type_by_name (&typenr, label, &what, pappldata);
            *paddress = get_label_adress (label, pappldata);
            return struct_elem (p, paddress, ptypenr, typenr, pappldata, pid, already_locked, only_base_types);
        case '\0':  // EOL  -> single parameter
            *l = '\0';
            idx = 0;
            if (SearchLabelByName (label, pappldata) < 0) {
                return -1;
            }            
            get_type_by_name (&typenr, label, &what, pappldata);
            *paddress = get_label_adress (label, pappldata);
            *ptypenr = typenr;
            if (only_base_types) {
                if (get_base_type_bb_type_ex (typenr, pappldata) == -1) return -1; // this is not a base data type
            }
            return 0;
        default:
            if (l == label) {
                if (!isalpha (*p) && *p != '_') {
                    return -1;
                }
            } else {
                if (*p == '{') {
                    int i = 1;
                    while ((*p != '}') && i) {
                        *l++ = *p++;
                        if (*p == '}') i--;
                        else if (*p == '{') i++;
                    }
                }
                if (!isalnum (*p) && (*p != '_') && (*p != ':') && (*p != '}') && (*p != '@')) {
                    return -1;
                }
            }
            *l++ = *p++;
        }
    }
}

int appl_label (DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata, int pid, const char *lname, uint64_t *paddress, int32_t *ptypenr)
{
    return appl_label_already_locked (pappldata, pid, lname, paddress, ptypenr, 0, 1);
}

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


#define DW_FORM_null   0x00
#define DW_FORM_addr   0x01
#define DW_FORM_block2   0x03
#define DW_FORM_block4   0x04
#define DW_FORM_data2   0x05
#define DW_FORM_data4   0x06
#define DW_FORM_data8   0x07
#define DW_FORM_string   0x08
#define DW_FORM_block   0x09
#define DW_FORM_block1   0x0a
#define DW_FORM_data1   0x0b
#define DW_FORM_flag   0x0c
#define DW_FORM_sdata   0x0d
#define DW_FORM_strp   0x0e
#define DW_FORM_udata   0x0f
#define DW_FORM_ref_addr   0x10
#define DW_FORM_ref1   0x11
#define DW_FORM_ref2   0x12
#define DW_FORM_ref4   0x13
#define DW_FORM_ref8   0x14
#define DW_FORM_ref_udata   0x15
#define DW_FORM_indirect   0x16
#define DW_FORM_sec_offset   0x17
#define DW_FORM_exprloc   0x18
#define DW_FORM_flag_present   0x19
// new with Dwarf5
#define DW_FORM_strx  0x1a
#define DW_FORM_addrx 0x1b
#define DW_FORM_ref_sup4 0x1c
#define DW_FORM_strp_sup 0x1d
#define DW_FORM_data16 0x1e
#define DW_FORM_line_strp 0x1f
#define DW_FORM_ref_sig8   0x20
#define DW_FORM_implicit_const 0x21
#define DW_FORM_loclistx 0x22
#define DW_FORM_rnglistx 0x23
#define DW_FORM_ref_sup8 0x24
#define DW_FORM_strx1 0x25
#define DW_FORM_strx2 0x26
#define DW_FORM_strx3 0x27
#define DW_FORM_strx4 0x28
#define DW_FORM_addrx1 0x29
#define DW_FORM_addrx2 0x2a
#define DW_FORM_addrx3 0x2b
#define DW_FORM_addrx4  0x2c

DW_NAME_TABLE DW_FORM_NameTable[] = { 
    {"DW_FORM_null",   0x00},
    {"DW_FORM_addr",   0x01},
    {"unknown",   0x02},
    {"DW_FORM_block2",   0x03},
    {"DW_FORM_block4",   0x04},
    {"DW_FORM_data2",   0x05},
    {"DW_FORM_data4",   0x06},
    {"DW_FORM_data8",   0x07},
    {"DW_FORM_string",   0x08},
    {"DW_FORM_block",   0x09},
    {"DW_FORM_block1",   0x0a},
    {"DW_FORM_data1",   0x0b},
    {"DW_FORM_flag",   0x0c},
    {"DW_FORM_sdata",   0x0d},
    {"DW_FORM_strp",   0x0e},
    {"DW_FORM_udata",   0x0f},
    {"DW_FORM_ref_addr",   0x10},
    {"DW_FORM_ref1",   0x11},
    {"DW_FORM_ref2",   0x12},
    {"DW_FORM_ref4",   0x13},
    {"DW_FORM_ref8",   0x14},
    {"DW_FORM_ref_udata",   0x15},
    {"DW_FORM_indirect",   0x16},
    {"DW_FORM_sec_offset",   0x17},
    {"DW_FORM_exprloc",   0x18},
    {"DW_FORM_flag_present",   0x19},
    {"DW_FORM_strx",  0x1a},
    {"DW_FORM_addrx",  0x1b},
    {"DW_FORM_ref_sup4",  0x1c},
    {"DW_FORM_strp_sup",  0x1d},
    {"DW_FORM_data16",   0x1e},
    {"DW_FORM_line_strp",   0x1f},
    // new with Dwarf5
    {"DW_FORM_ref_sig8",   0x20},
    {"DW_FORM_implicit_const",   0x21},
    {"DW_FORM_loclistx",   0x22},
    {"DW_FORM_rnglistx",   0x23},
    {"DW_FORM_ref_sup8",   0x24},
    {"DW_FORM_strx1",   0x25},
    {"DW_FORM_strx2",   0x26},
    {"DW_FORM_strx3",   0x27},
    {"DW_FORM_strx4",   0x28},
    {"DW_FORM_addrx1",   0x29},
    {"DW_FORM_addrx2",   0x2a},
    {"DW_FORM_addrx3",   0x2b},
    {"DW_FORM_addrx4",    0x2c}};


#define DW_FORM_NameTableSize (sizeof (DW_FORM_NameTable) / sizeof (DW_FORM_NameTable[0]))

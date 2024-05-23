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


#define DW_AT_null   0x00
#define DW_AT_sibling   0x01
#define DW_AT_location   0x02
#define DW_AT_name   0x03
#define DW_AT_ordering   0x09
#define DW_AT_byte_size   0x0b
#define DW_AT_bit_offset   0x0c
#define DW_AT_bit_size   0x0d
#define DW_AT_stmt_list   0x10
#define DW_AT_low_pc   0x11
#define DW_AT_high_pc   0x12
#define DW_AT_language   0x13
#define DW_AT_discr   0x15
#define DW_AT_discr_value   0x16
#define DW_AT_visibility   0x17
#define DW_AT_import   0x18
#define DW_AT_string_length   0x19
#define DW_AT_common_reference   0x1a
#define DW_AT_comp_dir   0x1b
#define DW_AT_const_value   0x1c
#define DW_AT_containing_type   0x1d
#define DW_AT_default_value   0x1e
#define DW_AT_inline   0x20
#define DW_AT_is_optional   0x21
#define DW_AT_lower_bound   0x22
#define DW_AT_producer   0x25
#define DW_AT_prototyped   0x27
#define DW_AT_return_addr   0x2a
#define DW_AT_start_scope   0x2c
#define DW_AT_bit_stride   0x2e
#define DW_AT_upper_bound   0x2f
#define DW_AT_abstract_origin   0x31
#define DW_AT_accessibility   0x32
#define DW_AT_address_class   0x33
#define DW_AT_artificial   0x34
#define DW_AT_base_types   0x35
#define DW_AT_calling_convention   0x36
#define DW_AT_count   0x37
#define DW_AT_data_member_location   0x38
#define DW_AT_decl_column   0x39
#define DW_AT_decl_file   0x3a
#define DW_AT_decl_line   0x3b
#define DW_AT_declaration   0x3c
#define DW_AT_discr_list   0x3d
#define DW_AT_encoding   0x3e
#define DW_AT_external   0x3f
#define DW_AT_frame_base   0x40
#define DW_AT_friend   0x41
#define DW_AT_identifier_case   0x42
#define DW_AT_macro_info   0x43
#define DW_AT_namelist_item   0x44
#define DW_AT_priority   0x45
#define DW_AT_segment   0x46
#define DW_AT_specification   0x47
#define DW_AT_static_link   0x48
#define DW_AT_type   0x49
#define DW_AT_use_location   0x4a
#define DW_AT_variable_parameter   0x4b
#define DW_AT_virtuality   0x4c
#define DW_AT_vtable_elem_location   0x4d
#define DW_AT_allocated   0x4e
#define DW_AT_associated   0x4f
#define DW_AT_data_location   0x50
#define DW_AT_byte_stride   0x51
#define DW_AT_entry_pc   0x52
#define DW_AT_use_UTF8   0x53
#define DW_AT_extension   0x54
#define DW_AT_ranges   0x55
#define DW_AT_trampoline   0x56
#define DW_AT_call_column   0x57
#define DW_AT_call_file   0x58
#define DW_AT_call_line   0x59
#define DW_AT_description   0x5a
#define DW_AT_binary_scale   0x5b
#define DW_AT_decimal_scale   0x5c
#define DW_AT_small   0x5d
#define DW_AT_decimal_sign   0x5e
#define DW_AT_digit_count   0x5f
#define DW_AT_picture_string   0x60
#define DW_AT_mutable   0x61
#define DW_AT_threads_scaled   0x62
#define DW_AT_explicit   0x63
#define DW_AT_object_pointer   0x64
#define DW_AT_endianity   0x65
#define DW_AT_elemental   0x66
#define DW_AT_pure   0x67
#define DW_AT_recursive   0x68
#define DW_AT_signature   0x69
#define DW_AT_main_subprogram   0x6a
#define DW_AT_data_bit_offset   0x6b
#define DW_AT_const_expr   0x6c
#define DW_AT_enum_class   0x6d
#define DW_AT_linkage_name   0x6e
// new for Dwarf5
#define DW_AT_string_length_bit_size   0x6f
#define DW_AT_string_length_byte_size    0x70
#define DW_AT_rank    0x71
#define DW_AT_str_offsets_base    0x72
#define DW_AT_addr_base    0x73
#define DW_AT_rnglists_base   0x74
#define DW_AT_dwo_name    0x76
#define DW_AT_reference    0x77
#define DW_AT_rvalue_reference    0x78
#define DW_AT_macros    0x79
#define DW_AT_call_all_calls    0x7a
#define DW_AT_call_all_source_calls   0x7b
#define DW_AT_call_all_tail_calls    0x7c
#define DW_AT_call_return_pc    0x7d
#define DW_AT_call_value    0x7e
#define DW_AT_call_origin    0x7f
#define DW_AT_call_parameter   0x80
#define DW_AT_call_pc    0x81
#define DW_AT_call_tail_call    0x82
#define DW_AT_call_target    0x83
#define DW_AT_call_target_clobbered    0x84
#define DW_AT_call_data_location    0x85
#define DW_AT_call_data_value    0x86
#define DW_AT_noreturn    0x87
#define DW_AT_alignment    0x88
#define DW_AT_export_symbols    0x89
#define DW_AT_deleted    0x8a
#define DW_AT_defaulted    0x8b
#define DW_AT_loclists_base    0x8c

#define DW_AT_MIPS_linkage_name  0x2007
#define DW_AT_GNU_all_tail_call_sites  0x2116
#define DW_AT_GNU_all_call_sites 0x2117
#define DW_AT_GNU_macros         0x2119


#define DW_AT_lo_user   0x2000
#define DW_AT_hi_user   0x3fff

DW_NAME_TABLE DW_AT_NameTable[] = { 
    {"DW_AT_null",   0x00},
    {"DW_AT_sibling",   0x01},
    {"DW_AT_location",   0x02},
    {"DW_AT_name",   0x03},
    {"unknown",   0x04},
    {"unknown",   0x05},
    {"unknown",   0x06},
    {"unknown",   0x07},
    {"unknown",   0x08},
    {"DW_AT_ordering",   0x09},
    {"unknown",   0x0a},
    {"DW_AT_byte_size",   0x0b},
    {"DW_AT_bit_offset",   0x0c},
    {"DW_AT_bit_size",   0x0d},
    {"unknown",   0x0e},
    {"unknown",   0x0f},
    {"DW_AT_stmt_list",   0x10},
    {"DW_AT_low_pc",   0x11},
    {"DW_AT_high_pc",   0x12},
    {"DW_AT_language",   0x13},
    {"unknown",   0x14},
    {"DW_AT_discr",   0x15},
    {"DW_AT_discr_value",   0x16},
    {"DW_AT_visibility",   0x17},
    {"DW_AT_import",   0x18},
    {"DW_AT_string_length",   0x19},
    {"DW_AT_common_reference",   0x1a},
    {"DW_AT_comp_dir",   0x1b},
    {"DW_AT_const_value",   0x1c},
    {"DW_AT_containing_type",   0x1d},
    {"DW_AT_default_value",   0x1e},
    {"unknown",   0x1f},
    {"DW_AT_inline",   0x20},
    {"DW_AT_is_optional",   0x21},
    {"DW_AT_lower_bound",   0x22},
    {"unknown",   0x23},
    {"unknown",   0x24},
    {"DW_AT_producer",   0x25},
    {"unknown",   0x26},
    {"DW_AT_prototyped",   0x27},
    {"unknown",   0x28},
    {"unknown",   0x29},
    {"DW_AT_return_addr",   0x2a},
    {"unknown",   0x2b},
    {"DW_AT_start_scope",   0x2c},
    {"unknown",   0x2d},
    {"DW_AT_bit_stride",   0x2e},
    {"DW_AT_upper_bound",   0x2f},
    {"unknown",   0x30},
    {"DW_AT_abstract_origin",   0x31},
    {"DW_AT_accessibility",   0x32},
    {"DW_AT_address_class",   0x33},
    {"DW_AT_artificial",   0x34},
    {"DW_AT_base_types",   0x35},
    {"DW_AT_calling_convention",   0x36},
    {"DW_AT_count",   0x37},
    {"DW_AT_data_member_location",   0x38},
    {"DW_AT_decl_column",   0x39},
    {"DW_AT_decl_file",   0x3a},
    {"DW_AT_decl_line",   0x3b},
    {"DW_AT_declaration",   0x3c},
    {"DW_AT_discr_list",   0x3d},
    {"DW_AT_encoding",   0x3e},
    {"DW_AT_external",   0x3f},
    {"DW_AT_frame_base",   0x40},
    {"DW_AT_friend",   0x41},
    {"DW_AT_identifier_case",   0x42},
    {"DW_AT_macro_info",   0x43},
    {"DW_AT_namelist_item",   0x44},
    {"DW_AT_priority",   0x45},
    {"DW_AT_segment",   0x46},
    {"DW_AT_specification",   0x47},
    {"DW_AT_static_link",   0x48},
    {"DW_AT_type",   0x49},
    {"DW_AT_use_location",   0x4a},
    {"DW_AT_variable_parameter",   0x4b},
    {"DW_AT_virtuality",   0x4c},
    {"DW_AT_vtable_elem_location",   0x4d},
    {"DW_AT_allocated",   0x4e},
    {"DW_AT_associated",   0x4f},
    {"DW_AT_data_location",   0x50},
    {"DW_AT_byte_stride",   0x51},
    {"DW_AT_entry_pc",   0x52},
    {"DW_AT_use_UTF8",   0x53},
    {"DW_AT_extension",   0x54},
    {"DW_AT_ranges",   0x55},
    {"DW_AT_trampoline",   0x56},
    {"DW_AT_call_column",   0x57},
    {"DW_AT_call_file",   0x58},
    {"DW_AT_call_line",   0x59},
    {"DW_AT_description",   0x5a},
    {"DW_AT_binary_scale",   0x5b},
    {"DW_AT_decimal_scale",   0x5c},
    {"DW_AT_small",   0x5d},
    {"DW_AT_decimal_sign",   0x5e},
    {"DW_AT_digit_count",   0x5f},
    {"DW_AT_picture_string",   0x60},
    {"DW_AT_mutable",   0x61},
    {"DW_AT_threads_scaled",   0x62},
    {"DW_AT_explicit",   0x63},
    {"DW_AT_object_pointer",   0x64},
    {"DW_AT_endianity",   0x65},
    {"DW_AT_elemental",   0x66},
    {"DW_AT_pure",   0x67},
    {"DW_AT_recursive",   0x68},
    {"DW_AT_signature",   0x69},
    {"DW_AT_main_subprogram",   0x6a},
    {"DW_AT_data_bit_offset",   0x6b},
    {"DW_AT_const_expr",   0x6c},
    {"DW_AT_enum_class",   0x6d},
    {"DW_AT_linkage_name",   0x6e},
    {"DW_AT_string_length_bit_size",   0x6f},
    {"DW_AT_string_length_byte_size",    0x70},
    {"DW_AT_rank",    0x71},
    {"DW_AT_str_offsets_base",     0x72},
    {"DW_AT_addr_base",     0x73},
    {"DW_AT_rnglists_base",    0x74},
    {"unknown",   0x75},
    {"DW_AT_dwo_name",     0x76},
    {"DW_AT_reference",     0x77},
    {"DW_AT_rvalue_reference",     0x78},
    {"DW_AT_macros",     0x79},
    {"DW_AT_call_all_calls",     0x7a},
    {"DW_AT_call_all_source_calls",    0x7b },
    {"DW_AT_call_all_tail_calls",     0x7c},
    {"DW_AT_call_return_pc",     0x7d},
    {"DW_AT_call_value",     0x7e},
    {"DW_AT_call_origin",     0x7f},
    {"DW_AT_call_parameter",    0x80},
    {"DW_AT_call_pc",     0x81},
    {"DW_AT_call_tail_call",     0x82},
    {"DW_AT_call_target",     0x83},
    {"DW_AT_call_target_clobbered",     0x84},
    {"DW_AT_call_data_location",     0x85},
    {"DW_AT_call_data_value",     0x86},
    {"DW_AT_noreturn",     0x87},
    {"DW_AT_alignment",     0x88},
    {"DW_AT_export_symbols",     0x89},
    {"DW_AT_deleted",     0x8a},
    {"DW_AT_defaulted",     0x8b},
    {"DW_AT_loclists_base",     0x8c}};

#define DW_AT_NameTableSize (sizeof (DW_AT_NameTable) / sizeof (DW_AT_NameTable[0]))

DW_NAME_TABLE DW_AT_NameTableUser[] = {

    {"DW_AT_MIPS_linkage_name",  0x2007},
    {"DW_AT_GNU_all_tail_call_sites", 0x2116},
    {"DW_AT_GNU_all_call_sites", 0x2117},
    {"DW_AT_GNU_macros", 0x2119}};

#define DW_AT_NameTableUserSize (sizeof (DW_AT_NameTableUser) / sizeof (DW_AT_NameTableUser[0]))

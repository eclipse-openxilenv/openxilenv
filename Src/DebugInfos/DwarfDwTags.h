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


#define DW_TAG_null  0x00
#define DW_TAG_array_type  0x01
#define DW_TAG_class_type   0x02
#define DW_TAG_entry_point   0x03
#define DW_TAG_enumeration_type   0x04
#define DW_TAG_formal_parameter   0x05
#define DW_TAG_imported_declaration   0x08
#define DW_TAG_label   0x0a
#define DW_TAG_lexical_block   0x0b
#define DW_TAG_member   0x0d
#define DW_TAG_pointer_type   0x0f
#define DW_TAG_reference_type   0x10
#define DW_TAG_compile_unit   0x11
#define DW_TAG_string_type   0x12
#define DW_TAG_structure_type   0x13
#define DW_TAG_subroutine_type   0x15
#define DW_TAG_typedef   0x16
#define DW_TAG_union_type   0x17
#define DW_TAG_unspecified_parameters   0x18
#define DW_TAG_variant   0x19
#define DW_TAG_common_block   0x1a
#define DW_TAG_common_inclusion   0x1b
#define DW_TAG_inheritance   0x1c
#define DW_TAG_inlined_subroutine   0x1d
#define DW_TAG_module   0x1e
#define DW_TAG_ptr_to_member_type   0x1f
#define DW_TAG_set_type   0x20
#define DW_TAG_subrange_type   0x21
#define DW_TAG_with_stmt   0x22
#define DW_TAG_access_declaration   0x23
#define DW_TAG_base_type   0x24
#define DW_TAG_catch_block   0x25
#define DW_TAG_const_type   0x26
#define DW_TAG_constant   0x27
#define DW_TAG_enumerator   0x28
#define DW_TAG_file_type   0x29
#define DW_TAG_friend   0x2a
#define DW_TAG_namelist   0x2b
#define DW_TAG_namelist_item   0x2c
#define DW_TAG_packed_type   0x2d
#define DW_TAG_subprogram   0x2e
#define DW_TAG_template_type_parameter   0x2f
#define DW_TAG_template_value_parameter   0x30
#define DW_TAG_thrown_type   0x31
#define DW_TAG_try_block   0x32
#define DW_TAG_variant_part   0x33
#define DW_TAG_variable   0x34
#define DW_TAG_volatile_type   0x35
#define DW_TAG_dwarf_procedure   0x36
#define DW_TAG_restrict_type   0x37
#define DW_TAG_interface_type   0x38
#define DW_TAG_namespace   0x39
#define DW_TAG_imported_module   0x3a
#define DW_TAG_unspecified_type   0x3b
#define DW_TAG_partial_unit   0x3c
#define DW_TAG_imported_unit   0x3d
#define DW_TAG_condition   0x3f
#define DW_TAG_shared_type   0x40
#define DW_TAG_type_unit   0x41
#define DW_TAG_rvalue_reference_type   0x42
#define DW_TAG_template_alias   0x43
#define DW_TAG_coarray_type  0x44
#define DW_TAG_generic_subrange  0x45
#define DW_TAG_dynamic_type  0x46
#define DW_TAG_atomic_type  0x47
#define DW_TAG_call_site  0x48
#define DW_TAG_call_site_parameter  0x49
#define DW_TAG_skeleton_unit  0x4a
#define DW_TAG_immutable_type  0x4b

#define DW_TAG_lo_user   0x4080
#define DW_TAG_hi_user   0xffff

#define DW_CHILDREN_no   0x00
#define DW_CHILDREN_yes  0x01

DW_NAME_TABLE DW_TAG_NameTable[] = { 
    {"unknown",   0x00},
    {"DW_TAG_array_type",   0x01},
    {"DW_TAG_class_type",   0x02},
    {"DW_TAG_entry_point",   0x03},
    {"DW_TAG_enumeration_type",   0x04},
    {"DW_TAG_formal_parameter",   0x05},
    {"unknown",   0x06},
    {"unknown",   0x07},
    {"DW_TAG_imported_declaration",   0x08},
    {"unknown",   0x09},
    {"DW_TAG_label",   0x0a},
    {"DW_TAG_lexical_block",   0x0b},
    {"unknown",   0x0c},
    {"DW_TAG_member",   0x0d},
    {"unknown",   0x0e},
    {"DW_TAG_pointer_type",   0x0f},
    {"DW_TAG_reference_type",   0x10},
    {"DW_TAG_compile_unit",   0x11},
    {"DW_TAG_string_type",   0x12},
    {"DW_TAG_structure_type",   0x13},
    {"unknown",   0x14},
    {"DW_TAG_subroutine_type",   0x15},
    {"DW_TAG_typedef",   0x16},
    {"DW_TAG_union_type",   0x17},
    {"DW_TAG_unspecified_parameters",   0x18},
    {"DW_TAG_variant",   0x19},
    {"DW_TAG_common_block",   0x1a},
    {"DW_TAG_common_inclusion",   0x1b},
    {"DW_TAG_inheritance",   0x1c},
    {"DW_TAG_inlined_subroutine",   0x1d},
    {"DW_TAG_module",   0x1e},
    {"DW_TAG_ptr_to_member_type",   0x1f},
    {"DW_TAG_set_type",   0x20},
    {"DW_TAG_subrange_type",   0x21},
    {"DW_TAG_with_stmt",   0x22},
    {"DW_TAG_access_declaration",   0x23},
    {"DW_TAG_base_type",   0x24},
    {"DW_TAG_catch_block",   0x25},
    {"DW_TAG_const_type",   0x26},
    {"DW_TAG_constant",   0x27},
    {"DW_TAG_enumerator",   0x28},
    {"DW_TAG_file_type",   0x29},
    {"DW_TAG_friend",   0x2a},
    {"DW_TAG_namelist",   0x2b},
    {"DW_TAG_namelist_item",   0x2c},
    {"DW_TAG_packed_type",   0x2d},
    {"DW_TAG_subprogram",   0x2e},
    {"DW_TAG_template_type_parameter",   0x2f},
    {"DW_TAG_template_value_parameter",   0x30},
    {"DW_TAG_thrown_type",   0x31},
    {"DW_TAG_try_block",   0x32},
    {"DW_TAG_variant_part",   0x33},
    {"DW_TAG_variable",   0x34},
    {"DW_TAG_volatile_type",   0x35},
    {"DW_TAG_dwarf_procedure",   0x36},
    {"DW_TAG_restrict_type",   0x37},
    {"DW_TAG_interface_type",   0x38},
    {"DW_TAG_namespace",   0x39},
    {"DW_TAG_imported_module",   0x3a},
    {"DW_TAG_unspecified_type",   0x3b},
    {"DW_TAG_partial_unit",   0x3c},
    {"DW_TAG_imported_unit",   0x3d},
    {"unknown",   0x3e},
    {"DW_TAG_condition",   0x3f},
    {"DW_TAG_shared_type",   0x40},
    {"DW_TAG_type_unit",   0x41},
    {"DW_TAG_rvalue_reference_type",   0x42},
    // Dwarf5
    {"DW_TAG_template_alias",   0x43},
    {"DW_TAG_coarray_type", 0x44},
    {"DW_TAG_generic_subrange", 0x45},
    {"DW_TAG_dynamic_type", 0x46},
    {"DW_TAG_atomic_type", 0x47},
    {"DW_TAG_call_site", 0x48},
    {"DW_TAG_call_site_parameter", 0x49},
    {"DW_TAG_skeleton_unit", 0x4a},
    {"DW_TAG_immutable_type", 0x4b}};

#define DW_TAG_NameTableSize (sizeof (DW_TAG_NameTable) / sizeof (DW_TAG_NameTable[0]))

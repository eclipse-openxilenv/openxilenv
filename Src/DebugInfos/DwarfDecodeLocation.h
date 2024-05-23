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


#define DW_OP_addr 0x03
#define DW_OP_deref  0x06 
#define DW_OP_const1u  0x08 
#define DW_OP_const1s  0x09 
#define DW_OP_const2u  0x0a 
#define DW_OP_const2s  0x0b 
#define DW_OP_const4u  0x0c 
#define DW_OP_const4s  0x0d 
#define DW_OP_const8u  0x0e 
#define DW_OP_const8s  0x0f 
#define DW_OP_constu  0x10 
#define DW_OP_consts  0x11 
#define DW_OP_dup  0x12 
#define DW_OP_drop  0x13 
#define DW_OP_over  0x14 
#define DW_OP_pick  0x15 
#define DW_OP_swap  0x16 
#define DW_OP_rot  0x17 
#define DW_OP_xderef  0x18 
#define DW_OP_abs  0x19 
#define DW_OP_and  0x1a 
#define DW_OP_div  0x1b 
#define DW_OP_minus  0x1c 
#define DW_OP_mod  0x1d 
#define DW_OP_mul  0x1e 
#define DW_OP_neg  0x1f 
#define DW_OP_not  0x20 
#define DW_OP_or  0x21 
#define DW_OP_plus  0x22 
#define DW_OP_plus_uconst  0x23 
#define DW_OP_shl  0x24 
#define DW_OP_shr  0x25 
#define DW_OP_shra  0x26 
#define DW_OP_xor  0x27 
#define DW_OP_bra  0x28 
#define DW_OP_eq  0x29 
#define DW_OP_ge  0x2a 
#define DW_OP_gt  0x2b 
#define DW_OP_le  0x2c 
#define DW_OP_lt  0x2d 
#define DW_OP_ne  0x2e 
#define DW_OP_skip  0x2f 
#define DW_OP_lit0  0x30 
#define DW_OP_lit1  0x31 
#define DW_OP_lit2  0x32 
#define DW_OP_lit3  0x33 
#define DW_OP_lit4  0x34 
#define DW_OP_lit5  0x35 
#define DW_OP_lit6  0x36 
#define DW_OP_lit7  0x37 
#define DW_OP_lit8  0x38 
#define DW_OP_lit9  0x39 
#define DW_OP_lit10  0x3a 
#define DW_OP_lit11  0x3b 
#define DW_OP_lit12  0x3c 
#define DW_OP_lit13  0x3d 
#define DW_OP_lit14  0x3e 
#define DW_OP_lit15  0x3f 
#define DW_OP_lit16  0x40 
#define DW_OP_lit17  0x41 
#define DW_OP_lit18  0x42 
#define DW_OP_lit19  0x43 
#define DW_OP_lit20  0x44 
#define DW_OP_lit21  0x45 
#define DW_OP_lit22  0x46 
#define DW_OP_lit23  0x47 
#define DW_OP_lit24  0x48 
#define DW_OP_lit25  0x49 
#define DW_OP_lit26  0x4a 
#define DW_OP_lit27  0x4b 
#define DW_OP_lit28  0x4c 
#define DW_OP_lit29  0x4d 
#define DW_OP_lit30  0x4e 
#define DW_OP_lit31  0x4f 
#define DW_OP_reg0  0x50 
#define DW_OP_reg1  0x51 
#define DW_OP_reg2  0x52 
#define DW_OP_reg3  0x53 
#define DW_OP_reg4  0x54 
#define DW_OP_reg5  0x55 
#define DW_OP_reg6  0x56 
#define DW_OP_reg7  0x57 
#define DW_OP_reg8  0x58 
#define DW_OP_reg9  0x59 
#define DW_OP_reg10  0x5a 
#define DW_OP_reg11  0x5b 
#define DW_OP_reg12  0x5c 
#define DW_OP_reg13  0x5d 
#define DW_OP_reg14  0x5e 
#define DW_OP_reg15  0x5f 
#define DW_OP_reg16  0x60 
#define DW_OP_reg17  0x61 
#define DW_OP_reg18  0x62 
#define DW_OP_reg19  0x63 
#define DW_OP_reg20  0x64 
#define DW_OP_reg21  0x65 
#define DW_OP_reg22  0x66 
#define DW_OP_reg23  0x67 
#define DW_OP_reg24  0x68 
#define DW_OP_reg25  0x69 
#define DW_OP_reg26  0x6a 
#define DW_OP_reg27  0x6b 
#define DW_OP_reg28  0x6c 
#define DW_OP_reg29  0x6d 
#define DW_OP_reg30  0x6e 
#define DW_OP_reg31  0x6f 
#define DW_OP_breg0  0x70 
#define DW_OP_breg1  0x71 
#define DW_OP_breg2  0x72 
#define DW_OP_breg3  0x73 
#define DW_OP_breg4  0x74 
#define DW_OP_breg5  0x75 
#define DW_OP_breg6  0x76 
#define DW_OP_breg7  0x77 
#define DW_OP_breg8  0x78 
#define DW_OP_breg9  0x79 
#define DW_OP_breg10  0x7a 
#define DW_OP_breg11  0x7b 
#define DW_OP_breg12  0x7c 
#define DW_OP_breg13  0x7d 
#define DW_OP_breg14  0x7e 
#define DW_OP_breg15  0x7f 
#define DW_OP_breg16  0x80 
#define DW_OP_breg17  0x81 
#define DW_OP_breg18  0x82 
#define DW_OP_breg19  0x83 
#define DW_OP_breg20  0x84 
#define DW_OP_breg21  0x85 
#define DW_OP_breg22  0x86 
#define DW_OP_breg23  0x87 
#define DW_OP_breg24  0x88 
#define DW_OP_breg25  0x89 
#define DW_OP_breg26  0x8a 
#define DW_OP_breg27  0x8b 
#define DW_OP_breg28  0x8c 
#define DW_OP_breg29  0x8d 
#define DW_OP_breg30  0x8e 
#define DW_OP_breg31  0x8f 
#define DW_OP_regx  0x90 
#define DW_OP_fbreg  0x91 
#define DW_OP_bregx  0x92 
#define DW_OP_piece  0x93 
#define DW_OP_deref_size  0x94 
#define DW_OP_xderef_size  0x95 
#define DW_OP_nop  0x96 
/* DWARF 3 extensions.  */
#define DW_OP_push_object_address  0x97 
#define DW_OP_call2  0x98 
#define DW_OP_call4  0x99 
#define DW_OP_call_ref  0x9a 
#define DW_OP_form_tls_address  0x9b 
#define DW_OP_call_frame_cfa  0x9c 
#define DW_OP_bit_piece  0x9d 

/* DWARF 4 extensions.  */
#define DW_OP_implicit_value  0x9e 
#define DW_OP_stack_value  0x9f 

#define DW_OP_lo_user 0xe0	/* Implementation-defined range start.  */
#define DW_OP_hi_user 0xff	/* Implementation-defined range end.  */


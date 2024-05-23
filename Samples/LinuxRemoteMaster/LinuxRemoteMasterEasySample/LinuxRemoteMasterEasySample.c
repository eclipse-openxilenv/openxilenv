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
#include <string.h>
#include "XilEnvRtProc.h"
#include "XilEnvExtProc.h"

#define TEST_VARIABLE
#define TEST_VARIABLE_ARRAY
#define TEST_VARIABLE_ALL_INFOS
#define TEST_VARIABLE_LABEL_FROM_DBGINFOS
#define ARRAY_SIZE 1000

int ret_value;
int rx_can_msg_cnt;

void exit(int);
int test_exit;

// Some test variable
signed char one_signed_char_variable;
unsigned char one_unsigned_char_variable;
signed short one_signed_short_variable;
unsigned short one_unsigned_short_variable;
signed int one_signed_int_variable;
unsigned int one_unsigned_int_variable;
float one_float_variable;
double one_double_variable;
unsigned int LoopCount = 2;

unsigned char CallExitFunc = 0;

double Array[ARRAY_SIZE];
double AllInfos1 = 1.0;
double AllInfos2 = 12;

double read_only_double = 1.0;
double write_only_double = 2.0;
double read_write_double = 3.0;

double AllInfosWithNoName = 4.0;

void reference_varis(void)
{
#ifdef TEST_VARIABLE
#ifdef TEST_VARIABLE_ARRAY
    int x;
#endif

    REF_DIR_DOUBLE_VAR(READ_ONLY_REFERENCE, read_only_double, "read_only_double", "unit");
    REF_DIR_DOUBLE_VAR(WRITE_ONLY_REFERENCE, write_only_double, "write_only_double", "unit");
    REF_DIR_DOUBLE_VAR(READ_WRITE_REFERENCE, read_write_double, "read_write_double", "unit");

#ifdef TEST_VARIABLE_ALL_INFOS
#ifdef TEST_VARIABLE_LABEL_FROM_DBGINFOS
    reference_double_var_all_infos(&AllInfosWithNoName, NULL, "unit", CONVTYPE_EQUATION, "#*2+10", -11.0, 12.0, 5, 6, 0xFF0000, 0, 2.5);
#endif
    REF_DOUBLE_VAR_AI(AllInfos1, "unit", CONVTYPE_TEXTREPLACE, "0 0 \"null\"; 1 1 \"eins\";", -11.0, 12.0, 5, 6, 0xFF0000, 0, 2.5);
    REF_DOUBLE_VAR_AI(AllInfos2, "unit", CONVTYPE_EQUATION, "#*2+10", -11.0, 12.0, 5, 6, 0xFF0000, 0, 2.5);
#endif
    REFERENCE_BYTE_VAR(one_signed_char_variable, "one_signed_char_variable")
    REFERENCE_UBYTE_VAR(one_unsigned_char_variable, "one_unsigned_char_variable")
    REFERENCE_WORD_VAR(one_signed_short_variable, "one_signed_short_variable")
    REFERENCE_UWORD_VAR(one_unsigned_short_variable, "one_unsigned_short_variable")
    REFERENCE_DWORD_VAR(one_signed_int_variable, "one_signed_int_variable")
    REFERENCE_UDWORD_VAR(one_unsigned_int_variable, "one_unsigned_int_variable")
    REFERENCE_FLOAT_VAR(one_float_variable, "one_float_variable")
    REFERENCE_DOUBLE_VAR(one_double_variable, "one_double_variable")
#ifdef TEST_VARIABLE_ARRAY  
    for (x = 0; x < ARRAY_SIZE; x++) {
        char Help[32];
        sprintf(Help, "array[%i]", x);
        REFERENCE_DOUBLE_VAR(Array[x], Help);
    }
#endif
    REFERENCE_UDWORD_VAR(LoopCount, "LoopCount");
#endif
    REFERENCE_UBYTE_VAR(CallExitFunc, "CallExitFunc")
}

int CanFifoHandle;

CAN_ACCEPT_MASK AcceptMask[] = { {0, 0, 0x1FFFFFFF, 0},  {1, 0, 0x1FFFFFFF, 0},  {2, 0, 0x1FFFFFFF, 0},  {3, 0, 0x1FFFFFFF, 0},  {4, 0, 0x1FFFFFFF, 0},  {5, 0, 0x1FFFFFFF, 0}, {-1, 0, 0x1FFFFFFF, 0} };

int init_test_object(void)
{
    LoopCount = 1;
    CanFifoHandle = CreateCanFifos(100, DEFAULT_PROCESS_PARAMETER);

    SetAcceptMask4CanFifo(DEFAULT_PROCESS_PARAMETER, AcceptMask, 7);
    return 0;
}

void terminate_test_object(void)
{
}

#ifdef TEST_VARIABLE
// Test of the exit function
void subroutine(void)
{
    if (test_exit) exit(1);
}
#endif


void CANSendMsg(int arg_channel, unsigned int arg_id, unsigned char arg_size, unsigned char *arg_ptr_data)
{
    CAN_FIFO_ELEM can_fifo;

    can_fifo.channel = arg_channel;
    can_fifo.id = arg_id;
    can_fifo.size = arg_size;
    memcpy(can_fifo.data, arg_ptr_data, 8);
    can_fifo.node = 1;
    can_fifo.flag = 0;
    can_fifo.ext = 1;
    WriteCanMessageFromProcess2Fifo(DEFAULT_PROCESS_PARAMETER, &can_fifo);
}

unsigned char CanData[8] = {1, 2, 3, 4, 5, 6, 7, 8};

void cyclic_test_object(void)
{
    CAN_FIFO_ELEM CanMessage;
    char Help[1024];

#ifdef TEST_VARIABLE
#ifdef TEST_VARIABLE_ARRAY
    unsigned int x, y;
#endif
    // do something not usefull
    one_signed_char_variable =
        one_unsigned_char_variable =
        one_signed_short_variable =
        one_unsigned_short_variable =
        one_signed_int_variable =
        one_unsigned_int_variable =
        one_float_variable =
        one_double_variable;
#ifdef TEST_VARIABLE_ARRAY
    // use cpu power
    for (x = 0; x < LoopCount; x++) {
        for (y = 0; y < (ARRAY_SIZE - 1); y++) {
            Array[y] = Array[y + 1];
        }
        Array[ARRAY_SIZE - 1]++;
        if (Array[ARRAY_SIZE - 1] > 100) Array[ARRAY_SIZE - 1] = 0;

    }
#endif
#endif
    CANSendMsg(0, 0x12345678, 8, &(CanData[0]));

    write_only_double += 0.001;

    for (rx_can_msg_cnt = 0; rx_can_msg_cnt < 100; rx_can_msg_cnt++) {
        ret_value = ReadCanMessageFromFifo2Process(DEFAULT_PROCESS_PARAMETER, &CanMessage);
        if (ret_value <= 0) break;
    }

    if (CallExitFunc) {
        ThrowError(1, Help);
        exit(1);
        CallExitFunc = 0;
    }
}

DEFINE_TASK_CONTROL_BLOCK(TestObjekt, 100)

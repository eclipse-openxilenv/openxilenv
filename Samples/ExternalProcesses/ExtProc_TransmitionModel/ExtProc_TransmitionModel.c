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

#define XILENV_INTERFACE_TYPE XILENV_DLL_INTERFACE_TYPE
#include "XilEnvExtProc.h"
#include "XilEnvExtProcMain.c"

const double Transmitions[2] = {0.1, 0.2};
//const double Transmitions[2] = {10.0, 20.0};

// Input and output signals
double MotorTorque;  // Nm
double MotorSpeed; // rmp
double OutputTorque;  // Nm
double OutputSpeed; // rmp
uint8_t GearNo;

// This is the frist call if the process is started 
// Here all input and output signal should be register
void reference_varis (void)
{
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, OutputTorque, "Nm", 0, "", 0.0, 10000.0, 6, 2, SC_RGB (255,255,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, MotorTorque, "Nm", 0, "", -1000.0, 1000.0, 6, 2, SC_RGB (170,85,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, OutputSpeed, "rpm", 0, "", -500.0, 1500.0, 6, 2, SC_RGB (0,255,255), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, MotorSpeed, "rpm", 0, "", -2000.0, 10000.0, 6, 2, SC_RGB (170,255,255), 0, 1.0);
    REF_DIR_UBYTE_VAR_AI (READ_WRITE_REFERENCE, GearNo, "", 0, "", 0.0, 1.0, 2, 0, COLOR_UNDEFINED, 0, 1.0);
}

// This will call one time after reference_varis
int init_test_object (void)
{
    GearNo = 0;
    return 0;   /* == 0 -> no error
                   != 0 -> a error occur do not continue */
}

void cyclic_test_object(void) 
{
    if (GearNo <= 1) {
        OutputTorque = MotorTorque / Transmitions[GearNo];
        MotorSpeed = OutputSpeed / Transmitions[GearNo];
    } else {
        OutputTorque = 0.0;
        MotorSpeed = 0.0;
    }
}

// This will be called if the process will be terminate
void terminate_test_object (void)
{
}

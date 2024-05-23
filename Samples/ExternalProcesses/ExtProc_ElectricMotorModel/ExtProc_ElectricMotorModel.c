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

// Model parameters
const double Efficiency = 0.9;
const double MaxTorque = 1000.0;

// Input and output signals
double MotorTorque;  // Nm
double MotorSpeed; // rpm
double MotorCurrent; // A
double MotorVoltage; // V
double SampleTime; // s

// This is the frist call if the process is started 
// Here all input and output signal should be register
void reference_varis (void)
{
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, MotorTorque, "Nm", 0, "", -1000.0, 1000.0, 6, 2, SC_RGB (170,85,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, MotorSpeed, "rpm", 0, "",  -2000.0, 10000.0, 6, 2, SC_RGB (170,255,255), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, MotorCurrent, "A", 0, "", 0.0, 400.0, 6, 2, SC_RGB (255,0,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, MotorVoltage, "V", 0, "", 0.0, 500.0, 6, 2, SC_RGB (0,0,255), 0, 1.0);
    REF_DIR_DOUBLE_VAR (READ_ONLY_REFERENCE, SampleTime, "XilEnv.SampleTime", "s");
}

// This will call one time after reference_varis
int init_test_object (void)
{
    return 0;   /* == 0 -> no error
                   != 0 -> a error occur do not continue */
}

void cyclic_test_object(void) 
{
    double Power = Efficiency * MotorCurrent * MotorVoltage;
    if (Power > 100.0) {
        if ((MaxTorque / 9.5492965855137201461330258023509 * MotorSpeed) <= Power) {
            MotorTorque = MaxTorque;
        } else {
            MotorTorque = 9.5492965855137201461330258023509 * Power / MotorSpeed;
        }
    } else {
        MotorTorque = 0.0;
    }
}

// This will be called if the process will be terminate
void terminate_test_object (void)
{
}

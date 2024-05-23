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


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define XILENV_INTERFACE_TYPE XILENV_DLL_INTERFACE_TYPE
#include "XilEnvExtProc.h"
#include "XilEnvExtProcMain.c"

const double StartBatteryCapacity = 60000.0; // Wh
const double StartVoltage  = 400.0; // V
const double EndVoltage  = 350.0; // V

// Inpit and output signals
double BatteryCurrent;  // A
double MotorCurrent;  // A
double ChargeCurrent; // A
double BatteryVoltage; // V
double BatteryCapacity; // Ah
double SampleTime;  // s

// This is the frist call if the process is started 
// Here all input and output signal should be register
void reference_varis (void)
{
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, BatteryVoltage, "V", 0, "", 0.0, 500.0, 6, 2, SC_RGB (10,20,30), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, BatteryCurrent, "A", 0, "", -20.0, 100.0, 6, 2, SC_RGB (10,20,30), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, MotorCurrent, "A", 0, "", 0.0, 400.0, 6, 2, SC_RGB (255,0,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, ChargeCurrent, "A", 0, "", -20.0, 100.0, 6, 2, SC_RGB (10,20,30), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, BatteryCapacity, "Wh", 0, "", 0.0, 100000.0, 6, 0, SC_RGB (10,20,30), 0, 1.0);
    REF_DIR_DOUBLE_VAR (READ_ONLY_REFERENCE, SampleTime, "XilEnv.SampleTime", "s");
}

// This will call one time after reference_varis
int init_test_object (void)
{
    BatteryCapacity = StartBatteryCapacity;
    return 0;   /* == 0 -> no error
                   != 0 -> a error occur do not continue */
}

void cyclic_test_object(void) 
{
    BatteryCurrent = MotorCurrent - ChargeCurrent;
    BatteryVoltage = EndVoltage + (StartVoltage - EndVoltage) * BatteryCapacity / StartBatteryCapacity;
    double Power = BatteryCurrent * BatteryVoltage;
    if (Power > 0.0) {
        if (BatteryCapacity > 0) {
            // Discharge
            BatteryCapacity -= Power * SampleTime / 3600.0;
        } else {
            BatteryVoltage = 0.0;
        }
    } else if (Power < 0.0) {
        if (BatteryCapacity < StartBatteryCapacity) {
            // Charge
            BatteryCapacity -= Power * SampleTime / 3600.0;
        }
    }
}

// This will be called if the process will be terminate
void terminate_test_object (void)
{
}

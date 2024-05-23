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

// Global variable of the external process
short Ramp;
double Sinus;
short Random;
double SampleTime;

// use volatile const to avoid compiler optimization
volatile const short BOTTOM_LIMIT_RAMP = 0;
volatile const short UPPER_LIMIT_RAMP = 1000;

volatile const double SINUS_AMPLITUDE  = 1.0;
volatile const double SINUS_FREQUENCY = 0.2;

// This will be called first if the process is started
void reference_varis (void)
{
    REFERENCE_WORD_VAR(Ramp, "Ramp");
    REFERENCE_DOUBLE_VAR(Sinus, "Sinus");
    REFERENCE_WORD_VAR(Random, "Random");
    REFERENCE_DOUBLE_VAR(SampleTime, "XilEnv.SampleTime");
}

// This function will be called next to reference_varis
int init_test_object(void)
{
    return 0;   // == 0 -> No error continue
        // != 0 -> Error do not continue
}

// This will call every simulated cycle
void cyclic_test_object(void)
{
    static double SinusTime;
    if(Ramp++ > UPPER_LIMIT_RAMP) Ramp = BOTTOM_LIMIT_RAMP;
    SinusTime += 2.0 * M_PI * SampleTime * SINUS_FREQUENCY;
    Sinus = SINUS_AMPLITUDE * sin(SinusTime);
    Random = rand();
}

// This will be called if the external processs will be terminated
void terminate_test_object(void)
{
}

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

// Model parameters
const double CarWeight = 1500;   // Kg
const double WheelRadius = 0.51915; // m
const double WheelCircumference = 1.63; // m
const double FacingSurface = 2.0; // m^2
const double AirDensity = 1.29;  // Kg/m^3
const double RollingResistance = 0.015;
const double AirResistance = 0.33;
const double Gravity = 9.81; // m/s^2
const double MaxBreak = 5.0; // m/s^2

// Input and output signals
double OutputTorque;  // Nm
double OutputSpeed;  // rpm
double Speed; // m/s
double BreakPedal;  // %
double RoadGradient;  // %
double SteeringAngle; // grad
double SampleTime; // s

// This is the frist call if the process is started 
// Here all input and output signal should be register
void reference_varis (void)
{
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, OutputTorque, "Nm", 0, "", 0.0, 10000.0, 6, 2, SC_RGB (255,255,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, OutputSpeed, "rpm", 0, "", -500.0, 1500.0, 6, 2, SC_RGB (0,255,255), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_WRITE_REFERENCE, Speed, "m/s", 0, "", -20.0, 100.0, 6, 2, SC_RGB (0,170,255), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, BreakPedal, "%", 0, "", 0.0, 100.0, 6, 2, SC_RGB (170,170,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, RoadGradient, "%", 0, "", 0.0, 100.0, 6, 2, SC_RGB (255,170,255), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, SteeringAngle, "GRAD", 0, "", -45.0, 45.0, 6, 2, SC_RGB (0,100,0), 0, 1.0);
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
    double Flr = 0.5 * FacingSurface * AirResistance * AirDensity *  Speed * Speed;
    double Fg = CarWeight * Gravity;
    double Fr = Fg * RollingResistance;
    double RoadGradientDiv100 = RoadGradient * 0.01;
    double Fs = Fg * RoadGradientDiv100 / sqrt(1 + RoadGradientDiv100 * RoadGradientDiv100);
    double Fe = OutputTorque / WheelRadius;
    double Fb = 0.01 * BreakPedal * CarWeight * Gravity; // BreakPedal unit %
    double F;

    if (Speed > 0.0) {
        F = Fe - Flr - Fr - Fb - Fs;
    } else if (Speed < 0.0) {
        F = Fe + Flr + Fr + Fb - Fs;
    } else {  // Speed == 0.0
        if ((Fe - Fs) > (Fb + Fr)) {
            F = Fe - Fs;
        } else if ((Fe - Fs) < -(Fb + Fr)) {
            F = Fe - Fs;
        } else {
            F = 0.0;
        }
    }
    double NewSpeed = Speed + SampleTime * F / CarWeight;           // m/s
    if (((NewSpeed < 0.0) && (Speed > 0.0)) ||  // if it go through the zero point set it zero
        ((NewSpeed > 0.0) && (Speed < 0.0))) {
        Speed = 0.0;
    } else {
        Speed = NewSpeed;
    }
    OutputSpeed = 9.5492965855137201461330258023509 * Speed / WheelRadius;
    // 60.0 / (2.0 * Pi) = 9.5492965855137201461330258023509
}

// This will be called if the process will be terminate
void terminate_test_object (void)
{
}

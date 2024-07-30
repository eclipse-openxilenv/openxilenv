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
#include "XilEnvExtProcCan.h"
#include "XilEnvExtProcMain.c"


// Parameter
uint32_t MaxPower = 100000; // W
uint16_t BatteryUnderVoltageLimit = 300;  // V
uint8_t CAN_TIMEOUT = 5;

// Input and output signals
double MotorTorque;  // Nm
double MotorSpeed; // rpm
double MotorCurrent; // A
double MotorVoltage; // V
double SampleTime; // s
uint8_t MotorErrorState;

// This is the frist call if the process is started 
// Here all input and output signal should be register
void reference_varis (void)
{
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, MotorTorque, "Nm", 0, "", -1000.0, 1000.0, 6, 2, SC_RGB (170,85,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, MotorSpeed, "rpm", 0, "", -2000.0, 10000.0, 6, 2, SC_RGB (170,255,255), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, MotorCurrent, "A", 0, "", 0.0, 400.0, 6, 2, SC_RGB (255,0,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, MotorVoltage, "V", 0, "", 0.0, 500.0, 6, 2, SC_RGB (0,0,255), 0, 1.0);
    REF_DIR_UBYTE_VAR_AI (WRITE_ONLY_REFERENCE, MotorErrorState, "", 2,
                          "0 0 \"RGB(0x00:0xFF:0x00)OK\";1 1 \"RGB(0xFF:0x00:0x00)CAN\";2 2 \"RGB(0xFF:0x00:0x00)Under voltage\";",
                          0.0, 5.0, 6, 2, SC_RGB (255,255,255), 0, 1.0);
}

// This will call one time after reference_varis
int init_test_object (void)
{
    MotorErrorState = 0;
    if ((sc_open_virt_can_fd (0, 0) == 0) &&
        (sc_cfg_virt_can_msg_buff (0, 0, 0x100, 0, 8, 0) == 0) &&  // Battery control unit
        (sc_cfg_virt_can_msg_buff (0, 1, 0x200, 0, 8, 1) == 0) &&  // Electric motor control unit
        (sc_cfg_virt_can_msg_buff (0, 2, 0x300, 0, 8, 0) == 0) &&  // Transmition control unit
        (sc_cfg_virt_can_msg_buff (0, 3, 0x400, 0, 8, 0) == 0)) {  // Vehicle control unit
        return 0;   // == 0 -> no error
    } else {
        return -1;  // != 0 -> a error occur do not continue */
    }
}

uint8_t BatteryCanTimeoutCounter;
uint8_t TransmitionCanTimeoutCounter;
uint8_t VehicleCanTimeoutCounter;
uint8_t BatteryCanTimeoutState;
uint8_t TransmitionCanTimeoutState;
uint8_t VehicleCanTimeoutState;

uint16_t AcceleratorPosition;  // %
uint8_t FireUp;
uint16_t BatteryCurrent;  // A
uint16_t BatteryVoltage; // V


void cyclic_test_object(void)
{
    uint8_t Ext;
    uint8_t Size;
    uint8_t Data[8];

    MotorErrorState = 0;

    // Receiving CAN messages
    if (sc_read_virt_can_msg_buff(0, 0, &Ext, &Size, Data) == 1) { // Battery control unit
        BatteryVoltage = (uint16_t)Data[1] + ((uint16_t)Data[0] << 8);
        BatteryCurrent = (uint16_t)Data[3] + ((uint16_t)Data[2] << 8);
        BatteryCanTimeoutCounter = 0;
        BatteryCanTimeoutState = 0;
    } else {
        BatteryCanTimeoutCounter++;
        if (BatteryCanTimeoutCounter > CAN_TIMEOUT) {
            BatteryCanTimeoutState = 1;
        } else {
            BatteryCanTimeoutCounter++;
        }
    }
    if (sc_read_virt_can_msg_buff(0, 2, &Ext, &Size, Data) == 1) { // Receive from transmition control unit
        // there are no needed information inside this message
        TransmitionCanTimeoutCounter = 0;
        TransmitionCanTimeoutState = 0;
    } else {
        TransmitionCanTimeoutCounter++;
        if (TransmitionCanTimeoutCounter > CAN_TIMEOUT) {
            TransmitionCanTimeoutState = 1;
        } else {
            TransmitionCanTimeoutCounter++;
        }
    }
    if (sc_read_virt_can_msg_buff(0, 3, &Ext, &Size, Data) == 1) { // Receive from vehicle control unit
        AcceleratorPosition = (uint16_t)Data[0] + ((uint16_t)Data[1] << 8);
        FireUp = Data[2] & 0x1;
        VehicleCanTimeoutCounter = 0;
        VehicleCanTimeoutState = 0;
    } else {
        VehicleCanTimeoutCounter++;
        if (VehicleCanTimeoutCounter > CAN_TIMEOUT) {
            VehicleCanTimeoutState = 1;
        } else {
            VehicleCanTimeoutCounter++;
        }
    }
    if (BatteryCanTimeoutState || TransmitionCanTimeoutState || VehicleCanTimeoutState) {
        MotorErrorState = 1;
    }

    if (FireUp) {
        if (BatteryVoltage < BatteryUnderVoltageLimit) {
            MotorErrorState = 2;
        } else {
            //double Power = MapAccess(MotorSpeed, AcceleratorPosition, &Map);
            double Power = ((double)MaxPower * (double)AcceleratorPosition) / 100.0;
            MotorVoltage = BatteryVoltage;
            MotorCurrent = Power / MotorVoltage;
        }
    } else {
        MotorVoltage = 0.0;
        MotorCurrent = 0.0;
    }

    if (MotorErrorState) {
        MotorVoltage = 0.0;
        MotorCurrent = 0.0;
    }

    // Transmit the motor control unit message
    memset(Data, 0, sizeof(Data));
    uint16_t MotorTorqueInt = (uint16_t)MotorTorque;
    Data[0] = MotorTorqueInt;
    Data[1] = MotorTorqueInt >> 8;
    uint16_t MotorCurrentInt = (uint16_t)MotorCurrent;
    Data[2] = MotorCurrentInt;
    Data[3] = MotorCurrentInt >> 8;
    uint16_t MotorSpeedInt = (uint16_t)MotorSpeed;
    Data[4] = MotorSpeedInt;
    Data[5] = MotorSpeedInt >> 8;
    Data[6] = MotorErrorState;
    sc_write_virt_can_msg_buff(0, 1, Data);   // write to CAN
}

// This will be called if the process will be terminate
void terminate_test_object (void)
{
    sc_close_virt_can(0);
}

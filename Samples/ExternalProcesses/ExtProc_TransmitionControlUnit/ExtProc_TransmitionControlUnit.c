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

#if defined(_WIN32) && !defined(__GNUC__)
#define MAP2MYSEC_BEFORE __declspec(allocate("mysec"))
#define MAP2MYSEC_BEHIND
#else
#define MAP2MYSEC_BEFORE
#define MAP2MYSEC_BEHIND __attribute__((section("mysec")))
#endif

// Parameters
#pragma section("mysec",read,write)
MAP2MYSEC_BEFORE const uint16_t MAP2MYSEC_BEHIND SwitchSpeedUpperLimit = 500;
MAP2MYSEC_BEFORE const uint16_t MAP2MYSEC_BEHIND SwitchSpeedLowerLimit = 300;
MAP2MYSEC_BEFORE const uint8_t MAP2MYSEC_BEHIND CAN_TIMEOUT = 10;

// Input and output signals
double OutputTorque;
//double MotorTorque;
double OutputSpeed;
//double MotorSpeed;
uint8_t GearNo;
uint8_t TransmitionErrorState;

// This is the frist call if the process is started 
// Here all input and output signal should be register
void reference_varis (void)
{
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, OutputTorque, "Nm", 0, "", 0.0, 10000.0, 6, 2, SC_RGB (255,255,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, OutputSpeed, "rpm", 0, "", -500.0, 1500.0, 6, 2, SC_RGB (0,255,255), 0, 1.0);
    REF_DIR_UBYTE_VAR_AI (WRITE_ONLY_REFERENCE, GearNo, "", 0, "", 0.0, 1.0, 2, 0, COLOR_UNDEFINED, 0, 1.0);
    REF_DIR_UBYTE_VAR_AI (WRITE_ONLY_REFERENCE, TransmitionErrorState, "", 2,
                          "0 0 \"RGB(0x00:0xFF:0x00)OK\";1 1 \"RGB(0xFF:0x00:0x00)CAN\";",
                          0.0, 5.0, 6, 2, COLOR_UNDEFINED, 0, 1.0);
}

// This will call one time after reference_varis
int init_test_object (void)
{
    GearNo = 0;
    //TransmitionErrorState = 0;
    if ((sc_open_virt_can_fd (0, 0) == 0) &&
        (sc_cfg_virt_can_msg_buff (0, 0, 0x100, 0, 8, 0) == 0) &&  // Battery control unit
        (sc_cfg_virt_can_msg_buff (0, 1, 0x200, 0, 8, 0) == 0) &&  // Electric motor control unit
        (sc_cfg_virt_can_msg_buff (0, 2, 0x300, 0, 8, 1) == 0) &&  // Transmition control unit
        (sc_cfg_virt_can_msg_buff (0, 3, 0x400, 0, 8, 0) == 0)) {  // Vehicle control unit
        return 0;   // == 0 -> no error
    } else {
        return -1;  // != 0 -> a error occur do not continue */
    }
}

uint8_t BatteryCanTimeoutCounter;
uint8_t MotorCanTimeoutCounter;
uint8_t VehicleCanTimeoutCounter;
uint8_t BatteryCanTimeoutState;
uint8_t MotorCanTimeoutState;
uint8_t VehicleCanTimeoutState;

uint16_t AcceleratorPosition;  // %
uint8_t FireUp;
uint16_t BatteryCurrent;  // A
uint16_t BatteryVoltage; // V
uint16_t MotorSpeed;   // rpm

void cyclic_test_object(void) 
{
    uint8_t Ext;
    uint8_t Size;
    uint8_t Data[8];

    TransmitionErrorState = 0;

    // Receiving CAN messages
    if (sc_read_virt_can_msg_buff(0, 0, &Ext, &Size, Data) == 1) { // Battery control unit
        BatteryCurrent = (uint16_t)Data[0] + ((uint16_t)Data[1] << 8);
        BatteryVoltage = (uint16_t)Data[2] + ((uint16_t)Data[3] << 8);
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
    if (sc_read_virt_can_msg_buff(0, 1, &Ext, &Size, Data) == 1) { // Receive from motor control unit
        MotorSpeed = (uint16_t)Data[4] + ((uint16_t)Data[5] << 8);
        MotorCanTimeoutCounter = 0;
        MotorCanTimeoutState = 0;
    } else {
        MotorCanTimeoutCounter++;
        if (MotorCanTimeoutCounter > CAN_TIMEOUT) {
            MotorCanTimeoutState = 1;
        } else {
            MotorCanTimeoutCounter++;
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
    if (BatteryCanTimeoutState || MotorCanTimeoutState || VehicleCanTimeoutState) {
        TransmitionErrorState = 1;
    }

    if (GearNo) {
        if (OutputSpeed < SwitchSpeedLowerLimit) {
            GearNo = 0; // Switch back to first gear
        }
    } else {
        if (OutputSpeed > SwitchSpeedUpperLimit) {
            GearNo = 1; // Switch to second gear
        }
    }

    // Transmit the transmition control unit message
    memset(Data, 0, sizeof(Data));
    uint16_t InputSpeedInt = (uint16_t)MotorSpeed;
    Data[0] = InputSpeedInt;
    Data[1] = InputSpeedInt >> 8;
    uint16_t OutputSpeedInt = (uint16_t)OutputSpeed;
    Data[2] = OutputSpeedInt;
    Data[3] = OutputSpeedInt >> 8;
    Data[4] = GearNo;
    Data[5] = TransmitionErrorState;
    sc_write_virt_can_msg_buff(0, 2, Data);   // write to CAN

}

// This will be called if the process will be terminate
void terminate_test_object (void)
{
}

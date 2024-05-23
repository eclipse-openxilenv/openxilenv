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

// Parameters
const double StartBatteryCapacity = 60000.0;    // Wh

const uint16_t BatteryUnderVoltageLimit  = 350; // V
const uint16_t BatteryCurrentLimitLimit  = 300; // A
const uint16_t BatteryMaxLoadCurrent  = 100;    // A
uint8_t CAN_TIMEOUT = 10;


// Input and output signals
double BatteryCurrent;  // A
double ChargeCurrent;   // A
double BatteryVoltage;  // V
double BatteryCapacity; // Wh
double SampleTime;      // s

uint8_t BatteryErrorState;

// This is the frist call if the process is started 
// Here all input and output signal should be register
void reference_varis (void)
{
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, BatteryVoltage, "V", 0, "", 0.0, 500.0, 6, 2, SC_RGB (0,0,255), 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (READ_ONLY_REFERENCE, BatteryCurrent, "A", 0, "", -400.0, 400.0, 6, 2, COLOR_UNDEFINED, 0, 1.0);
    REF_DIR_DOUBLE_VAR_AI (WRITE_ONLY_REFERENCE, ChargeCurrent, "A", 0, "", 0.0, 400.0, 6, 2, SC_RGB (0,170,0), 0, 1.0);
    REF_DIR_DOUBLE_VAR (READ_ONLY_REFERENCE, SampleTime, "XilEnv.SampleTime", "s");
    REF_DIR_UBYTE_VAR_AI (WRITE_ONLY_REFERENCE, BatteryErrorState, "", 2,
                          "0 0 \"RGB(0x00:0xFF:0x00)OK\";1 1 \"RGB(0xFF:0x00:0x00)CAN\";2 2 \"RGB(0xFF:0x00:0x00)Current\";3 3 \"RGB(0xFF:0x00:0x00)Empty\";",
                          0.0, 5.0, 6, 2, COLOR_UNDEFINED, 0, 1.0);
}

// This will call one time after reference_varis
int init_test_object (void)
{
    BatteryCapacity = StartBatteryCapacity;
    BatteryErrorState = 0;
    if ((sc_open_virt_can_fd (0, 0) == 0) &&
        (sc_cfg_virt_can_msg_buff (0, 0, 0x100, 0, 8, 1) == 0) &&  // Battery control unit
        (sc_cfg_virt_can_msg_buff (0, 1, 0x200, 0, 8, 0) == 0) &&  // Electric motor control unit
        (sc_cfg_virt_can_msg_buff (0, 2, 0x300, 0, 8, 0) == 0) &&  // Transmition control unit
        (sc_cfg_virt_can_msg_buff (0, 3, 0x400, 0, 8, 0) == 0)) {  // Vehicle control unit
        return 0;   // == 0 -> no error
    } else {
        return -1;  // != 0 -> a error occur do not continue */
    }
}

uint8_t MotorCanTimeoutCounter;
uint8_t TransmitionCanTimeoutCounter;
uint8_t VehicleCanTimeoutCounter;
uint8_t MotorCanTimeoutState;
uint8_t TransmitionCanTimeoutState;
uint8_t VehicleCanTimeoutState;

uint16_t AcceleratorPosition;  // %
uint8_t FireUp;
uint8_t PlugIn;
uint16_t MotorCurrent;  // A
uint16_t MotorVoltage; // V

void cyclic_test_object(void) 
{
    uint8_t Ext;
    uint8_t Size;
    uint8_t Data[8];

    BatteryErrorState = 0;

    // Receiving CAN messages
    if (sc_read_virt_can_msg_buff(0, 1, &Ext, &Size, Data) == 1) { // Motor control unit
        MotorCurrent = (uint16_t)Data[1] + ((uint16_t)Data[0] << 8);
        MotorVoltage = (uint16_t)Data[3] + ((uint16_t)Data[2] << 8);
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
        FireUp = Data[2] & 0x1;
        PlugIn = Data[3] & 0x1;
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
    if (MotorCanTimeoutState || TransmitionCanTimeoutState || VehicleCanTimeoutState) {
        BatteryErrorState = 1;
    }

    if (BatteryCurrent > BatteryCurrentLimitLimit) {
        BatteryErrorState = 2;
    } else if (BatteryVoltage < BatteryUnderVoltageLimit) {
        BatteryErrorState = 3;
    }

    // Try to reverse calculate the remaining capacity

    double Power = BatteryCurrent * BatteryVoltage;
    if (Power > 0.0) {
        if (BatteryCapacity > 0) {
            // Discharge
            BatteryCapacity -= Power * SampleTime / 3600.0;
        }
    } else if (Power < 0.0) {
        if (BatteryCapacity < StartBatteryCapacity) {
            // Charge
            BatteryCapacity -= Power * SampleTime / 3600.0;
        }
    }

    if (BatteryErrorState) {
        BatteryVoltage = 0.0;
        BatteryCurrent = 0.0;
    }

    if (!FireUp && PlugIn && (BatteryCapacity < StartBatteryCapacity)) {
        ChargeCurrent = BatteryMaxLoadCurrent;  // constant load current
    } else {
        ChargeCurrent = 0.0;
    }
    // Transmit the Battery control unit message
    memset(Data, 0, sizeof(Data));
    uint16_t BatteryVoltageInt = (uint16_t)BatteryVoltage;
    Data[1] = BatteryVoltageInt;
    Data[0] = BatteryVoltageInt >> 8;
    uint16_t BatteryCurrentInt = (uint16_t)BatteryCurrent;
    Data[3] = BatteryCurrentInt;
    Data[2] = BatteryCurrentInt >> 8;
    uint16_t BatteryCapacityInt = (uint16_t) BatteryCapacity;
    Data[5] = BatteryCapacityInt;
    Data[4] = BatteryCapacityInt >> 8;
    Data[6] = BatteryErrorState;
    sc_write_virt_can_msg_buff(0, 0, Data);   // write to CAN
}

// This will be called if the process will be terminate
void terminate_test_object (void)
{
}

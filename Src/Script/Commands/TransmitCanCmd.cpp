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


#include "Platform.h"
#include <stdio.h>

extern "C" {
#include "MyMemory.h"

#include "CanFifo.h"
#include "InterfaceToScript.h"

}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cTransmitCanCmd)


int cTransmitCanCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}


// Script need it's own message queue otherwise we would not see the messages inside the CAN message window!
// This will be also used inside the RPC-API
extern "C" int CheckCANMessageQueueScript (void)
{
    static int CanFiFoHandle;

    if (CanFiFoHandle <= 0) {
        CanFiFoHandle = CreateCanFifos (100, 0);
    }
    return CanFiFoHandle;
}

extern "C" int CheckCANFDMessageQueueScript (void)
{
    static int CanFiFoHandle;

    if (CanFiFoHandle <= 0) {
        CanFiFoHandle = CreateCanFifos (100, 1);
    }
    return CanFiFoHandle;
}

int cTransmitCanCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    CAN_FIFO_ELEM CANMessage;
    int Offset, x;
    int Help;
    int CanFiFoHandle;

    MEMSET (&CANMessage, 0, sizeof (CANMessage));
    CANMessage.node = 1;  // transmitted oneself
    if (par_Parser->SolveEquationForParameter (0, &Help, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (0));
        return -1;
    } 
    CANMessage.channel = static_cast<unsigned char>(Help);                               // 1. parameter is channel
    if (CANMessage.channel > 7) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "CAN Channel %i out of range (0...7)", static_cast<int>(CANMessage.channel));
        return -1;
    }
    if (par_Parser->SolveEquationForParameter (1, &Help, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (0));
        return -1;
    } 
    CANMessage.id = static_cast<uint32_t>(Help);
    if (!stricmp (par_Parser->GetParameter (2), "ext")) {                        // 3. paramter can be "Ext"
        CANMessage.ext = 1;
        Offset = 1;
    } else {
        CANMessage.ext = 0;
        Offset = 0;
    }
    if (par_Parser->SolveEquationForParameter (2 + Offset, &Help, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                           par_Parser->GetParameter (0));
        return -1;
    } 
    CANMessage.size = static_cast<unsigned char>(Help);  // 3. or 4. parameter is size
    if (CANMessage.size > 8) CANMessage.size = 8;
    for (x = 0; x < CANMessage.size; x++) {
        if (par_Parser->SolveEquationForParameter (3 + Offset + x, &Help, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                               par_Parser->GetParameter (0));
            return -1;
        } 
        CANMessage.data[x] = static_cast<uint8_t>(Help);
    }
    if (par_Parser->GetParameterCounter () != CANMessage.size + Offset + 3) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "TRANSMIT_CAN not possible! wrong parameter count");
        return -1;
    }

    // Check if CAN message queue exists
    CanFiFoHandle = CheckCANMessageQueueScript ();

    if (WriteCanMessageFromProcess2Fifo (CanFiFoHandle, &CANMessage)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "TRANSMIT_CAN not possible! CAN-channel not open");
        return -1;
    }
    return 0;
}

int cTransmitCanCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cTransmitCanCmd TransmitCanCmd ("TRANSMIT_CAN",                        
                                         2, 
                                         12,  
                                         nullptr,
                                         FALSE, 
                                         FALSE, 
                                         0,
                                         0,
                                         CMD_INSIDE_ATOMIC_ALLOWED);


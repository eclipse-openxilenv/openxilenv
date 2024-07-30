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


#ifndef XILENVEXTPROCCAN_H
#define XILENVEXTPROCCAN_H

#define XILENV_LIBRARY_INTERFACE_TYPE   0
#define XILENV_DLL_INTERFACE_TYPE       1
#define XILENV_STUB_INTERFACE_TYPE      2
/* if no interface type are selected use the library (mybe better set to XILENV_DLL_INTERFACE_TYPE) */
#ifndef XILENV_INTERFACE_TYPE
#define XILENV_INTERFACE_TYPE  XILENV_LIBRARY_INTERFACE_TYPE
#endif

#ifndef WSC_TYPE_INT32
#ifdef _WIN32
#define WSC_TYPE_INT32   long
#define WSC_TYPE_UINT32  unsigned long
#else
#define WSC_TYPE_INT32   int
#define WSC_TYPE_UINT32  unsigned int
#endif
#endif

#ifndef __FUNC_CALL_CONVETION__
#ifdef _WIN32
    #ifdef EXTPROC_DLL_EXPORTS
        #define __FUNC_CALL_CONVETION__  __stdcall
        #define EXPORT_OR_IMPORT __declspec(dllexport)
    #else
        #if defined USE_DLL
            #define __FUNC_CALL_CONVETION__ __stdcall
            #define EXPORT_OR_IMPORT __declspec(dllimport)
        #else
            #define __FUNC_CALL_CONVETION__
            #define EXPORT_OR_IMPORT extern
        #endif
    #endif
#else
    #define __FUNC_CALL_CONVETION__
    #define EXPORT_OR_IMPORT extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// This will open a CAN channel all CAN message buffer will be deleted
// channel = 0...15
// return value:
//    0 -> all OK
//    otherwise
//   -1 -> cannot find the virtual CAN controller interface
//   -2 -> invalid parameter
//   -3 -> CAN channel already open
//   -4 -> XilEnv and library have not the correct version
//   -5 -> Out off memory
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_open_virt_can (int channel);

// This will open a CAN channel all CAN message buffer will be deleted
// channel = 0...15
// fd_enable = 1 -> CAN-FD channel, 0 -> CAN
// return value:
//    0 -> all OK
//    otherwise
//   -1 -> cannot find the virtual CAN controller interface
//   -2 -> invalid parameter
//   -3 -> CAN channel already open
//   -4 -> XilEnv and library have not the correct version
//   -5 -> Out off memory
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_open_virt_can_fd (int channel, int fd_enable);

// This will close the CAN channel afterwards no receiving or transmitting will be possible
// channel = 0...15
// return value:
//    0 -> CAN channel was closed successful
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_close_virt_can (int channel);

// the ext parameter bit mask: bit 0 is set for extended id bit 1 is set if for a fd frame format bit 2 is set for bit rate switch
#define CAN_EXT_MASK    1
#define CANFD_BTS_MASK  2
#define CANFD_FDF_MASK  4

/**************** CAN-Fifos ***************************************************/

// This will write a CAN message into the CAN transmitting fifo of the virtual CAN controller
// channel = 0...15
// id = CAN-ID
// ext = 0 standart ID's
//     = 1 extendet ID's
// size = 1...8
// data = Daten der zu sendenden CAN-Message
// return value:
//    0 -> Data was written and there is space for an additional message iside the fifo
//    1 -> Data was written and there is NO space for an additional message iside the fifo
//   -1 -> Data was lost
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_fifo_write_virt_can (int channel, WSC_TYPE_UINT32 id, unsigned char ext, unsigned char size, unsigned char *data);

// This will read a CAN message from the receiving fifo of the virtual CAN controller
// channel = 0...15
// pid = returns the CAN Id
// pext = returns the Id type: =0 standart Id's, =1 extendet Id's
// psize = returns the size of the data 1...8
// data = returns the data of the received CAN message
// return value:
//    0 -> There are no CAN messages inside the receiving fifo of the virtual CAN controller
//    1 -> There was received one CAN message from the fifo
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_fifo_read_virt_can (int channel, WSC_TYPE_UINT32 *pid, unsigned char *pext, unsigned char *psize, unsigned char *data);

// This will return the filling level of the receiving fifo of the virtual CAN controller
// channel = 0...15
// return value:
//    0...n -> number of Objects inside the fifo
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_get_fifo_fill_level (int channel);

/**************** CAN-Message-Buffers******************************************/

// This will configure one of the CAN message object buffers
// channel = 0...15
// buffer_idx = 0...n
// id = CAN-Id
// ext = 0 standart Id's
//     = 1 extendet Id's
// size = 1...8
// dir = this will define if it receiving buffer (dir=0) or a transmitting buffer (dir=1)
// return value:
//    0 -> CAN message object buffer successful configured
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_cfg_virt_can_msg_buff (int channel, int buffer_idx, WSC_TYPE_UINT32 id, unsigned char ext, unsigned char size, unsigned char dir);

// This will read a CAN message from the CAN message object-buffer of the virtual CAN controller
// channel = 0...15
// buffer_idx = 0...n
// pext = Returns the Id typs: =0 standard ID's, =1 extendet ID's
//        this can be NULL if not needed
// psize = Returns the message size 1...8
//        this can be NULL if not needed
// data = Return buffer for CAN message data
//    0 -> gelesene Daten sind alt
//    1 -> es wurden neue Daten gelesen
//   -1 -> ungueltige Parameter
//   -3 -> CAN ist noch nicht geoeffnet
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_read_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *pext, unsigned char *psize, unsigned char *data);

// This will read a CAN message from the CAN message object-buffer of the virtual CAN controller
// channel = 0...15
// buffer_idx = 0...n
// return value:
//    0 -> The data are old
//    1 -> The data are new
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_peek_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *pext, unsigned char *psize, unsigned char *data);

// This will write a CAN message into a CAN message object buffer of the virtual CAN controller
// channel = 0...15
// buffer_idx = 0...n
// data = Data that should be written
// Return value:
//    0 -> Data was successful written into the buffer
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_write_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *data);

#ifdef __cplusplus
}
#endif

#endif

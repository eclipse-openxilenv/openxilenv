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


#ifndef __SCRPCDLL_H
#define __SCRPCDLL_H

#include <stdint.h>

#ifdef SCRPCDLL_EXPORTS
#ifdef _WIN32
#define SCRPCDLL_API __declspec(dllexport)
#else
#define SCRPCDLL_API __attribute__((visibility("default")))
#endif
#else
//#define SCRPCDLL_API __declspec(dllimport)
#define SCRPCDLL_API 
#endif

#ifdef _WIN32 
#ifdef __NOT_USE_STDCALL__
#define __STDCALL__
#else 
#define __STDCALL__  __stdcall
#endif
#else
#define __STDCALL__
#endif


// If this header will include from a c++ file
#ifdef __cplusplus
#define CFUNC extern "C"
#else
#define CFUNC
#endif


// Connect
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ConnectTo(const char* NetAddr);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ConnectToInstance(const char* NetAddr, const char *InstanceName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DisconnectFrom(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DisconnectAndClose(int SetErrorLevelFlag, int ErrorLevel);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_IsConnectedTo(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVersion(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetAPIVersion(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetAPIAlternativeVersion(void);

CFUNC SCRPCDLL_API char * __STDCALL__ XilEnv_GetAPIModulePath(void);

// Other
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CreateFileWithContent (const char *Filename,
                                                             const char *Content);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CopyFileToLocal (const char *SourceFilename,
                                                          const char *DestinationFilename);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CopyFileFromLocal (const char *SourceFilename,
                                                            const char *DestinationFilename);
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetEnvironVar (const char *EnvironVar);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetEnvironVar (const char *EnvironVar, const char *EnvironValue);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ChangeSettings (const char *SettingName, const char *ValueString);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TextOut (const char *TextOut);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ErrorTextOut (int ErrLevel, const char *TextOut);

// Scheduler
CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_StopScheduler(void);
CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_ContinueScheduler(void);
CFUNC SCRPCDLL_API int  __STDCALL__ XilEnv_IsSchedulerRunning(void);
CFUNC SCRPCDLL_API int  __STDCALL__ XilEnv_StartProcess(const char* name);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartProcessAndLoadSvl(const char* ProcessName, const char* SvlName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartProcessEx(const char* ProcessName,
                                                     int Prio, 
                                                     int Cycle, 
                                                     short Delay, 
                                                     int Timeout, 
                                                     const char *SVLFile, 
                                                     const char *BBPrefix,
                                                     int UseRangeControl,  // If 0, than ignore all following "RangeControl*" parameter
                                                     int RangeControlBeforeActiveFlags,
                                                     int RangeControlBehindActiveFlags,
                                                     int RangeControlStopSchedFlag,
                                                     int RangeControlOutput,
                                                     int RangeErrorCounterFlag,
                                                     const char *RangeErrorCounter,
                                                     int RangeControlVarFlag,
                                                     const char *RangeControl,
                                                     int RangeControlPhysFlag,
                                                     int RangeControlLimitValues);

// The parameter A2LFlags can have the folowing bit values:
#define A2L_LINK_NO_FLAGS                              0x0
#define A2L_LINK_UPDATE_FLAG                           0x1
#define A2L_LINK_UPDATE_IGNORE_FLAG                    0x2
#define A2L_LINK_UPDATE_ZERO_FLAG                      0x4
#define A2L_LINK_ADDRESS_TRANSLATION_DLL_FLAG          0x8
#define A2L_LINK_ADDRESS_TRANSLATION_MULTI_DLL_FLAG    0x10
#define A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG       0x20

SCRPCDLL_API int __STDCALL__ XilEnv_StartProcessEx2(const char* ProcessName,
                                                int Prio, 
                                                int Cycle, 
                                                short Delay, 
                                                int Timeout, 
                                                const char *SVLFile, 
                                                const char *A2LFile, 
                                                int A2LFlags,
                                                const char *BBPrefix,
                                                int UseRangeControl,  // If 0, than ignore all following "RangeControl*" parameter
                                                int RangeControlBeforeActiveFlags,
                                                int RangeControlBehindActiveFlags,
                                                int RangeControlStopSchedFlag,
                                                int RangeControlOutput,
                                                int RangeErrorCounterFlag,
                                                const char *RangeErrorCounter,
                                                int RangeControlVarFlag,
                                                const char *RangeControl,
                                                int RangeControlPhysFlag,
                                                int RangeControlLimitValues);

CFUNC SCRPCDLL_API int  __STDCALL__ XilEnv_StopProcess(const char* name);
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetNextProcess (int flag, char* filter);
CFUNC SCRPCDLL_API int  __STDCALL__ XilEnv_GetProcessState(const char* name);
CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_DoNextCycles (int Cycles);
CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_DoNextCyclesAndWait (int Cycles);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddBeforeProcessEquationFromFile(int Nr, const char *ProcName, const char *EquFile);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddBehindProcessEquationFromFile(int Nr, const char *ProcName, const char *EquFile);
CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_DelBeforeProcessEquations(int Nr, const char *ProcName);
CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_DelBehindProcessEquations(int Nr, const char *ProcName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WaitUntil (const char *Equation, int Cycles);

// Internal Processes
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartScript(const char* scrfile);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopScript(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartRecorder(const char* cfgfile);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopRecorder(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_RecorderAddComment(const char* Comment);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartPlayer(const char* cfgfile);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopPlayer(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartEquations(const char* equfile);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopEquations(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartGenerator(const char* genfile);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopGenerator(void);

// GUI
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadDesktop(const char* file);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SaveDesktop(const char* file);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ClearDesktop(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CreateDialog(const char* DialogName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddDialogItem(const char* Description, const char* VariName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ShowDialog(void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_IsDialogClosed(void);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SelectSheet (const char* SheetName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddSheet (const char* SheetName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DeleteSheet (const char* SheetName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_RenameSheet (const char* OldSheetName, const char* NewSheetName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_OpenWindow (const char* WindowName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CloseWindow (const char* WindowName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DeleteWindow (const char* WindowName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ImportWindow (const char* WindowName, const char* FileName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ExportWindow (const char* SheetName, const char* WindowName, const char* FileName);

// Blackboard
#if !defined SCRPCDLL_EXPORTS && !defined SC2PY_EXPORTS
enum BB_DATA_TYPES {BB_BYTE, BB_UBYTE, BB_WORD, BB_UWORD, BB_DWORD,
                    BB_UDWORD, BB_FLOAT, BB_DOUBLE, BB_UNKNOWN,
                    BB_UNKNOWN_DOUBLE, BB_UNKNOWN_WAIT, BB_QWORD=34, BB_UQWORD=35};
union BB_VARI {
    int8_t b;               // BB_BYTE
    uint8_t ub;             // BB_UBYTE
    int16_t w;              // BB_WORD
    uint16_t uw;            // BB_UWORD
    int32_t dw;             // BB_DWORD
    uint32_t udw;           // BB_UDWORD
    int64_t qw;             // BB_QWORD
    uint64_t uqw;           // BB_UQWORD
    float f;                // BB_FLOAT
    double d;               // BB_DOUBLE
};
#endif

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddVari(const char* label, int type, const char* unit);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_RemoveVari(int vid);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AttachVari(const char* label);
CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_Get(int vid);
CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetPhys(int vid);
CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_Set(int vid, double value);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetPhys(int vid, double value);
CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_Equ(const char* equ);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WrVariEnable(const char* label, const char* process);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WrVariDisable(const char* label, const char* process);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_IsWrVariEnabled(const char* label, const char* process);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadRefList(const char* reflist, const char* process);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AddRefList(const char* reflist, const char* process);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SaveRefList(const char* reflist, const char* process);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariConversionType(int vid);
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetVariConversionString(int vid);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariConversion(int vid, int type, const char* conv_string);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariType(int vid);
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetVariUnit(int vid);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariUnit(int vid, char *unit);
CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetVariMin(int vid);
CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetVariMax(int vid);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariMin(int vid, double min);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariMax(int vid, double max);
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetNextVari (int flag, char* filter);
CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetNextVariEx (int flag, char* filter, char *process, int AccessFlags);
#define XILENV_ACCESS_FLAG_ONLY_ENABLED  (0x1 << 0)
#define XILENV_ACCESS_FLAG_ONLY_DISABLED (0x1 << 1)
#define XILENV_ACCESS_FLAG_ENABLED_OR_DISABLED (0x3 << 0)

CFUNC SCRPCDLL_API char* __STDCALL__ XilEnv_GetVariEnum (int vid, double value);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariDisplayFormatWidth (int vid);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetVariDisplayFormatPrec (int vid);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetVariDisplayFormat (int vid, int width, int prec);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ImportVariProperties (const char* Filename);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_EnableRangeControl (const char* ProcessNameFilter, const char* VariableNameFilter);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DisableRangeControl (const char* ProcessNameFilter, const char* VariableNameFilter);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WriteFrame (int *Vids, double *ValueFrame, int Size);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetFrame (int *Vids, double *RetValueFrame, int Size);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_WriteFrameWaitReadFrame (int *WriteVids, double *WriteValues, int WriteSize,
                                                               int *ReadVids, double *ReadValuesRet, int ReadSize); 

#ifndef COLOR_UNDEFINED
#define COLOR_UNDEFINED        0xFFFFFFFFUL
// no conversion 1:1
#define CONVTYPE_NOTHING         0
// a equation as conversion for example "10.0*#+1000.0"
#define CONVTYPE_EQUATION        1
// an enum conversion for example "0 0 \"off\"; 1 1 \"on\";"
#define CONVTYPE_TEXTREPLACE     2
// do not change the conversion
#define CONVTYPE_UNDEFINED     255
// do not change the display format
#define WIDTH_UNDEFINED        255
#define PREC_UNDEFINED         255
#endif

// BB -> extern process
#define REFERENCE_SYMBOL_BB2EP_FLAG       0x0001
// extern process -> BB
#define REFERENCE_SYMBOL_EP2BB_FLAG       0x0002
// BB <-> extern process
#define REFERENCE_SYMBOL_READWRITE_FLAG   0x0003
#define REFERENCE_SYMBOL_ADD_TO_LIST      0x0004
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ReferenceSymbol(const char* Symbol, const char *DisplayName, const char* Process, const char *Unit, int ConversionType, const char *Conversion,
                                                      double Min, double Max, int Color, int Width, int Precision, int Flags);
#define DEREFERENCE_SYMBOL_REMOVE_FROM_LIST_FLAG  0x0004
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DereferenceSymbol(const char* Symbol, const char* Process, int Flags);

CFUNC SCRPCDLL_API enum BB_DATA_TYPES  __STDCALL__ XilEnv_GetRaw(int vid, union BB_VARI *ret_Value);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetRaw(int vid, enum BB_DATA_TYPES Type, union BB_VARI Value, int Flags);

// Calibration
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadSvl(const char* svlfile, const char* process);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SaveSvl(const char* svlfile, const char* process, const char* filter);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SaveSal(const char* salfile, const char* process, const char* filter);

CFUNC SCRPCDLL_API enum BB_DATA_TYPES __STDCALL__ XilEnv_GetSymbolRaw(const char* Symbol, const char* Process, int Flags, union BB_VARI *ret_Value);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetSymbolRaw(const char* Symbol, const char* Process, int Flags, enum BB_DATA_TYPES DataType, union BB_VARI Value);

// CAN
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanChannelCount (int ChannelCount);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadCanVariante(const char* canfile, int channel);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadAndSelCanVariante(const char* canfile, int channel);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_AppendCanVariante(const char* canfile, int channel);
CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_DelAllCanVariants(void);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadCCPConfig(int Connection, const char* ccpfile);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPBegin(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPAddVar(int Connection, const char* label);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPEnd(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopCCP(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPCalBegin(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPCalAddVar(int Connection, const char* label);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCCPCalEnd(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopCCPCal(int Connection);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_LoadXCPConfig(int Connection, const char* xcpfile);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPBegin(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPAddVar(int Connection, const char* label);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPEnd(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopXCP(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPCalBegin(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPCalAddVar(int Connection, const char* label);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartXCPCalEnd(int Connection);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopXCPCal(int Connection);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TransmitCAN (int channel, int id, int ext, int size, unsigned char data0, unsigned char data1,
                                                       unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5,
                                                       unsigned char data6, unsigned char data7);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TransmitCANFd (int channel, int id, int ext, int size, unsigned char *data);

// CAN message queues
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_OpenCANQueue (int Depth);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_OpenCANFdQueue (int Depth, int FdFlag);

#if !defined SCRPCDLL_EXPORTS && !defined SC2PY_EXPORTS
/* keine Luecken in dieser Struktur */
#pragma pack(push,1)
typedef struct CAN_ACCEPTANCE_WINDOWS {
    int Channel;   // Kanalnummer 0...3
    int StartId;   // Beginn des ID-Fensters
    int EndId;     // Ende des ID-Fensters (Wenn nur eine ID gewuenscht EndID = StartID)
    int Fill1;     // Fuellbytes
} CAN_ACCEPT_MASK;
#pragma pack(pop)
#endif

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCANAcceptanceWindows (int Size, CAN_ACCEPT_MASK* pWindows);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_FlushCANQueue (int Flags);
#define FLUSH_RX_CAN_QUEUE   0x00000001
#define FLUSH_TX_CAN_QUEUE   0x00000002

#if !defined SCRPCDLL_EXPORTS && !defined SC2PY_EXPORTS
/* keine Luecken in dieser Struktur */
#pragma pack(push,1)
typedef struct CAN_MESSAGE_QUEUE_ELEM {
    uint32_t id;             // 4Byte     Message ID
    unsigned char size;           // 1Byte     Laenge der Message (1...8)
    unsigned char ext;            // 1Byte     1 -> 29Bit-ID, 0 -> 11Bit-ID
    unsigned char flag;           // 1Byte     sollte 0 sein (interne Verwendung)
    unsigned char channel;        // 1Byte     CAN-Kanal (0...7)
    unsigned char node;           // 1Byte     1 -> wurde von XilEnv gesendet 0 -> kommt von extern (nur RX)
    unsigned char fill[7];        // 7Byte     N.C.
    uint64_t timestamp;   // 8Byte     N.C.
    unsigned char data[8];        // 8Byte     Daten-Bytes der Message
} CAN_FIFO_ELEM;                                // 32Bytes
typedef struct CAN_FD_MESSAGE_QUEUE_ELEM {
    uint32_t id;             // 4Byte     Message ID
    unsigned char size;           // 1Byte     Laenge der Message (1...64)
    unsigned char ext;            // 1Byte     1 -> 29Bit-ID, 0 -> 11Bit-ID
    unsigned char flag;           // 1Byte     sollte 0 sein (interne Verwendung)
    unsigned char channel;        // 1Byte     CAN-Kanal (0...7)
    unsigned char node;           // 1Byte     1 -> wurde von XilEnv gesendet 0 -> kommt von extern (nur RX)
    unsigned char fill[7];        // 7Byte     N.C.
    uint64_t timestamp;   // 8Byte     N.C.
    unsigned char data[64];       // 8Byte     Daten-Bytes der Message
} CAN_FD_FIFO_ELEM;                             // 88Bytes
#pragma pack(pop)
#endif

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ReadCANQueue (int ReadMaxElements,
                                                        CAN_FIFO_ELEM *pElements);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ReadCANFdQueue (int ReadMaxElements,
                                                          CAN_FD_FIFO_ELEM* pElements);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TransmitCANQueue (int WriteElements,
                                                            CAN_FIFO_ELEM *pElements);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_TransmitCANFdQueue (int WriteElements,
                                                              CAN_FD_FIFO_ELEM *pElements);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_CloseCANQueue (void);

// CAN bit error
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanErr (int Channel, int Id, int Startbit, int Bitsize, char *Byteorder, uint32_t Cycles, uint64_t BitErrValue);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanErrSignalName (int Channel, int Id, char *Signalname, uint32_t Cycles, uint64_t BitErrValue);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ClearCanErr (void);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetCanSignalConversion (int Channel, int Id, const char *Signalname, const char *Conversion);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ResetCanSignalConversion (int Channel, int Id, const char *Signalname);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ResetAllCanSignalConversion (int Channel, int Id);

// CAN recorder
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StartCANRecorder (const char *par_FileName,
                                                        const char *par_TriggerEqu,
                                                        int par_DisplayColumnCounterFlag,
                                                        int par_DisplayColumnTimeAbsoluteFlag,
                                                        int par_DisplayColumnTimeDiffFlag,
                                                        int par_DisplayColumnTimeDiffMinMaxFlag, 
                                                        int Elements, CAN_ACCEPT_MASK* pWindows);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_StopCANRecorder (void);


// A2LLink
typedef struct {
    void *Data;
} XILENV_LINK_DATA;

#ifndef DO_NOT_DEFINE_A2L_LINK_TYPES
// This are the possible vlues of the parameter TypeMask
#define A2L_LABEL_TYPE_ALL                        0xFFFF
#define A2L_LABEL_TYPE_MEASUREMENT                  0xFF
#define A2L_LABEL_TYPE_SINGEL_VALUE_MEASUREMENT      0x1
#define A2L_LABEL_TYPE_1_DIM_ARRAY_MEASUREMENT       0x2
#define A2L_LABEL_TYPE_2_DIM_ARRAY_MEASUREMENT       0x4
#define A2L_LABEL_TYPE_3_DIM_ARRAY_MEASUREMENT       0x8
#define A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT   0x10
#define A2L_LABEL_TYPE_REFERENCED_MEASUREMENT       0x20

#define A2L_LABEL_TYPE_CALIBRATION                0xFF00
#define A2L_LABEL_TYPE_SINGLE_VALUE_CALIBRATION    0x100
#define A2L_LABEL_TYPE_ASCII_CALIBRATION           0x200
#define A2L_LABEL_TYPE_VAL_BLK_CALIBRATION         0x400
#define A2L_LABEL_TYPE_CURVE_CALIBRATION           0x800
#define A2L_LABEL_TYPE_MAP_CALIBRATION            0x1000
#define A2L_LABEL_TYPE_CUBOID_CALIBRATION         0x2000
#define A2L_LABEL_TYPE_CUBE_4_CALIBRATION         0x4000
#define A2L_LABEL_TYPE_CUBE_5_CALIBRATION         0x8000

// this is the possible return value of the xxxxFlags() functions
#define A2L_VALUE_FLAG_CALIBRATION               0x1
#define A2L_VALUE_FLAG_MEASUREMENT               0x2
#define A2L_VALUE_FLAG_PHYS                      0x4  
#define A2L_VALUE_FLAG_READ_ONLY                 0x8
#define A2L_VALUE_FLAG_ONLY_VIRTUAL             0x10
#define A2L_VALUE_FLAG_UPDATE                 0x1000

// this is the possible value of PhysFlag parameter
#define A2L_GET_PHYS_FLAG            0x1
#define A2L_GET_TEXT_REPLACE_FLAG    0x2
#define A2L_GET_UNIT_FLAG            0x4

enum A2L_DATA_TYPE { A2L_DATA_TYPE_MEASUREMENT = 0, A2L_DATA_TYPE_VALUE = 1, A2L_DATA_TYPE_ASCII = 2,
                     A2L_DATA_TYPE_VAL_BLK = 3, A2L_DATA_TYPE_CURVE = 4, A2L_DATA_TYPE_MAP = 5, 
                     A2L_DATA_TYPE_CUBOID = 6, A2L_DATA_TYPE_CUBE_4 = 7, A2L_DATA_TYPE_CUBE_5 = 8, 
                     A2L_DATA_TYPE_ERROR = -1 };

enum A2L_ELEM_TYPE { A2L_ELEM_TYPE_INT = 0, A2L_ELEM_TYPE_UINT = 1, A2L_ELEM_TYPE_DOUBLE = 2, 
                     A2L_ELEM_TYPE_PHYS_DOUBLE = 3, A2L_ELEM_TYPE_TEXT_REPLACE = 4, A2L_ELEM_TYPE_ERROR = -1 };

// Are the same as blackboard data type numbers.
enum A2L_ELEM_TARGET_TYPE { A2L_ELEM_TARGET_TYPE_INT8 = 0, A2L_ELEM_TARGET_TYPE_UINT8 = 1,
                            A2L_ELEM_TARGET_TYPE_INT16 = 2, A2L_ELEM_TARGET_TYPE_UINT16 = 3,
                            A2L_ELEM_TARGET_TYPE_INT32 = 4, A2L_ELEM_TARGET_TYPE_UINT32 = 5,
                            A2L_ELEM_TARGET_TYPE_FLOAT32_IEEE = 6, A2L_ELEM_TARGET_TYPE_FLOAT64_IEEE = 7,
                            A2L_ELEM_TARGET_TYPE_INT64 = 34, A2L_ELEM_TARGET_TYPE_UINT64 = 35,
                            A2L_ELEM_TARGET_TYPE_FLOAT16_IEEE = 36,
                            A2L_ELEM_TARGET_TYPE_NO_TYPE = 100, A2L_ELEM_TARGET_TYPE_ERROR = -1 };
#endif

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetupLinkToExternProcess(const char *A2LFileName, const char *ProcessName, int UpdateFlag);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkToExternProcess(const char *ProcessName);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetIndexFromLink(int LinkNr, const char *Label, int TypeMask);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetNextSymbolFromLink(int LinkNr, int Index, int TypeMask, const char *Filter, char *ret_Label, int MaxChar);
CFUNC SCRPCDLL_API XILENV_LINK_DATA* __STDCALL__ XilEnv_GetDataFromLink(int LinkNr, int Index, XILENV_LINK_DATA *Reuse, int PhysFlag, const char **ret_Error);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetDataToLink(int LinkNr, int Index, XILENV_LINK_DATA *Data, const char **ret_Error);

// BB -> extern process
#define LINK_REFERENCE_SYMBOL_BB2EP_FLAG       0x0001
// extern process -> BB
#define LINK_REFERENCE_SYMBOL_EP2BB_FLAG       0x0002
// BB <-> extern process
#define LINK_REFERENCE_SYMBOL_READWRITE_FLAG   0x0003
// default as defined inside the A2L file
#define LINK_REFERENCE_SYMBOL_DEFAULT_FLAG     0x0000
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_ReferenceMeasurementToBlackboard(int LinkNr, int Index, int DirFlags);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_DereferenceMeasurementFromBlackboard(int LinkNr, int Index);

// A2LLinks helper
CFUNC SCRPCDLL_API enum A2L_DATA_TYPE __STDCALL__ XilEnv_GetLinkDataType(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkDataArrayCount(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkDataArraySize(XILENV_LINK_DATA *Data, int ArrayNo);
CFUNC SCRPCDLL_API XILENV_LINK_DATA* __STDCALL__ XilEnv_CopyLinkData(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API XILENV_LINK_DATA* __STDCALL__ XilEnv_FreeLinkData(XILENV_LINK_DATA *Data);

// Single values
CFUNC SCRPCDLL_API enum A2L_ELEM_TYPE __STDCALL__ XilEnv_GetLinkSingleValueDataType(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API enum A2L_ELEM_TARGET_TYPE __STDCALL__ XilEnv_GetLinkSingleValueTargetDataType(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API uint32_t __STDCALL__ XilEnv_GetLinkSingleValueFlags(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API uint64_t __STDCALL__ XilEnv_GetLinkSingleValueAddress(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueDimensionCount(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueDimension(XILENV_LINK_DATA *Data, int DimNo);
CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetLinkSingleValueDataDouble(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkSingleValueDataDouble(XILENV_LINK_DATA *Data, double Value);
CFUNC SCRPCDLL_API int64_t __STDCALL__ XilEnv_GetLinkSingleValueDataInt(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkSingleValueDataInt(XILENV_LINK_DATA *Data, int64_t Value);
CFUNC SCRPCDLL_API uint64_t __STDCALL__ XilEnv_GetLinkSingleValueDataUint(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkSingleValueDataUint(XILENV_LINK_DATA *Data, uint64_t Value);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueDataString(XILENV_LINK_DATA *Data, char *ret_Value, int MaxLen);
CFUNC SCRPCDLL_API const char* __STDCALL__ XilEnv_GetLinkSingleValueDataStringPtr(XILENV_LINK_DATA *Data);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkSingleValueDataString(XILENV_LINK_DATA *Data, const char *Value);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueUnit(XILENV_LINK_DATA *Data, char *ret_Value, int MaxLen);
CFUNC SCRPCDLL_API const char* __STDCALL__ XilEnv_GetLinkSingleValueUnitPtr(XILENV_LINK_DATA *Data);

// Array of values
CFUNC SCRPCDLL_API enum A2L_ELEM_TYPE __STDCALL__ XilEnv_GetLinkArrayValueDataType(XILENV_LINK_DATA *Data, int ArrayNo, int Number);
CFUNC SCRPCDLL_API enum A2L_ELEM_TARGET_TYPE __STDCALL__ XilEnv_GetLinkArrayValueTargetDataType(XILENV_LINK_DATA *Data, int ArrayNo, int Number);
CFUNC SCRPCDLL_API uint32_t __STDCALL__ XilEnv_GetLinkArrayValueFlags(XILENV_LINK_DATA *Data, int ArrayNo, int Number);
CFUNC SCRPCDLL_API uint64_t __STDCALL__ XilEnv_GetLinkArrayValueAddress(XILENV_LINK_DATA *Data, int ArrayNo, int Number);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueDimensionCount(XILENV_LINK_DATA *Data, int ArrayNo);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueDimension(XILENV_LINK_DATA *Data, int ArrayNo, int DimNo);
CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetLinkArrayValueDataDouble(XILENV_LINK_DATA *Data, int ArrayNo, int Number);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkArrayValueDataDouble(XILENV_LINK_DATA *Data, int ArrayNo, int Number, double Value);
CFUNC SCRPCDLL_API int64_t __STDCALL__ XilEnv_GetLinkArrayValueDataInt(XILENV_LINK_DATA *Data, int ArrayNo, int Number);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkArrayValueDataInt(XILENV_LINK_DATA *Data, int ArrayNo, int Number, int64_t Value);
CFUNC SCRPCDLL_API uint64_t __STDCALL__ XilEnv_GetLinkArrayValueDataUint(XILENV_LINK_DATA *Data, int ArrayNo, int Number);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkArrayValueDataUint(XILENV_LINK_DATA *Data, int ArrayNo, int Number, uint64_t Value);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueDataString(XILENV_LINK_DATA *Data, int ArrayNo, int Number, char *ret_Value, int MaxLen);
CFUNC SCRPCDLL_API const char* __STDCALL__ XilEnv_GetLinkArrayValueDataStringPtr(XILENV_LINK_DATA *Data, int ArrayNo, int Number);
CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkArrayValueDataString(XILENV_LINK_DATA *Data, int ArrayNo, int Number, const char *Value);

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueUnit(XILENV_LINK_DATA *Data, int ArrayNo, int Number, char *ret_Value, int MaxLen);
CFUNC SCRPCDLL_API const char* __STDCALL__ XilEnv_GetLinkArrayValueUnitPtr(XILENV_LINK_DATA *Data, int ArrayNo, int Number);

CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_PrintLinkData(XILENV_LINK_DATA *par_Data);

#endif

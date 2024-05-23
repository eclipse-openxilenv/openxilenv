#ifndef FMU3STRUCT_H
#define FMU3STRUCT_H

#include "fmi3Functions.h"

typedef struct {
    int ConversionType;
    char *ConversionString;
    double min;
    double max;
} AdditionalSoftcarInfos;

//    int *Types;  // 0 -> Real, 1 -> Integer, 2 -> Boolean, all other are not supported
#define MODEL_VARIABLES(Type)\
typedef struct { \
    int Count; \
    int Size; \
    char **Names; \
    fmi3ValueReference *References; \
    Type *Values; \
    Type *StartValues; \
    AdditionalSoftcarInfos *SoftcarInfos; \
} Model_##Type##_Variables

MODEL_VARIABLES(fmi3Float32);
MODEL_VARIABLES(fmi3Float64);
MODEL_VARIABLES(fmi3Int8);
MODEL_VARIABLES(fmi3UInt8);
MODEL_VARIABLES(fmi3Int16);
MODEL_VARIABLES(fmi3UInt16);
MODEL_VARIABLES(fmi3Int32);
MODEL_VARIABLES(fmi3UInt32);
MODEL_VARIABLES(fmi3Int64);
MODEL_VARIABLES(fmi3UInt64);
MODEL_VARIABLES(fmi3Boolean);
//MODEL_VARIABLES(fmi3Char);
//MODEL_VARIABLES(fmi3String);
//MODEL_VARIABLES(fmi3Byte);
//MODEL_VARIABLES(fmi3Binary);
//MODEL_VARIABLES(fmi3Clock);
MODEL_VARIABLES(void);

typedef struct {
   Model_fmi3Float32_Variables Float32;
   Model_fmi3Float64_Variables Float64;
   Model_fmi3Int8_Variables Int8;
   Model_fmi3UInt8_Variables UInt8;
   Model_fmi3Int16_Variables Int16;
   Model_fmi3UInt16_Variables UInt16;
   Model_fmi3Int32_Variables Int32;
   Model_fmi3UInt32_Variables UInt32;
   Model_fmi3Int64_Variables Int64;
   Model_fmi3UInt64_Variables UInt64;
   Model_fmi3Boolean_Variables Boolean;
} Model_Variables;

typedef struct {
    /* Inquire version numbers of header files */
//   fmi3GetTypesPlatformTYPE* fmi2GetTypesPlatform;
   fmi3GetVersionTYPE*       fmi3GetVersion;
   fmi3SetDebugLoggingTYPE*  fmi3SetDebugLogging;

/* Creation and destruction of FMU instances */
   fmi3InstantiateCoSimulationTYPE*  fmi3InstantiateCoSimulation;
   fmi3FreeInstanceTYPE* fmi3FreeInstance;

/* Enter and exit initialization mode, terminate and reset */
//   fmi3SetupExperimentTYPE*         fmi3SetupExperiment;
   fmi3EnterInitializationModeTYPE* fmi3EnterInitializationMode;
   fmi3ExitInitializationModeTYPE*  fmi3ExitInitializationMode;
   fmi3TerminateTYPE*               fmi3Terminate;
   fmi3ResetTYPE*                   fmi3Reset;

/* Getting and setting variables values */
   fmi3GetFloat32TYPE* fmi3GetFloat32;
   fmi3GetFloat64TYPE* fmi3GetFloat64;
   fmi3GetInt8TYPE*    fmi3GetInt8;
   fmi3GetUInt8TYPE*   fmi3GetUInt8;
   fmi3GetInt16TYPE*   fmi3GetInt16;
   fmi3GetUInt16TYPE*  fmi3GetUInt16;
   fmi3GetInt32TYPE*   fmi3GetInt32;
   fmi3GetUInt32TYPE*  fmi3GetUInt32;
   fmi3GetInt64TYPE*   fmi3GetInt64;
   fmi3GetUInt64TYPE*  fmi3GetUInt64;
   fmi3GetBooleanTYPE* fmi3GetBoolean;
   fmi3GetStringTYPE*  fmi3GetString;
   fmi3GetBinaryTYPE*  fmi3GetBinary;
   fmi3GetClockTYPE*   fmi3GetClock;

   fmi3SetFloat32TYPE* fmi3SetFloat32;
   fmi3SetFloat64TYPE* fmi3SetFloat64;
   fmi3SetInt8TYPE*    fmi3SetInt8;
   fmi3SetUInt8TYPE*   fmi3SetUInt8;
   fmi3SetInt16TYPE*   fmi3SetInt16;
   fmi3SetUInt16TYPE*  fmi3SetUInt16;
   fmi3SetInt32TYPE*   fmi3SetInt32;
   fmi3SetUInt32TYPE*  fmi3SetUInt32;
   fmi3SetInt64TYPE*   fmi3SetInt64;
   fmi3SetUInt64TYPE*  fmi3SetUInt64;
   fmi3SetBooleanTYPE* fmi3SetBoolean;
   fmi3SetStringTYPE*  fmi3SetString;
   fmi3SetBinaryTYPE*  fmi3SetBinary;
   fmi3SetClockTYPE*   fmi3SetClock;

/***************************************************
Functions for FMI3 for Co-Simulation
****************************************************/
   fmi3DoStepTYPE*     fmi3DoStep;


   fmi3Instance Instance;

   char *ModelIdentifier;
   char *ModelName;
   char *Path;
   int NumberOfEventIndicators;
   char instantiationToken[64];

   char *ProcessName;
   char *ExtractedFmuDirectrory;
   char *ResourceDirectory;   // must be IETF RFC3986 syntax
   char *XmlPath;
   char *DllPath;
   char *DllName;

   char *XcpXmlPath;

   void* DllModuleHandle;

   double TimeStep;
   double SimTime;
   fmi3Status Status;

   bool SyncParametersWithBlackboard;
   bool SyncLocalsWithBlackboard;

   Model_Variables Parameters;

   Model_Variables Inputs;
   Model_Variables Outputs;
   Model_Variables Locals;
   Model_Variables Independent;

} FMU;

#endif // FMU3STRUCT_H

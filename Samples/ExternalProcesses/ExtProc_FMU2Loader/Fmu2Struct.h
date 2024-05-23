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


#ifndef FMUSTRUCT_H
#define FMUSTRUCT_H

#include "fmi2Functions.h"

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
    fmi2ValueReference *References; \
    Type *Values; \
    Type *StartValues; \
    AdditionalSoftcarInfos *SoftcarInfos; \
} Model_##Type##_Variables

MODEL_VARIABLES(fmi2Real);
MODEL_VARIABLES(fmi2Integer);
MODEL_VARIABLES(fmi2Boolean);
MODEL_VARIABLES(void);

typedef struct {
   Model_fmi2Real_Variables Real;
   Model_fmi2Integer_Variables Integer;
   Model_fmi2Boolean_Variables Boolean;
} Model_Variables;

#define __PREFIX__

typedef struct {
    /* Inquire version numbers of header files */
   __PREFIX__ fmi2GetTypesPlatformTYPE* fmi2GetTypesPlatform;
   __PREFIX__ fmi2GetVersionTYPE*       fmi2GetVersion;
   __PREFIX__ fmi2SetDebugLoggingTYPE*  fmi2SetDebugLogging;

/* Creation and destruction of FMU instances */
   __PREFIX__ fmi2InstantiateTYPE*  fmi2Instantiate;
   __PREFIX__ fmi2FreeInstanceTYPE* fmi2FreeInstance;

/* Enter and exit initialization mode, terminate and reset */
   __PREFIX__ fmi2SetupExperimentTYPE*         fmi2SetupExperiment;
   __PREFIX__ fmi2EnterInitializationModeTYPE* fmi2EnterInitializationMode;
   __PREFIX__ fmi2ExitInitializationModeTYPE*  fmi2ExitInitializationMode;
   __PREFIX__ fmi2TerminateTYPE*               fmi2Terminate;
   __PREFIX__ fmi2ResetTYPE*                   fmi2Reset;

/* Getting and setting variables values */
   __PREFIX__ fmi2GetRealTYPE*    fmi2GetReal;
   __PREFIX__ fmi2GetIntegerTYPE* fmi2GetInteger;
   __PREFIX__ fmi2GetBooleanTYPE* fmi2GetBoolean;
   __PREFIX__ fmi2GetStringTYPE*  fmi2GetString;

   __PREFIX__ fmi2SetRealTYPE*    fmi2SetReal;
   __PREFIX__ fmi2SetIntegerTYPE* fmi2SetInteger;
   __PREFIX__ fmi2SetBooleanTYPE* fmi2SetBoolean;
   __PREFIX__ fmi2SetStringTYPE*  fmi2SetString;

/* Getting and setting the internal FMU state */
   __PREFIX__ fmi2GetFMUstateTYPE*            fmi2GetFMUstate;
   __PREFIX__ fmi2SetFMUstateTYPE*            fmi2SetFMUstate;
   __PREFIX__ fmi2FreeFMUstateTYPE*           fmi2FreeFMUstate;
   __PREFIX__ fmi2SerializedFMUstateSizeTYPE* fmi2SerializedFMUstateSize;
   __PREFIX__ fmi2SerializeFMUstateTYPE*      fmi2SerializeFMUstate;
   __PREFIX__ fmi2DeSerializeFMUstateTYPE*    fmi2DeSerializeFMUstate;

/* Getting partial derivatives */
   __PREFIX__ fmi2GetDirectionalDerivativeTYPE* fmi2GetDirectionalDerivative;


/***************************************************
Functions for FMI2 for Model Exchange
****************************************************/

/* Enter and exit the different modes */
   __PREFIX__ fmi2EnterEventModeTYPE*               fmi2EnterEventMode;
   __PREFIX__ fmi2NewDiscreteStatesTYPE*            fmi2NewDiscreteStates;
   __PREFIX__ fmi2EnterContinuousTimeModeTYPE*      fmi2EnterContinuousTimeMode;
   __PREFIX__ fmi2CompletedIntegratorStepTYPE*      fmi2CompletedIntegratorStep;

/* Providing independent variables and re-initialization of caching */
   __PREFIX__ fmi2SetTimeTYPE*             fmi2SetTime;
   __PREFIX__ fmi2SetContinuousStatesTYPE* fmi2SetContinuousStates;

/* Evaluation of the model equations */
   __PREFIX__ fmi2GetDerivativesTYPE*                fmi2GetDerivatives;
   __PREFIX__ fmi2GetEventIndicatorsTYPE*            fmi2GetEventIndicators;
   __PREFIX__ fmi2GetContinuousStatesTYPE*           fmi2GetContinuousStates;
   __PREFIX__ fmi2GetNominalsOfContinuousStatesTYPE* fmi2GetNominalsOfContinuousStates;


/***************************************************
Functions for FMI2 for Co-Simulation
****************************************************/

/* Simulating the slave */
   __PREFIX__ fmi2SetRealInputDerivativesTYPE*  fmi2SetRealInputDerivatives;
   __PREFIX__ fmi2GetRealOutputDerivativesTYPE* fmi2GetRealOutputDerivatives;

   __PREFIX__ fmi2DoStepTYPE*     fmi2DoStep;
   __PREFIX__ fmi2CancelStepTYPE* fmi2CancelStep;

/* Inquire slave status */
   __PREFIX__ fmi2GetStatusTYPE*        fmi2GetStatus;
   __PREFIX__ fmi2GetRealStatusTYPE*    fmi2GetRealStatus;
   __PREFIX__ fmi2GetIntegerStatusTYPE* fmi2GetIntegerStatus;
   __PREFIX__ fmi2GetBooleanStatusTYPE* fmi2GetBooleanStatus;
   __PREFIX__ fmi2GetStringStatusTYPE*  fmi2GetStringStatus;

   fmi2Component Component;

   char *ModelIdentifier;
   char *ModelName;
   char *Path;
   int NumberOfEventIndicators;
   char Guid[64];

   char *ProcessName;
   char *ExtractedFmuDirectrory;
   char *ResourceDirectory;   // must be IETF RFC3986 syntax
   char *XmlPath;
   char *DllPath;
   char *DllName;

   char *XcpXmlPath;

   void *DllModuleHandle;

   double TimeStep;
   double SimTime;
   fmi2Status Status;

   bool SyncParametersWithBlackboard;
   bool SyncLocalsWithBlackboard;

   Model_Variables Parameters;

   Model_Variables Inputs;
   Model_Variables Outputs;
   Model_Variables Locals;
   Model_Variables Independent;

} FMU;

#endif // FMUSTRUCT_H

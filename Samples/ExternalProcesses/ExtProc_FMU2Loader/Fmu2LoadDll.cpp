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


#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#define HMODULE void*
#endif
#include "fmi2Functions.h"
#include "Fmu2Struct.h"
extern "C" {
#include "XilEnvExtProc.h"
}
#include "Fmu2LoadDll.h"

static void *GetFuncAddress(HMODULE par_Handle, const char *par_DllName, const char *par_FunctionName)
{
#ifdef _WIN32
    void* Ret = (void *)GetProcAddress(par_Handle, par_FunctionName);
#else
    void* Ret = (void *)dlsym(par_Handle, par_FunctionName);
#endif
    if (Ret == NULL) {
        ThrowError (1, "Function %s not found inside %s", par_FunctionName, par_DllName);
    }
    return Ret;
}

#define INIT_FUNC_POINTER(Type, Name, MustExist)\
if (((ret_Fmu->Name = (Type*)GetFuncAddress(Module, par_DllName, #Name)) == NULL) && MustExist) return -1;

int FmuLoadDll(FMU *ret_Fmu, const char *par_DllName)
{
#ifdef _WIN32
    HMODULE Module = LoadLibrary(par_DllName);

    if (Module == NULL) {
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      dw,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf,
                      0, NULL );
        ThrowError (1, "could not load %s (%s)\n", par_DllName, lpMsgBuf);
        LocalFree (lpMsgBuf);
        return -1;
    }
#else
    HMODULE Module = dlopen(par_DllName, RTLD_LOCAL | RTLD_LAZY);
    if (Module == NULL) {
        ThrowError(1, "could not load %s", par_DllName);
        return -1;
    }
#endif

    ret_Fmu->DllModuleHandle = Module;

    /* Inquire version numbers of header files */
    INIT_FUNC_POINTER(fmi2GetTypesPlatformTYPE, fmi2GetTypesPlatform, 0);
    INIT_FUNC_POINTER(fmi2GetVersionTYPE,       fmi2GetVersion, 0);
    INIT_FUNC_POINTER(fmi2SetDebugLoggingTYPE,  fmi2SetDebugLogging, 0);

    /* Creation and destruction of FMU instances */
    INIT_FUNC_POINTER(fmi2InstantiateTYPE,  fmi2Instantiate, 1);
    INIT_FUNC_POINTER(fmi2FreeInstanceTYPE, fmi2FreeInstance, 1);

    /* Enter and exit initialization mode, terminate and reset */
    INIT_FUNC_POINTER(fmi2SetupExperimentTYPE,         fmi2SetupExperiment, 1);
    INIT_FUNC_POINTER(fmi2EnterInitializationModeTYPE, fmi2EnterInitializationMode, 1);
    INIT_FUNC_POINTER(fmi2ExitInitializationModeTYPE,  fmi2ExitInitializationMode, 1);
    INIT_FUNC_POINTER(fmi2TerminateTYPE,               fmi2Terminate, 1);
    INIT_FUNC_POINTER(fmi2ResetTYPE,                   fmi2Reset, 1);

    /* Getting and setting variables values */
    INIT_FUNC_POINTER(fmi2GetRealTYPE,    fmi2GetReal, 1);
    INIT_FUNC_POINTER(fmi2GetIntegerTYPE, fmi2GetInteger, 1);
    INIT_FUNC_POINTER(fmi2GetBooleanTYPE, fmi2GetBoolean, 1);
    INIT_FUNC_POINTER(fmi2GetStringTYPE,  fmi2GetString, 1);

    INIT_FUNC_POINTER(fmi2SetRealTYPE,    fmi2SetReal, 1);
    INIT_FUNC_POINTER(fmi2SetIntegerTYPE, fmi2SetInteger, 1);
    INIT_FUNC_POINTER(fmi2SetBooleanTYPE, fmi2SetBoolean, 1);
    INIT_FUNC_POINTER(fmi2SetStringTYPE,  fmi2SetString, 1);

    /* Getting and setting the internal FMU state */
    INIT_FUNC_POINTER(fmi2GetFMUstateTYPE,            fmi2GetFMUstate, 0);
    INIT_FUNC_POINTER(fmi2SetFMUstateTYPE,            fmi2SetFMUstate, 0);
    INIT_FUNC_POINTER(fmi2FreeFMUstateTYPE,           fmi2FreeFMUstate, 0);
    INIT_FUNC_POINTER(fmi2SerializedFMUstateSizeTYPE, fmi2SerializedFMUstateSize, 0);
    INIT_FUNC_POINTER(fmi2SerializeFMUstateTYPE,      fmi2SerializeFMUstate, 0);
    INIT_FUNC_POINTER(fmi2DeSerializeFMUstateTYPE,    fmi2DeSerializeFMUstate, 0);

    /* Getting partial derivatives */
    INIT_FUNC_POINTER(fmi2GetDirectionalDerivativeTYPE, fmi2GetDirectionalDerivative, 0);


/***************************************************
Functions for FMI2 for Model Exchange
****************************************************/
#if 0
/* Enter and exit the different modes */
    INIT_FUNC_POINTER(fmi2EnterEventModeTYPE,               fmi2EnterEventMode);
    INIT_FUNC_POINTER(fmi2NewDiscreteStatesTYPE,            fmi2NewDiscreteStates);
    INIT_FUNC_POINTER(fmi2EnterContinuousTimeModeTYPE,      fmi2EnterContinuousTimeMode);
    INIT_FUNC_POINTER(fmi2CompletedIntegratorStepTYPE,      fmi2CompletedIntegratorStep);

/* Providing independent variables and re-initialization of caching */
    INIT_FUNC_POINTER(fmi2SetTimeTYPE,             fmi2SetTime);
    INIT_FUNC_POINTER(fmi2SetContinuousStatesTYPE, fmi2SetContinuousStates);

/* Evaluation of the model equations */
    INIT_FUNC_POINTER(fmi2GetDerivativesTYPE,                fmi2GetDerivatives);
    INIT_FUNC_POINTER(fmi2GetEventIndicatorsTYPE,            fmi2GetEventIndicators);
    INIT_FUNC_POINTER(fmi2GetContinuousStatesTYPE,           fmi2GetContinuousStates);
    INIT_FUNC_POINTER(fmi2GetNominalsOfContinuousStatesTYPE, fmi2GetNominalsOfContinuousStates);
#endif


    /***************************************************
Functions for FMI2 for Co-Simulation
****************************************************/

    /* Simulating the slave */
    INIT_FUNC_POINTER(fmi2SetRealInputDerivativesTYPE,  fmi2SetRealInputDerivatives, 0);
    INIT_FUNC_POINTER(fmi2GetRealOutputDerivativesTYPE, fmi2GetRealOutputDerivatives, 0);

    INIT_FUNC_POINTER(fmi2DoStepTYPE,     fmi2DoStep, 1);
    INIT_FUNC_POINTER(fmi2CancelStepTYPE, fmi2CancelStep, 1);

    /* Inquire slave status */
    INIT_FUNC_POINTER(fmi2GetStatusTYPE,        fmi2GetStatus, 0);
    INIT_FUNC_POINTER(fmi2GetRealStatusTYPE,    fmi2GetRealStatus, 0);
    INIT_FUNC_POINTER(fmi2GetIntegerStatusTYPE, fmi2GetIntegerStatus, 0);
    INIT_FUNC_POINTER(fmi2GetBooleanStatusTYPE, fmi2GetBooleanStatus, 0);
    INIT_FUNC_POINTER(fmi2GetStringStatusTYPE,  fmi2GetStringStatus, 0);

    return 0;
}

void FmuFreeDll(FMU *ret_Fmu)
{
#ifdef _WIN32
    FreeLibrary((HMODULE)ret_Fmu->DllModuleHandle);
#else
    dlclose(ret_Fmu->DllModuleHandle);
#endif
}

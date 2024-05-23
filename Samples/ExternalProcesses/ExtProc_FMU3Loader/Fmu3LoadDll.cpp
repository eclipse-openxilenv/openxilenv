#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#define HMODULE void*
#endif
#include "fmi3Functions.h"
#include "Fmu3Struct.h"
extern "C" {
#include "XilEnvExtProc.h"
}
#include "Fmu3LoadDll.h"



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
//    INIT_FUNC_POINTER(fmi2GetTypesPlatformTYPE, fmi2GetTypesPlatform, 0);
    INIT_FUNC_POINTER(fmi3GetVersionTYPE,       fmi3GetVersion, 0);
    INIT_FUNC_POINTER(fmi3SetDebugLoggingTYPE,  fmi3SetDebugLogging, 0);

/* Creation and destruction of FMU instances */
    INIT_FUNC_POINTER(fmi3InstantiateCoSimulationTYPE,  fmi3InstantiateCoSimulation, 1);
    INIT_FUNC_POINTER(fmi3FreeInstanceTYPE, fmi3FreeInstance, 1);

/* Enter and exit initialization mode, terminate and reset */
    //INIT_FUNC_POINTER(fmi2SetupExperimentTYPE,         fmi2SetupExperiment, 1);
    INIT_FUNC_POINTER(fmi3EnterInitializationModeTYPE, fmi3EnterInitializationMode, 1);
    INIT_FUNC_POINTER(fmi3ExitInitializationModeTYPE,  fmi3ExitInitializationMode, 1);
    INIT_FUNC_POINTER(fmi3TerminateTYPE,               fmi3Terminate, 1);
    INIT_FUNC_POINTER(fmi3ResetTYPE,                   fmi3Reset, 1);

/* Getting and setting variables values */
    INIT_FUNC_POINTER(fmi3GetFloat32TYPE, fmi3GetFloat32, 1);
    INIT_FUNC_POINTER(fmi3GetFloat64TYPE, fmi3GetFloat64, 1);
    INIT_FUNC_POINTER(fmi3GetInt8TYPE, fmi3GetInt8, 1);
    INIT_FUNC_POINTER(fmi3GetUInt8TYPE, fmi3GetUInt8, 1);
    INIT_FUNC_POINTER(fmi3GetInt16TYPE, fmi3GetInt16, 1);
    INIT_FUNC_POINTER(fmi3GetUInt16TYPE, fmi3GetUInt16, 1);
    INIT_FUNC_POINTER(fmi3GetInt32TYPE, fmi3GetInt32, 1);
    INIT_FUNC_POINTER(fmi3GetUInt32TYPE, fmi3GetUInt32, 1);
    INIT_FUNC_POINTER(fmi3GetInt64TYPE, fmi3GetInt64, 1);
    INIT_FUNC_POINTER(fmi3GetUInt64TYPE, fmi3GetUInt64, 1);
    INIT_FUNC_POINTER(fmi3GetBooleanTYPE, fmi3GetBoolean, 1);

    INIT_FUNC_POINTER(fmi3SetFloat32TYPE, fmi3SetFloat32, 1);
    INIT_FUNC_POINTER(fmi3SetFloat64TYPE, fmi3SetFloat64, 1);
    INIT_FUNC_POINTER(fmi3SetInt8TYPE, fmi3SetInt8, 1);
    INIT_FUNC_POINTER(fmi3SetUInt8TYPE, fmi3SetUInt8, 1);
    INIT_FUNC_POINTER(fmi3SetInt16TYPE, fmi3SetInt16, 1);
    INIT_FUNC_POINTER(fmi3SetUInt16TYPE, fmi3SetUInt16, 1);
    INIT_FUNC_POINTER(fmi3SetInt32TYPE, fmi3SetInt32, 1);
    INIT_FUNC_POINTER(fmi3SetUInt32TYPE, fmi3SetUInt32, 1);
    INIT_FUNC_POINTER(fmi3SetInt64TYPE, fmi3SetInt64, 1);
    INIT_FUNC_POINTER(fmi3SetUInt64TYPE, fmi3SetUInt64, 1);
    INIT_FUNC_POINTER(fmi3SetBooleanTYPE, fmi3SetBoolean, 1);


/***************************************************
Functions for FMI3 for Co-Simulation
****************************************************/

    INIT_FUNC_POINTER(fmi3DoStepTYPE,     fmi3DoStep, 1);

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

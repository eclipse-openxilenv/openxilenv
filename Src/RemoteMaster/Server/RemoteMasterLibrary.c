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
#include <stdio.h>
#include <stdarg.h>
//#include <dlfcn.h>

#include "ThrowError.h"

#include "VersionInfoSection.h"
#include "RemoteMasterServer.h"
#include "SetModelNameAndFunctionPointers.h"


void sc_exit(int i)
{
    ThrowError(1, "Model process has called exit() function with value %d. The process will never called untill it is restarted!", i);
    if (i == 0) i = 1;
#ifdef WITH_EXIT_FUNCTION
    longjmp(jumper, i);
#endif
}

double __GetNotANumber(void)
{
    double ret;
    uint64_t x;

    x = 0x7FF80000UL;
    x <<= 32;
    ret = *(double*)&x;
    return ret;
}

int scrt_sprintf(char *str, const char *format, ...)
{
    int ret;
    va_list vl;
    va_start(vl, format);
    ret = vsprintf(str, format, vl);
    va_end(vl);
    return ret;
}

int scrt_vsprintf(char *str, const char *format, va_list ap)
{
    return vsprintf(str, format, ap);
}

extern char RealtimeModelName[];
extern int RealtimeModelPriority;

extern void cyclic_test_object(void);
extern int init_test_object(void);
extern void terminate_test_object(void);
extern void reference_varis(void);

int main(int argc, char *argv[])
{
    //void *SharedLib;
    //void (*SetModelNameAndFunctionPointers)(const char *par_Name, int par_Prio, MODEL_FUNCTION_POINTERS *par_FunctionPointers);
    //int(*inner_main)(int argc, char *argv[]);

    MODEL_FUNCTION_POINTERS ModelFunctionPointers;
    ModelFunctionPointers.reference_varis = reference_varis;
    ModelFunctionPointers.init_test_object = init_test_object;
    ModelFunctionPointers.cyclic_test_object = cyclic_test_object;
    ModelFunctionPointers.terminate_test_object = terminate_test_object;

    /*SharedLib = dlopen("../../../../LinuxRemoteMasterCore/bin/x64/Library_Debug/libLinuxRemoteMasterCore.so", RTLD_NOW);
    SetModelNameAndFunctionPointers = dlsym(SharedLib, "SetModelNameAndFunctionPointers");
    inner_main = dlsym(SharedLib, "inner_main");
        // != NULL
    */
    SetModelNameAndFunctionPointers(RealtimeModelName, RealtimeModelPriority, &ModelFunctionPointers);

    // avoid that the section .sc_vr_i will be removed!
    const struct OwnVersionInfosStruct *Ptr = GetOwnVersionInfos();
    printf("VersionInfos = %s\n", Ptr->VersionSignString);

    return inner_main(argc, argv);
}

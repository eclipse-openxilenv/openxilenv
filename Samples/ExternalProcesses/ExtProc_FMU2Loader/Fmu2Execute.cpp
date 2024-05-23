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


#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <stdarg.h>
#endif
extern "C" {
#include "XilEnvExtProc.h"
}
#include "Fmu2Struct.h"


// Logger is only a PopUp in Softcar, nothing more... many parameters are ignored
void loggerFunction(void *componentEnvironment, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...) {
	//ignore *componentEnvironment
	//ignore instanceName
	//ignore status
	//ignore category
    if (status != fmi2OK) {
        va_list argptr;
        va_start(argptr,message);
        ThrowErrorVariableArguments(1, message, argptr);
        va_end(argptr);
    }
}

void StepFinished (void *componentEnvironment, fmi2Status status)
{
}

void* CallbackAllocateMemory (size_t Count, size_t Size)
{
    return calloc(Count, Size);
}


static void SyncAllInputVariables(FMU *par_Fmu)
{
    if (par_Fmu->SyncParametersWithBlackboard) {
        // Parameters
        if (par_Fmu->Parameters.Real.Count > 0) {
            par_Fmu->fmi2SetReal(par_Fmu->Component, par_Fmu->Parameters.Real.References, par_Fmu->Parameters.Real.Count, par_Fmu->Parameters.Real.Values);
        }
        if (par_Fmu->Parameters.Integer.Count > 0) {
            par_Fmu->fmi2SetInteger(par_Fmu->Component, par_Fmu->Parameters.Integer.References, par_Fmu->Parameters.Integer.Count, par_Fmu->Parameters.Integer.Values);
        }
        if (par_Fmu->Parameters.Boolean.Count > 0) {
            par_Fmu->fmi2SetBoolean(par_Fmu->Component, par_Fmu->Parameters.Boolean.References, par_Fmu->Parameters.Boolean.Count, par_Fmu->Parameters.Boolean.Values);
        }
    }
    // Inputs
    if (par_Fmu->Inputs.Real.Count >= 0) {
        par_Fmu->fmi2SetReal(par_Fmu->Component, par_Fmu->Inputs.Real.References, par_Fmu->Inputs.Real.Count, par_Fmu->Inputs.Real.Values);
    }
    if (par_Fmu->Inputs.Integer.Count > 0) {
        par_Fmu->fmi2SetInteger(par_Fmu->Component, par_Fmu->Inputs.Integer.References, par_Fmu->Inputs.Integer.Count, par_Fmu->Inputs.Integer.Values);
    }
    if (par_Fmu->Outputs.Boolean.Count > 0) {
        par_Fmu->fmi2SetBoolean(par_Fmu->Component, par_Fmu->Inputs.Boolean.References, par_Fmu->Inputs.Boolean.Count, par_Fmu->Inputs.Boolean.Values);
    }
}

static void SyncAllOutputVariables(FMU *par_Fmu)
{
    // Outputs
    if (par_Fmu->Outputs.Real.Count > 0) {
        par_Fmu->fmi2GetReal(par_Fmu->Component, par_Fmu->Outputs.Real.References, par_Fmu->Outputs.Real.Count, par_Fmu->Outputs.Real.Values);
    }
    if (par_Fmu->Outputs.Integer.Count > 0) {
        par_Fmu->fmi2GetInteger(par_Fmu->Component, par_Fmu->Outputs.Integer.References, par_Fmu->Outputs.Integer.Count, par_Fmu->Outputs.Integer.Values);
    }
    if (par_Fmu->Outputs.Boolean.Count > 0) {
        par_Fmu->fmi2GetBoolean(par_Fmu->Component, par_Fmu->Outputs.Boolean.References, par_Fmu->Outputs.Boolean.Count, par_Fmu->Outputs.Boolean.Values);
    }
    if (par_Fmu->SyncLocalsWithBlackboard) {
        // Locals
        if (par_Fmu->Locals.Real.Count > 0) {
            par_Fmu->fmi2GetReal(par_Fmu->Component, par_Fmu->Locals.Real.References, par_Fmu->Locals.Real.Count, par_Fmu->Locals.Real.Values);
        }
        if (par_Fmu->Locals.Integer.Count > 0) {
            par_Fmu->fmi2GetInteger(par_Fmu->Component, par_Fmu->Locals.Integer.References, par_Fmu->Locals.Integer.Count, par_Fmu->Locals.Integer.Values);
        }
        if (par_Fmu->Locals.Boolean.Count > 0) {
            par_Fmu->fmi2GetBoolean(par_Fmu->Component, par_Fmu->Locals.Boolean.References, par_Fmu->Locals.Boolean.Count, par_Fmu->Locals.Boolean.Values);
        }
    }
}

int FmuInit(FMU *par_Fmu) 
{
    int Vid;
    bool SyncParametersWithBlackboardSave;
    // Initialization
    // Set callback functions
    static const fmi2CallbackFunctions cbf = {loggerFunction, //const fmi2CallbackLogger         logger;
                                              CallbackAllocateMemory, // const fmi2CallbackAllocateMemory allocateMemory;
                                              free, // const fmi2CallbackFreeMemory     freeMemory;
                                              NULL, //StepFinished, //const fmi2StepFinished           stepFinished;
                                              NULL}; //const fmi2ComponentEnvironment   componentEnvironment;;
    //Instantiate
    fmi2Component Component = par_Fmu->fmi2Instantiate(par_Fmu->ModelName , fmi2CoSimulation, par_Fmu->Guid, par_Fmu->ResourceDirectory, &cbf, fmi2False, fmi2False); //fmi2True, fmi2True);
    if (Component == NULL) {
        return -1;
    }
    par_Fmu->Component = Component;

    // Set all variable start values (of "ScalarVariable / <type> / start")
    SyncParametersWithBlackboardSave = par_Fmu->SyncParametersWithBlackboard;
    par_Fmu->SyncParametersWithBlackboard = 1;
    SyncAllInputVariables(par_Fmu);
    par_Fmu->SyncParametersWithBlackboard = SyncParametersWithBlackboardSave;

    // Initialize slaves
    par_Fmu->Status = par_Fmu->fmi2SetupExperiment(par_Fmu->Component, fmi2False, 0.0, 0.0, fmi2False, 0.0);  // Startzeit 0.0 und keine Stopzeit!
    par_Fmu->Status = par_Fmu->fmi2EnterInitializationMode(par_Fmu->Component);

    // Set the input values at time = startTime
    SyncAllInputVariables(par_Fmu);

    par_Fmu->Status = par_Fmu->fmi2ExitInitializationMode(par_Fmu->Component);

    // Communication step size
    Vid = attach_bbvari("XilEnv.SampleTime");
    par_Fmu->TimeStep = read_bbvari_double(Vid);
    remove_bbvari(Vid);

    par_Fmu->SimTime = 0.0;

    return 0;
}

int FmuOneCycle(FMU *par_Fmu)
{
    fmi2Boolean boolVal;
    double TimeStep = par_Fmu->TimeStep * (double)get_cycle_period();
    if (par_Fmu->Status != fmi2Discard) {
        SyncAllInputVariables(par_Fmu);
        par_Fmu->Status = par_Fmu->fmi2DoStep(par_Fmu->Component, par_Fmu->SimTime, TimeStep, fmi2True);
        switch (par_Fmu->Status) {
        case fmi2Discard:
            par_Fmu->fmi2GetBooleanStatus(par_Fmu->Component, fmi2Terminated, &boolVal);
            if (boolVal == fmi2True) {
                ThrowError(1, "FMU %s wants to terminate simulation.", par_Fmu->ModelName);
            }
            break;
        case fmi2Error:
        case fmi2Fatal:
            //return -1;
            break;
        }
        SyncAllOutputVariables(par_Fmu);
    }
    par_Fmu->SimTime += TimeStep;
    return 0;
}

void FmuTerminate(FMU *par_Fmu)
{
    if ((par_Fmu->Status != fmi2Error) && (par_Fmu->Status != fmi2Fatal)) {
        par_Fmu->fmi2Terminate(par_Fmu->Component);
    }
    if (par_Fmu->Status != fmi2Fatal) {
        par_Fmu->fmi2FreeInstance(par_Fmu->Component);
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <stdarg.h>
#endif
extern "C" {
#include "XilEnvExtProc.h"
}
#include "Fmu3Struct.h"

// The logger will rise a popup message except fmi2OK.
// If the environment variable FMU_LOGGING_FOLDER is set to a folder, the logger will write a log file (name is <model name>.log)
void loggerFunction(fmi3InstanceEnvironment instanceEnvironment, fmi3Status status, fmi3String category, fmi3String message)
{
    FMU* Fmu = (FMU*)instanceEnvironment;

    if (Fmu != NULL) {
        if (Fmu->LoggingEnable) {
            if (Fmu->LoggingFile == nullptr) {
                char* LoggingPath = (char*)malloc(strlen(Fmu->LoggingFolder) + strlen(Fmu->ModelName) + 6);
                strcpy(LoggingPath, Fmu->LoggingFolder);
#ifdef _WIN32
                strcat(LoggingPath, "\\");
#else
                strcat(LoggingPath, "/");
#endif
                strcat(LoggingPath, Fmu->ModelName);
                strcat(LoggingPath, ".log");
                Fmu->LoggingFile = fopen(LoggingPath, "wt");
                if (Fmu->LoggingFile == nullptr) {
                    ThrowError(1, "cannot open logging file \"%s\" -> logging disabled", LoggingPath);
                    Fmu->LoggingEnable = false;
                }
            }
            if (Fmu->LoggingFile != nullptr) {
                switch (status) {
                case fmi3OK:
                    fprintf(Fmu->LoggingFile, "fmi3OK: ");
                    break;
                case fmi3Warning:
                    fprintf(Fmu->LoggingFile, "fmi3Warning: ");
                    break;
                case fmi3Discard:
                    fprintf(Fmu->LoggingFile, "fmi3Discard: ");
                    break;
                case fmi3Error:
                    fprintf(Fmu->LoggingFile, "fmi3Error: ");
                    break;
                case fmi3Fatal:
                    fprintf(Fmu->LoggingFile, "fmi3Fatal: ");
                    break;
                default:
                    fprintf(Fmu->LoggingFile, "unknown: ");
                    break;
                }
                fprintf(Fmu->LoggingFile, "%s", message);
                fprintf(Fmu->LoggingFile, "\n");
            }
        }
    }
    if (status != fmi3OK) {
        ThrowErrorNoVariableArguments(1, message);
    }
}

void intermediateUpdateCallback (fmi3InstanceEnvironment instanceEnvironment,
                                 fmi3Float64 intermediateUpdateTime,
                                 fmi3Boolean intermediateVariableSetRequested,
                                 fmi3Boolean intermediateVariableGetAllowed,
                                 fmi3Boolean intermediateStepFinished,
                                 fmi3Boolean canReturnEarly,
                                 fmi3Boolean* earlyReturnRequested,
                                 fmi3Float64* earlyReturnTime)
{
    // do nothing
}

#define FMI3_SET(Fmu, Type, DataType)\
if (Fmu->Type.DataType.Count > 0) { \
        fmi3Status Status = (Fmu)->fmi3Set##DataType((Fmu)->Instance, (Fmu)->Type.DataType.References, (Fmu)->Type.DataType.Count, (Fmu)->Type.DataType.Values, (Fmu)->Type.DataType.Count);\
        if (Status != fmi3OK) {\
            ThrowError(1, "fmi3Set"#DataType"() returns an error %i", Status);\
    }\
}

#define FMI3_GET(Fmu, Type, DataType)\
if (Fmu->Type.DataType.Count > 0) { \
        fmi3Status Status = (Fmu)->fmi3Get##DataType((Fmu)->Instance, (Fmu)->Type.DataType.References, (Fmu)->Type.DataType.Count, (Fmu)->Type.DataType.Values, (Fmu)->Type.DataType.Count);\
        if (Status != fmi3OK) {\
            ThrowError(1, "fmi3Get"#DataType"() returns an error %i", Status);\
    }\
}

static void SyncAllInputVariables(FMU *par_Fmu)
{
    if (par_Fmu->SyncParametersWithBlackboard) {
        // Parameters
        FMI3_SET(par_Fmu, Parameters, Float32)
        FMI3_SET(par_Fmu, Parameters, Float64)
        FMI3_SET(par_Fmu, Parameters, Int8)
        FMI3_SET(par_Fmu, Parameters, UInt8)
        FMI3_SET(par_Fmu, Parameters, Int16)
        FMI3_SET(par_Fmu, Parameters, UInt16)
        FMI3_SET(par_Fmu, Parameters, Int32)
        FMI3_SET(par_Fmu, Parameters, UInt32)
        FMI3_SET(par_Fmu, Parameters, Int64)
        FMI3_SET(par_Fmu, Parameters, UInt64)
        FMI3_SET(par_Fmu, Parameters, Boolean)
    }
    // Inputs
    FMI3_SET(par_Fmu, Inputs, Float32)
    FMI3_SET(par_Fmu, Inputs, Float64)
    FMI3_SET(par_Fmu, Inputs, Int8)
    FMI3_SET(par_Fmu, Inputs, UInt8)
    FMI3_SET(par_Fmu, Inputs, Int16)
    FMI3_SET(par_Fmu, Inputs, UInt16)
    FMI3_SET(par_Fmu, Inputs, Int32)
    FMI3_SET(par_Fmu, Inputs, UInt32)
    FMI3_SET(par_Fmu, Inputs, Int64)
    FMI3_SET(par_Fmu, Inputs, UInt64)
    FMI3_SET(par_Fmu, Inputs, Boolean)
}

static void SyncAllOutputVariables(FMU *par_Fmu)
{
    // Outputs
    FMI3_GET(par_Fmu, Outputs, Float32)
    FMI3_GET(par_Fmu, Outputs, Float64)
    FMI3_GET(par_Fmu, Outputs, Int8)
    FMI3_GET(par_Fmu, Outputs, UInt8)
    FMI3_GET(par_Fmu, Outputs, Int16)
    FMI3_GET(par_Fmu, Outputs, UInt16)
    FMI3_GET(par_Fmu, Outputs, Int32)
    FMI3_GET(par_Fmu, Outputs, UInt32)
    FMI3_GET(par_Fmu, Outputs, Int64)
    FMI3_GET(par_Fmu, Outputs, UInt64)
    FMI3_GET(par_Fmu, Outputs, Boolean)
    if (par_Fmu->SyncLocalsWithBlackboard) {
        // Locals
        FMI3_GET(par_Fmu, Locals, Float32)
        FMI3_GET(par_Fmu, Locals, Float64)
        FMI3_GET(par_Fmu, Locals, Int8)
        FMI3_GET(par_Fmu, Locals, UInt8)
        FMI3_GET(par_Fmu, Locals, Int16)
        FMI3_GET(par_Fmu, Locals, UInt16)
        FMI3_GET(par_Fmu, Locals, Int32)
        FMI3_GET(par_Fmu, Locals, UInt32)
        FMI3_GET(par_Fmu, Locals, Int64)
        FMI3_GET(par_Fmu, Locals, UInt64)
        FMI3_GET(par_Fmu, Locals, Boolean)
    }
}

int FmuInit(FMU *par_Fmu) 
{
    int Vid;
    bool SyncParametersWithBlackboardSave;
    long long  SchedulerPeriod;

    //Instantiate
    fmi3Instance Instance = par_Fmu->fmi3InstantiateCoSimulation(par_Fmu->ModelName,
                                                                 par_Fmu->instantiationToken,
                                                                 par_Fmu->ResourceDirectory,
                                                                 fmi3False,  // visible
                                                                 par_Fmu->LoggingEnable,   // loggingOn
                                                                 fmi3False,  // eventModeUsed
                                                                 fmi3False,  // earlyReturnAllowed
                                                                 NULL,       // requiredIntermediateVariables
                                                                 0,          // nRequiredIntermediateVariables
                                                                 par_Fmu,    // instanceEnvironment,
                                                                 loggerFunction,
                                                                 intermediateUpdateCallback);
    if (Instance == NULL) {
        return -1;
    }
    par_Fmu->Instance = Instance;

    // set all variable start values (of "ScalarVariable / <type> / start")
    SyncParametersWithBlackboardSave = par_Fmu->SyncParametersWithBlackboard;
    par_Fmu->SyncParametersWithBlackboard = 1;
    SyncAllInputVariables(par_Fmu);
    par_Fmu->SyncParametersWithBlackboard = SyncParametersWithBlackboardSave;

    //Initialize slaves
    par_Fmu->Status = par_Fmu->fmi3EnterInitializationMode(par_Fmu->Instance,
                                                           fmi3False, // toleranceDefined,
                                                           0.0,       // tolerance,
                                                           0.0,       // startTime,
                                                           fmi3False, // stopTimeDefined,
                                                           0.0);      // stopTime

    // set the input values at time = startTime
    SyncAllInputVariables(par_Fmu);

    par_Fmu->Status = par_Fmu->fmi3ExitInitializationMode(par_Fmu->Instance);

    //communication step size
    SchedulerPeriod = 0;
    GetSchedulingInformation(nullptr, nullptr, &SchedulerPeriod, nullptr, nullptr, nullptr, nullptr);
    par_Fmu->TimeStep = (double)SchedulerPeriod * 0.000000001; // convert from ns to s

    par_Fmu->SimTime = 0.0;

    return 0;
}

int FmuOneCycle(FMU *par_Fmu)
{
    double TimeStep = par_Fmu->TimeStep * (double)get_cycle_period();
    if (par_Fmu->Status != fmi3Discard) {
        fmi3Boolean eventHandlingNeeded;
        fmi3Boolean terminateSimulation;
        fmi3Boolean earlyReturn;
        fmi3Float64 lastSuccessfulTime;

        SyncAllInputVariables(par_Fmu);

        par_Fmu->Status = par_Fmu->fmi3DoStep(par_Fmu->Instance,   // instance
                                              par_Fmu->SimTime,    // currentCommunicationPoint
                                              TimeStep,            // communicationStepSize
                                              fmi3True,            // noSetFMUStatePriorToCurrentPoint
                                              &eventHandlingNeeded,
                                              &terminateSimulation,
                                              &earlyReturn,
                                              &lastSuccessfulTime);
        switch (par_Fmu->Status) {
        case fmi3Discard:
            if (terminateSimulation == fmi3True) {
                ThrowError(1, "FMU %s wants to terminate simulation.", par_Fmu->ModelName);
            }
            break;
        default:
            break;
        }
        SyncAllOutputVariables(par_Fmu);
    }
    par_Fmu->SimTime += TimeStep;
    return 0;
}

void FmuTerminate(FMU *par_Fmu)
{
    if ((par_Fmu->Status != fmi3Error) && (par_Fmu->Status != fmi3Fatal)) {
        par_Fmu->fmi3Terminate(par_Fmu->Instance);
    }
    if (par_Fmu->Status != fmi3Fatal) {
        par_Fmu->fmi3FreeInstance(par_Fmu->Instance);
    }
}

#include "Platform.h"
#include <stdio.h>

extern "C" {
#include "Scheduler.h"
#include "A2LLink.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cImportA2lMeasurementListCmd)

int cImportA2lMeasurementListCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cImportA2lMeasurementListCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    int Pid;
    if ((Pid = get_pid_by_name (par_Parser->GetParameter (1))) == UNKNOWN_PROCESS) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unknown process \"%s\"", par_Parser->GetParameter (1));
        return -1;
    }
    int Ret = ImportMeasurementReferencesListForProcess (Pid, par_Parser->GetParameter (0));
    if (Ret == -1) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "file or path \"%s\" not found", par_Parser->GetParameter (0));
        return -1;
    }
    return 0;
}

int cImportA2lMeasurementListCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cImportA2lMeasurementListCmd LoadMeasurementListCmd ("IMPORT_A2L_MEASUREMENT_LIST",
                                       2, 
                                       2,  
                                       nullptr,
                                       FALSE, 
                                       FALSE,  
                                       0,
                                       0,
                                       CMD_INSIDE_ATOMIC_ALLOWED);

#pragma once

#include "Fmu3Struct.h"

int FmuInit(FMU *par_Fmu);
int FmuOneCycle(FMU *par_Fmu);
void FmuTerminate(FMU *par_Fmu);

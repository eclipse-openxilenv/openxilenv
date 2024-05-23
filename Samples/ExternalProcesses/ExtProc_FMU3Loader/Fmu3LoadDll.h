#pragma once

#include "Fmu3Struct.h"

int FmuLoadDll(FMU *ret_Fmu, const char *par_DllName);
void FmuFreeDll(FMU *ret_Fmu);

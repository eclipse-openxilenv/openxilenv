#pragma once

#include "Fmu3Struct.h"

int ReferenceAllVariablesToBlackboard(FMU *par_Fmu);
int ReadFmuXml(FMU *ret_Fmu, const char *par_XmlFileName);

#pragma once

#include "Fmu3Struct.h"

int ExtractFmuToUniqueTempDirectory(const char *par_FmuFileName, char *ret_ExtractedToPath);
int DeleteDirectory (const char *par_Directory);

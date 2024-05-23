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


#include "Platform.h"
#include "Config.h"
#include "ReadExeInfos.h"
#include "ThrowError.h"
#include "GetElfSectionVersionInfos.h"

int CheckIfExecutableAndXilEnvAndScharedLibraryMatches(const char *par_Executable,
                                                       const char *par_SharedLibrary)
{
    int ExeVersion;
    int ExeMinorVersion;
    int ExeRemoteMasterLibraryAPIVersion;
    int SharedLibVersion;
    int SharedLibMinorVersion;
    int SharedLibRemoteMasterLibraryAPIVersion;

    if (GetElfSectionVersionInfos(par_Executable, ".sc_vr_i", &ExeVersion, &ExeMinorVersion, &ExeRemoteMasterLibraryAPIVersion)) {
        return -1;
    }

    if (GetElfSectionVersionInfos(par_SharedLibrary, ".sc_vr_i", &SharedLibVersion, &SharedLibMinorVersion, &SharedLibRemoteMasterLibraryAPIVersion)) {
        return -1;
    }

    if ((SharedLibVersion != XILENV_VERSION) || (SharedLibMinorVersion != XILENV_MINOR_VERSION)) {
        ThrowError(1, "Version of the shared library \"%s\" API are %i patch (%i) but XilEnv have the version %i patch (%i)",
              par_SharedLibrary, SharedLibVersion, SharedLibMinorVersion, XILENV_VERSION, XILENV_MINOR_VERSION);
        return -1;
    }

    if (ExeRemoteMasterLibraryAPIVersion != SharedLibRemoteMasterLibraryAPIVersion) {
        ThrowError(1, "Version of the shared library \"%s\" API are %i the executable \"%s\" have version %i",
              par_SharedLibrary, SharedLibRemoteMasterLibraryAPIVersion, par_Executable, ExeRemoteMasterLibraryAPIVersion);
        return -1;
    }

    return 0;  //  Match
}

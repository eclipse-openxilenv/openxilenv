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


#include "Config.h"
#include "VersionInfoSection.h"

#define INNER_STR_VALUE(arg) #arg
#define STR_VALUE(arg) INNER_STR_VALUE(arg)

#define VERSION_STRING STR_VALUE(XILENV_VERSION)
#define MINOR_VERSION_STRING STR_VALUE(XILENV_MINOR_VERSION)

// darf nicht static sein sonst fehlt die Section in der shared library
const __attribute__((__section__(".sc_vr_i"))) struct OwnVersionInfosStruct OwnVersionInfos =
    {OWN_VERSION_SIGN_STRING,
     XILENV_VERSION,
     XILENV_MINOR_VERSION,
     REMOTE_MASTER_LIBRARY_API_VERSION,
     0,
     __DATE__,
     __TIME__,
     __DATE__ "  " __TIME__ "  Version " VERSION_STRING " (" MINOR_VERSION_STRING ")",
     "" };

const struct OwnVersionInfosStruct *GetOwnVersionInfos(void)
{
    return &OwnVersionInfos;
}

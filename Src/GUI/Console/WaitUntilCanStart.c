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
#include "Platform.h"
#include "ConfigurablePrefix.h"
#include "CheckIfAlreadyRunning.h"

int WaitUntilXilEnvCanStart(char *par_InstanceName, void *par_Application)
{
    const char *Name = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME);
    printf("%s with the instance \"%s\" already running. %s will be terminated immediately!\n", Name, par_InstanceName, Name);
    // Wenn 1 Zurueckgegeben wird XilEnv beenden
    // Wenn 0 XilEnv Startup fortfuehren
    return 1;
}

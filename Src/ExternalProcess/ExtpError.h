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


#ifndef EXTP_ERROR_H
#define EXTP_ERROR_H

#include "ExtpProcessAndTaskInfos.h"

int XilEnvInternal_ThrowError(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos,
                               int level,                       // Error level
                               char const *format,              // Format string (same as printf)
                               ...);                            // Parameter list

#endif // EXTP_ERROR_H

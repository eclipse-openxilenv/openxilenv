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


#ifndef __RUNTIMEMEASUREMENT_H
#define __RUNTIMEMEASUREMENT_H

int BeginRuntimeMeassurement (const char *Name, int id);

int EndRuntimeMeassurement (int id, unsigned int max);

int GetRuntimeMeassurement (int id, char *line);

#ifdef FUNCTION_RUNTIME
#define BEGIN_RUNTIME_MEASSUREMENT(FunctionName) \
{  \
    static int RuntimeMeassurementId;     \
    RuntimeMeassurementId = BeginRuntimeMeassurement (__FILE__ ":" FunctionName, RuntimeMeassurementId);

#define END_RUNTIME_MEASSUREMENT \
    EndRuntimeMeassurement (RuntimeMeassurementId, 0); \
}

#define END_RUNTIME_MEASSUREMENT_STOP(max) \
    if (EndRuntimeMeassurement (RuntimeMeassurementId, max)) { \
        error (1, "max runtime overflow");\
    } \
}
#else 
#define BEGIN_RUNTIME_MEASSUREMENT(FunctionName) 

#define END_RUNTIME_MEASSUREMENT

#define END_RUNTIME_MEASSUREMENT_STOP(max)
#endif

#endif

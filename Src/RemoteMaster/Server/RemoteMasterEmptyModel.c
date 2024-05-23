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
#include "RealtimeModelInterface.h"

void reference_varis(void)
{
}

int init_test_object(void)
{
    return 0;
}

void terminate_test_object(void)
{
}

void cyclic_test_object(void)
{
}

DEFINE_TASK_CONTROL_BLOCK(EmptyModel, 100)

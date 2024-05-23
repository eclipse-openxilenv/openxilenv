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


#ifndef MY_UDIV128_H
#define MY_UDIV128_H

#include <stdint.h>

uint64_t udiv128 (uint64_t nlo, uint64_t nhi, uint64_t d, uint64_t *rem);

uint64_t my_umuldiv64(uint64_t a, uint64_t b, uint64_t c);

#endif // MY_UDIV128_H

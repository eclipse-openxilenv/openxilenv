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


#include <stdint.h>
#ifdef _WIN32
#include <intrin.h>
#endif
#include "my_udiv128.h"


uint64_t my_umuldiv64(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t Ret;
#ifdef __GNUC__
    unsigned __int128 Help128 = a;
    Help128 = Help128 * b;
    if (c == 0)
    {
        c = 1;
    }
    Help128 = Help128 / c;
    Ret = (uint64_t)Help128;
#else
    uint64_t Low, High, Remainder;
    Low = _umul128(a, b, &High);
    Ret = _udiv128(High, Low, c, &Remainder);
    // Rouding
    if (Remainder >= (c >> 1)) {
        Ret += 1;
    }
#endif
    return Ret;
}

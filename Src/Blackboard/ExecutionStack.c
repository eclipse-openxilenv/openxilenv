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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#ifdef _WIN32
#include <intrin.h>
#else
static inline uint64_t _umul128(uint64_t a, uint64_t b, uint64_t *ret_HighPart)
{
    __int128 result = (__int128)a * (__int128)b;
    *ret_HighPart = result >> 64;
    return (uint64_t)result;
}
#endif

#define EXEC_ST_C
#include "Config.h"
#include "EquationParser.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#ifndef REALTIME
  #include "ThrowError.h"
#endif
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ReadCanCfg.h"
#ifndef REMOTE_MASTER
#include "EquationList.h"
#else
#include "RemoteMasterReqToClient.h"
#endif
#include "ExecutionStack.h"

#define UNUSED(x) (void)(x)

#ifndef MAXDOUBLE
#define MAXDOUBLE 1.7976931348623158e+308
#endif

#define UNUSED(x) (void)(x)

typedef struct {
    int OperationNo;
    int (*OperationPointer) (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *);
} OPERATION_TABLE_ELEM;

static inline double ToDouble(struct STACK_ENTRY Data)
{
    switch (Data.Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        return (double)Data.V.qw;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        return (double)Data.V.uqw;
    case FLOAT_OR_INT_64_TYPE_F64:
        return Data.V.d;
    default:
        return 0.0;   // this should never happen
    }
}

static inline int64_t ToInt64(struct STACK_ENTRY Data)
{
    switch (Data.Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        return Data.V.qw;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        return (int64_t)Data.V.uqw;
    case FLOAT_OR_INT_64_TYPE_F64:
        if (Data.V.d >= 0.0) return (int64_t)(Data.V.d + 0.5);
        return (int64_t)(Data.V.d - 0.5);
    default:
        return 0;   // this should never happen
    }
}

static inline uint64_t ToUint64(struct STACK_ENTRY Data)
{
    switch (Data.Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        return (uint64_t)Data.V.qw;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        return Data.V.uqw;
    case FLOAT_OR_INT_64_TYPE_F64:
        return (uint64_t)(Data.V.d + 0.5);
    default:
        return 0;   // this should never happen
    }
}

static inline int ToBool(struct STACK_ENTRY Data)
{
    switch (Data.Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        return (Data.V.qw != 0);
    case FLOAT_OR_INT_64_TYPE_UINT64:
        return (Data.V.uqw != 0);
    case FLOAT_OR_INT_64_TYPE_F64:
        if ((Data.V.d + 0.5) >= 1.0) return 1;
        if ((Data.V.d - 0.5) <= -1.0) return 1;
        return 0;
    default:
        return 0;   // this should never happen
    }
}

typedef struct {
   int64_t High;
   uint64_t Low;
} MY_128_INT;

static inline void init128(struct STACK_ENTRY *In, MY_128_INT *Result)
{
    switch (In->Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        if (In->V.qw < 0) {
            Result->High = -1;
        } else {
            Result->High = 0;
        }
        Result->Low = (uint64_t)In->V.qw;
        break;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        Result->High = 0;
        Result->Low = In->V.uqw;
        break;
    case FLOAT_OR_INT_64_TYPE_F64:
    default:
        Result->High = 0;
        Result->Low = In->V.uqw;
        ThrowError(1, "This should never happen");
        break;
    }
}

static inline void sub128(MY_128_INT *a, MY_128_INT *b, MY_128_INT *Result)
{
    Result->High = a->High - b->High;
    Result->Low = a->Low - b->Low;
    if (Result->Low > a->Low) {
        Result->High--;
    }
}

static inline void add128(MY_128_INT *a, MY_128_INT *b, MY_128_INT *Result)
{
    Result->High = a->High + b->High;
    Result->Low = a->Low + b->Low;
    if (Result->Low < a->Low) {
        Result->High++;
    }
}

static inline void inc128(MY_128_INT *a)
{
    uint64_t h = a->Low;
    h++;
    if (h < a->Low) a->High++;
    a->Low = h;
}


static inline void neg128(MY_128_INT *a)
{
    a->High = (int64_t)~((uint64_t)a->High);
    a->Low = ~a->Low;
    inc128(a);
}


static inline void limit128(MY_128_INT *In, struct STACK_ENTRY *Result)
{
    if (In->High < 0) {
        // negative
        Result->Type = FLOAT_OR_INT_64_TYPE_INT64;
        if (In->High < -1) {
            Result->V.uqw = 0x8000000000000000ULL;
        } else if (In->Low < 0x8000000000000000ULL) {
            Result->V.uqw = 0x8000000000000000ULL;
        } else {
            Result->V.uqw = In->Low;
        }
    } else {
        // postive
        Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
        if (In->High > 0) {
            Result->V.uqw = 0xFFFFFFFFFFFFFFFFULL;
        } else {
            Result->V.uqw = In->Low;
        }
    }
    Result->ByteWidth = 8;
}


static inline void SubtractLimit(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a - b
{
    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    sub128(&a128, &b128, &c128);

    limit128(&c128, Result);
}

static inline void AddLimit(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a + b
{
    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    add128(&a128, &b128, &c128);

    limit128(&c128, Result);
}

static inline void MultiplyLimit(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a * b
{

    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    if (a128.High < 0) {
        if (b128.High < 0) {
            // neg. * neg. = pos.
            neg128(&a128);
            neg128(&b128);
            c128.Low = _umul128(a128.Low, b128.Low, (uint64_t*)&(c128.High));
        } else {
            // neg. * pos. = neg.
            neg128(&a128);
            c128.Low = _umul128(a128.Low, b128.Low, (uint64_t*)&(c128.High));
            if ((uint64_t)(c128.High) > 0x7FFFFFFFFFFFFFFFULL)  {  // Limitate to min. max. value no overrun
                c128.High = 0x7FFFFFFFFFFFFFFFULL;
            }
            neg128(&c128);
        }
    } else {
        if (b128.High < 0) {
            // pos. * neg. = neg.
            neg128(&b128);
            c128.Low = _umul128(a128.Low, b128.Low, (uint64_t*)&(c128.High));
            if ((uint64_t)(c128.High) > 0x7FFFFFFFFFFFFFFFULL) {  // Limitate to min. max. value no overrun
                c128.High = 0x7FFFFFFFFFFFFFFFULL;
            }
            neg128(&c128);
        } else {
            // pos. * pos. = pos.
            c128.Low = _umul128(a128.Low, b128.Low, (uint64_t*)&(c128.High));
            if ((uint64_t)(c128.High) > 0x7FFFFFFFFFFFFFFFULL)  {  // Limitate to min. max. value no overrun
                c128.High = 0x7FFFFFFFFFFFFFFFULL;
            }
        }
    }
    limit128(&c128, Result);
}

static inline void DivideLimit(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a / b
{

    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    if ((b128.High == 0) && (b128.Low == 0)) {
        Result->V.uqw = 0xFFFFFFFFFFFFFFFF;  // Division by 0
        Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
        Result->ByteWidth = 8;
    } else {
        if (a128.High < 0) {
            if (b128.High < 0) {
                // neg. / neg. = pos.
                neg128(&a128);
                neg128(&b128);
                c128.Low = a128.Low / b128.Low;
                c128.High = 0;
            } else {
                // neg. / pos. = neg.
                neg128(&a128);
                c128.Low = a128.Low / b128.Low;
                c128.High = 0;
                neg128(&c128);
            }
        } else {
            if (b128.High < 0) {
                // pos. / neg. = neg.
                neg128(&b128);
                c128.Low = a128.Low / b128.Low;
                c128.High = 0;
                if (c128.Low > 0x7FFFFFFFFFFFFFFFULL) {  // Limitate to min. max. value no overrun
                    c128.Low = 0x7FFFFFFFFFFFFFFFULL;
                }
                neg128(&c128);
            } else {
                // pos. / pos. = pos.
                c128.Low = a128.Low / b128.Low;
                c128.High = 0;
            }
        }
        limit128(&c128, Result);
    }
}

static inline void ModuloLimit(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a % b
{

    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    if ((b128.High == 0) && (b128.Low == 0)) {
        Result->V.uqw = 0x0;  // Division by 0
        Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
        Result->ByteWidth = 8;
    } else {
        if (a128.High < 0) {
            if (b128.High < 0) {
                // neg. % neg. = neg.
                neg128(&a128);
                neg128(&b128);
                c128.Low = a128.Low % b128.Low;
                c128.High = 0;
                neg128(&c128);
            } else {
                // neg. % pos. = neg.
                neg128(&a128);
                c128.Low = a128.Low % b128.Low;
                c128.High = 0;
                neg128(&c128);
            }
        } else {
            if (b128.High < 0) {
                // pos. % neg. = pos.
                neg128(&b128);
                c128.Low = a128.Low % b128.Low;
                c128.High = 0;
                if (c128.Low > 0x7FFFFFFFFFFFFFFFULL) {  // Limitate to min. max. value no overrun
                    c128.Low = 0x7FFFFFFFFFFFFFFFULL;
                }
            } else {
                // pos. % pos. = pos.
                c128.Low = a128.Low % b128.Low;
                c128.High = 0;
            }
        }
        limit128(&c128, Result);
    }
}


static inline void NotEqual(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a != b
{
    MY_128_INT a128;
    MY_128_INT b128;

    init128(a, &a128);
    init128(b, &b128);

    Result->V.uqw = (a128.Low != b128.Low) || (a128.High != b128.High);
    Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
    Result->ByteWidth = 8;
}

static inline void Equal(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a == b
{
    MY_128_INT a128;
    MY_128_INT b128;

    init128(a, &a128);
    init128(b, &b128);

    Result->V.uqw = (a128.Low == b128.Low) && (a128.High == b128.High);
    Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
    Result->ByteWidth = 8;
}

static inline void Larger(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a > b
{
    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    sub128(&a128, &b128, &c128);

    if ((c128.High == 0) && (c128.Low == 0)) {
        Result->V.uqw = 0;
    } else if (c128.High < 0) {
        Result->V.uqw = 0;
    } else {
        Result->V.uqw = 1;
    }
    Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
    Result->ByteWidth = 8;
}

static inline void LargerOrEqual(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a >= b
{
    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    sub128(&a128, &b128, &c128);

    if ((c128.High == 0) && (c128.Low == 0)) {
        Result->V.uqw = 1;
    } else if (c128.High < 0) {
        Result->V.uqw = 0;
    } else {
        Result->V.uqw = 1;
    }
    Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
    Result->ByteWidth = 8;
}

static inline void Lesser(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a < b
{
    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    sub128(&a128, &b128, &c128);

    if ((c128.High == 0) && (c128.Low == 0)) {
        Result->V.uqw = 0;
    } else if (c128.High < 0) {
        Result->V.uqw = 1;
    } else {
        Result->V.uqw = 0;
    }
    Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
    Result->ByteWidth = 8;
}

static inline void LesserOrEqual(struct STACK_ENTRY *a, struct STACK_ENTRY *b, struct STACK_ENTRY *Result)   // Result = a <= b
{
    MY_128_INT a128;
    MY_128_INT b128;
    MY_128_INT c128;

    init128(a, &a128);
    init128(b, &b128);

    sub128(&a128, &b128, &c128);

    if ((c128.High == 0) && (c128.Low == 0)) {
        Result->V.uqw = 1;
    } else if (c128.High < 0) {
        Result->V.uqw = 1;
    } else {
        Result->V.uqw = 0;
    }
    Result->Type = FLOAT_OR_INT_64_TYPE_UINT64;
    Result->ByteWidth = 8;
}

int logical_and_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    pStack->Data[pStack->Pointer-1].V.uqw = ToBool(pStack->Data[pStack->Pointer]) && ToBool(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int logical_or_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    pStack->Data[pStack->Pointer-1].V.uqw = ToBool(pStack->Data[pStack->Pointer]) || ToBool(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static uint64_t CompareDoubleEqual(double a, double b)
{
    double diff = a - b;
    if ((diff <= 0.0) && (diff >= 0.0)) return 1;
    else return 0;
}

int equal_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.uqw = CompareDoubleEqual(ToDouble(pStack->Data[pStack->Pointer]), ToDouble(pStack->Data[pStack->Pointer-1]));
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        Equal(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int not_equal_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.uqw = !CompareDoubleEqual(ToDouble(pStack->Data[pStack->Pointer]), ToDouble(pStack->Data[pStack->Pointer-1]));
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        NotEqual(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int smaller_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.uqw = ToDouble(pStack->Data[pStack->Pointer-1]) < ToDouble(pStack->Data[pStack->Pointer]);
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        Lesser(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int smaller_or_equal_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.uqw = ToDouble(pStack->Data[pStack->Pointer-1]) <= ToDouble(pStack->Data[pStack->Pointer]);
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        LesserOrEqual(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int larger_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.uqw = ToDouble(pStack->Data[pStack->Pointer-1]) > ToDouble(pStack->Data[pStack->Pointer]);
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        Larger(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int larger_or_equal_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.uqw = ToDouble(pStack->Data[pStack->Pointer-1]) >= ToDouble(pStack->Data[pStack->Pointer]);
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        LargerOrEqual(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int plus_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.d = ToDouble(pStack->Data[pStack->Pointer]) + ToDouble(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        AddLimit(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int minus_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.d = ToDouble(pStack->Data[pStack->Pointer-1]) - ToDouble(pStack->Data[pStack->Pointer]);
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        SubtractLimit(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int neg_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    switch (pStack->Data[pStack->Pointer-1].Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        pStack->Data[pStack->Pointer-1].V.qw = -pStack->Data[pStack->Pointer-1].V.qw;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
        break;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        pStack->Data[pStack->Pointer-1].V.qw = -ToInt64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
        break;
    case FLOAT_OR_INT_64_TYPE_F64:
        pStack->Data[pStack->Pointer-1].V.d = -pStack->Data[pStack->Pointer-1].V.d;
        break;
    }
    return 0;
}

int not_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.uqw = !ToBool(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    return 0;
}

int mul_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        pStack->Data[pStack->Pointer-1].V.d = ToDouble(pStack->Data[pStack->Pointer]) * ToDouble(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        MultiplyLimit(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}

int div_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        double Divisor = ToDouble(pStack->Data[pStack->Pointer]);
        if (CompareDoubleEqual(Divisor, 0.0)) {
            double Divident = ToDouble(pStack->Data[pStack->Pointer-1]);
            if (Divident > 0.0) {
                pStack->Data[pStack->Pointer-1].V.d = MAXDOUBLE;
            } else if (Divident < 0) {
                pStack->Data[pStack->Pointer-1].V.d = -MAXDOUBLE;
            } else {
                pStack->Data[pStack->Pointer-1].V.d = 1.0;                // 0/0 = 1 !!!
            }
        } else {
            pStack->Data[pStack->Pointer-1].V.d = ToDouble(pStack->Data[pStack->Pointer-1]) / Divisor;
        }
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        DivideLimit(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}


// ToDo: this should be splitted into 3 functions: int64_value_operation, uint64_value_operation und double_value_operation
int number_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.d = param->param.value;  // current this is always a double value
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer].ByteWidth = 8;
    pStack->Pointer++;
    return 0;
}

int read_bbvari_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    if (pStack->Pointer >= 63) return -1;
    if (pStack->VidReplaceWithFixedValue == param->param.variref.vid) {
        pStack->Data[pStack->Pointer] = pStack->Parameter;
    } else {
        union BB_VARI Value;
        switch (read_bbvari_union_type (param->param.variref.vid, &Value)) {
        case BB_BYTE:
            pStack->Data[pStack->Pointer].V.qw = Value.b;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_INT64;
            pStack->Data[pStack->Pointer].ByteWidth = 1;
            break;
        case BB_UBYTE:
            pStack->Data[pStack->Pointer].V.uqw = Value.ub;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
            pStack->Data[pStack->Pointer].ByteWidth = 1;
            break;
        case BB_WORD:
            pStack->Data[pStack->Pointer].V.qw = Value.w;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_INT64;
            pStack->Data[pStack->Pointer].ByteWidth = 2;
            break;
        case BB_UWORD:
            pStack->Data[pStack->Pointer].V.uqw = Value.uw;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
            pStack->Data[pStack->Pointer].ByteWidth = 2;
            break;
        case BB_DWORD:
            pStack->Data[pStack->Pointer].V.qw = Value.dw;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_INT64;
            pStack->Data[pStack->Pointer].ByteWidth = 4;
            break;
        case BB_UDWORD:
            pStack->Data[pStack->Pointer].V.uqw = Value.udw;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
            pStack->Data[pStack->Pointer].ByteWidth = 4;
            break;
        case BB_QWORD:
            pStack->Data[pStack->Pointer].V.qw = Value.qw;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_INT64;
            pStack->Data[pStack->Pointer].ByteWidth = 8;
            break;
        case BB_UQWORD:
            pStack->Data[pStack->Pointer].V.uqw = Value.uqw;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
            pStack->Data[pStack->Pointer].ByteWidth = 8;
            break;
        case BB_FLOAT:
            pStack->Data[pStack->Pointer].V.d = (double)Value.f;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_F64;
            pStack->Data[pStack->Pointer].ByteWidth = 8;
            break;
        case BB_DOUBLE:
            pStack->Data[pStack->Pointer].V.d = Value.d;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_F64;
            pStack->Data[pStack->Pointer].ByteWidth = 8;
            break;
        default:
            pStack->Data[pStack->Pointer].V.d = 0.0;
            pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_F64;
            pStack->Data[pStack->Pointer].ByteWidth = 8;
            break;
        }
    }
    pStack->Pointer++;
    return 0;
}

int read_bbvari_phys_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    double v;

    if (pStack->Pointer >= 63) return -1;
    v = read_bbvari_equ (param->param.variref.vid);
    pStack->Data[pStack->Pointer].V.d = v;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer].ByteWidth = 8;
    pStack->Pointer++;
    return 0;
}


int write_bbvari_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    if (pStack->Pointer < 1) return -1;
    write_bbvari_convert_with_FloatAndInt64_cs(param->param.variref.vid, pStack->Data[pStack->Pointer-1].V, (int)pStack->Data[pStack->Pointer-1].Type, pStack->cs);
    return 0;
}

int write_bbvari_phys_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    if (pStack->Pointer < 1) return -1;
    if (write_bbvari_phys_minmax_check_cs (param->param.variref.vid, ToDouble(pStack->Data[pStack->Pointer-1]), pStack->cs)) return -2;
    return 0;
}

int sin_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = sin (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int cos_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = cos (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int tan_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = tan (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int sin_hyp_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = sinh (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int cos_hyp_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = cosh (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int tan_hyp_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = tanh (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int asin_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    double In, Out;
    if (pStack->Pointer < 1) return -1;
    In = ToDouble(pStack->Data[pStack->Pointer-1]);
    if (In >= 1.0) Out = M_PI/2.0;
    else if (In >= 1.0) Out = -(M_PI*2.0);
    else Out = asin (In);
    pStack->Data[pStack->Pointer-1].V.d = Out;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int acos_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    double In, Out;
    if (pStack->Pointer < 1) return -1;
    In = ToDouble(pStack->Data[pStack->Pointer-1]);
    if (In >= 1.0) Out = 0.0;
    else if (In >= 1.0) Out = M_PI;
    else Out = acos (In);
    pStack->Data[pStack->Pointer-1].V.d = Out;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int atan_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = atan (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int exp_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = exp (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int pow_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    pStack->Data[pStack->Pointer-1].V.d = pow (ToDouble(pStack->Data[pStack->Pointer-1]), ToDouble(pStack->Data[pStack->Pointer]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int sqrt_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    if (ToDouble(pStack->Data[pStack->Pointer-1]) <= 0.0) pStack->Data[pStack->Pointer-1].V.d = 0.0;
    else pStack->Data[pStack->Pointer-1].V.d = sqrt (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int log_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = log (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int log10_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = log10 (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int get_param_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer] = pStack->Parameter;
    pStack->Pointer++;
    return 0;
}

int getbits_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t start;
    uint64_t size;
    uint64_t mask;

    if (pStack->Pointer < 3) return -1;
    size = ToUint64(pStack->Data[--pStack->Pointer]);
    start = ToUint64(pStack->Data[--pStack->Pointer]);
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (start > 63) start = 63;
    if ((size + start) > 64) size = 64 - start;
    ret = ret >> start;
    if (size < 64) mask = ~(0xFFFFFFFFFFFFFFFFULL << size);
    else mask = 0xFFFFFFFFFFFFFFFFULL;
    ret = ret & mask;
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int setbits_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t start;
    uint64_t size;
    uint64_t b;
    uint64_t mask;

    if (pStack->Pointer < 4) return -1;
    b = ToUint64(pStack->Data[--pStack->Pointer]);
    size = ToUint64(pStack->Data[--pStack->Pointer]);
    start = ToUint64(pStack->Data[--pStack->Pointer]);
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (start > 63) start = 63;
    if ((size + start) > 64) size = 64 - start;
    if (size < 64) b = b & ~(0xFFFFFFFFFFFFFFFFULL << size);
    if ((size + start) < 64) mask = 0xFFFFFFFFFFFFFFFFULL << (size + start);
    else mask = 0;
    mask = mask | ~(0xFFFFFFFFFFFFFFFFULL << start);
    ret = ret & mask;
    ret = ret | (b << start);
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int andbits_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t start;
    uint64_t size;
    uint64_t b;
    uint64_t mask;

    if (pStack->Pointer < 4) return -1;
    b = ToUint64(pStack->Data[--pStack->Pointer]);
    size = ToUint64(pStack->Data[--pStack->Pointer]);
    start = ToUint64(pStack->Data[--pStack->Pointer]);
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (start > 63) start = 63;
    if ((size + start) > 64) size = 64 - start;
    if (size < 64) b = b & ~(0xFFFFFFFFFFFFFFFFULL << size);
    if ((size + start) < 64) mask = (0xFFFFFFFFFFFFFFFFULL << (size + start));
    else mask = 0;
    mask = mask | ~(0xFFFFFFFFFFFFFFFFULL << start);
    ret = ret & ((b << start) | mask);
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int orbits_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t start;
    uint64_t size;
    uint64_t b;

    if (pStack->Pointer < 4) return -1;
    b = ToUint64(pStack->Data[--pStack->Pointer]);
    size = ToUint64(pStack->Data[--pStack->Pointer]);
    start = ToUint64(pStack->Data[--pStack->Pointer]);
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (start > 63) start = 63;
    if ((size + start) > 64) size = 64 - start;
    if (size < 64) b = b & ~(0xFFFFFFFFFFFFFFFFULL << size);
    ret = ret | (b << start);
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int xorbits_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t start;
    uint64_t size;
    uint64_t b;

    if (pStack->Pointer < 4) return -1;
    b = ToUint64(pStack->Data[--pStack->Pointer]);
    size = ToUint64(pStack->Data[--pStack->Pointer]);
    start = ToUint64(pStack->Data[--pStack->Pointer]);
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (start > 63) start = 63;
    if ((size + start) > 64) size = 64 - start;
    if (size < 64) b = b & ~(0xFFFFFFFFFFFFFFFFULL << size);
    ret = ret ^ (b << start);
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int bitwise_and_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t b;

    if (pStack->Pointer < 2) return -1;
    b = ToUint64(pStack->Data[--pStack->Pointer]);
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    ret = ret & b;
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int bitwise_or_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t b;

    if (pStack->Pointer < 2) return -1;
    b = ToUint64(pStack->Data[--pStack->Pointer]);
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    ret = ret | b;
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int bitwise_xor_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t b;
    uint64_t mask;

    if (pStack->Pointer < 2) return -1;
    b = ToUint64(pStack->Data[--pStack->Pointer]);
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (pStack->Data[pStack->Pointer-1].ByteWidth == 8) {
        mask = 0xFFFFFFFFFFFFFFFFULL;
    } else {
        mask = ~(0xFFFFFFFFFFFFFFFFULL << (pStack->Data[pStack->Pointer-1].ByteWidth * 8));
    }
    ret = (ret ^ b) & mask;
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int bitwise_invert_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;
    uint64_t mask;

    if (pStack->Pointer < 1) return -1;
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (pStack->Data[pStack->Pointer-1].ByteWidth == 8) {
        mask = 0xFFFFFFFFFFFFFFFFULL;
    } else {
        mask = ~(0xFFFFFFFFFFFFFFFFULL << (pStack->Data[pStack->Pointer-1].ByteWidth * 8));
    }
    ret = ~ret & mask;
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}


// CAN objects access inside equations (CAN server)

static int can_byte_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;

    if ((pStack->Pointer < 1) || (pStack->CanObject == NULL)) return -1;
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (pStack->CanObject->size <= 1) ret = 0;
    else if (ret >= (uint64_t)pStack->CanObject->size) ret = (uint64_t)pStack->CanObject->size - 1;
    ret = pStack->CanObject->Data[ret];
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int can_word_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;

    if ((pStack->Pointer < 1) || (pStack->CanObject == NULL)) return -1;
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (pStack->CanObject->size <= 3) ret = 0;
    if (ret >= (uint64_t)(pStack->CanObject->size >> 1)) ret = (uint64_t)(pStack->CanObject->size >> 1) - 1;
    ret = ((uint16_t*)(void*)pStack->CanObject->Data)[ret];  // (void*) cast to suppress alignment warning
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int can_dword_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t ret;

    if ((pStack->Pointer < 1) || (pStack->CanObject == NULL)) return -1;
    ret = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (pStack->CanObject->size <= 7) ret = 0;
    if (ret >= (uint64_t)(pStack->CanObject->size >> 2)) ret = (uint64_t)(pStack->CanObject->size >> 2) - 1;
    ret = ((uint32_t*)(void*)pStack->CanObject->Data)[ret]; //  (void*) cast to suppress alignment warning
    pStack->Data[pStack->Pointer-1].V.uqw = ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int can_cyclic_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if ((pStack->Pointer >= 63) || (pStack->CanObject == NULL)) return -1;
    pStack->Data[pStack->Pointer].V.uqw = (pStack->CanObject->should_be_send != 0);
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer].ByteWidth = 8;
    pStack->Pointer++;
    return 0;
}

static int can_data_changed_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    int64_t Start;
    int64_t Size;
    uint64_t Ret;

    if ((pStack->Pointer < 2) || (pStack->CanObject == NULL)) return -1;
    Size = ToInt64(pStack->Data[--pStack->Pointer]);
    Start = ToInt64(pStack->Data[pStack->Pointer-1]);
    if ((Size <= 0) || (Start < 0)) {
        Ret = (uint64_t)memcmp (pStack->CanObject->Data, pStack->CanObject->OldData, (size_t)pStack->CanObject->size);
    } else {
        if (Start < pStack->CanObject->size) {
            if ((Start + Size)  >  pStack->CanObject->size) {
                Size = pStack->CanObject->size - Start;
            }
            Ret = (uint64_t)memcmp (pStack->CanObject->Data + Start, pStack->CanObject->OldData, (size_t)Size);
        } else {
            Ret = 0;   // out of range
        }
    }
    pStack->Data[pStack->Pointer-1].V.uqw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int abs_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    switch (pStack->Data[pStack->Pointer-1].Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        if (pStack->Data[pStack->Pointer-1].V.qw < 0) {
            pStack->Data[pStack->Pointer-1].V.qw = -pStack->Data[pStack->Pointer-1].V.qw;
        }
        break;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        // nothing
        break;
    case FLOAT_OR_INT_64_TYPE_F64:
        pStack->Data[pStack->Pointer-1].V.d = fabs(pStack->Data[pStack->Pointer-1].V.d);
        break;
    }
    return 0;
}

int equal_decimals_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    double v0, v1, range;

    if (pStack->Pointer < 3) return -1;
    range = ToDouble(pStack->Data[--pStack->Pointer]);
    v1 = ToDouble(pStack->Data[--pStack->Pointer]);
    v0 = ToDouble(pStack->Data[pStack->Pointer-1]);
    if (fabs (v0 - v1) > fabs (range)) {
        pStack->Data[pStack->Pointer-1].V.uqw = 0;
    } else {
        pStack->Data[pStack->Pointer-1].V.uqw = 1;
    }
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int swap_on_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    struct STACK_ENTRY Save;

    if (pStack->Pointer < 2) return -1;
    Save = pStack->Data[pStack->Pointer-1];
    pStack->Data[pStack->Pointer-1] = pStack->Data[pStack->Pointer-2];
    pStack->Data[pStack->Pointer-2] = Save;
    return 0;
}

int round_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = floor (ToDouble(pStack->Data[pStack->Pointer-1]) + 0.5);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int round_up_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = ceil (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int round_down_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = floor (ToDouble(pStack->Data[pStack->Pointer-1]));
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int modulo_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    pStack->Pointer--;
    if ((pStack->Data[pStack->Pointer].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        double Divisor = ToDouble(pStack->Data[pStack->Pointer]);
        if (CompareDoubleEqual(Divisor, 0.0)) {
            double Divident = ToDouble(pStack->Data[pStack->Pointer-1]);
            if (Divident > 0.0) {
                pStack->Data[pStack->Pointer-1].V.d = MAXDOUBLE;
            } else if (Divident < 0) {
                pStack->Data[pStack->Pointer-1].V.d = -MAXDOUBLE;
            } else {
                pStack->Data[pStack->Pointer-1].V.d = 1.0;                // 0/0 = 1 !!!
            }
        } else {
            pStack->Data[pStack->Pointer-1].V.d = fmod(ToDouble(pStack->Data[pStack->Pointer-1]), Divisor);
        }
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    } else {
        ModuloLimit(pStack->Data + pStack->Pointer-1, pStack->Data + pStack->Pointer, pStack->Data + pStack->Pointer-1);
    }
    return 0;
}


int add_msb_lsb_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    int64_t Ret;
    
    if (pStack->Pointer < 1) return -1;
    Ret = ToInt64(pStack->Data[pStack->Pointer-1]);
    Ret = (Ret & 0xFF) + ((Ret >> 8) & 0xFF);
    pStack->Data[pStack->Pointer-1].V.qw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int add_msn_lsn_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    int64_t Ret;
    
    if (pStack->Pointer < 1) return -1;
    Ret = ToInt64(pStack->Data[pStack->Pointer-1]);
    Ret = (Ret & 0xF) + ((Ret >> 4) & 0xF);
    pStack->Data[pStack->Pointer-1].V.qw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int overflow_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 3) return -1;
    // Stack-3 -> min,  Stack-2 -> max, Stack-1 -> Wert
    if ((pStack->Data[pStack->Pointer-2].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        double x = ToDouble(pStack->Data[pStack->Pointer-1]);
        double max = ToDouble(pStack->Data[pStack->Pointer-2]);
        if (x > max) {
            // pStack->Data[pStack->Pointer-3] = pStack->Data[pStack->Pointer-3];
        } else {
            pStack->Data[pStack->Pointer-3] = pStack->Data[pStack->Pointer-1];
        }
    } else {
        struct STACK_ENTRY Help;
        Larger(pStack->Data + pStack->Pointer-1 /*x*/, pStack->Data + pStack->Pointer-2 /*max*/, &Help);
        if (Help.V.uqw == 1) {
            // pStack->Data[pStack->Pointer-3] = pStack->Data[pStack->Pointer-3];
        } else {
            pStack->Data[pStack->Pointer-3] = pStack->Data[pStack->Pointer-1];
        }
    }
    pStack->Pointer-=2;
    return 0;
}

int underflow_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 3) return -1;
    // Stack-3 -> min,  Stack-2 -> max, Stack-1 -> Wert
    if ((pStack->Data[pStack->Pointer-2].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        double x = ToDouble(pStack->Data[pStack->Pointer-1]);
        double min = ToDouble(pStack->Data[pStack->Pointer-3]);
        if (x < min) {
            pStack->Data[pStack->Pointer-3] = pStack->Data[pStack->Pointer-2];
        } else {
            pStack->Data[pStack->Pointer-3] = pStack->Data[pStack->Pointer-1];
        }
    } else {
        struct STACK_ENTRY Help;
        Lesser(pStack->Data + pStack->Pointer-1 /*x*/, pStack->Data + pStack->Pointer-3 /*min*/, &Help);
        if (Help.V.uqw == 1) {
            pStack->Data[pStack->Pointer-3] = pStack->Data[pStack->Pointer-2];
        } else {
            pStack->Data[pStack->Pointer-3] = pStack->Data[pStack->Pointer-1];
        }
    }
    pStack->Pointer-=2;
    return 0;
}

int min_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    if ((pStack->Data[pStack->Pointer-2].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        double V1 = ToDouble(pStack->Data[pStack->Pointer-1]);
        double V2 = ToDouble(pStack->Data[pStack->Pointer-2]);
        pStack->Data[pStack->Pointer-2].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-2].ByteWidth = 8;
        if (V1 < V2) {
            pStack->Data[pStack->Pointer-2].V.d = V1;
        } else {
            pStack->Data[pStack->Pointer-2].V.d = V2;
        }
    } else {
        struct STACK_ENTRY Help;
        Lesser(pStack->Data + pStack->Pointer-2, pStack->Data + pStack->Pointer-1, &Help);
        if (Help.V.uqw == 0) {
            pStack->Data[pStack->Pointer-2] = pStack->Data[pStack->Pointer-1];
        }
    }
    pStack->Pointer--;
    return 0;
}

int max_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 2) return -1;
    if ((pStack->Data[pStack->Pointer-2].Type == FLOAT_OR_INT_64_TYPE_F64) ||
        (pStack->Data[pStack->Pointer-1].Type == FLOAT_OR_INT_64_TYPE_F64)) {
        double V1 = ToDouble(pStack->Data[pStack->Pointer-1]);
        double V2 = ToDouble(pStack->Data[pStack->Pointer-2]);
        pStack->Data[pStack->Pointer-2].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-2].ByteWidth = 8;
        if (V1 > V2) {
            pStack->Data[pStack->Pointer-2].V.d = V1;
        } else {
            pStack->Data[pStack->Pointer-2].V.d = V2;
        }
    } else {
        struct STACK_ENTRY Help;
        Larger(pStack->Data + pStack->Pointer-2, pStack->Data + pStack->Pointer-1, &Help);
        if (Help.V.uqw == 0) {
            pStack->Data[pStack->Pointer-2] = pStack->Data[pStack->Pointer-1];
        }
    }
    pStack->Pointer--;
    return 0;
}


static int  crc8_user_poly_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    int Idx;
    unsigned char CrcValue;
    unsigned char Byte_n;

    int IdxBit;
    int Count, Size;
    unsigned char cb_CRC_POLY;
    double CrcValueD;
    double ByteD;
    double cb_CRC_POLYD;

    if (pStack->Pointer < 1) return -1;
    Size = (int)ToInt64(pStack->Data[pStack->Pointer-1]);
    if ((Size < 0) || (pStack->Pointer <= Size)) return -1;
    CrcValueD = ToDouble(pStack->Data[pStack->Pointer-Size-1]);
    cb_CRC_POLYD = ToDouble(pStack->Data[pStack->Pointer-Size]);

    // CRC start value (usually 0xFF)
    if (CrcValueD < 0.0) CrcValue = 0;
    else if (CrcValueD > 255.0) CrcValue = 255;
    else CrcValue = (unsigned char)(CrcValueD + 0.5);

    // CRC polynom value (for example 0x1d)
    if (cb_CRC_POLYD < 0.0) cb_CRC_POLY = 0;
    else if (cb_CRC_POLYD > 255.0) cb_CRC_POLY = 255;
    else cb_CRC_POLY = (unsigned char)(cb_CRC_POLYD + 0.5);

    for (Count = 0; Count < (Size - 2); Count++) {                  /* Calculate CRC of data block */
        Idx = pStack->Pointer - Size + 1 + Count;
        ByteD = ToDouble(pStack->Data[Idx]);
        if (ByteD < 0.0) Byte_n = 0;
        else if (ByteD > 255.0) Byte_n = 255;
        else Byte_n = (unsigned char)(ByteD + 0.5);

        CrcValue ^= Byte_n; 
        for (IdxBit = 0; IdxBit < 8; IdxBit++) { 
            if ((CrcValue & 0x80) != 0) {
                CrcValue = (unsigned char)((CrcValue << 1) ^ cb_CRC_POLY);
            } else {
                CrcValue = (unsigned char)(CrcValue << 1);
            }
        }
    }
    CrcValue = ~CrcValue;
    pStack->Pointer -= (Count + 2); 
    pStack->Data[pStack->Pointer-1].V.uqw = CrcValue;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 1;
    return 0;
}

static int  crc16_user_poly_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    unsigned short CrcTemp;
    unsigned short CrcRet;
    unsigned short CRC_POLY;
    int bits_read = 0, bit_flag;
    int ArgIdx;
    int Count;
    int Size;
    unsigned char Data;
    int i, j;
    double CrcValueD;
    double ByteD;
    double cb_CRC_POLYD;

    if (pStack->Pointer < 1) return -1;
    Size = (int)ToInt64(pStack->Data[pStack->Pointer-1]);
    if ((Size < 0) || (pStack->Pointer <= Size)) return -1;
    CrcValueD = ToDouble(pStack->Data[pStack->Pointer-Size-1]);
    cb_CRC_POLYD = ToDouble(pStack->Data[pStack->Pointer-Size]);
    ByteD = ToDouble(pStack->Data[pStack->Pointer-Size+1]);

    // CRC start value (usually 0xFFFF)
    if (CrcValueD < 0.0) CrcTemp = 0;
    else if (CrcValueD > 65535.0) CrcTemp = 65535;
    else CrcTemp = (unsigned short)(CrcValueD + 0.5);

    // CRC plynom value (for example 0x8005)
    if (cb_CRC_POLYD < 0.0) CRC_POLY = 0;
    else if (cb_CRC_POLYD > 65535.0) CRC_POLY = 65535;
    else CRC_POLY = (unsigned short)(cb_CRC_POLYD + 0.5);

    // First byte
    if (ByteD < 0.0) Data = 0;
    else if (ByteD > 255.0) Data = 255;
    else Data = (unsigned char)(ByteD + 0.5);

    ArgIdx = 1;
    Count = Size - 2;
    while(Count > 0) {
        bit_flag = CrcTemp >> 15;

        /* Get next bit: */
        CrcTemp <<= 1;
        CrcTemp |= (Data >> bits_read) & 1; // item a) work from the least significant bits

        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7) {
            bits_read = 0;
            ArgIdx++;
            Count--;
            // following Bytes
            i = pStack->Pointer - Size + ArgIdx;
            ByteD = ToDouble(pStack->Data[i]);
            if (ByteD < 0.0) Data = 0;
            else if (ByteD > 255.0) Data = 255;
            else Data = (unsigned char)(ByteD + 0.5);
        }

        /* Cycle check: */
        if(bit_flag) {
            CrcTemp ^= CRC_POLY;
        }
    }

    // Push out the last 16 bits
    for (i = 0; i < 16; ++i) {
        bit_flag = CrcTemp >> 15;
        CrcTemp <<= 1;
        if(bit_flag)
            CrcTemp ^= CRC_POLY;
    }

    // Reverse the bits
    CrcRet = 0;
    i = 0x8000;
    j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & CrcTemp) CrcRet |= j;
    }

    pStack->Pointer -= (Count + 2);
    pStack->Data[pStack->Pointer-1].V.uqw = CrcRet;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 2;
    return 0;
}

static uint8_t Reflect8(uint8_t value)
{
    uint8_t Ret = 0;
    int i;

    for (i = 0; i < 8; i++) {
        if ((value & (1 << i)) != 0)  {
            Ret |= (uint8_t)(1UL << (7 - i));
        }
    }
    return Ret;
}

static uint16_t Reflect16(uint16_t value)
{
    uint16_t Ret = 0;
    int i;

    for (i = 0; i < 16; i++) {
        if ((value & (1 << i)) != 0)  {
            Ret |= (uint16_t)(1UL << (15 - i));
        }
    }
    return Ret;
}

static uint32_t Reflect32(uint32_t value)
{
    uint32_t Ret = 0;
    int i;

    for (i = 0; i < 32; i++) {
        if ((value & (1 << i)) != 0)  {
            Ret |= (uint32_t)(1UL << (31 - i));
        }
    }
    return Ret;
}

static int  crc8_user_poly_reflect_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint8_t Crc;
    uint8_t Ret;
    uint8_t Poly;
    uint8_t Xor;
    int Size;
    uint8_t Data;
    int i, j;
    int InputReflected;
    int OutputReflected;
    struct STACK_ENTRY *StackPtr;
    uint64_t Help;

    if (pStack->Pointer < 1) return -1;
    Size = (int)ToUint64(pStack->Data[pStack->Pointer-1]);

    StackPtr = &(pStack->Data[pStack->Pointer-Size-1]);

    // CRC polynom value (example 0x6938392D)
    Help = ToUint64(StackPtr[0]);
    if (Help  > 255U) Poly = 255U;
    else Poly = (uint8_t)Help;

    // CRC start value (usually 0xFFFFFFFF)
    Help = ToUint64(StackPtr[1]);
    if (Help  > 255U) Crc = 255U;
    else Crc = (uint8_t)Help;

    // CRC polynom XOR (example 0xFFFFFFFF)
    Help = ToUint64(StackPtr[2]);
    if (Help  > 255U) Xor = 255U;
    else Xor = (uint8_t)Help;

    if (ToUint64(StackPtr[3]) >= 1) InputReflected = 1;
    else InputReflected = 0;
    if (ToUint64(StackPtr[4]) >= 1) OutputReflected = 1;
    else OutputReflected = 0;

    for (i = 4; i < (Size-1); i++) {
        int Pos = pStack->Pointer - Size + i;
        Help = ToUint64(pStack->Data[Pos]);
        if (Help > 255) Data = 255;
        else Data = (uint8_t)Help;
        if (InputReflected) {
            Data = Reflect8(Data);
        }
        Crc ^= (uint8_t)Data; // << 24; ??
        for (j = 0; j < 8; j++) {
            if ((Crc & 0x80) != 0) {
                Crc = (uint32_t)((Crc << 1) ^ Poly);
            } else {
                Crc <<= 1;
            }
        }
    }
    if (OutputReflected) {
        Crc = Reflect8(Crc);
    }
    Ret = Crc ^ Xor;

    pStack->Pointer -= Size;
    pStack->Data[pStack->Pointer-1].V.uqw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 1;
    return 0;
}

static int  crc16_user_poly_reflect_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint16_t Crc;
    uint16_t Ret;
    uint16_t Poly;
    uint16_t Xor;
    int Size;
    uint8_t Data;
    int i, j;
    int InputReflected;
    int OutputReflected;
    struct STACK_ENTRY *StackPtr;
    uint64_t Help;

    if (pStack->Pointer < 1) return -1;
    Size = (int)ToUint64(pStack->Data[pStack->Pointer-1]);

    StackPtr = &(pStack->Data[pStack->Pointer-Size-1]);

    // CRC polynom value (example 0x6938392D)
    Help = ToUint64(StackPtr[0]);
    if (Help  > 65535U) Poly = 65535U;
    else Poly = (uint16_t)Help;

    // CRC start value (usually 0xFFFFFFFF)
    Help = ToUint64(StackPtr[1]);
    if (Help  > 65535U) Crc = 65535U;
    else Crc = (uint16_t)Help;

    // CRC polynom XOR (example 0xFFFFFFFF)
    Help = ToUint64(StackPtr[2]);
    if (Help  > 65535U) Xor = 65535U;
    else Xor = (uint16_t)Help;

    if (ToUint64(StackPtr[3]) >= 1) InputReflected = 1;
    else InputReflected = 0;
    if (ToUint64(StackPtr[4]) >= 1) OutputReflected = 1;
    else OutputReflected = 0;

    for (i = 4; i < (Size-1); i++) {
        int Pos = pStack->Pointer - Size + i;
        Help = ToUint64(pStack->Data[Pos]);
        if (Help > 255) Data = 255;
        else Data = (uint8_t)Help;
        if (InputReflected) {
            Data = Reflect8(Data);
        }
        Crc ^= (uint16_t)Data << 8;
        for (j = 0; j < 8; j++) {
            if ((Crc & 0x8000) != 0) {
                Crc = (uint16_t)((Crc << 1) ^ Poly);
            } else {
                Crc <<= 1;
            }
        }
    }
    if (OutputReflected) {
        Crc = Reflect16(Crc);
    }
    Ret = Crc ^ Xor;

    pStack->Pointer -= Size;
    pStack->Data[pStack->Pointer-1].V.uqw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 2;
    return 0;
}

static int  crc32_user_poly_reflect_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint32_t Crc;
    uint32_t Ret;
    uint32_t Poly;
    uint32_t Xor;
    int Size;
    uint8_t Data;
    int i, j;
    int InputReflected;
    int OutputReflected;
    struct STACK_ENTRY *StackPtr;
    uint64_t Help;

    if (pStack->Pointer < 1) return -1;
    Size = (int)ToUint64(pStack->Data[pStack->Pointer-1]);

    StackPtr = &(pStack->Data[pStack->Pointer-Size-1]);

    // CRC polynom value (example 0x6938392D)
    Help = ToUint64(StackPtr[0]);
    if (Help  > 4294967295UL) Poly = 4294967295UL;
    else Poly = (uint32_t)Help;

    // CRC start value (usually 0xFFFFFFFF)
    Help = ToUint64(StackPtr[1]);
    if (Help  > 4294967295UL) Crc = 4294967295UL;
    else Crc = (uint32_t)Help;

    // CRC polynom XOR (example 0xFFFFFFFF)
    Help = ToUint64(StackPtr[2]);
    if (Help  > 4294967295UL) Xor = 4294967295UL;
    else Xor = (uint32_t)Help;

    if (ToUint64(StackPtr[3]) >= 1) InputReflected = 1;
    else InputReflected = 0;
    if (ToUint64(StackPtr[4]) >= 1) OutputReflected = 1;
    else OutputReflected = 0;

    for (i = 4; i < (Size-1); i++) {
        int Pos = pStack->Pointer - Size + i;
        Help = ToUint64(pStack->Data[Pos]);
        if (Help > 255) Data = 255;
        else Data = (uint8_t)Help;
        if (InputReflected) {
            Data = Reflect8(Data);
        }
        Crc ^= (uint32_t)Data << 24;
        for (j = 0; j < 8; j++) {
            if ((Crc & 0x80000000) != 0) {
                Crc = (uint32_t)((Crc << 1) ^ Poly);
            } else {
                Crc <<= 1;
            }
        }
    }
    if (OutputReflected) {
        Crc = Reflect32(Crc);
    }
    Ret = Crc ^ Xor;

    pStack->Pointer -= Size;
    pStack->Data[pStack->Pointer-1].V.uqw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 4;
    return 0;
}

static int can_crc8RevInOut_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    unsigned char *DataPtr;
	unsigned char CrcValue, TestByte;
	int ByteCount;
	int BitCount;
    unsigned char Polynom = (unsigned char)ToUint64(pStack->Data[pStack->Pointer-4]);
    unsigned char IdxCrcByte = (unsigned char)ToUint64(pStack->Data[pStack->Pointer-3]);
    unsigned char ReverseInput = (unsigned char)ToUint64(pStack->Data[pStack->Pointer-2]);
    unsigned char ReverseOutput = (unsigned char)ToUint64(pStack->Data[pStack->Pointer-1]);
    CrcValue = 0;
    DataPtr = (unsigned char*)pStack->CanObject->Data;
	for (ByteCount = 0; ByteCount < pStack->CanObject->size; ByteCount++) {
        if (ByteCount != IdxCrcByte) { /*  Surpass CRC byte */
			for (BitCount = 0; BitCount < 8; BitCount++) {
				if (ReverseInput) 
                    TestByte = (unsigned char)((DataPtr[ByteCount] & (1<<BitCount))<<(7-BitCount));
				else
                    TestByte = (unsigned char)((DataPtr[ByteCount] & (1<<(7-BitCount)))<<BitCount);
					
				if (TestByte != (CrcValue & 0x80))
					CrcValue = (unsigned char)(((unsigned int)CrcValue * 2) ^ (unsigned int)Polynom);
				else
					CrcValue = (unsigned char)((unsigned int)CrcValue * 2);
			}
		}
	}
	if (ReverseOutput) {
        TestByte = (unsigned char)(CrcValue << 7);
		for (BitCount = 1; BitCount < 4; BitCount++) {
			TestByte |= (CrcValue & (1 << BitCount)) << (7 - BitCount*2);
		}
		for (BitCount = 4; BitCount < 8; BitCount++) {
			TestByte |= (CrcValue & (1 << BitCount)) >> (BitCount*2 - 7);
		}
		CrcValue = TestByte;
	}
    pStack->Pointer -= 3;
    pStack->Data[pStack->Pointer-1].V.uqw = CrcValue;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 1;
    return 0;
}

int swap16_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    unsigned short Ret;
    
    if (pStack->Pointer < 1) return -1;
    Ret = (unsigned short)ToUint64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].V.uqw = (((Ret << 8) | (Ret >> 8)) & 0xFFFF);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 2;
    return 0;
}

int swap32_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint32_t In;
    uint32_t Ret;
    uint32_t Mask = 0x0000FF00;
    
    if (pStack->Pointer < 1) return -1;
    In = (uint32_t)ToUint64(pStack->Data[pStack->Pointer-1]);
    Ret = In << 24;
	Ret |= (In & Mask) << 8;
	Mask <<= 8;
	Ret |= (In & Mask) >> 8;
	Mask <<= 8;
	Ret |= (In & Mask) >> 24;
    pStack->Data[pStack->Pointer-1].V.uqw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 4;
    return 0;
}

int shift_left_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t In;
    uint64_t Ret;
    uint64_t Bits;

    if (pStack->Pointer < 2) return -1;
    Bits = ToUint64(pStack->Data[--pStack->Pointer]);
    In = ToUint64(pStack->Data[pStack->Pointer-1]);
    if (Bits >= 64) Ret = 0;
    else Ret = In << Bits;
    pStack->Data[pStack->Pointer-1].V.uqw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

int shift_right_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint64_t In;
    uint64_t Ret;
    uint64_t Bits;

    if (pStack->Pointer < 2) return -1;
    Bits = ToUint64(pStack->Data[--pStack->Pointer]);
    In = ToUint64(pStack->Data[pStack->Pointer-1]);
	if (Bits >= 32) Ret = 0;
	else Ret = In >> Bits;
    pStack->Data[pStack->Pointer-1].V.uqw = Ret;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}


static int read_bbvari_ones_complement_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    union BB_VARI value;

    if (pStack->Pointer >= 63) return -1;
    value = read_bbvari_union (param->param.variref.vid);
    switch (get_bbvaritype (param->param.variref.vid)) {
    case BB_BYTE:
    case BB_UBYTE:
        pStack->Data[pStack->Pointer].V.uqw = (~value.ub & 0xFF);
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 1;
        break;
    case BB_WORD: 
    case BB_UWORD:
        pStack->Data[pStack->Pointer].V.uqw = (~value.uw & 0xFFFF);
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 2;
        break;
    case BB_DWORD:
    case BB_UDWORD:
        pStack->Data[pStack->Pointer].V.uqw = (~value.udw);
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 4;
        break;
    case BB_QWORD:
    case BB_UQWORD:
        pStack->Data[pStack->Pointer].V.uqw = (~value.uqw);
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 8;
        break;
    case BB_FLOAT:
        pStack->Data[pStack->Pointer].V.uqw = (~((uint64_t)value.f));
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 8;
        break;
    case BB_DOUBLE:
        pStack->Data[pStack->Pointer].V.uqw = (~((uint64_t)value.d));
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 8;
        break;
    }
    pStack->Pointer++;
    return 0;
}


static int read_bbvari_binary_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    union BB_VARI value;

    if (pStack->Pointer >= 63) return -1;
    value = read_bbvari_union (param->param.variref.vid);
    switch (get_bbvaritype (param->param.variref.vid)) {
    case BB_BYTE:
    case BB_UBYTE:
        pStack->Data[pStack->Pointer].V.uqw = (value.ub);
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 1;
        break;
    case BB_WORD: 
    case BB_UWORD:
        pStack->Data[pStack->Pointer].V.uqw = (value.uw);
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 2;
        break;
    case BB_DWORD:
    case BB_UDWORD:
        pStack->Data[pStack->Pointer].V.uqw = (value.udw);
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 4;
        break;
    case BB_QWORD:
    case BB_UQWORD:
        pStack->Data[pStack->Pointer].V.uqw = (value.uqw);
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 8;
        break;
    case BB_FLOAT:
        pStack->Data[pStack->Pointer].V.uqw = *(uint32_t*)&value.f;
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 4;
        break;
    case BB_DOUBLE:
        pStack->Data[pStack->Pointer].V.uqw = *(uint64_t*)&value.d;
        pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer].ByteWidth = 8;
        break;
    }
    pStack->Pointer++;
    return 0;
}

static int write_bbvari_binary_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    union BB_VARI bb;

    switch (get_bbvaritype (param->param.variref.vid)) {
    case BB_BYTE:
        bb.ub = (uint8_t)ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.uqw = (uint64_t)bb.b;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 1;
        break;
    case BB_UBYTE:
        bb.ub = (uint8_t)ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.uqw = bb.ub;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 1;
        break;
    case BB_WORD:
        bb.uw = (uint16_t)ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.uqw = (uint64_t)bb.w;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 2;
        break;
    case BB_UWORD:
        bb.uw = (uint16_t)ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.uqw = bb.uw;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 2;
        break;
    case BB_DWORD:
        bb.udw = (uint32_t)ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.uqw = (uint64_t)bb.dw;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 4;
        break;
    case BB_UDWORD:
        bb.udw = (uint32_t)ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.uqw = bb.udw;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 4;
        break;
    case BB_QWORD:
        bb.uqw = ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.uqw = (uint64_t)bb.qw;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
        break;
    case BB_UQWORD:
        bb.uqw = ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.uqw = bb.uqw;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
        break;
    case BB_FLOAT:
        bb.udw = (uint32_t)ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.d = (double)bb.f;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
        break;
    case BB_DOUBLE:
        bb.uqw = (uint64_t)ToUint64(pStack->Data[pStack->Pointer-1]);
        pStack->Data[pStack->Pointer-1].V.d = bb.d;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
        break;
    default:
        bb.uqw = 0;
        pStack->Data[pStack->Pointer-1].V.uqw = bb.uqw;
        pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
        pStack->Data[pStack->Pointer-1].ByteWidth = 8;
        break;
    }
    write_bbvari_union (param->param.variref.vid, bb);
    return 0;
}


static int has_changed_byte_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    static int next_function_idx = -1;

    param->param.value = ToDouble(pStack->Data[pStack->Pointer-1]);  // If this is the first call only notice the value inside "Parameter"
    pStack->Data[pStack->Pointer-1].V.d = 0.0;                       // and return 0.0
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    if (next_function_idx < 0) {
        next_function_idx = FindFunctionByKey (EQU_OP_HAS_CHANGED_NEXT);
    }
    if (next_function_idx >= 0) {
        param->op_pos = (short)next_function_idx;                    // And now call the functon below (change the index)
    }
    return 0;
}

static int has_changed_next_byte_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    if (!CompareDoubleEqual(param->param.value, ToDouble(pStack->Data[pStack->Pointer-1]))) { // The value has changed
        param->param.value = ToDouble(pStack->Data[pStack->Pointer-1]);                       // new compare value
        pStack->Data[pStack->Pointer-1].V.d = 1.0;                                            // and return 1.0
    } else {
        pStack->Data[pStack->Pointer-1].V.d = 0.0;
    }
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int slope_up_byte_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    static int next_function_idx = -1;

    param->param.value = ToDouble(pStack->Data[pStack->Pointer-1]);  // If this is the first call only notice the value inside "Parameter"
    pStack->Data[pStack->Pointer-1].V.d = 0.0;                       // and return 0.0
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    if (next_function_idx < 0) {
        next_function_idx = FindFunctionByKey (EQU_OP_SLOPE_UP_NEXT);
    }
    if (next_function_idx >= 0) {
        param->op_pos = (short)next_function_idx;                   // And now call the functon below (change the index)
    }
    return 0;
}

static int slope_up_next_byte_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    if (ToDouble(pStack->Data[pStack->Pointer-1]) > param->param.value) {   // It is larger than the last value
        param->param.value = ToDouble(pStack->Data[pStack->Pointer-1]);     // new compare value
        pStack->Data[pStack->Pointer-1].V.d = 1.0;                          // and return 1.0
    } else {
        param->param.value = ToDouble(pStack->Data[pStack->Pointer-1]);     // new compare value
        pStack->Data[pStack->Pointer-1].V.d = 0.0;
    }
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int slope_down_byte_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    static int next_function_idx = -1;

    param->param.value = ToDouble(pStack->Data[pStack->Pointer-1]);  // If this is the first call only notice the value inside "Parameter"
    pStack->Data[pStack->Pointer-1].V.d = 0.0;                       //and return 0.0
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    if (next_function_idx < 0) {
        next_function_idx = FindFunctionByKey (EQU_OP_SLOPE_DOWN_NEXT);
    }
    if (next_function_idx >= 0) {
        param->op_pos = (short)next_function_idx;                   // And now call the functon below (change the index)
    }
    return 0;
}

static int slope_down_next_byte_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    if (ToDouble(pStack->Data[pStack->Pointer-1]) < param->param.value) {  // It is smaller than the last value
        param->param.value = ToDouble(pStack->Data[pStack->Pointer-1]);    // new compare value
        pStack->Data[pStack->Pointer-1].V.d = 1.0;                         // and return 1.0
    } else {
        param->param.value = ToDouble(pStack->Data[pStack->Pointer-1]);    // new compare value
        pStack->Data[pStack->Pointer-1].V.d = 0.0;
    }
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int event_byte_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (ToDouble(pStack->Data[pStack->Pointer-2]) != 0.0) {
        pStack->Data[pStack->Pointer-2].V.d = 1.0;
        ThrowError (1, "Event");
    } else {
        pStack->Data[pStack->Pointer-2].V.d = 0.0;
    }
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    pStack->Pointer--;
    return 0;
}

static int can_cycles_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint32_t Ret;

    if ((pStack->Pointer >= 63) || (pStack->CanObject == NULL)) return -1;
    Ret = pStack->CanObject->cycles;
    pStack->Data[pStack->Pointer].V.uqw = (uint64_t)Ret;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer].ByteWidth = 8;
    pStack->Pointer++;
    return 0;
}

static int can_size_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    int Ret;

    if ((pStack->Pointer >= 63) || (pStack->CanObject == NULL)) return -1;
    Ret = pStack->CanObject->size; 
    pStack->Data[pStack->Pointer].V.uqw = (uint64_t)Ret;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer].ByteWidth = 8;
    pStack->Pointer++;
    return 0;
}

static int can_id_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    uint32_t Ret;

    if ((pStack->Pointer >= 63) || (pStack->CanObject == NULL)) return -1;
    Ret = pStack->CanObject->id;
    pStack->Data[pStack->Pointer].V.uqw = (uint64_t)Ret;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer].ByteWidth = 8;
    pStack->Pointer++;
    return 0;
}


static int to_double_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.d = ToDouble(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_F64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}


static void limit_to (struct STACK_ENTRY *Elem, int64_t BottomLimit, uint64_t TopLimit)
{
    switch (Elem->Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        if (Elem->V.qw < BottomLimit) Elem->V.qw = BottomLimit;
        else if (TopLimit < 0x7FFFFFFFFFFFFFFFULL) {
            if ((uint64_t)Elem->V.qw > TopLimit) Elem->V.qw = (int64_t)TopLimit;
        }
        break;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        if (Elem->V.uqw > TopLimit) Elem->V.uqw = TopLimit;
        break;
    case FLOAT_OR_INT_64_TYPE_F64:
        if (Elem->V.d < (double)BottomLimit) Elem->V.d = (double)BottomLimit;
        else if (Elem->V.d > (double)TopLimit) Elem->V.d = (double)TopLimit;
        break;
    }
}

static int to_int8_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    limit_to(&(pStack->Data[pStack->Pointer-1]), INT8_MIN, INT8_MAX);
    pStack->Data[pStack->Pointer-1].V.qw = ToInt64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 1;
    return 0;
}

static int to_int16_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    limit_to(&pStack->Data[pStack->Pointer-1], INT16_MIN, INT16_MAX);
    pStack->Data[pStack->Pointer-1].V.qw = ToInt64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 2;
    return 0;
}

static int to_int32_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    limit_to(&pStack->Data[pStack->Pointer-1], INT32_MIN, INT32_MAX);
    pStack->Data[pStack->Pointer-1].V.qw = ToInt64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 4;
    return 0;
}

static int to_int64_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    limit_to(&pStack->Data[pStack->Pointer-1], INT64_MIN, INT64_MAX);
    pStack->Data[pStack->Pointer-1].V.qw = ToInt64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int to_uint8_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    limit_to(&(pStack->Data[pStack->Pointer-1]), 0, UINT8_MAX);
    pStack->Data[pStack->Pointer-1].V.qw = ToInt64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 1;
    return 0;
}

static int to_uint16_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    limit_to(&pStack->Data[pStack->Pointer-1], 0, UINT16_MAX);
    pStack->Data[pStack->Pointer-1].V.qw = ToInt64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 2;
    return 0;
}

static int to_uint32_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    limit_to(&pStack->Data[pStack->Pointer-1], 0, UINT32_MAX);
    pStack->Data[pStack->Pointer-1].V.qw = ToInt64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 4;
    return 0;
}

static int to_uint64_stack_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    limit_to(&pStack->Data[pStack->Pointer-1], 0, UINT64_MAX);
    pStack->Data[pStack->Pointer-1].V.qw = ToInt64(pStack->Data[pStack->Pointer-1]);
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int value_int8_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.qw = param->param.value_i64;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer].ByteWidth = 1;
    pStack->Pointer++;
    return 0;
}

static int value_int16_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.qw = param->param.value_i64;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer].ByteWidth = 2;
    pStack->Pointer++;
    return 0;
}

static int value_int32_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.qw = param->param.value_i64;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer].ByteWidth = 4;
    pStack->Pointer++;
    return 0;
}

static int value_int64_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.qw = param->param.value_i64;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer].ByteWidth = 8;
    pStack->Pointer++;
    return 0;
}

static int value_uint8_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.uqw = param->param.value_ui64;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer].ByteWidth = 1;
    pStack->Pointer++;
    return 0;
}

static int value_uint16_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.uqw = param->param.value_ui64;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer].ByteWidth = 2;
    pStack->Pointer++;
    return 0;
}

static int value_uint32_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.uqw = param->param.value_ui64;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer].ByteWidth = 4;
    pStack->Pointer++;
    return 0;
}

static int value_uint64_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer >= 63) return -1;
    pStack->Data[pStack->Pointer].V.uqw = param->param.value_ui64;
    pStack->Data[pStack->Pointer].Type = FLOAT_OR_INT_64_TYPE_UINT64;
    pStack->Data[pStack->Pointer].ByteWidth = 8;
    pStack->Pointer++;
    return 0;
}

static int get_calc_data_type_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.qw = pStack->Data[pStack->Pointer-1].Type;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static int get_calc_byte_width_operation (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param)
{
    UNUSED(param);
    if (pStack->Pointer < 1) return -1;
    pStack->Data[pStack->Pointer-1].V.qw = pStack->Data[pStack->Pointer-1].ByteWidth;
    pStack->Data[pStack->Pointer-1].Type = FLOAT_OR_INT_64_TYPE_INT64;
    pStack->Data[pStack->Pointer-1].ByteWidth = 8;
    return 0;
}

static OPERATION_TABLE_ELEM OperationPointerTable[512] = {
    { EQU_OP_LOGICAL_AND, logical_and_operation },
    { EQU_OP_LOGICAL_OR, logical_or_operation },
    { EQU_OP_EQUAL, equal_operation },
    { EQU_OP_NOT_EQUAL, not_equal_operation},
    { EQU_OP_SMALLER, smaller_operation},
    { EQU_OP_SMALLER_EQUAL, smaller_or_equal_operation},
    { EQU_OP_LARGER, larger_operation},
    { EQU_OP_LARGER_EQUAL, larger_or_equal_operation},
    { EQU_OP_PLUS, plus_operation},
    { EQU_OP_MINUS, minus_operation},
    { EQU_OP_NEG, neg_operation},
    { EQU_OP_NOT, not_operation},
    { EQU_OP_MUL, mul_operation},
    { EQU_OP_DIV, div_operation},
    { EQU_OP_NUMBER, number_operation},
    { EQU_OP_READ_BBVARI, read_bbvari_operation},
    { EQU_OP_READ_BBVARI_PHYS, read_bbvari_phys_operation},
    { EQU_OP_WRITE_BBVARI, write_bbvari_operation},
    { EQU_OP_WRITE_BBVARI_PHYS, write_bbvari_phys_operation},
    { EQU_OP_SINUS, sin_operation},
    { EQU_OP_COSINUS, cos_operation},
    { EQU_OP_TANGENS, tan_operation},
    { EQU_OP_ARC_SINUS, asin_operation},
    { EQU_OP_ARC_COSINUS, acos_operation},
    { EQU_OP_ARC_TANGENS, atan_operation},
    { EQU_OP_EXP, exp_operation},
    { EQU_OP_POW, pow_operation},
    { EQU_OP_SQRT, sqrt_operation},
    { EQU_OP_LOG, log_operation},
    { EQU_OP_LOG10, log10_operation},
    { EQU_OP_GET_PARAM, get_param_operation},
    { EQU_OP_GETBITS, getbits_operation},
    { EQU_OP_SETBITS, setbits_operation},
    { EQU_OP_ANDBITS, andbits_operation},
    { EQU_OP_ORBITS, orbits_operation},
    { EQU_OP_XORBITS, xorbits_operation},
    { EQU_OP_BITWISE_AND, bitwise_and_operation},
    { EQU_OP_BITWISE_OR, bitwise_or_operation},
    { EQU_OP_BITWISE_XOR, bitwise_xor_operation},
    { EQU_OP_BITWISE_INVERT, bitwise_invert_operation},
    { EQU_OP_CANBYTE, can_byte_operation},
    { EQU_OP_CANWORD, can_word_operation},
    { EQU_OP_CANDWORD, can_dword_operation},
    { EQU_OP_ABS, abs_operation},
    { EQU_OP_ROUND, round_operation},
    { EQU_OP_ROUND_UP, round_up_operation},
    { EQU_OP_ROUND_DOWN, round_down_operation},
    { EQU_OP_MODULO, modulo_operation},
    { EQU_OP_CAN_CYCLIC, can_cyclic_operation},
    { EQU_OP_CAN_DATA_CHANGED, can_data_changed_operation},
    { EQU_OP_ADD_MSB_LSB, add_msb_lsb_operation},
    { EQU_OP_OVERFLOW, overflow_operation},
    { EQU_OP_UNDERFLOW, underflow_operation},
    { EQU_OP_MIN, min_operation},
    { EQU_OP_MAX, max_operation},
    { EQU_OP_SWAP16, swap16_operation},
    { EQU_OP_SWAP32, swap32_operation},
    { EQU_OP_SHIFT_LEFT, shift_left_operation},
    { EQU_OP_SHIFT_RIGHT, shift_right_operation},
    { EQU_OP_SINUS_HYP, sin_hyp_operation},
    { EQU_OP_COSINUS_HYP, cos_hyp_operation},
    { EQU_OP_TANGENS_HYP, tan_hyp_operation},
    { EQU_OP_ONES_COMPLEMENT_BBVARI, read_bbvari_ones_complement_operation},
    { EQU_OP_GET_BINARY_BBVARI, read_bbvari_binary_operation},
    { EQU_OP_SET_BINARY_BBVARI, write_bbvari_binary_operation},
    { EQU_OP_HAS_CHANGED, has_changed_byte_operation},
    { EQU_OP_HAS_CHANGED_NEXT, has_changed_next_byte_operation},
    { EQU_OP_SLOPE_UP, slope_up_byte_operation},
    { EQU_OP_SLOPE_UP_NEXT, slope_up_next_byte_operation},
    { EQU_OP_SLOPE_DOWN, slope_down_byte_operation},
    { EQU_OP_SLOPE_DOWN_NEXT, slope_down_next_byte_operation},
    { EQU_OP_CRC8_USER_POLY, crc8_user_poly_operation},
    { EQU_OP_CAN_ID, can_id_operation},
    { EQU_OP_CAN_SIZE, can_size_operation},
    { EQU_OP_ADD_MSN_LSN, add_msn_lsn_operation},
    { EQU_OP_CAN_CRC8_REV_IN_OUT, can_crc8RevInOut_operation},
    { EQU_OP_CRC16_USER_POLY, crc16_user_poly_operation},
    { EQU_OP_EQUAL_DECIMALS, equal_decimals_operation},
    { EQU_OP_SWAP_ON_STACK, swap_on_stack_operation},
    { EQU_OP_TO_DOUBLE, to_double_stack_operation},
    { EQU_OP_TO_INT8, to_int8_stack_operation},
    { EQU_OP_TO_INT16, to_int16_stack_operation},
    { EQU_OP_TO_INT32, to_int32_stack_operation},
    { EQU_OP_TO_INT64, to_int64_stack_operation},
    { EQU_OP_TO_UINT8, to_uint8_stack_operation},
    { EQU_OP_TO_UINT16, to_uint16_stack_operation},
    { EQU_OP_TO_UINT32, to_uint32_stack_operation},
    { EQU_OP_TO_UINT64, to_uint64_stack_operation},
    { EQU_OP_NUMBER_INT8, value_int8_operation},
    { EQU_OP_NUMBER_INT16, value_int16_operation},
    { EQU_OP_NUMBER_INT32, value_int32_operation},
    { EQU_OP_NUMBER_INT64, value_int64_operation},
    { EQU_OP_NUMBER_UINT8, value_uint8_operation},
    { EQU_OP_NUMBER_UINT16, value_uint16_operation},
    { EQU_OP_NUMBER_UINT32, value_uint32_operation},
    { EQU_OP_NUMBER_UINT64, value_uint64_operation},
    { EQU_OP_GET_CALC_DATA_TYPE, get_calc_data_type_operation},
    { EQU_OP_GET_CALC_DATA_WIDTH, get_calc_byte_width_operation},
    { EQU_OP_CRC8_USER_POLY_REFLECT, crc8_user_poly_reflect_operation},
    { EQU_OP_CRC16_USER_POLY_REFLECT, crc16_user_poly_reflect_operation},
    { EQU_OP_CRC32_USER_POLY_REFLECT, crc32_user_poly_reflect_operation},
    { EQU_OP_CAN_CYCLES,  can_cycles_operation},
    { -1, NULL }
};

static int OperationPointerTableSize = USER_DEFINED_BUILDIN_FUNCTION_OFFSET;
#define OPERATION_POINTER_TABLE_MAX_SIZE  (sizeof (OperationPointerTable) / sizeof (OperationPointerTable[0]))

int FindFunctionByKey (int Key)
{
    int x;
    for (x = 0; x < OperationPointerTableSize; x++) {
        if (OperationPointerTable[x].OperationNo == Key) {
            return x;
        }
    }
    return -1;
}


int add_exec_stack_elem (int command, char *name, union OP_PARAM value,
                         struct EXEC_STACK_ELEM **pExecStack, int cs, int Pid)
{
    OPERATION_TABLE_ELEM *p;
    int x, op_pos = 0;
    struct EXEC_STACK_ELEM *start_exec_stack = *pExecStack;
    int i;
    int ret = 0;

    if (start_exec_stack == NULL) { /* new stack */
        if ((start_exec_stack = (struct EXEC_STACK_ELEM*)my_malloc (2 * sizeof (struct EXEC_STACK_ELEM))) == NULL) {
            return -1;
        }
        start_exec_stack->op_pos = 1;    /* first element */
        start_exec_stack->param.unique_number = 0; /* is not yet registered */
    } else {
        start_exec_stack->op_pos++;      /* addtional element */
        if ((start_exec_stack = (struct EXEC_STACK_ELEM*)
            my_realloc (start_exec_stack, (size_t)(start_exec_stack->op_pos + 1) * sizeof (struct EXEC_STACK_ELEM))) == NULL) {
            *pExecStack = NULL;
            return -1;
        }
    }
    /* search command inside op_fkt_poi_array */
    x = start_exec_stack->op_pos;
    for (i = 0; i < OperationPointerTableSize; i++) {
        p = OperationPointerTable + i;
        if (p->OperationNo == command) {
            start_exec_stack[x].op_pos = (short)op_pos;
            if (name != NULL) {   /* Variable name */
                if ((OperationPointerTable[op_pos].OperationNo == EQU_OP_WRITE_BBVARI) ||
                    (OperationPointerTable[op_pos].OperationNo == EQU_OP_WRITE_BBVARI_PHYS)) {
                    if (value.value == 0.0) {
                        start_exec_stack[x].param.variref.vid = add_bbvari_pid (name, BB_UNKNOWN_DOUBLE, NULL, Pid);
                    } else {
                        start_exec_stack[x].param.variref.vid = add_bbvari_pid (name, BB_UNKNOWN, NULL, Pid);
                        if (start_exec_stack[x].param.variref.vid < 0) {
                            if (value.value == 1.0) {
                                start_exec_stack[x].param.variref.vid = add_bbvari_pid (name, BB_UNKNOWN_DOUBLE, NULL, Pid);
                                ret = 1;  // was added but should throw error message
                            }
                        }
                    }
                    if (start_exec_stack[x].param.variref.vid < 0) {
                        remove_exec_stack (start_exec_stack);
                        *pExecStack = NULL;
                        return -1;
                    }
                    // if physical writing check if variable has a conversion
                    if (OperationPointerTable[op_pos].OperationNo == EQU_OP_WRITE_BBVARI_PHYS) {
                        if (get_bbvari_conversiontype (start_exec_stack[x].param.variref.vid) != 1) {
                            remove_exec_stack_cs (start_exec_stack, cs);
                            *pExecStack = NULL;
                            return -2;
                        }
                    }
                } else { // EQU_OP_READ_BBVARI or EQU_OP_READ_BBVARI_PHYS
                    if (value.value == 0.0) {
                        start_exec_stack[x].param.variref.vid = add_bbvari_pid (name, BB_UNKNOWN_WAIT, NULL, Pid);
                    } else {
                        start_exec_stack[x].param.variref.vid = add_bbvari_pid (name, BB_UNKNOWN, NULL, Pid);
                        if (start_exec_stack[x].param.variref.vid < 0) {
                            if (value.value == 1.0) {
                                start_exec_stack[x].param.variref.vid = add_bbvari_pid (name, BB_UNKNOWN_WAIT, NULL, Pid);
                                ret = 1;  // was added but should throw error message
                            }
                        }
                    }
                    if (start_exec_stack[x].param.variref.vid < 0) {
                        remove_exec_stack_cs (start_exec_stack, cs);
                        *pExecStack = NULL;
                        return -3;
                    }
                    // if physical writing check if variable has a conversion
                    if (OperationPointerTable[op_pos].OperationNo == EQU_OP_READ_BBVARI_PHYS) {
                        if (get_bbvari_conversiontype (start_exec_stack[x].param.variref.vid) != 1) {
                            remove_exec_stack_cs (start_exec_stack, cs);
                            *pExecStack = NULL;
                            return -2;
                        }
                    }
                }
            } else {              /* command without variable */
                start_exec_stack[x].param = value;
            }
            *pExecStack = start_exec_stack;
            return ret;   // 0 all OK or 1 Ok but with error message

        }
        op_pos++;
    }
    remove_exec_stack_cs (start_exec_stack, cs);
    ThrowError (1, "unknown command %i %s[%i]", (int)command, __FILE__, __LINE__);
    *pExecStack = NULL;
    return -1;
}


void remove_exec_stack_cs (struct EXEC_STACK_ELEM *start_exec_stack, int cs)
{
    int x, items;

    if (start_exec_stack != NULL) {
        items = start_exec_stack->op_pos + 1;
        for (x = 1; x < items; x++) {
            /* delete the frame */
            if ((OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI_PHYS) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_ONES_COMPLEMENT_BBVARI) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_GET_BINARY_BBVARI)) {
                /* Free the blackboard access */
                remove_bbvari_unknown_wait_cs (start_exec_stack[x].param.variref.vid, cs);
            } else if ((OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI) ||
                       (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI_PHYS) ||
                       (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_SET_BINARY_BBVARI)) {
                /* Free the blackboard access */
                remove_bbvari_cs (start_exec_stack[x].param.variref.vid, cs);
            }
        }
        DetachRegisterEquation (start_exec_stack->param.unique_number);
        my_free (start_exec_stack);
    }
}

void remove_exec_stack (struct EXEC_STACK_ELEM *start_exec_stack)
{
    remove_exec_stack_cs (start_exec_stack, 1);
}

void remove_exec_stack_for_process_cs (struct EXEC_STACK_ELEM *start_exec_stack, int cs, int pid)
{
    int x, items;

    if (start_exec_stack != NULL) {
        items = start_exec_stack->op_pos + 1;
        for (x = 1; x < items; x++) {
            /* delete the frame */
            if ((OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI_PHYS) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_ONES_COMPLEMENT_BBVARI) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_GET_BINARY_BBVARI)) {
                /* Free the blackboard access */
                remove_bbvari_unknown_wait_for_process_cs (start_exec_stack[x].param.variref.vid, cs, pid);
            } else if ((OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI) ||
                       (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI_PHYS) ||
                       (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_SET_BINARY_BBVARI)) {
                /* Free the blackboard access */
                remove_bbvari_for_process_cs (start_exec_stack[x].param.variref.vid, cs, pid);
            }
        }
        DetachRegisterEquation (start_exec_stack->param.unique_number);
        my_free (start_exec_stack);
    }
}

void remove_exec_stack_for_process (struct EXEC_STACK_ELEM *start_exec_stack, int pid)
{
    remove_exec_stack_for_process_cs (start_exec_stack, 1, pid);
}


int sizeof_exec_stack (struct EXEC_STACK_ELEM *start_exec_stack)
{
    int items;

    if (start_exec_stack != NULL) {
        items = start_exec_stack->op_pos + 1;
        return (items * (int)sizeof (struct EXEC_STACK_ELEM));
    } else return 0;
}

int copy_exec_stack (struct EXEC_STACK_ELEM *dest_exec_stack,
                     struct EXEC_STACK_ELEM *src_exec_stack)
{
    int items;

    if (src_exec_stack == NULL)
        return -1;

    items = src_exec_stack->op_pos + 1;
    MEMCPY (dest_exec_stack, src_exec_stack, (size_t)items * sizeof(struct EXEC_STACK_ELEM));

    return 0;
}

int attach_exec_stack_by_pid(int pid, struct EXEC_STACK_ELEM *exec_stack)
{
    int x, items;

    if (exec_stack == NULL)
        return -1;

    items = exec_stack->op_pos + 1;
    for (x = 1; x < items; x++) {
        if ((OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI) ||
            (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI_PHYS) ||
            (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_SET_BINARY_BBVARI)) {
            /* double the blackboard access */
            attach_bbvari_pid(pid, exec_stack[x].param.variref.vid);
        }
        else if ((OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI) ||
            (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI_PHYS) ||
            (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_ONES_COMPLEMENT_BBVARI) ||
            (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_GET_BINARY_BBVARI)) {
            /* double the blackboard access */
            attach_bbvari_unknown_wait_pid(pid, exec_stack[x].param.variref.vid);
        }
    }
    AttachRegisterEquationPid(pid, exec_stack->param.unique_number);
    return 0;
}

int attach_exec_stack_cs (struct EXEC_STACK_ELEM *exec_stack, int cs)
{
    int x, items;

    if (exec_stack == NULL)
        return -1;

    items = exec_stack->op_pos + 1;
    for (x = 1; x < items; x++) {
        if ((OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI) ||
            (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI_PHYS) ||
            (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_SET_BINARY_BBVARI)) {
            /* double the blackboard access */
            attach_bbvari_cs (exec_stack[x].param.variref.vid, cs);
        } else if ((OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI) ||
                   (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI_PHYS) ||
                   (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_ONES_COMPLEMENT_BBVARI) ||
                   (OperationPointerTable[exec_stack[x].op_pos].OperationNo == EQU_OP_GET_BINARY_BBVARI)) {
            /* double the blackboard access */
            attach_bbvari_unknown_wait_cs (exec_stack[x].param.variref.vid, cs);
        }
    }
    AttachRegisterEquation (exec_stack->param.unique_number);

    return 0;
}

int attach_exec_stack (struct EXEC_STACK_ELEM *exec_stack)
{
    return attach_exec_stack_cs (exec_stack, 1);
}

void detach_exec_stack_cs (struct EXEC_STACK_ELEM *start_exec_stack, int cs)
{
    int x, items;

    if (start_exec_stack != NULL) {
        items = start_exec_stack->op_pos + 1;
        for (x = 1; x < items; x++) {
            /* Delete the frame */
            if ((OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_READ_BBVARI_PHYS) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_ONES_COMPLEMENT_BBVARI) ||
                (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_GET_BINARY_BBVARI)) {
                /* Give free the blackboard access */
                remove_bbvari_unknown_wait_cs (start_exec_stack[x].param.variref.vid, cs);
            } else if ((OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI) ||
                       (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_WRITE_BBVARI_PHYS) ||
                       (OperationPointerTable[start_exec_stack[x].op_pos].OperationNo == EQU_OP_SET_BINARY_BBVARI)) {
                /* Give free the blackboard access */
                remove_bbvari_cs (start_exec_stack[x].param.variref.vid, cs);
            }
        }
        DetachRegisterEquation (start_exec_stack->param.unique_number);
    }
}

void detach_exec_stack (struct EXEC_STACK_ELEM *start_exec_stack)
{
    detach_exec_stack_cs (start_exec_stack, 1);
}


double execute_stack_cs (struct EXEC_STACK_ELEM *start_exec_stack, int cs)
{
    int x, items = start_exec_stack->op_pos + 1;
    EXECUTION_STACK Stack;

    Stack.cs = cs;
    Stack.Pointer = 0;
    Stack.CanObject = NULL;
    Stack.VidReplaceWithFixedValue = 0;
    for (x = 1; x < items; x++) {
        if (OperationPointerTable[start_exec_stack[x].op_pos].OperationPointer (&Stack, &(start_exec_stack[x]))) {
            #ifndef REALTIME
            ThrowError (1, "cannot solve equation");
            #endif
            return 0.0;
        }
    }
    return ToDouble(Stack.Data[0]);
}

double execute_stack (struct EXEC_STACK_ELEM *start_exec_stack)
{
    return execute_stack_cs (start_exec_stack, 1);
}


int execute_stack_ret_err_code_cs (struct EXEC_STACK_ELEM *start_exec_stack, double *pRet, int cs)
{
    int x, items = start_exec_stack->op_pos + 1;
    EXECUTION_STACK Stack;
    int Ret;

    Stack.cs = cs;
    Stack.Pointer = 0;
    Stack.CanObject = NULL;
    Stack.VidReplaceWithFixedValue = 0;
    for (x = 1; x < items; x++) {
        if ((Ret = OperationPointerTable[start_exec_stack[x].op_pos].OperationPointer (&Stack, &(start_exec_stack[x]))) != 0) {
            if (pRet !=NULL) *pRet = 0.0;
            return Ret;
        }
    }
    if (pRet !=NULL) {
        *pRet = ToDouble(Stack.Data[0]);
    }
    return 0;
}


int execute_stack_ret_err_code (struct EXEC_STACK_ELEM *start_exec_stack, double *pRet)
{
    return execute_stack_ret_err_code_cs (start_exec_stack, pRet, 1);
}


double execute_stack_whith_parameter_cs (struct EXEC_STACK_ELEM *start_exec_stack, double parameter, int cs)
{
    int x, items = start_exec_stack->op_pos + 1;
    EXECUTION_STACK Stack;

    Stack.cs = cs;
    Stack.Pointer = 0;
    Stack.CanObject = NULL;
    Stack.VidReplaceWithFixedValue = 0;
    Stack.Parameter.V.d = parameter;
    Stack.Parameter.Type = FLOAT_OR_INT_64_TYPE_F64;
    Stack.Parameter.ByteWidth = 8;
    for (x = 1; x < items; x++) {
        if (OperationPointerTable[start_exec_stack[x].op_pos].OperationPointer (&Stack, &(start_exec_stack[x]))) {
            #ifndef REALTIME
            ThrowError (1, "cannot solve equation");
            #endif
            return 0.0;
        }
    }
    return ToDouble(Stack.Data[0]);
}

double execute_stack_whith_parameter (struct EXEC_STACK_ELEM *start_exec_stack, double parameter)
{
    return execute_stack_whith_parameter_cs (start_exec_stack, parameter, 1);
}

int execute_stack_whith_can_parameter_cs (struct EXEC_STACK_ELEM *start_exec_stack, union FloatOrInt64 parameter, int parameter_type, NEW_CAN_SERVER_OBJECT *CanObject, union FloatOrInt64 *ret_value, int cs)
{
    int x, items = start_exec_stack->op_pos + 1;
    EXECUTION_STACK Stack;

    Stack.cs = cs;
    Stack.Pointer = 0;

    Stack.Parameter.V = parameter;
    Stack.Parameter.Type = (uint32_t)parameter_type;
    Stack.Parameter.ByteWidth = 8;

    Stack.CanObject = CanObject;
    Stack.VidReplaceWithFixedValue = 0;
    for (x = 1; x < items; x++) {
        if (OperationPointerTable[start_exec_stack[x].op_pos].OperationPointer (&Stack, &(start_exec_stack[x]))) {
            ThrowError (1, "cannot solve equation");
            return FLOAT_OR_INT_64_TYPE_INVALID;
        }
    }
    *ret_value = Stack.Data[0].V;
    return (int)Stack.Data[0].Type;
}


int execute_stack_whith_can_parameter (struct EXEC_STACK_ELEM *start_exec_stack, union FloatOrInt64 parameter, int parameter_type, NEW_CAN_SERVER_OBJECT *CanObject, union FloatOrInt64 *ret_value)
{
    return execute_stack_whith_can_parameter_cs (start_exec_stack, parameter, parameter_type, CanObject, ret_value, 1);
}

double execute_stack_replace_variable_with_parameter_cs (struct EXEC_STACK_ELEM *start_exec_stack, int vid, double parameter, int cs)
{
    int x, items = start_exec_stack->op_pos + 1;
    EXECUTION_STACK Stack;

    Stack.cs = cs;
    Stack.Pointer = 0;
    Stack.CanObject = NULL;
    Stack.VidReplaceWithFixedValue = vid;
    Stack.Parameter.V.d = parameter;
    Stack.Parameter.Type = FLOAT_OR_INT_64_TYPE_F64;
    Stack.Parameter.ByteWidth = 8;
    for (x = 1; x < items; x++) {
        if (OperationPointerTable[start_exec_stack[x].op_pos].OperationPointer (&Stack, &(start_exec_stack[x]))) {
            #ifndef REALTIME
            ThrowError (1, "cannot solve equation");
            #endif
            return 0.0;
        }
    }
    return ToDouble(Stack.Data[0]);
}

double execute_stack_replace_variable_with_parameter (struct EXEC_STACK_ELEM *start_exec_stack, int vid, double parameter)
{
    return execute_stack_replace_variable_with_parameter_cs (start_exec_stack, vid, parameter, 1);
}

void init_stack(EXECUTION_STACK *Stack, double Parameter)
{
    Stack->Pointer = 0;
    Stack->cs = 0;
    Stack->NoBBSpinlockFlag = 0;
    Stack->WriteVariableEnable = 1;
    Stack->Parameter.V.d = Parameter;
    Stack->Parameter.Type = FLOAT_OR_INT_64_TYPE_F64;
    Stack->Parameter.ByteWidth = 8;
    Stack->CanObject = NULL;
}

int direct_execute_one_command (int Cmd, EXECUTION_STACK *Stack, union OP_PARAM Value)
{
    int x;
    struct EXEC_STACK_ELEM exec_stack_elem;
    exec_stack_elem.op_pos = (short)Cmd;
    exec_stack_elem.param = Value;
    /* seach command inside op_fkt_poi_array */
    for (x = 0; x < OperationPointerTableSize; x++) {
        if (OperationPointerTable[x].OperationNo == Cmd) {
            return OperationPointerTable[x].OperationPointer(Stack, &(exec_stack_elem));
        }
    }
    return -1;
}

double get_current_stack_value(EXECUTION_STACK *Stack)
{
    if (Stack->Pointer < 1) return 0.0;
    switch (Stack->Data[Stack->Pointer-1].Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        return (double)(Stack->Data[Stack->Pointer-1].V.qw);
    case FLOAT_OR_INT_64_TYPE_UINT64:
        return (double)(Stack->Data[Stack->Pointer-1].V.uqw);
    case FLOAT_OR_INT_64_TYPE_F64:
        return (double)(Stack->Data[Stack->Pointer-1].V.d);
    default:
        return 0.0;
    }
}

int get_current_stack_FloatOrInt64 (EXECUTION_STACK *Stack, union FloatOrInt64 *ret_Value)
{
    if (Stack->Pointer < 1) {
        ret_Value->uqw = 0;
        return FLOAT_OR_INT_64_TYPE_INVALID;
    }
    switch (Stack->Data[Stack->Pointer-1].Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        ret_Value->qw = Stack->Data[Stack->Pointer-1].V.qw;
        return FLOAT_OR_INT_64_TYPE_INT64;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        ret_Value->uqw = Stack->Data[Stack->Pointer-1].V.uqw;
        return FLOAT_OR_INT_64_TYPE_UINT64;
    case FLOAT_OR_INT_64_TYPE_F64:
        ret_Value->d = Stack->Data[Stack->Pointer-1].V.d;
        return FLOAT_OR_INT_64_TYPE_F64;
    default:
        ret_Value->uqw = 0;
        return FLOAT_OR_INT_64_TYPE_INVALID;
    }
}


int add_stack_element_FloatOrInt64 (EXECUTION_STACK *Stack, union FloatOrInt64 Value, int Type, int ByteWidth)
{
    if (Stack->Pointer > 62) return -1;
    Stack->Data[Stack->Pointer].Type = (uint32_t)Type;
    Stack->Data[Stack->Pointer].ByteWidth = (uint32_t)ByteWidth;
    Stack->Data[Stack->Pointer].V = Value;
    Stack->Pointer++;
    return 0;
}


int get_current_stack_union(EXECUTION_STACK *Stack, union BB_VARI *Ret, int *ret_BBType)
{
    if (Stack->Pointer < 1) {
        Ret->d = 0.0;
        *ret_BBType = BB_DOUBLE;
        return -1;
    }
    switch (Stack->Data[Stack->Pointer-1].Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        Ret->qw = Stack->Data[Stack->Pointer-1].V.qw;
        *ret_BBType = BB_QWORD;
        return 0;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        Ret->uqw = Stack->Data[Stack->Pointer-1].V.uqw;
        *ret_BBType = BB_UQWORD;
        return 0;
    case FLOAT_OR_INT_64_TYPE_F64:
        Ret->d = Stack->Data[Stack->Pointer-1].V.d;
        *ret_BBType = BB_DOUBLE;
        return 0;
    default:
        Ret->d = 0.0;
        *ret_BBType = BB_DOUBLE;
        return -1;
    }
}


int convert_to_stack_entry (union BB_VARI Value, int BBType, struct STACK_ENTRY *Ret)
{
    switch (BBType) {
    case BB_BYTE:
        Ret->V.qw = Value.b;
        Ret->Type = FLOAT_OR_INT_64_TYPE_INT64;
        Ret->ByteWidth = 1;
        break;
    case BB_UBYTE:
        Ret->V.uqw = Value.ub;
        Ret->Type = FLOAT_OR_INT_64_TYPE_UINT64;
        Ret->ByteWidth = 1;
        break;
    case BB_WORD:
        Ret->V.qw = Value.w;
        Ret->Type = FLOAT_OR_INT_64_TYPE_INT64;
        Ret->ByteWidth = 2;
        break;
    case BB_UWORD:
        Ret->V.uqw = Value.uw;
        Ret->Type = FLOAT_OR_INT_64_TYPE_UINT64;
        Ret->ByteWidth = 2;
        break;
    case BB_DWORD:
        Ret->V.qw = Value.dw;
        Ret->Type = FLOAT_OR_INT_64_TYPE_INT64;
        Ret->ByteWidth = 4;
        break;
    case BB_UDWORD:
        Ret->V.uqw = Value.udw;
        Ret->Type = FLOAT_OR_INT_64_TYPE_UINT64;
        Ret->ByteWidth = 4;
        break;
    case BB_QWORD:
        Ret->V.qw = Value.qw;
        Ret->Type = FLOAT_OR_INT_64_TYPE_INT64;
        Ret->ByteWidth = 8;
        break;
    case BB_UQWORD:
        Ret->V.uqw = Value.uqw;
        Ret->Type = FLOAT_OR_INT_64_TYPE_UINT64;
        Ret->ByteWidth = 8;
        break;
    case BB_FLOAT:
        Ret->V.d = (double)Value.f;
        Ret->Type = FLOAT_OR_INT_64_TYPE_F64;
        Ret->ByteWidth = 8;
        break;
    case BB_DOUBLE:
        Ret->V.d = Value.d;
        Ret->Type = FLOAT_OR_INT_64_TYPE_F64;
        Ret->ByteWidth = 8;
        break;
    default:
        Ret->V.d = 0.0;
        Ret->Type = FLOAT_OR_INT_64_TYPE_F64;
        Ret->ByteWidth = 8;
        return -1;
    }
    return 0;
}


int AddUserDefinedBuildinFunctionToExecutor(int par_Number, UserDefinedBuildinFunctionExecuteType par_UserDefinedBuildinFunctionExecute)
{
    if (par_Number < OPERATION_POINTER_TABLE_MAX_SIZE) {
        OperationPointerTable[par_Number].OperationPointer = par_UserDefinedBuildinFunctionExecute;
        OperationPointerTable[par_Number].OperationNo = par_Number;
        OperationPointerTableSize++;
    }
}

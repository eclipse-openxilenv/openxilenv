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


#ifndef BBVARI_CONVERSIONS_H
#define BBVARI_CONVERSIONS_H

#include <math.h>   // wegen round

#include "SharedDataTypes.h"

#ifdef _MSC_VER
#if (_MSC_VER < 1900)
// die gibt es bis einschliesslich VC2012 nicht
static __inline double round (double in)
{
    if (in > 0.0) {
        return floor(in + 0.5);
    } else if (in < 0.0) {
        return ceil(in - 0.5);
    } else {
        return 0.0;
    }
}

static __inline float roundf (float in)
{
    if (in > 0.0) {
        return floorf(in + 0.5);
    } else if (in < 0.0) {
        return ceilf(in - 0.5);
    } else {
        return 0.0;
    }
}
#endif
#endif

static __inline int size_of_bbvari (int Type)
{
    switch (Type) {
    case BB_BYTE:
        return 1;
    case BB_UBYTE:
        return 1;
    case BB_WORD:
        return 2;
    case BB_UWORD:
        return 2;
    case BB_DWORD:
        return 4;
    case BB_UDWORD:
        return 4;
    case BB_QWORD:
        return 8;
    case BB_UQWORD:
        return 8;
    case BB_FLOAT:
        return 4;
    case BB_DOUBLE:
        return 8;
    default:
        return -1;
    }
}

// convert to BYTE
static __inline int8_t sc_convert_byte2byte (int8_t b)
{
    return b;
}

static __inline int8_t sc_convert_ubyte2byte (uint8_t ub)
{
    if (ub > 127) return 127;
    else return (int8_t)ub;
}

static __inline int8_t sc_convert_word2byte (int16_t w)
{
    if (w < -128) return -128;
    if (w > 127) return 127;
    else return (int8_t)w;
}

static __inline int8_t sc_convert_uword2byte (uint16_t uw)
{
    if (uw > 127) return 127;
    else return (int8_t)uw;
}

static __inline int8_t sc_convert_dword2byte (int32_t dw)
{
    if (dw < -128) return -128;
    if (dw > 127) return 127;
    else return (int8_t)dw;
}

static __inline int8_t sc_convert_udword2byte (uint32_t udw)
{
    if (udw > 127) return 127;
    else return (int8_t)udw;
}

static __inline int8_t sc_convert_qword2byte (int64_t qw)
{
    if (qw < -128) return -128;
    if (qw > 127) return 127;
    else return (int8_t)qw;
}

static __inline int8_t sc_convert_uqword2byte (uint64_t uqw)
{
    if (uqw > 127) return 127;
    else return (int8_t)uqw;
}

static __inline int8_t sc_convert_float2byte (float f)
{
    if (f < -128.0) return -128;
    if (f > 127.0) return 127;
    else return (int8_t)roundf(f);
}

static __inline int8_t sc_convert_double2byte (double d)
{
    if (d < -128.0) return -128;
    if (d > 127.0) return 127;
    else return (int8_t)round(d);
}

static __inline int sc_convert_2byte (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->b = sc_convert_byte2byte (v->b);
        return 1;
    case BB_UBYTE:
        d->b = sc_convert_ubyte2byte (v->ub);
        return 1;
    case BB_WORD:
        d->b = sc_convert_word2byte (v->w);
        return 2;
    case BB_UWORD:
        d->b = sc_convert_uword2byte (v->uw);
        return 2;
    case BB_DWORD:
        d->b = sc_convert_dword2byte (v->dw);
        return 4;
    case BB_UDWORD:
        d->b = sc_convert_udword2byte (v->udw);
        return 4;
    case BB_QWORD:
        d->b = sc_convert_qword2byte (v->qw);
        return 8;
    case BB_UQWORD:
        d->b = sc_convert_uqword2byte (v->uqw);
        return 8;
    case BB_FLOAT:
        d->b = sc_convert_float2byte (v->f);
        return 4;
    case BB_DOUBLE:
        d->b = sc_convert_double2byte (v->d);
        return 8;
    // no default:
    }
    return 0;
}

// convert to UBYTE
static __inline uint8_t sc_convert_byte2ubyte (int8_t b)
{
    if (b < 0) return 0;
    return (uint8_t)b;
}

static __inline uint8_t sc_convert_ubyte2ubyte (uint8_t ub)
{
    return ub;
}

static __inline uint8_t sc_convert_word2ubyte (int16_t w)
{
    if (w < 0) return 0;
    if (w > 255) return 255;
    else return (uint8_t)w;
}

static __inline uint8_t sc_convert_uword2ubyte (uint16_t uw)
{
    if (uw > 255) return 255;
    else return (uint8_t)uw;
}

static __inline uint8_t sc_convert_dword2ubyte (int32_t dw)
{
    if (dw < 0) return 0;
    if (dw > 255) return 255;
    else return (uint8_t)dw;
}

static __inline uint8_t sc_convert_udword2ubyte (uint32_t udw)
{
    if (udw > 255) return 255;
    else return (uint8_t)udw;
}

static __inline uint8_t sc_convert_qword2ubyte (int64_t qw)
{
    if (qw < 0) return 0;
    if (qw > 255) return 255;
    else return (uint8_t)qw;
}

static __inline uint8_t sc_convert_uqword2ubyte (uint64_t uqw)
{
    if (uqw > 255) return 255;
    else return (uint8_t)uqw;
}

static __inline uint8_t sc_convert_float2ubyte (float f)
{
    if (f < 0.0) return 0;
    if (f > 255.0) return 255;
    else return (uint8_t)roundf(f);
}

static __inline uint8_t sc_convert_double2ubyte (double d)
{
    if (d < 0.0) return 0;
    if (d > 255.0) return 255;
    else return (uint8_t)round(d);
}

static __inline int sc_convert_2ubyte (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->ub = sc_convert_byte2ubyte (v->b);
        return 1;
    case BB_UBYTE:
        d->ub = sc_convert_ubyte2ubyte (v->ub);
        return 1;
    case BB_WORD:
        d->ub = sc_convert_word2ubyte (v->w);
        return 2;
    case BB_UWORD:
        d->ub = sc_convert_uword2ubyte (v->uw);
        return 2;
    case BB_DWORD:
        d->ub = sc_convert_dword2ubyte (v->dw);
        return 4;
    case BB_UDWORD:
        d->ub = sc_convert_udword2ubyte (v->udw);
        return 4;
    case BB_QWORD:
        d->ub = sc_convert_qword2ubyte (v->qw);
        return 4;
    case BB_UQWORD:
        d->ub = sc_convert_uqword2ubyte (v->uqw);
        return 4;
    case BB_FLOAT:
        d->ub = sc_convert_float2ubyte (v->f);
        return 4;
    case BB_DOUBLE:
        d->ub = sc_convert_double2ubyte (v->d);
        return 8;
    }
    return 0;
}


// convert to WORD
static __inline int16_t sc_convert_byte2word (int8_t b)
{
    return (int16_t)b;
}

static __inline int16_t sc_convert_ubyte2word (uint8_t ub)
{
    return (int16_t)ub;
}

static __inline int16_t sc_convert_word2word (int16_t w)
{
    return w;
}

static __inline int16_t sc_convert_uword2word (uint16_t uw)
{
    if (uw > 0x7FFF) return 0x7FFF;
    else return (int16_t)uw;
}

static __inline int16_t sc_convert_dword2word (int32_t dw)
{
    if (dw < -0x8000) return -0x8000;
    if (dw > 0x7FFF) return 0x7FFF;
    else return (int16_t)dw;
}

static __inline int16_t sc_convert_udword2word (uint32_t udw)
{
    if (udw > 0x7FFF) return 0x7FFF;
    else return (int16_t)udw;
}

static __inline int16_t sc_convert_qword2word (int64_t qw)
{
    if (qw < -0x8000) return -0x8000;
    if (qw > 0x7FFF) return 0x7FFF;
    else return (int16_t)qw;
}

static __inline int16_t sc_convert_uqword2word (uint64_t uqw)
{
    if (uqw > 0x7FFF) return 0x7FFF;
    else return (int16_t)uqw;
}

static __inline int16_t sc_convert_float2word (float f)
{
    if (f < -32768.0) return -32768;
    if (f > 32767.0) return 32767;
    else return (int16_t)roundf(f);
}

static __inline int16_t sc_convert_double2word (double d)
{
    if (d < -32768.0) return -32768;
    if (d > 32767.0) return 32767;
    else return (int16_t)round(d);
}

static __inline int sc_convert_2word (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->w = sc_convert_byte2word (v->b);
        return 1;
    case BB_UBYTE:
        d->w = sc_convert_ubyte2word (v->ub);
        return 1;
    case BB_WORD:
        d->w = sc_convert_word2word (v->w);
        return 2;
    case BB_UWORD:
        d->w = sc_convert_uword2word (v->uw);
        return 2;
    case BB_DWORD:
        d->w = sc_convert_dword2word (v->dw);
        return 4;
    case BB_UDWORD:
        d->w = sc_convert_udword2word (v->udw);
        return 4;
    case BB_QWORD:
        d->w = sc_convert_qword2word (v->qw);
        return 8;
    case BB_UQWORD:
        d->w = sc_convert_uqword2word (v->uqw);
        return 8;
    case BB_FLOAT:
        d->w = sc_convert_float2word (v->f);
        return 4;
    case BB_DOUBLE:
        d->w = sc_convert_double2word (v->d);
        return 8;
    // no default:
    }
    return 0;
}


// convert to UWORD
static __inline uint16_t sc_convert_byte2uword (int8_t b)
{
    if (b < 0) return 0;
    return (uint16_t)b;
}

static __inline uint16_t sc_convert_ubyte2uword (uint8_t ub)
{
    return (uint16_t)ub;
}

static __inline uint16_t sc_convert_word2uword (int16_t w)
{
    if (w < 0) return 0;
    else return (uint16_t)w;
}

static __inline uint16_t sc_convert_uword2uword (uint16_t uw)
{
    return uw;
}

static __inline uint16_t sc_convert_dword2uword (int32_t dw)
{
    if (dw < 0) return 0;
    if (dw > 0xFFFF) return 0xFFFF;
    else return (uint16_t)dw;
}

static __inline uint16_t sc_convert_udword2uword (uint32_t udw)
{
    if (udw > 0xFFFF) return 0xFFFF;
    else return (uint16_t)udw;
}

static __inline uint16_t sc_convert_qword2uword (int64_t qw)
{
    if (qw < 0) return 0;
    if (qw > 0xFFFF) return 0xFFFF;
    else return (uint16_t)qw;
}

static __inline uint16_t sc_convert_uqword2uword (uint64_t uqw)
{
    if (uqw > 0xFFFF) return 0xFFFF;
    else return (uint16_t)uqw;
}

static __inline uint16_t sc_convert_float2uword (float f)
{
    if (f < 0.0) return 0;
    if (f > 65535.0) return 65535;
    else return (uint16_t)roundf(f);
}

static __inline uint16_t sc_convert_double2uword (double d)
{
    if (d < 0.0) return 0;
    if (d > 65535.0) return 65535;
    else return (uint16_t)round(d);
}

static __inline int sc_convert_2uword (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->uw = sc_convert_byte2uword (v->b);
        return 1;
    case BB_UBYTE:
        d->uw = sc_convert_ubyte2uword (v->ub);
        return 1;
    case BB_WORD:
        d->uw = sc_convert_word2uword (v->w);
        return 2;
    case BB_UWORD:
        d->uw = sc_convert_uword2uword (v->uw);
        return 2;
    case BB_DWORD:
        d->uw = sc_convert_dword2uword (v->dw);
        return 4;
    case BB_UDWORD:
        d->uw = sc_convert_udword2uword (v->udw);
        return 4;
    case BB_QWORD:
        d->uw = sc_convert_qword2uword (v->qw);
        return 8;
    case BB_UQWORD:
        d->uw = sc_convert_uqword2uword (v->uqw);
        return 8;
    case BB_FLOAT:
        d->uw = sc_convert_float2uword (v->f);
        return 4;
    case BB_DOUBLE:
        d->uw = sc_convert_double2uword (v->d);
        return 8;
    // no default:
    }
    return 0;
}


// convert to DWORD
static __inline int32_t sc_convert_byte2dword (int8_t b)
{
    return (int32_t)b;
}

static __inline int32_t sc_convert_ubyte2dword (uint8_t ub)
{
    return (int32_t)ub;
}

static __inline int32_t sc_convert_word2dword (int16_t w)
{
    return (int32_t)w;
}

static __inline int32_t sc_convert_uword2dword (uint16_t uw)
{
    return (int32_t)uw;
}

static __inline int32_t sc_convert_dword2dword (int32_t dw)
{
    return dw;
}

static __inline int32_t sc_convert_udword2dword (uint32_t udw)
{
    if (udw > 0x7FFFFFFF) return 0x7FFFFFFF;
    else return (int32_t)udw;
}

static __inline int32_t sc_convert_qword2dword (int64_t qw)
{
    if (qw < INT32_MIN) return INT32_MIN;
    if (qw > INT32_MAX) return INT32_MAX;
    return (int32_t)qw;
}

static __inline int32_t sc_convert_uqword2dword (uint64_t uqw)
{
    if (uqw > INT32_MAX) return INT32_MAX;
    else return (int32_t)uqw;
}

static __inline int32_t sc_convert_float2dword (float f)
{
    if (f < -2147483648.0) return 0x80000000UL;
    if (f > 2147483647.0) return 2147483647;
    else return (int32_t)roundf(f);
}

static __inline int32_t sc_convert_double2dword (double d)
{
    if (d < -2147483648.0) return 0x80000000UL;
    if (d > 2147483647.0) return 2147483647;
    else return (int32_t)round(d);
}

static __inline int sc_convert_2dword (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->dw = sc_convert_byte2dword (v->b);
        return 1;
    case BB_UBYTE:
        d->dw = sc_convert_ubyte2dword (v->ub);
        return 1;
    case BB_WORD:
        d->dw = sc_convert_word2dword (v->w);
        return 2;
    case BB_UWORD:
        d->dw = sc_convert_uword2dword (v->uw);
        return 2;
    case BB_DWORD:
        d->dw = sc_convert_dword2dword (v->dw);
        return 4;
    case BB_UDWORD:
        d->dw = sc_convert_udword2dword (v->udw);
        return 4;
    case BB_QWORD:
        d->dw = sc_convert_qword2dword (v->qw);
        return 8;
    case BB_UQWORD:
        d->dw = sc_convert_uqword2dword (v->uqw);
        return 8;
    case BB_FLOAT:
        d->dw = sc_convert_float2dword (v->f);
        return 4;
    case BB_DOUBLE:
        d->dw = sc_convert_double2dword (v->d);
        return 8;
    // no default:
    }
    return 0;
}



// convert to UDWORD
static __inline uint32_t sc_convert_byte2udword (int8_t b)
{
    if (b < 0) return 0;
    return (uint32_t)b;
}

static __inline uint32_t sc_convert_ubyte2udword (uint8_t ub)
{
    return (uint32_t)ub;
}

static __inline uint32_t sc_convert_word2udword (int16_t w)
{
    if (w < 0) return 0;
    else return (uint32_t)w;
}

static __inline uint32_t sc_convert_uword2udword (uint16_t uw)
{
    return uw;
}

static __inline uint32_t sc_convert_dword2udword (int32_t dw)
{
    if (dw < 0) return 0;
    else return (uint32_t)dw;
}

static __inline uint32_t sc_convert_udword2udword (uint32_t udw)
{
    return udw;
}

static __inline uint32_t sc_convert_qword2udword (int64_t qw)
{
    if (qw < 0) return 0;
    if (qw > UINT32_MAX) return UINT32_MAX;
    else return (uint32_t)qw;
}

static __inline uint32_t sc_convert_uqword2udword (uint64_t uqw)
{
    if (uqw > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)uqw;
}


static __inline uint32_t sc_convert_float2udword (float f)
{
    if (f < 0.0) return 0;
    if (f > 4294967295.0) return 4294967295;
    else return (uint32_t)roundf(f);
}

static __inline uint32_t sc_convert_double2udword (double d)
{
    if (d < 0.0) return 0;
    if (d > 4294967295.0) return 4294967295;
    else return (uint32_t)round(d);
}

static __inline int sc_convert_2udword (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->udw = sc_convert_byte2udword (v->b);
        return 1;
    case BB_UBYTE:
        d->udw = sc_convert_ubyte2udword (v->ub);
        return 1;
    case BB_WORD:
        d->udw = sc_convert_word2udword (v->w);
        return 2;
    case BB_UWORD:
        d->udw = sc_convert_uword2udword (v->uw);
        return 2;
    case BB_DWORD:
        d->udw = sc_convert_dword2udword (v->dw);
        return 4;
    case BB_UDWORD:
        d->udw = sc_convert_udword2udword (v->udw);
        return 4;
    case BB_QWORD:
        d->udw = sc_convert_qword2udword (v->qw);
        return 8;
    case BB_UQWORD:
        d->udw = sc_convert_uqword2udword (v->uqw);
        return 8;
    case BB_FLOAT:
        d->udw = sc_convert_float2udword (v->f);
        return 4;
    case BB_DOUBLE:
        d->udw = sc_convert_double2udword (v->d);
        return 8;
    // no default:
    }
    return 0;
}

// convert to QWORD
static __inline int64_t sc_convert_byte2qword (int8_t b)
{
    return (int64_t)b;
}

static __inline int64_t sc_convert_ubyte2qword (uint8_t ub)
{
    return (int64_t)ub;
}

static __inline int64_t sc_convert_word2qword (int16_t w)
{
    return (int64_t)w;
}

static __inline int64_t sc_convert_uword2qword (uint16_t uw)
{
    return (int64_t)uw;
}

static __inline int64_t sc_convert_dword2qword (int32_t dw)
{
    return (int64_t)dw;
}

static __inline int64_t sc_convert_udword2qword (uint32_t udw)
{
    return (int64_t)udw;
}

static __inline int64_t sc_convert_qword2qword (int64_t qw)
{
    return qw;
}

static __inline int64_t sc_convert_uqword2qword (uint64_t uqw)
{
    if (uqw > INT64_MAX) return INT64_MAX;
    return (int64_t)uqw;
}

static __inline int64_t sc_convert_float2qword (float f)
{
    if (f <  -9.223372036854775e18) return INT64_MIN;
    if (f >   9.223372036854775e18) return INT64_MAX;
    return (int64_t)roundf(f);
}

static __inline int64_t sc_convert_double2qword (double d)
{
    if (d < -9.223372036854775e18) return INT64_MIN;
    if (d >  9.223372036854775e18) return INT64_MAX;
    else return (int64_t)round(d);
}

static __inline int sc_convert_2qword (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->qw = sc_convert_byte2qword (v->b);
        return 1;
    case BB_UBYTE:
        d->qw = sc_convert_ubyte2qword (v->ub);
        return 1;
    case BB_WORD:
        d->qw = sc_convert_word2qword (v->w);
        return 2;
    case BB_UWORD:
        d->qw = sc_convert_uword2qword (v->uw);
        return 2;
    case BB_DWORD:
        d->qw = sc_convert_dword2qword (v->dw);
        return 4;
    case BB_UDWORD:
        d->qw = sc_convert_udword2qword (v->udw);
        return 4;
    case BB_QWORD:
        d->qw = sc_convert_qword2qword (v->qw);
        return 8;
    case BB_UQWORD:
        d->qw = sc_convert_uqword2qword (v->uqw);
        return 8;
    case BB_FLOAT:
        d->qw = sc_convert_float2qword (v->f);
        return 4;
    case BB_DOUBLE:
        d->qw = sc_convert_double2qword (v->d);
        return 8;
    // no default:
    }
    return 0;
}



// convert to UQWORD
static __inline uint64_t sc_convert_byte2uqword (int8_t b)
{
    if (b < 0) return 0;
    return (uint64_t)b;
}

static __inline uint64_t sc_convert_ubyte2uqword (uint8_t ub)
{
    return (uint64_t)ub;
}

static __inline uint64_t sc_convert_word2uqword (int16_t w)
{
    if (w < 0) return 0;
    return (uint64_t)w;
}

static __inline uint64_t sc_convert_uword2uqword (uint16_t uw)
{
    return uw;
}

static __inline uint64_t sc_convert_dword2uqword (int32_t dw)
{
    if (dw < 0) return 0;
    return (uint64_t)dw;
}

static __inline uint64_t sc_convert_udword2uqword (uint32_t udw)
{
    return udw;
}

static __inline uint64_t sc_convert_qword2uqword (int64_t qw)
{
    if (qw < 0) return 0;
    return (uint64_t)qw;
}

static __inline uint64_t sc_convert_uqword2uqword (uint64_t uqw)
{
    return uqw;
}


static __inline uint64_t sc_convert_float2uqword (float f)
{
    if (f < 0.0) return 0;
    if (f > (float)UINT64_MAX) return UINT64_MAX;
    return (uint64_t)roundf(f);
}

static __inline uint64_t sc_convert_double2uqword (double d)
{
    if (d < 0.0) return 0;
    if (d > (double)UINT64_MAX) return UINT64_MAX;
    else return (uint64_t)round(d);
}

static __inline int sc_convert_2uqword (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->uqw = sc_convert_byte2uqword (v->b);
        return 1;
    case BB_UBYTE:
        d->uqw = sc_convert_ubyte2uqword (v->ub);
        return 1;
    case BB_WORD:
        d->uqw = sc_convert_word2uqword (v->w);
        return 2;
    case BB_UWORD:
        d->uqw = sc_convert_uword2uqword (v->uw);
        return 2;
    case BB_DWORD:
        d->uqw = sc_convert_dword2uqword (v->dw);
        return 4;
    case BB_UDWORD:
        d->uqw = sc_convert_udword2uqword (v->udw);
        return 4;
    case BB_QWORD:
        d->uqw = sc_convert_qword2uqword (v->qw);
        return 8;
    case BB_UQWORD:
        d->uqw = sc_convert_uqword2uqword (v->uqw);
        return 8;
    case BB_FLOAT:
        d->uqw = sc_convert_float2uqword (v->f);
        return 4;
    case BB_DOUBLE:
        d->uqw = sc_convert_double2uqword (v->d);
        return 8;
    // no default:
    }
    return 0;
}


// convert to FLOAT
static __inline float sc_convert_byte2float (int8_t b)
{
    return (float)b;
}

static __inline float sc_convert_ubyte2float (uint8_t ub)
{
    return (float)ub;
}

static __inline float sc_convert_word2float (int16_t w)
{
    return (float)w;
}

static __inline float sc_convert_uword2float (uint16_t uw)
{
    return (float)uw;
}

static __inline float sc_convert_dword2float (int32_t dw)
{
    return (float)dw;
}

static __inline float sc_convert_udword2float (uint32_t udw)
{
    return (float)udw;
}

static __inline float sc_convert_qword2float (int64_t qw)
{
    return (float)qw;
}

static __inline float sc_convert_uqword2float (uint64_t uqw)
{
    return (float)uqw;
}

static __inline float sc_convert_float2float (float f)
{
    return f;
}

static __inline float sc_convert_double2float (double d)
{
    return (float)d;
}

static __inline int sc_convert_2float (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->f = sc_convert_byte2float (v->b);
        return 1;
    case BB_UBYTE:
        d->f = sc_convert_ubyte2float (v->ub);
        return 1;
    case BB_WORD:
        d->f = sc_convert_word2float (v->w);
        return 2;
    case BB_UWORD:
        d->f = sc_convert_uword2float (v->uw);
        return 2;
    case BB_DWORD:
        d->f = sc_convert_dword2float (v->dw);
        return 4;
    case BB_UDWORD:
        d->f = sc_convert_udword2float (v->udw);
        return 4;
    case BB_QWORD:
        d->f = sc_convert_qword2float (v->qw);
        return 8;
    case BB_UQWORD:
        d->f = sc_convert_uqword2float (v->uqw);
        return 8;
    case BB_FLOAT:
        d->f = sc_convert_float2float (v->f);
        return 4;
    case BB_DOUBLE:
        d->f = sc_convert_double2float (v->d);
        return 8;
        // no default:
    }
    return 0;
}


// convert to DOUBLE
static __inline double sc_convert_byte2double (int8_t b)
{
    return (double)b;
}

static __inline double sc_convert_ubyte2double (uint8_t ub)
{
    return (double)ub;
}

static __inline double sc_convert_word2double (int16_t w)
{
    return (double)w;
}

static __inline double sc_convert_uword2double (uint16_t uw)
{
    return (double)uw;
}

static __inline double sc_convert_dword2double (int32_t dw)
{
    return (double)dw;
}

static __inline double sc_convert_udword2double (uint32_t udw)
{
    return (double)udw;
}

static __inline double sc_convert_qword2double (int64_t qw)
{
    return (double)qw;
}

static __inline double sc_convert_uqword2double (uint64_t uqw)
{
    return (double)uqw;
}

static __inline double sc_convert_float2double (float f)
{
    return (double)f;
}

static __inline double sc_convert_double2double (double d)
{
    return d;
}

static __inline int sc_convert_2double (int Type, union BB_VARI *v, union BB_VARI *d)
{
    switch (Type) {
    case BB_BYTE:
        d->d = sc_convert_byte2double (v->b);
        return 1;
    case BB_UBYTE:
        d->d = sc_convert_ubyte2double (v->ub);
        return 1;
    case BB_WORD:
        d->d = sc_convert_word2double (v->w);
        return 2;
    case BB_UWORD:
        d->d =  sc_convert_uword2double (v->uw);
        return 2;
    case BB_DWORD:
        d->d =  sc_convert_dword2double (v->dw);
        return 4;
    case BB_UDWORD:
        d->d = sc_convert_udword2double (v->udw);
        return 4;
    case BB_QWORD:
        d->d =  sc_convert_qword2double (v->qw);
        return 8;
    case BB_UQWORD:
        d->d = sc_convert_uqword2double (v->uqw);
        return 8;
    case BB_FLOAT:
        d->d = sc_convert_float2double (v->f);
        return 4;
    case BB_DOUBLE:
        d->d = sc_convert_double2double (v->d);
        return 8;
    // no default:
    }
    return 0;
}

static __inline int sc_convert_from_to (int FromType, union BB_VARI *s, int ToType, union BB_VARI *d)
{
    switch (ToType) {
    case BB_BYTE:
        return sc_convert_2byte (FromType, s, d);
    case BB_UBYTE:
        return sc_convert_2ubyte (FromType, s, d);
    case BB_WORD:
        return sc_convert_2word (FromType, s, d);
    case BB_UWORD:
        return sc_convert_2uword (FromType, s, d);
    case BB_DWORD:
        return sc_convert_2dword (FromType, s, d);
    case BB_UDWORD:
        return sc_convert_2udword (FromType, s, d);
    case BB_QWORD:
        return sc_convert_2qword (FromType, s, d);
    case BB_UQWORD:
        return sc_convert_2uqword (FromType, s, d);
    case BB_FLOAT:
        return sc_convert_2float (FromType, s, d);
    case BB_DOUBLE:
        return sc_convert_2double (FromType, s, d);
    default:
        return size_of_bbvari (FromType);  // must give back the size even an error occur
    }
}

static __inline int sc_convert_from_to_0 (int FromType, union BB_VARI *s, int ToType, union BB_VARI *d)
{
    switch (ToType) {
    case BB_BYTE:
        return sc_convert_2byte (FromType, s, d);
    case BB_UBYTE:
        return sc_convert_2ubyte (FromType, s, d);
    case BB_WORD:
        return sc_convert_2word (FromType, s, d);
    case BB_UWORD:
        return sc_convert_2uword (FromType, s, d);
    case BB_DWORD:
        return sc_convert_2dword (FromType, s, d);
    case BB_UDWORD:
        return sc_convert_2udword (FromType, s, d);
    case BB_QWORD:
        return sc_convert_2qword (FromType, s, d);
    case BB_UQWORD:
        return sc_convert_2uqword (FromType, s, d);
    case BB_FLOAT:
        return sc_convert_2float (FromType, s, d);
    case BB_DOUBLE:
        return sc_convert_2double (FromType, s, d);
    default:
        return 0;  // give back 0 if an error occur
    }
}

static __inline double FloatOrInt64_ToDouble(union FloatOrInt64 value, int type)
{
    switch (type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        return (double)value.qw;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        return (double)value.uqw;
    case FLOAT_OR_INT_64_TYPE_F64:
        return value.d;
    default:
        return 0.0;   // sollte nicht passieren!
    }
}

static __inline int64_t FloatOrInt64_ToInt64(union FloatOrInt64 value, int type)
{
    switch (type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        return value.qw;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        if (value.uqw > 0x7FFFFFFFFFFFFFFFllu) return 0x7FFFFFFFFFFFFFFFllu;
        else return (int64_t)value.uqw;
    case FLOAT_OR_INT_64_TYPE_F64:
        if (value.d < INT64_MIN) return INT64_MIN;
        else if (value.d > INT64_MAX) return INT64_MAX;
        else {
            double r = round(value.d);
            return (int64_t)r;
        }
    default:
        return 0;   // sollte nicht passieren!
    }
}

static __inline uint64_t FloatOrInt64_ToUint64(union FloatOrInt64 value, int type)
{
    switch (type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        if (value.qw < 0) return 0;
        else return (uint64_t)value.qw;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        return value.uqw;
    case FLOAT_OR_INT_64_TYPE_F64:
        if (value.d < 0.0) return 0;
        else if (value.d > UINT64_MAX) return UINT64_MAX;
        else {
            double r = round(value.d);
            return (uint64_t)r;
        }
    default:
        return 0;   // sollte nicht passieren!
    }
}

static __inline int FloatOrInt64_ToBool(union FloatOrInt64 value, int type)
{
    switch (type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        return (value.qw != 0);
    case FLOAT_OR_INT_64_TYPE_UINT64:
        return (value.uqw != 0);
    case FLOAT_OR_INT_64_TYPE_F64:
        if ((value.d + 0.5) >= 1.0) return 1;
        if ((value.d - 0.5) <= -1.0) return 1;
        return 0;
    default:
        return 0;   // sollte nicht passieren!
    }
}



static __inline int sc_convert_from_FloatOrInt64_to_BB_VARI (int FromType,  union FloatOrInt64 value, int ToType, union BB_VARI *d)
{
    switch (ToType) {
    case BB_BYTE:
        d->b = sc_convert_qword2byte(FloatOrInt64_ToInt64(value, FromType));
        return 0;
    case BB_UBYTE:
        d->ub  = sc_convert_uqword2ubyte(FloatOrInt64_ToUint64(value, FromType));
        return 0;
    case BB_WORD:
        d->w  = sc_convert_qword2word(FloatOrInt64_ToInt64(value, FromType));
        return 0;
    case BB_UWORD:
        d->uw  = sc_convert_uqword2uword(FloatOrInt64_ToUint64(value, FromType));
        return 0;
    case BB_DWORD:
        d->dw  = sc_convert_qword2dword(FloatOrInt64_ToInt64(value, FromType));
        return 0;
    case BB_UDWORD:
        d->udw  = sc_convert_uqword2udword(FloatOrInt64_ToUint64(value, FromType));
        return 0;
    case BB_QWORD:
        d->qw  = FloatOrInt64_ToInt64(value, FromType);
        return 0;
    case BB_UQWORD:
        d->uqw  = FloatOrInt64_ToUint64(value, FromType);
        return 0;
    case BB_FLOAT:
        d->f  = sc_convert_double2float(FloatOrInt64_ToDouble(value, FromType));
        return 0;
    case BB_DOUBLE:
        d->d  = FloatOrInt64_ToDouble(value, FromType);
        return 0;
    default:
        d->uqw = 0;
        return -1;
    }
}

#endif // BBVARI_CONVERSIONS_H


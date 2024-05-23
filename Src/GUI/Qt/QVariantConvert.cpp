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


#include "QVariantConvert.h"

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
}

QVariant ConvertFloatOrInt64ToQVariant(union FloatOrInt64 par_Value, int par_Type)
{
    switch (par_Type) {
    case 0:
        return QVariant((qlonglong)par_Value.qw);
    case 1:
        return QVariant((qulonglong)par_Value.uqw);
    case 2:
        return QVariant(par_Value.d);
    case 3:
    default:
        return QVariant();
        break;
    }
}

QVariant ConvertA2lSignleValueToQVariant(A2L_SINGLE_VALUE &par_Value)
{
    A2lSignleValue Local(&par_Value);
    QVariant Ret = QVariant::fromValue(Local);
    return Ret;
}

A2lSignleValue ConvertQVariantToA2lSignleValue(QVariant &par_Value)
{
    A2lSignleValue Ret = par_Value.value<A2lSignleValue>();
    return Ret;
}


int ConvertQVariantToFloatOrInt64(const QVariant &par_Value, union FloatOrInt64 *ret_Value)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    switch (par_Value.typeId()) {
#else
    switch (par_Value.type()) {
#endif
    case QVariant::LongLong:
        ret_Value->qw = par_Value.toLongLong();
        return 0; // 0 -> 64 bit integer
    case QVariant::ULongLong:
        ret_Value->uqw = par_Value.toULongLong();
        return 1; // 1 -> 64 bit unsigned integer
    case QVariant::Double:
        ret_Value->d = par_Value.toDouble();
        return 2; // 2 -> 64 bit float
    default:
        return 3;  // undefined (error)
        break;
    }
}

int ConvertA2LValueToFloatOrInt64(A2L_SINGLE_VALUE *par_Value, union FloatOrInt64 *ret_Value)
{
    switch(par_Value->Type) {
    case A2L_ELEM_TYPE_INT:
        ret_Value->qw = par_Value->Value.Int;
        return 0;
    case A2L_ELEM_TYPE_UINT:
        ret_Value->uqw = par_Value->Value.Uint;
        return 1;
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        ret_Value->d = par_Value->Value.Double;
        return 2;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    case A2L_ELEM_TYPE_ERROR:
    default:
        ret_Value->uqw = 0;
        return 3;
    }
}

A2lSignleValue::A2lSignleValue()
{
    m_Value = nullptr;
}

A2lSignleValue::A2lSignleValue(A2L_SINGLE_VALUE *par_Value)
{
    m_Value = (A2L_SINGLE_VALUE*)malloc(par_Value->SizeOfStruct);
    MEMCPY(m_Value, par_Value, par_Value->SizeOfStruct);
}

A2lSignleValue::~A2lSignleValue()
{
    if (m_Value != nullptr) my_free(m_Value);
}

A2lSignleValue::A2lSignleValue(const A2lSignleValue &par_Src)
{
    m_Value = (A2L_SINGLE_VALUE*)my_malloc(par_Src.m_Value->SizeOfStruct);
    MEMCPY(m_Value, &par_Src, par_Src.m_Value->SizeOfStruct);
}

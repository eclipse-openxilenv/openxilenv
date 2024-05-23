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


#pragma once

#include <QVariant>
#include "SharedDataTypes.h"
#include "A2LValue.h"

#include <QMetaType>
#include <QString>


class A2lSignleValue
{
public:
    A2lSignleValue();
    A2lSignleValue(A2L_SINGLE_VALUE *par_Value);
    ~A2lSignleValue();
    A2lSignleValue(const A2lSignleValue& par_Src);
    A2L_SINGLE_VALUE *m_Value;
};

Q_DECLARE_METATYPE(A2lSignleValue)


//Q_DECLARE_METATYPE(A2L_SINGLE_VALUE)

QVariant ConvertFloatOrInt64ToQVariant(union FloatOrInt64 par_Value, int par_Type);
int ConvertQVariantToFloatOrInt64(const QVariant &par_Value, union FloatOrInt64 *ret_Value);
int ConvertA2LValueToFloatOrInt64(A2L_SINGLE_VALUE *par_Value, union FloatOrInt64 *ret_Value);


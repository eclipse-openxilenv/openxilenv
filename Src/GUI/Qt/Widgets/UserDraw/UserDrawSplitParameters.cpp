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


#include "UserDrawSplitParameters.h"
#include "StringHelpers.h"

extern "C" {
#include "ThrowError.h"
}

QStringList SplitParameters(QString &par_ParameterString)
{
    QStringList Ret;
    QString ParameterTrimmed = par_ParameterString.trimmed();
    int Len = ParameterTrimmed.length();
    if ((Len > 0) && (ParameterTrimmed.at(0) != '(') &&  (ParameterTrimmed.at(Len-1) != ')')) {
        ThrowError (1, "expecting a'(' at the beginning and a ')' at the end \"%s\"", QStringToConstChar(ParameterTrimmed));
        return Ret;
    }
    int BraceCounter = 0;
    int StartParameter = 0;
    for (int x = 0; x < Len; x++) {
        QChar c = ParameterTrimmed.at(x);
        if (c == '(') {
            BraceCounter++;
            if (BraceCounter == 1) {
                StartParameter = x;
            }
        } else if (c == ')') {
            if ((BraceCounter == 0) && (x < (Len - 1))) {
                ThrowError (1, "missing '(' in \"%s\"", QStringToConstChar(ParameterTrimmed));
                return Ret;
            }
            if (BraceCounter == 1) {
                QString Parameter = ParameterTrimmed.mid(StartParameter+1, x - StartParameter-1);
                Ret.append(Parameter);
                StartParameter = x;
            }
            BraceCounter--;
        } else if (c == ',') {
            if (BraceCounter == 1) {
                QString Parameter = ParameterTrimmed.mid(StartParameter+1, x - StartParameter-1);
                Ret.append(Parameter);
                StartParameter = x;
            }
        }
    }
    return Ret;
}


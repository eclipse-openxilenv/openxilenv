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


#ifndef USERDRAWPARSER_H
#define USERDRAWPARSER_H

#include <QString>
#include <QColor>
#include <QPainter>

#include "UserDrawElement.h"
#include "UserDrawLayer.h"
#include "UserDrawRoot.h"

extern "C" {
#include "EquationParser.h"
#include "ExecutionStack.h"
}

class UserDrawParser {
public:
    UserDrawRoot *Parse(QString &par_WindowName);
    UserDrawElement *ParseOneLine(QString& par_Line, UserDrawElement *par_Parent);
    bool ParseParameters(QString& par_Line, int par_Pos, UserDrawElement *par_Elem, int par_Len = -1);
    enum DrawItemType { DrawItemTypeLayer, DrawItemTypeGroup }; // maybe all others...
private:
    bool ParseGroupRecusive(UserDrawElement *par_Parent, QString &par_WindowName, QString &par_Entry, int par_Fd);

};


#endif // USERDRAWPARSER_H

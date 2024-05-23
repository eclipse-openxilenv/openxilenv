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


#ifndef USERCONTROLPARSER_H
#define USERCONTROLPARSER_H

#include <QString>
#include <QColor>
#include <QPainter>

#include "UserControlElement.h"
#include "UserControlRoot.h"

extern "C" {
#include "EquationParser.h"
#include "ExecutionStack.h"
}

class UserControlParser {
public:
    UserControlRoot *Parse(QString &par_WindowName, UserControlWidget *par_Widget = nullptr);
    UserControlElement *ParseOneLine(QString& par_Line, UserControlElement *par_Parent);
    bool ParseParameters(QString& par_Line, int par_Pos, UserControlElement *par_Elem, int par_Len = -1);
    enum DrawItemType { DrawItemTypeLayer, DrawItemTypeGroup }; // maybe all others...
private:
    bool ParseGroupRecusive(UserControlElement *par_Parent, QString &par_WindowName, QString &par_Entry, int par_Fd);

};


#endif // USERCONTROLPARSER_H

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


#include "UserControlParser.h"

#include "UserControlRoot.h"
#include "UserControlGroup.h"
#include "UserControlButton.h"

#include "QtIniFile.h"
#include "StringHelpers.h"

extern "C" {
#include "PrintFormatToString.h"
#include "ThrowError.h"
}

UserControlRoot *UserControlParser::Parse(QString &par_WindowName, UserControlWidget *par_Widget)
{
    QString Entry = QString("x");
    QString Empty;
    UserControlRoot *Root = new UserControlRoot(-1, Empty, nullptr);
    Root->SetWidget(par_Widget);
    ParseGroupRecusive(Root, par_WindowName, Entry, ScQt_GetMainFileDescriptor());
    return Root;
}

UserControlElement *UserControlParser::ParseOneLine(QString &par_Line, UserControlElement *par_Parent)
{
    UserControlElement *Elem = nullptr;
    int Pos = UserControlGroup::Is(par_Line);
    if (Pos) {
        Elem = new UserControlGroup(Pos, par_Line, par_Parent);
    } else {
        Pos = UserControlButton::Is(par_Line);
        if (Pos) {
            Elem = new UserControlButton(Pos, par_Line, par_Parent);
        }
    }
    Elem->Init();
    return Elem;
}

bool UserControlParser::ParseParameters(QString &par_Line, int par_Pos, UserControlElement *par_Elem, int par_Len)
{
    QChar c;
    int x;
    if (par_Len < 0) {
        par_Len = par_Line.length();
    }
    for (x = par_Pos; x < par_Len; x++) {
        c = par_Line.at(x);
        if (!c.isSpace()) break;
    }
    if (c != '(') {
        ThrowError (1, "expecting a '(' in \"%s\"", QStringToConstChar(par_Line));
        return false;
    }
    par_Elem->ResetToDefault();
    par_Elem->m_ParameterLine = par_Line.mid(par_Pos);
    int BraceCounter = 0;
    int StartParameter = x;
    for (; x < par_Len; x++) {
        c = par_Line.at(x);
        if (c == '(') {
            BraceCounter++;
            if (BraceCounter == 1) {
                StartParameter = x;
            }
        } else if (c == ')') {
            if ((BraceCounter == 0) && (x < (par_Len - 1))) {
                ThrowError (1, "missing '(' in \"%s\"", QStringToConstChar(par_Line));
                return false;
            }
            if (BraceCounter == 1) {
                QString Parameter = par_Line.mid(StartParameter+1, x - StartParameter-1);
                par_Elem->ParseOneParameter(Parameter);
                StartParameter = x;
            }
            BraceCounter--;
        } else if (c == ',') {
            if (BraceCounter == 1) {
                QString Parameter = par_Line.mid(StartParameter+1, x - StartParameter-1);
                par_Elem->ParseOneParameter(Parameter);
                StartParameter = x;
            }
        }
    }
    return true;
}

bool UserControlParser::ParseGroupRecusive(UserControlElement *par_Parent, QString &par_WindowName,
                                           QString &par_Entry, int par_Fd)
{
    QString Entry;
    QString Line;

    for (int x = 0; x < 10000; x++) {  // max 10000 entrys inside one root/layer/group
        Entry = par_Entry;
        char Help[32];
        PrintFormatToString (Help, sizeof(Help),"_%i", x);
        Entry.append(QString(Help));
        Line = ScQt_IniFileDataBaseReadString(par_WindowName, Entry, "", par_Fd);
        if (Line.isEmpty()) break;
        UserControlElement *Item = ParseOneLine(Line, par_Parent);
        if (Item == nullptr) {
            break;
        }
        par_Parent->AddChild(Item);
        if (Item->IsGroup()) {
            ParseGroupRecusive(Item, par_WindowName, Entry, par_Fd);
        }
    }
    return true;
}


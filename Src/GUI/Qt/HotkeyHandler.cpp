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


#include "HotkeyHandler.h"

#include "MainWindow.h"
#include "StringHelpers.h"

extern "C"
{
    #include "IniDataBase.h"
    #include "MainValues.h"
    #include "EquationParser.h"
    #include "InterfaceToScript.h"
    #include "Scheduler.h"
}

cHotkeyHandler::cHotkeyHandler()
{
    int x, y, i;
    int start, stop;
    char entry[32];
    const char *modifier;
    char txt[INI_MAX_LINE_LENGTH];
    int func;
    char *endptr;
    char line[INI_MAX_LINE_LENGTH];
    QStringList string;
    int Fd = GetMainFileDescriptor();

    for (y = 2; y < 13; y++) {
        for (x = 100; x < 900; x+=200) {
            switch (x) {
            default:
            case 100:
                modifier = "Pure";
                break;
            case 300:
                modifier = "Alt";
                break;
            case 500:
                modifier = "Control";
                break;
            case 700:
                modifier = "Shift";
                break;
            }
            sprintf (entry, "%s F%i", modifier, y);
            IniFileDataBaseReadString ("GUI/FunctionHotkeys", entry, "",
                                       txt, sizeof(txt), Fd);
            func = strtol(txt, &endptr, 0);
            if (modifier[0] == 'P') modifier = "";
            if (modifier[0] == 'A') modifier = "Alt+";
            if (modifier[0] == 'C') modifier = "Ctrl+";
            if (modifier[0] == 'S') modifier = "Shift+";

            if (*endptr == ',')
            {
                endptr++;
                if (func)
                {
                    sprintf (line, "%sF%i", modifier, y);
                    string.append(line);
                    sprintf (line, "%s", TranslateFunctionNumber2String (func));
                    string.append(line);
                    sprintf (line, "%s", endptr);
                    string.append(line);
                    shortcutStringList.append(string);
                    string.clear();
                }
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if (i) {
            start = '0';
            stop = '9';
        } else {
            start = 'a';
            stop = 'z';
        }
        for (y = start; y <= stop; y++) {
            for (x = 0; x < 2; x++) {
                switch (x) {
                default:
                case 0:
                    modifier = "Alt";
                    break;
                case 1:
                    modifier = "Control";
                    break;
                }
                sprintf (entry, "%s %c", modifier, static_cast<char>(y));
                IniFileDataBaseReadString ("GUI/FunctionHotkeys", entry, "",
                                           txt, sizeof(txt), Fd);
                func = strtol(txt, &endptr, 0);
                if (modifier[0] == 'A') modifier = "Alt+";
                if (modifier[0] == 'C') modifier = "Ctrl+";
                if (*endptr == ',') endptr++;
                while (isspace(*endptr)) endptr++;
                if (func)
                {
                    sprintf (line, "%s%c", modifier, static_cast<char>(y-32));
                    string.append(line);
                    sprintf (line, "%s", TranslateFunctionNumber2String (func));
                    string.append(line);
                    sprintf (line, "%s", endptr);
                    string.append(line);
                    shortcutStringList.append(string);
                    string.clear();
                }
            }
        }
    }
}

cHotkeyHandler::~cHotkeyHandler()
{

}

void cHotkeyHandler::set_shortcutStringList(QStringList list)
{
    Q_UNUSED(list);
}

QList <QStringList> *cHotkeyHandler::get_shortcutStringList()
{
    return &shortcutStringList;
}

const char *cHotkeyHandler::TranslateFunctionNumber2String (int nr)
{
    switch (nr) {
    default:
    case 0:
        return "none";
    case 1:
        return "single shot equation";
    case 2:
        return "start script";
    case 3:
        return "stop script";
    case 4:
        return "change sheet";
    case 5:
        return "run control continue";
    case 6:
        return "run control stop";
    case 7:
        return "run control next one";
    case 8:
        return "run control next xxx";
    }
}

int cHotkeyHandler::TranslateString2FunctionNumber (QString string)
{
    if (string == "none") return 0;
    if (string == "single shot equation") return 1;
    if (string == "start script") return 2;
    if (string == "stop script") return 3;
    if (string == "change sheet") return 4;
    if (string == "run control continue") return 5;
    if (string == "run control stop") return 6;
    if (string == "run control next one") return 7;
    if (string == "run control next xxx") return 8;
    return -1;
}

void cHotkeyHandler::addItemToshortcutStringList(QStringList strList)
{
    shortcutStringList.append(strList);
}

QString *cHotkeyHandler::get_shortcutStringListItem(int index, int item)
{
    return &shortcutStringList[index][item];
}

bool cHotkeyHandler::saveHotkeys()
{
    int x, y, i;
    char entry[32];
    const char *stringModifier, *modifier;
    char txt[INI_MAX_LINE_LENGTH + 16];
    char Data[INI_MAX_LINE_LENGTH];
    int func;
    int start, stop;
    int idx = 0;
    bool found = false;
    QString hotkeyEntry;
    QStringList tmp;
    int Fd = GetMainFileDescriptor();

    for (y = 2; y < 13; y++) {
        for (x = 100; x < 900; x+=200) {
            switch (x) {
            default:
            case 100:
                modifier = "Pure";
                stringModifier = "";
                break;
            case 300:
                modifier = "Alt";
                stringModifier = "Alt+";
                break;
            case 500:
                modifier = "Control";
                stringModifier = "Ctrl+";
                break;
            case 700:
                modifier = "Shift";
                stringModifier = "Shift+";
                break;
            }
            sprintf (entry, "%s F%i", modifier, y);
            sprintf(Data, "%sF%i", stringModifier, y);
            hotkeyEntry = QString::fromLocal8Bit(Data);
            idx = 0;
            while (idx < shortcutStringList.count()) {
                tmp = shortcutStringList[idx];
                if (!hotkeyEntry.compare(tmp.at(0), Qt::CaseInsensitive)) {
                    func = TranslateString2FunctionNumber(tmp.at(1));
                    sprintf(Data, "%s", QStringToConstChar(tmp.at(2)));
                    sprintf (txt, "%i,%s", func, Data);
                    IniFileDataBaseWriteString ("GUI/FunctionHotkeys", entry, txt, Fd);
                    found = true;
                    break;
                }
                idx++;
            }
            if (!found) {
                sprintf(txt, "%i,", 0);
                IniFileDataBaseWriteString ("GUI/FunctionHotkeys", entry, txt, Fd);
            }
            found = false;
        }
    }

    for (i = 0; i < 2; i++) {
        if (i) {
            start = '0';
            stop = '9';
        } else {
            start = 'a';
            stop = 'z';
        }
        for (y = start; y <= stop; y++) {
            for (x = 0; x < 2; x++) {
                switch (x) {
                default:
                case 0:
                    modifier = "Alt";
                    stringModifier = "Alt+";
                    break;
                case 1:
                    modifier = "Control";
                    stringModifier = "Ctrl+";
                    break;
                }
                sprintf(Data, "%s%c", stringModifier, static_cast<char>(y-32));
                hotkeyEntry = QString::fromLocal8Bit(Data);
                sprintf (entry, "%s %c", modifier, static_cast<char>(y));
                idx = 0;
                while (idx < shortcutStringList.count()) {
                    tmp = shortcutStringList[idx];
                    if (!hotkeyEntry.compare(tmp.at(0), Qt::CaseInsensitive)) {
                        func = TranslateString2FunctionNumber(tmp.at(1));
                        sprintf(txt, "%i,%*s", func, (int)sizeof(txt)-16, QStringToConstChar(tmp.at(2)));
                        IniFileDataBaseWriteString ("GUI/FunctionHotkeys", entry, txt, Fd);
                        found = true;
                    }
                    idx++;
                }
                if(!found) {
                    sprintf (txt, "%i,", 0);
                    IniFileDataBaseWriteString ("GUI/FunctionHotkeys", entry, txt, Fd);
                }
                found = false;
            }
        }
    }
    return true;
}

void cHotkeyHandler::deleteshortcutStringListItem(int index)
{
    shortcutStringList.removeAt(index);
}


void cHotkeyHandler::register_hotkeys(QWidget *par_MainWindow)
{
    QString sequence, formula;
    QStringList tmp;
    int type;
    int idx = 0;
    clearLists();
    while (idx < shortcutStringList.count())
    {
        tmp = shortcutStringList[idx];
        sequence = tmp.at(0);
        type = TranslateString2FunctionNumber(tmp.at(1));
        formula = tmp.at(2);
        QShortcut *shortcut = new QShortcut(QKeySequence(sequence), par_MainWindow, Q_NULLPTR, Q_NULLPTR, Qt::ApplicationShortcut);
        cSCHotkey *hotkey = new cSCHotkey(type, formula);
        shortcutList.append(shortcut);
        hotkeyList.append(hotkey);
        connect(shortcutList.at(idx), SIGNAL(activated()), hotkeyList.at(idx), SLOT(activateHotkeys()));
        idx++;
    }
}

void cHotkeyHandler::clearLists()
{
    for (int i = 0; i < shortcutList.count(); i++)
    {
        delete shortcutList.at(i);
    }
    for (int i = 0; i < hotkeyList.count(); i++)
    {
        delete hotkeyList.at(i);
    }
    hotkeyList.clear();
    shortcutList.clear();
}

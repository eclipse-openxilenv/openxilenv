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


#ifndef HOTKEYHANDLER_H
#define HOTKEYHANDLER_H

#include <QString>
#include <QStringListModel>
#include <QShortcut>
#include <QObject>
#include "Hotkey.h"


class cHotkeyHandler : public QObject
{
    Q_OBJECT
private:
    QList <QStringList> shortcutStringList;
    QList <QShortcut*> shortcutList;
    QList <cSCHotkey*> hotkeyList;
    void ExecuteCommand (int func, QString formula = nullptr);
    void clearLists();


public:
    cHotkeyHandler();
    ~cHotkeyHandler();
    void set_shortcutStringList(QStringList list);
    void register_hotkeys(QWidget *par_MainWindow);
    QList <QStringList> *get_shortcutStringList();
    QString *get_shortcutStringListItem(int index, int item);
    const char *TranslateFunctionNumber2String(int nr);
    int TranslateString2FunctionNumber (QString string);
    void addItemToshortcutStringList(QStringList strList);
    bool saveHotkeys();
    void deleteshortcutStringListItem(int index);

};

#endif // HOTKEYHANDLER_H

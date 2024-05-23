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


#ifndef CSCHOTKEY_H
#define CSCHOTKEY_H

#include <QString>
#include <QObject>

class cSCHotkey : public QObject
{
    Q_OBJECT
private:
    int type;
    QString formula;
public:
    cSCHotkey(int type, QString formula = nullptr);
    ~cSCHotkey();
public slots:
    void activateHotkeys();
};

#endif // CSCHOTKEY_H

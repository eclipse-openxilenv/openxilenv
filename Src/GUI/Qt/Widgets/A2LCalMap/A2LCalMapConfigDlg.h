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


#ifndef A2LCALMAPCONFIGDLG_H
#define A2LCALMAPCONFIGDLG_H

#include <QDialog>
#include <QComboBox>
#include <Dialog.h>

extern "C" {
    #include "Blackboard.h"
    #include "Wildcards.h"
    #include "DebugInfos.h"
}

namespace Ui {
class A2LCalMapConfigDlg;
}

class A2LCalMapConfigDlg : public Dialog
{
    Q_OBJECT

public:
    explicit A2LCalMapConfigDlg (QString &par_WindowName,
                                 QString &par_ProcessName,
                                 QStringList &par_Characteristics,
                                 QWidget *parent = nullptr);

    ~A2LCalMapConfigDlg();

    QString GetWindowTitle ();
    QStringList GetCharacteristics ();
    QString GetProcess();

private:

    Ui::A2LCalMapConfigDlg *ui;

    QString m_WindowTitle;

public slots:
    //void accept();

private slots:
};

#endif // A2LCalMAPCONFIGDLG_H

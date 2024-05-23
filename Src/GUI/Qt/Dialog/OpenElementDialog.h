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


#ifndef OPENELEMENTDIALOG_H
#define OPENELEMENTDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QMessageBox>
#include <Dialog.h>
extern "C"{
#include "DeleteWindowFromIni.h"
#include "IniDataBase.h"
#include "Blackboard.h"
}

namespace Ui {
class OpenElementDialog;
}

class OpenElementDialog : public Dialog
{
    Q_OBJECT
    
public:
    explicit OpenElementDialog(QString par_WindowIniListName, QString par_WindowType, QWidget *parent = nullptr);
    ~OpenElementDialog();
    QStringList getToOpenWidgetName();
private Q_SLOTS:
    void deleteWidgetfromIni();
    void LoadWindowNamesWidgetList();
private:
    Ui::OpenElementDialog *ui;
    QString m_WindowIniListName;
    QString m_WindowType;
};

#endif // OPENELEMENTDIALOG_H

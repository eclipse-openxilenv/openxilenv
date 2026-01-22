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


#include "OpenElementDialog.h"
#include "ui_OpenElementDialog.h"

#include "IsWindowOpen.h"
#include "StringHelpers.h"

extern "C" {
#include "PrintFormatToString.h"
#include "IniDataBase.h"
}

OpenElementDialog::OpenElementDialog(QString par_WindowIniListName, QString par_WindowType, QWidget *parent) : Dialog(parent),
    ui(new Ui::OpenElementDialog)
{
    ui->setupUi(this);

    m_WindowIniListName = par_WindowIniListName;
    m_WindowType = par_WindowType;

    ui->d_labelWindowName->setText (QString ("open ").append (m_WindowType));

    ui->d_listWidgetNames->setAlternatingRowColors (true);
    ui->d_listWidgetNames->setSelectionMode (QAbstractItemView::ExtendedSelection);

    LoadWindowNamesWidgetList();
}

void OpenElementDialog::LoadWindowNamesWidgetList()
{
    ui->d_listWidgetNames->clear();
    for(int i = 0; ; i++) {
        char Entry[64];
        char WindowName[INI_MAX_LINE_LENGTH];
        PrintFormatToString (Entry, sizeof(Entry), "W%i", i);
        if (IniFileDataBaseReadString(QStringToConstChar(m_WindowIniListName), Entry, "", WindowName, sizeof (WindowName),
                                      GetMainFileDescriptor()) == 0) {
            break;
        }
        // Only list elments that are not open
        QString WindowNameString = CharToQString(WindowName);
        if (!IsWindowOpen (WindowNameString, true)) {
            ui->d_listWidgetNames->addItem(CharToQString(WindowName));
        }
    }
}

QStringList OpenElementDialog::getToOpenWidgetName()
{
    QStringList StringList;
    foreach (QListWidgetItem *Item, ui->d_listWidgetNames->selectedItems()) {
        StringList << Item->text();
    }
    return StringList;
}


void OpenElementDialog::deleteWidgetfromIni()
{
    foreach (QListWidgetItem *Item, ui->d_listWidgetNames->selectedItems()) {
        DeleteWindowFromIniFile (QStringToConstChar(Item->text()), QStringToConstChar(m_WindowIniListName));
    }
    LoadWindowNamesWidgetList();
}

OpenElementDialog::~OpenElementDialog()
{
    delete ui;
}

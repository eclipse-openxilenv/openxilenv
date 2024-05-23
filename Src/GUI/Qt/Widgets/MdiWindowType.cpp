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


#include "MdiWindowType.h"
#include "IsWindowOpen.h"

#include "WindowNameAlreadyInUse.h"
#include "StringHelpers.h"

#include "QtIniFile.h"

MdiWindowType::MdiWindowType(UpdateRate par_UpdateRate, QString par_WindowTypeName,
                                           QString par_WindowIniListName, QString par_MenuEntryName, QString par_DefaultWindowTitle,
                                           QObject *parent,
                                           QIcon par_Icon,
                                           int par_MinWidth, int par_MinHeight,
                                           int par_DefaultWidth, int par_DefaultHeight, bool par_StorageFlag) : QObject(parent)
{
    m_UpdateRate = par_UpdateRate;
    m_WindowTypeName = par_WindowTypeName;
    m_WindowIniListName = par_WindowIniListName;
    m_MenuEntryName = par_MenuEntryName;
    m_DefaultWindowTitle = par_DefaultWindowTitle;

    if (par_MinWidth > 10) m_MinWidth = par_MinWidth;
    else m_MinWidth = 10;
    if (par_MinHeight > 20) m_MinHeight = par_MinHeight;
    else m_MinHeight = 20;
    m_DefaultWidth = par_DefaultWidth;
    m_DefaultHeight = par_DefaultHeight;

    m_Icon = par_Icon;

    m_StorageFlag = par_StorageFlag;
}

MdiWindowType::~MdiWindowType()
{
}


void MdiWindowType::GetWindowTypeInfos(enum UpdateRate *ret_UpdateRate,
                                      QString *ret_WindowTypeName,
                                      QString *ret_WindowIniListName,
                                      QString *ret_MenuEntryName)
{
    *ret_UpdateRate = m_UpdateRate;
    *ret_WindowTypeName = m_WindowTypeName;
    *ret_WindowIniListName = m_WindowIniListName;
    *ret_MenuEntryName = m_MenuEntryName;

}

QString MdiWindowType::GetMenuEntryName() {
    return m_MenuEntryName;
}

QString MdiWindowType::GetWindowIniListName() {
    return m_WindowIniListName;
}

QString MdiWindowType::GetWindowTypeName() {
    return m_WindowTypeName;
}

InterfaceWidgetPlugin::UpdateRate MdiWindowType::GetUpdateRate() {
    return m_UpdateRate;
}

QSize MdiWindowType::GetDefaultSize()
{
    return QSize (m_DefaultWidth, m_DefaultHeight);
}

QSize MdiWindowType::GetMinSize()
{
    return QSize (m_MinWidth, m_MinHeight);
}

QIcon MdiWindowType::GetIcon()
{
    return m_Icon;
}

bool MdiWindowType::GetStorageFlag()
{
    return m_StorageFlag;
}



QString MdiWindowType::GetDefaultNewWindowTitle ()
{
    int x;
    QString WindowName;

    for (x = 1; ; x++) {
        WindowName = m_DefaultWindowTitle;
        WindowName.append(QString(" (%1)").arg(x));
        if (!WindowNameAlreadyInUse (WindowName)) break;
    }
    return WindowName;
}


void MdiWindowType::AddNewWidget (MdiWindowWidget *par_NewWidget)
{
    m_AllWidgetOfThisType.append(par_NewWidget);
}

void MdiWindowType::RemoveWidget (MdiWindowWidget *par_NewWidget)
{
    m_AllWidgetOfThisType.removeAll(par_NewWidget);
}

bool MdiWindowType::AddWindowToAllOfTypeIniList(MdiWindowWidget *par_Widget)
{
    bool Found = false;
    int Fd = ScQt_GetMainFileDescriptor();
    QString WindowTitle = par_Widget->GetWindowTitle();
    QString IniSectionPath = par_Widget->GetIniSectionPath();
    QString WindowTypeName = par_Widget->GetMdiWindowType()->GetWindowTypeName();
    QString Entry;
    for (int y = 0; ; y++) {
        Entry = QString("W%1").arg(y);
        QString Title = ScQt_IniFileDataBaseReadString (m_WindowIniListName, Entry, "", Fd);
        if (Title.isEmpty()) {
            break;
        }
        if (!WindowTitle.compare(Title)) {
            Found = true;
            break;
        }
    }
    if (!Found) {
        ScQt_IniFileDataBaseWriteString (m_WindowIniListName, Entry, WindowTitle, Fd);
        // If window are not inside INI file write the section with with window type to the INI file
        if (!ScQt_IniFileDataBaseLookIfSectionExist(IniSectionPath, Fd)) {
            ScQt_IniFileDataBaseWriteString (IniSectionPath, "type",
                                             WindowTypeName, Fd);
        }
    }
    return !Found;  // true if successful added
}

void MdiWindowType::RemoveWindowFromAllOfTypeIniList(MdiWindowWidget *par_Widget)
{
    bool Found = false;
    int y;
    int Fd = ScQt_GetMainFileDescriptor();

    QString WindowTitle = par_Widget->GetWindowTitle();
    for (y = 0; ; y++) {
        QString Entry = QString("W%1").arg(y);
        QString Title = ScQt_IniFileDataBaseReadString (m_WindowIniListName, Entry, "", Fd);
        if (Title.isEmpty()) {
            break;
        }
        if (!Found) {
            if (!WindowTitle.compare(Title)) {
                Found = true;
                break;
            }
        } else {
            Entry = QString("W%1").arg(y - 1);
            ScQt_IniFileDataBaseWriteString (m_WindowIniListName, Entry, Title, Fd);
        }
    }
    // If found delete the las entry
    if (Found) {
        QString Entry = QString("W%1").arg(y);
        ScQt_IniFileDataBaseWriteString (m_WindowIniListName, Entry, nullptr, Fd);
    }
}

QList<MdiWindowWidget *> MdiWindowType::GetAllOpenWidgetsOfThisType()
{
    return m_AllWidgetOfThisType;
}

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


#ifndef MDIWINDOWTYPE_H
#define MDIWINDOWTYPE_H

#include <QWidget>

#include "InterfaceWidgetPlugin.h"

#include <QIcon>

class MdiWindowType : public QObject, public InterfaceWidgetPlugin
{
    Q_OBJECT
    Q_INTERFACES(InterfaceWidgetPlugin)
public:
    explicit MdiWindowType(enum UpdateRate par_UpdateRate,
                                  QString par_WindowTypeName,
                                  QString par_WindowIniListName,
                                  QString par_MenuEntryName, QString par_DefaultWindowTitle, QObject *parent = nullptr,
                                  QIcon par_Icon = QIcon(),
                                  int par_MinWidth = 0, int par_MinHeight = 0,
                                  int par_DefaultWidth = 10, int par_DefaultHeight = 10,
                                  bool par_StorageFlag = false);
    ~MdiWindowType();

    void GetWindowTypeInfos(enum UpdateRate *ret_UpdateRate,
                            QString *ret_WindowTypeName,
                            QString *ret_WindowIniListName,
                            QString *ret_MenuEntryName);
    QString GetMenuEntryName();
    QString GetWindowIniListName();
    QString GetWindowTypeName();
    InterfaceWidgetPlugin::UpdateRate GetUpdateRate();
    QSize GetDefaultSize();
    QSize GetMinSize();
    QIcon GetIcon();
    bool GetStorageFlag();

    QString GetDefaultNewWindowTitle ();

    void AddNewWidget (MdiWindowWidget *par_NewWidget);
    void RemoveWidget (MdiWindowWidget *par_NewWidget);
    bool AddWindowToAllOfTypeIniList (MdiWindowWidget *par_Widget);
    void RemoveWindowFromAllOfTypeIniList (MdiWindowWidget *par_Widget);

    QList<MdiWindowWidget*> GetAllOpenWidgetsOfThisType ();

signals:

public slots:

private:
    enum UpdateRate m_UpdateRate;
    QString m_WindowTypeName;
    QString m_WindowIniListName;
    QString m_MenuEntryName;
    QString m_DefaultWindowTitle;

    int m_MinWidth;
    int m_MinHeight;
    int m_DefaultWidth;
    int m_DefaultHeight;

    QIcon m_Icon;

    QList <MdiWindowWidget*> m_AllWidgetOfThisType;

    bool m_StorageFlag;   // ist true falls widget einen Historie (Speicher) hat zum Beispiel Oszi, CAN-Message, ...
                          // dann werden diese Widgets sofort beim anlegen des Sheets erzeugt,
                          // ansonsten erst wenn das Sheet das erste mal aktiviert wird.
};

#endif // MDIWINDOWTYPE_H

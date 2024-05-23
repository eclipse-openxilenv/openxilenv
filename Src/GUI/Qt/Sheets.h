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


#ifndef SHEETS_H
#define SHEETS_H

#include <QString>
#include <QTabWidget>
#include <QMdiArea>
#include "MdiWindowWidget.h"
#include "WindowUpdateTimers.h"

class Sheets : public QMdiArea
{
    Q_OBJECT
public:
    Sheets(QTabWidget *arg_allSheetWindow, QMap<QString, MdiWindowType* > *par_AllElementPlugins,
           WindowUpdateTimers *arg_UpdateTimers, QString &arg_section, bool par_Selected, QWidget *parent = nullptr);
    ~Sheets() Q_DECL_OVERRIDE;
    void addSubWindowToSheet(MdiWindowWidget *arg_subWindow, int left, int top, int width, int height, int is_icon, int minimized_left, int minimized_right);
    void addSubWindowToSheet(MdiWindowWidget *arg_subWindow);

    QString GetName();

    void writeToIni(bool par_IsSelectedSheet);
    void readFromIni(bool par_WithStorage, bool par_WithoutStorage);

    QList<MdiWindowWidget *> OpenWindows(QStringList &par_WindowNames,
                                         QMap<QString, MdiWindowType* > *par_AllElementPlugins,
                                         WindowUpdateTimers *arg_UpdateTimers, bool arg_PosIsValid = false,
                                         QPoint arg_Pos = QPoint(), bool arg_SizeIsValid = false,
                                         int arg_Width = 0, int arg_Height = 0);

    int CloseWindowByName(char *par_WindowNameFilter);

    int WindowNameAlreadyInUse (QString &par_WindowName);

    bool CheckWidget(QString &par_WindowTitel);
    MdiWindowWidget *FetchWidget (QString &par_WindowTitel, QSize *ret_MinSize = nullptr, QSize *ret_MaxSize = nullptr);
    MdiWindowWidget *FetchWidget (MdiWindowWidget *par_Widget, QSize *ret_MinSize = nullptr, QSize *ret_MaxSize = nullptr);
    bool ReleaseWidget(MdiWindowWidget *par_Widget);
    void Activating ();

    void SelectWindow (QString par_WindowName);

    static void CleanIni();

signals:
    void NewOrOpenWindowMenu(const QPoint &par_Point);

protected:
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;

private:
    void ReadIniToCache();
    void WriteCacheToIni();

    QTabWidget *m_allSheet;

    bool m_FistTimeSelected;
    QMap<QString, MdiWindowType* > *m_AllElementPlugins;
    WindowUpdateTimers *m_UpdateTimers;
    QString m_Section;

    QList<QString> m_WindowNamesCache;
    QList<QString> m_WindowPlacesCache;
};

#endif // SHEETS_H

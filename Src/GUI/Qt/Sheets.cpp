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
#include "Sheets.h"

#include <QInputDialog>
#include <QApplication>
#include "MainWindow.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

extern "C" {
#include "Files.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "Wildcards.h"
#include "MainValues.h"
#include "RunTimeMeasurement.h"
#include "StringMaxChar.h"
}

Sheets::Sheets(QTabWidget *arg_allSheetWindow, QMap<QString, MdiWindowType* > *par_AllElementPlugins,
               WindowUpdateTimers *arg_UpdateTimers, QString &arg_section, bool par_Selected, QWidget *parent)  :
    QMdiArea(parent), m_allSheet(arg_allSheetWindow)
{
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // so set the background oneself
    // not eccessary because use QPalette::Dark
    /*if (s_main_ini_val.DarkMode) {
        setBackground(Qt::black);
    }*/
    m_FistTimeSelected = par_Selected;
    m_AllElementPlugins = par_AllElementPlugins;
    m_UpdateTimers = arg_UpdateTimers;
    m_Section = arg_section;

    ReadIniToCache();

    readFromIni(true, par_Selected);
}

Sheets::~Sheets()
{
}

void Sheets::writeToIni(bool par_IsSelectedSheet)
{
    int Index = m_allSheet->indexOf(this);
    QString SheetName = GetName(Index);
    QString SheetIniName = QString("GUI/OpenWindowsForSheet%1").arg(QString().number(Index));
    ScQt_IniFileDataBaseWriteString(SheetIniName, nullptr, nullptr, ScQt_GetMainFileDescriptor());
    ScQt_IniFileDataBaseWriteString(SheetIniName, "SheetName", SheetName, ScQt_GetMainFileDescriptor());
    // Only write if the sheet was displayed at least once
    if (m_FistTimeSelected) {
        QList<QMdiSubWindow*> MdiSubwindows = this->subWindowList(QMdiArea::StackingOrder);
        for(int j = 0; j < MdiSubwindows.size(); j++) {
            MdiSubWindow *Sub = qobject_cast<MdiSubWindow*>(MdiSubwindows.at(j));
            if(Sub) {
                Sub->writeToIni(SheetIniName, j);
            }
        }
    } else {
        // Otherwise writeback the cache
        WriteCacheToIni();
    }
}

void Sheets::readFromIni(bool par_WithStorage, bool par_WithoutStorage)
{
    QString NewWindowName;
    NewWindowName.clear();

    for (int x = 0; x < m_WindowNamesCache.size(); x++) {
        QString WindowName = m_WindowNamesCache.at(x);
        QString IniSection("GUI/Widgets/");
        IniSection.append(WindowName);

        // Find out the window type
        QString TypeString = ScQt_IniFileDataBaseReadString(IniSection, "type", "", ScQt_GetMainFileDescriptor());

        // Create a display subwindow depneding on the window type
        foreach(QString key, m_AllElementPlugins->keys()) {
            MdiWindowType *WindowType = m_AllElementPlugins->value(key);
            if(TypeString.compare(WindowType->GetWindowTypeName()) == 0) {
                // Create only subwindows with history (like osciloscope)
                // Or all if sheet should be active
                if ((par_WithStorage && WindowType->GetStorageFlag()) ||
                    (par_WithoutStorage && !WindowType->GetStorageFlag())) {
                    MdiSubWindow *SubWindow = new MdiSubWindow(this);
                    SubWindow->setWindowIcon(WindowType->GetIcon());
                    // Look if window already are displayed inside other sheets
                    QSize MinSize, MaxSize;
                    bool MinMaxSizeValid = false;
                    MdiWindowWidget *loc_SC_Widget = FetchWidget (WindowName, &MinSize, &MaxSize);
                    if (loc_SC_Widget != nullptr) {
                        MinMaxSizeValid = true;
                    } else {
                        MinMaxSizeValid = false;
                    }
                    if (loc_SC_Widget == nullptr) {
                        // If not than create the widget here
                        loc_SC_Widget = m_AllElementPlugins->value(key)->openElement(SubWindow, WindowName);
                    } else {
                        loc_SC_Widget->SetSubWindow (SubWindow, MinMaxSizeValid, MinSize, MaxSize);
                    }
                    if (loc_SC_Widget != nullptr) {
                        int left = 0;
                        int top = 0;
                        int width = WindowType->GetDefaultSize().width();
                        int height = WindowType->GetDefaultSize().height();
                        int is_icon = 0;
                        int minimized_left = -1;
                        int minimized_right = -1;
                        char WindowPlace[256];
                        StringCopyMaxCharTruncate(WindowPlace, QStringToConstChar(m_WindowPlacesCache.at(x)), (int)sizeof(WindowPlace));
                        if (strlen (WindowPlace) > 0) {
                            char *str_left;
                            char *str_top;
                            char *str_width;
                            char *str_height;
                            char *str_is_icon;
                            char *str_minimized_left;
                            char *str_minimized_right;
                            if (StringCommaSeparate (WindowPlace, &str_left, &str_top, &str_width, &str_height, &str_is_icon, &str_minimized_left, &str_minimized_right, nullptr) == 7) {
                                left = atoi (str_left);
                                top = atoi (str_top);
                                width = atoi (str_width);
                                height = atoi (str_height);
                                is_icon = atoi (str_is_icon);
                                minimized_left = atoi (str_minimized_left);
                                minimized_right = atoi (str_minimized_right);
                            }
                        }
                        this->addSubWindowToSheet(loc_SC_Widget, left, top, width, height, is_icon, minimized_left, minimized_right);
                        if (!connect(m_UpdateTimers->updateTimer(WindowType->GetUpdateRate()), SIGNAL(timeout()), loc_SC_Widget, SLOT(CyclicUpdate()))) {
                            ThrowError (1, "canot connect SIGNAL(timeout()) to SLOT(CyclicUpdate())");
                        }
                        connect(loc_SC_Widget, SIGNAL(openStandardDialog(QStringList,bool,bool,QColor,QFont,bool)), qobject_cast<MainWindow*>(m_allSheet->parentWidget()), SLOT(openElementDialog(QStringList,bool,bool,QColor,QFont,bool)));
                    }
                }
            }
        }
    }
}

QList<MdiWindowWidget*> Sheets::OpenWindows(QStringList &par_WindowNames,
                                            QMap<QString, MdiWindowType* > *arg_AllElementPlugins,
                                            WindowUpdateTimers *arg_UpdateTimers, bool arg_PosIsValid, QPoint arg_Pos,
                                            bool arg_SizeIsValid, int arg_Width, int arg_Height)
{
    QList<MdiWindowWidget*> Ret;
    foreach (QString WindowName, par_WindowNames) {
        QString IniSection("GUI/Widgets/");
        IniSection.append(WindowName);

        // Find out the window type
        QString Type = ScQt_IniFileDataBaseReadString(IniSection, "type", "", ScQt_GetMainFileDescriptor());

        // Create a display subwindow depneding on the window type
        foreach(QString key, arg_AllElementPlugins->keys()) {
            MdiWindowType *WindowType = arg_AllElementPlugins->value(key);
            if(Type.compare(WindowType->GetWindowTypeName()) == 0) {
                MdiSubWindow *SubWindow = new MdiSubWindow(this);
                SubWindow->setWindowIcon(WindowType->GetIcon());
                QSize MinSize, MaxSize;
                // Look if window already are displayed inside other sheets
                MdiWindowWidget *loc_SC_Widget = FetchWidget(WindowName, &MinSize, &MaxSize);
                if (loc_SC_Widget == nullptr) {
                    // If not than create the widget here
                    loc_SC_Widget = arg_AllElementPlugins->value(key)->openElement(SubWindow, WindowName);
                } else {
                    loc_SC_Widget->SetSubWindow (SubWindow, true, MinSize, MaxSize);
                }
                if (loc_SC_Widget != nullptr) {
                    int left = 0;
                    int top = 0;
                    int width = WindowType->GetDefaultSize().width();
                    int height = WindowType->GetDefaultSize().height();
                    int is_icon = 0;
                    int minimized_left = -1;
                    int minimized_right = -1;
                    this->addSubWindowToSheet(loc_SC_Widget, left, top, width, height, is_icon, minimized_left, minimized_right);
                    if (!connect(arg_UpdateTimers->updateTimer(WindowType->GetUpdateRate()), SIGNAL(timeout()), loc_SC_Widget, SLOT(CyclicUpdate()))) {
                        ThrowError (1, "cannot connect SIGNAL(timeout()) to SLOT(CyclicUpdate())");
                    }
                    if (arg_PosIsValid) {
                        QPoint Point = arg_Pos;
                        loc_SC_Widget->GetCustomMdiSubwindow()->move(Point.x(), Point.y());
                    }
                    if (arg_SizeIsValid) {
                        loc_SC_Widget->GetCustomMdiSubwindow()->resize(arg_Width, arg_Height);
                    }
                }
                Ret.append(loc_SC_Widget);
            }
        }
    }
    return Ret;
}

int Sheets::CloseWindowByName(char *par_WindowNameFilter)
{
    QList<QMdiSubWindow *>SubWindowList = this->subWindowList();

    foreach (QMdiSubWindow *SubWindow, SubWindowList) {
        if (!Compare2StringsWithWildcardsCaseSensitive (QStringToConstChar(SubWindow->windowTitle()), par_WindowNameFilter, 1)) {
            SubWindow->close();
            return 1;
        }
    }
    return 0;
}

void Sheets::addSubWindowToSheet(MdiWindowWidget *arg_subWindow, int left, int top, int width, int height, int is_icon, int minimized_left, int minimized_right)
{
    Q_UNUSED(minimized_left)
    Q_UNUSED(minimized_right)

    static int counter;

    counter++;
    this->addSubWindow(arg_subWindow->GetCustomMdiSubwindow());
    arg_subWindow->MdiSubWindowResize (width, height);
    arg_subWindow->MdiSubWindowMove (left, top);
    if (is_icon) {
        arg_subWindow->GetCustomMdiSubwindow()->showMinimized();
    } else {
        arg_subWindow->GetCustomMdiSubwindow()->show();
    }
    connect(arg_subWindow->GetCustomMdiSubwindow(), SIGNAL(focusInSubWindow()), arg_subWindow, SLOT(setDefaultFocus()));
}

void Sheets::addSubWindowToSheet(MdiWindowWidget *arg_subWindow)
{
    this->addSubWindow(arg_subWindow->GetCustomMdiSubwindow());
    arg_subWindow->GetCustomMdiSubwindow()->show();
    connect(arg_subWindow->GetCustomMdiSubwindow(), SIGNAL(focusInSubWindow()), arg_subWindow, SLOT(setDefaultFocus()));
}

QString Sheets::GetName(int par_Index)
{
    if (par_Index == -1) {
        par_Index = m_allSheet->indexOf(this);
    }
    QString SheetName = m_allSheet->tabText(par_Index);
    SheetName.remove('&');
    return SheetName;
}

int Sheets::WindowNameAlreadyInUse (QString &par_WindowName)
{
    QList<QMdiSubWindow *>SubWindowList = this->subWindowList();

    foreach (QMdiSubWindow *SubWindow, SubWindowList) {
        if (par_WindowName.compare (SubWindow->windowTitle()) == 0) {
            return 1;
        }
    }
    // If the sheet never was displayed also search inside the cache
    if (!m_FistTimeSelected) {
        foreach (QString Item, m_WindowNamesCache) {
            if (par_WindowName.compare (Item) == 0) {
                return 1;
            }
        }
    }

    return 0;
}


bool Sheets::CheckWidget(QString &par_WindowTitel)
{
    for (int i = 0; i < m_allSheet->count(); i++) {
        Sheets *Sheet = static_cast<Sheets*>(m_allSheet->widget(i));
        if (Sheet != this) {
            QList<QMdiSubWindow*> loc_mdiSubwindows = Sheet->subWindowList();
            for(int j = 0; j < loc_mdiSubwindows.size(); j++) {
                MdiSubWindow *loc_sub = qobject_cast<MdiSubWindow*>(loc_mdiSubwindows.at(j));
                if(loc_sub) {
                    QString Titel = loc_sub->windowTitle();
                    if (!par_WindowTitel.compare(Titel)) {
                        if (loc_sub->isOwnerOfWidget()) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}


MdiWindowWidget *Sheets::FetchWidget(QString &par_WindowTitel, QSize *ret_MinSize, QSize *ret_MaxSize)
{
    for (int i = 0; i < m_allSheet->count(); i++) {
        Sheets *Sheet = static_cast<Sheets*>(m_allSheet->widget(i));
        if (Sheet != this) {
            QList<QMdiSubWindow*> loc_mdiSubwindows = Sheet->subWindowList();
            for(int j = 0; j < loc_mdiSubwindows.size(); j++) {
                MdiSubWindow *loc_sub = qobject_cast<MdiSubWindow*>(loc_mdiSubwindows.at(j));
                if(loc_sub) {
                    QString Titel = loc_sub->windowTitle();
                    if (!par_WindowTitel.compare(Titel)) {
                        if (loc_sub->isOwnerOfWidget()) {
                            if (ret_MinSize != nullptr) {
                                *ret_MinSize = loc_sub->minimumSize();
                                if ((ret_MinSize->width() < 0) || (ret_MinSize->height() < 0)) {
                                    printf ("debug");
                                }
                            }
                            if (ret_MaxSize != nullptr) {
                                *ret_MaxSize = loc_sub->maximumSize();
                                if ((ret_MaxSize->width() < 0) || (ret_MaxSize->height() < 0)) {
                                    printf ("debug");
                                }
                            }
                            MdiWindowWidget *Ret = static_cast<MdiWindowWidget*>(loc_sub->widget());
                            loc_sub->setNotOwnerOfWidget();
                            return Ret;
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

MdiWindowWidget *Sheets::FetchWidget(MdiWindowWidget *par_Widget, QSize *ret_MinSize, QSize *ret_MaxSize)
{
    for (int i = 0; i < m_allSheet->count(); i++) {
        Sheets *Sheet = static_cast<Sheets*>(m_allSheet->widget(i));
        if (Sheet != this) {
            QList<QMdiSubWindow*> loc_mdiSubwindows = Sheet->subWindowList();
            for(int j = 0; j < loc_mdiSubwindows.size(); j++) {
                MdiSubWindow *loc_sub = qobject_cast<MdiSubWindow*>(loc_mdiSubwindows.at(j));
                if(loc_sub) {
                    MdiWindowWidget *Widget = static_cast<MdiWindowWidget*>(loc_sub->widget());
                    if (Widget == par_Widget) {
                        if (loc_sub->isOwnerOfWidget()) {
                            if (ret_MinSize != nullptr) {
                                *ret_MinSize = loc_sub->minimumSize();
                                if ((ret_MinSize->width() < 0) || (ret_MinSize->height() < 0)) {
                                    printf ("debug");
                                }
                            }
                            if (ret_MaxSize != nullptr) {
                                *ret_MaxSize = loc_sub->maximumSize();
                                if ((ret_MaxSize->width() < 0) || (ret_MaxSize->height() < 0)) {
                                    printf ("debug");
                                }
                            }
                            loc_sub->setNotOwnerOfWidget();
                            return Widget;
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

bool Sheets::ReleaseWidget(MdiWindowWidget *par_Widget)
{
    for (int i = 0; i < m_allSheet->count(); i++) {
        Sheets *Sheet = static_cast<Sheets*>(m_allSheet->widget(i));
        if (Sheet != this) {
            QList<QMdiSubWindow*> loc_mdiSubwindows = Sheet->subWindowList();
            for(int j = 0; j < loc_mdiSubwindows.size(); j++) {
                MdiSubWindow *loc_sub = qobject_cast<MdiSubWindow*>(loc_mdiSubwindows.at(j));
                if(loc_sub) {
                    MdiWindowWidget *Widget = loc_sub->GetSubWindowWidget();
                    if (Widget == par_Widget) {
                        if (!loc_sub->isOwnerOfWidget()) {
                            loc_sub->setWidget(par_Widget);
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

// This will called if a sheet will be activated
// All subwindows will be fetched back
void Sheets::Activating()
{
    if (!m_FistTimeSelected) {
        m_FistTimeSelected = true;
        readFromIni(false, true);
    }

    QList<QMdiSubWindow *>SubWindowList = this->subWindowList();

    foreach (QMdiSubWindow *SubWindow, SubWindowList) {
        MdiSubWindow *CustomSubWindow = static_cast<MdiSubWindow*>(SubWindow);
        MdiWindowWidget *Widget = CustomSubWindow->NotOwnerOfWidget();
        if (Widget != nullptr) {
            QSize MinSize, MaxSize;
            FetchWidget (Widget, &MinSize, &MaxSize);
            CustomSubWindow->setOwnerOfWidget(MinSize, MaxSize);
        }
    }
}

void Sheets::SelectWindow(QString par_WindowName)
{
    QList<QMdiSubWindow *>SubWindowList = this->subWindowList();

    foreach (QMdiSubWindow *SubWindow, SubWindowList) {
        if (par_WindowName.compare (SubWindow->windowTitle()) == 0) {
            SubWindow->hide();   // first hide so we are sure the window will became top most
            SubWindow->showNormal();
            SubWindow->activateWindow();
        }
    }
}

// Delete all sheets out of the INI file
void Sheets::CleanIni()
{
    char Section[INI_MAX_SECTION_LENGTH];
    for (int x = 0; ; x++) {
        QString Section = QString("GUI/OpenWindowsForSheet%1").arg(x);
        if (ScQt_IniFileDataBaseLookIfSectionExist (Section, GetMainFileDescriptor())) {
            ScQt_IniFileDataBaseWriteString(Section, nullptr, nullptr, GetMainFileDescriptor());
        } else {
            break;
        }
    }
}

void Sheets::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint Pos = event->globalPos();
    QWidget *w = QApplication::widgetAt(Pos);
    if (w != nullptr) {
        QWidget *wp = w->parentWidget();
        if (wp == this) {
            emit NewOrOpenWindowMenu(event->globalPos());
        }
    }
}

void Sheets::ReadIniToCache()
{
    QString Entry;

    for (int x = 0; ; x++) {
        Entry = QString("W%1").arg(x);
        QString Line = ScQt_IniFileDataBaseReadString(m_Section, Entry, "", ScQt_GetMainFileDescriptor());
        if (Line.isEmpty()) {
            break;
        }
        m_WindowNamesCache.append(Line);
        Entry = QString("WP%1").arg(x);
        Line = ScQt_IniFileDataBaseReadString(m_Section, Entry, "", ScQt_GetMainFileDescriptor());
        m_WindowPlacesCache.append(Line);
    }
    ScQt_IniFileDataBaseWriteString(m_Section, nullptr, nullptr, ScQt_GetMainFileDescriptor());
}

void Sheets::WriteCacheToIni()
{
    QString Entry;
    for (int x = 0; x < m_WindowNamesCache.size(); x++) {
        Entry = QString("W%1").arg(x);
        QString WindowName = m_WindowNamesCache.at(x);
        ScQt_IniFileDataBaseWriteString(m_Section, Entry, WindowName, ScQt_GetMainFileDescriptor());
        Entry = QString("WP%1").arg(x);
        QString Line = m_WindowPlacesCache.at(x);
        ScQt_IniFileDataBaseWriteString(m_Section,  Entry, Line, ScQt_GetMainFileDescriptor());
    }
}


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


#include "MdiWindowWidget.h"
#include "MdiSubWindow.h"
#include "Sheets.h"
#include "MdiWindowType.h"
#include "QtIniFile.h"

#include <QTextStream>

extern "C" {
#include "Blackboard.h"
}

MdiSubWindow::MdiSubWindow(Sheets *par_Sheet)
{
    m_OwnerOfWidget = false;
    m_SubWindowWidget = nullptr;
    m_Sheet = par_Sheet;
    setWindowFlags(Qt::SubWindow | Qt::CustomizeWindowHint |
                   Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);
}

MdiSubWindow::~MdiSubWindow()
{
}

void MdiSubWindow::closeEvent(QCloseEvent *event)
{
    MdiWindowWidget *Widget = static_cast<MdiWindowWidget*>(this->widget());
    if (Widget != nullptr) {
        m_Sheet->ReleaseWidget (Widget);
    }
    event->accept();
}

void MdiSubWindow::writeToIni(QString &par_Section, int par_Index)
{
    QString WindowTitle = this->GetWindowTitle();
    QString IniSectionPath = this->GetIniSectionPath();
    QRect Rect = this->geometry();
    QString Entry;
    int Fd = ScQt_GetMainFileDescriptor();

    Entry = QString("W%1").arg(par_Index);

    // write MDI subwindow name to the INI file
    ScQt_IniFileDataBaseWriteString(par_Section, Entry, WindowTitle, Fd);
    Entry = QString("WP%1").arg(par_Index);

    int State = windowState();
    QString WindowPosAndSize;
    QTextStream StreamWindowPosAndSize(&WindowPosAndSize);
    if ((State & (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)) == 0) {
        StreamWindowPosAndSize << Rect.x() << ','
                               << Rect.y() << ","
                               << Rect.width() << ","
                               <<  Rect.height() << ","
                               << ((this->isMinimized()) ? 1 : 0) << ",-1,-1";
    } else {
        StreamWindowPosAndSize << m_NormalViewGeometry.x() << ","
                               << m_NormalViewGeometry.y() << ","
                               << m_NormalViewGeometry.width() << ","
                               << m_NormalViewGeometry.height() << ","
                               << ((this->isMinimized()) ? 1 : 0) << ",-1,-1";
    }
    // write MDI sub window position into INI file
    ScQt_IniFileDataBaseWriteString(par_Section, Entry, WindowPosAndSize, Fd);

    MdiWindowWidget *Widget = qobject_cast<MdiWindowWidget*>(this->widget());
    if (Widget != nullptr) {
         // Child widget of the MDI sub window write to INI file
        if(!Widget->writeToIni()){
            // do nothing
        }
        // write the current position inside the sheet into the widget INI section
        //QString WindowType = Widget->GetMdiWindowType()->GetWindowTypeName();
        ScQt_IniFileDataBaseWriteInt(IniSectionPath, "left", Rect.x(), Fd);
        ScQt_IniFileDataBaseWriteInt(IniSectionPath, "top", Rect.y(), Fd);
        ScQt_IniFileDataBaseWriteInt(IniSectionPath, "width", Rect.width(), Fd);
        ScQt_IniFileDataBaseWriteInt(IniSectionPath, "height", Rect.height(), Fd);
    }
}

void MdiSubWindow::setWidget(MdiWindowWidget *par_Widget)
{
    m_OwnerOfWidget = true;
    m_SubWindowWidget = par_Widget;
    QMdiSubWindow::setWidget(par_Widget);
}

void MdiSubWindow::setNotOwnerOfWidget()
{
    m_OwnerOfWidget = false;
    QMdiSubWindow::setWidget(nullptr);
}

void MdiSubWindow::setOwnerOfWidget(QSize &par_MinSize, QSize &par_MaxSize)
{
    m_SubWindowWidget->SetSubWindow (this, true, par_MinSize, par_MaxSize);
}

MdiWindowWidget *MdiSubWindow::NotOwnerOfWidget()
{
    if ( m_OwnerOfWidget) {
        return nullptr;
    } else {
        return m_SubWindowWidget;
    }
}

MdiWindowWidget *MdiSubWindow::GetSubWindowWidget()
{
    return m_SubWindowWidget;
}

QString MdiSubWindow::GetWindowTitle()
{
    if (m_SubWindowWidget != nullptr) {
        return m_SubWindowWidget->GetWindowTitle();
    } else {
        return QString();
    }
}

QString MdiSubWindow::GetIniSectionPath()
{
    if (m_SubWindowWidget != nullptr) {
        return m_SubWindowWidget->GetIniSectionPath();
    } else {
        return QString();
    }
}


bool MdiSubWindow::isOwnerOfWidget()
{
    return m_OwnerOfWidget;
}

void MdiSubWindow::readFromIni(QString &par_Section, int par_Index)
{
    QString Entry;
    int Fd = ScQt_GetMainFileDescriptor();

    Entry = QString("W%1").arg(par_Index);
    // Read MDI sub window name from INI file
    QString WindowName = ScQt_IniFileDataBaseReadString(par_Section, Entry, "", Fd);
    this->setWindowTitle(WindowName);
    Entry = QString("WP%1").arg(par_Index);
    // Read MDI sub window position from INI file
    QString Line = ScQt_IniFileDataBaseReadString(par_Section, Entry, "", Fd);
    /*
     * posList[0] --> x position of MDI-Subwindow
     * posList[1] --> y position of MDI-Subwindow
     * posList[2] --> Width of MDI-Subwindow
     * posList[3] --> Hight of MDI-Subwindow
     * posList[4] --> MDI-subwindow icon flag
     * posList[5] --> Icon position x
     * posList[6] --> Icon position y
     */
    QStringList posList = Line.split(",");
    this->move(posList.at(0).toInt(), posList.at(1).toInt());
    this->resize(posList.at(2).toInt(), posList.at(3).toInt());
}

void MdiSubWindow::dragEnterEvent(QDragEnterEvent *event) {
    if(event->mimeData()->hasFormat("text/plain")) {
        event->acceptProposedAction();
    }
}

void MdiSubWindow::dragMoveEvent(QDragMoveEvent *event) {
    event->acceptProposedAction();
}

void MdiSubWindow::dragLeaveEvent(QDragLeaveEvent *event) {
    event->accept();
}

void MdiSubWindow::dropEvent(QDropEvent *event) {
    if((event->source() != this) && (event->possibleActions() == (Qt::MoveAction | Qt::CopyAction))) {
        if(event->mimeData()->hasFormat("text/plain")) {
            this->setWindowTitle(event->mimeData()->data("text/plain"));
            event->acceptProposedAction();
        }
    } else {
        return;
    }
}

bool MdiSubWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FocusIn:
        emit focusInSubWindow();
        break;
        // Because there is no possibility to get the window size and position of normal display mode,
        // if the sub window is minimized, store the position here
    case QEvent::WindowStateChange:
    case QEvent::Resize:
    case QEvent::Move:
        {
            int State = windowState();
            if ((State & (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)) == 0) {
                m_IsNormalView = true;
            } else {
                m_IsNormalView = false;
            }
            if (m_IsNormalView) {
                m_NormalViewGeometry = geometry();
            }
        }
        break;
    default:
        break;
    }
    return QMdiSubWindow::event(event);
}


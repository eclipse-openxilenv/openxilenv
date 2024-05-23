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
#include "MdiWindowType.h"

#include "MdiSubWindow.h"


MdiWindowWidget::MdiWindowWidget(QString par_WindowTitle, MdiSubWindow *par_MdiSubWindow, MdiWindowType *par_Type, QWidget *parent) : QWidget(parent)
{
    m_Type = par_Type;
    m_Type->AddNewWidget(this);
    m_MdiSubWindow = par_MdiSubWindow;

    m_MdiSubWindow->setWidget(this);

    m_WindowTitle = par_WindowTitle;

    setWindowTitle (m_WindowTitle);
    SetWindowIcon(par_Type->GetIcon());

    // set the minimal size of the window
    QSize MinSize = m_Type->GetMinSize();
    if (!MinSize.isNull()) {
        m_MdiSubWindow->setMinimumSize (MinSize);
    }
}

MdiWindowWidget::~MdiWindowWidget()
{
    m_Type->RemoveWidget(this);
}

void MdiWindowWidget::SetSubWindow(MdiSubWindow *par_MdiSubWindow, bool par_MinMaxSizeValid, const QSize &par_MinSize, const QSize &par_MaxSize)
{
    m_MdiSubWindow = par_MdiSubWindow;
    m_MdiSubWindow->setWidget(this);
    m_MdiSubWindow->setWindowTitle(m_WindowTitle);
    // guarantee that the widget will be visable if the sub window is not minimized.
    if (!par_MdiSubWindow->isMinimized()) {
        this->setVisible(true);
    }
    if (par_MinMaxSizeValid) {
        if ((par_MinSize.width() < 0) ||
            (par_MinSize.height() < 0)) {
            printf ("debug");
        }
        m_MdiSubWindow->setMinimumSize(par_MinSize);
        m_MdiSubWindow->setMaximumSize(par_MaxSize);
    }
}

void MdiWindowWidget::MdiSubWindowResize (int x, int y)
{
    parentWidget()->resize(x, y);
    parentWidget()->setGeometry(QRect(0, 0, x, y));
}


void MdiWindowWidget::MdiSubWindowMove (int x, int y)
{
    parentWidget()->move(x, y);
}

void MdiWindowWidget::RenameWindowTo (QString par_WindowTitle)
{
    if (m_WindowTitle.compare(par_WindowTitle)) {
        // Only rename if the name has be changed
        writeToIni();
        QString  SaveWindowTitle = m_WindowTitle;
        m_WindowTitle = par_WindowTitle;
        setWindowTitle (m_WindowTitle);
        if (!m_Type->AddWindowToAllOfTypeIniList (this)) {
            // If an error occur reset the changes
            m_WindowTitle = SaveWindowTitle;
            m_MdiSubWindow->setWindowTitle (m_WindowTitle);
        }
    }
}

QString MdiWindowWidget::GetWindowTitle()
{
    return m_WindowTitle;
}

void MdiWindowWidget::SetWindowIcon(QIcon par_WindowIcon)
{
    m_MdiSubWindow->setWindowIcon(par_WindowIcon);
}

QIcon MdiWindowWidget::GetWindowIcon()
{
    return m_MdiSubWindow->windowIcon();
}

QString MdiWindowWidget::GetDefaultNewWindowTitle ()
{
    return m_Type->GetDefaultNewWindowTitle ();
}

MdiWindowType* MdiWindowWidget::GetMdiWindowType() {
    return m_Type;
}


MdiSubWindow *MdiWindowWidget::GetCustomMdiSubwindow ()
{
    return m_MdiSubWindow;
}

bool MdiWindowWidget::readFromIni()
{
    return false;
}

QString MdiWindowWidget::GetIniSectionPath()
{
    return QString("GUI/Widgets/").append(m_WindowTitle);
}

void MdiWindowWidget::CyclicUpdate ()
{

}

void MdiWindowWidget::setDefaultFocus()
{
}


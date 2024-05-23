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

#include "UserDrawWidget.h"
#include "DragAndDrop.h"
#include "MdiWindowType.h"
#include "MdiSubWindow.h"
#include "QtIniFile.h"
#include <QPainter>
#include <QMenu>
#include <QApplication>
#include "UserDrawConfigDialog.h"

#include "UserDrawParser.h"
#include "UserDrawLayer.h"
#include "QtIniFile.h"

#include <math.h>

extern "C" {
    #include "Blackboard.h"
    #include "BlackboardAccess.h"
    #include "MainValues.h"
    #include "Config.h"
    #include "Files.h"
    #include "ThrowError.h"
}

UserDrawWidget::UserDrawWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType* par_Type, QWidget* parent) :
      MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent),
      m_ObserverConnection(this)
{
    QString Empty;
    m_Root = new UserDrawRoot(-1, Empty);
    m_FixedRatio = false;
    m_Antialiasing = false;
    m_ConfigAct = new QAction(tr("&config"), this);
    m_ConfigAct->setStatusTip(tr("configure slider"));
    connect(m_ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigureSlot()));
    readFromIni();
}

UserDrawWidget::~UserDrawWidget()
{
    writeToIni();
    if (m_Root != nullptr) delete m_Root;
    foreach(UserDrawImageItem *Item, m_Images) {
        delete Item;
    }
}

bool UserDrawWidget::WriteImagesToIni(QString &par_WindowName)
{
    QString Entry;
    int i = 0;
    char Help[64];
    int Fd = ScQt_GetMainFileDescriptor();

    foreach(UserDrawImageItem *Item, m_Images) {
        sprintf(Help, "Image_%i_Name", i);
        Entry = QString(Help);
        QString Line = QString().number(Item->m_Number);
        Line.append(",");
        Line.append(Item->m_Name);
        ScQt_IniFileDataBaseWriteString(par_WindowName, Entry, Line, Fd);
        sprintf(Help, "Image_%i_Buffer", i);
        Entry = QString(Help);
        ScQt_IniFileDataBaseWriteByteImage(par_WindowName, Entry, 1000, (void*)Item->m_BinaryImage.Get(),
                                           Item->m_BinaryImage.GetSize(), Fd);
        i++;
    }
    // delete additionl entrys if available
    for (int x = i; x < 1000; x++) {
        sprintf(Help, "Image_%i_Name", x);
        Entry = QString(Help);
        QString Name = ScQt_IniFileDataBaseReadString(par_WindowName, Entry, "", Fd);
        if (Name.isEmpty()) break;
        ScQt_IniFileDataBaseDeleteEntry(par_WindowName, Entry, Fd);
        sprintf(Help, "Image_%i_Buffer", i);
        Entry = QString(Help);
        ScQt_IniFileDataBaseWriteByteImage(par_WindowName, Entry, 1000, nullptr, 0, Fd);
    }
    return true;
}

bool UserDrawWidget::ReadImagesToIni(QString &par_WindowName)
{
    QString Entry;
    char Help[64];
    int Fd = ScQt_GetMainFileDescriptor();
    for (int x = 0; x < 1000; x++) {
        UserDrawImageItem *Item = new UserDrawImageItem();
        sprintf(Help, "Image_%i_Name", x);
        Entry = QString(Help);
        QString Line = ScQt_IniFileDataBaseReadString(par_WindowName, Entry, "", Fd);
        if (Line.isEmpty()) break;
        QStringList List = Line.split(',');
        if (List.count() == 2) {
            int Type;
            unsigned char *Buffer;
            sprintf(Help, "Image_%i_Buffer", x);
            Entry = QString(Help);
            int Len = ScQt_IniFileDataBaseReadByteImage(par_WindowName, Entry, &Type, &Buffer, 0, Fd);
            if (Len <= 0) return false;
            Item->m_Number = List.at(0).toUInt();
            Item->m_Name = List.at(1).trimmed();
            Item->m_BinaryImage.Set(Buffer, Len);
            Item->m_Image = Item->m_BinaryImage.GetImageFrom();
            m_Images.append(Item);
        }
    }
    return true;
}


bool UserDrawWidget::writeToIni() {

    QString SectionPath = GetIniSectionPath();
    QString Entry = QString("FixedRatio");
    char Help[64];
    int Fd = ScQt_GetMainFileDescriptor();
    ScQt_IniFileDataBaseWriteYesNo(SectionPath, Entry, m_Root->m_FixedRatio, Fd);
    Entry = QString("Antialiasing");
    ScQt_IniFileDataBaseWriteYesNo(SectionPath, Entry, m_Antialiasing, Fd);

    if (m_Root != nullptr) {
        for (int x = 0; x < m_Root->GetChildCount(); x++) {
            UserDrawElement *Child = m_Root->GetChild(x);
            QString ChildEntry("x");
            sprintf(Help, "_%i", x);
            ChildEntry.append(QString(Help));

            Child->WriteToINI(SectionPath, ChildEntry, Fd);
        }
    }
    WriteImagesToIni(SectionPath);
    return true;
}

bool UserDrawWidget::readFromIni() {

    QString WindowName = GetIniSectionPath();
    QString Entry = QString("FixedRatio");
    int Fd = ScQt_GetMainFileDescriptor();
    m_FixedRatio = ScQt_IniFileDataBaseReadYesNo(WindowName, Entry, 0, Fd);
    Entry = QString("Antialiasing");
    m_Antialiasing = ScQt_IniFileDataBaseReadYesNo(WindowName, Entry, 0, Fd);

    UserDrawParser Parser;
    if (m_Root != nullptr) delete m_Root;
    m_Root = Parser.Parse(WindowName);
    m_Root->m_FixedRatio = m_FixedRatio;
    m_Root->m_Antialiasing = m_Antialiasing;
    ReadImagesToIni(WindowName);
    return true;
}

CustomDialogFrame*UserDrawWidget::dialogSettings(QWidget* arg_parent)
{
    Q_UNUSED(arg_parent)
    return nullptr;
}

void UserDrawWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter Painter(this);
    int w = width();
    int h = height();

    if (m_FixedRatio) {
        if (w < h) h = w;
        else if (h < w) w = h;
    }
    DrawParameter D(0.0, 0.0, 1.0, 0.0, (double)w, (double)h, DrawParameter::UpdateTypeCyclic);
    if (m_Root != nullptr) {
        m_Root->Paint(&Painter, &m_Images, D);
    }
}

void UserDrawWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
}

void UserDrawWidget::mousePressEvent(QMouseEvent *event)
{
    int w = width();
    int h = height();

    if (m_FixedRatio) {
        if (w < h) h = w;
        else if (h < w) w = h;
    }
    DrawParameter D(0.0, 0.0, 1.0, 0.0, (double)w, (double)h, DrawParameter::UpdateTypeCyclic);
    if (m_Root != nullptr) {
        m_Root->MousePressEvent(event, D);
    }

}

void UserDrawWidget::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event)

    QMenu menu(this);
    menu.addAction (m_ConfigAct);
    menu.exec(event->globalPos());

}

void UserDrawWidget::CyclicUpdate()
{
    int w = width();
    int h = height();
    if ((w > 0) && (h > 0)) {
        update();
    }
}

void UserDrawWidget::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    Q_UNUSED(arg_vid)
    Q_UNUSED(arg_observationFlag)
}

void UserDrawWidget::ConfigureSlot()
{
    QString WindowName = GetWindowTitle();
    if (m_Root != nullptr) {
        UserDrawConfigDialog *dialog = new UserDrawConfigDialog(WindowName, m_Root, &m_Images, this);
        if(dialog->exec() == QDialog::Accepted) {
            if (dialog->WindowTitleChanged()) {
                RenameWindowTo(dialog->GetWindowTitle());
            }
            m_FixedRatio = m_Root->m_FixedRatio;
            writeToIni();
        } else {
            // if cancle reload from INI file
            delete m_Root;
            UserDrawParser Parser;
            m_Root = Parser.Parse(WindowName);
            m_Root->m_FixedRatio = m_FixedRatio;
            m_Root->m_Antialiasing = m_Antialiasing;
        }
        delete dialog;
    }
}

void UserDrawWidget::changeColor(QColor arg_color)
{
    Q_UNUSED(arg_color)

}

void UserDrawWidget::changeFont(QFont arg_font)
{
    Q_UNUSED(arg_font)

}

void UserDrawWidget::changeWindowName(QString arg_name)
{
    Q_UNUSED(arg_name)

}

void UserDrawWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    Q_UNUSED(arg_variable)
    Q_UNUSED(arg_visible)
}

void UserDrawWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_variables)
    Q_UNUSED(arg_visible)
}

void UserDrawWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables)
}


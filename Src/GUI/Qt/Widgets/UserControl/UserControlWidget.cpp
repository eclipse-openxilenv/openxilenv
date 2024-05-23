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


#include "UserControlWidget.h"
#include "MdiWindowType.h"
#include "DragAndDrop.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

#include "UserControlConfigDialog.h"
#include "UserControlParser.h"

#include "InterfaceToScript.h"

#include <QMenu>
#include <QGroupBox>

#define UNIFORM_DIALOGE

UserControlWidget::UserControlWidget(QString par_WindowTitle, MdiSubWindow *par_SubWindow, MdiWindowType *par_Type, QWidget *parent) :
    MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent),
       m_ObserverConnection(this)
{
    m_ConfigAct = new QAction(tr("&config"), this);
    m_ConfigAct->setStatusTip(tr("configure enums"));
    connect(m_ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigureSlot()));

    m_Root = nullptr; //new UserControlRoot(-1, Empty);

    m_Layout = new QGridLayout();
    setLayout(m_Layout);
    m_Layout->setContentsMargins(0,0,0,0);
/*
    QGroupBox *Gp1 = new QGroupBox("group1");
    m_Layout->addWidget(Gp1, 0, 0, 1, 1);
    QGroupBox *Gp2 = new QGroupBox("group2");
    m_Layout->addWidget(Gp2, 1, 1, 1, 1);
*/
    m_ScriptState = -1;
    //setAcceptDrops(true);
    readFromIni();
}

UserControlWidget::~UserControlWidget()
{
    writeToIni();
    if (m_Root != nullptr) delete m_Root;
}

bool UserControlWidget::writeToIni()
{
    QString WindowName = GetIniSectionPath();

    if (m_Root != nullptr) {
        for (int x = 0; x < m_Root->GetChildCount(); x++) {
            UserControlElement *Child = m_Root->GetChild(x);
            QString ChildEntry("x");
            char Help[32];
            sprintf(Help, "_%i", x);
            ChildEntry.append(QString(Help));
            Child->WriteToINI(WindowName, ChildEntry, ScQt_GetMainFileDescriptor());
        }
    }
    return true;
}

bool UserControlWidget::readFromIni()
{
    QString WindowName = GetIniSectionPath();
    UserControlParser Parser;
    if (m_Root != nullptr) delete m_Root;
    m_Root = Parser.Parse(WindowName, this);
    return true;
}

void UserControlWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction (m_ConfigAct);
    menu.exec(event->globalPos());
}

void UserControlWidget::CyclicUpdate()
{
    int State = GetScriptState();
    if (State != m_ScriptState) {
        m_Root->ScriptStateChanged(State);
        m_ScriptState = State;
    }
}

void UserControlWidget::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    Q_UNUSED(arg_vid)
    Q_UNUSED(arg_observationFlag)
}

void UserControlWidget::openDialog()
{

}

void UserControlWidget::ConfigureSlot(void)
{
    QString WindowName = GetWindowTitle();
    if (m_Root != nullptr) {
        UserControlConfigDialog *dialog = new UserControlConfigDialog(WindowName, m_Root, this);
        if(dialog->exec() == QDialog::Accepted) {
            if (dialog->WindowTitleChanged()) {
                RenameWindowTo(dialog->GetWindowTitle());
            }
            writeToIni();
        } else {
            // if cancle reload from INI file
            delete m_Root;
            UserControlParser Parser;
            m_Root = Parser.Parse(WindowName, this);
        }
        delete dialog;
    }
}


CustomDialogFrame*UserControlWidget::dialogSettings(QWidget* arg_parent) {
    arg_parent->setWindowTitle("Enum config");
    return nullptr;
}

void UserControlWidget::AddWidget(QWidget *par_Widget, int x, int y, int w, int h)
{
    m_Layout->addWidget(par_Widget,y,x,h,w);
}

void UserControlWidget::RemoveWidget(QWidget *par_Widget)
{
    m_Layout->removeWidget(par_Widget);
}

void UserControlWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        DragAndDropInfos Infos (mime->text());
        event->acceptProposedAction();
        QString VariableName = Infos.GetName();
        int Vid = get_bbvarivid_by_name(QStringToConstChar(VariableName));
        if ((Vid > 0) && (get_bbvari_conversiontype(Vid) == 2)) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->ignore();
    }
}


void UserControlWidget::dropEvent(QDropEvent *event)
{
    event->ignore();
}


void UserControlWidget::changeColor(QColor arg_color)
{
    Q_UNUSED(arg_color)
}

void UserControlWidget::changeFont(QFont arg_font)
{
    Q_UNUSED(arg_font)
}

void UserControlWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void UserControlWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    Q_UNUSED(arg_variable)
    Q_UNUSED(arg_visible)
}

void UserControlWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_variables)
    Q_UNUSED(arg_visible)
}

void UserControlWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables)
}

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


#include "EnumWidget.h"
#include "MdiWindowType.h"
#include "DragAndDrop.h"

#include "QtIniFile.h"
#include "StringHelpers.h"

extern "C" {
#include "PrintFormatToString.h"
#include "Blackboard.h"
}


#include <QMenu>

#define UNIFORM_DIALOGE

EnumWidget::EnumWidget(QString par_WindowTitle, MdiSubWindow *par_SubWindow, MdiWindowType *par_Type, QWidget *parent) :
    MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent),
    m_ObserverConnection(this)
{
    m_Layout = new QHBoxLayout();
    setLayout(m_Layout);
    m_Layout->setContentsMargins(0,0,0,0);

    m_ConfigAct = new QAction(tr("&config"), this);
    m_ConfigAct->setStatusTip(tr("configure enums"));
    connect(m_ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigureSlot()));

    setAcceptDrops(true);
    readFromIni();
}

EnumWidget::~EnumWidget()
{
    writeToIni();
    DeleteAllVariables();
}


bool EnumWidget::writeToIni()
{
    QString SectionPath = GetIniSectionPath();
    int Fd = ScQt_GetMainFileDescriptor();
    QString WindowType = GetMdiWindowType()->GetWindowTypeName();
    QString Entry("type");
    ScQt_IniFileDataBaseWriteString(SectionPath, Entry, WindowType, Fd);
    int i;
    for (i = 0; i < m_Variables.count(); i++) {
        Entry = QString("Variable%1").arg(i);
        QString VariableName = m_Variables.at(i)->GetName();
        ScQt_IniFileDataBaseWriteString(SectionPath, Entry, VariableName, Fd);
    }
    Entry = QString("VariableCount");
    ScQt_IniFileDataBaseWriteInt(SectionPath, Entry, i, Fd);
    return true;
}

bool EnumWidget::readFromIni()
{
    QString SectionPath = GetIniSectionPath();
    int Fd = ScQt_GetMainFileDescriptor();
    char loc_Entry[32];

    int loc_Count = ScQt_IniFileDataBaseReadInt(SectionPath, "VariableCount", 0, Fd);
    for (int i = 0; i < loc_Count; ++i) {
        PrintFormatToString (loc_Entry, sizeof(loc_Entry), "Variable%i",i);
        QString VariableName = ScQt_IniFileDataBaseReadString(SectionPath, loc_Entry, "", Fd);
        if (!VariableName.isEmpty()) {
            OneEnumVariable *Variable = new OneEnumVariable(QString(VariableName), this);
            m_ObserverConnection.AddObserveVariable(Variable->GetVid(), OBSERVE_CONVERSION_CHANGED | OBSERVE_TYPE_CHANGED); //OBSERVE_CONFIG_ANYTHING_CHANGED);
            m_Variables.append(Variable);
            m_Layout->addWidget(Variable);
        }
    }
    return true;
}

CustomDialogFrame*EnumWidget::dialogSettings(QWidget* arg_parent)
{
    arg_parent->setWindowTitle("Enum config");
    return nullptr;
}


void EnumWidget::dragEnterEvent(QDragEnterEvent *event)
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


void EnumWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        DragAndDropInfos Infos (mime->text());
        event->acceptProposedAction();
        QString VariableName = Infos.GetName();
        foreach (OneEnumVariable *Variable, m_Variables) {
            if (!VariableName.compare(Variable->GetName())) {
                return;  // Variable ist schon im Enum-Fenster vorhanden
            }
        }
        OneEnumVariable *Variable = new OneEnumVariable(VariableName, this);
        m_ObserverConnection.AddObserveVariable(Variable->GetVid(), OBSERVE_CONVERSION_CHANGED | OBSERVE_TYPE_CHANGED); //OBSERVE_CONFIG_ANYTHING_CHANGED);
        m_Layout->addWidget(Variable);
        m_Variables.append(Variable);
    } else {
        event->ignore();
    }
}


void EnumWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction (m_ConfigAct);
    menu.exec(event->globalPos());
}

void EnumWidget::CyclicUpdate()
{
    foreach (OneEnumVariable *Variable, m_Variables) {
        Variable->CyclicUpdate();
    }
}

void EnumWidget::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    //Q_UNUSED(arg_observationFlag);
    if (((arg_observationFlag & OBSERVE_CONVERSION_CHANGED) == OBSERVE_CONVERSION_CHANGED) ||
        ((arg_observationFlag & OBSERVE_TYPE_CHANGED) == OBSERVE_TYPE_CHANGED)) {
        if(arg_vid > 0) {
            foreach (OneEnumVariable *Variable, m_Variables) {
                if (arg_vid == Variable->GetVid()) {
                    Variable->UpdateButtons();
                }
            }
        }
    }
}

void EnumWidget::DeleteAllVariables()
{
    foreach (OneEnumVariable *Variable, m_Variables) {
        m_ObserverConnection.RemoveObserveVariable(Variable->GetVid());
        delete Variable;
    }
    m_Variables.clear();
}

void EnumWidget::openDialog()
{

}

void EnumWidget::ConfigureSlot(void)
{
#ifdef UNIFORM_DIALOGE
    QStringList loc_enumvars;
    for(int i = 0; i < m_Variables.count(); ++i) {
        loc_enumvars.append(m_Variables.at(i)->GetName());
    }
    emit openStandardDialog(loc_enumvars, false);
#else
    EnumDialog *dlg = new EnumDialog(this);
    dlg->setWindowTitle("Enum config");
    QStringList loc_currentVariables;

    dlg->setWindowName(GetWindowTitle());
    //Alle Widgetnamen finden
    for (int i = 0; i < m_Variables.count(); i++) {
        loc_currentVariables.append(m_Variables.at(i)->GetName());
    }
    dlg->setSelectedVariables(loc_currentVariables);
    if(dlg->exec() == QDialog::Accepted) {

        setWindowTitle(dlg->windowName());

        DeleteAllVariables();

        foreach (QString VariableName, dlg->selectedVariables()) {
            OneEnumVariable *Variable = new OneEnumVariable(VariableName, this);
            m_Variables.append(Variable);
            m_Layout->addWidget(Variable);
        }

        this->resize(this->sizeHint());
    }
    delete dlg;
#endif
}

void EnumWidget::changeColor(QColor arg_color)
{
    Q_UNUSED(arg_color)
}

void EnumWidget::changeFont(QFont arg_font)
{
    Q_UNUSED(arg_font)
}

void EnumWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void EnumWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    if(arg_visible) {
        OneEnumVariable *Variable = new OneEnumVariable(arg_variable, this);
        m_ObserverConnection.AddObserveVariable(Variable->GetVid(), OBSERVE_CONVERSION_CHANGED | OBSERVE_TYPE_CHANGED);
        m_Variables.append(Variable);
        m_Layout->addWidget(Variable);
    } else {
        for(int i = 0; i < m_Variables.count(); ++i) {
            if(m_Variables.at(i)->GetName().compare(arg_variable) == 0) {
                m_ObserverConnection.RemoveObserveVariable(m_Variables.at(i)->GetVid());
                m_Layout->removeWidget(m_Variables.at(i));
                OneEnumVariable *Variable = m_Variables.takeAt(i);
                delete Variable;
                break;
            }
        }
    }
}

void EnumWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_variables)
    Q_UNUSED(arg_visible)
}

void EnumWidget::resetDefaultVariables(QStringList arg_variables)
{
    DeleteAllVariables();

    foreach (QString VariableName, arg_variables) {
        OneEnumVariable *Variable = new OneEnumVariable(VariableName, this);
        m_ObserverConnection.AddObserveVariable(Variable->GetVid(), OBSERVE_CONVERSION_CHANGED | OBSERVE_TYPE_CHANGED);
        m_Variables.append(Variable);
        m_Layout->addWidget(Variable);
    }
}

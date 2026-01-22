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

#include "TextWidget.h"
#include "MdiWindowType.h"
#include "DragAndDrop.h"
#include <QFile>
#include <QBrush>
#include <QPalette>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <QDoubleValidator>
#include <QComboBox>
#include <QApplication>
#include "FileDialog.h"
#include "TextWindowChangeValueDialog.h"
#include "BlackboardInfoDialog.h"
#include "GetEventPos.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

extern "C" {
#include "MainValues.h"
#include "ThrowError.h"
#include "Files.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#include "FileExtensions.h"
#include "MyMemory.h"
#include "EquationParser.h"
}

#define UNIFORM_DIALOGE

TextWidget::TextWidget(QString par_WindowTitle, MdiSubWindow *par_SubWindow, MdiWindowType* par_Type, QWidget* par_parent) :
    MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, par_parent),
    m_ObserverConnection(this)
{
    m_ShowUnitColumn = s_main_ini_val.TextDefaultShowUnitColumn;
    m_ShowDisplayTypeColumn = s_main_ini_val.TextDefaultShowDispayTypeColumn;
    m_BackgroundColor = QColor(Qt::white);
    m_icon = 0;
    m_tableViewVariables = new TextTableView(this);

    m_dataModel = new TextTableModel();

    m_tableViewVariables->setModel(m_dataModel);
    // This should be happen after setModel, elsewise it will be ignored
    m_tableViewVariables->setColumnHidden(2, !m_ShowUnitColumn);
    m_tableViewVariables->setColumnHidden(3, !m_ShowDisplayTypeColumn);

    m_tableViewVariables->resizeColumnsToContents();

    if (strlen(s_main_ini_val.TextDefaultFont)) { // is there defined a default font?
        QFont Font(s_main_ini_val.TextDefaultFont, s_main_ini_val.TextDefaultFontSize);
        changeFont(Font);
    } else {
        QFont Font(QString("Segoe UI"), 10);
        changeFont(Font);
    }

    QVBoxLayout *loc_layout = new QVBoxLayout(this);
    loc_layout->addWidget(m_tableViewVariables);
    loc_layout->setContentsMargins(0, 0, 0, 0);
    setAcceptDrops(true);

    connect(m_tableViewVariables, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenu(QPoint)));
    connect(m_tableViewVariables->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(openDialog()));

    connect(m_dataModel, SIGNAL(columnWidthChanged(int,int,int,int)), m_tableViewVariables, SLOT(columnWidthChangeSlot(int,int,int,int)));

    m_ConfigAct = new QAction(tr("&config"), this);
    m_ConfigAct->setStatusTip(tr("configure text window"));
    connect(m_ConfigAct, SIGNAL(triggered()), this, SLOT(openDialog()));
    m_ShowUnitColumnAct = new QAction(tr("show &unit"), this);
    m_ShowUnitColumnAct->setStatusTip(tr("show unit column"));
    connect(m_ShowUnitColumnAct, SIGNAL(triggered()), this, SLOT(ShowUnitColumn()));
    m_ShowDisplayTypeColumnAct = new QAction(tr("show &display type"), this);
    m_ShowDisplayTypeColumnAct->setStatusTip(tr("show display column"));;
    connect(m_ShowDisplayTypeColumnAct, SIGNAL(triggered()), this, SLOT(ShowDisplayTypeColumn()));
    m_HideUnitColumnAct = new QAction(tr("hide &unit"), this);
    m_HideUnitColumnAct->setStatusTip(tr("hide unit column"));
    connect(m_HideUnitColumnAct, SIGNAL(triggered()), this, SLOT(HideUnitColumn()));
    m_HideDisplayTypeColumnAct = new QAction(tr("hide &display type"), this);
    m_HideDisplayTypeColumnAct->setStatusTip(tr("hide display column"));;
    connect(m_HideDisplayTypeColumnAct, SIGNAL(triggered()), this, SLOT(HideDisplayTypeColumn()));
    m_BlackboardInfosAct = new QAction(tr("blackboard &variable info"), this);
    m_BlackboardInfosAct->setStatusTip(tr("blackboard &variable info"));;
    connect(m_BlackboardInfosAct, SIGNAL(triggered()), this, SLOT(BlackboardInfos()));

    m_SaveThisToScriptAct = new QAction(tr("to script file"), this);
    connect(m_SaveThisToScriptAct, SIGNAL(triggered()), this, SLOT(SaveThisToScriptAct()));
    m_SaveAllToScriptAct = new QAction(tr("to script file"), this);
    connect(m_SaveAllToScriptAct, SIGNAL(triggered()), this, SLOT(SaveAllToScriptAct()));
    m_SaveThisToScriptAtomicAct = new QAction(tr("to script file with ATOMIC"), this);
    connect(m_SaveThisToScriptAtomicAct, SIGNAL(triggered()), this, SLOT(SaveThisToScriptAtomicAct()));
    m_SaveAllToScriptAtomicAct = new QAction(tr("to script file with ATOMIC"), this);
    connect(m_SaveAllToScriptAtomicAct, SIGNAL(triggered()), this, SLOT(SaveAllToScriptAtomicAct()));

    m_SaveThisToEquAct = new QAction(tr("to equation file"), this);
    connect(m_SaveThisToEquAct, SIGNAL(triggered()), this, SLOT(SaveThisToEquAct()));
    m_SaveAllToEquAct = new QAction(tr("to equation file"), this);
    connect(m_SaveAllToEquAct, SIGNAL(triggered()), this, SLOT(SaveAllToEquAct()));

    m_SaveThisToSnapshotAct = new QAction(tr("to snapshot"), this);
    connect(m_SaveThisToSnapshotAct, SIGNAL(triggered()), this, SLOT(SaveThisToSnapshotAct()));
    m_SaveAllToSnapshotAct = new QAction(tr("to snapshot"), this);
    connect(m_SaveAllToSnapshotAct, SIGNAL(triggered()), this, SLOT(SaveAllToSnapshotAct()));

    m_LoadFromEquAct = new QAction(tr("load from euation file"), this);
    connect(m_LoadFromEquAct, SIGNAL(triggered()), this, SLOT(LoadFromEquAct()));
    m_LoadFromSnapshotAct = new QAction(tr("load from snapshot"), this);
    connect(m_LoadFromSnapshotAct, SIGNAL(triggered()), this, SLOT(LoadFromSnapshotAct()));

    readFromIni();

    m_tableViewVariables->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

}

TextWidget::~TextWidget()
{
    writeToIni();
    // remove all connections to blackboard variable
    QList<TextTableModel::Variable*> loc_variableList = m_dataModel->getList();
    for(int i = 0; i < loc_variableList.size(); i++) {
        m_ObserverConnection.RemoveObserveVariable(loc_variableList.at(i)->m_vid);
        remove_bbvari_unknown_wait(loc_variableList.at(i)->m_vid);
    }
    delete m_tableViewVariables;
    delete m_dataModel;
    delete m_ConfigAct;
    delete m_ShowUnitColumnAct;
    delete m_ShowDisplayTypeColumnAct;
    delete m_HideUnitColumnAct;
    delete m_HideDisplayTypeColumnAct;
    delete m_BlackboardInfosAct;
    delete m_SaveThisToScriptAct;
    delete m_SaveAllToScriptAct;
    delete m_SaveThisToScriptAtomicAct;
    delete m_SaveAllToScriptAtomicAct;
    delete m_SaveThisToSnapshotAct;
    delete m_SaveAllToSnapshotAct;
    delete m_LoadFromEquAct;
    delete m_LoadFromSnapshotAct;
}

bool TextWidget::writeToIni()
{
    QString SectionPath = GetIniSectionPath();
    QString WindowTypeName = GetMdiWindowType()->GetWindowTypeName();
    int Fd = ScQt_GetMainFileDescriptor();

    ScQt_IniFileDataBaseWriteString(SectionPath, "type", WindowTypeName, Fd);
    int FontSize = m_tableViewVariables->font().pointSize();
    QString FontString = m_tableViewVariables->font().family();
    FontString.append(QString(", %1").arg(FontSize));
    ScQt_IniFileDataBaseWriteString(SectionPath, "Font", FontString, Fd);

    ScQt_IniFileDataBaseWriteInt(SectionPath, "icon", m_icon, Fd);
    unsigned int BkColor = 0;
    BkColor += m_BackgroundColor.red();
    BkColor += m_BackgroundColor.green() << 8;
    BkColor += m_BackgroundColor.blue() << 16;
    QString ColorString = QString("0x%1").arg(BkColor, 0, 16);
    ScQt_IniFileDataBaseWriteString(SectionPath, "BgColor", ColorString, Fd);

    QList<TextTableModel::Variable*> loc_variableList = m_dataModel->getList();
    int i;
    int loc_logicalIndex = 0;
    // 0 -> dez, 1 -> hex,
    // 2 -> bin, 3 -> phys
    for(i = 0; i < loc_variableList.size(); i++) {
        loc_logicalIndex = m_tableViewVariables->verticalHeader()->logicalIndex(i);
        QString Entry = QString("E%1").arg(i);
        QString Line = QString().number(loc_variableList.at(loc_logicalIndex)->m_type);
        Line.append(",");
        Line.append(loc_variableList.at(loc_logicalIndex)->m_name);
        ScQt_IniFileDataBaseWriteString(SectionPath, Entry, Line, Fd);
    }
    for(; ; i++) {
        QString Entry = QString("E%1").arg(i);
        QString Line = ScQt_IniFileDataBaseReadString(SectionPath, Entry, "", Fd);
        if (Line.isEmpty()) {
            break;
        }
        ScQt_IniFileDataBaseWriteString(SectionPath, Entry, nullptr, Fd);
    }

    ScQt_IniFileDataBaseWriteYesNo(SectionPath, "ShowUnitColumn", m_ShowUnitColumn, Fd);
    ScQt_IniFileDataBaseWriteYesNo(SectionPath, "ShowDisplayTypeColumn", m_ShowDisplayTypeColumn, Fd);

    ScQt_IniFileDataBaseWriteInt(SectionPath, "ColumnAlignmentName", m_dataModel->columnAlignment(0), Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "ColumnAlignmentValue",  m_dataModel->columnAlignment(1), Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "ColumnAlignmentUnit",  m_dataModel->columnAlignment(2), Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "ColumnAlignmentDisplayType", m_dataModel->columnAlignment(3), Fd);

    return true;
}

bool TextWidget::readFromIni()
{
    QString SectionPath = GetIniSectionPath();
    int BackgroundColor;
    int Fd = ScQt_GetMainFileDescriptor();

    QString FontString = ScQt_IniFileDataBaseReadString(SectionPath, "Font", "", Fd);
    if (!FontString.isEmpty()) {
        QStringList List = FontString.split(",");
        if (List.size() == 2) {
            QFont Font(QString(List.at(0).trimmed()), List.at(1).toUInt());
            changeFont(Font);
        }
    }
    m_icon = ScQt_IniFileDataBaseReadInt(SectionPath, "icon", 0, Fd);
    BackgroundColor = ScQt_IniFileDataBaseReadInt(SectionPath, "BgColor", 0xFFFFFF, Fd);
    if (s_main_ini_val.DarkMode) {
        if (BackgroundColor == 0xFFFFFF) {
            BackgroundColor = 0;
        }
    } else {
        if (BackgroundColor == 0) {
            BackgroundColor = 0xFFFFFF;
        }
    }
    m_BackgroundColor = QColor(BackgroundColor&0x000000FF, (BackgroundColor&0x0000FF00)>>8, (BackgroundColor&0x00FF0000)>>16); // 0x00bbggrr

    m_dataModel->setBackgroundColor(m_BackgroundColor);

    for(int i = 0; ; i++) {
        QString Entry = QString("E%1").arg(i);
        QString Line = ScQt_IniFileDataBaseReadString(SectionPath, Entry, "", Fd);
        if (Line.isEmpty()) {
            break;
        } else {
            QStringList List = Line.split(",");
            if(List.size() == 2) {
                addBlackboardVariableToModel(List.at(1).trimmed(), -1, List.at(0).toInt());
            }
        }
    }

    m_dataModel->setColumnAlignment(0, ScQt_IniFileDataBaseReadInt(SectionPath, "ColumnAlignmentName", static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft), Fd));
    m_dataModel->setColumnAlignment(1, ScQt_IniFileDataBaseReadInt(SectionPath, "ColumnAlignmentValue", static_cast<int>(Qt::AlignVCenter | Qt::AlignRight), Fd));
    m_dataModel->setColumnAlignment(2, ScQt_IniFileDataBaseReadInt(SectionPath, "ColumnAlignmentUnit", static_cast<int>(Qt::AlignVCenter | Qt::AlignHCenter), Fd));
    m_dataModel->setColumnAlignment(3, ScQt_IniFileDataBaseReadInt(SectionPath, "ColumnAlignmentDisplayType", static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft), Fd));

    m_ShowUnitColumn = ScQt_IniFileDataBaseReadYesNo(SectionPath, "ShowUnitColumn", m_ShowUnitColumn, Fd);
    m_ShowDisplayTypeColumn = ScQt_IniFileDataBaseReadYesNo(SectionPath, "ShowDisplayTypeColumn", m_ShowDisplayTypeColumn, Fd);
    m_tableViewVariables->setColumnHidden(2, !m_ShowUnitColumn);
    m_tableViewVariables->setColumnHidden(3, !m_ShowDisplayTypeColumn);

    m_tableViewVariables->resizeColumnsToContents();
    return true;
}

CustomDialogFrame* TextWidget::dialogSettings(QWidget* arg_parent)
{
    // NOTE: required by inheritance but not used
    arg_parent->setWindowTitle("Text config");
    return nullptr;
}

void TextWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasText()) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void TextWidget::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void TextWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void TextWidget::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        DragAndDropInfos Infos (mime->text());
        if (event->source() == this) {
            QString loc_variableName = Infos.GetName();
            int loc_oldRow = m_dataModel->getRowIndex(loc_variableName);
            int Pos = GetDropEventYPos(event);
            int HeaderHeight = m_tableViewVariables->horizontalHeader()->height();
            Pos -= HeaderHeight;
            int loc_newRow = m_tableViewVariables->rowAt(Pos);

            m_dataModel->moveRow(QModelIndex(), loc_oldRow, QModelIndex(), loc_newRow);

            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
            QString loc_variableName = Infos.GetName();
            bool loc_notRegistered = true;
            foreach(TextTableModel::Variable *loc_variable, m_dataModel->getList()) {
                if(!loc_variable->m_name.compare(loc_variableName)) {
                    loc_notRegistered = false;
                    break;
                }
            }
            if(loc_notRegistered) {
                int Pos = GetDropEventYPos(event);
                int HeaderHeight = m_tableViewVariables->horizontalHeader()->height();
                Pos -= HeaderHeight;
                int Row = m_tableViewVariables->rowAt(Pos);
                addBlackboardVariableToModel(loc_variableName, Row);
                m_tableViewVariables->resizeColumnsToContents();
            }
        }
    } else {
        event->ignore();
    }
}

void TextWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(m_tableViewVariables->selectionModel()->selectedIndexes().size() > 0) {
        QModelIndex loc_index = m_tableViewVariables->selectionModel()->selectedIndexes().first();

        if(loc_index.isValid()) {
            if(loc_index.model()) {
                TextTableModel::Variable *loc_variable = m_dataModel->getVariable(loc_index.row());
                TextWindowChangeValueDialog *loc_dlg = new TextWindowChangeValueDialog(this);
                loc_dlg->SetVid(loc_variable->m_vid);
                loc_dlg->setVariableName(loc_variable->m_name);

                int Base = 10;
                switch(loc_variable->m_type) {
                    case 0:
                        loc_dlg->setDisplaytype(TextWindowChangeValueDialog::Displaytype::decimal);
                        break;
                    case 1:
                        loc_dlg->setDisplaytype(TextWindowChangeValueDialog::Displaytype::hexadecimal);
                        Base = 16;
                        break;
                    case 2:
                        loc_dlg->setDisplaytype(TextWindowChangeValueDialog::Displaytype::binary);
                        Base = 2;
                        break;
                    case 3:
                        loc_dlg->setDisplaytype(TextWindowChangeValueDialog::Displaytype::physically);
                        break;
                    default:
                        break;
                }
                union BB_VARI ValueUnion;
                enum BB_DATA_TYPES Type = read_bbvari_union_type(loc_variable->m_vid, &ValueUnion);
                loc_dlg->setValue(ValueUnion, Type, Base);
                loc_dlg->setStep(loc_variable->m_step);
                if(loc_variable->m_stepType) {
                    loc_dlg->setLinear(false);
                } else {
                    loc_dlg->setLinear(true);
                }

                if(loc_dlg->exec() == QDialog::Accepted) {
                    Type = loc_dlg->value(&ValueUnion);
                    write_bbvari_convert_to (GET_PID (), loc_variable->m_vid, static_cast<int>(Type), &ValueUnion);
                    QModelIndex loc_columnIndex;
                    loc_columnIndex = m_dataModel->index(loc_index.row(), 2, QModelIndex());
                    switch(loc_dlg->displaytype()) {
                    case TextWindowChangeValueDialog::Displaytype::decimal:
                        m_dataModel->setData(loc_columnIndex, 0, Qt::EditRole);
                        break;
                    case TextWindowChangeValueDialog::Displaytype::hexadecimal:
                        m_dataModel->setData(loc_columnIndex, 1, Qt::EditRole);
                        break;
                    case TextWindowChangeValueDialog::Displaytype::binary:
                        m_dataModel->setData(loc_columnIndex, 2, Qt::EditRole);
                        break;
                    case TextWindowChangeValueDialog::Displaytype::physically:
                        m_dataModel->setData(loc_columnIndex, 3, Qt::EditRole);
                        break;
                    }

                    m_dataModel->setStep(loc_columnIndex.row(), loc_dlg->step());

                    if(loc_dlg->isLinear()) {
                        m_dataModel->setStepType(loc_columnIndex.row(), 0);
                    } else {
                        m_dataModel->setStepType(loc_columnIndex.row(), 1);
                    }
                }

                delete loc_dlg;
            }
        }
    }
    event->accept();
}

void TextWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_startDragPosition = GetEventGlobalPos(event);
    } else {
        if(event->button() == Qt::MiddleButton) {
            if(m_tableViewVariables->selectionModel()->selectedIndexes().size() > 0) {
                QModelIndex loc_index = m_tableViewVariables->selectionModel()->selectedIndexes().first();
                if(loc_index.isValid()) {
                    if(loc_index.model()) {
                        TextTableModel::Variable *loc_variable = m_dataModel->getVariable(loc_index.row());
                        BlackboardInfoDialog Dlg;
                        Dlg.editCurrentVariable(loc_variable->m_name);

                        if (Dlg.exec() == QDialog::Accepted) {
                            ;
                        }
                    }
                }
            }
        }
    }

    event->accept();
}

void TextWidget::mouseMoveEvent(QMouseEvent* event)
{
    if(!(event->buttons() & Qt::LeftButton)) {
        event->accept();
        return;
    }
    if((GetEventGlobalPos(event) - m_startDragPosition).manhattanLength() < QApplication::startDragDistance()) {
        event->accept();
        return;
    }

    QModelIndex loc_index = m_tableViewVariables->indexAt(m_tableViewVariables->viewport()->mapFromGlobal(m_startDragPosition));
    if(loc_index.isValid()) {
        char loc_variableName[BBVARI_NAME_SIZE];
        TextTableModel::Variable* Var = m_dataModel->getVariable(loc_index.row());
        VID loc_vid = Var->m_vid;
        GetBlackboardVariableName(loc_vid, loc_variableName, sizeof(loc_variableName));
        DragAndDropInfos Infos;
        QString Name(loc_variableName);
        Infos.SetName(Name);
        Infos.SetMinMaxValue(get_bbvari_min(loc_vid), get_bbvari_max(loc_vid));
        Infos.SetColor(get_bbvari_color(loc_vid));
        enum DragAndDropDisplayMode DisplayType;
        switch(Var->m_type) {
        case 0:
            DisplayType = DISPLAY_MODE_DEC;
            break;
        case 1:
            DisplayType = DISPLAY_MODE_HEX;
            break;
        case 2:
            DisplayType = DISPLAY_MODE_BIN;
            break;
        case 3:
            DisplayType = DISPLAY_MODE_PHYS;
            break;
        default:
            DisplayType = DISPLAY_MODE_UNKNOWN;
            break;
        }
        Infos.SetDisplayMode(DisplayType);

        QDrag *loc_dragObject = buildDragObject(this, &Infos);
        loc_dragObject->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
    }
    event->accept();
}

void TextWidget::BlackboardInfos()
{
    QModelIndex Index = m_tableViewVariables->currentIndex();
    Index = m_tableViewVariables->model()->index(Index.row(), 0);
    if (Index.isValid()) {
        QString Name = Index.data(Qt::DisplayRole).toString();
        BlackboardInfoDialog Dlg;
        Dlg.editCurrentVariable(Name);

        if (Dlg.exec() == QDialog::Accepted) {
            ;
        }
    }
}

void TextWidget::SaveToFile(enum WhatType par_Type)
{
    QTextStream Stream;
    QFile File;

    if ((par_Type == SAVE_THIS_TO_TEMP_BUFFER) || (par_Type == SAVE_ALL_TO_TEMP_BUFFER)) {
        Stream.setString(&m_SnapshotBuffer);
    } else {
        QString FileName;
        if ((par_Type == SAVE_THIS_TO_EQU) || (par_Type == SAVE_ALL_TO_EQU)) {
            FileName = FileDialog::getSaveFileName(this, QString ("Equation file name"), QString(), QString (TRIGGER_EXT));
        } else {
            FileName = FileDialog::getSaveFileName(this, QString ("Script file name"), QString(), QString (SCRIPT_EXT));
        }
        if(FileName.isEmpty()) return;
        File.setFileName(FileName);
        if (File.open(QFile::WriteOnly | QFile::Truncate)) {
            Stream.setDevice(&File);
        } else {
            ThrowError(1, "cannot write to file %s", FileName.toLatin1().data());
        }
    }
    if (1) {
        if ((par_Type == SAVE_THIS_TO_SCRIPT_ATOMIC) || (par_Type == SAVE_ALL_TO_SCRIPT_ATOMIC)) {
            Stream << "ATOMIC\n";
        }
        if ((par_Type == SAVE_ALL_TO_SCRIPT) || (par_Type == SAVE_ALL_TO_SCRIPT_ATOMIC) ||
            (par_Type == SAVE_ALL_TO_EQU)) {
            QList<MdiWindowWidget*> TextWindowList;
            TextWindowList = GetMdiWindowType()->GetAllOpenWidgetsOfThisType();
            foreach(MdiWindowWidget *Window, TextWindowList) {
                (static_cast<TextWidget*>(Window))->SaveToFile(par_Type, Stream);
            }
        } else {
            SaveToFile(par_Type, Stream);
        }
        if ((par_Type == SAVE_THIS_TO_SCRIPT_ATOMIC) || (par_Type == SAVE_ALL_TO_SCRIPT_ATOMIC)) {
            Stream << "END_ATOMIC\n";
        }
        Stream.flush();
        if (File.isOpen()) {
            File.close();
        }
    }
}

void TextWidget::SaveToFile(enum WhatType par_Type, QTextStream &Stream)
{
    switch(par_Type) {
    case SAVE_THIS_TO_SCRIPT:
    case SAVE_ALL_TO_SCRIPT:
        Stream << "/* From window " << GetWindowTitle() << " */\n";
        m_dataModel->WriteContentToFile(false, TextTableModel::SCRIPT_FILE_TYPE, Stream);
        break;
    case SAVE_THIS_TO_SCRIPT_ATOMIC:
    case SAVE_ALL_TO_SCRIPT_ATOMIC:
        Stream << "/* From window " << GetWindowTitle() << " */\n";
        m_dataModel->WriteContentToFile(true, TextTableModel::SCRIPT_FILE_TYPE, Stream);
        break;
    case SAVE_THIS_TO_EQU:
    case SAVE_ALL_TO_EQU:
    case SAVE_THIS_TO_TEMP_BUFFER:
    case SAVE_ALL_TO_TEMP_BUFFER:
        Stream << "; From window " << GetWindowTitle() << "\n";
        m_dataModel->WriteContentToFile(false, TextTableModel::EQU_FILE_TYPE, Stream);
        break;
    }
}

void TextWidget::SaveThisToScriptAct()
{
    SaveToFile(SAVE_THIS_TO_SCRIPT);
}

void TextWidget::SaveAllToScriptAct()
{
    SaveToFile(SAVE_ALL_TO_SCRIPT);
}

void TextWidget::SaveThisToScriptAtomicAct()
{
    SaveToFile(SAVE_THIS_TO_SCRIPT_ATOMIC);
}

void TextWidget::SaveAllToScriptAtomicAct()
{
    SaveToFile(SAVE_ALL_TO_SCRIPT_ATOMIC);
}

void TextWidget::SaveThisToEquAct()
{
    SaveToFile(SAVE_THIS_TO_EQU);
}

void TextWidget::SaveAllToEquAct()
{
    SaveToFile(SAVE_ALL_TO_EQU);
}

void TextWidget::SaveThisToSnapshotAct()
{
    SaveToFile(SAVE_THIS_TO_TEMP_BUFFER);
}

void TextWidget::SaveAllToSnapshotAct()
{
    SaveToFile(SAVE_ALL_TO_TEMP_BUFFER);
}

void TextWidget::LoadFromEquAct()
{
    QString FileName;
    char *ErrString = nullptr;
    FileName = FileDialog::getOpenFileName(this, QString ("Equation file name"), QString(), QString (TRIGGER_EXT));
    if(FileName.isEmpty()) return;
    if (DirectSolveEquationFile (FileName.toLatin1().data(), nullptr, &ErrString)) {
        ThrowError(1, "cannot load file %s because of error %s", FileName.toLatin1().data(),
                   (ErrString == nullptr) ? "unknown" : ErrString);
        if (ErrString != nullptr) {
            my_free(ErrString);
        }
    }
}

void TextWidget::LoadFromSnapshotAct()
{
    if (!m_SnapshotBuffer.isEmpty()) {
        char *ErrString = nullptr;
        if (DirectSolveEquationFile (nullptr, m_SnapshotBuffer.toLatin1().data(), &ErrString)) {
            ThrowError(1, "cannot load snapshot because of error %s",
                       (ErrString == nullptr) ? "unknown" : ErrString);
            if (ErrString != nullptr) {
                my_free(ErrString);
            }
        }
    }
}

static QString ConvertAlignmentEnumToString(int par_Alignement)
{
    if ((par_Alignement & Qt::AlignLeft) == Qt::AlignLeft) return QString("Left");
    if ((par_Alignement & Qt::AlignLeft) == Qt::AlignRight) return QString("Right");
    if ((par_Alignement & Qt::AlignLeft) == Qt::AlignHCenter) return QString("Center");
    return QString("Right");
}

static int ConvertStringToAlignmentEnum(const QString &par_Alignement)
{
    if (!par_Alignement.compare("Left")) return Qt::AlignVCenter |Qt::AlignLeft;
    if (!par_Alignement.compare("Right")) return Qt::AlignVCenter |Qt::AlignRight;
    if (!par_Alignement.compare("Center")) return Qt::AlignVCenter |Qt::AlignHCenter;
    return Qt::AlignVCenter |Qt::AlignRight;
}

void TextWidget::customContextMenu(QPoint arg_point)
{
    QMenu menu(this);
    menu.addAction (m_ConfigAct);
    menu.addSection(QString());
    if (m_ShowUnitColumn) menu.addAction (m_HideUnitColumnAct);
    else menu.addAction (m_ShowUnitColumnAct);
    if (m_ShowDisplayTypeColumn) menu.addAction (m_HideDisplayTypeColumnAct);
    else menu.addAction (m_ShowDisplayTypeColumnAct);
    QMenu *AlignmentMenu = menu.addMenu("Column alignment");

    // Column 0 (Label name)
    QMenu *NameAlignmentMenu = AlignmentMenu->addMenu("Name");
    QComboBox *NameComboBox = new QComboBox(NameAlignmentMenu);
    NameComboBox->addItem("Right");
    NameComboBox->addItem("Left");
    NameComboBox->addItem("Center");
    NameComboBox->setCurrentText(ConvertAlignmentEnumToString(this->m_dataModel->columnAlignment(0)));
    QWidgetAction *NameComboBoxAction = new QWidgetAction(NameAlignmentMenu);
    NameComboBoxAction->setDefaultWidget(NameComboBox);
    NameAlignmentMenu->addAction(NameComboBoxAction);

    // Column 1 (Value)
    QMenu *ValueAlignmentMenu = AlignmentMenu->addMenu("Value");
    QComboBox *ValueComboBox = new QComboBox(ValueAlignmentMenu);
    ValueComboBox->addItem("Right");
    ValueComboBox->addItem("Left");
    ValueComboBox->addItem("Center");
    ValueComboBox->setCurrentText(ConvertAlignmentEnumToString(this->m_dataModel->columnAlignment(1)));
    QWidgetAction *ValueComboBoxAction = new QWidgetAction(ValueAlignmentMenu);
    ValueComboBoxAction->setDefaultWidget(ValueComboBox);
    ValueAlignmentMenu->addAction(ValueComboBoxAction);

    QComboBox *UnitComboBox = nullptr;
    if (m_ShowUnitColumn) {
        // Column 1 (Value)
        QMenu *UnitAlignmentMenu = AlignmentMenu->addMenu("Unit");
        UnitComboBox = new QComboBox(UnitAlignmentMenu);
        UnitComboBox->addItem("Right");
        UnitComboBox->addItem("Left");
        UnitComboBox->addItem("Center");
        UnitComboBox->setCurrentText(ConvertAlignmentEnumToString(this->m_dataModel->columnAlignment(2)));
        QWidgetAction *UnitComboBoxAction = new QWidgetAction(UnitAlignmentMenu);
        UnitComboBoxAction->setDefaultWidget(UnitComboBox);
        UnitAlignmentMenu->addAction(UnitComboBoxAction);
    }

    QComboBox *DispComboBox = nullptr;
    if (m_ShowDisplayTypeColumn) {
        // Column 1 (Value)
        QMenu *DispAlignmentMenu = AlignmentMenu->addMenu("Display type");
        DispComboBox = new QComboBox(DispAlignmentMenu);
        DispComboBox->addItem("Right");
        DispComboBox->addItem("Left");
        DispComboBox->addItem("Center");
        DispComboBox->setCurrentText(ConvertAlignmentEnumToString(this->m_dataModel->columnAlignment(3)));
        QWidgetAction *DispComboBoxAction = new QWidgetAction(DispAlignmentMenu);
        DispComboBoxAction->setDefaultWidget(DispComboBox);
        DispAlignmentMenu->addAction(DispComboBoxAction);
    }

    QModelIndex Index = m_tableViewVariables->currentIndex();
    if(Index.isValid()) {
        menu.addAction (m_BlackboardInfosAct);
    }

    QMenu *SaveMenu = menu.addMenu("Save content");
    QMenu *SaveOfThisMenu = SaveMenu->addMenu("of this");
    QMenu *SaveOfAllMenu = SaveMenu->addMenu("of all");
    SaveOfThisMenu->addAction (m_SaveThisToSnapshotAct);
    SaveOfAllMenu->addAction (m_SaveAllToSnapshotAct);
    SaveOfThisMenu->addAction (m_SaveThisToEquAct);
    SaveOfAllMenu->addAction (m_SaveAllToEquAct);
    SaveOfThisMenu->addAction (m_SaveThisToScriptAct);
    SaveOfAllMenu->addAction (m_SaveAllToScriptAct);
    SaveOfThisMenu->addAction (m_SaveThisToScriptAtomicAct);
    SaveOfAllMenu->addAction (m_SaveAllToScriptAtomicAct);

    menu.addAction (m_LoadFromEquAct);
    if (!m_SnapshotBuffer.isEmpty()) {
        menu.addAction (m_LoadFromSnapshotAct);
    }

    menu.exec(mapToGlobal(arg_point));
    this->m_dataModel->setColumnAlignment(0, ConvertStringToAlignmentEnum(NameComboBox->currentText()));
    this->m_dataModel->setColumnAlignment(1, ConvertStringToAlignmentEnum(ValueComboBox->currentText()));
    if (m_ShowUnitColumn && (UnitComboBox != nullptr)) {
        this->m_dataModel->setColumnAlignment(2, ConvertStringToAlignmentEnum(UnitComboBox->currentText()));
    }
    if (m_ShowDisplayTypeColumn && (DispComboBox != nullptr)) {
        this->m_dataModel->setColumnAlignment(3, ConvertStringToAlignmentEnum(DispComboBox->currentText()));
    }
}

void TextWidget::addBlackboardVariableToModel(QString arg_variableName, int arg_Row, int arg_displayType)
{
    char loc_unit[BBVARI_UNIT_SIZE];
    VID loc_vid = add_bbvari(QStringToConstChar(arg_variableName), BB_UNKNOWN_WAIT, nullptr);
    m_ObserverConnection.AddObserveVariable(loc_vid, OBSERVE_CONFIG_ANYTHING_CHANGED);
    int loc_rowCount;
    if (arg_Row < 0) {
        loc_rowCount = m_dataModel->rowCount();
    } else {
        loc_rowCount = arg_Row;
    }
    m_dataModel->insertRows(loc_rowCount, 1, QModelIndex());
    QModelIndex loc_index = m_dataModel->index(loc_rowCount, 0, QModelIndex());
    m_dataModel->setName(loc_rowCount, arg_variableName);
    m_dataModel->setVidValue(loc_rowCount, loc_vid);
    m_dataModel->setVariablePrecision(loc_rowCount, get_bbvari_format_prec(loc_vid));
    get_bbvari_unit(loc_vid, loc_unit, BBVARI_UNIT_SIZE);
    m_dataModel->setUnit(loc_rowCount, QString::fromLocal8Bit(loc_unit));

    bool Exist;
    if (loc_vid > 0) {
        enum BB_DATA_TYPES loc_dataType = static_cast<enum BB_DATA_TYPES>(get_bbvaritype(loc_vid));
        m_dataModel->setVariableDataType(loc_index.row(), loc_dataType);
        Exist = (loc_dataType != BB_UNKNOWN_WAIT);
    } else {
        Exist = false;
    }
    m_dataModel->setVariableExists(loc_index.row(), Exist);

    int loc_conversionType;
    if (Exist) {
        loc_conversionType = get_bbvari_conversiontype(loc_vid);
        if((loc_conversionType == BB_CONV_FORMULA) ||
            (loc_conversionType == BB_CONV_TEXTREP) ||
            (loc_conversionType == BB_CONV_FACTOFF) ||
            (loc_conversionType == BB_CONV_OFFFACT) ||
            (loc_conversionType == BB_CONV_TAB_INTP) ||
            (loc_conversionType == BB_CONV_TAB_NOINTP) ||
            (loc_conversionType == BB_CONV_RAT_FUNC)) {
            m_dataModel->setVariableHasConversion(loc_index.row(), true);
        }
    } else {
        loc_conversionType = 0;
    }

    if (arg_displayType >= 0) {
        m_dataModel->setDisplayType(loc_index.row(), arg_displayType);  // For variables with conversion phys is the default display type
    } else {
        if(Exist && ((loc_conversionType == BB_CONV_FORMULA) ||
                     (loc_conversionType == BB_CONV_TEXTREP) ||
                      (loc_conversionType == BB_CONV_FACTOFF) ||
                      (loc_conversionType == BB_CONV_OFFFACT) ||
                      (loc_conversionType == BB_CONV_TAB_INTP) ||
                      (loc_conversionType == BB_CONV_TAB_NOINTP) ||
                      (loc_conversionType == BB_CONV_RAT_FUNC))) {
            m_dataModel->setDisplayType(loc_index.row(), 3);  // For variables with conversion phys is the default display type
        } else {
            m_dataModel->setDisplayType(loc_index.row(), 0);  // For variables without conversion raw is the default display type
        }
    }
    double Step;
    unsigned char StepType;
    if (get_bbvari_step(loc_vid, &StepType, &Step) == 0) {
        m_dataModel->setStep(loc_index.row(), Step);
        m_dataModel->setStepType(loc_index.row(), StepType);
    }

    if (Exist) {
        m_dataModel->CyclicUpdateValues(loc_rowCount, loc_rowCount, true);   // update display
    }
}

void TextWidget::CyclicUpdate()
{
    m_dataModel->CyclicUpdateValues(0, 1000, false);
}

void TextWidget::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    m_dataModel->blackboardVariableConfigChanged(arg_vid, arg_observationFlag);
}

void TextWidget::openDialog()
{
    m_dataModel->makeBackup();
    emit openStandardDialog(m_dataModel->getAllVariableNames(), true, true, m_BackgroundColor, m_tableViewVariables->font());
}

void TextWidget::ShowUnitColumn()
{
    m_ShowUnitColumn = true;
    m_tableViewVariables->setColumnHidden(2, !m_ShowUnitColumn);
}

void TextWidget::ShowDisplayTypeColumn()
{
    m_ShowDisplayTypeColumn = true;
    m_tableViewVariables->setColumnHidden(3, !m_ShowDisplayTypeColumn);
}

void TextWidget::HideUnitColumn()
{
    m_ShowUnitColumn = false;
    m_tableViewVariables->setColumnHidden(2, !m_ShowUnitColumn);
}

void TextWidget::HideDisplayTypeColumn()
{
    m_ShowDisplayTypeColumn = false;
    m_tableViewVariables->setColumnHidden(3, !m_ShowDisplayTypeColumn);
}

void TextWidget::changeFont(QFont arg_newFont)
{
    m_tableViewVariables->setFont(arg_newFont);
    m_dataModel->setFont(arg_newFont);  // The Model need also the font for column width calculation
    m_tableViewVariables->horizontalHeader()->setFont(arg_newFont);
    m_tableViewVariables->verticalHeader()->setFont(arg_newFont);
    QFontMetrics loc_FontMetrics(arg_newFont);
    int loc_FontSize = loc_FontMetrics.height();
    int loc_RowHeight;
    if ((loc_FontSize / 8) < 2) {
        loc_RowHeight = loc_FontSize + loc_FontSize / 8;
    } else {
        loc_RowHeight = loc_FontSize + 2;
    }
    m_tableViewVariables->verticalHeader()->setDefaultSectionSize(loc_RowHeight);
    m_tableViewVariables->verticalHeader()->setMaximumSectionSize(loc_RowHeight);
    m_tableViewVariables->verticalHeader()->setMinimumSectionSize(loc_RowHeight);
}

void TextWidget::changeColor(QColor arg_color)
{
    m_BackgroundColor = arg_color;
    m_dataModel->setBackgroundColor(m_BackgroundColor);
}

void TextWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void TextWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    if(arg_visible) {
        if(!m_dataModel->getAllVariableNames().contains(arg_variable)) {
            addBlackboardVariableToModel(arg_variable);
        }
    } else {
        int loc_row = m_dataModel->getRowIndex(arg_variable);
        if(loc_row >= 0) {
            m_dataModel->removeRows(loc_row, 1);
        }
    }
}

void TextWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    if(arg_visible) {
        for(int i = 0; i < arg_variables.size(); ++i) {
            if(!m_dataModel->getAllVariableNames().contains(arg_variables.at(i))) {
                addBlackboardVariableToModel(arg_variables.at(i));
            }
        }
    } else {
        for(int i = 0; i < arg_variables.size(); ++i) {
            int loc_row = m_dataModel->getRowIndex(arg_variables.at(i));
            if(loc_row >= 0) {
                m_dataModel->removeRows(loc_row, 1);
            }
        }
    }
}

void TextWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables);
    m_dataModel->restoreBackup();
}

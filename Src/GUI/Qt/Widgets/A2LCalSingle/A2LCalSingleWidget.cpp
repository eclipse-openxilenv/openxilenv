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


#include "A2LCalSingleWidget.h"
#include "MdiWindowType.h"
#include "DragAndDrop.h"
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
#include "A2LCalWidgetSync.h"
#include "A2LCalSingleConfigDlg.h"
#include "TextWindowChangeValueDialog.h"
#include "A2LCalSinglePropertiesDlg.h"
#include "GetEventPos.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

extern "C" {
#include "MainValues.h"
#include "ThrowError.h"
#include "Files.h"
#include "Scheduler.h"
#include "A2LLinkThread.h"
#include "IniDataBase.h"
}

#define UNIFORM_DIALOGE

A2LCalSingleWidget::A2LCalSingleWidget(QString par_WindowTitle, MdiSubWindow *par_SubWindow, MdiWindowType* par_Type, QWidget* par_parent) :
    MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, par_parent)
{
    m_LinkNo = -1;

    m_ShowUnitColumn = s_main_ini_val.TextDefaultShowUnitColumn;
    m_ShowDisplayTypeColumn = s_main_ini_val.TextDefaultShowDispayTypeColumn;
    m_BackgroundColor = QColor(Qt::white);
    m_icon = 0;
    m_tableViewVariables = new A2LCalSingleTableView(this);
    m_tableViewVariables->setEditTriggers(QAbstractItemView::EditKeyPressed |
                                          QAbstractItemView::DoubleClicked |
                                          QAbstractItemView::SelectedClicked);

    m_DelegateEditor = new A2LCalSingleDelegateEditor(this);
    m_tableViewVariables->setItemDelegate(m_DelegateEditor);

    m_Model = new A2LCalSingleModel(this);
    m_tableViewVariables->horizontalHeader()->setStretchLastSection(true);
    m_tableViewVariables->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_tableViewVariables->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    m_tableViewVariables->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);  // Fixed?

    m_tableViewVariables->setModel(m_Model);
    // This should be done after setModel(), otherwise it will ignored
    m_tableViewVariables->setColumnHidden(2, !m_ShowUnitColumn);
    m_tableViewVariables->setColumnHidden(3, !m_ShowDisplayTypeColumn);

    if (strlen(s_main_ini_val.TextDefaultFont)) { // are there a default font defined
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

    connect(m_Model, SIGNAL(columnWidthChanged(int,int,int,int)), m_tableViewVariables, SLOT(columnWidthChangeSlot(int,int,int,int)));

    m_ConfigAct = new QAction(tr("&config"), this);
    m_ConfigAct->setStatusTip(tr("configure window"));
    connect(m_ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigSlot()));

    m_UpdateAct = new QAction(tr("&update"), this);
    m_UpdateAct->setStatusTip(tr("update window"));
    connect(m_UpdateAct, SIGNAL(triggered()), this, SLOT(UpdateSlot()));

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
    m_CharacteristicPropertiesAct = new QAction(tr("characteristic properties"), this);
    connect(m_CharacteristicPropertiesAct, SIGNAL(triggered()), this, SLOT(CharacteristicProperties()));

    // Check if the SyncObject exists
    SetupA2LCalWidgetSyncObject(this);

    AddA2LCalSingleWidgetToSyncObject(this);

    readFromIni();

    m_tableViewVariables->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
}

A2LCalSingleWidget::~A2LCalSingleWidget()
{
    writeToIni();
    RemoveA2LCalSingleWidgetFromSyncObject(this);
    A2LCloseLink(m_LinkNo, QStringToConstChar(m_Model->GetProcessName()));
}

bool A2LCalSingleWidget::writeToIni()
{
    QString SectionPath = GetIniSectionPath();
    char loc_txt[INI_MAX_LINE_LENGTH];
    char loc_entry[INI_MAX_ENTRYNAME_LENGTH];
    int loc_BkColor = 0;
    int Fd = ScQt_GetMainFileDescriptor();

    ScQt_IniFileDataBaseWriteString(SectionPath, "type", QStringToConstChar(GetMdiWindowType()->GetWindowTypeName()), Fd);

    ScQt_IniFileDataBaseWriteString(SectionPath, "process", QStringToConstChar(m_Model->GetProcessName()), Fd);

    int loc_FontSize = m_tableViewVariables->font().pointSize();
    sprintf (loc_txt, "%s, %i", QStringToConstChar(m_tableViewVariables->font().family()), loc_FontSize);
    ScQt_IniFileDataBaseWriteString(SectionPath, "Font", loc_txt, Fd);

    ScQt_IniFileDataBaseWriteInt(SectionPath, "icon", m_icon, Fd);
    QColor loc_BkQColor = m_BackgroundColor;
    loc_BkColor += loc_BkQColor.red();
    loc_BkColor += loc_BkQColor.green() << 8;
    loc_BkColor += loc_BkQColor.blue() << 16;
    sprintf(loc_txt, "0x%08X", loc_BkColor);
    ScQt_IniFileDataBaseWriteString(SectionPath, "BgColor", loc_txt, Fd);

    QList<A2LCalSingleData*> loc_variableList = m_Model->getList();
    int i;
    int loc_logicalIndex = 0;
    // 0 -> dez, 1 -> hex,
    // 2 -> bin, 3 -> phys
    for(i = 0; i < loc_variableList.size(); i++) {
        loc_logicalIndex = m_tableViewVariables->verticalHeader()->logicalIndex(i);
        sprintf(loc_entry, "E%d", i + 1);
        sprintf(loc_txt, "%d,%s", loc_variableList.at(loc_logicalIndex)->GetDisplayType(),
                QStringToConstChar(loc_variableList.at(loc_logicalIndex)->GetCharacteristicName()));
        ScQt_IniFileDataBaseWriteString(SectionPath, loc_entry, loc_txt, Fd);
    }
    for(; ; i++) {
        sprintf(loc_entry, "E%d", i + 1);
        if (ScQt_IniFileDataBaseReadString(SectionPath, loc_entry, "", Fd).isEmpty()) break;
        ScQt_IniFileDataBaseWriteString(SectionPath, loc_entry, nullptr, Fd);
    }

    ScQt_IniFileDataBaseWriteYesNo(SectionPath, "ShowUnitColumn", m_ShowUnitColumn, Fd);
    ScQt_IniFileDataBaseWriteYesNo(SectionPath, "ShowDisplayTypeColumn", m_ShowDisplayTypeColumn, Fd);

    ScQt_IniFileDataBaseWriteInt(SectionPath, "ColumnAlignmentName", m_Model->columnAlignment(0), Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "ColumnAlignmentValue",  m_Model->columnAlignment(1), Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "ColumnAlignmentUnit",  m_Model->columnAlignment(2), Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "ColumnAlignmentDisplayType", m_Model->columnAlignment(3), Fd);

    return true;
}

bool A2LCalSingleWidget::readFromIni()
{
    QString Line;
    QString SectionPath = GetIniSectionPath();
    char loc_valueString[INI_MAX_LINE_LENGTH];
    int loc_backgroundColor;
    int Fd = GetMainFileDescriptor();

    Line = ScQt_IniFileDataBaseReadString(SectionPath, "BgColor", "", Fd);
    if(!Line.isEmpty()) {
        loc_backgroundColor = Line.toUInt();
    }
    Line = ScQt_IniFileDataBaseReadString(SectionPath, "Font", "", Fd);
    if (!Line.isEmpty()) {
        QStringList List(Line.split(","));
        if(List.size() == 2) {
            QFont Font(List.at(0), List.at(1).toUInt());
            changeFont(Font);
        }
    }

    QString ProcessName = ScQt_IniFileDataBaseReadString(SectionPath, "process", "", Fd);
    m_Model->SetProcessName(ProcessName, this);

    m_icon = ScQt_IniFileDataBaseReadInt(SectionPath, "icon", 0, Fd);
    loc_backgroundColor = ScQt_IniFileDataBaseReadInt(SectionPath, "BgColor", 0xFFFFFF, Fd);
    m_BackgroundColor = QColor(loc_backgroundColor&0x000000FF, (loc_backgroundColor&0x0000FF00)>>8, (loc_backgroundColor&0x00FF0000)>>16); // Sh. Win Colors 0x00bbggrr
    m_Model->setBackgroundColor(m_BackgroundColor);

    char loc_text[INI_MAX_LINE_LENGTH];
    char loc_entry[INI_MAX_ENTRYNAME_LENGTH];
    for(int i = 1; ; i++) {
        sprintf(loc_entry, "E%d", i);
        QString Line = ScQt_IniFileDataBaseReadString(SectionPath, loc_entry, "", Fd);
        if (!Line.isEmpty()) {
            QStringList loc_iniTextValues(Line.split(","));
            if(loc_iniTextValues.size() == 2) {
                AddCharacteristicToModel(loc_iniTextValues.at(1), -1, loc_iniTextValues.at(0).toInt());
            }
        } else {
            break;
        }
    }

    m_Model->setColumnAlignment(0, ScQt_IniFileDataBaseReadInt(SectionPath, "ColumnAlignmentName", static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft), Fd));
    m_Model->setColumnAlignment(1, ScQt_IniFileDataBaseReadInt(SectionPath, "ColumnAlignmentValue", static_cast<int>(Qt::AlignVCenter | Qt::AlignRight), Fd));
    m_Model->setColumnAlignment(2, ScQt_IniFileDataBaseReadInt(SectionPath, "ColumnAlignmentUnit", static_cast<int>(Qt::AlignVCenter | Qt::AlignHCenter), Fd));
    m_Model->setColumnAlignment(3, ScQt_IniFileDataBaseReadInt(SectionPath, "ColumnAlignmentDisplayType", static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft), Fd));

    m_ShowUnitColumn = ScQt_IniFileDataBaseReadYesNo(SectionPath, "ShowUnitColumn", m_ShowUnitColumn, Fd);
    m_ShowDisplayTypeColumn = ScQt_IniFileDataBaseReadYesNo(SectionPath, "ShowDisplayTypeColumn", m_ShowDisplayTypeColumn, Fd);
    m_tableViewVariables->setColumnHidden(2, !m_ShowUnitColumn);
    m_tableViewVariables->setColumnHidden(3, !m_ShowDisplayTypeColumn);

    m_tableViewVariables->resizeColumnsToContents();

    return true;
}

CustomDialogFrame* A2LCalSingleWidget::dialogSettings(QWidget* arg_parent)
{
    // NOTE: required by inheritance but not used
    arg_parent->setWindowTitle("Text config");
    return nullptr;
}

int A2LCalSingleWidget::NotifiyStartStopProcess(QString par_ProcessName, int par_Flags, int par_Action)
{
    Q_UNUSED(par_Flags)
    if (Compare2ProcessNames(QStringToConstChar(par_ProcessName), QStringToConstChar(m_Model->GetProcessName())) == 0) {
        if (par_Action == 0) {
            m_Model->ProcessTerminated();
        } else if (par_Action == 1) {
            m_Model->ProcessStarted(this);
            m_Model->UpdateAllValuesReq();
        }
        update();  // repaint
    }
    return 0;
}

void A2LCalSingleWidget::NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data)
{
    m_Model->NotifyDataChanged(par_LinkNo, par_Index, par_Data);
}

int A2LCalSingleWidget::NotifiyGetDataFromLinkAck(void *par_IndexData, int par_FetchDataChannelNo)
{
    Q_UNUSED(par_FetchDataChannelNo)
    INDEX_DATA_BLOCK *IndexData = (INDEX_DATA_BLOCK*)par_IndexData;
    m_Model->UpdateValuesAck(IndexData);
    FreeIndexDataBlock(IndexData);
    return 0;

}

void A2LCalSingleWidget::dragEnterEvent(QDragEnterEvent* event)
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

void A2LCalSingleWidget::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void A2LCalSingleWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void A2LCalSingleWidget::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        DragAndDropInfos Infos (mime->text());
        if (event->source() == this) {
            QString loc_variableName = Infos.GetName();
            int loc_oldRow = m_Model->getRowIndex(loc_variableName);
            int Pos = GetDropEventYPos(event);
            int HeaderHeight = m_tableViewVariables->horizontalHeader()->height();
            Pos -= HeaderHeight;
            int loc_newRow = m_tableViewVariables->rowAt(Pos);

            m_Model->moveRow(QModelIndex(), loc_oldRow, QModelIndex(), loc_newRow);

            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
            QString loc_variableName = Infos.GetName();
            bool loc_notRegistered = true;
            foreach(A2LCalSingleData *loc_variable, m_Model->getList()) {
                if(!loc_variable->GetCharacteristicName().compare(loc_variableName)) {
                    loc_notRegistered = false;
                    break;
                }
            }
            if(loc_notRegistered) {
                int Pos = GetDropEventYPos(event);
                int HeaderHeight = m_tableViewVariables->horizontalHeader()->height();
                Pos -= HeaderHeight;
                int Row = m_tableViewVariables->rowAt(Pos);
                AddCharacteristicToModel(loc_variableName, Row);
                m_tableViewVariables->resizeColumnsToContents();
            }
        }
    } else {
        event->ignore();
    }
}

void A2LCalSingleWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(m_tableViewVariables->selectionModel()->selectedIndexes().size() > 0) {
        QModelIndex loc_index = m_tableViewVariables->selectionModel()->selectedIndexes().first();

        if(loc_index.isValid()) {
            if(loc_index.model()) {
                // do nothing
            }
        }
    }
    event->accept();
}

void A2LCalSingleWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_startDragPosition = GetEventGlobalPos(event);
    } else {
        if(event->button() == Qt::MiddleButton) {
            if(m_tableViewVariables->selectionModel()->selectedIndexes().size() > 0) {
                QModelIndex loc_index = m_tableViewVariables->selectionModel()->selectedIndexes().first();
                if(loc_index.isValid()) {
                    if(loc_index.model()) {
                        // do nothing
                    }
                }
            }
        }
    }

    event->accept();
}

void A2LCalSingleWidget::mouseMoveEvent(QMouseEvent* event)
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
        A2LCalSingleData* Var = m_Model->getVariable(loc_index.row());
        DragAndDropInfos Infos;
        QString Name(Var->GetCharacteristicName());
        Infos.SetName(Name);
        enum DragAndDropDisplayMode DisplayType;
        switch(Var->GetDisplayType()) {
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

void A2LCalSingleWidget::CharacteristicProperties()
{
    QModelIndex Index = m_tableViewVariables->currentIndex();
    Index = m_tableViewVariables->model()->index(Index.row(), 0);
    if (Index.isValid()) {
        A2LCalSingleData *Variable = m_Model->getVariable(Index.row());
        uint64_t Address = 0;
        Variable->GetAddress(&Address);
        A2LCalSinglePropertiesDlg Dlg(Variable->GetCharacteristicName(), Address, Variable->GetInfos());

        if (Dlg.exec() == QDialog::Accepted) {
            Dlg.Get(Variable->GetInfos());
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

void A2LCalSingleWidget::customContextMenu(QPoint arg_point)
{
    QMenu menu(this);
    menu.addAction (m_ConfigAct);
    menu.addAction (m_UpdateAct);
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
    NameComboBox->setCurrentText(ConvertAlignmentEnumToString(this->m_Model->columnAlignment(0)));
    QWidgetAction *NameComboBoxAction = new QWidgetAction(NameAlignmentMenu);
    NameComboBoxAction->setDefaultWidget(NameComboBox);
    NameAlignmentMenu->addAction(NameComboBoxAction);

    // Column 1 (Value)
    QMenu *ValueAlignmentMenu = AlignmentMenu->addMenu("Value");
    QComboBox *ValueComboBox = new QComboBox(ValueAlignmentMenu);
    ValueComboBox->addItem("Right");
    ValueComboBox->addItem("Left");
    ValueComboBox->addItem("Center");
    ValueComboBox->setCurrentText(ConvertAlignmentEnumToString(this->m_Model->columnAlignment(1)));
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
        UnitComboBox->setCurrentText(ConvertAlignmentEnumToString(this->m_Model->columnAlignment(2)));
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
        DispComboBox->setCurrentText(ConvertAlignmentEnumToString(this->m_Model->columnAlignment(3)));
        QWidgetAction *DispComboBoxAction = new QWidgetAction(DispAlignmentMenu);
        DispComboBoxAction->setDefaultWidget(DispComboBox);
        DispAlignmentMenu->addAction(DispComboBoxAction);
    }

    QModelIndex Index = m_tableViewVariables->currentIndex();
    if(Index.isValid()) {
        menu.addAction (m_CharacteristicPropertiesAct);
    }
    m_CurrentSelectedIndex = m_tableViewVariables->indexAt(arg_point);

    menu.exec(mapToGlobal(arg_point));
    this->m_Model->setColumnAlignment(0, ConvertStringToAlignmentEnum(NameComboBox->currentText()));
    this->m_Model->setColumnAlignment(1, ConvertStringToAlignmentEnum(ValueComboBox->currentText()));
    if (m_ShowUnitColumn && (UnitComboBox != nullptr)) {
        this->m_Model->setColumnAlignment(2, ConvertStringToAlignmentEnum(UnitComboBox->currentText()));
    }
    if (m_ShowDisplayTypeColumn && (DispComboBox != nullptr)) {
        this->m_Model->setColumnAlignment(3, ConvertStringToAlignmentEnum(DispComboBox->currentText()));
    }
}


void A2LCalSingleWidget::AddCharacteristicToModel(QString par_CharacteristicName, int arg_Row, int arg_displayType)
{
    int loc_rowCount;
    if (arg_Row < 0) {
        loc_rowCount = m_Model->rowCount();
    } else {
        loc_rowCount = arg_Row;
    }
    m_Model->insertRows(loc_rowCount, 1, QModelIndex());
    m_Model->SetCharacteristicName(loc_rowCount, par_CharacteristicName);
    m_Model->SetDisplayType(loc_rowCount, arg_displayType);
    m_Model->CheckExistanceOfOne(loc_rowCount, m_Model->GetLinkNo());
    m_Model->UpdateOneValueReq(loc_rowCount, true, true);  // this mus be called again after SetDisplayType
}


void A2LCalSingleWidget::CyclicUpdate()
{
}

void A2LCalSingleWidget::ShowUnitColumn()
{
    m_ShowUnitColumn = true;
    m_tableViewVariables->setColumnHidden(2, !m_ShowUnitColumn);
}

void A2LCalSingleWidget::ShowDisplayTypeColumn()
{
    m_ShowDisplayTypeColumn = true;
    m_tableViewVariables->setColumnHidden(3, !m_ShowDisplayTypeColumn);
}

void A2LCalSingleWidget::HideUnitColumn()
{
    m_ShowUnitColumn = false;
    m_tableViewVariables->setColumnHidden(2, !m_ShowUnitColumn);
}

void A2LCalSingleWidget::HideDisplayTypeColumn()
{
    m_ShowDisplayTypeColumn = false;
    m_tableViewVariables->setColumnHidden(3, !m_ShowDisplayTypeColumn);
}

void A2LCalSingleWidget::changeFont(QFont arg_newFont) {
    m_tableViewVariables->setFont(arg_newFont);
    m_Model->setFont(arg_newFont);  // The model need the font for calculation of the column width
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

void A2LCalSingleWidget::changeColor(QColor arg_color)
{
    m_BackgroundColor = arg_color;
    m_Model->setBackgroundColor(m_BackgroundColor);
}

void A2LCalSingleWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void A2LCalSingleWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    if(arg_visible) {
        if(!m_Model->getAllCharacteristicsNames().contains(arg_variable)) {
            AddCharacteristicToModel(arg_variable);
        }
    } else {
        int loc_row = m_Model->getRowIndex(arg_variable);
        if(loc_row >= 0) {
            m_Model->removeRows(loc_row, 1);
        }
    }
}

void A2LCalSingleWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    if(arg_visible) {
        for(int i = 0; i < arg_variables.size(); ++i) {
            if(!m_Model->getAllCharacteristicsNames().contains(arg_variables.at(i))) {
                AddCharacteristicToModel(arg_variables.at(i));
            }
        }
    } else {
        for(int i = 0; i < arg_variables.size(); ++i) {
            int loc_row = m_Model->getRowIndex(arg_variables.at(i));
            if(loc_row >= 0) {
                m_Model->removeRows(loc_row, 1);
            }
        }
    }
}

void A2LCalSingleWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables);
    m_Model->restoreBackup();
}


void A2LCalSingleWidget::ConfigSlot()
{
    ConfigDialog();
}

void A2LCalSingleWidget::UpdateSlot()
{
   m_Model->UpdateAllValuesReq(true);
}

void A2LCalSingleWidget::ConfigDialog()
{
    A2LCalSingleConfigDlg *Dlg;

    QStringList Characteristics;
    Characteristics.append(m_Model->getAllCharacteristicsNames());
    QString WindowName = this->GetWindowTitle();
    QString ProcessName = m_Model->GetProcessName();
    Dlg = new A2LCalSingleConfigDlg (WindowName, ProcessName, Characteristics);

    if (Dlg->exec() == QDialog::Accepted) {
        QString NewWindowName = Dlg->GetWindowTitle();
        if (WindowName.compare (QString (NewWindowName))) {
            RenameWindowTo (NewWindowName);
        }
        QStringList Characteristics = Dlg->GetCharacteristics();
        QString Process = Dlg->GetProcess();
        if (!ProcessName.isEmpty()) {
            // if there was a process selected before look if it stay the same
            if (Compare2ProcessNames(QStringToConstChar(Process), QStringToConstChar(ProcessName)) != 0) {
                if (m_Model->rowCount() > 0) {
                    if (ThrowError(ERROR_OK_CANCEL_CANCELALL, "You have changed the process remove all items?\n"
                                                         "(one window can only display characteristics of one process)") == IDOK) {
                        while(m_Model->rowCount()) m_Model->removeRow(0);
                        m_Model->SetProcessName(Process, this);
                    } else {
                        return;
                    }
                }
            }
        } else {
            m_Model->SetProcessName(Process, this);
        }
        if ((Process.size() > 0) && (Characteristics.size() > 0)) {
            foreach(QString Characteristic, Characteristics) {
                AddCharacteristicToModel(Characteristic, m_CurrentSelectedIndex.row());
            }
        }
    }
    delete Dlg;
}

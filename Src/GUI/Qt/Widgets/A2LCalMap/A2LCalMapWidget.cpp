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


#include <stdint.h>
#include <QMenu>
#include <QHeaderView>

#include "MdiWindowType.h"
#include "MdiWindowWidget.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

#include "A2LCalMapKeyBindigDlg.h"

#include "A2LCalMapWidget.h"
#include "A2LCalMapConfigDlg.h"
#include "A2LCalMapPropertiesDlg.h"

#include "ChangeValueTextDialog.h"
#include "A2LCalWidgetSync.h"

extern "C" {
    #include "Config.h"
    #include "PrintFormatToString.h"
    #include "EquationParser.h"
    #include "MyMemory.h"
    #include "ThrowError.h"
    #include "DebugInfoDB.h"
    #include "ReadWriteValue.h"
    #include "DebugInfoAccessExpression.h"
    #include "GetNextStructEntry.h"
    #include "UniqueNumber.h"
    #include "Scheduler.h"
    #include "A2LLinkThread.h"
}

A2LCalMapWidget::A2LCalMapWidget(QString par_WindowTitle, MdiSubWindow *par_SubWindow, MdiWindowType *par_Type, QWidget *parent) : MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent)
{
    setFocusPolicy (Qt::StrongFocus);  // because of key in
    CreateActions ();

    m_Layout = new QVBoxLayout;

    m_Splitter = new QSplitter(this);
    m_Splitter->setOrientation(Qt::Vertical);
    m_Splitter->setHandleWidth(5);
    m_Splitter->setStyleSheet(QString("QSplitter::handle{background: yellow;}"));

    m_Model = new A2LCalMapModel();
    m_3DView = new A2LCalMap3DView(m_Model->GetInternalDataStruct(), this);
    m_Table = new QTableView(this);
    m_Table->horizontalHeader()->hide();
    m_Table->verticalHeader()->hide();
    m_Table->setEditTriggers(QAbstractItemView::EditKeyPressed |  // this is the F2 key
                             QAbstractItemView::DoubleClicked |
                             QAbstractItemView::SelectedClicked);

    m_DelegateEditor = new  A2LCalMapDelegateEditor;
    m_Table->setItemDelegate(m_DelegateEditor);

    m_Table->setModel(m_Model);

    // Graphical isplay equal min. size of tabele
    m_3DView->setMinimumSize(m_Table->minimumSize());

    m_Splitter->addWidget(m_3DView);
    m_Splitter->addWidget(m_Table);

    m_Layout->addWidget(m_Splitter);

    setLayout(m_Layout);

    connect (m_Model, SIGNAL(dataChangedFromTable(int,int,int,double)), m_3DView, SLOT(update()));
    //connect (m_Model, SIGNAL(PhysRawChangedFromTable(bool)), m_3DView, SLOT(update()));
    //connect (m_Model, SIGNAL(PhysRawChangedFromTable(bool)), m_Table, SLOT(update()));

    connect (m_3DView, SIGNAL(ChangeValueByWheel(int)), this, SLOT(ChangeValueByWheelSlot(int)));
    connect (m_3DView, SIGNAL(SelectionChanged()), this, SLOT(SelectionChangedBy3DView()));

    QItemSelectionModel *SectionModel = m_Table->selectionModel();
    connect (SectionModel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(SelectionChangedByTable(const QItemSelection&, const QItemSelection&)));
    connect (SectionModel, SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(CurrentChangedByTable(const QModelIndex&, const QModelIndex&)));

    readFromIni();

    UpdateProcessAndCharacteristics();

    QList<int> SizeList;
    SizeList.append(m_SplitterGraphicView);
    SizeList.append(m_SplitterTableView);
    m_Splitter->setSizes(SizeList);

    m_UpdateDataRequest = false;

    // If SyncObject doesn't exist than create it inside the GUI-thread
    SetupA2LCalWidgetSyncObject(this);

    AddA2LCalMapWidgetToSyncObject(this);

    GetData ();
}

A2LCalMapWidget::~A2LCalMapWidget()
{
    writeToIni();
    RemoveA2LCalMapWidgetFromSyncObject(this);
    delete m_Model;
}

int A2LCalMapWidget::NotifiyStartStopProcess (QString par_ProcessName, int par_Flags, int par_Action)
{
    Q_UNUSED(par_Flags)
    if (Compare2ProcessNames(QStringToConstChar(par_ProcessName), QStringToConstChar(m_Process)) == 0) {
        if (par_Action == 0) {
            m_Model->ProcessTerminated();
        } else if (par_Action == 1) {
            UpdateProcessAndCharacteristics();
        }
        update();  // repaint it
    }
    return 0;
}

void A2LCalMapWidget::NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data)
{
    m_Model->NotifyDataChanged(par_LinkNo, par_Index, par_Data);
}

int A2LCalMapWidget::NotifiyGetDataFromLinkAck(void *par_IndexData, int par_FetchDataChannelNo)
{
    Q_UNUSED(par_FetchDataChannelNo)
    INDEX_DATA_BLOCK *IndexData = (INDEX_DATA_BLOCK*)par_IndexData;
    m_Model->UpdateAck(IndexData->Data->LinkNo,
                       IndexData->Data->Index,
                       IndexData->Data->Data);
    return 0;
}

CustomDialogFrame* A2LCalMapWidget::dialogSettings(QWidget* arg_parent)
{
    Q_UNUSED(arg_parent);
    // NOTE: required by inheritance but not used
    return nullptr;
}


void A2LCalMapWidget::CyclicUpdate()
{
    if (m_UpdateDataRequest) {
        m_UpdateDataRequest = false;
        GetData();
    }
    m_3DView->update();
}

int A2LCalMapWidget::GetData (void)
{
    m_Model->Update();
    return 0;
}

void A2LCalMapWidget::SelectAll (void)
{
    m_Model->ChangeSelectionOfAll(true);
    SelectionChangedBy3DView();
}

void A2LCalMapWidget::UnselectAll (void)
{
    m_Model->ChangeSelectionOfAll(false);
    SelectionChangedBy3DView();
}


void A2LCalMapWidget::AdjustMinMax (void)
{
    m_Model->AdjustMinMax();
}

void A2LCalMapWidget::ConfigDialog (void)
{
    A2LCalMapConfigDlg *Dlg;

    QStringList Characteristics;
    Characteristics.append(m_Model->GetCharacteristicName());
    QString WindowName = this->GetWindowTitle();
    QString ProcessName = m_Model->GetProcess();
    Dlg = new A2LCalMapConfigDlg (WindowName, ProcessName, Characteristics);

    if (Dlg->exec() == QDialog::Accepted) {
        m_Model->Reset();
        QString NewWindowName = Dlg->GetWindowTitle();
        if (m_WindowName.compare (QString (NewWindowName))) {
            RenameWindowTo (NewWindowName);
        }
        QStringList Characteristics = Dlg->GetCharacteristics();
        QString Process = Dlg->GetProcess();
        if ((Process.size() > 0) && (Characteristics.size() > 0)) {
            m_Process = Process;
            m_Model->SetProcess(Process, this);
            m_Characteristic = Characteristics.at(0);
            m_Model->SetCharacteristicName(m_Characteristic);
            GetData ();

            // Set default View settings
            if (m_Model->GetType() == 2) m_3DView->init_trans3d ();
            else m_3DView->init_trans2d ();
        }
    }
    delete Dlg;
}

void A2LCalMapWidget::PropertiesDialog()
{
    A2LCalMapPropertiesDlg *Dlg;

    QString Characteristic = m_Characteristic;
    uint64_t Address = 0;
    m_Model->GetAddress(&Address);
    Dlg = new A2LCalMapPropertiesDlg(Characteristic,
                                     m_Model->GetInternalDataStruct()->GetType(),
                                     Address,
                                     m_Model->GetInternalDataStruct()->GetAxisInfos(0),
                                     m_Model->GetInternalDataStruct()->GetAxisInfos(1),
                                     m_Model->GetInternalDataStruct()->GetAxisInfos(2));
    if (Dlg->exec() == QDialog::Accepted) {
        Dlg->Get(m_Model->GetInternalDataStruct()->GetAxisInfos(0),
                 m_Model->GetInternalDataStruct()->GetAxisInfos(1),
                 m_Model->GetInternalDataStruct()->GetAxisInfos(2));
    }
    delete Dlg;
}

void A2LCalMapWidget::ViewKeyBinding ()
{
    A2LCalMapKeyBindigDlg Dlg;
    Dlg.exec();
}

void A2LCalMapWidget::ConfigSlot ()
{
    ConfigDialog ();
}

void A2LCalMapWidget::PropertiesSlot()
{
    PropertiesDialog();
}

void A2LCalMapWidget::SwitchToRawSlot()
{
    m_Model->SetPhysical(false);
}

void A2LCalMapWidget::SwitchToPhysSlot()
{
    m_Model->SetPhysical(true);
}

void A2LCalMapWidget::KeyBindingsSlot ()
{
    ViewKeyBinding ();
}

void A2LCalMapWidget::ChangeValueByWheelSlot(int delta)
{
    double Scale = m_3DView->get_scale();
    double MaxMinusMin = m_Model->GetMaxMinusMin(2);  // 2 == Map
    double Add = Scale * MaxMinusMin * (0.01/120.0)*static_cast<double>(delta);
    m_Model->ChangeSelectedMapValues(Add, 1);  // + (Add) / (Max - Min)
    m_3DView->update();
}

void A2LCalMapWidget::SelectOrDeselectMapElements (QModelIndexList &IndexList, int SelectOrDeselect)
{

    int XDim =  m_Model->GetInternalDataStruct()->GetXDim();
    int YDim =  m_Model->GetInternalDataStruct()->GetYDim();
    foreach (QModelIndex Index, IndexList) {
        int Row = Index.row();
        int Column = Index.column();
        if ((Row > 0) && (Column > 0)) {
            // Only the map is selecist selectable
            int x = Column - 1;
            int y = Row - 1;
            if ((x < XDim) && (y < YDim)) {
                m_Model->GetInternalDataStruct()->SetPick(x, y, (SelectOrDeselect != 0) ? 1 : 2);
            }
        }
    }
}


void A2LCalMapWidget::SelectionChangedBy3DView()
{
    int XDim =  m_Model->GetInternalDataStruct()->GetXDim();
    int YDim =  m_Model->GetInternalDataStruct()->GetYDim();
    QItemSelectionModel *SectionModel = m_Table->selectionModel();
    SectionModel->clear();
    m_Model->SetNoRecursiveSetPickMapFlag(true);  // avoid that the selected elements are removed
    for (int y = 0; y < YDim; y++) {
        for (int x = 0; x < XDim; x++) {
            if (m_Model->GetInternalDataStruct()->IsPicked(x, y)) {
                QModelIndex Index = m_Model->index(y+1, x+1);
                SectionModel->select(Index, QItemSelectionModel::Select);
            }

        }
    }
    m_Model->SetNoRecursiveSetPickMapFlag(false);
}

void A2LCalMapWidget::SelectionChangedByTable(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (!m_Model->GetNoRecursiveSetPickMapFlag()) {
        QModelIndexList DeselList = deselected.indexes();
        SelectOrDeselectMapElements (DeselList, 0);
        QModelIndexList SelList = selected.indexes();
        SelectOrDeselectMapElements (SelList, 1);
        m_3DView->update();
    }
}

void A2LCalMapWidget::CurrentChangedByTable(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void A2LCalMapWidget::changeColor(QColor arg_color)
{
    Q_UNUSED(arg_color)
    // NOTE: required by inheritance but not used
}

void A2LCalMapWidget::changeFont(QFont arg_font)
{
    Q_UNUSED(arg_font)
    // NOTE: required by inheritance but not used
}

void A2LCalMapWidget::changeWindowName(QString arg_name)
{
    Q_UNUSED(arg_name)
    // NOTE: required by inheritance but not used
}

void A2LCalMapWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    Q_UNUSED(arg_variable)
    Q_UNUSED(arg_visible)
    // NOTE: required by inheritance but not used
}

void A2LCalMapWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_variables)
    Q_UNUSED(arg_visible)
    // NOTE: required by inheritance but not used
}

void A2LCalMapWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables)
    // NOTE: required by inheritance but not used
}

void A2LCalMapWidget::UpdateProcessAndCharacteristics()
{
    QStringList Characteristics;
    Characteristics.append(m_Characteristic);
    if ((m_Process.size() > 0) && (Characteristics.size() > 0)) {
        m_Model->SetProcess(m_Process, this);
        QString Characteristic = Characteristics.at(0);
        m_Model->SetCharacteristicName(Characteristic);

        GetData ();

        // set default view attitudes
        if (m_Model->GetType() == 2) m_3DView->init_trans3d ();
        else m_3DView->init_trans2d ();
    }

}

void A2LCalMapWidget::CreateActions(void)
{
    m_ConfigAct = new QAction(tr("&config"), this);
    connect(m_ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigSlot()));
    m_PropertiesAct = new QAction(tr("&properties"), this);
    connect(m_PropertiesAct, SIGNAL(triggered()), this, SLOT(PropertiesSlot()));
    m_SwitchToRawAct = new QAction(tr("switch to &raw"), this);
    connect(m_SwitchToRawAct, SIGNAL(triggered()), this, SLOT(SwitchToRawSlot()));
    m_SwitchToPhysAct = new QAction(tr("switch to ph&ysical"), this);
    connect(m_SwitchToPhysAct, SIGNAL(triggered()), this, SLOT(SwitchToPhysSlot()));
    m_KeyBindingsAct = new QAction(tr("v&iew key bindings"), this);
    m_KeyBindingsAct->setStatusTip(tr("view al help text about the keys can be used inside this window"));
    connect(m_KeyBindingsAct, SIGNAL(triggered()), this, SLOT(KeyBindingsSlot()));
}

void A2LCalMapWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    menu.addAction (m_ConfigAct);
    menu.addAction (m_PropertiesAct);
    if (m_Model->IsPhysical()) {
        menu.addAction (m_SwitchToRawAct);
    } else {
        menu.addAction (m_SwitchToPhysAct);
    }
    menu.addAction (m_KeyBindingsAct);
    menu.exec(event->globalPos());
}

void A2LCalMapWidget::keyPressEvent(QKeyEvent * event)
{
    int Key = event->key();
    switch (Key) {
    case '#':
        AdjustMinMax ();
        m_Model->CalcRawMinMaxValues();
        update ();
        break;
    case 'c':
    case 'C':
        ConfigDialog ();
        update ();
        break;
    case 'h':
    case 'H':
    case '?':
    case 'k':
    case 'K':
        ViewKeyBinding();
        break;
    case Qt::Key_F5:
    case 'g':
    case 'G': // Daten neu lesen
        GetData ();
        update ();
        break;
    case 'd':
    case 'D':                    // Reset the representation to the presetting
        if (m_Model->GetType() == 2) m_3DView->init_trans3d ();
        else m_3DView->init_trans2d ();
        update ();
        break;
    case 'r':
    case 'R':
        m_3DView->add_x_offset (-0.1);
        update ();
        break;
    case 'l':
    case 'L':
        m_3DView->add_x_offset (0.1);
        update ();
        break;
    case 't':
    case 'T':
        m_3DView->add_z_offset (0.1);
        update ();
        break;
    case 'b':
    case 'B':
        m_3DView->add_z_offset (-0.1);
        update ();
        break;
// Scaling
    case 'q':
    case 'Q':
        m_3DView->scale_factor(1.0/1.1);
        update ();
        break;
    case 'a':
    case 'A':
        m_3DView->scale_factor(1.1);
        update ();
        break;

// Rotation
    case 'x':
    case 'X':
        if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) {
            if (m_Model->GetType() == 2) {
                m_3DView->rotate_x_angle(-M_PI/40);
                m_3DView->update_sin_cos ();
            }
        } else {
            if (m_Model->GetType() == 2) {
                m_3DView->rotate_x_angle(M_PI/40);
                m_3DView->update_sin_cos ();
            }
        }
        update ();
        break;
    case 'y':
    case 'Y':
        if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) {
            if (m_Model->GetType() == 2) {
                m_3DView->rotate_y_angle(-M_PI/40);
                m_3DView->update_sin_cos ();
            }
        } else {
            if (m_Model->GetType() == 2) {
                m_3DView->rotate_y_angle(M_PI/40);
                m_3DView->update_sin_cos ();
            }
        }
        update ();
        break;
    case 'z':
    case 'Z':
        if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) {
            if (m_Model->GetType() == 2) {
                m_3DView->rotate_z_angle(-M_PI/40);
                m_3DView->update_sin_cos ();
            }
        } else {
            if (m_Model->GetType() == 2) {
                m_3DView->rotate_z_angle(M_PI/40);
                m_3DView->update_sin_cos ();
            }
        }
        update ();
        break;
// Change the value(s=
    case '+':
    case '-':
    case '*':
    case '/':
    case 'i':
    case 'I':
    case 'o':
    case 'O':
    case 'p':
    case 'P':
    case 'm':
    case 'M':
        m_Model->ChangeSelectedValuesWithDialog (this, Key);
        update ();
        break;
    case 's':
    case 'S':
        SelectAll ();
        update ();
        break;
    case 'u':
    case 'U':
        UnselectAll ();
        update ();
        break;
    }
}

bool A2LCalMapWidget::writeToIni(void)
{
    QString WindowTitle = GetIniSectionPath();
    QString Entry;
    QString Line;
    char Help[64];
    int Fd = ScQt_GetMainFileDescriptor();

    Entry = QString("type");
    Line = GetMdiWindowType()->GetWindowTypeName();
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("process");
    Line = m_Model->GetProcess();
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("characteristic");
    Line = m_Model->GetCharacteristicName();
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("Pysical");
    ScQt_IniFileDataBaseWriteYesNo(WindowTitle, Entry, m_Model->IsPhysical(), Fd);
    Entry = QString("DecIncOffset");
    ScQt_IniFileDataBaseWriteFloat(WindowTitle, Entry, m_Model->GetInternalDataStruct()->GetDecIncOffset(), Fd);

    // Angle and zoom factor
    Entry = QString("trans3d.ax");
    PrintFormatToString (Help, sizeof(Help), "%g", m_3DView->get_x_angle());
    Line = QString(Help);
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("trans3d.ay");
    PrintFormatToString (Help, sizeof(Help), "%g", m_3DView->get_y_angle());
    Line = QString(Help);
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("trans3d.az");
    PrintFormatToString (Help, sizeof(Help), "%g", m_3DView->get_z_angle());
    Line = QString(Help);
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("trans3d.k_depth");
    PrintFormatToString (Help, sizeof(Help), "%g", m_3DView->get_depth());
    Line = QString(Help);
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("trans3d.scale");
    PrintFormatToString (Help, sizeof(Help), "%g", m_3DView->get_scale());
    Line = QString(Help);
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("trans3d.x_off");
    PrintFormatToString (Help, sizeof(Help), "%g",  m_3DView->get_x_offset());
    Line = QString(Help);
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("trans3d.y_off");
    PrintFormatToString (Help, sizeof(Help), "%g",  m_3DView->get_y_offset());
    Line = QString(Help);
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);
    Entry = QString("trans3d.z_off");
    PrintFormatToString (Help, sizeof(Help), "%g",  m_3DView->get_z_offset());
    Line = QString(Help);
    ScQt_IniFileDataBaseWriteString(WindowTitle, Entry, Line, Fd);

    QList<int>ListSizes = m_Splitter->sizes();
    m_SplitterGraphicView = ListSizes.at(0);
    m_SplitterTableView = ListSizes.at(1);
    Entry = QString("SplitterGraphicView");
    ScQt_IniFileDataBaseWriteFloat(WindowTitle, Entry, m_SplitterGraphicView, Fd);
    Entry = QString("SplitterTableView");
    ScQt_IniFileDataBaseWriteFloat(WindowTitle, Entry, m_SplitterTableView, Fd);
    return true;
}

bool A2LCalMapWidget::readFromIni()
{
    QString Entry;
    QString WindowTitle = GetIniSectionPath();
    int Fd = ScQt_GetMainFileDescriptor();

    Entry = QString("process");
    m_Process = ScQt_IniFileDataBaseReadString(WindowTitle, Entry, "", Fd);
    Entry = QString("characteristic");
    m_Characteristic = ScQt_IniFileDataBaseReadString(WindowTitle, Entry, "", Fd);
    Entry = QString("Pysical");
    int PhysicalFlag =  ScQt_IniFileDataBaseReadYesNo(WindowTitle, Entry, 1, Fd);
    m_Model->SetPhysical(PhysicalFlag);
    Entry = QString("DecIncOffset");
    double DecIncOffset = ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, 1.0, Fd);
    m_Model->GetInternalDataStruct()->SetDecIncOffset(DecIncOffset);

    // Angle and zoom factor
    Entry = QString("trans3d.ax");
    m_3DView->set_x_angle(ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, 0.0, Fd));
    Entry = QString("trans3d.ay");
    m_3DView->set_y_angle(ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, 0.0, Fd));
    Entry = QString("trans3d.az");
    m_3DView->set_z_angle(ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, -3.141592, Fd));
    m_3DView->update_sin_cos ();
    Entry = QString("trans3d.k_depth");
    m_3DView->set_depth(ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, 0.5, Fd));
    Entry = QString("trans3d.scale");
    m_3DView->set_scale(ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, 1.0, Fd));
    Entry = QString("trans3d.x_off");
    m_3DView->set_x_offset(ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, 0.4, Fd));
    Entry = QString("trans3d.y_off");
    m_3DView->set_y_offset(ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, 0.0, Fd));
    Entry = QString("trans3d.z_off");
    m_3DView->set_z_offset(ScQt_IniFileDataBaseReadFloat(WindowTitle, Entry, -0.3, Fd));


    Entry = QString("SplitterGraphicView");
    m_SplitterGraphicView = ScQt_IniFileDataBaseReadInt(WindowTitle, Entry, 3 * height() / 4, Fd);
    Entry = QString("SplitterTableView");
    m_SplitterTableView = ScQt_IniFileDataBaseReadInt(WindowTitle, Entry, height() / 4, Fd);
    return true;
}



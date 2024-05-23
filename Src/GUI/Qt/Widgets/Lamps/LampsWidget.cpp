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


#include "LampsWidget.h"
#include <QtGlobal>
#include <QDebug>
#include <QMdiSubWindow>
#include <QMenu>
#include <QAction>
#include <QSignalMapper>
#include <QDebug>
#include "MdiWindowType.h"
#include "LampsModel.h"
#include "BlackboardInfoDialog.h"
#include "ImExportControlLampsDialog.h"
#include "QtIniFile.h"
#include "GetEventPos.h"
#include "StringHelpers.h"

extern "C"{
#include "TextReplace.h"
#include "BlackboardAccess.h"
#include "MainValues.h"
}

LampsWidget::LampsWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent) : MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent)
{
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(openContextMenu(QPoint)));
    m_layout = nullptr; //new QGridLayout(this);
    m_selectedLamp = nullptr;
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setMinimumSize(30, 30);

    ResetLayout();
    readFromIni();
}

LampsWidget::~LampsWidget()
{
    writeToIni();
}

void LampsWidget::openContextMenu(QPoint p)
{
    CustomLampsGraphicsView *loc_view = qobject_cast<CustomLampsGraphicsView*>(sender());
    if(loc_view) {
        setSelectedLamp(loc_view);
    } else {
        setSelectedLamp(nullptr);
    }
    QList<QAction*> loc_openDialogs;
    QMenu loc_contextMenu(tr("ConfigMenu"), this);
    QAction loc_newLamp(tr("Add new lamp"), this);
    loc_contextMenu.addAction(&loc_newLamp);
    connect(&loc_newLamp, SIGNAL(triggered()), this, SLOT(addNewLamp()));
    QMenu *loc_contextSubMenu = loc_contextMenu.addMenu(QString("Config lamp"));
    QSignalMapper* loc_signalMapperConfig = new QSignalMapper(this);
    QSignalMapper* loc_signalMapperHoveredConfig = new QSignalMapper(this);

    int i = 0;
    foreach (CustomLampsGraphicsView* loc_lamp, m_allLamps) {
        QString Text;
        Text.append(QString("config lamp \"%1\" (").arg(loc_lamp->variableName()));
        Text.append(QString("%1, ").arg(loc_lamp->getFromColumn()));
        Text.append(QString("%1, ").arg(loc_lamp->getFromRow()));
        Text.append(QString("%1, ").arg(loc_lamp->getColumnSpan()));
        Text.append(QString("%1)").arg(loc_lamp->getRowSpan()));
        QAction *loc_action = new QAction (Text, this);
        loc_openDialogs.append(loc_action);
        loc_signalMapperConfig->setMapping(loc_action, i);
        loc_signalMapperHoveredConfig->setMapping(loc_action, i);
        connect(loc_action, SIGNAL(triggered()), loc_signalMapperConfig, SLOT(map()));
        connect(loc_action, SIGNAL(hovered()), loc_signalMapperHoveredConfig, SLOT(map()));
        loc_contextSubMenu->addAction(loc_action);
        i++;
    }
    connect (loc_signalMapperConfig, SIGNAL(mappedInt(int)), this, SLOT(OwnOpenDialog(int))) ;
    connect (loc_signalMapperHoveredConfig, SIGNAL(mappedInt(int)), this, SLOT(hoveredLamp(int))) ;
    loc_contextSubMenu = loc_contextMenu.addMenu(QString("Delete lamp"));
    QSignalMapper* loc_signalMapperDelete = new QSignalMapper(this);
    QSignalMapper* loc_signalMapperHoveredDelete = new QSignalMapper(this);
    i = 0;
    foreach (CustomLampsGraphicsView* loc_lamp, m_allLamps) {
        QString Text;
        Text.append(QString("delete lamp \"%1\" (").arg(loc_lamp->variableName()));
        Text.append(QString("%1, ").arg(loc_lamp->getFromColumn()));
        Text.append(QString("%1, ").arg(loc_lamp->getFromRow()));
        Text.append(QString("%1, ").arg(loc_lamp->getColumnSpan()));
        Text.append(QString("%1)").arg(loc_lamp->getRowSpan()));
        QAction *loc_action = new QAction (Text, this);
        loc_openDialogs.append(loc_action);
        loc_signalMapperDelete->setMapping(loc_action, i);
        loc_signalMapperHoveredDelete->setMapping(loc_action, i);
        connect(loc_action, SIGNAL(triggered()), loc_signalMapperDelete, SLOT(map()));
        connect(loc_action, SIGNAL(hovered()), loc_signalMapperHoveredDelete, SLOT(map()));
        loc_contextSubMenu->addAction(loc_action);
        i++;
    }
    connect (loc_signalMapperDelete, SIGNAL(mappedInt(int)), this, SLOT(deleteLamp(int))) ;
    connect (loc_signalMapperHoveredDelete, SIGNAL(mappedInt(int)), this, SLOT(hoveredLamp(int))) ;

    QAction *loc_action = new QAction ("im/export user defined lamps", this);
    connect(loc_action, SIGNAL(triggered()), this, SLOT(ImExportAllLamps()));
    loc_contextMenu.addAction(loc_action);

    loc_contextMenu.exec(mapToGlobal(p));
    backgroundColorChange(m_sceneBackgroundColor);  // Farbe zuruecksetzen
    delete loc_signalMapperConfig;
    delete loc_signalMapperHoveredConfig;
    delete loc_signalMapperDelete;
    delete loc_signalMapperHoveredDelete;
}

void LampsWidget::OwnOpenDialog(int arg_LampNumber)
{
    backgroundColorChange(m_sceneBackgroundColor);  // Farbe zurucksetzen
    if ((arg_LampNumber >= 0) && (arg_LampNumber < m_allLamps.count())) {
        QStringList VariableName;
        m_selectedLamp = m_allLamps.at(arg_LampNumber);
        if (m_selectedLamp != nullptr) {
            VariableName.append(m_selectedLamp->variableName());
        }
        emit openStandardDialog(VariableName, false, false);
    }
}

void LampsWidget::openDialog()
{
    int LampNumber = m_allLamps.indexOf(m_selectedLamp);
    if (LampNumber >= 0) {
        OwnOpenDialog(LampNumber);
    } else {
       // tue nix, vielleicht neue Lampe anlegen?
    }
}

void LampsWidget::deleteLamp(int arg_LampNumber)
{
    backgroundColorChange(m_sceneBackgroundColor);  // Farbe zurucksetzen
    if ((arg_LampNumber >= 0) && (arg_LampNumber < m_allLamps.count())) {
        CustomLampsGraphicsView *loc_view = m_allLamps.at(arg_LampNumber);
        m_layout->removeWidget(loc_view);
        m_allLamps.removeAt(arg_LampNumber);
        if (loc_view == m_selectedLamp) {
            m_selectedLamp = nullptr;
        }
        delete loc_view;
    }
}

static QColor LighterOrDarkerColor(QColor &arg_color)
{
    QColor loc_color = arg_color;
    loc_color = arg_color.toHsl();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qreal loc_hue, loc_saturation, loc_lightness, loc_alpha;
#else
    float loc_hue, loc_saturation, loc_lightness, loc_alpha;
#endif
    loc_color.getHslF (&loc_hue, &loc_saturation, &loc_lightness, &loc_alpha);
    if (loc_lightness > 0.5) {
        loc_lightness -= 0.25;
    } else {
        loc_lightness += 0.25;
    }
    loc_color.setHslF(loc_hue, loc_saturation, loc_lightness, loc_alpha);
    QColor loc_ret = loc_color.toRgb();
    return loc_ret;
}

void LampsWidget::hoveredLamp(int arg_LampNumber)
{
    backgroundColorChange(m_sceneBackgroundColor);  // Farbe zurcksetzen
    CustomLampsGraphicsView *loc_view = m_allLamps.at(arg_LampNumber);
    loc_view->updateBackgroundColor(LighterOrDarkerColor(m_sceneBackgroundColor));
}

void LampsWidget::addNewLamp()
{
    CustomLampsGraphicsView *loc_view = new CustomLampsGraphicsView(this);

    int MaxX = 0;
    int MaxY = 0;
    foreach(CustomLampsGraphicsView *Elem, m_allLamps) {
        if ((Elem->getFromColumn() + Elem->getColumnSpan()) > MaxX) {
            MaxX = Elem->getFromColumn() + Elem->getColumnSpan();
        }
        if ((Elem->getFromRow() + Elem->getRowSpan()) > MaxY) {
            MaxY = Elem->getFromRow() + Elem->getRowSpan();
        }
    }
    if (MaxX < MaxY) {
        loc_view->setPositionAndSize(0, MaxX, 1, 1);
    } else {
        loc_view->setPositionAndSize(MaxY, 0, 1, 1);
    }

    loc_view->setShouldBeRemovedByCancel(); // CustomLampsGraphicsView should be removed inside openStandardDialog if "cancel" is pressed
    m_allLamps.append(loc_view);
    m_layout->addWidget(loc_view, loc_view->getFromColumn(), loc_view->getFromRow(), loc_view->getColumnSpan(), loc_view->getRowSpan());
    m_selectedLamp = loc_view;
    connectNewViewToSlots(loc_view);
    emit openStandardDialog(QStringList(), false, false);
}

void LampsWidget::deleteExistingLamp()
{
}

void LampsWidget::setWindowName(QString name)
{
    parentWidget()->setWindowTitle(name);
}

int LampsWidget::getControlLampsCount()
{
    return m_layout->count();
}

void LampsWidget::setSelectedLamp(CustomLampsGraphicsView *lamp)
{
    m_selectedLamp = lamp;
}

void LampsWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    //openDialog();
}

bool LampsWidget::writeToIni()
{
    int loc_BkColor = 0;
    int loc_lampCount = 0;
    QList<CustomLampsGraphicsView*> loc_allLamps = this->findChildren<CustomLampsGraphicsView*>("");
    if(!loc_allLamps.isEmpty()) {
        QString loc_section = GetIniSectionPath();
        QString loc_entry;
        QString loc_text;
        int Fd = ScQt_GetMainFileDescriptor();

        loc_entry = QString("type");
        QString loc_window_type = GetMdiWindowType()->GetWindowTypeName();
        ScQt_IniFileDataBaseWriteString(loc_section, loc_entry, loc_window_type, Fd);
        loc_entry = QString("Columns");
        ScQt_IniFileDataBaseWriteInt(loc_section, loc_entry, m_layout->columnCount(), Fd);
        loc_entry = QString("Rows");
        ScQt_IniFileDataBaseWriteInt(loc_section, loc_entry, m_layout->rowCount(), Fd);
        QColor loc_BkQColor = loc_allLamps.first()->backgroundColor();
        loc_BkColor += loc_BkQColor.red();
        loc_BkColor += loc_BkQColor.green() << 8;
        loc_BkColor += loc_BkQColor.blue() << 16;
        char Help[32];
        sprintf(Help, "0x%08X", loc_BkColor);
        loc_text = QString(Help);
        loc_entry = QString("BkColor");
        ScQt_IniFileDataBaseWriteString(loc_section, loc_entry, loc_text, Fd);
        foreach (CustomLampsGraphicsView* loc_lamp, loc_allLamps) {
            loc_entry = QString("Lamp_%1").arg(loc_lampCount);
            int index = m_layout->indexOf(loc_lamp);
            int loc_row, loc_column, loc_rowSpan, loc_columnSpan, loc_conversionFlag;
            loc_conversionFlag = 0;
            m_layout->getItemPosition(index, &loc_row, &loc_column, &loc_rowSpan, &loc_columnSpan);
            loc_text = loc_lamp->variableName();
            loc_text.append(", ");
            loc_text.append(QString().number(loc_row));
            loc_text.append(", ");
            loc_text.append(QString().number(loc_column));
            loc_text.append(", ");
            loc_text.append(QString().number(loc_rowSpan));
            loc_text.append(", ");
            loc_text.append(QString().number(loc_columnSpan));
            loc_text.append(", ");
            loc_text.append(loc_lamp->stancilName());
            loc_text.append(", ");
            loc_text.append(QString().number(loc_conversionFlag));
            ScQt_IniFileDataBaseWriteString(loc_section, loc_entry, loc_text, Fd);
            loc_lampCount++;
        }
    }
    return true;
}

bool LampsWidget::readFromIni()
{
    QString loc_section = GetIniSectionPath();
    QString loc_entry;
    QString loc_text;
    int loc_BkColor;
    int loc_lampIndex = 0;
    int Fd = ScQt_GetMainFileDescriptor();

    loc_entry = QString("BkColor");
    loc_BkColor = ScQt_IniFileDataBaseReadInt(loc_section, loc_entry, 0, Fd);
    m_sceneBackgroundColor = QColor(loc_BkColor&0x000000FF, (loc_BkColor&0x0000FF00)>>8, (loc_BkColor&0x00FF0000)>>16); // Sh. Win Colors 0x00bbggrr
    loc_entry = QString("Columns");
    ControlLampModel loc_stancilModel;
    customStancilFromIni(&loc_stancilModel);

    while(1) {
        loc_entry = QString("Lamp_%1").arg(loc_lampIndex);
        loc_text = ScQt_IniFileDataBaseReadString(loc_section, loc_entry, "", Fd);
        if (loc_text.isEmpty()) break;
        QString loc_stancilConfig(loc_text);
        QStringList loc_stancilConfigList(loc_stancilConfig.split(","));
        if(loc_stancilConfigList.count() == 7) {
            CustomLampsGraphicsView *view = new CustomLampsGraphicsView();
            view->setVarialeName(loc_stancilConfigList.at(0));
            QVariant loc_vid(add_bbvari(QStringToConstChar(loc_stancilConfigList.at(0)), BB_UNKNOWN_WAIT, ""));
            view->setProperty("VID", loc_vid);
            view->setVarialeName(loc_stancilConfigList.at(0));
            view->setStancilName(loc_stancilConfigList.at(5).simplified());
            QList<QPolygonF>* loc_foreground = loc_stancilModel.getForegroundStancil(loc_stancilConfigList.at(5).simplified());
            QList<QPolygonF>* loc_background = loc_stancilModel.getBackgroundStancil(loc_stancilConfigList.at(5).simplified());
            view->setGraphicsItemGroup(loc_foreground, loc_background);
            view->updateBackgroundColor(m_sceneBackgroundColor);
            int loc_fromRow = loc_stancilConfigList.at(1).toInt();
            int loc_fromColumn = loc_stancilConfigList.at(2).toInt();
            int loc_rowSpan = loc_stancilConfigList.at(3).toInt();
            int loc_columnSpant = loc_stancilConfigList.at(4).toInt();
            view->setPositionAndSize(loc_fromRow, loc_fromColumn, loc_rowSpan, loc_columnSpant);
            //m_layout->addWidget(view, loc_XPos, loc_YPos, loc_Width, loc_Height);
            m_allLamps.append(view);
            //view->setVarialeName(loc_stancilConfigList.at(0));
            connectNewViewToSlots(view);
        } else {
            //TODO: to much or less arguments inside the INI file
        }
        loc_lampIndex++;
    }
    ResetLayout();

    return true;
}

CustomDialogFrame* LampsWidget::dialogSettings(QWidget* arg_parent)
{
    arg_parent->setWindowTitle("Config lamp");
    if (m_selectedLamp == nullptr) {
        return nullptr;
    }
    CustomDialogFrame* loc_dialogFrame = new CustomLampsDialogFrame(m_selectedLamp, arg_parent);
    connect(loc_dialogFrame, SIGNAL(cancelNewLamp(CustomLampsGraphicsView*)), this, SLOT(cancelNewLamp(CustomLampsGraphicsView*)));
    return loc_dialogFrame;
}

void LampsWidget::ResetLayout()
{
    if (m_layout != nullptr) {
        delete m_layout;
    }
    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    setLayout(m_layout);
    foreach (CustomLampsGraphicsView *loc_view, m_allLamps) {
        m_layout->addWidget(loc_view, loc_view->getFromRow(), loc_view->getFromColumn(), loc_view->getRowSpan(), loc_view->getColumnSpan());
    }
    for(int i = 0; i < m_layout->rowCount(); ++i) {
        m_layout->setRowStretch(i, 1);
        for(int j = 0; j < m_layout->columnCount(); ++j) {
            m_layout->setColumnStretch(j, 1);
        }
    }
}

void LampsWidget::cancelNewLamp(CustomLampsGraphicsView *arg_newLamp)
{
    if (m_selectedLamp == arg_newLamp) {
        int loc_index = m_allLamps.indexOf(arg_newLamp);
        if (loc_index >= 0) {
            deleteLamp(loc_index);
        }
    }
}

void LampsWidget::ImExportAllLamps()
{
    ImExportControlLampsDialog *Dlg = new ImExportControlLampsDialog(this);

    if (Dlg->exec() == QDialog::Accepted) {
        ;
    }
    delete Dlg;
}

void LampsWidget::CyclicUpdate()
{
    QList<CustomLampsGraphicsView*> loc_allLamps = this->findChildren<CustomLampsGraphicsView*>("");
    foreach (CustomLampsGraphicsView* loc_lamp, loc_allLamps) {
        char EnumString[BBVARI_ENUM_NAME_SIZE];
        int loc_blackboardColor;
        QVariant loc_Variant = loc_lamp->property("VID");
        int Vid = loc_Variant.toInt();
        if ((get_bbvaritype(Vid) != BB_UNKNOWN_WAIT) &&
            (get_bbvari_conversiontype(Vid) == BB_CONV_TEXTREP)) {
            QColor loc_color;
            if (read_bbvari_textreplace (Vid, EnumString, sizeof(EnumString), &loc_blackboardColor) == 0) {
                loc_color = QColor(loc_blackboardColor&0x000000FF, (loc_blackboardColor&0x0000FF00)>>8, (loc_blackboardColor&0x00FF0000)>>16); // Sh. Win Colors 0x00bbggrr
            } else {
                loc_color = m_sceneBackgroundColor;  // im Fehlerfall Hintergrundfarbe
            }
            loc_lamp->updateForegroundColor(loc_color);
        }
    }
}

void LampsWidget::connectNewViewToSlots(CustomLampsGraphicsView *arg_newView)
{
    connect(arg_newView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(openContextMenu(QPoint)));
    connect(arg_newView, SIGNAL(openDialog()), this, SLOT(openDialog()));
    connect(arg_newView, SIGNAL(ResetLayout()), this, SLOT(ResetLayout()));
    for(int i = 0; i < m_layout->rowCount(); ++i) {
        m_layout->setRowStretch(i, 1);
        for(int j = 0; j < m_layout->columnCount(); ++j) {
            m_layout->setColumnStretch(j, 1);
        }
    }
}

void LampsWidget::disconnectViewFromSlots(CustomLampsGraphicsView *arg_view)
{
    disconnect(arg_view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(openContextMenu(QPoint)));
    disconnect(arg_view, SIGNAL(openDialog()), this, SLOT(openDialog()));
    disconnect(arg_view, SIGNAL(ResetLayout()), this, SLOT(ResetLayout()));
    this->layout()->takeAt(this->layout()->indexOf(arg_view))->widget()->deleteLater();
    for(int i = 0; i < m_layout->rowCount(); ++i) {
        m_layout->setRowStretch(i, 1);
        for(int j = 0; j < m_layout->columnCount(); ++j) {
            m_layout->setColumnStretch(j, 1);
        }
    }
}

void LampsWidget::backgroundColorChange(QColor arg_color)
{
    m_sceneBackgroundColor = arg_color;
    foreach (CustomLampsGraphicsView* loc_lamp, this->findChildren<CustomLampsGraphicsView*>("")) {
        loc_lamp->updateBackgroundColor(m_sceneBackgroundColor);
    }
}

void LampsWidget::changeColor(QColor arg_color)
{
    backgroundColorChange(arg_color);
}

void LampsWidget::changeFont(QFont arg_font)
{
    this->setFont(arg_font);
}

void LampsWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void LampsWidget::changeVariable(QString arg_variable, bool arg_visible)
{
   Q_UNUSED(arg_visible);
   Q_UNUSED(arg_variable);
}

void LampsWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_visible);
    Q_UNUSED(arg_variables);
}

void LampsWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables);
}

void LampsWidget::dragEnterEvent(QDragEnterEvent *event)
{
    Q_UNUSED(event)
}

void LampsWidget::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event)
}

void LampsWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
}

void LampsWidget::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event)
}

void LampsWidget::mousePressEvent(QMouseEvent* arg_event)
{
    switch (arg_event->button()) {
        case Qt::LeftButton:
        case Qt::RightButton:
        {
            QWidget *loc_widget = this->childAt(GetEventXPos(arg_event), GetEventYPos(arg_event));
            if (loc_widget != nullptr) {
                CustomLampsGraphicsView* loc_view = qobject_cast<CustomLampsGraphicsView*>(loc_widget);
                setSelectedLamp(loc_view);
            }
            break;
        }
        default:
            break;
    }
}

void LampsWidget::keyPressEvent(QKeyEvent* arg_event)
{
    switch(arg_event->key()) {
        case Qt::Key_I:
            if(arg_event->modifiers() == Qt::ControlModifier) {
                // TODO: Fokus der einzelnen GraphicsView fehlt, daher kann auch der Info Dialog nicht aufgerufen werden.
//                if(m_vid > 0) {
//                    char loc_variableName[BBVARI_NAME_SIZE];
//                    _get_bbvari_name(m_vid, loc_variableName);
//                    BlackboardInfoDialog Dlg;
//                    Dlg.editCurrentVariable(QString(loc_variableName));

//                    if (Dlg.exec() == QDialog::Accepted) {
//                        ;
//                    }
//                }
            }
            break;
        default:
            break;
    }
}

void LampsWidget::customStancilFromIni(ControlLampModel *arg_model) {
    QString loc_text;
    QString loc_entry;
    QString loc_section = QString("GUI/AllUserDefinedStancilForControlLamps");
    int loc_stancilIndex = 0;
    while(1) {
        loc_entry = QString("Stancil_%1").arg(loc_stancilIndex);
        loc_text = ScQt_IniFileDataBaseReadString(loc_section, loc_entry, "", ScQt_GetMainFileDescriptor());
        if (loc_text.isEmpty()) break;
        Lamp *Elem = new Lamp;
        if (Elem->init(loc_text).isEmpty()) {
            arg_model->addLamp (Elem);
        }

        loc_stancilIndex++;
    }
}


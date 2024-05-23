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


#include <string.h>
#include <inttypes.h>
#include "KnobOrTachoWidget.h"
#include "BlackboardObserver.h"
#include "MdiWindowType.h"
#include "StringHelpers.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QLabel>
#include "DragAndDrop.h"
#include "BlackboardInfoDialog.h"
#include "KnobDialogFrame.h"
#include "GetEventPos.h"
#include "QtIniFile.h"

#include <QMenu>

#include <math.h>

extern "C" {
#include "Config.h"
#include "Files.h"
#include "Compare2DoubleEqual.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "MainValues.h"
#include "EnvironmentVariables.h"
#include "ThrowError.h"
#include "IniDataBase.h"
}

#define DEFAULT_KNOP_RADIUS_FACTOR    0.95
#define DEFAULT_SLIDER_LEGTH_FACTOR   0.85
#define SPACE_BETWEEN_SCALE   5
#define UPPERFRAMESPACE   5
#define SPACE_BETWEEN_SCALETEXT   20
#define LONGSCALE 14
#define SHORTSCALE 6
#define DEFAULT_SCALE_PIXEL_STEP  100

KnobOrTachoWidget::KnobOrTachoWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType* par_Type,
                                     KnobOrTachoType par_KnobOrTachoType, QWidget* parent) :
    MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent),
    m_ObserverConnection(this)
{
    //layout for Widged
    QVBoxLayout *MyLayout = new QVBoxLayout();
    this->setLayout(MyLayout);
    VariableNameLabel = new QLabel("Test");
    ValueLabel = new QLabel("0815");
    Drawarea = new QWidget;
    VariableNameLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    ValueLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    Drawarea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
//    QPalette pal = QPalette();

//    // set black background
//    // Qt::black / "#000000" / "black"
//    pal.setColor(QPalette::Window, Qt::red);

//    Drawarea->setAutoFillBackground(true);
//    Drawarea->setPalette(pal);

    MyLayout->addWidget(VariableNameLabel,Qt::AlignTop);
    MyLayout->addWidget(Drawarea);
    //MyLayout->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Minimum,QSizePolicy::Expanding));
    MyLayout->addWidget(ValueLabel,Qt::AlignBottom);

    m_KnobOrTachoType = par_KnobOrTachoType;

    m_FirstUpdate = true;

    m_MoveFlag = false;
    m_Overwind = false;

    m_Vid = -1;

    m_Attributes.m_LowerBound = 0.0;
    m_Attributes.m_UpperBound = 1.0;

    m_FontSizeDyn = -1;

    m_Attributes.m_Physical = false;

    m_LastXpos = 0;
    m_LastYpos = 0;
    m_DebugAngle = 0.0;

    m_Attributes.m_TextAlign = 0;
    m_Attributes.m_NameAlign = 0;

    setAcceptDrops(true);
    maxLabelSize = 0;

    m_ConfigAct = new QAction(tr("&config"), this);
    m_ConfigAct->setStatusTip(tr("configure knob"));
    connect(m_ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigureSlot()));

    readFromIni();
}

KnobOrTachoWidget::~KnobOrTachoWidget()
{
    writeToIni();
    if (m_Vid > 0) {
        remove_bbvari_unknown_wait(m_Vid);
        m_ObserverConnection.RemoveObserveVariable(m_Vid);
    }
    if (m_DragStateVid > 0) {
        remove_bbvari_unknown_wait(m_DragStateVid);
    }
}

bool KnobOrTachoWidget::writeToIni()
{
    QString SectionPath = GetIniSectionPath();
    char loc_txt[INI_MAX_LINE_LENGTH], loc_variname[BBVARI_NAME_SIZE];
    int Fd = GetMainFileDescriptor();

   ScQt_IniFileDataBaseWriteString(SectionPath, "type", QStringToConstChar(GetMdiWindowType()->GetWindowTypeName()), Fd);

    if (m_Vid > 0){
        GetBlackboardVariableName (m_Vid, loc_variname, sizeof(loc_variname));
        sprintf (loc_txt, "%s, %f, %f, (%i,%i,%i), %s",
                 loc_variname,
                 m_Attributes.m_LowerBound,
                 m_Attributes.m_UpperBound,
                 m_Attributes.m_Color.red(),
                 m_Attributes.m_Color.green(),
                 m_Attributes.m_Color.blue(),
                 ((m_Attributes.m_Physical) ? "phys" : "dec"));
       ScQt_IniFileDataBaseWriteString(SectionPath, "variable", loc_txt, Fd);
    } else {
       ScQt_IniFileDataBaseWriteString(SectionPath, "variable", nullptr, Fd);
    }
    m_Attributes.writeToIni(SectionPath, m_KnobOrTachoType);

    return true;
}

bool KnobOrTachoWidget::readFromIni()
{
    QString SectionPath = GetIniSectionPath();
    char loc_VarStr[BBVARI_NAME_SIZE];
    int loc_red, loc_green, loc_blue;
    QString loc_Var;
    QStringList loc_VarList;
    int Fd = GetMainFileDescriptor();

    loc_red = 0;
    loc_green = 0;
    loc_blue = 0;
    if(ScQt_IniFileDataBaseReadString(SectionPath, "variable", "", loc_VarStr, sizeof (loc_VarStr), Fd)) {
        loc_Var = CharToQString(loc_VarStr);
        loc_VarList = loc_Var.split(",");
        parentWidget()->setToolTip(loc_VarList.at(0));
        if ((m_Vid = add_bbvari(QStringToConstChar(loc_VarList.value(0)), BB_UNKNOWN_WAIT, nullptr)) > 0) {
            m_ObserverConnection.AddObserveVariable(m_Vid, OBSERVE_CONFIG_ANYTHING_CHANGED);
            m_VariableName = loc_VarList.at(0);
            m_Attributes.m_LowerBound = loc_VarList.value(1).toDouble();
            m_Attributes.m_UpperBound = loc_VarList.value(2).toDouble();
            if (m_Attributes.m_UpperBound <= (m_Attributes.m_LowerBound + 0.0001)) m_Attributes.m_UpperBound = m_Attributes.m_LowerBound + 1.0;
            loc_red = loc_VarList.value(3).remove("(").toInt();
            loc_green = loc_VarList.value(4).toInt();
            loc_blue = loc_VarList.value(5).remove(")").toInt();
            m_Attributes.m_Color = QColor(loc_red, loc_green, loc_blue);
            if(loc_VarList.value(6).trimmed() == "phys") {
                m_Attributes.m_Physical = true;
            } else {
                m_Attributes.m_Physical = false;
            }
        }
    }
    setFont(m_Attributes.m_Font);
    m_Attributes.readFromIni(SectionPath, m_KnobOrTachoType);

    if (m_Attributes.m_DragStateVarFlag) {
        m_DragStateVid = add_bbvari (QStringToConstChar(m_Attributes.m_DragStateVariableName), BB_UNKNOWN_WAIT, nullptr);
    }

    QFileInfo loc_checkFile(m_Attributes.m_bitmap);
    if(loc_checkFile.exists() && loc_checkFile.isFile()) {
        m_bitmapPathExist = true;
    } else {
        m_bitmapPathExist = false;
    }
    if (m_bitmapPathExist) {
        if (!m_PixmapOrginalSize.load(SearchAndReplaceEnvironmentVariables(m_Attributes.m_bitmap))) {
            m_bitmapPathExist = false;
        }
        CheckBitmapWindowSizeBehaviour();
    }
    blackboardVariableConfigChanged(m_Vid, false);

    // Observer Funktion setzen um zu erfahren wann sich die Variable im Blackboard anmeldet
    //SetObserationCallbackFunction(observeVariable);
    //SetBbvariObservation(m_vid, OBSERVE_ALL_CONFIG);
    return true;
}

CustomDialogFrame* KnobOrTachoWidget::dialogSettings(QWidget* arg_parent)
{
    KnobDialogFrame *loc_frame = new KnobDialogFrame(this, arg_parent);
    switch (m_KnobOrTachoType) {
    case KnobType:
        arg_parent->setWindowTitle("Knob configuration");
        loc_frame->setTextforknoborslider(QString("Knob"));
        break;
    case TachoType:
        arg_parent->setWindowTitle("Tacho configuration");
        loc_frame->setTextforknoborslider(QString("Needle"));
        break;
    case SliderType:
        arg_parent->setWindowTitle("Slider configuration");
        loc_frame->setTextforknoborslider(QString("Knob"));
        break;
    case BargraphType:
        arg_parent->setWindowTitle("Bargrap configuration");
        loc_frame->setTextforknoborslider(QString("Bar"));
        break;
    }
    return loc_frame;
}

KnobOrTachoWidget::KnobOrTachoType KnobOrTachoWidget::GetType()
{
    return m_KnobOrTachoType;
}

void KnobOrTachoWidget::dragEnterEvent(QDragEnterEvent* event)
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

void KnobOrTachoWidget::dragMoveEvent(QDragMoveEvent* event)
{
    Q_UNUSED(event)
}

void KnobOrTachoWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event)
}

void KnobOrTachoWidget::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        DragAndDropInfos Infos (mime->text());
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
            if (m_Vid > 0) {
                remove_bbvari_unknown_wait(m_Vid);
                m_ObserverConnection.RemoveObserveVariable(m_Vid);
            }
            m_VariableName = Infos.GetName();
            m_Vid = add_bbvari(QStringToConstChar(Infos.GetName()), BB_UNKNOWN_WAIT, nullptr);
            if (m_Vid > 0) m_ObserverConnection.AddObserveVariable(m_Vid, OBSERVE_CONFIG_ANYTHING_CHANGED);
            //this->setWindowTitle(Infos.GetName());
            double min, max;
            if (Infos.GetMinMaxValue(&min, &max)) {
                 // todo m_knob.setScale(min, max);
            }
            /*QColor color = Infos.GetColor();
            if(color.isValid()) {
                QPalette loc_palette;
                loc_palette.setColor(QPalette::Button, color);
                setPalette(loc_palette);
            }*/
        }
    } else {
        event->ignore();
    }
}

void KnobOrTachoWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    openDialog();
}

void KnobOrTachoWidget::keyPressEvent(QKeyEvent* arg_event)
{
    switch(arg_event->key()) {
        case Qt::Key_I:
            if(arg_event->modifiers() == Qt::ControlModifier) {
                if(m_Vid > 0) {
                    char loc_variableName[BBVARI_NAME_SIZE];
                    GetBlackboardVariableName(m_Vid, loc_variableName, sizeof(loc_variableName));
                    BlackboardInfoDialog Dlg;
                    Dlg.editCurrentVariable(QString(loc_variableName));

                    if (Dlg.exec() == QDialog::Accepted) {
                        ;
                    }
                }
            }
            break;
        default:
            break;
    }
}

void KnobOrTachoWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction (m_ConfigAct);
    menu.exec(event->globalPos());
}

QColor KnobOrTachoWidget::getTextColorForBackground(const QColor& arg_color) const
{
    int colorSum = arg_color.red() + arg_color.green() + arg_color.blue();
    if((arg_color.green() > 196)||(colorSum > 384)) {
        return QColor(0,0,0);
    } else {
        if(arg_color == QColor(0,0,0)) {
            return QColor(0,255,0);
        } else {
            return QColor(255,255,255);
        }
    }
}

// Basic

bool KnobOrTachoWidget::getUseBlackboardConfig()
{
    return m_Attributes.m_BBCfgDirectFlag;
}

void KnobOrTachoWidget::setUseBlackboardConfig(bool arg_useConfig)
{
    m_Attributes.m_BBCfgDirectFlag = arg_useConfig;
}

double KnobOrTachoWidget::getMin()
{
    return m_Attributes.m_LowerBound;
}

void KnobOrTachoWidget::setMin(double arg_min)
{
    m_Attributes.m_LowerBound = arg_min;
}

double KnobOrTachoWidget::getMax()
{
    return m_Attributes.m_UpperBound;
}

void KnobOrTachoWidget::setMax(double arg_max)
{
    m_Attributes.m_UpperBound = arg_max;
}

char KnobOrTachoWidget::isPhys()
{
    return m_Attributes.m_Physical;
}

void KnobOrTachoWidget::setPhys(char arg_isPhys)
{
    m_Attributes.m_Physical = arg_isPhys;
}

QColor KnobOrTachoWidget::getColor()
{
    return m_Attributes.m_Color;
}

QFont KnobOrTachoWidget::getFont()
{
    return m_Attributes.m_Font;
}

void KnobOrTachoWidget::setFont(QFont par_Font)
{
    m_Attributes.m_Font = par_Font;
}

void KnobOrTachoWidget::setColor(QColor par_Color)
{
    m_Attributes.m_Color = par_Color;
}

// Scale

int KnobOrTachoWidget::getScaleFlag()
{
    return m_Attributes.m_ScaleFlag;
}

void KnobOrTachoWidget::setScaleFlag(int arg_showScale)
{
    m_Attributes.m_ScaleFlag = arg_showScale;
}

double KnobOrTachoWidget::getMaxAngle()
{
    return m_Attributes.m_MaxAngle;
}

void KnobOrTachoWidget::setMaxAngle(double arg_maxAngle)
{
    m_Attributes.m_MaxAngle = arg_maxAngle;
}

double KnobOrTachoWidget::getMinAngle()
{
    return m_Attributes.m_MinAngle;
}

void KnobOrTachoWidget::setMinAngle(double arg_minAngle)
{
    m_Attributes.m_MinAngle = arg_minAngle;
}

bool KnobOrTachoWidget::getScaleStepFlag()
{
    return m_Attributes.m_ScaleStepFlag;
}

void KnobOrTachoWidget::setScaleStepFlag(bool arg_stepFlag)
{
    m_Attributes.m_ScaleStepFlag = arg_stepFlag;
}

double KnobOrTachoWidget::getScaleStep()
{
    return m_Attributes.m_ScaleStep;
}

void KnobOrTachoWidget::setScaleStep(double arg_Step)
{
    m_Attributes.m_ScaleStep = arg_Step;
}

double KnobOrTachoWidget::getScalePosX()
{
    return m_Attributes.m_ScalePosX;
}

void KnobOrTachoWidget::setScalePosX(double arg_posX)
{
    m_Attributes.m_ScalePosX = arg_posX;
}

double KnobOrTachoWidget::getScalePosY()
{
    return m_Attributes.m_ScalePosY;
}

void KnobOrTachoWidget::setScalePosY(double arg_posY)
{
    m_Attributes.m_ScalePosY = arg_posY;
}

bool KnobOrTachoWidget::getScaleSizeFlag()
{
    return m_Attributes.m_ScaleSizeFlag;
}

void KnobOrTachoWidget::setScaleSizeFlag(bool arg_sizeFlag)
{
    m_Attributes.m_ScaleSizeFlag = arg_sizeFlag;
}

double KnobOrTachoWidget::getScaleSize()
{
    return m_Attributes.m_ScaleSize;
}

void KnobOrTachoWidget::setScaleSize(double arg_length)
{
    m_Attributes.m_ScaleSize = arg_length;
}

bool KnobOrTachoWidget::getScaleColorFlag()
{
    return m_Attributes.m_ScaleColorFlag;
}

void KnobOrTachoWidget::setScaleColorFlag(bool arg_colorFlag)
{
    m_Attributes.m_ScaleColorFlag = arg_colorFlag;
}

QColor KnobOrTachoWidget::getScaleColor()
{
    return m_Attributes.m_ScaleColor;
}

void KnobOrTachoWidget::setScaleColor(QColor arg_color)
{
    m_Attributes.m_ScaleColor = arg_color;
}

// Knob

int KnobOrTachoWidget::getNeedleFlag()
{
    return m_Attributes.m_NeedleFlag;
}

void KnobOrTachoWidget::setNeedleFlag(int arg_showNeedle)
{
    m_Attributes.m_NeedleFlag = arg_showNeedle;
}

double KnobOrTachoWidget::getNeedlePosX()
{
    return m_Attributes.m_NeedlePosX;
}

void KnobOrTachoWidget::setNeedlePosX(double arg_posX)
{
    m_Attributes.m_NeedlePosX = arg_posX;
}

double KnobOrTachoWidget::getNeedlePosY()
{
    return m_Attributes.m_NeedlePosY;
}

void KnobOrTachoWidget::setNeedlePosY(double arg_posY)
{
    m_Attributes.m_NeedlePosY = arg_posY;
}

bool KnobOrTachoWidget::getNeedleSizeFlag()
{
    return m_Attributes.m_NeedleSizeFlag;
}

void KnobOrTachoWidget::setNeedleSizeFlag(bool arg_SizeFlag)
{
    m_Attributes.m_NeedleSizeFlag = arg_SizeFlag;
}

double KnobOrTachoWidget::getNeedleLength()
{
    return m_Attributes.m_NeedleSize;
}

void KnobOrTachoWidget::setNeedleLength(double arg_length)
{
    m_Attributes.m_NeedleSize = arg_length;
}

bool KnobOrTachoWidget::getNeedleLineColorFlag()
{
    return m_Attributes.m_NeedleLineColorFlag;
}

void KnobOrTachoWidget::setNeedleLineColorFlag(bool arg_colorFlag)
{
    m_Attributes.m_NeedleLineColorFlag = arg_colorFlag;
}

QColor KnobOrTachoWidget::getNeedleLineColor()
{
    return m_Attributes.m_NeedleLineColor;
}

void KnobOrTachoWidget::setNeedleLineColor(QColor arg_color)
{
    m_Attributes.m_NeedleLineColor = arg_color;
}

bool KnobOrTachoWidget::getNeedleFillColorFlag()
{
    return m_Attributes.m_NeedleFillColorFlag;
}

void KnobOrTachoWidget::setNeedleFillColorFlag(bool arg_colorFlag)
{
    m_Attributes.m_NeedleFillColorFlag = arg_colorFlag;
}

QColor KnobOrTachoWidget::getNeedleFillColor()
{
    return m_Attributes.m_NeedleFillColor;
}

void KnobOrTachoWidget::setNeedleFillColor(QColor arg_color)
{
    m_Attributes.m_NeedleFillColor = arg_color;
}

// Value

int KnobOrTachoWidget::getTextFlag()
{
    return m_Attributes.m_TextFlag;
}

void KnobOrTachoWidget::setTextFlag(int arg_showText)
{
    m_Attributes.m_TextFlag = arg_showText;
}

double KnobOrTachoWidget::getTextPosX()
{
    return m_Attributes.m_TextPosX;
}

void KnobOrTachoWidget::setTextPosX(double arg_posX)
{
    m_Attributes.m_TextPosX = arg_posX;
}

double KnobOrTachoWidget::getTextPosY()
{
    return m_Attributes.m_TextPosY;
}

void KnobOrTachoWidget::setTextPosY(double arg_posY)
{
    m_Attributes.m_TextPosY = arg_posY;
}

bool KnobOrTachoWidget::getTextFontFlag()
{
   return m_Attributes.m_TextFontFlag;
}

void KnobOrTachoWidget::setTextFontFlag(bool par_FontFlag)
{
    m_Attributes.m_TextFontFlag = par_FontFlag;
}

QFont KnobOrTachoWidget::getTextFont()
{
   return m_Attributes.m_TextFont;
}

void KnobOrTachoWidget::setTextFont(QFont &par_Font)
{
    m_Attributes.m_TextFont = par_Font;
}

int KnobOrTachoWidget::getTextAlign()
{
    return m_Attributes.m_TextAlign;
}

void KnobOrTachoWidget::SetTextAlign(int par_Align)
{
    m_Attributes.m_TextAlign = par_Align;
}

// Name

int KnobOrTachoWidget::getNameFlag()
{
    return m_Attributes.m_NameFlag;
}

void KnobOrTachoWidget::setNameFlag(int arg_showName)
{
    m_Attributes.m_NameFlag = arg_showName;
}

double KnobOrTachoWidget::getNamePosX()
{
    return m_Attributes.m_NamePosX;
}

void KnobOrTachoWidget::setNamePosX(double arg_posX)
{
    m_Attributes.m_NamePosX = arg_posX;
}

double KnobOrTachoWidget::getNamePosY()
{
    return m_Attributes.m_NamePosY;
}

void KnobOrTachoWidget::setNamePosY(double arg_posY)
{
    m_Attributes.m_NamePosY = arg_posY;
}

bool KnobOrTachoWidget::getNameFontFlag()
{
   return m_Attributes.m_NameFontFlag;
}

void KnobOrTachoWidget::setNameFontFlag(bool par_FontFlag)
{
    m_Attributes.m_NameFontFlag = par_FontFlag;
}

QFont KnobOrTachoWidget::getNameFont()
{
   return m_Attributes.m_NameFont;
}

void KnobOrTachoWidget::setNameFont(QFont &par_Font)
{
    m_Attributes.m_NameFont = par_Font;
}

int KnobOrTachoWidget::getNameAlign()
{
    return m_Attributes.m_NameAlign;
}

void KnobOrTachoWidget::SetNameAlign(int par_Align)
{
    m_Attributes.m_NameAlign = par_Align;
}

// Bitmap

int KnobOrTachoWidget::getBitmapFlag()
{
    return m_Attributes.m_BitmapFlag;
}

void KnobOrTachoWidget::setBitmapFlag(int arg_showBitmap)
{
    m_Attributes.m_BitmapFlag = arg_showBitmap;
}

QString KnobOrTachoWidget::getBitmapPath()
{
    return m_Attributes.m_bitmap;
}

void KnobOrTachoWidget::setBitmapPath(QString arg_path)
{
    QFileInfo loc_checkFile(arg_path);
    if(loc_checkFile.exists() && loc_checkFile.isFile()) {
        m_bitmapPathExist = true;
    } else {
        m_bitmapPathExist = false;
    }
    m_Attributes.m_bitmap = arg_path;
}

void KnobOrTachoWidget::setBitmapSizeBehaviour(KnobOrTachoWidget::BitmapWindowSizeBehaviour arg_behaviour)
{
    m_Attributes.m_BitmapWindowSizeBehaviour = arg_behaviour;
    CheckBitmapWindowSizeBehaviour();
}

// If release fallback to

bool KnobOrTachoWidget::getIfReleaseFallbackToFlag()
{
    return m_Attributes.m_IfReleaseFallbackFlag;
}

void KnobOrTachoWidget::setIfReleaseFallbackToFlag(bool arg_Flag)
{
    m_Attributes.m_IfReleaseFallbackFlag = arg_Flag;
}

double KnobOrTachoWidget::getIfReleaseFallbackToValue()
{
    return m_Attributes.m_IfReleaseFallbackValue;
}

void KnobOrTachoWidget::setIfReleaseFallbackToValue(double arg_Value)
{
    m_Attributes.m_IfReleaseFallbackValue = arg_Value;
}

// Drag status

bool KnobOrTachoWidget::getDragStatusFlag()
{
    return m_Attributes.m_DragStateVarFlag;
}

void KnobOrTachoWidget::setDragStatusFlag(bool arg_Flag)
{
    m_Attributes.m_DragStateVarFlag = arg_Flag;
}

QString KnobOrTachoWidget::getDragStatusSignal()
{
    return m_Attributes.m_DragStateVariableName;
}

void KnobOrTachoWidget::setDragStatusSignal(QString arg_Name)
{
    m_Attributes.m_DragStateVariableName = arg_Name;
}

void KnobOrTachoWidget::CopyAttributesFrom(KnobOrTachoWidget *par_From)
{
    m_Attributes = par_From->m_Attributes;
}

void KnobOrTachoWidget::CheckChangeOfDragStatusSignal(bool par_DragStatusFlagOld, QString par_DragStatusNameOld)
{
    if (m_Attributes.m_DragStateVarFlag) {
        if (m_Attributes.m_DragStateVariableName.compare(par_DragStatusNameOld)) {
            // Name has changed
            if (m_DragStateVid > 0) {
                remove_bbvari_unknown_wait(m_DragStateVid);
            }
            m_DragStateVid = add_bbvari (QStringToConstChar(m_Attributes.m_DragStateVariableName), BB_UNKNOWN_WAIT, nullptr);
        }
    } else {
        if (par_DragStatusFlagOld) {
            if (m_DragStateVid > 0) {
                remove_bbvari_unknown_wait(m_DragStateVid);
            }
            m_DragStateVid = -1;
        }
    }
}

KnobOrTachoWidget::BitmapWindowSizeBehaviour KnobOrTachoWidget::getBitmapSizeBehaviour()
{
    return m_Attributes.m_BitmapWindowSizeBehaviour;
}

void KnobOrTachoWidget::CheckBitmapWindowSizeBehaviour()
{
    MdiSubWindow *MdiSubWidow = GetCustomMdiSubwindow();
    if ((m_Attributes.m_BitmapWindowSizeBehaviour == FixedWindowSize) && m_bitmapPathExist) {
        QSize MdiSize = MdiSubWidow->size();
        QSize ChildSize = size();
        int DiffX = MdiSize.width() - ChildSize.width();
        int DiffY = MdiSize.height() - ChildSize.height();
        QSize PixmapSize = m_PixmapOrginalSize.size();
        QSize FixedSize;
        FixedSize.setWidth(PixmapSize.width()+DiffX);
        FixedSize.setHeight(PixmapSize.height()+DiffY);
        MdiSubWidow->resize(FixedSize);
        MdiSubWidow->setMaximumSize(FixedSize);
        MdiSubWidow->setMinimumSize(FixedSize);
    } else {
        MdiSubWidow->setMaximumSize(QSize(16777215,16777215));
        MdiSubWidow->setMinimumSize(GetMdiWindowType()->GetMinSize());
    }
}

double KnobOrTachoWidget::minArcAngleForDial(const double arg_minArcIni) const
{
    return arg_minArcIni -90.0;
}

double KnobOrTachoWidget::maxArcAngleForDial(const double arg_minArcIni, const double arg_maxArcIni) const
{
    return abs(arg_minArcIni) + arg_maxArcIni;
}

double KnobOrTachoWidget::minArcAngleForIni(const double arg_minArcDial) const
{
    return arg_minArcDial + 90.0;
}

double KnobOrTachoWidget::maxArcAngleForIni(const double arg_minArcDial, const double arg_maxArcDial) const
{
    return arg_minArcDial + arg_maxArcDial + 90.0;
}

void KnobOrTachoWidget::openDialog() {
    m_SavedAttributes = m_Attributes;
    QStringList List;
    List.append(m_VariableName);
    emit openStandardDialog(List, true, false, m_Attributes.m_Color);
}

void KnobOrTachoWidget::CyclicUpdate()
{
    if (m_FirstUpdate) {
        m_FirstUpdate = false;
        CheckBitmapWindowSizeBehaviour();
    }
    if (m_Attributes.m_BitmapWindowSizeBehaviour == ScaleBitMap) {
        QSize Size = size();
        if (((Size.width()) != m_Size.width()) || (Size.height() != m_Size .height())) {
            m_PixmapResized = m_PixmapOrginalSize.scaled(Size);
            m_Size = Size;
        }
    }

    bool HasChangdSomething = false;
    if (m_Vid > 0) {
        int loc_DataType = get_bbvaritype(m_Vid);
        if (m_DataType != loc_DataType) HasChangdSomething = true;
        m_DataType = loc_DataType;
        if (m_DataType != BB_UNKNOWN_WAIT) {
            int loc_ConversionType = get_bbvari_conversiontype(m_Vid);
            if (m_ConversionType != loc_ConversionType) HasChangdSomething = true;
            m_ConversionType = loc_ConversionType;
            double loc_value;
            if(m_Attributes.m_Physical && (m_ConversionType == BB_CONV_FORMULA)) {
                loc_value = read_bbvari_equ(m_Vid);
                if (DisplayUnitForNonePhysicalValues) m_DisplayUnit = true;
                else m_DisplayUnit = false;
            } else {
                loc_value = read_bbvari_convert_double(m_Vid);
                m_DisplayUnit = true;
            }
            if (loc_value > m_Attributes.m_UpperBound) loc_value = m_Attributes.m_UpperBound;
            if (loc_value < m_Attributes.m_LowerBound) loc_value = m_Attributes.m_LowerBound;
            if(!CompareDoubleEqual_int(m_CurrentValue, loc_value)) {
                HasChangdSomething = true;
                m_CurrentValue = loc_value;
            }
            else
            {
                drawValue();
            }
        }
    } else if (m_VidLastUpdate != m_Vid) {
        HasChangdSomething = true;
        m_CurrentValue = 0.0;
        m_DisplayUnit = false;
    }
    if (HasChangdSomething) {
        update();
    }
    m_VidLastUpdate = m_Vid;
}

void KnobOrTachoWidget::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    Q_UNUSED(arg_observationFlag)
    if(m_Vid == arg_vid) {
        if (m_Attributes.m_BBCfgDirectFlag) { //Informations locally stored
            m_Attributes.m_UpperBound = get_bbvari_max(m_Vid);
            m_Attributes.m_LowerBound = get_bbvari_min(m_Vid);
            /*int loc_color = get_bbvari_color(m_vid);
            QColor loc_backgroundColor = QColor(loc_color&0x000000FF, (loc_color&0x0000FF00)>>8, (loc_color&0x00FF0000)>>16); // Sh. Win Colors 0x00bbggrr
            QPalette loc_palette;
            loc_palette.setColor(QPalette::Text, getTextColorForBackground(loc_backgroundColor));
            loc_palette.setColor(QPalette::Base, loc_backgroundColor);
            setPalette(loc_palette);*/
        }
        char loc_unit[BBVARI_UNIT_SIZE];
        get_bbvari_unit(m_Vid, loc_unit, BBVARI_UNIT_SIZE);
        if(get_bbvaritype(m_Vid) != BB_UNKNOWN_WAIT) {
            double loc_value;
            if(m_Attributes.m_Physical && (get_bbvari_conversiontype(m_Vid) == BB_CONV_FORMULA)) {
                loc_value = read_bbvari_equ(m_Vid);
            } else {
                loc_value = read_bbvari_convert_double(m_Vid);
            }
            m_CurrentValue = loc_value;
        }
    }
}

void KnobOrTachoWidget::changeBlackboardValue(double arg_value)
{
    if (m_Attributes.m_Physical && (get_bbvari_conversiontype(m_Vid) == BB_CONV_FORMULA)) {
        write_bbvari_phys_minmax_check(m_Vid, arg_value);
    } else {
        write_bbvari_minmax_check(m_Vid, arg_value);
    }
}

void KnobOrTachoWidget::changeColor(QColor arg_color)
{
    m_Attributes.m_Color = arg_color;
}

void KnobOrTachoWidget::changeFont(QFont arg_font)
{
    m_Attributes.m_Font = arg_font;
    setFont(arg_font);
}

void KnobOrTachoWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void KnobOrTachoWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    Q_UNUSED(arg_visible)
    if(m_Vid > 0) {
        m_ObserverConnection.RemoveObserveVariable(m_Vid);
        remove_bbvari_unknown_wait(m_Vid);
    }
    m_VariableName = arg_variable;
    m_Vid = add_bbvari(arg_variable.toLocal8Bit().data(), BB_UNKNOWN_WAIT, "");
    if(m_Vid > 0) {
        m_ObserverConnection.AddObserveVariable(m_Vid, OBSERVE_CONFIG_ANYTHING_CHANGED);
    }
}

void KnobOrTachoWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_variables)
    Q_UNUSED(arg_visible)
}

void KnobOrTachoWidget::resetDefaultVariables(QStringList arg_variables)
{
    if(m_Vid > 0) {
        m_ObserverConnection.RemoveObserveVariable(m_Vid);
        remove_bbvari_unknown_wait(m_Vid);
    }
    if(arg_variables.count() > 0) {
        m_Vid = add_bbvari(arg_variables.first().toLocal8Bit().data(), BB_UNKNOWN_WAIT, "");
        if(m_Vid > 0) {
            m_ObserverConnection.AddObserveVariable(m_Vid, OBSERVE_CONFIG_ANYTHING_CHANGED);
        }
    }
    m_Attributes = m_SavedAttributes;
}

void KnobOrTachoWidget::setBBConfigDirect(bool arg_value)
{
    m_Attributes.m_BBCfgDirectFlag = arg_value;
}

void KnobOrTachoWidget::setVariablePhysical(bool arg_value)
{
    m_Attributes.m_Physical = arg_value;
}

void KnobOrTachoWidget::setBitmapVisible(bool arg_value)
{
    m_Attributes.m_BitmapFlag = arg_value;
}

void KnobOrTachoWidget::ConfigureSlot()
{
    openDialog();
}

void KnobOrTachoWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_KnobOrTachoType == KnobType) {
        if (event->button() == Qt::LeftButton) {
            QPoint Pos = event->pos();
            int xdim = width();
            int ydim = height();
            double KnobRadius;
            int x = Pos.rx() - xdim/2;
            int y = Pos.ry() - ydim/2;
            double RadiusQ;

            if (m_Attributes.m_NeedleSizeFlag) {
                KnobRadius = ((xdim < ydim) ? xdim : ydim) * m_Attributes.m_NeedleSize / 100.0;
            } else {
                KnobRadius = ((xdim < ydim) ? xdim : ydim) * DEFAULT_KNOP_RADIUS_FACTOR;
            }
            if (y == 0) {
                RadiusQ = abs(x);
                RadiusQ = RadiusQ * RadiusQ;
            } else {
                RadiusQ = ((double)x * (double)x) + ((double)y + (double)y);
            }
            if (RadiusQ < (KnobRadius * KnobRadius)) {
                m_MoveStartPoint = Pos;
                if (m_Attributes.m_DragStateVarFlag &&
                    m_DragStateVid > 0) {
                    write_bbvari_minmax_check (m_DragStateVid, 1.0);
                }
                m_MoveFlag = true;
                m_Overwind = false;
                return;
            } else {
                // maybe Drag & Drop?
                m_startDragPosition = GetEventGlobalPos(event);
            }
        }
    } else if (m_KnobOrTachoType == SliderType) {
        if (event->button() == Qt::LeftButton) {
            QPoint Pos = event->pos();
            QRect Rect = QRect(0, Drawarea->y(),Drawarea->width(),Drawarea->height());// Drawarea->rect(); //CalcSliderRect(width(), height());
            if ((Pos.rx() >= Rect.left()) && (Pos.rx() <= Rect.right()) &&
                (Pos.ry() >= Rect.top()) && (Pos.ry() <=  Rect.bottom())) {
                m_MoveStartPoint = Pos;
                m_DragValue = m_CurrentValue;
                if (m_Attributes.m_DragStateVarFlag &&
                    (m_DragStateVid > 0)) {
                    write_bbvari_minmax_check (m_DragStateVid, 1.0);
                }
                m_MoveFlag = true;
                return;
            } else {
                // maybe Drag &Drop?
                m_startDragPosition = GetEventGlobalPos(event);
            }
        }
    }
    MdiWindowWidget::mousePressEvent(event);
}

void KnobOrTachoWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_MoveFlag) {
        if (m_Attributes.m_IfReleaseFallbackFlag) {  // FallBack is active
            if ((event->modifiers() & Qt::ShiftModifier) != Qt::ShiftModifier) { // FallBack if Shift is pressed
                WriteSliderValue (m_Attributes.m_IfReleaseFallbackValue);
            }
        } else {                                     // FallBack is not active
            if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) { // FallBack if Shift is not pressed
                WriteSliderValue (m_Attributes.m_IfReleaseFallbackValue);
            }
        }
        if (m_Attributes.m_DragStateVarFlag &&
            (m_DragStateVid > 0)) {
            write_bbvari_minmax_check (m_DragStateVid, 0.0);
        }
        m_MoveFlag = false;
    } else {
        MdiWindowWidget::mouseReleaseEvent(event);
    }
}

void KnobOrTachoWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_MoveFlag) {
        if (m_KnobOrTachoType == KnobType) {
            int dx = event->pos().rx() - width() / 2;
            int dy = event->pos().ry() - height() / 2;

            if (m_Overwind) {
                if (m_OverwindDir) {
                    if ((dx > 0) && (dy > 0)) {
                        m_Overwind = false;
                    }
                } else {
                    if ((dx < 0) && (dy > 0)) {
                        m_Overwind = false;
                    }
                }
            } else {
                if ((dy > 0) && (m_LastXpos < 0) && (dx >= 0)) {
                    m_Overwind = true;
                    m_OverwindDir = false;
                } else if ((dy > 0) && (m_LastXpos > 0) && (dx <= 0)) {
                    m_Overwind = true;
                    m_OverwindDir = true;
                }
            }
            m_LastXpos = dx;
            m_LastYpos = dy;
            if (!m_Overwind) {
                if (dy != 0) {
                    double a, NewValue, Angle;
                    if (dy < 0) {
                        a = (double)dx / (double)(-dy);
                        Angle = atan(a) * 360.0 / (2.0 * M_PI);
                    } else if (dy > 0) {
                        a = (double)dx / (double)dy;
                        if (dx >= 0) {
                            Angle = 180.0 - atan(a) * 360.0 / (2.0 * M_PI);
                        } else {
                            Angle = -180.0 - atan(a) * 360.0 / (2.0 * M_PI);
                        }
                    } else {
                        Angle = 0.0;
                    }
                    m_DebugAngle = Angle;
                    NewValue = (Angle - m_Attributes.m_MinAngle) / (m_Attributes.m_MaxAngle - m_Attributes.m_MinAngle) * (m_Attributes.m_UpperBound - m_Attributes.m_LowerBound) + m_Attributes.m_LowerBound;
                    if (NewValue < m_Attributes.m_LowerBound) NewValue = m_Attributes.m_LowerBound;
                    if (NewValue > m_Attributes.m_UpperBound) NewValue = m_Attributes.m_UpperBound;

                    WriteSliderValue (NewValue);
                    update();
                }
            } else {
                update();   // only for debugging
            }
        } else if (m_KnobOrTachoType == SliderType) {
            double Len = CalcSliderLen(height());
            double Diff = m_MoveStartPoint.ry() - event->pos().ry();
            double Range = m_Attributes.m_UpperBound - m_Attributes.m_LowerBound;
            double new_value = m_DragValue + (Diff / Len) * Range;
            if (new_value < m_Attributes.m_LowerBound) new_value = m_Attributes.m_LowerBound;
            if (new_value > m_Attributes.m_UpperBound) new_value = m_Attributes.m_UpperBound;

            WriteSliderValue (new_value);

            update();
        }
    } else {
        if(!(event->buttons() & Qt::LeftButton)) {
            return;
        }
        if((GetEventGlobalPos(event) - m_startDragPosition).manhattanLength() < QApplication::startDragDistance()) {
            return;
        }
        //enum DragAndDropAlignment Alignment;
        enum DragAndDropDisplayMode DisplayMode;

        if (m_Attributes.m_Physical) {
            DisplayMode = DISPLAY_MODE_PHYS;
        } else {
            DisplayMode = DISPLAY_MODE_DEC;
        }

        DragAndDropInfos Infos;
        Infos.SetName(m_VariableName);
        Infos.SetColor(m_Attributes.m_Color);
        Infos.SetMinMaxValue(m_Attributes.m_LowerBound, m_Attributes.m_UpperBound);
        Infos.SetDisplayMode(DisplayMode);
        //Infos.SetAlignment(Alignment);
        QDrag *loc_dragObject = buildDragObject(this, &Infos);
        loc_dragObject->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);

        MdiWindowWidget::mouseMoveEvent(event);
    }
}

double KnobOrTachoWidget::PaintRoundScale(QPainter &Painter, int par_Width, int par_Height, int par_Offset)
{
    QPen SavePen;
    double pixel_per_unit;
    double pixel_per_line; // space between two lines
    double pixel_first_line, value_first_text;

    double Diameter = ((par_Width < par_Height) ? par_Width : par_Height);
    double Radius = Diameter / 2.0;
    double ValueRange = m_Attributes.m_UpperBound - m_Attributes.m_LowerBound; //range of Value
    double Circumference = M_PI * Diameter;
    double SegmentCircumference = Circumference / 360.0 * (m_Attributes.m_MaxAngle - m_Attributes.m_MinAngle); //length of drawing cicle in Pixel

    int x;
    int moduloforlabel = 10;

    if (SegmentCircumference > 1000)
    {
         moduloforlabel = 20;
    }

    ym = Drawarea->y() + (Drawarea->height()* m_Attributes.m_ScalePosY / 100.0);
    xm = (int)((double)par_Width * m_Attributes.m_ScalePosX / 100.0);

    // adapt scale
    if (ValueRange <= 0.0) {
        pixel_per_line = pixel_per_unit = 1.0;
    } else {
        pixel_per_line = pixel_per_unit = SegmentCircumference / ValueRange;
    }
    if (pixel_per_line <= 0.0) pixel_per_line = 1.0;

    if (m_Attributes.m_ScaleStepFlag) {
        pixel_per_line = SegmentCircumference / (m_Attributes.m_MaxAngle - m_Attributes.m_MinAngle) * m_Attributes.m_ScaleStep; //calc user Scale
    } else {
        x = 0;
        if (pixel_per_line > DEFAULT_SCALE_PIXEL_STEP) {
            while (pixel_per_line > DEFAULT_SCALE_PIXEL_STEP) {
                if ((x % 3 == 0) || (x % 3 == 2)) pixel_per_line /= 2.0;
                else if (x % 3 == 1) pixel_per_line /= 2.5;
                x++;
            }
        } else if (pixel_per_line < DEFAULT_SCALE_PIXEL_STEP) {
            while (pixel_per_line < DEFAULT_SCALE_PIXEL_STEP) {
                if ((x % 3 == 0) || (x % 3 == 2)) pixel_per_line *= 2.0;
                else if (x % 3 == 1) pixel_per_line *= 2.5;
                x++;
            }
        }
    }
    pixel_first_line = ceil (m_Attributes.m_LowerBound * pixel_per_unit / pixel_per_line) * pixel_per_line
                           - m_Attributes.m_LowerBound * pixel_per_unit;
    value_first_text = ceil (m_Attributes.m_LowerBound * pixel_per_unit / pixel_per_line) * pixel_per_line / pixel_per_unit;

    pixel_first_line -= pixel_per_line;
    value_first_text -= pixel_per_line / pixel_per_unit;
    pixel_per_line /= 10;  /* 10 interval */

    // paint the scale
    int FontSize = Radius / 12 + 1;
    if (FontSize < 6) FontSize = 6;
    if (FontSize > 20) FontSize = 20;
    if (m_FontSizeDyn != FontSize) {
        m_ScaleFont = QFont(m_Attributes.m_Font.family(), FontSize);
        m_FontSizeDyn = FontSize;
    }
    Painter.setFont(m_ScaleFont);

    double savepixel_first_line = pixel_first_line;
    double savevalue_first_text = value_first_text;
    x = 0;
    maxLabelSize = 0;
    maxLabelSizey = 0;
    while (pixel_first_line < SegmentCircumference)
    {
        if (pixel_first_line >= 0.0) {
            if (!(x%moduloforlabel))
            {
                char txt[64];
                if ((value_first_text > 1000000.0) || (value_first_text < -1000000.0)) {
                    sprintf (txt, "%e", value_first_text);
                } else if ((value_first_text < 0.0000001) && (value_first_text > -0.0000001)) {
                    sprintf (txt, "0");
                } else {
                    sprintf (txt, "%g", value_first_text);
                }
                QString ValueString(txt);
                QFontMetrics FontMetrics(m_ScaleFont);
                QRect Rect = FontMetrics.boundingRect(ValueString);
                int Width = Rect.width();
                int Height =Rect.height();
                if (maxLabelSize < Width)
                {
                    maxLabelSize = Width;
                }
                if (maxLabelSizey < Height)
                {
                    maxLabelSizey = Height;
                }
            }
        }
        value_first_text += pixel_per_line / pixel_per_unit;
        pixel_first_line += pixel_per_line;
        x++;
    }
    maxLabelSize = maxLabelSize;
    maxLabelSizey = maxLabelSizey;

    //new calculation of Radius with the know of label length and heigh
    double circlewidth = par_Width - (2 * maxLabelSize) - (3 * LONGSCALE);
    double circleheight = par_Height - (2 * maxLabelSizey) - (3 * LONGSCALE);
    Diameter = ((circlewidth < circleheight) ? circlewidth : circleheight);
    Radius = Diameter / 2.0;

    if (m_Attributes.m_ScaleSizeFlag) {
        Radius = m_Attributes.m_ScaleSize * Radius / 100.0; //scaling from User
    }

    value_first_text = savevalue_first_text;
    pixel_first_line = savepixel_first_line;

    x = 0;
    while (pixel_first_line < SegmentCircumference) {
        if (pixel_first_line >= 0.0) {
            double Angle =  m_Attributes.m_MinAngle + (pixel_first_line / Circumference) * 360.0;
            int Len = ((x%5)?SHORTSCALE:(x%10)?10:LONGSCALE);
            double xp = sin((Angle / 360.0) * 2.0 * M_PI);
            double yp = cos((Angle / 360.0) * 2.0 * M_PI);
            int x1 = xp * Radius;
            int x2 = xp * (Radius + Len);
            int y1 = yp * Radius;
            int y2 = yp * (Radius + Len);
            Painter.drawLine (x1 + xm, ym - y1, x2 + xm, ym - y2);
            if (!(x%moduloforlabel)) {
                {//if (!m_Attributes.m_TextFlag)  {//|| ((Angle < -30.0) || (Angle > 30.0))) {
                    char txt[64];
                    if ((value_first_text > 1000000.0) || (value_first_text < -1000000.0)) {
                        sprintf (txt, "%e", value_first_text);
                    } else if ((value_first_text < 0.0000001) && (value_first_text > -0.0000001)) {
                            sprintf (txt, "0");
                    } else {
                        sprintf (txt, "%g", value_first_text);
                    }
                    double x3 = xp * (Radius + 1.5 * Len);
                    double y3 = yp * (Radius + 1.5 * Len);
                    QString ValueString(txt);
                    QFontMetrics FontMetrics(m_ScaleFont);
                    QRect Rect = FontMetrics.boundingRect(ValueString);
                    int Width = Rect.width();
                    int Height =Rect.height();
                    Qt::AlignmentFlag Alignment;
                    if (Angle > 30.0) {
                        Alignment = Qt::AlignRight;
                        Rect.setLeft(Rect.left() + xm + x3);
                    } else if (Angle < -30.0) {
                        Alignment = Qt::AlignLeft;
                        Rect.setLeft(Rect.left() + xm + x3 - Width);
                    } else {
                        Alignment = Qt::AlignCenter;
                        Rect.setLeft(Rect.left() + xm + x3 - Width/2);
                    }
                    Rect.setTop(Rect.top() + ym - y3);
                    Rect.setWidth(Width);
                    Rect.setHeight(Height);

                    Painter.drawText (Rect, Alignment, ValueString);
                }
            }
        }
        value_first_text += pixel_per_line / pixel_per_unit;
        pixel_first_line += pixel_per_line;
        x++;
    }
    if (m_Attributes.m_ScaleColorFlag) {
        Painter.setPen(SavePen);
    }



    // drawing the round scale
    QRect Rectangle(xm - Radius, ym - Radius, Radius*2, Radius*2);
    if (m_Attributes.m_ScaleColorFlag) {
        SavePen = Painter.pen();
        Painter.setPen(m_Attributes.m_ScaleColor);
    }
    Painter.drawArc(Rectangle, (90 - m_Attributes.m_MaxAngle) * 16, (m_Attributes.m_MaxAngle - m_Attributes.m_MinAngle) * 16);

    return Radius;
}

void KnobOrTachoWidget::PaintLinearSlopeScale(QPainter &Painter, int par_Width, int par_Height, int par_Offset)
{
    QPen SavePen;
    double pixel_per_unit, pixel_per_line;
    double pixel_first_line, value_first_text;
    int x;

    //int xm = (int)((double)par_Width * m_Attributes.m_ScalePosX / 100.0);
    int SliderLen;
    int StartSlider;
    int EndSlider;

    if (m_KnobOrTachoType == BargraphType)
    {
        if (m_Attributes.m_NeedleSizeFlag) {
            SliderLen = (int)((double)(par_Height) * m_Attributes.m_NeedleSize / 100.0);
        } else {
            SliderLen = par_Height - UPPERFRAMESPACE;
        }
    }
    else
    {
        if (m_Attributes.m_NeedleSizeFlag) {
            SliderLen = (int)((double)(par_Height - par_Height/8) * m_Attributes.m_NeedleSize / 100.0);
        } else {
            SliderLen = par_Height - par_Height/8;
        }
    }

    // adapt scale
    double ValueRange = m_Attributes.m_UpperBound - m_Attributes.m_LowerBound;
    if (ValueRange <= 0.0) {
        pixel_per_line = pixel_per_unit = 1.0;
    } else {
        pixel_per_line = pixel_per_unit = SliderLen / ValueRange;
    }
    if (pixel_per_line <= 0.0) pixel_per_line = 1.0;

    if (m_Attributes.m_ScaleStepFlag) {
        pixel_per_line = (double)SliderLen * m_Attributes.m_ScaleStep / 100.0;
        //pixel_per_unit = (double)SliderLen / ValueRange;
    } else {
        x = 0;
        if (pixel_per_line > DEFAULT_SCALE_PIXEL_STEP) {
            while (pixel_per_line > DEFAULT_SCALE_PIXEL_STEP) {
                if ((x % 3 == 0) || (x % 3 == 2)) pixel_per_line /= 2.0;
                else if (x % 3 == 1) pixel_per_line /= 2.5;
                x++;
            }
        } else if (pixel_per_line < DEFAULT_SCALE_PIXEL_STEP) {
            while (pixel_per_line < DEFAULT_SCALE_PIXEL_STEP) {
                if ((x % 3 == 0) || (x % 3 == 2)) pixel_per_line *= 2.0;
                else if (x % 3 == 1) pixel_per_line *= 2.5;
                x++;
            }
        }
    }
    pixel_first_line = ceil (m_Attributes.m_LowerBound * pixel_per_unit / pixel_per_line) * pixel_per_line
                           - m_Attributes.m_LowerBound * pixel_per_unit;
    value_first_text = ceil (m_Attributes.m_LowerBound * pixel_per_unit / pixel_per_line) * pixel_per_line / pixel_per_unit;

    pixel_first_line -= pixel_per_line;
    value_first_text -= pixel_per_line / pixel_per_unit;
    pixel_per_line /= 10;  /* 10 interval */

    // paint the scale

    // set font size
    int FontSize = par_Width / 12 + 1;
    if (FontSize < 6) FontSize = 6;
    if (FontSize > (int)pixel_per_line*5) FontSize = (int)pixel_per_line*5;
    if (m_FontSizeDyn != FontSize) {
        m_ScaleFont = QFont(m_Attributes.m_Font.family(), FontSize);
        m_FontSizeDyn = FontSize;
    }
    Painter.setFont(m_ScaleFont);

    double savepixel_first_line = pixel_first_line;
    double savevalue_first_text = value_first_text;

    // first find the longest string and find the width of it
    x = 0;
    while (pixel_first_line < SliderLen) {
        if (pixel_first_line >= 0.0) {
            if (!(x%5)) { //every y you get a label
                char txt[64];
                if ((value_first_text > 1000000.0) || (value_first_text < -1000000.0)) {
                    sprintf (txt, "%e", value_first_text);
                } else if ((value_first_text < 0.0000001) && (value_first_text > -0.0000001)) {
                    sprintf (txt, "0");
                } else {
                    sprintf (txt, "%g", value_first_text);
                }
                QString ValueString(txt);
                QFontMetrics FontMetrics(m_ScaleFont);
                QRect Rect = FontMetrics.boundingRect(ValueString);
                int Width = Rect.width();
                if (Width > maxLabelSize)
                {
                    maxLabelSize = Width; //maximal width for Label
                }
            }
        }
        value_first_text += pixel_per_line / pixel_per_unit;
        pixel_first_line += pixel_per_line;
        x++;
    }
    maxLabelSize + 17;
    value_first_text = savevalue_first_text;
    pixel_first_line = savepixel_first_line;

    if (VariableNameLabel->isVisible())
    {
        StartSlider = par_Height - ((par_Height - SliderLen) / 2) + VariableNameLabel->height() + 5;
    }
    else
    {
        StartSlider = par_Height - ((par_Height - SliderLen) / 2);
    }
    EndSlider = StartSlider - SliderLen;

    if (m_Attributes.m_ScaleColorFlag) {
        SavePen = Painter.pen();
        Painter.setPen(m_Attributes.m_ScaleColor);
    }
    Painter.drawLine(QPoint(maxLabelSize + SPACE_BETWEEN_SCALETEXT, StartSlider), QPoint(maxLabelSize + SPACE_BETWEEN_SCALETEXT, EndSlider));

    x = 0;
    while (pixel_first_line < SliderLen) {
        if (pixel_first_line >= 0.0) {
            int  Pos =  StartSlider - (int)pixel_first_line;
            double Len = ((x%5)?6:(x%10)?10:14);
            double x1 = maxLabelSize + SPACE_BETWEEN_SCALETEXT - Len;
            double x2 = maxLabelSize + SPACE_BETWEEN_SCALETEXT;
            double y1 = Pos;
            double y2 = Pos;
            Painter.drawLine (x1, y1, x2, y2);
            if (!(x%5)) { //every y you get a label
                char txt[64];
                if ((value_first_text > 1000000.0) || (value_first_text < -1000000.0)) {
                    sprintf (txt, "%e", value_first_text);
                } else if ((value_first_text < 0.0000001) && (value_first_text > -0.0000001)) {
                        sprintf (txt, "0");
                } else {
                    sprintf (txt, "%g", value_first_text);
                }
                QString ValueString(txt);
                QFontMetrics FontMetrics(m_ScaleFont);
                QRect Rect = FontMetrics.boundingRect(ValueString);
                int Width = Rect.width();
                int Height =Rect.height();
                Qt::AlignmentFlag Alignment;
                Alignment = Qt::AlignLeft;
                Rect.setLeft(maxLabelSize - Width);// - Len);
                Rect.setTop(Pos - Height/2);
                Rect.setWidth(Width);
                Rect.setHeight(Height);
                Painter.drawText (Rect, Alignment, ValueString);
            }
        }
        value_first_text += pixel_per_line / pixel_per_unit;
        pixel_first_line += pixel_per_line;
        x++;
    }
    if (m_Attributes.m_ScaleColorFlag) {
        Painter.setPen(SavePen);
    }
}

void KnobOrTachoWidget::PaintKnob(QPainter &Painter, int par_Width, int par_Height, double par_Radius, int par_Offset)
{
    QBrush SaveBrush = Painter.brush();
    QPen SavePen = Painter.pen();

    if (m_Attributes.m_NeedleSizeFlag) {
        par_Radius = m_Attributes.m_NeedleSize * par_Radius / 100.0;
    } else {
        par_Radius = DEFAULT_KNOP_RADIUS_FACTOR * par_Radius;
    }
    if (m_Attributes.m_NeedleLineColorFlag) {
        Painter.setPen(m_Attributes.m_NeedleLineColor);
    }
    if (m_Attributes.m_NeedleFillColorFlag) {
        Painter.setBrush(m_Attributes.m_NeedleFillColor);
    } else {
        Painter.setBrush(QBrush(Qt::darkGray));
    }
    int Radius = (int)par_Radius;
    Painter.drawEllipse(QPoint(xm, ym), Radius, Radius);
    double Angle = (m_CurrentValue - m_Attributes.m_LowerBound) / (m_Attributes.m_UpperBound - m_Attributes.m_LowerBound) * (m_Attributes.m_MaxAngle - m_Attributes.m_MinAngle) + m_Attributes.m_MinAngle;
    double xp = sin((Angle / 360.0) * 2.0 * M_PI);
    double yp = cos((Angle / 360.0) * 2.0 * M_PI);
    double x1 = xp * (par_Radius * 0.2);
    double x2 = xp * (par_Radius * 0.8);
    double y1 = yp * (par_Radius * 0.2);
    double y2 = yp * (par_Radius * 0.8);
    QPen Pen;
    int Width = (int)(par_Radius * 0.1);
    if (Width <= 0) Width = 1;
    Pen.setWidth(Width);
    if (m_Attributes.m_NeedleLineColorFlag) {
        Pen.setColor(m_Attributes.m_NeedleLineColor);
    }
    Pen.setCapStyle(Qt::RoundCap);
    Painter.setPen(Pen);
    Painter.drawLine (x1 + xm, ym - y1, x2 + xm, ym - y2);
    Painter.setBrush(SaveBrush);
    Painter.setPen(SavePen);
}

void KnobOrTachoWidget::PaintNeedle(QPainter &Painter, int par_Width, int par_Height, double par_Radius, int par_Offset)
{
    QBrush SaveBrush = Painter.brush();
    QPen SavePen = Painter.pen();

    int rn = par_Radius;
    if (m_Attributes.m_NeedleSizeFlag) {
        rn = static_cast<int>(par_Radius * m_Attributes.m_NeedleSize / 100.0);
    }
    int rnm = static_cast<int>(par_Radius * 1.0 / 12.0);
    int rnn = static_cast<int>(par_Radius * 1.0 / 8.0);

    int rns = static_cast<int>(par_Radius / 20.0);

    double value;
    if (m_CurrentValue > m_Attributes.m_UpperBound) value = m_Attributes.m_UpperBound;
    else if (m_CurrentValue < m_Attributes.m_LowerBound) value = m_Attributes.m_LowerBound;
    else value = m_CurrentValue;

    double w = 90.0 + m_Attributes.m_MinAngle +
        ((value - m_Attributes.m_LowerBound) * (m_Attributes.m_MaxAngle - m_Attributes.m_MinAngle) / (m_Attributes.m_UpperBound - m_Attributes.m_LowerBound));

    w = w * ((2.0*M_PI) / 360.0);
    QPolygon Polygon;
    SaveBrush = Painter.brush();
    if (m_Attributes.m_NeedleFillColorFlag) {
        Painter.setBrush(QBrush(m_Attributes.m_NeedleFillColor));
    } else {
        Painter.setBrush(QBrush(Qt::red));         // if not user defined use red
    }
    SavePen = Painter.pen();
    if (m_Attributes.m_NeedleLineColorFlag) {
        Painter.setPen(m_Attributes.m_NeedleLineColor);
    } else {
        // if not user defined use default color
    }
    Polygon.append(QPoint(xm - static_cast<int>(cos (w) * rn), ym - static_cast<int>(sin (w) * rn)));
    Polygon.append(QPoint(xm + static_cast<int>(sin (w) * rnm), ym - static_cast<int>(cos (w) * rnm)));
    Polygon.append(QPoint(xm - static_cast<int>(sin (w) * rnm), ym + static_cast<int>(cos (w) * rnm)));

    Painter.drawEllipse(QPoint(xm,ym), rnn, rnn);  // red cycle with 1/8 radius of the needle length
    Painter.drawPolygon(Polygon);
    Painter.drawLine(QPoint(xm - static_cast<int>(cos (w) * rn), ym - static_cast<int>(sin (w) * rn)), QPoint(xm, ym));
    Painter.drawEllipse (QPoint(xm,ym), rnm, rnm); //  red cycle with 1/12 radius of the needle length

    //Painter.drawEllipse (QPoint(xm - static_cast<int>(cos (w) * rn), ym - static_cast<int>(sin (w) * rn)), rns, rns); //  red cycle with 1/20 radius of the needle length

    Painter.setBrush(SaveBrush);
    Painter.setPen(SavePen);
}

void KnobOrTachoWidget::PaintSliderKnob(QPainter &Painter, int par_Width, int par_Height, int par_Offset)
{
    QBrush SaveBrush = Painter.brush();
    QPen SavePen = Painter.pen();
    //QRect Rect = CalcSliderRect(par_Width, par_Height);
    double Value;
    if (m_CurrentValue > m_Attributes.m_UpperBound) Value = m_Attributes.m_UpperBound;
    else if (m_CurrentValue < m_Attributes.m_LowerBound) Value = m_Attributes.m_LowerBound;
    else Value = m_CurrentValue;

    //int posscale = (int)((double)par_Width * m_Attributes.m_ScalePosX / 100.0) + SPACE_BETWEEN_SCALE; // possition of scala
    int posscale = maxLabelSize + SPACE_BETWEEN_SCALETEXT + SPACE_BETWEEN_SCALE; // possition of scala

    int SliderLen = 0;
    int SliderWidth;
    int StartSlider;
    int EndSlider;
    if (m_Attributes.m_NeedleSizeFlag)
    {
        SliderLen = (int)((double)(par_Height - par_Height/8) * m_Attributes.m_NeedleSize / 100.0);
    }
    else
    {
        SliderLen = par_Height - par_Height/8;
    }

    if (m_Attributes.m_ScaleFlag)
    {
        SliderWidth = par_Width - posscale; // width of slider
    }
    else
    {
        SliderWidth = par_Width;
        posscale = 0;
    }


    //StartSlider = par_Height - ((par_Height -SliderLen) / 2) + (VariableNameLabel->height()*2);
    if (VariableNameLabel->isVisible())
    {
        StartSlider = (par_Height + VariableNameLabel->height() + 5) + ((SliderLen - par_Height)/2);
    }
    else
    {
        StartSlider = par_Height + ((SliderLen - par_Height)/2);
    }
    EndSlider = StartSlider - SliderLen;
    int CurrentSliderPos = StartSlider - static_cast<int>(SliderLen * (Value - m_Attributes.m_LowerBound) / (m_Attributes.m_UpperBound - m_Attributes.m_LowerBound));

    QRect Rect;
    Rect.setTop(CurrentSliderPos + par_Height/16);
    Rect.setBottom(CurrentSliderPos - par_Height/16);
    Rect.setLeft((par_Width+posscale)/2  - (SliderWidth/2));
    Rect.setRight((par_Width+posscale)/2 + (SliderWidth/2));

    SaveBrush = Painter.brush();
    if (m_Attributes.m_NeedleFillColorFlag) {
        Painter.setBrush(QBrush(m_Attributes.m_NeedleFillColor));
    } else {
        Painter.setBrush(Qt::lightGray);
    }
    SavePen = Painter.pen();
    if (m_Attributes.m_NeedleLineColorFlag) {
        Painter.setPen(m_Attributes.m_NeedleLineColor);
    } else {
        // if not user defined use default color
    }


    QPainterPath Path;
    //Path.addRoundedRect(Rect, SliderWidth/5, SliderWidth/5);
    Path.addRoundedRect(Rect, 5, 5);
    Painter.drawPath(Path);

    // draw inner
    Rect.setTop(CurrentSliderPos + SliderLen/22);
    Rect.setBottom(CurrentSliderPos - SliderLen/22);
    Rect.setLeft((par_Width+posscale)/2  - (SliderWidth/3));
    Rect.setRight((par_Width+posscale)/2 + (SliderWidth/3));

    Painter.setBrush(m_Attributes.m_Color); //Qt::black);
    Painter.drawRect(Rect);

    Painter.setBrush(SaveBrush);
    Painter.setPen(SavePen);
}

void KnobOrTachoWidget::PaintBargraph(QPainter &Painter, int par_Width, int par_Height, int par_Offset)
{
    QBrush SaveBrush = Painter.brush();
    QPen SavePen = Painter.pen();
    double Value;
    if (m_CurrentValue > m_Attributes.m_UpperBound) Value = m_Attributes.m_UpperBound;
    else if (m_CurrentValue < m_Attributes.m_LowerBound) Value = m_Attributes.m_LowerBound;
    else Value = m_CurrentValue;

    int BargraphLen;
    if (m_Attributes.m_NeedleSizeFlag) {
        BargraphLen = (int)((double)(par_Height - par_Height/8) * m_Attributes.m_NeedleSize / 100.0);
    } else {
        BargraphLen = par_Height - UPPERFRAMESPACE;// - par_Height/8;
    }
    //int BargraphStart = (par_Height - BargraphLen) / 2 + (VariableNameLabel->height()*2);
    int BargraphStart;
    if (VariableNameLabel->isVisible())
    {
        BargraphStart = par_Height - ((par_Height - BargraphLen) / 2) + VariableNameLabel->height() + 5;
    }
    else
    {
        BargraphStart = par_Height - ((par_Height - BargraphLen) / 2);
    }
    int BargraphEnd = BargraphStart - BargraphLen;
    int CurrentPos = BargraphStart - static_cast<int>(BargraphLen * (Value - m_Attributes.m_LowerBound) / (m_Attributes.m_UpperBound - m_Attributes.m_LowerBound));

    int LeftPos = maxLabelSize + SPACE_BETWEEN_SCALETEXT + SPACE_BETWEEN_SCALE; // possition of scalapar_Width/16;
    int RightPos = par_Width;// - par_Width/16;

    QRect Rect;
    Rect.setTop(CurrentPos);
    Rect.setBottom(BargraphEnd);
    Rect.setLeft(LeftPos);
    Rect.setRight(RightPos);
    Painter.setBrush(m_Attributes.m_Color);
    SavePen = Painter.pen();
    if (m_Attributes.m_NeedleLineColorFlag) {
        Painter.setPen(m_Attributes.m_NeedleLineColor);
    } else {
        // if not user defined use default color
    }
    Painter.drawRect(Rect);
    SaveBrush = Painter.brush();
    if (m_Attributes.m_NeedleFillColorFlag) {
        Painter.setBrush(QBrush(m_Attributes.m_NeedleFillColor));
    } else {
        Painter.setBrush(QBrush(Qt::darkGray));
    }

    Rect.setTop(BargraphStart);
    Rect.setBottom(CurrentPos);
    Painter.drawRect(Rect);

    Painter.setBrush(SaveBrush);
    Painter.setPen(SavePen);
}

void KnobOrTachoWidget::PaintSliderLine(QPainter &Painter, int par_Width, int par_Height, int par_Offset)
{
    QBrush SaveBrush = Painter.brush();
    int SliderLen;
    int SliderWidth;
    int SliderStart;
    int SliderEnd;

    //int posscale = (int)((double)par_Width * m_Attributes.m_ScalePosX / 100.0) + SPACE_BETWEEN_SCALE; // possition of scala
    int posscale = maxLabelSize + SPACE_BETWEEN_SCALETEXT + SPACE_BETWEEN_SCALE; // possition of scala

    if (m_Attributes.m_NeedleSizeFlag) {
        SliderLen = (int)((double)(par_Height - par_Height/8) * m_Attributes.m_NeedleSize / 100.0);
    } else {
        SliderLen = par_Height - par_Height/8;
    }
    if (m_Attributes.m_ScaleFlag)
    {
        SliderWidth = par_Width - posscale; // width of slider
    }
    else
    {
        SliderWidth = par_Width;
        posscale = 0;
    }

    if (VariableNameLabel->isVisible())
    {
        SliderStart = par_Height - ((par_Height - SliderLen) / 2) + VariableNameLabel->height() + 5;
    }
    else
    {
        SliderStart = par_Height - ((par_Height - SliderLen) / 2);
    }
    SliderEnd = SliderStart - SliderLen;
    QRect Rect;
    Rect.setTop(SliderStart);
    Rect.setBottom(SliderEnd);
    Rect.setLeft((par_Width+posscale)/2 + (SliderWidth/10));
    Rect.setRight((par_Width+posscale)/2 - (SliderWidth/10));

    // todo: Background color
    SaveBrush = Painter.brush();
    Painter.setBrush(QBrush(Qt::darkGray));
    Painter.drawRect(Rect);
    Painter.setBrush(SaveBrush);
}

static QRect BoundingRect(QPainter &par_Painter, double par_PosX, double par_PosY, QString &par_String,
                          int par_XDim, int par_YDim)
{
    QRect Rect;
    QFontMetrics FontMetrics = par_Painter.fontMetrics();
    QRect BoundingRect = FontMetrics.boundingRect(par_String);
    int Width2 = BoundingRect.width() / 2 + 1;
    int Height2 = BoundingRect.height() / 2 + 1;
    int x = (int)((double)par_XDim * par_PosX / 100.0);
    int y = (int)((double)par_YDim * par_PosY / 100.0);
    Rect.setTop(y - Height2);
    Rect.setBottom(y + Height2);
    Rect.setLeft(x - Width2);
    Rect.setRight(x + Width2);
    return Rect;
}

void KnobOrTachoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter Painter(this);
    Painter.setRenderHint(QPainter::Antialiasing);
    Painter.setRenderHint(QPainter::TextAntialiasing);
    Painter.setFont(m_Attributes.m_Font);

    QSize Size = Drawarea->size();
    int xdim = Size.width();
    int ydim = Size.height();
    int Offset = 0;
    double Radius = 1;

    if(m_Attributes.m_BitmapFlag && m_bitmapPathExist) {
        if (m_Attributes.m_BitmapWindowSizeBehaviour == ScaleBitMap) {
            Painter.drawPixmap(0, 0, m_PixmapResized);
        } else {
            Painter.drawPixmap(0, 0, m_PixmapOrginalSize);
        }
    }

    /* only for debuging
    Painter.drawText(0, 14, QString().number(m_LastXpos));
    Painter.drawText(0, 28, QString().number(m_LastYpos));
    Painter.drawText(0, 42, QString().number(m_DebugAngle));
    Painter.drawText(0, 56, QString().number(m_Overwind));*/

    if (m_Attributes.m_ScaleFlag) {
        maxLabelSize = 0;
        switch(m_KnobOrTachoType) {
        case KnobType:
        case TachoType:
            Radius = PaintRoundScale(Painter, xdim, ydim, Offset);
            break;
        case SliderType:
        case BargraphType:
            PaintLinearSlopeScale(Painter, xdim, ydim, Offset);
            break;
        }
    }
    else
    {
        maxLabelSize = 0;
    }

    if (m_Attributes.m_NeedleFlag) {
        switch(m_KnobOrTachoType) {
        case KnobType:
            PaintKnob(Painter, xdim, ydim, Radius, Offset);
            break;
        case TachoType:
            PaintNeedle(Painter, xdim, ydim, Radius, Offset);
            break;
        case SliderType:
            PaintSliderLine(Painter, xdim, ydim, Offset);
            PaintSliderKnob(Painter, xdim, ydim, Offset);
            break;
        case BargraphType:
            PaintBargraph(Painter, xdim, ydim, Offset);
            break;
        }
    }

    /* only for debuging
    Painter.drawPoint(m_MoveStartPoint);*/

    if (m_Attributes.m_NameFlag) {
        VariableNameLabel->setVisible(true);
        QFont Font;
        // Draw the signal name
        if (m_Attributes.m_NameFontFlag) {
            Font = m_Attributes.m_NameFont;
        } else {
            Font = m_Attributes.m_Font;
        }
        VariableNameLabel->setText(m_VariableName);
        VariableNameLabel->setFont(Font);
        switch (m_Attributes.m_NameAlign) {
        case 0:  // Center
        default:
            VariableNameLabel->setAlignment(Qt::AlignCenter);
            break;
        case 1:  // left
            VariableNameLabel->setAlignment(Qt::AlignLeft);
            break;
        case 2:  // right
            VariableNameLabel->setAlignment(Qt::AlignRight);
            break;
        }
    }
    else
    {
        VariableNameLabel->setVisible(false);
    }
    drawValue();
}

void KnobOrTachoWidget::drawValue()
{
    char txt[512];
    if (m_Attributes.m_TextFlag && (m_Vid > 0)) {
        // Draw the signal value
        ValueLabel->setVisible(true);
        double loc_value;
        if(m_Attributes.m_Physical && (m_ConversionType == BB_CONV_FORMULA)) {
            loc_value = read_bbvari_equ(m_Vid);
        } else {
            loc_value = read_bbvari_convert_double(m_Vid);
        }
        QFont Font;
        if (m_Attributes.m_TextFontFlag) {
            Font = m_Attributes.m_TextFont;
        } else {
            Font = m_Attributes.m_Font;
        }
        ValueLabel->setFont(Font);
        if ((m_ConversionType == BB_CONV_TEXTREP) && (m_Attributes.m_Physical)) {
            int color;
            convert_value_textreplace (m_Vid, static_cast<int32_t>(loc_value), txt, sizeof (txt), &color);
        } else {
            int DataType;
            if (m_Attributes.m_Physical && (((m_DataType >= BB_BYTE) && (m_DataType <= BB_DOUBLE)) ||
                                            ((m_DataType == BB_QWORD) && (m_DataType == BB_QWORD)))) {
                DataType = BB_DOUBLE;
            } else {
                DataType = m_DataType;
            }
            switch (DataType) {
            case BB_DOUBLE:
            case BB_FLOAT:
                sprintf (txt, "%*.*lf",
                        get_bbvari_format_width(m_Vid),
                        get_bbvari_format_prec(m_Vid),
                        //read_bbvari_convert_double(m_Vid));
                        loc_value);
                break;
            case BB_UBYTE:
            case BB_UWORD:
            case BB_UDWORD:
            case BB_UQWORD:
                sprintf (txt, "%" PRIu64 "", static_cast<uint64_t>(loc_value));
                //sprintf (txt, "%" PRIu64 "", static_cast<uint64_t>(read_bbvari_convert_double(m_Vid)));
                break;
            case BB_BYTE:
            case BB_WORD:
            case BB_DWORD:
            case BB_QWORD:
                sprintf (txt, "%" PRIi64 "", static_cast<int64_t>(loc_value));
                //sprintf (txt, "%" PRIi64 "", static_cast<int64_t>(read_bbvari_convert_double(m_Vid)));
                break;
            default:
                sprintf (txt, "unknown data type");
                break;
            }
        }
        strcat(txt, "  ");
        if (m_DisplayUnit) {
            get_bbvari_unit(m_Vid, txt+strlen(txt), static_cast<int>(sizeof(txt)-strlen(txt)));
        }
        QString ValueString(txt);
        ValueLabel->setText(ValueString);
        switch (m_Attributes.m_TextAlign) {
        case 0:  // Center
        default:
            ValueLabel->setAlignment(Qt::AlignCenter);
            break;
        case 1:  // left
            ValueLabel->setAlignment(Qt::AlignLeft);
            break;
        case 2:  // right
            ValueLabel->setAlignment(Qt::AlignRight);
            break;
        }
    }
    else
    {
        ValueLabel->setVisible(false);
    }
}

void KnobOrTachoWidget::WriteSliderValue(double arg_Value)
{
    if (m_Attributes.m_Physical && (get_bbvari_conversiontype(m_Vid) == BB_CONV_FORMULA)) {
        int help;
        help = write_bbvari_phys_minmax_check (m_Vid, arg_Value);
        if (help) {
            m_MoveFlag = false;
            ThrowError (1, "a nonlinear conversion-function can't be used with a knob switch to raw value");
            m_Attributes.m_Physical = 0;
        }
    } else {
        write_bbvari_minmax_check (m_Vid, arg_Value);
    }
}


KnobOrTachoWidget::KnobAttributes::KnobAttributes()
{
    m_Physical = false;
    m_BBCfgDirectFlag = false;
    m_LowerBound = 0.0;
    m_UpperBound = 1.0;
    // Scale
    m_ScaleFlag = false;
    m_MinAngle = -160.0;
    m_MaxAngle = 160.0;
    m_ScaleStepFlag = false;
    m_ScaleStep = 30.0;
    m_ScalePosX = 50.0;
    m_ScalePosY = 50.0;
    m_ScaleSizeFlag = false;
    m_ScaleSize = 95.0;
    m_ScaleColorFlag = false;

    // Knob
    m_NeedleFlag = false;
    m_NeedlePosX = 50.0;
    m_NeedlePosY = 50.0;
    m_NeedleSizeFlag = false;
    m_NeedleSize = 95.0;
    m_NeedleLineColorFlag = false;
    m_NeedleFillColorFlag = false;

    // Value
    m_TextFlag = false;
    m_TextPosX = 50.0;
    m_TextPosY = 96.0;
    m_TextFontFlag = false;
    m_TextAlign = 0;  // Center

    // Name
    m_NameFlag = false;
    m_NamePosX = 50.0;
    m_NamePosY = 4.0;
    m_NameFontFlag = false;
    m_NameAlign = 0;  // Center

    // Bitmap
    bool m_BitmapFlag = false;
    m_BitmapWindowSizeBehaviour = ScaleBitMap;

    // If release fallback to
    m_IfReleaseFallbackFlag = false;
    m_IfReleaseFallbackValue = 0.0;

    // Drag status
    m_DragStateVarFlag = false;
}

static char *ColorToString(QColor par_Color, char *ret_String)
{
    sprintf (ret_String, "(%i,%i,%i)", par_Color.red(), par_Color.green(), par_Color.blue());
    return ret_String;
}

static bool StringToColor(char *par_String, QColor *ret_Color)
{
    char *p = par_String;
    if (*p == '(') {
        int r, g, b;
        r = strtoul(p+1, &p, 0);
        if (*p == ',') {
            g = strtoul(p+1, &p, 0);
            if (*p == ',') {
                b = strtoul(p+1, &p, 0);
                if (*p == ')') {
                    *ret_Color = QColor(r, g, b);
                    return true;
                }
            }
        }
    }
    *ret_Color = QColor(Qt::black);
    return false;
}

void KnobOrTachoWidget::KnobAttributes::writeToIni(QString &par_SectionPath, KnobOrTachoType par_KnobOrTachoType)
{
    char ColorString[64];
    int Fd = GetMainFileDescriptor();
    // Basics
    // phys, color, ... will be save inside the widget class oneself
   ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "BBCfgDirectFlag", m_BBCfgDirectFlag, Fd);
   ScQt_IniFileDataBaseWriteInt(par_SectionPath, "FontSize", m_Font.pointSize(), Fd);
   ScQt_IniFileDataBaseWriteString(par_SectionPath, "FontName",  QStringToConstChar(m_Font.family()), Fd);
    // Scale
   ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "ScaleFlag", m_ScaleFlag, Fd);
    if (m_ScaleFlag) {
       ScQt_IniFileDataBaseWriteFloat(par_SectionPath, "MinAngle", m_MinAngle, Fd);
       ScQt_IniFileDataBaseWriteFloat(par_SectionPath, "MaxAngle", m_MaxAngle, Fd);
       ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "ScaleStepFlag", m_ScaleStepFlag, Fd);
       ScQt_IniFileDataBaseWriteFloat(par_SectionPath, "ScaleStep", m_ScaleStep, Fd);
       ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "ScaleSizeFlag", m_ScaleSizeFlag, Fd);
       ScQt_IniFileDataBaseWriteFloat(par_SectionPath, "ScaleSize", m_ScaleSize, Fd);
       ScQt_IniFileDataBaseWriteFloat(par_SectionPath, "ScalePosX", m_ScalePosX, Fd);
       ScQt_IniFileDataBaseWriteFloat(par_SectionPath, "ScalePosY", m_ScalePosY, Fd);
       ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "ScaleColorFlag", m_ScaleColorFlag, Fd);
       ScQt_IniFileDataBaseWriteString(par_SectionPath, "ScaleColor", ColorToString(m_ScaleColor, ColorString), Fd);
    }
    // Knob or Needle
    //if (par_KnobOrTachoType != BargraphType) {   // Bargraph has no needle/knob
       ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "NeedleFlag", m_NeedleFlag, Fd);
        if (m_NeedleFlag) {
           ScQt_IniFileDataBaseWriteInt(par_SectionPath, "NeedlePosX", m_NeedlePosX, Fd);
           ScQt_IniFileDataBaseWriteInt(par_SectionPath, "NeedlePosY", m_NeedlePosY, Fd);
           ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "NeedleSizeFlag", m_NeedleSizeFlag, Fd);
           ScQt_IniFileDataBaseWriteInt(par_SectionPath, "NeedleSize", m_NeedleSize, Fd);
           ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "NeedleLineColorFlag", m_NeedleLineColorFlag, Fd);
           ScQt_IniFileDataBaseWriteString(par_SectionPath, "NeedleLineColor", ColorToString(m_NeedleLineColor, ColorString), Fd);
           ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "NeedleFillColorFlag", m_NeedleFillColorFlag, Fd);
           ScQt_IniFileDataBaseWriteString(par_SectionPath, "NeedleFillColor", ColorToString(m_NeedleFillColor, ColorString), Fd);
        }
    //}
    // Value
   ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "TextFlag", m_TextFlag, Fd);
    if (m_TextFlag) {
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "TextPosX", m_TextPosX, Fd);
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "TextPosY", m_TextPosY, Fd);
       ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "TextFontFlag", m_TextFontFlag, Fd);
       ScQt_IniFileDataBaseWriteString(par_SectionPath, "TextFont", QStringToConstChar(m_TextFont.family()), Fd);
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "TextFontSize", m_TextFont.pointSize(), Fd);
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "TextAlign", m_TextAlign, Fd);
    }
    // Name
   ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "NameFlag", m_NameFlag, Fd);
    if (m_NameFlag) {
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "NamePosX", m_NamePosX, Fd);
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "NamePosY", m_NamePosY, Fd);
       ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "NameFontFlag", m_NameFontFlag, Fd);
       ScQt_IniFileDataBaseWriteString(par_SectionPath, "NameFont", QStringToConstChar(m_NameFont.family()), Fd);
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "NameFontSize", m_NameFont.pointSize(), Fd);
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "NameAlign", m_NameAlign, Fd);
    }
    // Bitmap
   ScQt_IniFileDataBaseWriteYesNo(par_SectionPath, "BitmapFlag", m_BitmapFlag, Fd);
    if (m_BitmapFlag) {
       ScQt_IniFileDataBaseWriteString(par_SectionPath, "Bitmap", QStringToConstChar(m_bitmap), Fd);
       ScQt_IniFileDataBaseWriteInt(par_SectionPath, "BitmapScalingOrFixedSize", m_BitmapWindowSizeBehaviour, Fd);
    }
    if ((par_KnobOrTachoType == SliderType) ||
        (par_KnobOrTachoType == KnobType)) {
        char Buffer[2*BBVARI_NAME_SIZE];
        // if release fallback to
        sprintf (Buffer, "%i, %g", m_IfReleaseFallbackFlag, m_IfReleaseFallbackValue);
       ScQt_IniFileDataBaseWriteString(par_SectionPath, "IfReleaseFallback", Buffer, Fd);
        // Drag status
        sprintf (Buffer, "%i, %s", m_DragStateVarFlag, QStringToConstChar(m_DragStateVariableName));
       ScQt_IniFileDataBaseWriteString(par_SectionPath, "DragStatusFlag", Buffer, Fd);
    }
}

void KnobOrTachoWidget::KnobAttributes::readFromIni(QString &par_SectionPath, enum KnobOrTachoType par_KnobOrTachoType)
{
    char Buffer[INI_MAX_LINE_LENGTH];
    double DefaultLen;
    double DefaultPosX;
    int Fd = GetMainFileDescriptor();

    if ((par_KnobOrTachoType == SliderType) ||
        (par_KnobOrTachoType == BargraphType)) {
        DefaultLen = 85.0;
        DefaultPosX = 40.0;
    } else {
        DefaultLen = 95.0;
        DefaultPosX = 50.0;
    }

    // Basics
    m_BBCfgDirectFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "BBCfgDirectFlag", 0, Fd);
    int FontSize =ScQt_IniFileDataBaseReadInt(par_SectionPath, "FontSize", 10, Fd);
    if (ScQt_IniFileDataBaseReadString(par_SectionPath, "FontName", "", Buffer, sizeof (Buffer), Fd) > 0) {
        m_Font = QFont(QString(Buffer), FontSize);
    }
    // Scale
    m_ScaleFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "ScaleFlag", 1, Fd);
    m_MinAngle =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "MinAngle", -160.0, Fd);
    m_MaxAngle =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "MaxAngle", 160.0, Fd);
    m_ScaleStepFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "ScaleStepFlag", 0, Fd);
    m_ScaleStep =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "ScaleStep", 30.0, Fd);
    m_ScaleSizeFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "ScaleSizeFlag", 0, Fd);
    m_ScaleSize =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "ScaleSize", DefaultLen, Fd);
    m_ScalePosX =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "ScalePosX", DefaultPosX, Fd);
    m_ScalePosY =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "ScalePosY", 50.0, Fd);
    m_ScaleColorFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "ScaleColorFlag", 0, Fd);
    if (m_ScaleColorFlag) {
       ScQt_IniFileDataBaseReadString(par_SectionPath, "ScaleColor", "", Buffer, sizeof(Buffer), Fd);
        if (!StringToColor(Buffer, &m_ScaleColor)) {
            m_ScaleColorFlag = false;
        }
    }
    // Knob or Needle
    m_NeedleFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "NeedleFlag", 1, Fd);
    m_NeedlePosX =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "NeedlePosX", 50, Fd);
    m_NeedlePosY =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "NeedlePosY", 50, Fd);
    m_NeedleSizeFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "NeedleSizeFlag", 0, Fd);
    m_NeedleSize =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "NeedleSize", DefaultLen, Fd);
    m_NeedleLineColorFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "NeedleLineColorFlag", 0, Fd);
    if (m_NeedleLineColorFlag) {
       ScQt_IniFileDataBaseReadString(par_SectionPath, "NeedleLineColor", "", Buffer, sizeof(Buffer), Fd);
        if (!StringToColor(Buffer, &m_NeedleLineColor)) {
            m_NeedleLineColorFlag = false;
        }
    }
    m_NeedleFillColorFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "NeedleFillColorFlag", 0, Fd);
    if (m_NeedleFillColorFlag) {
       ScQt_IniFileDataBaseReadString(par_SectionPath, "NeedleFillColor", "", Buffer, sizeof(Buffer), Fd);
        if (!StringToColor(Buffer, &m_NeedleFillColor)) {
            m_NeedleFillColorFlag = false;
        }
    }
    // Value
    m_TextFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "TextFlag", 1, Fd);
    m_TextPosX =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "TextPosX", 50, Fd);
    m_TextPosY =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "TextPosY", 96, Fd);
    m_TextFontFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "TextFontFlag", 0, Fd);
    FontSize =ScQt_IniFileDataBaseReadInt(par_SectionPath, "TextFontSize", 10, Fd);
    if (ScQt_IniFileDataBaseReadString(par_SectionPath, "TextFont", "", Buffer, sizeof (Buffer), Fd) > 0) {
        m_TextFont = QFont(QString(Buffer), FontSize);
    }
    m_TextAlign =ScQt_IniFileDataBaseReadInt(par_SectionPath, "TextAlign", 0, Fd);
    // Name
    m_NameFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "NameFlag", 1, Fd);
    m_NamePosX =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "NamePosX", 50, Fd);
    m_NamePosY =ScQt_IniFileDataBaseReadFloat(par_SectionPath, "NamePosY", 3, Fd);
    m_NameFontFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "NameFontFlag", 0, Fd);
    FontSize =ScQt_IniFileDataBaseReadInt(par_SectionPath, "NameFontSize", 10, Fd);
    if (ScQt_IniFileDataBaseReadString(par_SectionPath, "NameFont", "", Buffer, sizeof (Buffer), Fd) > 0) {
        m_NameFont = QFont(QString(Buffer), FontSize);
    }
    m_NameAlign =ScQt_IniFileDataBaseReadInt(par_SectionPath, "NameAlign", 0, Fd);
    // Bitmap
    m_BitmapFlag =ScQt_IniFileDataBaseReadYesNo(par_SectionPath, "BitmapFlag", 0, Fd);
   ScQt_IniFileDataBaseReadString(par_SectionPath, "Bitmap", "", Buffer, sizeof(Buffer), Fd);
    m_bitmap = QString::fromLocal8Bit(Buffer);
    m_BitmapWindowSizeBehaviour = static_cast<BitmapWindowSizeBehaviour>(ScQt_IniFileDataBaseReadInt(par_SectionPath, "BitmapScalingOrFixedSize", 0, Fd));

    if ((par_KnobOrTachoType == SliderType) ||
        (par_KnobOrTachoType == KnobType)) {
        // if release fallback to
       ScQt_IniFileDataBaseReadString(par_SectionPath, "IfReleaseFallback", "0, 0.00000", Buffer, sizeof(Buffer), Fd);
        char *a, *b;
        if (StringCommaSeparate (Buffer, &a, &b, nullptr) == 2) {
            if (atol (a)) {
                m_IfReleaseFallbackFlag = true;
                m_IfReleaseFallbackValue = atof (b);
            } else {
                m_IfReleaseFallbackFlag = false;
                m_IfReleaseFallbackValue = 0.0;
            }
        } else {
            m_IfReleaseFallbackFlag = false;
            m_IfReleaseFallbackValue = 0.0;
        }
        // Drag status
        m_DragStateVarFlag = false;
       ScQt_IniFileDataBaseReadString(par_SectionPath, "DragStatusFlag", "0,", Buffer, sizeof (Buffer), Fd);
        if (StringCommaSeparate (Buffer, &a, &b, nullptr) == 2) {
            if (atol (a)) m_DragStateVarFlag = true;
            if (m_DragStateVarFlag) {
                m_DragStateVariableName = QString(b);
            }
        }
    } else {
        m_IfReleaseFallbackFlag = false;
        m_IfReleaseFallbackValue = 0.0;
        m_DragStateVarFlag = false;
        m_DragStateVariableName.clear();
    }
}

QString KnobOrTachoWidget::SearchAndReplaceEnvironmentVariables(QString &par_String)
{
    char DstString[2*MAX_PATH];
    SearchAndReplaceEnvironmentStrings(QStringToConstChar(par_String), DstString, MAX_PATH);
    return QString(DstString);
}

QRect KnobOrTachoWidget::CalcSliderRect(int par_Width, int par_Height)
{
    double Value;
    if (m_CurrentValue > m_Attributes.m_UpperBound) Value = m_Attributes.m_UpperBound;
    else if (m_CurrentValue < m_Attributes.m_LowerBound) Value = m_Attributes.m_LowerBound;
    else Value = m_CurrentValue;

    int SliderLen;
    int SliderWidth;
    int StartSlider;
    int EndSlider;
    if (m_Attributes.m_NeedleSizeFlag) {
        SliderLen = (int)((double)par_Height * m_Attributes.m_NeedleSize / 100.0);
        SliderWidth = (int)((double)par_Width * m_Attributes.m_NeedleSize / 800.0);
    } else {
        SliderLen = (int)((double)par_Height * DEFAULT_SLIDER_LEGTH_FACTOR);
        SliderWidth = (int)((double)par_Width * DEFAULT_SLIDER_LEGTH_FACTOR / 8.0);
    }
    int CurrentSliderPos = SliderLen - static_cast<int>(SliderLen * (Value - m_Attributes.m_LowerBound) / (m_Attributes.m_UpperBound - m_Attributes.m_LowerBound));

    StartSlider = (par_Height - SliderLen) / 2;
    EndSlider = StartSlider + SliderLen;
    QRect Rect;
    Rect.setTop(StartSlider + CurrentSliderPos - par_Width/8);
    Rect.setBottom(Rect.top() + par_Width/4);
    Rect.setLeft(par_Width/2 - par_Width*2/5);
    Rect.setRight(par_Width/2 + par_Width*2/5);
    return Rect;
}

int KnobOrTachoWidget::CalcSliderLen(int par_Height)
{
    int Len;
    if (m_Attributes.m_NeedleSizeFlag) {
        Len = (int)((double)par_Height * m_Attributes.m_NeedleSize / 100.0);
    } else {
        Len = (int)((double)par_Height * DEFAULT_SLIDER_LEGTH_FACTOR);
    }
    return Len;
}

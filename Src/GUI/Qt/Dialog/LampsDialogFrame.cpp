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


#include "LampsDialogFrame.h"
#include "ui_LampsDialogFrame.h"
#include <QMessageBox>
#include <QBrush>
#include "DefaultElementDialog.h"
#include "StringHelpers.h"

CustomLampsDialogFrame::CustomLampsDialogFrame(CustomLampsGraphicsView *arg_graphicsView, QWidget *arg_parent) : CustomDialogFrame(arg_parent), ui(new Ui::CustomLampsDialogFrame)
{
    ui->setupUi(this);

    m_currentView = arg_graphicsView;
    m_changeStancilFlag = false;
    m_lampModel = new ControlLampModel(this);
    ui->d_listViewStancil->setModel(m_lampModel);
    int loc_row, loc_column, loc_rowDimension, loc_columnDimension;
    if (arg_graphicsView != nullptr) {
        loc_row = arg_graphicsView->getFromRow();
        loc_column = arg_graphicsView->getFromColumn();
        loc_rowDimension = arg_graphicsView->getRowSpan();
        loc_columnDimension = arg_graphicsView->getColumnSpan();
        ui->d_widgetViewStancil->updateBackgroundColor(arg_graphicsView->backgroundColor());
        ui->d_widgetViewStancilInput->updateBackgroundColor(arg_graphicsView->backgroundColor());
        m_currentVariable = arg_graphicsView->variableName();
        m_currentStancil = arg_graphicsView->stancilName();
    } else {
        loc_row = 0;
        loc_column = 0;
        loc_rowDimension = 1;
        loc_columnDimension = 1;
        ui->d_widgetViewStancil->updateBackgroundColor(Qt::black);
        ui->d_widgetViewStancilInput->updateBackgroundColor(Qt::black);
    }
    m_currentFromRow = loc_row;
    m_currentFromColumn = loc_column;
    m_currentRowSpan = loc_rowDimension;
    m_currentColumnSpan = loc_columnDimension;

    //connect(ui->d_comboBoxSelectLamp, SIGNAL(currentIndexChanged(int)), this, SLOT(showCoordinatesOfVariable(int)));
    connect(ui->d_listViewStancil, SIGNAL(clicked(QModelIndex)), this, SLOT(changeCurrentLamp(QModelIndex)));
    connect(ui->d_listViewStancil, SIGNAL(activated(QModelIndex)), this, SLOT(changeCurrentLamp(QModelIndex)));
    connect(ui->d_listViewStancil->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(changeCurrentLamp(QModelIndex)));
    connect(ui->d_pushButtonAddStancil, SIGNAL(clicked()), this, SLOT(stancilListAdd()));
    connect(ui->d_pushButtonDeleteStancil, SIGNAL(clicked()), this, SLOT(stancilListDelete()));
    connect(ui->d_pushButtonChangeStancil, SIGNAL(clicked()), this, SLOT(stancilListChange()));
    connect(ui->d_pushButtonStancilOk, SIGNAL(clicked()), this, SLOT(stancilInputOk()));
    connect(ui->d_pushButtonStancilCancel, SIGNAL(clicked()), this, SLOT(stancilInputCancel()));
    connect(ui->d_plainTextEditStancil, SIGNAL(textChanged()), this, SLOT(stancilInputPreview()));
    DefaultElementDialog *loc_dlg = qobject_cast<DefaultElementDialog*>(arg_parent);
    if(loc_dlg) {
        if(arg_graphicsView != nullptr) {
            loc_dlg->setCurrentColor(arg_graphicsView->backgroundColor());
        }else {
            loc_dlg->setCurrentColor(Qt::black);
        }
        connect(loc_dlg->getDefaultFrame(), SIGNAL(colorChanged(QColor)), this, SLOT(changeBackgroundColor(QColor)));
        connect(loc_dlg->getDefaultFrame(), SIGNAL(variableSelectionChanged(QString,bool)), this, SLOT(changeLampVaraible(QString,bool)));
        connect(this, SIGNAL(variableSelectionChanged(QString,Qt::CheckState)), loc_dlg->getDefaultFrame(), SLOT(changeSelectedVariable(QString,Qt::CheckState)));
    }
    if(arg_graphicsView != nullptr) {
        showStancil(m_lampModel->getItemIndex(arg_graphicsView->stancilName()));
    }
    ui->d_spinBoxRowPosition->setValue(loc_row);
    ui->d_spinBoxRowDimension->setValue(loc_rowDimension);
    ui->d_spinBoxColumnPosition->setValue(loc_column);
    ui->d_spinBoxColumnDimension->setValue(loc_columnDimension);
    connect(ui->d_spinBoxRowPosition, SIGNAL(valueChanged(int)), this, SLOT(RowPosition(int)));
    connect(ui->d_spinBoxRowDimension, SIGNAL(valueChanged(int)), this, SLOT(RowDimension(int)));
    connect(ui->d_spinBoxColumnPosition, SIGNAL(valueChanged(int)), this, SLOT(ColumnPosition(int)));
    connect(ui->d_spinBoxColumnDimension, SIGNAL(valueChanged(int)), this, SLOT(ColumnDimension(int)));

    if (m_currentView->shouldBeRemovedByCancel()) {  // wenn neu angelegt dann die Position/Groesse setzen
        m_currentView->setPositionAndSize(loc_row, loc_column, loc_rowDimension, loc_columnDimension);
    }
}

CustomLampsDialogFrame::~CustomLampsDialogFrame()
{
    delete ui;
}

void CustomLampsDialogFrame::setPositionColumn(int arg_column)
{
    ui->d_spinBoxColumnPosition->setValue(arg_column);
}

void CustomLampsDialogFrame::setPositionRow(int arg_row)
{
    ui->d_spinBoxRowPosition->setValue(arg_row);
}

void CustomLampsDialogFrame::setDimensionColumn(int arg_column)
{
    ui->d_spinBoxColumnDimension->setValue(arg_column);
}

void CustomLampsDialogFrame::setDimensionRow(int arg_row)
{
    ui->d_spinBoxRowDimension->setValue(arg_row);
}

void CustomLampsDialogFrame::userAccept()
{
    m_currentView->resetShouldBeRemovedByCancel();
}

void CustomLampsDialogFrame::userReject()
{
    RowPosition(m_currentFromRow);
    RowDimension(m_currentRowSpan);
    ColumnPosition(m_currentFromColumn);
    ColumnDimension(m_currentColumnSpan);
    changeLampVaraible(m_currentVariable, true);
    m_currentView->setStancilName(m_currentStancil);
    if (m_currentView != nullptr) {
        if (m_currentView->shouldBeRemovedByCancel()) {
            // set size and position to adjust layout back
            m_currentView->setPositionAndSize(0, 0, 1, 1);
            emit cancelNewLamp(m_currentView);
        }
    }
}

void CustomLampsDialogFrame::showStancil(QModelIndex index)
{
    if(!index.isValid()) {
        return;
    }
    ui->d_widgetViewStancil->clearGraphicsItemGroup();
    ui->d_widgetViewStancil->setGraphicsItemGroup(m_lampModel->getForegroundStancil(index), m_lampModel->getBackgroundStancil(index));
    ui->d_widgetViewStancil->updateBackgroundColor(m_currentView->backgroundColor());
    ui->d_listViewStancil->setCurrentIndex(index);
}

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    #define SKIP_EMPTY_PARTS QString::SkipEmptyParts
#else
    #define SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#endif

void CustomLampsDialogFrame::stancilInputOk()
{
    QString loc_polygonText = ui->d_plainTextEditStancil->toPlainText();
    QString loc_stancilName = ui->d_lineEditStancilName->text();
    loc_polygonText = loc_polygonText.simplified();
    loc_polygonText.remove(" ");
    loc_stancilName = loc_stancilName.simplified();
    if(m_changeStancilFlag) {
        stancilListDelete();
        m_changeStancilFlag = false;
    }
    if(!loc_stancilName.isEmpty()) {
        if(!m_lampModel->nameExists(loc_stancilName)) {
            if(!loc_polygonText.isEmpty()) {
                if(loc_polygonText.count("(") == loc_polygonText.count(")")) {
                    QStringList loc_polygons = loc_polygonText.split(")", SKIP_EMPTY_PARTS);
                    if(!loc_polygons.isEmpty()) {
                        QList<QPolygonF> loc_foregroundPolygon;
                        QList<QPolygonF> loc_backgroundPolygon;
                        bool loc_isForeground = true;
                        foreach(QString loc_polygon, loc_polygons) {
                            if(loc_polygon.startsWith("(+")) {
                                loc_isForeground = true;
                                loc_polygon.remove(0, 2);
                            } else {
                                if(loc_polygon.startsWith("(-")) {
                                    loc_isForeground = false;
                                    loc_polygon.remove(0, 2);
                                } else {
                                    QMessageBox::information(this, tr("Syntax error"), tr("Polygon not as foreground (+) or background (-) defined! \n (+0.1/0.1;0.9/0.5;0.1/0.9;) (-0.4/0.4;0.6/0.5;0.4/0.6;)"));
                                    break;
                                }
                            }
                            QStringList loc_points = loc_polygon.split(";", SKIP_EMPTY_PARTS);
                            if(!loc_points.isEmpty()) {
                                QPolygonF loc_polygonValues;
                                foreach(QString loc_point, loc_points) {
                                    QStringList loc_values = loc_point.split("/");
                                    if(loc_values.count() == 2) {
                                        bool loc_firstDouble = false;
                                        bool loc_secondDouble = false;
                                        double loc_first = loc_values.first().toDouble(&loc_firstDouble);
                                        double loc_second = loc_values.last().toDouble(&loc_secondDouble);
                                        if(loc_firstDouble && loc_secondDouble) {
                                            loc_polygonValues.append(QPointF(loc_first, loc_second));
                                        }
                                    }
                                }
                                if(!loc_polygonValues.isEmpty()) {
                                    if(loc_isForeground) {
                                        loc_foregroundPolygon.append(loc_polygonValues);
                                    } else {
                                        loc_backgroundPolygon.append(loc_polygonValues);
                                    }
                                }
                            }
                        }

                        m_lampModel->addStancil(ui->d_lineEditStancilName->text(),
                                                loc_polygonText,
                                                loc_foregroundPolygon, loc_backgroundPolygon);
                        // falls einer mit diesm Namen exisitiert erst mal loeschen
                        m_lampModel->removeStancilFromIni(ui->d_lineEditStancilName->text());
                        m_lampModel->addStancilToIni(ui->d_lineEditStancilName->text(), loc_polygonText);

                        ui->d_stackedWidgetStancil->setCurrentIndex(0);
                        ui->d_plainTextEditStancil->clear();
                        ui->d_lineEditStancilName->clear();
                        ui->d_widgetViewStancilInput->clearGraphicsItemGroup();
                    } else {
                        QMessageBox::information(this, tr("Syntax error"), tr("No polygons defined! \n (+0.1/0.1;0.9/0.5;0.1/0.9;) (-0.4/0.4;0.6/0.5;0.4/0.6;)"));
                    }
                } else {
                    QMessageBox::information(this, tr("Syntax error"), tr("The number of \"(\" and \")\" is not the same!"));
                }
            } else {
                QMessageBox::information(this, tr("Syntax error"), tr("No stancil defined! \n (+0.1/0.1;0.9/0.5;0.1/0.9;) (-0.4/0.4;0.6/0.5;0.4/0.6;)"));
            }
        } else {
            QMessageBox::information(this, tr("Syntax error"), tr("Stancil name already exists!"));
        }
    } else {
        QMessageBox::information(this, tr("Syntax error"), tr("No stancil name defined!"));
    }
}

void CustomLampsDialogFrame::stancilInputCancel()
{
    ui->d_plainTextEditStancil->clear();
    ui->d_lineEditStancilName->clear();
    ui->d_widgetViewStancilInput->clearGraphicsItemGroup();
    ui->d_stackedWidgetStancil->setCurrentIndex(0);
}

void CustomLampsDialogFrame::stancilListAdd()
{
    ui->d_stackedWidgetStancil->setCurrentIndex(1);
}

void CustomLampsDialogFrame::stancilListDelete()
{
    QModelIndex loc_index = ui->d_listViewStancil->selectionModel()->currentIndex();
    if(loc_index.row() > m_lampModel->GetPredefinedCount()) {
        m_lampModel->removeStancilFromIni(m_lampModel->data(loc_index, Qt::DisplayRole).toString());
        m_lampModel->removeStancil(loc_index);
    }
}

void CustomLampsDialogFrame::stancilListChange()
{
    QModelIndex loc_index = ui->d_listViewStancil->selectionModel()->currentIndex();
    if(loc_index.row() > m_lampModel->GetPredefinedCount()) {
        m_changeStancilFlag = true;
    }
    Lamp *loc_lamp = m_lampModel->getStancilInformation(loc_index.row());
    if (loc_lamp != nullptr) {
        ui->d_lineEditStancilName->setText(loc_lamp->m_name);
        ui->d_plainTextEditStancil->setPlainText(loc_lamp->m_polygon); //loc_coordinates);
        ui->d_stackedWidgetStancil->setCurrentIndex(1);
    }
}

void CustomLampsDialogFrame::stancilInputPreview()
{
    Lamp loc_lamp;
    QString input;
    input = ui->d_lineEditStancilName->text();
    input.append(",");
    input.append(ui->d_plainTextEditStancil->toPlainText());
    if (loc_lamp.init (input).isEmpty()) {
        ui->d_widgetViewStancilInput->setGraphicsItemGroup(&loc_lamp.m_polygonForeground, &loc_lamp.m_polygonBackground);
    } else {
        ui->d_widgetViewStancilInput->clearGraphicsItemGroup();
    }
}

void CustomLampsDialogFrame::changeBackgroundColor(QColor arg_color)
{
    ui->d_widgetViewStancil->updateBackgroundColor(arg_color);
}

void CustomLampsDialogFrame::changeLampVaraible(QString arg_variable, bool arg_checked)
{
    if(arg_checked) {
        QVariant loc_Variant(static_cast<int>(get_bbvarivid_by_name(QStringToConstChar(arg_variable))));
        if(loc_Variant.toInt() != 0) {
            if(m_currentView) {
                m_currentView->setProperty("VID", loc_Variant);
                m_currentView->setVarialeName(arg_variable);
            }
        }
    } else {
        if(m_currentView) {
            m_currentView->setProperty("VID", 0);
            m_currentView->setVarialeName(QString());
        }
    }
}

void CustomLampsDialogFrame::changeCurrentLamp(QModelIndex arg_index)
{
    Q_UNUSED(arg_index);
    m_currentView->setStancilName(m_lampModel->stancilName(ui->d_listViewStancil->currentIndex()));
    QList<QPolygonF>* loc_foreground = m_lampModel->getForegroundStancil(ui->d_listViewStancil->currentIndex());
    QList<QPolygonF>* loc_background = m_lampModel->getBackgroundStancil(ui->d_listViewStancil->currentIndex());
    m_currentView->setGraphicsItemGroup(loc_foreground, loc_background);
    ui->d_widgetViewStancil->setGraphicsItemGroup(loc_foreground, loc_background);
}

void CustomLampsDialogFrame::RowPosition(int arg_rowPosition)
{
    m_currentView->setPositionAndSize(arg_rowPosition, m_currentView->getFromColumn(), m_currentView->getRowSpan(), m_currentView->getColumnSpan());
}

void CustomLampsDialogFrame::RowDimension(int arg_rowDimension)
{
    m_currentView->setPositionAndSize(m_currentView->getFromRow(), m_currentView->getFromColumn(), arg_rowDimension, m_currentView->getColumnSpan());
}

void CustomLampsDialogFrame::ColumnPosition(int arg_ColumnPosition)
{
    m_currentView->setPositionAndSize(m_currentView->getFromRow(), arg_ColumnPosition, m_currentView->getRowSpan(), m_currentView->getColumnSpan());
}

void CustomLampsDialogFrame::ColumnDimension(int arg_ColumnPosition)
{
    m_currentView->setPositionAndSize(m_currentView->getFromRow(), m_currentView->getFromColumn(), m_currentView->getRowSpan(), arg_ColumnPosition);
}



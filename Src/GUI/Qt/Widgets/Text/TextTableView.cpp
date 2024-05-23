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


#include "TextTableView.h"
#include <QMouseEvent>
#include <QHeaderView>
//#include <QDebug>
#include "TextWidget.h"
#include "BlackboardInfoDialog.h"
#include "TextTableViewDelegate.h"

extern "C" {
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "MainValues.h"
}

TextTableView::TextTableView(QWidget *arg_parent) : QTableView(arg_parent)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->setDragDropMode(QAbstractItemView::InternalMove);
    verticalHeader()->setSectionsMovable(true);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);  // Fixed or ResizeToContents?
    verticalHeader()->setMinimumSectionSize(0);
    horizontalHeader()->setMinimumSectionSize(0);
    horizontalHeader()->setVisible(true);
    m_EditorOpenFlag = false;

    horizontalHeader()->setStretchLastSection(true);
    if (s_main_ini_val.DarkMode) {
        horizontalHeader()->setStyleSheet("QHeaderView::section{background:darkblue;}");
    } else {
        horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    }
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    TextTableViewDelegate *DelegateEditor = new  TextTableViewDelegate;
    setItemDelegate(DelegateEditor);

    connect(DelegateEditor, SIGNAL(editorClosed()), this, SLOT(editorClosedSlot()));
}

TextTableView::~TextTableView() {

}

void TextTableView::mouseDoubleClickEvent(QMouseEvent* arg_event)
{
    arg_event->ignore();
}

void TextTableView::mousePressEvent(QMouseEvent* arg_event)
{
    QTableView::mousePressEvent(arg_event);
    arg_event->ignore();
}

void TextTableView::mouseMoveEvent(QMouseEvent* arg_event)
{
    arg_event->ignore();
}

void TextTableView::IncOrDecVariable(TextTableModel *arg_model, int arg_row, double arg_Sign, double arg_stepMultiplier)
{
    arg_stepMultiplier = arg_Sign * arg_stepMultiplier;
    TextTableModel::Variable *loc_variable = arg_model->getVariable(arg_row);
    double loc_step = loc_variable->m_step;
    int loc_stepType = loc_variable->m_stepType;
    if((loc_variable->m_type == 3) && (get_bbvari_conversiontype(loc_variable->m_vid) == BB_CONV_FORMULA)) {
        double loc_actualValue = read_bbvari_equ(loc_variable->m_vid);

        if(loc_stepType == 0) {
            write_bbvari_phys_minmax_check(loc_variable->m_vid, loc_actualValue + (loc_step * arg_stepMultiplier));
        } else {
            write_bbvari_phys_minmax_check(loc_variable->m_vid, (loc_actualValue + (loc_actualValue * (loc_step * arg_stepMultiplier))));
        }
    } else {
        if(loc_stepType == 0) {
            union BB_VARI ValueUnion;
            enum BB_DATA_TYPES par_Type = read_bbvari_union_type(loc_variable->m_vid, &ValueUnion);
            ValueUnion = AddOffsetTo(ValueUnion, par_Type, loc_step * arg_stepMultiplier);
            write_bbvari_union(loc_variable->m_vid, ValueUnion);
        } else {
            double loc_actualValue = read_bbvari_convert_double(loc_variable->m_vid);
            write_bbvari_minmax_check(loc_variable->m_vid, (loc_actualValue + (loc_actualValue * (loc_step * arg_stepMultiplier))));
        }
    }
}

static double modifierToStepMultiplyer(Qt::KeyboardModifiers arg_modifier)
{
    double loc_stepMultiplier = 1;
    if(arg_modifier & Qt::ShiftModifier) {
        loc_stepMultiplier = 100;
    } else {
        if(arg_modifier & Qt::ControlModifier) {
            loc_stepMultiplier = 10;
        }
    }
    return loc_stepMultiplier;
}

void TextTableView::keyPressEvent(QKeyEvent* arg_event)
{
    switch(arg_event->key()) {
    case Qt::Key_Delete:
        {
            QModelIndexList loc_indexes = selectionModel()->selection().indexes();
            foreach (QModelIndex loc_index, loc_indexes) {
                if(loc_index.column() == 1) {
                    int loc_row = loc_index.row();
                    static_cast<TextTableModel*>(model())->removeRows(loc_row, 1);
                }
            }
        }
        break;
    case Qt::Key_PageUp:
    case Qt::Key_Plus:
        {
            QModelIndexList loc_indexes = selectionModel()->selection().indexes();
            foreach (QModelIndex loc_index, loc_indexes) {
                if(loc_index.column() == 1) {
                    int loc_row = loc_index.row();
                    IncOrDecVariable(static_cast<TextTableModel*>(model()), loc_row, +1.0,
                                     modifierToStepMultiplyer(arg_event->modifiers()));
                }
            }
        }
        break;
    case Qt::Key_PageDown:
    case Qt::Key_Minus:
        {
            QModelIndexList loc_indexes = selectionModel()->selection().indexes();
            foreach (QModelIndex loc_index, loc_indexes) {
                if(loc_index.column() == 1) {
                    int loc_row = loc_index.row();
                    IncOrDecVariable(static_cast<TextTableModel*>(model()), loc_row, -1.0,
                                     modifierToStepMultiplyer(arg_event->modifiers()));
                }
            }
        }
        break;
    case Qt::Key_Backtab:
    case Qt::Key_Up:
        m_EditorOpenFlag = false;
        if (arg_event->modifiers() == Qt::NoModifier) {
            QModelIndexList loc_indexes = selectionModel()->selection().indexes();
            if(loc_indexes.count() > 0) {
                if(loc_indexes.first().row() > 0) {
                    QItemSelectionModel *loc_selection = selectionModel();
                    loc_selection->setCurrentIndex(model()->index(loc_indexes.first().row() - 1, 1 ),
                                                   (QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows));
                    setSelectionModel(loc_selection);
                }
            }
        }
        break;
    case Qt::Key_Tab:
    case Qt::Key_Down:
        m_EditorOpenFlag = false;
        if (arg_event->modifiers() == Qt::NoModifier) {
            QModelIndexList loc_indexes = selectionModel()->selection().indexes();
            if(loc_indexes.count() > 0) {
                if(loc_indexes.first().row() < model()->rowCount(QModelIndex()) - 1) {
                    QItemSelectionModel *loc_selection = selectionModel();
                    loc_selection->setCurrentIndex(model()->index(loc_indexes.first().row() + 1, 1 ),
                                                   (QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows));
                    setSelectionModel(loc_selection);
                }
            }
        }
        break;
    case Qt::Key_I:
        {
            if(arg_event->modifiers() == Qt::ControlModifier) {
                if(selectionModel()->selectedIndexes().size() > 0) {
                    QModelIndex loc_index = selectionModel()->selectedIndexes().first();
                    if(loc_index.isValid()) {
                        if(loc_index.model()) {
                            TextTableModel::Variable *loc_variable = static_cast<TextTableModel*>(model())->getVariable(loc_index.row());
                            BlackboardInfoDialog Dlg;
                            Dlg.editCurrentVariable(loc_variable->m_name);

                            if (Dlg.exec() == QDialog::Accepted) {
                                // If the font has changed
                                verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
                            }
                        }
                    }
                }
            }
        }
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_F2:
        if (m_EditorOpenFlag) {
            m_EditorOpenFlag = false;
        } else {
            m_EditorOpenFlag = true;
            QItemSelectionModel *loc_selection = selectionModel();
            if(loc_selection->selectedIndexes().size() > 0) {
                int row = loc_selection->selectedIndexes().first().row();
                QModelIndex loc_index = model()->index(row, 1);  // 1 -> Row 1 -> Values
                loc_selection->setCurrentIndex(loc_index, (QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows));
                edit(loc_index);
            }
        }
        break;
    case Qt::Key_D:
        if (m_EditorOpenFlag) {
            m_EditorOpenFlag = false;
        } else {
            m_EditorOpenFlag = true;
            if(arg_event->modifiers() == Qt::NoModifier) {
                QItemSelectionModel *loc_selection = selectionModel();
                if(loc_selection->selectedIndexes().size() > 0) {
                    int row = loc_selection->selectedIndexes().first().row();
                    QModelIndex loc_index = model()->index(row, 3);  // 3 -> Row 3 -> Display type
                    loc_selection->setCurrentIndex(loc_index, (QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows));
                    edit(loc_index);
                }
            }
        }
        break;
    default:
        break;
    }
}

void TextTableView::editorClosedSlot()
{
    m_EditorOpenFlag = false;
}

void TextTableView::columnWidthChangeSlot(int arg_columnFlags, int arg_nameWidth, int arg_valueWidth, int arg_unitWidth)
{
    if ((arg_columnFlags & CALC_PIXELWIDTH_OF_NAME) == CALC_PIXELWIDTH_OF_NAME) {
        setColumnWidth(0, arg_nameWidth + 20);
    } else if ((arg_columnFlags & CALC_PIXELWIDTH_OF_VALUES) == CALC_PIXELWIDTH_OF_VALUES) {
        setColumnWidth(1, arg_valueWidth + 20);
    } else if ((arg_columnFlags & CALC_PIXELWIDTH_OF_UNIT) == CALC_PIXELWIDTH_OF_UNIT) {
        setColumnWidth(2, arg_unitWidth + 20);
    }
}

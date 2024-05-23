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


#include "DefaultDialogFrame.h"
#include "ui_DefaultDialogFrame.h"
#include <QFontDialog>
#include <QColorDialog>
#include "SharedDataTypes.h"
#include "MainWindow.h"
#include "A2LSelectWidget.h"

extern "C" {
#include "A2LLink.h"
}

DefaultDialogFrame::DefaultDialogFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::DefaultDialogFrame)
{
    ui->setupUi(this);

    m_A2LSelectWidget = NULL;

    QPixmap loc_pixmap(16, 16);
    loc_pixmap.fill(Qt::white);
    QIcon loc_icon(loc_pixmap);
    ui->colorPushButton->setIcon(loc_icon);

    m_defaultColor = QColor(Qt::white);
    m_defaultFont = QFont();

    m_checkableFilterVariableModel = new CheckableSortFilterProxyModel2(this);

    m_blackboardVariableModel = MainWindow::GetBlackboardVariableModel();
    m_checkableFilterVariableModel->setSourceModel(m_blackboardVariableModel);
    m_checkableFilterVariableModel->SetCheckable(true);
    m_checkableFilterVariableModel->sort(0);
    ui->variableListView->setModel(m_checkableFilterVariableModel);
    setVaraibleList();

    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(setFilter(QString)));
    connect(ui->filterAutomaticallyExtendCheckBox, SIGNAL(toggled(bool)), this, SLOT(filterAutomaticallyExtendRegExpr(bool)));
    connect(ui->selectAllCheckBox, SIGNAL(toggled(bool)), this, SLOT(filterSelectAll(bool)));
    connect(ui->filterVisibleCheckBox, SIGNAL(toggled(bool)), m_checkableFilterVariableModel, SLOT(SetOnlyVisable(bool)));
    connect(ui->colorPushButton, SIGNAL(clicked()), this, SLOT(openColorDialog()));
    connect(ui->fontPushButton, SIGNAL(clicked()), this, SLOT(openFontDialog()));
    connect(m_checkableFilterVariableModel, SIGNAL(checkedStateChanged(QString,bool)), this, SLOT(checkStateChanged(QString,bool)));

    m_RegExpCheckedByUser = false;
    ui->filterLineEdit->setText("*");
    // Am Anfang immer "xxxx" zu "*xxxx*" erweitern
    m_AutomaticallyExtendRegExpr = true;
    ui->filterAutomaticallyExtendCheckBox->setChecked(m_AutomaticallyExtendRegExpr);
    setFilter("*");
    ui->filterLineEdit->setFocus();

    ui->variableListView->installEventFilter(this);
}

DefaultDialogFrame::~DefaultDialogFrame()
{
    delete ui;
    if (m_checkableFilterVariableModel != nullptr) delete m_checkableFilterVariableModel;
}

void DefaultDialogFrame::setMultiSelect(bool arg_multiSelect)
{
    m_multiSelect = arg_multiSelect;
    m_checkableFilterVariableModel->setMultiSelect(arg_multiSelect);
    if(arg_multiSelect) {
        m_checkableFilterVariableModel->setMultiSelect(true);
        ui->selectAllCheckBox->setEnabled(true);
        ui->selectAllCheckBox->setVisible(true);
        ui->variableListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    } else {
        m_checkableFilterVariableModel->setMultiSelect(false);
        ui->selectAllCheckBox->setEnabled(false);
        ui->selectAllCheckBox->setVisible(false);
        ui->variableListView->setSelectionMode(QAbstractItemView::SingleSelection);
    }
}

void DefaultDialogFrame::setCurrentVisibleVariable(QString arg_variable)
{
    if (!arg_variable.isEmpty()) {
        m_defaultVariables.append(arg_variable);
    }
    bool Found = false;
    for(int j = 0; j < m_checkableFilterVariableModel->rowCount(); ++j) {
        if(m_checkableFilterVariableModel->data(m_checkableFilterVariableModel->index(j, 0)).toString().compare(arg_variable) == 0) {
            m_checkableFilterVariableModel->setCheckedState(m_checkableFilterVariableModel->index(j, 0), true);
            Found = true;
            break;
        }
    }
    if (!Found) {  // Die Variable exisitier zur Zeit nicht im Blackboard
        //m_blackboardAdditionalVariableModel->setStringList(QStringList(arg_variable));
    }
    ScrollToTheFirstSelected();
}

void DefaultDialogFrame::setCurrentVisibleVariables(QStringList arg_variables)
{
    m_checkableFilterVariableModel->SetCheckedVariableNames(arg_variables);
    m_defaultVariables = arg_variables;
    m_defaultVariables.sort(Qt::CaseInsensitive);
    QStringList UnknownVariables;
    for(int i = 0; i < m_defaultVariables.count(); ++i) {
        bool Found = false;
        for(int j = 0; j < m_checkableFilterVariableModel->rowCount(); ++j) {
            if(m_checkableFilterVariableModel->data(m_checkableFilterVariableModel->index(j, 0)).toString().compare(m_defaultVariables.at(i)) == 0) {
                //m_checkableFilterVariableModel->setCheckedState(m_checkableFilterVariableModel->index(j, 0), true);
                Found = true;
                break;
            }
        }
        if (!Found) {  // Die Variable exisitier zur Zeit nicht im Blackboard
            UnknownVariables.append(m_defaultVariables.at(i));
        }
    }
    //m_blackboardAdditionalVariableModel->setStringList(UnknownVariables);
    ScrollToTheFirstSelected();
}

void DefaultDialogFrame::setCurrentColor(QColor arg_color)
{
    m_defaultColor = arg_color;
    m_currentColor = arg_color;
    QPixmap loc_pixmap = ui->colorPushButton->icon().pixmap(ui->colorPushButton->iconSize());
    loc_pixmap.fill(arg_color);
    QIcon loc_icon(loc_pixmap);
    ui->colorPushButton->setIcon(loc_icon);
}

void DefaultDialogFrame::setCurrentFont(QFont arg_font)
{
    m_defaultFont = arg_font;
    m_currentFont = arg_font;
}

void DefaultDialogFrame::setVaraibleList(DefaultDialogFrame::VARIABLE_TYPE arg_type)
{
    switch(arg_type) {
    case ANY:
        m_checkableFilterVariableModel->SetFilterConversionType(static_cast<enum BB_CONV_TYPES>(-1));
        break;
    case ENUM:
        m_checkableFilterVariableModel->SetFilterConversionType(BB_CONV_TEXTREP);
        break;
    }
}

void DefaultDialogFrame::userReject()
{
    emit colorChanged(m_defaultColor);
    emit fontChanged(m_defaultFont);
    //emit windowNameChanged(m_defaultWindowName);
    emit defaultVariables(m_defaultVariables);
}

QString DefaultDialogFrame::selectedVariable()
{
    QModelIndex loc_index;
    for(int i = 0; i < m_checkableFilterVariableModel->rowCount(); ++i) {
        loc_index = m_checkableFilterVariableModel->index(i, 0);
        if(m_checkableFilterVariableModel->currentCheckState(loc_index) == Qt::Checked) {
            return m_checkableFilterVariableModel->data(loc_index).toString();
        }
    }
    return QString();
}

QStringList DefaultDialogFrame::selectedVariables()
{
    QModelIndex loc_index;
    QStringList loc_variables;
    for(int i = 0; i < m_checkableFilterVariableModel->rowCount(); ++i) {
        loc_index = m_checkableFilterVariableModel->index(i, 0);
        if(m_checkableFilterVariableModel->currentCheckState(loc_index) == Qt::Checked) {
            loc_variables << m_checkableFilterVariableModel->data(loc_index).toString();
        }
    }
    return loc_variables;
}

QColor DefaultDialogFrame::selectedColor()
{
    return m_currentColor;
}

QFont DefaultDialogFrame::selectedFont()
{
    return m_currentFont;
}

void DefaultDialogFrame::ScrollToTheFirstSelected()
{
    for(int Row = 0; Row < m_checkableFilterVariableModel->rowCount(); Row++) {
        QModelIndex Index = m_checkableFilterVariableModel->index(Row, 0);
        if (m_checkableFilterVariableModel->currentCheckState(Index) == Qt::Checked) {
            ui->variableListView->scrollTo(Index);
            break;  // nur den als erstes gefundene "Checked" Variable
        }
    }
}

bool DefaultDialogFrame::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->variableListView && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *loc_keyEvent = static_cast<QKeyEvent*>(event);
        if(loc_keyEvent->key() == Qt::Key_Space)
        {
            handleMultiselect(loc_keyEvent);
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

void DefaultDialogFrame::openColorDialog()
{
    QColorDialog loc_dialog(m_currentColor, this);
    if(loc_dialog.exec() == QDialog::Accepted) {
        m_currentColor = loc_dialog.selectedColor();
        QPixmap loc_pixmap = ui->colorPushButton->icon().pixmap(ui->colorPushButton->iconSize());
        loc_pixmap.fill(m_currentColor);
        ui->colorPushButton->setIcon(QIcon(loc_pixmap));
        emit colorChanged(m_currentColor);
    }
}

void DefaultDialogFrame::openFontDialog()
{
    QFontDialog loc_fontDialog(m_currentFont, this);
    if(loc_fontDialog.exec() == QDialog::Accepted) {
        m_currentFont = loc_fontDialog.currentFont();
        emit fontChanged(loc_fontDialog.currentFont());
    }
}

void DefaultDialogFrame::variableSelected(QModelIndex arg_variableIndex)
{
    if(m_checkableFilterVariableModel->data(arg_variableIndex, Qt::CheckStateRole) == Qt::Checked) {
        emit variableSelectionChanged(m_checkableFilterVariableModel->data(arg_variableIndex).toString(), true);
    } else {
        emit variableSelectionChanged(m_checkableFilterVariableModel->data(arg_variableIndex).toString(), false);
    }
}

void DefaultDialogFrame::filterAutomaticallyExtendRegExpr(bool arg_setFilter)
{
    m_AutomaticallyExtendRegExpr = arg_setFilter;
    setFilter(ui->filterLineEdit->text());
}

void DefaultDialogFrame::filterSelectAll(bool arg_selectAll) {
    for(int i = 0; i < m_checkableFilterVariableModel->rowCount(); ++i) {
        m_checkableFilterVariableModel->setCheckedState(m_checkableFilterVariableModel->index(i, 0), arg_selectAll);
    }
    m_checkableFilterVariableModel->ResetProxyModel();
}

void DefaultDialogFrame::checkStateChanged(QString arg_name, bool arg_state)
{
    emit variableSelectionChanged(arg_name, arg_state);
}

void DefaultDialogFrame::changeSelectedVariable(QString arg_name, Qt::CheckState arg_state)
{
    if(arg_name.isEmpty()) {
        m_checkableFilterVariableModel->resetToDefault();
    } else {
        for(int j = 0; j < m_checkableFilterVariableModel->rowCount(); ++j) {
            if(m_checkableFilterVariableModel->data(m_checkableFilterVariableModel->index(j, 0)).toString().compare(arg_name) == 0) {
                m_checkableFilterVariableModel->setCheckedState(m_checkableFilterVariableModel->index(j, 0), (arg_state == Qt::Unchecked) ? false : true);
                break;
            }

        }
    }
}

void DefaultDialogFrame::clearSelection()
{
    m_checkableFilterVariableModel->resetToDefault();
}

void DefaultDialogFrame::setFilter(const QString &pattern)
{
    if (m_AutomaticallyExtendRegExpr) {
        QString NewPattern;
        int Len = pattern.size();
        if (Len == 0) {
            NewPattern = QString("*");
        } else {
            if (pattern.at(0) != QChar('*')) {
                if (pattern.at(Len-1) != QChar('*')) {
                    NewPattern = QString("*").append(pattern).append("*");
                } else {
                    NewPattern = QString("*").append(pattern);
                }
            } else {
                if (pattern.at(Len-1) != QChar('*')) {
                    NewPattern = pattern;
                    NewPattern.append("*");
                } else {
                    // ** zu einem * zusammenfassen
                    if (Len == 2) {
                        NewPattern = QString("*");
                    } else {
                        NewPattern = pattern;
                    }
                }
            }
        }
        m_checkableFilterVariableModel->SetFilter(NewPattern);
        if (m_A2LSelectWidget != nullptr) m_A2LSelectWidget->SetFilter(NewPattern);
    } else {
        m_checkableFilterVariableModel->SetFilter(pattern);
        if (m_A2LSelectWidget != nullptr) m_A2LSelectWidget->SetFilter(pattern);
    }
}

void DefaultDialogFrame::on_A2LPushButton_clicked()
{
    if (m_A2LSelectWidget == nullptr) {
        m_A2LSelectWidget = new A2LSelectWidget();
        m_A2LSelectWidget->SetVisable(true, A2L_LABEL_TYPE_MEASUREMENT); // display only all measurements
        m_A2LSelectWidget->SetFilter(ui->filterLineEdit->text());
        ui->splitter->addWidget(m_A2LSelectWidget);
        ui->A2LPushButton->setText("<< A2L");
    } else {
        delete m_A2LSelectWidget;
        m_A2LSelectWidget = nullptr;
        ui->A2LPushButton->setText("A2L >>");
    }
}

void DefaultDialogFrame::handleMultiselect(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Space)
    {
        for(QModelIndex &index: ui->variableListView->selectionModel()->selectedIndexes())
        {
            if(m_checkableFilterVariableModel->currentCheckState(index) == Qt::Unchecked)
            {
                m_checkableFilterVariableModel->setCheckedState(index, true);
            }
            else
            {
                m_checkableFilterVariableModel->setCheckedState(index, false);
            }
        }

        m_checkableFilterVariableModel->ResetProxyModel();
    }
}

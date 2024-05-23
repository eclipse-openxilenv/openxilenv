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


#ifndef DEFAULTDIALOGFRAME_H
#define DEFAULTDIALOGFRAME_H

#include <QFrame>
#include <QStringListModel>
#include "CheckableSortFilterProxyModel.h"
#include "BlackboardVariableModel.h"

#include "A2LSelectWidget.h"

extern "C"{
#include "Blackboard.h"
}

namespace Ui {
class DefaultDialogFrame;
}

class DefaultDialogFrame : public QFrame
{
    Q_OBJECT

public:
    enum VARIABLE_TYPE {
        ANY = 0,
        ENUM
    };

    explicit DefaultDialogFrame(QWidget *parent = nullptr);
    ~DefaultDialogFrame();
    void setMultiSelect(bool arg_multiSelect = true);
    void setCurrentVisibleVariable(QString arg_variable);
    void setCurrentVisibleVariables(QStringList arg_variables);
    void setCurrentColor(QColor arg_color);
    void setCurrentFont(QFont arg_font);
    void setVaraibleList(VARIABLE_TYPE arg_type = ANY);
    void userReject();
    QString selectedVariable();
    QStringList selectedVariables();
    QColor selectedColor();
    QFont selectedFont();
    void ScrollToTheFirstSelected();
    bool eventFilter(QObject *watched, QEvent *event);

public slots:
    void openColorDialog();
    void openFontDialog();
    void variableSelected(QModelIndex arg_variableIndex);
    void filterAutomaticallyExtendRegExpr(bool arg_setFilter);
    void filterSelectAll(bool arg_selectAll);
    void checkStateChanged(QString arg_name, bool arg_state);
    void changeSelectedVariable(QString arg_name, Qt::CheckState arg_state);
    void clearSelection();

    void setFilter(const QString &pattern);

signals:
    void colorChanged(QColor arg_color);
    void fontChanged(QFont arg_font);
    void variableSelectionChanged(QString arg_variable, bool arg_selected);
    void variablesSelectionChanged(QStringList arg_varaibles, bool arg_selected);
    void defaultVariables(QStringList arg_variables);

private slots:
    void on_A2LPushButton_clicked();

private:

    void ChangeFilterModel(bool WithWildcards);
    void handleMultiselect(QKeyEvent *e);

    Ui::DefaultDialogFrame *ui;
    QFont m_defaultFont;
    QColor m_defaultColor;
    QFont m_currentFont;
    QColor m_currentColor;
    QStringList m_defaultVariables;

    BlackboardVariableModel *m_blackboardVariableModel; //GetBlackboardVariableModel();
    CheckableSortFilterProxyModel2 *m_checkableFilterVariableModel;

    bool m_AutomaticallyExtendRegExpr;
    bool m_multiSelect;

    bool m_RegExpCheckedByUser;

    A2LSelectWidget *m_A2LSelectWidget;
};

#endif // DEFAULTDIALOGFRAME_H

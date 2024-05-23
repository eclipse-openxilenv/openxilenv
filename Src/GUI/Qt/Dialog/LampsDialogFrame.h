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


#ifndef LAMPSDIALOGFRAME_H
#define LAMPSDIALOGFRAME_H

#include <QFrame>
#include <QGridLayout>
#include <QColor>
#include "LampsModel.h"
#include "LampsGraphicsView.h"
#include "DialogFrame.h"

namespace Ui {
class CustomLampsDialogFrame;
}

class CustomLampsDialogFrame : public CustomDialogFrame
{
    Q_OBJECT

public:
    explicit CustomLampsDialogFrame(CustomLampsGraphicsView *arg_graphicsView, QWidget *arg_parent = nullptr);
    ~CustomLampsDialogFrame();
    void setPositionColumn(int arg_column);
    void setPositionRow(int arg_row);
    void setDimensionColumn(int arg_column);
    void setDimensionRow(int arg_row);
    void userAccept();
    void userReject();

public slots:
    void showStancil(QModelIndex index);
    void stancilInputOk();
    void stancilInputCancel();
    void stancilListAdd();
    void stancilListDelete();
    void stancilListChange();
    void stancilInputPreview();
    void changeBackgroundColor(QColor arg_color);
    void changeLampVaraible(QString arg_variable, bool arg_checked);
    void changeCurrentLamp(QModelIndex arg_index);
    void RowPosition(int arg_rowPosition);
    void RowDimension(int arg_rowDimension);
    void ColumnPosition(int arg_ColumnPosition);
    void ColumnDimension(int arg_ColumnPosition);

signals:
    void variableSelectionChanged(QString arg_name, Qt::CheckState arg_checkState);
    void cancelNewLamp(CustomLampsGraphicsView *arg_newLamp);

private:
    Ui::CustomLampsDialogFrame *ui;
    ControlLampModel *m_lampModel;
    CustomLampsGraphicsView *m_currentView; // die aktuelle View referenzieren!!
    bool m_changeStancilFlag;

    int m_currentFromRow;
    int m_currentFromColumn;
    int m_currentRowSpan;
    int m_currentColumnSpan;
    QString m_currentVariable;
    QString m_currentStancil;
};

#endif // LAMPSDIALOGFRAME_H

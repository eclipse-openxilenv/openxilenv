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


#ifndef A2LSELECTWIDGET_H
#define A2LSELECTWIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListView>
#include "A2LMeasurementCalibrationModel.h"
#include "A2LFunctionModel.h"


namespace Ui {
class A2LSelectWidget;
}

class A2LSelectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit A2LSelectWidget(QWidget *parent = nullptr);
    ~A2LSelectWidget();

    void SetVisable(bool par_Visable, unsigned int par_TypeMask);
    void SetFilter(QString par_Filter);

    void SetFilerVisable(bool par_Visable);

    void SetReferenceButtonsVisable(bool par_Visable);

    void SetLabelGroupName(QString &par_Name);

    void SetSelected(QStringList &par_Selected);

    QStringList GetSelected();

    QString GetProcess();
    void SetProcess(QString &par_ProcessName);

private slots:

    void on_FunctionPushButton_clicked();

    void on_ReferencePushButton_clicked();

    void on_DereferencingPushButton_clicked();

    void on_ProcessListView_itemSelectionChanged();

    void FunctionListView_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void on_FIlterLineEdit_textChanged(const QString &arg1);

    void on_ViewOnlyReferencedCheckBox_clicked(bool checked);

    void on_ViewOnlyNotReferencedCheckBox_clicked(bool checked);

    void on_ViewOnlyCharacteristicsCheckBox_clicked(bool checked);

    void on_ViewOnlyMeasurementsCheckBox_clicked(bool checked);

private:
    void Init(void);

    void InitModels(int par_LabelsFlag, int par_FunctionsFlag);

    void ReferenceVariable(int par_LinkNr, int par_Index,
                           char *par_Name, char *par_Unit, int par_ConvType, char *par_Conv,
                           int par_FormatLength, int par_FormatLayout,
                           uint64_t par_Address, int par_Pid, int par_Type, int par_Dir,
                           bool par_ReferenceFlag);

    void ReferenceOrUnreference(bool par_ReferenceFlag);

    void ChangeGroupBoxHeader();

    Ui::A2LSelectWidget *ui;

    bool m_Visable;
    bool m_GroupBoxVisable;

    unsigned int m_TypeMask;

    int m_ReferencedFlags;

    A2LMeasurementCalibrationModel *m_Model;
    QSortFilterProxyModel *m_SortedModel;

    QGroupBox *m_FunctionGroupBox;
    QVBoxLayout *m_VerticalLayout;
    QPushButton *m_PushButton;
    QListView *m_FunctionListView;

    A2LFunctionModel *m_FunctionModel;
    QSortFilterProxyModel *m_SortedFunctionModel;

    QStringList m_SelectedFunctions;
};

#endif // A2LSELECTWIDGET_H

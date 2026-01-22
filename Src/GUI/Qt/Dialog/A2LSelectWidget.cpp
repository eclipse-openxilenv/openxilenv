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


#include "A2LSelectWidget.h"
#include "ui_A2LSelectWidget.h"
#include "StringHelpers.h"

#include <QModelIndex>
#include <QModelIndexList>
#include <QList>
#include "FileDialog.h"
#include "FileExtensions.h"
extern "C" {
#include "Platform.h"
#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "ThrowError.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "A2LLink.h"
}


A2LSelectWidget::A2LSelectWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::A2LSelectWidget)
{
    ui->setupUi(this);
    // default is the filter not visable
    ui->FilterLabel->setVisible(false);
    ui->FIlterLineEdit->setVisible(false);
    m_Model = nullptr;
    m_SortedModel = nullptr;
    m_FunctionGroupBox = nullptr;
    m_PushButton = nullptr;
    m_FunctionListView = nullptr;
    m_FunctionModel = nullptr;
    m_SortedFunctionModel = nullptr;
    m_GroupBoxVisable = false;

    m_Visable = 0;
    m_TypeMask = 0;

    m_ReferencedFlags = 0x3;
    ui->ViewOnlyMeasurementsCheckBox->setChecked(true);
    ui->ViewOnlyCharacteristicsCheckBox->setChecked(false);
    ui->ViewOnlyReferencedCheckBox->setChecked(true);
    ui->ViewOnlyNotReferencedCheckBox->setChecked(true);

    // make process list small as possiple
    ui->ProcessFunctionLabelSplitter->setStretchFactor(0, 1);
    ui->ProcessFunctionLabelSplitter->setStretchFactor(1, 5);
}

A2LSelectWidget::~A2LSelectWidget()
{
    delete ui;
    if (m_SortedModel) delete m_SortedModel;
    if (m_Model) delete m_Model;
}

void A2LSelectWidget::SetVisable(bool par_Visable, unsigned int par_TypeMask)
{
    m_TypeMask = par_TypeMask;
    ChangeGroupBoxHeader();
    if (!m_Visable && par_Visable) {
        Init();
    }
    m_Visable = par_Visable;
}

void A2LSelectWidget::SetFilter(QString par_Filter)
{
    m_SortedModel->setFilterWildcard(par_Filter);
}

void A2LSelectWidget::SetFilerVisable(bool par_Visable)
{
    ui->FilterLabel->setVisible(par_Visable);
    ui->FIlterLineEdit->setVisible(par_Visable);
}

void A2LSelectWidget::SetReferenceButtonsVisable(bool par_Visable)
{
    ui->ReferencePushButton->setVisible(par_Visable);
    ui->DereferencingPushButton->setVisible(par_Visable);
}

void A2LSelectWidget::SetLabelGroupName(QString &par_Name)
{
    ui->LabelGroupBox->setTitle(par_Name);
}

void A2LSelectWidget::SetSelected(QStringList &par_Selected)
{
    ui->LabelListView->selectionModel()->clear();
    foreach(QString Label, par_Selected) {
        QModelIndex Index = m_Model->indexOfVariable(Label);
        Index = m_SortedModel->mapFromSource(Index);
        if (Index.isValid()) ui->LabelListView->selectionModel()->select(Index, QItemSelectionModel::Select);
    }
}

QStringList A2LSelectWidget::GetSelected()
{
    QStringList Ret;
    QModelIndexList Selected = ui->LabelListView->selectionModel()->selectedIndexes();
    foreach(QModelIndex Index, Selected) {
        Ret.append(Index.data(Qt::DisplayRole).toString());
    }
    return Ret;
}

QString A2LSelectWidget::GetProcess()
{
    QList<QListWidgetItem*> Items = ui->ProcessListView->selectedItems();
    if ((Items.size() > 0) && (Items.at(0) != nullptr)) {
        return Items.at(0)->text();
    } else {
       return QString();
    }
}

void A2LSelectWidget::SetProcess(QString &par_ProcessName)
{
    for (int Row = 0; Row < ui->ProcessListView->count(); Row++) {
        QListWidgetItem *Item = ui->ProcessListView->item(Row);
        if (Compare2ProcessNames(QStringToConstChar(Item->data(Qt::DisplayRole).toString()),
                                 QStringToConstChar(par_ProcessName)) == 0) {
            ui->ProcessListView->setCurrentItem(Item, QItemSelectionModel::SelectCurrent);
            break;
        }
    }
}

void A2LSelectWidget::on_FunctionPushButton_clicked()
{
    if (m_FunctionGroupBox == nullptr) {
        m_FunctionGroupBox = new QGroupBox(this);
        m_FunctionGroupBox->setTitle("Functions");
        m_VerticalLayout = new QVBoxLayout(m_FunctionGroupBox);

        m_PushButton = new QPushButton(m_FunctionGroupBox);
        m_PushButton->setText("All");
        m_FunctionListView = new QListView(m_FunctionGroupBox);

        m_FunctionModel = new A2LFunctionModel;

        InitModels(0, 1);

        m_SortedFunctionModel = new QSortFilterProxyModel;
        m_SortedFunctionModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        m_SortedFunctionModel->sort(0);
        m_SortedFunctionModel->setSourceModel(m_FunctionModel);
        m_FunctionListView->setModel(m_SortedFunctionModel);

        m_VerticalLayout->addWidget(m_PushButton);
        m_VerticalLayout->addWidget(m_FunctionListView);

        bool Ret;
        Ret = connect(m_FunctionListView->selectionModel(),
                      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                      this,
                      SLOT(FunctionListView_selectionChanged(const QItemSelection &, const QItemSelection &)));

        ui->FunctionLabelSplitter->addWidget(m_FunctionGroupBox);
        ui->FunctionPushButton->setText("<< Function");
    } else {
        delete m_FunctionListView;
        delete m_PushButton;
        delete m_VerticalLayout;
        delete m_FunctionGroupBox;
        delete m_SortedFunctionModel;
        delete m_FunctionModel;
        m_FunctionGroupBox = nullptr;
        ui->FunctionPushButton->setText("Function >>");
        m_SelectedFunctions.clear();
        InitModels(1, 0);
    }
}

void A2LSelectWidget::ReferenceVariable(int par_LinkNr, int par_Index,
                                        char *par_Name, char *par_Unit, int par_ConvType, char *par_Conv,
                                        int par_FormatLength, int par_FormatLayout,
                                        uint64_t par_Address, int par_Pid, int par_Type, int par_Dir,
                                        bool par_ReferenceFlag)
{
    int Vid;
    if (par_ReferenceFlag) {
        if (scm_ref_vari (par_Address, par_Pid, par_Name, par_Type, 0x3) ||
            ((Vid = get_bbvarivid_by_name(par_Name)) <= 0)) {
            ThrowError (1, "cannot reference \"%s\"", par_Name);
        } else {
            if (Vid > 0) {
                set_bbvari_unit(Vid, par_Unit);
                set_bbvari_conversion(Vid, par_ConvType, par_Conv);
                set_bbvari_format(Vid, par_FormatLength, par_FormatLayout);
                A2LIncDecGetReferenceToDataFromLink(par_LinkNr, par_Index, +1, par_Dir, Vid);
                m_Model->ReferenceChanged(par_Index);
            }
        }
    } else {
        scm_unref_vari(par_Address, par_Pid, par_Name, par_Type);
        A2LIncDecGetReferenceToDataFromLink(par_LinkNr, par_Index, -1, par_Dir, 0);
        m_Model->ReferenceChanged(par_Index);
    }
}

void A2LSelectWidget::ReferenceOrUnreference(bool par_ReferenceFlag)
{
    QModelIndexList Indexes = ui->LabelListView->selectionModel()->selectedIndexes();
    char Name[512];
    int Type;
    uint64_t Address;
    int ConvType;
    char *Conv = (char*)my_malloc(128*1024);
    char Unit[64];
    int FormatLength;
    int FormatLayout;
    int XDim, YDim, ZDim;
    double Min, Max;
    int IsWritable;

    foreach (QModelIndex Index, Indexes) {
        if (Index.isValid()) {
            int Pid;
            int LinkNr;
            int Idx;
            Index = m_SortedModel->mapToSource(Index);
            Idx = m_Model->GetIndexPidLink(Index, &Pid, &LinkNr);
            if ((A2LIncDecGetReferenceToDataFromLink(LinkNr, Idx, 0, 0, 0) != 0) ^ par_ReferenceFlag) {
                if (A2lGetMeasurementInfos (LinkNr, Idx,
                                            Name, sizeof(Name),
                                            &Type, &Address,
                                            &ConvType, Conv, 128*1024,
                                            Unit, sizeof(Unit),
                                            &FormatLength, &FormatLayout,
                                            &XDim, &YDim, &ZDim,
                                            &Min, &Max, &IsWritable) == 0) {
                    int Dir;
                    if (par_ReferenceFlag) {
                        Dir = A2LGetReadWritablFlagsFromLink(LinkNr, Idx, 0);
                    } else {
                        Dir = 0;
                    }
                    if ((XDim <= 1) && (YDim <= 1) && (ZDim <= 1)) {
                        ReferenceVariable(LinkNr, Idx,
                                          Name, Unit, ConvType, Conv,
                                          FormatLength, FormatLayout,
                                          Address, Pid, Type, Dir, par_ReferenceFlag);
                    } else if ((YDim <= 1) && (ZDim <= 1)) {  // one dimensional array
                        int i;
                        char *p = Name + strlen(Name);
                        for (i = 0; i < XDim; i++) {
                            PrintFormatToString(p, sizeof(Name) - (p - Name), "[%i]", i);
                            ReferenceVariable(LinkNr, Idx,
                                              Name, Unit, ConvType, Conv,
                                              FormatLength, FormatLayout,
                                              Address, Pid, Type, Dir, par_ReferenceFlag);
                            Address += GetDataTypeByteSize(Type);
                        }
                    } else if (ZDim <= 1) { // two dimensional array
                        int i, j;
                        char *p = Name + strlen(Name);
                        for (j = 0; j < YDim; j++) {
                            for (i = 0; i < XDim; i++) {
                                PrintFormatToString(p, sizeof(Name) - (p - Name), "[%i][%i]", j, i);
                                ReferenceVariable(LinkNr, Idx,
                                                  Name, Unit, ConvType, Conv,
                                                  FormatLength, FormatLayout,
                                                  Address, Pid, Type, Dir, par_ReferenceFlag);
                                Address += GetDataTypeByteSize(Type);
                            }
                        }
                    }
                }
            }
        }
    }
    my_free(Conv);
}

void A2LSelectWidget::ChangeGroupBoxHeader()
{
    if (((m_TypeMask & 0xFF) != 0) && ((m_TypeMask & 0xFF00) == 0)) {
        ui->LabelGroupBox->setTitle("Measurements");
    } else if (((m_TypeMask & 0xFF) == 0) && ((m_TypeMask & 0xFF00) != 0)) {
        ui->LabelGroupBox->setTitle("Characteristics");
    } else {
        ui->LabelGroupBox->setTitle("Measurements and Characteristics");
    }
}

void A2LSelectWidget::on_ReferencePushButton_clicked()
{
    ReferenceOrUnreference(true);
}

void A2LSelectWidget::on_DereferencingPushButton_clicked()
{
    ReferenceOrUnreference(false);
}

void A2LSelectWidget::on_ProcessListView_itemSelectionChanged()
{
    InitModels(1, 1);
}

void A2LSelectWidget::FunctionListView_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);
    m_SelectedFunctions.clear();
    QModelIndexList Selected = m_FunctionListView->selectionModel()->selectedRows();
    foreach (QModelIndex Item, Selected) {
        QString ProcessName = Item.data(Qt::DisplayRole).toString();
        m_SelectedFunctions.append(ProcessName);
    }
    InitModels(1, 0);
}

void A2LSelectWidget::on_FIlterLineEdit_textChanged(const QString &arg1)
{
    SetFilter(arg1);
}

void A2LSelectWidget::Init()
{
    m_Model = new A2LMeasurementCalibrationModel;
    m_SortedModel = new QSortFilterProxyModel;
    m_SortedModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_SortedModel->sort(0);
    m_SortedModel->setSourceModel(m_Model);
    ui->LabelListView->setModel(m_SortedModel);

    char *Name;
    READ_NEXT_PROCESS_NAME *Processes = init_read_next_process_name (2 | 0x100);
    int Count = 0;
    while ((Name = read_next_process_name (Processes)) != nullptr) {
        //if (!IsInternalProcess (Name)) {
            if (A2LGetLinkToExternProcess(Name) > 0) {
                ui->ProcessListView->addItem (CharToQString(Name));
                Count++;
            }
        //}
    }
    close_read_next_process_name(Processes);
    if (Count == 1) {
        // if only one select this
        ui->ProcessListView->selectionModel()->select(ui->ProcessListView->model()->index(0, 0, QModelIndex()), QItemSelectionModel::Select);
    }
}

void A2LSelectWidget::InitModels(int par_LabelsFlag, int par_FunctionsFlag)
{
    QStringList Processes;
    QList<QListWidgetItem*> Selected = ui->ProcessListView->selectedItems();
    foreach (QListWidgetItem*Item, Selected) {
        QString ProcessName = Item->data(Qt::DisplayRole).toString();
        Processes.append(ProcessName);
    }
    //QStringList Functions;
    if (par_LabelsFlag) {
        m_Model->SetProcessesAndTypes(Processes, m_TypeMask, m_SelectedFunctions, m_ReferencedFlags);
    }
    if (par_FunctionsFlag) {
        if (m_FunctionGroupBox != nullptr) {
            m_FunctionModel->InitForProcess(Processes);
        }
    }
}


void A2LSelectWidget::on_ViewOnlyReferencedCheckBox_clicked(bool checked)
{
    m_ReferencedFlags = (m_ReferencedFlags & 0x2) | (int)checked;
    InitModels(1, 0);
}

void A2LSelectWidget::on_ViewOnlyNotReferencedCheckBox_clicked(bool checked)
{
    m_ReferencedFlags = (m_ReferencedFlags & 0x1) | ((int)checked << 1);
    InitModels(1, 0);
}

void A2LSelectWidget::on_ViewOnlyCharacteristicsCheckBox_clicked(bool checked)
{
    uint32_t Mask = A2L_LABEL_TYPE_SINGLE_VALUE_CALIBRATION | A2L_LABEL_TYPE_VAL_BLK_CALIBRATION;
    if (checked) {
        m_TypeMask |= Mask;
    } else {
        m_TypeMask &= ~Mask;
    }
    ChangeGroupBoxHeader();
    InitModels(1, 0);
}

void A2LSelectWidget::on_ViewOnlyMeasurementsCheckBox_clicked(bool checked)
{
    uint32_t Mask = A2L_LABEL_TYPE_MEASUREMENT;
    if (checked) {
        m_TypeMask |= Mask;
    } else {
        m_TypeMask &= ~Mask;
    }
    ChangeGroupBoxHeader();
    InitModels(1, 0);
}

void A2LSelectWidget::on_ExportPushButton_clicked()
{
    QString ProcessName = GetProcess();
    if (!ProcessName.isEmpty()) {
        int Pid = get_pid_by_name(ProcessName.toLatin1().data());
        if (Pid > 0) {
            QString FileName = FileDialog::getSaveFileName(this, QString ("Export A2L Measurements list file"), QString(), QString (INI_EXT));
            if(!FileName.isEmpty()) {
                ExportMeasurementReferencesListForProcess(Pid, FileName.toLatin1().data());
            }
            return;
        }
    }
    ThrowError(1, "You have to select a process");
}

void A2LSelectWidget::on_ImportPushButton_clicked()
{
    QString ProcessName = GetProcess();
    if (!ProcessName.isEmpty()) {
        int Pid = get_pid_by_name(ProcessName.toLatin1().data());
        if (Pid > 0) {
            QString FileName = FileDialog::getOpenFileName(this, QString ("Import A2L Measurements list file"), QString(), QString (INI_EXT));
            if(!FileName.isEmpty()) {
                ImportMeasurementReferencesListForProcess(Pid, FileName.toLatin1().data());
                ui->LabelListView->reset();
            }
            return;
        }
    }
    ThrowError(1, "You have to select a process");
}

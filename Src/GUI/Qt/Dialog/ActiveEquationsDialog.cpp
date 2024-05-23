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


#include "ActiveEquationsDialog.h"
#include "ui_ActiveEquationsDialog.h"
#include "QtIniFile.h"

#include <QTextStream>

extern "C" {
#include "EquationList.h"
#include "MainValues.h"
#include "Files.h"
#include "MyMemory.h"
#include "ThrowError.h"
}

ActiveEquationsDialog::ActiveEquationsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ActiveEquationsDialog)
{
    setAttribute (Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    m_Model = new ActiveEquationsModel (this);
    ui->treeView->setModel(m_Model);
    ui->treeView->setColumnWidth(0, 400);

    int XPos, YPos, XSize, YSize;
    int Fd = ScQt_GetMainFileDescriptor();

    QString Line = ScQt_IniFileDataBaseReadString ("BasicSettings", "ActiveEquationsWindowSizeAndPos", "", Fd);
    QStringList List = Line.split(",");
    if (List.size() == 4) {
        XPos = List.at(0).toInt();
        YPos = List.at(1).toInt();
        XSize = List.at(2).toInt();
        YSize = List.at(3).toInt();
        // Check if window is inside the visable range
#if _WIN32
        HWND HwndDesktop;
        RECT RcClient;
        HwndDesktop = GetDesktopWindow ();
        if (GetClientRect (HwndDesktop, &RcClient)) {
            if (XPos > (RcClient.right - RcClient.left - 200))
                XPos = (RcClient.right - RcClient.left - 200);
            if (YPos > (RcClient.bottom - RcClient.top - 200))
                YPos  = (RcClient.bottom - RcClient.top - 200);
        }
#endif
        this->move (XPos, YPos);
        this->resize(XSize, YSize);
    }

    m_Filter = BuildIncludeExcludeFilterFromIni ("BasicSettings", "ActiveEquationsFilter", Fd);
    ui->Filter->SetFilter(m_Filter);
    bool Ret = connect(ui->Filter, SIGNAL(FilterClicked()), this, SLOT(on_Filter_clicked ()));
    if (!Ret) {
        ThrowError (1, "connect");
    }

    SetTypeMask(static_cast<int>(ScQt_IniFileDataBaseReadUInt("BasicSettings", "ActiveEquationsWindowTypeFilterMask", 0xFFFFFFFF, Fd)));
}

static ActiveEquationsDialog *Dialog;

ActiveEquationsDialog::~ActiveEquationsDialog()
{
    delete ui;
    delete m_Model;
    FreeIncludeExcludeFilter(m_Filter);
    Dialog = nullptr;
}

void ActiveEquationsDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    int Fd = ScQt_GetMainFileDescriptor();
    QString Line;
    QTextStream Stream(&Line);
    Stream << this->x() << ", "
           << this->y() << ", "
           << this->width() << ", "
           << this->height();
    ScQt_IniFileDataBaseWriteString("BasicSettings", "ActiveEquationsWindowSizeAndPos", Line, Fd);
    SaveIncludeExcludeListsToIni (m_Filter, "BasicSettings", "ActiveEquationsFilter", Fd);
    ScQt_IniFileDataBaseWriteUIntHex("BasicSettings", "ActiveEquationsWindowTypeFilterMask",static_cast<unsigned int>(GetTypeMask()), Fd);
}

int ActiveEquationsDialog::GetTypeMask()
{
    int Ret = 0;
    if (ui->BlackboardCheckBox->isChecked()) Ret |= EQU_TYPE_BLACKBOARD;
    if (ui->GlobalCheckBox->isChecked()) Ret |= EQU_TYPE_GLOBAL_EQU_CALCULATOR;
    if (ui->BeforeCheckBox->isChecked()) Ret |= EQU_TYPE_BEFORE_PROCESS;
    if (ui->BehindCheckBox->isChecked()) Ret |= EQU_TYPE_BEHIND_PROCESS;
    if (ui->CANCheckBox->isChecked()) Ret |= EQU_TYPE_CAN;
    if (ui->AnalogCheckBox->isChecked()) Ret |= EQU_TYPE_ANALOG;
    if (ui->RampeCheckBox->isChecked()) Ret |= EQU_TYPE_RAMPE;
    if (ui->WaitUntilCheckBox->isChecked()) Ret |= EQU_TYPE_WAIT_UNTIL;
    return Ret;
}

void ActiveEquationsDialog::SetTypeMask(int par_Mask)
{
    ui->BlackboardCheckBox->setChecked((par_Mask & EQU_TYPE_BLACKBOARD) != 0);
    ui->GlobalCheckBox->setChecked((par_Mask & EQU_TYPE_GLOBAL_EQU_CALCULATOR) != 0);
    ui->BeforeCheckBox->setChecked((par_Mask & EQU_TYPE_BEFORE_PROCESS) != 0);
    ui->BehindCheckBox->setChecked((par_Mask & EQU_TYPE_BEHIND_PROCESS) != 0);
    ui->CANCheckBox->setChecked((par_Mask & EQU_TYPE_CAN) != 0);
    ui->AnalogCheckBox->setChecked((par_Mask & EQU_TYPE_ANALOG) != 0);
    ui->RampeCheckBox->setChecked((par_Mask & EQU_TYPE_RAMPE) != 0);
    ui->WaitUntilCheckBox->setChecked((par_Mask & EQU_TYPE_WAIT_UNTIL) != 0);
}


void ActiveEquationsDialog::Open(QWidget *Parent)
{
    if (Dialog == nullptr) {
        Dialog = new ActiveEquationsDialog (Parent);
        Dialog->show();
    }
}


ActiveEquationsModel::ActiveEquationsModel(QObject *parent)
{
    Q_UNUSED(parent)
    m_NumberOfActiveEquations = 0;
    m_ActiveEquationSnapShot = nullptr;
    m_SizeOfActiveEquationSnapShot = 0;
    m_StringBufferSize = 0;
    m_StringBuffer = nullptr;

    m_AnalogPixmap = new QPixmap (":/Icons/equ_list_analog.png");
    m_BbPixmap = new QPixmap (":/Icons/equ_list_bb.png");
    m_BeforePixmap = new QPixmap (":/Icons/equ_list_before.png");
    m_BehindPixmap = new QPixmap (":/Icons/equ_list_behind.png");
    m_CanPixmap = new QPixmap (":/Icons/equ_list_can.png");
    m_GlobalPixmap = new QPixmap (":/Icons/equ_list_global.png");
    m_RampePixmap = new QPixmap (":/Icons/equ_list_rampe.png");
    m_UnknownPixmap = new QPixmap (":/Icons/equ_list_unknown.png");
    m_WaitUntilPixmap = new QPixmap (":/Icons/equ_list_waituntil.png");
}

ActiveEquationsModel::~ActiveEquationsModel()
{
    if (m_ActiveEquationSnapShot != nullptr) {
        my_free (m_ActiveEquationSnapShot);
    }
    if (m_StringBuffer != nullptr) {
        my_free (m_StringBuffer);
    }
}

QVariant ActiveEquationsModel::data(const QModelIndex &index, int role) const
{
    int Row = index.row();
    int Column = index.column();
    if (role == Qt::DisplayRole) {
        if (Row < m_SizeOfActiveEquationSnapShot) {
            switch (Column) {
            case 0:
                return QString (m_ActiveEquationSnapShot[Row].Equation);
            case 1:
                {
                    int x;
                    char Processes[256];
                    Processes[0] = 0;
                    for (x = 0; x < 32; x++) {
                        if (m_ActiveEquationSnapShot[Row].Pids[x]) {
                            sprintf (Processes + strlen (Processes), "%i ", m_ActiveEquationSnapShot[Row].Pids[x]);
                        }
                    }
                    return QString(Processes);
                }
            case 2:
                return QString("%1").arg(m_ActiveEquationSnapShot[Row].CycleLoaded);
            case 3:
                return QString("%1").arg(m_ActiveEquationSnapShot[Row].AttachCounter);
            case 4:
                return QString (m_ActiveEquationSnapShot[Row].AdditionalInfos);
            default:
                return QVariant();
            }
        }
    } else if (role == Qt::DecorationRole) {
        if (Column == 0) {
            switch (m_ActiveEquationSnapShot[Row].TypeFlags) {
            case EQU_TYPE_BLACKBOARD:
                return QVariant(*m_BbPixmap);
            case EQU_TYPE_GLOBAL_EQU_CALCULATOR:
                return QVariant(*m_GlobalPixmap);
            case EQU_TYPE_BEFORE_PROCESS:
                return QVariant(*m_BeforePixmap);
            case EQU_TYPE_BEHIND_PROCESS:
                return QVariant(*m_BehindPixmap);
            case EQU_TYPE_CAN:
                return QVariant(*m_CanPixmap);
            case EQU_TYPE_ANALOG:
                return QVariant(*m_AnalogPixmap);
            case EQU_TYPE_RAMPE:
                return QVariant(*m_RampePixmap);
            case EQU_TYPE_WAIT_UNTIL:
                return QVariant(*m_WaitUntilPixmap);
            default:
                return QVariant(*m_UnknownPixmap);
            }
        }
    }

    return QVariant();
}

QVariant ActiveEquationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return QString ("Equation");
        case 1:
            return QString ("Process");
        case 2:
            return QString ("Cycle");
        case 3:
            return QString ("Attach counter");
        case 4:
            return QString ("Additional infos");
        default:
            return QVariant();
        }
    }
    return QVariant();

}

QModelIndex ActiveEquationsModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex (row, column);
}

QModelIndex ActiveEquationsModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int ActiveEquationsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_NumberOfActiveEquations;
}

int ActiveEquationsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 5;
}

void ActiveEquationsModel::UpdateSnapShot(INCLUDE_EXCLUDE_FILTER *par_Filter, int par_TypeMask)
{
    m_NumberOfActiveEquations = GetActiveEquationSnapShot (par_Filter, par_TypeMask, &m_SizeOfActiveEquationSnapShot, &m_ActiveEquationSnapShot, &m_StringBufferSize, &m_StringBuffer);
}

void ActiveEquationsDialog::on_RefreshPushButton_clicked()
{
    FreeIncludeExcludeFilter(m_Filter);
    m_Filter = ui->Filter->GetFilter();
    m_Model->UpdateSnapShot(m_Filter, GetTypeMask());
    ui->treeView->reset();
}

void ActiveEquationsDialog::on_Filter_clicked()
{
    FreeIncludeExcludeFilter(m_Filter);
    m_Filter = ui->Filter->GetFilter();
    m_Model->UpdateSnapShot(m_Filter, GetTypeMask());
    ui->treeView->reset();
}

void ActiveEquationsDialog::on_ClosePushButton_clicked()
{
    close();
}

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


#include "InsertCANErrorDialog.h"
#include "ui_InsertCANErrorDialog.h"

#include "QtIniFile.h"

#include <QString>
#include <QMenu>

extern "C" {
#include "PrintFormatToString.h"
#include "ConfigurablePrefix.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "IniDataBase.h"
#include "CanDataBase.h"
#include "ReadCanCfg.h"
#include "ThrowError.h"
}

InsertCANErrorDialog::InsertCANErrorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InsertCANErrorDialog)
{
    ui->setupUi(this);

    m_Channel = 0;
    m_Id = 0;
    m_StartBit = 0;
    m_BitSize = 1;
    m_ByteOrder = 0;
    m_BitValue = 0;
    m_Cycles = 0x7FFFFFFF;
    m_Size = 8;

    for (int x = 0; x < (MAX_CAN_CHANNELS - 1); x++) {
        QString ChannelNumber = QString().number(x);
        ui->ChannelComboBox->addItem(ChannelNumber);
        ui->ChannelComboBoxSize->addItem(ChannelNumber);
        ui->ChannelComboBoxInterrupt->addItem(ChannelNumber);
    }
    ui->ByteOrderComboBox->addItem("lsb_first");
    ui->ByteOrderComboBox->addItem("msb_first");

    m_CanConfig = new CanConfigBase(true);  // Read only one variant which is associated a CAN channel
    m_CanConfig->ReadFromIni();
    m_CanConfigTreeModel = new CanConfigTreeModel (m_CanConfig, ui->CanTreeView);
    ui->CanTreeView->setModel(m_CanConfigTreeModel);
    QModelIndex Index = m_CanConfigTreeModel->index(0,0);
    ui->CanTreeView->expandAll();
    ui->CanTreeView->resizeColumnToContents(0);
    ui->CanTreeView->collapseAll();
    ui->CanTreeView->expand(Index);

    ui->BitValueLineEdit->SetValue(static_cast<uint64_t>(0));
    ui->CyclesLineEdit->SetValue(0x7FFFFFFF, 16);
    ui->CyclesLineEditSize->SetValue(0x7FFFFFFF, 16);
    ui->CyclesLineEditInterrupt->SetValue(0x7FFFFFFF, 16);
    char Name[BBVARI_NAME_SIZE];
    m_CanBitErrorVid = add_bbvari(ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES, ".CANBitError.Counter", Name, sizeof(Name)),
                                   BB_UNKNOWN, "");
    QString Help = QString("0");
    ui->CounterLineEdit->setText("0");

    FillHistory();

    // CAN-Tree-View Section Changed weiterleiten an Dialog
    connect(ui->CanTreeView, SIGNAL(selectionChangedSignal(const QItemSelection &, const QItemSelection &)), this, SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));
    // Tree-View Context Menu Event an Dialog umleiten
    ui->CanTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->CanTreeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(CanTreeContextMenuRequestedSlot(const QPoint &)));

    m_SearchVariableAct = new QAction(tr("&Search varible"), this);
    connect(m_SearchVariableAct, SIGNAL(triggered()), this, SLOT(SearchVariableSlot()));

    m_Timer = new QTimer(this);
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(CyclicTimeout()));
    m_Timer->setInterval(200);
    m_Timer->start();
}

InsertCANErrorDialog::~InsertCANErrorDialog()
{
    remove_bbvari(m_CanBitErrorVid);
    delete ui;
    delete m_Timer;
}

void InsertCANErrorDialog::on_StartPushButton_clicked()
{
    GetAll();
    SaveHistory();
    ScriptSetCanErr (m_Channel, m_Id, m_StartBit, m_BitSize, static_cast<const char*>((m_ByteOrder) ? "msb_first" : "lsb_first"), m_Cycles, m_BitValue);
}

void InsertCANErrorDialog::on_StopPushButton_clicked()
{
    ScriptClearCanErr();
}

void InsertCANErrorDialog::on_ClosePushButton_clicked()
{
    this->close();
}

bool InsertCANErrorDialog::GetAll()
{
    bool Help, Ret = true;
    switch(ui->tabWidget->currentIndex()) {
    case 0:  // change bits
        m_Channel = ui->ChannelComboBox->currentIndex();
        m_Id = ui->ObjectIdLineEdit->GetIntValue(&Help);
        Ret = Ret && Help;
        m_StartBit = ui->StartBitLineEdit->GetIntValue(&Help);
        Ret = Ret && Help;
        m_BitSize = ui->BitSizeLineEdit->GetIntValue(&Help);
        Ret = Ret && Help;
        m_ByteOrder = ui->ByteOrderComboBox->currentIndex();
        m_BitValue = ui->BitValueLineEdit->GetUInt64Value(&Help);
        Ret = Ret && Help;
        m_Cycles = ui->CyclesLineEdit->GetUIntValue();
        Ret = Ret && Help;
        break;
    case 1: // change size
        m_Channel = ui->ChannelComboBoxSize->currentIndex();
        m_Id = ui->ObjectIdLineEditSize->GetIntValue(&Help);
        Ret = Ret && Help;
        m_StartBit = -1;
        m_BitSize = ui->SizeToChangeTo->GetIntValue(&Help);  // this will store the new size
        Ret = Ret && Help;
        m_Size = m_BitSize;
        m_ByteOrder = 0;
        m_BitValue = 0;
        m_Cycles = ui->CyclesLineEditSize->GetUIntValue();
        Ret = Ret && Help;
        break;
    case 2: // interrupt
        m_Channel = ui->ChannelComboBoxInterrupt->currentIndex();
        m_Id = ui->ObjectIdLineEditInterrupt->GetIntValue(&Help);
        Ret = Ret && Help;
        m_StartBit = -2;
        m_BitSize = 0;
        m_ByteOrder = 0;
        m_BitValue = 0;
        m_Cycles = ui->CyclesLineEditInterrupt->GetUIntValue();
        Ret = Ret && Help;
        break;
    }
    return Ret;
}

void InsertCANErrorDialog::FillHistory()
{
    QString Section = QString("CAN/Global");
    QString Entry = QString("can_biterr_history0");
    QString Line = ScQt_IniFileDataBaseReadString (Section, Entry, "", ScQt_GetMainFileDescriptor());
    QStringList Elements = Line.split(",");
    if (Elements.count() >= 8) {
        QString Channel = Elements.at(0);
        ui->ChannelComboBox->setCurrentText(Channel);
        ui->ChannelComboBoxSize->setCurrentText(Channel);
        ui->ChannelComboBoxInterrupt->setCurrentText(Channel);
        QString Id = Elements.at(1);
        ui->ObjectIdLineEdit->setText(Id);
        ui->ObjectIdLineEditSize->setText(Id);
        ui->ObjectIdLineEditInterrupt->setText(Id);
        QString StartBit = Elements.at(2);
        ui->StartBitLineEdit->setText(StartBit);
        QString BitSize = Elements.at(3);
        ui->BitSizeLineEdit->setText(BitSize);
        QString ByteOrder = Elements.at(4);
        ui->ByteOrderComboBox->setCurrentText(ByteOrder);
        QString BitValue = Elements.at(5);
        ui->BitValueLineEdit->setText(BitValue);
        QString Size = Elements.at(6);
        ui->SizeToChangeTo->setText(Size);
        QString Cycles = Elements.at(7);
        ui->CyclesLineEdit->setText(Cycles);
        ui->CyclesLineEditSize->setText(Cycles);
        ui->CyclesLineEditInterrupt->setText(Cycles);
    }
}

void InsertCANErrorDialog::SaveHistory()
{
    QString Line = QString().number(m_Channel);
    Line.append(",0x");
    Line.append(QString().number(m_Id, 16));
    Line.append(",");
    Line.append(QString().number(m_StartBit));
    Line.append(",");
    Line.append(QString().number(m_BitSize));
    if (m_ByteOrder) Line.append(",msb_first,");
    else Line.append(",lsb_first,");
    Line.append(QString().number(m_BitValue));
    Line.append(",");
    Line.append(QString().number(m_Size));
    Line.append(",");
    Line.append(QString().number(m_Cycles));
    QString Section = QString("CAN/Global");
    QString Entry = QString("can_biterr_history0");
    ScQt_IniFileDataBaseWriteString(Section, Entry, Line, ScQt_GetMainFileDescriptor());
}

void InsertCANErrorDialog::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (!selected.isEmpty()) {
        QModelIndex SelectItem = selected.indexes().at(0);
        CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectItem);
        if ((Item->GetType() == CanConfigElement::Signal) ||
            (Item->GetType() == CanConfigElement::Object)) {
            CanConfigSignal *Signal;
            CanConfigObject *Object;
            if (Item->GetType() == CanConfigElement::Signal) {
                Signal = static_cast<CanConfigSignal*>(Item);
                Object = static_cast<CanConfigObject*>(Signal->parent());;
            } else {
                Signal = nullptr;
                Object = static_cast<CanConfigObject*>(Item);
            }
            if (Object != nullptr) {
                ui->ObjectIdLineEdit->SetValue(Object->GetId(), 16);
                ui->ObjectIdLineEditSize->SetValue(Object->GetId(), 16);
                ui->ObjectIdLineEditInterrupt->SetValue(Object->GetId(), 16);
                ui->SizeToChangeTo->SetValue(Object->GetSize(), 10);
                CanConfigVariant *Variant = static_cast<CanConfigVariant*>(Object->parent());
                if (Variant != nullptr) {
                    char section[INI_MAX_SECTION_LENGTH];
                    PrintFormatToString (section, sizeof(section), "CAN/Global");
                    int NumberOfChannels = GetCanControllerCountFromIni (section, GetMainFileDescriptor());

                    for (int i = 0; i < NumberOfChannels; i++) {
                        if (Variant->IsConnectedToChannel(i+1)) {
                            ui->ChannelComboBox->setCurrentIndex(i);
                            ui->ChannelComboBoxSize->setCurrentIndex(i);
                            ui->ChannelComboBoxInterrupt->setCurrentIndex(i);
                            if (Signal != nullptr) {
                                ui->StartBitLineEdit->SetValue(Signal->GetStartBitAddress());
                                ui->BitSizeLineEdit->SetValue(Signal->GetBitSize());
                                ui->ByteOrderComboBox->setCurrentText((Signal->GetByteOrder() == MsbFirst) ? "msb_first" : "lsb_first");
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

void InsertCANErrorDialog::CyclicTimeout()
{
    double Value = read_bbvari_convert_double(m_CanBitErrorVid);
    uint64_t ValueUInt = static_cast<uint64_t>(Value);
    QString ValueString = QString().number(ValueUInt);
    ui->CounterLineEdit->setText(ValueString);
    ui->CounterLineEditSize->setText(ValueString);
    ui->CounterLineEditInterrupt->setText(ValueString);
}

void InsertCANErrorDialog::CanTreeContextMenuRequestedSlot(const QPoint &pos)
{
    QMenu menu(this);

    QModelIndex Index = ui->CanTreeView->indexAt (pos);

    if (Index.isValid()) {
        switch (m_CanConfigTreeModel->GetItem(Index)->GetType()) {
        case CanConfigElement::Server:
        case CanConfigElement::Variant:
        case CanConfigElement::Object:
            menu.addAction (m_SearchVariableAct);
            menu.exec(ui->CanTreeView->mapToGlobal(pos));
            break;
        default:
        case CanConfigElement::Signal:
            break;
        }
    }
}

void InsertCANErrorDialog::SearchVariableSlot()
{
    QModelIndexList Selected = ui->CanTreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        CanConfigSearchVariableDialog Dlg(m_CanConfigTreeModel, Selected.at(0), this);
        bool Ret;
        Ret = connect(&Dlg, SIGNAL(SelectSignal(QModelIndex &)), this, SLOT(SelectSignalSlot(QModelIndex &)));
        if (Dlg.exec() == QDialog::Accepted) {
            ;
        }
    }
}

void InsertCANErrorDialog::SelectSignalSlot(QModelIndex &SignalIndex)
{
    ui->CanTreeView->setCurrentIndex(SignalIndex);
}


void InsertCANErrorDialog::on_StartPushButtonSize_clicked()
{
    GetAll();
    SaveHistory();
    ScriptSetCanErr (m_Channel, m_Id, m_StartBit, m_BitSize, static_cast<const char*>((m_ByteOrder) ? "msb_first" : "lsb_first"), m_Cycles, m_BitValue);
}


void InsertCANErrorDialog::on_StopPushButtonSize_clicked()
{
    ScriptClearCanErr();
}


void InsertCANErrorDialog::on_StartPushButtonInterrupt_clicked()
{
    GetAll();
    SaveHistory();
    ScriptSetCanErr (m_Channel, m_Id, m_StartBit, m_BitSize, static_cast<const char*>((m_ByteOrder) ? "msb_first" : "lsb_first"), m_Cycles, m_BitValue);
}


void InsertCANErrorDialog::on_StopPushButtonInterrupt_clicked()
{
    ScriptClearCanErr();
}


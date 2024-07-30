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


#include <inttypes.h>

#include "CanConfigDialog.h"
#include "ui_CanConfigDialog.h"

#include <QMenu>
#include <QPainter>
#include <QItemDelegate>
#include <QComboBox>

#include "FileDialog.h"
#include "QtIniFile.h"
#include "StringHelpers.h"
#include "CanConfigSearchVariableDialog.h"
#include "CanConfigJ1939AdditionalsDialog.h"
#include "CanConfigJ1939MultiPGDialog.h"

extern "C" {
#include "FileExtensions.h"
#include "EquationParser.h"
#include "IniDataBase.h"
#include "ThrowError.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#include "Message.h"
#include "MainValues.h"
#include "ReadCanCfg.h"
#if BUILD_WITH_GATEWAY_VIRTUAL_CAN
#include "GatewayCanDriver.h"
#endif
#include "RemoteMasterControlProcess.h"
}

static void EnableTab(QTabWidget *par_TabWidget, int par_TabNumber)
{
    for (int x = 0; x < par_TabWidget->count(); x++) {
        par_TabWidget->setTabEnabled(x, (x == par_TabNumber));
    }
}

static void EnableGatewayFlags(Ui::CanConfigDialog *ui, bool par_Enable)
{
    ui->VirtualGatewayComboBox->setEnabled(par_Enable);
    ui->VirtualGatewayLineEdit->setEnabled(par_Enable);
    ui->RealGatewayComboBox->setEnabled(par_Enable);
    ui->RealGatewayLineEdit->setEnabled(par_Enable);
}

class QTreeWidgetEditorDelegate : public QItemDelegate
{
    //    Q_OBJECT
public:
    QTreeWidgetEditorDelegate(QObject *parent):QItemDelegate(parent) {};
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


QWidget* QTreeWidgetEditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.column() == 2) {
        QComboBox *Editor = new QComboBox(parent);
        Editor->addItem("OFF");
        Editor->addItem("ON");
        Editor->addItem("IF_NOT_EXIST_OFF");
        Editor->addItem("IF_NOT_EXIST_ON");
        return Editor;
    } else {
        return nullptr;
    }
}

CanConfigDialog::CanConfigDialog(QWidget *par_Parent) : Dialog(par_Parent),
    ui(new Ui::CanConfigDialog)
{
    m_CopyBuffer = nullptr;
    m_CanConfig = new CanConfigBase(false);
    m_CanConfig->ReadFromIni();

    ui->setupUi(this);

#ifndef _WIN32
    // Gateway virtual CAN exists only under windows
    ui->EnableGatewayDeviceDriverCheckBox->setDisabled(true);
#endif

    // the gateway device drive access is not included
    ui->EnableGatewayDeviceDriverCheckBox->setVisible(false);
    ui->RealGatewayLabel->setVisible(false);
    ui->RealGatewayComboBox->setVisible(false);
    ui->RealGatewayLineEdit->setVisible(false);
    ui->VirtualGatewayLabel->setVisible(false);
    ui->VirtualGatewayComboBox->setVisible(false);
    ui->VirtualGatewayLineEdit->setVisible(false);

    QString Prefix = QString(GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES));
    QString Text = Prefix;
    Text.append(".CANx.\"Id\"");
    ui->ObjectControlIdRadioButton->setText(Text);
    Text = Prefix;
    Text.append(".CANx.\"object name\"");
    ui->ObjectControlObjNameRadioButton->setText(Text);
    Text = Prefix;
    Text.append(".CANx.\"variante name\".\"object name\"");
    ui->ObjectControlVarObjNameRadioButton->setText(Text);

    createActions();

    ui->MappingTreeWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
    QTreeWidgetEditorDelegate *DelegateComboBox = new QTreeWidgetEditorDelegate(ui->MappingTreeWidget);
    ui->MappingTreeWidget->setItemDelegate(DelegateComboBox);

    ui->NumberOfChannelsSpinBox->setValue(m_CanConfig->GetNumberOfChannels());

    m_CanConfigTreeModel = new CanConfigTreeModel (m_CanConfig, ui->TreeView);
    ui->TreeView->setModel(m_CanConfigTreeModel);
    QModelIndex Index = m_CanConfigTreeModel->index(0,0);
    ui->TreeView->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->TreeView->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->TreeView->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    //ui->TreeView->header()->setStretchLastSection(false);
    ui->TreeView->setColumnWidth(0, 200);
    ui->TreeView->setColumnWidth(1, 30);
    ui->TreeView->setColumnWidth(2, 10);
    ui->TreeView->expand(Index);

    ui->ObjectAllSignalsListView->setModel(m_CanConfigTreeModel);

    ui->AllVariantListView->setModel(m_CanConfigTreeModel);
    ui->AllVariantListView->setRootIndex(m_CanConfigTreeModel->GetListViewRootIndex());

    // disable all tabs except "Server"
    EnableTab (ui->tabWidget, 0);

    ui->SignalByteOrderComboBox->addItem(QString("lsb_first"));
    ui->SignalByteOrderComboBox->addItem(QString("msb_first"));
    ui->SignalSignComboBox->addItem(QString("signed"));
    ui->SignalSignComboBox->addItem(QString("unsigned"));
    ui->SignalSignComboBox->addItem(QString("float"));

    ui->SignalBlackboardDataTypeComboBox->addItem(QString("BYTE"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("UBYTE"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("WORD"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("UWORD"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("DWORD"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("UDWORD"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("QWORD"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("UQWORD"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("FLOAT"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("DOUBLE"));
    ui->SignalBlackboardDataTypeComboBox->addItem(QString("UNKNOWN_DOUBLE"));

    ui->ObjectByteOrderComboBox->addItem(QString("lsb_first"));
    ui->ObjectByteOrderComboBox->addItem(QString("msb_first"));

    ui->ObjectDirectionComboBox->addItem(QString("write"));
    ui->ObjectDirectionComboBox->addItem(QString("read"));
    ui->ObjectDirectionComboBox->addItem(QString("write (variable id)"));

    ui->VirtualGatewayComboBox->addItem(QString("no"));
    ui->VirtualGatewayComboBox->addItem(QString("yes"));
    ui->VirtualGatewayComboBox->addItem(QString("equ"));

    ui->RealGatewayComboBox->addItem(QString("no"));
    ui->RealGatewayComboBox->addItem(QString("yes"));
    ui->RealGatewayComboBox->addItem(QString("equ"));

    ui->ObjectTypeJ1939ComboBox->addItem(QString("21 Multi package"));
    ui->ObjectTypeJ1939ComboBox->addItem(QString("22 Multi-PG"));
    ui->ObjectTypeJ1939ComboBox->addItem(QString("22 C-PG"));

    EnableGatewayFlags(ui, 0);    // first disabled

    m_CanObjectModel = new CanObjectModel(this);
    ui->ObjectSignalBitPositionsTableView->setModel(m_CanObjectModel);
    CanObjectBitDelegate *Delegate = new CanObjectBitDelegate(this);
    ui->ObjectSignalBitPositionsTableView->setItemDelegate(Delegate);

    ui->ObjectSignalBitPositionsTableView->horizontalHeader()->setMinimumSectionSize(30);
    ui->ObjectSignalBitPositionsTableView->horizontalHeader()->setMaximumSectionSize(30);
    for (int c = 0; c < 8; c++) {
        ui->ObjectSignalBitPositionsTableView->setColumnWidth(c, 30);
    }

    ui->NumberOfChannelsSpinBox->setRange(1, MAX_CAN_CHANNELS);

    CanSignalDelegate *SignalDelegate = new CanSignalDelegate(this);
    ui->ObjectAllSignalsListView->setItemDelegate(SignalDelegate);

    ui->TreeView->setCurrentIndex(m_CanConfigTreeModel->index(0,0));
}

CanConfigDialog::~CanConfigDialog()
{
    delete ui;
    delete m_CanConfig;
    if (m_CopyBuffer != nullptr) {
        delete m_CopyBuffer;
    }
}

void CanConfigDialog::SaveToIni()
{
    // first delete all "CAN"-Sections
    char section[INI_MAX_SECTION_LENGTH];
    int Fd = GetMainFileDescriptor();
    sprintf (section, "CAN/Global");
    if (IniFileDataBaseLookIfSectionExist (section, Fd)) {
        IniFileDataBaseWriteString(section, nullptr, nullptr, Fd);
        for (int v = 0; ; v++) {
            sprintf (section, "CAN/Variante_%i", v);
            if (IniFileDataBaseLookIfSectionExist (section, Fd)) {
                IniFileDataBaseWriteString(section, nullptr, nullptr, Fd);
                for (int o = 0; ; o++) {
                    sprintf (section, "CAN/Variante_%i/Object_%i", v, o);
                    if (IniFileDataBaseLookIfSectionExist (section, Fd)) {
                        IniFileDataBaseWriteString(section, nullptr, nullptr, Fd);
                        for (int s = 0; ; s++) {
                            sprintf (section, "CAN/Variante_%i/Object_%i/Signal_%i", v, o, s);
                            if (IniFileDataBaseLookIfSectionExist (section, Fd)) {
                                IniFileDataBaseWriteString(section, nullptr, nullptr, Fd);
                            } else {
                                break;
                            }
                        }
                    } else {
                        break;
                    }
                }
            } else {
                break;
            }
        }
    }
    // than save all amended
    m_CanConfig->WriteToIni();

    // If desired restart the CANServer with the new configuration
    if (m_CanConfig->ShouldRestartCanImmediately()) {
        write_message (get_pid_by_name ("CANServer"),
                                        NEW_CANSERVER_STOP,
                                        0,
                                        nullptr);
    }
}

void CanConfigDialog::MappingTreeContextMenuRequestedSlot(const QPoint &par_Pos)
{
    QMenu menu(this);

    QModelIndex Index = ui->MappingTreeWidget->indexAt (par_Pos);

    if (Index.isValid()) {
        QModelIndex Parent = Index.parent();
        if (Parent.isValid()) {  // If a valid parent exist than context menue of a variant
            menu.addAction (m_AddNewVariantAct);
        } else {   // If a valid parent exist than context menue of a channel
            // do nothing
        }
    }
}

void CanConfigDialog::selectionChanged(const QItemSelection &par_Selected, const QItemSelection &par_Deselected)
{
    if (!par_Deselected.isEmpty()) {
        QModelIndex DeselectItem = par_Deselected.indexes().at(0);
        CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(DeselectItem);
        Item->StoreDialogTab(ui);
    }
    if (!par_Selected.isEmpty()) {
        QModelIndex SelectItem = par_Selected.indexes().at(0);
        CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectItem);
        Item->FillDialogTab(ui, SelectItem);
        if (Item->GetType() == CanConfigElement::Object) {
             m_CanObjectModel->SetCanObject(static_cast<CanConfigObject*>(Item));
        }
    }
}

void CanConfigDialog::CanTreeContextMenuRequestedSlot(const QPoint &par_Pos)
{
    QMenu menu(this);

    QModelIndex Index = ui->TreeView->indexAt (par_Pos);

    if (Index.isValid()) {
        switch (m_CanConfigTreeModel->GetItem(Index)->GetType()) {
        case CanConfigElement::Base:
            break;
        case CanConfigElement::Server:
            menu.addAction (m_AddNewVariantAct);
            if ((m_CopyBuffer != nullptr) && (m_CopyBuffer->GetType() == CanConfigElement::Variant)) menu.addAction (m_PasteVariantAct);
            menu.addAction (m_ImportVariantAct);
            menu.addAction (m_SearchVariableAct);
            menu.exec(ui->TreeView->mapToGlobal(par_Pos));
            break;
        case CanConfigElement::Variant:
            menu.addAction (m_AddNewObjectAct);
            menu.addAction (m_DeleteVarianteAct);
            menu.addAction (m_CopyVarianteAct);
            if ((m_CopyBuffer != nullptr) && (m_CopyBuffer->GetType() == CanConfigElement::Object)) menu.addAction (m_PasteObjectAct);
            menu.addAction (m_ExportVarianteAct);
            menu.addAction (m_ExportSortedVarianteAct);
            menu.addAction (m_ImportObjectAct);
            menu.addAction (m_SearchVariableAct);
            menu.addAction (m_AppendVarianteAct);
            //menu.addAction (m_SwapObjectReadWriteAct);
            //menu.addAction (m_SwapObjecAndSignaltReadWriteAct);
            menu.addAction (m_SortOjectsByIdAct);
            menu.addAction (m_SortOjectsByNameAct);
            menu.exec(ui->TreeView->mapToGlobal(par_Pos));
            break;
        case CanConfigElement::Object:
            menu.addAction (m_AddNewSignalAct);
            menu.addAction (m_DeleteObjectAct);
            menu.addAction (m_CopyObjectAct);
            if ((m_CopyBuffer != nullptr) && (m_CopyBuffer->GetType() == CanConfigElement::Signal)) menu.addAction (m_PasteSignalAct);
            menu.addAction (m_ExportObjectAct);
            menu.addAction (m_ImportSignalAct);
            menu.addAction (m_SearchVariableAct);
            menu.exec(ui->TreeView->mapToGlobal(par_Pos));
            break;
        case CanConfigElement::Signal:
            menu.addAction (m_DeleteSignalAct);
            menu.addAction (m_CopySignalAct);
            menu.addAction (m_ExportSignalAct);
            menu.addAction (m_AddBeforeSignalAct);
            menu.addAction (m_AddBehindSignalAct);
            if ((m_CopyBuffer != nullptr) && (m_CopyBuffer->GetType() == CanConfigElement::Signal)) {
                menu.addAction (m_PasteBeforeSignalAct);
                menu.addAction (m_PasteBehindSignalAct);
            }
            menu.exec(ui->TreeView->mapToGlobal(par_Pos));
            break;
        }
    }
}

void CanConfigDialog::AddNewVariantSlot()
{
    QModelIndex ParentIndex = ui->TreeView->currentIndex();
    if (ParentIndex.isValid()) {
        m_CanConfigTreeModel->insertRow(0, ParentIndex);
    }
}

void CanConfigDialog::PasteVariantSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if (m_CopyBuffer != nullptr) {
            if (m_CopyBuffer->GetType() == CanConfigElement::Variant) {
                m_CanConfigTreeModel->insertRow(0, m_CopyBuffer, Selected.at(0));
            }
        }
    }
}

void CanConfigDialog::ImportVariantSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if ((static_cast<CanConfigElement*>(Selected.at(0).internalPointer())->GetType()) == CanConfigElement::Server) {
            CanConfigServer *Server = static_cast<CanConfigServer*>(Selected.at(0).internalPointer());
            QString Filename = FileDialog::getOpenFileName(this,  QString("Import CAN variant"), QString(), QString(CAN_EXT));
            if (!Filename.isEmpty()) {
                char File[MAX_PATH];
                StringCopyMaxCharTruncate (File, QStringToConstChar(Filename), MAX_PATH);
                File[MAX_PATH-1]= 0;
                int Fd = IniFileDataBaseOpen(File);
                if (Fd <= 0) {
                    ThrowError (1, "cannot import CAN variant \"%s\"", File);
                } else {
                    if (IniFileDataBaseReadInt ("CAN/Global", "copy_buffer_type", 0, Fd) != 2) {
                        ThrowError (1, "cannot import CAN variant \"%s\" (there is no variant defined in this file)", File);
                    } else {
                        CanConfigVariant *Imported = new CanConfigVariant(Server);
                        if (Imported->ReadFromIni(9999, Fd)) {
                            m_CanConfigTreeModel->insertRow(0, Imported, Selected.at(0));
                        } else {
                            delete Imported;
                        }
                    }
                    IniFileDataBaseClose(Fd);
                }
            }
        }
    }
}

void CanConfigDialog::SearchVariableSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        //if (((CanConfigElement*)Selected.at(0).internalPointer())->GetType() == CanConfigElement::Server) {
            CanConfigSearchVariableDialog Dlg(m_CanConfigTreeModel, Selected.at(0), this);
            bool Ret;
            Ret = connect(&Dlg, SIGNAL(SelectSignal(QModelIndex &)), this, SLOT(SelectSignalSlot(QModelIndex &)));
            if (Dlg.exec() == QDialog::Accepted) {
                ;
            }
        //}
    }
}

void CanConfigDialog::AddNewObjectSlot()
{
    QModelIndex ParentIndex = ui->TreeView->currentIndex();
    if (ParentIndex.isValid()) {
        m_CanConfigTreeModel->insertRow(0, ParentIndex);
    }
}

void CanConfigDialog::DeleteVarianteSlot()
{
    CopyVarianteSlot();
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        QModelIndex ParentIndex = m_CanConfigTreeModel->parent(Selected.at(0));
        if (ParentIndex.isValid()) {
            m_CanConfigTreeModel->removeRow(Selected.at(0).row(), ParentIndex);
            // if deleted select parent
            ui->TreeView->selectionModel()->setCurrentIndex(ParentIndex, QItemSelectionModel::Select);
        }
    }
}

void CanConfigDialog::CopyVarianteSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        CanConfigVariant *Item = static_cast<CanConfigVariant*>(Selected.at(0).internalPointer());
        m_CopyBuffer = new CanConfigVariant(*Item);  // copy contructor!?
    }
}

void CanConfigDialog::PasteObjectSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if (m_CopyBuffer != nullptr) {
            if (m_CopyBuffer->GetType() == CanConfigElement::Object) {
                m_CanConfigTreeModel->insertRow(0, m_CopyBuffer, Selected.at(0));
            }
        }
    }
}

void CanConfigDialog::ExportVarianteSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if ((static_cast<CanConfigElement*>(Selected.at(0).internalPointer()))->GetType() == CanConfigElement::Variant) {
            CanConfigVariant *Item = static_cast<CanConfigVariant*>(Selected.at(0).internalPointer());
            QString Filename = FileDialog::getSaveFileName(this,  QString("Export CAN variant"), QString(), QString(CAN_EXT));
            if (!Filename.isEmpty()) {
                char File[MAX_PATH];
                StringCopyMaxCharTruncate (File, QStringToConstChar(Filename), MAX_PATH);
                File[MAX_PATH-1]= 0;
                int Fd = IniFileDataBaseCreateAndOpenNewIniFile(File);
                IniFileDataBaseWriteString ("CAN/Global", "copy_buffer_type", "2", Fd);
                Item->WriteToIni(9999, Fd);
                if (IniFileDataBaseSave(Fd, File, 2)) {
                    ThrowError (1, "cannot export CAN-DB to file %s", File);
                }
            }
        }
    }
}

void CanConfigDialog::ExportSortedVarianteSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        CanConfigVariant *Item = static_cast<CanConfigVariant*>(Selected.at(0).internalPointer());

        QString Filename = FileDialog::getSaveFileName(this,  QString("Export sorted CAN variant"), QString(), QString(CAN_EXT));
        if (!Filename.isEmpty()) {
            char File[MAX_PATH];
            StringCopyMaxCharTruncate (File, QStringToConstChar(Filename), MAX_PATH);
            File[MAX_PATH-1]= 0;
            int Fd = IniFileDataBaseCreateAndOpenNewIniFile(File);
            if (Fd > 0) {
                IniFileDataBaseWriteString ("CAN/Global", "copy_buffer_type", "2", Fd);
                CanConfigVariant *SortedItem = new CanConfigVariant(*Item);
                SortedItem->SortObjectsByIdentifier();
                SortedItem->WriteToIni(9999, Fd);
                if (IniFileDataBaseSave(Fd, File, 2)) {
                    ThrowError (1, "cannot export CAN-DB to file %s", File);
                }
                delete SortedItem;
            } else {
                ThrowError(1, "cannot export to file \"%s\"", File);
            }
        }
    }
}

void CanConfigDialog::ImportObjectSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if ((static_cast<CanConfigElement*>(Selected.at(0).internalPointer()))->GetType() == CanConfigElement::Variant) {
            CanConfigVariant *Variant = static_cast<CanConfigVariant*>(Selected.at(0).internalPointer());
            QString Filename = FileDialog::getOpenFileName(this,  QString("Import CAN object"), QString(), QString(CAN_EXT));
            if (!Filename.isEmpty()) {
                char File[MAX_PATH];
                StringCopyMaxCharTruncate (File, QStringToConstChar(Filename), MAX_PATH);
                File[MAX_PATH-1]= 0;
                int Fd = IniFileDataBaseOpen(File);
                if (Fd <= 0) {
                    ThrowError (1, "cannot import CAN object \"%s\"", File);
                } else {
                    if (IniFileDataBaseReadInt ("CAN/Global", "copy_buffer_type", 0, Fd) != 3) {
                        ThrowError (1, "cannot import CAN variant \"%s\" (there is no object defined in this file)", File);
                    } else {
                        CanConfigObject *Imported = new CanConfigObject(Variant);
                        if (Imported->ReadFromIni(9999, 9999, Fd)) {
                            m_CanConfigTreeModel->insertRow(0, Imported, Selected.at(0));
                        } else {
                            delete Imported;
                        }
                    }
                    IniFileDataBaseClose(Fd);
                }
            }
        }
    }
}

void CanConfigDialog::AppendVarianteSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if ((static_cast<CanConfigElement*>(Selected.at(0).internalPointer()))->GetType() == CanConfigElement::Variant) {
            CanConfigVariant *Variant = static_cast<CanConfigVariant*>(Selected.at(0).internalPointer());
            QString Filename = FileDialog::getOpenFileName(this,  QString("Append CAN variant"), QString(), QString(CAN_EXT));
            if (!Filename.isEmpty()) {
                char File[MAX_PATH];
                StringCopyMaxCharTruncate (File, QStringToConstChar(Filename), MAX_PATH);
                File[MAX_PATH-1]= 0;
                int Fd = IniFileDataBaseOpen(File);
                if (Fd <= 0) {
                    ThrowError (1, "cannot append CAN variant \"%s\"", File);
                } else {
                    if (IniFileDataBaseReadInt ("CAN/Global", "copy_buffer_type", 0, Fd) != 2) {
                        ThrowError (1, "cannot append CAN variant \"%s\" (there is no variant defined in this file)", File);
                    } else {
                        for (int o = 0; ; o++) {
                            CanConfigObject *Imported = new CanConfigObject(Variant);
                            if (Imported->ReadFromIni(9999, o, Fd)) {
                                Variant->AddChild(0, Imported);
                            } else {
                                delete Imported;
                                break;
                            }
                        }
                    }
                    IniFileDataBaseClose(Fd);
                }
            }
        }
    }
}

void CanConfigDialog::SwapObjectReadWriteSlot()
{

}

void CanConfigDialog::SwapObjecAndSignaltReadWriteSlot()
{

}

void CanConfigDialog::SortObjectsById()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    foreach(QModelIndex Index, Selected) {
        m_CanConfigTreeModel->SortObjectsById(Index);
    }
}

void CanConfigDialog::SortObjectsByName()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    foreach(QModelIndex Index, Selected) {
        m_CanConfigTreeModel->SortObjectsByName(Index);
    }
}

void CanConfigDialog::AddNewSignalSlot()
{
    QModelIndex ParentIndex = ui->TreeView->currentIndex();
    if (ParentIndex.isValid()) {
        m_CanConfigTreeModel->insertRow(0, ParentIndex);
        CanConfigElement *Element = static_cast<CanConfigElement*>(ParentIndex.internalPointer());
        if (Element->GetType() == CanConfigElement::Object) {
            CanConfigObject *CanObject = static_cast<CanConfigObject*>(Element);
            m_CanObjectModel->SetCanObject(CanObject);
        }
    }
}

void CanConfigDialog::DeleteObjectSlot()
{
    CopyObjectSlot();
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        QModelIndex ParentIndex = m_CanConfigTreeModel->parent(Selected.at(0));
        if (ParentIndex.isValid()) {
            /*if (ui->ObjectAllSignalsListView->rootIndex() == Selected.at(0)) {
                ui->ObjectAllSignalsListView->setRootIndex(QModelIndex());
            //ui->ObjectAllSignalsListView->reset();
            }*/
            m_CanConfigTreeModel->removeRow(Selected.at(0).row(), ParentIndex);
            // if deleted select parent
            ui->TreeView->selectionModel()->setCurrentIndex(ParentIndex, QItemSelectionModel::Select);
        }
    }
}

void CanConfigDialog::CopyObjectSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        CanConfigObject *Item = static_cast<CanConfigObject*>(Selected.at(0).internalPointer());
        m_CopyBuffer = new CanConfigObject(*Item);  // copy contructor!?
    }
}

void CanConfigDialog::PasteSignalSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if (m_CopyBuffer != nullptr) {
            if (m_CopyBuffer->GetType() == CanConfigElement::Signal) {
                m_CanConfigTreeModel->insertRow(0, m_CopyBuffer, Selected.at(0));
            }
        }
    }
}

void CanConfigDialog::ExportObjectSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if ((static_cast<CanConfigElement*>(Selected.at(0).internalPointer()))->GetType() == CanConfigElement::Object) {
            CanConfigObject *Item = static_cast<CanConfigObject*>(Selected.at(0).internalPointer());
            QString Filename = FileDialog::getSaveFileName(this,  QString("Export CAN object"), QString(), QString(CAN_EXT));
            if (!Filename.isEmpty()) {
                char File[MAX_PATH];
                StringCopyMaxCharTruncate (File, QStringToConstChar(Filename), MAX_PATH);
                File[MAX_PATH-1]= 0;
                int Fd = IniFileDataBaseCreateAndOpenNewIniFile(File);
                if (Fd > 0) {
                    IniFileDataBaseWriteString ("CAN/Global", "copy_buffer_type", "3", Fd);
                    Item->WriteToIni(9999, 9999, Fd);
                    if (IniFileDataBaseSave (Fd, File, 2)) {
                        ThrowError (1, "cannot export CAN-DB to file %s", File);
                    }
                } else {
                    ThrowError(1, "cannot export to file \"%s\"", File);
                }
            }
        }
    }
}

void CanConfigDialog::ImportSignalSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if ((static_cast<CanConfigElement*>(Selected.at(0).internalPointer()))->GetType() == CanConfigElement::Object) {
            CanConfigObject *Object = static_cast<CanConfigObject*>(Selected.at(0).internalPointer());
            QString Filename = FileDialog::getOpenFileName(this,  QString("Import CAN signal"), QString(), QString(CAN_EXT));
            if (!Filename.isEmpty()) {
                char File[MAX_PATH];
                StringCopyMaxCharTruncate (File, QStringToConstChar(Filename), MAX_PATH);
                File[MAX_PATH-1]= 0;
                int Fd = IniFileDataBaseOpen(File);
                if (Fd <= 0) {
                    ThrowError (1, "cannot import CAN signal \"%s\"", File);
                } else {
                    if (IniFileDataBaseReadInt ("CAN/Global", "copy_buffer_type", 0, Fd) != 4) {
                        ThrowError (1, "cannot import CAN variant \"%s\" (there is no signal defined in this file)", File);
                    } else {
                        CanConfigSignal *Imported = new CanConfigSignal(Object);
                        if (Imported->ReadFromIni(9999, 9999, 9999, Fd)) {
                            //Object->AddChild(Imported);
                            m_CanConfigTreeModel->insertRow(0, Imported, Selected.at(0));
                        } else {
                            delete Imported;
                        }
                    }
                    IniFileDataBaseClose(Fd);
                }
            }
        }
    }
}

void CanConfigDialog::DeleteSignalSlot()
{
    CopySignalSlot();
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        QModelIndex ParentIndex = m_CanConfigTreeModel->parent(Selected.at(0));
        if (ParentIndex.isValid()) {
            m_CanConfigTreeModel->removeRow(Selected.at(0).row(), ParentIndex);
            // Wenn geloescht Parent auswaehlen
            ui->TreeView->selectionModel()->setCurrentIndex(ParentIndex, QItemSelectionModel::Select);
        }
    }
}

void CanConfigDialog::CopySignalSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        CanConfigSignal *Item = static_cast<CanConfigSignal*>(Selected.at(0).internalPointer());
        m_CopyBuffer = new CanConfigSignal(*Item);  // copy contructor!?
    }
}

void CanConfigDialog::ExportSignalSlot()
{
    QModelIndexList Selected = ui->TreeView->GetSelectedIndexes();
    if (Selected.size() >= 1) {
        if ((static_cast<CanConfigElement*>(Selected.at(0).internalPointer()))->GetType() == CanConfigElement::Signal) {
            CanConfigSignal *Item = static_cast<CanConfigSignal*>(Selected.at(0).internalPointer());
            QString Filename = FileDialog::getSaveFileName(this,  QString("Export CAN signal"), QString(), QString(CAN_EXT));
            if (!Filename.isEmpty()) {
                char File[MAX_PATH];
                StringCopyMaxCharTruncate (File, QStringToConstChar(Filename), MAX_PATH);
                File[MAX_PATH-1]= 0;
                int Fd = IniFileDataBaseCreateAndOpenNewIniFile(File);
                if (Fd > 0) {
                    IniFileDataBaseWriteString ("CAN/Global", "copy_buffer_type", "4", Fd);
                    Item->WriteToIni(9999, 9999, 9999, Fd);
                    if (IniFileDataBaseSave(Fd, File, 2)) {
                        ThrowError (1, "cannot export CAN-DB to file %s", File);
                    }
                    IniFileDataBaseClose(Fd);
                } else {
                    ThrowError(1, "cannot export to file \"%s\"", File);
                }
            }
        }
    }
}

void CanConfigDialog::AddSignalBeforeOrBehind(int par_Offset)
{
    QModelIndex Current = ui->TreeView->currentIndex();
    if (Current.isValid()) {
        if ((static_cast<CanConfigElement*>(Current.internalPointer()))->GetType() == CanConfigElement::Signal) {
            //CanConfigSignal *Item = static_cast<CanConfigSignal*>(Current.internalPointer());
            QModelIndex ParentIndex = Current.parent();
            if (ParentIndex.isValid()) {
                m_CanConfigTreeModel->insertRow(Current.row() + par_Offset, ParentIndex);
                CanConfigElement *Element = static_cast<CanConfigElement*>(ParentIndex.internalPointer());
                if (Element->GetType() == CanConfigElement::Object) {
                    CanConfigObject *CanObject = static_cast<CanConfigObject*>(Element);
                    m_CanObjectModel->SetCanObject(CanObject);
                }
            }
        }
    }
}

void CanConfigDialog::AddSignalBeforeSlot()
{
    AddSignalBeforeOrBehind(0);
}

void CanConfigDialog::AddSignalBehindSlot()
{
    AddSignalBeforeOrBehind(1);
}


void CanConfigDialog::PasteSignalBeforeOrBehind(int par_Offset)
{
    QModelIndex Current = ui->TreeView->currentIndex();
    if (Current.isValid()) {
        if ((static_cast<CanConfigElement*>(Current.internalPointer()))->GetType() == CanConfigElement::Signal) {
            QModelIndex ParentIndex = Current.parent();
            if (ParentIndex.isValid()) {
                if (m_CopyBuffer != nullptr) {
                    if (m_CopyBuffer->GetType() == CanConfigElement::Signal) {
                        m_CanConfigTreeModel->insertRow(Current.row() + par_Offset, m_CopyBuffer, ParentIndex);
                    }
                }
            }
        }
    }
}

void CanConfigDialog::PasteSignalBeforeSlot()
{
    PasteSignalBeforeOrBehind(0);
}

void CanConfigDialog::PasteSignalBehindSlot()
{
    PasteSignalBeforeOrBehind(1);
}

void CanConfigDialog::SelectSignalSlot(QModelIndex &par_SignalIndex)
{
     //QModelIndex Index = m_CanConfigTreeModel->GetIndexOf(Signal);
    ui->TreeView->setCurrentIndex(par_SignalIndex);
}

void CanConfigDialog::accept()
{
    QModelIndex SelectedItem = ui->TreeView->currentIndex();
    if (SelectedItem.isValid()) {
        CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectedItem);
        Item->StoreDialogTab(ui);
    }
    QDialog::accept();
}

void CanConfigDialog::createActions()
{
    bool Ret;

    // Mapping Tree-View Context Menu Event an Dialog umleiten
    ui->MappingTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    Ret = connect(ui->MappingTreeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(MappingTreeContextMenuRequestedSlot(const QPoint &)));

    // CAN-Tree-View Section Changed weiterleiten an Dialog
    Ret = connect(ui->TreeView, SIGNAL(selectionChangedSignal(const QItemSelection &, const QItemSelection &)), this, SLOT(selectionChanged(const QItemSelection &, const QItemSelection &)));

    // Tree-View Context Menu Event an Dialog umleiten
    ui->TreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    Ret = connect(ui->TreeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(CanTreeContextMenuRequestedSlot(const QPoint &)));

    m_AddNewVariantAct = new QAction(tr("&Add new variante"), this);
    Ret = connect(m_AddNewVariantAct, SIGNAL(triggered()), this, SLOT(AddNewVariantSlot()));
    m_PasteVariantAct = new QAction(tr("&Paste variante"), this);
    Ret = connect(m_PasteVariantAct, SIGNAL(triggered()), this, SLOT(PasteVariantSlot()));
    m_ImportVariantAct = new QAction(tr("&Import variante"), this);
    Ret = connect(m_ImportVariantAct, SIGNAL(triggered()), this, SLOT(ImportVariantSlot()));
    m_SearchVariableAct = new QAction(tr("&Search varible"), this);
    Ret = connect(m_SearchVariableAct, SIGNAL(triggered()), this, SLOT(SearchVariableSlot()));

    m_AddNewObjectAct = new QAction(tr("&Add new object"), this);
    Ret = connect(m_AddNewObjectAct, SIGNAL(triggered()), this, SLOT(AddNewObjectSlot()));
    m_DeleteVarianteAct = new QAction(tr("&Delete variante"), this);
    Ret = connect(m_DeleteVarianteAct, SIGNAL(triggered()), this, SLOT(DeleteVarianteSlot()));
    m_CopyVarianteAct = new QAction(tr("&Copy variante"), this);
    Ret = connect(m_CopyVarianteAct, SIGNAL(triggered()), this, SLOT(CopyVarianteSlot()));
    m_PasteObjectAct = new QAction(tr("&Paste object"), this);
    Ret = connect(m_PasteObjectAct, SIGNAL(triggered()), this, SLOT(PasteObjectSlot()));
    m_ExportVarianteAct = new QAction(tr("&export variante"), this);
    Ret = connect(m_ExportVarianteAct, SIGNAL(triggered()), this, SLOT(ExportVarianteSlot()));
    m_ExportSortedVarianteAct = new QAction(tr("&export sorted variante"), this);
    Ret = connect(m_ExportSortedVarianteAct, SIGNAL(triggered()), this, SLOT(ExportSortedVarianteSlot()));
    m_ImportObjectAct = new QAction(tr("&import object"), this);
    Ret = connect(m_ImportObjectAct, SIGNAL(triggered()), this, SLOT(ImportObjectSlot()));
    m_AppendVarianteAct = new QAction(tr("&append variante"), this);
    Ret = connect(m_AppendVarianteAct, SIGNAL(triggered()), this, SLOT(AppendVarianteSlot()));
    m_SwapObjectReadWriteAct = new QAction(tr("&swap object read/write"), this);
    Ret = connect(m_SwapObjectReadWriteAct, SIGNAL(triggered()), this, SLOT(SwapObjectReadWriteSlot()));
    m_SwapObjecAndSignaltReadWriteAct = new QAction(tr("swap object and signals read/write"), this);
    Ret = connect(m_SwapObjecAndSignaltReadWriteAct, SIGNAL(triggered()), this, SLOT(SwapObjecAndSignaltReadWriteSlot()));

    m_SortOjectsByIdAct = new QAction(tr("sort objects by identifier"), this);
    Ret = connect(m_SortOjectsByIdAct, SIGNAL(triggered()), this, SLOT(SortObjectsById()));
    m_SortOjectsByNameAct = new QAction(tr("sort objects by name"), this);
    Ret = connect(m_SortOjectsByNameAct, SIGNAL(triggered()), this, SLOT(SortObjectsByName()));

    m_AddNewSignalAct = new QAction(tr("Add new signal"), this);
    Ret = connect(m_AddNewSignalAct, SIGNAL(triggered()), this, SLOT(AddNewSignalSlot()));
    m_DeleteObjectAct = new QAction(tr("Delete object"), this);
    Ret = connect(m_DeleteObjectAct, SIGNAL(triggered()), this, SLOT(DeleteObjectSlot()));
    m_CopyObjectAct = new QAction(tr("Copy object"), this);
    Ret = connect(m_CopyObjectAct, SIGNAL(triggered()), this, SLOT(CopyObjectSlot()));
    m_PasteSignalAct = new QAction(tr("Paste signal"), this);
    Ret = connect(m_PasteSignalAct, SIGNAL(triggered()), this, SLOT(PasteSignalSlot()));
    m_ExportObjectAct = new QAction(tr("export object"), this);
    Ret = connect(m_ExportObjectAct, SIGNAL(triggered()), this, SLOT(ExportObjectSlot()));
    m_ImportSignalAct = new QAction(tr("import signal"), this);
    Ret = connect(m_ImportSignalAct, SIGNAL(triggered()), this, SLOT(ImportSignalSlot()));

    m_DeleteSignalAct = new QAction(tr("Delete signal"), this);
    Ret = connect(m_DeleteSignalAct, SIGNAL(triggered()), this, SLOT(DeleteSignalSlot()));
    m_CopySignalAct = new QAction(tr("Copy signal"), this);
    Ret = connect(m_CopySignalAct, SIGNAL(triggered()), this, SLOT(CopySignalSlot()));
    m_ExportSignalAct = new QAction(tr("export signal"), this);
    Ret = connect(m_ExportSignalAct, SIGNAL(triggered()), this, SLOT(ExportSignalSlot()));

    m_AddBeforeSignalAct = new QAction(tr("Add new signal before"), this);
    Ret = connect(m_AddBeforeSignalAct, SIGNAL(triggered()), this, SLOT(AddSignalBeforeSlot()));
    m_AddBehindSignalAct = new QAction(tr("Add new signal behind"), this);
    Ret = connect(m_AddBehindSignalAct, SIGNAL(triggered()), this, SLOT(AddSignalBehindSlot()));
    m_PasteBeforeSignalAct = new QAction(tr("Paste signal before"), this);
    Ret = connect(m_PasteBeforeSignalAct, SIGNAL(triggered()), this, SLOT(PasteSignalBeforeSlot()));
    m_PasteBehindSignalAct = new QAction(tr("Paste signal behind"), this);
    Ret = connect(m_PasteBehindSignalAct, SIGNAL(triggered()), this, SLOT(PasteSignalBehindSlot()));
}

CanConfigBase::CanConfigBase()
{
    CanConfigBase(false);
}

CanConfigBase::CanConfigBase(bool par_OnlyUsedVariants)
{
    m_Server = new CanConfigServer(this, par_OnlyUsedVariants);
}

CanConfigBase::~CanConfigBase()
{
    delete m_Server;
}

void CanConfigBase::ReadFromIni()
{
    m_Server->ReadFromIni();
}

void CanConfigBase::WriteToIni(bool par_Delete)
{
    m_Server->WriteToIni(par_Delete);
}

int CanConfigBase::GetNumberOfChannels()
{
   return  m_Server->m_NumberOfChannels;
}

CanConfigElement *CanConfigBase::child(int par_Number)
{
    Q_UNUSED(par_Number)
    return m_Server;
}

int CanConfigBase::childCount()
{
     return 1;
}

int CanConfigBase::columnCount()
{
    return 2;
}

QVariant CanConfigBase::data(int par_Column)
{
    switch (par_Column) {
    case 0:
        return QString ("Server");
    case 1:  // Order
    case 2:  // Description
        return QString ("");
    default:
        return QVariant ();
    }
}

CanConfigElement *CanConfigBase::parent()
{
     return nullptr;  // ist das Root-Element!
}

int CanConfigBase::childNumber()
{
    return 0; // ist das Root-Element!
}

bool CanConfigBase::setData(int par_Column, const QVariant &par_Value)
{
    Q_UNUSED(par_Column)
    Q_UNUSED(par_Value)
    return false;
}

CanConfigElement::Type CanConfigBase::GetType()
{
    return Base;
}

void CanConfigBase::FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index)
{
    Q_UNUSED(ui)
    Q_UNUSED(par_Index)
    // nothing to do
}

void CanConfigBase::StoreDialogTab(Ui::CanConfigDialog *ui)
{
    Q_UNUSED(ui)
    // nothing to do
}

void CanConfigBase::AddNewChild(int par_Row)
{
    Q_UNUSED(par_Row)
    // nothing to do
}

void CanConfigBase::AddChild(int par_Row, CanConfigElement *par_Child)
{
    Q_UNUSED(par_Row)
    Q_UNUSED(par_Child)
    // nothing to do
}

void CanConfigBase::RemoveChild(int par_Row)
{
    Q_UNUSED(par_Row)
    // nothing to do
}

int CanConfigBase::GetIndexOfChild(CanConfigServer *par_Server)
{
    Q_UNUSED(par_Server)
    return 0;
}

bool CanConfigBase::ShouldRestartCanImmediately()
{
    if (m_Server != nullptr) {
        return m_Server->m_RestartCanImmediately;
    } else {
        return false;
    }
}


CanConfigServer::CanConfigServer(CanConfigBase *par_ParentBase, bool par_OnlyUsedVariants)
{
    m_ParentBase = par_ParentBase;
    m_NumberOfChannels = 0;
    m_DoNotUseUnits = false;
    m_RestartCanImmediately = false;
    m_OnlyUsedVariants = par_OnlyUsedVariants;
}


CanConfigServer::~CanConfigServer()
{
    foreach (CanConfigVariant *Variant, m_Variants) {
        delete Variant;
    }
}

void CanConfigServer::ReadFromIni()
{
    char section[INI_MAX_SECTION_LENGTH];
    char entry[INI_MAX_ENTRYNAME_LENGTH];
    char txt2[INI_MAX_LINE_LENGTH];
    int Fd = GetMainFileDescriptor();

    sprintf (section, "CAN/Global");
    m_NumberOfChannels = GetCanControllerCountFromIni (section, Fd);
    m_DoNotUseUnits = IniFileDataBaseReadInt (section, "dont use units for blackboard", 0, Fd);
    m_RestartCanImmediately = IniFileDataBaseReadInt (section, "restart CAN immediately", 0, Fd);

    m_EnableGatewayDeviceDriver = IniFileDataBaseReadYesNo (section, "enable gateway device driver", 0, Fd);
    if (m_EnableGatewayDeviceDriver) {
        m_EnableGatewayVirtualDeviceDriver = IniFileDataBaseReadUInt (section, "enable gateway virtual device driver", 0x1, Fd);
        m_EnableGatewayRealDeviceDriver = IniFileDataBaseReadUInt (section, "enable gateway real device driver", 0x1, Fd);
        QString Section(section);
        QString Entry("enable gateway virtual device driver equ");
        m_EnableGatewayirtualDeviceDriverEqu = ScQt_IniFileDataBaseReadString(Section, Entry, "", ScQt_GetMainFileDescriptor());
        Entry = QString("enable gateway real device driver equ");
        m_EnableGatewayRealDeviceDriverEqu = ScQt_IniFileDataBaseReadString(Section, Entry, "", ScQt_GetMainFileDescriptor());
    } else {
        m_EnableGatewayVirtualDeviceDriver = 1;   // 1 -> yes
        m_EnableGatewayRealDeviceDriver = 1;
    }

    QList<int> UsedVariants;
    for (int i = 0; i < m_NumberOfChannels; i++) {
        sprintf (entry, "can_controller%i_variante", i+1);
        IniFileDataBaseReadString (section, entry, "", txt2, sizeof (txt2), Fd);
        char *p = txt2;
        while (isdigit (*p)) {
            int v = strtol (p, &p, 0);
            if (*p == ',') p++;
            UsedVariants.append(v);
        }
    }

    QList<int> IndexOfVariantNos;

    // Read the variants
    for (int VariantNr = 0; ; VariantNr++) {
        sprintf (section, "CAN/Variante_%i", VariantNr);
        if (!IniFileDataBaseLookIfSectionExist(section, Fd)) break;   // no more variants
        if (m_OnlyUsedVariants) {
            if (UsedVariants.indexOf(VariantNr) < 0) {
                continue;   // variant is not connected to a channel
            }
        }
        IndexOfVariantNos.append(VariantNr);
        CanConfigVariant *NewVariant = new CanConfigVariant(this);
        NewVariant->ReadFromIni(VariantNr, Fd);
        m_Variants.append(NewVariant);
    }

    sprintf (section, "CAN/Global");
    for (int i = 0; i < m_NumberOfChannels; i++) {
        sprintf (entry, "can_controller%i_startup_state", i+1);
        int StartupState = IniFileDataBaseReadInt("CAN/Global", entry, 1, Fd);
        m_ChannelStartupState.append(StartupState);
        sprintf (entry, "can_controller%i_variante", i+1);
        IniFileDataBaseReadString (section, entry, "", txt2, sizeof (txt2), Fd);
        char *p = txt2;
        while (isdigit (*p)) {
            int v = strtol (p, &p, 0);
            if (*p == ',') p++;
            int Index = IndexOfVariantNos.indexOf(v);
            if ((Index >= 0) && (Index < m_Variants.size())) {
                CanConfigVariant *Variant = m_Variants.at(Index);
                Variant->AddToChannel(i+1);
            }
        }
    }
}

void CanConfigServer::WriteToIni(bool par_Delete)
{
    char section[INI_MAX_SECTION_LENGTH];
    char entry[INI_MAX_ENTRYNAME_LENGTH];
    char txt[INI_MAX_LINE_LENGTH];
    int Fd = GetMainFileDescriptor();

    sprintf (section, "CAN/Global");

    IniFileDataBaseWriteInt (section, "can_controller_count_ext", m_NumberOfChannels, Fd);
    IniFileDataBaseWriteInt (section, "can_controller_count", (m_NumberOfChannels > 4) ? 4 : m_NumberOfChannels, Fd);

    IniFileDataBaseWriteInt (section, "can_varianten_count", m_Variants.size(), Fd);

    IniFileDataBaseWriteString (section, "dont use units for blackboard", m_DoNotUseUnits ? "1" : "0", Fd);
    IniFileDataBaseWriteString (section, "restart CAN immediately", m_RestartCanImmediately ? "1" : "0", Fd);

    IniFileDataBaseWriteYesNo (section, "enable gateway device driver", m_EnableGatewayDeviceDriver, Fd);
    if (m_EnableGatewayDeviceDriver) {
        IniFileDataBaseWriteUInt (section, "enable gateway virtual device driver", m_EnableGatewayVirtualDeviceDriver, Fd);
        IniFileDataBaseWriteUInt (section, "enable gateway real device driver",m_EnableGatewayRealDeviceDriver, Fd);
        QString Section(section);
        QString Entry("enable gateway virtual device driver equ");
        ScQt_IniFileDataBaseWriteString(Section, Entry, m_EnableGatewayirtualDeviceDriverEqu, Fd);
        Entry = QString("enable gateway real device driver equ");
        ScQt_IniFileDataBaseWriteString(Section, Entry, m_EnableGatewayRealDeviceDriverEqu, Fd);
    }
    // Save the variante into the INI-DB
    for (int VariantNr = 0; VariantNr < m_Variants.size(); VariantNr++) {
        m_Variants.at(VariantNr)->WriteToIni(VariantNr, Fd, par_Delete);
    }

    for (int c = 0; c < m_NumberOfChannels; c++) {
        sprintf (entry, "can_controller%i_startup_state", c+1);
        IniFileDataBaseWriteInt("CAN/Global", entry, m_ChannelStartupState.at(c), Fd);
        sprintf (entry, "can_controller%i_variante", c+1);
        strcpy (txt, "");
        for (int v = 0; v < m_Variants.size(); v++) {
            CanConfigVariant *Variant = m_Variants.at(v);
            if (Variant->IsConnectedToChannel(c+1)) {
                if (txt[0] != 0) strcat (txt, ",");
                sprintf (txt + strlen(txt), "%i", v);
            }
        }
        IniFileDataBaseWriteString (section, entry, txt, Fd);
    }
}

CanConfigElement *CanConfigServer::child(int par_Number)
{
    return this->m_Variants.at(par_Number);
}

int CanConfigServer::childCount()
{
     return this->m_Variants.size();
}

int CanConfigServer::columnCount()
{
    return 2;
}

QVariant CanConfigServer::data(int par_Column)
{
    switch (par_Column) {
    case 0:
        return QString ("Server");
    case 1:  // Order
    case 2:  // Description
        return QString ("");
    default:
        return QVariant ();
    }
}

CanConfigElement *CanConfigServer::parent()
{
     return m_ParentBase;
}

int CanConfigServer::childNumber()
{
    return 0; // ist das Root-Element!
}

bool CanConfigServer::setData(int par_Column, const QVariant &par_Value)
{
    Q_UNUSED(par_Column)
    Q_UNUSED(par_Value)
    return false;
}

CanConfigElement::Type CanConfigServer::GetType()
{
    return Server;
}

#ifdef _WIN32
static int GetGatewayFlags(Ui::CanConfigDialog *ui)
{
    QString Equ;
    double Value;
    int Ret = 0;
    switch (ui->VirtualGatewayComboBox->currentIndex()) {
    default:
    case 1: // yes
        Ret |= 1;
        break;
    case 0: // no
        break;
    case 2: // equ
        Equ = ui->VirtualGatewayLineEdit->text();
        direct_solve_equation_err_sate(QStringToConstChar(Equ), &Value);
        Ret |= (Value > 0.5) ? 1 : 0;
        break;
    }
    switch (ui->RealGatewayComboBox->currentIndex()) {
    default:
    case 1: // yes
        Ret |= 2;
        break;
    case 0: // no
        break;
    case 2: // equ
        Equ = ui->RealGatewayLineEdit->text();
        direct_solve_equation_err_sate(QStringToConstChar(Equ), &Value);
        Ret |= (Value > 0.5) ? 2 : 0;
        break;
    }
    return Ret;
}
#endif

static void FillCANCardList(Ui::CanConfigDialog *ui)
{
    ui->FoundCANCardslistWidget->clear();
    if (s_main_ini_val.ConnectToRemoteMaster) {
        for (int c = 0; c < MAX_CAN_CHANNELS; c++) {
            QString Name = QString("%1.  ").arg(c);
            Name.append(QString(GetCanCardName (c)));
            ui->FoundCANCardslistWidget->addItem(Name);
        }
        EnableGatewayFlags(ui, false);
    } else {
        int x = 0;
#if defined(_WIN32) && defined(VECTROR_VIRTUAL_CAN)
        if (ui->EnableGatewayDeviceDriverCheckBox->isChecked()) {
            InitGatewayDeviceDriver(GetGatewayFlags(ui));
            for ( ; x < GetGatewayDeviceDriverCount(); x++) {
                char Name[32];
                uint32_t DriverVersion;
                uint32_t DllVersion;
                uint32_t InterfaceVersion;
                GetGatewayDeviceDriverInfos(x, Name, 32, &DriverVersion, &DllVersion, &InterfaceVersion);
                char Text[256];
                sprintf(Text, "%i.  Gateway %s (versions: DLL 0x%X, driver 0x%X, interface 0x%X)", x, Name, DllVersion, DriverVersion, InterfaceVersion);
                ui->FoundCANCardslistWidget->addItem(Text);
            }
            EnableGatewayFlags(ui, true);
        } else {
            EnableGatewayFlags(ui, false);
        }
#endif
        for ( ; x < MAX_CAN_CHANNELS; x++) {
            char Text[256];
            sprintf(Text, "%i.  virtual can device", x);
            ui->FoundCANCardslistWidget->addItem(Text);
        }
    }
}

void CanConfigServer::FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index)
{
    Q_UNUSED(par_Index)
    EnableTab(ui->tabWidget, 0);
    ui->tabWidget->setCurrentIndex(0);  // 0 -> Server-Tab
    // this must be before FillCANCardList
    ui->EnableGatewayDeviceDriverCheckBox->setCheckState(this->m_EnableGatewayDeviceDriver ?  Qt::Checked : Qt::Unchecked);
    ui->VirtualGatewayLineEdit->setText(this->m_EnableGatewayirtualDeviceDriverEqu);
    ui->RealGatewayLineEdit->setText(this->m_EnableGatewayRealDeviceDriverEqu);
    ui->VirtualGatewayComboBox->setCurrentIndex(this->m_EnableGatewayVirtualDeviceDriver);
    ui->RealGatewayComboBox->setCurrentIndex(this->m_EnableGatewayRealDeviceDriver);
    FillCANCardList(ui);
    ui->NumberOfChannelsSpinBox->setMaximum(MAX_CAN_CHANNELS);
    ui->NumberOfChannelsSpinBox->setValue(this->m_NumberOfChannels);
    ui->DoNotUseUnitsCheckBox->setCheckState(this->m_DoNotUseUnits ?  Qt::Checked : Qt::Unchecked);
    ui->RestartCanImmediatelyCheckBox->setCheckState(this->m_RestartCanImmediately ?  Qt::Checked : Qt::Unchecked);

    UpdateChannelMapping (ui);
}


void CanConfigServer::UpdateChannelMapping(Ui::CanConfigDialog *ui)
{
    ui->MappingTreeWidget->clear();
    m_ChannelVariant.clear();
    int i = 0;
    for (int c = 0; c < m_NumberOfChannels; c++) {
        QStringList ChannelLine;
        ChannelLine.append(QString().number(c));
        ChannelLine.append(QString());
        if (m_ChannelStartupState.size() > c) {
            switch(m_ChannelStartupState.at(c)) {
            case 0:
                ChannelLine.append(QString("OFF"));
                break;
            default:
            case 1:
                ChannelLine.append(QString("ON"));
                break;
            case 2:
                ChannelLine.append(QString("IF_NOT_EXIST_OFF"));
                break;
            case 3:
                ChannelLine.append(QString("IF_NOT_EXIST_ON"));
                break;
            }
        } else {
            ChannelLine.append(QString("ON"));  // new one are by default enabled
        }
        QTreeWidgetItem *ChannelItem = new QTreeWidgetItem(ui->MappingTreeWidget, ChannelLine);
        ChannelItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        ui->MappingTreeWidget->addTopLevelItem(ChannelItem);
        foreach (CanConfigVariant *Variant, m_Variants) {
            if (Variant->IsConnectedToChannel(c+1)) {
                QStringList VariantLine;
                VariantLine.append(Variant->GetName());
                VariantLine.append(Variant->GetDescription());
                VariantLine.append(QString()); // no data for the variant inside the startup state column
                VariantLine.append(QString().number(i));  // 4. column will be used to save the index inside in m_ChannelVariant verwendet and must be invisible
                QTreeWidgetItem *VariantItem = new QTreeWidgetItem(ChannelItem, VariantLine);
                m_ChannelVariant.insert(i, Variant);
                ChannelItem->addChild(VariantItem);
                i++;
            }
        }
        ChannelItem->setExpanded(true);
    }
}

void CanConfigServer::StoreDialogTab(Ui::CanConfigDialog *ui)
{
    this->m_DoNotUseUnits = ui->DoNotUseUnitsCheckBox->isChecked();
    this->m_RestartCanImmediately = ui->RestartCanImmediatelyCheckBox->isChecked();
    this->m_EnableGatewayDeviceDriver = ui->EnableGatewayDeviceDriverCheckBox->isChecked();
    this->m_EnableGatewayVirtualDeviceDriver = ui->VirtualGatewayComboBox->currentIndex();
    this->m_EnableGatewayirtualDeviceDriverEqu = ui->VirtualGatewayLineEdit->text();
    this->m_EnableGatewayRealDeviceDriver = ui->RealGatewayComboBox->currentIndex();
    this->m_EnableGatewayRealDeviceDriverEqu = ui->RealGatewayLineEdit->text();
    m_ChannelStartupState.clear();
    for (int c = 0; c < ui->MappingTreeWidget->topLevelItemCount(); c++) {
        QString StartupState = ui->MappingTreeWidget->model()->index(c, 2).data(Qt::DisplayRole).toString();
        if (!StartupState.compare("OFF")) {
            m_ChannelStartupState.append(0);
        } else if (!StartupState.compare("ON")) {
            m_ChannelStartupState.append(1);
        } else if (!StartupState.compare("IF_NOT_EXIST_OFF")) {
            m_ChannelStartupState.append(2);
        } else if (!StartupState.compare("IF_NOT_EXIST_ON")) {
            m_ChannelStartupState.append(3);
        } else {
            m_ChannelStartupState.append(1);
        }
    }
}

void CanConfigServer::AddNewChild(int par_Row)
{
    Q_UNUSED(par_Row)
    QString NewName = GenerateUniqueName(QString("new"));
    m_Variants.insert(0, new CanConfigVariant(this, NewName));
}

QString CanConfigServer::GenerateUniqueName(QString par_Name)
{
    QString NewName;
    bool InUse = true;
    NewName = par_Name;
    for (int i = 0; ; i++, NewName = QString(par_Name).append(QString("_%1").arg(i))) {
        InUse = false;
        foreach (CanConfigVariant *Variant, m_Variants) {
            if (!NewName.compare(Variant->GetName())) {
                InUse = true;
            }
        }
        if (!InUse) break;
    }
    return NewName;
}

void CanConfigServer::AddChild(int par_Row, CanConfigElement *par_Child)
{
    CanConfigVariant *NewVariant = new CanConfigVariant(*static_cast<CanConfigVariant*>(par_Child));
    NewVariant->SetName(GenerateUniqueName(NewVariant->GetName()));
    NewVariant->SetParent(this);
    m_Variants.insert(par_Row, NewVariant);
}

void CanConfigServer::RemoveChild(int par_Row)
{
    CanConfigVariant *Variant = m_Variants.at(par_Row);
    m_Variants.removeAt(par_Row);
    delete Variant;
}

int CanConfigServer::GetIndexOfChild(CanConfigVariant *par_Variant)
{
    return m_Variants.indexOf(par_Variant);
}

void CanConfigServer::AddVariantToChannel(int par_ChannelNr, CanConfigVariant *par_Variant)
{
    par_Variant->AddToChannel(par_ChannelNr+1);
}

void CanConfigServer::RemoveVariantFromChannel(int par_IndexNr, int par_ChannelNr)
{
    CanConfigVariant *v = m_ChannelVariant.value(par_IndexNr, nullptr);
    v->RemoveFromChannel(par_ChannelNr+1);
}

QList<CanConfigSignal *> CanConfigServer::SearchSignal(const QRegularExpression &par_RegExp)
{
    QList<CanConfigSignal*> Ret;
    for (int v = 0; v < m_Variants.size(); v++) {
        Ret.append(m_Variants.at(v)->SearchSignal(par_RegExp));
    }
    return Ret;
}

int CanConfigServer::GetCanControllerCountFromIni (char *par_Section, int par_Fd)
{
    int can_controller_count;
    int can_controller_count_ext;
    int ret;

    can_controller_count_ext = IniFileDataBaseReadInt (par_Section, "can_controller_count_ext", 0, par_Fd);
    can_controller_count = IniFileDataBaseReadInt (par_Section, "can_controller_count", 0, par_Fd);

    if (can_controller_count_ext >= 4) ret = can_controller_count_ext;
    else ret = can_controller_count;

    if (ret > MAX_CAN_CHANNELS) ret = MAX_CAN_CHANNELS;
    return ret;
}

void CanConfigServer::SetCanControllerCountToIni (char *par_Section, int par_Fd, int par_CanControllerCount)
{
    int can_controller_count;
    int can_controller_count_ext;

    if (par_CanControllerCount > MAX_CAN_CHANNELS) par_CanControllerCount = MAX_CAN_CHANNELS;

    if (par_CanControllerCount < 4) can_controller_count = par_CanControllerCount;
    else can_controller_count = 4;
    can_controller_count_ext = par_CanControllerCount;
    IniFileDataBaseWriteInt (par_Section, "can_controller_count", can_controller_count, par_Fd);
    IniFileDataBaseWriteInt (par_Section, "can_controller_count_ext", can_controller_count_ext, par_Fd);
}


CanConfigVariant::CanConfigVariant(CanConfigServer *par_ParentBase, QString par_Name)
{
    m_Name = par_Name;
    m_ParentServer = par_ParentBase;
    m_BaudRate = 500;
    m_SamplePoint = 0.75;
    m_CanFdEnable = false;
    m_DataBaudRate = 4000;
    m_DataSamplePoint = 0.8;
    m_EnableJ1939 = false;
    for (int x = 0; x < 4; x++) {
        m_EnableJ1939Addresses[x] = false;
        m_J1939Addresses[x] = 0;
    }
    m_ControlBlackboardName = PrefixId;
}

CanConfigVariant::CanConfigVariant(const CanConfigVariant &par_Variant)
{
    m_Name = par_Variant.m_Name;
    m_Description = par_Variant.m_Description;
    m_BaudRate = par_Variant.m_BaudRate;
    m_SamplePoint = par_Variant.m_SamplePoint;
    m_CanFdEnable = par_Variant.m_CanFdEnable;
    m_DataBaudRate = par_Variant.m_DataBaudRate;
    m_DataSamplePoint = par_Variant.m_DataSamplePoint;
    m_EnableJ1939 = par_Variant.m_EnableJ1939;
    for (int x = 0; x < 4; x++) {
        m_EnableJ1939Addresses[x] = par_Variant.m_EnableJ1939Addresses[x];
        m_J1939Addresses[x] = par_Variant.m_J1939Addresses[x];
    }
    m_ControlBlackboardName = par_Variant.m_ControlBlackboardName;

    foreach (CanConfigObject *Object, par_Variant.m_Objects) {
        CanConfigObject *NewObject = new CanConfigObject(*Object);
        NewObject->SetParent(this);
        m_Objects.append(NewObject);
    }
    m_ParentServer = nullptr;

}

CanConfigVariant::~CanConfigVariant()
{
    foreach (CanConfigObject *Object, m_Objects) {
        delete Object;
    }
}

bool CanConfigVariant::ReadFromIni(int par_VariantNr, int par_Fd)
{
    char section[INI_MAX_SECTION_LENGTH];
    char entry[32];
    char txt[INI_MAX_LINE_LENGTH];
    int x;

    sprintf (section, "CAN/Variante_%i", par_VariantNr);
    if (!IniFileDataBaseLookIfSectionExist(section, par_Fd)) return false;  // This variante don't exist
    IniFileDataBaseReadString (section, "name", "", txt, sizeof (txt), par_Fd);
    m_Name = CharToQString(txt);
    IniFileDataBaseReadString (section, "desc", "", txt, sizeof (txt), par_Fd);
    m_Description = CharToQString(txt);

    m_BaudRate = IniFileDataBaseReadInt (section, "baud rate", 500, par_Fd);
    m_SamplePoint = IniFileDataBaseReadFloat (section, "sample point", 0.75, par_Fd);
    m_CanFdEnable = IniFileDataBaseReadYesNo (section, "can fd", 0, par_Fd);
    m_DataBaudRate = IniFileDataBaseReadInt (section, "data baud rate", 4000, par_Fd);
    m_DataSamplePoint = IniFileDataBaseReadFloat (section, "data sample point", 0.8, par_Fd);

    m_EnableJ1939 = IniFileDataBaseReadYesNo(section, "j1939", 0, par_Fd);

    for (x = 0; x < 4; x++) {
        sprintf (entry, "j1939 src addr %i", x);
        if (IniFileDataBaseReadString (section, entry, "", txt, sizeof (txt), par_Fd) > 0) {
            m_EnableJ1939Addresses[x] = true;
            m_J1939Addresses[x] = strtol (txt, nullptr, 0);
        } else {
            m_EnableJ1939Addresses[x] = false;
            m_J1939Addresses[x] = 0;
        }
    }
    IniFileDataBaseReadString (section, "ControlBbName", "PrefixId", txt, sizeof (txt), par_Fd);
    if (!strcmpi ("NoPrefixObjName", txt)) {
        m_ControlBlackboardName = NoPrefixObjName;
    } else if (!strcmpi ("PrefixObjName", txt)) {
        m_ControlBlackboardName = PrefixObjName;
    }  else if (!strcmpi ("NoPrefixVarObjName", txt)) {
        m_ControlBlackboardName = NoPrefixVarObjName;
    } else if (!strcmpi ("PrefixVarObjName", txt)) {
        m_ControlBlackboardName = PrefixVarObjName;
    } else {
        m_ControlBlackboardName = PrefixId;
    }

    // read objects
    for (int ObjectNr = 0; ; ObjectNr++) {
        CanConfigObject *NewObject = new CanConfigObject(this);
        if (NewObject->ReadFromIni(par_VariantNr, ObjectNr, par_Fd)) {
            m_Objects.append(NewObject);
        } else {
            delete NewObject;
            break;
        }
    }
    foreach(CanConfigObject* Object, m_Objects)  {
        Object->CheckExistanceOfAll_C_PGs();
    }

    return true;
}

void CanConfigVariant::WriteToIni(int par_VariantNr, int par_Fd, bool par_Delete)
{
    char section[INI_MAX_SECTION_LENGTH];
    char entry[32];
    int x;

    sprintf (section, "CAN/Variante_%i", par_VariantNr);
    IniFileDataBaseWriteString(section, nullptr, nullptr, par_Fd);
    if (par_Delete) return;
    IniFileDataBaseWriteString(section, "name", QStringToConstChar(m_Name), par_Fd);
    IniFileDataBaseWriteString(section, "desc", QStringToConstChar(m_Description), par_Fd);
    IniFileDataBaseWriteInt (section, "baud rate", m_BaudRate, par_Fd);
    IniFileDataBaseWriteFloat (section, "sample point", m_SamplePoint, par_Fd);
    IniFileDataBaseWriteYesNo (section, "can fd", m_CanFdEnable, par_Fd);
    IniFileDataBaseWriteInt (section, "data baud rate", m_DataBaudRate, par_Fd);
    IniFileDataBaseWriteFloat (section, "data sample point", m_DataSamplePoint, par_Fd);

    IniFileDataBaseWriteYesNo(section, "j1939", m_EnableJ1939, par_Fd);

    for (x = 0; x < 4; x++) {
        if (m_EnableJ1939Addresses[x]) {
            sprintf (entry, "j1939 src addr %i", x);
            IniFileDataBaseWriteUInt (section, entry,  m_J1939Addresses[x], par_Fd);
        }
    }
    switch (m_ControlBlackboardName) {
    case NoPrefixObjName:
        IniFileDataBaseWriteString (section, "ControlBbName", "NoPrefixObjName", par_Fd);
        break;
    case PrefixObjName:
        IniFileDataBaseWriteString (section, "ControlBbName", "PrefixObjName", par_Fd);
        break;
    case NoPrefixVarObjName:
        IniFileDataBaseWriteString (section, "ControlBbName", "NoPrefixVarObjName", par_Fd);
        break;
    case PrefixVarObjName:
        IniFileDataBaseWriteString (section, "ControlBbName", "PrefixVarObjName", par_Fd);
        break;
    case PrefixId:
    //default:
        IniFileDataBaseWriteString (section, "ControlBbName", "PrefixId", par_Fd);
        break;
    }

    IniFileDataBaseWriteInt (section, "can_object_count", m_Objects.size(), par_Fd);

    // Objekte schreiben
    for (int ObjectNr = 0; ObjectNr < m_Objects.size(); ObjectNr++) {
        m_Objects.at(ObjectNr)->WriteToIni(par_VariantNr, ObjectNr, par_Fd, par_Delete);
    }
}

void CanConfigVariant::AddToChannel(int par_ChannelNr)
{
    if (!ConnectToChannelNrs.contains(par_ChannelNr)) {
        ConnectToChannelNrs.append(par_ChannelNr);
    }
}

void CanConfigVariant::RemoveFromChannel(int par_ChannelNr)
{
    ConnectToChannelNrs.removeAll(par_ChannelNr);
}

bool CanConfigVariant::IsConnectedToChannel(int par_ChannelNr)
{
    return ConnectToChannelNrs.contains(par_ChannelNr);
}

CanConfigElement *CanConfigVariant::child(int par_Number)
{
    return this->m_Objects.at(par_Number);
}

int CanConfigVariant::childCount()
{
     return this->m_Objects.size();
}

int CanConfigVariant::columnCount()
{
    return 2;
}

QVariant CanConfigVariant::data(int par_Column)
{
    switch (par_Column) {
    case 0:
        return m_Name;
    case 1:  // Order
        return QVariant ();
    case 2:  // Description
        return m_Description;
    default:
        return QVariant ();
    }
}

CanConfigElement *CanConfigVariant::parent()
{
    return m_ParentServer;
}

int CanConfigVariant::childNumber()
{
    return this->m_ParentServer->GetIndexOfChild(this);
}

bool CanConfigVariant::setData(int par_Column, const QVariant &par_Value)
{
    Q_UNUSED(par_Column)
    Q_UNUSED(par_Value)
    return false;
}

CanConfigElement::Type CanConfigVariant::GetType()
{
    return Variant;
}

void CanConfigVariant::FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index)
{
    Q_UNUSED(par_Index)
    EnableTab(ui->tabWidget, 1);
    ui->tabWidget->setCurrentIndex(1);  // 3 -> Variant-Tab
    ui->VariantNameLineEdit->setText(m_Name);
    ui->VariantDescriptionLineEdit->setText(m_Description);
    if ((m_BaudRate & 0x10000000) == 0x10000000) {
        int bt0 = m_BaudRate & 0xFF;
        int bt1 = (m_BaudRate >> 8) & 0xFF;
        ui->VariantBaudRateRegisterRadioButton->setChecked(true);
        ui->VariantBaudRateRegisterBT0LineEdit->SetValue(bt0);
        ui->VariantBaudRateRegisterBT1LineEdit->SetValue(bt1);
    } else {
        ui->VariantBaudRateValueRadioButton->setChecked(true);
        ui->VariantBaudRateValueLineEdit->SetValue(m_BaudRate & 0xFFFF);
    }

    ui->VariantSamplePointLineEdit->SetValue(m_SamplePoint);
    ui->VariantCanFdCheckBox->setChecked(m_CanFdEnable);
    ui->VariantDataBitRateLineEdit->SetValue(m_DataBaudRate);
    ui->VariantDataSamplePointLineEdit->SetValue(m_DataSamplePoint);

    ui->VariantEnableJ1939CheckBox->setChecked(m_EnableJ1939);
    ui->VariantEnableJ1939FirstAddressCheckBox->setChecked(m_EnableJ1939Addresses[0]);
    ui->VariantJ1939FirstAddressLineEdit->SetValue(m_J1939Addresses[0], 16);
    ui->VariantEnableJ1939SecondAddressCheckBox->setChecked(m_EnableJ1939Addresses[1]);
    ui->VariantJ1939SecondAddressLineEdit->SetValue(m_J1939Addresses[1], 16);
    ui->VariantEnableJ1939ThirdAddressCheckBox->setChecked(m_EnableJ1939Addresses[2]);
    ui->VariantJ1939ThirdAddressLineEdit->SetValue(m_J1939Addresses[2], 16);
    ui->VariantEnableJ1939FourthAddressCheckBox->setChecked(m_EnableJ1939Addresses[3]);
    ui->VariantJ1939FourthAddressLineEdit->SetValue(m_J1939Addresses[3], 16);

    switch (m_ControlBlackboardName) {
    case NoPrefixObjName:
         ui->ObjectControlOnlyObjNameRadioButton->setChecked(true);
         break;
    case PrefixObjName:
        ui->ObjectControlObjNameRadioButton->setChecked(true);
        break;
    case NoPrefixVarObjName:
         ui->ObjectControlOnlyVarObjNameRadioButton->setChecked(true);
         break;
    case PrefixVarObjName:
        ui->ObjectControlVarObjNameRadioButton->setChecked(true);
        break;
    case PrefixId:
        ui->ObjectControlIdRadioButton->setChecked(true);
        break;
    }
}

void CanConfigVariant::StoreDialogTab(Ui::CanConfigDialog *ui)
{
    m_Name = ui->VariantNameLineEdit->text();
    m_Description = ui->VariantDescriptionLineEdit->text();
    if (ui->VariantBaudRateRegisterRadioButton->isChecked()) {
        int bt0 = ui->VariantBaudRateRegisterBT0LineEdit->GetIntValue();
        int bt1 = ui->VariantBaudRateRegisterBT1LineEdit->GetIntValue();
        m_BaudRate = bt0 | (bt1 << 8);
    } else {
        m_BaudRate = ui->VariantBaudRateValueLineEdit->GetIntValue();
    }
    m_SamplePoint = ui->VariantSamplePointLineEdit->GetDoubleValue();
    m_CanFdEnable = ui->VariantCanFdCheckBox->isChecked();
    m_DataBaudRate = ui->VariantDataBitRateLineEdit->GetIntValue();
    m_DataSamplePoint = ui->VariantDataSamplePointLineEdit->GetDoubleValue();

    m_EnableJ1939 = ui->VariantEnableJ1939CheckBox->isChecked();
    m_EnableJ1939Addresses[0] = ui->VariantEnableJ1939FirstAddressCheckBox->isChecked();
    m_J1939Addresses[0] = ui->VariantJ1939FirstAddressLineEdit->GetIntValue();
    m_EnableJ1939Addresses[1] = ui->VariantEnableJ1939SecondAddressCheckBox->isChecked();
    m_J1939Addresses[1] = ui->VariantJ1939SecondAddressLineEdit->GetIntValue();
    m_EnableJ1939Addresses[2] = ui->VariantEnableJ1939ThirdAddressCheckBox->isChecked();
    m_J1939Addresses[2] = ui->VariantJ1939ThirdAddressLineEdit->GetIntValue();
    m_EnableJ1939Addresses[3] = ui->VariantEnableJ1939FourthAddressCheckBox->isChecked();
    m_J1939Addresses[3] = ui->VariantJ1939FourthAddressLineEdit->GetIntValue();

    if (ui->ObjectControlOnlyObjNameRadioButton->isChecked()) {
        m_ControlBlackboardName = NoPrefixObjName;
    } else if (ui->ObjectControlObjNameRadioButton->isChecked()) {
        m_ControlBlackboardName = PrefixObjName;
    } else if (ui->ObjectControlOnlyVarObjNameRadioButton->isChecked()) {
        m_ControlBlackboardName = NoPrefixVarObjName;
    } else if (ui->ObjectControlVarObjNameRadioButton->isChecked()) {
        m_ControlBlackboardName = PrefixVarObjName;
    } else if (ui->ObjectControlIdRadioButton->isChecked()) {
        m_ControlBlackboardName = PrefixId;
    } else {
        m_ControlBlackboardName = PrefixId;
    }
}

void CanConfigVariant::AddNewChild(int par_Row)
{
    Q_UNUSED(par_Row)
    QString NewName = GenerateUniqueName(QString("new"));
    m_Objects.insert(0, new CanConfigObject(this, NewName));
}

QString CanConfigVariant::GenerateUniqueName(QString par_Name)
{
    QString NewName;
    bool InUse = true;
    NewName = par_Name;
    for (int i = 0; ; i++, NewName = QString(par_Name).append(QString("_%1").arg(i))) {
        InUse = false;
        foreach (CanConfigObject *Object, m_Objects) {
            if (!NewName.compare(Object->GetName())) {
                InUse = true;
            }
        }
        if (!InUse) break;
    }
    return NewName;
}

void CanConfigVariant::AddChild(int par_Row, CanConfigElement *par_Child)
{
    CanConfigObject *NewObject = new CanConfigObject(*static_cast<CanConfigObject*>(par_Child));
    NewObject->SetName(GenerateUniqueName(NewObject->GetName()));
    NewObject->SetParent(this);
    m_Objects.insert(par_Row, NewObject);
}

void CanConfigVariant::RemoveChild(int par_Row)
{
    CanConfigObject *Object = m_Objects.at(par_Row);
    m_Objects.removeAt(par_Row);
    delete Object;
}

void CanConfigVariant::SetParent(CanConfigServer *par_Parent)
{
    m_ParentServer = par_Parent;
}

int CanConfigVariant::GetIndexOfChild(CanConfigObject *par_Object)
{
    return m_Objects.indexOf(par_Object);
}

void CanConfigVariant::SortObjectsByIdentifier()
{
    struct CanConfigObjectIdntifierLesser
    {
        bool operator()(const CanConfigObject* a, const CanConfigObject* b) const
        {
            return a->GetId() < b->GetId();
        }
    };
    std::sort(m_Objects.begin(), m_Objects.end(), CanConfigObjectIdntifierLesser());
}

void CanConfigVariant::SortObjectsByName()
{
    struct CanConfigObjectNameLesser
    {
        bool operator()(const CanConfigObject* a, const CanConfigObject* b) const
        {
            return (a->GetNameConst().compare(b->GetNameConst()) <= 0);
        }
    };
    std::sort(m_Objects.begin(), m_Objects.end(), CanConfigObjectNameLesser());
}

QList<CanConfigSignal*> CanConfigVariant::SearchSignal(const QRegularExpression &par_RegExp)
{
    QList<CanConfigSignal*> Ret;
    for (int o = 0; o < m_Objects.size(); o++) {
        Ret.append(m_Objects.at(o)->SearchSignal(par_RegExp));
    }
    return Ret;
}

CanConfigObject::CanConfigObject(CanConfigVariant *par_ParentVariant, QString par_Name)
{
    m_Name = par_Name;
    m_ParentVariant = par_ParentVariant;
    m_Identifier = 0;
    m_ExtendedId = false;
    m_BitRateSwitch = false;
    m_FDFrameFormatSwitch = false;
    m_Size = 8;
    m_Direction = Read;
    m_TransmitEventType = Cyclic;
    m_TransmitCycles = QString("1");
    m_TransmitDelay = QString("0");
    m_Type = Normal;
    m_MultiplexerStartBit = 0;
    m_MultiplexerBitSize = 1;
    m_MultiplexerValue = 0;
    m_VariableDlc = false;
    m_DestAddressType = Fixed;
    m_FixedDestAddress = 0;
    m_VariableDestAddressInitValue = 0;
    m_ObjectInitValue = 0;
}

CanConfigObject::CanConfigObject(const CanConfigObject &par_Object)
{
    m_Name = par_Object.m_Name;
    m_Description = par_Object.m_Description;
    m_Identifier = par_Object.m_Identifier;
    m_ByteOrder = par_Object.m_ByteOrder;
    m_ExtendedId = par_Object.m_ExtendedId;
    m_BitRateSwitch = par_Object.m_BitRateSwitch;
    m_FDFrameFormatSwitch = par_Object.m_FDFrameFormatSwitch;
    m_Size = par_Object.m_Size;
    m_Direction = par_Object.m_Direction;
    m_TransmitEventType = par_Object.m_TransmitEventType;
    m_TransmitCycles = par_Object.m_TransmitCycles;
    m_TransmitDelay = par_Object.m_TransmitDelay;
    m_TransmitEquation = par_Object.m_TransmitEquation;
    m_Type = par_Object.m_Type;
    m_MultiplexerStartBit = par_Object.m_MultiplexerStartBit;
    m_MultiplexerBitSize = par_Object.m_MultiplexerBitSize;
    m_MultiplexerValue = par_Object.m_MultiplexerValue;
    m_VariableDlc = par_Object.m_VariableDlc;
    m_VariableDlcVariableName = par_Object.m_VariableDlcVariableName;
    m_DestAddressType = par_Object.m_DestAddressType;
    m_FixedDestAddress = par_Object.m_FixedDestAddress;
    m_VariableDestAddressVariableName = par_Object.m_VariableDestAddressVariableName;
    m_VariableDestAddressInitValue = par_Object.m_VariableDestAddressInitValue;
    m_ObjectInitValue = par_Object.m_ObjectInitValue;
    foreach (CanConfigSignal *Signal, par_Object.m_Signals) {
        CanConfigSignal *NewSignal = new CanConfigSignal(*Signal);
        NewSignal->SetParent(this);
        m_Signals.append(NewSignal);
    }
    m_AdditionalVariables = par_Object.m_AdditionalVariables;
    m_AdditionalEquationBefore = par_Object.m_AdditionalEquationBefore;
    m_AdditionalEquationBehind = par_Object.m_AdditionalEquationBehind;

    m_C_PG_Ids = par_Object.m_C_PG_Ids;
    m_ParentVariant = nullptr;
}


CanConfigObject::~CanConfigObject()
{
    foreach (CanConfigSignal *Signal, m_Signals) {
        delete Signal;
    }
}

bool CanConfigObject::ReadFromIni(int par_VariantNr, int par_ObjectNr, int par_Fd)
{
    char section[INI_MAX_SECTION_LENGTH];
    char entry[INI_MAX_ENTRYNAME_LENGTH];
    char txt[INI_MAX_LINE_LENGTH];
    int i;

    sprintf (section, "CAN/Variante_%i/Object_%i", par_VariantNr, par_ObjectNr);
    if (!IniFileDataBaseLookIfSectionExist(section, par_Fd)) return false;  // This object don't exist
    IniFileDataBaseReadString (section, "name", "", txt, sizeof (txt), par_Fd);
    m_Name = CharToQString(txt);
    IniFileDataBaseReadString (section, "desc", "", txt, sizeof (txt), par_Fd);
    m_Description = CharToQString(txt);
    IniFileDataBaseReadString (section, "id", "", txt, sizeof (txt), par_Fd);
    m_Identifier = strtol (txt, nullptr, 0);
    if (IniFileDataBaseReadString (section, "variable_id", "", txt, sizeof (txt), par_Fd) > 0) {
        m_VariableId = QString(txt);
    }
    m_ExtendedId = IniFileDataBaseReadYesNo(section, "extended", 0, par_Fd);
    m_BitRateSwitch = IniFileDataBaseReadYesNo(section, "bit_rate_switch", 0, par_Fd);
    m_FDFrameFormatSwitch = IniFileDataBaseReadYesNo(section, "fd_frame_format_switch", 0, par_Fd);
    IniFileDataBaseReadString (section, "byteorder", "lsb_first", txt, sizeof (txt), par_Fd);
    if (!strcmp (txt, "msb_first")) {
        m_ByteOrder = MsbFirst;
    } else {
        m_ByteOrder = LsbFirst;
    }
    m_Size = IniFileDataBaseReadInt (section, "size", 1, par_Fd);
    if (m_Size < 0) m_Size = 0;
    IniFileDataBaseReadString (section, "direction", "read", txt, sizeof (txt), par_Fd);
    if (!strcmp ("write_variable_id", txt)) {
        m_Direction = WriteVariableId;
    } else if (!strcmp ("write", txt)) {
        m_Direction = Write;
    } else {  // read
        m_Direction = Read;
    }
    IniFileDataBaseReadString (section, "type", "", txt, sizeof (txt), par_Fd);
    if (!strcmpi (txt, "mux")) {
        m_Type = Multiplexed;
    } else if (!strcmpi (txt, "j1939")) {
        m_Type = J1939;
        IniFileDataBaseReadString (section, "variable_dlc", "", txt, sizeof (txt), par_Fd);
        if (strlen(txt)) {
            m_VariableDlc = true;
            m_VariableDlcVariableName = QString(txt);
        } else {
            m_VariableDlc = false;
            m_VariableDlcVariableName = QString();
        }
        IniFileDataBaseReadString (section, "variable_dst_addr", "", txt, sizeof (txt), par_Fd);
        if (strlen(txt)) {
            m_DestAddressType = Variable;
            m_VariableDestAddressVariableName = QString(txt);
            m_VariableDestAddressInitValue = IniFileDataBaseReadUInt (section, "init_dst_addr", 0xFF, par_Fd);
        } else {
            m_DestAddressType = Fixed;
            m_VariableDestAddressVariableName = QString();
            m_FixedDestAddress = IniFileDataBaseReadUInt (section, "fixed_dst_addr", 0xFF, par_Fd);
        }
    } else if (!strcmpi (txt, "j1939_multi_c_pg")) {
        m_Type = J1939_Multi_PG;
    } else if (!strcmpi (txt, "j1939_c_pg")) {
        m_Type = J1939_C_PG;
    }else {
        m_Type = Normal;
    }
    m_MultiplexerStartBit = IniFileDataBaseReadInt (section, "mux_startbit", 0, par_Fd);
    m_MultiplexerBitSize = IniFileDataBaseReadInt (section, "mux_bitsize", 0, par_Fd);
    m_MultiplexerValue = IniFileDataBaseReadInt (section, "mux_value", 0, par_Fd);


    IniFileDataBaseReadString (section, "CyclicOrEvent", "Cyclic", txt, sizeof (txt), par_Fd);
    if (!strcmpi ("Event", txt)) {
        m_TransmitEventType = Equation;
    } else {
        m_TransmitEventType = Cyclic;
    }

    // Cycles
    IniFileDataBaseReadString(section, "cycles", "1", txt, sizeof(txt), par_Fd);
    m_TransmitCycles = QString(txt);
    // Delay
    IniFileDataBaseReadString(section, "delay", "0", txt, sizeof(txt), par_Fd);
    m_TransmitDelay = QString(txt);

    IniFileDataBaseReadString (section, "EventEquation", "", txt, sizeof (txt), par_Fd);
    m_TransmitEquation = CharToQString(txt);

    // "additional equation"
    m_AdditionalVariables.clear();
    for (i = 0; ; i++) {
        sprintf (entry, "additional_variable_%i", i);
        if (IniFileDataBaseReadString (section, entry, "", txt, sizeof (txt), par_Fd) <= 0) {
            break;
        }
        m_AdditionalVariables.append(CharToQString(txt));
        //m_AdditionalVariables.append(QString("\n"));
    }
    for (i = 0; ; i++) {
        sprintf (entry, "equ_before_%i", i);
        if (IniFileDataBaseReadString (section, entry, "", txt, sizeof (txt), par_Fd) <= 0) {
            break;
        }
        m_AdditionalEquationBefore.append(CharToQString(txt));
        //m_AdditionalEquationBefore.append(QString("\n"));
    }
    for (i = 0; ; i++) {
        sprintf (entry, "equ_behind_%i", i);
        if (IniFileDataBaseReadString (section, entry, "", txt, sizeof (txt), par_Fd) <= 0) {
            break;
        }
        m_AdditionalEquationBehind.append(CharToQString(txt));
        //m_AdditionalEquationBehind.append(QString("\n"));
    }

    // Init Data
    if (IniFileDataBaseReadString (section, "InitData", "", txt, sizeof (txt), par_Fd)) {
#ifdef _WIN32
        m_ObjectInitValue = _strtoui64 (txt, nullptr, 0);
#else
        m_ObjectInitValue = strtoull (txt, nullptr, 0);
#endif
    } else {
        m_ObjectInitValue = 0;
    }

    // read C_PG Ids
    if (m_Type == J1939_Multi_PG) {
        if (IniFileDataBaseReadString (section, "C_PGs", "", txt, sizeof (txt), par_Fd)) {
            char *p = txt;
            for (int x = 0; ; x++) {
                char *h = nullptr;
                int Id = strtoul(p, &h, 0);
                if ((h == nullptr) || (h == p)) break;
                m_C_PG_Ids.append(Id);
                p = h;
                while(isascii(*p) && isspace(*p)) p++;
                if (*p != ',') break;
                p++;
            }
        }
    }

    // Signale einlesen
    for (int SignalNr = 0; ; SignalNr++) {
        CanConfigSignal *NewSignal = new CanConfigSignal(this);
        if (NewSignal->ReadFromIni(par_VariantNr, par_ObjectNr, SignalNr, par_Fd)) {
            m_Signals.append(NewSignal);
        } else {
            delete NewSignal;
            break;
        }
    }
    return true;
}

void CanConfigObject::WriteToIni(int par_VariantNr, int par_ObjectNr, int par_Fd, bool par_Delete)
{
    char section[INI_MAX_SECTION_LENGTH];
    char entry[INI_MAX_ENTRYNAME_LENGTH];
    char txt[INI_MAX_LINE_LENGTH];
    int i;

    sprintf (section, "CAN/Variante_%i/Object_%i", par_VariantNr, par_ObjectNr);
    IniFileDataBaseWriteString(section, nullptr, nullptr, par_Fd);
    if (par_Delete) return;
    IniFileDataBaseWriteString (section, "name", QStringToConstChar(m_Name), par_Fd);
    IniFileDataBaseWriteString (section, "desc", QStringToConstChar(m_Description), par_Fd);
    sprintf (txt, "0x%X", m_Identifier);
    IniFileDataBaseWriteString (section, "id", txt, par_Fd);
    if (!m_VariableId.isEmpty()) {
        IniFileDataBaseWriteString (section, "variable_id", QStringToConstChar(m_VariableId), par_Fd);
    }
    IniFileDataBaseWriteYesNo(section, "extended", m_ExtendedId, par_Fd);
    IniFileDataBaseWriteYesNo(section, "bit_rate_switch", m_BitRateSwitch, par_Fd);
    IniFileDataBaseWriteYesNo(section, "fd_frame_format_switch", m_FDFrameFormatSwitch, par_Fd);
    if (m_ByteOrder == MsbFirst) {
        IniFileDataBaseWriteString (section, "byteorder", "msb_first", par_Fd);
    } else {
        IniFileDataBaseWriteString (section, "byteorder", "lsb_first", par_Fd);
    }
    IniFileDataBaseWriteInt (section, "size", m_Size, par_Fd);
    switch (m_Direction) {
    case WriteVariableId:
        IniFileDataBaseWriteString (section, "direction", "write_variable_id", par_Fd);
        break;
    case Write:
        IniFileDataBaseWriteString (section, "direction", "write", par_Fd);
        break;
    case Read:
    //default:
        IniFileDataBaseWriteString (section, "direction", "read", par_Fd);
        break;
    }
    switch (m_Type) {
    case Multiplexed:
        IniFileDataBaseWriteString (section, "type", "mux", par_Fd);
        break;
    case J1939:
        IniFileDataBaseWriteString (section, "type", "j1939", par_Fd);
        if (m_VariableDlc) {
            IniFileDataBaseWriteString (section, "variable_dlc", QStringToConstChar(m_VariableDlcVariableName), par_Fd);
        }
        if (m_DestAddressType == Variable) {
            IniFileDataBaseWriteString (section, "variable_dst_addr", QStringToConstChar(m_VariableDestAddressVariableName), par_Fd);
            m_VariableDestAddressInitValue = IniFileDataBaseWriteUInt (section, "init_dst_addr", m_VariableDestAddressInitValue, par_Fd);
        } else {
            IniFileDataBaseWriteUInt (section, "fixed_dst_addr", m_FixedDestAddress, par_Fd);
        }
        break;
    case J1939_Multi_PG:
        IniFileDataBaseWriteString (section, "type", "j1939_multi_c_pg", par_Fd);
        break;
    case J1939_C_PG:
        IniFileDataBaseWriteString (section, "type", "j1939_c_pg", par_Fd);
        break;
    case Normal:
    //default:
        IniFileDataBaseWriteString (section, "type", "normal", par_Fd);
        break;
    }
    IniFileDataBaseWriteInt (section, "mux_startbit", m_MultiplexerStartBit, par_Fd);
    IniFileDataBaseWriteInt (section, "mux_bitsize", m_MultiplexerBitSize, par_Fd);
    IniFileDataBaseWriteInt (section, "mux_value", m_MultiplexerValue, par_Fd);

    switch (m_TransmitEventType) {
    case Equation:
        IniFileDataBaseWriteString (section, "CyclicOrEvent", "Event", par_Fd);
        IniFileDataBaseWriteString (section, "EventEquation", QStringToConstChar(m_TransmitEquation), par_Fd);
        break;
    case Cyclic:
    //default:
        IniFileDataBaseWriteString (section, "CyclicOrEvent", "Cyclic", par_Fd);
        break;
    }

    // Cycles
    IniFileDataBaseWriteString(section, "cycles", QStringToConstChar(m_TransmitCycles), par_Fd);
    // Delay
    IniFileDataBaseWriteString(section, "delay", QStringToConstChar(m_TransmitDelay), par_Fd);


    // "additional equation"
    i = 0;
    foreach (QString VariableDefinition, m_AdditionalVariables) {
        sprintf (entry, "additional_variable_%i", i);
        IniFileDataBaseWriteString (section, entry, QStringToConstChar(VariableDefinition), par_Fd);
        i++;
    }
    i = 0;
    foreach (QString EquationBefore, m_AdditionalEquationBefore) {
        sprintf (entry, "equ_before_%i", i);
        IniFileDataBaseWriteString (section, entry, QStringToConstChar(EquationBefore), par_Fd);
        i++;
    }
    i = 0;
    foreach (QString EquationBehind, m_AdditionalEquationBehind) {
        sprintf (entry, "equ_behind_%i", i);
        IniFileDataBaseWriteString (section, entry, QStringToConstChar(EquationBehind), par_Fd);
        i++;
    }

    // Init Data
    sprintf (txt, "0x%" PRIX64 "", m_ObjectInitValue);
    IniFileDataBaseWriteString (section, "InitData", txt, par_Fd);

    // write C_PG Ids
    if (m_Type == J1939_Multi_PG) {
        char *p = txt;
        *p = 0;
        foreach(int Id, m_C_PG_Ids) {
            if ((p - txt) > ((int)sizeof(txt) - 32)) break;
            sprintf(p, (p == txt) ? "0x%X" : ", 0x%X", Id);
            p = p + strlen(p);
        }
        IniFileDataBaseWriteString (section, "C_PGs", txt, par_Fd);
    }

    // Signale einlesen
    for (int SignalNr = 0; SignalNr < m_Signals.size(); SignalNr++) {
        m_Signals.at(SignalNr)->WriteToIni(par_VariantNr, par_ObjectNr, SignalNr, par_Fd, par_Delete);
    }
    IniFileDataBaseWriteInt (section, "signal_count", m_Signals.size(), par_Fd);

}

CanConfigElement *CanConfigObject::child(int par_Number)
{
    if ((par_Number >= 0) && (par_Number < m_Signals.size())) {
        return m_Signals.at(par_Number);
    } else {
        return nullptr;
    }
}

int CanConfigObject::childCount()
{
    return this->m_Signals.size();
}

int CanConfigObject::columnCount()
{
    return 2;
}

QVariant CanConfigObject::data(int par_Column)
{
    switch (par_Column) {
    case 0:
        if (m_Direction == WriteVariableId) {
            return QString("-> [0x%1(var)] ").arg(m_Identifier, 0, 16).append(m_Name);
        } else if (m_Direction == Write) {
            return QString("-> [0x%1] ").arg(m_Identifier, 0, 16).append(m_Name);
        } else {
            return QString("<- [0x%1] ").arg(m_Identifier, 0, 16).append(m_Name);
        }
        //return m_Name;
    case 1:  // Order
        return QVariant ();
    case 2:  // Description
        return m_Description;
    default:
        return QVariant ();
    }
}

CanConfigElement *CanConfigObject::parent()
{
    return m_ParentVariant;
}

int CanConfigObject::childNumber()
{
    return this->m_ParentVariant->GetIndexOfChild(this);
}

bool CanConfigObject::setData(int column, const QVariant &value)
{
    Q_UNUSED(column)
    Q_UNUSED(value)
    return false;
}

CanConfigElement::Type CanConfigObject::GetType()
{
    return Object;
}

void CanConfigObject::FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index)
{
    EnableTab(ui->tabWidget, 2);
    ui->tabWidget->setCurrentIndex(2);  // 2 -> Object-Tab

    // maybe on_ObjectSizeSpinBox_valueChanged will be called from setRange!
    int SizeSave = m_Size;

    ui->ObjectNameLineEdit->setText(m_Name);
    ui->ObjectDescriptionLineEdit->setText(m_Description);
    ui->ObjectIdentifierLineEdit->SetValue(m_Identifier, 16);
    ui->ObjectExtendedIdCheckBox->setChecked(m_ExtendedId);
    ui->ObjectBitRateSwitchCheckBox->setChecked(m_BitRateSwitch);
    ui->ObjectFDFrameFromatCheckBox->setChecked(m_FDFrameFormatSwitch);
    if (m_ByteOrder == MsbFirst) {
        ui->ObjectByteOrderComboBox->setCurrentText(QString("msb_first"));
    } else {
        ui->ObjectByteOrderComboBox->setCurrentText(QString("lsb_first"));
    }
    if (m_Type == J1939) {
        ui->ObjectSizeSpinBox->setRange(1, 1785); // J1939_MAX_DATA_LENGTH);
    } else if ((m_BitRateSwitch) || (m_Type == J1939_Multi_PG) || (m_Type == J1939_C_PG)) {
        ui->ObjectSizeSpinBox->setRange(1, 64);
    } else {
        ui->ObjectSizeSpinBox->setRange(1, 8);
    }
    m_Size = SizeSave;
    ui->ObjectSizeSpinBox->setValue(m_Size);
    ui->ObjectDirectionComboBox->setCurrentIndex(m_Direction);
    switch (m_TransmitEventType) {
    case Cyclic:
         ui->ObjectTransmitEventCyclicRadioButton->setChecked(true);
         break;
    case Equation:
        ui->ObjectTransmitEventEquationRadioButton->setChecked(true);
        break;
    }

    ui->ObjectTransmitEventCyclesLineEdit->setText(m_TransmitCycles);
    ui->ObjectTransmitEventDelayLineEdit->setText(m_TransmitDelay);
    ui->ObjectTransmitEventEquationLineEdit->setText(m_TransmitEquation);

    switch (m_Type) {
    case Normal:
    //default:
        ui->ObjectTypeNormalRadioButton->setChecked(true);
        break;
    case Multiplexed:
        ui->ObjectTypeMultiplexedRadioButton->setChecked(true);
        break;
    case J1939:
        ui->ObjectTypeJ1939RadioButton->setChecked(true);
        ui->ObjectTypeJ1939ComboBox->setCurrentIndex(0);
        break;
    case J1939_Multi_PG:
        ui->ObjectTypeJ1939RadioButton->setChecked(true);
        ui->ObjectTypeJ1939ComboBox->setCurrentIndex(1);
        break;
    case J1939_C_PG:
        ui->ObjectTypeJ1939RadioButton->setChecked(true);
        ui->ObjectTypeJ1939ComboBox->setCurrentIndex(2);
        break;
    }
    ui->ObjectTypeMultiplexedStartBitLineEdit->SetValue(m_MultiplexerStartBit);
    ui->ObjectTypeMultiplexedBitSizeLineEdit->SetValue(m_MultiplexerBitSize);
    ui->ObjectTypeMultiplexedValueLineEdit->SetValue(m_MultiplexerValue);

    ui->ObjectAllSignalsListView->setRootIndex(par_Index);
    /*
    ui->ObjectAllSignalsListWidget->clear();
    foreach (CanConfigSignal* Signal, m_Signals) {
        ui->ObjectAllSignalsListWidget->addItem(Signal->GetName());
    }*/

    ui->AdditionalVariablesPlainTextEdit->clear();
    foreach (QString Item, m_AdditionalVariables) {
        ui->AdditionalVariablesPlainTextEdit->appendPlainText(Item.append(";"));
    }
    ui->AdditionalEquationsBeforePlainTextEdit->clear();
    foreach (QString Item, m_AdditionalEquationBefore) {
        ui->AdditionalEquationsBeforePlainTextEdit->appendPlainText(Item.append(";"));
    }
    ui->AdditionalEquationsBehindPlainTextEdit->clear();
    foreach (QString Item, m_AdditionalEquationBehind) {
        ui->AdditionalEquationsBehindPlainTextEdit->appendPlainText(Item.append(";"));
    }

    ui->ObjectInitValueLineEdit->SetValue(m_ObjectInitValue, 16);
}

static QStringList PlainTextToStringList (const QString &par_PlainText)
{
    QString Text = par_PlainText.trimmed();
    Text.remove('\r');  // brauch es das?
    Text.replace('\n', " ");
    QStringList Temp = Text.split(";");
    QStringList Ret;
    foreach (QString Item, Temp) {   // Leerzeilen loeschen
        Item = Item.trimmed();
        if (Item.size()) {
            Ret.append(Item);
        }
    }
    return Ret;
}

void CanConfigObject::StoreDialogTab(Ui::CanConfigDialog *ui)
{
    m_Name = ui->ObjectNameLineEdit->text();
    m_Description = ui->ObjectDescriptionLineEdit->text();
    m_Identifier = ui->ObjectIdentifierLineEdit->GetIntValue();
    m_ExtendedId = ui->ObjectExtendedIdCheckBox->isChecked();
    m_BitRateSwitch = ui->ObjectBitRateSwitchCheckBox->isChecked();
    m_FDFrameFormatSwitch = ui->ObjectFDFrameFromatCheckBox->isChecked();
    if (ui->ObjectByteOrderComboBox->currentText().compare(QString("msb_first")) == 0) {
        m_ByteOrder = MsbFirst;
    } else {
        m_ByteOrder = LsbFirst;
    }

    m_Size = ui->ObjectSizeSpinBox->value();
    m_Direction = static_cast<enum Direction>(ui->ObjectDirectionComboBox->currentIndex());
    if (ui->ObjectTransmitEventCyclicRadioButton->isChecked()) {
        m_TransmitEventType = Cyclic;
    } else if (ui->ObjectTransmitEventEquationRadioButton->isChecked()) {
        m_TransmitEventType = Equation;
    } else {
        m_TransmitEventType = Cyclic;
    }

    m_TransmitCycles = ui->ObjectTransmitEventCyclesLineEdit->text().trimmed();
    m_TransmitDelay = ui->ObjectTransmitEventDelayLineEdit->text().trimmed();
    m_TransmitEquation = ui->ObjectTransmitEventEquationLineEdit->text();

    if (ui->ObjectTypeNormalRadioButton->isChecked()) {
        m_Type = Normal;
    } else if (ui->ObjectTypeMultiplexedRadioButton->isChecked()) {
        m_Type = Multiplexed;
    } else if (ui->ObjectTypeJ1939RadioButton->isChecked()) {
        switch(ui->ObjectTypeJ1939ComboBox->currentIndex()) {
        default:
        case 0:
            m_Type = J1939;
            break;
        case 1:
            m_Type = J1939_Multi_PG;
            break;
        case 2:
            m_Type = J1939_C_PG;
            break;
        }
    } else {
        m_Type = Normal;
    }
    m_MultiplexerStartBit = ui->ObjectTypeMultiplexedStartBitLineEdit->GetIntValue();
    m_MultiplexerBitSize = ui->ObjectTypeMultiplexedBitSizeLineEdit->GetIntValue();
    m_MultiplexerValue = ui->ObjectTypeMultiplexedValueLineEdit->GetIntValue();

    m_AdditionalVariables = PlainTextToStringList(ui->AdditionalVariablesPlainTextEdit->toPlainText());
    m_AdditionalEquationBefore = PlainTextToStringList(ui->AdditionalEquationsBeforePlainTextEdit->toPlainText());
    m_AdditionalEquationBehind = PlainTextToStringList(ui->AdditionalEquationsBehindPlainTextEdit->toPlainText());

    m_ObjectInitValue = ui->ObjectInitValueLineEdit->GetUInt64Value();
}

void CanConfigObject::AddNewChild(int par_Row)
{
    Q_UNUSED(par_Row)
    QString NewName = GenerateUniqueName(QString("new"));
    m_Signals.insert(par_Row, new CanConfigSignal(this, NewName));
}

QString CanConfigObject::GenerateUniqueName(QString par_Name)
{
    QString NewName;
    bool InUse = true;
    NewName = par_Name;
    for (int i = 0; ; i++, NewName = QString(par_Name).append(QString("_%1").arg(i))) {
        InUse = false;
        foreach (CanConfigSignal *Signal, m_Signals) {
            if (!NewName.compare(Signal->GetName())) {
                InUse = true;
            }
        }
        if (!InUse) break;
    }
    return NewName;
}

void CanConfigObject::AddChild(int par_Row, CanConfigElement *par_Child)
{
    CanConfigSignal *NewSignal = new CanConfigSignal(*static_cast<CanConfigSignal*>(par_Child));
    NewSignal->SetName(GenerateUniqueName(NewSignal->GetName()));
    NewSignal->SetParent(this);
    m_Signals.insert(par_Row, NewSignal);
}

void CanConfigObject::RemoveChild(int par_Row)
{
    CanConfigSignal *Signal = m_Signals.at(par_Row);
    m_Signals.removeAt(par_Row);
    delete Signal;
}

void CanConfigObject::SetParent(CanConfigVariant *par_Parent)
{
    m_ParentVariant = par_Parent;
}

int CanConfigObject::GetIndexOfChild(CanConfigSignal *par_Signal)
{
    return m_Signals.indexOf(par_Signal);
}

QColor CanConfigObject::GetColorOfSignal(CanConfigSignal *par_Signal)
{
    int Index = m_Signals.indexOf(par_Signal) % 8;
    //int NumberOfSteps = m_Signals.size();
    switch (Index) {
    case 0: return QColor(200, 200, 200);
    case 1: return QColor(255, 200, 200);
    case 2: return QColor(200, 255, 200);
    case 3: return QColor(255, 255, 200);
    case 4: return QColor(200, 200, 255);
    case 5: return QColor(255, 200, 255);
    case 6: return QColor(200, 255, 255);
    case 7: return QColor(255, 255, 255);
    default: return QColor(200, 200, 200);
    }
}

QList<CanConfigSignal*> CanConfigObject::SearchSignal(const QRegularExpression &par_RegExp)
{
    QList<CanConfigSignal*> Ret;
    for (int s = 0; s < m_Signals.size(); s++) {
        CanConfigSignal *Signal = m_Signals.at(s);
        if (par_RegExp.match(Signal->GetName(), QRegularExpression::NormalMatch).hasMatch()) {
            Ret.append(Signal);
        }
    }
    return Ret;
}

bool CanConfigObject::CheckExistanceOfAll_C_PGs()
{
    bool Ret = true;
    if (m_Type == J1939_Multi_PG) {
        for(int i = 0; i < m_C_PG_Ids.size(); ) {
            bool Found = false;
            int Id = m_C_PG_Ids.at(i);
            for (int x = 0; x < m_ParentVariant->childCount(); x++) {
                CanConfigObject *CanObject = static_cast<CanConfigObject*>(m_ParentVariant->child(x));
                if (CanObject->GetId() == Id) {
                    Found = true;
                }
            }
            if (!Found) {
                m_C_PG_Ids.removeAt(i);
                Ret = false;
            } else {
                i++;
            }
        }
    } else {
        m_C_PG_Ids.clear();
    }
    return Ret;
}

CanConfigSignal *CanConfigObject::BitInsideSignal(int par_BitPos, bool *ret_FirstBit, bool *ret_LasrBit)
{
    foreach (CanConfigSignal *Signal, m_Signals) {
        if (Signal->BitInsideSignal (par_BitPos, m_ByteOrder, m_Size, ret_FirstBit, ret_LasrBit)) {
            return Signal;
        }
    }
    if (ret_FirstBit != nullptr) *ret_FirstBit = false;
    if (ret_LasrBit != nullptr) *ret_LasrBit = false;
    return nullptr;
}


CanConfigSignal::CanConfigSignal(CanConfigObject *par_ParentObject, QString par_Name)
{
    m_Name = par_Name;
    m_ParentObject = par_ParentObject;
    m_ConvertionType = MultiplyFactorAddOffset;
    m_ConversionFactor.d = 1.0;
    m_ConversionFactorType = FLOAT_OR_INT_64_TYPE_F64;
    m_ConversionOffset.d = 0.0;
    m_ConversionOffsetType = FLOAT_OR_INT_64_TYPE_F64;
    m_StartBitAddress = 0;
    m_BitSize = 8;
    m_ByteOrder = LsbFirst;
    m_Sign = Unsigned;
    m_StartValueEnable = true;
    m_StartValue.d = 0.0;
    m_StartValueType = FLOAT_OR_INT_64_TYPE_F64;
    m_BlackboardDataType = BB_UNKNOWN_DOUBLE;
    m_SignalType = Normal;
    m_MultiplexedStartBit = 0;
    m_MultiplexedBitSize = 1;
    m_MultiplexedValue = 0;
}

CanConfigSignal::~CanConfigSignal()
{
    // nix
}


bool CanConfigSignal::ReadFromIni(int par_VariantNr, int par_ObjectNr, int par_SignalNr, int par_Fd)
{
    char section[INI_MAX_SECTION_LENGTH];
    char txt[INI_MAX_LINE_LENGTH];

    sprintf (section, "CAN/Variante_%i/Object_%i/Signal_%i", par_VariantNr, par_ObjectNr, par_SignalNr);
    if (!IniFileDataBaseLookIfSectionExist(section, par_Fd)) return false;  // This signal don't exist
    IniFileDataBaseReadString (section, "name", "", txt, sizeof (txt), par_Fd);
    m_Name = CharToQString(txt);
    IniFileDataBaseReadString (section, "desc", "", txt, sizeof (txt), par_Fd);
    m_Description = CharToQString(txt);
    IniFileDataBaseReadString (section, "unit", "", txt, sizeof (txt), par_Fd);
    m_Unit = CharToQString(txt);
    IniFileDataBaseReadString (section, "convtype", "mx+b", txt, sizeof (txt), par_Fd);
    if (!strcmpi (txt, "mx+b")) {
        m_ConvertionType = MultiplyFactorAddOffset;
    } else if (!strcmpi (txt, "m(x+b)")) {
        m_ConvertionType = AddOffsetMultiplyFactor;
    } else if (!strcmpi (txt, "curve")) {
        m_ConvertionType = Curve;
    } else if (!strcmpi (txt, "equation")) {
        m_ConvertionType = Equation;
    }
    IniFileDataBaseReadString (section, "convert", "1", txt, sizeof (txt), par_Fd);
    m_ConversionFactorType = string_to_FloatOrInt64(txt, &m_ConversionFactor);
    IniFileDataBaseReadString (section, "offset", "0", txt, sizeof (txt), par_Fd);
    m_ConversionOffsetType = string_to_FloatOrInt64(txt, &m_ConversionOffset);
    IniFileDataBaseReadString (section, "convstring", "", txt, sizeof (txt), par_Fd);
    m_ConversionEquationOrCurve = CharToQString(txt);
    m_StartBitAddress = IniFileDataBaseReadInt (section, "startbit", 0, par_Fd);
    m_BitSize = IniFileDataBaseReadInt (section, "bitsize", 8, par_Fd);
    IniFileDataBaseReadString (section, "byteorder", "", txt, sizeof (txt), par_Fd);
    if (!strcmp ("lsb_first", txt)) {
        m_ByteOrder = LsbFirst;
    } else {
        m_ByteOrder = MsbFirst;
    }
    m_StartValueEnable = IniFileDataBaseReadYesNo (section, "startvalue active", 1, par_Fd);
    IniFileDataBaseReadString (section, "startvalue", "0", txt, sizeof (txt), par_Fd);
    m_StartValueType = string_to_FloatOrInt64(txt, &m_StartValue);

    IniFileDataBaseReadString (section, "type", "", txt, sizeof (txt), par_Fd);
    if (!strcmpi (txt, "mux_by")) {
        char MuxedBySignal[512];
        m_SignalType = MultiplexedBy;
        if (IniFileDataBaseReadString(section, "mux_by_signal", "", MuxedBySignal, sizeof(MuxedBySignal), par_Fd)) {
            m_MultiplexedBy = QString(MuxedBySignal);
        }
        m_MultiplexedStartBit = 0;
        m_MultiplexedBitSize = 0;
        m_MultiplexedValue = IniFileDataBaseReadInt (section, "mux_value", 0, par_Fd);
    } else if (!strcmpi (txt, "mux")) {
        m_SignalType = Multiplexed;
        m_MultiplexedStartBit = IniFileDataBaseReadInt (section, "mux_startbit", 0, par_Fd);
        m_MultiplexedBitSize = IniFileDataBaseReadInt (section, "mux_bitsize", 1, par_Fd);
        m_MultiplexedValue = IniFileDataBaseReadInt (section, "mux_value", 0, par_Fd);
    } else {
        m_SignalType = Normal;
        m_MultiplexedStartBit = 0;
        m_MultiplexedBitSize = 0;
        m_MultiplexedValue = 0;
    }

    IniFileDataBaseReadString (section, "bbtype", "UNKNOWN_DOUBLE", txt, sizeof (txt), par_Fd);
    m_BlackboardDataType = GetDataTypebyName (txt);
    if (m_BlackboardDataType < 0) m_BlackboardDataType = BB_UNKNOWN_DOUBLE;

    IniFileDataBaseReadString (section, "sign", "unsigned", txt, sizeof (txt), par_Fd);
    if (!strcmp ("signed", txt)) {
        m_Sign = Signed;
    } else if (!strcmp ("float", txt)) {
        m_Sign = Float;
    } else {
        m_Sign = Unsigned;
    }
    return true;
}

void CanConfigSignal::WriteToIni(int par_VariantNr, int par_ObjectNr, int par_SignalNr, int par_Fd, bool par_Delete)
{
    char section[INI_MAX_SECTION_LENGTH];

    sprintf (section, "CAN/Variante_%i/Object_%i/Signal_%i", par_VariantNr, par_ObjectNr, par_SignalNr);
    IniFileDataBaseWriteString(section, nullptr, nullptr, par_Fd);
    if (par_Delete) return;
    IniFileDataBaseWriteString (section, "name", QStringToConstChar(m_Name), par_Fd);
    IniFileDataBaseWriteString (section, "desc", QStringToConstChar(m_Description), par_Fd);
    IniFileDataBaseWriteString (section, "unit", QStringToConstChar(m_Unit), par_Fd);
    switch (m_ConvertionType) {
    case MultiplyFactorAddOffset:
    //default:
        IniFileDataBaseWriteString (section, "convtype", "mx+b", par_Fd);
        break;
    case AddOffsetMultiplyFactor:
        IniFileDataBaseWriteString (section, "convtype", "m(x+b)", par_Fd);
        break;
    case Curve:
        IniFileDataBaseWriteString (section, "convtype", "curve", par_Fd);
        break;
    case Equation:
        IniFileDataBaseWriteString (section, "convtype", "equation", par_Fd);
        break;
    }
    char Help[128];
    FloatOrInt64_to_string(m_ConversionFactor, m_ConversionFactorType, 10, Help, sizeof(Help));
    IniFileDataBaseWriteString (section, "convert", Help, par_Fd);
    FloatOrInt64_to_string(m_ConversionOffset, m_ConversionOffsetType, 10, Help, sizeof(Help));
    IniFileDataBaseWriteString (section, "offset", Help, par_Fd);
    IniFileDataBaseWriteString (section, "convstring", QStringToConstChar(m_ConversionEquationOrCurve), par_Fd);
    IniFileDataBaseWriteInt (section, "startbit", m_StartBitAddress, par_Fd);
    IniFileDataBaseWriteInt (section, "bitsize", m_BitSize, par_Fd);
    switch (m_ByteOrder) {
    case LsbFirst:
    //default:
        IniFileDataBaseWriteString (section, "byteorder", "lsb_first", par_Fd);
        break;
    case MsbFirst:
        IniFileDataBaseWriteString (section, "byteorder", "msb_first", par_Fd);
        break;
    }
    IniFileDataBaseWriteYesNo (section, "startvalue active", m_StartValueEnable, par_Fd);
    FloatOrInt64_to_string(m_StartValue, m_StartValueType, 10, Help, sizeof(Help));
    IniFileDataBaseWriteString (section, "startvalue", Help, par_Fd);

    switch (m_SignalType) {
    case MultiplexedBy:
        IniFileDataBaseWriteString (section, "type", "mux_by", par_Fd);
        IniFileDataBaseWriteString(section, "mux_by_signal", QStringToConstChar(m_MultiplexedBy), par_Fd);
        break;
    case Multiplexed:
        IniFileDataBaseWriteString (section, "type", "mux", par_Fd);
        break;
    case Normal:
    //default:
        IniFileDataBaseWriteString (section, "type", "normal", par_Fd);
        break;
    }
    IniFileDataBaseWriteInt (section, "mux_startbit", m_MultiplexedStartBit, par_Fd);
    IniFileDataBaseWriteInt (section, "mux_bitsize", m_MultiplexedBitSize, par_Fd);
    IniFileDataBaseWriteInt (section, "mux_value", m_MultiplexedValue, par_Fd);


    IniFileDataBaseWriteString (section, "bbtype", GetDataTypeName(m_BlackboardDataType), par_Fd);
    switch (m_Sign) {
    case Signed:
        IniFileDataBaseWriteString (section, "sign", "signed", par_Fd);
        break;
    case Float:
        IniFileDataBaseWriteString (section, "sign", "float", par_Fd);
        break;
    case Unsigned:
    //default:
        IniFileDataBaseWriteString (section, "sign", "unsigned", par_Fd);
        break;
    }
}

CanConfigElement *CanConfigSignal::child(int par_Number)
{
    Q_UNUSED(par_Number)
    return nullptr;  // Signale have no childs
}

int CanConfigSignal::childCount()
{
    return 0; // Signale have no childs
}

int CanConfigSignal::columnCount()
{
    return 2;
}

QVariant CanConfigSignal::data(int par_Column)
{
    switch (par_Column) {
    case 0:
        return m_Name;
    case 1:  // Order
        return QString().number(m_ParentObject->GetIndexOfChild(this));
    case 2:  // Description
        return m_Description;
    default:
        return QVariant ();
    }
}

CanConfigElement *CanConfigSignal::parent()
{
    return m_ParentObject;
}

int CanConfigSignal::childNumber()
{
    if (m_ParentObject) {
        return m_ParentObject->GetIndexOfChild(this);
    }
    return 0;
}

bool CanConfigSignal::setData(int par_Column, const QVariant &par_Value)
{
    Q_UNUSED(par_Column)
    Q_UNUSED(par_Value)
    return false;
}

CanConfigElement::Type CanConfigSignal::GetType()
{
    return Signal;
}

void CanConfigSignal::FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index)
{
    Q_UNUSED(par_Index)
    EnableTab(ui->tabWidget, 3);
    ui->tabWidget->setCurrentIndex(3);  // 3 -> Signal-Tab
    ui->tabWidget->setTabEnabled(3, true);
    ui->SignalNameLineEdit->setText(m_Name);
    ui->SignalDescriptionLineEdit->setText(m_Description);
    ui->SignalUnitLineEdit->setText(m_Unit);

    CanConfigObject *Object = static_cast<CanConfigObject*>(parent());
    if (Object != nullptr) {
        if (Object->GetDir() == CanConfigObject::Read) {
            ui->SignalConvertionLabel1->setText("Blackboard = ");
            ui->SignalConvertionLabel2->setText(" * CAN + ");
        } else {
            ui->SignalConvertionLabel1->setText("CAN = ");
            ui->SignalConvertionLabel2->setText(" * Blackboard + ");
        }
    }
    switch (m_ConvertionType) {
    case MultiplyFactorAddOffset:
        ui->SignalConvertionMultiplyAddRadioButton->setChecked(true);
        ui->SignalConvertionCurveLineEdit->clear();
        ui->SignalConvertionEquationLineEdit->clear();
        break;
    case AddOffsetMultiplyFactor:
        ui->SignalConvertionAddMultiplyRadioButton->setChecked(true);
        ui->SignalConvertionCurveLineEdit->clear();
        ui->SignalConvertionEquationLineEdit->clear();
        break;
    case Curve:
        ui->SignalConvertionCurveRadioButton->setChecked(true);
        ui->SignalConvertionCurveLineEdit->setText(m_ConversionEquationOrCurve);
        ui->SignalConvertionEquationLineEdit->clear();
        break;
    case Equation:
        ui->SignalConvertionEquationRadioButton->setChecked(true);
        ui->SignalConvertionCurveLineEdit->clear();
        ui->SignalConvertionEquationLineEdit->setText(m_ConversionEquationOrCurve);
        break;
    }
    ui->SignalConvertionFactorLineEdit->SetValue(m_ConversionFactor, m_ConversionFactorType);
    ui->SignalConvertionOffsetLineEdit->SetValue(m_ConversionOffset, m_ConversionOffsetType);

    ui->SignalStartBitAddressLineEdit->SetValue(m_StartBitAddress);
    ui->SignalBitSizeLineEdit->SetValue(m_BitSize);
    ui->SignalByteOrderComboBox->setCurrentIndex(m_ByteOrder);
    ui->SignalSignComboBox->setCurrentIndex(m_Sign);
    ui->SignalStartValueCheckBox->setChecked(m_StartValueEnable);
    ui->SignalStartValueLineEdit->SetValue(m_StartValue, m_StartValueType);
    ui->SignalBlackboardDataTypeComboBox->setCurrentText(QString(GetDataTypeName(m_BlackboardDataType)));
    switch (this->m_SignalType) {
    case Normal:
        ui->SignalTypeNormalRadioButton->setChecked(true);
        break;
    case Multiplexed:
        ui->SignalTypeMultiplexedRadioButton->setChecked(true);
        break;
    case MultiplexedBy:
        ui->SignalTypeMultiplexedByRadioButton->setChecked(true);
        break;
    }
    ui->SignalTypeMultiplexedByLineEdit->setText(m_MultiplexedBy);
    ui->SignalMultiplexedStartBitLineEdit->SetValue(m_MultiplexedStartBit);
    ui->SignalMultiplexedBitSizeLineEdit->SetValue(m_MultiplexedBitSize);
    ui->SignalMultiplexedValueLineEdit->SetValue(m_MultiplexedValue);
    ui->SignalMultiplexedByValueLineEdit->SetValue(m_MultiplexedValue);
}

void CanConfigSignal::StoreDialogTab(Ui::CanConfigDialog *ui)
{
    m_Name = ui->SignalNameLineEdit->text();
    m_Description = ui->SignalDescriptionLineEdit->text();
    m_Unit = ui->SignalUnitLineEdit->text();
    if (ui->SignalConvertionMultiplyAddRadioButton->isChecked()) {
        m_ConvertionType = MultiplyFactorAddOffset;
        m_ConversionEquationOrCurve = QString();
    } else if (ui->SignalConvertionAddMultiplyRadioButton->isChecked()) {
        m_ConvertionType = AddOffsetMultiplyFactor;
        m_ConversionEquationOrCurve = QString();
    } else if (ui->SignalConvertionCurveRadioButton->isChecked()) {
        m_ConvertionType = Curve;
        m_ConversionEquationOrCurve = ui->SignalConvertionCurveLineEdit->text();
    } else if (ui->SignalConvertionEquationRadioButton->isChecked()) {
        m_ConvertionType = Equation;
        m_ConversionEquationOrCurve = ui->SignalConvertionEquationLineEdit->text();
    } else {
        m_ConvertionType = MultiplyFactorAddOffset;
        m_ConversionEquationOrCurve = QString();
    }
    m_ConversionFactorType = ui->SignalConvertionFactorLineEdit->GetFloatOrInt64Value(&m_ConversionFactor);
    m_ConversionOffsetType = ui->SignalConvertionOffsetLineEdit->GetFloatOrInt64Value(&m_ConversionOffset);

    m_StartBitAddress = ui->SignalStartBitAddressLineEdit->GetIntValue();
    m_BitSize = ui->SignalBitSizeLineEdit->GetIntValue();
    m_ByteOrder = static_cast<enum ByteOrder>(ui->SignalByteOrderComboBox->currentIndex());
    m_Sign = static_cast<enum Sign>(ui->SignalSignComboBox->currentIndex());
    m_StartValueEnable = ui->SignalStartValueCheckBox->isChecked();
    m_StartValueType = ui->SignalStartValueLineEdit->GetFloatOrInt64Value(&m_StartValue);
    m_BlackboardDataType = GetDataTypebyName(QStringToConstChar(ui->SignalBlackboardDataTypeComboBox->currentText()));
    if (ui->SignalTypeNormalRadioButton->isChecked()) {
        m_SignalType = Normal;
    } else if (ui->SignalTypeMultiplexedRadioButton->isChecked()) {
        m_SignalType = Multiplexed;
    } else if (ui->SignalTypeMultiplexedByRadioButton->isChecked()) {
        m_SignalType = MultiplexedBy;
    } else {
        m_SignalType = Normal;
    }
    m_MultiplexedBy = ui->SignalTypeMultiplexedByLineEdit->text();
    m_MultiplexedStartBit = ui->SignalMultiplexedStartBitLineEdit->GetIntValue();
    m_MultiplexedBitSize = ui->SignalMultiplexedBitSizeLineEdit->GetIntValue();
    if (m_SignalType == MultiplexedBy) m_MultiplexedValue = ui->SignalMultiplexedByValueLineEdit->GetIntValue();
    else m_MultiplexedValue = ui->SignalMultiplexedValueLineEdit->GetIntValue();
}

void CanConfigSignal::AddNewChild(int par_Row)
{
    Q_UNUSED(par_Row)
    // nothing to do!
}

void CanConfigSignal::AddChild(int par_Row, CanConfigElement *par_Child)
{
    Q_UNUSED(par_Row)
    Q_UNUSED(par_Child)
    // nothing to do!
}

void CanConfigSignal::RemoveChild(int par_Row)
{
    Q_UNUSED(par_Row)
    // nothing to do!
}

void CanConfigSignal::SetParent(CanConfigObject *par_Parent)
{
    m_ParentObject = par_Parent;
}

bool CanConfigSignal::BitInsideSignal(int par_BitPos, enum ByteOrder par_ObjectByteOrder, int par_ObjectSize, bool *ret_FirstBit, bool *ret_LastBit)
{
    if (par_ObjectByteOrder == m_ByteOrder) {
        if (ret_FirstBit != nullptr) *ret_FirstBit = (par_BitPos == m_StartBitAddress);
        if (ret_LastBit != nullptr) *ret_LastBit = (par_BitPos == (m_StartBitAddress + m_BitSize - 1));
        return ((par_BitPos >= m_StartBitAddress) && (par_BitPos < m_StartBitAddress + m_BitSize));
    } else if ((par_ObjectByteOrder == LsbFirst) && (m_ByteOrder == MsbFirst)) {
        for (int x = 0; x < m_BitSize; x++) {
            int loc_BitPos = m_StartBitAddress + x;
            loc_BitPos = 8 * (par_ObjectSize - 1 - (loc_BitPos >> 3)) + (loc_BitPos & 0x7);
            if (par_BitPos == loc_BitPos) {
                if (ret_FirstBit != nullptr) *ret_FirstBit = (x == 0);
                if (ret_LastBit != nullptr) *ret_LastBit = (x == (m_BitSize - 1));
                return 1;
            }
        }
        if (ret_FirstBit != nullptr) *ret_FirstBit = false;
        if (ret_LastBit != nullptr) *ret_LastBit = false;
        return 0;
    } else if ((par_ObjectByteOrder == MsbFirst) && (m_ByteOrder == LsbFirst)) {
        int loc_BitPos = 8 * (par_ObjectSize - 1 - (par_BitPos >> 3)) + (par_BitPos & 0x7);
        if (ret_FirstBit != nullptr) *ret_FirstBit = (loc_BitPos == m_StartBitAddress);
        if (ret_LastBit != nullptr) *ret_LastBit = (loc_BitPos == (m_StartBitAddress + m_BitSize - 1));
        return ((loc_BitPos >= m_StartBitAddress) && (loc_BitPos < m_StartBitAddress + m_BitSize));
    }
    return false;
}


CanConfigTreeModel::CanConfigTreeModel(CanConfigBase *par_CanConfig, QObject *par_Parent)
{
    Q_UNUSED(par_Parent)
    m_RootItem = par_CanConfig;
}

QVariant CanConfigTreeModel::data(const QModelIndex &par_Index, int par_Role) const
{
    if (!par_Index.isValid()) {
        return QVariant();
    }
    CanConfigElement *Item = GetItem (par_Index);

    switch (par_Role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        // column:
        // 0 -> Name
        // 1 -> Description
        return Item->data (par_Index.column());
        //break;
    case Qt::TextAlignmentRole:
        if (par_Index.column() == 2) {
            return Qt::AlignRight;
        } else {
            return Qt::AlignLeft;
        }
        //break;
    default:
        return QVariant();
        //break;
    }
    //return QVariant();
}

QVariant CanConfigTreeModel::headerData(int par_Section, Qt::Orientation par_Orientation, int par_Role) const
{
    if (par_Orientation == Qt::Horizontal && par_Role == Qt::DisplayRole) {
        switch (par_Section) {
        case 0:
            return QString ("name");
        case 1:
            return QString ("order");
        case 2:
            return QString ("description");
        }
    }
    return QVariant();

}

QModelIndex CanConfigTreeModel::index(int par_Row, int par_Column, const QModelIndex &par_Parent) const
{
    if (par_Parent.isValid() && par_Parent.column() != 0) {
        return QModelIndex();
    }
    CanConfigElement *parentItem = GetItem(par_Parent);

    CanConfigElement *childItem = parentItem->child(par_Row);
    if (childItem) {
        return createIndex (par_Row, par_Column, childItem);
    } else {
        return QModelIndex();
    }
}

QModelIndex CanConfigTreeModel::parent(const QModelIndex &par_Index) const
{
    if (!par_Index.isValid()) {
        return QModelIndex();
    }
    CanConfigElement *childItem = GetItem(par_Index);
    CanConfigElement *parentItem = childItem->parent();

    if (parentItem == m_RootItem) {
        return QModelIndex();
    }
    return createIndex (parentItem->childNumber(), 0, parentItem);
}

int CanConfigTreeModel::rowCount(const QModelIndex &par_Parent) const
{
    CanConfigElement *parentItem = GetItem (par_Parent);
    return parentItem->childCount();
}

int CanConfigTreeModel::columnCount(const QModelIndex &par_Parent) const
{
    Q_UNUSED(par_Parent)
    return 3;
}

Qt::ItemFlags CanConfigTreeModel::flags(const QModelIndex &par_Index) const
{
    if (!par_Index.isValid()) {
        return Qt::NoItemFlags;
    }
    if (par_Index.column() == 0) {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(par_Index);
    } else {
        return QAbstractItemModel::flags(par_Index);
    }
}

bool CanConfigTreeModel::setData(const QModelIndex &par_Index, const QVariant &par_Value, int par_Role)
{
    if (par_Role != Qt::EditRole) {
        return false;
    }
    CanConfigElement *item = GetItem (par_Index);
    bool Result = item->setData (par_Index.column(), par_Value);

    if (Result) {
        emit dataChanged (par_Index, par_Index);
    }
    return Result;
}

bool CanConfigTreeModel::insertRow(int par_Row, const QModelIndex &par_Parent)
{
    CanConfigElement *ParentPtr = static_cast<CanConfigElement*>(par_Parent.internalPointer());
    beginInsertRows(par_Parent, par_Row, par_Row);
    ParentPtr->AddNewChild(par_Row);
    endInsertRows();
    return true;
}

bool CanConfigTreeModel::insertRow(int par_Row, CanConfigElement *par_NewElem, const QModelIndex &par_Parent)
{
    Q_UNUSED(par_Row)
    CanConfigElement *ParentPtr = static_cast<CanConfigElement*>(par_Parent.internalPointer());
    beginInsertRows(par_Parent, par_Row, par_Row);
    ParentPtr->AddChild(par_Row, par_NewElem);
    endInsertRows();
    return true;
}

bool CanConfigTreeModel::removeRow(int par_Row, const QModelIndex &par_Parent)
{
    CanConfigElement *ParentPtr = static_cast<CanConfigElement*>(par_Parent.internalPointer());
    beginRemoveRows(par_Parent, par_Row, par_Row);
    ParentPtr->RemoveChild(par_Row);
    endRemoveRows();
    /*xxxxx

    emit selectionChangedSignal()   // selectionChanged
      */
    return true;
}

CanConfigElement *CanConfigTreeModel::GetItem(const QModelIndex &par_Index) const
{
    if (par_Index.isValid()) {
        CanConfigElement *item = static_cast<CanConfigElement*>(par_Index.internalPointer());
        if (item) {
            return item;
        }
    }
    return m_RootItem;
}

QModelIndex CanConfigTreeModel::GetListViewRootIndex()
{
    return createIndex (0, 0, m_RootItem->child(0));
}

QModelIndex CanConfigTreeModel::SeachNextSignal(QModelIndex &par_BaseIndex, QModelIndex &par_StartIndex, QRegularExpression *par_RegExp, int par_StartRow)
{
    //static int RecursivCounter = 0;

    //RecursivCounter++;
    CanConfigElement *item = static_cast<CanConfigElement*>(par_StartIndex.internalPointer());
    int Type = item->GetType();
    // first all Childs
    int RowCount = rowCount(par_StartIndex);
    for (int Row = par_StartRow; Row < RowCount; Row++) {
        QModelIndex ChildIndex = index(Row, 0, par_StartIndex);
        CanConfigElement *Item = static_cast<CanConfigElement*>(ChildIndex.internalPointer());
        Type = Item->GetType();
        if (Item->GetType() == CanConfigElement::Signal) {
            CanConfigSignal* Signal = static_cast<CanConfigSignal*>(Item);
            QString Name = Signal->GetName();
            if (par_RegExp->match(Name).hasMatch()) {
                //RecursivCounter--;
                return ChildIndex;
            }
        } else {
            QModelIndex Index = SeachNextSignal(ChildIndex, ChildIndex, par_RegExp);
            if (Index.isValid()) {
                //RecursivCounter--;
                return Index;
            }
        }
    }
    if (par_StartIndex == par_BaseIndex) return QModelIndex();  // If come to the base
    // follwed by siblings
    QModelIndex ParentIndex = parent(par_StartIndex);
    QModelIndex Ret = SeachNextSignal (par_BaseIndex, ParentIndex, par_RegExp, par_StartIndex.row() + 1);
    //RecursivCounter--;
    return Ret;
}

void CanConfigTreeModel::SortObjectsById(QModelIndex &par_Index)
{
    if ((static_cast<CanConfigElement*>(par_Index.internalPointer()))->GetType() == CanConfigElement::Variant) {
        CanConfigVariant *Variant = static_cast<CanConfigVariant*>(par_Index.internalPointer());
        if (!Variant->m_Objects.isEmpty()) {
            beginRemoveRows(par_Index, 0, Variant->m_Objects.size() - 1);
            QList<CanConfigObject*> Objects = Variant->m_Objects;
            Variant->m_Objects.clear();
            endRemoveRows();
            beginInsertRows(par_Index, 0, Objects.size() - 1);
            Variant->m_Objects = Objects;
            Variant->SortObjectsByIdentifier();
            endInsertRows();
        }
    }
}

void CanConfigTreeModel::SortObjectsByName(QModelIndex &par_Index)
{
    if ((static_cast<CanConfigElement*>(par_Index.internalPointer()))->GetType() == CanConfigElement::Variant) {
        CanConfigVariant *Variant = static_cast<CanConfigVariant*>(par_Index.internalPointer());
        if (!Variant->m_Objects.isEmpty()) {
            beginRemoveRows(par_Index, 0, Variant->m_Objects.size() - 1);
            QList<CanConfigObject*> Objects = Variant->m_Objects;
            Variant->m_Objects.clear();
            endRemoveRows();
            beginInsertRows(par_Index, 0, Objects.size() - 1);
            Variant->m_Objects = Objects;
            Variant->SortObjectsByName();
            endInsertRows();
        }
    }
}

CanConfigTreeView::CanConfigTreeView(QWidget *par_Parent) : QTreeView(par_Parent)
{
    //setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    //setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

}

void CanConfigTreeView::selectionChanged(const QItemSelection &par_Selected, const QItemSelection &par_Deselected)
{
    QTreeView::selectionChanged(par_Selected, par_Deselected);
    emit selectionChangedSignal(par_Selected, par_Deselected);  // Signal pass through to Dialog class
}

QModelIndexList CanConfigTreeView::GetSelectedIndexes()
{
    return selectedIndexes();
}

void CanConfigTreeView::resizeEvent(QResizeEvent *par_Event)
{
    int NewWidth = par_Event->size().width();
    if (NewWidth != par_Event->oldSize().width()) {
        int s0 = header()->sectionSize(0);
        int s1 = header()->sectionSize(1);
        int s2 = header()->sectionSize(2);
        if (NewWidth > (s0 + s1 + s2)) {
            // only widen the first coulumn if the tree view widen
            setColumnWidth(0, NewWidth - (s1 + s2));
        }
    }
    QTreeView::resizeEvent(par_Event);
}

void CanConfigDialog::on_AddVariantToChannelPushButton_clicked()
{
    QModelIndex SelectedChannel = ui->MappingTreeWidget->currentIndex();
    if (SelectedChannel.isValid()) {
        if (!SelectedChannel.parent().isValid()) {
            QModelIndex Selected = ui->AllVariantListView->currentIndex();
            if (Selected.isValid()) {
                CanConfigVariant *Item = static_cast<CanConfigVariant*>(this->m_CanConfigTreeModel->GetItem(Selected));
                Item->AddToChannel(SelectedChannel.row()+1);
                m_CanConfig->m_Server->UpdateChannelMapping(ui);
            }
        }
    }
}

void CanConfigDialog::on_RemoveVariantFromPushButton_clicked()
{
    QModelIndex SelectedChannel = ui->MappingTreeWidget->currentIndex();
    if (SelectedChannel.isValid()) {
        int Row = SelectedChannel.row();
        QModelIndex Parent = SelectedChannel.parent();
        if (Parent.isValid()) {
            QModelIndex Index = Parent.model()->index(Row, 3, Parent);
            if (Index.isValid()) {
                QVariant v = Index.data(Qt::DisplayRole);
                if (v.isValid()) {
                    int i = v.toInt();
                    v = Parent.data(Qt::DisplayRole);
                    if (v.isValid()) {
                        int c = v.toInt();
                        m_CanConfig->m_Server->RemoveVariantFromChannel(i, c);
                        m_CanConfig->m_Server->UpdateChannelMapping(ui);
                    }
                }
            }
        }
    }
}

void CanConfigDialog::on_NumberOfChannelsSpinBox_valueChanged(int par_Value)
{
    m_CanConfig->m_Server->m_NumberOfChannels = par_Value;
    m_CanConfig->m_Server->UpdateChannelMapping(ui);
}


CanObjectModel::CanObjectModel(QObject *par_Parent)
    : QAbstractTableModel(par_Parent)
{
    m_Object = nullptr;
}

void CanObjectModel::SetCanObject(CanConfigObject *par_Object)
{
    beginResetModel();
    m_Object = par_Object;
    endResetModel();
    //reset();
}

int CanObjectModel::rowCount(const QModelIndex &par_Parent) const
{
    Q_UNUSED(par_Parent)
    if (m_Object == nullptr) return 0;
    return m_Object->GetSize();
}

int CanObjectModel::columnCount(const QModelIndex &par_Parent) const
{
    Q_UNUSED(par_Parent)
    return 8;   // alwasy 8 bits per column
}

QVariant CanObjectModel::data(const QModelIndex &par_Index, int par_Role) const
{
    if (!par_Index.isValid() || par_Role != Qt::DisplayRole)
        return QVariant();
    if (m_Object->m_ByteOrder == MsbFirst) {
        return QVariant().fromValue((7 - par_Index.column()) + 8 * (m_Object->GetSize() - 1 - par_Index.row()));
    } else {
        return QVariant().fromValue((7 - par_Index.column()) + 8 * par_Index.row());
    }
}

QVariant CanObjectModel::headerData(int par_Section, Qt::Orientation par_Orientation, int par_Role) const
{
    if (par_Role == Qt::DisplayRole) {
        if (par_Orientation == Qt::Horizontal) {
            return QString().number(7 - par_Section);
        } else if (par_Orientation == Qt::Vertical) {
            if (m_Object->m_ByteOrder == MsbFirst) {
                return QString().number(m_Object->GetSize() - 1 - par_Section);
            } else {
                return QString().number(par_Section);
            }
        }
    }
    return QVariant();
}

QModelIndex CanObjectModel::GetIndexOf(CanConfigSignal *par_Signal)
{
    int Row = m_Object->GetIndexOfChild(par_Signal);
    return createIndex (Row, 0, par_Signal);
}


QItemSelection CanObjectModel::GetModelIndexesOfCompleteSignal(CanConfigSignal *par_Signal) const
{
    QItemSelection Selections;

    if (par_Signal != nullptr) {
        for (int BitNr = par_Signal->GetStartBitAddress(); BitNr < par_Signal->GetStartBitAddress() + par_Signal->GetBitSize(); BitNr++) {
            // LSB first
            int Row = BitNr >> 3;
            int Column = 7 -  (BitNr & 0x7);
            if (par_Signal->GetByteOrder() == MsbFirst) {
                Row = m_Object->GetSize() - 1 - Row;
            }
            QModelIndex Index = createIndex(Row, Column, par_Signal);
            QItemSelectionRange SelectionRange(Index);
            Selections.append(SelectionRange);
        }
    }
    return Selections;
}

QItemSelection CanObjectModel::GetModelIndexesOfCompleteSignal(int par_BitNr) const
{
    QItemSelection Selections;

    CanConfigSignal *Signal = m_Object->BitInsideSignal(par_BitNr, nullptr, nullptr);
    if (Signal != nullptr) {
        GetModelIndexesOfCompleteSignal(Signal);
    }
    return Selections;
}


CanObjectBitDelegate::CanObjectBitDelegate(QObject *par_Parent)
    : QAbstractItemDelegate(par_Parent)
{
}

void CanObjectBitDelegate::paint(QPainter *par_Painter, const QStyleOptionViewItem &par_Option, const QModelIndex &par_Index) const
{
    CanConfigObject *Object = (static_cast<const CanObjectModel*>(par_Index.model()))->GetCanObject();
    int Value =  par_Index.model()->data(par_Index, Qt::DisplayRole).toInt();
    bool FirstBit;
    bool LastBit;
    CanConfigSignal *Signal = Object->BitInsideSignal(Value, &FirstBit, &LastBit);
    QColor Color = Object->GetColorOfSignal(Signal);

    par_Painter->save();
    if (par_Option.state & QStyle::State_Selected) {
        par_Painter->fillRect(par_Option.rect, par_Option.palette.highlight());
        par_Painter->setBrush(par_Option.palette.highlightedText());
    } else {
        par_Painter->fillRect(par_Option.rect, Color);
        par_Painter->setBrush(par_Option.palette.text());
    }
    par_Painter->drawText (par_Option.rect, Qt::AlignCenter, QString().number(Value));

    if (FirstBit) {
        par_Painter->drawLine (par_Option.rect.x() + par_Option.rect.width() - 1, par_Option.rect.y(), par_Option.rect.x() + par_Option.rect.width() - 1, par_Option.rect.y() + par_Option.rect.height());
    }
    if (LastBit) {
        par_Painter->drawLine (par_Option.rect.x(), par_Option.rect.y(), par_Option.rect.x(), par_Option.rect.y() + par_Option.rect.height());
    }
    if (Signal != nullptr) {
        par_Painter->drawLine (par_Option.rect.x(), par_Option.rect.y(), par_Option.rect.x() + par_Option.rect.width(), par_Option.rect.y());
        par_Painter->drawLine (par_Option.rect.x(), par_Option.rect.y() +  par_Option.rect.height() - 1, par_Option.rect.x() + par_Option.rect.width() - 1, par_Option.rect.y() + par_Option.rect.height() - 1);
    }
    par_Painter->restore();
}

QSize CanObjectBitDelegate::sizeHint(const QStyleOptionViewItem &par_Option, const QModelIndex &par_Index) const
{
    Q_UNUSED(par_Option)
    Q_UNUSED(par_Index)
    return QSize(16, 16);
}

CanSignalDelegate::CanSignalDelegate(QObject *par_Parent)
    : QAbstractItemDelegate(par_Parent)
{

}

void CanSignalDelegate::paint(QPainter *par_Painter, const QStyleOptionViewItem &par_Option, const QModelIndex &par_Index) const
{

    //if (0) {
    if (par_Index.isValid()) {
        CanConfigElement *Element = static_cast<CanConfigElement*>(par_Index.internalPointer());
        if (Element->GetType() == CanConfigElement::Signal) {
            CanConfigSignal *Signal = static_cast<CanConfigSignal*>(Element);
            CanConfigObject *Object = static_cast<CanConfigObject*>(Signal->parent());
            QColor Color = Object->GetColorOfSignal(Signal);

            par_Painter->save();
            if (par_Option.state & QStyle::State_Selected) {
                par_Painter->fillRect(par_Option.rect, par_Option.palette.highlight());
                par_Painter->setBrush(par_Option.palette.highlightedText());
            } else {
                par_Painter->fillRect(par_Option.rect, Color);
                par_Painter->setBrush(par_Option.palette.text());
            }
            par_Painter->drawText (par_Option.rect, Qt::AlignLeft, Signal->GetName());

            par_Painter->restore();
        }
    }
}

QSize CanSignalDelegate::sizeHint(const QStyleOptionViewItem &par_Option, const QModelIndex &par_Index) const
{
    Q_UNUSED(par_Option)
    Q_UNUSED(par_Index)
    return QSize(16, 16);
}

void CanConfigDialog::on_ObjectSignalBitPositionsTableView_pressed(const QModelIndex &par_Index)
{
    const CanObjectModel *Model = static_cast<const CanObjectModel*>(par_Index.model());
    int Value =  Model->data(par_Index, Qt::DisplayRole).toInt();
    QItemSelection SelectionRanges = Model->GetModelIndexesOfCompleteSignal(Value);
    QItemSelectionModel *SelectionModel = ui->ObjectSignalBitPositionsTableView->selectionModel();
    SelectionModel->select(static_cast<QItemSelection>(SelectionRanges), QItemSelectionModel::ClearAndSelect);

    CanConfigObject *Object = Model->GetCanObject();
    CanConfigSignal *Signal = Object->BitInsideSignal(Value);
    int Row = Object->GetIndexOfChild(Signal);
    QModelIndex Parent = ui->ObjectAllSignalsListView->rootIndex();
    ui->ObjectAllSignalsListView->setCurrentIndex(m_CanConfigTreeModel->index(Row, 0, Parent));
}

void CanConfigDialog::on_ObjectAllSignalsListView_clicked(const QModelIndex &par_Index)
{
    CanConfigSignal *Signal = static_cast<CanConfigSignal*>(par_Index.internalPointer());
    QItemSelection SelectionRanges = m_CanObjectModel->GetModelIndexesOfCompleteSignal(Signal); //->GetStartBitAddress());
    QItemSelectionModel *SelectionModel = ui->ObjectSignalBitPositionsTableView->selectionModel();
    SelectionModel->select(static_cast<QItemSelection>(SelectionRanges), QItemSelectionModel::ClearAndSelect);
}

void CanConfigDialog::on_ObjectInitValueAll0PushButton_clicked()
{
    QString Line = QString("0x0000000000000000");
    ui->ObjectInitValueLineEdit->setText(Line);
}

void CanConfigDialog::on_ObjectInitValueAll1PushButton_clicked()
{
    QString Line = QString("0xFFFFFFFFFFFFFFFF");
    ui->ObjectInitValueLineEdit->setText(Line);
}

void CanConfigDialog::on_ObjectTypeJ1939ConfigPushButton_clicked()
{
    QModelIndex SelectItem = ui->TreeView->currentIndex();
    if (SelectItem.isValid()) {
        CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectItem);
        if (Item->GetType() == CanConfigElement::Object) {
            switch(ui->ObjectTypeJ1939ComboBox->currentIndex()) {
            case 0:
                {
                    CanConfigJ1939AdditionalsDialog Dlg (static_cast<CanConfigObject*>(Item));
                    Dlg.exec();
                }
                break;
            case 1:
                {
                    CanConfigObject *Object = static_cast<CanConfigObject*>(Item);
                    Object->CheckExistanceOfAll_C_PGs();
                    int Dir = Object->GetDir();
                    QList<int> C_PGs = Object->Get_C_PGs();
                    CanConfigJ1939MultiPGDialog Dlg (m_CanConfigTreeModel, SelectItem, Dir, C_PGs, this);
                    if (Dlg.exec() == QDialog::Accepted) {
                        C_PGs = Dlg.Get_C_PGs();
                        Object->Set_C_PGs(C_PGs);
                    }
                }
                break;
            default:
                break;
            }
        }
    }
}

void CanConfigDialog::on_ObjectSizeSpinBox_valueChanged(int par_Value)
{
    QModelIndex SelectItem = ui->TreeView->currentIndex();
    if (SelectItem.isValid()) {
        CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectItem);
        if (Item->GetType() == CanConfigElement::Object) {
            (static_cast<CanConfigObject*>(Item))->SetSize(par_Value);
            m_CanObjectModel->SetCanObject(static_cast<CanConfigObject*>(Item));
        }
    }
}

void CanConfigDialog::on_ObjectTypeMultiplexedRadioButton_toggled(bool par_Checked)
{
    ui->ObjectTypeMultiplexedStartBitLineEdit->setEnabled(par_Checked);
    ui->ObjectTypeMultiplexedBitSizeLineEdit->setEnabled(par_Checked);
    ui->ObjectTypeMultiplexedValueLineEdit->setEnabled(par_Checked);
}

void CanConfigDialog::on_ObjectTypeJ1939RadioButton_toggled(bool par_Checked)
{
    ui->ObjectTypeJ1939ConfigPushButton->setEnabled(par_Checked);
    ui->ObjectTypeJ1939ComboBox->setEnabled(par_Checked);
}

void CanConfigDialog::on_ObjectByteOrderComboBox_currentTextChanged(const QString &par_Text)
{
    QModelIndex SelectItem = ui->TreeView->currentIndex();
    if (SelectItem.isValid()) {
        CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectItem);
        if (Item->GetType() == CanConfigElement::Object) {
            CanConfigObject *Object = static_cast<CanConfigObject*>(Item);
            if (!par_Text.compare(QString("msb_first"))) {
                Object->m_ByteOrder = MsbFirst;
            } else {
                Object->m_ByteOrder = LsbFirst;
            }
            m_CanObjectModel->SetCanObject(Object);
        }
    }
}

void CanConfigDialog::on_SignalTypeNormalRadioButton_toggled(bool par_Checked)
{
    if (par_Checked) {
        ui->SignalMultiplexedBitSizeLineEdit->setDisabled(true);
        ui->SignalMultiplexedStartBitLineEdit->setDisabled(true);
        ui->SignalMultiplexedValueLineEdit->setDisabled(true);
        ui->SignalMultiplexedByValueLineEdit->setDisabled(true);
        ui->SignalTypeMultiplexedByLineEdit->setDisabled(true);
    }
}

void CanConfigDialog::on_SignalTypeMultiplexedRadioButton_toggled(bool par_Checked)
{
    if (par_Checked) {
        ui->SignalMultiplexedBitSizeLineEdit->setEnabled(true);
        ui->SignalMultiplexedStartBitLineEdit->setEnabled(true);
        ui->SignalMultiplexedValueLineEdit->setEnabled(true);
        ui->SignalMultiplexedByValueLineEdit->setDisabled(true);
        ui->SignalTypeMultiplexedByLineEdit->setDisabled(true);
    }
}

void CanConfigDialog::on_SignalTypeMultiplexedByRadioButton_toggled(bool par_Checked)
{
    if (par_Checked) {
        ui->SignalMultiplexedBitSizeLineEdit->setDisabled(true);
        ui->SignalMultiplexedStartBitLineEdit->setDisabled(true);
        ui->SignalMultiplexedValueLineEdit->setDisabled(true);
        ui->SignalMultiplexedByValueLineEdit->setEnabled(true);
        ui->SignalTypeMultiplexedByLineEdit->setEnabled(true);
    }
}

CanConfigElement::~CanConfigElement()
{

}

void CanConfigDialog::on_EnableGatewayDeviceDriverCheckBox_clicked(bool par_Checked)
{
    Q_UNUSED(par_Checked)
    FillCANCardList(ui);
}

void CanConfigDialog::on_VirtualGatewayComboBox_currentIndexChanged(int par_Index)
{
    Q_UNUSED(par_Index)
    FillCANCardList(ui);
}

void CanConfigDialog::on_RealGatewayComboBox_currentIndexChanged(int par_Index)
{
    Q_UNUSED(par_Index)
    FillCANCardList(ui);
}


void CanConfigDialog::SetMaxObjectSize(bool par_SetFD,
                                       bool par_ResetFD,
                                       bool par_SetJ1939,
                                       bool par_SetNoJ1939)
{
    QModelIndex SelectItem = ui->TreeView->currentIndex();
    if (SelectItem.isValid()) {
        CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectItem);
        if (Item->GetType() == CanConfigElement::Object) {
            int Save = ui->ObjectSizeSpinBox->value();
            if (par_SetFD) {
                if (ui->ObjectTypeJ1939RadioButton->isChecked()) {
                    ui->ObjectSizeSpinBox->setRange(1, 1785); // J1939_MAX_DATA_LENGTH);
                } else {
                    ui->ObjectSizeSpinBox->setRange(1, 64);
                }
            } else if (par_ResetFD) {
                if (ui->ObjectTypeJ1939RadioButton->isChecked()) {
                    ui->ObjectSizeSpinBox->setRange(1, 1785); // J1939_MAX_DATA_LENGTH);
                } else {
                    ui->ObjectSizeSpinBox->setRange(1, 8);
                }
            } else if (par_SetJ1939) {
                ui->ObjectSizeSpinBox->setRange(1, 1785); // J1939_MAX_DATA_LENGTH);
            } else if (par_SetNoJ1939) {
                if (ui->ObjectBitRateSwitchCheckBox->isChecked()) {
                    ui->ObjectSizeSpinBox->setRange(1, 64);
                } else {
                    ui->ObjectSizeSpinBox->setRange(1, 8);
                }
            }
            ui->ObjectSizeSpinBox->setValue(Save);
        }
    }

}

void CanConfigDialog::on_ObjectBitRateSwitchCheckBox_stateChanged(int par_Value)
{
    SetMaxObjectSize(par_Value != 0, par_Value == 0, false, false);
}

void CanConfigDialog::on_ObjectTypeNormalRadioButton_clicked(bool par_Checked)
{
    Q_UNUSED(par_Checked)
    SetMaxObjectSize(false, false, false, true);
}

void CanConfigDialog::on_ObjectTypeMultiplexedRadioButton_clicked(bool par_Checked)
{
    Q_UNUSED(par_Checked)
    SetMaxObjectSize(false, false, false, true);
}

void CanConfigDialog::on_ObjectTypeJ1939RadioButton_clicked(bool par_Checked)
{
    Q_UNUSED(par_Checked)
    SetMaxObjectSize(false, false, true, false);
}

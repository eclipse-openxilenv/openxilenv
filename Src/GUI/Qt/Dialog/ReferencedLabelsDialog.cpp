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


#include <stdint.h>
#include "ReferencedLabelsDialog.h"
#include "ui_ReferencedLabelsDialog.h"
#include "StringHelpers.h"

#include <QItemSelectionModel>

extern "C" {
#include "Scheduler.h"
#include "Blackboard.h"
#include "IniDataBase.h"
#include "ThrowError.h"
#include "DebugInfoDB.h"
#include "ExtProcessReferences.h"
#include "GetNextStructEntry.h"
#include "ExtProcessReferences.h"
#include "UniqueNumber.h"
#include "DebugInfoAccessExpression.h"
}

ReferencedLabelsDialog::ReferencedLabelsDialog(QString par_ProcessName, QWidget *parent) : Dialog(parent),
    ui(new Ui::ReferencedLabelsDialog)
{
    m_ProcessName = par_ProcessName;
    CreateModel();

    ui->setupUi(this);
    // Loop through all processes
    char *Name;
    bool ProcessRunning  = false;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((Name = read_next_process_name (Buffer)) != nullptr) {
        if (!IsInternalProcess (Name)) {
            if (!Compare2ProcessNames (Name, QStringToConstChar(par_ProcessName))) {
                ProcessRunning = true;
            }
            ui->ProcessComboBox->addItem (CharToQString(Name));
        }
    }
    close_read_next_process_name(Buffer);
    if (!ProcessRunning) {
        if (par_ProcessName.isEmpty()) {
            ui->ProcessComboBox->addItem (QString (""));
            ui->ProcessComboBox->setCurrentText (QString (""));
        } else {
            ui->ProcessComboBox->addItem (par_ProcessName.append (QString (" (not running)")));
            ui->ProcessComboBox->setCurrentText (par_ProcessName.append (QString (" (not running)")));
        }
    } else {
        char ShortProcessName[MAX_PATH];
        TruncatePathFromProcessName (ShortProcessName, QStringToConstChar(par_ProcessName));
        ui->ProcessComboBox->setCurrentText (CharToQString(ShortProcessName));
    }
    FillModel(m_ProcessName);
    ui->treeView->setModel (m_Model);
    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);
}

ReferencedLabelsDialog::~ReferencedLabelsDialog()
{
    delete ui;
    delete m_Model;
}

void ReferencedLabelsDialog::accept()
{
    WriteModelToIni (m_ProcessName);
    QDialog::accept();
}

void ReferencedLabelsDialog::on_ProcessComboBox_currentIndexChanged(const QString &arg_ProcessName)
{
    FillModel (arg_ProcessName);
    ui->treeView->setModel (m_Model);
}


void ReferencedLabelsDialog::CreateModel (void)
{
    m_Model = new ReferencedLabelsDialogModel (this);
}


void ReferencedLabelsDialog::FillModel (const QString &par_Process)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    char displayname[BBVARI_NAME_SIZE];
    int Fd = GetMainFileDescriptor();

    m_Model->clear();

    m_Model->setColumnCount(2);
    m_Model->setHeaderData (0, Qt::Horizontal, QObject::tr("label"));
    m_Model->setHeaderData (1, Qt::Horizontal, QObject::tr("rename to"));

    TruncatePathFromProcessName (pname_withoutpath, QStringToConstChar(par_Process));
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), Fd) >= 0) {
        for (int Row = 0;;Row++) {
            sprintf (entry, "Ref%i", Row);
            if (IniFileDataBaseReadString (section, entry, "", variname, BBVARI_NAME_SIZE, Fd) == 0) break;
            m_Model->insertRow (Row);
            m_Model->setData (m_Model->index (Row, 0), CharToQString(variname));
            if (GetDisplayName (section, Row, variname, displayname, sizeof (displayname)) == 0) {
                ConvertLabelAsapCombatible (variname, sizeof(variname), 0);   // Replace :: with ._. if neccessary
                m_Model->setData (m_Model->index (Row, 1), QString (displayname));
            } else {
                m_Model->setData (m_Model->index (Row, 1), QString (""));
            }
        }
    }
}


int ReferencedLabelsDialog::UnreferenceOneVariable (int Pid, DEBUG_INFOS_ASSOCIATED_CONNECTION *DebugInfos, char *VariName, char *DisplayName)
{
    char Name[BBVARI_NAME_SIZE+100];
    strncpy (Name, VariName, BBVARI_NAME_SIZE);
    Name[BBVARI_NAME_SIZE-1] = 0;
    ConvertLabelAsapCombatible (Name, sizeof(Name), 2);   // Always replace ._. with ::
    uint64_t Address;
    int32_t TypeNr;
    int Type;
    int Ret = 0;
    if (appl_label (DebugInfos, Pid, Name, &Address, &TypeNr)) {
        ThrowError (1, "try to derefrence unknown variable %s", Name);
        Ret = -1;
    } else if ((Type = get_base_type_bb_type_ex (TypeNr, DebugInfos)) < 0) {
        ThrowError (1, "derefrence variable %s with unknown type", Name);
        Ret = -1;
    } else {
        char Help[BBVARI_NAME_SIZE + 100];  // +100 because of ConvertLabelAsapCombatible
        // Delete label from refernce liste inside INI file
        if ((DisplayName == nullptr) || (strlen(DisplayName) == 0)) {
            strcpy (Help, Name);
        } else {
            strncpy (Help, DisplayName, BBVARI_NAME_SIZE);
            Help[BBVARI_NAME_SIZE-1] = 0;
        }
        ConvertLabelAsapCombatible (Help, sizeof(Help), 0);   // Replace :: with ._. if neccessary
        if (scm_unref_vari (Address, Pid, Help, Type)) {
            ThrowError (1, "cannot dereference variable (extern process not running or halted in debugger)");
            Ret = -1;
        } else {
            Ret = 0;
        }
    }
    DelVarFromProcessRefList (Pid, Name);
    return Ret;
}

int ReferencedLabelsDialog::RenameReferenceOneVariable(int Pid, DEBUG_INFOS_ASSOCIATED_CONNECTION *DebugInfos,
                                                       const char *VariName, const char *OldDisplayName,
                                                       const char *NewDisplayName)
{
    char Name[BBVARI_NAME_SIZE];
    strcpy (Name, VariName);
    ConvertLabelAsapCombatible (Name, sizeof(Name), 2);   // Always replace ._. with ::
    uint64_t Address;
    int32_t TypeNr;
    int Type;
    int Ret = 0;
    if (appl_label (DebugInfos, Pid, Name, &Address, &TypeNr)) {
        ThrowError (1, "try to derefrence unknown variable %s", Name);
        Ret = -1;
    } else if ((Type = get_base_type_bb_type_ex (TypeNr, DebugInfos)) < 0) {
        ThrowError (1, "derefrence variable %s with unknown type", Name);
        Ret = -1;
    } else {
        char Help[BBVARI_NAME_SIZE + 100];  // +100 because of ConvertLabelAsapCombatible
        if (strlen(OldDisplayName)) {
            strncpy (Help, OldDisplayName, BBVARI_NAME_SIZE);
        } else {
            strncpy (Help, VariName, BBVARI_NAME_SIZE);
        }
        Help[BBVARI_NAME_SIZE] = 0;
        ConvertLabelAsapCombatible (Help, sizeof(Help), 0);   // Replace :: with ._. if neccessary
        if (scm_unref_vari (Address, Pid, Help, Type)) {
            ThrowError (1, "cannot dereference variable (extern process not running or halted in debugger)");
            Ret = -1;
        } else {
            Ret = 0;
        }
        if (strlen(NewDisplayName)) {
            strncpy (Help, NewDisplayName, BBVARI_NAME_SIZE);
        } else {
            strncpy (Help, VariName, BBVARI_NAME_SIZE);
        }
        Help[BBVARI_NAME_SIZE] = 0;
        ConvertLabelAsapCombatible (Help, sizeof(Help), 0);   // Replace :: with ._. if neccessary
        if (scm_ref_vari (Address, Pid, Help, Type, 0x3)) {
            ThrowError (1, "cannot reference variable (extern process not running or halted in debugger)");
            Ret = -1;
        } else {
            Ret = 0;
        }
    }
    return Ret;
}

void ReferencedLabelsDialog::WriteModelToIni (const QString &par_Process)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    char displayname[BBVARI_NAME_SIZE];


    TruncatePathFromProcessName (pname_withoutpath, QStringToConstChar(par_Process));
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), GetMainFileDescriptor()) >= 0) {
        DEBUG_INFOS_ASSOCIATED_CONNECTION *DebugInfos;
        int Pid;

        int UniqueNumber = AquireUniqueNumber ();

        DebugInfos = ConnectToProcessDebugInfos (UniqueNumber,
                                                 pname_withoutpath,
                                                 &Pid,
                                                 nullptr,
                                                 nullptr,
                                                 nullptr,
                                                 0);
        if (DebugInfos != nullptr) {
            for (int Row = 0;;Row++) {
                sprintf (entry, "Ref%i", Row);
                if (IniFileDataBaseReadString (section, entry, "", variname, BBVARI_NAME_SIZE, GetMainFileDescriptor()) == 0) break;
                if (GetDisplayName (section, Row, variname, displayname, sizeof (displayname)) != 0) {
                    displayname[0] = 0;    // no display name
                }
                // First look it should not be referenced any more
                int r;
                bool found = false;
                for (r = 0; r < m_Model->rowCount(); r++) {
                    QString ReferenceName = m_Model->item(r, 0)->data(Qt::DisplayRole).toString();
                    if (!CmpVariName(QStringToConstChar(ReferenceName), variname)) {
                        QString DisplayName = m_Model->item(r, 1)->data(Qt::DisplayRole).toString();
                        if (DisplayName.compare(QString(displayname))) {
                            // The display name has changed
                            RenameReferenceOneVariable (Pid, DebugInfos, variname, displayname, QStringToConstChar(DisplayName));
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    UnreferenceOneVariable (Pid, DebugInfos, variname, displayname);
                }
            }
            // Now delete entirety section
            IniFileDataBaseWriteString (section, nullptr, nullptr, GetMainFileDescriptor());

            for (int r = 0; r < m_Model->rowCount(); r++) {
                QString ReferenceName = m_Model->item(r, 0)->data(Qt::DisplayRole).toString();
                QString DisplayName = m_Model->item(r, 1)->data(Qt::DisplayRole).toString();
                char Entry[32];
                sprintf (Entry, "Ref%i", r);
                IniFileDataBaseWriteString (section, Entry, QStringToConstChar(ReferenceName), GetMainFileDescriptor());
                // If display name not equal variable name?
                if ((DisplayName.size() > 0) && DisplayName.compare(ReferenceName)) {
                    char Help[INI_MAX_LINE_LENGTH];
                    sprintf (Entry, "Dsp%i", r);
                    sprintf (Help, "%s %s", QStringToConstChar(ReferenceName), QStringToConstChar(DisplayName));
                    IniFileDataBaseWriteString (section, Entry, Help, GetMainFileDescriptor());
                }
            }
            RemoveConnectFromProcessDebugInfos (UniqueNumber);
            FreeUniqueNumber (UniqueNumber);
        }
    }
}

void ReferencedLabelsDialog::on_DereferenceLabelPushButton_clicked()
{
    QModelIndexList Selected = ui->treeView->selectionModel()->selectedIndexes();

    // Must be sorted climb down
    std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top

    foreach (QModelIndex Index, Selected) {
        if (Index.column() == 0) {
            if (m_Model->removeRow(Index.row(), Index.parent())) {
                // do nothing
            }
        }
    }
}

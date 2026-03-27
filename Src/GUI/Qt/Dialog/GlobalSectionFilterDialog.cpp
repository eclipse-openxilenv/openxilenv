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


#include "GlobalSectionFilterDialog.h"
#include "ui_GlobalSectionFilterDialog.h"

#include "AddFilterDialog.h"
#include "StringHelpers.h"

extern "C"
{
    #include "StringMaxChar.h"
    #include "SectionFilter.h"
    #include "Scheduler.h"
    #include "DebugInfoDB.h"
    #include "MyMemory.h"
    #include "Wildcards.h"
    #include "UniqueNumber.h"
    #include "IniDataBase.h"
}

globalsectionfilterdialog::globalsectionfilterdialog(QWidget *parent) : Dialog(parent),
    sfdp(new SECTION_FILTER_DLG_PARAM),
    ui(new Ui::globalsectionfilterdialog)

{
    ui->setupUi(this);

    UniqueId = AquireUniqueNumber();

    int x;
    INCLUDE_EXCLUDE_FILTER *Filter;

    sfdp->Section =  "BasicSettings";
    sfdp->EntryPrefix = "GlobalSection_";
    sfdp->ProcessName = nullptr;
    sfdp->DlgName = "Global Section Filter";
    sfdp->AllProcsSelected = 1;
    sfdp->ProcListEnabled = 0;

    char *Name;
    bool ProcessRunning  = false;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((Name = read_next_process_name (Buffer)) != nullptr) {
        if (!IsInternalProcess (Name)) {
                ProcessRunning = true;
            ui->listWidgetProcesses->addItem(QString(Name));
        }
    }
    close_read_next_process_name(Buffer);
    if (sfdp->AllProcsSelected) {
        for (x = 0; x < ui->listWidgetProcesses->count(); x++) {
            char ProcName[MAX_PATH];
            STRING_COPY_TO_ARRAY(ProcName, QStringToConstChar(ui->listWidgetProcesses->item(x)->text()));
            addSectionToListBox (ProcName);
        }
    } else {
        // Zuletzt ausgewaehlten Prozess selektieren
        char CurProcess[MAX_PATH];
        STRING_COPY_TO_ARRAY(CurProcess, QStringToConstChar(ui->listWidgetProcesses->currentItem()->text()));
        QListWidgetItem *ProcessItem = new QListWidgetItem;
        ProcessItem->setText(CurProcess);
        ui->listWidgetProcesses->setCurrentItem(ProcessItem);
        addSectionToListBox (CurProcess);
    }
    Filter = BuildIncludeExcludeFilterFromIni (sfdp->Section, sfdp->EntryPrefix, GetMainFileDescriptor());
    if (Filter != nullptr) {
        int i, e;

        for (i = 0; i < Filter->IncludeSize; i++) {
            ui->listWidgetIncludeSection->addItem(Filter->IncludeArray[i]);
        }
        for (e = 0; e < Filter->ExcludeSize; e++) {
            ui->listWidgetExcludeSection->addItem(Filter->ExcludeArray[e]);
        }
        FreeIncludeExcludeFilter (Filter);
    }
}

globalsectionfilterdialog::~globalsectionfilterdialog()
{
    delete sfdp;
    delete ui;

    FreeUniqueNumber (UniqueId);
}

void globalsectionfilterdialog::on_pushButtonIncludeNew_clicked()
{
    /* include = 0 exclude = 1 */
    addFilterDialog Dlg(false);

    if (Dlg.exec() == QDialog::Accepted) {
        ui->listWidgetIncludeSection->addItem(Dlg.getFilterName());
    }
}

void globalsectionfilterdialog::on_pushButtonExludeNew_clicked()
{
    /* include = 0 exclude = 1 */
    addFilterDialog Dlg(true);

    if (Dlg.exec() == QDialog::Accepted) {
        ui->listWidgetExcludeSection->addItem(Dlg.getFilterName());
    }
}

void globalsectionfilterdialog::on_pushButtonIncludeAdd_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetIncludeAvailableSection->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString SectionName = Item->data(Qt::DisplayRole).toString();
        if (ui->listWidgetIncludeSection->findItems(SectionName, Qt::MatchFixedString).isEmpty()) {
            ui->listWidgetIncludeSection->addItem(SectionName);
        }
    }
}

void globalsectionfilterdialog::on_pushButtonExcludeAdd_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetExcludeAvailableSection->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString SectionName = Item->data(Qt::DisplayRole).toString();
        if (ui->listWidgetExcludeSection->findItems(SectionName, Qt::MatchFixedString).isEmpty()) {
            ui->listWidgetExcludeSection->addItem(SectionName);
        }
    }
}

void globalsectionfilterdialog::on_pushButtonExcludeDel_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetExcludeSection->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void globalsectionfilterdialog::on_pushButtonIncludeDel_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->listWidgetIncludeSection->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void globalsectionfilterdialog::addIncludeExcludeSection(QString section, bool includeExclude)
{
    /* include = 0 exclude = 1 */
    if (includeExclude)
    {
        ui->listWidgetExcludeSection->addItem(section);
    } else {
        ui->listWidgetIncludeSection->addItem(section);
    }
}

void globalsectionfilterdialog::addSectionToListBox (char *ProcessName)
{
    PROCESS_APPL_DATA *pappldata;
    int x;

    if (ProcessName != nullptr) {
        int Pid;
        pappldata = ConnectToProcessDebugInfos (UniqueId,
                                                ProcessName,
                                                &Pid,
                                                nullptr,
                                                nullptr,
                                                nullptr,
                                                1);
        if (pappldata != nullptr) {
            DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
            DEBUG_INFOS_DATA *AssociatedDebugInfos;

            AssociatedProcess = pappldata->AssociatedProcess;
            if (AssociatedProcess != nullptr) {
                AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
                if (AssociatedDebugInfos != nullptr) {
                    if ((AssociatedDebugInfos->SectionSizeOfRawData != nullptr) ||
                        (AssociatedDebugInfos->SectionVirtualSize == nullptr) ||
                        (AssociatedDebugInfos->SectionVirtualAddress == nullptr) ||
                        (AssociatedDebugInfos->SectionNames == nullptr) ||
                        (AssociatedDebugInfos->NumOfSections > 0)) {
                        for (x = 0; x < AssociatedDebugInfos->NumOfSections; x++)
                        {
                            if (strlen(AssociatedDebugInfos->SectionNames[x])) { // GCC mach NULL Sections, die wollen wir nicht sehen
                                if (ui->listWidgetExcludeAvailableSection->findItems(AssociatedDebugInfos->SectionNames[x], Qt::MatchExactly).isEmpty()) {
                                    ui->listWidgetExcludeAvailableSection->addItem(AssociatedDebugInfos->SectionNames[x]);
                                    ui->listWidgetIncludeAvailableSection->addItem(AssociatedDebugInfos->SectionNames[x]);
                                }
                            }
                        }
                    }
                }
            }
            RemoveConnectFromProcessDebugInfos(UniqueId);
        }
    }
}

void globalsectionfilterdialog::accept()
{
    QString String;
    QByteArray ByteArray;
    QList<QListWidgetItem*> AllItems;
    INCLUDE_EXCLUDE_FILTER *Filter = static_cast<INCLUDE_EXCLUDE_FILTER*>(my_calloc (1, sizeof (INCLUDE_EXCLUDE_FILTER)));

    AllItems = ui->listWidgetIncludeSection->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Filter->IncludeSize = AllItems.count();
    if (Filter->IncludeSize > 0) {
        Filter->IncludeArray = static_cast<char**>(my_calloc (static_cast<size_t>(Filter->IncludeSize), sizeof (char*)));
        int i = 0;
        foreach (QListWidgetItem *Item, AllItems) {
            String = Item->data(Qt::DisplayRole).toString();
            Filter->IncludeArray[i] = MallocCopyString(String);
            if (Filter->IncludeArray[i] == nullptr) return;
            i++;
        }
    }

    AllItems = ui->listWidgetExcludeSection->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Filter->ExcludeSize = AllItems.count();
    if (Filter->ExcludeSize > 0) {
        Filter->ExcludeArray = static_cast<char**>(my_calloc (static_cast<size_t>(Filter->ExcludeSize), sizeof (char*)));
        int e = 0;
        foreach (QListWidgetItem *Item, AllItems) {
            String = Item->data(Qt::DisplayRole).toString();
            Filter->ExcludeArray[e] = MallocCopyString(String);
            if (Filter->ExcludeArray[e] == nullptr) return;
            e++;
        }
    }


    if (Filter != nullptr) {
        SaveIncludeExcludeListsToIni (Filter, sfdp->Section, sfdp->EntryPrefix,  GetMainFileDescriptor());
        FreeIncludeExcludeFilter (Filter);
    }
    QDialog::accept();
}

void globalsectionfilterdialog::on_listWidgetProcesses_itemClicked(QListWidgetItem *item)
{
    char CurProcess[MAX_PATH];
    STRING_COPY_TO_ARRAY(CurProcess, QStringToConstChar(item->text()));
    ui->listWidgetProcesses->setCurrentItem(item);
    ui->listWidgetExcludeAvailableSection->clear();
    ui->listWidgetIncludeAvailableSection->clear();
    addSectionToListBox (CurProcess);
}

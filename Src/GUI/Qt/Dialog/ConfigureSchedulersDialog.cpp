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


#include "Platform.h"
#include "ConfigureSchedulersDialog.h"
#include "ui_ConfigureSchedulersDialog.h"

#include "AddNewProcessSchedulerBarrier.h"
#include "ConfigureBarrierDialog.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

extern "C" {
    #include "Scheduler.h"
    #include "MyMemory.h"
    #include "ThrowError.h"
    #include "Files.h"
}

ConfigureSchedulersDialog::ConfigureSchedulersDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::ConfigureSchedulersDialog)
{
    ui->setupUi(this);
    ui->SchedulerPrioComboBox->addItem(QString("THREAD_PRIORITY_IDLE"), THREAD_PRIORITY_IDLE);
    ui->SchedulerPrioComboBox->addItem(QString("THREAD_PRIORITY_LOWEST"), THREAD_PRIORITY_LOWEST);
    ui->SchedulerPrioComboBox->addItem(QString("THREAD_PRIORITY_BELOW_NORMAL"), THREAD_PRIORITY_BELOW_NORMAL);
    ui->SchedulerPrioComboBox->addItem(QString("THREAD_PRIORITY_NORMAL"), THREAD_PRIORITY_NORMAL);
    ui->SchedulerPrioComboBox->addItem(QString("THREAD_PRIORITY_ABOVE_NORMAL"), THREAD_PRIORITY_ABOVE_NORMAL);
    ui->SchedulerPrioComboBox->addItem(QString("THREAD_PRIORITY_HIGHEST"), THREAD_PRIORITY_HIGHEST);
    ui->SchedulerPrioComboBox->addItem(QString("THREAD_PRIORITY_TIME_CRITICAL"), THREAD_PRIORITY_TIME_CRITICAL);

    InsertAllBarriers();
    ReadAllSchedulerInfos2Struct();

    m_SchedulerInfosSave = m_SchedulerInfos;

    ui->SchedulersListWidget->setCurrentRow(0);  // Select first scheduler
}

ConfigureSchedulersDialog::~ConfigureSchedulersDialog()
{
    delete ui;
}


bool ConfigureSchedulersDialog::StructContainsSchedulerName (QString par_SchedulerName)
{
    QMap<QString, struct ConfigureSchedulersDialog::DialogData>::iterator i;

    for (i = m_SchedulerInfos.begin(); i != m_SchedulerInfos.end(); ++i) {
        QString SchedulerName(i.key());
        if (!SchedulerName.compare (par_SchedulerName, Qt::CaseInsensitive)) return true;
    }
    return false;
}


void ConfigureSchedulersDialog::ReadAllSchedulerInfos2Struct()
{
    // The first scheduler always exist with index 0
    QString Scheduler("Scheduler");
    ui->SchedulersListWidget->addItem (Scheduler);
    m_SchedulerInfos.insert ("Scheduler", ConfigureSchedulersDialog::ReadSchedulerInfosFromIni2Struct (Scheduler));
    int Fd = GetMainFileDescriptor();
    // Additional scheduler
    for (int x = 1; ; x++) {
        char Entry[64];
        char Line[INI_MAX_LINE_LENGTH];
        sprintf (Entry, "Scheduler_%i", x);
        if (IniFileDataBaseReadString ("Scheduler", Entry, "", Line, sizeof (Line), Fd) <= 0) {
            break;
        }
        QString QStringLine = CharToQString(Line);
        ui->SchedulersListWidget->addItem (QStringLine);
        m_SchedulerInfos.insert (Line, ConfigureSchedulersDialog::ReadSchedulerInfosFromIni2Struct (QStringLine));
    }
}

void ConfigureSchedulersDialog::InsertAllBarriers()
{
    char Entry[64];
    char Line[INI_MAX_LINE_LENGTH];
    for (int b = 0; ; b++) {
        sprintf (Entry, "Barrier_%i", b);
        if (IniFileDataBaseReadString ("SchedulerBarriersConfiguration", Entry, "", Line, sizeof (Line), GetMainFileDescriptor()) <= 0) {
            break;
        }
        char *BarrierName;
        char *WaitIfAlone;
        int WaitIfAloneFlag = 0;
        if (StringCommaSeparate (Line, &BarrierName, &WaitIfAlone, NULL) >= 2) {
            if (!strcmpi (WaitIfAlone, "WaitIfAlone")) {
                WaitIfAloneFlag = 1;
            }
        } else {
            BarrierName = Line;
        }

        ui->AvailableBeforeBarrierListWidget->addItem(CharToQString(BarrierName));
        ui->AvailableBehindBarrierListWidget->addItem(CharToQString(BarrierName));
    }
}


void ConfigureSchedulersDialog::UpdateSchedulerListView()
{
    ui->SchedulersListWidget->clear();
    QMap<QString, struct ConfigureSchedulersDialog::DialogData>::iterator i;

    for (i = m_SchedulerInfos.begin(); i != m_SchedulerInfos.end(); ++i) {
        QString SchedulerName(i.key());
        ui->SchedulersListWidget->addItem (SchedulerName);
    }
}


struct ConfigureSchedulersDialog::DialogData ConfigureSchedulersDialog::ReadSchedulerInfosFromIni2Struct(QString &par_Scheduler)
{
    QString Entry;
    struct DialogData Ret;
    int Fd = ScQt_GetMainFileDescriptor();
    Entry = QString("BarriersBeforeOnlySignalForScheduler ").append(par_Scheduler);
    Ret.m_BarriersBeforeOnlySignal = ScQt_IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", Fd);
    Entry = QString("BarriersBeforeSignalAndWaitForScheduler ").append(par_Scheduler);
    Ret.m_BarriersBeforeSignalAndWait = ScQt_IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", Fd);
    Entry = QString("BarriersBehindOnlySignalForScheduler ").append(par_Scheduler);
    Ret.m_BarriersBehindOnlySignal = ScQt_IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", Fd);
    Entry = QString("BarriersBehindSignalAndWaitForScheduler ").append(par_Scheduler);
    Ret.m_BarriersBehindSignalAndWait = ScQt_IniFileDataBaseReadString(SCHED_INI_SECTION, Entry, "", Fd);
    Entry = QString("PriorityForScheduler ").append(par_Scheduler);
    Ret.m_SchedulerPrio = ScQt_IniFileDataBaseReadInt(SCHED_INI_SECTION, Entry, THREAD_PRIORITY_IDLE, Fd);
    if (Ret.m_SchedulerPrio < THREAD_PRIORITY_IDLE) Ret.m_SchedulerPrio = THREAD_PRIORITY_IDLE;
    if (Ret.m_SchedulerPrio > THREAD_PRIORITY_TIME_CRITICAL) Ret.m_SchedulerPrio = THREAD_PRIORITY_TIME_CRITICAL;
    return Ret;
}

static void FillBarrierListWidget (QString &String, QListWidget *ListWidget)
{
    char *p, *pp;

    ListWidget->clear();
    QStringList List = String.split(";");
    for(int x = 0; x < List.size(); x++) {
        ListWidget->addItem (List.at(x).trimmed());
    }
}

void ConfigureSchedulersDialog::SetSchedulerInfosFromStruct2Dialog (QString &par_SelectedSchduler)
{
    if (!par_SelectedSchduler.isEmpty()) {
        struct DialogData Data = m_SchedulerInfos.value(par_SelectedSchduler);

        FillBarrierListWidget (Data.m_BarriersBeforeOnlySignal, ui->BeforeBarrierListWidget);
        FillBarrierListWidget (Data.m_BarriersBeforeSignalAndWait, ui->BeforeBarrierWaitListWidget);
        FillBarrierListWidget (Data.m_BarriersBehindOnlySignal, ui->BehindBarrierListWidget);
        FillBarrierListWidget (Data.m_BarriersBehindSignalAndWait, ui->BehindBarrierWaitListWidget);
        for (int x = 0; x < ui->SchedulerPrioComboBox->count(); x++) {
            if (Data.m_SchedulerPrio >= ui->SchedulerPrioComboBox->itemData(x).toInt()) {
                ui->SchedulerPrioComboBox->setCurrentIndex(x);
            }
        }
    }
}

void ConfigureSchedulersDialog::StoreSchedulerInfosFromStruct2Ini (QString &par_SchedulerName)
{
    int Fd = ScQt_GetMainFileDescriptor();
    QString Section("Scheduler");

    if (!par_SchedulerName.isEmpty()) {
        struct DialogData Data = GetSchedulerInfosFromDialog2Struct (par_SchedulerName);
        if (Data.m_Changed) {
            QString Entry;
            if (par_SchedulerName.compare("Scheduler")) {
                for (int x = 1; ; x++) {
                    Entry = QString("Scheduler_%1").arg(x);
                    QString Line = ScQt_IniFileDataBaseReadString("Scheduler", Entry, "", Fd);
                    if (!Line.isEmpty()) {
                        ScQt_IniFileDataBaseWriteString(Section, Entry, par_SchedulerName, Fd);
                        break;
                    }
                    if (!par_SchedulerName.compare(Line)) {
                        break;   // Is already inside the list
                    }
                }
            }

            Entry = QString("BarriersBeforeOnlySignalForScheduler ").append(par_SchedulerName);
            ScQt_IniFileDataBaseWriteString(Section, Entry, Data.m_BarriersBeforeOnlySignal, Fd);

            Entry = QString("BarriersBeforeSignalAndWaitForScheduler s").append(par_SchedulerName);
            ScQt_IniFileDataBaseWriteString(Section, Entry, Data.m_BarriersBeforeSignalAndWait, Fd);

            Entry =  QString("BarriersBehindOnlySignalForScheduler ").append(par_SchedulerName);
            ScQt_IniFileDataBaseWriteString(Section, Entry, Data.m_BarriersBehindOnlySignal, Fd);

            Entry = QString("BarriersBehindSignalAndWaitForScheduler ").append(par_SchedulerName);
            ScQt_IniFileDataBaseWriteString(Section, Entry, Data.m_BarriersBehindSignalAndWait, Fd);

            Entry = QString("PriorityForScheduler").append(par_SchedulerName);
            ScQt_IniFileDataBaseWriteInt (Section, Entry, Data.m_SchedulerPrio, Fd);
        }
    }
}

struct ConfigureSchedulersDialog::DialogData ConfigureSchedulersDialog::GetSchedulerInfosFromDialog2Struct(QString &par_Scheduler)
{
    struct DialogData Ret = m_SchedulerInfos.value (par_Scheduler);
    QList<QListWidgetItem*> AllItems;
    int Count;

    AllItems = ui->BeforeBarrierListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersBeforeOnlySignal.clear();
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) Ret.m_BarriersBeforeOnlySignal.append(";");
        Ret.m_BarriersBeforeOnlySignal.append(Item->data(Qt::DisplayRole).toString());
    }
    AllItems = ui->BeforeBarrierWaitListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersBeforeSignalAndWait.clear();
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) Ret.m_BarriersBeforeSignalAndWait.append(";");
        Ret.m_BarriersBeforeSignalAndWait.append(Item->data(Qt::DisplayRole).toString());
    }
    AllItems = ui->BehindBarrierListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersBehindOnlySignal.clear();
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) Ret.m_BarriersBehindOnlySignal.append(";");
        Ret.m_BarriersBehindOnlySignal.append(Item->data(Qt::DisplayRole).toString());
    }
    AllItems = ui->BehindBarrierWaitListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret.m_BarriersBehindSignalAndWait.clear();
    Count = 0;
    foreach (QListWidgetItem *Item, AllItems) {
        if (Count++) Ret.m_BarriersBehindSignalAndWait.append(";");
        Ret.m_BarriersBehindSignalAndWait.append(Item->data(Qt::DisplayRole).toString());
    }

    Ret.m_SchedulerPrio = ui->SchedulerPrioComboBox->currentData().toInt();

    return Ret;
}


void ConfigureSchedulersDialog::on_AddSchedulerPushButton_clicked()
{
    AddNewProcessSchedulerBarrier Dlg (QString ("scheduler"));
    if (Dlg.exec() == QDialog::Accepted) {
        QString NewScheduler = Dlg.GetNew();
        if (!NewScheduler.isEmpty()) {
            if (!StructContainsSchedulerName (NewScheduler)) {
                struct DialogData New;
                memset (&New, 0, sizeof (New));
                New.m_Changed = 1;  // Mark new scheduler immediately  as changed otherwise it will not stored
                New.m_SchedulerPrio = THREAD_PRIORITY_IDLE;  // default is always lowest prio
                m_SchedulerInfos.insert (NewScheduler, New);
                UpdateSchedulerListView();
            } else {
                ThrowError (1, "scheduler name \"%s\" already used", QStringToConstChar(NewScheduler));
            }
        }
    }
}

void ConfigureSchedulersDialog::on_DeleteSchedulerPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->SchedulersListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString SchedulerName = Item->data(Qt::DisplayRole).toString();
        if (!SchedulerName.compare("Scheduler")) {
            ThrowError (1, "cannot delete main scheduler \"Scheduler\"");
        } else {
            m_SchedulerInfos.remove (SchedulerName);
        }
    }
    UpdateSchedulerListView ();
}


int ConfigureSchedulersDialog::CheckChanges (QString &SelectedScheduler)
{
    if (!SelectedScheduler.isEmpty() &&
        m_SchedulerInfos.contains (SelectedScheduler)) {   // Process must be still exist
        struct DialogData Prev = GetSchedulerInfosFromDialog2Struct (SelectedScheduler);
        struct DialogData Old = m_SchedulerInfos.value (SelectedScheduler);
        if (memcmp (&Old, &Prev, sizeof (Prev))) {
            // There was changed something
            Prev.m_Changed = 1;
            m_SchedulerInfos.insert (SelectedScheduler, Prev);  // overwrite
            return 1;
        }
    }
    return 0;
}


void ConfigureSchedulersDialog::on_SchedulersListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (previous != nullptr) {
        // Store the data of the previos selectd processes
        QString PreviousSelectedScheduler = previous->data(Qt::DisplayRole).toString();
        if (!PreviousSelectedScheduler.isEmpty()) {
            CheckChanges (PreviousSelectedScheduler);
        }
    }
    if (current != nullptr) {
        QString NewSelectedScheduler = current->data(Qt::DisplayRole).toString();
        if (!NewSelectedScheduler.isEmpty()) {
            SetSchedulerInfosFromStruct2Dialog (NewSelectedScheduler);
        }
    }
}

void ConfigureSchedulersDialog::on_EditBarrierPushButton_clicked()
{
    ConfigureBarrierDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
    }
}


void ConfigureSchedulersDialog::DeleteSchedulerInfosFromIni (QString par_Scheduler)
{
    int Fd = GetMainFileDescriptor();
    for (int x = 1; ; x++) {
        char Entry[64];
        char Line[INI_MAX_LINE_LENGTH];
        sprintf (Entry, "Scheduler_%i", x);
        if (IniFileDataBaseReadString ("Scheduler", Entry, "", Line, sizeof (Line), Fd) <= 0) break;
        if (!strcmp (QStringToConstChar(par_Scheduler), Line)) {
            ScQt_IniFileDataBaseWriteString ("Scheduler", Entry, nullptr, Fd);
            for (x++; ; x++) {
                sprintf (Entry, "Scheduler_%i", x);
                if (IniFileDataBaseReadString ("Scheduler", Entry, "", Line, sizeof (Line), Fd) <= 0) break;
                ScQt_IniFileDataBaseWriteString ("Scheduler", Entry, nullptr, Fd);
                sprintf (Entry, "Scheduler_%i", x - 1);
                ScQt_IniFileDataBaseWriteString ("Scheduler", Entry, Line, Fd);
            }
            sprintf (Entry, "PriorityForScheduler %s", QStringToConstChar(par_Scheduler));
            ScQt_IniFileDataBaseWriteString ("Scheduler", Entry, nullptr, Fd);

            RemoveProcessOrSchedulerFromAllBarrierIniConfig (QStringToConstChar(par_Scheduler), 0);
            // Scheduler namen should be exist only one time
            break;
        }
    }
}

void ConfigureSchedulersDialog::SaveAll()
{
    QMap<QString, struct ConfigureSchedulersDialog::DialogData>::iterator i;
    QMap<QString, struct ConfigureSchedulersDialog::DialogData>::iterator j;

    // First all scheduler that should be deleted
    for (j = m_SchedulerInfosSave.begin(); j != m_SchedulerInfosSave.end(); ++j) {
        int Found = 0;
        QString SchedulerNameJ (j.key());
        for (i = m_SchedulerInfos.begin(); i != m_SchedulerInfos.end(); ++i) {
            QString SchedulerNameI (i.key());
            if (!SchedulerNameI.compare (SchedulerNameJ)) {
                Found = 1;
                break;
            }
        }
        if (!Found) {
            DeleteSchedulerInfosFromIni (j.key());
        }
    }
    for (i = m_SchedulerInfos.begin(); i != m_SchedulerInfos.end(); ++i) {
        QString SchedulerName (i.key());
        StoreSchedulerInfosFromStruct2Ini (SchedulerName);
    }
}

void ConfigureSchedulersDialog::accept()
{
    QListWidgetItem *SelectedItem = ui->SchedulersListWidget->currentItem();
    if (SelectedItem != nullptr) {
        QString SelectedScheduler = SelectedItem->data(Qt::DisplayRole).toString();
        if (!SelectedScheduler.isEmpty()) {
            CheckChanges (SelectedScheduler);
        }
    }
    SaveAll();
    QDialog::accept();
}

void ConfigureSchedulersDialog::on_AddBeforeBarrierPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBeforeBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BeforeBarrierListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BeforeBarrierListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureSchedulersDialog::on_DeleteBeforeBarrierPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BeforeBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureSchedulersDialog::on_AddBeforeBarrierWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBeforeBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BeforeBarrierWaitListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BeforeBarrierWaitListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureSchedulersDialog::on_DeleteBeforeBarrierWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BeforeBarrierWaitListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureSchedulersDialog::on_AddBehindBarrierPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBehindBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BehindBarrierListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BehindBarrierListWidget->addItem(BarrierName);
        }
    }
}

void ConfigureSchedulersDialog::on_DeleteBehindBarrierPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BehindBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigureSchedulersDialog::on_AddBehindBarrierWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AvailableBehindBarrierListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString BarrierName = Item->data(Qt::DisplayRole).toString();
        if (ui->BehindBarrierWaitListWidget->findItems(BarrierName, Qt::MatchFixedString).isEmpty()) {
            ui->BehindBarrierWaitListWidget->addItem(BarrierName);
        }
    }

}

void ConfigureSchedulersDialog::on_DeleteBehindBarrierWaitPushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->BehindBarrierWaitListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

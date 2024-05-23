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


#include "ConfigFilter.h"
#include "StringHelpers.h"

#include <QMenu>
#include <QKeyEvent>

extern "C" {
#include "MyMemory.h"
}

ConfigFilter::ConfigFilter(QWidget *parent) : QGroupBox(parent)
{
    this->setMinimumHeight(100);
    m_MainFilterLabel = new QLabel (QString ("main filter"), this);
    m_MainFilterLineEdit = new QLineEdit (QString ("*"), this);
    m_FilterPushButton = new QPushButton (QString ("Filter"), this);
    m_MainFilterLayout = new QHBoxLayout ();
    m_MainFilterLayout->addWidget (m_MainFilterLabel);
    m_MainFilterLayout->addWidget (m_MainFilterLineEdit);
    m_MainFilterLayout->addWidget (m_FilterPushButton);

    m_IncludeExcludeFilterTab = new QTabWidget (this);

    m_IncludeFilterTab = new QWidget (m_IncludeExcludeFilterTab);
    m_IncludeExcludeFilterTab->addTab (m_IncludeFilterTab, QString ("include filter(s)"));
    m_AddBeforeIncludePushButton = new QPushButton (QString ("Add before"), m_IncludeFilterTab);
    m_AddBehindIncludePushButton = new QPushButton (QString ("Add behind"), m_IncludeFilterTab);
    m_InsertBeforeIncludePushButton = new QPushButton (QString ("Paste before"), m_IncludeFilterTab);
    m_InsertBehindIncludePushButton = new QPushButton (QString ("Paste behind"), m_IncludeFilterTab);
    m_DeleteIncludePushButton = new QPushButton (QString ("Delete"), m_IncludeFilterTab);
    m_CopyIncludePushButton = new QPushButton (QString ("Copy"), m_IncludeFilterTab);
    m_IncludeButtonsLayout = new QVBoxLayout ();
    m_IncludeButtonsLayout->addWidget (m_AddBeforeIncludePushButton);
    m_IncludeButtonsLayout->addWidget (m_AddBehindIncludePushButton);
    m_IncludeButtonsLayout->addWidget (m_InsertBeforeIncludePushButton);
    m_IncludeButtonsLayout->addWidget (m_InsertBehindIncludePushButton);
    m_IncludeButtonsLayout->addWidget (m_DeleteIncludePushButton);
    m_IncludeButtonsLayout->addWidget (m_CopyIncludePushButton);
    m_IncludeButtonsLayout->addStretch();
    m_IncludeFilterListWidget = new QListWidget (m_IncludeFilterTab);
    m_IncludeLayout = new QHBoxLayout ();
    m_IncludeLayout->addWidget(m_IncludeFilterListWidget);
    m_IncludeLayout->addLayout(m_IncludeButtonsLayout);
    m_IncludeFilterTab->setLayout(m_IncludeLayout);

    m_ExcludeFilterTab = new QWidget (m_IncludeExcludeFilterTab);
    m_IncludeExcludeFilterTab->addTab (m_ExcludeFilterTab, QString ("exclude filter(s)"));
    m_AddBeforeExcludePushButton = new QPushButton (QString ("Add before"), m_ExcludeFilterTab);
    m_AddBehindExcludePushButton = new QPushButton (QString ("Add behind"), m_ExcludeFilterTab);
    m_InsertBeforeExcludePushButton = new QPushButton (QString ("Paste before"), m_ExcludeFilterTab);
    m_InsertBehindExcludePushButton = new QPushButton (QString ("Paste behind"), m_ExcludeFilterTab);
    m_DeleteExcludePushButton = new QPushButton (QString ("Delete"), m_ExcludeFilterTab);
    m_CopyExcludePushButton = new QPushButton (QString ("Copy"), m_ExcludeFilterTab);
    m_ExcludeButtonsLayout = new QVBoxLayout ();
    m_ExcludeButtonsLayout->addWidget (m_AddBeforeExcludePushButton);
    m_ExcludeButtonsLayout->addWidget (m_AddBehindExcludePushButton);
    m_ExcludeButtonsLayout->addWidget (m_InsertBeforeExcludePushButton);
    m_ExcludeButtonsLayout->addWidget (m_InsertBehindExcludePushButton);
    m_ExcludeButtonsLayout->addWidget (m_DeleteExcludePushButton);
    m_ExcludeButtonsLayout->addWidget (m_CopyExcludePushButton);
    m_ExcludeButtonsLayout->addStretch();
    m_ExcludeFilterListWidget = new QListWidget (m_ExcludeFilterTab);
    m_ExcludeLayout = new QHBoxLayout ();
    m_ExcludeLayout->addWidget(m_ExcludeFilterListWidget);
    m_ExcludeLayout->addLayout(m_ExcludeButtonsLayout);
    m_ExcludeFilterTab->setLayout(m_ExcludeLayout);

    m_FilterGroupLayout = new QVBoxLayout ();
    m_FilterGroupLayout->addLayout(m_MainFilterLayout);
    m_FilterGroupLayout->addWidget(m_IncludeExcludeFilterTab);
    this->setLayout(m_FilterGroupLayout);

    m_IncludeFilterListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_IncludeFilterListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ExcludeFilterListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_ExcludeFilterListWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    bool Ret = connect(m_IncludeFilterListWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ContextMenuIncludeSlot (QPoint)));
    Ret = connect(m_ExcludeFilterListWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ContextMenuExcludeSlot (QPoint)));

    Ret = connect (m_IncludeFilterListWidget, SIGNAL (itemDoubleClicked (QListWidgetItem *)), this, SLOT (EditIncludeFilterItemSlot (QListWidgetItem *)));
    Ret = connect (m_ExcludeFilterListWidget, SIGNAL (itemDoubleClicked (QListWidgetItem *)), this, SLOT (EditExcludeFilterItemSlot (QListWidgetItem *)));

    Ret = connect(m_AddBeforeIncludePushButton, SIGNAL(clicked()), this, SLOT(AddBeforeIncludeSlot ()));
    Ret = connect(m_AddBehindIncludePushButton, SIGNAL(clicked()), this, SLOT(AddBehindIncludeSlot ()));
    Ret = connect(m_InsertBeforeIncludePushButton, SIGNAL(clicked()), this, SLOT(InsertBeforeIncludeSlot ()));
    Ret = connect(m_InsertBehindIncludePushButton, SIGNAL(clicked()), this, SLOT(InsertBehindIncludeSlot ()));
    Ret = connect(m_DeleteIncludePushButton, SIGNAL(clicked()), this, SLOT(DeleteIncludeSlot ()));
    Ret = connect(m_CopyIncludePushButton, SIGNAL(clicked()), this, SLOT(CopyIncludeSlot ()));
    Ret = connect(m_AddBeforeExcludePushButton, SIGNAL(clicked()), this, SLOT(AddBeforeExcludeSlot ()));
    Ret = connect(m_AddBehindExcludePushButton, SIGNAL(clicked()), this, SLOT(AddBehindExcludeSlot ()));
    Ret = connect(m_InsertBeforeExcludePushButton, SIGNAL(clicked()), this, SLOT(InsertBeforeExcludeSlot ()));
    Ret = connect(m_InsertBehindExcludePushButton, SIGNAL(clicked()), this, SLOT(InsertBehindExcludeSlot ()));
    Ret = connect(m_DeleteExcludePushButton, SIGNAL(clicked()), this, SLOT(DeleteExcludeSlot ()));
    Ret = connect(m_CopyExcludePushButton, SIGNAL(clicked()), this, SLOT(DeleteExcludeSlot ()));

    Ret =  connect(m_FilterPushButton, SIGNAL(clicked()), this, SLOT(FilterSlot ()));
    createActions();

    m_MainFilterLineEdit->installEventFilter(this);

    SetCopyBufferState (false);
}

ConfigFilter::~ConfigFilter()
{

}

void ConfigFilter::SetFilter(INCLUDE_EXCLUDE_FILTER *Filter)
{
    if (Filter->MainFilter != nullptr) {
        m_MainFilterLineEdit->setText (QString (Filter->MainFilter));
    }
    m_IncludeFilterListWidget->clear();
    for (int i = 0; i < Filter->IncludeSize; i++) {
        QListWidgetItem *NewItem = new QListWidgetItem (QString (Filter->IncludeArray[i]));
        NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
        m_IncludeFilterListWidget->addItem (NewItem);
    }
    m_ExcludeFilterListWidget->clear();
    for (int e = 0; e < Filter->ExcludeSize; e++) {
        QListWidgetItem *NewItem = new QListWidgetItem (QString (Filter->ExcludeArray[e]));
        NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
        m_ExcludeFilterListWidget->addItem (NewItem);
    }
}

INCLUDE_EXCLUDE_FILTER *ConfigFilter::GetFilter()
{
    QList<QListWidgetItem*> AllItems;

    INCLUDE_EXCLUDE_FILTER *Ret = static_cast<INCLUDE_EXCLUDE_FILTER*>(my_calloc (1, sizeof (INCLUDE_EXCLUDE_FILTER)));
    if (Ret == nullptr) return nullptr;
    QString FilterString = m_MainFilterLineEdit->text();
    Ret->MainFilter = MallocCopyString(FilterString);
    if (Ret->MainFilter == nullptr) goto __ERROR;

    AllItems = m_IncludeFilterListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret->IncludeSize = AllItems.count();
    if (Ret->IncludeSize > 0) {
        Ret->IncludeArray = static_cast<char**>(my_calloc (static_cast<size_t>(Ret->IncludeSize), sizeof (char*)));
        int i = 0;
        foreach (QListWidgetItem *Item, AllItems) {
            FilterString = Item->data(Qt::DisplayRole).toString();
            Ret->IncludeArray[i] = MallocCopyString(FilterString);
            if (Ret->IncludeArray[i] == nullptr) goto __ERROR;
            i++;
        }
    }
    AllItems = m_ExcludeFilterListWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
    Ret->ExcludeSize = AllItems.count();
    if (Ret->ExcludeSize > 0) {
        Ret->ExcludeArray = static_cast<char**>(my_calloc (static_cast<size_t>(Ret->ExcludeSize), sizeof (char*)));
        int e = 0;
        foreach (QListWidgetItem *Item, AllItems) {
            FilterString = Item->data(Qt::DisplayRole).toString();
            Ret->ExcludeArray[e] = MallocCopyString(FilterString);
            if (Ret->ExcludeArray[e] == nullptr) goto __ERROR;
            e++;
        }
    }
    return Ret;
__ERROR:
    FreeIncludeExcludeFilter (Ret);
    return nullptr;
}

void ConfigFilter::ContextMenuIncludeSlot (const QPoint &pos)
{
    QMenu menu(this);

    menu.addAction (m_AddBeforeIncludeAct);
    menu.addAction (m_AddBehindIncludeAct);
    menu.addAction (m_InsertBeforeIncludeAct);
    menu.addAction (m_InsertBehindIncludeAct);
    menu.addAction (m_DeleteIncludeAct);
    menu.addAction (m_CopyIncludeAct);
    menu.exec(m_IncludeFilterListWidget->mapToGlobal (pos));
}

void ConfigFilter::ContextMenuExcludeSlot(const QPoint &pos)
{
    QMenu menu(this);

    menu.addAction (m_AddBeforeExcludeAct);
    menu.addAction (m_AddBehindExcludeAct);
    menu.addAction (m_InsertBeforeExcludeAct);
    menu.addAction (m_InsertBehindExcludeAct);
    menu.addAction (m_DeleteExcludeAct);
    menu.addAction (m_CopyExcludeAct);
    menu.exec(m_ExcludeFilterListWidget->mapToGlobal (pos));
}

void ConfigFilter::EditIncludeFilterItemSlot(QListWidgetItem *Item)
{
    m_IncludeFilterListWidget->editItem (Item);
}

void ConfigFilter::EditExcludeFilterItemSlot(QListWidgetItem *Item)
{
    m_ExcludeFilterListWidget->editItem (Item);
}


void ConfigFilter::createActions(void)
{
    bool Ret;

    m_AddBeforeIncludeAct = new QAction(tr("&add new include before"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_AddBeforeIncludeAct->setStatusTip(tr("add new include filter item before selected item"));
    Ret = connect(m_AddBeforeIncludeAct, SIGNAL(triggered()), this, SLOT(AddBeforeIncludeSlot()));

    m_AddBehindIncludeAct = new QAction(tr("&add new include behind"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_AddBehindIncludeAct->setStatusTip(tr("add new include filter item behind selected item"));
    Ret = connect(m_AddBehindIncludeAct, SIGNAL(triggered()), this, SLOT(AddBehindIncludeSlot()));

    m_InsertBeforeIncludeAct = new QAction(tr("&paste include before"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_InsertBeforeIncludeAct->setStatusTip(tr("paste include filter item before selected item"));
    Ret = connect(m_InsertBeforeIncludeAct, SIGNAL(triggered()), this, SLOT(InsertBeforeIncludeSlot()));

    m_InsertBehindIncludeAct = new QAction(tr("&paste include behind"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_InsertBehindIncludeAct->setStatusTip(tr("paste include filter item behind selected item"));
    Ret = connect(m_InsertBehindIncludeAct, SIGNAL(triggered()), this, SLOT(InsertBehindIncludeSlot()));

    m_DeleteIncludeAct = new QAction(tr("&delete include"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_DeleteIncludeAct->setStatusTip(tr("delete include filter item"));
    Ret = connect(m_DeleteIncludeAct, SIGNAL(triggered()), this, SLOT(DeleteIncludeSlot()));

    m_CopyIncludeAct = new QAction(tr("&copy include"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_CopyIncludeAct->setStatusTip(tr("copy include filter item"));
    Ret = connect(m_CopyIncludeAct, SIGNAL(triggered()), this, SLOT(CopyIncludeSlot()));


    m_AddBeforeExcludeAct = new QAction(tr("&add new exclude before"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_AddBeforeExcludeAct->setStatusTip(tr("add new exclude filter item before selected item"));
    Ret = connect(m_AddBeforeExcludeAct, SIGNAL(triggered()), this, SLOT(AddBeforeExcludeSlot()));

    m_AddBehindExcludeAct = new QAction(tr("&add new exclude behind"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_AddBehindExcludeAct->setStatusTip(tr("add new exclude filter item behind selected item"));
    Ret = connect(m_AddBehindExcludeAct, SIGNAL(triggered()), this, SLOT(AddBehindExcludeSlot()));

    m_InsertBeforeExcludeAct = new QAction(tr("&paste exclude before"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_InsertBeforeExcludeAct->setStatusTip(tr("paste exclude filter item before selected item"));
    Ret = connect(m_InsertBeforeExcludeAct, SIGNAL(triggered()), this, SLOT(InsertBeforeExcludeSlot()));

    m_InsertBehindExcludeAct = new QAction(tr("&paste exclude behind"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_InsertBehindExcludeAct->setStatusTip(tr("paste exclude filter item behind selected item"));
    Ret = connect(m_InsertBehindExcludeAct, SIGNAL(triggered()), this, SLOT(InsertBehindExcludeSlot()));

    m_DeleteExcludeAct = new QAction(tr("&delete exclude"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_DeleteExcludeAct->setStatusTip(tr("delete exclude filter item"));
    Ret = connect(m_DeleteExcludeAct, SIGNAL(triggered()), this, SLOT(DeleteExcludeSlot()));

    m_CopyExcludeAct = new QAction(tr("&copy exclude"), this);
    //OnlineAct->setShortcuts(QKeySequence::New);
    m_CopyExcludeAct->setStatusTip(tr("copy exclude filter item"));
    Ret = connect(m_CopyExcludeAct, SIGNAL(triggered()), this, SLOT(CopyExcludeSlot()));
}

void ConfigFilter::SetCopyBufferState (bool par_State)
{
    m_InsertBeforeIncludeAct->setEnabled (par_State);
    m_InsertBehindIncludeAct->setEnabled (par_State);
    m_InsertBeforeIncludePushButton->setEnabled (par_State);
    m_InsertBehindIncludePushButton->setEnabled (par_State);
    m_InsertBeforeExcludeAct->setEnabled (par_State);
    m_InsertBehindExcludeAct->setEnabled (par_State);
    m_InsertBeforeExcludePushButton->setEnabled (par_State);
    m_InsertBehindExcludePushButton->setEnabled (par_State);
}


void ConfigFilter::AddBeforeIncludeSlot()
{
    int Row = 0;
    QList<QListWidgetItem*> SelectedItems = m_IncludeFilterListWidget->selectedItems();
    if (SelectedItems.size() >= 1) {
        QListWidgetItem *Item = SelectedItems.at(0);
        Row = m_IncludeFilterListWidget->row(Item);
    }
    QListWidgetItem *NewItem = new QListWidgetItem (QString ("*"));
    NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
    m_IncludeFilterListWidget->insertItem (Row, NewItem);
}

void ConfigFilter::AddBehindIncludeSlot()
{
    int Row = 0;
    QList<QListWidgetItem*> SelectedItems = m_IncludeFilterListWidget->selectedItems();
    if (SelectedItems.size() >= 1) {
        QListWidgetItem *Item = SelectedItems.at(0);
        Row = m_IncludeFilterListWidget->row(Item) + 1;
    }
    QListWidgetItem *NewItem = new QListWidgetItem (QString ("*"));
    NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
    m_IncludeFilterListWidget->insertItem (Row, NewItem);
}

void ConfigFilter::InsertBeforeIncludeSlot()
{
    int Row = 0;
    QList<QListWidgetItem*> SelectedItems = m_IncludeFilterListWidget->selectedItems();
    if (SelectedItems.size() >= 1) {
        QListWidgetItem *Item = SelectedItems.at(0);
        Row = m_IncludeFilterListWidget->row(Item);
    }
    foreach (QString Item, m_CopyBuffer) {
        QListWidgetItem *NewItem = new QListWidgetItem (Item);
        NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
        m_IncludeFilterListWidget->insertItem (Row, NewItem);
        Row++;
    }
}

void ConfigFilter::InsertBehindIncludeSlot()
{
    int Row = 0;
    QList<QListWidgetItem*> SelectedItems = m_IncludeFilterListWidget->selectedItems();
    if (SelectedItems.size() >= 1) {
        QListWidgetItem *Item = SelectedItems.at(0);
        Row = m_IncludeFilterListWidget->row(Item) + 1;
    }
    foreach (QString Item, m_CopyBuffer) {
        QListWidgetItem *NewItem = new QListWidgetItem (Item);
        NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
        m_IncludeFilterListWidget->insertItem (Row, NewItem);
        Row++;
    }
}

void ConfigFilter::CopyIncludeSlot()
{
    QList<QListWidgetItem*> SelectedItems = m_IncludeFilterListWidget->selectedItems();
    if (SelectedItems.count() > 0) {
        m_CopyBuffer.clear();
        foreach (QListWidgetItem* Item, SelectedItems) {
            m_CopyBuffer.append (Item->data(Qt::DisplayRole).toString());
        }
        SetCopyBufferState (true);
    }
}

void ConfigFilter::DeleteIncludeSlot()
{
    CopyIncludeSlot ();
    QList<QListWidgetItem*> SelectedItems = m_IncludeFilterListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

void ConfigFilter::AddBeforeExcludeSlot()
{
    int Row = 0;
    QList<QListWidgetItem*> SelectedItems = m_ExcludeFilterListWidget->selectedItems();
    if (SelectedItems.size() >= 1) {
        QListWidgetItem *Item = SelectedItems.at(0);
        Row = m_ExcludeFilterListWidget->row(Item);
    }
    QListWidgetItem *NewItem = new QListWidgetItem (QString ("*"));
    NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
    m_ExcludeFilterListWidget->insertItem (Row, NewItem);
}

void ConfigFilter::AddBehindExcludeSlot()
{
    int Row = 0;
    QList<QListWidgetItem*> SelectedItems = m_ExcludeFilterListWidget->selectedItems();
    if (SelectedItems.size() >= 1) {
        QListWidgetItem *Item = SelectedItems.at(0);
        Row = m_ExcludeFilterListWidget->row(Item) + 1;
    }
    QListWidgetItem *NewItem = new QListWidgetItem (QString ("*"));
    NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
    m_ExcludeFilterListWidget->insertItem (Row, NewItem);
}

void ConfigFilter::InsertBeforeExcludeSlot()
{
    int Row = 0;
    QList<QListWidgetItem*> SelectedItems = m_ExcludeFilterListWidget->selectedItems();
    if (SelectedItems.size() >= 1) {
        QListWidgetItem *Item = SelectedItems.at(0);
        Row = m_ExcludeFilterListWidget->row(Item);
    }
    foreach (QString Item, m_CopyBuffer) {
        QListWidgetItem *NewItem = new QListWidgetItem (Item);
        NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
        m_ExcludeFilterListWidget->insertItem (Row, NewItem);
        Row++;
    }
}

void ConfigFilter::InsertBehindExcludeSlot()
{
    int Row = 0;
    QList<QListWidgetItem*> SelectedItems = m_ExcludeFilterListWidget->selectedItems();
    if (SelectedItems.size() >= 1) {
        QListWidgetItem *Item = SelectedItems.at(0);
        Row = m_ExcludeFilterListWidget->row(Item) + 1;
    }
    foreach (QString Item, m_CopyBuffer) {
        QListWidgetItem *NewItem = new QListWidgetItem (Item);
        NewItem->setFlags (NewItem->flags() | Qt::ItemIsEditable);
        m_ExcludeFilterListWidget->insertItem (Row, NewItem);
        Row++;
    }
}

void ConfigFilter::CopyExcludeSlot()
{
    QList<QListWidgetItem*> SelectedItems = m_ExcludeFilterListWidget->selectedItems();
    if (SelectedItems.count() > 0) {
        m_CopyBuffer.clear();
        foreach (QListWidgetItem* Item, SelectedItems) {
            m_CopyBuffer.append (Item->data(Qt::DisplayRole).toString());
        }
        SetCopyBufferState (true);
    }
}

void ConfigFilter::FilterSlot()
{
    emit FilterClicked();
}

void ConfigFilter::DeleteExcludeSlot()
{
    CopyExcludeSlot ();
    QList<QListWidgetItem*> SelectedItems = m_ExcludeFilterListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        delete Item;
    }
}

//Reimplentierte eventFilter Methode
bool ConfigFilter::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent*>(event);
        switch (key->key()) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
                emit FilterClicked();
                break;
            default:
                return QObject::eventFilter(target, event);
        }
        return true;
    } else {
        return  QObject::eventFilter(target, event);
    }
    //return false;
}



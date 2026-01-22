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


#include "UserControlConfigDialog.h"
#include "ui_UserControlConfigDialog.h"

#include "WindowNameAlreadyInUse.h"
#include "FileDialog.h"
#include "StringHelpers.h"

#include <QMenu>
#include <QAction>

#include "UserControlParser.h"

#include "UserControlButton.h"
#include "UserControlGroup.h"
#include "UserControlConfigElementDelegate.h"

#include "QtIniFile.h"

extern "C" {
#include "ThrowError.h"
}


UserControlConfigDialog::UserControlConfigDialog(QString &par_WindowName, UserControlRoot *par_Root, QWidget *parent) : Dialog(parent),
    ui(new Ui::UserControlConfigDialog)
{
    ui->setupUi(this);
    m_CopyBuffer = nullptr;
    m_WindowTitleChanged = false;
    m_Root = par_Root;
    m_WindowTitle = par_WindowName;
    ui->WindowNameLineEdit->setText(par_WindowName);

    m_Model = new UserControlModel(par_Root, this);
    ui->ControlItemsTreeView->setModel(m_Model);

    ui->ControlItemsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->ControlItemsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(DrawItemsTreeContextMenuRequestedSlot(const QPoint &)));
    connect(ui->ControlItemsTreeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));

    QHeaderView *Header = ui->PropertiesTableView->verticalHeader();
    Header->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ProprtiesModel = new UserControlConfigElementPropertiesModel(this);
    ui->PropertiesTableView->setModel(m_ProprtiesModel);
    UserControlConfigElementDelegate *delegate = new UserControlConfigElementDelegate();
    connect(delegate, SIGNAL(PropertiesChanged(const QString &)), this, SLOT(PropertiesChanged(const QString &)));
    ui->PropertiesTableView->setItemDelegate(delegate);

    connect(m_Model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(PropertiesChangedInsideTreeview(const QModelIndex &, const QModelIndex &)));

    ui->ImageTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->ImageTableView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ImageListContextMenuRequestedSlot(const QPoint &)));

    ui->ImageTableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->ImageTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    BUIILD_ACTION(Group, "group");
    BUIILD_ACTION(Button, "button");

    m_DeleteAct = new QAction(tr("Delete"), this);
    connect(m_DeleteAct, SIGNAL(triggered()), this, SLOT(DeleteAct()));
    m_CopyAct = new QAction(tr("Copy"), this);
    connect(m_CopyAct, SIGNAL(triggered()), this, SLOT(CopyAct()));

    m_PasteAct = new QAction(tr("Paste"), this);
    connect(m_PasteAct, SIGNAL(triggered()), this, SLOT(PasteAct()));
    m_PasteBeforeAct = new QAction(tr("Paste before"), this);
    connect(m_PasteBeforeAct, SIGNAL(triggered()), this, SLOT(PasteBeforeAct()));
    m_PasteBehindAct = new QAction(tr("Paste behind"), this);
    connect(m_PasteBehindAct, SIGNAL(triggered()), this, SLOT(PasteBehindAct()));

    m_DeleteImageAct = new QAction(tr("Delete"), this);
    connect(m_DeleteImageAct, SIGNAL(triggered()), this, SLOT(DeleteImageAct()));
    m_AddImageAct = new QAction(tr("Add"), this);
    connect(m_AddImageAct, SIGNAL(triggered()), this, SLOT(AddImageAct()));
}

UserControlConfigDialog::~UserControlConfigDialog()
{
    delete ui;
    delete m_Model;
    delete m_ProprtiesModel;
}

bool UserControlConfigDialog::WindowTitleChanged()
{
    return m_WindowTitleChanged;
}

QString UserControlConfigDialog::GetWindowTitle()
{
    return ui->WindowNameLineEdit->text().trimmed();
}

void UserControlConfigDialog::accept()
{
    bool WindowTitleOk = false;
    QString NewWindowTitle = ui->WindowNameLineEdit->text().trimmed();

    if (NewWindowTitle.compare (m_WindowTitle)) {
        if (NewWindowTitle.length() >= 1) {
            if(!WindowNameAlreadyInUse(NewWindowTitle)) {
                if (IsAValidWindowSectionName(NewWindowTitle)) {
                    WindowTitleOk = true;
                    m_WindowTitleChanged = true;
                    QDialog::accept();
                } else {
                    WindowTitleOk = false;
                    ThrowError (1, "Inside window name only ascii characters and no []\\ are allowed \"%s\"",
                               QStringToConstChar(NewWindowTitle));
                }
            } else {
                WindowTitleOk = false;
                ThrowError (1, "window name \"%s\" already in use", QStringToConstChar(NewWindowTitle));
            }
        } else {
            WindowTitleOk = false;
            ThrowError (1, "window name must have one or more character \"%s\"", QStringToConstChar(NewWindowTitle));
        }
    } else {
        WindowTitleOk = true;
        Dialog::accept();
    }
}

void UserControlConfigDialog::reject()
{
    Dialog::reject();
}

void UserControlConfigDialog::DrawItemsTreeContextMenuRequestedSlot(const QPoint &par_Pos)
{
    QMenu Menu(this);

    bool AddBeforeBehind = false;
    bool AddTo = false;
    bool DeleteCopy = false;
    bool Paste = (m_CopyBuffer != nullptr);

    QModelIndex Index = ui->ControlItemsTreeView->indexAt (par_Pos);

    if (Index.isValid()) {
        DeleteCopy = true;
        AddBeforeBehind = true;
        m_AddToRootFlag = false;
        QString TypeString = m_Model->data(Index, Qt::DisplayRole).toString();
        if (!TypeString.compare("Root")||
            !TypeString.compare("Layer") ||
            !TypeString.compare("Group")) {
            AddTo = true;
        }
        /*QModelIndex ParentIndex = Index.parent();
        if (ParentIndex.isValid()) {
            QString TypeString = m_Model->data(ParentIndex, Qt::DisplayRole).toString();
            if (!TypeString.compare("Root")||
                !TypeString.compare("Layer") ||
                !TypeString.compare("Group")) {
                AddBeforeBehind = true;
            }
        }*/
    } else {
         AddTo = true;
         m_AddToRootFlag = true;
    }
    if (AddTo) {
        QMenu *AddMenu = Menu.addMenu("Add");
        AddMenu->addAction(m_AddNewGroupAct);
        AddMenu->addAction(m_AddNewButtonAct);
    }
    if (AddBeforeBehind) {
        QMenu *AddMenu = Menu.addMenu("Add before");
        AddMenu->addAction(m_AddNewGroupBeforeAct);
        AddMenu->addAction(m_AddNewButtonBeforeAct);
        AddMenu = Menu.addMenu("Add behind");
        AddMenu->addAction(m_AddNewGroupBehindAct);
        AddMenu->addAction(m_AddNewButtonBehindAct);
    }
    if (DeleteCopy) {
        Menu.addAction(m_DeleteAct);
        Menu.addAction(m_CopyAct);
    }
    if (Paste) {
        if (AddTo) {
            Menu.addAction(m_PasteAct);
        }
        if (AddBeforeBehind) {
            Menu.addAction(m_PasteBeforeAct);
            Menu.addAction(m_PasteBehindAct);
        }
    }
    Menu.exec(ui->ControlItemsTreeView->mapToGlobal(par_Pos));
}

void UserControlConfigDialog::ImageListContextMenuRequestedSlot(const QPoint &par_Pos)
{
    QMenu Menu(this);
    Menu.addAction(m_DeleteImageAct);
    Menu.addAction(m_AddImageAct);
    Menu.exec(ui->ImageTableView->mapToGlobal(par_Pos));

}

BUIILD_USER_CONTROL_SLOT_METHODS(Group)
BUIILD_USER_CONTROL_SLOT_METHODS(Button)

void UserControlConfigDialog::DeleteAct()
{
    QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
    if (Index.isValid()) {
        // first copy to buffer
        if (m_CopyBuffer != nullptr) {
            delete m_CopyBuffer;
        }
        m_CopyBuffer = new CopyBufferElem();
        CopyRecursiveToBuffer(m_CopyBuffer, Index);
        // than delete
        int Row = Index.row();
        Index = Index.parent();
        m_Model->DeleteItem(Row, Index);
    }
}

void UserControlConfigDialog::CopyAct()
{
    QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
    if (Index.isValid()) {
        if (m_CopyBuffer != nullptr) {
            delete m_CopyBuffer;
        }
        m_CopyBuffer = new CopyBufferElem();
        CopyRecursiveToBuffer(m_CopyBuffer, Index);
    }
}

void UserControlConfigDialog::PasteAct()
{
    if (m_CopyBuffer != nullptr) {
        QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
        UserControlElement *Item = PasteBufferRecursive(m_CopyBuffer, nullptr, -1);   // -1 -> appand
        if (Item != nullptr) {
            m_Model->InsertItem(-1, Item, Index);
        }
    }
}

void UserControlConfigDialog::PasteBeforeAct()
{
    if (m_CopyBuffer != nullptr) {
        QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
        if (Index.isValid()) {
            QModelIndex ParentIndex = Index.parent();
            UserControlElement *Item = PasteBufferRecursive(m_CopyBuffer, nullptr, Index.row());
            if (Item != nullptr) {
                m_Model->InsertItem(Index.row(), Item, ParentIndex);
            }
        }
    }
}

void UserControlConfigDialog::PasteBehindAct()
{
    if (m_CopyBuffer != nullptr) {
        QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
        if (Index.isValid()) {
            QModelIndex ParentIndex = Index.parent();
            UserControlElement *Item = PasteBufferRecursive(m_CopyBuffer, nullptr, Index.row());
            if (Item != nullptr) {
                m_Model->InsertItem(Index.row()+1, Item, ParentIndex);
            }
        }
    }
}

void UserControlConfigDialog::DeleteImageAct()
{

}

void UserControlConfigDialog::AddImageAct()
{
    QString ImageFile;
    ImageFile = FileDialog::getOpenNewOrExistingFileName (this, QString("Select file"),
                                                                 ImageFile,
                                                                 QString("Image file (*.jpg)"));
    if (!ImageFile.isEmpty()) {
    }
}

void UserControlConfigDialog::on_UpdatePushButton_clicked()
{
}


void UserControlConfigDialog::CopyRecursiveToBuffer(UserControlConfigDialog::CopyBufferElem *par_Buffer, QModelIndex &par_Index)
{
    QModelIndex Index = par_Index.model()->index(par_Index.row(), 0, par_Index);
    par_Buffer->m_Line =Index.data(Qt::DisplayRole).toString();
    par_Buffer->m_Line.append(Index.data(Qt::UserRole).toString());  // UserRole == Parameter
    for (int Row = 0; ; Row++) {
        QModelIndex Child = par_Index.model()->index(Row, 0, par_Index);
        if (!Child.isValid()) break;
        CopyBufferElem *Buffer = new CopyBufferElem();
        par_Buffer->m_Childs.append(Buffer);
        CopyRecursiveToBuffer(Buffer, Child);
    }
}

UserControlElement *UserControlConfigDialog::PasteBufferRecursive(UserControlConfigDialog::CopyBufferElem *par_Buffer, UserControlElement *par_Parent, int par_Row)
{
    UserControlParser Parser;
    UserControlElement *Elem = Parser.ParseOneLine(par_Buffer->m_Line, par_Parent);
    if (Elem != nullptr) {
        if (par_Parent != nullptr) {
            par_Parent->AddChild(Elem, par_Row);
        }
        foreach (CopyBufferElem *Child, par_Buffer->m_Childs) {
            PasteBufferRecursive(Child, Elem, -1);
        }
    }
    return Elem;
}

void UserControlConfigDialog::AddNewItemToSelected(UserControlElement *par_NewItem)
{
    //QModelIndexList Selected = ui->ControlItemsTreeView->selectionModel()->selectedRows();  // currentIndex();
    //if (Selected.isEmpty()) m_Model->InsertItem(-1, par_NewItem, QModelIndex());
    if (m_AddToRootFlag) {
        m_Model->InsertItem(-1, par_NewItem, QModelIndex());
    } else {
        QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
        m_Model->InsertItem(-1, par_NewItem, Index);
    }
}

void UserControlConfigDialog::AddNewItemBeforeSelected(UserControlElement *par_NewItem)
{
    QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
    int Row = Index.row();
    Index = Index.parent();
    m_Model->InsertItem(Row, par_NewItem, Index);
}

void UserControlConfigDialog::AddNewItemBehindSelected(UserControlElement *par_NewItem)
{
    QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
    int Row = Index.row();
    Index = Index.parent();
    m_Model->InsertItem(Row+1, par_NewItem, Index);
}

void UserControlConfigDialog::currentChanged(const QModelIndex &current, const QModelIndex &previos)
{
    if (previos.isValid()) {
        //QModelIndex DeselectItem = deselected.indexes().at(0);
        //CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(DeselectItem);
    }
    if (current.isValid()) {
        UserControlElement *Element = (UserControlElement*)current.internalPointer();
        m_ProprtiesModel->SetPorperties(&Element->m_Properties);
        //current.data().toString();
        //QModelIndex SelectItem = selected.indexes().at(0);
        //CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectItem);
    }
}


// static so you can copy this from on to an other widget
UserControlConfigDialog::CopyBufferElem *UserControlConfigDialog::m_CopyBuffer;

void UserControlConfigDialog::on_FixedRatioCheckBox_stateChanged(int arg1)
{
    Q_UNUSED(arg1)
}

void UserControlConfigDialog::PropertiesChanged(const QString &par_Properties)
{
    QModelIndex Index = ui->ControlItemsTreeView->currentIndex();
    if (Index.isValid()) {
        m_Model->setData(Index, par_Properties);
    }
}

void UserControlConfigDialog::PropertiesChangedInsideTreeview(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_UNUSED(bottomRight)
    UserControlElement *Element = (UserControlElement*)topLeft.internalPointer();
    m_ProprtiesModel->SetPorperties(&Element->m_Properties);
}

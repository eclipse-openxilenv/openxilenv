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


#include "UserDrawConfigDialog.h"
#include "ui_UserDrawConfigDialog.h"

#include "WindowNameAlreadyInUse.h"
#include "FileDialog.h"
#include "StringHelpers.h"

#include <QMenu>
#include <QAction>

#include "UserDrawParser.h"
#include "UserDrawLayer.h"
#include "UserDrawGroup.h"
#include "UserDrawRect.h"
#include "UserDrawButton.h"
#include "UserDrawCircle.h"
#include "UserDrawPoint.h"
#include "UserDrawImage.h"
#include "UserDrawText.h"
#include "UserDrawLine.h"
#include "UserDrawPolygon.h"
#include "UserDrawXYPlot.h"
#include "UserDrawConfigElementDelegate.h"

#include "QtIniFile.h"

extern "C" {
#include "ThrowError.h"
}

//void BuildAction(QAction **par_Actions, QString &par_Name);



UserDrawConfigDialog::UserDrawConfigDialog(QString &par_WindowName, UserDrawRoot *par_Root, QList<UserDrawImageItem *> *par_Images, QWidget *parent) : Dialog(parent),
    ui(new Ui::UserDrawConfigDialog)
{
    ui->setupUi(this);
    m_CopyBuffer = nullptr;
    m_WindowTitleChanged = false;
    m_Root = par_Root;
    m_WindowTitle = par_WindowName;
    ui->WindowNameLineEdit->setText(par_WindowName);
    ui->FixedRatioCheckBox->setChecked(par_Root->m_FixedRatio);
    ui->AntialiasingCheckBox->setChecked(par_Root->m_Antialiasing);

    m_Model = new UserDrawModel(par_Root, this);
    ui->DrawItemsTreeView->setModel(m_Model);

    ui->DrawItemsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->DrawItemsTreeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(DrawItemsTreeContextMenuRequestedSlot(const QPoint &)));
    connect(ui->DrawItemsTreeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));

    QHeaderView *Header = ui->PropertiesTableView->verticalHeader();
    Header->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ProprtiesModel = new UserDrawConfigElementPropertiesModel(this);
    ui->PropertiesTableView->setModel(m_ProprtiesModel);
    UserDrawConfigElementDelegate *delegate = new UserDrawConfigElementDelegate();
    connect(delegate, SIGNAL(PropertiesChanged(const QString &)), this, SLOT(PropertiesChanged(const QString &)));
    ui->PropertiesTableView->setItemDelegate(delegate);

    connect(m_Model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(PropertiesChangedInsideTreeview(const QModelIndex &, const QModelIndex &)));

    m_ImageModel = new UserDrawImageModel(par_Images, this);
    ui->ImageTableView->setModel(m_ImageModel);

    ui->ImageTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->ImageTableView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ImageListContextMenuRequestedSlot(const QPoint &)));

    ui->ImageTableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->ImageTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    BUIILD_ACTION(Layer, "layer");
    BUIILD_ACTION(Group, "group");
    BUIILD_ACTION(Rect, "rect");
    BUIILD_ACTION(Circle, "circle");
    BUIILD_ACTION(Point, "point");
    BUIILD_ACTION(Image, "image");
    BUIILD_ACTION(Text, "text");
    BUIILD_ACTION(Line, "line");
    BUIILD_ACTION(Polygon, "polygon");
    BUIILD_ACTION(XYPlot, "xy plot");
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

UserDrawConfigDialog::~UserDrawConfigDialog()
{
    delete ui;
    delete m_Model;
    delete m_ProprtiesModel;
    delete m_ImageModel;
}

bool UserDrawConfigDialog::WindowTitleChanged()
{
    return m_WindowTitleChanged;
}

QString UserDrawConfigDialog::GetWindowTitle()
{
    return ui->WindowNameLineEdit->text().trimmed();
}

void UserDrawConfigDialog::accept()
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

void UserDrawConfigDialog::reject()
{
    Dialog::reject();
}

void UserDrawConfigDialog::DrawItemsTreeContextMenuRequestedSlot(const QPoint &par_Pos)
{
    QMenu Menu(this);

    bool AddBeforeBehind = false;
    bool AddTo = false;
    bool DeleteCopy = false;
    bool Paste = (m_CopyBuffer != nullptr);

    QModelIndex Index = ui->DrawItemsTreeView->indexAt (par_Pos);

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
        AddMenu->addAction(m_AddNewLayerAct);
        AddMenu->addAction(m_AddNewGroupAct);
        AddMenu->addAction(m_AddNewRectAct);
        AddMenu->addAction(m_AddNewCircleAct);
        AddMenu->addAction(m_AddNewPointAct);
        AddMenu->addAction(m_AddNewImageAct);
        AddMenu->addAction(m_AddNewTextAct);
        AddMenu->addAction(m_AddNewLineAct);
        AddMenu->addAction(m_AddNewPolygonAct);
        AddMenu->addAction(m_AddNewXYPlotAct);
        AddMenu->addAction(m_AddNewButtonAct);
    }
    if (AddBeforeBehind) {
        QMenu *AddMenu = Menu.addMenu("Add before");
        AddMenu->addAction(m_AddNewLayerBeforeAct);
        AddMenu->addAction(m_AddNewGroupBeforeAct);
        AddMenu->addAction(m_AddNewRectBeforeAct);
        AddMenu->addAction(m_AddNewCircleBeforeAct);
        AddMenu->addAction(m_AddNewPointBeforeAct);
        AddMenu->addAction(m_AddNewImageBeforeAct);
        AddMenu->addAction(m_AddNewTextBeforeAct);
        AddMenu->addAction(m_AddNewLineBeforeAct);
        AddMenu->addAction(m_AddNewPolygonBeforeAct);
        AddMenu->addAction(m_AddNewXYPlotBeforeAct);
        AddMenu->addAction(m_AddNewButtonBeforeAct);
        AddMenu = Menu.addMenu("Add behind");
        AddMenu->addAction(m_AddNewLayerBehindAct);
        AddMenu->addAction(m_AddNewGroupBehindAct);
        AddMenu->addAction(m_AddNewRectBehindAct);
        AddMenu->addAction(m_AddNewCircleBehindAct);
        AddMenu->addAction(m_AddNewPointBehindAct);
        AddMenu->addAction(m_AddNewImageBehindAct);
        AddMenu->addAction(m_AddNewTextBehindAct);
        AddMenu->addAction(m_AddNewLineBehindAct);
        AddMenu->addAction(m_AddNewPolygonBehindAct);
        AddMenu->addAction(m_AddNewXYPlotBehindAct);
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
    Menu.exec(ui->DrawItemsTreeView->mapToGlobal(par_Pos));
}

void UserDrawConfigDialog::ImageListContextMenuRequestedSlot(const QPoint &par_Pos)
{
    QMenu Menu(this);
    Menu.addAction(m_DeleteImageAct);
    Menu.addAction(m_AddImageAct);
    Menu.exec(ui->ImageTableView->mapToGlobal(par_Pos));

}


BUIILD_USER_DRAW_SLOT_METHODS(Layer)
BUIILD_USER_DRAW_SLOT_METHODS(Group)
BUIILD_USER_DRAW_SLOT_METHODS(Rect)
BUIILD_USER_DRAW_SLOT_METHODS(Circle)
BUIILD_USER_DRAW_SLOT_METHODS(Point)
BUIILD_USER_DRAW_SLOT_METHODS(Image)
BUIILD_USER_DRAW_SLOT_METHODS(Text)
BUIILD_USER_DRAW_SLOT_METHODS(Line)
BUIILD_USER_DRAW_SLOT_METHODS(Polygon)
BUIILD_USER_DRAW_SLOT_METHODS(XYPlot)
BUIILD_USER_DRAW_SLOT_METHODS(Button)

void UserDrawConfigDialog::DeleteAct()
{
    QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
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

void UserDrawConfigDialog::CopyAct()
{
    QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
    if (Index.isValid()) {
        if (m_CopyBuffer != nullptr) {
            delete m_CopyBuffer;
        }
        m_CopyBuffer = new CopyBufferElem();
        CopyRecursiveToBuffer(m_CopyBuffer, Index);
    }
}

void UserDrawConfigDialog::PasteAct()
{
    if (m_CopyBuffer != nullptr) {
        QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
        UserDrawElement *Item = PasteBufferRecursive(m_CopyBuffer, nullptr, -1);   // -1 -> appand
        if (Item != nullptr) {
            m_Model->InsertItem(-1, Item, Index);
        }
    }
}

void UserDrawConfigDialog::PasteBeforeAct()
{
    if (m_CopyBuffer != nullptr) {
        QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
        if (Index.isValid()) {
            QModelIndex ParentIndex = Index.parent();
            UserDrawElement *Item = PasteBufferRecursive(m_CopyBuffer, nullptr, Index.row());
            if (Item != nullptr) {
                m_Model->InsertItem(Index.row(), Item, ParentIndex);
            }
        }
    }
}

void UserDrawConfigDialog::PasteBehindAct()
{
    if (m_CopyBuffer != nullptr) {
        QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
        if (Index.isValid()) {
            QModelIndex ParentIndex = Index.parent();
            UserDrawElement *Item = PasteBufferRecursive(m_CopyBuffer, nullptr, Index.row());
            if (Item != nullptr) {
                m_Model->InsertItem(Index.row()+1, Item, ParentIndex);
            }
        }
    }
}

void UserDrawConfigDialog::DeleteImageAct()
{

}

void UserDrawConfigDialog::AddImageAct()
{
    QString ImageFile;
    ImageFile = FileDialog::getOpenNewOrExistingFileName (this, QString("Select file"),
                                                                 ImageFile,
                                                                 QString("Image file (*.jpg)"));
    if (!ImageFile.isEmpty()) {
        /*QString TempEntry("TempImage");
        QString IniFile = QString(INIFILE);
        ScQt_DBWritePrivateProfileImage(m_WindowTitle, TempEntry, ImageFile, IniFile);
        QImage *Image = ScQt_DBGetPrivateProfileImage(m_WindowTitle, TempEntry, IniFile);*/

        UserDrawImageItem *Item = new UserDrawImageItem;
        Item->m_Name = m_ImageModel->GetUniqueName("Image");
        Item->m_Number = m_ImageModel->GetUniqueNumber(0);
        Item->m_BinaryImage.Load(ImageFile);
        Item->m_Image = Item->m_BinaryImage.GetImageFrom();
        QModelIndex Index = ui->ImageTableView->currentIndex();
        if (Index.isValid()) m_ImageModel->InsertItem(Index.row(), Item);
        else m_ImageModel->InsertItem(0, Item);
    }
}

void UserDrawConfigDialog::on_UpdatePushButton_clicked()
{
    m_Root->m_FixedRatio = ui->FixedRatioCheckBox->isChecked();
    m_Root->m_Antialiasing = ui->AntialiasingCheckBox->isChecked();
}


void UserDrawConfigDialog::CopyRecursiveToBuffer(UserDrawConfigDialog::CopyBufferElem *par_Buffer, QModelIndex &par_Index)
{
    QModelIndex Index = par_Index.model()->index(par_Index.row(), 0, par_Index);
    par_Buffer->m_Line = Index.data(Qt::DisplayRole).toString();
    par_Buffer->m_Line.append(Index.data(Qt::UserRole).toString());  // UserRole == Parameter
    for (int Row = 0; ; Row++) {
        QModelIndex Child = par_Index.model()->index(Row, 0, par_Index);
        if (!Child.isValid()) break;
        CopyBufferElem *Buffer = new CopyBufferElem();
        par_Buffer->m_Childs.append(Buffer);
        CopyRecursiveToBuffer(Buffer, Child);
    }
}

UserDrawElement *UserDrawConfigDialog::PasteBufferRecursive(UserDrawConfigDialog::CopyBufferElem *par_Buffer, UserDrawElement *par_Parent, int par_Row)
{
    UserDrawParser Parser;
    UserDrawElement *Elem = Parser.ParseOneLine(par_Buffer->m_Line, par_Parent);
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

void UserDrawConfigDialog::AddNewItemToSelected(UserDrawElement *par_NewItem)
{
    //QModelIndexList Selected = ui->DrawItemsTreeView->selectionModel()->selectedRows();  // currentIndex();
    //if (Selected.isEmpty()) m_Model->InsertItem(-1, par_NewItem, QModelIndex());
    if (m_AddToRootFlag) {
        m_Model->InsertItem(-1, par_NewItem, QModelIndex());
    } else {
        QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
        m_Model->InsertItem(-1, par_NewItem, Index);
    }
}

void UserDrawConfigDialog::AddNewItemBeforeSelected(UserDrawElement *par_NewItem)
{
    QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
    int Row = Index.row();
    Index = Index.parent();
    m_Model->InsertItem(Row, par_NewItem, Index);
}

void UserDrawConfigDialog::AddNewItemBehindSelected(UserDrawElement *par_NewItem)
{
    QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
    int Row = Index.row();
    Index = Index.parent();
    m_Model->InsertItem(Row+1, par_NewItem, Index);
}

void UserDrawConfigDialog::currentChanged(const QModelIndex &current, const QModelIndex &previos)
{
    if (previos.isValid()) {
        //QModelIndex DeselectItem = deselected.indexes().at(0);
        //CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(DeselectItem);
    }
    if (current.isValid()) {
        UserDrawElement *Element = (UserDrawElement*)current.internalPointer();
        m_ProprtiesModel->SetPorperties(&Element->m_Properties);
        //current.data().toString();
        //QModelIndex SelectItem = selected.indexes().at(0);
        //CanConfigElement *Item = this->m_CanConfigTreeModel->GetItem(SelectItem);
    }
}


// static so you can copy this from on to an other widget
UserDrawConfigDialog::CopyBufferElem *UserDrawConfigDialog::m_CopyBuffer;

void UserDrawConfigDialog::on_FixedRatioCheckBox_stateChanged(int arg1)
{
    m_Root->m_FixedRatio = arg1;
}

void UserDrawConfigDialog::PropertiesChanged(const QString &par_Properties)
{
    QModelIndex Index = ui->DrawItemsTreeView->currentIndex();
    if (Index.isValid()) {
        m_Model->setData(Index, par_Properties);
    }
}

void UserDrawConfigDialog::PropertiesChangedInsideTreeview(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_UNUSED(bottomRight)
    UserDrawElement *Element = (UserDrawElement*)topLeft.internalPointer();
    m_ProprtiesModel->SetPorperties(&Element->m_Properties);
}

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


#include "EditTextReplaceDialog.h"
#include "ui_EditTextReplaceDialog.h"
#include "StringHelpers.h"

#include <QStandardItemModel>
#include <QMenu>
#include <QColorDialog>

#include <QPainter>
#include <QLineEdit>

extern "C" {
#include "PrintFormatToString.h"
#include "TextReplace.h"
#include "ThrowError.h"
}

ColorView::ColorView (int par_Color)
{
    m_Color = par_Color;
}


QSize ColorView::sizeHint() const
{
    return QSize (20, 20);
}

void ColorView::paint (QPainter *par_Painter, const QRect &par_Rect) const
{
    par_Painter->save();

    par_Painter->setRenderHint(QPainter::Antialiasing, true);
    if (m_Color < 0) {
        // paint a X for no color
        par_Painter->drawLine(par_Rect.x()+2,
                          par_Rect.y()+2,
                          par_Rect.x() + par_Rect.width() - 4,
                          par_Rect.y() + par_Rect.height() - 4);
        par_Painter->drawLine(par_Rect.x() + par_Rect.width() - 4,
                          par_Rect.y()+2,
                          par_Rect.x()+2,
                          par_Rect.y() + par_Rect.height() - 4);
    } else {
        par_Painter->setPen(Qt::NoPen);
        QBrush Brush = QBrush (QColor ((m_Color >> 0) & 0xFF,
                                       (m_Color >> 8) & 0xFF,
                                       (m_Color >> 16) & 0xFF));
        par_Painter->fillRect (par_Rect.x()+2, par_Rect.y()+2, par_Rect.width()-4, par_Rect.height()-4, Brush);
    }
    par_Painter->restore();
}



void ColorDelegate::paint(QPainter *par_Painter, const QStyleOptionViewItem &par_Option,
                         const QModelIndex &par_Index) const
{
    if (par_Index.data().canConvert<ColorView>()) {
        ColorView Color = qvariant_cast<ColorView>(par_Index.data());

        if (par_Option.state & QStyle::State_Selected)
            par_Painter->fillRect (par_Option.rect, par_Option.palette.highlight());

        Color.paint (par_Painter, par_Option.rect);
    } else {
        QStyledItemDelegate::paint (par_Painter, par_Option, par_Index);
    }
}

QSize ColorDelegate::sizeHint(const QStyleOptionViewItem &par_Option,
                              const QModelIndex &par_Index) const
{
    if (par_Index.data().canConvert<ColorView>()) {
        ColorView Color = qvariant_cast<ColorView>(par_Index.data());
        return Color.sizeHint();
    } else {
        return QStyledItemDelegate::sizeHint(par_Option, par_Index);
    }
}

QWidget *ColorDelegate::createEditor(QWidget *par_Parent,
                                     const QStyleOptionViewItem &par_Option,
                                     const QModelIndex &par_Index) const

{
    if (par_Index.data().canConvert<ColorView>()) {
        // Instead a editor inside the tree view a color dialog will be opened
        ColorView ColorItem = qvariant_cast<ColorView>(par_Index.data());
        QColor InitColor = ColorItem.Color();
        QColor Color = QColorDialog::getColor (InitColor, par_Parent, QString ("Choice color of enum"));
        if (Color.isValid()) {
            QAbstractItemModel *Model = const_cast<QAbstractItemModel*>(par_Index.model());
            int ColorInt = Color.red() | Color.green() << 8 | Color.blue() << 16;
            Model->setData (Model->index(par_Index.row(), 3),
                            QVariant::fromValue (ColorView(ColorInt)));
            char Help[64];
            PrintFormatToString (Help, sizeof(Help), "0x%02X:0x%02X:0x%02X", (ColorInt >> 0) & 0xFF, (ColorInt >> 8) & 0xFF, (ColorInt >> 16) & 0xFF);
            Model->setData (Model->index(par_Index.row(), 4), QVariant::fromValue (QString  (Help)));
        }

        return nullptr;
    } else {
        return QStyledItemDelegate::createEditor(par_Parent, par_Option, par_Index);
    }
}

QWidget *ColorTextDelegate::createEditor (QWidget *par_Parent,
                                          const QStyleOptionViewItem &par_Option,
                                          const QModelIndex &par_Index) const

{
    if (par_Index.data().canConvert<QString>()) {
        QLineEdit *LineEdit = new QLineEdit(par_Parent);
        LineEdit->setValidator (new ColorTextValidator (LineEdit));
        return LineEdit;
    } else {
        return QStyledItemDelegate::createEditor(par_Parent, par_Option, par_Index);
    }
}

void ColorTextDelegate::setEditorData (QWidget *par_Editor,
                                       const QModelIndex &par_Index) const
{
    QString ColorText = par_Index.data().toString();
    QLineEdit *Editor = qobject_cast<QLineEdit *>(par_Editor);
    Editor->setText (ColorText);
}


int ColorStringToInt (QString par_ColorString, QValidator::State *ret_State)
{
    if (par_ColorString.isEmpty()) {
        if (ret_State != nullptr) *ret_State = QValidator::Acceptable;
        return -1;   // empty string correspond no color
    }
    bool Ok;
    int M1 = par_ColorString.toInt (&Ok);
    if (Ok && (M1 == -1)) {
        if (ret_State != nullptr) *ret_State = QValidator::Acceptable;
        return -1;
    }
    QStringList List = par_ColorString.split(":");
    if (List.count() == 3) { // always 3 colors
        bool RedOk;
        QString RedString = List.at(0);
        int Red = RedString.toInt (&RedOk, 0);
        bool GreenOk;
        QString GreenString = List.at(1);
        int Green = GreenString.toInt (&GreenOk, 0);
        bool BlueOk;
        QString BlueString = List.at(2);
        int Blue = BlueString.toInt (&BlueOk, 0);

        foreach (QString Item, List) {
            unsigned int Color = Item.toUInt (&Ok, 0);
            if ((Ok || Item.isEmpty() || !Item.compare("0x", Qt::CaseInsensitive)) && (Color <= 0xFF)) {
                ;
            } else {
                if (ret_State != nullptr) *ret_State = QValidator::Invalid;
                return -1;
            }
        }

        if (RedOk && GreenOk && BlueOk) {
            if (((Red <= 0xFF) && (Red >= 0)) &&
                ((Green <= 0xFF) && (Green >= 0)) &&
                ((Blue <= 0xFF) && (Blue >= 0))) {
                if (ret_State != nullptr) *ret_State = QValidator::Acceptable;
                return Red | (Green << 8) | (Blue << 16);
            }
        }
    } else if (List.count() > 3) {
        if (ret_State != nullptr) *ret_State = QValidator::Invalid;
        return -1;
    } else {
        foreach (QString Item, List) {
            unsigned int Color = Item.toUInt (&Ok, 0);
            if ((Ok || Item.isEmpty() || !Item.compare("0x", Qt::CaseInsensitive)) && (Color <= 0xFF)) {
                ;
            } else {
                if (ret_State != nullptr) *ret_State = QValidator::Invalid;
                return -1;
            }
        }
    }
    // should not reached
    if (ret_State != nullptr) *ret_State = QValidator::Intermediate;
    return -1;
}

void ColorTextDelegate::setModelData (QWidget *par_Editor, QAbstractItemModel *par_Model,
                                      const QModelIndex &par_Index) const
{
    QLineEdit *Editor = qobject_cast<QLineEdit *>(par_Editor);
    QString ColorString = Editor->text();
    QValidator::State State;
    int ColorInt = ColorStringToInt (ColorString, &State);
    if (State == QValidator::Acceptable) {
        par_Model->setData (par_Index, QVariant::fromValue (ColorString));
        int Row = par_Index.row();
        par_Model->setData (par_Model->index (Row, 3), QVariant::fromValue (ColorView (ColorInt)));
    }
}

void ColorTextDelegate::commitAndCloseEditor()
{
    QLineEdit *Editor = qobject_cast<QLineEdit *>(sender());
    emit commitData (Editor);
    emit closeEditor (Editor);
}










SortEnumProxyModel::SortEnumProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

SortEnumProxyModel::~SortEnumProxyModel()
{
}

bool SortEnumProxyModel::lessThan(const QModelIndex &left,
                                  const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);

    double leftValue = leftData.toString().toDouble();
    double rightValue = rightData.toString().toDouble();

    return (leftValue < rightValue);
}


EditTextReplaceDialog::EditTextReplaceDialog (QString &par_TextReplaceString, QWidget *parent) : Dialog(parent),
    ui(new Ui::EditTextReplaceDialog)
{
    ui->setupUi(this);
    createActions();
    ui->EnumsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->EnumsTreeView->setItemDelegateForColumn(3, new ColorDelegate);
    ui->EnumsTreeView->setItemDelegateForColumn(4, new ColorTextDelegate);
    createEnumModel (par_TextReplaceString, this);
    ui->EnumsTreeView->setModel (m_proxyModel); //(m_Model);
    ui->EnumsTreeView->setColumnWidth(3, 30);

}

EditTextReplaceDialog::~EditTextReplaceDialog()
{
    delete ui;
}

void EditTextReplaceDialog::createActions(void)
{
    bool Ret;

    m_CutAct = new QAction(tr("&cut"), this);
    m_CutAct->setStatusTip(tr("cut selected enums"));
    Ret = connect(m_CutAct, SIGNAL(triggered()), this, SLOT(CutSlot()));

    m_CopyAct = new QAction(tr("&copy"), this);
    m_CopyAct->setStatusTip(tr("copy selected enums"));
    Ret = connect(m_CopyAct, SIGNAL(triggered()), this, SLOT(CopySlot()));

    m_PasteBeforeAct = new QAction(tr("&paste before"), this);
    m_PasteBeforeAct->setStatusTip(tr("paste enums before selected enum"));
    Ret = connect(m_PasteBeforeAct, SIGNAL(triggered()), this, SLOT(PasteBeforeSlot()));

    m_PasteBehindAct = new QAction(tr("&paste behind"), this);
    m_PasteBehindAct->setStatusTip(tr("paste enums behind selected enum"));
    Ret = connect(m_PasteBehindAct, SIGNAL(triggered()), this, SLOT(PasteBehindSlot()));

    m_NewBeforeAct = new QAction(tr("&new before"), this);
    m_NewBeforeAct->setStatusTip(tr("add a new enum before selected enum"));
    Ret = connect(m_NewBeforeAct, SIGNAL(triggered()), this, SLOT(NewBeforeSlot()));

    m_NewBehindAct = new QAction(tr("&new behind"), this);
    m_NewBehindAct->setStatusTip(tr("ad a new enum behind selected enum"));
    Ret = connect(m_NewBehindAct, SIGNAL(triggered()), this, SLOT(NewBehindSlot()));

    m_SelectAllAct = new QAction(tr("select all"), this);
    m_SelectAllAct->setStatusTip(tr("select all enums"));
    Ret = connect(m_SelectAllAct, SIGNAL(triggered()), this, SLOT(SelectAllSlot()));

    m_SortAllAct = new QAction(tr("sort all"), this);
    m_SortAllAct->setStatusTip(tr("sort all enums by \"from\" column"));
    Ret = connect(m_SortAllAct, SIGNAL(triggered()), this, SLOT(SortAllSlot()));

    m_SyntaxCheckAct = new QAction(tr("syntax check"), this);
    m_SyntaxCheckAct->setStatusTip(tr("check the syntax of the text replace"));
    Ret = connect(m_SyntaxCheckAct, SIGNAL(triggered()), this, SLOT(SyntaxCheckSlot()));
}


void EditTextReplaceDialog::InsertReplaceStringBeginAt (QString &par_TextReplaceString, int par_StartAtRow)
{
    int Index = 0;
    char EnumString[1024];
    int64_t From;
    int64_t To;
    int32_t Color;
    int Row = par_StartAtRow;

    while (1) {
        if (GetNextEnumFromEnumList (Index, QStringToConstChar(par_TextReplaceString),
                                     EnumString, sizeof(EnumString), &From, &To, &Color) < 0) {
            break;
        }
        m_proxyModel->insertRow (Row);

        if (From > INT64_MIN) { // int minimum
            m_proxyModel->setData (m_proxyModel->index (Row, 0), QString ("%1").arg (From));
        } else {
            m_proxyModel->setData (m_proxyModel->index (Row, 0), QString ("*"));
        }
        if (To < INT64_MAX) { // int maximum
            m_proxyModel->setData (m_proxyModel->index (Row, 1), QString ("%1").arg (To));
        } else {
            m_proxyModel->setData (m_proxyModel->index (Row, 1), QString ("*"));
        }
        m_proxyModel->setData (m_proxyModel->index(Row, 2), QString (EnumString));
        m_proxyModel->setData (m_proxyModel->index(Row, 3),
                               QVariant::fromValue (ColorView(Color)));
        char Help[64];
        PrintFormatToString (Help, sizeof(Help), "0x%02X:0x%02X:0x%02X", (Color >> 0) & 0xFF, (Color >> 8) & 0xFF, (Color >> 16) & 0xFF);
        m_proxyModel->setData (m_proxyModel->index(Row, 4), QVariant::fromValue (QString  (Help)));
        Row++;
        Index++;
    }
}

void EditTextReplaceDialog::createEnumModel(QString &par_TextReplaceString, QObject *parent)
{
    QStandardItemModel *Model = new QStandardItemModel (0, 5, parent);

    Model->setHeaderData(0, Qt::Horizontal, QObject::tr("From"));
    Model->setHeaderData(1, Qt::Horizontal, QObject::tr("To"));
    Model->setHeaderData(2, Qt::Horizontal, QObject::tr("Text"));
    Model->setHeaderData(3, Qt::Horizontal, QObject::tr("C"));
    Model->setHeaderData(4, Qt::Horizontal, QObject::tr("Color (RGB)"));

    m_proxyModel = new SortEnumProxyModel(this);
    m_proxyModel->setSourceModel (Model);

    InsertReplaceStringBeginAt (par_TextReplaceString, 0);
}

QString EditTextReplaceDialog::GetModifiedTextReplaceString ()
{
    QString Ret;
    for (int Row = 0; Row < m_proxyModel->rowCount(); Row++) {
        // From
        QModelIndex Idx = m_proxyModel->index (Row, 0);
        QVariant From = Idx.data (Qt::DisplayRole);
        Ret.append (From.toString());
        Ret.append (" ");
        // To
        Idx = m_proxyModel->index (Row, 1);
        QVariant To = Idx.data (Qt::DisplayRole);
        Ret.append (To.toString());
        Ret.append (" \"");
        // Color
        Idx = m_proxyModel->index (Row, 3);
        ColorView Color = qvariant_cast<ColorView>(Idx.data());
        int ColorInt = Color.ColorInt();
        if (ColorInt >= 0) {  // valid color
            char Help[64];
            PrintFormatToString (Help, sizeof(Help), "RGB(0x%02X:0x%02X:0x%02X)", (ColorInt >> 0) & 0xFF, (ColorInt >> 8) & 0xFF, (ColorInt >> 16) & 0xFF);
            Ret.append (QString (Help));
        }
        // Name
        Idx = m_proxyModel->index (Row, 2);
        QVariant Name = Idx.data (Qt::DisplayRole);
        Ret.append (Name.toString());
        Ret.append ("\";");
    }
    return Ret;
}

QString EditTextReplaceDialog::GetTextSelectedReplaceString ()
{
    QString Ret;
    QModelIndexList Selected = ui->EnumsTreeView->selectionModel()->selectedRows();

    foreach (QModelIndex Index, Selected) {
        if (Index.column() == 0) {
            int Row = Index.row();
            // From
            QModelIndex Idx = m_proxyModel->index (Row, 0);
            QVariant From = Idx.data (Qt::DisplayRole);
            Ret.append (From.toString());
            Ret.append (" ");
            // To
            Idx = m_proxyModel->index (Row, 1);
            QVariant To = Idx.data (Qt::DisplayRole);
            Ret.append (To.toString());
            Ret.append (" \"");

            // Color
            Idx = m_proxyModel->index (Row, 3);
            ColorView Color = qvariant_cast<ColorView>(Idx.data());
            int ColorInt = Color.ColorInt();
            if (ColorInt >= 0) {  // valid color
                char Help[64];
                PrintFormatToString (Help, sizeof(Help), "RGB(0x%02X:0x%02X:0x%02X)", (ColorInt >> 0) & 0xFF, (ColorInt >> 8) & 0xFF, (ColorInt >> 16) & 0xFF);
                Ret.append (QString (Help));
            }
            // Name
            Idx = m_proxyModel->index (Row, 2);
            QVariant Name = Idx.data (Qt::DisplayRole);
            Ret.append (Name.toString());
            Ret.append ("\";");
        }
    }
    return Ret;
}

void EditTextReplaceDialog::on_EnumsTreeView_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    menu.addAction (m_CutAct);
    menu.addAction (m_CopyAct);
    menu.addAction (m_PasteBeforeAct);
    menu.addAction (m_PasteBehindAct);
    menu.addAction (m_NewBeforeAct);
    menu.addAction (m_NewBehindAct);
    menu.addAction (m_SelectAllAct);
    menu.addAction (m_SortAllAct);
    menu.addAction (m_SyntaxCheckAct);
    menu.exec(ui->EnumsTreeView->mapToGlobal (pos));
}

void EditTextReplaceDialog::CutSlot (void)
{
    m_CopyBuffer = GetTextSelectedReplaceString();

    QModelIndexList Selected = ui->EnumsTreeView->selectionModel()->selectedRows();
    QAbstractItemModel *model = ui->EnumsTreeView->model();

    std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top

    foreach (QModelIndex Index, Selected) {
        if (Index.column() == 0) {
            if (model->removeRow(Index.row(), Index.parent())) {
                //updateActions();
            }
        }
    }
}

void EditTextReplaceDialog::CopySlot (void)
{
    m_CopyBuffer = GetTextSelectedReplaceString();
}


void EditTextReplaceDialog::PasteBeforeSlot (void)
{
    if (!m_CopyBuffer.isEmpty()) {
        QModelIndex Selected = ui->EnumsTreeView->selectionModel()->currentIndex();
        if (Selected.isValid()) {
            InsertReplaceStringBeginAt (m_CopyBuffer, Selected.row());
        }
    }
}

void EditTextReplaceDialog::PasteBehindSlot (void)
{
    if (!m_CopyBuffer.isEmpty()) {
        QModelIndex Selected = ui->EnumsTreeView->selectionModel()->currentIndex();
        if (Selected.isValid()) {
            InsertReplaceStringBeginAt (m_CopyBuffer, Selected.row() + 1);
        }
    }
}


void EditTextReplaceDialog::InitNewRow (QAbstractItemModel *Model, int Row)
{
    int From = 0;
    // From
    QModelIndex IdxBefore = Model->index (Row-1, 0);
    QModelIndex IdxBehind = Model->index (Row+1, 0);
    if (IdxBefore.isValid()) {
        double FromBefore = IdxBefore.data (Qt::DisplayRole).toDouble();
        if (IdxBehind.isValid()) {
            double FromBehind = IdxBehind.data (Qt::DisplayRole).toDouble();
            From = static_cast<int>((FromBefore + (FromBehind - FromBefore) / 2.0 + 0.5));
        } else {
            From = static_cast<int>(FromBefore + 0.5 + 1.0);
        }
    } else {
        if (IdxBehind.isValid()) {
            double FromBehind = IdxBehind.data (Qt::DisplayRole).toDouble();
            From = static_cast<int>(FromBehind + 0.5 - 1.0);
        }
    }

    Model->setData (Model->index (Row, 0), QString ("%1").arg (From));
    Model->setData (Model->index (Row, 1), QString ("%1").arg (From));
    Model->setData(Model->index (Row, 2), QString ("x"));
    Model->setData (Model->index(Row, 3), QVariant::fromValue (ColorView (-1)));
    Model->setData (Model->index(Row, 4), QVariant::fromValue (QString  ("-1")));
}

void EditTextReplaceDialog::NewBeforeSlot (void)
{
    QModelIndex index = ui->EnumsTreeView->selectionModel()->currentIndex();
    QAbstractItemModel *model = ui->EnumsTreeView->model();

    if (!model->insertRow(index.row(), index.parent()))
        return;

    InitNewRow (model, index.row());
}

void EditTextReplaceDialog::NewBehindSlot (void)
{
    QModelIndex index = ui->EnumsTreeView->selectionModel()->currentIndex();
    QAbstractItemModel *model = ui->EnumsTreeView->model();

    if (!model->insertRow(index.row()+1, index.parent()))
        return;

    InitNewRow (model, index.row()+1);
}

void EditTextReplaceDialog::SelectAllSlot (void)
{
    ui->EnumsTreeView->selectAll();
}


void EditTextReplaceDialog::SortAllSlot (void)
{
    m_proxyModel->sort(0);
}

void EditTextReplaceDialog::SyntaxCheckSlot (void)
{
    QString TextReplaceString = GetModifiedTextReplaceString();
    SyntaxCheck (TextReplaceString);
}

void EditTextReplaceDialog::on_CutPushButton_clicked()
{
    CutSlot();
}

void EditTextReplaceDialog::on_CopyPushButton_clicked()
{
    CopySlot();
}

void EditTextReplaceDialog::on_PasteBeforePushButton_clicked()
{
    PasteBeforeSlot();
}

void EditTextReplaceDialog::on_PasteBehindPushButton_clicked()
{
    PasteBehindSlot();
}

void EditTextReplaceDialog::on_NewBeforePushButton_clicked()
{
    NewBeforeSlot();
}

void EditTextReplaceDialog::on_NewBehindPushButton_clicked()
{
    NewBehindSlot();
}

void EditTextReplaceDialog::on_SelectAllPushButton_clicked()
{
    SelectAllSlot();
}

void EditTextReplaceDialog::on_pushButton_clicked()
{
    SortAllSlot ();
}

void EditTextReplaceDialog::on_SyntaxCheckPushButton_clicked()
{
    SyntaxCheckSlot ();
}



bool EditTextReplaceDialog::SyntaxCheck (QString &par_TextReplaceString)
{
    int Index = 0;
    char EnumString[1024];
    int64_t From;
    int64_t To;
    int32_t Color;
    int Row = 0;
    char ErrMsgBuffer[1024];

    while (1) {
        int Ret = GetNextEnumFromEnumListErrMsg (Index, QStringToConstChar(par_TextReplaceString),
                                                 EnumString, sizeof(EnumString), &From, &To, &Color, ErrMsgBuffer, sizeof (ErrMsgBuffer));
        if (Ret == -1) {
            // End of the text replace string reached
            break;
        } else if (Ret == -2) {
            // Error inside the text replace string
            ui->EnumsTreeView->setCurrentIndex (m_proxyModel->index (Row, 0));
            ThrowError (1, "text replace string\n"
                      "%s\n"
                      "has error(s): %s", QStringToConstChar(par_TextReplaceString), ErrMsgBuffer);
            return false;  // not OK
        }
        Row++;
        Index++;
    }
    return true;  // all OK
}



void EditTextReplaceDialog::accept()
{
    QString TextReplaceString = GetModifiedTextReplaceString();
    if (SyntaxCheck (TextReplaceString)) {
        QDialog::accept();
    }
}

ColorTextValidator::ColorTextValidator (QObject *parent) :
     QValidator(parent)
{
}


QValidator::State ColorTextValidator::validate (QString &Input, int &Pos) const
{
    Q_UNUSED(Pos)
    QValidator::State State;
    ColorStringToInt (Input, &State);
    return State;
}



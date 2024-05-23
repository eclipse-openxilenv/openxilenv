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


#ifndef EDITTEXTREPLACEDIALOG_H
#define EDITTEXTREPLACEDIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QAction>
#include <QValidator>
#include <Dialog.h>

#include <QStyledItemDelegate>


int ColorStringToInt (QString par_ColorString, QValidator::State *ret_State);

class ColorView
{
public:
    enum EditMode { Editable, ReadOnly };

    explicit ColorView(int par_Color = -1);

    void paint(QPainter *par_Painter, const QRect &par_Rect) const;
    QSize sizeHint() const;
    int ColorInt() const { return m_Color; }
    int RedColorInt() const { return (m_Color & 0xFF); }
    int GreenColorInt() const { return ((m_Color >> 8) & 0xFF); }
    int BlueColorInt() const { return ((m_Color >> 16) & 0xFF); }
    QColor Color () const { return QColor ((m_Color >> 0) & 0xFF, (m_Color >> 8) & 0xFF, (m_Color >> 16) & 0xFF);}

private:
    int m_Color;
};

Q_DECLARE_METATYPE(ColorView)

class ColorDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ColorDelegate (QWidget *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint (QPainter *par_Painter, const QStyleOptionViewItem &par_Option,
                const QModelIndex &par_Index) const Q_DECL_OVERRIDE;
    QSize sizeHint (const QStyleOptionViewItem &par_Option,
                    const QModelIndex &par_Index) const Q_DECL_OVERRIDE;
    QWidget *createEditor (QWidget *par_Parent, const QStyleOptionViewItem &par_Option,
                           const QModelIndex &par_Index) const Q_DECL_OVERRIDE;
};

class ColorTextValidator : public QValidator
{
    Q_OBJECT
public:
    explicit ColorTextValidator (QObject *parent = nullptr);
    State validate ( QString & input, int & pos ) const Q_DECL_OVERRIDE;
};

class ColorTextDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ColorTextDelegate (QWidget *parent = nullptr) : QStyledItemDelegate(parent) {}

    QWidget *createEditor (QWidget *par_Parent, const QStyleOptionViewItem &par_Option,
                           const QModelIndex &par_Index) const Q_DECL_OVERRIDE;

    void setEditorData(QWidget *par_Editor, const QModelIndex &par_Index) const Q_DECL_OVERRIDE;
    void setModelData(QWidget *par_Editor, QAbstractItemModel *model,
                      const QModelIndex &par_Index) const Q_DECL_OVERRIDE;

private slots:
    void commitAndCloseEditor();
};


class SortEnumProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SortEnumProxyModel (QObject *parent = nullptr);
    ~SortEnumProxyModel() Q_DECL_OVERRIDE;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;

private:
    bool dateInRange(const QDate &date) const;
};


namespace Ui {
class EditTextReplaceDialog;
}

class EditTextReplaceDialog : public Dialog
{
    Q_OBJECT

public:
    explicit EditTextReplaceDialog(QString &par_TextReplaceString, QWidget *parent = nullptr);
    ~EditTextReplaceDialog();

    QString GetModifiedTextReplaceString ();

public slots:
    void accept();

private slots:
    void on_EnumsTreeView_customContextMenuRequested(const QPoint &pos);

//public slots:
    void CutSlot (void);
    void CopySlot (void);
    void PasteBeforeSlot (void);
    void PasteBehindSlot (void);
    void NewBeforeSlot (void);
    void NewBehindSlot (void);
    void SelectAllSlot (void);
    void SortAllSlot (void);
    void SyntaxCheckSlot (void);


    void on_CutPushButton_clicked();

    void on_CopyPushButton_clicked();

    void on_PasteBeforePushButton_clicked();

    void on_PasteBehindPushButton_clicked();

    void on_NewBeforePushButton_clicked();

    void on_NewBehindPushButton_clicked();

    void on_SelectAllPushButton_clicked();

    void on_pushButton_clicked();

    void on_SyntaxCheckPushButton_clicked();

private:
    void createEnumModel(QString &par_TextReplaceString, QObject *parent);
    void createActions(void);
    QString GetTextSelectedReplaceString ();
    void InsertReplaceStringBeginAt (QString &par_TextReplaceString, int par_StartAtRow);

    void InitNewRow (QAbstractItemModel *Model, int Row);
    bool SyntaxCheck(QString &par_TextReplaceString);

    Ui::EditTextReplaceDialog *ui;

    SortEnumProxyModel *m_proxyModel;
    //QStandardItemModel *m_Model;

    QAction *m_CutAct;
    QAction *m_CopyAct;
    QAction *m_PasteBeforeAct;
    QAction *m_PasteBehindAct;
    QAction *m_NewBeforeAct;
    QAction *m_NewBehindAct;
    QAction *m_SelectAllAct;
    QAction *m_SortAllAct;
    QAction *m_SyntaxCheckAct;

    QString m_CopyBuffer;
};

#endif // EDITTEXTREPLACEDIALOG_H

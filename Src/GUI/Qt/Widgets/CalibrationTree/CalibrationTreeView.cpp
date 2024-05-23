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


#include <QMenu>
#include "CalibrationTreeView.h"
#include "CalibrationTreeWidget.h"
#include "StringHelpers.h"

extern "C" {
#include "ExtProcessReferences.h"
}


CalibrationTreeView::CalibrationTreeView (QWidget *parent, CalibrationTreeWidget *par_CalibrationTreeWidget) : QTreeView(parent)
{
    m_CalibrationTreeWidget = par_CalibrationTreeWidget;

    this->setObjectName (QStringLiteral("view"));
    this->setAlternatingRowColors (true);
    this->setSelectionBehavior (QAbstractItemView::SelectItems);
    this->setSelectionMode (QAbstractItemView::ExtendedSelection);
    this->setHorizontalScrollMode (QAbstractItemView::ScrollPerPixel);
    this->setAnimated (false);
    this->setAllColumnsShowFocus (true);
    m_DelegateEditor = new  CalibrationTreeDelegateEditor;
    this->setItemDelegate(m_DelegateEditor);
    createActions ();
}

CalibrationTreeView::~CalibrationTreeView()
{
}

QModelIndexList CalibrationTreeView::GetSelectedIndexes()
{
    return selectedIndexes();
}


void CalibrationTreeView::FitColumnSizes (void)
{
}

void CalibrationTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_CalibrationTreeWidget != nullptr) {
        QMenu menu(this);

        menu.addAction (m_ConfigDialogAct);
        menu.addAction (m_ReloadValuesAct);
        if (m_CalibrationTreeWidget != nullptr) {
            QModelIndex Selected = currentIndex();
            if (Selected.isValid()) {
                int BBDataTypeNr = Selected.data(Qt::UserRole + 7).toInt();  // BlackboardTypeNr or -1 if not a base data type
                if (BBDataTypeNr >= 0) {
                    QString Label = Selected.data(Qt::UserRole + 4).toString();       // complete lable name
                    if (Label.size() > 0) {
                        int Pid = m_CalibrationTreeWidget->GetPid();
                        if (Pid > 0) {
                            uint64_t Addr;
                            int Type;
                            if (GetVarAddrFromProcessRefList (Pid, QStringToConstChar(Label), &Addr, &Type)) {
                                menu.addAction (m_ReferenceAct);
                                menu.addAction (m_ReferenceUserNameAct);
                            } else {
                                menu.addAction (m_DeReferenceAct);
                            }
                        }
                    }
                }
            }
        }
        menu.addAction (m_ChangeAllSelectedValuesAct);
        menu.addAction (m_ListAllReferencesAct);
        menu.exec(event->globalPos());
    }
}

void CalibrationTreeView::paintEvent(QPaintEvent *event)
{
    QTreeView::paintEvent(event);
}

void CalibrationTreeView::createActions(void)
{
    bool Ret;
    if (m_CalibrationTreeWidget != nullptr) {
        m_ConfigDialogAct = new QAction(tr("&config"), this);
        m_ConfigDialogAct->setStatusTip(tr("configure calibration tree view"));
        Ret = connect(m_ConfigDialogAct, SIGNAL(triggered()), m_CalibrationTreeWidget, SLOT(ConfigDialogSlot()));

        m_ReloadValuesAct = new QAction(tr("&reload values"), this);
        m_ReloadValuesAct->setStatusTip(tr("reload the values from external process"));
        Ret = connect(m_ReloadValuesAct, SIGNAL(triggered()), m_CalibrationTreeWidget, SLOT(ReloadValuesSlot()));

        m_ReferenceAct = new QAction(tr("re&ference label"), this);
        m_ReferenceAct->setStatusTip(tr("reference label"));
        Ret = connect(m_ReferenceAct, SIGNAL(triggered()), m_CalibrationTreeWidget, SLOT(ReferenceLabelSlot()));

        m_ReferenceUserNameAct = new QAction(tr("reference label(s) &with renaming ..."), this);
        m_ReferenceUserNameAct->setStatusTip(tr("Reference label(s) with renaming ..."));
        Ret = connect(m_ReferenceUserNameAct, SIGNAL(triggered()), m_CalibrationTreeWidget, SLOT(ReferenceLabelUserNameSlot()));

        m_DeReferenceAct = new QAction(tr("&dereference label"), this);
        m_DeReferenceAct->setStatusTip(tr("dreference label"));
        Ret = connect(m_DeReferenceAct, SIGNAL(triggered()), m_CalibrationTreeWidget, SLOT(DeReferenceLabelSlot()));

        m_ChangeAllSelectedValuesAct = new QAction(tr("change &all selected values"), this);
        m_ChangeAllSelectedValuesAct->setStatusTip(tr("change all selected values"));
        Ret = connect(m_ChangeAllSelectedValuesAct, SIGNAL(triggered()), m_CalibrationTreeWidget, SLOT(ChangeValuesAllSelectedLabelsSlot()));

        m_ListAllReferencesAct = new QAction(tr("&list all referenced label"), this);
        m_ListAllReferencesAct->setStatusTip(tr("list all referenced label for this external process"));
        Ret = connect(m_ListAllReferencesAct, SIGNAL(triggered()), m_CalibrationTreeWidget, SLOT(ListAllReferencedLabelSlot()));
    }
}

QString CalibrationTreeView::GetSelectedLabel ()
{
    QList<QString>Selected = GetSelectedLabels();
    if (Selected.size() > 0) {
        return Selected.at(0);
    }
    return QString();
}

QList<QString>CalibrationTreeView::GetSelectedLabels ()
{
    QList<QString>Ret;
    QList<QModelIndex> Selected = GetSelectedIndexes();
    for (int i = 0; i < Selected.size(); ++i) {
        QModelIndex ModelIndex = Selected.at(i);
        QVariant Variant = ModelIndex.data(Qt::UserRole + 4);  // complete lable
        QString CompleteLabel = Variant.toString();
        Ret.append(CompleteLabel);
    }
    return Ret;
}

void CalibrationTreeView::UpdateVisableValue()
{
    CalibrationTreeModel *Model = static_cast<CalibrationTreeModel*>(this->model());
    QModelIndex StartIndex = this->indexAt(rect().topLeft());
    QModelIndex EndIndex = this->indexAt(rect().bottomRight());

    for (QModelIndex Index = StartIndex; Index.isValid() && (Index != EndIndex); Index = indexBelow(Index)) {
        int Row = Index.row();
        int Column = Index.column();
        if (Column == 0) { // only the value column
            QModelIndex Index2 = Model->index(Row, 1, Model->parent(Index));
            Model->data(Index2, Qt::DisplayRole);   // trigger a delayed read
        }
    }
}



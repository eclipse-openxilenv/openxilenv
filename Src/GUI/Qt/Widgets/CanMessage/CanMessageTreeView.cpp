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


#include "CanMessageTreeView.h"
#include "CanMessageWindowWidget.h"

#include <QMenu>


CANMessageTreeView::CANMessageTreeView (CANMessageWindowWidget *par_CANMessageWindowWidget, QWidget *parent) : QTreeView(parent)
{
    m_CANMessageWindowWidget = par_CANMessageWindowWidget;
    createActions();
}

CANMessageTreeView::~CANMessageTreeView()
{

}


void CANMessageTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_CANMessageWindowWidget != nullptr) {
        QMenu menu(this);

        menu.addAction (m_ConfigDialogAct);
        menu.addAction (m_ClearAct);
        menu.exec(event->globalPos());
    }
}


void CANMessageTreeView::createActions(void)
{
    if (m_CANMessageWindowWidget != nullptr) {
        m_ConfigDialogAct = new QAction(tr("&config"), this);
        m_ConfigDialogAct->setStatusTip(tr("configure CAN message view"));
        connect(m_ConfigDialogAct, SIGNAL(triggered()), m_CANMessageWindowWidget, SLOT(ConfigDialogSlot()));
        m_ClearAct = new QAction(tr("&clear"), this);
        m_ClearAct->setStatusTip(tr("clear CAN message view"));
        connect(m_ClearAct, SIGNAL(triggered()), m_CANMessageWindowWidget, SLOT(ClearSlot()));
    }
}

void CANMessageTreeView::FitColumnSizes (void)
{
    for (int column = 0; column < this->model()->columnCount(); ++column) {
        resizeColumnToContents(column);
    }
}

void CANMessageTreeView::GetColumnsWidth(int par_Column)
{
    columnWidth(par_Column);
}

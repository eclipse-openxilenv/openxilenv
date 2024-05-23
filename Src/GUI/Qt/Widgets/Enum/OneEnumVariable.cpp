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


#include "OneEnumVariable.h"
#include "DragAndDrop.h"
#include "GetEventPos.h"
#include "StringHelpers.h"

#include <QVBoxLayout>
#include <QDrag>
#include <QApplication>

extern "C" {
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "MyMemory.h"
#include "TextReplace.h"
}

OneEnumVariable::OneEnumVariable(QString par_VariableName, QWidget *parent) : QWidget(parent)
{
    m_VariableName = par_VariableName;
    m_Layout = new QVBoxLayout();
    m_Layout->setContentsMargins(QMargins(1,1,1,1));
    setLayout(m_Layout);
    m_Label = new QLabel(m_VariableName, this);
    m_Label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_Label->setMinimumHeight(this->fontMetrics().height());
    m_Label->setMaximumHeight(this->fontMetrics().height());
    m_Layout->addWidget(m_Label);
    m_ScrollArea = new QScrollArea(this);
    m_ScrollArea->setWidgetResizable(true);
    m_ScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_ScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_Layout->addWidget(m_ScrollArea);
    m_ScrollAreaWidget = new QWidget(m_ScrollArea);
    m_ScrollAreaLayout = new QVBoxLayout(m_ScrollAreaWidget);
    m_ScrollAreaLayout->setSpacing(1);
    m_ScrollAreaLayout->setContentsMargins(QMargins(1,1,1,1));
    m_ScrollAreaWidget->setLayout(m_ScrollAreaLayout);
    m_ScrollArea->setWidget(m_ScrollAreaWidget);

    m_Vid = add_bbvari(QStringToConstChar(par_VariableName), BB_UNKNOWN_WAIT, nullptr);

    UpdateButtons();
}

OneEnumVariable::~OneEnumVariable()
{
    if (m_Vid > 0) {
        remove_bbvari_unknown_wait(m_Vid);
    }
    DeleteAllButtons();
}

QString OneEnumVariable::GetName()
{
    return m_VariableName;
}

int OneEnumVariable::GetVid()
{
    return m_Vid;
}


void OneEnumVariable::UpdateButtons()
{
    char *loc_EnumString;
    char loc_EnumStringBuffer[512];
    int loc_Idx;
    char loc_EnumEntry[512];
    int64_t loc_Low, loc_High;
    int32_t loc_Color;

    // first delete the old buttons
    DeleteAllButtons();

    int DataType = get_bbvaritype(m_Vid);
    if (DataType > BB_DOUBLE) {
        return;  // only with valid data type
    }
    if (get_bbvari_conversiontype(m_Vid) != 2) {
        return;  // it has no enum
    }
    // get needed buffer size
    loc_EnumString = loc_EnumStringBuffer;
    int loc_NegSize = get_bbvari_conversion(m_Vid, loc_EnumString, sizeof(loc_EnumStringBuffer));
    if (loc_NegSize < 0) {
        // don't fit
        if (loc_NegSize <= -static_cast<int>(sizeof(loc_EnumStringBuffer))) {
            loc_EnumString = static_cast<char*>(my_malloc (static_cast<size_t>(-loc_NegSize)));
            if (get_bbvari_conversion(m_Vid, loc_EnumString, -loc_NegSize) <= 0) {
                return;
            }
        }
    }
    loc_Idx = 0;
    while ((loc_Idx = GetNextEnumFromEnumList (loc_Idx, loc_EnumString,
                                               loc_EnumEntry, sizeof (loc_EnumEntry), &loc_Low, &loc_High, &loc_Color)) > 0) {
        QColor Color = QColor(loc_Color&0x000000FF, (loc_Color&0x0000FF00)>>8, (loc_Color&0x00FF0000)>>16); // Sh. Win Colors 0x00bbggrr
        OneEnumButton *Button = new OneEnumButton(CharToQString(loc_EnumEntry), Color, loc_Low, loc_High,  this, m_ScrollAreaWidget);
        m_ScrollAreaLayout->addWidget(Button);
        m_Buttons.append(Button);
    }
    if (loc_EnumString != loc_EnumStringBuffer) {
        // If ther was allocated a buffer delete it
        my_free(loc_EnumString);
    }
}

void OneEnumVariable::blackboardValueChange(double arg_Value)
{
    write_bbvari_minmax_check(m_Vid, arg_Value);
}

void OneEnumVariable::CyclicUpdate()
{
    double Value = read_bbvari_convert_double(m_Vid);
    foreach (OneEnumButton *Button, m_Buttons) {
        Button->CyclicUpdate(Value);
    }
}

void OneEnumVariable::mousePressEvent(QMouseEvent *event)
{
    // Drag & Drop?
    m_startDragPosition = GetEventGlobalPos(event);
}

void OneEnumVariable::mouseMoveEvent(QMouseEvent *event)
{
    if(!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    if((GetEventGlobalPos(event) - m_startDragPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }
    DragAndDropInfos Infos;
    Infos.SetName(m_VariableName);
    QDrag *loc_dragObject = buildDragObject(this, &Infos);
    loc_dragObject->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);

    QWidget::mouseMoveEvent(event);

}

void OneEnumVariable::DeleteAllButtons()
{
    foreach (OneEnumButton *Button, m_Buttons) {
        delete Button;
    }
    m_Buttons.clear();
}

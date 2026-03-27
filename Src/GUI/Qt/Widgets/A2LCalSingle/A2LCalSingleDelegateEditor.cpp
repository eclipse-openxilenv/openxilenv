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


#include <float.h>
#include "A2LCalSingleDelegateEditor.h"

#include <QComboBox>

#include "PhysValueInput.h"
#include "A2LCalSingleModel.h"
extern "C" {
#include "BlackboardConversion.h"
}

A2LCalSingleDelegateEditor::A2LCalSingleDelegateEditor(QObject *parent) :
    QItemDelegate(parent)
{
}

A2LCalSingleDelegateEditor::~A2LCalSingleDelegateEditor()
{
}


QWidget *A2LCalSingleDelegateEditor::createEditor(QWidget *parent,
                                                  const QStyleOptionViewItem &option,
                                                  const QModelIndex &index) const
{
    Q_UNUSED(index)
    Q_UNUSED(option)

    int Row = index.row();
    int Column = index.column();
    const A2LCalSingleModel *Model = static_cast<const A2LCalSingleModel*>(index.model());
    A2LCalSingleData *Item = Model->getVariable(Row);
    if (Item == nullptr) return nullptr;

    switch (Column) {
    case 3:  // Display type
        {
            // Phys/Raw Umschaltung ueber Combobox
            QComboBox *editor = new QComboBox(parent);
            editor->addItem("[dec]");
            editor->addItem("[hex]");
            editor->addItem("[bin]");
            editor->addItem("[phys]");
            return editor;
        }
    case 1:  // Value
        {
            bool Physical = Item->GetDisplayType() == 3;
            PhysValueInput *editor = new PhysValueInput(parent, -1, !Physical, Physical);
            if (Physical) {  // physical
                editor->SetConersionTypeAndString((enum BB_CONV_TYPES)Item->GetInfos()->m_ConvType,
                                                  Item->GetInfos()->m_Conversion);
            }
            double Min, Max;

            // tune a value only inside the range between the previous and next value
            if (Item->GetMinMax(&Min, &Max)) {
                editor->SetRawMinMaxValue(Min, Max);
                connect(editor, SIGNAL(LeaveEditor()), this, SLOT(commitAndCloseEditor()));
                return editor;
            }
        }
        break;
    }
    return nullptr;
}


void A2LCalSingleDelegateEditor::setEditorData(QWidget *editor,
                                               const QModelIndex &index) const
{
    int Row = index.row();
    int Column = index.column();
    const A2LCalSingleModel *Model = static_cast<const A2LCalSingleModel*>(index.model());
    A2LCalSingleData *Item = Model->getVariable(Row);
    if (Item == nullptr) return;
    int Type = Item->GetType();
    if (Type != 1) return;  // only single values
    A2L_SINGLE_VALUE* ValuePtr = Item->GetValue();
    if (ValuePtr == nullptr) return;

    switch (Column) {
    case 3:  // Display type
        {
            QComboBox *Edit = static_cast<QComboBox*>(editor);
            switch (Item->GetDisplayType()) {
            case 0: Edit->setCurrentText("[dec]"); break;
            case 1: Edit->setCurrentText("[hex]"); break;
            case 2: Edit->setCurrentText("[bin]"); break;
            case 3: Edit->setCurrentText("[phys]"); break;
            }
        }
        break;
    case 1:  // Value
        {
            double Min, Max;
            PhysValueInput *Edit = static_cast<PhysValueInput*>(editor);
            Item->GetMinMax(&Min, &Max);
            Edit->SetRawMinMaxValue(Min, Max);
            int Base;
            switch (Item->GetDisplayType()) {
            default:
            case 0:
                Base = 10;
                break;
            case 1:  // Hex
                Base = 16;
                break;
            case 2:  // Bin
                Base = 2;
                break;
            }
            switch(ValuePtr->Type) {
            case A2L_ELEM_TYPE_INT:
                Edit->SetRawValue(ValuePtr->Value.Int, Base);
                break;
            case A2L_ELEM_TYPE_UINT:
                Edit->SetRawValue(ValuePtr->Value.Uint, Base);
                break;
            case A2L_ELEM_TYPE_DOUBLE:
                Edit->SetRawValue(ValuePtr->Value.Double);
                break;
            case A2L_ELEM_TYPE_PHYS_DOUBLE:
            case A2L_ELEM_TYPE_TEXT_REPLACE:
            case A2L_ELEM_TYPE_ERROR:
            default:
                break;
            }
        }
        break;
    }
}


void A2LCalSingleDelegateEditor::setModelData(QWidget *editor,
                                              QAbstractItemModel *model,
                                              const QModelIndex &index) const
{
    int Row = index.row();
    int Column = index.column();
    const A2LCalSingleModel *Model = static_cast<const A2LCalSingleModel*>(index.model());
    A2LCalSingleData *Item = Model->getVariable(Row);
    if (Item == nullptr) return;
    int Type = Item->GetType();
    if (Type != 1) return;  // only single values
    A2L_SINGLE_VALUE* ValuePtr = Item->GetValue();
    if (ValuePtr == nullptr) return;

    switch (Column) {
    case 3:  // Display type
        {
            QComboBox *Edit = static_cast<QComboBox*>(editor);
            QVariant Value;
            Value = Edit->currentIndex();
            model->setData(index, Value, Qt::EditRole);
        }
        break;
    case 1: // Value
        {
            PhysValueInput *Edit = static_cast<PhysValueInput*>(editor);
            bool Ok = false;
            QVariant Value;
            switch(ValuePtr->Type) {
            case A2L_ELEM_TYPE_INT:
                ValuePtr->Value.Int = Edit->GetInt64RawValue(&Ok);
                Value = (qlonglong)ValuePtr->Value.Int;
                break;
            case A2L_ELEM_TYPE_UINT:
                ValuePtr->Value.Uint = Edit->GetUInt64RawValue(&Ok);
                Value = (qulonglong)ValuePtr->Value.Uint;
                break;
            case A2L_ELEM_TYPE_DOUBLE:
                Value = ValuePtr->Value.Double = Edit->GetDoubleRawValue(&Ok);
                break;
            case A2L_ELEM_TYPE_PHYS_DOUBLE:
            case A2L_ELEM_TYPE_TEXT_REPLACE:
            case A2L_ELEM_TYPE_ERROR:
            default:
                break;
            }
            if (Ok) {
                model->setData(index, Value, Qt::EditRole);
            }
        }
        break;
    default:
        break;
    }
}


void A2LCalSingleDelegateEditor::updateEditorGeometry(QWidget *editor,
                                                      const QStyleOptionViewItem &option,
                                                      const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

void A2LCalSingleDelegateEditor::commitAndCloseEditor()
{
    PhysValueInput *editor = qobject_cast<PhysValueInput *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}




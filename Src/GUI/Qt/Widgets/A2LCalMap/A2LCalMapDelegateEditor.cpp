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
#include "A2LCalMapDelegateEditor.h"

#include <QComboBox>

#include "PhysValueInput.h"
#include "A2LCalMapModel.h"
#include "A2LCalMapData.h"

A2LCalMapDelegateEditor::A2LCalMapDelegateEditor(QObject *parent) :
    QItemDelegate(parent)
{
}

A2LCalMapDelegateEditor::~A2LCalMapDelegateEditor()
{
}

QWidget *A2LCalMapDelegateEditor::createEditor(QWidget *parent,
                                               const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const
{
    Q_UNUSED(index)
    Q_UNUSED(option)

    int Row = index.row();
    int Column = index.column();
    const A2LCalMapModel *Model = static_cast<const A2LCalMapModel*>(index.model());
    A2LCalMapData *Data = Model->GetInternalDataStruct();

    if ((Row == 0) && (Column == 0)) {
        // Phys/Raw Umschaltung ueber Combobox
        QComboBox *editor = new QComboBox(parent);
        editor->addItem("raw");
        editor->addItem("phys");

        return editor;
    } else {
        PhysValueInput *editor = new PhysValueInput(parent);
        if (Data->IsPhysical()) {
            const char *Conversion;
            int ConvType;
            if (Row == 0) {
                // x axis
                Conversion = Data->GetXConversion();
                ConvType = Data->GetXConversionType();
            } else if (Column == 0) {
                // y axis
                Conversion = Data->GetYConversion();
                ConvType = Data->GetYConversionType();
            } else{
                // Map
                Conversion = Data->GetMapConversion();
                ConvType = Data->GetMapConversionType();
            }
            switch (ConvType) {
            case 1:
                editor->SetFormulaString(Conversion);
                break;
            case 2:
                editor->SetEnumString(Conversion);
                break;
            }
            editor->SetDisplayPhysValue(true);
            editor->SetDisplayRawValue(false);
        } else {
            editor->SetDisplayRawValue(true);
            editor->SetDisplayPhysValue(false);
        }
        double Min, Max;

        // tune a value only inside the range between the previous and next value
        Min = DBL_MIN;
        Max = DBL_MAX;
        bool CheckMinMax = false;
        int Type = Data->GetType();
        if (((Type == 1) || (Type == 2)) &&   // if this is a curve or map
            (Row == 0) && (Column > 0)) {    // and it is the x axis
            int XDim = Data->GetXDim();
            if ((XDim > 1) && (!Data->XAxisNotCalibratable())) {      // more than one value and tunable
                // x axis
                int x = Column - 1;
                if (x < XDim) {  // safety check
                    if ((x == 0) || (x == (XDim-1))) {
                        // first or last element of the x axis
                        get_datatype_min_max_value (Data->GetDataType(A2LCalMapData::ENUM_AXIS_TYPE_X_AXIS, x), &Min, &Max);
                        if (x == 0) {
                            Max = Data->GetRawValue(0, 1, 0);
                        } else {
                            Min = Data->GetRawValue(0, XDim-1, 0);
                        }
                    } else {
                        Max = Data->GetRawValue(0, x+1, 0);
                        Min = Data->GetRawValue(0, x-1, 0);
                    }
                    CheckMinMax = true;
                }
            }
        } else if ((Type == 2) &&       // it is a map
                   (Column == 0) && (Row > 0)) {    // an it is the y axis
            int YDim = Data->GetYDim();
            if ((YDim > 1) && (!Data->YAxisNotCalibratable())) {      // more than one value and tunable
                // y axis
                int y = Row - 1;
                if (y < YDim) {  // nur zur Sicherheit, sollte immer true sein
                    if ((y == 0) || (y == (YDim-1))) {
                        // first or last element of the x axis
                        get_datatype_min_max_value (Data->GetDataType(A2LCalMapData::ENUM_AXIS_TYPE_Y_AXIS, y), &Min, &Max);
                        if (y == 0) {
                            Max = Data->GetRawValue(1, 1, 0);
                        } else {
                            Min = Data->GetRawValue(1, YDim-1, 0);
                        }
                    } else {
                        Max = Data->GetRawValue(1, y+1, 0);
                        Min = Data->GetRawValue(1, y-1, 0);
                    }
                    CheckMinMax = true;
                }
            }
        }
        if (CheckMinMax) editor->SetRawMinMaxValue(Min, Max);

        connect(editor, SIGNAL(LeaveEditor()), this, SLOT(commitAndCloseEditor()));

        return editor;
    }
}


void A2LCalMapDelegateEditor::setEditorData(QWidget *editor,
                                                  const QModelIndex &index) const
{
    int Row = index.row();
    int Column = index.column();
    const A2LCalMapModel *Model = static_cast<const A2LCalMapModel*>(index.model());
    if ((Row == 0) && (Column == 0)) {
        QComboBox *Edit = static_cast<QComboBox*>(editor);
        A2LCalMapData *Data = Model->GetInternalDataStruct();
        if (Data->IsPhysical()) {
            Edit->setCurrentText("phys");
        } else {
            Edit->setCurrentText("raw");
        }
    } else {
        PhysValueInput *Edit = static_cast<PhysValueInput*>(editor);
        bool Ok;
        double Value = Model->data(index, Qt::UserRole + 1).toDouble(&Ok);
        if (Ok) {
            Edit->SetRawValue(Value);
        }
    }
}


void A2LCalMapDelegateEditor::setModelData(QWidget *editor,
                                                 QAbstractItemModel *model,
                                                 const QModelIndex &index) const
{
    int Row = index.row();
    int Column = index.column();
    if ((Row == 0) && (Column == 0)) {
        QComboBox *Edit = static_cast<QComboBox*>(editor);
        if (!Edit->currentText().compare("phys")) {
            model->setData(index, 1, Qt::UserRole + 1);
        } else {
            model->setData(index, 0, Qt::UserRole + 1);
        }
    } else {
        PhysValueInput *Edit = static_cast<PhysValueInput*>(editor);
        bool Ok;
        double Value = Edit->GetDoubleRawValue(&Ok);
        if (Ok) {
            model->setData(index, Value, Qt::UserRole + 1);   // Double Value
        }
    }
}


void A2LCalMapDelegateEditor::updateEditorGeometry(QWidget *editor,
                                                   const QStyleOptionViewItem &option,
                                                   const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

void A2LCalMapDelegateEditor::commitAndCloseEditor()
{
    PhysValueInput *editor = qobject_cast<PhysValueInput *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}




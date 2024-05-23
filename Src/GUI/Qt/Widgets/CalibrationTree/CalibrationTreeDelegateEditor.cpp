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


#include "CalibrationTreeDelegateEditor.h"

#include "ValueInput.h"

extern "C" {
#include "DebugInfoDB.h"
}


CalibrationTreeDelegateEditor::CalibrationTreeDelegateEditor(QObject *parent) :
    QItemDelegate(parent)
{
}

CalibrationTreeDelegateEditor::~CalibrationTreeDelegateEditor()
{
}

QWidget *CalibrationTreeDelegateEditor::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    Q_UNUSED(index)
    Q_UNUSED(option)
    ValueInput *editor = new ValueInput(parent);
    return editor;
}


void CalibrationTreeDelegateEditor::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    QString value =index.model()->data(index, Qt::EditRole).toString();
    int Type = index.model()->data(index, Qt::UserRole + 1).toInt();  // TypeNr
    ValueInput *line = static_cast<ValueInput*>(editor);
    union BB_VARI MinRawValue;
    union BB_VARI MaxRawValue;
    enum BB_DATA_TYPES BbVariType = static_cast<enum BB_DATA_TYPES>(get_base_type_bb_type(Type));
    if (get_datatype_min_max_union (BbVariType, &MinRawValue, &MaxRawValue) == 0) {
        line->SetMinMaxValue(MinRawValue, MaxRawValue, BbVariType);
    }
    line->setText(value);
}


void CalibrationTreeDelegateEditor::setModelData(QWidget *editor,
                                QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    ValueInput *line = static_cast<ValueInput*>(editor);
    QString value = line->text();
    model->setData(index, value);
}


void CalibrationTreeDelegateEditor::updateEditorGeometry(QWidget *editor,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

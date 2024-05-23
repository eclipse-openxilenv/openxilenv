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


#include "TextTableViewDelegate.h"
#include "PhysValueInput.h"
#include "TextTableModel.h"

extern "C" {
#include "BlackboardAccess.h"
}

TextTableViewDelegate::TextTableViewDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

TextTableViewDelegate::~TextTableViewDelegate()
{
}

QWidget *TextTableViewDelegate::createEditor(QWidget *parent,
                                                   const QStyleOptionViewItem &option,
                                                   const QModelIndex &index) const
{
    Q_UNUSED(index)
    Q_UNUSED(option)

    int Row = index.row();
    int Column = index.column();
    const TextTableModel *Model = static_cast<const TextTableModel*>(index.model());
    TextTableModel::Variable *Variable = Model->getVariable(Row);

    if (Column == 3) {    // Column 3 -> Raw/Hex/Bin/Phys switch
        QComboBox *editor = new QComboBox(parent);
        editor->addItem("raw");
        editor->addItem("hex");
        editor->addItem("bin");
        editor->addItem("phys");
        return editor;
    } else if (Column == 1) {  // Column 1 -> value
        bool DisplayRaw = true;
        bool DisplayPhys = false;
        enum PhysValueInput::DisplayTypeRawValue DisplayType = PhysValueInput::DISPLAY_RAW_VALUE_DECIMAL;
        switch (Variable->m_type) {
        case 0:
            DisplayRaw = true;
            DisplayPhys = false;
            DisplayType = PhysValueInput::DISPLAY_RAW_VALUE_DECIMAL;
            break;
        case 1:
            DisplayRaw = true;
            DisplayPhys = false;
            DisplayType = PhysValueInput::DISPLAY_RAW_VALUE_HEXAMAL;
            break;
        case 2:
            DisplayRaw = true;
            DisplayPhys = false;
            DisplayType = PhysValueInput::DISPLAY_RAW_VALUE_BINARY;
            break;
        case 3:
            DisplayRaw = false;
            DisplayPhys = true;
            DisplayType = PhysValueInput::DISPLAY_RAW_VALUE_DECIMAL;
            break;
        }
        PhysValueInput *editor = new PhysValueInput(parent, Variable->m_vid, DisplayRaw, DisplayPhys, DisplayType);
        editor->DisableAutoScaling();

        return editor;
    }
    return nullptr;
}


void TextTableViewDelegate::setEditorData(QWidget *editor,
                                                const QModelIndex &index) const
{
    int Row = index.row();
    int Column = index.column();
    const TextTableModel *Model = static_cast<const TextTableModel*>(index.model());
    TextTableModel::Variable *Variable = Model->getVariable(Row);
    if (Column == 3) {  // Display type is inside column 3
        QComboBox *Edit = static_cast<QComboBox*>(editor);
        Edit->setCurrentIndex(Variable->m_type);
    }
}


void TextTableViewDelegate::setModelData(QWidget *editor,
                                               QAbstractItemModel *model,
                                               const QModelIndex &index) const
{
    int Row = index.row();
    int Column = index.column();
    TextTableModel *Model = static_cast<TextTableModel*>(model);
    TextTableModel::Variable *Variable = Model->getVariable(Row);
    if (Column == 3) {  //  Display type is inside column 3
        QComboBox *Edit = static_cast<QComboBox*>(editor);
        Model->setDisplayType(Row, Edit->currentIndex());
    } else {
        PhysValueInput *Edit = static_cast<PhysValueInput*>(editor);
        bool Ok;
        double Value = Edit->GetDoubleRawValue(&Ok);
        if (Ok) {
           write_bbvari_minmax_check(Variable->m_vid, Value);
           Model->CyclicUpdateValues(Row, Row, true);
        }
    }
}


void TextTableViewDelegate::updateEditorGeometry(QWidget *editor,
                                                       const QStyleOptionViewItem &option,
                                                       const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}


void TextTableViewDelegate::commitAndCloseEditor()
{
    PhysValueInput *editor = qobject_cast<PhysValueInput *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
    emit editorClosed();
}




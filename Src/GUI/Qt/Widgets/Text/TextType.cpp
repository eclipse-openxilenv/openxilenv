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


#include "TextWidget.h"
#include "OpenElementDialog.h"
#include "TextType.h"

TextType::TextType(QObject* arg_parent) :
    MdiWindowType(MediumUpdateWindow,
                         "Text",
                         "GUI/AllTextWindows",
                         "Text",
                         "Blackboard",
                         arg_parent,
                         QIcon(":/Icons/Text.png"),
                          50, 50, 300, 300)
{
}

TextType::~TextType() {
    m_texts->clear();
    delete m_texts;
}

MdiWindowWidget*TextType::newElement(MdiSubWindow* par_SubWindow)
{
    TextWidget *loc_text = new TextWidget(GetDefaultNewWindowTitle(), par_SubWindow, this);
    loc_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return loc_text;
}

MdiWindowWidget*TextType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    return new TextWidget(arg_WindowName, par_SubWindow, this);
}

QStringList TextType::openElementDialog()
{
    QStringList WindowNameList;
    OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}

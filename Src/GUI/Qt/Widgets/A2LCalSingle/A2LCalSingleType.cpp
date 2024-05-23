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


#include "A2LCalSingleType.h"
#include "OpenElementDialog.h"
#include "A2LCalSingleWidget.h"

A2LCalSingleType::A2LCalSingleType(QObject* arg_parent) :
    MdiWindowType(MediumUpdateWindow,
                         "A2lSingleView",
                         "GUI/AllA2lSingleViewWindows",
                         "A2LCal single",
                         "a2l single view",
                         arg_parent,
                         QIcon (":/Icons/ScrewDriver.ico"),
                          50, 50, 300, 300)
{
}

A2LCalSingleType::~A2LCalSingleType() {
    m_texts->clear();
    delete m_texts;
}

MdiWindowWidget*A2LCalSingleType::newElement(MdiSubWindow* par_SubWindow)
{
    A2LCalSingleWidget *loc_text = new A2LCalSingleWidget(GetDefaultNewWindowTitle(), par_SubWindow, this);
    loc_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return loc_text;
}

MdiWindowWidget*A2LCalSingleType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    return new A2LCalSingleWidget(arg_WindowName, par_SubWindow, this);
}

QStringList A2LCalSingleType::openElementDialog() {
    QStringList WindowNameList;
    OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}

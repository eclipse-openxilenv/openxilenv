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


#include "LampsType.h"

LampsType::LampsType(QObject *parent) :
    MdiWindowType(MediumUpdateWindow, "ControlLamps", "GUI/AllControlLampsViewWindows", "Control Lamps",
                         "Control lamps view", parent, QIcon(":/Icons/Lamps.png"), 20, 20, 200, 200)
{
}

LampsType::~LampsType()
{
    m_lamps->clear();
    delete m_lamps;
}

MdiWindowWidget* LampsType::newElement(MdiSubWindow* par_SubWindow)
{
    return new LampsWidget(GetDefaultNewWindowTitle(), par_SubWindow, this);
}

MdiWindowWidget* LampsType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    if (arg_WindowName == nullptr)
    {
        return newElement(par_SubWindow);
    }
    return new LampsWidget(arg_WindowName, par_SubWindow, this);
}

QStringList LampsType::openElementDialog()
{
    QStringList WindowNameList;
    OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}

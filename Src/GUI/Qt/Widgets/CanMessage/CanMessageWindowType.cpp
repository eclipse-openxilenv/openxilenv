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


#include "CanMessageWindowType.h"
#include "CanMessageWindowWidget.h"
#include "OpenElementDialog.h"

extern "C" {
#include "ThrowError.h"
}

CANMessageWindowType::CANMessageWindowType(QObject *parent) :
    MdiWindowType(MediumUpdateWindow,
                         "CANMessages",
                         "GUI/AllCanMessageWindows",
                         "CAN messages",
                         "CAN Messages",
                         parent,
                         QIcon (":/Icons/CAN.png"),
                         50, 50, 400, 200, true)
{
}

CANMessageWindowType::~CANMessageWindowType()
{
}

MdiWindowWidget* CANMessageWindowType::newElement(MdiSubWindow* par_SubWindow)
{
    return new CANMessageWindowWidget (GetDefaultNewWindowTitle (), par_SubWindow, this);
}

MdiWindowWidget* CANMessageWindowType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    if (arg_WindowName == nullptr) {
        return newElement(par_SubWindow);
    }
    return new CANMessageWindowWidget (QString(arg_WindowName), par_SubWindow, this);
}

QStringList CANMessageWindowType::openElementDialog()
{
    QStringList WindowNameList;
    OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}



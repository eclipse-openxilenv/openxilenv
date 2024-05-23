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


#include "A2LCalMapType.h"
#include "A2LCalMapWidget.h"
#include "OpenElementDialog.h"


A2LCalMapType::A2LCalMapType(QObject *parent) :
    MdiWindowType(SlowUpdateWindow,
                         "A2lMapView",
                         "GUI/AllA2lMapViewWindows",
                         "A2LCal map",
                         "a2l map view",
                         parent,
                         QIcon (":/Icons/Curve.png"),
                         50, 50, 200, 200)
{
}

A2LCalMapType::~A2LCalMapType()
{
}

MdiWindowWidget* A2LCalMapType::newElement(MdiSubWindow* par_SubWindow)
{
    return new A2LCalMapWidget (GetDefaultNewWindowTitle (), par_SubWindow, this);
}


MdiWindowWidget* A2LCalMapType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName) {
    if (arg_WindowName == nullptr) {
        return newElement(par_SubWindow);
    }
    return new A2LCalMapWidget (arg_WindowName, par_SubWindow, this);
}

QStringList A2LCalMapType::openElementDialog() {
   QStringList WindowNameList;
    OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}

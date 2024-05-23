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


#include "BargraphType.h"
#include "BargraphWidget.h"
#include "OpenElementDialog.h"


BargraphType::BargraphType(QObject* parent) : MdiWindowType(FastUpdateWindow,
                                                            "Bargraph",
                                                            "GUI/AllBargraphWindows",
                                                            "Bargraph",
                                                            "Bargraph",
                                                            parent,
                                                            QIcon(":/Icons/Bargraph.ico"),
                                                            20, 20, 100, 200)
{
}

BargraphType::~BargraphType()
{
}

MdiWindowWidget* BargraphType::newElement(MdiSubWindow* par_SubWindow)
{
    BargraphWidget *loc_tacho = new BargraphWidget(GetDefaultNewWindowTitle(), par_SubWindow, this);
    loc_tacho->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return loc_tacho;
}

MdiWindowWidget* BargraphType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    if(arg_WindowName == nullptr) {
        return newElement(par_SubWindow);
    } else {
        BargraphWidget *loc_bargraph = new BargraphWidget(arg_WindowName, par_SubWindow, this);
        return loc_bargraph;
    }
}

QStringList BargraphType::openElementDialog()
{
    QStringList WindowNameList;
    OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}


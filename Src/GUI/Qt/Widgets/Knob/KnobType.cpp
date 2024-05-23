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


#include "KnobType.h"
#include "KnobWidget.h"
#include "OpenElementDialog.h"

KnobType::KnobType(QObject* parent) : MdiWindowType(FastUpdateWindow,
                                                    "Knob",
                                                    "GUI/AllKnobWindows",
                                                    "Knob",
                                                    "Knob", parent,
                                                    QIcon(":/Icons/Knob.png"),
                                                    50, 50, 160, 160)
{
}

KnobType::~KnobType()
{
    m_knobs->clear();
    delete m_knobs;
}

MdiWindowWidget*KnobType::newElement(MdiSubWindow* par_SubWindow)
{
    KnobWidget *loc_knob = new KnobWidget(GetDefaultNewWindowTitle(), par_SubWindow, this);
    loc_knob->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return loc_knob;
}

MdiWindowWidget*KnobType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    if(arg_WindowName == nullptr) {
        return newElement(par_SubWindow);
    } else {
        KnobWidget *loc_knob = new KnobWidget(arg_WindowName, par_SubWindow, this);
        return loc_knob;
    }
}

QStringList KnobType::openElementDialog()
{
    QStringList WindowNameList;
    OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}


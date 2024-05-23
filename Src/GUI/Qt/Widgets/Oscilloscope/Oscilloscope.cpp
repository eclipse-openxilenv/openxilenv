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

#include <math.h>
#include "OpenElementDialog.h"
#include "Oscilloscope.h"

extern "C" {
    #include "ThrowError.h"
    #include "Scheduler.h"
}

OscilloscopeType::OscilloscopeType(QObject *parent) :
    MdiWindowType(FastUpdateWindow,
                         "Oscilloscope",
                         "GUI/AllOscilloscopeWindows",
                         "Oscilloscope",
                         "Oscilloscope",
                         parent,
                         QIcon (":/Icons/Graph.ico"),
                         100, 100, 400, 200, true) {
}

OscilloscopeType::~OscilloscopeType() {
}

MdiWindowWidget* OscilloscopeType::newElement(MdiSubWindow* par_SubWindow) {
    if (get_pid_by_name ("Oscilloscope") > 0) {
        return new OscilloscopeWidget (GetDefaultNewWindowTitle (), par_SubWindow, this);
    } else {
        ThrowError (MESSAGE_STOP, "cannot open oscillocope window because the \"Oscilloscope\" process doesn't run");
        return nullptr;
    }
}

MdiWindowWidget* OscilloscopeType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    if (get_pid_by_name ("Oscilloscope") > 0) {
        return new OscilloscopeWidget(arg_WindowName, par_SubWindow, this);
    } else {
        ThrowError (MESSAGE_STOP, "cannot open oscillocope window because the \"Oscilloscope\" process doesn't run");
        return nullptr;
    }
}

QStringList OscilloscopeType::openElementDialog()
{
    QStringList WindowNameList;
    if (get_pid_by_name ("Oscilloscope") > 0) {
        OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
        if (dlg.exec() == QDialog::Accepted) {
            WindowNameList = dlg.getToOpenWidgetName();
        }
    } else {
        ThrowError (MESSAGE_STOP, "cannot open oscillocope window because the \"Oscilloscope\" process doesn't run");
    }
    return WindowNameList;
}

QList<OscilloscopeWidget *> OscilloscopeType::GetAllOpenOscilloscopeWidgets()
{
    QList<OscilloscopeWidget *>Ret;
    foreach (MdiWindowWidget* Item, this->GetAllOpenWidgetsOfThisType()) {
        Ret.append(static_cast<OscilloscopeWidget*>(Item));
    }
    return Ret;
}

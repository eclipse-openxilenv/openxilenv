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


#include "SliderType.h"
#include "SliderWidget.h"
#include "OpenElementDialog.h"


SliderType::SliderType(QObject* parent) : MdiWindowType(FastUpdateWindow,
                                                        "Slider",
                                                        "GUI/AllSliderWindows",
                                                        "Slider",
                                                        "Slider",
                                                        parent,
                                                        QIcon (":/Icons/Slider.ico"),
                                                        20, 20, 100, 200) {

}

SliderType::~SliderType() {

}

MdiWindowWidget* SliderType::newElement(MdiSubWindow* par_SubWindow)
{
    SliderWidget *loc_tacho = new SliderWidget(GetDefaultNewWindowTitle(), par_SubWindow, this);
    loc_tacho->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return loc_tacho;
}

MdiWindowWidget* SliderType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    if(arg_WindowName == nullptr) {
        return newElement(par_SubWindow);
    } else {
        SliderWidget *loc_slider = new SliderWidget(arg_WindowName, par_SubWindow, this);
        return loc_slider;
    }
}

QStringList SliderType::openElementDialog()
{
    QStringList WindowNameList;
    OpenElementDialog dlg (GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}


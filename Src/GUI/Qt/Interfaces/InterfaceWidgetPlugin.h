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


#ifndef INTERFACEWIDGETPLUGIN_H
#define INTERFACEWIDGETPLUGIN_H

#include <QString>
#include "MdiWindowWidget.h"

class MainWindow;
class MdiSubWindow;

class InterfaceWidgetPlugin {
public:
    enum UpdateRate {FastUpdateWindow, MediumUpdateWindow, SlowUpdateWindow, NoneUpdateWindow};
    virtual ~InterfaceWidgetPlugin() {}
    virtual void GetWindowTypeInfos(enum UpdateRate *ret_UpdateRate,
                                    QString *ret_WindowTypeName,
                                    QString *ret_WindowIniListName,
                                    QString *ret_MenuEntryName) = 0;
    virtual MdiWindowWidget* newElement(MdiSubWindow *arg_SubWindow) = 0;
    virtual MdiWindowWidget* openElement(MdiSubWindow *arg_SubWindow, QString &arg_WindowName) = 0;
    virtual QStringList openElementDialog() = 0;
    virtual QString GetMenuEntryName() = 0;
    virtual QString GetWindowIniListName() = 0;
    virtual QString GetWindowTypeName() = 0;
    virtual UpdateRate GetUpdateRate() = 0;
};

QT_BEGIN_NAMESPACE

#define OpenXilEnvPlugin_iid "com.ZF.Qt.OpenXilEnvPlugin"

Q_DECLARE_INTERFACE(InterfaceWidgetPlugin, OpenXilEnvPlugin_iid)
QT_END_NAMESPACE

#endif // INTERFACEWIDGETPLUGIN_H

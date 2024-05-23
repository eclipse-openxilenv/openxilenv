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


#ifndef INTERFACEWIDGETELEMENT_H
#define INTERFACEWIDGETELEMENT_H

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

class InterfaceWidgetElement {
public:
    virtual ~InterfaceWidgetElement() {}
    virtual bool writeToIni() = 0;
    virtual bool readFromIni() = 0;
    virtual void CyclicUpdate() = 0;
    virtual void setDefaultFocus() = 0;

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) = 0;
    virtual void dragMoveEvent(QDragMoveEvent *event) = 0;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) = 0;
    virtual void dropEvent(QDropEvent *event) = 0;
};

QT_BEGIN_NAMESPACE

#define OpenXilEnvElement_iid "com.ZF.Qt.OpenXilEnvElement"

Q_DECLARE_INTERFACE(InterfaceWidgetElement, OpenXilEnvElement_iid)
QT_END_NAMESPACE

#endif // INTERFACEWIDGETELEMENT_H


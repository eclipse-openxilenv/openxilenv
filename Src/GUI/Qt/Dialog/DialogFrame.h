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


#ifndef CUSTOMDIALOGFRAME_H
#define CUSTOMDIALOGFRAME_H

#include <QFrame>

class CustomDialogFrame : public QFrame
{
    Q_OBJECT
public:
    CustomDialogFrame(QWidget *parent = nullptr);
    ~CustomDialogFrame();
    virtual void userAccept() = 0;
    virtual void userReject() = 0;
};

#endif // CUSTOMDIALOGFRAME_H

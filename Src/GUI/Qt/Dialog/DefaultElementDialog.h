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


#ifndef DEFAULTELEMENTDIALOG_H
#define DEFAULTELEMENTDIALOG_H

#include <QDialog>
#include <QFrame>
#include <Dialog.h>
#include "DefaultDialogFrame.h"
#include "DialogFrame.h"

namespace Ui {
class DefaultElementDialog;
}

class DefaultElementDialog : public Dialog
{
    Q_OBJECT

public:

    explicit DefaultElementDialog(QWidget *parent = nullptr);
    ~DefaultElementDialog();
    void addSettings(CustomDialogFrame *arg_settingFrame);
    void setMultiSelect(bool arg_multiSelect = true);
    void setElementWindowName(QString arg_windowTitle);
    void setCurrentVisibleVariable(QString arg_variable);
    void setCurrentVisibleVariables(QStringList arg_variables);
    void setCurrentColor(QColor arg_color);
    void setCurrentFont(QFont arg_font);
    void setVaraibleList(DefaultDialogFrame::VARIABLE_TYPE arg_type = DefaultDialogFrame::ANY);
    DefaultDialogFrame *getDefaultFrame();

private:
    void removeSettingFrame();

public slots:
    void accept();
    void reject();

signals:
    void windowNameChanged(QString arg_name);

private:
    Ui::DefaultElementDialog *ui;

    QString m_defaultWindowName;

    CustomDialogFrame* m_settingFrame;
};

#endif // DEFAULTELEMENTDIALOG_H

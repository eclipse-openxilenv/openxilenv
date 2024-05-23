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


#ifndef NEWHOTKEYDIALOG_H
#define NEWHOTKEYDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <Dialog.h>

namespace Ui {
class NewHotkeyDialog;
}

class NewHotkeyDialog : public Dialog
{
    Q_OBJECT

public:
    explicit NewHotkeyDialog(QWidget *parent = nullptr);
    explicit NewHotkeyDialog(QString shortcut, QString function, QString formula, QWidget *parent = nullptr);
    ~NewHotkeyDialog();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    QStringList reservedHotkeys;
    void setContextValidator(void);
    void setCompleter(void);
    bool isUnreservedHotkey();

public slots:
    void accept();


private slots:
    void on_comboBoxAction_currentIndexChanged(int index);

private:
    Ui::NewHotkeyDialog *ui;
};

#endif // NEWHOTKEYDIALOG_H

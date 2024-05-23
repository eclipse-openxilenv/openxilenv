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


#ifndef HOTKEYSDIALOG_H
#define HOTKEYSDIALOG_H

#include <QDialog>
#include <QStringListModel>
#include <NewHotkeyDialog.h>
#include <Dialog.h>

namespace Ui {
class hotkeysdialog;
}

class HotkeysDialog : public Dialog
{
    Q_OBJECT

public:
    explicit HotkeysDialog(QWidget *parent = nullptr);
    ~HotkeysDialog();


private:
    Ui::hotkeysdialog *ui;

    QStringListModel shortcutStringList;

    void refresh (void);

private slots:

    void on_pushButtonNew_clicked();
    void on_pushButtonClose_clicked();
    void on_pushButtonEdit_clicked();
    void on_pushButtonDelete_clicked();
};

#endif // HOTKEYSDIALOG_H

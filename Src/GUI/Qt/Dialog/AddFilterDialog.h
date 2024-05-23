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


#ifndef ADDFILTERDIALOG_H
#define ADDFILTERDIALOG_H

#include <QDialog>
#include <Dialog.h>

namespace Ui {
class addFilterDialog;
}

class addFilterDialog : public Dialog
{
    Q_OBJECT

public:
    /* include = 0 exclude = 1 */
    explicit addFilterDialog(bool includeExclude, QWidget *parent = nullptr);
    ~addFilterDialog();
    QString getFilterName();

public slots:
    void accept();

private:
    bool includeExclude;
    QString filterName;
    Ui::addFilterDialog *ui;
};

#endif // ADDFILTERDIALOG_H

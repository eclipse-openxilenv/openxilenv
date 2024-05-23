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


#ifndef GLOBALSECTIONFILTERDIALOG_H
#define GLOBALSECTIONFILTERDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <Dialog.h>

extern "C"
{
    #include "SectionFilter.h"
}

namespace Ui {
class globalsectionfilterdialog;
}

class globalsectionfilterdialog : public Dialog
{
    Q_OBJECT

public:
    explicit globalsectionfilterdialog(QWidget *parent = nullptr);
    void addIncludeExcludeSection(QString section, bool includeExclude);
    ~globalsectionfilterdialog();

public slots:
    void accept();

private slots:
    void on_pushButtonIncludeNew_clicked();

    void on_pushButtonExludeNew_clicked();

    void on_pushButtonIncludeAdd_clicked();

    void on_pushButtonExcludeAdd_clicked();

    void on_pushButtonExcludeDel_clicked();

    void on_pushButtonIncludeDel_clicked();

    void on_listWidgetProcesses_itemClicked(QListWidgetItem *item);

private:
    void addSectionToListBox (char *ProcessName);
    SECTION_FILTER_DLG_PARAM *sfdp;
    Ui::globalsectionfilterdialog *ui;
    int UniqueId;
};

#endif // GLOBALSECTIONFILTERDIALOG_H

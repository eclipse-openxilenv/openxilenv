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


#ifndef SEARCHWINDOWINCLUDEDVARIABLEDIALOG_H
#define SEARCHWINDOWINCLUDEDVARIABLEDIALOG_H

#include <QDialog>
#include <QTableWidgetItem>

class MainWindow;

namespace Ui {
class SearchWindowIncludedVariableDialog;
}

class SearchWindowIncludedVariableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchWindowIncludedVariableDialog(MainWindow *par_MainWindow, QWidget *parent = nullptr);
    ~SearchWindowIncludedVariableDialog();

private slots:
    void on_Filter_clicked();

    void on_OpenAndSwitchPushButton_clicked();

    void on_OpenInActualPushButton_clicked();

    void on_ClosePushButton_clicked();

    void on_BlackboardVariableListWidget_clicked(const QModelIndex &index);

    void on_WindowTableWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous);

private:
    void FillBlackboardVariableList();
    int SearchWindowInAllSheets (const char *param_WindowName, const char *WindowType);
    int SearchVariableInAllWindows (const char *par_VariableName);


    Ui::SearchWindowIncludedVariableDialog *ui;

    MainWindow *m_MainWindow;
};

#endif // SEARCHWINDOWINCLUDEDVARIABLEDIALOG_H

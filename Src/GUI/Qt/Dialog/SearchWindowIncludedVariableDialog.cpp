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


#include "SearchWindowIncludedVariableDialog.h"
#include "ui_SearchWindowIncludedVariableDialog.h"

#include "MainWindow.h"
#include "StringHelpers.h"

extern "C" {
#include "IniDataBase.h"
#include "MainValues.h"
#include "Wildcards.h"

}

SearchWindowIncludedVariableDialog::SearchWindowIncludedVariableDialog(MainWindow *par_MainWindow, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchWindowIncludedVariableDialog)
{
    m_MainWindow = par_MainWindow;
    ui->setupUi(this);
    ui->WindowTableWidget->setColumnCount(3);
    QStringList HeaderList;
    HeaderList << "Sheet" << "Window" << "Type";
    ui->WindowTableWidget->setHorizontalHeaderLabels (HeaderList);

    connect(ui->Filter, SIGNAL(FilterClicked()), this, SLOT(on_Filter_clicked ()));
    FillBlackboardVariableList();
}

SearchWindowIncludedVariableDialog::~SearchWindowIncludedVariableDialog()
{
    delete ui;
}

void SearchWindowIncludedVariableDialog::FillBlackboardVariableList()
{
    int index = 0;
    char name[BBVARI_NAME_SIZE];

    ui->BlackboardVariableListWidget->clear();
    INCLUDE_EXCLUDE_FILTER *Filter = ui->Filter->GetFilter();
    while ((index = read_next_blackboard_vari (index, name, sizeof(name))) >= 0) {
        if (CheckIncludeExcludeFilter (name, Filter)) {
            ui->BlackboardVariableListWidget->addItem(CharToQString(name));
        }
    }
    FreeIncludeExcludeFilter(Filter);
}


int SearchWindowIncludedVariableDialog::SearchWindowInAllSheets (const char *param_WindowName, const char *WindowType)
{
    QStringList Sheets = m_MainWindow->GetSheetsInsideWindowIsOpen(QString(param_WindowName));

    if (Sheets.empty()) {
        int Row = ui->WindowTableWidget->rowCount();
        ui->WindowTableWidget->insertRow(Row);
        ui->WindowTableWidget->setItem(Row, 0, new QTableWidgetItem (QString("(not open)")));
        ui->WindowTableWidget->setItem(Row, 1, new QTableWidgetItem (CharToQString(param_WindowName)));
        ui->WindowTableWidget->setItem(Row, 2, new QTableWidgetItem (CharToQString(WindowType)));
    } else {
        foreach (QString Sheet, Sheets) {
            int Row = ui->WindowTableWidget->rowCount();
            ui->WindowTableWidget->insertRow(Row);
            ui->WindowTableWidget->setItem(Row, 0, new QTableWidgetItem (Sheet));
            ui->WindowTableWidget->setItem(Row, 1, new QTableWidgetItem (CharToQString(param_WindowName)));
            ui->WindowTableWidget->setItem(Row, 2, new QTableWidgetItem (CharToQString(WindowType)));
        }
    }
    return 0;
}


int SearchWindowIncludedVariableDialog::SearchVariableInAllWindows (const char *par_VariableName)
{
    int i, x, y;
    char *p;
    char WindowSectionName[INI_MAX_SECTION_LENGTH];
    char *WindowName;
    int WindowNameMaxSize;
    char WindowVariableName[INI_MAX_LINE_LENGTH];
    char Entry[32];
    int Fd = GetMainFileDescriptor();

    // Make sure that all infos are inside the INI file
    SaveAllInfosToIniDataBase();

    strcpy(WindowSectionName, "GUI/Widgets/");
    WindowName = WindowSectionName + strlen(WindowSectionName);
    WindowNameMaxSize = sizeof(WindowSectionName) - strlen(WindowSectionName);

    for (x = 0;;x++) {
        sprintf (Entry, "W%i", x);
        if (IniFileDataBaseReadString ("GUI/AllEnumWindows", Entry, "", WindowName, WindowNameMaxSize, Fd) == 0) {
            break;
        }
        for (i = 0;;i++) {
            sprintf (Entry, "Variable%i", i);
            if (IniFileDataBaseReadString (WindowName, Entry, "", WindowVariableName, sizeof (WindowVariableName), Fd) == 0)
                break;
            if (!strcmp (par_VariableName, WindowVariableName)) {
                SearchWindowInAllSheets (WindowName, "Enum");
            }
        }
    }
    for (x = 0;;x++) {
        sprintf (Entry, "W%i", x);
        if (IniFileDataBaseReadString ("GUI/AllOscilloscopeWindows", Entry, "", WindowName, WindowNameMaxSize, Fd) == 0)
            break;
        for (y = 0; y < 2; y++) {
            for (i = 0; i < 20; i++) {
                if (y) sprintf (Entry, "vl%i", i);
                else sprintf (Entry, "vr%i", i);
                if (IniFileDataBaseReadString (WindowSectionName, Entry, "", WindowVariableName, sizeof (WindowVariableName), Fd) > 0) {
                    // Only name ignore all other things
                    p = WindowVariableName;
                    while ((*p != 0) && (*p != ',')) p++;
                    *p = 0;
                    if (!strcmp (par_VariableName, WindowVariableName)) {
                        SearchWindowInAllSheets (WindowName, "Oscilloscope");
                    }
                }
            }
        }
    }
    for (x = 0;;x++) {
        sprintf (Entry, "W%i", x);
        if (IniFileDataBaseReadString ("GUI/AllTextWindows", Entry, "", WindowName, WindowNameMaxSize, Fd) == 0)
            break;
        for (i = 0;;i++) {
            sprintf (Entry, "E%i", i);
            if (IniFileDataBaseReadString (WindowSectionName, Entry, "", WindowVariableName, sizeof (WindowVariableName), Fd) == 0)
                break;
            // Only name ignore all other things
            p = WindowVariableName;
            while ((*p != 0) && (*p != ',')) p++;
            if (*p == ',') p++;
            while (isspace (*p)) p++;
            if (!strcmp (par_VariableName, p)) {
                SearchWindowInAllSheets (WindowName, "Text");
            }
        }
    }
    for (x = 0;;x++) {
        sprintf (Entry, "W%i", x);
        if (IniFileDataBaseReadString ("GUI/AllTachometerWindows", Entry, "", WindowName, WindowNameMaxSize, Fd) == 0)
            break;
        if (IniFileDataBaseReadString (WindowSectionName, "variable", "", WindowVariableName, sizeof (WindowVariableName), Fd) == 0)
            continue;
        // Only name ignore all other things
        p = WindowVariableName;
        while ((*p != 0) && (*p != ',')) p++;
        *p = 0;
        if (!strcmp (par_VariableName, WindowVariableName)) {
            SearchWindowInAllSheets (WindowName, "Tacho");
        }
    }
    for (x = 0;;x++) {
        sprintf (Entry, "W%i", x);
        if (IniFileDataBaseReadString ("GUI/AllKnobWindows", Entry, "", WindowName, WindowNameMaxSize, Fd) == 0)
            break;
        if (IniFileDataBaseReadString (WindowSectionName, "variable", "", WindowVariableName, sizeof (WindowVariableName), Fd) == 0)
            continue;
        // Only name ignore all other things
        p = WindowVariableName;
        while ((*p != 0) && (*p != ',')) p++;
        *p = 0;
        if (!strcmp (par_VariableName, WindowVariableName)) {
            SearchWindowInAllSheets (WindowName, "Knob");
        }
    }
    for (x = 0;;x++) {
        sprintf (Entry, "W%i", x);
        if (IniFileDataBaseReadString ("GUI/AllSliderWindows", Entry, "", WindowName, WindowNameMaxSize, Fd) == 0)
            break;
        if (IniFileDataBaseReadString (WindowSectionName, "variable", "", WindowVariableName, sizeof (WindowVariableName), Fd) == 0)
            continue;
        // Only name ignore all other things
        p = WindowVariableName;
        while ((*p != 0) && (*p != ',')) p++;
        *p = 0;
        if (!strcmp (par_VariableName, WindowVariableName)) {
            SearchWindowInAllSheets (WindowName, "Slider");
        }
    }
    for (x = 0;;x++) {
        sprintf (Entry, "W%i", x);
        if (IniFileDataBaseReadString ("GUI/AllControlLampsViewWindows", Entry, "", WindowName, WindowNameMaxSize, Fd) == 0)
            break;
        for (i = 0;;i++) {
            sprintf (Entry, "Lamp_%i", i);
            if (IniFileDataBaseReadString (WindowSectionName, Entry, "", WindowVariableName, sizeof (WindowVariableName), Fd) == 0)
                break;
            // Only name ignore all other things
            p = WindowVariableName;
            while ((*p != 0) && (*p != ',')) p++;
            *p = 0;
            if (!strcmp (par_VariableName, WindowVariableName)) {
                SearchWindowInAllSheets (WindowName, "Lamps");
            }
        }
    }
    return 0;
}


void SearchWindowIncludedVariableDialog::on_Filter_clicked()
{
    FillBlackboardVariableList();
}


void SearchWindowIncludedVariableDialog::on_OpenAndSwitchPushButton_clicked()
{
    QList<QTableWidgetItem*> Selected = ui->WindowTableWidget->selectedItems();
    if (Selected.size()) {
        int Row = Selected.at(0)->row();
         QString Sheet = ui->WindowTableWidget->item (Row, 0)->data(Qt::DisplayRole).toString();
         QString Window = ui->WindowTableWidget->item (Row, 1)->data(Qt::DisplayRole).toString();
         m_MainWindow->SelectSheetAndWindow (Sheet, Window);
    }
}

void SearchWindowIncludedVariableDialog::on_OpenInActualPushButton_clicked()
{
}

void SearchWindowIncludedVariableDialog::on_ClosePushButton_clicked()
{
    this->close();
}


void SearchWindowIncludedVariableDialog::on_BlackboardVariableListWidget_clicked(const QModelIndex &index)
{
    for (int Row = ui->WindowTableWidget->rowCount(); Row >= 0; Row--) {
        ui->WindowTableWidget->removeRow(Row);
    }
    QListWidgetItem *Item = ui->BlackboardVariableListWidget->item(index.row());
    QString Name = Item->data(Qt::DisplayRole).toString();
    if (get_bbvarivid_by_name(QStringToConstChar(Name)) > 0) {
        SearchVariableInAllWindows(QStringToConstChar(Name));
    }
    ui->WindowTableWidget->resizeColumnsToContents();
    ui->OpenAndSwitchPushButton->setEnabled(false);
    ui->OpenInActualPushButton->setEnabled(false);
}

void SearchWindowIncludedVariableDialog::on_WindowTableWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (current != nullptr) {
        QString CurrentSheet = m_MainWindow->GetCurrentSheet();
        QString SheetName = current->data(Qt::DisplayRole).toString();
        if (!SheetName.compare(CurrentSheet)) {
            ui->OpenAndSwitchPushButton->setEnabled(true);
            ui->OpenInActualPushButton->setEnabled(true);
        } else if (!SheetName.compare ("(not open)")) {
            ui->OpenAndSwitchPushButton->setEnabled(false);
            ui->OpenInActualPushButton->setEnabled(true);
        } else {
            ui->OpenAndSwitchPushButton->setEnabled(true);
            ui->OpenInActualPushButton->setEnabled(false);
        }
    }
}

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


#include "StartStopCcpCalWidget.h"
#include "ui_StartStopCcpCalWidget.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

#include <QDialog>

extern "C"
{
    #include "StringMaxChar.h"
    #include "ConfigurablePrefix.h"
    #include "Message.h"
    #include "Scheduler.h"
    #include "Blackboard.h"
    #include "BlackboardAccess.h"
    #include "Files.h"
    #include "ThrowError.h"
    #include "MyMemory.h"
    #include "LoadSaveToFile.h"
    #include "Wildcards.h"
    #include "EquationParser.h"
    #include "CcpMessages.h"
    #include "CcpControl.h"
    #include "FileExtensions.h"
}

int StartStopCCPCalWidget::Connection = 0;

StartStopCCPCalWidget::StartStopCCPCalWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StartStopCCPCalWidget),
    ccpconfig_data(new CCPCFGDLG_SHEET_DATA)
{
    ui->setupUi(this);
    curConnection = Connection;
    ui->lineEditStatus->setDisabled(true);
    ui->lineEditFilter->setText("*");
    FillListBoxes("*", true);
    Connection++;
}

StartStopCCPCalWidget::~StartStopCCPCalWidget()
{
    Connection--;
    delete ui;
}

void StartStopCCPCalWidget::on_pushButtonFilter_clicked()
{
    FillListBoxes(QStringToConstChar(ui->lineEditFilter->text()), false);
}

void StartStopCCPCalWidget::on_pushButtonRight_clicked()
{    
    QModelIndexList selectedIndexes;
    selectedIndexes = ui->listWidgetAllVar->selectionModel()->selectedIndexes();
    foreach (QModelIndex Index, selectedIndexes) {
         QString Name = Index.data(Qt::DisplayRole).toString();
         if (!InOnList(Name)) {
             ui->listWidgetOnVar->addItem(Name);
         }
    }
}

void StartStopCCPCalWidget::on_pushButtonLeft_clicked()
{
    QModelIndexList selectedIndexes;
    selectedIndexes = ui->listWidgetOnVar->selectionModel()->selectedIndexes();
    std::sort(selectedIndexes.begin(),selectedIndexes.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
    foreach (QModelIndex Index, selectedIndexes) {
         ui->listWidgetOnVar->model()->removeRow(Index.row()); // remove each row
    }
}

void StartStopCCPCalWidget::FillListBoxes(const char* Filter, bool UpdateSelected)
{
    int vid;
    int Fd = ScQt_GetMainFileDescriptor();

    ui->listWidgetAllVar->clear();
    QString Section = QString("CCP Configuration for Target %1").arg(curConnection);
    int index = 0;
    while (1) {
        QString Entry = QString("p%1").arg(index);
        QString Line = ScQt_IniFileDataBaseReadString (Section, Entry, "", Fd);
        if (Line.isEmpty()) {
            break;
        }
        QStringList List = Line.split(",");
        QString Label = List.at(1).trimmed();
        if (!Compare2StringsWithWildcards (QStringToConstChar(Label), Filter)) {
            ui->listWidgetAllVar->addItem(Label);
        }
        index++;
    }
    QString Label = CharToQString(GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES));
    Label.append(QString(".CCP[%1].Status").arg(curConnection));
    vid = add_bbvari (QStringToChar(Label), BB_UDWORD, nullptr);
    char Text[BBVARI_ENUM_NAME_SIZE];
    read_bbvari_textreplace (vid, Text, sizeof (Text), nullptr);
    remove_bbvari (vid);
    ui->lineEditStatus->setText(CharToQString(Text));
    if (UpdateSelected) {
        ui->listWidgetOnVar->clear();
        for (int i = 0; ; i++) {
            QString Entry  = QString("cal lastselcted_%1").arg(i);
            QString Line = ScQt_IniFileDataBaseReadString(Section, Entry, "", Fd);
            if(Line.isEmpty()) {
                break;
            }
            ui->listWidgetOnVar->addItem(Line);
        }
    }
}

void StartStopCCPCalWidget::on_pushButtonStart_clicked()
{
    int i;
    int Fd = ScQt_GetMainFileDescriptor();

    if (!ui->listWidgetOnVar->count()) {
        ThrowError (1, "no Variable selected");
        return;
    } else {
        // First count the needd size
        int NeededSize = 0;
        for (i = 0; i < ui->listWidgetOnVar->count(); i++) {
            NeededSize = strlen(QStringToConstChar(ui->listWidgetOnVar->item(i)->text())) + 1;
        }
        char *Buffer = (char*)my_malloc(NeededSize);
        char **Pointers = (char**)my_malloc(i * sizeof(char*));
        if ((Buffer == nullptr) || (Pointers == nullptr)) {
            return;
        }
        // Now copy the labels
        char *p = Buffer;
        for (i = 0; i < ui->listWidgetOnVar->count(); i++) {
            StringCopyMaxCharTruncate(p , QStringToConstChar(ui->listWidgetOnVar->item(i)->text()), NeededSize - (p - Buffer));
            Pointers[i] = p;
            p = p + strlen(p) + 1;
        }
        Start_CCP (curConnection, START_CALIBRATION, i, Pointers);
        my_free(Buffer);
        my_free(Pointers);

        QString Section = QString("CCP Configuration for Target %1").arg(curConnection);
        for (i = 0; ; i++) {
            QString Entry = QString("cal lastselcted_%1").arg(i);
            if (i < ui->listWidgetOnVar->count()) {
                QString Label = ui->listWidgetOnVar->item(i)->text();
                ScQt_IniFileDataBaseWriteString (Section, Entry, Label, Fd);
            } else {
                if (ScQt_IniFileDataBaseReadString (Section, Entry, "", Fd).isEmpty()) {
                    break;
                }
                ScQt_IniFileDataBaseWriteString (Section, Entry, nullptr, Fd);
            }
        }
        CloseDialog();
    }
}

void StartStopCCPCalWidget::on_pushButtonStop_clicked()
{
    Stop_CCP (curConnection, STOP_CALIBRATION);
    CloseDialog();
}

bool StartStopCCPCalWidget::InOnList(QString &Name)
{
    for (int i = 0; i < ui->listWidgetOnVar->count(); i++) {
        if (!ui->listWidgetOnVar->item(i)->text().compare(Name)) return true;
    }
    return false;
}

void StartStopCCPCalWidget::CloseDialog()
{
    QWidget *Parent = parentWidget();
    QString Name = Parent->objectName();
    Parent = Parent->parentWidget();
    Name = Parent->objectName();
    Parent = Parent->parentWidget();
    Name = Parent->objectName();
    QDialog *Dialog = static_cast<QDialog*>(Parent);
    Dialog->accept();
}

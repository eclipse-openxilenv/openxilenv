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


#include "ImExportControlLampsDialog.h"
#include "ui_ImExportControlLampsDialog.h"

#include "Platform.h"
#include "QtIniFile.h"
#include "FileDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "FileExtensions.h"
#include "ThrowError.h"
}

ImExportControlLampsDialog::ImExportControlLampsDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::ImExportControlLampsDialog)
{
    ui->setupUi(this);

    EnableDisableButtons(false);

    ui->PreviewGraphicsView->updateBackgroundColor(Qt::black);
    ui->PreviewGraphicsView->updateForegroundColor(Qt::red);

    m_InternalModel = new ControlLampModel(this, false, -1);
    m_InternalModel->hasChanged(true);
    m_ExternalModel = nullptr;
    m_Fd = -1;
    ui->InternalListView->setModel(m_InternalModel);
}

ImExportControlLampsDialog::~ImExportControlLampsDialog()
{
    delete ui;
    if (m_InternalModel != nullptr) delete m_InternalModel;
    if (m_ExternalModel != nullptr) delete m_ExternalModel;
}

void ImExportControlLampsDialog::accept()
{
    if (m_InternalModel != nullptr) {
        if (m_InternalModel->hasChanged(false)) {
            m_InternalModel->writeToIni(-1);
        }
    }
    if (m_Filename.isEmpty()) {
        Dialog::accept();
    } else {
        if (m_ExternalModel != nullptr) {
            if ((m_ExternalModel->hasChanged(false)) &&
                (ThrowError (QUESTION_OKCANCEL, "you have chaged control lamp configuration in \"%s\" would you save the changes",
                             QStringToConstChar(m_Filename)) == IDOK)) {
                m_ExternalModel->writeToIni(m_Fd);
                if (ScQt_IniFileDataBaseSave(m_Fd, m_Filename, 2)) {
                    ThrowError (1, "cannot write to file %s", QStringToConstChar(m_Filename));
                } else {
                    Dialog::accept();
                }
            } else {
                ScQt_IniFileDataBaseClose(m_Fd);
                m_Fd = -1;
                Dialog::accept();
            }
        }
    }
}

void ImExportControlLampsDialog::reject()
{
    if (m_Fd > 0) ScQt_IniFileDataBaseClose(m_Fd);
    m_Fd = -1;
    Dialog::reject();
}

void ImExportControlLampsDialog::on_ChoicePushButton_clicked()
{
    QString loc_Filename = FileDialog::getOpenFileName(this,  QString("Import export control lamps"), QString(), QString(INI_EXT));
    if (!loc_Filename.isEmpty()) {
        m_Filename = loc_Filename;
        m_Fd = ScQt_IniFileDataBaseOpen(m_Filename);
        if (m_Fd > 0) {
            m_ExternalModel = new ControlLampModel(this, false, m_Fd);
            m_ExternalModel->hasChanged(true);
            ui->ExternalListView->setModel(m_ExternalModel);
            ui->FileNameLineEdit->setText(m_Filename);
            EnableDisableButtons(true);
        } else {
            m_Filename.clear();
        }
    }
}


QString ImExportControlLampsDialog::AvoidDuplicatedNames(ControlLampModel *arg_model, QString &arg_name)
{
    if (arg_model->hasStancil(arg_name)) {
        QString loc_newName;
        for (int x = 1; ; x++) {
            loc_newName = arg_name;
            loc_newName.append(QString("_%1").arg(x));
            if (!arg_model->hasStancil(loc_newName)) break;
        }
        return loc_newName;
    }
    return arg_name;
}

void ImExportControlLampsDialog::on_ImportMovePushButton_clicked()
{
    QModelIndexList Selected = ui->ExternalListView->selectionModel()->selectedIndexes();
    if (Selected.size() >= 1) {
        std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
        foreach (QModelIndex Index, Selected) {
            if ((m_InternalModel != nullptr) && (m_InternalModel != nullptr)) {
                Lamp *loc_lamp = m_ExternalModel->getStancilInformation(Index.row());
                // If there are alread one with this name choose a new name
                loc_lamp->m_name = AvoidDuplicatedNames (m_InternalModel, loc_lamp->m_name);
                m_InternalModel->addLamp(loc_lamp);
                m_ExternalModel->removeStancil(Index);
            }
        }
    }
}

void ImExportControlLampsDialog::on_ExportMovePushButton_clicked()
{
    QModelIndexList Selected = ui->InternalListView->selectionModel()->selectedIndexes();
    if (Selected.size() >= 1) {
        std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
        foreach (QModelIndex Index, Selected) {
            if ((m_InternalModel != nullptr) && (m_InternalModel != nullptr)) {
                Lamp *loc_lamp = m_InternalModel->getStancilInformation(Index.row());
                // If there are alread one with this name choose a new name
                loc_lamp->m_name = AvoidDuplicatedNames (m_ExternalModel, loc_lamp->m_name);
                m_ExternalModel->addLamp(loc_lamp);
                m_InternalModel->removeStancil(Index);
            }
        }
    }
}

void ImExportControlLampsDialog::on_ImportCopyPushButton_clicked()
{
    QModelIndexList Selected = ui->ExternalListView->selectionModel()->selectedIndexes();
    if (Selected.size() >= 1) {
        std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
        foreach (QModelIndex Index, Selected) {
            if ((m_InternalModel != nullptr) && (m_InternalModel != nullptr)) {
                Lamp *loc_lamp = m_ExternalModel->getStancilInformation(Index.row());
                Lamp *loc_newLamp = new Lamp;
                QString loc_text = loc_lamp->m_name;
                loc_text.append(",");
                loc_text.append(loc_lamp->m_polygon);
                loc_newLamp->init(loc_text);
                // If there are alread one with this name choose a new name
                loc_newLamp->m_name = AvoidDuplicatedNames (m_InternalModel, loc_newLamp->m_name);
                m_InternalModel->addLamp(loc_newLamp);
            }
        }
    }
}

void ImExportControlLampsDialog::on_ExportCopyPushButton_clicked()
{
    QModelIndexList Selected = ui->InternalListView->selectionModel()->selectedIndexes();
    if (Selected.size() >= 1) {
        std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
        foreach (QModelIndex Index, Selected) {
            if ((m_InternalModel != nullptr) && (m_InternalModel != nullptr)) {
                Lamp *loc_lamp = m_InternalModel->getStancilInformation(Index.row());
                Lamp *loc_newLamp = new Lamp;
                QString loc_text = loc_lamp->m_name;
                loc_text.append(",");
                loc_text.append(loc_lamp->m_polygon);
                loc_newLamp->init(loc_text);
                // If there are alread one with this name choose a new name
                loc_newLamp->m_name = AvoidDuplicatedNames (m_ExternalModel, loc_newLamp->m_name);
                m_ExternalModel->addLamp(loc_newLamp);
            }
        }
    }
}

void ImExportControlLampsDialog::on_DeletePushButton_clicked()
{
    QModelIndexList Selected = ui->InternalListView->selectionModel()->selectedIndexes();
    if (Selected.size() >= 1) {
        std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
        foreach (QModelIndex Index, Selected) {
            if ((m_InternalModel != nullptr) && (m_InternalModel != nullptr)) {
                m_InternalModel->removeStancil(Index);
            }
        }
    }
}


void ImExportControlLampsDialog::on_ExternalListView_clicked(const QModelIndex &index)
{
    if (m_ExternalModel != nullptr) {
        ui->PreviewGraphicsView->setGraphicsItemGroup(m_ExternalModel->getForegroundStancil(index), m_ExternalModel->getBackgroundStancil(index));
    }
}

void ImExportControlLampsDialog::on_InternalListView_clicked(const QModelIndex &index)
{
    if (m_InternalModel != nullptr) {
        ui->PreviewGraphicsView->setGraphicsItemGroup(m_InternalModel->getForegroundStancil(index), m_InternalModel->getBackgroundStancil(index));
    }
}

void ImExportControlLampsDialog::EnableDisableButtons(bool par_Enable)
{
    ui->ExportCopyPushButton->setEnabled(par_Enable);
    ui->ImportCopyPushButton->setEnabled(par_Enable);
    ui->ExportMovePushButton->setEnabled(par_Enable);
    ui->ImportMovePushButton->setEnabled(par_Enable);
}

void ImExportControlLampsDialog::on_NewPushButton_clicked()
{
    QString loc_Filename = FileDialog::getSaveFileName(this,  QString("Export control lamps"), QString(), QString(INI_EXT));
    if (!loc_Filename.isEmpty()) {
        int Fd = ScQt_IniFileDataBaseCreateAndOpenNewIniFile(loc_Filename);
        if (Fd <= 0) {
            ThrowError (1, "cannot write to file \"%s\"", QStringToConstChar(loc_Filename));
            m_Filename.clear();
        } else {
            m_ExternalModel = new ControlLampModel(this, false, Fd);
            m_ExternalModel->hasChanged(true);
            ui->ExternalListView->setModel(m_ExternalModel);
            ui->FileNameLineEdit->setText(m_Filename);
            EnableDisableButtons(true);
            ScQt_IniFileDataBaseSave(Fd, m_Filename, 2);
        }
    }
}

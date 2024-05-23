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


#include "DefaultElementDialog.h"
#include "ui_DefaultElementDialog.h"
#include "QtIniFile.h"
#include "WindowNameAlreadyInUse.h"
#include "StringHelpers.h"

extern "C" {
#include "ThrowError.h"
}


DefaultElementDialog::DefaultElementDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::DefaultElementDialog)
{
    ui->setupUi(this);
    m_settingFrame = nullptr;
    ui->splitter->setBackgroundRole(QPalette::Mid); //QPalette::Dark);
}

DefaultElementDialog::~DefaultElementDialog() {
    delete ui;
}

void DefaultElementDialog::addSettings(CustomDialogFrame* arg_settingFrame)
{
    if(arg_settingFrame != nullptr) {
        arg_settingFrame->setParent(this);
        ui->splitter->insertWidget(1, arg_settingFrame);
        m_settingFrame = arg_settingFrame;
    }
}

void DefaultElementDialog::setMultiSelect(bool arg_multiSelect)
{
    ui->defaultFrame->setMultiSelect(arg_multiSelect);
}

void DefaultElementDialog::setElementWindowName(QString arg_windowTitle)
{
    m_defaultWindowName = arg_windowTitle;
    ui->WindowNameLineEdit->setText(arg_windowTitle);
}

void DefaultElementDialog::setCurrentVisibleVariable(QString arg_variable)
{
    ui->defaultFrame->setCurrentVisibleVariable(arg_variable);
}

void DefaultElementDialog::setCurrentVisibleVariables(QStringList arg_variables)
{
    ui->defaultFrame->setCurrentVisibleVariables(arg_variables);
}

void DefaultElementDialog::setCurrentColor(QColor arg_color)
{
    ui->defaultFrame->setCurrentColor(arg_color);
}

void DefaultElementDialog::setCurrentFont(QFont arg_font)
{
    ui->defaultFrame->setCurrentFont(arg_font);
}

void DefaultElementDialog::setVaraibleList(DefaultDialogFrame::VARIABLE_TYPE arg_type)
{
    ui->defaultFrame->setVaraibleList(arg_type);
}

DefaultDialogFrame*DefaultElementDialog::getDefaultFrame()
{
    return ui->defaultFrame;
}

void DefaultElementDialog::removeSettingFrame()
{
    if (m_settingFrame != nullptr) {
        delete m_settingFrame;
        m_settingFrame = nullptr;
    }
}

void DefaultElementDialog::accept()
{
    QString NewWindowTitle = ui->WindowNameLineEdit->text();
    if (m_defaultWindowName.compare(NewWindowTitle) != 0) {
        if (NewWindowTitle.length() >= 1) {
            if(!WindowNameAlreadyInUse(NewWindowTitle)) {
                if (IsAValidWindowSectionName(NewWindowTitle)) {
                    emit windowNameChanged(NewWindowTitle);
                } else {
                    ThrowError (1, "Inside window name only ascii characters and no []\\ are allowed \"%s\"", QStringToConstChar(NewWindowTitle));
                    return;
                }
            } else {
                ThrowError (1, "window name \"%s\" already in use", QStringToConstChar(NewWindowTitle));
                return;
            }
        } else {
            ThrowError (1, "window name must have one or more character \"%s\"", QStringToConstChar(NewWindowTitle));
            return;
        }
    }
    if (m_settingFrame != nullptr) {
        m_settingFrame->userAccept();
    }
    removeSettingFrame();
    emit accepted();
    done(QDialog::Accepted);
}

void DefaultElementDialog::reject()
{
    if (m_settingFrame != nullptr) {
        m_settingFrame->userReject();
    }
    ui->defaultFrame->userReject();
    removeSettingFrame();
    emit rejected();
    done(QDialog::Rejected);
}

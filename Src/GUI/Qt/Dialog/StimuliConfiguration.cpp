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


#include <QApplication>
#include <QClipboard>
#include "StimuliConfiguration.h"
#include "ui_StimuliConfiguration.h"

#include "FileDialog.h"
#include "BlackboardVariableModel.h"
#include "MainWindow.h"
#include "StringHelpers.h"

#include "Platform.h"

extern "C" {
#include "Blackboard.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StimulusReadFile.h"
#include "StimulusReadMdfFile.h"
}

StimuliConfiguration::StimuliConfiguration(QString &par_ConfigurationFileName, QWidget *parent) : Dialog(parent),
    ui(new Ui::StimuliConfiguration)
{
    ui->setupUi(this);

    ui->TriggerEventComboBox->addItem(QString(">"));

    m_SortModelComboBox = nullptr;

    ui->ConfigurationFileNameLineEdit->setText (par_ConfigurationFileName);

    if (!par_ConfigurationFileName.isEmpty() &&
        (access (QStringToConstChar(par_ConfigurationFileName), 0) == 0)) {
        m_OrgCfgFileName = par_ConfigurationFileName;
        QString CfgFileContent = FileToString(par_ConfigurationFileName);
        if (StringToDialog (CfgFileContent) == 0) {
            m_OrgCfgFileContent = DialogToString();
        } else {
            SetDefaultValues();
        }
    } else {
        // Set the default values
        SetDefaultValues();
    }
}

StimuliConfiguration::~StimuliConfiguration()
{
    delete ui;
    if (m_SortModelComboBox != nullptr) delete m_SortModelComboBox;
}

bool StimuliConfiguration::ReadStimuliVariablesFromHeader (QString par_StimuliFileName)
{
    ui->AllVariableListWidget->clear();
    ui->ToPlayVariableListWidget->clear();
    if (QFile(par_StimuliFileName).exists()) {
        char *VariableList;
        if (IsMdfFormat(QStringToConstChar(par_StimuliFileName), nullptr)) {
            VariableList = MdfReadStimulHeaderVariabeles (QStringToConstChar(par_StimuliFileName));
        } else {
            VariableList = ReadStimulHeaderVariabeles (QStringToConstChar(par_StimuliFileName));
        }
        if (VariableList != nullptr) {
            char *p, *pp;
            bool NotTheEnd = true;
            for (pp = p = VariableList; NotTheEnd; p++) {
                if ((*p == ';') || (*p == 0)) {
                    if (*p == 0) NotTheEnd = false;
                    else *p = 0;
                    ui->AllVariableListWidget->addItem (QString (pp));
                    pp = p + 1;
                }
            }
            my_free (VariableList);
            return true;
        }
    }
    return false;
}

bool StimuliConfiguration::GetCfgEntry (QTextStream &par_Stream, const QString &par_Token, QString *ret_Target,
                                        int nRwndFlag)
  {
      int TokenLen = par_Token.size();

      if (nRwndFlag != 0) {
          par_Stream.seek(0);
      }

      while ((par_Stream.status() == QTextStream::Ok) && (!par_Stream.atEnd())) {
          QString Line;
          Line = par_Stream.readLine().trimmed();
          // Ignore empty lines and comment lines
          if ((Line.isEmpty()) || (Line.at(0) == ';')) continue;

          if (Line.startsWith(par_Token)) {
              if (ret_Target != nullptr) {
                  Line.remove(0, TokenLen);
                  int CommentPos = Line.indexOf(';', 0);
                  if (CommentPos >= 0) Line.truncate(CommentPos);
                  *ret_Target = Line.trimmed();
              }
              return true;
          }
      }
      // nothing found
      return false;
}

void StimuliConfiguration::SetDefaultValues()
{
    ui->PlayFileNameLineEdit->setText (QString(""));
    ui->TriggerEventComboBox->setCurrentText (QString ("@trigger not active"));
    ui->TriggerEventComboBox->setCurrentText (QString (">"));
    ui->TriggerValueLineEdit->setText (QString ("0"));

    ui->ToPlayVariableListWidget->clear();
}

QString StimuliConfiguration::GetConfigurationFileName()
{
    return ui->ConfigurationFileNameLineEdit->text();
}

void StimuliConfiguration::done (int r)
{
    if (r == QDialog::Accepted) {
        QString ConfigureFileName = ui->ConfigurationFileNameLineEdit->text();
        ConfigureFileName = ConfigureFileName.trimmed();

        QString Content = DialogToString();
        if (!Content.isEmpty()) {
            StringToFile(ConfigureFileName, Content);
            QDialog::done (r);
        }
    } else {
        QDialog::done(r);
    }
}

void StimuliConfiguration::on_ConfigurationFileNamePushButton_clicked()
{
    QString ConfigurationFile = ui->ConfigurationFileNameLineEdit->text();
    ConfigurationFile = FileDialog::getOpenNewOrExistingFileName (this, QString("Select file"),
                                                      ConfigurationFile,
                                                      QString(PLAYER_EXT));
    if (!ConfigurationFile.isEmpty()) {
        ui->ConfigurationFileNameLineEdit->setText (ConfigurationFile);
        LoadNewCfgFile(ConfigurationFile);
    }
}

void StimuliConfiguration::on_PlayFileNamePushButton_clicked()
{
    QString StimuliFile = ui->PlayFileNameLineEdit->text();
    StimuliFile = FileDialog::getOpenNewOrExistingFileName (this, QString("Select file"),
                                                StimuliFile,
                                                QString(MEASUREMENT_EXT));
    if (!StimuliFile.isEmpty()) {
        ui->PlayFileNameLineEdit->setText (StimuliFile);
        ReadStimuliVariablesFromHeader (StimuliFile);
    }
}

void StimuliConfiguration::on_PlayFileNameLineEdit_editingFinished()
{
    QString StimuliFileName = ui->PlayFileNameLineEdit->text();
    ReadStimuliVariablesFromHeader (StimuliFileName);
}

void StimuliConfiguration::on_AddVariablePushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->AllVariableListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString Name = Item->data(Qt::DisplayRole).toString();
        if (ui->ToPlayVariableListWidget->findItems(Name, Qt::MatchFixedString).isEmpty()) {
            ui->ToPlayVariableListWidget->addItem (Name);
            delete Item;
        }
    }
}

void StimuliConfiguration::on_DeleteVariablePushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->ToPlayVariableListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString Name = Item->data(Qt::DisplayRole).toString();
        if (ui->AllVariableListWidget->findItems(Name, Qt::MatchFixedString).isEmpty()) {
            ui->AllVariableListWidget->addItem (Name);
            delete Item;
        } else {
            delete Item;
        }
    }
}


void StimuliConfiguration::EnableDisableTrigger(bool par_Enable, bool par_Init)
{
    if (par_Enable) {
        if (m_SortModelComboBox == nullptr) {
            // Start speedup Combobox
            static_cast<QListView*>(ui->TriggerVariableComboBox->view())->setUniformItemSizes(true);
            static_cast<QListView*>(ui->TriggerVariableComboBox->view())->setLayoutMode(QListView::Batched);
            // End speedup Combobox

            m_SortModelComboBox = new QSortFilterProxyModel;
            m_SortModelComboBox->setSortCaseSensitivity(Qt::CaseInsensitive);
            m_SortModelComboBox->sort(0);

            BlackboardVariableModel *Model = MainWindow::GetBlackboardVariableModel();

            m_SortModelComboBox->setSourceModel(Model);
            ui->TriggerVariableComboBox->setModel(m_SortModelComboBox);
        }
        ui->TriggerVariableComboBox->setEnabled(true);
        ui->TriggerEventComboBox->setEnabled(true);
        ui->TriggerValueLineEdit->setEnabled(true);
        if (par_Init) ui->TriggerActiveCheckBox->setChecked(true);
    } else {
        ui->TriggerVariableComboBox->setDisabled(true);
        ui->TriggerEventComboBox->setDisabled(true);
        ui->TriggerValueLineEdit->setDisabled(true);
        if (par_Init) ui->TriggerActiveCheckBox->setChecked(false);
    }
}

int StimuliConfiguration::LoadNewCfgFile(QString &par_NewCfgFile)
{
    if (par_NewCfgFile.compare(m_OrgCfgFileName)) {
        if (access (QStringToConstChar(par_NewCfgFile), 0) == 0) {
            QString DlgCfgFileContent;
            DlgCfgFileContent = DialogToString();
            if (!m_OrgCfgFileContent.isEmpty()) {
                if (m_OrgCfgFileContent.compare(DlgCfgFileContent) != 0) {
                    QString NewCfgFileContent;
                    switch (ThrowError(QUESTION_SAVE_IGNORE_CANCLE, "you have made changes would you like to save this to \"%s\" before selecting a new config file",
                              QStringToConstChar(m_OrgCfgFileName))) {
                    case IDSAVE:
                        {
                            StringToFile(m_OrgCfgFileName, DlgCfgFileContent);
                            NewCfgFileContent = FileToString(par_NewCfgFile);
                            if (!NewCfgFileContent.isEmpty()) {
                                StringToDialog(NewCfgFileContent);
                                m_OrgCfgFileContent = DialogToString();
                            } else {
                                m_OrgCfgFileContent = DlgCfgFileContent;
                            }
                        }
                        break;
                    case IDIGNORE:
                        {
                            NewCfgFileContent = FileToString(par_NewCfgFile);
                            if (!NewCfgFileContent.isEmpty()) {
                                if (ThrowError(QUESTION_OKCANCEL, "would you load the content from \"%s\"",
                                          QStringToConstChar(par_NewCfgFile)) == IDOK) {
                                    StringToDialog(NewCfgFileContent);
                                    m_OrgCfgFileContent = NewCfgFileContent;
                                }
                            }
                        }
                        break;
                    case IDCANCEL:
                        // reset filename
                        ui->ConfigurationFileNameLineEdit->setText (m_OrgCfgFileName);
                        return 0;
                    }
                } else {
                    // There was no changes inside the dialog
                    QString NewCfgFileContent;
                    NewCfgFileContent = FileToString(par_NewCfgFile);
                    if (!NewCfgFileContent.isEmpty()) {
                        StringToDialog(NewCfgFileContent);
                        m_OrgCfgFileContent = DialogToString();
                    }
                }
                m_OrgCfgFileName = par_NewCfgFile;
            }
        }
    }
    return 1;
}

void StimuliConfiguration::on_TriggerActiveCheckBox_clicked(bool checked)
{
    EnableDisableTrigger(checked);
}

QString StimuliConfiguration::FileToString(QString par_FileName)
{
    QString Ret;
    QFile File (par_FileName);
    if (File.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream In(&File);
        Ret = In.readAll();
        File.close();
    }
    return Ret;
}

bool StimuliConfiguration::StringToFile(QString par_FileName, QString &par_String)
{
    QFile File(par_FileName);
    if (File.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream Out(&File);
        Out << par_String;
        File.close();
        return true;
    }
    return false;
}

int StimuliConfiguration::StringToDialog(QString &par_CfgFileContent)
{
    QString Text;
    QTextStream Stream(&par_CfgFileContent);
    // Is it a  stimulus config file
    if (!GetCfgEntry (Stream, HDPLAY_CONF_FILE, nullptr, 0)) {
        return -1;
    }

    // File name of the recorder file
    if (!GetCfgEntry (Stream, HDPLAY_FILE, &Text, 1)) {
        return -1;
    }
    ui->PlayFileNameLineEdit->setText (Text);

    ReadStimuliVariablesFromHeader (Text);

    // Trigger
    if (GetCfgEntry (Stream, HDPLAY_TRIGGER, &Text, 1)) {
        Text = Text.simplified();
        QStringList List = Text.split(' ');
        if (List.size() != 3) {
            return -1;
        }
        EnableDisableTrigger(true, true);
        ui->TriggerVariableComboBox->setCurrentText (List.at(0));
        ui->TriggerEventComboBox->setCurrentText (List.at(1));
        ui->TriggerValueLineEdit->setText (List.at(2));
    } else {
        EnableDisableTrigger(false, true);
    }

    // Start of the variable list
    if (!GetCfgEntry (Stream, HDPLAY_STARTVAR, nullptr, 1)) {
        return -1;
    }
    // All variables
    while (GetCfgEntry (Stream, HDPLAY_VAR, &Text, 0)) {
        QList<QListWidgetItem *> Items = ui->AllVariableListWidget->findItems (Text, Qt::MatchExactly);
        if (Items.size() >= 1) {
            QListWidgetItem *Item = Items.at(0);
            if (Item != nullptr) {
                QString Variable = Item->data(Qt::DisplayRole).toString();
                if (!Variable.isEmpty()) {
                    ui->ToPlayVariableListWidget->addItem (Variable);
                }
                delete Item;
            }
        }
    }
    return 0;
}

QString StimuliConfiguration::DialogToString()
{
    QString Ret;
    QTextStream Stream(&Ret);
    int nSelVar;

    Stream << "; Configuration file for stimuli player\n";
    Stream << "   " << HDPLAY_CONF_FILE << "\n\n";

    Stream << "; Filename of stimuli file (*.dat)\n";
    QString PlayerFileName = ui->PlayFileNameLineEdit->text();
    PlayerFileName = PlayerFileName.trimmed();
    if (PlayerFileName.isEmpty()) {
        ThrowError (1, "you have to define a stimuli file name");
        return Ret;
    }
    Stream << "   " << HDPLAY_FILE << " " << PlayerFileName;

    if (ui->TriggerActiveCheckBox->isChecked()) {
        QString TriggerVariable = ui->TriggerVariableComboBox->currentText();
        TriggerVariable = TriggerVariable.trimmed();
        Stream << "; Startevent\n";
        QString TriggerEvent = ui->TriggerEventComboBox->currentText();
        QString TriggerValue = ui->TriggerValueLineEdit->text();
        bool Ok;
        TriggerValue.toDouble (&Ok);
        if (!Ok) {
            ThrowError (1, "\"%s\" is not a valid trigger value", QStringToConstChar(TriggerValue));
            return QString();
        }
        QString TriggerLine (TriggerVariable);
        TriggerLine.append (" ");
        TriggerLine.append (TriggerEvent);
        TriggerLine.append (" ");
        TriggerLine.append (TriggerValue);
        Stream << "   " << HDPLAY_TRIGGER << " " << TriggerLine << "\n\n";
    }
    Stream << "; which variables should be played\n";
    Stream << "   " << HDPLAY_STARTVAR "\n";
    nSelVar = ui->ToPlayVariableListWidget->count();
    for (int i = 0; i < nSelVar; i++) {
        QListWidgetItem *Item = ui->ToPlayVariableListWidget->item (i);
        if (Item != nullptr) {
            QString Variable = Item->data(Qt::DisplayRole).toString();
            Stream << "      " << HDPLAY_VAR " " << Variable << "\n";
        }
    }
    Stream << "   " << HDPLAY_ENDVAR << "\n\n";

    return Ret;
}

void StimuliConfiguration::on_CopyToClipboardPushButton_clicked()
{
    QApplication::clipboard()->setText(DialogToString());
}

void StimuliConfiguration::on_ConfigurationFileNameLineEdit_editingFinished()
{
    QString ConfigurationFile = ui->ConfigurationFileNameLineEdit->text();
    LoadNewCfgFile(ConfigurationFile);
}

void StimuliConfiguration::on_SelectAllAvailableVariablesPushButton_clicked()
{
    ui->AllVariableListWidget->selectAll();
}

void StimuliConfiguration::on_SelectAllPlayVariablePushButton_clicked()
{
    ui->ToPlayVariableListWidget->selectAll();
}

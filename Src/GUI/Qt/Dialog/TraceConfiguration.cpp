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
#include "TraceConfiguration.h"
#include "ui_TraceConfiguration.h"
#include "MainWindow.h"

#include "FileDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "Blackboard.h"
#include "ThrowError.h"
}

TraceConfiguration::TraceConfiguration(QString &par_ConfigurationFileName, QWidget *parent) : Dialog(parent),
    ui(new Ui::TraceConfiguration)
{
    ui->setupUi(this);

    connect(ui->A2LCheckBox, SIGNAL(clicked(bool)), ui->A2LFileNameLineEdit, SLOT(setEnabled(bool)));
    connect(ui->A2LCheckBox, SIGNAL(clicked(bool)), ui->A2LFileNamePushButton, SLOT(setEnabled(bool)));

    ui->A2LFileNameLineEdit->setDisabled(true);
    ui->A2LFileNamePushButton->setDisabled(true);

    ui->TriggerEventComboBox->addItem(QString(">"));

    m_Model = MainWindow::GetBlackboardVariableModel();
    m_ModelCopy = m_Model->MakeASnapshotCopy();
    m_SortModelListView = new QSortFilterProxyModel;
    m_SortModelListView->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_SortModelListView->sort(0);
    m_SortModelListView->setSourceModel(m_ModelCopy);

    connect(ui->NotRecFilterLineEdit, SIGNAL(textChanged(QString)), m_SortModelListView, SLOT(setFilterWildcard(QString)));

    ui->AllVariableListView->setModel(m_SortModelListView);

    connect(ui->AllVariableListView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(handleSelectionChanged(QItemSelection,QItemSelection)));

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
        // set the default value
        SetDefaultValues();
    }
}


TraceConfiguration::~TraceConfiguration()
{
    delete ui;
    if (m_SortModelComboBox != nullptr) delete m_SortModelComboBox;
    delete m_SortModelListView;
    delete m_ModelCopy;
}


bool TraceConfiguration::GetCfgEntry (QTextStream &par_Stream, const QString &par_Token, QString *ret_Target,
                                      int nRwndFlag)
{
    int TokenLen = par_Token.size();

    if (nRwndFlag != 0) {
        par_Stream.seek(0);
    }

    while ((par_Stream.status() == QTextStream::Ok) && (!par_Stream.atEnd())) {
        QString Line;
        Line = par_Stream.readLine().trimmed();
        // Ignore empty lines ore comment lines
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
    // not found
    return false;
}

void TraceConfiguration::EnableDisableTrigger(bool par_Enable, bool par_Init)
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
            m_SortModelComboBox->setSourceModel(m_Model);
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

int TraceConfiguration::LoadNewCfgFile(QString &par_NewCfgFile)
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
                        // reset the file name
                        ui->ConfigurationFileNameLineEdit->setText (m_OrgCfgFileName);
                        return 0;
                    }
                } else {
                    // There where no changes inside the dialog
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


int TraceConfiguration::StringToDialog (QString &par_CfgFileContent)
{
    QString Text;
    QTextStream Stream(&par_CfgFileContent);

    // Is it a recorder configuration file
    if (!GetCfgEntry (Stream, HDREC_CONF_FILE, nullptr, 0)) {
        return -1;
    }

    // File name of he  recording file name
    if (!GetCfgEntry (Stream, HDREC_FILE, &Text, 1)) {
        return -1;
    }
    ui->RecordingFileNameLineEdit->setText (Text);

    // Which format() should be written
    bool DatFlag = GetCfgEntry (Stream, HDREC_TEXT_FORMAT, nullptr, 1);
    bool MdfFlag = GetCfgEntry (Stream, HDREC_MDF_FORMAT, nullptr, 1);

    // If there is nothing defined use DAT
    if (!MdfFlag) {
        DatFlag = true;
    }
    ui->FileFormatDatCheckBox->setChecked (DatFlag);
    ui->FileFormatMdfCheckBox->setChecked (MdfFlag);

    if (GetCfgEntry (Stream, HDREC_DESCRIPTION_FILE, &Text, 1)) {
        ui->A2LFileNameLineEdit->setText (Text);
    }
    if (GetCfgEntry (Stream, HDREC_AUTOGEN_DESCRIPTION_FILE, nullptr, 1)) {
        ui->A2LCheckBox->setChecked (true);
    } else {
        ui->A2LCheckBox->setChecked (false);
    }

    // Trigger
    if (GetCfgEntry (Stream, HDREC_TRIGGER, &Text, 1)) {
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

    // Physical
    if (GetCfgEntry (Stream, HDREC_PHYSICAL, nullptr, 1)) {
        ui->PhysicalCheckBox->setChecked (true);
    } else {
        ui->PhysicalCheckBox->setChecked (false);
    }
    // Start of the variable liste
    if (!GetCfgEntry (Stream, HDREC_STARTVAR, nullptr, 1)) {
        return -1;
    }
    ui->ToRecordVariableListWidget->clear();
    delete m_ModelCopy;
    m_ModelCopy = m_Model->MakeASnapshotCopy();
    m_SortModelListView->setSourceModel(m_ModelCopy);
    // All variable must be also available inside the blackboard
    while (GetCfgEntry (Stream, HDREC_VAR, &Text, 0)) {
        if (m_ModelCopy->DeleteVariableByName (Text)) {
            ui->ToRecordVariableListWidget->addItem (Text);
        }
    }

    return 0;
}

int TraceConfiguration::SaveRecorderCFGfromDlg (const char *par_ConfigurationFileName)
{
    QString Content = DialogToString();
    if (!Content.isEmpty()) {
        QFile CfgFile(par_ConfigurationFileName);
        if (!CfgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ThrowError (1, "cannot write configuration file \"%s\"", par_ConfigurationFileName);
            return -1;
        }
        LogFileAccess(par_ConfigurationFileName);
        QTextStream Out(&CfgFile);
        Out << Content;
        CfgFile.close();
        return 0;
    }
    return -1;
}


void TraceConfiguration::on_AddVariablePushButton_clicked()
{
    QModelIndexList SelectedIndexes = ui->AllVariableListView->selectionModel()->selectedIndexes();
    int LastSelPos = SelectedIndexes.last().row();
    std::sort(SelectedIndexes.begin(),SelectedIndexes.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
    foreach (QModelIndex Index, SelectedIndexes) {
        if (Index.row() < LastSelPos) LastSelPos--;
        QString Variable = Index.data(Qt::DisplayRole).toString();
        if (m_ModelCopy->DeleteVariableByName (Variable)) {
            ui->ToRecordVariableListWidget->addItem (Variable);
        }
    }
    // After deleting select a variable again
    const QAbstractItemModel *Model = SelectedIndexes.first().model();
    if (LastSelPos >= (Model->rowCount()-2)) LastSelPos = Model->rowCount() - 3;
    if (LastSelPos < 0) LastSelPos = 0;
    QModelIndex Index = Model->index(LastSelPos, 0);
    while ((LastSelPos > 0) && !Index.isValid()) {
        LastSelPos--;
    }
    ui->AllVariableListView->scrollTo(Index);
}

void TraceConfiguration::on_DeleteVariablePushButton_clicked()
{
    QList<QListWidgetItem*> SelectedItems = ui->ToRecordVariableListWidget->selectedItems();
    foreach (QListWidgetItem* Item, SelectedItems) {
        QString Name = Item->data(Qt::DisplayRole).toString();
        m_ModelCopy->AddVariableByName(Name);
        delete Item;
    }
}

void TraceConfiguration::on_SelectAllAvailableVariablesPushButton_clicked()
{
    ui->AllVariableListView->selectAll();
}

void TraceConfiguration::on_SelectAllRecordVariablePushButton_clicked()
{
    ui->ToRecordVariableListWidget->selectAll();
}

void TraceConfiguration::on_ConfigurationFileNamePushButton_clicked()
{
    QString ConfigurationFile = ui->ConfigurationFileNameLineEdit->text();
    ConfigurationFile = FileDialog::getOpenNewOrExistingFileName (this, QString ("Select file"),
                                                                         ConfigurationFile,
                                                                         QString (RECORDER_EXT));
    if (!ConfigurationFile.isEmpty()) {
        ui->ConfigurationFileNameLineEdit->setText (ConfigurationFile);
        LoadNewCfgFile(ConfigurationFile);
    }
}

void TraceConfiguration::on_RecordingFileNamePushButton_clicked()
{
    QString RecordingFile = ui->RecordingFileNameLineEdit->text();
    RecordingFile = FileDialog::getOpenNewOrExistingFileName (this, QString ("Select file"),
                                                  RecordingFile,
                                                  QString ("Trace recording files (*.DAT *.MDF *.ASCII *.REC)"));
    if (!RecordingFile.isEmpty()) {
        ui->RecordingFileNameLineEdit->setText (RecordingFile);
    }
}

void TraceConfiguration::on_A2LFileNamePushButton_clicked()
{
    QString A2LFile = ui->A2LFileNameLineEdit->text();
    A2LFile = FileDialog::getOpenNewOrExistingFileName (this, QString("Select file"),
                                                  A2LFile,
                                                  QString("Description files (*.A2L)"));
    if (!A2LFile.isEmpty()) {
        ui->A2LFileNameLineEdit->setText (A2LFile);
    }
}

void TraceConfiguration::SetDefaultValues()
{
    ui->A2LFileNameLineEdit->setText (QString(""));
    ui->RecordingFileNameLineEdit->setText (QString(""));
    ui->A2LCheckBox->setChecked (false);
    ui->FileFormatDatCheckBox->setChecked (true);
    ui->FileFormatMdfCheckBox->setChecked (false);
    ui->TriggerEventComboBox->setCurrentText (QString ("@trigger not active"));
    ui->TriggerEventComboBox->setCurrentText (QString (">"));
    ui->TriggerValueLineEdit->setText (QString ("0"));
    ui->PhysicalCheckBox->setChecked (false);

    ui->ToRecordVariableListWidget->clear();
}

QString TraceConfiguration::GetConfigurationFileName()
{
    return ui->ConfigurationFileNameLineEdit->text();
}

void TraceConfiguration::done (int r)
{
    if (r == QDialog::Accepted) {
        QString ConfigureFileName = ui->ConfigurationFileNameLineEdit->text();
        ConfigureFileName = ConfigureFileName.trimmed();

        if (SaveRecorderCFGfromDlg (QStringToConstChar(ConfigureFileName)) == 0) {
            QDialog::done (r);
        }
    } else {
        QDialog::done(r);
    }
}


void TraceConfiguration::on_TriggerActiveCheckBox_clicked(bool checked)
{
    EnableDisableTrigger(checked);
}

QString TraceConfiguration::DialogToString()
{
    QString Ret;
    QTextStream Stream(&Ret);
    int FormatSecCounter = 0;

    Stream << "; Config-File for HD-Recorder\n";
    Stream << "   " << HDREC_CONF_FILE << "\n\n";

    Stream << "; Filename of Tracefile (*.dat (text), *.rec MCS)\n";
    QString RecordingFileName = ui->RecordingFileNameLineEdit->text();
    RecordingFileName = RecordingFileName.trimmed();
    if (RecordingFileName.isEmpty()) {
        ThrowError (1, "you have to define a recoring file name");
        return QString();
    }
    Stream << "   " << HDREC_FILE << " " << RecordingFileName << "\n\n";
    Stream << "; Samplerate in cycles\n";
    Stream << "   " << HDREC_SAMPLERATE << " = 1\n\n";

    Stream << "; Samplelength in cycles\n";
    Stream << "   " << HDREC_SAMPLELEN " = " << ULONG_MAX << "\n\n";

    if (ui->FileFormatMdfCheckBox->isChecked()) {
        FormatSecCounter++;
        Stream << "; Generate MDF files\n";
        Stream << "   MDF_FORMAT\n";
    }
    if (ui->FileFormatDatCheckBox->isChecked()) {
        FormatSecCounter++;
        Stream << "; Generate text files\n";
        Stream << "   TEXT_FORMAT\n";
    }
    Stream << "\n";

    if (ui->A2LCheckBox->isChecked()) {
        Stream << "   AUTOGEN_DESCRIPTION_FILE\n";
        QString A2LFile = ui->A2LFileNameLineEdit->text();
        A2LFile = A2LFile.trimmed();
        if (A2LFile.isEmpty()) {
            ThrowError (1, "you have to define a description file name");
            return QString();
        }
        Stream << "   DESCRIPTION_FILE " << A2LFile << " \n";
        Stream << "\n";
    }

    if (ui->TriggerActiveCheckBox->isChecked()) {
        QString TriggerVariable = ui->TriggerVariableComboBox->currentText();
        TriggerVariable = TriggerVariable.trimmed();
        if (!TriggerVariable.isEmpty()) {
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
            Stream << "   " << HDREC_TRIGGER << " " << TriggerLine;
        }
    }
    if (ui->PhysicalCheckBox->isChecked()) {
        Stream << "; Record variables physical if conversion is available\n";
        Stream << "   PHYSICAL\n";
    }

    Stream << "; which Variables should be recorded\n";
    Stream << "   " << HDREC_STARTVAR << "\n";
    int nSelVar = ui->ToRecordVariableListWidget->count();
    for (int i = 0; i < nSelVar; i++) {
        QListWidgetItem *Item = ui->ToRecordVariableListWidget->item (i);
        if (Item != nullptr) {
            QString Variable = Item->data(Qt::DisplayRole).toString();
            Stream << "      " << HDREC_VAR << " " << Variable << "\n";
        }
    }
    Stream << "   " << HDREC_ENDVAR "\n\n";

    return Ret;
}

QString TraceConfiguration::FileToString(QString par_FileName)
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

bool TraceConfiguration::StringToFile(QString par_FileName, QString &par_String)
{
    QFile File(par_FileName);
    if (File.open(QIODevice::WriteOnly)) {
        QTextStream Out(&File);
        Out << par_String;
        File.close();
        return true;
    }
    return false;
}

void TraceConfiguration::on_CopyToClipboardPushButton_clicked()
{
    QApplication::clipboard()->setText(DialogToString());
}

void TraceConfiguration::on_ConfigurationFileNameLineEdit_editingFinished()
{
    QString ConfigurationFile = ui->ConfigurationFileNameLineEdit->text();
    LoadNewCfgFile(ConfigurationFile);
}


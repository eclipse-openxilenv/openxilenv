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


#ifndef TRACECONFIGURATION_H
#define TRACECONFIGURATION_H

#include <QDialog>
#include <QCloseEvent>
#include <QSortFilterProxyModel>
#include <QTextStream>
#include "Dialog.h"

#include "BlackboardVariableModel.h"


#define HDREC_CONF_FILE     "HDRECORDER_CONFIG_FILE"
#define HDREC_FILE          "HDRECORDER_FILE"
#define HDREC_SAMPLERATE    "HDRECORDER_SAMPLERATE"
#define HDREC_SAMPLELEN     "HDRECORDER_SAMPLELENGTH"
#define HDREC_MDF_FORMAT    "MDF_FORMAT"
#define HDREC_TEXT_FORMAT   "TEXT_FORMAT"
#define HDREC_TRIGGER       "TRIGGER"
#define HDREC_STARTVAR      "STARTVARLIST"
#define HDREC_ENDVAR        "ENDVARLIST"
#define HDREC_VAR           "VARIABLE"
#define HDREC_DESCRIPTION_FILE    "DESCRIPTION_FILE"
#define HDREC_AUTOGEN_DESCRIPTION_FILE    "AUTOGEN_DESCRIPTION_FILE"
#define HDREC_PHYSICAL      "PHYSICAL"

#define DEFAULT_RECCFGFILE  "TMP_REC.CFG"


namespace Ui {
class TraceConfiguration;
}

class TraceConfiguration : public Dialog
{
    Q_OBJECT

public:
    explicit TraceConfiguration(QString &par_ConfigurationFileName, QWidget *parent = nullptr);
    ~TraceConfiguration();

    QString GetConfigurationFileName ();

public slots:
    void done (int r);

private slots:
    void on_AddVariablePushButton_clicked();

    void on_DeleteVariablePushButton_clicked();

    void on_SelectAllAvailableVariablesPushButton_clicked();

    void on_SelectAllRecordVariablePushButton_clicked();

    void on_ConfigurationFileNamePushButton_clicked();

    void on_RecordingFileNamePushButton_clicked();

    void on_A2LFileNamePushButton_clicked();

    void on_TriggerActiveCheckBox_clicked(bool checked);

    void on_CopyToClipboardPushButton_clicked();

    void on_ConfigurationFileNameLineEdit_editingFinished();

private:
    QString DialogToString();
    QString FileToString(QString par_FileName);
    bool StringToFile(QString par_FileName, QString &par_String);
    void SetDefaultValues();
    int StringToDialog (QString &par_CfgFileContent);
    int SaveRecorderCFGfromDlg (const char *par_ConfigurationFileName);
    bool GetCfgEntry (QTextStream &par_Stream, const QString &par_Token, QString *ret_Target, int nRwndFlag);
    void EnableDisableTrigger(bool par_Enable, bool par_Init = false);
    int LoadNewCfgFile(QString &par_NewCfgFile);
    Ui::TraceConfiguration *ui;

    BlackboardVariableModel *m_Model;
    BlackboardVariableModel *m_ModelCopy;
    QSortFilterProxyModel *m_SortModelListView;
    QSortFilterProxyModel *m_SortModelComboBox;

    QString m_OrgCfgFileName;
    QString m_OrgCfgFileContent;
};

#endif // TRACECONFIGURATION_H

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


#ifndef STIMULICONFIGURATION_H
#define STIMULICONFIGURATION_H

#include <QDialog>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QTextStream>
#include "Dialog.h"


#define HDPLAY_CONF_FILE     "HDPLAYER_CONFIG_FILE"
#define HDPLAY_FILE          "HDPLAYER_FILE"
#define HDPLAY_TRIGGER       "TRIGGER"
#define HDPLAY_STARTVAR      "STARTVARLIST"
#define HDPLAY_ENDVAR        "ENDVARLIST"
#define HDPLAY_VAR           "VARIABLE"


#define DEFAULT_PLAYCFGFILE  "TMP_PLAY.CFG"

namespace Ui {
class StimuliConfiguration;
}

class StimuliConfiguration : public Dialog
{
    Q_OBJECT

public:
    explicit StimuliConfiguration(QString &par_ConfigurationFileName, QWidget *parent = nullptr);
    ~StimuliConfiguration();

    QString GetConfigurationFileName ();

public slots:
    void done (int r);

private slots:
    void on_ConfigurationFileNamePushButton_clicked();

    void on_PlayFileNamePushButton_clicked();

    void on_PlayFileNameLineEdit_editingFinished();

    void on_AddVariablePushButton_clicked();

    void on_DeleteVariablePushButton_clicked();

    void on_TriggerActiveCheckBox_clicked(bool checked);

    void on_CopyToClipboardPushButton_clicked();

    void on_ConfigurationFileNameLineEdit_editingFinished();

    void on_SelectAllAvailableVariablesPushButton_clicked();

    void on_SelectAllPlayVariablePushButton_clicked();

private:
    QString DialogToString();
    QString FileToString(QString par_FileName);
    bool StringToFile(QString par_FileName, QString &par_String);
    int StringToDialog (QString &par_CfgFileContent);
    void SetDefaultValues();
    bool ReadStimuliVariablesFromHeader(QString par_StimuliFileName);
    bool GetCfgEntry (QTextStream &par_Stream, const QString &par_Token, QString *ret_Target,int nRwndFlag);

    void EnableDisableTrigger(bool par_Enable, bool par_Init = false);

    int LoadNewCfgFile(QString &par_NewCfgFile);

    QStringListModel *m_Model;
    QSortFilterProxyModel *m_SortModelListView;

    QSortFilterProxyModel *m_SortModelComboBox;

    Ui::StimuliConfiguration *ui;

    QString m_OrgCfgFileName;
    QString m_OrgCfgFileContent;
};

#endif // STIMULICONFIGURATION_H

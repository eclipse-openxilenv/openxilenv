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


#ifndef SCRIPTERRORDIALOG_H
#define SCRIPTERRORDIALOG_H

#include <QDialog>
#include <QAbstractItemModel>

namespace Ui {
class ScriptErrorDialog;
}

typedef struct SCRIPT_ERROR_LINE_STRUCT {
    int Level;
    int LineNr;
    char *Filename;
    char *Message;
} SCRIPT_ERROR_LINE;

class ScriptErrorModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ScriptErrorModel(QObject *parent = nullptr);
    ~ScriptErrorModel() Q_DECL_OVERRIDE;

    int AddNewScriptErrorMessage (int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message);

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void Clear();
    int GetLinNr (int par_Row);
    char *GetFilename (int par_Row);

private:
    SCRIPT_ERROR_LINE *m_ScriptErrorMessageLines;
    int m_ScriptErrorMessageRowAllocated;
    int m_ScriptErrorMessageRowCount;

    QPixmap *m_ErrorPixmap;
    QPixmap *m_InfoPixmap;
    QPixmap *m_WarningPixmap;
    QPixmap *m_UnknownPixmap;
};


class ScriptErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptErrorDialog(QWidget *parent = nullptr);
    ~ScriptErrorDialog();

    void AddNewScriptErrorMessage(int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message);
    bool IsVisable();
    void MakeItVisable();

    static void ScriptErrorMsgDlgAddMsgFromOtherThread (int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message);
    static void ScriptErrorMsgDlgAddMsgGuiThread (int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message, QWidget *Parent = nullptr);
    static void ScriptErrorMsgDlgReset (void);

    /*
private slots:
    void AddNewScriptErrorMessageSlot (int par_Level, int par_LineNr, char *par_Filename, char *par_Message);
*/

private slots:
    void on_ErrorTreeView_doubleClicked(const QModelIndex &index);

private:
    void closeEvent(QCloseEvent *event);

    void OpenFileAtLineNr (char *par_Filename, int par_LineNr);

    Ui::ScriptErrorDialog *ui;

    bool m_IsVisable;
    bool m_TerminationFlag;   // ist true wenn XilEnv runter faehr und kein Error-Message-Fenster mehr aufgehen soll

    ScriptErrorModel *m_Model;
};

#endif // SCRIPTERRORDIALOG_H

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


#ifndef EXTERRORDIALOG_H
#define EXTERRORDIALOG_H

#include <QDialog>
#include <QAbstractItemModel>
#include <QVariant>
#include <QIcon>

namespace Ui {
class ExtErrorDialog;
}


typedef struct EXT_ERROR_LINE_STRUCT {
    int Level;
    uint64_t Cycle;
    //char *ProcessName;
    int Pid;
    char *Message;
} EXT_ERROR_LINE;


class ExtErrorModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ExtErrorModel(QObject *parent = nullptr);
    ~ExtErrorModel() Q_DECL_OVERRIDE;

    int AddNewExtErrorMessage (int Level, uint64_t Cycle, int Pid, /*char *ProcessName,*/ const char *Message);

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void Clear();
    void Delete (int par_Row);

private:
    EXT_ERROR_LINE *m_ExtErrorMessageLines;
    int m_ExtErrorMessageRowAllocated;
    int m_ExtErrorMessageRowCount;

    QPixmap *m_ErrorPixmap;
    QPixmap *m_InfoPixmap;
    QPixmap *m_WarningPixmap;
    QPixmap *m_RealtimePixmap;
    QPixmap *m_UnknownPixmap;
};



class ExtendedErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExtendedErrorDialog(QWidget *parent = nullptr);
    ~ExtendedErrorDialog();
    void AddMessage (int Level, uint64_t Cycle, int Pid, const char *Message);
    bool IsVisable();
    void MakeItVisable();

private slots:
    void on_DeletePushButton_clicked();

    void on_DeleteAllPushButton_clicked();

    void on_ClosePushButton_clicked();

private:
    void closeEvent(QCloseEvent *event);

    Ui::ExtErrorDialog *ui;

    bool m_IsVisable;
    bool m_TerminationFlag;   // is true if termintion is in process and no error message window should be poppup

    ExtErrorModel *m_Model;
};


int AddAddNoneModalErrorMessageWithCycle (int Level, unsigned long Cycle, char *ProcessName, char *Message);


#endif // EXTERRORDIALOG_H

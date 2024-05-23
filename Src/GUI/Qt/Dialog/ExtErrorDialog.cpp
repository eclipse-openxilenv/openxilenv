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


#include "ExtErrorDialog.h"
#include "ui_ExtErrorDialog.h"
#include "QtIniFile.h"

#include <QCloseEvent>
#include <QTextStream>

extern "C" {
#include "MyMemory.h"
#include "ThrowError.h"
#include "MainValues.h"
#include "Files.h"
#include "Scheduler.h"
}

ExtErrorModel::ExtErrorModel(QObject *parent)
   : QAbstractItemModel(parent)
{
    m_ExtErrorMessageRowCount = 0;
    m_ExtErrorMessageRowAllocated = 0;
    m_ExtErrorMessageLines = nullptr;

    m_ErrorPixmap = new QPixmap (":/Icons/ext_err_error.png");
    m_InfoPixmap = new QPixmap (":/Icons/ext_err_info.png");
    m_WarningPixmap = new QPixmap (":/Icons/ext_err_warning.png");
    m_RealtimePixmap = new QPixmap (":/Icons/ext_err_realtime.png");
    m_UnknownPixmap = new QPixmap (":/Icons/ext_err_unknown.png");
}

ExtErrorModel::~ExtErrorModel()
{
    Clear();
}


int ExtErrorModel::AddNewExtErrorMessage (int Level, uint64_t Cycle, int Pid, const char *Message)
{
    beginInsertRows(QModelIndex(), m_ExtErrorMessageRowCount, m_ExtErrorMessageRowCount);
    if (m_ExtErrorMessageRowCount >= m_ExtErrorMessageRowAllocated) {
        m_ExtErrorMessageRowAllocated += 20 + m_ExtErrorMessageRowAllocated / 8;
        m_ExtErrorMessageLines = static_cast<EXT_ERROR_LINE*>(my_realloc (m_ExtErrorMessageLines, static_cast<size_t>(m_ExtErrorMessageRowAllocated) * sizeof (EXT_ERROR_LINE)));
        if (m_ExtErrorMessageLines == nullptr) {
            ThrowError (1, "out of memory");
        }
    }
    m_ExtErrorMessageLines[m_ExtErrorMessageRowCount].Level = Level;
    m_ExtErrorMessageLines[m_ExtErrorMessageRowCount].Cycle = Cycle;
    m_ExtErrorMessageLines[m_ExtErrorMessageRowCount].Pid = Pid;
    size_t Len = strlen (Message) + 1;
    m_ExtErrorMessageLines[m_ExtErrorMessageRowCount].Message = static_cast<char*>(my_malloc (Len));

    // Replace all white spaces with empty spaces
    char *d = m_ExtErrorMessageLines[m_ExtErrorMessageRowCount].Message;
    const char *s = Message;
    while (*s != 0) {
        if (isascii(*s) && isspace(*s)) *d = ' ';
        else *d = *s;
        d++;
        s++;
    }
    *d = 0;

    endInsertRows ();

    int Ret = m_ExtErrorMessageRowCount;
    m_ExtErrorMessageRowCount++;
    return Ret;
}


QVariant ExtErrorModel::data(const QModelIndex &index, int role) const
{
    int Row = index.row();
    int Column = index.column();
    if (role == Qt::DisplayRole) {
        if (Row < m_ExtErrorMessageRowCount) {
            if (Column == 0) {
                return QVariant();
            } else if (Column == 1) {
                return QString().number(m_ExtErrorMessageLines[Row].Cycle);
            } else if (Column == 2) {
                char ProcessName[MAX_PATH];
                if (GetProcessShortName (m_ExtErrorMessageLines[Row].Pid, ProcessName)) {
                    return QString().number(m_ExtErrorMessageLines[Row].Pid);
                } else {
                    return QString(ProcessName);
                }
            } else if (Column == 3) {
                return QString(m_ExtErrorMessageLines[Row].Message);
            } else {
                return QVariant();
            }
        }
    } else if (role == Qt::DecorationRole) {
        if (Column == 0) {
            switch (m_ExtErrorMessageLines[Row].Level) {
            case RT_ERROR:
                return QVariant(*m_RealtimePixmap);
            case INFO_NO_STOP:
            case INFO_NOT_WRITEN_TO_MSG_FILE:
                return QVariant(*m_InfoPixmap);
            case WARNING_NO_STOP:
                return QVariant(*m_WarningPixmap);
            case ERROR_NO_STOP:
                return QVariant(*m_ErrorPixmap);
            default:
                return QVariant(*m_UnknownPixmap);
            }
        }
    } else if (role == Qt::SizeHintRole) {
        return m_RealtimePixmap->size();
    }

    return QVariant();
}

QVariant ExtErrorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) {
            return QString ("Level");
        } else if (section == 1) {
            return QString ("Cycle");
        } else if (section == 2) {
            return QString ("Process");
        } else if (section == 3) {
            return QString ("Error message ");
        } else {
            return QVariant();
        }
    }
    return QVariant();
}

QModelIndex ExtErrorModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex (row, column);
}

QModelIndex ExtErrorModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int ExtErrorModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_ExtErrorMessageRowCount;
}

int ExtErrorModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 4;
}

void ExtErrorModel::Clear()
{
    for (int x = 0; x < m_ExtErrorMessageRowCount; x++) {
        my_free (m_ExtErrorMessageLines[x].Message);
    }
    m_ExtErrorMessageRowCount = 0;
    m_ExtErrorMessageRowAllocated = 0;
    if (m_ExtErrorMessageLines != nullptr) my_free (m_ExtErrorMessageLines);
    m_ExtErrorMessageLines = nullptr;
}

void ExtErrorModel::Delete(int par_Row)
{
    if ((par_Row < m_ExtErrorMessageRowCount) && (par_Row >= 0)) {
        my_free (m_ExtErrorMessageLines[par_Row].Message);
        for (int x = par_Row; x < (m_ExtErrorMessageRowCount-1); x++) {
            m_ExtErrorMessageLines[x].Message = m_ExtErrorMessageLines[x+1].Message;
        }
        m_ExtErrorMessageRowCount--;
    }
}


ExtendedErrorDialog::ExtendedErrorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExtErrorDialog)
{
    setAttribute (Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    char Text[INI_MAX_LINE_LENGTH];
    QString Line = ScQt_IniFileDataBaseReadString ("BasicSettings", "ErrorWindowSizeAndPos", "", ScQt_GetMainFileDescriptor());
    QStringList List = Line.split((","));
    if (List.size() == 4) {
#ifdef _WIN32
        int XPos = List.at(0).toInt();
        int YPos = List.at(1).toInt();
        int XSize = List.at(2).toInt();
        int YSize = List.at(3).toInt();
        HWND HwndDesktop = GetDesktopWindow ();
        RECT RcClient;
        if (GetClientRect (HwndDesktop, &RcClient)) {
            if (XPos > (RcClient.right - RcClient.left - 200))
                XPos = (RcClient.right - RcClient.left - 200);
            if (YPos > (RcClient.bottom - RcClient.top - 200))
                YPos  = (RcClient.bottom - RcClient.top - 200);

            this->move (XPos, YPos);
            this->resize (XSize, YSize);
        }
#else
#endif
    }
    m_Model = new ExtErrorModel (this);
    ui->ErrorTreeView->setModel(m_Model);

    m_IsVisable = true;
    m_TerminationFlag = false;
}

ExtendedErrorDialog::~ExtendedErrorDialog()
{
    delete ui;
}

void ExtendedErrorDialog::AddMessage(int Level, uint64_t Cycle, int Pid, const char *Message)
{
    m_Model->AddNewExtErrorMessage(Level, Cycle, Pid, Message);
}

bool ExtendedErrorDialog::IsVisable()
{
    return m_IsVisable;
}

void ExtendedErrorDialog::MakeItVisable()
{
    if (!m_IsVisable) {
        this->show();
    }
}

void ExtendedErrorDialog::on_DeletePushButton_clicked()
{
    QModelIndexList Selected = ui->ErrorTreeView->selectionModel()->selectedIndexes();

    std::sort(Selected.begin(),Selected.end(),[](const QModelIndex& a, const QModelIndex& b)->bool{return a.row()>b.row();}); // sort from bottom to top
    for (int i = 0; i < Selected.size(); ++i) {
        QModelIndex ModelIndex = Selected.at(i);
        int Column = ModelIndex.column();
        if (Column == 0) {
            int Row = ModelIndex.row();
            printf ("%i", Row);
            m_Model->Delete (Row);
        }
    }
    ui->ErrorTreeView->reset();
}

void ExtendedErrorDialog::on_DeleteAllPushButton_clicked()
{
    ui->ErrorTreeView->reset();
    m_Model->Clear();
}

void ExtendedErrorDialog::on_ClosePushButton_clicked()
{
    ui->ErrorTreeView->reset();
    m_Model->Clear();
    m_IsVisable = false;
    this->setVisible(false);
}

void ExtendedErrorDialog::closeEvent(QCloseEvent *event)
{
    ui->ErrorTreeView->reset();
    m_Model->Clear();
    m_IsVisable = false;
    this->setVisible(false);
    QString Line;
    QTextStream Stream(&Line);
    Stream << this->x() << ", "
           << this->y() << ", "
           << this->width() << ", "
           << this->height();
    ScQt_IniFileDataBaseWriteString ("BasicSettings", "ErrorWindowSizeAndPos", Line, ScQt_GetMainFileDescriptor());

    event->ignore();
}

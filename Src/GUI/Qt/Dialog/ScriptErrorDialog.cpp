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


#include "ScriptErrorDialog.h"
#include "ui_ScriptErrorDialog.h"
#include "MainWinowSyncWithOtherThreads.h"

#include <QCloseEvent>

#include "Platform.h"   // _access

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "MainValues.h"
#include "IniDataBase.h"
#include "Files.h"
#include "ScriptMessageFile.h"
}

#define UNUSED(x) (void)(x)

ScriptErrorModel::ScriptErrorModel(QObject *parent)
   : QAbstractItemModel(parent)
{
    m_ScriptErrorMessageRowCount = 0;
    m_ScriptErrorMessageRowAllocated = 0;
    m_ScriptErrorMessageLines = nullptr;

    m_ErrorPixmap = new QPixmap (":/Icons/ext_err_error.png");
    m_InfoPixmap = new QPixmap (":/Icons/ext_err_info.png");
    m_WarningPixmap = new QPixmap (":/Icons/ext_err_warning.png");
    m_UnknownPixmap = new QPixmap (":/Icons/ext_err_unknown.png");
}

ScriptErrorModel::~ScriptErrorModel()
{
    // Speicher frei geben
    Clear();
    delete m_ErrorPixmap;
    delete m_InfoPixmap;
    delete m_WarningPixmap;
    delete m_UnknownPixmap;

}

int ScriptErrorModel::AddNewScriptErrorMessage(int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message)
{
    if (m_ScriptErrorMessageRowCount >= m_ScriptErrorMessageRowAllocated) {
        m_ScriptErrorMessageRowAllocated += 20 + m_ScriptErrorMessageRowAllocated / 8;
        m_ScriptErrorMessageLines = static_cast<SCRIPT_ERROR_LINE*>(my_realloc (m_ScriptErrorMessageLines, static_cast<size_t>(m_ScriptErrorMessageRowAllocated) * sizeof (SCRIPT_ERROR_LINE)));
        if (m_ScriptErrorMessageLines == nullptr) {
            ThrowError (1, "out of memory");
        }
    }
    m_ScriptErrorMessageLines[m_ScriptErrorMessageRowCount].Level = par_Level;
    m_ScriptErrorMessageLines[m_ScriptErrorMessageRowCount].LineNr = par_LineNr;
    size_t Len = strlen (par_Filename) + 1;
    m_ScriptErrorMessageLines[m_ScriptErrorMessageRowCount].Filename = static_cast<char*>(my_malloc (Len));
    MEMCPY (m_ScriptErrorMessageLines[m_ScriptErrorMessageRowCount].Filename, par_Filename, Len);
    Len = strlen (par_Message) + 1;
    m_ScriptErrorMessageLines[m_ScriptErrorMessageRowCount].Message = static_cast<char*>(my_malloc (Len));
    MEMCPY (m_ScriptErrorMessageLines[m_ScriptErrorMessageRowCount].Message, par_Message, Len);

    int Ret = m_ScriptErrorMessageRowCount;
    m_ScriptErrorMessageRowCount++;
    return Ret;
}

QVariant ScriptErrorModel::data(const QModelIndex &index, int role) const
{
    int Row = index.row();
    int Column = index.column();
    if (role == Qt::DisplayRole) {
        if (Row < m_ScriptErrorMessageRowCount) {
            if (Column == 0) {
                return QVariant();
            } else if (Column == 1) {
                return QString (m_ScriptErrorMessageLines[Row].Filename).append(QString("(%1)").arg(m_ScriptErrorMessageLines[Row].LineNr));
            } else if (Column == 2) {
                return QString(m_ScriptErrorMessageLines[Row].Message);
            } else {
                return QVariant();
            }
        }
    } else if (role == Qt::DecorationRole) {
        if (Column == 0) {
            switch (m_ScriptErrorMessageLines[Row].Level) {
            case 0:
                return QVariant(*m_InfoPixmap);
            case 1:
                return QVariant(*m_WarningPixmap);
            case 2:
                return QVariant(*m_ErrorPixmap);
            default:
                return QVariant(*m_UnknownPixmap);
            }
        }
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignTop | Qt::AlignLeft);
    }
    return QVariant();
}

QVariant ScriptErrorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) {
            return QString ("Level");
        } else if (section == 1) {
            return QString ("Location");
        } else if (section == 2) {
            return QString ("Error message");
        } else {
            return QVariant();
        }
    }
    return QVariant();
}

QModelIndex ScriptErrorModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex (row, column);
}

QModelIndex ScriptErrorModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int ScriptErrorModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_ScriptErrorMessageRowCount;
}

int ScriptErrorModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 3;
}

void ScriptErrorModel::Clear()
{
    for (int x = 0; x < m_ScriptErrorMessageRowCount; x++) {
        my_free (m_ScriptErrorMessageLines[x].Filename);
        my_free (m_ScriptErrorMessageLines[x].Message);
    }
    m_ScriptErrorMessageRowCount = 0;
    m_ScriptErrorMessageRowAllocated = 0;
    if (m_ScriptErrorMessageLines != nullptr) my_free (m_ScriptErrorMessageLines);
    m_ScriptErrorMessageLines = nullptr;
}

int ScriptErrorModel::GetLinNr(int par_Row)
{
    return (m_ScriptErrorMessageLines[par_Row].LineNr);
}

char *ScriptErrorModel::GetFilename(int par_Row)
{
    return (m_ScriptErrorMessageLines[par_Row].Filename);
}


ScriptErrorDialog::ScriptErrorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScriptErrorDialog)
{
    setAttribute (Qt::WA_DeleteOnClose);
    ui->setupUi(this);

#ifdef _WIN32
    int XPos = IniFileDataBaseReadInt ("BasicSettings", "ScriptErrorMsgDlgXPosition", 0, GetMainFileDescriptor());
    int YPos = IniFileDataBaseReadInt ("BasicSettings", "ScriptErrorMsgDlgYPosition", 0, GetMainFileDescriptor());
    HWND HwndDesktop = GetDesktopWindow ();
    RECT RcClient;
    if (GetClientRect (HwndDesktop, &RcClient)) {
        if (XPos > (RcClient.right - RcClient.left - 200))
            XPos = (RcClient.right - RcClient.left - 200);
        if (YPos > (RcClient.bottom - RcClient.top - 200))
            YPos  = (RcClient.bottom - RcClient.top - 200);

        this->move (XPos, YPos);
    }
#else
#endif
    ui->ErrorTreeView->setIndentation(0);
    ui->ErrorTreeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->ErrorTreeView->header()->setStretchLastSection(false);
    m_Model = new ScriptErrorModel (this);
    ui->ErrorTreeView->setModel(m_Model);

    m_IsVisable = true;
    m_TerminationFlag = false;
}

ScriptErrorDialog::~ScriptErrorDialog()
{
    delete ui;
    delete m_Model;
}

void ScriptErrorDialog::AddNewScriptErrorMessage(int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message)
{
    if ((par_Filename == nullptr) ||
        (par_Message == nullptr)) {
        m_Model->Clear();
    } else {
        MakeItVisable();
        m_Model->AddNewScriptErrorMessage(par_Level, par_LineNr, par_Filename, par_Message);
    }
    ui->ErrorTreeView->reset();
}

bool ScriptErrorDialog::IsVisable()
{
    return m_IsVisable;
}

void ScriptErrorDialog::MakeItVisable()
{
    if (!m_IsVisable) {
        this->show();
    }
}

void ScriptErrorDialog::ScriptErrorMsgDlgAddMsgFromOtherThread(int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message)
{
    AddNewScriptErrorMessageFromOtherThread (par_Level, par_LineNr, par_Filename, par_Message);
}

static ScriptErrorDialog *Dialog;

void ScriptErrorDialog::ScriptErrorMsgDlgAddMsgGuiThread(int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message, QWidget *Parent)
{
    if (GetError2MessageFlag()) {
        if (par_Message != nullptr) {
            char *MessageBuffer = (char*)my_malloc(strlen(par_Message) + 32);
            sprintf (MessageBuffer, "Scipt error: %s", par_Message);
            AddScriptMessageOnlyMessageFile(MessageBuffer);
            my_free(MessageBuffer);
        }
    }
    if (Dialog == nullptr) {
        if (par_Filename != nullptr) {
            Dialog = new ScriptErrorDialog (Parent);
            Dialog->show();
        }
    }
    if (Dialog != nullptr) {
        Dialog->AddNewScriptErrorMessage (par_Level, par_LineNr, par_Filename, par_Message);
    }
}

void ScriptErrorDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    IniFileDataBaseWriteInt ("BasicSettings", "ScriptErrorMsgDlgXPosition", this->x(), GetMainFileDescriptor());
    IniFileDataBaseWriteInt ("BasicSettings", "ScriptErrorMsgDlgYPosition", this->y(), GetMainFileDescriptor());
    Dialog = nullptr;
}

void ScriptErrorDialog::OpenFileAtLineNr(char *par_Filename, int par_LineNr)
{
#ifdef _WIN32
    int Ret;
    char CommandLine[2048];
    STARTUPINFO         sStartInfo;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION sProcInfo;

    char loc_Filename[MAX_PATH];
    if (expand_filename (par_Filename, loc_Filename, sizeof (loc_Filename)) != 0) {
        strcpy (loc_Filename, par_Filename);
    }

    char *p = CommandLine;
    if (IniFileDataBaseReadString ("BasicSettings", "PathToScriptEditor", "",
                                  CommandLine, sizeof (CommandLine), GetMainFileDescriptor()) > 0) {
        while (*p != 0) p++;
        if ((p == CommandLine) || (*(p-1) != '\\') && (*(p-1) != '/')) {
#ifdef _WIN32
            *p++ = '\\';
#else
            *p++ = '/';
#endif
        }
    }

    sprintf (p, "code.exe -g %s:%i", loc_Filename, par_LineNr);

    memset (&sStartInfo, 0, sizeof (sStartInfo));
    sStartInfo.cb            = sizeof(STARTUPINFO);
    sStartInfo.dwFlags       = STARTF_USESHOWWINDOW;
    sStartInfo.wShowWindow   = SW_SHOWDEFAULT;
    sStartInfo.lpReserved    = nullptr;
    sStartInfo.lpDesktop     = nullptr;
    sStartInfo.lpTitle       = nullptr;
    sStartInfo.cbReserved2   = 0;
    sStartInfo.lpReserved2   = nullptr;
    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle       = TRUE;

    Ret = CreateProcess (nullptr,
                         CommandLine,
                         nullptr,
                         nullptr,
                         FALSE,
                         CREATE_NEW_CONSOLE,
                         nullptr,
                         nullptr,
                         &sStartInfo,
                         &sProcInfo);

    if (!Ret) {
        ThrowError (1, "cannot start visual code \"%s\"", CommandLine);
    } else {
        CloseHandle(sProcInfo.hThread);
        CloseHandle(sProcInfo.hProcess);
    }
#else
    UNUSED(par_Filename);
    UNUSED(par_LineNr);
    ThrowError (1, "not supported %s(%i)", __FILE__, __LINE__);
#endif

}

void ScriptErrorDialog::on_ErrorTreeView_doubleClicked(const QModelIndex &index)
{
    int Row = index.row();

    OpenFileAtLineNr(m_Model->GetFilename(Row), m_Model->GetLinNr(Row));
}

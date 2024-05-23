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


#include <QDir>
#include <QFileDialog>

#include "FileDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "IniDataBase.h"
#include "MainValues.h"
}


static QString RemoveFileName (QString &Path)
{
    QString Ret = Path;

    // search the last '/' or '\'
    int Index1 = Ret.lastIndexOf('\\');
    int Index2 = Ret.lastIndexOf('/');
    if (Index2 > Index1) Index1 = Index2;
    if (Index1 > 0) {
        Ret.truncate(Index1);
    } else if (Index1 == 0) {
        Ret = '.';
    }
    return Ret;
}

QString FileDialog::getOpenFileName(QWidget *parent,
                                    const QString &caption,
                                    const QString &dir,
                                    const QString &filter)
{
    char StartDirString[MAX_PATH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    QString StartDir;

    if (dir.isEmpty() && !filter.isEmpty() && (filter.size() < (sizeof(Entry) - 32))) {
        sprintf (Entry, "Last Directory for %s", QStringToConstChar(filter));
        if (IniFileDataBaseReadString ("BasicSettings", Entry, "", StartDirString, sizeof(StartDirString), GetMainFileDescriptor()) <= 0) {
            strcpy (StartDirString, s_main_ini_val.WorkDir);
        }
        StartDir = QString(StartDirString);
    } else {
        StartDir = dir;
    }

    QString Ret = QFileDialog::getOpenFileName(parent,
                                               caption,
                                               StartDir,
                                               filter);
    if (!Ret.isEmpty()) {
        if (s_main_ini_val.UseRelativePaths) {
            char CurrentDirectory[MAX_PATH];
            GetCurrentDirectory (sizeof (CurrentDirectory), CurrentDirectory);
            QDir Dir(CurrentDirectory);
            Ret = Dir.relativeFilePath(Ret);
        }
        QString FileDir = RemoveFileName(Ret);
        sprintf (Entry, "Last Directory for %s", QStringToConstChar(filter));
        IniFileDataBaseWriteString ("BasicSettings", Entry, QStringToConstChar(FileDir), GetMainFileDescriptor());

    }
    return Ret;
}

QString FileDialog::getOpenNewOrExistingFileName(QWidget *parent,
                                                        const QString &caption,
                                                        const QString &dir,
                                                        const QString &filter,
                                                        bool *ret_Exists)
{
    char StartDirString[MAX_PATH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    QString Ret, StartDir;

    if (dir.isEmpty()) {
        sprintf (Entry, "Last Directory for %s", QStringToConstChar(filter));
        if (IniFileDataBaseReadString ("BasicSettings", Entry, "", StartDirString, sizeof(StartDirString), GetMainFileDescriptor()) <= 0) {
            strcpy (StartDirString, s_main_ini_val.WorkDir);
        }
        StartDir = QString(StartDirString);
    } else {
        StartDir = dir;
    }

    QFileDialog dialog(parent, caption);
    dialog.setNameFilter(filter);
    dialog.setDirectory(StartDir);

    if (dialog.exec()) {
        Ret = dialog.selectedFiles().at(0);
        if (!Ret.isEmpty()) {
            if (s_main_ini_val.UseRelativePaths) {
                char CurrentDirectory[MAX_PATH];
                GetCurrentDirectory (sizeof (CurrentDirectory), CurrentDirectory);
                QDir Dir(CurrentDirectory);
                Ret = Dir.relativeFilePath(Ret);
            }
            QString FileDir = RemoveFileName(Ret);
            sprintf (Entry, "Last Directory for %s", QStringToConstChar(filter));
            IniFileDataBaseWriteString ("BasicSettings", Entry, QStringToConstChar(FileDir), GetMainFileDescriptor());
            if (ret_Exists != nullptr) {
                QFile File(Ret);
                *ret_Exists = File.exists();
            }
        }
    }
    return Ret;
}


QString FileDialog::getSaveFileName(QWidget *parent,
                                           const QString &caption, const QString &dir,
                                           const QString &filter)
{
    char StartDirString[MAX_PATH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    QString StartDir;

    if (dir.isEmpty()) {
        sprintf (Entry, "Last Directory for %s", QStringToConstChar(filter));
        if (IniFileDataBaseReadString ("BasicSettings", Entry, "", StartDirString, sizeof(StartDirString), GetMainFileDescriptor()) <= 0) {
            strcpy (StartDirString, s_main_ini_val.WorkDir);
        }
        StartDir = QString(StartDirString);
    } else {
        StartDir = dir;
    }

    QString Ret = QFileDialog::getSaveFileName(parent,
                                               caption,
                                               StartDir,
                                               filter,
                                               nullptr);
    if (!Ret.isEmpty()) {
        if (s_main_ini_val.UseRelativePaths) {
            char CurrentDirectory[MAX_PATH];
            GetCurrentDirectory (sizeof (CurrentDirectory), CurrentDirectory);
            QDir Dir(CurrentDirectory);
            Ret = Dir.relativeFilePath(Ret);
        }
        QString FileDir = RemoveFileName(Ret);
        IniFileDataBaseWriteString ("BasicSettings", Entry, QStringToConstChar(FileDir), GetMainFileDescriptor());
    }
    return Ret;
}

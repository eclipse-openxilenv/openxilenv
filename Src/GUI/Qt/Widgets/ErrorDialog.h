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


#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#ifdef __cplusplus

#include <QDialog>
#include "ExtErrorDialog.h"
#include <stdint.h>

#define MAX_SIMULTANEOUSLY_ERROR_EVENTS  16

void SetSyncWithGuiThreadIsInitAndRunning(void);

class ErrorMessagesFromOtherThread : public QObject
{
    Q_OBJECT
public:
    int Error(int par_Level, uint64_t par_Cycle, const char *par_HeaderString, const char *par_ErrorString, int par_IsGuiMainThreadFlag);

signals:
    void ErrorMessagesFromOtherThreadSignal(int par_Index);

private:
};

class ErrorMessagesGuiThread : public QObject
{
    Q_OBJECT
public:
    ErrorMessagesGuiThread();
    void Init();
    void TerminateExtErrorMessageWindow (void);
    void AddNoneModalErrorMessage (int Level, uint64_t Cycle, int Pid, const char *Message);
    void AddErrorToStatusBar(int Level, const char *Header, const char *Message);

public slots:
    void ErrorMessagesFromOtherThreadSlot(int par_Index);

private:
    ExtendedErrorDialog *m_ExtErrorDialog;
    bool m_TerminatedFlag;  // ist true wenn XilEnv beendet wird -> kein ExtError-Message Fenter mehr aufmachen!
};

class SatatusbarEventFilter : public QObject
{
    Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

extern "C" {
#endif
int ErrorMessagesFromOtherThreadInst_Error(int par_Level, uint64_t par_Cycle, const char *par_HeaderString, const char *par_ErrorString, int par_IsGuiMainThreadFlag);
int IsMainThread(void);
void InitErrorDialog (void);
void TerminateExtErrorMessageWindow (void);
void TerminateErrorDialog (void);
void CheckErrorMessagesBeforeMainWindowIsOpen(void);

#ifdef __cplusplus
}
#endif

#endif // ERRORDIALOG_H

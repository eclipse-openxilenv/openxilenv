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


#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#ifdef __cplusplus

#include <QDialog>
#include "ControlPanel.h"
#include <Dialog.h>

namespace Ui {
class MessageWindow;
}

class MessageWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MessageWindow(QWidget *parent = nullptr);
    ~MessageWindow();
    void SetMaxHistoryLines (int par_MaxLines);

private:
    void WriteToIni();
    void closeEvent(QCloseEvent *event);

signals:
    void SetMessageWindowStateSignal(bool par_IsVisable);

public slots:
   void AddMessage (QString par_MessageString);

private slots:
    void on_SelectAllpushButton_clicked();

    void on_CopyPushButton_clicked();

    void on_ClosePushButton_clicked();
    void on_StayOnTopCheckBox_clicked(bool checked);

private:
    Ui::MessageWindow *ui;

    bool m_StayOnTop;
};


class MessagesFromOtherThread : public QObject
{
    Q_OBJECT
public:
    void AddMessages(QString par_Messsage);
    void AddMessagesChar(const char *par_Messsage);

signals:
    void AddMessagesFromOtherThreadSignal(QString par_Messsage);

private:
};


class MessagesToGuiThread : public QObject
{
    Q_OBJECT

private:
    MessageWindow *m_MessageWindow;
    QWidget *m_Parent;    // This must point to the MainWindow so the script message window can be closed
    ControlPanel *m_ControlPanel;  // This have to point to the ControlPanel so the checkbox inside the ControlPanel can be changed

public:
    void Init(ControlPanel *par_ControlPanel, QWidget *par_Parent);
    void Terminate();

public slots:
    void AddMessagesToGuiThreadSlot(QString par_Message);

private:
    int m_MessageCounter;
};

void AddMessages(QString par_Messsage);
void AddMessagesChar(const char *par_Messsage);
void InitMessageWindow(ControlPanel *par_ControlPanel, QWidget *parent = nullptr);
void CloseMessageWindow();

#else

void AddMessages(const char *par_Messsage);
void InitMessageWindowCriticalSections (void);

#endif // __cplusplus



#endif // MESSAGEWINDOW_H

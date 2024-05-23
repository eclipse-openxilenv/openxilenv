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


#ifndef ISALREADYRUNNUNG_H
#define ISALREADYRUNNUNG_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>

class IsAlreadyRunnung : public QWidget
{
    Q_OBJECT

public:
    explicit IsAlreadyRunnung(char *par_InstanceName, QWidget *parent = nullptr);
    ~IsAlreadyRunnung();
    int GetReturnValue(void);

private:
    QTimer *m_Timer;
    QLabel *m_Label;
    QPushButton *m_CancelButton;
    QVBoxLayout *m_Layout;

signals:
    void CloseDialog();

private slots:
    void CancelSlot();
    void CyclicTimeout();

private:
    int m_ReturnValue;
    char *m_InstanceName;
};

extern "C" {
int WaitUntilXilEnvCanStart(char *par_InstanceName, void *par_Application);
}

#endif // ISALREADYRUNNUNG_H

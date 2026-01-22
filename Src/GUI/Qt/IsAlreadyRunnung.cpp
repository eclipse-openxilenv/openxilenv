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


#include "IsAlreadyRunnung.h"

#include <QApplication>

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "CheckIfAlreadyRunning.h"
}

IsAlreadyRunnung::IsAlreadyRunnung(char *par_InstanceName, QWidget *parent) : QWidget(parent)
{
    m_ReturnValue = 0;

    m_InstanceName = StringMalloc (par_InstanceName);

    QString LabelText = QString("XilEnv with instance\n"
                        "\"%1\"\n"
                        "already running.\n"
                        "Close the other XilEnv, than this XilEnv will start immediately, or press Cancel to exit").arg(par_InstanceName);

    m_Label = new QLabel(LabelText);
    m_Label->setAlignment(Qt::AlignCenter);
    m_CancelButton = new QPushButton("Cancel");
    m_Layout = new QVBoxLayout ();
    m_Layout->addWidget (m_Label);
    m_Layout->addWidget (m_CancelButton);
    this->setLayout(m_Layout);

    connect(m_CancelButton, SIGNAL(clicked()), this, SLOT(CancelSlot()));

    m_Timer = new QTimer(this);
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(CyclicTimeout()));
    m_Timer->setInterval(200);
    m_Timer->start();

}

IsAlreadyRunnung::~IsAlreadyRunnung()
{
    delete m_Layout;
    delete m_Label;
    delete m_CancelButton;
    delete m_Timer;
    if (m_InstanceName != nullptr) {
        my_free(m_InstanceName);
    }
}

int IsAlreadyRunnung::GetReturnValue()
{
    return m_ReturnValue;
}

void IsAlreadyRunnung::CancelSlot()
{
    m_ReturnValue = 1;
    emit CloseDialog();
}

void IsAlreadyRunnung::CyclicTimeout()
{
    if (m_InstanceName != nullptr) {
        if (TryToLockInstance (m_InstanceName)) {
            m_ReturnValue = 0;
            emit CloseDialog();
        }
    }
}


extern "C" {
int WaitUntilXilEnvCanStart(char *par_InstanceName, void *par_Application)
{
    QApplication *Application = static_cast<QApplication*>(par_Application);
    IsAlreadyRunnung *Window = new IsAlreadyRunnung(par_InstanceName);

    QObject::connect(Window,SIGNAL(CloseDialog()),Application,SLOT(quit()));
    Window->show();

    // Start th Qt event loop
    Application->exec();

    int Ret = Window->GetReturnValue();

    delete Window;

    return Ret;
}
}

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


#include "Dialog.h"
#include "MainWindow.h"
extern "C" {
#include "Scheduler.h"
#include "MainValues.h"
}

int Dialog::instanceCounter = 0;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent)
{
    if (s_main_ini_val.StopSchedulerWhileDialogOpen && instanceCounter == 0)
    {
        disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_SYSTEM, nullptr, nullptr);
    }
    instanceCounter++;
    MainWindow::GetPtrToMainWindow()->removeTopHintFlagControlPanel();
}

Dialog::~Dialog()
{
    MainWindow::GetPtrToMainWindow()->setTopHintFlagControlPanel();
    instanceCounter--;
    if (s_main_ini_val.StopSchedulerWhileDialogOpen && instanceCounter == 0)
    {
        enable_scheduler (SCHEDULER_CONTROLED_BY_SYSTEM);
    }
}


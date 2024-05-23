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


#include "ProgressBar.h"
#include "ui_ProgressBar.h"

#include<QApplication>

#include "MainWindow.h"

int ProgressBar::instanceCounter = 0;

ProgressBar::ProgressBar(int par_Id, QString par_progressName, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProgressBar)
{    
    ui->setupUi(this);

    avoidRecusiveCall = false;

    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setWindowTitle(par_progressName);
    setAttribute( Qt::WA_TranslucentBackground, true );
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(100);
    ui->progressBar->setValue(0);
    //ui->progressBar->repaint();
    ui->label->setText(par_progressName);
    if (par_Id < 0) {
        progressID = instanceCounter;
    } else {
        progressID = par_Id;
    }
    instanceCounter++;
    //connect(MainWindow::PointerToMainWindow, SIGNAL(SetProgressBarSignal(int)), this, SLOT(valueChanged(int)));
}

ProgressBar::~ProgressBar()
{
    // der zaehlt jetzt nur noch hoch
    //instanceCounter--;
    delete ui;
}

void ProgressBar::valueChanged(int value, int par_progressID, bool par_CalledFromGuiThread)
{
    if(par_progressID == progressID)
    {
        ui->progressBar->setValue(value);
        if (!avoidRecusiveCall) {
            if (par_CalledFromGuiThread) {
                avoidRecusiveCall = true;
                //ui->progressBar->repaint();
                //qApp->processEvents();
                avoidRecusiveCall = false;
            }
        }
    }
}

int ProgressBar::getProgressID()
{
    return progressID;
}

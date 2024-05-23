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


#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QWidget>

namespace Ui {
class ProgressBar;
}

class ProgressBar : public QWidget
{
    Q_OBJECT
    static int instanceCounter;

public:
    explicit ProgressBar(int par_Id, QString par_progressName, QWidget *parent = nullptr);
    int getProgressID();
    ~ProgressBar();

public slots:
    void valueChanged(int value, int par_progressID, bool par_CalledFromGuiThread);

private:
    Ui::ProgressBar *ui;
    int progressID;
    bool avoidRecusiveCall;
};

#endif // PROGRESSBAR_H

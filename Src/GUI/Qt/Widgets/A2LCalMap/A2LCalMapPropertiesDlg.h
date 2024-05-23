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


#ifndef A2LCALMAPPROPERTIESDLG_H
#define A2LCALMAPPROPERTIESDLG_H

#include <QDialog>
#include <Dialog.h>

extern "C" {
#include "A2LLink.h"
}

namespace Ui {
class A2LCalMapPropertiesDlg;
}

class A2LCalMapPropertiesDlg : public Dialog
{
    Q_OBJECT

public:
    explicit A2LCalMapPropertiesDlg(QString par_Label,
                                    int par_Type,
                                    uint64_t par_Address,
                                    CHARACTERISTIC_AXIS_INFO *par_XAxisInfo,
                                    CHARACTERISTIC_AXIS_INFO *par_YAxisInfo,
                                    CHARACTERISTIC_AXIS_INFO *par_ZAxisInfo,
                                    QWidget *parent = nullptr);
    ~A2LCalMapPropertiesDlg();

    void Get(CHARACTERISTIC_AXIS_INFO *ret_XAxisInfo,
             CHARACTERISTIC_AXIS_INFO *ret_YAxisInfo,
             CHARACTERISTIC_AXIS_INFO *ret_ZAxisInfo);

private:
    Ui::A2LCalMapPropertiesDlg *ui;
    int m_Type;
};

#endif // A2LCALMAPPROPERTIESDLG_H

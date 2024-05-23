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


#ifndef CANMESSAGEWINDOWMODELITEM_H
#define CANMESSAGEWINDOWMODELITEM_H

#include <QList>

extern "C" {
#include "canfifo.h"
}

class CANMessageWindowModelItem
{
public:
    CANMessageWindowModelItem(CAN_FIFO_ELEM &par_CanMessage);
    ~CANMessageWindowModelItem();
private:
    CAN_FIFO_ELEM m_CanMessage;
    double m_LastTime;
    double m_dT;
    double m_dTMin;
    double m_dTMax;
    int m_Counter;

    QList<CANMessageWindowModelItem*> m_ChildItems;
    CANMessageWindowModelItem *m_ParentItem;
};

#endif // CANMESSAGEWINDOWMODELITEM_H

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


#ifndef A2LCALWIDGETSYNC_H
#define A2LCALWIDGETSYNC_H

//#ifdef __cplusplus
//extern "C" {
//#endif
//    #include "A2LLinkThread.h"
//#ifdef __cplusplus
//}
//#endif

#ifdef __cplusplus

#include <QObject>
#include "A2LCalMapWidget.h"
#include "A2LCalSingleWidget.h"

class A2LCalMapWidgetSyncObject : public QObject
{
    Q_OBJECT
public:
    void NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data);
    void NotifiyStartStopProcessFromSchedulerThread (const char *par_ProcessName, int par_Flags, int par_Action);
    void NotifiyGetDataFromLinkAck (void *par_IndexData, int par_FetchDataChannelNo, void *par_Param);
    void AddA2LCalMapWidget (A2LCalMapWidget *par_A2LCalMapWidget);
    void RemoveA2LCalMapWidget (A2LCalMapWidget *par_A2LCalMapWidget);
    void AddA2LCalSingleWidget (A2LCalSingleWidget *par_A2LCalSingleWidget);
    void RemoveA2LCalSingleWidget (A2LCalSingleWidget *par_A2LCalSingleWidget);
signals:
    void NotifiyStartStopProcessFromSchedulerSignal (QString par_ProcessName, int par_Flags, int par_Action);
    void GetDataFromLinkAckSignal (void *par_IndexData, int par_FetchDataChannelNo, void *par_Param);
public slots:
    void NotifiyStartStopProcessToWidgetThreadSlot (QString par_ProcessName, int par_Flags, int par_Action);
    void NotifiyGetDataFromLinkAckToWidgetThreadSlot (void *par_IndexData, int par_FetchDataChannelNo, void *par_Param);
private:
    QList<A2LCalMapWidget*> m_ListOfAllA2LCalMapWidget;
    QList<A2LCalSingleWidget*> m_ListOfAllA2LCalSingleWidget;
};

void SetupA2LCalWidgetSyncObject(QObject *par_Object);

void NotifyDataChangedToSyncObject(int par_LinkNo, int par_Index, A2L_DATA *par_Data);
void AddA2LCalMapWidgetToSyncObject(A2LCalMapWidget *par_A2LCalMapWidget);
void RemoveA2LCalMapWidgetFromSyncObject(A2LCalMapWidget *par_A2LCalMapWidget);
void AddA2LCalSingleWidgetToSyncObject(A2LCalSingleWidget *par_A2LCalSingleWidget);
void RemoveA2LCalSingleWidgetFromSyncObject(A2LCalSingleWidget *par_A2LCalSingleWidget);

extern "C" {
#endif
int GlobalNotifiyA2LCalMapWidgetFuncSchedThread (const char *par_ProcessName, int par_Flags, int par_Action);
int GlobalNotifiyGetDataFromLinkAckCallback (void *par_IndexData, int par_FetchDataChannelNo, void *par_Param);
#ifdef __cplusplus
}
#endif
#endif // A2LCALWIDGETSYNC_H

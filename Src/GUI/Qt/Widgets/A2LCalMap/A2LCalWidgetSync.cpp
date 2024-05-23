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


#include <stdint.h>

#include "A2LCalWidgetSync.h"
extern "C" {
#include "ThrowError.h"
#include "A2LLinkThread.h"
}

void A2LCalMapWidgetSyncObject::NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data)
{
    foreach (A2LCalMapWidget* Widget, m_ListOfAllA2LCalMapWidget) {
        Widget->NotifyDataChanged(par_LinkNo, par_Index, par_Data);
    }
    foreach (A2LCalSingleWidget* Widget, m_ListOfAllA2LCalSingleWidget) {
        Widget->NotifyDataChanged(par_LinkNo, par_Index, par_Data);
    }
}

void A2LCalMapWidgetSyncObject::NotifiyStartStopProcessFromSchedulerThread (const char *par_ProcessName, int par_Flags, int par_Action)
{
    QString ProcessName = QString(par_ProcessName);
    emit NotifiyStartStopProcessFromSchedulerSignal (ProcessName, par_Flags, par_Action);
}

void A2LCalMapWidgetSyncObject::NotifiyGetDataFromLinkAck(void *par_IndexData, int par_FetchDataChannelNo, void *par_Param)
{
    emit GetDataFromLinkAckSignal (par_IndexData, par_FetchDataChannelNo, par_Param);
}

void A2LCalMapWidgetSyncObject::AddA2LCalMapWidget(A2LCalMapWidget *par_A2LCalMapWidget)
{
    m_ListOfAllA2LCalMapWidget.append(par_A2LCalMapWidget);
}

void A2LCalMapWidgetSyncObject::RemoveA2LCalMapWidget(A2LCalMapWidget *par_A2LCalMapWidget)
{
    m_ListOfAllA2LCalMapWidget.removeAll(par_A2LCalMapWidget);
}

void A2LCalMapWidgetSyncObject::AddA2LCalSingleWidget(A2LCalSingleWidget *par_A2LCalSingleWidget)
{
    m_ListOfAllA2LCalSingleWidget.append(par_A2LCalSingleWidget);
}

void A2LCalMapWidgetSyncObject::RemoveA2LCalSingleWidget(A2LCalSingleWidget *par_A2LCalSingleWidget)
{
    m_ListOfAllA2LCalSingleWidget.removeAll(par_A2LCalSingleWidget);
}

void A2LCalMapWidgetSyncObject::NotifiyStartStopProcessToWidgetThreadSlot (QString par_ProcessName, int par_Flags, int par_Action)
{
    // now we are inside the context of the thread where the A2LCalWidgets live.
    foreach (A2LCalMapWidget* Widget, m_ListOfAllA2LCalMapWidget) {
        Widget->NotifiyStartStopProcess (par_ProcessName, par_Flags, par_Action);
    }
    foreach (A2LCalSingleWidget* Widget, m_ListOfAllA2LCalSingleWidget) {
        Widget->NotifiyStartStopProcess (par_ProcessName, par_Flags, par_Action);
    }
}

void A2LCalMapWidgetSyncObject::NotifiyGetDataFromLinkAckToWidgetThreadSlot(void *par_IndexData,int par_FetchDataChannelNo, void *par_Param)
{
    // now we are inside the context of the thread where the A2LCalWidgets live.
    foreach (A2LCalMapWidget* Widget, m_ListOfAllA2LCalMapWidget) {
        if (Widget == par_Param) {
            Widget->NotifiyGetDataFromLinkAck (par_IndexData, par_FetchDataChannelNo);
            return;
        }
    }
    foreach (A2LCalSingleWidget* Widget, m_ListOfAllA2LCalSingleWidget) {
        if (Widget == par_Param) {
            Widget->NotifiyGetDataFromLinkAck (par_IndexData, par_FetchDataChannelNo);
            return;
        }
    }
}

static A2LCalMapWidgetSyncObject *SyncObject;

void SetupA2LCalWidgetSyncObject(QObject *par_Object)
{
    if (SyncObject == nullptr) {
        SyncObject = new A2LCalMapWidgetSyncObject;
        bool Ret = par_Object->connect (SyncObject, SIGNAL(NotifiyStartStopProcessFromSchedulerSignal(QString, int, int)),
                                        SyncObject, SLOT(NotifiyStartStopProcessToWidgetThreadSlot(QString, int, int)));
        if (!Ret) {
            ThrowError (1, "connect error");
        }
        Ret = par_Object->connect (SyncObject, SIGNAL(GetDataFromLinkAckSignal(void* ,int, void*)),
                                   SyncObject, SLOT(NotifiyGetDataFromLinkAckToWidgetThreadSlot(void* ,int, void*)));
        if (!Ret) {
            ThrowError (1, "connect error");
        }
    }
}

void NotifyDataChangedToSyncObject(int par_LinkNo, int par_Index, A2L_DATA *par_Data)
{
    if (SyncObject != nullptr) {
        SyncObject->NotifyDataChanged(par_LinkNo, par_Index, par_Data);
    }
}

void AddA2LCalMapWidgetToSyncObject(A2LCalMapWidget *par_A2LCalMapWidget)
{
    if (SyncObject != nullptr) {
        SyncObject->AddA2LCalMapWidget(par_A2LCalMapWidget);
    }
}

void RemoveA2LCalMapWidgetFromSyncObject(A2LCalMapWidget *par_A2LCalMapWidget)
{
    if (SyncObject != nullptr) {
        SyncObject->RemoveA2LCalMapWidget(par_A2LCalMapWidget);
    }
}

void AddA2LCalSingleWidgetToSyncObject(A2LCalSingleWidget *par_A2LCalSingleWidget)
{
    if (SyncObject != nullptr) {
        SyncObject->AddA2LCalSingleWidget(par_A2LCalSingleWidget);
    }
}

void RemoveA2LCalSingleWidgetFromSyncObject(A2LCalSingleWidget *par_A2LCalSingleWidget)
{
    if (SyncObject != nullptr) {
        SyncObject->RemoveA2LCalSingleWidget(par_A2LCalSingleWidget);
    }
}

// This funktion will be called from the scheduler thread every time a process is started or stopped
// synced with signal and slots
extern "C" {
int GlobalNotifiyA2LCalMapWidgetFuncSchedThread (const char *par_ProcessName, int par_Flags, int par_Action)
{
    if (SyncObject != nullptr) {
        SyncObject->NotifiyStartStopProcessFromSchedulerThread (par_ProcessName, par_Flags, par_Action);
    }
    return 0;
}

int GlobalNotifiyGetDataFromLinkAckCallback (void *par_IndexData, int par_FetchDataChannelNo, void *par_Param)
{
    if (SyncObject != nullptr) {
        INDEX_DATA_BLOCK *IndexData = (INDEX_DATA_BLOCK*)par_IndexData;
        SyncObject->NotifiyGetDataFromLinkAck(IndexData,  par_FetchDataChannelNo, par_Param);
    }
    return 0;
}

}


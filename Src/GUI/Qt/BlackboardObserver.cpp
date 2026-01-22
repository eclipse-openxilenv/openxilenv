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


#include "BlackboardObserver.h"

extern "C" {
#include "Blackboard.h"
}

#ifdef _UNUCODE
#error  _UNUCODE
#endif

#ifdef UNUCODE
#error  UNUCODE
#endif

static BlackboardObserver blackboardObserverInst;


static void GlobalVariableChanged (VID arg_vid, uint32_t arg_observationFlags, uint32_t arg_observationData)
{
    blackboardObserverInst.VariableChanged(arg_vid, arg_observationFlags, arg_observationData);
}

BlackboardObserver::BlackboardObserver()
{
    InitializeCriticalSection (&m_CriticalSection);
}

BlackboardObserver::~BlackboardObserver()
{
}

void BlackboardObserver::ConnectTo(BlackboardObserverConnection *par_Connection, QObject *par_OwnerObject)
{
    EnterCriticalSection(&m_CriticalSection);
    par_Connection->SetOwner(par_OwnerObject);
    this->m_Connections.append(par_Connection);
    LeaveCriticalSection(&m_CriticalSection);
}

void BlackboardObserver::DisconnectFrom(BlackboardObserverConnection *par_Connection)
{
    EnterCriticalSection(&m_CriticalSection);
    this->m_Connections.removeAll(par_Connection);
    for (int x = 0; x < m_ObservedVariables.size(); /*nothing*/) {
        uint32_t Flags = 0UL;
        ObserverdVariableEntry *Entry = m_ObservedVariables.at(x);
        for (int y = 0; y < Entry->m_ConnectionFlags.size(); /*nothing*/) {
            if (Entry->m_ConnectionFlags.at(y).m_Connection == par_Connection) {
                Entry->m_ConnectionFlags.removeAt(y);
            } else {
                Flags = Flags | Entry->m_ConnectionFlags.at(y).m_Flags;
                y++;
            }
        }
        if (Entry->m_ConnectionFlags.size() == 0) {
            // Do not observe variable any more
            SetBbvariObservation (Entry->m_Vid, OBSERVE_RESET_FLAGS, 0UL);
            m_ObservedVariables.removeAt(x);
            delete Entry;
        } else {
            if (Entry->m_Flags != Flags) {
                // Observation flags have chagend
                SetBbvariObservation (Entry->m_Vid, OBSERVE_RESET_FLAGS | Flags, static_cast<uint32_t>(x));
            }
            x++;
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
}

void BlackboardObserver::AddObserveVariable(BlackboardObserverConnection *arg_Connection, VID arg_Vid, uint32_t arg_ObservationFlags)
{
    int x;
    EnterCriticalSection(&m_CriticalSection);
    if (m_Connections.contains(arg_Connection)) {
        for (x = 0; x < m_ObservedVariables.size(); x++) {
            if (m_ObservedVariables.at(x)->m_Vid == arg_Vid) {
                break;
            }
        }
        if (x == m_ObservedVariables.size()) {
            // New variable
            ObserverdVariableEntry* NewEntry = new ObserverdVariableEntry;
            NewEntry->m_Flags = arg_ObservationFlags;
            NewEntry->m_Vid = arg_Vid;
            m_ObservedVariables.append(NewEntry);
        } else {
            // Variable alread there
            m_ObservedVariables.at(x)->m_Flags |= arg_ObservationFlags;
        }
        SetBbvariObservation (m_ObservedVariables.at(x)->m_Vid, m_ObservedVariables.at(x)->m_Flags, static_cast<uint32_t>(x));
        ObserverdVariableObjectEntry New;
        New.m_Flags = arg_ObservationFlags;
        New.m_Connection = arg_Connection;
        m_ObservedVariables.at(x)->m_ConnectionFlags.append(New);
    }
    LeaveCriticalSection(&m_CriticalSection);
}

void BlackboardObserver::RemoveObserveVariable(BlackboardObserverConnection *arg_Connection, VID arg_Vid)
{
    EnterCriticalSection(&m_CriticalSection);
    if (m_Connections.contains(arg_Connection)) {
        int x;
        for (x = 0; x < m_ObservedVariables.size(); /* nothing */) {
            ObserverdVariableEntry *Entry = m_ObservedVariables.at(x);
            if (Entry->m_Vid == arg_Vid) {
                uint32_t Flags = 0UL;
                for (int y = 0; y < Entry->m_ConnectionFlags.size(); /* nothing */) {
                    if (Entry->m_ConnectionFlags.at(y).m_Connection == arg_Connection) {
                        Entry->m_ConnectionFlags.removeAt(y);
                    } else {
                        Flags = Flags | Entry->m_ConnectionFlags.at(y).m_Flags;
                        y++;
                    }
                }
                if (Entry->m_ConnectionFlags.size() == 0) {
                    // Do not observe variable any more
                    SetBbvariObservation (Entry->m_Vid, OBSERVE_RESET_FLAGS, 0UL);
                    m_ObservedVariables.removeAt(x);
                    delete Entry;
                } else {
                    if (Entry->m_Flags != Flags) {
                        // Observation flags have chagend
                        SetBbvariObservation (Entry->m_Vid, OBSERVE_RESET_FLAGS | Flags, static_cast<uint32_t>(x));
                    }
                    x++;
                }
            } else {
                x++;
            }
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
}

void BlackboardObserver::AddObserveBlackboard(BlackboardObserverConnection* arg_Connection, uint32_t arg_ObservationFlags, int **ret_Vids, int *ret_VidsCount)
{
    uint32_t Flags = 0UL;
    EnterCriticalSection(&m_CriticalSection);
    arg_Connection->m_Flags = arg_ObservationFlags;
    m_GlobalConnections.append(arg_Connection);
    for(int x = 0; x < m_GlobalConnections.size(); x++) {
        Flags |= m_GlobalConnections.at(x)->m_Flags;
    }
    SetGlobalBlackboradObservation (Flags, 0, ret_Vids, ret_VidsCount);
    LeaveCriticalSection(&m_CriticalSection);
}

void BlackboardObserver::RemoveObserveBlackboard(BlackboardObserverConnection* arg_Connection)
{
    uint32_t Flags = 0UL;
    EnterCriticalSection(&m_CriticalSection);
    m_GlobalConnections.removeOne(arg_Connection);
    for(int x = 0; x < m_GlobalConnections.size(); x++) {
        Flags |= m_GlobalConnections.at(x)->m_Flags;
    }
    SetGlobalBlackboradObservation (Flags, 0UL, nullptr, nullptr);
    LeaveCriticalSection(&m_CriticalSection);
}

void BlackboardObserver::VariableChanged(VID arg_Vid, uint32_t arg_observationFlags, uint32_t arg_observationData)
{
    EnterCriticalSection(&m_CriticalSection);
    // this list should only have one entry (BlackboardVariableModel)
    for (int x = 0; x < m_GlobalConnections.size(); x++) {
        if ((m_GlobalConnections.at(x)->m_Flags & arg_observationFlags) != 0) {
            emit (m_GlobalConnections.at(x)->variableChanged (arg_Vid, arg_observationFlags));
        }
    }
    // arg_observationData will be used as an index into  m_ObservedVariables
    if (arg_observationData < static_cast<uint32_t>(this->m_ObservedVariables.size())) {
        ObserverdVariableEntry *Entry = this->m_ObservedVariables.at(static_cast<int>(arg_observationData));
        for (int x = 0; x < Entry->m_ConnectionFlags.size(); x++) {
            if ((Entry->m_ConnectionFlags.at(x).m_Flags & arg_observationFlags) != 0) {
                emit Entry->m_ConnectionFlags.at(x).m_Connection->variableChanged (arg_Vid, arg_observationFlags);
            }
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
}

BlackboardObserverConnection::BlackboardObserverConnection(QObject *par_OwnerObject)
{
    connect(this, SIGNAL(variableChanged(int,unsigned int)), par_OwnerObject, SLOT(blackboardVariableConfigChanged(int,unsigned int)));
    blackboardObserverInst.ConnectTo(this, par_OwnerObject);
}

BlackboardObserverConnection::~BlackboardObserverConnection()
{
    // disconnet necessary?
    disconnect(this, SIGNAL(variableChanged(int,unsigned int)), m_Owner, SLOT(blackboardVariableConfigChanged(int,unsigned int)));
    blackboardObserverInst.DisconnectFrom(this);
}

void BlackboardObserverConnection::AddObserveVariable(VID arg_Vid, uint32_t arg_ObservationFlags)
{
    blackboardObserverInst.AddObserveVariable(this, arg_Vid, arg_ObservationFlags);
}

void BlackboardObserverConnection::RemoveObserveVariable(VID arg_Vid)
{
    blackboardObserverInst.RemoveObserveVariable(this, arg_Vid);
}

void BlackboardObserverConnection::AddObserveBlackboard(uint32_t arg_ObservationFlags, int **ret_Vids, int *ret_VidsCount)
{
    blackboardObserverInst.AddObserveBlackboard(this, arg_ObservationFlags, ret_Vids, ret_VidsCount);
}

void BlackboardObserverConnection::RemoveObserveBlackboard()
{
    blackboardObserverInst.RemoveObserveBlackboard(this);
}

extern "C" void StartBlackboardObserver(void)
{
    SetObserationCallbackFunction(GlobalVariableChanged);
}

extern "C" void StopBlackboardObserver(void)
{
    SetObserationCallbackFunction(nullptr);
}

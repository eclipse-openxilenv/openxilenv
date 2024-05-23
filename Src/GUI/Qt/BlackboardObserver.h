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


#ifndef BLACKBOARDOBSERVER_H
#define BLACKBOARDOBSERVER_H

#ifdef __cplusplus

#include <QObject>
#include "Platform.h"
extern "C" {
#include "GlobalDataTypes.h"
}


class BlackboardObserverConnection : public QObject
{
    Q_OBJECT
public:
    BlackboardObserverConnection (QObject *par_OwnerObject);
    ~BlackboardObserverConnection ();
    void SetOwner(QObject *par_Owner) { m_Owner = par_Owner; }
    void AddObserveVariable(VID arg_Vid, uint32_t arg_ObservationFlags);
    void RemoveObserveVariable(VID arg_Vid);
    void AddObserveBlackboard(uint32_t arg_ObservationFlags, int **ret_Vids = nullptr, int *ret_VidsCount = nullptr);
    void RemoveObserveBlackboard();

signals:
    void variableChanged(int arg_vid, unsigned int arg_ObservationFlagsFired);

private:
    QObject *m_Owner;
};


class ObserverdVariableObjectEntry
{
public:
    BlackboardObserverConnection *m_Connection;
    uint32_t m_Flags;
};

class ObserverdVariableEntry
{
public:
    int m_Vid;
    uint32_t m_Flags;
    QList<ObserverdVariableObjectEntry> m_ConnectionFlags;
};



class BlackboardObserver : public QObject
{
    Q_OBJECT
public:
    BlackboardObserver();
    ~BlackboardObserver();
    void ConnectTo (BlackboardObserverConnection *par_Connection, QObject *par_OwnerObject);
    void DisconnectFrom (BlackboardObserverConnection *par_Connection);

    void AddObserveVariable(BlackboardObserverConnection *arg_Connection, VID arg_Vid, uint32_t arg_ObservationFlags);
    void RemoveObserveVariable(BlackboardObserverConnection *arg_Connection, VID arg_Vid);
    void AddObserveBlackboard(BlackboardObserverConnection *arg_Connection, uint32_t arg_ObservationFlags, int **ret_Vids = nullptr, int *ret_VidsCount = nullptr);
    void RemoveObserveBlackboard(BlackboardObserverConnection *arg_Connection);

    void VariableChanged(VID arg_Vid, uint32_t arg_observationFlags, uint32_t arg_observationData);

private:
    QList<BlackboardObserverConnection*> m_Connections;
    QList<ObserverdVariableEntry*> m_ObservedVariables;

    CRITICAL_SECTION m_CriticalSection;
};

extern "C" void StartBlackboardObserver(void);

extern "C" void StopBlackboardObserver(void);

#else

void StartBlackboardObserver(void);

void StopBlackboardObserver(void);

#endif


#endif // BLACKBOARDOBSERVER_H

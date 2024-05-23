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


#ifndef A2LCALMAPMODEL_H
#define A2LCALMAPMODEL_H

#include <QAbstractItemModel>

#include "A2LCalMapData.h"


class A2LCalMapModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    A2LCalMapModel(); //A2LCalMapData *par_Data);
    ~A2LCalMapModel() Q_DECL_OVERRIDE;

    int GetType();
    int GetAddress(uint64_t *ret_Address);
    QString GetCharacteristicName();
    QString GetProcess();
    void SetCharacteristicName(QString &par_Characteristic);
    void SetProcess(QString par_Process, void *par_CallbaclParam);
    bool IsPhysical();
    void SetPhysical(bool par_PhysFlag);
    void ProcessTerminated();
    void ProcessStarted();
    void Update();
    void UpdateAck(void *par_Data);
    void Reset();
    double GetMaxMinusMin (int axis);
    void CalcRawMinMaxValues();

    void SetNoRecursiveSetPickMapFlag(bool par_Flag);
    bool GetNoRecursiveSetPickMapFlag();

    int GetMapDataType();
    const char *GetMapConversion();
    int ChangeSelectedValuesWithDialog(QWidget *parent, int op);
    int ChangeSelectedMapValues(double NewValue, int op);
    int ChangeSelectedMapValues(union FloatOrInt64 NewValue, int NewValueType, int op);
    void ChangeSelectionOfAll(bool par_Select);
    void AdjustMinMax();

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &par_index, const QVariant &par_value,
                 int par_role = Qt::EditRole) Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

    void BeginUpdateData();
    void EndUpdateData();

    void ChangeData(int axis, int x, int y);

    A2LCalMapData *GetInternalDataStruct() const;

    void NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data);

private:

    A2LCalMapData *m_Data;

signals:
    void dataChangedFromTable(int axis, int x, int y, double NewValue);
    void PhysRawChangedFromTable(bool Phys);


};

#endif // A2LCalMAPMODEL_H

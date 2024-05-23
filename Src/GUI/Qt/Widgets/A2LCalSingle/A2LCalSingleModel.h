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


#ifndef A2LCALSINGLEMODEL_H
#define A2LCALSINGLEMODEL_H

#include <QAbstractTableModel>
#include <QPair>
#include <QList>
#include <QString>
#include <QColor>
#include <QFontMetrics>

#include "A2LCalSingleData.h"

extern "C" {
#include "A2LAccessData.h"
#include "Blackboard.h"
#include "DebugInfos.h"
#include "A2LLink.h"
}


class A2LCalSingleModel : public QAbstractTableModel {
    Q_OBJECT
public:

    A2LCalSingleModel(QObject *arg_parent = nullptr);
    ~A2LCalSingleModel() Q_DECL_OVERRIDE;

    void SetProcessName(QString &par_ProcessName, void* par_Parent);
    QString GetProcessName();
    int GetLinkNo();
    void ProcessTerminated();
    void ProcessStarted(void *par_Parent);
    void NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data);

    int rowCount(const QModelIndex &arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &arg_index, int arg_role) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &arg_index, const QVariant &arg_value, int arg_role) Q_DECL_OVERRIDE;
    QVariant headerData(int arg_section, Qt::Orientation arg_orientation, int arg_role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &arg_index) const Q_DECL_OVERRIDE;
    bool insertRows(int arg_position, int arg_rows, const QModelIndex &arg_index = QModelIndex()) Q_DECL_OVERRIDE;
    bool removeRows(int arg_position, int arg_rows, const QModelIndex &arg_index = QModelIndex()) Q_DECL_OVERRIDE;
    bool moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild);
    QList<A2LCalSingleData *> getList();
    A2LCalSingleData *getVariable(int arg_row) const;
    void setVariable(int arg_row, A2LCalSingleData *arg_variable);
    int getRowIndex(int par_Index);
    int getRowIndex(QString arg_name);
    void SetCharacteristicName(int arg_row, QString &par_Characteristic);
    void SetDisplayType(int par_Row, int par_DisplayType);
    void setBackgroundColor(QColor &arg_Color);

    int columnAlignment(int par_Column);
    void setColumnAlignment(int par_Column, int par_ColumnAlignments);

    void UpdateAllValuesReq(bool arg_updateAlways = false, bool arg_checkUnitColumnWidth = false);
    void UpdateOneValueReq(int par_Row, bool arg_updateAlways = false, bool arg_checkUnitColumnWidth = false);
    void UpdateValuesAck(void *par_IndexData);

    void setFont(QFont &arg_font);
    QStringList getAllCharacteristicsNames();
    void makeBackup();
    void restoreBackup();

    bool CheckExistanceOfAll(int par_LinkNo);
    bool CheckExistanceOfOne(int par_Index, int par_LinkNo);

signals:
    void columnWidthChanged(int arg_columnFlags, int arg_nameWidth, int arg_valueWidth, int arg_unitWidth);

private:
    void SetupLinkTo(void *par_Parent);

#define CALC_PIXELWIDTH_OF_NAME     0x1
#define CALC_PIXELWIDTH_OF_VALUES   0x2
#define CALC_PIXELWIDTH_OF_UNIT     0x4

    int calculateMinMaxColumnSize(int arg_rowFirst, int arg_rowLast, int arg_columnFlags);
    QList<A2LCalSingleData*> m_listOfElements;
    QList<A2LCalSingleData*> m_backupListOfElements;
    QFontMetrics *m_FontMetrics;
    int m_RowHeight;

    QColor m_BackgroundColor;

    int m_nameMaxPixelWidth;
    int m_valueMaxPixelWidth;
    int m_unitMaxPixelWidth;
    int m_nameMaxWidthIdx;
    int m_valueMaxWidthIdx;
    int m_unitMaxWidthIdx;

    int m_ColumnAlignments[4];      // Qt::AlignRight

    // Synchron read of all values
    int UpdateOneVariable(A2LCalSingleData *par_Variable, int par_Row, bool par_UpdateAlways);

    QString m_ProcessName;
    int m_DataFromLinkChannelNo;
    int m_LinkNo;
};

#endif // A2LCALSINGLEMODEL_H

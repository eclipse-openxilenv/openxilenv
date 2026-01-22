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


#ifndef TEXTTABLEMODEL_H
#define TEXTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QPair>
#include <QList>
#include <QString>
#include <QColor>
#include <QFontMetrics>
#include <QTextStream>

extern "C" {
#include "Blackboard.h"
}

class TextTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    class Variable {
    public:
        Variable();
        QString m_name;
        QString m_unit;
        int m_type;
        QString m_value;
        int m_vid;
        double m_step;
        int m_stepType;
        bool m_exists;
        bool m_hasConversion;
        bool m_hasOwnBackgroundColor;
        QColor m_backgroundColor;
        int m_precision;
        int m_conversionType;
        enum DATATYPE_CODED {SIGNEDINT_DATATYTE, UNSIGNEDINT_DATATYTE, FLOAT_DATATYTE, ERROR_DATATYPE} m_codedDataType;
        double m_rawDoubleValue;
        uint64_t m_rawUnsigendValue;
        int64_t m_rawSigendValue;
        enum BB_DATA_TYPES m_dataType;
        union BB_VARI m_rawValue;
        bool m_hasChanged;
        int m_namePixelWidth;
        int m_valuePixelWidth;
        int m_unitPixelWidth;
    };

    TextTableModel(QObject *arg_parent = nullptr);
    ~TextTableModel() Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &arg_parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &arg_index, int arg_role) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &arg_index, const QVariant &arg_value, int arg_role) Q_DECL_OVERRIDE;
    QVariant headerData(int arg_section, Qt::Orientation arg_orientation, int arg_role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &arg_index) const Q_DECL_OVERRIDE;
    bool insertRows(int arg_position, int arg_rows, const QModelIndex &arg_index = QModelIndex()) Q_DECL_OVERRIDE;
    bool removeRows(int arg_position, int arg_rows, const QModelIndex &arg_index = QModelIndex()) Q_DECL_OVERRIDE;
    bool moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild);
    QList<Variable *> getList();
    Variable *getVariable(int arg_row) const;
    void setVariable(int arg_row, Variable *arg_variable);
    void setVidValue(int arg_row, int arg_vid);
    void setVariableExists(int arg_row, bool arg_show);
    void setVariableHasConversion(int arg_row, bool arg_hasConversion);
    void setVariablePrecision(int arg_row, int arg_precision);
    void setVariableDataType(int arg_row, enum BB_DATA_TYPES arg_dataType);
    int getRowIndex(int arg_vid);
    int getRowIndex(QString arg_name);
    void setStep(int arg_row, double arg_step);
    void setStepType(int arg_row, int arg_stepType);
    void setDisplayType(int arg_row, int arg_displayType);
    void setConversionType(int arg_row, int arg_conversionType);
    void setUnit(int arg_row, QString arg_unit);
    void setName(int arg_row, QString arg_name);
    void setBackgroundColor(QColor &arg_Color);

    int columnAlignment(int par_Column);
    void setColumnAlignment(int par_Column, int par_ColumnAlignments);

    void CyclicUpdateValues(int arg_fromRow, int arg_toRow, bool arg_updateAlways = false, bool arg_checkUnitColumnWidth = false);

    void setFont(QFont &arg_font);
    QStringList getAllVariableNames();
    void makeBackup();
    void restoreBackup();

    void blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag);
    enum FileTypeEnum {SCRIPT_FILE_TYPE, EQU_FILE_TYPE};
    void WriteContentToFile(bool par_IndentLine, enum FileTypeEnum par_FileType, QTextStream &Stream);

signals:
    void columnWidthChanged(int arg_columnFlags, int arg_nameWidth, int arg_valueWidth, int arg_unitWidth);

private:
#define CALC_PIXELWIDTH_OF_NAME     0x1
#define CALC_PIXELWIDTH_OF_VALUES   0x2
#define CALC_PIXELWIDTH_OF_UNIT     0x4

    int calculateMinMaxColumnSize(int arg_rowFirst, int arg_rowLast, int arg_columnFlags);
    QList<Variable*> m_listOfElements;
    QList<Variable*> m_backupListOfElements;
    QFontMetrics *m_FontMetrics;
    int m_RowHeight;

    QColor m_BackgroundColor;

    int m_nameMaxPixelWidth;
    int m_valueMaxPixelWidth;
    int m_unitMaxPixelWidth;
    int m_nameMaxWidthIdx;
    int m_valueMaxWidthIdx;
    int m_unitMaxWidthIdx;

    int m_ColumnAlignments[4];

    // This necessary to synchronous read of all values
    void SyncCopyBuffer(void);
    int UpdateOneVariable(Variable *par_Variable, int row, bool arg_updateAlways,
                          union BB_VARI loc_rawValue, enum BB_DATA_TYPES loc_dataType);
    size_t m_BufferSize;
    int m_Size;
    int m_SizeExisting;
    int *m_Vids;
    enum BB_DATA_TYPES *m_Types;
    union BB_VARI *m_Values;
    int *m_Pos;
};

#endif // TEXTTABLEMODEL_H

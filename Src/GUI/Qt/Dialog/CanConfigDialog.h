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


#ifndef CANCONFIGDIALOG_H
#define CANCONFIGDIALOG_H

#include <QDialog>
#include <QTreeView>
#include <QAbstractItemModel>

#include "Dialog.h"
#include "CanConfigSearchVariableDialog.h"

extern "C" {
#include "Blackboard.h"
}

namespace Ui {
class CanConfigDialog;
}

enum ByteOrder {LsbFirst, MsbFirst};

class CanConfigElement
{
public:
    virtual ~CanConfigElement();
    enum Type {Base, Server, Variant, Object, Signal};
    virtual CanConfigElement *child(int par_Number) = 0;
    virtual int childCount() = 0;
    virtual int columnCount() = 0;
    virtual QVariant data(int column) = 0;
    virtual CanConfigElement *parent() = 0;
    virtual int childNumber() = 0;
    virtual bool setData(int column, const QVariant &value) = 0;
    virtual enum Type GetType() = 0;
    virtual void FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex Index) = 0;
    virtual void StoreDialogTab(Ui::CanConfigDialog *ui) = 0;

    virtual void AddNewChild(int row) = 0;
    virtual void AddChild (int row, CanConfigElement *Child) = 0;
    virtual void RemoveChild(int row) = 0;
    virtual QString GetName() = 0;
    virtual void SetName(QString Name) = 0;
};

class CanConfigObject;

class CanConfigSignal : public CanConfigElement
{
public:
    CanConfigSignal(CanConfigObject *par_ParentObject, QString par_Name = QString());
    ~CanConfigSignal();
    bool ReadFromIni(int par_VariantNr, int par_ObjectNr, int par_SignalNr, int par_Fd);
    void WriteToIni(int par_VariantNr, int par_ObjectNr, int par_SignalNr, int par_Fd, bool par_Delete = false);

    CanConfigElement *child(int par_Number);
    int childCount();
    int columnCount();
    QVariant data(int par_Column);
    CanConfigElement *parent();
    int childNumber();
    bool setData(int par_Column, const QVariant &par_Value);
    enum Type GetType();
    void FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index);
    void StoreDialogTab(Ui::CanConfigDialog *ui);
    void AddNewChild(int par_Row);
    void AddChild (int par_Row, CanConfigElement *par_Child);
    void RemoveChild(int par_Row);
    void SetParent(CanConfigObject *par_Parent);

    QString GetName() { return m_Name; }
    void SetName (QString par_Name) { m_Name = par_Name; }
    bool BitInsideSignal(int par_BitPos, enum ByteOrder par_ObjectByteOrder, int par_ObjectSize, bool *ret_FirstBit = nullptr, bool *ret_LastBit = nullptr);

    int GetStartBitAddress() { return m_StartBitAddress; }
    int GetBitSize() { return m_BitSize; }

    enum ByteOrder GetByteOrder() { return m_ByteOrder; }

private:
    QString m_Name;
    QString m_Description;
    QString m_Unit;
    enum ConversionType {MultiplyFactorAddOffset, AddOffsetMultiplyFactor, Curve, Equation} m_ConvertionType;
    union FloatOrInt64 m_ConversionFactor;
    int m_ConversionFactorType;
    union FloatOrInt64 m_ConversionOffset;
    int m_ConversionOffsetType;
    QString m_ConversionEquationOrCurve;
    int m_StartBitAddress;
    int m_BitSize;
    enum ByteOrder m_ByteOrder;
    enum Sign {Signed, Unsigned, Float} m_Sign;
    bool m_StartValueEnable;
    union FloatOrInt64 m_StartValue;
    int m_StartValueType;
    int m_BlackboardDataType;
    enum SignalType {Normal, Multiplexed, MultiplexedBy } m_SignalType;
    int m_MultiplexedStartBit;
    int m_MultiplexedBitSize;
    int m_MultiplexedValue;
    QString m_MultiplexedBy;

    CanConfigObject *m_ParentObject;
};

class CanConfigVariant;

class CanConfigObject : public CanConfigElement
{
public:
    CanConfigObject(CanConfigVariant *par_ParentVariant, QString par_Name = QString());
    CanConfigObject(const CanConfigObject &par_Object);   // copy constructor
    ~CanConfigObject();
    bool ReadFromIni(int par_VariantNr, int par_ObjectNr, int par_Fd);
    void WriteToIni(int par_VariantNr, int par_ObjectNr, int par_Fd, bool par_Delete = false);

    CanConfigElement *child(int par_Number);
    int childCount();
    int columnCount();
    QVariant data(int par_Column);
    CanConfigElement *parent();
    int childNumber();
    bool setData(int column, const QVariant &value);
    enum Type GetType();
    void FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index);
    void StoreDialogTab(Ui::CanConfigDialog *ui);
    void AddNewChild(int par_Row);
    QString GenerateUniqueName (QString par_Name);
    void AddChild (int par_Row, CanConfigElement *par_Child);
    void RemoveChild(int par_Row);
    void SetParent(CanConfigVariant *par_Parent);

    QString GetName() { return m_Name; }
    QString GetNameConst() const { return m_Name; }
    void SetName (QString par_Name) { m_Name = par_Name; }

    int GetIndexOfChild(CanConfigSignal *par_Signal);

    int GetSize() { return m_Size; }
    void SetSize(int par_Size) { m_Size = par_Size; }

    int GetId() const { return m_Identifier; }
    QString GetIdFormula() { return m_VariableId; }
    void SetIdFormula(QString &par_Formula) {  m_VariableId = par_Formula; }

    bool BitRateSwitch() { return m_BitRateSwitch; }

    CanConfigSignal *BitInsideSignal(int par_BitPos, bool *ret_FirstBit = nullptr, bool *ret_LasrBit = nullptr);
    QColor GetColorOfSignal(CanConfigSignal *par_Signal);

    QList<CanConfigSignal*> SearchSignal(const QRegularExpression &par_RegExp);

    enum Type {Normal, Multiplexed, J1939, J1939_Multi_PG, J1939_C_PG};
    enum Type GetObjectType() {
        return m_Type;
    }
    enum Direction {Write, Read, WriteVariableId};
    enum Direction GetDir() {
        return m_Direction;
    }
    QList<int> Get_C_PGs() {
        return m_C_PG_Ids;
    }
    void Set_C_PGs(QList<int> &par_C_PGs) {
        m_C_PG_Ids = par_C_PGs;
    }
    bool CheckExistanceOfAll_C_PGs();

private:
    QString m_Name;
    QString m_VariableId;
    QString m_Description;
    int m_Identifier;
    bool m_ExtendedId;
    bool m_BitRateSwitch;
    bool m_FDFrameFormatSwitch;
    int m_Size;
public:
    enum ByteOrder m_ByteOrder;  // only for display
private:
    enum Direction m_Direction;
    enum TransmitEventType {Cyclic, Equation} m_TransmitEventType;
    QString m_TransmitCycles;
    QString m_TransmitDelay;
    QString m_TransmitEquation;
    enum Type m_Type;
    // wenn Type == Multiplexed
    int m_MultiplexerStartBit;
    int m_MultiplexerBitSize;
    int m_MultiplexerValue;
    // wenn Type == J1939
public:
    bool m_VariableDlc;
    QString m_VariableDlcVariableName;
    enum DstAddress {Fixed, Variable} m_DestAddressType;
    // wenn DestAddressType == Fixed
    unsigned int m_FixedDestAddress;
    // wenn DestAddressType == Variable
    QString m_VariableDestAddressVariableName;
    unsigned int m_VariableDestAddressInitValue;
private:
    uint64_t m_ObjectInitValue;

    QList<CanConfigSignal*> m_Signals;
    QList<int> m_C_PG_Ids;   // if m_Type == J1939_Multi_PG

    QList<QString> m_AdditionalVariables;
    QList<QString> m_AdditionalEquationBefore;
    QList<QString> m_AdditionalEquationBehind;

    CanConfigVariant *m_ParentVariant;
};

class CanConfigServer;

class CanConfigVariant : public CanConfigElement
{
public:
    CanConfigVariant(CanConfigServer *par_ParentBase, QString par_Name = QString());
    CanConfigVariant(const CanConfigVariant &par_Variant);   // copy constructor
    ~CanConfigVariant();
    bool ReadFromIni(int par_VariantNr, int par_Fd);
    void WriteToIni(int par_VariantNr, int par_Fd, bool par_Delete = false);
    void AddToChannel (int par_ChannelNr);
    void RemoveFromChannel (int par_ChannelNr);
    bool IsConnectedToChannel (int par_ChannelNr);

    CanConfigElement *child(int par_Number);
    int childCount();
    int columnCount();
    QVariant data(int par_Column);
    CanConfigElement *parent();
    int childNumber();
    bool setData(int par_Column, const QVariant &par_Value);
    enum Type GetType();
    void FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index);
    void StoreDialogTab(Ui::CanConfigDialog *ui);
    void AddNewChild(int par_Row);
    QString GenerateUniqueName (QString par_Name);
    void AddChild (int par_Row, CanConfigElement *par_Child);
    void RemoveChild(int par_Row);
    void SetParent(CanConfigServer *par_Parent);

    QString GetName() { return m_Name; }
    void SetName (QString par_Name) { m_Name = par_Name; }
    QString GetDescription() { return m_Description; }

    int GetIndexOfChild(CanConfigObject *par_Object);

    void SortObjectsByIdentifier();
    void SortObjectsByName();

    QList<CanConfigSignal*> SearchSignal(const QRegularExpression &par_RegExp);

    QString m_Name;
    QString m_Description;
    int m_BaudRate;
    double m_SamplePoint;
    bool m_CanFdEnable;
    int m_DataBaudRate;
    double m_DataSamplePoint;
    bool m_EnableJ1939;
    bool m_EnableJ1939Addresses[4];
    int m_J1939Addresses[4];

    enum ControlBlackboardName {PrefixId, PrefixObjName, NoPrefixObjName, PrefixVarObjName, NoPrefixVarObjName} m_ControlBlackboardName;

    QList<CanConfigObject*> m_Objects;

    CanConfigServer *m_ParentServer;

    QList<int> ConnectToChannelNrs;
};


class CanConfigBase;

class CanConfigServer : public CanConfigElement
{
public:
    CanConfigServer(CanConfigBase *par_ParentBase, bool par_OnlyUsedVariants = false);
    ~CanConfigServer();
    void ReadFromIni();
    void WriteToIni(bool par_Delete = false);

    int m_NumberOfChannels;
    bool m_DoNotUseUnits;
    bool m_OnlyUsedVariants;
    bool m_RestartCanImmediately;

    bool m_EnableGatewayDeviceDriver;
    int m_EnableGatewayVirtualDeviceDriver;  // 0 -> off, 1 -> on, 2 -> equation
    int m_EnableGatewayRealDeviceDriver;   // 0 -> off, 1 -> on, 2 -> equation
    QString m_EnableGatewayirtualDeviceDriverEqu;
    QString m_EnableGatewayRealDeviceDriverEqu;

    CanConfigElement *child(int par_Number);
    int childCount();
    int columnCount();
    QVariant data(int par_Column);
    CanConfigElement *parent();
    int childNumber();
    bool setData(int par_Column, const QVariant &par_Value);
    enum Type GetType();
    void FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index);
    void UpdateChannelMapping(Ui::CanConfigDialog *ui);
    void StoreDialogTab(Ui::CanConfigDialog *ui);
    void AddNewChild(int par_Row);
    QString GenerateUniqueName (QString par_Name);
    void AddChild (int par_Row, CanConfigElement *par_Child);
    void RemoveChild(int par_Row);

    QString GetName() { return QString(); }
    void SetName (QString par_Name) { Q_UNUSED(par_Name);}

    int GetIndexOfChild(CanConfigVariant *par_Variant);

    void AddVariantToChannel(int par_ChannelNr, CanConfigVariant *par_Variant);
    void RemoveVariantFromChannel(int par_IndexNr, int par_ChannelNr);

    QList<CanConfigSignal*> SearchSignal(const QRegularExpression &par_RegExp);

private:
    int GetCanControllerCountFromIni (char *par_Section, int par_Fd);
    void SetCanControllerCountToIni (char *par_Section, int par_Fd, int par_CanControllerCount);
    QList<CanConfigVariant*> m_Variants;

    QMap<int, CanConfigVariant*> m_ChannelVariant;
    QList<int> m_ChannelStartupState;

    CanConfigBase *m_ParentBase;
};


class CanConfigBase : public CanConfigElement
{
public:
    CanConfigBase();
    CanConfigBase(bool par_OnlyUsedVariants = false);
    ~CanConfigBase();
    void ReadFromIni();
    void WriteToIni(bool par_Delete = false);

    int GetNumberOfChannels();

    CanConfigServer *m_Server;

    CanConfigElement *child(int par_Number);
    int childCount();
    int columnCount();
    QVariant data(int par_Column);
    CanConfigElement *parent();
    int childNumber();
    bool setData(int par_Column, const QVariant &par_Value);
    enum Type GetType();
    void FillDialogTab(Ui::CanConfigDialog *ui, QModelIndex par_Index);
    void StoreDialogTab(Ui::CanConfigDialog *ui);
    void AddNewChild(int par_Row);
    void AddChild (int par_Row, CanConfigElement *par_Child);
    void RemoveChild(int par_Row);

    QString GetName() { return QString(); }
    void SetName (QString par_Name) { Q_UNUSED(par_Name); }

    int GetIndexOfChild(CanConfigServer *par_Server);

    bool ShouldRestartCanImmediately();
};


class CanConfigTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    CanConfigTreeModel (CanConfigBase *par_CanConfig,
                        QObject *par_Parent = nullptr);
    //~CanConfigTreeModel();

    QVariant data(const QModelIndex &par_Index, int par_Role) const Q_DECL_OVERRIDE;
    QVariant headerData(int par_Section, Qt::Orientation par_Orientation,
                        int par_Role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int par_Row, int par_Column,
                      const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &par_Index) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;

    Qt::ItemFlags flags(const QModelIndex &par_Index) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &par_Index, const QVariant &par_Value,
                 int par_Role = Qt::EditRole) Q_DECL_OVERRIDE;

    bool insertRow(int par_Row, const QModelIndex &par_Parent = QModelIndex());
    bool insertRow(int par_Row, CanConfigElement *par_NewElem, const QModelIndex &par_Parent = QModelIndex());
    bool removeRow(int par_Row, const QModelIndex &par_Parent = QModelIndex());

    CanConfigElement *GetItem(const QModelIndex &par_Index) const;

    QModelIndex GetListViewRootIndex();

    QModelIndex SeachNextSignal(QModelIndex &par_BaseIndex, QModelIndex &par_StartIndex, QRegularExpression *par_RegExp, int par_StartRow = 0);

    void SortObjectsById(QModelIndex &par_Index);
    void SortObjectsByName(QModelIndex &par_Index);
private:

    CanConfigElement *m_RootItem;
};

class CanConfigTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit CanConfigTreeView(QWidget *par_Parent = nullptr);
    QModelIndexList GetSelectedIndexes();
    void resizeEvent(QResizeEvent *par_Event) Q_DECL_OVERRIDE;
signals:
    void selectionChangedSignal(const QItemSelection &selected, const QItemSelection &deselected);
public slots:
    void selectionChanged(const QItemSelection &par_Selected, const QItemSelection &par_Deselected) Q_DECL_OVERRIDE;
};



class CanObjectModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    CanObjectModel(QObject *par_Parent = nullptr);

    void SetCanObject(CanConfigObject *par_Object);

    int rowCount(const QModelIndex &par_Parent = QModelIndex()) const;
    int columnCount(const QModelIndex &par_Parent = QModelIndex()) const;

    QVariant data(const QModelIndex &par_Index, int par_Role = Qt::DisplayRole) const;
    QVariant headerData(int par_Section, Qt::Orientation par_Orientation, int par_Role = Qt::DisplayRole) const;

    QModelIndex GetIndexOf(CanConfigSignal *par_Signal);

    CanConfigObject* GetCanObject() const { return m_Object; }

    QItemSelection GetModelIndexesOfCompleteSignal(CanConfigSignal *par_Signal) const;
    QItemSelection GetModelIndexesOfCompleteSignal(int par_BitNr) const;
private:
    CanConfigObject *m_Object;
};

class CanObjectBitDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    CanObjectBitDelegate(QObject *par_Parent = nullptr);

    void paint(QPainter *par_Painter, const QStyleOptionViewItem &par_Option,
               const QModelIndex &par_Index) const;

    QSize sizeHint(const QStyleOptionViewItem &par_Option,
                   const QModelIndex &index ) const;
};


class CanSignalDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    CanSignalDelegate(QObject *par_Parent = nullptr);

    void paint(QPainter *par_Painter, const QStyleOptionViewItem &par_Option,
               const QModelIndex &par_Index) const;

    QSize sizeHint(const QStyleOptionViewItem &par_Option,
                   const QModelIndex &par_Index ) const;
};


class CanConfigDialog : public Dialog
{
    Q_OBJECT

public:
    explicit CanConfigDialog(QWidget *par_Parent = nullptr);
    ~CanConfigDialog();
    void SaveToIni();

public slots:
    void MappingTreeContextMenuRequestedSlot(const QPoint &par_Pos);
    void selectionChanged(const QItemSelection &par_Selected, const QItemSelection &deselected);
    void CanTreeContextMenuRequestedSlot(const QPoint &par_Pos);
    void AddNewVariantSlot();
    void PasteVariantSlot();
    void ImportVariantSlot();
    void SearchVariableSlot();

    void AddNewObjectSlot();
    void DeleteVarianteSlot();
    void CopyVarianteSlot();
    void PasteObjectSlot();
    void ExportVarianteSlot();
    void ExportSortedVarianteSlot();
    void ImportObjectSlot();
    void AppendVarianteSlot();
    void SwapObjectReadWriteSlot();
    void SwapObjecAndSignaltReadWriteSlot();
    void SortObjectsById();
    void SortObjectsByName();

    void AddNewSignalSlot();
    void DeleteObjectSlot();
    void CopyObjectSlot();
    void PasteSignalSlot();
    void ExportObjectSlot();
    void ImportSignalSlot();

    void DeleteSignalSlot();
    void CopySignalSlot();
    void ExportSignalSlot();
    void AddSignalBeforeSlot();
    void AddSignalBehindSlot();
    void PasteSignalBeforeSlot();
    void PasteSignalBehindSlot();

    void SelectSignalSlot(QModelIndex &par_SignalIndex);

    void accept();

private slots:
    void on_AddVariantToChannelPushButton_clicked();

    void on_RemoveVariantFromPushButton_clicked();

    void on_NumberOfChannelsSpinBox_valueChanged(int par_Value);

    void on_ObjectSignalBitPositionsTableView_pressed(const QModelIndex &par_Index);

    void on_ObjectAllSignalsListView_clicked(const QModelIndex &par_Index);

    void on_ObjectInitValueAll0PushButton_clicked();

    void on_ObjectInitValueAll1PushButton_clicked();

    void on_ObjectTypeJ1939ConfigPushButton_clicked();

    void on_ObjectSizeSpinBox_valueChanged(int par_Value);

    void on_ObjectTypeMultiplexedRadioButton_toggled(bool par_Checked);

    void on_ObjectTypeJ1939RadioButton_toggled(bool par_Checked);

    void on_ObjectByteOrderComboBox_currentTextChanged(const QString &par_Text);

    void on_SignalTypeNormalRadioButton_toggled(bool par_Checked);

    void on_SignalTypeMultiplexedRadioButton_toggled(bool par_Checked);

    void on_SignalTypeMultiplexedByRadioButton_toggled(bool par_Checked);

    void on_EnableGatewayDeviceDriverCheckBox_clicked(bool par_Checked);

    void on_VirtualGatewayComboBox_currentIndexChanged(int par_Index);

    void on_RealGatewayComboBox_currentIndexChanged(int par_Index);

    void on_ObjectBitRateSwitchCheckBox_stateChanged(int par_Value);

    void on_ObjectTypeNormalRadioButton_clicked(bool par_Checked);

    void on_ObjectTypeMultiplexedRadioButton_clicked(bool par_Checked);

    void on_ObjectTypeJ1939RadioButton_clicked(bool par_Checked);

private:
    void createActions ();
    void AddSignalBeforeOrBehind(int par_Offset);
    void PasteSignalBeforeOrBehind(int par_Offset);

    void SetMaxObjectSize(bool par_SetFD, bool par_ResetFD, bool par_SetJ1939, bool par_SetNoJ1939);

    Ui::CanConfigDialog *ui;

    CanConfigBase *m_CanConfig;
    CanConfigTreeModel *m_CanConfigTreeModel;

    CanConfigElement *m_CopyBuffer;

    CanObjectModel *m_CanObjectModel;

    CanConfigSearchVariableDialog *m_SearchDialog;

    QAction *m_AddNewVariantAct;
    QAction *m_PasteVariantAct;
    QAction *m_ImportVariantAct;
    QAction *m_SearchVariableAct;

    QAction *m_AddNewObjectAct;
    QAction *m_DeleteVarianteAct;
    QAction *m_CopyVarianteAct;
    QAction *m_PasteObjectAct;
    QAction *m_ExportVarianteAct;
    QAction *m_ExportSortedVarianteAct;
    QAction *m_ImportObjectAct;
    QAction *m_AppendVarianteAct;
    QAction *m_SwapObjectReadWriteAct;
    QAction *m_SwapObjecAndSignaltReadWriteAct;
    QAction *m_SortOjectsByIdAct;
    QAction *m_SortOjectsByNameAct;

    QAction *m_AddNewSignalAct;
    QAction *m_DeleteObjectAct;
    QAction *m_CopyObjectAct;
    QAction *m_PasteSignalAct;
    QAction *m_ExportObjectAct;
    QAction *m_ImportSignalAct;

    QAction *m_DeleteSignalAct;
    QAction *m_CopySignalAct;
    QAction *m_ExportSignalAct;
    QAction *m_AddBeforeSignalAct;
    QAction *m_AddBehindSignalAct;
    QAction *m_PasteBeforeSignalAct;
    QAction *m_PasteBehindSignalAct;
};

#endif // CANCONFIGDIALOG_H

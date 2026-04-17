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


#ifndef CANMESSAGEWINDOWMODEL_H
#define CANMESSAGEWINDOWMODEL_H

#include <QAbstractItemModel>
#include <QVariant>

#include <float.h>
extern "C" {
#include "ThrowError.h"
#include "CanFifo.h"
#include "ReadCanCfg.h"
}

class CurrentCanConfigObserver : public QObject
{
    Q_OBJECT
public:
    CurrentCanConfigObserver();
    void ConnectTo();
    void CurrentCanConfigChanged();
signals:
    void CurrentCanConfigChangedSignal();
private:
    bool m_InitFlag = false;
};

typedef struct CAN_MESSAGE_LINE_STRUCT {
    // CAN_FD_FIFO_ELEM CanMessage;
    CAN_FIFO_ELEM_HEADER CanMessage;
    uint8_t *DataPtr;
    int DataSize;
    double LastTime;
    double dT;
    double dTMin;
    double dTMax;
    int Counter;
    int ObjectNo;
    bool UpdateNeeded;
} CAN_MESSAGE_LINE;

class CANMessageWindowModel;

class CanMessageElement
{
public:
    CanMessageElement(CANMessageWindowModel *par_Model, CanMessageElement *par_Parent = nullptr);
    enum Type {TREE_ROOT,
               TREE_RAW_MESSAGE_DATA_LINE,
               TREE_MESSAGE_MULTIPLEXER,
               TREE_MUX_MESSAGE,
               TREE_MULTI_C_PG,
               TREE_C_PG,
               TREE_SIGNAL_MULTIPLEXER,
               TREE_MUX_SIGNAL,
               TREE_SIGNAL};
    virtual CanMessageElement* const child(int par_Number) = 0;
    virtual int childCount() = 0;
    CanMessageElement *parent() { return m_Parent; };
    virtual int childNumber(CanMessageElement *par_Child) = 0;
    virtual enum Type GetType() = 0;
    CANMessageWindowModel *GetModel() { return m_Model; }
    bool NeedUpdate() { bool h = m_NeedUpdate; m_NeedUpdate = false; return h; }
    void SetNeedUpdate(double par_TimeStamp) {
        m_NeedUpdate = true; //m_Counter++; m_TimeStamp = par_TimeStamp;
        if (m_Counter) {
            m_DiffTime = par_TimeStamp - m_TimeStamp;
            if (m_DiffTime < m_MinDiffTime) m_MinDiffTime = m_DiffTime;
            else if (m_DiffTime > m_MaxDiffTime) m_MaxDiffTime = m_DiffTime;
        }
        m_Counter++;
        m_TimeStamp = par_TimeStamp;
    }
    double GetTimeStamp() { return m_TimeStamp; }
    uint64_t GetCounter() { return m_Counter; }
    double GetDiffTime() { return m_DiffTime; }
    double GetMinDiffTime() { return m_MinDiffTime; }
    double GetMaxDiffTime() { return m_MaxDiffTime; }
private:
    CanMessageElement *m_Parent;
    CANMessageWindowModel *m_Model;
    bool m_NeedUpdate;
    uint64_t m_Counter;
    double m_TimeStamp;
    double m_DiffTime;
    double m_MinDiffTime;
    double m_MaxDiffTime;
};

class CanMessageMultiplexer;
class CanMessageSignal;
class CanMessageSignalMultiplexer;

class CanMessageBaseObject : public CanMessageElement
{
public:
    CanMessageBaseObject(CANMessageWindowModel *par_Model, int par_ObjectNo, bool par_RawOrConverted, int par_Type, CanMessageElement *par_Parent);
    ~CanMessageBaseObject();
    CanMessageElement * const child(int par_Number) {
        int Number = par_Number;
        if(Number < m_SignalMultiplexers.count()) {
            // it is a signal multiplexer
            if((Number < 0) || (Number >= m_SignalMultiplexers.count())) {
                ThrowError(1, "invalid child position");
            }
            return (CanMessageElement* const)m_SignalMultiplexers.at(Number);
        } else {
            // it is a signal
            Number -= m_SignalMultiplexers.count();
            if((Number < 0) || (Number >= m_Signals.count())) {
                ThrowError(1, "invalid child position");
            }
            return (CanMessageElement* const)m_Signals.at(Number);
        }
    }
    int childCount() { return m_SignalMultiplexers.count() + m_Signals.count(); }
    int childNumber(CanMessageElement *par_Child) {
        switch(par_Child->GetType()) {
        case CanMessageElement::TREE_SIGNAL_MULTIPLEXER:
            return m_SignalMultiplexers.indexOf((CanMessageSignalMultiplexer*)par_Child);
        case CanMessageElement::TREE_SIGNAL:
            return m_SignalMultiplexers.count() + m_Signals.indexOf((CanMessageSignal*)par_Child);
        default:
            return 0;
        }
    }
    enum Type GetType() = 0;
    int GetNumberOfSignalMultiplexers() { return m_SignalMultiplexers.count(); }
    int GetNumberOfSignals() { return m_Signals.count(); }
    CanMessageSignalMultiplexer* GetSignalMultipexer(int par_Index) { return m_SignalMultiplexers.at(par_Index); }
    CanMessageSignal* GetSignal(int par_Index) { return m_Signals.at(par_Index); }
    int GetObjectNo() { return m_ObjectNo; }

    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr,
                        double par_TimeStamp);

    QVariant GetValueString();
    int GetSize() { return m_CanMessage.size; }
    void StoreMessage(CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr);
    void CopyMessagePart(uint8_t *par_Src, int par_Size);
private:
    QList<CanMessageSignalMultiplexer*> m_SignalMultiplexers;
    QList<CanMessageSignal*> m_Signals;
    int m_ObjectNo;
    // it has it own copy of the message
public:
    CAN_FIFO_ELEM_HEADER m_CanMessage;
    uint8_t *m_DataPtr;
    int m_DataSize;
};


class CanMessageMuxObject : public CanMessageBaseObject
{
public:
    CanMessageMuxObject(CANMessageWindowModel *par_Model, int par_MuxValue, int par_ObjectNo, bool par_RawOrConverted, CanMessageElement *par_Parent);
    enum Type GetType() { return TREE_MUX_MESSAGE; }

    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp, int par_MuxValue);

    int GetMuxValue() { return m_MuxValue; }
private:
    int m_MuxValue;
};

class CanMessageCPg : public CanMessageBaseObject
{
public:
    CanMessageCPg(CANMessageWindowModel *par_Model, int par_ObjectNo, bool par_RawOrConverted, CanMessageElement *par_Parent);
    enum Type GetType() { return TREE_C_PG; }
    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp, int TosAndServiceHeaader, int C_PG_Size, int Pos);
private:
};

class CanMessageMultiCPg : public CanMessageElement
{
public:
    CanMessageMultiCPg(CANMessageWindowModel *par_Model, int par_ObjectNo, bool par_RawOrConverted, CanMessageElement *par_Parent);
    ~CanMessageMultiCPg();
    CanMessageElement* const child(int par_Number) {
        if((par_Number < 0) || (par_Number >= m_CPgs.count())) {
            ThrowError(1, "invalid child position");
        }
        return (CanMessageElement* const)m_CPgs.at(par_Number);
    }
    int childCount() { return m_CPgs.count(); }
    int childNumber(CanMessageElement *par_Child) {
        switch(par_Child->GetType()) {
        case CanMessageElement::TREE_C_PG:
            return m_CPgs.indexOf((CanMessageCPg*)par_Child);
        default:
            return 0;
        }
    }
    enum Type GetType() { return TREE_MULTI_C_PG; }
    int GetObjectlNo() { return m_ObjectNo; }
    CanMessageCPg* GetCPgObject(int par_Index) { return m_CPgs.at(par_Index); }
    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_MESSAGE_LINE *par_CanMessageLines);
    int GetMuxValue() { return m_CurrentMuxValue; }

private:
    QList<CanMessageCPg*> m_CPgs;
    int m_ObjectNo;
    int m_CurrentMuxValue;
};

class CanMessageRawDataLine : public CanMessageElement
{
public:
    CanMessageRawDataLine(CANMessageWindowModel *par_Model, int par_MessageLineNo, bool par_RawOrConverted, CanMessageElement *par_Parent);
    ~CanMessageRawDataLine();
    CanMessageElement * const child(int par_Number) {
        if(par_Number < m_MultiCpg.count()) {
            // it is a multi PG object multiplexer
            if((par_Number < 0) || (par_Number >= m_MultiCpg.count())) {
                ThrowError(1, "invalid child position");
            }
            return (CanMessageElement* const)m_MultiCpg.at(par_Number);
        } else {
            int Number = par_Number - m_MultiCpg.count();
            if(Number < m_Multiplexers.count()) {
                // it is a object multiplexer
                if((Number < 0) || (Number >= m_Multiplexers.count())) {
                    ThrowError(1, "invalid child position");
                }
                return (CanMessageElement* const)m_Multiplexers.at(par_Number);
            } else {
                int Number = par_Number - m_Multiplexers.count();
                if(Number < m_SignalMultiplexers.count()) {
                    // it is a signal multiplexer
                    if((Number < 0) || (Number >= m_SignalMultiplexers.count())) {
                        ThrowError(1, "invalid child position");
                    }
                    return (CanMessageElement* const)m_SignalMultiplexers.at(Number);
                } else {
                    // it is a signal
                    Number -= m_SignalMultiplexers.count();
                    if((Number < 0) || (Number >= m_Signals.count())) {
                        ThrowError(1, "invalid child position");
                    }
                    return (CanMessageElement* const)m_Signals.at(Number);
                }
            }
        }
    }
    int childCount() { return m_MultiCpg.count() + m_Multiplexers.count() + m_SignalMultiplexers.count() + m_Signals.count(); }
    int childNumber(CanMessageElement *par_Child) {
        switch(par_Child->GetType()) {
        case CanMessageElement::TREE_MULTI_C_PG:
            return m_MultiCpg.indexOf((CanMessageMultiCPg*)par_Child);
        case CanMessageElement::TREE_MESSAGE_MULTIPLEXER:
            return m_MultiCpg.count() + m_Multiplexers.indexOf((CanMessageMultiplexer*)par_Child);
        case CanMessageElement::TREE_SIGNAL_MULTIPLEXER:
            return m_MultiCpg.count() + m_Multiplexers.count() + m_SignalMultiplexers.indexOf((CanMessageSignalMultiplexer*)par_Child);
        case CanMessageElement::TREE_SIGNAL:
            return m_MultiCpg.count() + m_Multiplexers.count() + m_SignalMultiplexers.count() + m_Signals.indexOf((CanMessageSignal*)par_Child);
        default:
            return 0;
        }
    }
    enum Type GetType() { return TREE_RAW_MESSAGE_DATA_LINE; }
    int GetNumberOfMultiPgMultipexers() { return m_MultiCpg.count(); }
    int GetNumberOfObjectMultipexers() { return m_Multiplexers.count(); }
    int GetNumberOfSignalMultiplexers() { return m_SignalMultiplexers.count(); }
    int GetNumberOfSignals() { return m_Signals.count(); }
    CanMessageMultiCPg* GetMultiPgMultiplexer(int par_Index) { return m_MultiCpg.at(par_Index); }
    CanMessageMultiplexer* GetObjectMultipexer(int par_Index) { return m_Multiplexers.at(par_Index); }
    CanMessageSignalMultiplexer* GetSignalMultipexer(int par_Index) { return m_SignalMultiplexers.at(par_Index); }
    CanMessageSignal* GetSignal(int par_Index) { return m_Signals.at(par_Index); }
    int GetObjectNo() { return m_ObjectNo; }

    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_MESSAGE_LINE *par_CanMessageLines);
private:
    QList<CanMessageMultiCPg*> m_MultiCpg;  // this can have only one element
    QList<CanMessageMultiplexer*> m_Multiplexers;
    QList<CanMessageSignalMultiplexer*> m_SignalMultiplexers;
    QList<CanMessageSignal*> m_Signals;
    int m_ObjectNo;
};

class CanMessageMultiplexer : public CanMessageElement
{
public:
    CanMessageMultiplexer(CANMessageWindowModel *par_Model, int par_ObjectNo, bool par_RawOrConverted, CanMessageElement *par_Parent);
    ~CanMessageMultiplexer();
    CanMessageElement* const child(int par_Number) {
        if((par_Number < 0) || (par_Number >= m_MuxObjects.count())) {
            ThrowError(1, "invalid child position");
        }
        return (CanMessageElement* const)m_MuxObjects.at(par_Number);
    }
    int childCount() { return m_MuxObjects.count(); }
    int childNumber(CanMessageElement *par_Child) {
        switch(par_Child->GetType()) {
        case CanMessageElement::TREE_RAW_MESSAGE_DATA_LINE:
            return m_MuxObjects.indexOf((CanMessageMuxObject*)par_Child);
        default:
            return 0;
        }
    }
    enum Type GetType() { return TREE_MESSAGE_MULTIPLEXER; }
    int GetObjectlNo() { return m_ObjectNo; }
    CanMessageMuxObject* GetMuxObject(int par_Index) { return m_MuxObjects.at(par_Index); }
    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp);
    int GetMuxValue() { return m_CurrentMuxValue; }

private:
    QList<CanMessageMuxObject*> m_MuxObjects;
    int m_ObjectNo;
    int m_CurrentMuxValue;
};

class CanMessageSignal : public CanMessageElement
{
public:
    CanMessageSignal(CANMessageWindowModel *par_Model, int par_ObjectNo, int par_SignalNo, bool par_RawOrConverted, CanMessageElement *par_Parent) :
        CanMessageElement(par_Model, par_Parent) {
        if ((par_ObjectNo < 0) || (par_SignalNo < 0)) {
            ThrowError(1, "par_ObjectNo = %i or par_SignalNo = %i should never be negative", par_ObjectNo, par_SignalNo);
        }
        m_ObjectNo = par_ObjectNo;
        m_SignalNo = par_SignalNo;
        m_RawOrConverted = par_RawOrConverted;
        m_Type = FLOAT_OR_INT_64_TYPE_INVALID;
        m_ConvertedType = FLOAT_OR_INT_64_TYPE_INVALID;
    }

    CanMessageElement* const child(int par_Number) { return nullptr; }
    int childCount() { return 0; }
    int childNumber(CanMessageElement *par_Child) {
        return 0;
    }
    enum Type GetType() { return TREE_SIGNAL; }
    int GetObjectNo() { return m_ObjectNo; }
    int GetSignalNo() { return m_SignalNo; }
    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double pat_TimeStamp);
    QVariant GetValueString(NEW_CAN_SERVER_CONFIG* par_CanConfig);
private:
    int NonInvertedConvertSignal (NEW_CAN_SERVER_CONFIG *csc, int o_pos, int s_pos);
    int InvertedConvertSignal (NEW_CAN_SERVER_CONFIG *csc, int o_pos, int s_pos);
    int ConvertSignal(NEW_CAN_SERVER_CONFIG *csc, int o_pos, int s_pos);
    int m_ObjectNo;
    int m_SignalNo;
    union FloatOrInt64 m_Value;
    int m_Type;
    union FloatOrInt64 m_ConvertedValue;
    int m_ConvertedType;
    bool m_RawOrConverted;
};

class CanMessageMuxSignal : public CanMessageSignal
{
public:
    CanMessageMuxSignal(CANMessageWindowModel *par_Model, int par_MuxValue, int par_ObjectNo, int par_SignalNo, bool par_RawOrConverted, CanMessageElement *par_Parent) :
        CanMessageSignal(par_Model, par_ObjectNo, par_SignalNo, par_RawOrConverted, par_Parent) {
        m_MuxValue = par_MuxValue;
    }
    enum Type GetType() { return TREE_MUX_SIGNAL; }
    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp, int par_MuxValue);
private:
    int m_MuxValue;
};

class CanMessageSignalMultiplexer : public CanMessageElement
{
public:
    CanMessageSignalMultiplexer(CANMessageWindowModel *par_Model, int par_ObjectNo, int par_SignalNo, bool par_RawOrConverted, CanMessageElement *par_Parent);
    ~CanMessageSignalMultiplexer() {
        foreach(CanMessageMuxSignal *p, m_MuxSignals) delete p;
    }
    CanMessageElement* const child(int par_Number) {
        if((par_Number < 0) || (par_Number >= m_MuxSignals.count())) {
            ThrowError(1, "invalid child position");
        }
        return (CanMessageElement* const)m_MuxSignals.at(par_Number);
    }
    int childCount() { return m_MuxSignals.count(); }
    int childNumber(CanMessageElement *par_Child) {
        switch(par_Child->GetType()) {
        case CanMessageElement::TREE_SIGNAL:
            return m_MuxSignals.indexOf((CanMessageMuxSignal*)par_Child);
        default:
            return 0;
        }
    }
    enum Type GetType() { return TREE_SIGNAL_MULTIPLEXER; }
    CanMessageMuxSignal* GetMuxSignal(int par_Index) { return m_MuxSignals.at(par_Index); }

    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp);
    int GetMuxValue() { return m_CurrentMuxValue; }

private:
    QList<CanMessageMuxSignal*> m_MuxSignals;
    int m_ObjectNo;
    int m_SignalNo;
    int m_CurrentMuxValue;
};

class CanMessageRoot : public CanMessageElement
{
public:
    CanMessageRoot(CANMessageWindowModel *par_Model) :
        CanMessageElement(par_Model) {
    }
    ~CanMessageRoot() {
        foreach(CanMessageRawDataLine *p, m_RawDataLines) delete p;
    }
    CanMessageElement* const child(int par_Number) {
        if((par_Number < 0) || (par_Number >= m_RawDataLines.count())) {
            ThrowError(1, "invalid child position");
        }
        return (CanMessageElement* const)m_RawDataLines.at(par_Number);
    }
    int childCount() { return m_RawDataLines.count(); }
    int childNumber(CanMessageElement *par_Child) {
        switch(par_Child->GetType()) {
        case CanMessageElement::TREE_RAW_MESSAGE_DATA_LINE:
            return m_RawDataLines.indexOf((CanMessageRawDataLine*)par_Child);
        default:
            return 0;
        }
    }
    enum Type GetType() { return TREE_ROOT; }
    void AddNewMessageAt(int par_Index, bool par_RawOrConverted) {
        m_RawDataLines.insert(par_Index, new CanMessageRawDataLine(GetModel(), par_Index, par_RawOrConverted, this));
    }
    void AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_MESSAGE_LINE *par_CanMessageLines, int par_Index) {
        m_RawDataLines.at(par_Index)->AnalyzeMessage(par_CanConfig, par_CanMessageLines + par_Index);
    }

private:
    QList<CanMessageRawDataLine*> m_RawDataLines;
};

class CANMessageTreeView;

// An instance of this model can use only used by one widget instance!
class CANMessageWindowModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum ColumnType {COLUMN_TYPE_COUNT,
                     COLUMN_TYPE_TIME,
                     COLUMN_TYPE_DIFF_TIME,
                     COLUMN_TYPE_MIN_MAX_DIFF_TIME,
                     COLUMN_TYPE_DIR,
                     COLUMN_TYPE_CHANNEL,
                     COLUMN_TYPE_IDENTIFIER,
                     COLUMN_TYPE_TYPE,
                     COLUMN_TYPE_SIZE,
                     COLUMN_TYPE_NAME,
                     COLUMN_TYPE_DATA};
    CANMessageWindowModel(double par_AbtastPeriodeInMs, bool par_DecodeFlag,
                          QObject *parent = nullptr);
    ~CANMessageWindowModel() Q_DECL_OVERRIDE;

    //void AddNewCANMessages (int par_NumberOfMessages, CAN_FD_FIFO_ELEM *par_CANMessages);
    void AddNewCANMessages (int par_NumberOfMessages, CAN_FIFO_ELEM_HEADER *par_CANMessages, uint8_t **DataPtrs);

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE  { Q_UNUSED(parent) return 11; }

    void UpdateViews(CANMessageTreeView *par_View,
                     int par_VisibleLeftColumn, int par_VisibleRightColumn, int par_VisibleTopRow, int par_VisibleButtomRow);

    void ClearAll(bool par_DecodeFlag, bool par_DelCanCfgFlag = false);
    void SetColumnFlags(bool par_DisplayColumnCounterFlag,
                        bool par_DisplayColumnTimeAbsoluteFlag,
                        bool par_DisplayColumnTimeDiffFlag,
                        bool par_DisplayColumnTimeDiffMinMaxFlag) {
        m_DisplayColumnCounterFlag = par_DisplayColumnCounterFlag;
        m_DisplayColumnTimeAbsoluteFlag = par_DisplayColumnTimeAbsoluteFlag;
        m_DisplayColumnTimeDiffFlag = par_DisplayColumnTimeDiffFlag;
        m_DisplayColumnTimeDiffMinMaxFlag = par_DisplayColumnTimeDiffMinMaxFlag;
        m_DisplayFirst4ColumnFlag = m_DisplayColumnCounterFlag && m_DisplayColumnTimeAbsoluteFlag &&
                                    m_DisplayColumnTimeDiffFlag && m_DisplayColumnTimeDiffMinMaxFlag;
    }

    CAN_MESSAGE_LINE *GetCanMessageLines() { return m_CanMessageLines; }
    int GetCanMessageRowCount() { return m_CanMessageRowCount; }
    NEW_CAN_SERVER_CONFIG* GetCanServerConfig() { return m_CanServerConfig; }

    int GetObjectPos(int par_messageLineNo) { return m_CanMessageLines[par_messageLineNo].ObjectNo; }
    bool CheckIfInsideCanConfig(int par_LineNo) const;

public slots:
    void UpdateCanServerConfigSlot();

private:
    void DeleteAllRawMessageLines();
    void TimestampCalcLine (CAN_MESSAGE_LINE *pcml);
    double TimestampCalc (CAN_FIFO_ELEM_HEADER *pCanMessage);
    bool BinarySearchLowestMostChannelId(CAN_FIFO_ELEM_HEADER *par_Message, int *ret_Pos);
    bool CheckIdList();
    void UpdateOneRowCounterTime(int par_Row, QModelIndex &par_ParentIndex,
                                 int par_VisibleLeftColumn, int par_VisibleRightColumn);
    void UpdateOneRowSignal(int par_Row, QModelIndex &par_ParentIndex,
                            int par_VisibleLeftColumn, int par_VisibleRightColumn);
    void UpdateOneRowSignalMultiplexer(CANMessageTreeView *par_View, int par_Row, CanMessageSignalMultiplexer* par_SignalMultiplexer,
                                       QModelIndex &par_ParentIndex,
                                       int par_VisibleLeftColumn, int par_VisibleRightColumn);

    CAN_MESSAGE_LINE *m_CanMessageLines;
    int m_CanMessageRowCount;

    double m_AbtastPeriodeInMs;

    NEW_CAN_SERVER_CONFIG *m_CanServerConfig;

    bool m_DecodeFlag;
    bool m_RawOrConverted;

    CanMessageRoot *m_Root;

    bool m_DisplayFirst4ColumnFlag;
    bool m_DisplayColumnCounterFlag;
    bool m_DisplayColumnTimeAbsoluteFlag;
    bool m_DisplayColumnTimeDiffFlag;
    bool m_DisplayColumnTimeDiffMinMaxFlag;

    QVector<int> m_Roles;
};

#endif // CANMESSAGEWINDOWMODEL_H

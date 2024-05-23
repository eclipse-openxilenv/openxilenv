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


#ifndef A2LCALMAPWIDGET_H
#define A2LCALMAPWIDGET_H

#include <QWidget>
#include <QAction>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

#include "MdiWindowWidget.h"

#include "A2LCalMapData.h"
#include "A2LCalMapModel.h"
#include "A2LCalMap3DView.h"
#include "A2LCalMapDelegateEditor.h"

#define READ_FROM_LABEL 1


class A2LCalMapWidget : public MdiWindowWidget
{
    Q_OBJECT
public:
    explicit A2LCalMapWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
    ~A2LCalMapWidget() Q_DECL_OVERRIDE;

    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;

    int NotifiyStartStopProcess (QString par_ProcessName, int par_Flags, int par_Action);
    void NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data);
    int NotifiyGetDataFromLinkAck (void *par_IndexData, int par_FetchDataChannelNo);
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

protected:
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent * event) Q_DECL_OVERRIDE;

public slots:
    void ConfigSlot();
    void PropertiesSlot();
    void SwitchToRawSlot();
    void SwitchToPhysSlot();
    void KeyBindingsSlot();

private slots:
    void ChangeValueByWheelSlot(int delta);
    void SelectionChangedByTable(const QItemSelection &selected, const QItemSelection &deselected);
    void SelectionChangedBy3DView();
    void CurrentChangedByTable(const QModelIndex &current, const QModelIndex &previous);
    virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
    virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;

private:
    QAction *m_ConfigAct;
    QAction *m_PropertiesAct;
    QAction *m_SwitchToRawAct;
    QAction *m_SwitchToPhysAct;
    QAction *m_KeyBindingsAct;

    void UpdateProcessAndCharacteristics();

    void CreateActions (void);
    void ConfigDialog (void);
    void PropertiesDialog ();
    void SelectAll (void);
    void UnselectAll (void);
    int ChangeSelectedValuesWithDialog (int op);
//    int ChangeSelectedMapValues (double NewValue, int op);
    int GetData (void);
    void ViewKeyBinding ();
    void AdjustMinMax (void);

    void SelectOrDeselectMapElements (QModelIndexList &IndexList, int SelectOrDeselect);

    QString m_WindowName;
    QString m_Characteristic;
    QString m_Process;

    A2LCalMap3DView *m_3DView;
    QTableView *m_Table;
    A2LCalMapDelegateEditor *m_DelegateEditor;
    A2LCalMapModel *m_Model;
    QSplitter *m_Splitter;
    QVBoxLayout *m_Layout;

    bool m_UpdateDataRequest;

    int m_SplitterTableView;
    int m_SplitterGraphicView;

};
#endif // A2LCALMAPWIDGET_H

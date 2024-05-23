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


#ifndef CALLIBRATIONTREEWIDGET_H
#define CALLIBRATIONTREEWIDGET_H
#include <QWidget>
#include "MdiWindowWidget.h"
#include "CalibrationTreeView.h"
#include "CalibrationTreeModel.h"
#include "CalibrationTreeProxyModel.h"
extern "C" {
#include "Wildcards.h"
#include "SectionFilter.h"
}

class CalibrationTreeWidget;

class CalibrationTreeWidgetSyncObject : public QObject
{
    Q_OBJECT
public:
    void NotifiyStartStopProcessFromSchedulerThread (int par_UniqueId, void *par_User, int par_Flags, int par_Action);
    void AddCalibrationTreeWidget (CalibrationTreeWidget *par_CalibrationTreeWidget);
    void RemoveCalibrationTreeWidget (CalibrationTreeWidget *par_CalibrationTreeWidget);

signals:
    void NotifiyStartStopProcessFromSchedulerSignal (int par_UniqueId, void *par_User, int par_Flags, int par_Action);
public slots:
    void NotifiyStartStopProcessToWidgetThreadSlot (int par_UniqueId, void *par_User, int par_Flags, int par_Action);

private:
    QList<CalibrationTreeWidget*> m_ListOfAllCalibrationTreeWidget;
};



class CalibrationTreeWidget : public MdiWindowWidget
{
    Q_OBJECT

public:
    explicit CalibrationTreeWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
    ~CalibrationTreeWidget() Q_DECL_OVERRIDE;

    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;  // because F5 key for update

    void OpenConfigDialog ();

    int NotifiyStartStopProcess (int par_Flags, int par_Action);

    int GetPid();
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

private:
    void ShowHideColums();
    void SetColumnWidth();

    CalibrationTreeView *m_View;
    CalibrationTreeModel *m_Model;

    char m_ProcessName[MAX_PATH];
    int m_UniqueNumber;

    int m_Pid;
    DEBUG_INFOS_ASSOCIATED_CONNECTION *m_DebugInfos;

    char m_Filter[512];
    int m_ConstOnly;
    int m_ShowValues;
    int m_ShowAddress;
    int m_ShowType;
    int m_HexOrDec;

    int m_ColumnWidthLabel;
    int m_ColumnWidthValue;
    int m_ColumnWidthAddress;
    int m_ColumnWidthType;

    INCLUDE_EXCLUDE_FILTER *m_IncExcFilter;
    SECTION_ADDR_RANGES_FILTER *m_GlobalSectionFilter;

    bool m_FirstResize;

public slots:
    void ConfigDialogSlot (void);
    void ReloadValuesSlot (void);
    void ReferenceLabelSlot (void);
    void ReferenceLabelUserNameSlot (void);
    void DeReferenceLabelSlot (void);
    void ChangeValuesAllSelectedLabelsSlot (void);
    void ListAllReferencedLabelSlot (void);
    virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
    virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;

};

#endif // CALLIBRATIONTREEWIDGET_H

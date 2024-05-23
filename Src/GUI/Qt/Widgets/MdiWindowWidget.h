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


#ifndef MDIWINDOWWIDGET_H
#define MDIWINDOWWIDGET_H

#include <QWidget>
#include "InterfaceWidgetElement.h"
#include "MdiSubWindow.h"
#include "DialogFrame.h"


class MdiWindowType;

class MdiWindowWidget : public QWidget, public InterfaceWidgetElement
{
    Q_OBJECT
    Q_INTERFACES(InterfaceWidgetElement)
public:
    explicit MdiWindowWidget(QString par_WindowTitle, MdiSubWindow *par_MdiSubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
    ~MdiWindowWidget();

    void SetSubWindow (MdiSubWindow *par_MdiSubWindow, bool par_MinMaxSizeValid = false, const QSize &par_MinSize = QSize(), const QSize &par_MaxSize = QSize());

    void MdiSubWindowResize (int x, int y);
    void MdiSubWindowMove (int x, int y);

    void RenameWindowTo (QString par_WindowTitle);
    QString GetWindowTitle ();
    void SetWindowIcon (QIcon par_WindowIcon);
    QIcon GetWindowIcon ();

    QString GetDefaultNewWindowTitle ();
    MdiWindowType* GetMdiWindowType();
    MdiSubWindow *GetCustomMdiSubwindow ();

    virtual bool readFromIni();
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) = 0;

    QString GetIniSectionPath();

protected:
    void dragEnterEvent(QDragEnterEvent *event) { Q_UNUSED(event) }
    void dragMoveEvent(QDragMoveEvent *event) { Q_UNUSED(event) }
    void dragLeaveEvent(QDragLeaveEvent *event) { Q_UNUSED(event) }
    void dropEvent(QDropEvent *event) { Q_UNUSED(event) }


signals:
    void openStandardDialog(QStringList arg_selectedVariables, bool arg_allVariables = true, bool arg_multiselect = true, QColor arg_color = QColor(), QFont arg_font = QFont(), bool arg_TitleEditable = true);

public slots:
    virtual void CyclicUpdate();
    virtual void setDefaultFocus();
    virtual void openDialog() { emit openStandardDialog(QStringList()); }
    virtual void changeColor(QColor arg_color) = 0;
    virtual void changeFont(QFont arg_font) = 0;
    virtual void changeWindowName(QString arg_name) = 0;
    virtual void changeVariable(QString arg_variable, bool arg_visible) = 0;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) = 0;
    virtual void resetDefaultVariables(QStringList arg_variables) = 0;

private:
    MdiSubWindow *m_MdiSubWindow;
    MdiWindowType *m_Type;
    QString m_WindowTitle;
};

#endif // MDIWINDOWWIDGET_H

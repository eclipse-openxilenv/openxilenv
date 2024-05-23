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


#ifndef MDISUBWINDOW_H
#define MDISUBWINDOW_H

#include <QMdiSubWindow>
#include <QCloseEvent>
#include <QDrag>
#include <QMimeData>

class Sheets;
class MdiWindowWidget;

class MdiSubWindow : public QMdiSubWindow
{
    Q_OBJECT
public:
    MdiSubWindow(Sheets *par_Sheet);
    ~MdiSubWindow() Q_DECL_OVERRIDE;

    void readFromIni(QString &par_Section, int par_Index);
    void writeToIni(QString &par_Section, int par_Index);

    void setWidget (MdiWindowWidget *par_Widget);
    void setNotOwnerOfWidget ();
    bool isOwnerOfWidget ();
    void setOwnerOfWidget (QSize &par_MinSize, QSize &par_MaxSize);
    MdiWindowWidget *NotOwnerOfWidget ();
    MdiWindowWidget *GetSubWindowWidget();

    QString GetWindowTitle();
    QString GetIniSectionPath();

protected:
    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    bool event(QEvent *event) Q_DECL_OVERRIDE;

signals:
    void focusInSubWindow(void);

private:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    bool m_OwnerOfWidget;
    MdiWindowWidget *m_SubWindowWidget;

    Sheets *m_Sheet;

    QRect m_NormalViewGeometry;
    bool m_IsNormalView;
};

#endif // MDISUBWINDOW_H

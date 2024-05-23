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


#ifndef USERCONTROLWIDGET_H
#define USERCONTROLWIDGET_H

#include "MdiWindowWidget.h"
#include "BlackboardObserver.h"

#include "UserControlRoot.h"

#include <QPushButton>
#include <QGridLayout>
#include <QAction>

class UserControlWidget: public MdiWindowWidget {
   Q_OBJECT
public:
   UserControlWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
   ~UserControlWidget() Q_DECL_OVERRIDE;

   virtual bool writeToIni() Q_DECL_OVERRIDE;
   virtual bool readFromIni() Q_DECL_OVERRIDE;
   virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

   void AddWidget(QWidget *par_Widget, int x, int y, int w, int h);
   void RemoveWidget(QWidget *par_Widget);

protected:
   virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
   virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
   void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
   virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
   virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
   virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
   virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
   virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
   virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;


private slots:
   virtual void CyclicUpdate() Q_DECL_OVERRIDE;
   void blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag);
   void ConfigureSlot();

private:
   void openDialog() Q_DECL_OVERRIDE;

   BlackboardObserverConnection m_ObserverConnection;

   QAction *m_ConfigAct;

   UserControlRoot *m_Root;

   int m_ScriptState;

   QGridLayout *m_Layout;
};

#endif // USERCONTROLWIDGET_H

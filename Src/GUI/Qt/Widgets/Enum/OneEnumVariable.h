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


#ifndef ONEENUMVARIABLE_H
#define ONEENUMVARIABLE_H

#include "OneEnumButton.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QMouseEvent>

class OneEnumVariable : public QWidget
{
    Q_OBJECT
public:
    explicit OneEnumVariable(QString par_VariableName, QWidget *parent = nullptr);
    ~OneEnumVariable();
    QString GetName();
    int GetVid();
    void UpdateButtons();

signals:

public slots:
    void blackboardValueChange(double arg_Value);
    void CyclicUpdate();

protected:
    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent* event);

private:
    void DeleteAllButtons();

    QString m_VariableName;
    int m_Vid;

    QLabel *m_Label;

    QVBoxLayout *m_Layout;
    QScrollArea *m_ScrollArea;
    QWidget *m_ScrollAreaWidget;
    QVBoxLayout *m_ScrollAreaLayout;

    QList<OneEnumButton*> m_Buttons;

    QPoint m_startDragPosition;

};

#endif // ONEENUMVARIABLE_H

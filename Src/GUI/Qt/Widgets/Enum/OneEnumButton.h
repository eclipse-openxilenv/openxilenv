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


#ifndef ONEENUMBUTTON_H
#define ONEENUMBUTTON_H

#include <QPushButton>

class OneEnumVariable;

class OneEnumButton : public QPushButton
{
    Q_OBJECT

public:
    OneEnumButton(QString label, QColor color, double par_StartValue, double par_EndValue, OneEnumVariable *par_Variable, QWidget *parent = nullptr);
    ~OneEnumButton();

public slots:
    void CyclicUpdate(double par_Value);

private slots:
    void blackboardValueChange();

signals:
    void blackboardValueChange(double arg_Value);

private:
    QString buttonLabel;
    QColor buttonColor;
    double m_StartValue;
    double m_EndValue;
protected:
    void paintEvent(QPaintEvent *);
};

#endif // ONEENUMBUTTON_H

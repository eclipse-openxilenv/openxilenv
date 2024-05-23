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


#ifndef A2LCALSINGLETABLEVIEW_H
#define A2LCALSINGLETABLEVIEW_H

#include <QTableView>
#include <QAbstractItemDelegate>

#include "A2LCalSingleModel.h"

class A2LCalSingleTableView : public QTableView {
    Q_OBJECT
public:
    explicit A2LCalSingleTableView(QWidget *arg_parent = nullptr);
    ~A2LCalSingleTableView() Q_DECL_OVERRIDE;

protected:
    void mousePressEvent(QMouseEvent *arg_event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *arg_event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *arg_event) Q_DECL_OVERRIDE;

public slots:
    void editorClosedSlot();
    void columnWidthChangeSlot(int arg_columnFlags, int arg_nameWidth, int arg_valueWidth, int arg_unitWidth);

private:
    void IncOrDecVariable(A2LCalSingleModel *arg_model, int arg_row, double arg_Sign, double arg_stepMultiplier);

    bool m_EditorOpenFlag;

};

#endif // A2LCALSINGLETABLEVIEW_H

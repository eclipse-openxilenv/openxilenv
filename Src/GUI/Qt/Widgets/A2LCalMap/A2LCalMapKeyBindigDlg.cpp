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


#include "A2LCalMapKeyBindigDlg.h"
#include "ui_A2LCalMapKeyBindigDlg.h"

A2LCalMapKeyBindigDlg::A2LCalMapKeyBindigDlg(QWidget *parent) : Dialog(parent),
    ui(new Ui::A2LCalMapKeyBindigDlg)
{
    ui->setupUi(this);
    ui->plainTextEdit->setPlainText("Key:\tMeaning:\n"
                                    "?,h,k:\tOpen this Window\n"
                                    "d:\tReset the view configuration (angels, zoom, ...)\n"
                                    "#:\tAdjust min. max. settings\n"
                                    "Move:\n"
                                    "l:\tMove left\n"
                                    "r:\tMove right\n"
                                    "t:\tMove top\n"
                                    "b:\tMove bottom\n"
                                    "Rotate:\n"
                                    "x:\tRotate X-Axis forward\n"
                                    "X:\tRotate X-Axis backward\n"
                                    "y:\tRotate Y-Axis forward\n"
                                    "Y:\tRotate Y-Axis backward\n"
                                    "z:\tRotate Z-Axis forward\n"
                                    "Z:\tRotate Z-Axis backward\n"
                                    "Scaling:\n"
                                    "q:\tZoom out\n"
                                    "a:\tZoom in\n"
                                    "Picking:\n"
                                    "Cursor:\tMove selected point\n"
                                    "s:\tSelect all points\n"
                                    "u:\tUnselect all points\n"
                                    "Change values:\n"
                                    "i:\tInsert a new value in all selected points\n"
                                    "+:\tAdd a value to all selected points\n"
                                    "-:\tSubtract a value from all selected points\n"
                                    "*:\tMultipy a value with all selected points\n"
                                    "/:\tDivide all selected points with a value\n"
                                    "p:\tAdd an offset to all selected points\n"
                                    "m:\tSubtract an offset to all selected points\n"
                                    "o:\tSpecify the offset for the p and m key\n"
                                    "g:\tGet new Data from Process");
}

A2LCalMapKeyBindigDlg::~A2LCalMapKeyBindigDlg()
{
    delete ui;
}

void A2LCalMapKeyBindigDlg::on_ClosePushButton_clicked()
{
    this->accept();
}

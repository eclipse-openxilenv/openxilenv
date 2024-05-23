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


#ifndef SOFTARDRAGANDDROP_H
#define SOFTARDRAGANDDROP_H

#include <QDrag>
#include <QUrl>

enum DragAndDropDisplayMode {DISPLAY_MODE_UNKNOWN, DISPLAY_MODE_DEC, DISPLAY_MODE_PHYS, DISPLAY_MODE_HEX, DISPLAY_MODE_BIN};
enum DragAndDropAlignment {DISPLAY_UNKNOWN_ALIGNED, DISPLAY_LEFT_ALIGNED, DISPLAY_RIGHT_ALIGNED};

class DragAndDropInfos
{
public:
    DragAndDropInfos(QString par_InfoString = QString());

public:
    void SetName(QString &par_Name);
    QString GetName();
    bool GetMinMaxValue(double *ret_MinValue, double *ret_MaxValue);
    double GetMinValue();
    double GetMaxValue();
    void SetMinMaxValue (double par_MinValue, double par_MaxValue);
    void SetColor (QColor par_Color);
    void SetColor (int par_Color);
    QColor GetColor ();
    bool GetColor (int *ret_Color);
    void SetLineWidth (int par_LineWidth);
    int GetLineWidth();

    void SetDisplayMode(enum DragAndDropDisplayMode par_DisplayType);
    enum DragAndDropDisplayMode GetDisplayMode();
    void SetAlignment(enum DragAndDropAlignment par_Alignment);
    enum DragAndDropAlignment GetAlignment();


    QString GetInfosString();
private:
    QString GetInfo (QString Key);
    void AddInfo (QString Key, QString Value);
    void AddInfo (QString Key, int Value);
    void AddInfo (QString Key, double Value);
    QString InfosString;
    QStringList InfosStringList;
};

QDrag* buildDragObject(QObject *arg_Source, DragAndDropInfos *arg_AdditionalInfos = nullptr);

#endif // SOFTARDRAGANDDROP_H

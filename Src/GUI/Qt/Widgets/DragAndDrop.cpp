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


#include <QLabel>
#include <QMimeData>
#include <QRgb>

#include "DragAndDrop.h"

QDrag* buildDragObject(QObject *arg_Source, DragAndDropInfos *arg_AdditionalInfos) {
    QMimeData *loc_mimeData = new QMimeData();
    loc_mimeData->setText(arg_AdditionalInfos->GetInfosString());
    QLabel *loc_label = new QLabel(arg_AdditionalInfos->GetName());
    QFont loc_font = loc_label->font();
    QFontMetrics FontMetrics(loc_font);
    QRect loc_rect = FontMetrics.boundingRect(arg_AdditionalInfos->GetName());
    loc_label->resize(loc_rect.width(), loc_rect.height());
    QPixmap loc_pixmap(loc_label->size());
    loc_label->render(&loc_pixmap);

    QDrag *loc_drag = new QDrag(arg_Source);
    loc_drag->setMimeData(loc_mimeData);
    loc_drag->setPixmap(loc_pixmap);

    return loc_drag;
}

DragAndDropInfos::DragAndDropInfos(QString par_InfoString)
{
    InfosString = par_InfoString;
}

void DragAndDropInfos::SetName(QString &par_Name)
{
    AddInfo(QString("name"), par_Name);
}

QString DragAndDropInfos::GetName()
{
    return GetInfo(QString("name"));
}

bool DragAndDropInfos::GetMinMaxValue(double *ret_MinValue, double *ret_MaxValue)
{
    QString Text = GetInfo(QString("min_max"));
    QStringList MinMax = Text.split ("/");
    bool Ret1;
    bool Ret2;
    *ret_MinValue = MinMax.first().toDouble (&Ret1);
    if (!Ret1) *ret_MinValue = 0.0;
    *ret_MaxValue = MinMax.last().toDouble (&Ret2);
    if (!Ret2) *ret_MinValue = *ret_MinValue + 1.0;
    if (*ret_MaxValue <= *ret_MinValue) *ret_MaxValue = *ret_MinValue + 1.0;
    return (Ret1 && Ret2);

}

double DragAndDropInfos::GetMinValue()
{
    double MinValue;
    double MaxValue;
    if (!GetMinMaxValue (&MinValue, &MaxValue)) {
        return MinValue;
    }
    return 0.0;
}

double DragAndDropInfos::GetMaxValue()
{
    double MinValue;
    double MaxValue;
    if (!GetMinMaxValue (&MinValue, &MaxValue)) {
        return MaxValue;
    }
    return 1.0;
}

void DragAndDropInfos::SetMinMaxValue(double par_MinValue, double par_MaxValue)
{
    QString MinMax;
    MinMax.append(QString().number(par_MinValue));
    MinMax.append("/");
    MinMax.append(QString().number(par_MaxValue));
    AddInfo(QString("min_max"), MinMax);

}

void DragAndDropInfos::SetColor(QColor par_Color)
{
    AddInfo(QString("color"), static_cast<int>(par_Color.rgb()));
}

void DragAndDropInfos::SetColor(int par_Color)
{
    AddInfo(QString("color"), par_Color);
}

QColor DragAndDropInfos::GetColor()
{
    QString Text = GetInfo(QString("color"));
    bool Ret;
    QRgb Rgb = Text.toUInt (&Ret);
    if (Ret)
        return QColor (Rgb);
    else
        return QColor::Invalid;
}

bool DragAndDropInfos::GetColor(int *ret_Color)
{
    QString Text = GetInfo(QString("color"));
    bool Ret;
    *ret_Color = Text.toInt (&Ret);
    return Ret;
}



void DragAndDropInfos::SetLineWidth(int par_LineWidth)
{
    AddInfo(QString("line_width"), par_LineWidth);
}

int DragAndDropInfos::GetLineWidth()
{
    QString Text = GetInfo(QString("line_width"));
    bool Ok;
    int Ret = Text.toInt (&Ok);
    if (Ok) {
        return Ret;
    } else {
        return -1;
    }
}

void DragAndDropInfos::SetDisplayMode(enum DragAndDropDisplayMode par_DisplayType)
{
    AddInfo(QString("display_mode"), static_cast<int>(par_DisplayType));
}

DragAndDropDisplayMode DragAndDropInfos::GetDisplayMode()
{
    QString Text = GetInfo(QString("display_mode"));
    enum DragAndDropDisplayMode Ret;
    bool Ok;
    Ret = static_cast<enum DragAndDropDisplayMode>(Text.toInt (&Ok));
    if (Ok) {
        return Ret;
    } else {
        return DISPLAY_MODE_UNKNOWN;
    }
}

void DragAndDropInfos::SetAlignment(enum DragAndDropAlignment par_Alignment)
{
    AddInfo(QString("alignment"), static_cast<int>(par_Alignment));
}

enum DragAndDropAlignment DragAndDropInfos::GetAlignment()
{
    QString Text = GetInfo(QString("alignment"));
    enum DragAndDropAlignment Ret;
    bool Ok;
    Ret = static_cast<enum DragAndDropAlignment>(Text.toInt (&Ok));
    if (Ok) {
        return Ret;
    } else {
        return DISPLAY_UNKNOWN_ALIGNED;
    }
}

QString DragAndDropInfos::GetInfosString()
{
    return InfosString;
}

QString DragAndDropInfos::GetInfo(QString Key)
{
    if (InfosStringList.isEmpty()) {
        InfosStringList = InfosString.split(";");
    }
    foreach (QString Info, InfosStringList) {
        QStringList InfoList = Info.split ("=");
        if (InfoList.first().compare(Key) == 0) {
            return InfoList.at(1);
        }
    }
    return QString();
}

void DragAndDropInfos::AddInfo(QString Key, QString Value)
{
    InfosString.append(Key);
    InfosString.append("=");
    InfosString.append(Value);
    InfosString.append(";");
}

void DragAndDropInfos::AddInfo(QString Key, int Value)
{
    AddInfo(Key, QString().number(Value));
}

void DragAndDropInfos::AddInfo(QString Key, double Value)
{
    AddInfo(Key, QString().number(Value));
}

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


#include <Qt>
#include "LampsModel.h"
#include "QtIniFile.h"
extern "C" {
#include "ThrowError.h"
}

ControlLampModel::ControlLampModel(QObject* parent, bool predef, int arg_Fd) : QAbstractListModel(parent)
{
    m_PredefinedCount = 0;
    if (predef) {
        predefinedStencils();
    }
    if (arg_Fd < 0) {
        arg_Fd = ScQt_GetMainFileDescriptor();
    }
    customStancilFromIni(arg_Fd);
    m_hasChanged = false;  // This must done at last
}

ControlLampModel::~ControlLampModel()
{
    foreach (Lamp* lamp, m_values) {
        delete lamp;
    }
}

int ControlLampModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    int Rows = m_values.size();
    return Rows;
}

QVariant ControlLampModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) {
        return QVariant();
    }
    if(index.row() >= m_values.size()) {
        return QVariant();
    }
    if(role == Qt::DisplayRole) {
        return m_values.at(index.row())->m_name;
    } else if(role == Qt::BackgroundRole) {
        if (m_values.at(index.row())->m_predefined) {
            return QColor(Qt::lightGray);
        }
    }
    return QVariant();
}

QVariant ControlLampModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    if(role != Qt::DisplayRole) {
        return QVariant();
    }
    return QVariant();
}

bool ControlLampModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value)
    if(index.isValid() && role == Qt::EditRole) {
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

QList<QPolygonF>* ControlLampModel::getForegroundStancil(const QModelIndex index)
{
    Lamp *loc_lamp = m_values.at(index.row());
    return &loc_lamp->m_polygonForeground;
}

QList<QPolygonF>* ControlLampModel::getBackgroundStancil(const QModelIndex index)
{
    Lamp *loc_lamp = m_values.at(index.row());
    return &loc_lamp->m_polygonBackground;
}

QList<QPolygonF>*ControlLampModel::getForegroundStancil(QString arg_name)
{
    foreach (Lamp *loc_lamp, m_values) {
        if(loc_lamp->m_name.compare(arg_name) == 0){
            return &loc_lamp->m_polygonForeground;
        }
    }
    return nullptr;
}

QList<QPolygonF> *ControlLampModel::getBackgroundStancil(QString arg_name)
{
    foreach (Lamp *loc_lamp, m_values) {
        if(loc_lamp->m_name.compare(arg_name) == 0){
            return &loc_lamp->m_polygonBackground;
        }
    }
    return nullptr;
}

QString ControlLampModel::stancilName(const QModelIndex arg_index)
{
    return m_values.at(arg_index.row())->m_name;
}

QString ControlLampModel::stancilPolygon(const QModelIndex arg_index)
{
    return m_values.at(arg_index.row())->m_polygon;
}

void ControlLampModel::addStancil(const QString arg_name, const QString arg_polygon,
                                  const QList<QPolygonF> arg_polygonForeground, const QList<QPolygonF> arg_polygonBackground)
{
    if((!arg_name.isEmpty()) && ((!arg_polygonForeground.empty()) || (!arg_polygonBackground.empty()))) {
        Lamp *custom = new Lamp;
        custom->m_name = arg_name;
        custom->m_polygon = arg_polygon;

        custom->m_polygonForeground = arg_polygonForeground;
        custom->m_polygonBackground = arg_polygonBackground;
        custom->m_predefined = false;
        m_values.append(custom);
        m_hasChanged = true;

        emit dataChanged(QModelIndex(), QModelIndex());
    }
}

bool ControlLampModel::removeStancil(const QModelIndex arg_index)
{
    int loc_row = arg_index.row();
    if(loc_row < m_values.count()) {
        beginRemoveRows(QModelIndex(), loc_row, loc_row);
        m_values.removeAt(loc_row);
        m_hasChanged = true;
        endRemoveRows();
        return true;
    } else {
        return false;
    }
}

bool ControlLampModel::nameExists(QString arg_name)
{
    for(int i = 0; i < m_values.count(); i++) {
        if(m_values.at(i)->m_name.compare(arg_name) == 0) {
            return true;
        }
    }
    return false;
}

Lamp *ControlLampModel::getStancilInformation(int arg_row)
{
    return m_values.value(arg_row);
}

QModelIndex ControlLampModel::getItemIndex(QString arg_stancilName)
{
    for(int i = 0; i < m_values.count(); ++i) {
        if(arg_stancilName.compare(m_values.at(i)->m_name) == 0) {
            return index(i);
        }
    }
    return QModelIndex();
}

void ControlLampModel::addLamp(Lamp *arg_Lamp)
{
    beginInsertRows(QModelIndex(), m_values.size(), m_values.size());
    m_values.append(arg_Lamp);
    m_hasChanged = true;
    endInsertRows();
}

void ControlLampModel::predefinedStencils()
{
    Lamp *Elem;
    Elem = new Lamp();
    Elem->init("predef plus,"
               "(+0.2/0.4;0.2/0.6;0.4/0.6;0.4/0.8;0.6/0.8;0.6/0.6;0.8/0.6;0.8/0.4;0.6/0.4;0.6/0.2;0.4/0.2;0.4/0.4;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef minus,"
               "(+0.2/0.4;0.2/0.6;0.8/0.6;0.8/0.4;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef face spanner,"
               "(+0.109375/0.296875;"
               "0.453125/0.640625;"
               "0.453125/0.796875;"
               "0.578125/0.921875;"
               "0.625/0.875;"
               "0.546875/0.796875;"
               "0.546875/0.671875;"
               "0.65625/0.5625;"
               "0.78125/0.5625;"
               "0.859375/0.640625;"
               "0.90625/0.59375;"
               "0.78125/0.46875;"
               "0.625/0.46875;"
               "0.28125/0.125;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef oil can,"
               "(+0.06/0.77;"
               "0.28/0.66;"
               "0.38/0.77;"
               "0.73/0.77;"
               "0.81/0.72;"
               "0.92/0.72;"
               "0.96/0.68;"
               "0.96/0.51;"
               "0.92/0.47;"
               "0.82/0.47;"
               "0.73/0.24;"
               "0.33/0.24;"
               "0.24/0.46;"
               "0.03/0.72;)"
             "(-0.84/0.66;"
               "0.92/0.66;"
               "0.92/0.52;"
               "0.84/0.52;)"
             "(+0.03/0.60;"
               "0.01/0.52;"
               "0.03/0.50;"
               "0.05/0.52;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef key 1,"
               "(+0.42/0.08;"
               "0.42/0.11;"
               "0.34/0.12;"
               "0.34/0.17;"
               "0.39/0.17;"
               "0.39/0.22;"
               "0.31/0.22;"
               "0.31/0.27;"
               "0.42/0.27;"
               "0.42/0.58;"
               "0.34/0.58;"
               "0.27/0.65;"
               "0.27/0.85;"
               "0.34/0.92;"
               "0.66/0.92;"
               "0.73/0.85;"
               "0.73/0.65;"
               "0.66/0.58;"
               "0.58/0.58;"
               "0.58/0.08;)"
               "(-0.37/0.65;"
               "0.34/0.68;"
               "0.34/0.82;"
               "0.37/0.85;"
               "0.63/0.85;"
               "0.66/0.82;"
               "0.66/0.68;"
               "0.63/0.65;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef key 2,"
               "(+0.13/0.67;"
               "0.10/0.53;"
               "0.10/0.43;"
               "0.13/0.31;"
               "0.25/0.31;"
               "0.28/0.43;"
               "0.63/0.43;"
               "0.63/0.32;"
               "0.66/0.32;"
               "0.66/0.37;"
               "0.69/0.37;"
               "0.69/0.32;"
               "0.72/0.32;"
               "0.72/0.43;"
               "0.76/0.43;"
               "0.76/0.35;"
               "0.79/0.35;"
               "0.79/0.32;"
               "0.82/0.32;"
               "0.82/0.43;"
               "0.85/0.43;"
               "0.87/0.47;"
               "0.87/0.49;"
               "0.85/0.53;"
               "0.28/0.53;"
               "0.25/0.67;"
               "0.13/0.67;)"
               "(-0.18/0.61;"
               "0.16/0.49;"
               "0.16/0.48;"
               "0.18/0.37;"
               "0.20/0.37;"
               "0.22/0.48;"
               "0.22/0.49;"
               "0.20/0.61;"
               "0.18/0.61;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef cycle,"
               "(+0.5000/0.9000;"
               "0.5626/0.8951;"
               "0.6236/0.8804;"
               "0.6816/0.8564;"
               "0.7351/0.8236;"
               "0.7828/0.7828;"
               "0.8236/0.7351;"
               "0.8564/0.6816;"
               "0.8804/0.6236;"
               "0.8951/0.5626;"
               "0.9000/0.5000;"
               "0.8951/0.4374;"
               "0.8804/0.3764;"
               "0.8564/0.3184;"
               "0.8236/0.2649;"
               "0.7828/0.2172;"
               "0.7351/0.1764;"
               "0.6816/0.1436;"
               "0.6236/0.1196;"
               "0.5626/0.1049;"
               "0.5000/0.1000;"
               "0.4374/0.1049;"
               "0.3764/0.1196;"
               "0.3184/0.1436;"
               "0.2649/0.1764;"
               "0.2172/0.2172;"
               "0.1764/0.2649;"
               "0.1436/0.3184;"
               "0.1196/0.3764;"
               "0.1049/0.4374;"
               "0.1000/0.5000;"
               "0.1049/0.5626;"
               "0.1196/0.6236;"
               "0.1436/0.6816;"
               "0.1764/0.7351;"
               "0.2172/0.7828;"
               "0.2649/0.8236;"
               "0.3184/0.8564;"
               "0.3764/0.8804;"
               "0.4374/0.8951;"
               "0.5000/0.9000;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef arrow right,"
               "(+0.11/0.71;0.53/0.71;0.53/0.90;0.93/0.50;0.53/0.10;0.53/0.29;0.11/0.29)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef arrow left,"
               "(+0.89/0.71;0.47/0.71;0.47/0.90;0.07/0.50;0.47/0.10;0.47/0.29;0.89/0.29)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef arrow up,"
               "(+0.29/0.11;0.29/0.53;0.10/0.53;0.50/0.93;0.90/0.53;0.71/0.53;0.71/0.11)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef arrow down,"
               "(+0.29/0.89;0.29/0.47;0.10/0.47;0.50/0.07;0.90/0.47;0.71/0.47;0.71/0.89)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef rectangle,"
               "(+0.2/0.2;0.2/0.8;0.8/0.8;0.8/0.2;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef buzzer,"
               "(+0.1/0.3;0.3/0.3;0.5/0.1;0.5/0.9;0.3/0.7;0.1/0.7;)"
               "(+0.6/0.8;0.6/0.2;)"
               "(+0.7/0.7;0.7/0.3;)"
               "(+0.8/0.6;0.8/0.4;)", true);
    m_values.append(Elem);
    Elem = new Lamp();
    Elem->init("predef battery,"
               "(+0.2/0.2;0.2/0.7;0.8/0.7;0.8/0.2;)"
               "(+0.3/0.7;0.3/0.8;0.4/0.8;0.4/0.7;)"
               "(+0.6/0.7;0.6/0.8;0.7/0.8;0.7/0.7;)"
               "(-0.3/0.55;0.4/0.55;)"
               "(-0.35/0.5;0.35/0.6;)"
               "(-0.6/0.55;0.7/0.55;)"
               , true);
    m_values.append(Elem);

    Elem = new Lamp();
    Elem->init("predef charging station,"
               "(+0.1/0.2;0.5/0.2;0.5/0.1;0.1/0.1;)"
               "(+0.1/0.25;0.1/0.8;0.2/0.85;0.4/0.85;0.5/0.8;0.5/0.25)"
               //         /            -          /         -        /           -          /
               "(-0.2/0.3; 0.275/0.475; 0.2/0.475; 0.25/0.6; 0.4/0.6; 0.35/0.525; 0.4/0.525; 0.2/0.3)"  // flash
               "(-0.2/0.7;0.2/0.8;0.4/0.8;0.4/0.7;)"
               //          -         /          |           -           |          /          -
               "(+0.5/0.35; 0.7/0.35; 0.725/0.4; 0.725/0.55; 0.775/0.55; 0.775/0.4; 0.725/0.3; 0.5/0.3)"  // cable
               "(+0.6/0.8;0.9/0.8;0.9/0.6;0.85/0.55;0.65/0.55;0.6/0.6)"  // plug
               "(+0.65/0.8;0.65/0.9;0.7/0.9;0.7/0.8)" // pin left
               "(+0.8/0.8; 0.8/0.9;0.85/0.9;0.85/0.8)"  // pin right
               , true);
    m_values.append(Elem);
    m_PredefinedCount = m_values.count();
}

void ControlLampModel::customStancilFromIni(int arg_Fd)
{
    QString loc_section("GUI/AllUserDefinedStancilForControlLamps");
    for (int loc_stancilIndex = 0; ; loc_stancilIndex++) {
        QString loc_entry(QString("Stancil_%1").arg(loc_stancilIndex));
        QString loc_text = ScQt_IniFileDataBaseReadString(loc_section, loc_entry, "", arg_Fd);
        if (loc_text.isEmpty()) {
            break;
        }
        Lamp *Elem = new Lamp;
        if (Elem->init(loc_text).isEmpty()) {
            addLamp (Elem);
        }
    }
}

void ControlLampModel::addStancilToIni(QString arg_name, QString arg_polygons)
{
    int loc_index = 0;
    QString loc_section("GUI/AllUserDefinedStancilForControlLamps");
    QString loc_entry("Stancil_0");

    while (!ScQt_IniFileDataBaseReadString(loc_section, loc_entry, "", ScQt_GetMainFileDescriptor()).isEmpty()) {
        loc_index++;
        loc_entry = QString("Stancil_%1").arg(loc_index);
    }

    arg_name.append(",");
    arg_name.append(arg_polygons);
    ScQt_IniFileDataBaseWriteString(loc_section, loc_entry, arg_name, ScQt_GetMainFileDescriptor());
}

void ControlLampModel::removeStancilFromIni(QString arg_name)
{
    QString loc_section("GUI/AllUserDefinedStancilForControlLamps");
    int Offset = 0;
    int loc_stancilIndex;
    for (loc_stancilIndex = 0; ; loc_stancilIndex++) {
        QString loc_entry(QString("Stancil_%1").arg(loc_stancilIndex));
        QString loc_text = ScQt_IniFileDataBaseReadString(loc_section, loc_entry, "", ScQt_GetMainFileDescriptor());
        if (loc_text.isEmpty()) {
            break;
        }
        QString loc_stancilName = QString(loc_text).split(",").first();
        if (!arg_name.compare(loc_stancilName)) {
            Offset--;
        } else {
            if (Offset < 0) {
                QString loc_entry2(QString("Stancil_%1").arg(loc_stancilIndex + Offset));
                ScQt_IniFileDataBaseWriteString(loc_section, loc_entry2, loc_text, ScQt_GetMainFileDescriptor());
            }
        }
    }
    if (Offset < 0) {
        for (int loc_stancilIndex2 = loc_stancilIndex + Offset; loc_stancilIndex2 < loc_stancilIndex; loc_stancilIndex2++) {
            QString loc_entry2(QString("Stancil_%1").arg(loc_stancilIndex + Offset));
            ScQt_IniFileDataBaseDeleteEntry(loc_section, loc_entry2, ScQt_GetMainFileDescriptor());
        }
    }
}

void ControlLampModel::writeToIni(int arg_Fd)
{
    QString loc_section("GUI/AllUserDefinedStancilForControlLamps");
    int loc_stancilIndex = 0;
    if (arg_Fd <= 0) {
        arg_Fd = ScQt_GetMainFileDescriptor();
    }
    ScQt_IniFileDataBaseDeleteSection(loc_section, arg_Fd);
    foreach (Lamp *loc_lamp, m_values) {
        QString loc_entry(QString("Stancil_%1").arg(loc_stancilIndex));
        QString loc_text = loc_lamp->m_name;
        loc_text.append(",");
        loc_text.append(loc_lamp->m_polygon);
        ScQt_IniFileDataBaseWriteString(loc_section, loc_entry, loc_text, arg_Fd);
        loc_stancilIndex++;
    }
}

bool ControlLampModel::hasChanged(bool arg_reset)
{
    bool loc_ret = m_hasChanged;
    if (arg_reset) {
        m_hasChanged = false;
    }
    return loc_ret;
}

bool ControlLampModel::hasStancil(QString &arg_name)
{
    foreach (Lamp *loc_lamp, m_values) {
        if(loc_lamp->m_name.compare(arg_name) == 0){
            return true;
        }
    }
    return false;
}

int ControlLampModel::GetPredefinedCount()
{
    return m_PredefinedCount;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    #define SKIP_EMPTY_PARTS QString::SkipEmptyParts
#else
    #define SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#endif

QString Lamp::init(QString arg_stancilString, bool arg_predefined)
{
    m_predefined = arg_predefined;
    QStringList loc_customStancilList = arg_stancilString.split(",", SKIP_EMPTY_PARTS);
    if (loc_customStancilList.count() < 2) {
        return QString ("Lamp definition needs a name followed with polygons \"%i\"").arg (arg_stancilString);
    }
    QString loc_stancilName = loc_customStancilList.at(0);
    QString loc_polygonText = loc_customStancilList.at(1);

    if(loc_polygonText.count("(") == loc_polygonText.count(")")) {
        QStringList loc_polygons = loc_polygonText.split(")", SKIP_EMPTY_PARTS);
        if(!loc_polygons.isEmpty()) {
            QList<QPolygonF> loc_foregroundPolygon;
            QList<QPolygonF> loc_backgroundPolygon;
            bool loc_isForeground = true;
            foreach(QString loc_polygon, loc_polygons) {
                if(loc_polygon.startsWith("(+")) {
                    loc_isForeground = true;
                    loc_polygon.remove(0, 2);
                } else {
                    if(loc_polygon.startsWith("(-")) {
                        loc_isForeground = false;
                        loc_polygon.remove(0, 2);
                    } else {
                        return QString("Polygon \"%1\" not as foreground (+) or background (-) defined! \n (+0.1/0.1;0.9/0.5;0.1/0.9;) (-0.4/0.4;0.6/0.5;0.4/0.6;)").arg(loc_stancilName);
                    }
                }
                QStringList loc_points = loc_polygon.split(";", SKIP_EMPTY_PARTS);
                if(!loc_points.isEmpty()) {
                    QPolygonF loc_polygonValues;
                    foreach(QString loc_point, loc_points) {
                        QStringList loc_values = loc_point.split("/");
                        if(loc_values.count() == 2) {
                            bool loc_firstDouble = false;
                            bool loc_secondDouble = false;
                            double loc_first = loc_values.first().toDouble(&loc_firstDouble);
                            double loc_second = loc_values.last().toDouble(&loc_secondDouble);
                            if(loc_firstDouble && loc_secondDouble) {
                                //NOTE: value range from 0...100 line width is 1
                                loc_polygonValues.append(QPointF(loc_first, loc_second));
                            }
                        }
                    }
                    if(!loc_polygonValues.isEmpty()) {
                        if(loc_isForeground) {
                            loc_foregroundPolygon.append(loc_polygonValues);
                        } else {
                            loc_backgroundPolygon.append(loc_polygonValues);
                        }
                    }
                }
            }
            m_name = loc_stancilName;
            m_polygon = loc_polygonText;
            m_polygonForeground = loc_foregroundPolygon;
            m_polygonBackground = loc_backgroundPolygon;
        } else {
            return QString ("No polygons defined");
        }
    } else {
        return QString ("The number of \"(\" and \")\" is not the same for stancil \"%1\"!").arg(loc_stancilName);
    }
    return QString();
}



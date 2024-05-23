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


#ifndef CONTROLLAMPMODEL_H
#define CONTROLLAMPMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QPolygonF>
#include <QVariant>
#include <QString>
#include <QModelIndex>
#include <QList>
#include <QColor>
#include <QGraphicsPolygonItem>
#include <QBrush>
#include <QPen>

class Lamp{
public:
    QString init(QString arg_stancilString, bool arg_predefined = false);
    QString m_name;
    QString m_polygon;
    bool m_predefined;
    QList<QPolygonF> m_polygonForeground;
    QList<QPolygonF> m_polygonBackground;
};


class ControlLampModel : public QAbstractListModel
{
    Q_OBJECT
public:

    ControlLampModel(QObject *parent = nullptr, bool predef = true, int arg_Fd = -1);
    ~ControlLampModel();

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QList<QPolygonF>* getForegroundStancil(const QModelIndex index);
    QList<QPolygonF>* getBackgroundStancil(const QModelIndex index);
    QList<QPolygonF>* getForegroundStancil(QString arg_name);
    QList<QPolygonF>* getBackgroundStancil(QString arg_name);
    QString stancilName(const QModelIndex arg_index);
    QString stancilPolygon(const QModelIndex arg_index);
    void addStancil(const QString arg_name, const QString arg_polygon,
                    const QList<QPolygonF> arg_polygonForeground, const QList<QPolygonF> arg_polygonBackground);
    bool removeStancil(const QModelIndex arg_index);
    bool nameExists(QString arg_name);
    Lamp *getStancilInformation(int arg_row);
    QModelIndex getItemIndex(QString arg_stancilName);

    void addLamp(Lamp *arg_Lamp);

    void addStancilToIni(QString arg_name, QString arg_polygons);
    void removeStancilFromIni(QString arg_name);

    void writeToIni(int arg_Fd);

    bool hasChanged(bool arg_reset);
    bool hasStancil(QString &arg_name);

    int GetPredefinedCount();

private:
    void predefinedStencils();
    void customStancilFromIni(int arg_Fd);

private:
    QList<Lamp*> m_values;
    bool m_hasChanged;
    int m_PredefinedCount;
};

#endif // CONTROLLAMPMODEL_H

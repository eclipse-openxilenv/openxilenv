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


#include "BinaryCompareDialog.h"
#include "ui_BinaryCompareDialog.h"

#include <QFile>
#include <QScrollBar>

#include "FileDialog.h"
extern "C" {
#include "FileExtensions.h"
#include "ThrowError.h"
}


BinaryCompareDialog::BinaryCompareDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::BinaryCompareDialog)
{
    ui->setupUi(this);
    m_File1Model.SetDispayBuffer(&m_File1Image);
    m_File1Model.SetRefBuffer(&m_File2Image);
    m_File2Model.SetDispayBuffer(&m_File2Image);
    m_File2Model.SetRefBuffer(&m_File1Image);

    ui->File1TableView->setModel(&m_File1Model);
    ui->File2TableView->setModel(&m_File2Model);

    //Scrollbar synchronization
    connect(static_cast<QObject*>(ui->File1TableView->verticalScrollBar()), SIGNAL(valueChanged(int)),
            static_cast<QObject*>(ui->File2TableView->verticalScrollBar()), SLOT(setValue(int)));
    connect(static_cast<QObject*>(ui->File2TableView->verticalScrollBar()), SIGNAL(valueChanged(int)),
            static_cast<QObject*>(ui->File1TableView->verticalScrollBar()), SLOT(setValue(int)));
}

BinaryCompareDialog::~BinaryCompareDialog()
{
    delete ui;
}

void BinaryCompareDialog::on_File1OpenPushButton_clicked()
{
    QString loc_Filename = FileDialog::getOpenFileName(this,  QString("Open binary file 1"), QString(), QString(ALL_EXT));
    if (!loc_Filename.isEmpty()) {
        ui->File1LineEdit->setText(loc_Filename);
        LoadFile(loc_Filename, 1);
    }
}

void BinaryCompareDialog::on_File2OpenPushButton_clicked()
{
    QString loc_Filename = FileDialog::getOpenFileName(this,  QString("Open binary file 2"), QString(), QString(ALL_EXT));
    if (!loc_Filename.isEmpty()) {
        ui->File2LineEdit->setText(loc_Filename);
        LoadFile(loc_Filename, 2);
    }
}

void BinaryCompareDialog::on_UpPushButton_clicked()
{
}

void BinaryCompareDialog::on_DownPushButton_clicked()
{
}

void BinaryCompareDialog::LoadFile(QString &par_Filename, int par_Number)
{
    QFile File(par_Filename);
    if (!File.open(QIODevice::ReadOnly)) return;
    m_File1Model.beginUpdate();
    m_File2Model.beginUpdate();
    switch (par_Number) {
    case 1:
        m_File1Image = File.readAll();
        break;
    case 2:
        m_File2Image = File.readAll();
        break;
    }
    m_File1Model.endUpdate();
    m_File2Model.endUpdate();
}

BinaryCompareModel::BinaryCompareModel()
{
    m_DispBuffer = nullptr;
    m_RefBuffer = nullptr;
}

BinaryCompareModel::~BinaryCompareModel()
{
}

void BinaryCompareModel::SetDispayBuffer(QByteArray *arg_DispBuffer)
{
    m_DispBuffer = arg_DispBuffer;
}

void BinaryCompareModel::SetRefBuffer(QByteArray *arg_RefBuffer)
{
    m_RefBuffer = arg_RefBuffer;
}

QVariant BinaryCompareModel::data(const QModelIndex &index, int role) const
{
    int Row = index.row();
    int Column = index.column();

    int Pos = Row * 16 + Column;

    switch (role) {
    case Qt::BackgroundRole:
        if (((m_DispBuffer != nullptr) && (Pos < m_DispBuffer->size())) &&
            ((m_RefBuffer != nullptr) && (Pos < m_RefBuffer->size()))) {
            unsigned char DispByte = static_cast<unsigned char>(m_DispBuffer->at(Pos));
            unsigned char RefByte = static_cast<unsigned char>(m_RefBuffer->at(Pos));
            if (DispByte != RefByte) {
                QColor Ret = QColor(Qt::red);
                return Ret;
            }
        }
        break;
    case Qt::DisplayRole:
        if ((m_DispBuffer != nullptr) && (Pos < m_DispBuffer->size())) {
            char Buffer[32];
            unsigned char Byte = static_cast<unsigned char>(m_DispBuffer->at(Pos));
            sprintf (Buffer, "%02X", static_cast<unsigned int>(Byte));
            QString Ret(Buffer);
            return Ret;
        }
        break;
    }
    return QVariant();
}

QVariant BinaryCompareModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole:
            char Buffer[8];
            sprintf (Buffer, "%02X", static_cast<unsigned int>(section));
            QString Ret(Buffer);
            return Ret;
        }
    } else if (orientation == Qt::Vertical) {
        switch (role) {
        case Qt::DisplayRole:
            char Buffer[16];
            sprintf (Buffer, "%08X", static_cast<unsigned int>(section * 16));
            QString Ret(Buffer);
            return Ret;
        }
    }
    return QVariant();
}

int BinaryCompareModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (m_DispBuffer == nullptr) {
        return 0;
    } else {
        return m_DispBuffer->size() / 16;
    }
}

int BinaryCompareModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 16;
}

void BinaryCompareModel::beginUpdate()
{
    beginResetModel();
}

void BinaryCompareModel::endUpdate()
{
    endResetModel();
}

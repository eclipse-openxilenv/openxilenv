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


#include "UserDrawBinaryImage.h"
#include "StringHelpers.h"

#include <QFile>
#include <QDataStream>

extern "C" {
#include "MyMemory.h"
#include "Files.h"
#include "ThrowError.h"
}

UserDrawBinaryImage::UserDrawBinaryImage()
{
    m_ImageBuffer = nullptr;
}

UserDrawBinaryImage::~UserDrawBinaryImage()
{
    if (m_ImageBuffer != nullptr) my_free(m_ImageBuffer);
}

bool UserDrawBinaryImage::Load(QString &par_FileName)
{
    QFile File(par_FileName);

    if (!File.open(QIODevice::ReadOnly)) {
        return false;
    }
    LogFileAccess (QStringToConstChar(par_FileName));
    m_ImageBufferSize = File.size();
    m_ImageBuffer = (unsigned char*)my_malloc(m_ImageBufferSize);
    if (m_ImageBuffer == nullptr) {
        ThrowError (1, "cannot allocate %i bytes for image \"%s\" buffer", m_ImageBufferSize,
                   QStringToConstChar(par_FileName));
        File.close();
        return false;
    }
    QDataStream Out(&File);
    if (Out.readRawData((char*)m_ImageBuffer, m_ImageBufferSize) != m_ImageBufferSize) {
        ThrowError (1, "cannot read %i bytes for image \"%s\"", m_ImageBufferSize,
                   QStringToConstChar(par_FileName));
        File.close();
    }
    File.close();
    return true;
}

QImage UserDrawBinaryImage::GetImageFrom()
{
    QImage Ret;
    if (!Ret.loadFromData(m_ImageBuffer, m_ImageBufferSize)) {
    }
    return Ret;
}

bool UserDrawBinaryImage::AllocBuffer(int par_Size)
{
    m_ImageBuffer = (unsigned char*)my_realloc(m_ImageBuffer, par_Size);
    return (m_ImageBuffer != nullptr);
}

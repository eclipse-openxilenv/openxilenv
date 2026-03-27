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


#ifndef BLACKBOARDINFODIALOG_H
#define BLACKBOARDINFODIALOG_H

#include <QDialog>
#include <QMap>
#include <QListWidgetItem>
#include <QSortFilterProxyModel>
#include "Dialog.h"

namespace Ui {
class BlackboardInfoDialog;
}

class BlackboardInfoDialog : public Dialog
{
    Q_OBJECT

public:
    explicit BlackboardInfoDialog(QWidget *parent = nullptr);
    ~BlackboardInfoDialog();
    void editCurrentVariable(QString arg_variableName);

public slots:
    void accept();

private slots:

    void on_ConversionEditPushButton_clicked();

    void on_ChoiceColorPushButton_clicked();

    void on_ColorLineEdit_editingFinished();

    void on_ColorValidCheckBox_stateChanged(int arg1);

    void handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void on_ConversionTypeComboBox_currentIndexChanged(int index);

private:
    class DialogData {
    public:
        int Vid;
        int DataType;
        unsigned int AttachCounter;
        double Value;
        QString Unit;
        double MinValue;
        double MaxValue;
        int ConversionType;
        QString ConversionString;
        QColor Color;
        bool ColorValid;
        int Width;
        int Precision;
    };

    QMap <QString, DialogData> m_Data;
    QMap <QString, DialogData> m_DataSave;

    Ui::BlackboardInfoDialog *ui;

    void NewVariableSelected (QString VariableName);
    void StoreVariableSelected (QString VariableName);

    void SetColorView (QColor color);

    char *m_Buffer;
    int m_BufferSize;
};

#endif // BLACKBOARDINFODIALOG_H

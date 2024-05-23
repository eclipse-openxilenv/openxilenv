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


#include "NewHotkeyDialog.h"
#include "ui_NewHotkeyDialog.h"

#include <QCompleter>
#include <QPushButton>
#include <HotkeyHandler.h>
#include <MainWindow.h>

extern "C"
{
    #include "ThrowError.h"
}


NewHotkeyDialog::NewHotkeyDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::NewHotkeyDialog)
{
    ui->setupUi(this);

    reservedHotkeys << "f1"
                    << "alt+f1"
                    << "shift+f1"
                    << "ctrl+f1"
                    << "strg+f1"
                    << "alt+f4"
                    << "ctrl+f4"
                    << "strg+f4"
                    << "ctrl+f6"
                    << "strg+f6"
                    << "f8"
                    << "ctrl+f10"
                    << "strg+f10"
                    << "shift+f10"
                    << "f12"
                    << "shift+f12"
                    << "alt+f"
                    << "alt+d"
                    << "alt+s"
                    << "alt+w"
                    << "alt+a"
                    << "alt+c"
                    << "alt+b"
                    << "ctrl+t"
                    << "strg+t";

    ui->lineEditShortcut->setFocus();

    setContextValidator();
    setCompleter();

}

NewHotkeyDialog::NewHotkeyDialog(QString shortcut, QString function, QString formula, QWidget *parent) : Dialog(parent),
    ui(new Ui::NewHotkeyDialog)
{
    ui->setupUi(this);

    ui->lineEditShortcut->setText(shortcut);
    ui->comboBoxAction->setCurrentText(function);
    ui->lineEditFormula->setText(formula);
    ui->lineEditShortcut->setFocus();

    if(ui->comboBoxAction->currentIndex() == 1)
    {
        ui->label_2->setText("File:");
    } else {
        ui->lineEditFormula->setEnabled(false);
    }

    setContextValidator();
    setCompleter();
}

NewHotkeyDialog::~NewHotkeyDialog()
{
    delete ui;    
}

void NewHotkeyDialog::keyPressEvent(QKeyEvent *event)
{
    QString tmp = ui->lineEditShortcut->text();

    if (tmp.isEmpty())
    {
        switch(event->key()){
            case Qt::Key_Alt:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText("Alt+");

                }
                break;
            case Qt::Key_Control:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText("Ctrl+");
                }
                break;
            case Qt::Key_Shift:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText("Shift+");
                }
                break;
            case Qt::Key_F1:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F1");
                }
                break;
            case Qt::Key_F2:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F2");
                }
                break;
            case Qt::Key_F3:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F3");
                }
                break;
            case Qt::Key_F4:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F4");
                }
                break;
            case Qt::Key_F5:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F5");
                }
                break;
            case Qt::Key_F6:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F6");
                }
                break;
            case Qt::Key_F7:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F7");
                }
                break;
            case Qt::Key_F8:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F8");
                }
                break;
            case Qt::Key_F9:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F9");
                }
                break;
            case Qt::Key_F10:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F10");
                }
                break;
            case Qt::Key_F11:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F11");
                }
                break;
            case Qt::Key_F12:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F12");
                }
                break;
            default:
                break;
        }
    } else if ((tmp.endsWith(" ", Qt::CaseSensitive)) || tmp.endsWith("+", Qt::CaseSensitive)) {
        switch(event->key()){
            case Qt::Key_F1:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F1");
                }
                break;
            case Qt::Key_F2:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F2");
                }
                break;
            case Qt::Key_F3:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F3");
                }
                break;
            case Qt::Key_F4:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F4");
                }
                break;
            case Qt::Key_F5:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F5");
                }
                break;
            case Qt::Key_F6:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F6");
                }
                break;
            case Qt::Key_F7:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F7");
                }
                break;
            case Qt::Key_F8:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F8");
                }
                break;
            case Qt::Key_F9:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F9");
                }
                break;
            case Qt::Key_F10:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F10");
                }
                break;
            case Qt::Key_F11:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F11");
                }
                break;
            case Qt::Key_F12:
                if(ui->lineEditShortcut->hasFocus()) {
                    ui->lineEditShortcut->setText(tmp + "F12");
                }
                break;
            case Qt::Key_Escape:
                ui->lineEditShortcut->clear();
                break;
            default:
                if (event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z)
                {
                    char key = static_cast<char>(event->key());
                    QString character(key);
                    if(ui->lineEditShortcut->hasFocus()) {
                        ui->lineEditShortcut->setText(tmp + character);
                    }
                }
                break;
        }
    } else {
        if (event->key() == Qt::Key_Escape)
        {
            ui->lineEditShortcut->clear();
        }
    }
}

void NewHotkeyDialog::accept()
{
    QStringList strList;

    QList <QStringList> *ptrToStrList = MainWindow::PointerToMainWindow->get_hotkeyHandler()->get_shortcutStringList();

    for (int i = 0; i < ptrToStrList->count(); i++) {
        QString string = ptrToStrList[0][i][0];
        if (!QString::compare(ui->lineEditShortcut->text(), string, Qt::CaseInsensitive))
        {
            if (ThrowError(ERROR_OKCANCEL, "Are you sure to overwrite the shortcut?") != 1)
            {
                return;
            }
            MainWindow::PointerToMainWindow->get_hotkeyHandler()->deleteshortcutStringListItem(i);
            break;
        }
    }
    if (!ui->lineEditShortcut->hasAcceptableInput())
    {
        ThrowError(MESSAGE_ONLY,"Syntax error: Shortcut is not in the right form \n"
                           "E.g.: Ctrl+F6");
        ui->lineEditShortcut->setFocus();
        return;
    }
    if(!isUnreservedHotkey()) return;

    strList.append(ui->lineEditShortcut->text());
    strList.append(ui->comboBoxAction->currentText());
    strList.append(ui->lineEditFormula->text());

    MainWindow::PointerToMainWindow->get_hotkeyHandler()->addItemToshortcutStringList(strList);

    QDialog::accept();
}

void NewHotkeyDialog::setContextValidator()
{
    QRegularExpression regExpShortcut("(([Cc][Tt][Rr][Ll]|[Aa][Ll][Tt]|[Ss][Hh][Ii][Ff][Tt])\\+([Ff][1-9]|[Ff][1][0]|[Ff][1][1]|[Ff][1][2]))|"
                           "(([Cc][Tt][Rr][Ll]|[Aa][Ll][Tt]|[Ss][Hh][Ii][Ff][Tt])\\+([a-zA-Z0-9]))|"
                           "([Ff][1-9]|[Ff][1][0]|[Ff][1][1]|[Ff][1][2])");
    ui->lineEditShortcut->setValidator(new QRegularExpressionValidator(regExpShortcut, this));
}

void NewHotkeyDialog::setCompleter()
{
    QStringList *list = new QStringList();

    *list   << "Ctrl+"
            << "Alt+"
            << "Shift+";
    QCompleter *completer = new QCompleter(*list, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->lineEditShortcut->setCompleter(completer);
}

bool NewHotkeyDialog::isUnreservedHotkey()
{
    for (int i = 0; i < reservedHotkeys.count(); i++)
    {
        if (!ui->lineEditShortcut->text().compare(reservedHotkeys.at(i), Qt::CaseInsensitive))
        {
            ThrowError(MESSAGE_ONLY,"The expression is reserved by the system!");
            ui->lineEditShortcut->setFocus();
            return false;
        }
    }
    return true;
}

void NewHotkeyDialog::on_comboBoxAction_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->label_2->setText("Formula:");
        ui->lineEditFormula->setEnabled(true);
    }
    else if(index == 1)
    {
        ui->label_2->setText("File:");
        ui->lineEditFormula->setEnabled(true);
    } else {
        ui->lineEditFormula->setEnabled(false);
    }
}

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


#include "AboutDialog.h"
#include "ui_AboutDialog.h"

extern"C"
{
    #include "Config.h"
}

AboutDialog::AboutDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    char Help[512];
    const char *Year;
    int ExeAddrSize = 8 * (int)sizeof(void*);

    const char *SystemName =
#ifdef WIN32
            "Win";
#elif defined __Linux__
            "Linux";
#else
            "Unknown";
#endif

#if (XILENV_MINOR_VERSION == 0)
    sprintf (Help, "Version %.3lf.final %s%i (build %s)\n", XILENV_VERSION/1000.0,
             SystemName, ExeAddrSize, __DATE__);
#elif (XILENV_MINOR_VERSION < 0)
    sprintf (Help, "Version %.3lf.pre-%d %s%i (build %s)\n", XILENV_VERSION/1000.0, -
             XILENV_MINOR_VERSION, SystemName, ExeAddrSize, __DATE__);
#else
    sprintf (Help, "Version %d.%d.%d %s%i (build %s)\n", XILENV_VERSION, XILENV_MINOR_VERSION, XILENV_PATCH_VERSION,
             SystemName, ExeAddrSize, __DATE__);
#endif
    ui->textBrowser->insertPlainText(Help);
    Year = __DATE__; // set pointer to the begin of the string
    Year += strlen(__DATE__); // set pointer to the end of the string
    Year -= 4;  //set pointer to the begin of the year
    sprintf (Help, "Copyright  1997-%s ZF Friedrichshafen AG\n\n", Year);
    ui->textBrowser->insertPlainText(Help);
    ui->textBrowser->insertPlainText("Licensed under the Apache License, Version 2.0 (the \"License\");\n"
                                     "you may not use this file except in compliance with the License.\n"
                                     "You may obtain a copy of the License at\n"
                                     "\n"
                                     "    http://www.apache.org/licenses/LICENSE-2.0\n"
                                     "\n"
                                     "Unless required by applicable law or agreed to in writing, software\n"
                                     "distributed under the License is distributed on an \"AS IS\" BASIS\n"
                                     "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
                                     "See the License for the specific language governing permissions and\n"
                                     "limitations under the License.\n");
#ifdef _MSC_FULL_VER
    sprintf (Help, "\nCompiled with Visual C compiler version %i\n", _MSC_FULL_VER);
#else
    sprintf (Help, "\nCompiled with GCC compiler version %i.%i.%i\n",__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__);
#endif
    ui->textBrowser->insertPlainText(Help);

    sprintf (Help, "dynamic linkend agains Qt Library %s\n"
                   "The Qt Toolkit is Copyright (C) %s The Qt Company Ltd. and other contributors\n"
                   "LGPL version 3 (http://doc.qt.io/qt-%i/lgpl.html)\n\n", QT_VERSION_STR, Year, QT_VERSION_MAJOR);
    ui->textBrowser->insertPlainText(Help);
    ui->textBrowser->insertPlainText("Supported target compiler:\n GCC");
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

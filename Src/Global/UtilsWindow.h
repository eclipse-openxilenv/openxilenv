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


#ifndef UTILSWINDOW_H
#define UTILSWINDOW_H

void OpenIniWindows (void);

int OpenWindowByName (char *WindowName);
int OpenWindowByFilter (char *WindowNameFilter);

int CloseWindowByName (char *WindowName);
int CloseWindowByFilter (char *WindowNameFilter);

int ExportWindowByFilter (char *SheetFilter, char *WindowNameFilter, char *FileName);

int ImportWindowByFilter (char *WindowNameFilter, char *FileName);

int DeleteWindowByName (char *WindowName);
int DeleteWindowByFilter (char *WindowNameFilter);
int ScriptOpenWindowByFilter (char *WindowNameFilter);
int ScriptCloseWindowByFilter (char *WindowNameFilter);
int ScriptExportWindowByFilter (char *SheetFilter, char *WindowNameFilter, char *FileName);
int ScriptImportWindowByFilter (char *WindowNameFilter, char *FileName);
int ScriptDeleteWindowByFilter (char *WindowNameFilter);

#endif // UTILSWINDOW_H

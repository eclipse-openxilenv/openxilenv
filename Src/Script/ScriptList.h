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


#ifndef SC_LIST_H
#define SC_LIST_H

int ScriptAddParamsToParamList (char *Name, int Size, char *Params[]);

int ScriptDelParamsFromParamList (char *Name, int Size, char *Params[]);

int ScriptNewParamList (char *Name, int Size, char *Params[]);
        
int ScriptDeleteParamList (char *Name);

int ScriptInsertParameterList (char *NameBegin, char *NameEnd, char *ptr_parameter[], int Pos, int Max);

void ScriptDeleteAllParamLists (void);

void ScriptPrintAllParamLists (char *Txt, int Maxc);

int ScriptAddReferenceVariList (char *ProcessName, int Pid, int Connection, int VariCount, char *Varis[]);

int ScriptDelReferenceVariList (int Pid, int Connection);

void ScriptDeleteAllReferenceVariLists (void);

#endif

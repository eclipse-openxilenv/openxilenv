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


#include "ScriptErrorDialog.h"
#include "MainWinowSyncWithOtherThreads.h"

void ScriptErrorDialog::ScriptErrorMsgDlgAddMsgFromOtherThread(int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message)
{
    AddNewScriptErrorMessageFromOtherThread(par_Level, par_LineNr, par_Filename, par_Message);
}

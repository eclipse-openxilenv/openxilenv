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


#ifndef __SC_CHANGE_SETTINGS
#define __SC_CHANGE_SETTINGS

#define CHANGE_SETTINGS_COMMAND_SET       0
#define CHANGE_SETTINGS_COMMAND_PUSH_SET  1
#define CHANGE_SETTINGS_COMMAND_GET       2
#define CHANGE_SETTINGS_COMMAND_POP       3

int ScriptChangeBasicSettings (int Command, int OnlyCheckFlag, const char *Param1, const char *Param2);
void ResetChangeSettingsStack(void);
#endif


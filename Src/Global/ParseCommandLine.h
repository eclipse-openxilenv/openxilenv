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


#ifndef PARSECOMMANDLINE
#define PARSECOMMANDLINE


int ShouldStartedAsIcon (void);

int OpenWithNoGui (void);

int GetNoXcp (void);

int GetInstanceParameterInfrontOfInit (char *ret_Instance, int par_MaxCharsInstance);

int ParseCommandLine (char *ret_IniFile, unsigned int par_MaxCharsIniFile,
                     char *ret_Instance, unsigned int par_MaxCharsInstance, void *par_Application);

#ifndef _WIN32
int SaveCommandArgs(int argc, char *argv[]);
#endif

#endif // PARSECOMMANDLINE


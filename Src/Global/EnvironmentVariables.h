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


#pragma once

// Inside XilEnv there exist 3 types of environment variables:
// 1. System environment variables are inherited from the operationg system
// 2. Fixed defined environment variables inside XilEnv
// 3. User defined environment variables
//    Which are set for example by script command SET_ENV_VAR (name, value)

int InitEnvironmentVariables (void);

int SearchAndReplaceEnvironmentStrings (const char *src, char *dest, int maxc);
int SearchAndReplaceEnvironmentStringsExt (const char *src, char *dest, int maxc, int *pSolvedEnvVars, int *pNoneSolvedEnvVars);
int CheckIfEnvironmentVariableExist (const char *Name);
int RemoveUserEnvironmentVariable (const char *Name);

int SetUserEnvironmentVariable (const char *Name, const char *Value);

int GetEnvironmentVariableList (char **Buffer, int *SizeOfBuffer, char **RefBuffer, int *SizeOfRefBuffer);

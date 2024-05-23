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

int RemoteMasterCopyAndStartExecutable(const char *par_ExecutableNameClient, const char *par_ExecutableNameServer, const char *par_SharedLibraryDir,
                                       const char *par_RemoteMasterAddr, int par_RemoteMasterPort, int par_ExecFlag);

int RemoteMasterSyncDateTime(char *par_RemoteMasterAddr, int par_RemoteMasterPort);

int RemoteMasterKillAll(char *par_RemoteMasterAddr, int par_RemoteMasterPort, const char *par_Name);

int RemoteMasterRemoveVarLogMessages(char *par_RemoteMasterAddr, int par_RemoteMasterPort);

int RemoteMasterPing(char *par_RemoteMasterAddr, int par_RemoteMasterPort);

int RemoteMasterShutdown(char *par_RemoteMasterAddr, int par_RemoteMasterPort);

int RemoteMasterWakeOnLan(const unsigned char *par_MacAddress);

void RemoteMasterGetMacAddress(char *par_RemoteMasterAddr, unsigned char *ret_MacAddress);


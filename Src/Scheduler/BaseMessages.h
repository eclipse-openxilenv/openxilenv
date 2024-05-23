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


#ifndef BASEMESSAGES_H
#define BASEMESSAGES_H

#include <stdio.h>
#include "tcb.h"
#include "PipeMessagesShared.h"

void MessageLoggingToFile (FILE *LoggingFile, int par_LineNr, int par_Dir, int par_Pid, void *par_Message);

int InitExternProcessMessages (char *par_Prefix, int par_LogingFlag, unsigned int par_SocketPort);

void TerminateExternProcessMessages (void);

int CallExternProcessFunction (int Function, uint64_t Cycle, TASK_CONTROL_BLOCK *pTbc,
                               PIPE_API_BASE_CMD_MESSAGE *pCmdMessageBuffer, int par_SnapShotSize, int par_Size,
                               PIPE_API_BASE_CMD_MESSAGE_ACK *pCmdMessageAckBuffer);

void SendKillExternalProcessMessage (TASK_CONTROL_BLOCK *pTcb);

int scm_ref_vari (uint64_t address, PID pid, char *name, int type, int dir);
int scm_ref_vari_lock_flag (uint64_t address, PID pid, const char *name, int type, int dir, int lock_flag);

int scm_unref_vari(uint64_t address, PID pid, char *name, int type);
int scm_unref_vari_lock_flag (uint64_t address, PID pid, char *name, int type, int lock_flag);

int dereference_all_blackboard_variables (PID pid);

void ClosePipeToExternProcess (TASK_CONTROL_BLOCK *pTcb);

int scm_write_section_to_exe(PID Pid, char *SelectedSection);

int scm_read_bytes (uint64_t address, PID pid, char *dest, int len);
int scm_write_bytes (uint64_t address, PID pid, unsigned char *src, int len);

int scm_write_bytes_pid_array (uint64_t address, PID *pid_array, int size, unsigned char *src, int len);

int CheckExternProcessComunicationProtocolVersion (PIPE_API_LOGIN_MESSAGE *LoginMessage);


#endif

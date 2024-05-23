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
#include "Message.h"
#include "StructsRM_Message.h"

int rm_write_message_ts_as (int pid, int mid, uint64_t timestamp, int msize, char *mblock, int tx_pid);

int rm_write_important_message_ts (int pid, int mid, uint64_t timestamp, int msize, char *mblock);

int rm_test_message (MESSAGE_HEAD *mhead);

int rm_read_message (MESSAGE_HEAD *mhead, char *message, int maxlen);

int rm_remove_message (void);

int remove_message_as(int pid);
int test_message_as(MESSAGE_HEAD *mhead, int pid);
int read_message_as(MESSAGE_HEAD *mhead, char *message, int maxlen, int pid);




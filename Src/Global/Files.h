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


#ifndef FILES_H
#define FILES_H

#include <stdio.h>

#define END_OF_LINE   1
#define END_OF_FILE   2
#define STRING_TRUNCATED   0x1000

struct file_cblock {
    char *name;                   /* File name */
    FILE *fh;                     /* File handle */
    int pid;
    struct file_cblock *next;     /* Next entry */
    void *own_buffer;
};

int init_files (void);
char *get_open_files_infos(int index);
int get_count_open_files_infos (void);
int expand_filename (const char *src_name, char *dst_name, int maxc);

FILE *open_file (const char * const name, const char *flags);
FILE *OpenFile4WriteWithPrefix (char *name, char *flags);

void close_file (FILE *handle);

int read_next_word (FILE *file, char *word, int maxlen);
int read_next_word_line_counter (FILE *file, char *word, int maxlen, int *linec);

char *get_date_time_string (void);

void EnterGlobalCriticalSection (void);
void LeaveGlobalCriticalSection (void);

int StringCommaSeparate (char *str, ...);

const char *GetProcessNameFromPath(const char *ProcessPath);

void LogFileAccess (const char *Filename);

unsigned long long GetLastWriteTimeOfFile (char *Filename);

// aus abwaerts Kobatibilitaet
//#ifdef _WIN32
//#include "EnvironmentVariables.h"
//#endif

#endif

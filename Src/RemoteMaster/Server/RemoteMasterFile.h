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


#ifndef REMOTEMASTERFILE_H
#define REMOTEMASTERFILE_H

#define REL_TO_BEGIN	0
#define REL_TO_ACTUAL	1
#define REL_TO_END		2

#define RT_FILE FILE

int Read_ini_entry(char* file, char* section, char* entry, char* def);
int Write_ini_entry(char* file, char* section, char* entry, char* param);
int rt_fopen(char *fname);
RT_FILE* get_rt_file_stream(void);
void rt_fclose(RT_FILE *file);
char rt_fgetc(RT_FILE *file);
long rt_fgetpos(RT_FILE *file);
long rt_ftell(RT_FILE *file, int relative_to);
int rt_fsetpos(RT_FILE *file, long position);
int rt_fseek(RT_FILE *file, long position, int relative_to);
char* rt_fgets(char* s, int n, RT_FILE *file);
long rt_fgetsize(RT_FILE *file);

#endif

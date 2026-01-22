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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <malloc.h>
#include <alloca.h>
#include <pwd.h>

#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Message.h"
#include "RealtimeScheduler.h"
#include "RemoteMasterFile.h"

int rt_fopen(char *fname)
{
    if (write_message(get_pid_by_name("RemoteMasterControl"), RT_FOPEN, strlen(fname) + 1, fname) < 0) {
        return (-1);
    }
    return (0);
}

RT_FILE* get_rt_file_stream(void)
{
    MESSAGE_HEAD mhead;
    RT_FILE* pointer_out;
    char *res;
    char *buf = NULL;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if ((test_message(&mhead) > 0) && (mhead.mid == RT_FOPEN_ACK)) {
        char *Filename = alloca(mhead.size);
        read_message(&mhead, Filename, mhead.size);
        // replace "~"
        if (Filename[0] == '~') {
            char *homedir;
            int Size;
            if ((homedir = getenv("HOME")) == NULL) {
                homedir = getpwuid(getuid())->pw_dir;
            }
            Size = mhead.size + strlen(homedir) + 1;
            buf = my_malloc(Size);

            StringCopyMaxCharTruncate(buf, homedir, Size);
            StringAppendMaxCharTruncate(buf, &(Filename[1]), Size);
            res = buf;
        }
        else {
            res = Filename;
        }

        pointer_out = fopen(res, "rt");
        if (buf != NULL) my_free(buf);
        return pointer_out;
    }
    else return (NULL);
}

void rt_fclose(RT_FILE *file)
{
    fclose(file);
}

char rt_fgetc(RT_FILE *file)
{
    return fgetc(file);
}

long rt_fgetpos(RT_FILE *file)
{

    fpos_t Ret;
    fgetpos(file, &Ret);
    return (long)Ret.__pos;
}

long rt_ftell(RT_FILE *file, int relative_to)
{
    switch (relative_to) {
    case 0:
        return ftell(file);
    case 1:
        {
            struct stat buf;
            fstat(fileno(file), &buf);
            return buf.st_size - ftell(file);

        }
    default:
        return -1;
    }
}

int rt_fsetpos(RT_FILE *file, long position)
{
    fpos_t Pos;
    Pos.__pos = position;
    return fsetpos(file, &Pos);
}

int rt_fseek(RT_FILE *file, long position, int relative_to)
{
    return fseek(file, position, relative_to);
}

char* rt_fgets(char* s, int n, RT_FILE *file)
{
    return fgets(s, n, file);
}

long rt_fgetsize(RT_FILE *file)
{
    struct stat buf;
    if (fstat(fileno(file), &buf) == 0) {
        return buf.st_size;
    } else {
        return -1;
    }
}

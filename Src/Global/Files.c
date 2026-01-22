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


#define FILES_C

#include "Platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <malloc.h>

#include "Config.h"
#include "Scheduler.h"
#include "BlackboardAccess.h"
#include "MainValues.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "IniDataBase.h"
#include "EquationParser.h"
#include "Message.h"
#include "EnvironmentVariables.h"
#include "Files.h"

#define UNUSED(x) (void)(x)

static struct file_cblock *open_file_list;

static CRITICAL_SECTION FilesCriticalSection;


int init_files (void)
{
    INIT_CS (&FilesCriticalSection);
    srand ((unsigned)time (NULL));

    return 0;
}


static void LogFileAccessNoC (const char *Filename)
{
    static char **FileNameArray;
    static int SizeOfFileNameArray;
    int x, xx;
    static FILE *fh;
    char FullPath[MAX_PATH];
     
#ifdef _WIN32
    if (_fullpath (FullPath, Filename, MAX_PATH) == NULL) {
        strncpy (FullPath, Filename, MAX_PATH);
        FullPath[MAX_PATH-1] = 0;
    }
#else
    realpath (Filename, FullPath);
#endif

    if (fh == NULL) {
        fh = fopen (s_main_ini_val.LogFileForAccessedFilesName, "wt");
        if (fh == NULL) {
            ThrowError (1, "cannot open file \"%s\" for file access logging", s_main_ini_val.LogFileForAccessedFilesName);
            s_main_ini_val.LogFileForAccessedFilesFlag = 0;
            return;
        }
    }
    for (x = 0; (x < SizeOfFileNameArray) && (FileNameArray[x] != NULL); x++) {
        if (!strcmpi (FullPath, FileNameArray[x])) {
            return; // File was already logged
        }
    }
    fprintf (fh, "%s\n", FullPath);
    fflush (fh);
    if (x == SizeOfFileNameArray) {
        SizeOfFileNameArray += 4;
        FileNameArray = (char**)my_realloc (FileNameArray, (size_t)SizeOfFileNameArray * sizeof (char*));
        for (xx = x; xx < SizeOfFileNameArray; xx++) {
            FileNameArray[xx] = NULL;
        }
    }
    if (FileNameArray != NULL) {
        FileNameArray[x] = StringMalloc (FullPath);
    }
}

void LogFileAccess (const char *Filename)
{
    if (s_main_ini_val.LogFileForAccessedFilesFlag) {
        ENTER_CS (&FilesCriticalSection);
        LogFileAccessNoC (Filename);
        LEAVE_CS (&FilesCriticalSection);
    }
}

FILE *open_file (const char * const name, const char *flags)
{
     FILE *file;
     struct file_cblock *ofl_elem;
     char help[MAX_PATH];
     int SetFileBufferFlag = 0;
     uint32_t FileSize;
     uint32_t FileSize_2N;
     void *OwnBuffer;
     if (SearchAndReplaceEnvironmentStrings (name, help, MAX_PATH) <= 0) {
         return NULL;
     }
     if (*flags == '@') {  // File should be fit into the file buffer
         SetFileBufferFlag = 1;
         flags++;
     }
     ENTER_CS (&FilesCriticalSection);
#if defined(_WIN32) && !defined(__GNUC__)
     file = _fsopen (help, flags, SH_DENYWR);
#else
     file = fopen (help, flags);
#endif
     if (file != NULL) {
         if (s_main_ini_val.LogFileForAccessedFilesFlag) {
             LogFileAccess (name);
         }
         if (SetFileBufferFlag) {
#ifdef _WIN32
            FileSize = (uint32_t)_filelength (_fileno (file));
#else
            struct stat st;
            fstat(fileno (file), &st);
            FileSize = st.st_size;
#endif

            // This should ne 2^n large
            FileSize_2N = 1024;
                
            while ((FileSize_2N < FileSize) && (FileSize_2N < 0x100000)) FileSize_2N <<= 1;
            OwnBuffer = my_malloc (FileSize_2N);
            // If no free memory not using own buffer
            if (OwnBuffer == NULL) {
                SetFileBufferFlag = 0;         
                if (setvbuf (file, OwnBuffer, _IOFBF, FileSize_2N)) {
                    my_free (OwnBuffer);
                    SetFileBufferFlag = 0;
                }
            }
         } else OwnBuffer = NULL;

         if ((ofl_elem = (struct file_cblock *)my_malloc (sizeof (struct file_cblock))) == NULL) {
             LEAVE_CS (&FilesCriticalSection);
             return NULL;
         }
         if ((ofl_elem->name = StringMalloc (help)) == NULL) {
             LEAVE_CS (&FilesCriticalSection);
             return NULL;
         }
     } else {
         LEAVE_CS (&FilesCriticalSection);
         return NULL;
     }
     if (SetFileBufferFlag) {
         ofl_elem->own_buffer = OwnBuffer;
     } else {
         ofl_elem->own_buffer = NULL;
     }

     ofl_elem->pid = GET_PID ();
     ofl_elem->fh = file;
     ofl_elem->next = open_file_list;
     open_file_list = ofl_elem;

     LEAVE_CS (&FilesCriticalSection);
     return file;
}

void close_file (FILE *handle)
{
    struct file_cblock *cblock, *lastblock = NULL;

    ENTER_CS (&FilesCriticalSection);
    if (open_file_list != NULL) {
        for (cblock = open_file_list; cblock != NULL; cblock = cblock->next) {
            if (cblock->fh == handle) {
                if (cblock == open_file_list)
                    open_file_list = cblock->next;
                else lastblock->next = cblock->next;

                fclose (handle);
                if (cblock->own_buffer != NULL) my_free (cblock->own_buffer);
                my_free (cblock->name);
                my_free (cblock);
                LEAVE_CS (&FilesCriticalSection);
                return;
            }
            lastblock = cblock;
        }
    }
    LEAVE_CS (&FilesCriticalSection);
    ThrowError (1, "close a file thats not open");
}

char* get_open_files_infos (int index)
{
    struct file_cblock *cblock;
    int items = 0;
    static char buffer[280];


    ENTER_CS (&FilesCriticalSection);
    for (cblock = open_file_list; cblock != NULL; cblock = cblock->next) {
        PrintFormatToString (buffer, sizeof(buffer), "%i. (%i): %s", ++items, cblock->pid, cblock->name);
        if(index == (items-1))
        {
            return buffer;
        }
    }
    LEAVE_CS (&FilesCriticalSection);
    return NULL;
}

int get_count_open_files_infos (void)
{
    struct file_cblock *cblock;
    int items = 0;

    ENTER_CS (&FilesCriticalSection);
    for (cblock = open_file_list; cblock != NULL; cblock = cblock->next) {
        items++;
    }
    LEAVE_CS (&FilesCriticalSection);
    return items;
}


int read_next_word (FILE *file, char *word, int maxlen)
{
    int char_count = 0;
    int one_char;
    int end_of_word = 0;
    char *wp = word;

    do {
        one_char = fgetc (file);
    } while (isspace (one_char));
    do {
        if ((one_char == EOF) || isspace (one_char)) {
            *wp = '\0';
            end_of_word = 1;
        } else {
            *wp++ = (char)one_char;
            if (++char_count >= maxlen) return -1;
        }
        if (!end_of_word) {
            one_char = fgetc (file);
        }
    } while (!end_of_word);

    while ((one_char != '\n') && (one_char != EOF) && (isspace (one_char))) {
        one_char = fgetc (file);
    }
    if (!isspace (one_char) && one_char != EOF) {
        ungetc (one_char, file);
    }

    if (one_char == '\n') return END_OF_LINE;  /* End of line reached */
    if (one_char == EOF) return END_OF_FILE;   /* End of file reached */
    return 0;
}


int my_fgetc (FILE *fh, int *linec)
{
    int one_char;

    one_char = fgetc (fh);
    if (one_char == '\n') {
        if (linec != NULL) *linec += 1;
    }
    return one_char;
}

int read_next_word_line_counter (FILE *file, char *word, int maxlen, int *linec)
{
    int char_count = 0;
    int one_char;
    int end_of_word = 0;
    char *wp = word;
    int q_flag = 0;   // This is 1 if a ""-Block reached
    int truncated = 0;

    do {
        one_char = my_fgetc (file, linec);
    } while (isspace (one_char));
    do {
        if ((one_char == EOF) || (isspace (one_char) && !q_flag)) {
            *wp = '\0';
            end_of_word = 1;
        } else {
            if (++char_count >= maxlen) {
                *wp = 0;
                truncated = STRING_TRUNCATED;  // String to long
            }
            if (truncated != STRING_TRUNCATED) *wp++ = (char)one_char;
        }
        if (one_char == '\"') {      // we are inside a ""-Block
            q_flag = !q_flag;
        }
        if (!end_of_word) {
            one_char = my_fgetc (file, linec);
        }
    } while (!end_of_word);

    while ((one_char != '\n') && (one_char != EOF) && (isspace (one_char))) {
        one_char = my_fgetc (file, linec);
    }
    if (!isspace (one_char) && one_char != EOF) {
        ungetc (one_char, file);
    }

    if (one_char == '\n') {
        return END_OF_LINE;  /* End of line reached */
    }

    if (one_char == EOF) return END_OF_FILE;   /* End of file reached */
    return 0;
}

char *get_date_time_string (void)
{
    time_t time_count;
    char *time_str, *p_tstr;

    time_count = time(&time_count);
    time_str = ctime(&time_count);

    if (time_str == NULL) {
        time_str = "";
    }
    p_tstr = time_str;
    while (*p_tstr != '\0') {
          if (*p_tstr == '\n') {
            *p_tstr = '\0';
            break;
        }
        p_tstr++;
    }
    return time_str;
}

int expand_filename (const char *src_name, char *dst_name, int maxc)
{
#ifdef _WIN32
    if (GetFullPathNameA (src_name, maxc, dst_name, NULL) == 0) return -1;
#else
    char *path = realpath (src_name, NULL);
    if (path == NULL) return -1;
    if ((int)strlen(path) >= maxc) {
        free(path);
        return -1;
    }
    StringCopyMaxCharTruncate(dst_name, path, maxc);
    free(path);
#endif
    return 0;
}

void EnterGlobalCriticalSection (void)
{
    ENTER_CS (&FilesCriticalSection);
}

void LeaveGlobalCriticalSection (void)
{
    LEAVE_CS (&FilesCriticalSection);
}

int StringCommaSeparate (char *str, ...)
{
    int counter = 0;
    va_list ap;
    char **p;

    va_start(ap, str);
    while ((p = va_arg (ap, char**)) != NULL) {
        if (*str != '\0') {
            *p = str;
            counter++;
        } else *p = NULL;
        while ((*str != ',') && (*str != '\0')) str++;
        if (*str == ',') *str++ = '\0';
        while ((*str == ' ') || (*str == '\t')) str++;
    }
    va_end (ap);
    return counter;
}

FILE *OpenFile4WriteWithPrefix (char *name, char *flags)
{
     char txt[512];
     char txt2[512];
     FILE *file;
     char *p;
     int qpos = -1;
     int qsize = -1;
     int max_count;
     int x;
#ifdef _WIN32
     HANDLE h;
     int err;
#else
     int fd;
#endif

     // If a '?' is inside the file name and at which position is it
     p = name;
     while (*p != 0) {
         if (*p == '?') {
             qpos = (int)(p - name);
             while (*p == '?') p++;
             qsize = (int)(p - name) - qpos;
             break;
         }
         p++;
     }
     if (qpos != -1) {
         max_count = 10;
         for (x = 1; x < qsize; x++) {
             max_count *= 10;
         }
         for (x = 0; x < max_count; x++) {
             StringCopyMaxCharTruncate(txt, name, (size_t)qpos);      // File name till the first '?'
             PrintFormatToString (&(txt[qpos]), sizeof(txt) - qpos, "%0*i", qsize, x);
             StringCopyMaxCharTruncate (&(txt[qpos+qsize]), &(name[qpos+qsize]), sizeof(txt) - (qpos+qsize));
             SearchAndReplaceEnvironmentStrings (txt, txt2, sizeof (txt2));
#ifdef _WIN32
             h = CreateFile(txt2, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                            CREATE_NEW, FILE_ATTRIBUTE_ARCHIVE, NULL);
             if (h == INVALID_HANDLE_VALUE) {
                 err = (int)GetLastError();
                 printf ("%i\n", err);
                 if (err != ERROR_FILE_EXISTS) {
                     break;
                 }
             } else {
                 CloseHandle (h);
                 break;
             }
#else
             if ((fd = creat (txt2, O_RDWR)) > 0) {
                 close (fd);
                 break;
             }
#endif
         }
     } else StringCopyMaxCharTruncate (txt, name, sizeof(txt));
     file = open_file (txt, flags);

     return file;
}


const char *GetProcessNameFromPath (const char *ProcessPath)
{
    const char *p;
    const char *pbs;
    const char *pka;
    const char *ppo;

    if (ProcessPath == NULL) return NULL;
    p = ProcessPath;
    pbs = NULL;
    pka = NULL;
    ppo = NULL;
    while (*p != 0) {
        if ((*p == '\\') || (*p == '/')) pbs = p;
        else if (*p == '@') pka = p;
        else if (*p == '.') ppo = p;
        p++;
    }
    if (pbs == NULL) return ProcessPath;
    if (ppo != NULL) {
        if (strnicmp (ppo, ".EXE", 4) &&
            strnicmp (ppo, ".FMU", 4) &&
            strnicmp (ppo, ".SCQ", 4)) {
            ThrowError (1, "\"%s\" is not a valid external prozess name", ProcessPath);
        }
    }
    return pbs + 1;
}

// This is only for debug usage
int HeapDump2File (char *FileName)
{
#ifdef _WIN32
    _HEAPINFO hinfo;
    int heapstatus;
    FILE *fh;
    int Count = 0;
    int Ret = 0;
   
    fh = fopen (FileName, "wt");
    if (fh == NULL) return -1;

    hinfo._pentry = NULL;
    while ((heapstatus = _heapwalk (&hinfo)) == _HEAPOK) { 
        fprintf (fh, "%04i: %6s block at %p of size %4.4X\n", Count++,
                 (hinfo._useflag == _USEDENTRY ? "USED" : "FREE"),
                 (void*)hinfo._pentry, (uint32_t)hinfo._size);
    }

    switch (heapstatus) {
    case _HEAPEMPTY:
        fprintf (fh, "OK - empty heap\n" );
        break;
    case _HEAPEND:
        fprintf (fh, "OK - end of heap\n" );
        break;
    case _HEAPBADPTR:
        fprintf (fh, "ERROR - bad pointer to heap\n" );
        Ret = -1;
        break;
    case _HEAPBADBEGIN:
        fprintf (fh, "ERROR - bad start of heap\n" );
        Ret = -1;
        break;
    case _HEAPBADNODE:
        fprintf (fh, "ERROR - bad node in heap\n" );
        Ret = -1;
        break;
    }
    fclose (fh);
    return Ret;
#else
    UNUSED(FileName);
    return -1;
#endif
}

unsigned long long GetLastWriteTimeOfFile (char *Filename)
{
#ifdef _WIN32
    union {
       FILETIME ft;
       unsigned long long u64;
    } Ret;
    HANDLE hFile = CreateFile (Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        Ret.u64 = 0;
    } else {
        if (!GetFileTime(hFile, NULL, NULL, &(Ret.ft))) {
            Ret.u64 = 0;
        }
        CloseHandle (hFile);
    }
    return Ret.u64;
#else
    struct stat Stat;
    if (stat (Filename, &Stat) == 0) {
        return Stat.st_mtime;
    } else {
        return 0;
    }
#endif
}


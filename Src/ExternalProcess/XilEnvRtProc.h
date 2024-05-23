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


#ifndef _RTEXTP_H
#define _RTEXTP_H

#include <stdint.h>
#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
*                              data types
*/
typedef int32_t PID;        // Process-ID
typedef int32_t VID;        // Variablen-ID

    /* Heap-Funktionen: */
#define my_malloc(s) __my_malloc (__FILE__, __LINE__, s)
#define my_realloc(b, s) __my_realloc (__FILE__, __LINE__, b, s)
#define my_calloc(n, s) __my_calloc (__FILE__, __LINE__, n, s)

void* __my_malloc(char *file, int line, long size);
void my_free(void * block_ptr);
void* __my_realloc(char *file, int line, void* old_ptr, long new_size);
void* __my_calloc(char *file, int line, long num, long size);

/* File-Operationen */
typedef struct {
    void *Ptr;
} RT_FILE;

int Read_ini_entry(char* file, char* section, char* entry, char* def);
int Write_ini_entry(char* file, char* section, char* entry, char* param);
void* translate_winpointer(void* winpointer);
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

int scrt_sprintf(char *str, const char *format, ...);
int scrt_vsprintf(char *str, const char *format, va_list ap);

/* CAN functions (to read or transmit CAN message from the model */

// Remarks: same structure insied  gleiche Strukturen inside CanFifo.h
typedef struct {
    uint32_t id;            // 4
    uint8_t data[8];        // 8
    uint8_t size;           // 1
    uint8_t ext;            // 1
    uint8_t flag;           // 1
    uint8_t channel;        // 1
    uint8_t node;           // 1      1 -> was transmitted by oneself
    uint8_t fill[7];        // 7
    uint64_t timestamp;   // 8
} __attribute__((packed)) CAN_FIFO_ELEM;                  // 32 Byte

typedef struct {
    int32_t Channel;
    uint32_t Start;
    uint32_t Stop;
    int32_t Fill1;
}  __attribute__((packed)) CAN_ACCEPT_MASK;

/* The parameter 'process' can be set with following value */
#define DEFAULT_PROCESS_PARAMETER   0xFFFFFFFF

int CreateCanFifos(int Depth, uint32_t Process);
int DeleteCanFifos(uint32_t Process);
int FlushCanFifo(uint32_t Process, int Flags);
int SetAcceptMask4CanFifo(uint32_t Process, CAN_ACCEPT_MASK *cam, int Size);

// FIFO->Process
int ReadCanMessageFromFifo2Process(uint32_t Process, CAN_FIFO_ELEM *pCanMessage);
int ReadCanMessagesFromFifo2Process(uint32_t Process, CAN_FIFO_ELEM *pCanMessage, int MaxMessages);
// Process->FIFO
int WriteCanMessagesFromProcess2Fifo(uint32_t Process, CAN_FIFO_ELEM *pCanMessage, int MaxMessages);
int WriteCanMessageFromProcess2Fifo(uint32_t Process, CAN_FIFO_ELEM *pCanMessage);

int SendCanObjectForOtherProcesses(int Channel, unsigned int Id, int Ext, int Size, char *Data);

/* Process task control block (stores the model process informationen) */
#define DEFINE_TASK_CONTROL_BLOCK(name, prio) \
char RealtimeModelName[512] = #name; \
int RealtimeModelPriority = prio;

#ifdef __cplusplus
}
#endif

#endif


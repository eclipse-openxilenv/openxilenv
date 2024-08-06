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


#ifndef TRACEWRITEFILE_H
#define TRACEWRITEFILE_H

#include "TraceRecorder.h"
#include "Blackboard.h"

/* ------------------------------------------------------------ *\
 *    Global defines  					        *
\* ------------------------------------------------------------ */

/* lese Test-Makros */
#ifdef FILEWROP_C
    #ifdef FILEWROP_TEST
        #define TEST
    #endif
    #include "test_mac.h"
    #undef TEST
#endif

#if 0
/* Look back Ring Buffer Size als Potenz von zwei */
#define TWO_POT_LOCK_BACK_DEPTH  2
#define LOCK_BACK_DEPTH  (1 << TWO_POT_LOCK_BACK_DEPTH)
#define RINGBUFF_CLIP_MASK  ((1 << TWO_POT_LOCK_BACK_DEPTH) - 1)

#define WRITE_FILE_ERROR    -1501
#define READ_FILE_ERROR     -1502


/* ------------------------------------------------------------ *\
 *    Type definitions 					        *
\* ------------------------------------------------------------ */

typedef struct {
    unsigned long timestamp;     /* Zeitmarke in Timer-Clock-Ticks */
    VARI_IN_PIPE *entrylist;     /* Zeiger auf Ringpuffer-Eintraege */
} RING_BUFFER_COLOMN;            /* 8 Bytes 2^3 */

#endif

/* MCS-Rec-Dateien */
typedef struct {
    long fileEnd;
    long filePos;
    unsigned short vers;
    unsigned short dataBeg;
    unsigned short recSize;
    char usrCmnt[79+1];
    float sampleRate;
    float eventTick;
    unsigned short nSrcRefs;
    unsigned short nElemRefs;
} recoFileHdr;

typedef struct {
    char robName[79+1];
    char binName[79+1];
} recoSrcRef;

typedef struct {
    unsigned short recoOfs;
    char srcFile;
    char kname[16+1];
} recoElemRef;


extern int conv_rec_time_steps;    /*  Recording step size */

extern int read_next_word (FILE *file, char *word, int maxlen);

extern int open_write_stimuli_head (START_MESSAGE_DATA hdrec_data,
                                    int32_t *vids, FILE **pfile);

extern int open_write_mcsrec_head (START_MESSAGE_DATA hdrec_data,
                                   int32_t *vids, FILE **pfile);

extern int conv_bin_stimuli_write (RECORDER_STRUCT *pRecorder, 
                                   uint32_t time_stamp, int elem_count,
                                   VARI_IN_PIPE *pipevari_list, long *vids);


extern void put_varilist_ringbuff (RING_BUFFER_COLOMN *ring_buff,
                                   VARI_IN_PIPE *pipevari_list, int32_t *vids,
                                   int pipevari_count);

int write_ringbuff_stimuli (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count);

int write_ringbuff_mcsrec  (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count);

int TailStimuliFile (FILE *fh);

int WriteCommentToStimuli (FILE *File, uint64_t Tmestamp, const char *Comment);

#undef extern
#endif

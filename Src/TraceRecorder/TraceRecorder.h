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


#ifndef TRACERECORDER_H
#define TRACERECORDER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "tcb.h"
#include "Blackboard.h"
#include "Message.h"
#include "ReadFromBlackboardPipe.h"


/* Error types */
#define INIT_REC_ERROR      -901
#define MESSAGE_ORDER_ERR   -902
#define VARIABLE_COUNT_ERR  -903
#define HDREC_NO_MEMMORY    -904
#define RDPIPE_NOT_RUNNING  -905
#define REC_NOT_STARTED     -906
#define CANNOT_OPEN_RECFILE -907
#define CANNOT_ATACH_FRAME  -908

typedef struct {           /* Message for starting the recorder */
    int FormatFlags; /* Recording format flag 0-> ASCII 1-> binaer */
#define REC_FORMAT_FLAG_TEXT_STIMULI     0x00000001
#define REC_FORMAT_FLAG_MDF              0x00000008
    int FormatCounter;
    char Filename[MAX_PATH];   /* File name */
    char Comment[MAX_PATH];    /* Comment */
    char RecordingName[MAX_PATH];    /* Name of the recording  */
    char DescriprionName[MAX_PATH];/* Name of the A2L */
    int AutoGenDescriprionFlag;        /* if this is 1 a description A2L file would be generated */
    char Trigger[BBVARI_NAME_SIZE];  /* Name of the trigge variable */
    int32_t TriggerVid;             /* Variable-ID of the trigger variable */
    double TriggerValue; /* Trigger level */
    int RecTimeSteps;   /* Recording step size */
    double SamplePeriod;       /* Period of the sample frequency */
    int VariableCount;            /* Number of the variable */
    uint32_t SampleCounter;    /* Sampe counter */
    uint32_t MaxSampleCounter;      /* max. samples should be recorded */
    int PhysicalFlag;
} START_MESSAGE_DATA;


/* Look back ring buffer size must be 2^n */
#define TWO_POT_LOCK_BACK_DEPTH  2
#define LOCK_BACK_DEPTH  (1 << TWO_POT_LOCK_BACK_DEPTH)
#define RINGBUFF_CLIP_MASK  ((1 << TWO_POT_LOCK_BACK_DEPTH) - 1)

#define WRITE_FILE_ERROR    -1501
#define READ_FILE_ERROR     -1502


typedef struct {
    uint64_t TimeStamp;       /* Time stamp in timer clock ticks */
    VARI_IN_PIPE *EntryList;  /* Pointer to a ring buffer entry */
} RING_BUFFER_COLOMN;


typedef struct {
    FILE *StimuliFile;          /* Handle for recorder file */
    FILE *MdfFile;              /* Handle for recorder file */
    struct PIPE_VARI *MessageBuffer;  /* Pointer to the message buffer */
    int MessageBufferSize;              /* Size of the message buffer */

    START_MESSAGE_DATA StartMessage;
    char **RecordVariables;       /* Array of all variable names */
    char *RecordVariablesBuffer;
    int RecordVariablesBufferSize;
    int RecordVariBufferPos;
    int RecordVariablesCount;    /* Number of recording variables */

    int RecorderStatus;      /* State of the recorder */
    #define HDREC_SLEEP      1
    #define HDREC_INIT       2
    #define HDREC_RECORD     3
    int32_t RecorderStatusVid;

    int TextFileStatus;       /* Status of the file */
    int MdfFileStatus;           /* Status of the file */
    int AllFileStatus;           

    uint32_t LineNumberCounter;
    uint64_t CurrentTimeStamp;  /* zeigt immer aktuellstes Datum an */
    double d_stimuli_timestamp_counter;  /* zeigt immer aktuellstes Datum an */
    uint64_t TimeStep;  /* Time step for one sample */
    RING_BUFFER_COLOMN RingBuffer[LOCK_BACK_DEPTH]; /* Ring buffer */
    int RinBufferCurrentPosition;           /* Read pointer inside the ring buffer */
    RING_BUFFER_COLOMN *RingPufferPtr;  // == &RingBuffer[RinBufferCurrentPosition]
    RING_BUFFER_COLOMN Stamp; /* Stamp of the current sample values */
    int RingBufferVariableCount;         /* Ringpuffer width (variable count) */
    int StartFlag;       /* Is 1 for the first sample  */
    int ConverterStatus;      /* Converter status 0: off 1: on */
    uint64_t RecordStartTime;  /* Start of the recording */

    int32_t *Vids;
    char *PhysicalFlags;
    char TextFileName[MAX_PATH];      /* Name of the recording file */
    char MdfFileName[MAX_PATH];          /* Name of the recording file */

    int ReqFiFo;
    int AckFiFo;
    int DataFiFo;

    int AckPid;

} RECORDER_STRUCT;


int InitFileWriter (RECORDER_STRUCT *Recorder, uint32_t time_stamp, int elem_count);
int TerminateFileWriter (RECORDER_STRUCT *Recorder);
int WriteWriter (RECORDER_STRUCT *Recorder, uint64_t time_stamp, int elem_count, VARI_IN_PIPE *pipevari_list);

void cyclic_hdrec (void);
int init_hdrec (void);
void terminate_hdrec (void);

int start_recorder (MESSAGE_HEAD *mhead, RECORDER_STRUCT *Recorder);
int init_recorder_message (MESSAGE_HEAD *mhead);
int init_recorder (void);

int stop_recorder (int32_t *vids);

/* ------------------------------------------------------------ *\
 *    Task Control Block                                     *
\* ------------------------------------------------------------ */

extern TASK_CONTROL_BLOCK hdrec_tcb;

#endif

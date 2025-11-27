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


#include <stdio.h>
#include <string.h>
#include "Config.h"

#include "TraceRecorder.h"

#include "ReadFromBlackboardPipe.h"
#include "Scheduler.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "Message.h"
#include "Fifos.h"
#include "Files.h"
#include "TraceWriteFile.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ReadConfig.h"
#include "TraceWriteMdfFile.h"
#include "TraceWriteMdf4File.h"
#include "TimeStamp.h"
#include "ExportA2L.h"

#define UNUSED(x) (void)(x)

static RECORDER_STRUCT RecorderData;

int init_recorder (void)
{
    int vid;
    RECORDER_STRUCT *Recorder = &RecorderData;

    if ((Recorder->RecordVariables = (char**)my_malloc ((size_t)Recorder->StartMessage.VariableCount * sizeof(char*))) == NULL) {
        return NO_FREE_MEMORY;
    }

    /* Read sample period from blackboard */
    vid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SAMPLE_TIME), BB_DOUBLE, "s");
    if (vid <= 0) {
        ThrowError (1, "cannot attach variable 'abt_per'");
    }
    /* Read sample frequency */
    Recorder->StartMessage.SamplePeriod = read_bbvari_double (vid)
        * Recorder->StartMessage.RecTimeSteps;
    remove_bbvari (vid);

    if (strlen (Recorder->StartMessage.Trigger)) {
        Recorder->StartMessage.TriggerVid = add_bbvari (Recorder->StartMessage.Trigger, BB_UNKNOWN, NULL);
    } else {
        Recorder->StartMessage.TriggerVid = -1;
    }
    Recorder->StartMessage.SampleCounter = 0;

    Recorder->RecordVariablesCount = 0;
    Recorder->RecorderStatus = HDREC_INIT;
    return 0;
}

int init_recorder_message (MESSAGE_HEAD *mhead)
{
    MESSAGE_HEAD dummy_mhead;
    RECORDER_STRUCT *Recorder = &RecorderData;

    /* Read message */
    if (sizeof (START_MESSAGE_DATA) == mhead->size) {
        read_message (&dummy_mhead, (char*)&Recorder->StartMessage, mhead->size);
    } else {
       remove_message ();
       return INIT_REC_ERROR;
    }
    return init_recorder ();
}


int start_recorder (MESSAGE_HEAD *mhead, RECORDER_STRUCT *Recorder)
{
    uint64_t wrflag;
    int32_t *vids;
    char *dec_phys_flags;
    int x, y;
    int pid;
    int status;
    TRIGGER_INFO_MESS trigg_info;

    /* Delete message */
    if (mhead != NULL) remove_message ();

    if (Recorder->RecorderStatus != HDREC_INIT) {
        ThrowError (1, "START_REC_MESSAGE before INIT_REC_MESSAGE");
        Recorder->RecorderStatus = HDREC_SLEEP;
        return MESSAGE_ORDER_ERR;
    }

    if (Recorder->RecordVariablesCount != Recorder->StartMessage.VariableCount) {
        ThrowError (1, "noch nicht alle VARI_REC_MESSAGE's wie INIT_REC_MESSAGE angemeldet gesendet");
        Recorder->RecorderStatus = HDREC_SLEEP;
        return VARIABLE_COUNT_ERR;
    }

    vids = (int32_t*)my_malloc (sizeof (int32_t) * (size_t)(Recorder->RecordVariablesCount + 1));
    if (vids == NULL) return HDREC_NO_MEMMORY;

    /* Fetch all variable ID's */
    for (x = 0; x < Recorder->RecordVariablesCount; x++) {
        if ((vids[x] = add_bbvari (Recorder->RecordVariables[x], BB_UNKNOWN, NULL)) <= 0) {
            ThrowError (1, "cannot record variable %s", Recorder->RecordVariables[x]);
            Recorder->RecordVariablesCount--;
            for (y = x; y < Recorder->RecordVariablesCount; y++) {
                Recorder->RecordVariables[y] = Recorder->RecordVariables[y+1];
            }
            x--;
        }
    }
    vids[Recorder->RecordVariablesCount] = 0;
    Recorder->Vids = vids;

    if (Recorder->StartMessage.PhysicalFlag) {
        dec_phys_flags = my_calloc ((size_t)(Recorder->RecordVariablesCount+1), 1);
		if (dec_phys_flags != NULL) {
            for (x = 0; x <= Recorder->RecordVariablesCount; x++) {
                if (get_bbvari_conversiontype (vids[x]) == 1) dec_phys_flags[x] = 1;
                else dec_phys_flags[x] = 0;
            }
            dec_phys_flags[Recorder->RecordVariablesCount] = 0;
		}
	} else dec_phys_flags = NULL;
    Recorder->PhysicalFlags = dec_phys_flags;

    /* Init file writer */
    status = InitFileWriter (Recorder, get_sched_periode_timer_clocks (), Recorder->RecordVariablesCount);
    if (status == 0) {
        /* Search RDPIPE process */
        if ((pid = get_pid_by_name (PN_TRACE_QUEUE)) < 0) {
            ThrowError (1, "process 'TraceQueue' not found");
            Recorder->RecorderStatus = HDREC_SLEEP;
            return RDPIPE_NOT_RUNNING;
        }
        if ((Recorder->ReqFiFo = TxAttachFiFo(GET_PID(), READ_PIPE_RECORDER_REQ_FIFO_NAME)) < 0) {
            ThrowError (1, "fifo \"%s\" not found", READ_PIPE_RECORDER_REQ_FIFO_NAME);
            Recorder->RecorderStatus = HDREC_SLEEP;
            return RDPIPE_NOT_RUNNING;
        }
        /* Transmit registration of the variables to the RDPIPE process */
        if (WriteToFiFo (Recorder->ReqFiFo, RDPIPE_LOGON, GET_PID(), get_timestamp_counter(), (Recorder->RecordVariablesCount+1) * (int)sizeof (int32_t), (char*)vids)) {
            ThrowError (1, "pipe overflow");
            Recorder->RecorderStatus = HDREC_SLEEP;
            return RDPIPE_NOT_RUNNING;
        }
        if (Recorder->StartMessage.PhysicalFlag) {
            if (WriteToFiFo (Recorder->ReqFiFo, RDPIPE_DEC_PHYS_MASK, GET_PID(), get_timestamp_counter(), Recorder->RecordVariablesCount+1, dec_phys_flags)) {
				ThrowError (1, "pipe overflow");
                Recorder->RecorderStatus = HDREC_SLEEP;
				return RDPIPE_NOT_RUNNING;
			}
		} 

        /* Transmit trigger variable */
        if (Recorder->StartMessage.TriggerVid > 0) {
            trigg_info.trigger_vid = Recorder->StartMessage.TriggerVid;
            trigg_info.trigger_value = Recorder->StartMessage.TriggerValue;
            if (WriteToFiFo (Recorder->ReqFiFo, RDPIPE_SEND_TRIGGER, GET_PID(), get_timestamp_counter(), sizeof (TRIGGER_INFO_MESS), (char*)&trigg_info)) {
                ThrowError (1, "pipe overflow");
                Recorder->RecorderStatus = HDREC_SLEEP;
                return RDPIPE_NOT_RUNNING;
            }
        }

        /* Transmit recording length */
        if (WriteToFiFo (Recorder->ReqFiFo, RDPIPE_SEND_LENGTH, GET_PID(), get_timestamp_counter(), sizeof (unsigned long), (char*)&(Recorder->StartMessage.MaxSampleCounter))) {
            ThrowError (1, "pipe overflow\n");
            Recorder->RecorderStatus = HDREC_SLEEP;
            return RDPIPE_NOT_RUNNING;

        }

        /* Fetch free write flag */
        if ((status = get_free_wrflag (get_pid_by_name (PN_TRACE_RECORDER), &wrflag)) != 0) return status;
        /* Transmit write flag to RDPIPE process */
        if (WriteToFiFo (Recorder->ReqFiFo, RDPIPE_SEND_WRFLAG, GET_PID(), get_timestamp_counter(), sizeof (uint64_t), (char*)&wrflag)) {
            ThrowError (1, "pipe overflow");
            Recorder->RecorderStatus = HDREC_SLEEP;
            return RDPIPE_NOT_RUNNING;
        }
        write_bbvari_ubyte (Recorder->RecorderStatusVid, 1);

        Recorder->RecorderStatus = HDREC_RECORD;
        Recorder->Vids = vids;    /* Pointer to variable ID list */
        Recorder->PhysicalFlags = dec_phys_flags;
        Recorder->RinBufferCurrentPosition = 0;
    } else {
        TerminateFileWriter (Recorder);
    }
    return status;    /* all OK */
}

int stop_recorder (int32_t *vids)
{
    int pid;
    int x;
    RECORDER_STRUCT *Recorder = &RecorderData;

    if (Recorder->RecorderStatus != HDREC_RECORD) {
        ThrowError (1, "STOP_REC_MESSAGE vor START_REC_MESSAGE");
        remove_message ();
        return REC_NOT_STARTED;
    }
    /* Fetch PID of RDPIPE process */
    if ((pid = get_pid_by_name (PN_TRACE_QUEUE)) < 0) {
        printf ("process %s not found", PN_TRACE_QUEUE);
        return RDPIPE_NOT_RUNNING;
    }
    /* Transmit registration to RDPIPE process */
    x = WriteToFiFo (Recorder->ReqFiFo, RDPIPE_LOGOFF, GET_PID(), get_timestamp_counter(), 0, NULL);
    if (x != 0 && (x != FIFO_ERR_UNKNOWN_HANDLE)) {  // If XilEnv will be closed during running recorder the fifo can be removed
        ThrowError (1, "pipe overflow");
    }

    /* Close file converter */
    TerminateFileWriter (Recorder);

    Recorder->RecorderStatus = HDREC_SLEEP;
    write_bbvari_ubyte (Recorder->RecorderStatusVid, 0);

    /* remove all variables */
    for (x = 0; x < Recorder->RecordVariablesCount; x++) {
        remove_bbvari (vids[x]);
        vids[x] = 0;
    }
    if (Recorder->RecordVariables != NULL) my_free (Recorder->RecordVariables);
    Recorder->RecordVariables = NULL;
    Recorder->RecordVariablesCount = 0;
    if (Recorder->RecordVariablesBuffer != NULL) my_free (Recorder->RecordVariablesBuffer);
    Recorder->RecordVariablesBuffer = NULL;

    return 0;
}

static int BuildRecorderFileNames (RECORDER_STRUCT *Recorder)
{
    char *p, *pend;
    int StartExt;

    strcpy (Recorder->TextFileName, Recorder->StartMessage.Filename);
    strcpy (Recorder->MdfFileName, Recorder->StartMessage.Filename);
    strcpy (Recorder->Mdf4FileName, Recorder->StartMessage.Filename);

    // If there would be writen more than oe file format the file extension will be ignored
    // and the default file extension would be used
    if (Recorder->StartMessage.FormatCounter > 1) {
        p = Recorder->StartMessage.Filename;
        while (*p != 0) p++;
        pend = p;
        while (*p != '.') {
            if (p == Recorder->StartMessage.Filename) {
                // There is a file extension
                p = pend; 
                break;
            }
            p--;
        }
        StartExt = (int)(p - Recorder->StartMessage.Filename);
        strcpy (Recorder->TextFileName + StartExt, ".dat"); 
        strcpy (Recorder->MdfFileName + StartExt, ".mdf");
        strcpy (Recorder->Mdf4FileName + StartExt, ".mf4");
    }
    return 0;
}

static int AddVariable( RECORDER_STRUCT *Recorder, char *Variable)
{
    int Len = (int)strlen (Variable) + 1;
    if ((Recorder->RecordVariBufferPos + Len + 1) >= Recorder->RecordVariablesBufferSize) {
        Recorder->RecordVariablesBufferSize += Recorder->RecordVariablesBufferSize / 4 + 1024;
        if (((Recorder->RecordVariablesBuffer = (char*)my_realloc (Recorder->RecordVariablesBuffer, Recorder->RecordVariablesBufferSize)) == NULL)) {
            ThrowError (1, "out of memory");
            return -1;
        }
    }
    MEMCPY(Recorder->RecordVariablesBuffer + Recorder->RecordVariBufferPos, Variable, Len);
    Recorder->RecordVariBufferPos += Len;
    Recorder->RecordVariablesBuffer[Recorder->RecordVariBufferPos] = 0;
    return 0;
}

int read_recorder_configfile (MESSAGE_HEAD *mhead, RECORDER_STRUCT *Recorder)
{
    char word[BBVARI_NAME_SIZE];
    char filename[MAX_PATH];
    FILE *fh;
    int token, token_count = 0;
    int config_file_line_counter;

    if (Recorder->RecorderStatus != HDREC_SLEEP) {
        ThrowError (1, "recorder already running");
        remove_message ();
        return REC_NOT_STARTED;
    }

    read_message (mhead, filename, MAX_PATH);

    if ((fh = open_file (filename, "rt")) == NULL) {
        ThrowError (1, "cannot open file %s", filename);
        return -1;
    }
    Recorder->StartMessage.PhysicalFlag = 0;
    Recorder->StartMessage.FormatCounter = 0;
    Recorder->StartMessage.FormatFlags = 0;
    Recorder->StartMessage.AutoGenDescriprionFlag = 0;
    Recorder->StartMessage.DescriprionName[0] = '\0';

    Recorder->StartMessage.TriggerVid = 0;
    Recorder->StartMessage.Trigger[0] = 0;
    Recorder->StartMessage.TriggerValue = 0.0;

    config_file_line_counter = 1;
    while ((token = read_token (fh, word, sizeof(word)-1, &config_file_line_counter)) != EOF) {
        token_count++;
        if ((token_count == 1) && (token != HDRECORDER_CONFIG_FILE_TOKEN)) {
            ThrowError (1, "file %s(%i):\r\n"
                        "   erste Anweisung nicht 'HDRECORDER_CONFIG_FILE'\r\n",
                      filename, config_file_line_counter);
            close_file (fh);
            return -1;
        } else {
            switch (token) {
            case HDRECORDER_FILE_TOKEN:
                if (read_word_with_blacks (fh, Recorder->StartMessage.Filename, MAX_PATH, &config_file_line_counter) != 0) {
                    ThrowError (1, "file %s(%i):\r\n"
                                "   erwarte ein Filename in HDRECORDER_FILE-Anweisung\n",
                           filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                break;
            case HDRECORDER_SAMPLERATE_TOKEN:
                if (read_token (fh, word, sizeof(word)-1, &config_file_line_counter) != EQUAL_TOKEN) {
                    ThrowError (1, "file %s(%i):\r\n"
                                "   erwarte ein = in HDRECORDER_SAMPLERATE-Anweisung\n",
                           filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                if (read_token (fh, word, sizeof(word)-1, &config_file_line_counter) != 0) {
                    ThrowError (1, "file %s(%i):\r\n"
                                "   erwarte eine Zahl in HDRECORDER_SAMPLERATE-Anweisung\n",
                           filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                SSCANF_ERR (word, "%i", &(Recorder->StartMessage.RecTimeSteps));
                // 1, 2, 3, ...
                if (Recorder->StartMessage.RecTimeSteps < 1) Recorder->StartMessage.RecTimeSteps = 1;
                break;
            case HDRECORDER_SAMPLELENGTH_TOKEN:
                if (read_token (fh, word, sizeof(word)-1, &config_file_line_counter) != EQUAL_TOKEN) {
                    ThrowError (1, "file %s(%i):\r\n"
                                "   erwarte ein = in HDRECORDER_SAMPLELENGTH-Anweisung\n",
                           filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                if (read_token (fh, word, sizeof(word)-1, &config_file_line_counter) != 0) {
                    ThrowError (1, "file %s(%i):\r\n"
                                "   erwarte eine Zahl in HDRECORDER_SAMPLELENGTH-Anweisung\n",
                           filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                SSCANF_ERR (word, "%i", &(Recorder->StartMessage.MaxSampleCounter));
                Recorder->StartMessage.SampleCounter = 0;
                break;
            case TRIGGER_TOKEN:
                if (read_token (fh, word, sizeof (NAME_STRING)-1, &config_file_line_counter) != 0) {
                    ThrowError (1, "file %s(%i):\r\n"
                                "   erwarte einen Name in TRIGGER-Anweisung\r\n",
                    filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                strcpy (Recorder->StartMessage.Trigger, word);

                read_token (fh, word, sizeof(word)-1, &config_file_line_counter);
                if (strcmp (word, ">")) {
                    ThrowError (1, "in Datei %s(%i):\r\n"
                                "   erwarte ein > in TRIGGER-Anweisung\n",
                           filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                if (read_token (fh, word, sizeof(word)-1, &config_file_line_counter) != 0) {
                    ThrowError (1, "in Datei %s(%i):\r\n"
                                "   erwarte eine Zahl in TRIGGER-Anweisung\n",
                           filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                SSCANF_ERR (word, "%lf", &(Recorder->StartMessage.TriggerValue));
                break;
            case STARTVARLIST_TOKEN:
                Recorder->StartMessage.VariableCount = 0;
                strcpy (Recorder->StartMessage.Comment, Recorder->StartMessage.Filename);
                strcpy (Recorder->StartMessage.RecordingName, Recorder->StartMessage.Filename);

                Recorder->RecordVariablesBufferSize = 1024;  // Start with 1024 bytes
                Recorder->RecordVariBufferPos = 0;
                if ((Recorder->RecordVariablesBuffer = (char*)my_malloc (Recorder->RecordVariablesBufferSize)) == NULL) {
                    ThrowError (1, "out of memory");
                    goto ERROR_IN_CONFIGFILE;
                }
                Recorder->RecordVariablesBuffer[0] = 0;

                init_recorder ();

                while ((token = read_token (fh, word, sizeof(word)-1, &config_file_line_counter)) != ENDVARLIST_TOKEN) {
                    switch (token) {
                    case EOF:
                        ThrowError (1, "inside file %s(%i):\n"
                                    "   missing ENDVARLIST statement",
                               filename, config_file_line_counter);
                        goto ERROR_IN_CONFIGFILE;
                    case VARIABLE_TOKEN:
                        if (read_token (fh, word, sizeof (word)-1, &config_file_line_counter) != 0) {
                            ThrowError (1, "inside file %s(%i):\r\n"
                                        "   expecting a name in VARIABLE statement",
                                   filename, config_file_line_counter);
                            goto ERROR_IN_CONFIGFILE;
                        }
                        if (AddVariable(Recorder, word)) {
                            goto ERROR_IN_CONFIGFILE;
                        }
                        Recorder->StartMessage.VariableCount++;
                        break;
                    case ALL_LABEL_OF_PROCESS_TOKEN:
                        if (read_token (fh, word, sizeof (word)-1, &config_file_line_counter) != 0) {
                            ThrowError (1, "file %s(%i):\n"
                                        "   expecting aprocess name in ALL_LABEL_OF_PROCESS statement",
                                filename, config_file_line_counter);
                            goto ERROR_IN_CONFIGFILE;
                        }
                        {
                            PID pid;
                            int index = 0;
                            if ((pid = get_pid_by_name (word)) < 0) {
                                ThrowError (1, "cannot record variables of process %s, process is not running", word);
                            }
                            while ((index = ReadNextVariableProcessAccess (index, pid, 3, word, sizeof(word))) > 0) {
                                if (AddVariable(Recorder, word)) {
                                    goto ERROR_IN_CONFIGFILE;
                                }
                                Recorder->StartMessage.VariableCount++;
                            }
                        }
                        break;
                    default:
                        ThrowError (1, "inside file %s(%i):\n"
                                    "   not expected token %s expect a VARIABLE token",
                                    filename, config_file_line_counter, word);
                        goto ERROR_IN_CONFIGFILE;
                    }
                }

                break;
            case MDF_FORMAT_TOKEN:
                Recorder->StartMessage.FormatFlags |= REC_FORMAT_FLAG_MDF;
                Recorder->StartMessage.FormatCounter++;
                break;
            case MDF4_FORMAT_TOKEN:
                Recorder->StartMessage.FormatFlags |= REC_FORMAT_FLAG_MDF4;
                Recorder->StartMessage.FormatCounter++;
                break;
            case TEXT_FORMAT_TOKEN:
                Recorder->StartMessage.FormatFlags |= REC_FORMAT_FLAG_TEXT_STIMULI;
                Recorder->StartMessage.FormatCounter++;
                break;
            case HDRECORDER_PHYSICAL_TOKEN:
                Recorder->StartMessage.PhysicalFlag = 1;
                break;
            case DESCRIPTION_FILE_TOKEN:
                if (read_word_with_blacks (fh, Recorder->StartMessage.DescriprionName, sizeof(Recorder->StartMessage.DescriprionName)-1, &config_file_line_counter) != 0) {
                    ThrowError (1, "file %s(%i):\r\n"
                              "   expecting a file name in DESCRIPTION_FILE statement",
                           filename, config_file_line_counter);
                    close_file (fh);
                    return -1;
                }
                break;
            case AUTOGEN_DESCRIPTION_FILE_TOKEN:
                Recorder->StartMessage.AutoGenDescriprionFlag = 1;
                break;
            }
        }
    }
    if ((Recorder->RecordVariables = (char**)my_malloc (Recorder->StartMessage.VariableCount * sizeof (char*))) == NULL) {
        ThrowError (1, "out of memory");
        goto ERROR_IN_CONFIGFILE;
    } else {
        char *p = Recorder->RecordVariablesBuffer;
        Recorder->RecordVariablesCount = 0;
        while (*p != 0) {
            Recorder->RecordVariables[Recorder->RecordVariablesCount] = p;
            Recorder->RecordVariablesCount++;
            if (Recorder->RecordVariablesCount > Recorder->StartMessage.VariableCount) {
                ThrowError (1, "internal %s (%i)", __FILE__, __LINE__);
                goto ERROR_IN_CONFIGFILE;
            }
            while (*p != 0) p++;
            p++;
        }
    }

    // If there is no format defined use text stimulus (*.DAT) format
    if (Recorder->StartMessage.FormatFlags == 0)  {
        Recorder->StartMessage.FormatFlags = REC_FORMAT_FLAG_TEXT_STIMULI;
        Recorder->StartMessage.FormatCounter++;
    }
    close_file (fh);
    if (Recorder->StartMessage.AutoGenDescriprionFlag) {
        if (strlen (Recorder->StartMessage.DescriprionName) == 0) {
            ThrowError (1, "cannot generate description file because there is no name secified");
        } else {
            ExportA2lFile (Recorder->StartMessage.DescriprionName);
        }
    }
    BuildRecorderFileNames (Recorder);
    start_recorder (NULL, Recorder);
    return 0;
ERROR_IN_CONFIGFILE:
    if (Recorder->RecordVariables != NULL) my_free(Recorder->RecordVariables);
    Recorder->RecordVariables = NULL;
    if (Recorder->RecordVariablesBuffer != NULL) my_free(Recorder->RecordVariablesBuffer);
    Recorder->RecordVariablesBuffer = NULL;
    close_file (fh);
    return -1;
}

int WriteCommentToRecord (MESSAGE_HEAD *mhead, RECORDER_STRUCT *Recorder)
{
    if (Recorder->RecorderStatus != HDREC_RECORD) {
        ThrowError (WARNING_NO_STOP, "comment will be ignored because recorder not running");
        remove_message ();
        return -1;
    }
    char *Comment = (char*)my_malloc(mhead->size);
    if(Comment != NULL) {
        read_message (mhead, Comment, mhead->size);
        if (!Recorder->MdfFileStatus) {
            if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF) == REC_FORMAT_FLAG_MDF) {
                WriteCommentToMdf (Recorder->MdfFile, mhead->timestamp, Comment);
            }
        }
        if (!Recorder->Mdf4FileStatus) {
            if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF4) == REC_FORMAT_FLAG_MDF4) {
                WriteCommentToMdf4 (Recorder->MdfFile, mhead->timestamp, Comment);
            }
        }
        if (!Recorder->TextFileStatus) {
            if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_TEXT_STIMULI) == REC_FORMAT_FLAG_TEXT_STIMULI) {
                WriteCommentToStimuli (Recorder->StimuliFile, mhead->timestamp, Comment);
            }
        }
        my_free(Comment);
        return 0;
    } else {
        remove_message();
        return -1;
    }
}

static void cleanup_vids (int32_t **vids, char **dec_phys_flags)
{
    if ((vids != NULL) && (*vids != NULL)) {
        my_free (*vids);
        *vids = NULL;
    }
    if ((dec_phys_flags != NULL) && (*dec_phys_flags != NULL)) {
        my_free (*dec_phys_flags);
        *dec_phys_flags = NULL;
    }
}


void cyclic_hdrec (void)
{
    int varicount;
    MESSAGE_HEAD mhead;
    FIFO_ENTRY_HEADER Header;
    int message_count = 0;
    //int pid = -1;
    RECORDER_STRUCT *Recorder = &RecorderData;

    while ((CheckFiFo (Recorder->DataFiFo, &Header) > 0) && (message_count++ < 20)) {
        switch (Header.MessageId) {
        case RECORD_MESSAGE:    /* Message contains Data of the recording variables */
            if (Recorder->RecorderStatus == HDREC_RECORD) {
                write_bbvari_ubyte (Recorder->RecorderStatusVid, 2);

                if ((int)Header.Size > Recorder->MessageBufferSize) {  /* is the buffer large enough? */
                    Recorder->MessageBufferSize = Header.Size;
                    if (Recorder->MessageBuffer != NULL) {  /* Are a buffer exists */
                         my_free (Recorder->MessageBuffer);
                    }
                    /* new larger message buffer */
                    Recorder->MessageBuffer = (struct PIPE_VARI*)my_malloc (Recorder->MessageBufferSize);
                }
                /* Read message content */
                ReadFromFiFo (Recorder->DataFiFo, &Header, (char*)Recorder->MessageBuffer, Recorder->MessageBufferSize);
                varicount = Header.Size / (int32_t)sizeof (struct PIPE_VARI);

                /* Convert  data and write it to the recording file(s) */
                if (WriteWriter (Recorder, Header.Timestamp, varicount, Recorder->MessageBuffer)) {
                    ThrowError (1, "disc full! Stop recoring");
                    stop_recorder (Recorder->Vids);
                    cleanup_vids (&Recorder->Vids, &Recorder->PhysicalFlags);
                }
            } else RemoveOneMessageFromFiFo (Recorder->DataFiFo);
            break;
        case STOP_REC_MESSAGE:       /* Stop recording */
            RemoveOneMessageFromFiFo (Recorder->DataFiFo);
            if (Recorder->RecorderStatus != HDREC_SLEEP) {
                stop_recorder (Recorder->Vids);
                cleanup_vids (&Recorder->Vids, &Recorder->PhysicalFlags);
            }
            break;
        default:
            RemoveOneMessageFromFiFo (Recorder->DataFiFo);
            ThrowError (1, "unknown MESSAGE  %i:  %i\n", Header.TramsmiterPid, Header.MessageId);
        }

    }

    if (CheckFiFo (Recorder->AckFiFo, &Header) > 0) {
        switch (Header.MessageId) {
        case RDPIPE_ACK:
            RemoveOneMessageFromFiFo (Recorder->AckFiFo);
            write_message (Recorder->AckPid, HDREC_ACK, 0, NULL);
            break;
        default:
            RemoveOneMessageFromFiFo (Recorder->AckFiFo);
            ThrowError (1, "unknown MESSAGE  %i:  %i\n", Header.TramsmiterPid, Header.MessageId);
        }
    }

    while (test_message (&mhead)) {
                                 /* Check if a message is received */
                                 /* Analyze max. 20 messages inside one cycle */
        switch (mhead.mid) {
            case STOP_REC_MESSAGE:       /* Stop recording */
                remove_message ();
                if (Recorder->RecorderStatus != HDREC_SLEEP) {
                    stop_recorder (Recorder->Vids);
                    cleanup_vids (&Recorder->Vids, &Recorder->PhysicalFlags);
                }
                break;
            case REC_CONFIG_FILENAME_MESSAGE:
                read_recorder_configfile (&mhead, Recorder);
                // Store pid how is sending the request to transmit later the acknowledge
                Recorder->AckPid = mhead.pid;
                break;
            case RDPIPE_ACK:
                write_message (Recorder->AckPid, HDREC_ACK, 0, NULL);
                remove_message ();
                break;
            case HDREC_COMMENT_MESSAGE:
                WriteCommentToRecord(&mhead, Recorder);
                break;
            default:
                remove_message ();
                ThrowError (1, "unknown MESSAGE  %i:  %i\n", mhead.pid, mhead.mid);
        }
    }
}

int init_hdrec (void)
{
    RECORDER_STRUCT *Recorder = &RecorderData;
    Recorder->RecorderStatus = HDREC_SLEEP;
    Recorder->MessageBuffer = NULL;
    Recorder->MessageBufferSize = 0;
    Recorder->RecordVariables = NULL;
    Recorder->RecordVariablesCount = 0;

    Recorder->AckFiFo = CreateNewRxFifo (GET_PID(), 1024, READ_PIPE_RECORDER_ACK_FIFO_NAME);
    Recorder->DataFiFo = CreateNewRxFifo (GET_PID(), 1024*1024, READ_PIPE_RECORDER_DATA_FIFO_NAME);

    Recorder->RecorderStatusVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_TRACE_RECORDER), BB_UBYTE, NULL);
    if (Recorder->RecorderStatusVid <= 0) {
        ThrowError (1, "cannot attach variable 'HD_Rec'");
        return -1; 
    }
    set_bbvari_conversion (Recorder->RecorderStatusVid, 2, "0 0 \"Sleep\"; 1 1 \"Trigger\"; 2 2 \"Rec\";");
    write_bbvari_ubyte (Recorder->RecorderStatusVid, 0);

    return 0;
}

void terminate_hdrec (void)
{
    RECORDER_STRUCT *Recorder = &RecorderData;

    if (Recorder->RecorderStatus == HDREC_RECORD) {
        stop_recorder (Recorder->Vids);
    }
    Recorder->RecorderStatus = HDREC_SLEEP;
    if (Recorder->MessageBuffer != NULL) my_free (Recorder->MessageBuffer);
    if (Recorder->RecordVariables != NULL) my_free (Recorder->RecordVariables);
    Recorder->MessageBuffer = NULL;
    Recorder->MessageBufferSize = 0;
    Recorder->RecordVariables = NULL;
    Recorder->RecordVariablesCount = 0;

    DeleteFiFo (Recorder->AckFiFo, GET_PID());
    DeleteFiFo (Recorder->DataFiFo, GET_PID());
}


void put_varilist_ringbuff (RING_BUFFER_COLOMN *ring_buff,
                                   VARI_IN_PIPE *pipevari_list, int32_t *vids,
                                   int pipevari_count)
{
    int i, x;

    /* Which variable are included */
    i = 0;
    for (x = 0; vids[x] > 0; x++) {
        /* Check and write to the ring buffer */
        if (vids[x] == pipevari_list[i].vid) {
            /* Copy pipe variable list element into ring buffer element */
            ring_buff->EntryList[x+2] = pipevari_list[i++];
        }
        if (i >= pipevari_count) break;
    }
}


static void CopyRingPufferToStamp (RECORDER_STRUCT *Recorder)
{
    int i;

    for (i = 0; i < Recorder->RingBufferVariableCount; i++) {
        if (i < 2) {     /* Step variable and time variable */
            if (i == 0) Recorder->Stamp.EntryList[0].value.udw = Recorder->LineNumberCounter - LOCK_BACK_DEPTH;
            if (i == 1) Recorder->Stamp.EntryList[1].value.d =
                        (double)(Recorder->RingPufferPtr->TimeStamp - Recorder->RecordStartTime) / TIMERCLKFRQ;
        } else if (Recorder->RingPufferPtr->EntryList[i].vid) {
            /* copy to stamp */
            Recorder->Stamp.EntryList[i] = Recorder->RingPufferPtr->EntryList[i];
            Recorder->RingPufferPtr->EntryList[i].vid = 0;
        }
    }
}


int InitFileWriter (RECORDER_STRUCT *Recorder, uint32_t time_stamp, int elem_count)
{
    int x;
    int status = 0;

    Recorder->TextFileStatus = 1;
    Recorder->MdfFileStatus = 1;
    Recorder->Mdf4FileStatus = 1;
    Recorder->AllFileStatus = 0;

    Recorder->LineNumberCounter = 0;

    if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF) == REC_FORMAT_FLAG_MDF) {
        strcpy (Recorder->StartMessage.Filename, Recorder->MdfFileName);
        if ((Recorder->MdfFileStatus = OpenWriteMdfHead (Recorder->StartMessage, Recorder->Vids, Recorder->PhysicalFlags, &Recorder->MdfFile)) != 0) {
            Recorder->AllFileStatus = Recorder->MdfFileStatus;
            Recorder->RecorderStatus = HDREC_SLEEP;
            return Recorder->MdfFileStatus;
        }
    }
    if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF4) == REC_FORMAT_FLAG_MDF4) {
        strcpy (Recorder->StartMessage.Filename, Recorder->Mdf4FileName);
        if ((Recorder->Mdf4FileStatus = OpenWriteMdf4Head (Recorder->StartMessage, Recorder->Vids, Recorder->PhysicalFlags, &Recorder->Mdf4File)) != 0) {
            Recorder->AllFileStatus = Recorder->Mdf4FileStatus;
            Recorder->RecorderStatus = HDREC_SLEEP;
            return Recorder->Mdf4FileStatus;
        }
    }
    if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_TEXT_STIMULI) == REC_FORMAT_FLAG_TEXT_STIMULI) {
        strcpy (Recorder->StartMessage.Filename, Recorder->TextFileName);
        if ((Recorder->TextFileStatus = open_write_stimuli_head (Recorder->StartMessage, Recorder->Vids, &Recorder->StimuliFile)) != 0) {
            Recorder->AllFileStatus = Recorder->TextFileStatus;
            Recorder->RecorderStatus = HDREC_SLEEP;
            return Recorder->TextFileStatus;
        }
    }

    Recorder->RingBufferVariableCount = elem_count + 2; /* set count of Variable inside the ring buffer */
                                             /* 2 more for sample und time */
                                             /* set time resolution iside the ring buffer */
    Recorder->TimeStep = (uint64_t)time_stamp * (uint64_t)Recorder->StartMessage.RecTimeSteps;
    Recorder->StartFlag = 1;
    if ((Recorder->Stamp.EntryList = (VARI_IN_PIPE*)my_calloc (sizeof (VARI_IN_PIPE), Recorder->RingBufferVariableCount)) == NULL) {
        ThrowError (1, "cannot start binary to stimuli converter");
        return NO_FREE_MEMORY;
    }
    /* Set all variables to invalid inside the stamp */
    for (x = 2; x < Recorder->RingBufferVariableCount; x++) {
        Recorder->Stamp.EntryList[x].vid = 0;
        Recorder->Stamp.EntryList[x].type = BB_BYTE;
    }
    /* Set variable for the first two columns of the recording file */
    Recorder->Stamp.EntryList[0].vid = 0x7FFFFFFF-1;   /* Step variable (count) */
    Recorder->Stamp.EntryList[0].type = BB_UDWORD;
    Recorder->Stamp.EntryList[1].vid = 0x7FFFFFFF-2;   /* Time variable (time) */
    Recorder->Stamp.EntryList[1].type = BB_DOUBLE;
    /* Alloc memory for the ring buffer */
    for (x = 0; x < LOCK_BACK_DEPTH; x++) {
        if ((Recorder->RingBuffer[x].EntryList = (VARI_IN_PIPE*)my_calloc (sizeof (VARI_IN_PIPE), Recorder->RingBufferVariableCount)) == NULL) {
            ThrowError (1, "cannot start binary to stimulus converter");
            return NO_FREE_MEMORY;
        }
    }
    Recorder->ConverterStatus = 1;   /* Convert is now running */
    return status;
}

int TerminateFileWriter (RECORDER_STRUCT *Recorder)
{
    int x;

    /* empty the ring buffer */
    if (Recorder->AllFileStatus == 0) {
        for (x = 0; x < LOCK_BACK_DEPTH; x++) {
            Recorder->RinBufferCurrentPosition = (++(Recorder->RinBufferCurrentPosition)) & RINGBUFF_CLIP_MASK;
            ++(Recorder->RinBufferCurrentPosition);
            Recorder->RinBufferCurrentPosition = Recorder->RinBufferCurrentPosition & RINGBUFF_CLIP_MASK;
            /* first entry inside the ring buffer are invalid */
            if (++Recorder->LineNumberCounter >= LOCK_BACK_DEPTH) {
                CopyRingPufferToStamp (Recorder);
                if (!Recorder->MdfFileStatus &&
                    ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF) == REC_FORMAT_FLAG_MDF)) {
                    if (WriteRingbuffMdf (Recorder->MdfFile, &Recorder->Stamp, Recorder->RingBufferVariableCount) == EOF) {
                        Recorder->AllFileStatus = Recorder->MdfFileStatus = WRITE_FILE_ERROR;
                        break;   /* for loop */
                    }
                }
                if (!Recorder->Mdf4FileStatus &&
                    ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF4) == REC_FORMAT_FLAG_MDF4)) {
                    if (WriteRingbuffMdf4 (Recorder->Mdf4File, &Recorder->Stamp, Recorder->RingBufferVariableCount) == EOF) {
                        Recorder->AllFileStatus = Recorder->Mdf4FileStatus = WRITE_FILE_ERROR;
                        break;   /* for loop */
                    }
                }

                if (!Recorder->TextFileStatus &&
                    ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_TEXT_STIMULI) == REC_FORMAT_FLAG_TEXT_STIMULI)) {
                    if (write_ringbuff_stimuli (Recorder->StimuliFile, &Recorder->Stamp, Recorder->RingBufferVariableCount) == EOF) {
                        Recorder->AllFileStatus = Recorder->TextFileStatus = WRITE_FILE_ERROR;
                        break;   /* for loop */
                    }
                }
            }
        }
    }
    if (!Recorder->MdfFileStatus &&
        ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF) == REC_FORMAT_FLAG_MDF)) {
        Recorder->MdfFileStatus = 1;
        TailMdfFile (Recorder->MdfFile, Recorder->LineNumberCounter - 3, Recorder->RecordStartTime);
    }
    if (!Recorder->Mdf4FileStatus &&
        ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF4) == REC_FORMAT_FLAG_MDF4)) {
        Recorder->Mdf4FileStatus = 1;
        TailMdf4File (Recorder->Mdf4File, Recorder->LineNumberCounter - 3, Recorder->RecordStartTime);
    }
    if (!Recorder->TextFileStatus &&
        ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_TEXT_STIMULI) == REC_FORMAT_FLAG_TEXT_STIMULI)) {
        Recorder->TextFileStatus = 1;
        TailStimuliFile (Recorder->StimuliFile);
    }
    if (Recorder->Stamp.EntryList != NULL) {
        my_free (Recorder->Stamp.EntryList);
        Recorder->Stamp.EntryList = NULL;
    }
    for (x = 0; x < LOCK_BACK_DEPTH; x++) {
        if (Recorder->RingBuffer[x].EntryList != NULL) {
            my_free (Recorder->RingBuffer[x].EntryList);
            Recorder->RingBuffer[x].EntryList = NULL;
        }
    }
    Recorder->ConverterStatus = 0;   /* Convert is off */
    return Recorder->AllFileStatus;
}


/* Convert a message queue entry to a recording file line */
int WriteWriter (RECORDER_STRUCT *Recorder, uint64_t time_stamp, int elem_count, VARI_IN_PIPE *pipevari_list)
{
    unsigned int x;
    int wr_ringp;                    /* Write pointer of the ring buffers */

    if (Recorder->StartFlag) {   /* first entry inside the ring buffer */
        Recorder->CurrentTimeStamp = time_stamp;
        Recorder->d_stimuli_timestamp_counter = (double)time_stamp;
        /* First message will set the start time of the recording */
        Recorder->RecordStartTime = time_stamp;
        /* init all timestamps inside the ring buffer */
        for (x = 0; x < LOCK_BACK_DEPTH; x++) {
            Recorder->RingBuffer[x].TimeStamp = Recorder->CurrentTimeStamp -
                 Recorder->TimeStep * x;  /* Step width */
        }
        Recorder->StartFlag = 0;
    }

    #ifdef CONVERT_TEST
    printf ("Message: ");
    for (x = 0; x < elem_count; x++) {
        printf ("VID = %i value = %f\t",
                pipevari_list[x].vid, pipevari_list[x].value.d);
    }
    printf ("\n");
    #endif /* CONVERT_TEST */

    if (TSCOUNT_SMALLER_TS (Recorder->CurrentTimeStamp + Recorder->TimeStep/2, time_stamp)) {
        /* If yes write from ring buffer entry to recording file */
        while (TSCOUNT_SMALLER_TS (Recorder->CurrentTimeStamp + Recorder->TimeStep/2, time_stamp)) {
            /* Increment ring buffer read and write pointer */
            ++(Recorder->RinBufferCurrentPosition);
            Recorder->RinBufferCurrentPosition = Recorder->RinBufferCurrentPosition & RINGBUFF_CLIP_MASK;
            Recorder->RingPufferPtr = Recorder->RingBuffer + Recorder->RinBufferCurrentPosition;
            /* First entrys inside the ring buffer are invalid */
            if (++(Recorder->LineNumberCounter) >= LOCK_BACK_DEPTH) {
                CopyRingPufferToStamp (Recorder);
                if (!Recorder->MdfFileStatus) {
                    if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF) == REC_FORMAT_FLAG_MDF) {
                        if (WriteRingbuffMdf (Recorder->MdfFile, &Recorder->Stamp, Recorder->RingBufferVariableCount) == EOF) {
                            Recorder->AllFileStatus = Recorder->MdfFileStatus = WRITE_FILE_ERROR;
                            break;  /* while loop */
                        }
                    }
                }
                if (!Recorder->Mdf4FileStatus) {
                    if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_MDF4) == REC_FORMAT_FLAG_MDF4) {
                        if (WriteRingbuffMdf4 (Recorder->Mdf4File, &Recorder->Stamp, Recorder->RingBufferVariableCount) == EOF) {
                            Recorder->AllFileStatus = Recorder->Mdf4FileStatus = WRITE_FILE_ERROR;
                            break;  /* while loop */
                        }
                    }
                }

                if (!Recorder->TextFileStatus) {
                    if ((Recorder->StartMessage.FormatFlags & REC_FORMAT_FLAG_TEXT_STIMULI) == REC_FORMAT_FLAG_TEXT_STIMULI) {

                        if (write_ringbuff_stimuli (Recorder->StimuliFile, &Recorder->Stamp, Recorder->RingBufferVariableCount) == EOF) {
                            Recorder->AllFileStatus = Recorder->TextFileStatus = WRITE_FILE_ERROR;
                            break;  /* while loop */
                        }
                    }
                }
            }
            Recorder->CurrentTimeStamp += Recorder->TimeStep;
            Recorder->d_stimuli_timestamp_counter += (double)Recorder->TimeStep;
            Recorder->RingBuffer[Recorder->RinBufferCurrentPosition].TimeStamp = Recorder->CurrentTimeStamp;
        }
        /* Write variable liste into this ring buffer element */
        put_varilist_ringbuff (&(Recorder->RingBuffer[Recorder->RinBufferCurrentPosition]), pipevari_list,
                               Recorder->Vids, elem_count);
    } else {
       /* Otherwihe try to write an existing ring buffer elemente */
        wr_ringp = Recorder->RinBufferCurrentPosition;
        do {
            if (TSCOUNT_LARGER_TS (time_stamp + Recorder->TimeStep/2, Recorder->RingBuffer[wr_ringp].TimeStamp)) {
            /* Write variable list in this ring buffer element */
                 put_varilist_ringbuff (&(Recorder->RingBuffer[wr_ringp]), pipevari_list,
                                        Recorder->Vids, elem_count);
                 break;  /* break do-while loop */
            }
            wr_ringp++;
            wr_ringp = wr_ringp & RINGBUFF_CLIP_MASK;
        } while (wr_ringp == Recorder->RinBufferCurrentPosition);
    }
    return Recorder->AllFileStatus;
}

TASK_CONTROL_BLOCK hdrec_tcb =
        INIT_TASK_COTROL_BLOCK(TRACE_RECORDER, INTERN_ASYNC, 211, cyclic_hdrec, init_hdrec, terminate_hdrec, 262144);

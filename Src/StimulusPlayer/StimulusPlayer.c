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


#define HDPLAY_C
#include <stdio.h>
#include <string.h>
#include "Config.h"
#include "ConfigurablePrefix.h"
#include "StimulusReadFile.h"
#include "StimulusPlayer.h"
#include "WriteToBlackboardPipe.h"
#include "ReadFromBlackboardPipe.h"
#include "Scheduler.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "EnvironmentVariables.h"
#include "Message.h"
#include "Fifos.h"
#include "Files.h"
#include "ReadConfig.h"
#include "BlackboardAccess.h"

#define PING_LIMIT  100000

typedef struct {
    int *vids;
    char play_file_name[MAX_PATH];
    STIMULI_FILE *file;

    uint64_t sample_number;
    int varicount;
    int wrpipe_pid;
    VARI_IN_PIPE *pipevari_list;
    uint64_t current_sample_time;
    int file_status;
    int pipe_status;
    int use_a_config_file;
    int connect_pid;
    char *vari_list;

    int ControlReqFiFo;
    int ControlAckFiFo;
    int DataFiFo;

    int my_pid;
    int player_status;
    int should_send_ping_message;

    int vid_hdplay_state;
} STIMULUS_PLAYER_DATA;


static STIMULUS_PLAYER_DATA StimulusData;

static int WriteToFiFoOrMessageQueue(STIMULUS_PLAYER_DATA *StimulusData, uint32_t MessageId, int Size, void *Data)
{
    int TxPid = GET_PID();
    if ((MessageId == SEND_VIDS_MESSAGE) || (StimulusData->ControlReqFiFo <= 0)) {
        StimulusData->ControlReqFiFo = TxAttachFiFo(TxPid, WRITE_PIPE_REQ_FIFO_NAME);
        StimulusData->DataFiFo = TxAttachFiFo(TxPid, WRITE_PIPE_DATA_FIFO_NAME);
    }
    if (StimulusData->ControlReqFiFo > 0) {
        return WriteToFiFo(StimulusData->ControlReqFiFo, MessageId, TxPid, get_timestamp_counter(), Size, Data);
    } else {
        return -1;
    }
}


static int WriteOnSamplesToFiFo(STIMULUS_PLAYER_DATA *StimulusData)
{
    int Mid;

    if (StimulusData->file_status != STIMULI_END_OF_FILE) {  // No file end
        uint32_t Mask;
        int SamplesFitIntoQueue = 262144 / ((StimulusData->varicount * sizeof(VARI_IN_PIPE) + sizeof(FIFO_ENTRY_HEADER)));
        if (SamplesFitIntoQueue > 0) {
#ifdef _MSC_VER
            unsigned long Index;
            _BitScanReverse(&Index, SamplesFitIntoQueue);
            Mask = ~(UINT32_MAX << Index);
#else
            uint32_t Index;
            Index =  32 - __builtin_clz(SamplesFitIntoQueue);
            Mask = ~(UINT32_MAX << Index);
#endif
        } else {
            Mask = 0;
        }
        if (((StimulusData->sample_number & Mask) == Mask) ||
            StimulusData->should_send_ping_message) {
            Mid = PLAY_DATA_MESSAGE_PING;
            StimulusData->should_send_ping_message = 1;
        } else {
            Mid = PLAY_DATA_MESSAGE;
        }
        StimulusData->pipe_status = WriteToFiFo (StimulusData->DataFiFo,
                                                 Mid,
                                                 StimulusData->my_pid,
                                                 StimulusData->current_sample_time, (int)sizeof (VARI_IN_PIPE) * StimulusData->varicount,
                                                 (char*)(StimulusData->pipevari_list));
        if (StimulusData->pipe_status == 0) {
            StimulusData->should_send_ping_message = 0;
        }
        return 0;
    } else {

        StimulusData->pipe_status = WriteToFiFo (StimulusData->DataFiFo,
                                                 PLAY_NO_MORE_DATA_MESSAGE,
                                                 GET_PID(),
                                                 StimulusData->current_sample_time, 0,
                                                 NULL);
        return 1;
    }
}

static void ReadNSamplesFromFileAndWriteItToFiFo(STIMULUS_PLAYER_DATA *StimulusData, int par_MaxSampleCount)
{
    int x = 0;

    if (StimulusData->file == NULL) return;

    if (StimulusData->pipe_status) {  // There was before a pipe overflow
        WriteOnSamplesToFiFo(StimulusData);
        x = 1;
    }

    for (; (x < par_MaxSampleCount) && (StimulusData->pipe_status == 0); x++) {  /* Write max. N data messages  */
        StimulusData->file_status = ReadOneStimuliTimeSlot (StimulusData->file, StimulusData->pipevari_list, &StimulusData->current_sample_time);
        if (WriteOnSamplesToFiFo(StimulusData)) {
            /* File end */
            break;
        }
        StimulusData->sample_number++;
    }
}

void cyclic_hdplay (void)
{
    FIFO_ENTRY_HEADER Header;
    MESSAGE_HEAD mhead;
    int x;
    TRIGGER_INFO_MESS trigger_info;
    int pos;
    int vlpc;
    int len;
    FILE *fh;

    if (test_message (&mhead)) { /* Check if a message are received */
        switch (mhead.mid) {      /* If jes which type */
            case HDPLAY_CONFIG_FILENAME_MESSAGE:
                StimulusData.connect_pid = mhead.pid;
                {
                    char filename[MAX_PATH];
                    int token_count = 0;
                    char word[BBVARI_NAME_SIZE];
                    int token;
                    int config_file_line_counter;

                    StimulusData.vari_list = NULL;
                    if (StimulusData.player_status != HDPLAY_SLEEP) {
                        ThrowError (1, "HD-Player not in sleep mode");
                        remove_message ();
                        break;
                    }
                    read_message (&mhead, filename, MAX_PATH);
                    if ((fh = open_file (filename, "rt")) == NULL) {
                        ThrowError (1, "cannot open file \"%s\"", filename);
                        break;
                    }
                    trigger_info.trigger_vid = -1;
                    config_file_line_counter = 1;
                    while ((token = read_token (fh, word, sizeof(word)-1, &config_file_line_counter)) != EOF) {
                        token_count++;
                        if (token_count == 1) {
                            if (token != HDPLAYER_CONFIG_FILE_TOKEN) {
                                ThrowError (1, "inside file %s(%i):\n"
                                          "first keyword must be 'HDPLAYER_CONFIG_FILE'",
                                       filename, config_file_line_counter);
                                goto ERROR_IN_CONFIGFILE;
                            } else continue;
                        } else {
                            switch (token) {
                            case HDPLAYER_FILE_TOKEN:
                                if (read_word_with_blacks (fh, word, sizeof(word)-1, &config_file_line_counter) != 0) {
                                    ThrowError (1, "inside file %s(%i):\n"
                                              "   expecting a = in HDPLAYER_FILE statement",
                                           filename, config_file_line_counter);
                                    goto ERROR_IN_CONFIGFILE;
                                } else {
                                    SearchAndReplaceEnvironmentStrings(word, StimulusData.play_file_name, sizeof(StimulusData.play_file_name));
                                }
                                break;
                            case TRIGGER_TOKEN:
                                if (read_token (fh, word, sizeof (word)-1, &config_file_line_counter) != 0) {
                                    ThrowError (1, "inside file %s(%i):\n"
                                              "   expecting a name in TRIGGER statement",
                                           filename, config_file_line_counter);
                                    goto ERROR_IN_CONFIGFILE;
                                }
                                trigger_info.trigger_vid = add_bbvari (word, BB_UNKNOWN, NULL);

                                read_token (fh, word, sizeof(word)-1, &config_file_line_counter);
                                if (strcmp (word, ">")) {
                                    ThrowError (1, "inside file %s(%i):\n"
                                              "   expecting a > in TRIGGER statement",
                                           filename, config_file_line_counter);
                                    goto ERROR_IN_CONFIGFILE;
                                }
                                if (read_token (fh, word, sizeof(word)-1, &config_file_line_counter) != 0) {
                                    ThrowError (1, "inside file %s(%i):\r\n"
                                              "   expecting a number in TRIGGER statement",
                                           filename, config_file_line_counter);
                                    goto ERROR_IN_CONFIGFILE;
                                }
                                SSCANF_ERR_NRV (word, "%lf", &(trigger_info.trigger_value));
                                break;
                            case STARTVARLIST_TOKEN:
                                vlpc = 1024;  // Start with 1024 bytes
                                pos = 0;
                                if ((StimulusData.vari_list = (char*)my_malloc (vlpc)) == NULL) {
                                    ThrowError (1, "out of memory");
                                    goto ERROR_IN_CONFIGFILE;
                                }
                                while ((token = read_token (fh, word, sizeof(word)-1, &config_file_line_counter)) != ENDVARLIST_TOKEN) {
                                    switch (token) {
                                    case EOF:
                                        ThrowError (1, "inside file %s(%i):\n"
                                               "   missing ENDVARLIST token",
                                               filename, config_file_line_counter);
                                        goto ERROR_IN_CONFIGFILE;
                                    case VARIABLE_TOKEN:
                                        if (read_token (fh, word, BBVARI_NAME_SIZE, &config_file_line_counter) != 0) {
                                            ThrowError (1, "inside file %s(%i):\n"
                                                   "   expecting a name in VARIABLE statement",
                                                   filename, config_file_line_counter);
                                            goto ERROR_IN_CONFIGFILE;
                                        }
                                        len = (int)strlen (word) + 1;
                                        if ((pos + len + 1) > vlpc) {
                                            vlpc += vlpc / 4 + 1024;
                                            if ((StimulusData.vari_list = (char*)my_realloc (StimulusData.vari_list, vlpc)) == NULL) {
                                                ThrowError (1, "out of memory");
                                                goto ERROR_IN_CONFIGFILE;
                                            }
                                        }
                                        MEMCPY (StimulusData.vari_list + pos, word, len);
                                        pos += len;
                                        break;
                                    default:
                                        ThrowError (1, "inside file %s(%i):\n"
                                               "   not expected token %s",
                                               filename, config_file_line_counter, word);
                                        goto ERROR_IN_CONFIGFILE;
                                    }
                                }
                                StimulusData.vari_list[pos] = 0;  // terminate list
                                break;
                            default:
                                if (token_count != 1) {
                                    ThrowError (1, "inside file %s(%i):\n"
                                              "   not expected token %s",
                                           filename, config_file_line_counter, word);
                                    goto ERROR_IN_CONFIGFILE;
                                }
                            }
                        }
                    }
                    close_file (fh);
                }
                StimulusData.use_a_config_file = 1;
                goto HAVE_CONFIGFILE;
              ERROR_IN_CONFIGFILE:
                if (StimulusData.vari_list != NULL) my_free (StimulusData.vari_list);
                StimulusData.vari_list = NULL;
                close_file (fh);
                break;
            case HDPLAY_FILENAME_MESSAGE:   /* Message contains file name */
                StimulusData.use_a_config_file = 0;
                if (StimulusData.player_status != HDPLAY_SLEEP) {
                    ThrowError (1, "Stimulus player not in sleep mode");
                    remove_message ();
                    break;
                }
                trigger_info.trigger_vid = -1;
                read_message (&mhead, StimulusData.play_file_name, MAX_PATH);

              HAVE_CONFIGFILE:
                /* Open the file */
                /* and read the file header */
                if ((StimulusData.file = OpenAndReadStimuliHeader (StimulusData.play_file_name, StimulusData.vari_list)) == NULL) {
                    break;
                }
                if (StimulusData.vari_list != NULL) my_free (StimulusData.vari_list);
                StimulusData.vari_list = NULL;

                StimulusData.vids = GetStimuliVariableIds(StimulusData.file);
                StimulusData.varicount = GetNumberOfStimuliVariables(StimulusData.file);

                StimulusData.pipevari_list = (VARI_IN_PIPE*)my_malloc(sizeof(VARI_IN_PIPE) * StimulusData.varicount);
                if (StimulusData.pipevari_list == NULL) {
                    ThrowError (1, "out of memory", PN_STIMULI_QUEUE);
                    CloseStimuliFile (StimulusData.file);
                    StimulusData.file = NULL;
                    break;
                }
                if ((StimulusData.wrpipe_pid = get_pid_by_name (PN_STIMULI_QUEUE)) <= 0) {
                    ThrowError (1, "\"%s\" process not running", PN_STIMULI_QUEUE);
                    CloseStimuliFile (StimulusData.file);
                    StimulusData.file = NULL;
                    if (StimulusData.pipevari_list == NULL) my_free (StimulusData.pipevari_list);
                    StimulusData.pipevari_list = NULL;
                    break;
                }
                /* transmit the VID's to the WRPIPE process */
                WriteToFiFoOrMessageQueue (&StimulusData, SEND_VIDS_MESSAGE,
                               (int)sizeof (int) * StimulusData.varicount, (char*)(StimulusData.vids));
                /* transmit trigger inforation to the WRPIPE prozess if exists */
                WriteToFiFoOrMessageQueue (&StimulusData, TRIGGER_INFO_MESSAGE,
                               sizeof (TRIGGER_INFO_MESS), (char*)&trigger_info);

                /* Fillup the message queue inside the next cycle */
                StimulusData.pipe_status = 0;
                StimulusData.sample_number = 0;
                StimulusData.current_sample_time = 0;
                StimulusData.should_send_ping_message = 0;

                StimulusData.player_status = HDPLAY_FILE_NAME;

                /* Write some data messages immediately */
                ReadNSamplesFromFileAndWriteItToFiFo(&StimulusData, PING_LIMIT);
                break;
            case HDPLAY_STOP_MESSAGE:  /* Terminate HD-Player without waiting
                                          for file end */
                /* Stop message mhave to be written before all other messages
                   into the message queue */
                WriteToFiFoOrMessageQueue (&StimulusData, STOP_WRPIPE_MESSAGE, 0, NULL);
                remove_message ();
                break;
            default:
                remove_message ();
                ThrowError (1, "unknown message %i: %i\n", mhead.pid, mhead.mid);
        }
    }
    if (CheckFiFo(StimulusData.ControlAckFiFo, &Header) > 0) {
        switch (Header.MessageId) {
        case PLAY_DATA_MESSAGE_PING:
            do {
                RemoveOneMessageFromFiFo (StimulusData.ControlAckFiFo);
            } while((CheckFiFo(StimulusData.ControlAckFiFo, &Header) > 0) &&
                     (Header.MessageId == PLAY_DATA_MESSAGE_PING));
            ReadNSamplesFromFileAndWriteItToFiFo(&StimulusData, PING_LIMIT);
            break;
        case HDPLAY_ACK_MESSAGE:
            WriteToFiFoOrMessageQueue (&StimulusData, START_WRPIPE_MESSAGE, 0, NULL);
            StimulusData.player_status = HDPLAY_PLAY;
            if (StimulusData.use_a_config_file) {
                write_bbvari_ubyte (StimulusData.vid_hdplay_state, 1);
            } else {
                write_bbvari_ubyte (StimulusData.vid_hdplay_state, 2);
            }
            StimulusData.use_a_config_file = 0;
            RemoveOneMessageFromFiFo (StimulusData.ControlAckFiFo);
            break;
        case HDPLAY_TRIGGER_MESSAGE: /* WRPIPE pProcess report that the trigger condition matched */
            write_bbvari_ubyte (StimulusData.vid_hdplay_state, 2);
            RemoveOneMessageFromFiFo (StimulusData.ControlAckFiFo);
            break;
        case WRPIPE_ACK:
            write_message (StimulusData.connect_pid, HDPLAY_ACK, 0, NULL);
            RemoveOneMessageFromFiFo (StimulusData.ControlAckFiFo);
            break;
        case HDPLAY_STOP_MESSAGE:  /* Terminate HD-Player without waiting
                                      for file end */
            RemoveOneMessageFromFiFo (StimulusData.ControlAckFiFo);
            for (x = 0; StimulusData.vids[x] != 0; x++) {
                if (StimulusData.vids[x] > 0) remove_bbvari (StimulusData.vids[x]);
            }
            if (StimulusData.file != NULL) {
                CloseStimuliFile (StimulusData.file);
                StimulusData.file = NULL;
            }
            if (StimulusData.pipevari_list == NULL) {
                my_free (StimulusData.pipevari_list);
                StimulusData.pipevari_list = NULL;
            }
            write_bbvari_ubyte (StimulusData.vid_hdplay_state, 0);
            StimulusData.player_status = HDPLAY_SLEEP;
            break;
        default:
            ThrowError (1, "unknown message %i send by %i",
                   Header.MessageId, Header.TramsmiterPid);
            RemoveOneMessageFromFiFo (StimulusData.ControlReqFiFo);
        }
    }
}

int init_hdplay (void)
{
    StimulusData.ControlAckFiFo = CreateNewRxFifo (GET_PID(), 1024, WRITE_PIPE_ACK_FIFO_NAME);

    StimulusData.my_pid = GET_PID();

    StimulusData.vid_hdplay_state = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_STIMULUS_PLAYER), BB_UBYTE, NULL);
    if (StimulusData.vid_hdplay_state <= 0) {
        ThrowError (1, "cannot attach variable 'HD_Play'");
        return -1;
    }
    set_bbvari_conversion (StimulusData.vid_hdplay_state, 2, "0 0 \"Sleep\"; 1 1 \"Trigger\"; 2 2 \"Play\";");
    write_bbvari_ubyte (StimulusData.vid_hdplay_state, 0);

    StimulusData.player_status = HDPLAY_SLEEP;
    return 0;
}

void terminate_hdplay (void)
{
    DeleteFiFo (StimulusData.ControlAckFiFo, GET_PID());
    remove_bbvari (StimulusData.vid_hdplay_state);
}

TASK_CONTROL_BLOCK hdplay_tcb
        = INIT_TASK_COTROL_BLOCK(STIMULI_PLAYER, INTERN_ASYNC, 20, cyclic_hdplay, init_hdplay, terminate_hdplay, 32*1024);

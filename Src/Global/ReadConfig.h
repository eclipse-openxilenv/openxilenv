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


#ifndef READCONF_H
#define READCONF_H

//#include "canser.h"

#ifdef READCONF_C
#define extern
#endif

/* ------------------------------------------------------------ *\
 *    Global defines  					        *
\* ------------------------------------------------------------ */

#define SSCANF_ERR(word, format, conv) \
    if (!sscanf (word, format, conv)) { \
        ThrowError (1, "in Datei %s(%i):\r\n" \
                    "    kann String %s nicht mit Format %s Konvertieren", \
                    filename, config_file_line_counter, word, format); \
        return -1;  \
    }
#define SSCANF_ERR_NRV(word, format, conv) \
    if (!sscanf (word, format, conv)) { \
        ThrowError (1, "in Datei %s(%i):\r\n" \
                    "    kann String %s nicht mit Format %s Konvertieren", \
                    filename, config_file_line_counter, word, format); \
        return;  \
    }

#define EQUAL_TOKEN                 265
#define VARCOUNT_TOKEN              267
#define STARTVARLIST_TOKEN          268
#define VARIABLE_TOKEN              269
#define ENDVARLIST_TOKEN            270

#define DESCRIPTION_FILE_TOKEN            282
#define AUTOGEN_DESCRIPTION_FILE_TOKEN    283
#define MDF_FORMAT_TOKEN                  285
#define TEXT_FORMAT_TOKEN                 286
#define HDRECORDER_PHYSICAL_TOKEN         287
#define MDF4_FORMAT_TOKEN                 288

#define HDRECORDER_CONFIG_FILE_TOKEN  300
#define HDRECORDER_SAMPLERATE_TOKEN   301
#define HDRECORDER_SAMPLELENGTH_TOKEN 302
#define HDRECORDER_FILE_TOKEN         303
#define TRIGGER_TOKEN                 304
#define HDPLAYER_FILE_TOKEN           305
#define HDPLAYER_CONFIG_FILE_TOKEN    306
#define MCSREC_FORMAT_TOKEN           307
#define ALL_LABEL_OF_PROCESS_TOKEN    308

typedef struct {
	char *s;
	int t;
} TOKEN_LIST;

extern TOKEN_LIST tokens[]
#ifdef extern
            = {{"HDRECORDER_CONFIG_FILE",  HDRECORDER_CONFIG_FILE_TOKEN},
			   {"HDRECORDER_SAMPLERATE",   HDRECORDER_SAMPLERATE_TOKEN},
			   {"HDRECORDER_SAMPLELENGTH", HDRECORDER_SAMPLELENGTH_TOKEN},
			   {"HDRECORDER_FILE",         HDRECORDER_FILE_TOKEN},
			   {"HDPLAYER_CONFIG_FILE",    HDPLAYER_CONFIG_FILE_TOKEN},
			   {"TRIGGER",                 TRIGGER_TOKEN},
			   {"HDPLAYER_FILE",           HDPLAYER_FILE_TOKEN},
			   {"MDF_FORMAT",              MDF_FORMAT_TOKEN},
               {"MDF3_FORMAT",             MDF_FORMAT_TOKEN},
               {"MDF4_FORMAT",             MDF4_FORMAT_TOKEN},
               {"TEXT_FORMAT",             TEXT_FORMAT_TOKEN},
               {"ALL_LABEL_OF_PROCESS",    ALL_LABEL_OF_PROCESS_TOKEN},
               {"VARCOUNT",                VARCOUNT_TOKEN},
               {"STARTVARLIST",            STARTVARLIST_TOKEN},
               {"VARIABLE",                VARIABLE_TOKEN},
               {"ENDVARLIST",              ENDVARLIST_TOKEN},
			   {"DESCRIPTION_FILE",        DESCRIPTION_FILE_TOKEN},
			   {"AUTOGEN_DESCRIPTION_FILE", AUTOGEN_DESCRIPTION_FILE_TOKEN},
               {"PHYSICAL",                HDRECORDER_PHYSICAL_TOKEN},
               {"=",                       EQUAL_TOKEN},
               { NULL,                     0}}
#endif
;

extern int read_token (FILE *file, char *word, int maxlen, int *line_counter);

extern int read_word (FILE *file, char *word, int maxlen, int *line_counter);

extern int read_word_with_blacks (FILE *file, char *word, int maxlen, int *line_counter);

extern void remove_comment (FILE *file, int *line_counter);

#undef extern
#endif /* READCONF_H */

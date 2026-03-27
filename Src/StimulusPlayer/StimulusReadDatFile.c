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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Config.h"
#include "Platform.h"
#include "StringMaxChar.h"
#include "Files.h"
#include "ReadFromBlackboardPipe.h"
#include "TraceRecorder.h"
#include "StimulusPlayer.h"
#include "Blackboard.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "StimulusReadFile.h"
#include "StimulusReadDatFile.h"


/* This will read all variable names from a stimulus files  (with ; separation) */
char *DatReadStimulHeaderVariabeles (const char *par_Filename)
{
    int one_char = 0;
    char word[BBVARI_NAME_SIZE];
    int varicount;
    int status;
    FILE *file;
    char *Ret = NULL;
    int LenOfRet = 0;
    int Pos;

    if ((file = open_file (par_Filename, "rt")) == NULL) {
        ThrowError (1, "cannot open file %s", par_Filename);
        return NULL;
    }
    while ((one_char != '\n') && (one_char != EOF)) { /* remove comment line */
        one_char = fgetc (file);
    }
    if ((read_next_word (file, word, sizeof(word)-1) != END_OF_LINE) || (strcmp ("FORMAT", word))) {
        ThrowError (1, "missing token 'FORMAT' in file head");
        close_file (file);
        return NULL;
    }
    do {  /* Graphic name */
        one_char = fgetc (file);
    } while (one_char != '\n' && one_char != EOF);
    if (read_next_word (file, word, sizeof(word)-1) != END_OF_LINE) { /* Sample rate */
        ThrowError (1, "missing sample rate in file head");
        close_file (file);
        return NULL;
    }

    varicount = 0;
    do {          /* Counting the variable and read the variable names */
        status = read_next_word (file, word, sizeof(word)-1);
        Pos = LenOfRet;
        if (varicount) {
            LenOfRet += 1;   // ';'
        }
        LenOfRet += (int)strlen (word);
        Ret = my_realloc (Ret, LenOfRet + 1);
        if (Ret == NULL) {
            ThrowError (1, "Out of memory");
            return NULL;
        }
        if (varicount) {
            StringCopyMaxCharTruncate (Ret + Pos, ";", (LenOfRet + 1) - Pos);
            Pos += 1;
        }
        StringCopyMaxCharTruncate (Ret + Pos, word, (LenOfRet + 1) - Pos);
        Pos += (int)strlen (word);
        if (varicount > HDPLAY_MAX_VARIS) {
            ThrowError (1, "max. %i variable allowed", (int)HDPLAY_MAX_VARIS);
            close_file (file);
            return Ret;
        }
        varicount++;
    } while (status != END_OF_LINE);
    close_file (file);
    return Ret;    /* all OK */
}



/* Returns a pointer to the  Stimulus file */
/* or NULL on error */
STIMULI_FILE* DatOpenAndReadStimuliHeader (const char *par_Filename, const char *vari_list)
{
    int c, one_char = 0;
    char word[BBVARI_NAME_SIZE];
    int varicount;
    int columncount;
    int unioncount = 0;
    int status, x;
    const char *vlp;
    STIMULI_FILE *Ret = NULL;
    DAT_STIMULI_FILE *Dat = NULL;

    Ret = (STIMULI_FILE*)my_calloc(1, sizeof(STIMULI_FILE));
    if (Ret == NULL) {
        ThrowError(1, "out of memmory");
        goto __ERROR;
    }
    Dat = (DAT_STIMULI_FILE*)my_calloc(1, sizeof(DAT_STIMULI_FILE));
    if (Dat == NULL) {
        ThrowError(1, "out of memmory");
        goto __ERROR;
    }
    Ret->File = (void*)Dat;
    Ret->FileType = DAT_FILE;

    if ((Dat->fh = open_file (par_Filename, "rt")) == NULL) {
        ThrowError (1, "cannot open file \"%s\"", par_Filename);
        goto __ERROR;
    }

    while ((one_char != '\n') && (one_char != EOF)) { /* remove comment line */
        one_char = fgetc (Dat->fh);
    }
    Dat->line_counter++;

    if ((read_next_word (Dat->fh, word, sizeof(word)-1) != END_OF_LINE) || (strcmp ("FORMAT", word))) {
        ThrowError (1, "missing keyword 'FORMAT' inside \"%s\" file header", par_Filename);
        goto __ERROR;
    }
    Dat->line_counter++;
    do {  /* Graphic name */
        one_char = fgetc (Dat->fh);
    } while (one_char != '\n' && one_char != EOF);
    Dat->line_counter++;
    if (read_next_word (Dat->fh, word, sizeof(word)-1) != END_OF_LINE) { /* Sample rate */
        ThrowError (1, "missing sample rate inside \"%s\" file header", par_Filename);
        goto __ERROR;
    }
    Dat->line_counter++;
    Dat->sample_rate = atof (word);
    Dat->sample_rate_uint64 = (uint64_t)(Dat->sample_rate * TIMERCLKFRQ);

    varicount = 0;
    columncount = 0;

    Dat->size_of_vids = 100;
    Dat->vids = (int*)my_realloc(Dat->vids, Dat->size_of_vids * sizeof(int));
    Dat->dtypes = (int*)my_realloc(Dat->dtypes, Dat->size_of_vids * sizeof(int));
    Dat->columns = (int*)my_realloc(Dat->columns, Dat->size_of_vids * sizeof(int));
    if ((Dat->vids == NULL) || (Dat->columns == NULL)) {
        goto __ERROR;
    }
    do {          /* Counting the variable and read the variable names */
        status = read_next_word (Dat->fh, word, sizeof(word)-1);
        if (varicount > HDPLAY_MAX_VARIS) {
            ThrowError (1, "max. %i variables allowed", (int)HDPLAY_MAX_VARIS);
            goto __ERROR;
        }
        if ((varicount + 1) >= Dat->size_of_vids) { // +1 because the List will be terminate with a 0
            Dat->size_of_vids += 100;
            Dat->vids = (int*)my_realloc(Dat->vids, Dat->size_of_vids * sizeof(int));
            Dat->dtypes = (int*)my_realloc(Dat->dtypes, Dat->size_of_vids * sizeof(int));
            Dat->columns = (int*)my_realloc(Dat->columns, Dat->size_of_vids * sizeof(int));
            if ((Dat->vids == NULL) || (Dat->columns == NULL)) {
                goto __ERROR;
            }
        }
        // Look if variable should be used
        if (vari_list != NULL) {
            Dat->vids[varicount] = -1;
            vlp = vari_list;
            while (*vlp != '\0') {
                if (!strcmp (vlp, word)) {
                    Dat->vids[varicount] = add_bbvari (word, ((varicount == 0)&&!strcmp(word, "sample")) ? BB_UDWORD :
                                                       ((varicount == 1)&&!strcmp(word,"time")) ? BB_DOUBLE : BB_UNKNOWN_DOUBLE, NULL);
                    Dat->dtypes[varicount] = get_bbvaritype (Dat->vids[varicount]);
                    Dat->columns[varicount] = columncount;
                    varicount++;
                    break;
                }
                while (*vlp != '\0') vlp++;
                vlp++;
            }
        } else {
            // All variables
            Dat->vids[varicount] = add_bbvari (word, ((varicount == 0)&&!strcmp(word,"sample")) ? BB_UDWORD :
                                               ((varicount == 1)&&!strcmp(word,"time")) ? BB_DOUBLE : BB_UNKNOWN_DOUBLE, NULL);
            Dat->dtypes[varicount] = get_bbvaritype (Dat->vids[varicount]);
            Dat->columns[varicount] = columncount;
            varicount++;
        }
        columncount++;
    } while (status != END_OF_LINE);
    Dat->line_counter++;
    Dat->vids[varicount] = 0;


    // Ignore whitespaces
    do {
        c = fgetc (Dat->fh);
    } while (isascii (c) && isspace (c) && (c != '\n'));

     /* Read units of the variables */
    do {
        int BracketDepth;
        int ErrFlag;

        if (c != '[') {
            ThrowError (1, "there is missing an '[' bracket in unit definition");
            goto __ERROR;
        }
        // Ignore whitespaces
        do {
            c = fgetc (Dat->fh);
        } while (isascii (c) && isspace (c) && (c != '\n') && (c != EOF));

        ErrFlag = 0;
        BracketDepth = 0;
        x = 0;
        for (;;) {
            if (c == '\n') {
                ThrowError (1, "unexpected new line in unit definition");
                break;
            } else if (c == EOF) {
                ThrowError (1, "unexpected end of file in unit definition");
                break;
            } else if ((c == ']') && (BracketDepth == 0)) {
                while ((x > 0) && (isascii (word[x-1]) && isspace (word[x-1]))) x--;
                word[x] = 0;
                if (unioncount >= columncount) {
                    ThrowError (1, "more units (%i) than variables (%i) defined", unioncount+1, columncount);
                } else {
                    if (Dat->vids[unioncount]) set_bbvari_unit (Dat->vids[unioncount], word);
                    unioncount++;
                }
                break;
            } else if (x >= (BBVARI_UNIT_SIZE-1)) {
                if (!ErrFlag) ThrowError (1, "the %i. unit exceeds %i chars", unioncount+1, BBVARI_UNIT_SIZE-1);
                ErrFlag = 1;
            } else {
                if (c == '[') BracketDepth++;
                else if (c == ']') BracketDepth--;
                word[x++] = (char)c;
            }
            c = fgetc (Dat->fh);
        }
        // Ignore whitespaces
        do {
            c = fgetc (Dat->fh);
        } while (isascii (c) && isspace (c) && (c != '\n') && (c != EOF));
    } while ((c != '\n') && (c != EOF));
    Dat->line_counter++;

    if ((read_next_word (Dat->fh, word, sizeof(word)-1) != END_OF_LINE) || (strcmp ("DATA", word))) {
        ThrowError (1, "missing keyword \"DATA\" inside stimuli file header");
        goto __ERROR;
    }
    Dat->line_counter++;
    Dat->column_count = columncount;
    Dat->variable_count = varicount;

    return Ret;     /* all OK */
__ERROR:
    if (Ret != NULL) my_free(Ret);
    if (Dat != NULL) {
        if (Dat->fh != NULL) close_file(Dat->fh);
        if (Dat->vids != NULL) my_free(Dat->vids);
        if (Dat->dtypes != NULL) my_free(Dat->dtypes);
        if (Dat->columns != NULL) my_free(Dat->columns);
        my_free(Dat);
    }
    return NULL;
}

/* This will read one line from the simulus file and build a message */
int DatReadOneStimuliTimeSlot (STIMULI_FILE* par_File, VARI_IN_PIPE *pipevari_list, uint64_t *ret_t)
{
    double value;
    int64_t value_i64 = 0;
    uint64_t value_ui64 = 0;
    int type;
    char *p;
    char word[BBVARI_NAME_SIZE];
    int column_counter;
    int variable_counter;
    int status;
    DAT_STIMULI_FILE *Dat = (DAT_STIMULI_FILE*)par_File->File;

    *ret_t = Dat->current_sample_time;
    Dat->current_sample_time += Dat->sample_rate_uint64;

    column_counter = 0;      /* Column counter */
    variable_counter = 0;    /* Variable counter */
    do {
        if ((status = read_next_word (Dat->fh, word, sizeof(word)-1)) == END_OF_FILE) {
            break;
        }
        if (column_counter > Dat->column_count) {
            ThrowError (1, "too many columns in line %i (expect %i not %i)", Dat->line_counter, Dat->column_count, column_counter+1);
            return STIMULI_END_OF_FILE;
        }
        if ((status == END_OF_LINE) && (column_counter != (Dat->column_count-1))) {
            ThrowError (1, "too less columns in line %i (expect %i not %i)", Dat->line_counter, Dat->column_count, column_counter+1);
            return STIMULI_END_OF_FILE;
        }
        if (status == END_OF_LINE) {
            Dat->line_counter++;
        }
        if (Dat->columns[variable_counter] == column_counter) {
            p = word;
            while (isascii(*p) && isspace(*p)) p++;
            if (*p == '-') {
                value_i64 = strtoll(p, &p, 0);
                type = 0;
            } else {
                value_ui64 = strtoull(p, &p, 0);
                type = 1;
            }
            if ((*p == '.') || (*p == 'e') || (*p == 'E')) {
                value = atof (word);   /* convert to double */
                type = 2;
            }

            value = atof (word);   /* convert to double */
            switch (Dat->dtypes[variable_counter]) { /* convert to the appropriate data types */
            case BB_BYTE:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.b = (int8_t)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.b = (int8_t)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.b = (int8_t)value;
                    break;
                }
                break;
            case BB_UBYTE:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.ub = (uint8_t)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.ub = (uint8_t)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.ub = (uint8_t)value;
                    break;
                }
                break;
            case BB_WORD:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.w = (int16_t)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.w = (int16_t)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.w = (int16_t)value;
                    break;
                }
                break;
            case BB_UWORD:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.uw = (uint16_t)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.uw = (uint16_t)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.uw = (uint16_t)value;
                    break;
                }
                break;
            case BB_DWORD:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.dw = (int32_t)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.dw = (int32_t)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.dw = (int32_t)value;
                    break;
                }
                break;
            case BB_UDWORD:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.udw = (uint32_t)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.udw = (uint32_t)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.udw = (uint32_t)value;
                    break;
                }
                break;
            case BB_QWORD:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.qw = (int64_t)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.qw = (int64_t)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.qw = (int64_t)value;
                    break;
                }
                break;
            case BB_UQWORD:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.uqw = (uint64_t)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.uqw = (uint64_t)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.uqw = (uint64_t)value;
                    break;
                }
                break;
            case BB_FLOAT:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.f = (float)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.f = (float)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.f = (float)value;
                    break;
                }
                break;
            case BB_DOUBLE:
                switch (type) {
                case 0:
                    pipevari_list[variable_counter].value.d = (double)value_i64;
                    break;
                case 1:
                    pipevari_list[variable_counter].value.d = (double)value_ui64;
                    break;
                case 2:
                    pipevari_list[variable_counter].value.d = (double)value;
                    break;
                }
                break;
            }
            pipevari_list[variable_counter].vid = Dat->vids[variable_counter];
            pipevari_list[variable_counter].type = Dat->dtypes[variable_counter];
            variable_counter++;
        }
        column_counter++;
    } while (status == 0);
    if (status == END_OF_FILE) {
        if (column_counter) return variable_counter;
        else return STIMULI_END_OF_FILE;
    }
    return variable_counter;       /* There are more lines inside the file */
}

int DatGetNumberOfStimuliVariables(STIMULI_FILE *par_File)
{
    DAT_STIMULI_FILE *Dat = (DAT_STIMULI_FILE*)par_File->File;
    return (Dat->variable_count);
}

void DatCloseStimuliFile(STIMULI_FILE *par_File)
{
    if (par_File != NULL) {
        DAT_STIMULI_FILE *Dat = (DAT_STIMULI_FILE*)par_File->File;
        if (Dat != NULL) {
            if (Dat->fh != NULL) close_file(Dat->fh);
            if (Dat->vids != NULL) my_free(Dat->vids);
            if (Dat->dtypes != NULL) my_free(Dat->dtypes);
            if (Dat->columns != NULL) my_free(Dat->columns);
            my_free(Dat);
        }
        my_free(par_File);
    }
}

int *DatGetStimuliVariableIds(STIMULI_FILE *par_File)
{
    DAT_STIMULI_FILE *Dat = (DAT_STIMULI_FILE*)par_File->File;
    return (Dat->vids);
}

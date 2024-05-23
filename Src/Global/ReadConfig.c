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


#define READCONF_C
#include <stdio.h>
#include <string.h>
#include "Files.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "ReadConfig.h"


void remove_comment (FILE *file, int *line_counter)
{
    int one_char;
    while ((one_char = fgetc (file)) != '\n')
        if (one_char == EOF) return;
    if (line_counter != NULL) (*line_counter)++;
}

int read_token (FILE *file, char *word, int maxlen, int *line_counter)
{
    int x = 0;
    int status;

    status = read_word (file, word, maxlen, line_counter);
    while (tokens[x].s != NULL) {       /* Compare if it is a key word */
        if (!strcmp (tokens[x].s, word))
           return tokens[x].t;
        x++;
    }
    return status;
}

int read_word (FILE *file, char *word, int maxlen, int *line_counter)
{
    int char_count = 0;
    int one_char;
    int end_of_word = 0;
    char *wp = word;

    do {
        one_char = fgetc (file);
        if ((one_char == '\n') && (line_counter != NULL)) (*line_counter)++;
        if (one_char == ';') remove_comment (file, line_counter);
    } while ((one_char == ',') || (one_char == '\n') ||
            (one_char == '\t') || (one_char == '\r') ||
            (one_char == ' ') || (one_char == ';'));
    do {
        if ((one_char == EOF) || (one_char == ',') ||
            (one_char == '\n') || (one_char == '\t') ||
            (one_char == '\r') || (one_char == ' ') ||
            (one_char == ';')) {
            if (one_char == ';') remove_comment (file, line_counter);
            *wp = '\0';
            end_of_word = 1;
        } else {
            *wp++ = (char)one_char;
            if (++char_count >= maxlen) {
                *wp = '\0';
                return -1;
            }
        }
        if (!end_of_word) {
            one_char = fgetc (file);
            if ((one_char == '\n') && (line_counter != NULL)) (*line_counter)++;
        }
    } while (!end_of_word);
    if (one_char == EOF) return EOF;
    return 0;
}


int read_word_with_blacks (FILE *file, char *word, int maxlen, int *line_counter)
{
    int char_count = 0;
    int one_char;
    int blank_counter = 0;
    int end_of_word = 0;
    char *wp = word;

    do {
        one_char = fgetc (file);
        if ((one_char == '\n') && (line_counter != NULL)) (*line_counter)++;
        if (one_char == ';') remove_comment (file, line_counter);
    } while ((one_char == ',') || (one_char == '\n') ||
             (one_char == '\t') || (one_char == '\r') ||
             (one_char == ' ') || (one_char == ';'));
    do {
        if ((one_char == EOF) || (one_char == ',') ||
            (one_char == '\n') || (one_char == '\t') ||
            (one_char == '\r') || (one_char == ';')) {
            if (one_char == ';') remove_comment (file, line_counter);
            *wp = '\0';
            end_of_word = 1;
        } else if (one_char == ' ') {
            blank_counter++;   // First only count
        } else {
            if (blank_counter) {  // takeover whitespaces into token
                if ((char_count + blank_counter + 1) >= maxlen) {
                    *wp = '\0';
                    return -1;
                }
                while (blank_counter > 0) {
                    blank_counter--;
                    *wp++ = ' ';
                }
            }
            *wp++ = (char)one_char;
            if (++char_count >= maxlen) {
                *wp = '\0';
                return -1;
            }
        }
        if (!end_of_word) {
            one_char = fgetc (file);
            if ((one_char == '\n') && (line_counter != NULL)) (*line_counter)++;
        }
    } while (!end_of_word);
    if (one_char == EOF) return EOF;
    return 0;
}

int read_token_with_blacks (FILE *file, char *word, int maxlen, int *line_counter)
{
    int x = 0;
    int status;

    status = read_word_with_blacks (file, word, maxlen, line_counter);
    while (tokens[x].s != NULL) {       /* Compare if it is a keyword */
        if (!strcmp (tokens[x].s, word))
           return tokens[x].t;
        x++;
    }
    return status;
}

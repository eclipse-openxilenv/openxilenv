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


#include <inttypes.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Blackboard.h"
#include "ThrowError.h"
#include "TextReplace.h"

static const char *remove_whitespace (const char *s)
{
    while (isascii(*s) && isspace(*s)) s++;
    return s;
}

// This will parse a color string of an enum
// Syntax: RGB(r:g:b) r,g,b -> [0 .. 255].
// The return value is a pointer to the next character after the closing ) if a valid GB macro was found
// otherwise it will return the txt pointer
// *ret_enum_color = 0x00bbggrr
static const char *GetRGBMacroValue (const char *txt, int32_t *ret_enum_color)
{
    int32_t color_tmp;
    const char *p;
    if (!strncmp ("RGB(", txt, 4)) {
        p = txt + 4; // Jump over "RGB("
        color_tmp = (strtoul (p, (char**)&p, 0) & 0xFFUL);
        if (*p == ':') {
            p++;
            color_tmp |= (strtoul (p, (char**)&p, 0) & 0xFFUL) << 8;
            if (*p == ':') {
                p++;
                color_tmp |= (strtoul (p, (char**)&p, 0) & 0xFFUL) << 16;
                if (*p == ')') {
                    p++;
                    p = remove_whitespace (p);
                    // Have found a valid RGB() Macro
                    if (ret_enum_color != NULL) *ret_enum_color = color_tmp;
                    return p;
                }
            }
        }
    }
    if (ret_enum_color != NULL) *ret_enum_color = -1;  // If no RGB macro found color is undefined
    return txt;
}


int convert_value2textreplace (int64_t wert, const char *conversion,
                               char *txt, int maxc, int *pcolor)
{
    int64_t from, to = 0, to_old;
    char *endp;
    const char *c;
    char *t;
    int thats_it = 0;
    int char_count, enum_count = 0;

    c = conversion;

    while (*c != '\0') {
        to_old = to;
        c = remove_whitespace (c);

        /* From */
        if (*c == '*') {
            c++;
            from = INT64_MIN;
        } else {
            from = strtoll (c, &endp, 0);
            if (c == endp) {
                 goto ERROR_LABEL;
            } else {
                 c = endp;
            }
        }
        c = remove_whitespace (c);

        /* To */
        if (*c == '*') {
            c++;
            to = INT64_MAX;
        } else {
            to = strtoll (c, &endp, 0);
            if (c == endp) {
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }

        /* Check if the ranges overlap */
        if ((enum_count && (to_old >= from)) || (from > to)) {
            goto ERROR_LABEL;
        }

        c = remove_whitespace (c);

        /* Check if the text replace matches */
        if ((wert >= from) && (wert <= to)) {
            thats_it = 1;
        }

        /* Text */
        char_count = 0;         /* Rest the character counter */
        if (*c == '\"') {       /* Text replace have to start with an " */
            c++;

            c = GetRGBMacroValue (c, pcolor);
            t = txt;
            while (*c != '\"') {
                if (char_count >= maxc) {
                    goto ERROR_LABEL;
                }

                /* Text replace should not include a ; or \0 */
                if ((*c == '\0') || (*c == ';')) {
                    goto ERROR_LABEL;
                }

                /* If there is a Text replace copy it to dest */
                if (thats_it) {
                    *t++ = *c++;
                } else {
                    c++;
                }
            }
            c++;                /* Afterwards jump over '\"' character */
            if ((*c != ';') && (*c != '\0')) { /* A end of line or a ';' must follow  */
                goto ERROR_LABEL;
            }
            if (*c != '\0') c++;

            if (thats_it) {
                *t = '\0';     /* Termiate the string  */
                return 0;      /* We have found a enum with the matching range */
            }
            enum_count++;       /* Increment enum count */
            c = remove_whitespace (c);
        } else {
            goto ERROR_LABEL;
        }
    }

    StringCopyMaxCharTruncate (txt, "out of range", maxc);
    return -2;

    ERROR_LABEL:
    *txt = '\0';
    return -1;
}

// Syntax: 0 0 "RGB(0xFF:0x00:0x00)text";
int convert_textreplace2value (const char *conversion, char *txt, int64_t *pfrom, int64_t *pto)
{
    int64_t from, to = 0, to_old;
    char *endp;
    const char *c;
    char *t;
    int thats_it = 0;
    int enum_count = 0;

    c = conversion;

    while (*c != '\0') {
        to_old = to;
        c = remove_whitespace (c);

        /* From */
        if (*c == '*') {
            c++;
            from = INT64_MIN;
        } else {
            from = strtoll (c, &endp, 0);
            if (c == endp) {
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }
        c = remove_whitespace (c);

        /* To */
        if (*c == '*') {
            c++;
            to = INT64_MAX;
        } else {
            to = strtoll (c, &endp, 0);
            if (c == endp) {
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }

        /* Check if the text replace matches */
        if ((enum_count && (to_old >= from)) || (from > to))  {
            goto ERROR_LABEL;
        }

        c = remove_whitespace (c);

        /* Text */
        if (*c == '\"') {   /* Text replace have to start with an " */
            c++;
            t = txt;
            // If this will start with an RGB() macro ignore that
            if ((c[0]  == '(') &&
                (c[1]  == 'R') &&
                (c[2]  == 'G') &&
                (c[3]  == 'B')) {
                c += 4;
                while (*c != ')') {
                    if (*c == 0) {
                        goto ERROR_LABEL;
                    }
                    c++;
                }
                c++;  // jump over ')'
            }
            thats_it = 1;
            while (*c != '\"') {
                /* Text replace should not include a ; or \0 */
                if ((*c == '\0') || (*c == ';')) {
                    goto ERROR_LABEL;
                }

                if (*t != *c) {
                    thats_it = 0;   // don't match
                }
                c++;
                if (*t != 0) t++;
            }
            c++;                /* Afterwards jump over '\"' character */
            if ((*c != ';') && (*c != '\0')) {  /* A end of line or a ';' must follow  */
                goto ERROR_LABEL;
            }
            if (*c != '\0') c++;
            if (*t != 0) thats_it = 0;     // Compare string is longer so it is no match

            if (thats_it) {
                if (pfrom != NULL) *pfrom = from;
                if (pto != NULL) *pto = to;
                return 0;        // We have found a matching enum
            }
            enum_count++;       /* Increment enum count */
            c = remove_whitespace (c);
        } else {
            goto ERROR_LABEL;
        }
    }
  ERROR_LABEL:
    return -1;
}



int textreplace2asap (const char *sc_conv, char *a2l_conv)
{
    int64_t from, to = 0, to_old;
    char *endp;
    const char *c;
    char *t;
    int char_count, enum_count = 0;
    char *txt;

    txt = my_malloc (BBVARI_CONVERSION_SIZE*2);
    if (txt == NULL) {
        return -1;
    }

    c = sc_conv;

    StringCopyMaxCharTruncate (a2l_conv, "", BBVARI_CONVERSION_SIZE);
    while (*c != '\0') {
        to_old = to;
        c = remove_whitespace (c);

        /* from */
        if (*c == '*') {
            c++;
            from = INT64_MIN;
        } else {
            from = strtoll (c, &endp, 0);
            if (c == endp) {
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }
        c = remove_whitespace (c);

        /* to */
        if (*c == '*') {
            c++;
            to = INT64_MAX;
        } else {
            to = strtoll (c, &endp, 0);
            if (c == endp) {
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }

        /* Check if the ranges overlap */
        if ((enum_count && (to_old >= from)) || (from > to)) {
            goto ERROR_LABEL;
        }

        c = remove_whitespace (c);

        /* Text */
        char_count = 0;
        if (*c == '\"') {   /* Text replace have to start with an " */
            c++;
            c = GetRGBMacroValue (c, NULL); /* If there are RGB Macro jump over it */

            sprintf (txt, "      %" PRIi64 " \"", to);
            t = txt + strlen (txt);
            while (*c != '\"') {
                if (char_count >= (int)(sizeof (txt)-2)) {
                    goto ERROR_LABEL;
                }

                /* Text replace should not include a ; or \0 */
                if ((*c == '\0') || (*c == ';')) {
                    goto ERROR_LABEL;
                }

                *t++ = *c++;
            }
            c++;                /* Afterwards jump over '\"' character */
            if ((*c != ';') && (*c != '\0')) {  /* A end of line or a ';' must follow  */
                goto ERROR_LABEL;
            }
            if (*c != '\0') c++;

            *t = '\0';     /* Terminate string  */

            strcat (txt, "\"\r\n");
            strcat (a2l_conv, txt);
            enum_count++;
            c = remove_whitespace (c);
        } else {
            goto ERROR_LABEL;
        }
    }
    my_free(txt);
    return enum_count;

    ERROR_LABEL:
    my_free(txt);
    return -1;
}


static char *strncpy_x (char *dst, char *src, int maxc)
{
    size_t len = strlen (src) + 1;
    if (len > (size_t)maxc) {
        MEMCPY (dst, src, (size_t)maxc - 1);
        dst[maxc-1] = 0;
    } else {
        MEMCPY (dst, src, len);
    }
    return dst;
}


int GetNextEnumFromEnumListErrMsg (int idx, const char *conversion,
                                   char *txt, int maxc, int64_t *from_v, int64_t *to_v, int32_t *enum_color, char *ErrMsgBuffer, int SizeOfErrMsgBuffer)
{
    int64_t from, to = 0, to_old;
    char *endp;
    const char *c;
    char *t;
    int thats_it = 0;
    int word_count = 0;
    int idx_counter = 0;

    c = conversion;

    while (*c != '\0') {
        to_old = to;
        c = remove_whitespace (c);

        /* From */
        if (*c == '*') {
            c++;
            from = INT64_MIN;
        } else {
            from = strtoll (c, &endp, 0);
            if (c == endp) {
                if (ErrMsgBuffer != NULL) {
                    strncpy_x (ErrMsgBuffer, "\"from\" is not a valid integer value", SizeOfErrMsgBuffer);
                }
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }
        c = remove_whitespace (c);

        /* To */
        if (*c == '*') {
            c++;
            to = INT64_MAX;
        } else {
            to = strtoll (c, &endp, 0);
            if (c == endp) {
                if (ErrMsgBuffer != NULL) {
                    strncpy_x (ErrMsgBuffer, "\"to\" is not a valid integer value", SizeOfErrMsgBuffer);
                }
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }

        /* Check if the ranges overlap */
        if ((word_count && (to_old >= from)) || (from > to)) {
            if (ErrMsgBuffer != NULL) {
                strncpy_x (ErrMsgBuffer, "\"from\" is overlapping \"to\" before", SizeOfErrMsgBuffer);
            }
            goto ERROR_LABEL;
        }

        c = remove_whitespace (c);

        /* Check if te right one */
        if (idx == idx_counter) {
            thats_it = 1;
        }
        idx_counter++;

        /* Text */
        if (*c == '\"') {   /* Text replace have to start with an " */
            c++;
            c = GetRGBMacroValue (c, enum_color);

            t = txt;
            while (*c != '\"') {
                /* Text replace should not include a ; or \0 */
                if ((*c == '\0') || (*c == ';')) {
                    if (ErrMsgBuffer != NULL) {
                        strncpy_x (ErrMsgBuffer, "text replace missing \" or illeagal ;", SizeOfErrMsgBuffer);
                    }
                    goto ERROR_LABEL;
                    }

                if ((t - txt) == (maxc - 1)) {
                    ThrowError (1, "more chars inside an enum as allowed (%i) it will be truncated", maxc - 1);
                }
                /* If we found matching text enum copy it to dest */
                if (thats_it && ((t - txt) < (maxc - 1))) {
                    *t++ = *c++;
                } else {
                    c++;
                }
            }
            c++;                 /* Afterwards jump over '\"' character */
            if ((*c != ';') && (*c != '\0')) {  /* A end of line or a ';' must follow  */
                if (ErrMsgBuffer != NULL) {
                    strncpy_x (ErrMsgBuffer, "text replace missing \" or illeagal ;", SizeOfErrMsgBuffer);
                }
                goto ERROR_LABEL;
            }
            if (*c != '\0') c++;

            if (thats_it) {
                *t = '\0';     /* Terminate string  */
                *from_v = from;
                *to_v = to;
                if (ErrMsgBuffer != NULL) {
                    strncpy_x (ErrMsgBuffer, "no errors", SizeOfErrMsgBuffer);
                }
                return idx_counter;     /* Return the current index number */
            }
            word_count++;
            c = remove_whitespace (c);
        } else {
            if (ErrMsgBuffer != NULL) {
                strncpy_x (ErrMsgBuffer, "text replace missing \"", SizeOfErrMsgBuffer);
            }
            goto ERROR_LABEL;
        }
    }
    if (ErrMsgBuffer != NULL) {
        strncpy_x (ErrMsgBuffer, "last element reached", SizeOfErrMsgBuffer);
    }
    StringCopyMaxCharTruncate (txt, "out of range", maxc);
    if (enum_color != NULL) *enum_color = -1;
    return -1;

    ERROR_LABEL:
    *txt = '\0';
    if (enum_color != NULL) *enum_color = -1;
    return -2;
}

int GetNextEnumFromEnumList (int idx, const char *conversion,
                             char *txt, int maxc, int64_t *from_v, int64_t *to_v, int32_t *enum_color)
{
    return GetNextEnumFromEnumListErrMsg (idx, conversion,
                                          txt, maxc, from_v, to_v, enum_color, NULL, 0);
}


int GetNextEnumFromEnumListErrMsg_Pos (int pos, const char *conversion,
                                       char *txt, int maxc, int64_t *from_v, int64_t *to_v, int32_t *enum_color, char *ErrMsgBuffer, int SizeOfErrMsgBuffer)
{
    int64_t from, to = 0;
    char *endp;
    const char *c;
    char *t;

    c = conversion + pos;

    c = remove_whitespace (c);

    if (*c != '\0') {
        c = remove_whitespace (c);

        /* From */
        if (*c == '*') {
            c++;
            from = INT64_MIN;
        } else {
            from = strtoll (c, &endp, 0);
            if (c == endp) {
                if (ErrMsgBuffer != NULL) {
                    strncpy_x (ErrMsgBuffer, "\"from\" is not a valid integer value", SizeOfErrMsgBuffer);
                }
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }
        c = remove_whitespace (c);

        /* To */
        if (*c == '*') {
            c++;
            to = INT64_MAX;
        } else {
            to = strtoll (c, &endp, 0);
            if (c == endp) {
                if (ErrMsgBuffer != NULL) {
                    strncpy_x (ErrMsgBuffer, "\"to\" is not a valid integer value", SizeOfErrMsgBuffer);
                }
                goto ERROR_LABEL;
            } else {
                c = endp;
            }
        }

        c = remove_whitespace (c);

        /* Text */
        if (*c == '\"') {   /* Text replace have to start with an " */
            c++;
            c = GetRGBMacroValue (c, enum_color);

            t = txt;
            while (*c != '\"') {
                /* Text replace should not include a ; or \0 */
                if ((*c == '\0') || (*c == ';')) {
                    if (ErrMsgBuffer != NULL) {
                        strncpy_x (ErrMsgBuffer, "text replace missing \" or illeagal ;", SizeOfErrMsgBuffer);
                    }
                    goto ERROR_LABEL;
                }
                if (txt == NULL) {
                    c++;
                } else {
                    if ((t - txt) == (maxc - 1)) {
                        ThrowError (1, "more chars inside an enum as allowed (%i) it will be truncated", maxc - 1);
                    }
                    /* If we found matching text enum copy it to dest */
                    if ((t - txt) < (maxc - 1)) {
                        *t++ = *c++;
                    } else {
                        c++;
                    }
                }
            }
            c++;                /* Afterwards jump over '\"' character */
            if ((*c != ';') && (*c != '\0' )) { //* A end of line or a ';' must follow  */
                if (ErrMsgBuffer != NULL) {
                    strncpy_x (ErrMsgBuffer, "text replace missing \" or illeagal ;", SizeOfErrMsgBuffer);
                }
                goto ERROR_LABEL;
            }
            if (*c != '\0') c++;

            if (txt != NULL) *t = '\0';     /* Terminate the string */
            if (from_v != NULL) *from_v = from;
            if (to_v != NULL) *to_v = to;
            if (ErrMsgBuffer != NULL) {
                strncpy_x (ErrMsgBuffer, "no errors", SizeOfErrMsgBuffer);
            }
            c = remove_whitespace (c);
            return (int)(c - conversion);     /* Successful return the position inside the string */
        } else {
            if (ErrMsgBuffer != NULL) {
                strncpy_x (ErrMsgBuffer, "text replace missing \"", SizeOfErrMsgBuffer);
            }
            goto ERROR_LABEL;
        }
    }
    if (ErrMsgBuffer != NULL) {
        strncpy_x (ErrMsgBuffer, "last element reached", SizeOfErrMsgBuffer);
    }
    if ((txt != NULL) && (maxc > 13)) StringCopyMaxCharTruncate (txt, "out of range", maxc);
    if (enum_color != NULL) *enum_color = -1;
    return -1;

    ERROR_LABEL:
    if (txt != NULL) *txt = '\0';
    if (enum_color != NULL) *enum_color = -1;
    return -2;
}

int GetNextEnumFromEnumList_Pos (int pos, char *conversion,
                             char *txt, int maxc, int64_t *from_v, int64_t *to_v, int32_t *enum_color)
{
    return GetNextEnumFromEnumListErrMsg_Pos (pos, conversion,
                                          txt, maxc, from_v, to_v, enum_color, NULL, 0);
}

int GetEnumListSize (char *conversion)
{
    int pos = 0;
    int ret = 0;
    while (pos >= 0) {
        pos = GetNextEnumFromEnumList_Pos (pos, conversion, NULL, 0, NULL, NULL, NULL);
        if (pos > 0) ret++;
        else if (pos == -2) {
            // Text replace have error(s)
            return 0;
        }
    }
    return ret;
}

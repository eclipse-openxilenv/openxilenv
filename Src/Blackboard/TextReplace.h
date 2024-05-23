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


#ifndef TEXTREPLACE_H
#define TEXTREPLACE_H

#include <stdint.h>

int convert_value2textreplace (int64_t wert, const char *conversion,
                               char *txt, int maxc, int *pcolor);

int convert_textreplace2value (const char *conversion, char *txt, int64_t *pfrom, int64_t *pto);

int textreplace2asap (const char *sc_conv, char *a2l_conv);

int GetNextEnumFromEnumList (int idx, const char *conversion,
                             char *txt, int maxc, int64_t *from_v, int64_t *to_v, int32_t *enum_color);

int GetNextEnumFromEnumListErrMsg (int idx, const char *conversion,
                                   char *txt, int maxc, int64_t *from_v, int64_t *to_v, int32_t *enum_color, char *ErrMsgBuffer, int SizeOfErrMsgBuffer);

int GetNextEnumFromEnumListErrMsg_Pos (int pos, const char *conversion,
                                       char *txt, int maxc, int64_t *from_v, int64_t *to_v, int32_t *enum_color, char *ErrMsgBuffer, int SizeOfErrMsgBuffer);

int GetNextEnumFromEnumList_Pos (int pos, char *conversion,
                             char *txt, int maxc, int64_t *from_v, int64_t *to_v, int32_t *enum_color);

int GetEnumListSize (char *conversion);

#endif

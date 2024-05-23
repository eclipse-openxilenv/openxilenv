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


#ifndef __A2LTOKENIZER_H
#define __A2LTOKENIZER_H

#include "A2LParser.h"

// Open the A2L file, create a parser object and initialize it.
ASAP2_PARSER *InitASAP2Parser (const char *Filename, int Flags);

// Read a string starts with a whitespace or a comment
// Is the Flag 'BufferedFlag' are set the result will be stored inside an own buffer
// If 'BufferedFlag' is not set the result will be overwritten with the next call of ths function
char* ParseNextString (ASAP2_PARSER *Parser, int BufferedFlag);

// Will be return TRUE if file end is reached.
int IsEndOfParsing (ASAP2_PARSER *Parser);

#endif

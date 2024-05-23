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


#ifndef OSCILLOSCOPEINI_H
#define OSCILLOSCOPEINI_H

#include "OscilloscopeData.h"

#ifdef __cplusplus
extern "C" {
#endif

void write_osziwin_ini (const char *SectionName, OSCILLOSCOPE_DATA* poszidata, int xpos, int ypos, int xsize, int ysize, int minimized);

void read_osziwin_ini (const char *SectionName, OSCILLOSCOPE_DATA* poszidata);


#ifdef __cplusplus
}
#endif

#endif // OSCILLOSCOPEINI_H


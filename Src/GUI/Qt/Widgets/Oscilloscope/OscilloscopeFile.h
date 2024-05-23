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


#ifndef OSCILLOSCOPEFILE_H
#define OSCILLOSCOPEFILE_H

#include "OscilloscopeData.h"

// Write oscilloscope buffer content as stimulation file
#define FILE_FORMAT_DAT_TE      1
#define FILE_FORMAT_EXT_DAT_TE  2
int WriteOsziBuffer2StimuliFile (OSCILLOSCOPE_DATA* poszidata, const char *par_FileName, int par_Format, const char *par_Title);

#endif

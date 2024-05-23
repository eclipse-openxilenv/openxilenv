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


#ifndef __TRACEWRITEMDFFILE_H
#define __TRACEWRITEMDFFILE_H

#include "TraceWriteFile.h"

int OpenWriteMdfHead (START_MESSAGE_DATA hdrec_data,
                      int32_t *vids, char *dec_phys_flags, FILE **pfile);

int WriteRingbuffMdf (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count);

int TailMdfFile (FILE *fh, uint32_t Samples, uint64_t RecorderStartTime);

int WriteCommentToMdf (FILE *File, uint64_t Timestamp, const char *Comment);

#endif

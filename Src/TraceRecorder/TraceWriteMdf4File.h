/* ---------- write_mdf4.h ------------------------------------------------ *\
 *                                                                         *
 *  Programm: Softcar                                                      *
 *                                                                         *
 *  Autor : Bieber         Abt.: TE-P          Datum: 22.08.2007           *
 *                                                                         *
 *  Funktion : Stimuli-File-Converter und MDF-Converter fuer dem           *
 *             HD-Recorder                                                 *
 *                                                                         *
\* ------------------------------------------------------------------------*/

#ifndef __TRACEWRITEMDF4FILE_H
#define __TRACEWRITEMDF4FILE_H

#include "TraceWriteFile.h"

int OpenWriteMdf4Head (START_MESSAGE_DATA hdrec_data,
                      int32_t *vids, char *dec_phys_flags, FILE **pfile);

int WriteRingbuffMdf4 (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count);

int TailMdf4File (FILE *fh, uint32_t Samples, uint64_t RecorderStartTime);

int WriteCommentToMdf4 (FILE *File, uint64_t Timestamp, const char *Comment);

#endif

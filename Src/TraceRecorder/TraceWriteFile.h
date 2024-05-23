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


#ifndef TRACEWRITEFILE_H
#define TRACEWRITEFILE_H

#include "TraceRecorder.h"
#include "Blackboard.h"

/* ------------------------------------------------------------ *\
 *    Global defines  					        *
\* ------------------------------------------------------------ */

/* lese Test-Makros */
#ifdef FILEWROP_C
    #ifdef FILEWROP_TEST
        #define TEST
    #endif
    #include "test_mac.h"
    #undef TEST
#endif

#if 0
/* Look back Ring Buffer Size als Potenz von zwei */
#define TWO_POT_LOCK_BACK_DEPTH  2
#define LOCK_BACK_DEPTH  (1 << TWO_POT_LOCK_BACK_DEPTH)
#define RINGBUFF_CLIP_MASK  ((1 << TWO_POT_LOCK_BACK_DEPTH) - 1)

#define WRITE_FILE_ERROR    -1501
#define READ_FILE_ERROR     -1502


/* ------------------------------------------------------------ *\
 *    Type definitions 					        *
\* ------------------------------------------------------------ */

typedef struct {
    unsigned long timestamp;     /* Zeitmarke in Timer-Clock-Ticks */
    VARI_IN_PIPE *entrylist;     /* Zeiger auf Ringpuffer-Eintraege */
} RING_BUFFER_COLOMN;            /* 8 Bytes 2^3 */

#endif

/* MCS-Rec-Dateien */
typedef struct {
    long fileEnd;
    long filePos;
    unsigned short vers;
    unsigned short dataBeg;
    unsigned short recSize;
    char usrCmnt[79+1];
    float sampleRate;
    float eventTick;
    unsigned short nSrcRefs;
    unsigned short nElemRefs;
} recoFileHdr;

typedef struct {
    char robName[79+1];
    char binName[79+1];
} recoSrcRef;

typedef struct {
    unsigned short recoOfs;
    char srcFile;
    char kname[16+1];
} recoElemRef;

/* ------------------------------------------------------------ *\
 *    Global variables 	                    	        	*
\* ------------------------------------------------------------ */

extern int conv_rec_time_steps;    /* Aufzeichnungsschrittweite in Timer
                                      Steps */

/* ------------------------------------------------------------ *\
 *    Global constants                                          *
\* ------------------------------------------------------------ */

/* ------------------------------------------------------------ *\
 *    Global Functions                                          *
\* ------------------------------------------------------------ */

/* ---- read_next_word --------------------------------------------------- *\
 *    Liest aus aktueller Dateiposition ein Wort, das von einem Leer-      *
 *    zeichen, Tabulator bzw. Return begrenzt ist.                         *
 * ---- Parameter -------------------------------------------------------- *
 *    FILE *file  :        Zeiger auf Struktur des Dateisteuerblock        *
 *    char *word  :        Zeiger wohin Wort gelesen werden soll           *
 *    int maxlen  :        Max. Laenge des Wortes ohne Stringendezeichen   *
 * ---- Rueckgabe -------------------------------------------------------- *
 *    END_OF_LINE wenn das Ende einer Zeile erreicht wurde                 *
 *    END_OF_FILE wenn das Dateiende erreicht wurde                        *
 *    oder -1 wenn Wort laenger als 'maxlen' ist                           *
 * ---- Unterfunktionen -------------------------------------------------- *
 *    keine
 * ---- Globale Variable ------------------------------------------------- *
 *    keine                                                                *
 * ----------------------------------------------------------------------- */
extern int read_next_word (FILE *file, char *word, int maxlen);

/* ---- open_write_stimuli_head ------------------------------------------ *\
 *    oeffne eine Stimuli-File zum Schreiben und schreibe Stimuli-         *
 *    Filekopf hinein                                                      *
 * ---- Parameter -------------------------------------------------------- *
 *    START_MESSAGE_DATA hdrec_data: Info-Daten ueber Stimuli-Datei        *
 *    BLACKBOARD_ELEM ** variframe:  Blackboard-Zugriffs-Frame der aufzu-  *
 *                                   zeichnenden Variable                  *
 *    FILE **pfile         Rueckgabe des Filepointers                      *
 * ---- Rueckgabe -------------------------------------------------------- *
 *    CANNOT_OPEN_RECFILE wenn Stimuli-Datei nicht geoeffnet werden konnte *
 +    oder 0 wenn alles OK                                                 *
 * ---- Unterfunktionen -------------------------------------------------- *
 *    FILE *open_file (char *name, char *flags);                           *
 *    void error (int level, char *txt, ...);                              *
 * ---- Globale Variable ------------------------------------------------- *
 *    conv_rec_time_steps                                                  *
 * ----------------------------------------------------------------------- */
extern int open_write_stimuli_head (START_MESSAGE_DATA hdrec_data,
                                    int32_t *vids, FILE **pfile);

extern int open_write_mcsrec_head (START_MESSAGE_DATA hdrec_data,
                                   int32_t *vids, FILE **pfile);

extern int open_write_etasascii_head (START_MESSAGE_DATA hdrec_data,
                                      int32_t *vids, char *dec_phys_flags, FILE **pfile);


/* ---- conv_bin_stimuli_write ------------------------------------------- *\
 *    konvertiert eine Message des RDPIPE-Prozesse in das Stimuli-         *
 *    Datei-Format und schreibt diese gleich ins File.                     *
 *    Wird diese Funktion mit (pipevari_list == NULL) aufgerufen, so       *
 *    initiallisiert dies den Konverter. dabei haben die Variable          *
 *    folgende Bedeutung: time_stamp -> Startzeit der Aufzeichnung,        *
 *                        elem_count -> Anzahl der aufzuzeichnenden        *
 *                                      Variable                           *
 *     bei einem erneuten Aufruf mit (pipevari_list == NULL) wird der      *
 *     Konverter beendet                                                   *
 * ---- Parameter -------------------------------------------------------- *
 *    FILE *file  :        Zeiger auf Struktur des Dateisteuerblock        *
 *    unsigned long time_stamp:  Zeitmarke der Nachricht (Absendezeitpunkt)*
 *    int elem_count:               Anzahl Variable in Nachricht           *
 *    VARI_IN_PIPE *pipevari_list:  Daten der einer Nachricht              *
 *    int *vids:          Variable-ID's der einzelnen Variablen            *
 * ---- Rueckgabe -------------------------------------------------------- *
 *    WRITE_FILE_ERROR wenn ein Fehler beim Schreiben ins Stimuli-File     *
 *    aufgetreten ist oder 0 wenn alles OK                                 *
 * ---- Unterfunktionen -------------------------------------------------- *
 *    Makro TSCOUNT_LARGER_TS(ts_count, ts)                                *
 *    Makro TSCOUNT_SMALLER_TS(ts_count, ts)                               *
 *    void put_varilist_ringbuff (RING_BUFFER_COLOMN *ring_buff,           *
 *                                 VARI_IN_PIPE *pipevari_list, int *vids, *
 *                                 int pipevari_count);                    *
 *    int write_ringbuff_stimuli (FILE *file,RING_BUFFER_COLOMN *ring_buff,*
 *                    RING_BUFFER_COLOMN *stamp, int rpvari_count,         *
 *                    unsigned long timersteps, unsigned long line_count); *

 * ---- Globale Variable ------------------------------------------------- *
 *    conv_rec_time_steps                                                  *
 * ----------------------------------------------------------------------- */
extern int conv_bin_stimuli_write (RECORDER_STRUCT *pRecorder, 
                                   uint32_t time_stamp, int elem_count,
                                   VARI_IN_PIPE *pipevari_list, long *vids);



/* ------------------------------------------------------------ *\
 *    Local Functions                                           *
\* ------------------------------------------------------------ */

/* ---- put_varilist_ringbuff -------------------------------------------- *\
 *    schreibt den Inhalt einer Message in den Look-Back-Ringbuffer        *
 *    Look-Back-Ringbuffer sortiert eingehende Nachrichten nach ihrer      *
 *    Zeitmarke (timestamp)                                                *
 * ---- Parameter -------------------------------------------------------- *
 *    RING_BUFFER_COLOMN *ring_buff: Ringpuffer-Zeile in die Nachricht     *
 *                                   eingetragen werden soll               *
 *    VARI_IN_PIPE *pipevari_list: Daten der empfangenen Nachricht         *
 *    long *vids:           Variable-ID's der einzelnen Variablen          *
 *    int pipevari_count:  Anzahl Variable in Nachricht                    *
 * ---- Rueckgabe -------------------------------------------------------- *
 *    keine                                                                *
 * ---- Unterfunktionen -------------------------------------------------- *
 *    keine                                                                *
 * ---- Globale Variable ------------------------------------------------- *
 *    keine                                                                *
 * ----------------------------------------------------------------------- */
extern void put_varilist_ringbuff (RING_BUFFER_COLOMN *ring_buff,
                                   VARI_IN_PIPE *pipevari_list, int32_t *vids,
                                   int pipevari_count);

/* ---- write_ringbuff_stimuli ------------------------------------------- *\
 *    uebertrage Variable aus letzen Ringpuffereintrag in den Stemple      *
 *    und schreibe Stempel in Stimuli-Datei                                *
 * ---- Parameter -------------------------------------------------------- *
 *    FILE *file  :        Zeiger auf Struktur des Dateisteuerblock        *
 *    RING_BUFFER_COLOMN *ring_buff: Ringpuffer-Zeile die in Stempel ueber-*
 *                                   tragen und freigegeben werden soll    *
 *    RING_BUFFER_COLOMN *stamp: enthaelt immer die als naechstes ins File *
 *                               geschriebenen Werte (Stempel)             *
 *    int rpvari_count:    Ringpuffer Breite (Anzahl der Variable)         *
 *    unsigned long timersteps:   Zeitschittweite im Ringpuffer            *
 *                                in 1/frqtimer                            *
 *    unsigned long line_count   Zeilennummer in Stimuli-Datei             *
 * ---- Rueckgabe -------------------------------------------------------- *
 *    keine                                                                *
 * ---- Unterfunktionen -------------------------------------------------- *
 *    keine                                                                *
 * ---- Globale Variable ------------------------------------------------- *
 *    keine                                                                *
 * ----------------------------------------------------------------------- */
int write_ringbuff_stimuli (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count);

int write_ringbuff_mcsrec  (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count);

int write_ringbuff_etasascii (FILE *file, RING_BUFFER_COLOMN *stamp, int rpvari_count);

int TailMcsRecFile (FILE *fh, unsigned int Samples);

int TailEtasAsciiFile (FILE *fh, unsigned int Samples);

int TailStimuliFile (FILE *fh);

int WriteCommentToStimuli (FILE *File, uint64_t Tmestamp, const char *Comment);

#undef extern
#endif

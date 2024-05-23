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


#ifndef _TS_H
#define _TS_H

#if 1
#define TSCOUNT_LARGER_TS(ts_count, ts) \
    ((ts_count) >= (ts))

#define TSCOUNT_SMALLER_TS(ts_count, ts) \
    ((ts_count) <= (ts))

#else
/* Makro vergleicht ts_count mit ts, liefert wahr zurueck wenn
   (ts_counter > ts) ist, beruecksichtigt dabei eventuell aufgetretenen
   Ueberlaefe in ts_count und ts. Wenn ts viel kleiner ts_count
   (um 0xC0000000 kleiner) wird dies als groesser gewertet. */
#define TSCOUNT_LARGER_TS(ts_count, ts) \
    ((((ts) > 0xD0000000UL) && ((ts_count) < 0x20000000UL)) ? 1 : \
     ((((ts) < 0x20000000UL) && ((ts_count) > 0xD0000000UL)) ? 0 : \
     ((ts_count) >= (ts))))

#define TSCOUNT_SMALLER_TS(ts_count, ts) \
    ((((ts) > 0xD0000000UL) && ((ts_count) < 0x20000000UL)) ? 0 : \
     ((((ts) < 0x20000000UL) && ((ts_count) > 0xD0000000UL)) ? 1 : \
     ((ts_count) <= (ts))))
#endif

/* sollte noch get_time () aufrufen um genauere Zeitangabe zu erhalten !!! */
#define GET_TIMESTAMP() \
    get_timestamp_counter ()


#endif

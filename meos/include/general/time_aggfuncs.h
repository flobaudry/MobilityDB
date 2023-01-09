/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2023, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2023, PostGIS contributors
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written
 * agreement is hereby granted, provided that the above copyright notice and
 * this paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL UNIVERSITE LIBRE DE BRUXELLES BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF UNIVERSITE LIBRE DE BRUXELLES HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * UNIVERSITE LIBRE DE BRUXELLES SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND UNIVERSITE LIBRE DE BRUXELLES HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS. 
 *
 *****************************************************************************/

/**
 * @brief Aggregate functions for time types
 */

#ifndef __TIME_AGGFUNCS_H__
#define __TIME_AGGFUNCS_H__

/* MEOS */
#include "general/span.h"
#include "general/skiplist.h"

/*****************************************************************************/

extern Datum datum_sum_int32(Datum l, Datum r);
extern TimestampTz *timestamp_tagg(TimestampTz *times1, int count1,
  TimestampTz *times2, int count2, int *newcount);
extern Span **period_tagg(Span **periods1, int count1, Span **periods2,
  int count2, int *newcount);

extern SkipList *tstzset_tagg_transfn(SkipList *state, const Set *ts);
extern SkipList *period_tagg_transfn(SkipList *state, const Span *p);
extern SkipList *periodset_tagg_transfn(SkipList *state, const SpanSet *ps);
extern void ensure_same_timetype_skiplist(SkipList *state, uint8 subtype);
extern SkipList *time_tagg_combinefn(SkipList *state1, SkipList *state2);

/*****************************************************************************/

#endif

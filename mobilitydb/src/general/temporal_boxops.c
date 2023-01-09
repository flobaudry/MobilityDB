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
 * @brief Bounding box operators for temporal types.
 *
 * The bounding box of temporal values are
 * - a `Period` for temporal Booleans
 * - a `TBox` for temporal integers and floats, where the *x* coordinate is for
 *   the value dimension and the *t* coordinate is for the time dimension.
 * The following operators are defined: `overlaps`, `contains`, `contained`,
 * `same`, and `adjacent`.
 *
 * The operators consider as many dimensions as they are shared in both
 * arguments: only the value dimension, only the time dimension, or both
 * the value and the time dimensions.
 */

#include "general/temporal_boxops.h"

/* PostgreSQL */
#include <postgres.h>
#include <utils/palloc.h>
#include <fmgr.h>
#include <utils/timestamp.h>
/* MEOS */
#include <meos.h>
#include "point/tpoint_boxops.h"
/* MobilityDB */
#include "pg_general/meos_catalog.h"

/*****************************************************************************/

/**
 * @brief Return the size in bytes to read from toast to get the basic
 * information from a temporal: Temporal struct (i.e., TInstant,
 * TSequence, TSequence, or TSequenceSet) and bounding box size
*/
uint32_t
temporal_max_header_size(void)
{
  size_t sz1 = Max(sizeof(TInstant), sizeof(TSequence));
  size_t sz2 = Max(sizeof(TSequence), sizeof(TSequenceSet));
  return double_pad(Max(sz1, sz2)) + double_pad(sizeof(bboxunion));
}

/*****************************************************************************
 * Bounding box operators for temporal types: Generic functions
 * The inclusive/exclusive bounds are taken into account for the comparisons
 *****************************************************************************/

/**
 * @brief Generic bounding box operator for a timestamp and a temporal value
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_timestamp_temporal_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  TimestampTz t = PG_GETARG_TIMESTAMPTZ(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_temporal_timestamp(temp, t, func, true);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal value and a timestamp
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_temporal_timestamp_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TimestampTz t = PG_GETARG_TIMESTAMPTZ(1);
  bool result = boxop_temporal_timestamp(temp, t, func, false);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a tstzset and a temporal value
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_tstzset_temporal_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  Set *ts = PG_GETARG_SET_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_temporal_tstzset(temp, ts, func, true);
  PG_FREE_IF_COPY(ts, 0);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal value and a tstzset
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_temporal_tstzset_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Set *ts = PG_GETARG_SET_P(1);
  bool result = boxop_temporal_tstzset(temp, ts, func, false);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(ts, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a period and a temporal value
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_period_temporal_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  Span *p = PG_GETARG_SPAN_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_temporal_period(temp, p, func, true);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal value and a period
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_temporal_period_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Span *p = PG_GETARG_SPAN_P(1);
  bool result = boxop_temporal_period(temp, p, func, false);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a periodset and a temporal value
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_periodset_temporal_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  SpanSet *ps = PG_GETARG_SPANSET_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_temporal_periodset(temp, ps, func, true);
  PG_FREE_IF_COPY(ps, 0);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal value and a periodset
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_temporal_periodset_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  SpanSet *ps = PG_GETARG_SPANSET_P(1);
  bool result = boxop_temporal_periodset(temp, ps, func, false);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(ps, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for two temporal values
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_temporal_temporal_ext(FunctionCallInfo fcinfo,
  bool (*func)(const Span *, const Span *))
{
  Temporal *temp1 = PG_GETARG_TEMPORAL_P(0);
  Temporal *temp2 = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_temporal_temporal(temp1, temp2, func);
  PG_FREE_IF_COPY(temp1, 0);
  PG_FREE_IF_COPY(temp2, 1);
  PG_RETURN_BOOL(result);
}

/*****************************************************************************
 * Bounding box operators for temporal types
 *****************************************************************************/

PG_FUNCTION_INFO_V1(Contains_timestamp_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a timestamp contains the bounding period of the temporal
 * value
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_timestamp_temporal(PG_FUNCTION_ARGS)
{
  return boxop_timestamp_temporal_ext(fcinfo, &contains_span_span);
}

PG_FUNCTION_INFO_V1(Contains_temporal_timestamp);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value contains the
 * timestamp
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_temporal_timestamp(PG_FUNCTION_ARGS)
{
  return boxop_temporal_timestamp_ext(fcinfo, &contains_span_span);
}

PG_FUNCTION_INFO_V1(Contains_tstzset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a tstzset contains the one
 * of a temporal value
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_tstzset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_tstzset_temporal_ext(fcinfo, &contains_span_span);
}

PG_FUNCTION_INFO_V1(Contains_temporal_tstzset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value contains the
 * one of a tstzset
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_temporal_tstzset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_tstzset_ext(fcinfo, &contains_span_span);
}

PG_FUNCTION_INFO_V1(Contains_period_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a period contains the bounding period of a temporal value
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_period_temporal(PG_FUNCTION_ARGS)
{
  return boxop_period_temporal_ext(fcinfo, &contains_span_span);
}

PG_FUNCTION_INFO_V1(Contains_temporal_period);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value contains a period
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_temporal_period(PG_FUNCTION_ARGS)
{
  return boxop_temporal_period_ext(fcinfo, &contains_span_span);
}

PG_FUNCTION_INFO_V1(Contains_periodset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a period set contains the one
 * of a temporal value
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_periodset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_periodset_temporal_ext(fcinfo, &contains_span_span);
}

PG_FUNCTION_INFO_V1(Contains_temporal_periodset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value contains the
 * one of a period set
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_temporal_periodset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_periodset_ext(fcinfo, &contains_span_span);
}

PG_FUNCTION_INFO_V1(Contains_temporal_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of the first temporal value contains
 * the one of the second one.
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_temporal_temporal(PG_FUNCTION_ARGS)
{
  return boxop_temporal_temporal_ext(fcinfo, &contains_span_span);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(Contained_timestamp_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a timestamp is contained in the bounding period of the
 * temporal value
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_timestamp_temporal(PG_FUNCTION_ARGS)
{
  return boxop_timestamp_temporal_ext(fcinfo, &contained_span_span);
}

PG_FUNCTION_INFO_V1(Contained_temporal_timestamp);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value is contained in
 * a timestamp
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_temporal_timestamp(PG_FUNCTION_ARGS)
{
  return boxop_temporal_timestamp_ext(fcinfo, &contained_span_span);
}

PG_FUNCTION_INFO_V1(Contained_tstzset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a tstzset is contained in the
 * one of a temporal value overlap
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_tstzset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_tstzset_temporal_ext(fcinfo, &contained_span_span);
}

PG_FUNCTION_INFO_V1(Contained_temporal_tstzset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value is contained in
 * the one of a tstzset
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_temporal_tstzset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_tstzset_ext(fcinfo, &contained_span_span);
}

PG_FUNCTION_INFO_V1(Contained_period_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a period is contained the bounding period of the temporal
 * value
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_period_temporal(PG_FUNCTION_ARGS)
{
  return boxop_period_temporal_ext(fcinfo, &contained_span_span);
}

PG_FUNCTION_INFO_V1(Contained_temporal_period);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value is contained in
 * a period
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_temporal_period(PG_FUNCTION_ARGS)
{
  return boxop_temporal_period_ext(fcinfo, &contained_span_span);
}

PG_FUNCTION_INFO_V1(Contained_periodset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a period set is contained in the
 * one of a temporal value
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_periodset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_periodset_temporal_ext(fcinfo, &contained_span_span);
}

PG_FUNCTION_INFO_V1(Contained_temporal_periodset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value is contained in
 * the one of a period set
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_temporal_periodset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_periodset_ext(fcinfo, &contained_span_span);
}

PG_FUNCTION_INFO_V1(Contained_temporal_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of the first temporal value is contained
 * in the one of the second temporal value.
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_temporal_temporal(PG_FUNCTION_ARGS)
{
  return boxop_temporal_temporal_ext(fcinfo, &contained_span_span);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(Overlaps_timestamp_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a timestamp and the bounding period of a temporal value
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_timestamp_temporal(PG_FUNCTION_ARGS)
{
  return boxop_timestamp_temporal_ext(fcinfo, &overlaps_span_span);
}

PG_FUNCTION_INFO_V1(Overlaps_temporal_timestamp);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value and a timestamp
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_temporal_timestamp(PG_FUNCTION_ARGS)
{
  return boxop_temporal_timestamp_ext(fcinfo, &overlaps_span_span);
}

PG_FUNCTION_INFO_V1(Overlaps_tstzset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period a tstzset and the one
 * of a temporal value overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_tstzset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_tstzset_temporal_ext(fcinfo, &overlaps_span_span);
}

PG_FUNCTION_INFO_V1(Overlaps_temporal_tstzset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a temporal value and of
 * a tstzset overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_temporal_tstzset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_tstzset_ext(fcinfo, &overlaps_span_span);
}

PG_FUNCTION_INFO_V1(Overlaps_period_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a period and the bounding period of a temporal value
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_period_temporal(PG_FUNCTION_ARGS)
{
  return boxop_period_temporal_ext(fcinfo, &overlaps_span_span);
}

PG_FUNCTION_INFO_V1(Overlaps_temporal_period);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value and a period
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_temporal_period(PG_FUNCTION_ARGS)
{
  return boxop_temporal_period_ext(fcinfo, &overlaps_span_span);
}

PG_FUNCTION_INFO_V1(Overlaps_periodset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period a period set and the one of
 * a temporal value overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_periodset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_periodset_temporal_ext(fcinfo, &overlaps_span_span);
}

PG_FUNCTION_INFO_V1(Overlaps_temporal_periodset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a temporal value and of
 * a period set overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_temporal_periodset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_periodset_ext(fcinfo, &overlaps_span_span);
}

PG_FUNCTION_INFO_V1(Overlaps_temporal_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of the temporal values overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_temporal_temporal(PG_FUNCTION_ARGS)
{
  return boxop_temporal_temporal_ext(fcinfo, &overlaps_span_span);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(Same_timestamp_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a timestamp and the bounding period of a temporal value
 * are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_timestamp_temporal(PG_FUNCTION_ARGS)
{
  return boxop_timestamp_temporal_ext(fcinfo, &span_eq);
}

PG_FUNCTION_INFO_V1(Same_temporal_timestamp);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value and a timestamp
 * are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_temporal_timestamp(PG_FUNCTION_ARGS)
{
  return boxop_temporal_timestamp_ext(fcinfo, &span_eq);
}

PG_FUNCTION_INFO_V1(Same_tstzset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a tstzset and of
 * a temporal value are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_tstzset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_tstzset_temporal_ext(fcinfo, &span_eq);
}

PG_FUNCTION_INFO_V1(Same_temporal_tstzset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a temporal value and of
 * a tstzset are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_temporal_tstzset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_tstzset_ext(fcinfo, &span_eq);
}

PG_FUNCTION_INFO_V1(Same_period_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a period and the bounding period of a temporal value
 * are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_period_temporal(PG_FUNCTION_ARGS)
{
  return boxop_period_temporal_ext(fcinfo, &span_eq);
}

PG_FUNCTION_INFO_V1(Same_temporal_period);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value and a period
 * are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_temporal_period(PG_FUNCTION_ARGS)
{
  return boxop_temporal_period_ext(fcinfo, &span_eq);
}

PG_FUNCTION_INFO_V1(Same_periodset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a period set and of
 * a temporal value are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_periodset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_periodset_temporal_ext(fcinfo, &span_eq);
}

PG_FUNCTION_INFO_V1(Same_temporal_periodset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a temporal value and of
 * a period set are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_temporal_periodset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_periodset_ext(fcinfo, &span_eq);
}

PG_FUNCTION_INFO_V1(Same_temporal_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of the temporal values are equal
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_temporal_temporal(PG_FUNCTION_ARGS)
{
  return boxop_temporal_temporal_ext(fcinfo, &span_eq);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(Adjacent_timestamp_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a timestamp and the bounding period of a temporal value
 * are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_timestamp_temporal(PG_FUNCTION_ARGS)
{
  return boxop_timestamp_temporal_ext(fcinfo, &adjacent_span_span);
}

PG_FUNCTION_INFO_V1(Adjacent_temporal_timestamp);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value and a timestamp
 * are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_temporal_timestamp(PG_FUNCTION_ARGS)
{
  return boxop_temporal_timestamp_ext(fcinfo, &adjacent_span_span);
}

PG_FUNCTION_INFO_V1(Adjacent_tstzset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a tstzset and of
 * a temporal value are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_tstzset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_tstzset_temporal_ext(fcinfo, &adjacent_span_span);
}

PG_FUNCTION_INFO_V1(Adjacent_temporal_tstzset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a temporal value and of
 * a tstzset are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_temporal_tstzset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_tstzset_ext(fcinfo, &adjacent_span_span);
}

PG_FUNCTION_INFO_V1(Adjacent_period_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if a period and the bounding period of a temporal value
 * are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_period_temporal(PG_FUNCTION_ARGS)
{
  return boxop_period_temporal_ext(fcinfo, &adjacent_span_span);
}

PG_FUNCTION_INFO_V1(Adjacent_temporal_period);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding period of a temporal value and a period
 * are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_temporal_period(PG_FUNCTION_ARGS)
{
  return boxop_temporal_period_ext(fcinfo, &adjacent_span_span);
}

PG_FUNCTION_INFO_V1(Adjacent_periodset_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a period set and of
 * a temporal value are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_periodset_temporal(PG_FUNCTION_ARGS)
{
  return boxop_periodset_temporal_ext(fcinfo, &adjacent_span_span);
}

PG_FUNCTION_INFO_V1(Adjacent_temporal_periodset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of a temporal value and of
 * a period set are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_temporal_periodset(PG_FUNCTION_ARGS)
{
  return boxop_temporal_periodset_ext(fcinfo, &adjacent_span_span);
}

PG_FUNCTION_INFO_V1(Adjacent_temporal_temporal);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding periods of the temporal values are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_temporal_temporal(PG_FUNCTION_ARGS)
{
  return boxop_temporal_temporal_ext(fcinfo, &adjacent_span_span);
}

/*****************************************************************************
 * Bounding box operators for temporal number types: Generic functions
 *****************************************************************************/

/**
 * @brief Generic bounding box operator for a number and a temporal number
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_number_tnumber_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  Datum value = PG_GETARG_DATUM(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  meosType basetype = oid_type(get_fn_expr_argtype(fcinfo->flinfo, 0));
  bool result = boxop_tnumber_number(temp, value, basetype, func, INVERT);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal number and a number
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_tnumber_number_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Datum value = PG_GETARG_DATUM(1);
  meosType basetype = oid_type(get_fn_expr_argtype(fcinfo->flinfo, 1));
  bool result = boxop_tnumber_number(temp, value, basetype, func, INVERT_NO);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a span and a temporal number
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_numspan_tnumber_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  Span *span = PG_GETARG_SPAN_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_tnumber_numspan(temp, span, func, INVERT);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal number and a span
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_tnumber_numspan_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  Span *span = PG_GETARG_SPAN_P(1);
  bool result = boxop_tnumber_numspan(temp, span, func, INVERT_NO);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a span set and a temporal number
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_numspanset_tnumber_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  SpanSet *ss = PG_GETARG_SPANSET_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_tnumber_numspanset(temp, ss, func, INVERT);
  PG_FREE_IF_COPY(ss, 0);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal number and a span set
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_tnumber_numspanset_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  SpanSet *ss = PG_GETARG_SPANSET_P(1);
  bool result = boxop_tnumber_numspanset(temp, ss, func, INVERT_NO);
  PG_FREE_IF_COPY(temp, 0);
  PG_FREE_IF_COPY(ss, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal box and a temporal number
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_tbox_tnumber_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  TBox *box = PG_GETARG_TBOX_P(0);
  Temporal *temp = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_tnumber_tbox(temp, box, func, INVERT);
  PG_FREE_IF_COPY(temp, 1);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for a temporal number and a temporal box
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_tnumber_tbox_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TBox *box = PG_GETARG_TBOX_P(1);
  bool result = boxop_tnumber_tbox(temp, box, func, INVERT_NO);
  PG_FREE_IF_COPY(temp, 0);
  PG_RETURN_BOOL(result);
}

/**
 * @brief Generic bounding box operator for two temporal numbers
 *
 * @param[in] fcinfo Catalog information about the external function
 * @param[in] func Bounding box function
 */
Datum
boxop_tnumber_tnumber_ext(FunctionCallInfo fcinfo,
  bool (*func)(const TBox *, const TBox *))
{
  Temporal *temp1 = PG_GETARG_TEMPORAL_P(0);
  Temporal *temp2 = PG_GETARG_TEMPORAL_P(1);
  bool result = boxop_tnumber_tnumber(temp1, temp2, func);
  PG_FREE_IF_COPY(temp1, 0);
  PG_FREE_IF_COPY(temp2, 1);
  PG_RETURN_BOOL(result);
}

/*****************************************************************************
 * Bounding box operators for temporal numbers
 *****************************************************************************/

PG_FUNCTION_INFO_V1(Contains_number_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span contains the bounding box of the temporal number
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_number_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_number_tnumber_ext(fcinfo, &contains_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contains_tnumber_number);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number contains the
 * the span
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_tnumber_number(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_number_ext(fcinfo, &contains_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contains_numspan_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span contains the bounding box of the temporal number
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_numspan_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspan_tnumber_ext(fcinfo, &contains_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contains_tnumber_numspan);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number contains the span
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_tnumber_numspan(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspan_ext(fcinfo, &contains_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contains_numspanset_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span set contains the bounding box of the temporal number
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_numspanset_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspan_tnumber_ext(fcinfo, &contains_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contains_tnumber_numspanset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number contains the
 * span set
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_tnumber_numspanset(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspanset_ext(fcinfo, &contains_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contains_tbox_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the temporal box contains the bounding box of the
 * temporal number
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_tbox_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tbox_tnumber_ext(fcinfo, &contains_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contains_tnumber_tbox);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number contains the temporal
 * box
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_tnumber_tbox(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tbox_ext(fcinfo, &contains_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contains_tnumber_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the first temporal number contains the one
 * of the second temporal number
 * @sqlfunc contains_bbox()
 * @sqlop @p \@>
 */
PGDLLEXPORT Datum
Contains_tnumber_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tnumber_ext(fcinfo, &contains_tbox_tbox);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(Contained_number_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span is contained in the bounding box of the temporal
 * number
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_number_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_number_tnumber_ext(fcinfo, &contained_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contained_tnumber_number);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number is contained in the
 * the span
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_tnumber_number(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_number_ext(fcinfo, &contained_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contained_numspan_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span is contained in the bounding box of the temporal
 * number
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_numspan_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspan_tnumber_ext(fcinfo, &contained_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contained_tnumber_numspan);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number is contained in
 * the span
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_tnumber_numspan(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspan_ext(fcinfo, &contained_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contained_numspanset_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span set is contained in the bounding box of the
 * temporal number
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_numspanset_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspanset_tnumber_ext(fcinfo, &contained_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contained_tnumber_numspanset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number is contained
 * in the span set
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_tnumber_numspanset(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspanset_ext(fcinfo, &contained_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contained_tbox_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the temporal box is contained in the bounding box
 * of the temporal number
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_tbox_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tbox_tnumber_ext(fcinfo, &contained_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contained_tnumber_tbox);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number is contained in
 * the temporal box
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_tnumber_tbox(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tbox_ext(fcinfo, &contained_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Contained_tnumber_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the first temporal number is contained
 * in the one of the second temporal number
 * @sqlfunc contained_bbox()
 * @sqlop @p <@
 */
PGDLLEXPORT Datum
Contained_tnumber_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tnumber_ext(fcinfo, &contained_tbox_tbox);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(Overlaps_number_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the number and the bounding box of the temporal number
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_number_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_number_tnumber_ext(fcinfo, &overlaps_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Overlaps_tnumber_number);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the number
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_tnumber_number(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_number_ext(fcinfo, &overlaps_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Overlaps_numspan_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span and the bounding box of the temporal number
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_numspan_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspan_tnumber_ext(fcinfo, &overlaps_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Overlaps_tnumber_numspan);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the span
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_tnumber_numspan(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspan_ext(fcinfo, &overlaps_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Overlaps_numspanset_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span set and the bounding box of the temporal
 * number overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_numspanset_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspanset_tnumber_ext(fcinfo, &overlaps_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Overlaps_tnumber_numspanset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the span
 * set overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_tnumber_numspanset(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspanset_ext(fcinfo, &overlaps_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Overlaps_tbox_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the temporal box and the bounding box of the temporal number
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_tbox_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tbox_tnumber_ext(fcinfo, &overlaps_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Overlaps_tnumber_tbox);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the temporal box
 * overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_tnumber_tbox(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tbox_ext(fcinfo, &overlaps_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Overlaps_tnumber_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding boxes of the temporal numbers overlap
 * @sqlfunc overlaps_bbox()
 * @sqlop @p &&
 */
PGDLLEXPORT Datum
Overlaps_tnumber_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tnumber_ext(fcinfo, &overlaps_tbox_tbox);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(Same_number_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the number and the bounding box of the temporal number are
 * equal on the common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_number_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_number_tnumber_ext(fcinfo, &same_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Same_tnumber_number);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the number are
 * equal on the common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_tnumber_number(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_number_ext(fcinfo, &same_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Same_numspan_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span and the bounding box of the temporal number are
 * equal on the common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_numspan_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspan_tnumber_ext(fcinfo, &same_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Same_tnumber_numspan);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the span are
 * equal on the common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_tnumber_numspan(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspan_ext(fcinfo, &same_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Same_numspanset_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span set and the bounding box of the temporal
 * number are equal on the common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_numspanset_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspanset_tnumber_ext(fcinfo, &same_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Same_tnumber_numspanset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the span
 * set are equal on the common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_tnumber_numspanset(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspanset_ext(fcinfo, &same_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Same_tbox_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the temporal box and the bounding box of the temporal number
 * are equal in the common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_tbox_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tbox_tnumber_ext(fcinfo, &same_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Same_tnumber_tbox);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the temporal box
 * are equal in the common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_tnumber_tbox(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tbox_ext(fcinfo, &same_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Same_tnumber_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding boxes of the temporal numbers are equal in the
 * common dimensions
 * @sqlfunc same_bbox()
 * @sqlop @p ~=
 */
PGDLLEXPORT Datum
Same_tnumber_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tnumber_ext(fcinfo, &same_tbox_tbox);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(Adjacent_number_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the number and the bounding box of the temporal number are
 * adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_number_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_number_tnumber_ext(fcinfo, &adjacent_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Adjacent_tnumber_number);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the number are
 * adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_tnumber_number(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_number_ext(fcinfo, &adjacent_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Adjacent_numspan_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span and the bounding box of the temporal number are
 * adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_numspan_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspan_tnumber_ext(fcinfo, &adjacent_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Adjacent_tnumber_numspan);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the span are
 * adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_tnumber_numspan(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspan_ext(fcinfo, &adjacent_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Adjacent_numspanset_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the span set and the bounding box of the temporal
 * number are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_numspanset_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_numspanset_tnumber_ext(fcinfo, &adjacent_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Adjacent_tnumber_numspanset);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the span
 * set are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_tnumber_numspanset(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_numspanset_ext(fcinfo, &adjacent_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Adjacent_tbox_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the temporal box and the bounding box of the temporal number
 * are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_tbox_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tbox_tnumber_ext(fcinfo, &adjacent_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Adjacent_tnumber_tbox);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding box of the temporal number and the temporal box
 * are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_tnumber_tbox(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tbox_ext(fcinfo, &adjacent_tbox_tbox);
}

PG_FUNCTION_INFO_V1(Adjacent_tnumber_tnumber);
/**
 * @ingroup mobilitydb_temporal_topo
 * @brief Return true if the bounding boxes of the temporal numbers are adjacent
 * @sqlfunc adjacent_bbox()
 * @sqlop @p -|-
 */
PGDLLEXPORT Datum
Adjacent_tnumber_tnumber(PG_FUNCTION_ARGS)
{
  return boxop_tnumber_tnumber_ext(fcinfo, &adjacent_tbox_tbox);
}

/*****************************************************************************/
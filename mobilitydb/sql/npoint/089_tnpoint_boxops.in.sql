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
 * tnpoint_boxops.sql
 * Bounding box operators for temporal network points.
 */

/*****************************************************************************/

CREATE FUNCTION tnpoint_sel(internal, oid, internal, integer)
  RETURNS float
  AS 'MODULE_PATHNAME', 'Tnpoint_sel'
  LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION tnpoint_joinsel(internal, oid, internal, smallint, internal)
  RETURNS float
  AS 'MODULE_PATHNAME', 'Tnpoint_joinsel'
  LANGUAGE C IMMUTABLE STRICT;

/*****************************************************************************
 * Temporal npoint to stbox
 *****************************************************************************/

CREATE FUNCTION stbox(npoint)
  RETURNS stbox
  AS 'MODULE_PATHNAME', 'Npoint_to_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION stbox(nsegment)
  RETURNS stbox
  AS 'MODULE_PATHNAME', 'Nsegment_to_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION stbox(npoint, timestamptz)
  RETURNS stbox
  AS 'MODULE_PATHNAME', 'Npoint_timestamp_to_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION stbox(npoint, tstzspan)
  RETURNS stbox
  AS 'MODULE_PATHNAME', 'Npoint_period_to_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION stbox(tnpoint)
  RETURNS stbox
  AS 'MODULE_PATHNAME', 'Tnpoint_to_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (npoint AS stbox) WITH FUNCTION stbox(npoint);
CREATE CAST (nsegment AS stbox) WITH FUNCTION stbox(nsegment);
CREATE CAST (tnpoint AS stbox) WITH FUNCTION stbox(tnpoint);

/*****************************************************************************/

CREATE FUNCTION expandSpatial(tnpoint, float)
  RETURNS stbox
  AS 'MODULE_PATHNAME', 'Tpoint_expand_spatial'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*****************************************************************************
 * Contains
 *****************************************************************************/

CREATE FUNCTION contains_bbox(timestamptz, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_timestamp_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tnpoint, timestamptz)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_temporal_timestamp'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tstzset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_tstzset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tnpoint, tstzset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_temporal_tstzset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tstzspan, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_period_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tnpoint, tstzspan)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_temporal_period'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tstzspanset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_periodset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tnpoint, tstzspanset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_temporal_periodset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = timestamptz, RIGHTARG = tnpoint,
  COMMUTATOR = <@,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tnpoint, RIGHTARG = timestamptz,
  COMMUTATOR = <@,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tstzset, RIGHTARG = tnpoint,
  COMMUTATOR = <@,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzset,
  COMMUTATOR = <@,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tstzspan, RIGHTARG = tnpoint,
  COMMUTATOR = <@,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspan,
  COMMUTATOR = <@,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tstzspanset, RIGHTARG = tnpoint,
  COMMUTATOR = <@,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspanset,
  COMMUTATOR = <@,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);

/*****************************************************************************/

CREATE FUNCTION contains_bbox(geometry, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_bbox_geo_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(stbox, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_stbox_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = geometry, RIGHTARG = tnpoint,
  COMMUTATOR = <@,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = stbox, RIGHTARG = tnpoint,
  COMMUTATOR = <@,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = npoint, RIGHTARG = tnpoint,
  COMMUTATOR = <@,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

CREATE FUNCTION contains_bbox(tnpoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_tnpoint_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tnpoint, stbox)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_tnpoint_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contains_bbox(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contains_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tnpoint, RIGHTARG = geometry,
  COMMUTATOR = <@,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tnpoint, RIGHTARG = stbox,
  COMMUTATOR = <@,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tnpoint, RIGHTARG = npoint,
  COMMUTATOR = <@,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR @> (
  PROCEDURE = contains_bbox,
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  COMMUTATOR = <@,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

/*****************************************************************************
 * Contained
 *****************************************************************************/

CREATE FUNCTION contained_bbox(timestamptz, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_timestamp_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tnpoint, timestamptz)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_temporal_timestamp'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tstzset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_tstzset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tnpoint, tstzset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_temporal_tstzset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tstzspan, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_period_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tnpoint, tstzspan)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_temporal_period'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tstzspanset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_periodset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tnpoint, tstzspanset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_temporal_periodset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = timestamptz, RIGHTARG = tnpoint,
  COMMUTATOR = @>,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tnpoint, RIGHTARG = timestamptz,
  COMMUTATOR = @>,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tstzset, RIGHTARG = tnpoint,
  COMMUTATOR = @>,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzset,
  COMMUTATOR = @>,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tstzspan, RIGHTARG = tnpoint,
  COMMUTATOR = @>,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspan,
  COMMUTATOR = @>,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tstzspanset, RIGHTARG = tnpoint,
  COMMUTATOR = @>,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspanset,
  COMMUTATOR = @>,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);

/*****************************************************************************/

CREATE FUNCTION contained_bbox(geometry, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_geo_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(stbox, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_stbox_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = geometry, RIGHTARG = tnpoint,
  COMMUTATOR = @>,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = stbox, RIGHTARG = tnpoint,
  COMMUTATOR = @>,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = npoint, RIGHTARG = tnpoint,
  COMMUTATOR = @>,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

CREATE FUNCTION contained_bbox(tnpoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_tnpoint_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tnpoint, stbox)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_tnpoint_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION contained_bbox(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Contained_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tnpoint, RIGHTARG = geometry,
  COMMUTATOR = @>,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tnpoint, RIGHTARG = stbox,
  COMMUTATOR = @>,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tnpoint, RIGHTARG = npoint,
  COMMUTATOR = @>,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR <@ (
  PROCEDURE = contained_bbox,
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  COMMUTATOR = @>,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

/*****************************************************************************
 * Overlaps
 *****************************************************************************/

CREATE FUNCTION overlaps_bbox(timestamptz, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_timestamp_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tnpoint, timestamptz)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_temporal_timestamp'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tstzset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_tstzset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tnpoint, tstzset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_temporal_tstzset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tstzspan, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_period_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tnpoint, tstzspan)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_temporal_period'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tstzspanset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_periodset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tnpoint, tstzspanset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_temporal_periodset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = timestamptz, RIGHTARG = tnpoint,
  COMMUTATOR = &&,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tnpoint, RIGHTARG = timestamptz,
  COMMUTATOR = &&,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tstzset, RIGHTARG = tnpoint,
  COMMUTATOR = &&,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzset,
  COMMUTATOR = &&,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tstzspan, RIGHTARG = tnpoint,
  COMMUTATOR = &&,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspan,
  COMMUTATOR = &&,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tstzspanset, RIGHTARG = tnpoint,
  COMMUTATOR = &&,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspanset,
  COMMUTATOR = &&,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);

/*****************************************************************************/

CREATE FUNCTION overlaps_bbox(geometry, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_geo_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(stbox, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_stbox_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = geometry, RIGHTARG = tnpoint,
  COMMUTATOR = &&,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = stbox, RIGHTARG = tnpoint,
  COMMUTATOR = &&,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = npoint, RIGHTARG = tnpoint,
  COMMUTATOR = &&,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

CREATE FUNCTION overlaps_bbox(tnpoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_tnpoint_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tnpoint, stbox)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_tnpoint_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION overlaps_bbox(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Overlaps_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tnpoint, RIGHTARG = geometry,
  COMMUTATOR = &&,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tnpoint, RIGHTARG = stbox,
  COMMUTATOR = &&,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tnpoint, RIGHTARG = npoint,
  COMMUTATOR = &&,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR && (
  PROCEDURE = overlaps_bbox,
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  COMMUTATOR = &&,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

/*****************************************************************************
 * Same
 *****************************************************************************/

CREATE FUNCTION same_bbox(timestamptz, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_timestamp_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tnpoint, timestamptz)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_temporal_timestamp'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tstzset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_tstzset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tnpoint, tstzset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_temporal_tstzset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tstzspan, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_period_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tnpoint, tstzspan)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_temporal_period'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tstzspanset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_periodset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tnpoint, tstzspanset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_temporal_periodset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = timestamptz, RIGHTARG = tnpoint,
  COMMUTATOR = ~=,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tnpoint, RIGHTARG = timestamptz,
  COMMUTATOR = ~=,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tstzset, RIGHTARG = tnpoint,
  COMMUTATOR = ~=,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzset,
  COMMUTATOR = ~=,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tstzspan, RIGHTARG = tnpoint,
  COMMUTATOR = ~=,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspan,
  COMMUTATOR = ~=,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tstzspanset, RIGHTARG = tnpoint,
  COMMUTATOR = ~=,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspanset,
  COMMUTATOR = ~=,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);

/*****************************************************************************/

CREATE FUNCTION same_bbox(geometry, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_geo_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(stbox, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_stbox_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = geometry, RIGHTARG = tnpoint,
  COMMUTATOR = ~=,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = stbox, RIGHTARG = tnpoint,
  COMMUTATOR = ~=,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = npoint, RIGHTARG = tnpoint,
  COMMUTATOR = ~=,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

CREATE FUNCTION same_bbox(tnpoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_tnpoint_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tnpoint, stbox)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_tnpoint_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION same_bbox(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Same_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tnpoint, RIGHTARG = geometry,
  COMMUTATOR = ~=,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tnpoint, RIGHTARG = stbox,
  COMMUTATOR = ~=,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tnpoint, RIGHTARG = npoint,
  COMMUTATOR = ~=,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR ~= (
  PROCEDURE = same_bbox,
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  COMMUTATOR = ~=,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

/*****************************************************************************
 * adjacent
 *****************************************************************************/

CREATE FUNCTION adjacent_bbox(timestamptz, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_timestamp_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tnpoint, timestamptz)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_temporal_timestamp'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tstzset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_tstzset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tnpoint, tstzset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_temporal_tstzset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tstzspan, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_period_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tnpoint, tstzspan)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_temporal_period'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tstzspanset, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_periodset_temporal'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tnpoint, tstzspanset)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_temporal_periodset'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = timestamptz, RIGHTARG = tnpoint,
  COMMUTATOR = -|-,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tnpoint, RIGHTARG = timestamptz,
  COMMUTATOR = -|-,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tstzset, RIGHTARG = tnpoint,
  COMMUTATOR = -|-,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzset,
  COMMUTATOR = -|-,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tstzspan, RIGHTARG = tnpoint,
  COMMUTATOR = -|-,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspan,
  COMMUTATOR = -|-,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tstzspanset, RIGHTARG = tnpoint,
  COMMUTATOR = -|-,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tnpoint, RIGHTARG = tstzspanset,
  COMMUTATOR = -|-,
  RESTRICT = temporal_sel, JOIN = temporal_joinsel
);

/*****************************************************************************/

CREATE FUNCTION adjacent_bbox(geometry, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_geo_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(stbox, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_stbox_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(npoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_npoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = geometry, RIGHTARG = tnpoint,
  COMMUTATOR = -|-,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = stbox, RIGHTARG = tnpoint,
  COMMUTATOR = -|-,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = npoint, RIGHTARG = tnpoint,
  COMMUTATOR = -|-,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

CREATE FUNCTION adjacent_bbox(tnpoint, geometry)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_tnpoint_geo'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tnpoint, stbox)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_tnpoint_stbox'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tnpoint, npoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_tnpoint_npoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION adjacent_bbox(tnpoint, tnpoint)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'Adjacent_tnpoint_tnpoint'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tnpoint, RIGHTARG = geometry,
  COMMUTATOR = -|-,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tnpoint, RIGHTARG = stbox,
  COMMUTATOR = -|-,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tnpoint, RIGHTARG = npoint,
  COMMUTATOR = -|-,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);
CREATE OPERATOR -|- (
  PROCEDURE = adjacent_bbox,
  LEFTARG = tnpoint, RIGHTARG = tnpoint,
  COMMUTATOR = -|-,
  RESTRICT = tnpoint_sel, JOIN = tnpoint_joinsel
);

/*****************************************************************************/
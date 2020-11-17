/*****************************************************************************
 *
 * temporal_boolops.sql
 *    Temporal Boolean function and operators.
 *
 * Copyright (c) 2020, Université libre de Bruxelles and MobilityDB contributors
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for any purpose, without fee, and without a written agreement is hereby
 * granted, provided that the above copyright notice and this paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL UNIVERSITE LIBRE DE BRUXELLES BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
 * PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF UNIVERSITE LIBRE DE BRUXELLES HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * UNIVERSITE LIBRE DE BRUXELLES SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND UNIVERSITE LIBRE DE BRUXELLES HAS NO OBLIGATIONS TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS. 
 *
 *****************************************************************************/

/*****************************************************************************
 * Temporal and
 *****************************************************************************/

CREATE FUNCTION temporal_and(boolean, tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'tand_bool_tbool'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal_and(tbool, boolean)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'tand_tbool_bool'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal_and(tbool, tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'tand_tbool_tbool'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR & (
  PROCEDURE = temporal_and,
  LEFTARG = boolean, RIGHTARG = tbool,
  COMMUTATOR = &
);
CREATE OPERATOR & (
  PROCEDURE = temporal_and,
  LEFTARG = tbool, RIGHTARG = boolean,
  COMMUTATOR = &
);
CREATE OPERATOR & (
  PROCEDURE = temporal_and,
  LEFTARG = tbool, RIGHTARG = tbool,
  COMMUTATOR = &
);

/*****************************************************************************
 * Temporal or
 *****************************************************************************/

CREATE FUNCTION temporal_or(boolean, tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'tor_bool_tbool'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal_or(tbool, boolean)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'tor_tbool_bool'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION temporal_or(tbool, tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'tor_tbool_tbool'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR | (
  PROCEDURE = temporal_or,
  LEFTARG = boolean, RIGHTARG = tbool,
  COMMUTATOR = |
);
CREATE OPERATOR | (
  PROCEDURE = temporal_or,
  LEFTARG = tbool, RIGHTARG = boolean,
  COMMUTATOR = |
);
CREATE OPERATOR | (
  PROCEDURE = temporal_or,
  LEFTARG = tbool, RIGHTARG = tbool,
  COMMUTATOR = |
);

/*****************************************************************************
 * Temporal not
 *****************************************************************************/

CREATE FUNCTION temporal_not(tbool)
  RETURNS tbool
  AS 'MODULE_PATHNAME', 'tnot_tbool'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ~ (
  PROCEDURE = temporal_not, RIGHTARG = tbool
);

/*****************************************************************************/
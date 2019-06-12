/*****************************************************************************
 *
 * GeomAggFuncs.c
 *	  Aggregate functions for temporal points.
 *
 * The only function currently provided is temporal centroid.
 *
 * Portions Copyright (c) 2019, Esteban Zimanyi, Arthur Lesuisse,
 *	  Universite Libre de Bruxelles
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *****************************************************************************/

#include "TemporalPoint.h"

/*****************************************************************************
 * Generic functions
 *****************************************************************************/

struct GeoAggregateState
{
	int32_t srid;
	bool hasz;
};

static void geoaggstate_check(SkipList *state, int32_t srid, bool hasz)
{
	if(! state)
		return;
	struct GeoAggregateState *extra = state->extra;
	if (extra && extra->srid != srid)
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Geometries need to have the same SRID for temporal aggregation")));
	if (extra && extra->hasz != hasz)
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Geometries need to have the same dimensionality for temporal aggregation")));
}

static void geoaggstate_check_as(SkipList *state1, SkipList *state2)
{
	struct GeoAggregateState *extra2 = state2->extra;
	if (extra2)
		geoaggstate_check(state1, extra2->srid, extra2->hasz);
}

static void geoaggstate_check_t(SkipList *state, Temporal *t)
{
	geoaggstate_check(state, tpoint_srid_internal(t), MOBDB_FLAGS_GET_Z(t->flags) != 0);
}

/*****************************************************************************/

/*
 * Transform a temporal point type into a temporal double3/double4 type for 
 * performing centroid aggregation 
 */
static TemporalInst *
tpointinst_transform_tcentroid(TemporalInst *inst)
{
	TemporalInst *result;
	if (MOBDB_FLAGS_GET_Z(inst->flags))
	{
		POINT3DZ point = datum_get_point3dz(temporalinst_value(inst));
		double4 *dvalue = double4_construct(point.x, point.y, point.z, 1);
		result = temporalinst_make(PointerGetDatum(dvalue), inst->t,
			type_oid(T_DOUBLE4));
	}
	else 
	{
		POINT2D point = datum_get_point2d(temporalinst_value(inst));
		double3 *dvalue = double3_construct(point.x, point.y, 1);
		result = temporalinst_make(PointerGetDatum(dvalue), inst->t,
			type_oid(T_DOUBLE3));
	}
	return result;
}

static TemporalInst **
tpointi_transform_tcentroid(TemporalI *ti)
{
	TemporalInst **result = palloc(sizeof(TemporalInst *) * ti->count);
	for (int i = 0; i < ti->count; i++)
	{
		TemporalInst *inst = temporali_inst_n(ti, i);
		result[i] = tpointinst_transform_tcentroid(inst);
	}
	return result;
}

static TemporalSeq *
tpointseq_transform_tcentroid(TemporalSeq *seq)
{
	TemporalInst **instants = palloc(sizeof(TemporalInst *) * seq->count);
	for (int i = 0; i < seq->count; i++)
	{
		TemporalInst *inst = temporalseq_inst_n(seq, i);
		instants[i] = tpointinst_transform_tcentroid(inst);
	}
	TemporalSeq *result = temporalseq_from_temporalinstarr(instants, 
		seq->count, seq->period.lower_inc, seq->period.upper_inc, false);
	
	for (int i = 0; i < seq->count; i++)
		pfree(instants[i]);
	pfree(instants);
		
	return result;
}

static TemporalSeq **
tpoints_transform_tcentroid(TemporalS *ts)
{
	TemporalSeq **result = palloc(sizeof(TemporalSeq *) * ts->count);
	for (int i = 0; i < ts->count; i++)
	{
		TemporalSeq *seq = temporals_seq_n(ts, i);
		result[i] = tpointseq_transform_tcentroid(seq);
	}
	return result;
}

/*****************************************************************************
 * Aggregate functions
 *****************************************************************************/

static SkipList *
skiplist_make_tcentroid(FunctionCallInfo fcinfo, Temporal *temp)
{
    SkipList *result = NULL;
    if (temp->duration == TEMPORALINST) 
	{
        TemporalInst *inst = (TemporalInst*) temp;
        TemporalInst *newinst = tpointinst_transform_tcentroid(inst);
        result = skiplist_make(fcinfo, (Temporal **)&newinst, 1);
        pfree(newinst);
    }
	else if (temp->duration == TEMPORALI)
	{
        TemporalI *ti = (TemporalI*) temp;
        TemporalInst **instants = tpointi_transform_tcentroid(ti);
        result = skiplist_make(fcinfo, (Temporal **)instants, ti->count);
        for (int i = 0; i < ti->count; i++)
            pfree(instants[i]);
        pfree(instants);
    } 
	else if (temp->duration == TEMPORALSEQ)
	{
        TemporalSeq *seq = (TemporalSeq*) temp;
        TemporalSeq *newseq = tpointseq_transform_tcentroid(seq);
        result = skiplist_make(fcinfo, (Temporal **)&newseq, 1);
        pfree(newseq);
    }
	else if (temp->duration == TEMPORALS)
	{
        TemporalS *ts = (TemporalS*) temp;
        TemporalSeq **sequences = tpoints_transform_tcentroid(ts);
        result = skiplist_make(fcinfo, (Temporal **)sequences, ts->count);
        for (int i = 0; i < ts->count; i++)
            pfree(sequences[i]);
        pfree(sequences);
    }
    assert(result != NULL);

    struct GeoAggregateState extra =
	{
        .srid = tpoint_srid_internal(temp),
        .hasz = MOBDB_FLAGS_GET_Z(temp->flags) != 0
    };

    aggstate_set_extra(fcinfo, result, &extra, sizeof(struct GeoAggregateState));

    return result;
}

PG_FUNCTION_INFO_V1(tpoint_tcentroid_transfn);

PGDLLEXPORT Datum
tpoint_tcentroid_transfn(PG_FUNCTION_ARGS)
{
	SkipList *state = PG_ARGISNULL(0) ? NULL : 
		(SkipList *) PG_GETARG_POINTER(0);
    if (PG_ARGISNULL(1)) {
        if (state)
            PG_RETURN_POINTER(state);
        else
            PG_RETURN_NULL();
    }
	Temporal *temp = PG_GETARG_TEMPORAL(1);

	geoaggstate_check_t(state, temp);
	Datum (*func)(Datum, Datum) = MOBDB_FLAGS_GET_Z(temp->flags) ?
		&datum_sum_double4 : &datum_sum_double3;

    SkipList *state2 = skiplist_make_tcentroid(fcinfo, temp);
    SkipList *result = temporal_tagg_combinefn(fcinfo, state, state2, func, false);

    //aggstate_move_extra(result, state2);

    if(result != state2)
        pfree(state2);

    PG_FREE_IF_COPY(temp, 1);
	PG_RETURN_POINTER(result);
}

/*****************************************************************************/
/* Centroid combine function */

PG_FUNCTION_INFO_V1(tpoint_tcentroid_combinefn);

PGDLLEXPORT Datum
tpoint_tcentroid_combinefn(PG_FUNCTION_ARGS)
{
	SkipList *state1 = PG_ARGISNULL(0) ? NULL : 
		(SkipList *) PG_GETARG_POINTER(0);
	SkipList *state2 = PG_ARGISNULL(1) ? NULL :
		(SkipList *) PG_GETARG_POINTER(1);
	geoaggstate_check_as(state1, state2);
	bool hasz = MOBDB_FLAGS_GET_Z(skiplist_headval(state1 ? state1 : state2)->flags);
	Datum (*func)(Datum, Datum) = hasz ?
		&datum_sum_double4 : &datum_sum_double3;
	SkipList *result = temporal_tagg_combinefn(fcinfo, state1, state2, 
		func, false);

	PG_RETURN_POINTER(result);
}

/*****************************************************************************/

/* Centroid final function */

TemporalI *
tpointinst_tcentroid_finalfn(TemporalInst **instants, int count)
{
	TemporalInst **newinstants = palloc(sizeof(TemporalInst *) * count);
	for (int i = 0; i < count; i++)
	{
		TemporalInst *inst = instants[i];
		Datum value = 0;
		assert(inst->valuetypid == type_oid(T_DOUBLE4) || 
			inst->valuetypid == type_oid(T_DOUBLE3));
		if (inst->valuetypid == type_oid(T_DOUBLE4))
		{
			double4 *value4 = (double4 *)DatumGetPointer(temporalinst_value(inst));
			double valuea = value4->a / value4->d;
			double valueb = value4->b / value4->d;
			double valuec = value4->c / value4->d;
			value = call_function3(LWGEOM_makepoint, Float8GetDatum(valuea),
				Float8GetDatum(valueb), Float8GetDatum(valuec));
		}
		else if (inst->valuetypid == type_oid(T_DOUBLE3))
		{
			double3 *value3 = (double3 *)DatumGetPointer(temporalinst_value(inst));
			double valuea = value3->a / value3->c;
			double valueb = value3->b / value3->c;
			value = call_function2(LWGEOM_makepoint, Float8GetDatum(valuea),
				Float8GetDatum(valueb));
		}
		newinstants[i] = temporalinst_make(value, inst->t, type_oid(T_GEOMETRY));
		pfree(DatumGetPointer(value));
	}
	TemporalI *result = temporali_from_temporalinstarr(newinstants, count);

	for (int i = 0; i < count; i++)
		pfree(newinstants[i]);
	pfree(newinstants);
	
	return result;
}

TemporalS *
tpointseq_tcentroid_finalfn(TemporalSeq **sequences, int count)
{
	TemporalSeq **newsequences = palloc(sizeof(TemporalSeq *) * count);
	for (int i = 0; i < count; i++)
	{
		TemporalSeq *seq = sequences[i];
		TemporalInst **instants = palloc(sizeof(TemporalInst *) * seq->count);
		for (int j = 0; j < seq->count; j++)
		{
			TemporalInst *inst = temporalseq_inst_n(seq, j);
			Datum value = 0;
			assert(inst->valuetypid == type_oid(T_DOUBLE4) || 
				inst->valuetypid == type_oid(T_DOUBLE3));
			if (inst->valuetypid == type_oid(T_DOUBLE4))
			{
				double4 *value4 = (double4 *)DatumGetPointer(temporalinst_value(inst));
				double valuea = value4->a / value4->d;
				double valueb = value4->b / value4->d;
				double valuec = value4->c / value4->d;
				value = call_function3(LWGEOM_makepoint, Float8GetDatum(valuea),
					Float8GetDatum(valueb), Float8GetDatum(valuec));
			}
			else if (inst->valuetypid == type_oid(T_DOUBLE3))
			{
				double3 *value3 = (double3 *)DatumGetPointer(temporalinst_value(inst));
				double valuea = value3->a / value3->c;
				double valueb = value3->b / value3->c;
				value = call_function2(LWGEOM_makepoint, Float8GetDatum(valuea),
					Float8GetDatum(valueb));
			}
			instants[j] = temporalinst_make(value, inst->t, type_oid(T_GEOMETRY));
			pfree(DatumGetPointer(value));
		}
		newsequences[i] = temporalseq_from_temporalinstarr(instants, 
			seq->count, seq->period.lower_inc, seq->period.upper_inc, true);
		for (int j = 0; j < seq->count; j++)
			pfree(instants[j]);
		pfree(instants);
	}
	TemporalS *result = temporals_from_temporalseqarr(newsequences, count, true);

	for (int i = 0; i < count; i++)
		pfree(newsequences[i]);
	pfree(newsequences);
	
	return result;
}

PG_FUNCTION_INFO_V1(tpoint_tcentroid_finalfn);

PGDLLEXPORT Datum
tpoint_tcentroid_finalfn(PG_FUNCTION_ARGS)
{
	/* The final function is strict, we do not need to test for null values */
	SkipList *state = (SkipList *) PG_GETARG_POINTER(0);
	if (state->length == 0)
		PG_RETURN_NULL();

	Temporal **values = skiplist_values(state);
	Temporal *result = NULL;
	assert(values[0]->duration == TEMPORALINST ||
		values[0]->duration == TEMPORALSEQ);
	if (values[0]->duration == TEMPORALINST)
		result = (Temporal *)tpointinst_tcentroid_finalfn(
			(TemporalInst **)values, state->length);
	else if (values[0]->duration == TEMPORALSEQ)
		result = (Temporal *)tpointseq_tcentroid_finalfn(
			(TemporalSeq **)values, state->length);

	int32_t srid = ((struct GeoAggregateState*) state->extra)->srid;
	Temporal *sridresult = tpoint_set_srid_internal(result, srid);
	pfree(result);

	PG_RETURN_POINTER(sridresult);
}

/*****************************************************************************/

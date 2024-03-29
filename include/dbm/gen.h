/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : gen.h (dbm)
 * C header.
 *
 * Generation of DBMs and points.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: gen.h,v 1.1 2005/04/22 15:20:10 adavid Exp $
 *
 **********************************************************************/

#ifndef INCLUDE_DBM_GEN_H
#define INCLUDE_DBM_GEN_H

#include "dbm/constraints.h"

#include <stddef.h>  // size_t

#ifdef __cplusplus
extern "C" {
#endif

/** Generate a random closed and non empty DBM.
 * @param dbm: where to write the DBM.
 * @param dim: dimension.
 * @param range: approximate range for the values.
 * @return true if it is a non trivial one (sometimes
 * the generation may fail and fall back to a trivial
 * DBM). A trivial DBM is created by dbm_init.
 * @pre dbm is a raw_t[dim*dim]
 */
bool dbm_generate(raw_t* dbm, cindex_t dim, raw_t range);

/** Generate a random closed and non empty DBM
 * that satisfy a number of constraints.
 * @param dbm: where to write the DBM.
 * @param dim: dimension.
 * @param range: approximate range for the values.
 * @param constraints,n: the n constraints the DBM
 * has to satisfy.
 * @pre constraints[n] do not result in an empty DBM
 * dbm is a raw_t[dim*dim], dim > 0
 * @post the result is non empty
 * @return true if generation succeeded, false if it failed.
 */
bool dbm_generateConstrained(raw_t* dbm, cindex_t dim, raw_t range, const constraint_t* constraints, size_t n);

/** Constrain randomly an already constrained DBM.
 * @param dbm,dim: DBM of dimension dim to constrain.
 * @param range: maximal range for the generated valued.
 * @param bitMatrix: bit matrix marking the constraints that
 * should be kept.
 * @pre dbm is closed and non empty, dim > 0.
 * @post dbm is closed and non empty.
 */
void dbm_generatePreConstrained(raw_t* dbm, cindex_t dim, raw_t range, const uint32_t* bitMatrix);

/** Generate 2nd DBM argument for intersection/substraction
 * with a first DBM.
 * @param dbm,dim: first DBM of dimension dim to generate
 * a second DBM argument from.
 * @param arg: where to generate a DBM of dimension dim
 * to test for intersection or substraction.
 * @pre dbm closed and not empty, arg is a raw_t[dim*dim]
 * @post arg is closed and not empty
 */
void dbm_generateArgDBM(raw_t* arg, const raw_t* dbm, cindex_t dim);

/** Generate a superset DBM.
 * @param src: the original DBM.
 * @param dst: where to write superset.
 * @param dim: dimension.
 * @pre
 * - src and dst are raw_t[dim*dim]
 * - src is non empty and closed
 * @post
 * - dst is non empty and closed
 * - dst >= src
 */
void dbm_generateSuperset(raw_t* dst, const raw_t* src, cindex_t dim);

/** Generate a subset DBM.
 * @param src: the original DBM.
 * @param dst: where to write subset.
 * @param dim: dimension.
 * @pre
 * - src and dst are raw_t[dim*dim]
 * - src is non empty and closed
 * @post
 * - dst is non empty and closed
 * - dst >= src
 * @return true if the subset is strict
 */
bool dbm_generateSubset(raw_t* dst, const raw_t* src, cindex_t dim);

/** Generate a random discrete point that belongs to the zone.
 * @param pt: memory where to write the point (x0,x1,x2..)
 * @param dbm: DBM.
 * @param dim: dimension.
 * @pre
 * - dbm is a raw_t[dim*dim] and ptr is a int32_t[dim]
 * - dbm is closed and non empty, dim > 0
 * @return true if generated point is included in the DBM.
 */
bool dbm_generatePoint(int32_t* pt, const raw_t* dbm, cindex_t dim);

/** Generate a random real point that belongs to the zone.
 * Always succeeds if zone is not empty (pre-condition).
 * @param pt: memory where to write the point (x0,x1,x2..)
 * @param dbm: DBM.
 * @param dim: dimension.
 * @pre
 * - dbm is a raw_t[dim*dim] and ptr is a int32_t[dim]
 * - dbm is closed and non empty, dim > 0
 * @return true if generation succeeded
 * @post pt is valid only if the generation succeeded
 */
bool dbm_generateRealPoint(double* pt, const raw_t* dbm, cindex_t dim);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DBM_GEN_H */

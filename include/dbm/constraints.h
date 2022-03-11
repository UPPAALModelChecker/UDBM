/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : constraints.h (dbm)
 * C header.
 *
 * Definition of type, constants, and basic operations for constraints.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * v 1.2 reviewed.
 * $Id: constraints.h,v 1.20 2005/09/15 12:39:22 adavid Exp $
 *
 *********************************************************************/

#ifndef INCLUDE_DBM_CONSTRAINTS_H
#define INCLUDE_DBM_CONSTRAINTS_H

#include "base/inttypes.h"

#include <assert.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/** To distinguish normal integers and those
 * representing constraints. "raw" is used
 * because it represents an encoding and this
 * is the raw representation of it.
 */
typedef int32_t raw_t;

/** Basic clock constraint representation.
 * xi-xj <= value (with the special encoding)
 */
typedef struct
{
    cindex_t i, j; /**< indices for xi and xj */
    raw_t value;
} constraint_t;

/** *Bound* constants.
 */
enum {
    dbm_INFINITY = INT_MAX >> 1, /**< infinity                           */
    dbm_OVERFLOW = INT_MAX >> 2  /**< to detect overflow on computations */
};

/** Bound *strictness*. Vital constant values *DO NOT CHANGE*.
 */
typedef enum {
    dbm_STRICT = 0, /**< strict less than constraints:  < x */
    dbm_WEAK = 1    /**< less or equal constraints   : <= x */
} strictness_t;

/** *Raw* encoded constants.
 */
enum {
    dbm_LE_ZERO = 1,                       /**< Less Equal Zero                    */
    dbm_LS_INFINITY = (dbm_INFINITY << 1), /**< Less Strict than infinity          */
    dbm_LE_OVERFLOW = dbm_LS_INFINITY >> 1 /**< to detect overflow on computations */
};

/** Encoding of bound into (strict) less or less equal.
 * @param bound,strict: the bound to encode with the strictness.
 * @return encoded constraint ("raw").
 */
static inline raw_t dbm_bound2raw(int32_t bound, strictness_t strict) { return (bound * 2) | strict; }

/** Encoding of bound into (strict) less or less equal.
 * @param bound,isStrict: the bound to encode with a flag
 * telling if the bound is strict or not.
 * if isStrict is true then dbm_STRICT is taken,
 * otherwise dbm_WEAK.
 * @return encoded constraint ("raw").
 */
#ifdef __cplusplus
static inline raw_t dbm_boundbool2raw(int32_t bound, bool isStrict) { return (bound * 2) | (isStrict ^ 1); }
#else
static inline raw_t dbm_boundbool2raw(int32_t bound, bool isStrict) { return (bound * 2) | (isStrict ^ 1); }
#endif

/** Decoding of raw representation: bound.
 * @param raw: encoded constraint (bound + strictness).
 * @return the decoded bound value.
 */
static inline int32_t dbm_raw2bound(raw_t raw) { return (raw >> 1); }

/** Make an encoded constraint weak.
 * @param raw: bound to make weak.
 * @pre raw != dbm_LS_INFINITY because <= infinity
 * is wrong.
 * @return weak raw.
 */
static inline raw_t dbm_weakRaw(raw_t raw)
{
    assert(dbm_WEAK == 1);
    assert(raw != dbm_LS_INFINITY);
    return raw | dbm_WEAK;  // set bit
}

/** Make an encoded constraint strict.
 * @param raw: bound to make strict.
 * @return strict raw.
 */
static inline raw_t dbm_strictRaw(raw_t raw)
{
    assert(dbm_WEAK == 1);
    return raw & ~dbm_WEAK;  // set bit
}

/** Decoding of raw representation: strictness.
 * @param raw: encoded constraint (bound + strictness).
 * @return the decoded strictness.
 */
static inline strictness_t dbm_raw2strict(raw_t raw) { return (strictness_t)(raw & 1); }

/** Tests of strictness.
 * @param raw: encoded constraint (bound + strictness).
 * @return true if the constraint is strict.
 * dbm_rawIsStrict(x) == !dbm_rawIsEq(x)
 */
static inline bool dbm_rawIsStrict(raw_t raw) { return ((raw & 1) ^ dbm_WEAK); }

/** Tests of non strictness.
 * @param raw: encoded constraint (bound + strictness).
 * @return true if the constraint is not strict.
 * dbm_rawIsStrict(x) == !dbm_rawIsEq(x)
 */
static inline bool dbm_rawIsWeak(raw_t raw) { return ((raw & 1) ^ dbm_STRICT); }

/** Negate the strictness of a constraint.
 * @param strictness: the flag to negate.
 */
static inline strictness_t dbm_negStrict(strictness_t strictness) { return (strictness_t)(strictness ^ 1); }

/** Negate a constraint:
 * neg(<a) = <=-a
 * neg(<=a) = <-a
 * @param c: the constraint.
 */
static inline raw_t dbm_negRaw(raw_t c)
{
    /* Check that the trick is correct
     */
    assert(1 - c == dbm_bound2raw(-dbm_raw2bound(c), dbm_negStrict(dbm_raw2strict(c))));
    return 1 - c;
}

/** "Weak" negate a constraint:
 * neg(<=a) = <= -a.
 * @pre c is weak.
 */
static inline raw_t dbm_weakNegRaw(raw_t c)
{
    assert(dbm_rawIsWeak(c));
    assert(2 - c == dbm_bound2raw(-dbm_raw2bound(c), dbm_WEAK));
    return 2 - c;
}

/** A valid raw bound should not cause overflow in computations.
 * @param x: encoded constraint (bound + strictness)
 * @return true if adding this constraint to any constraint
 * does not overflow.
 */
static inline bool dbm_isValidRaw(raw_t x)
{
    return (x == dbm_LS_INFINITY || (x < dbm_LE_OVERFLOW && -x < dbm_LE_OVERFLOW));
}

/** Constraint addition on raw values : + constraints - excess bit.
 * @param x,y: encoded constraints to add.
 * @return encoded constraint x+y.
 */
static inline raw_t dbm_addRawRaw(raw_t x, raw_t y)
{
    assert(x <= dbm_LS_INFINITY);
    assert(y <= dbm_LS_INFINITY);
    assert(dbm_isValidRaw(x));
    assert(dbm_isValidRaw(y));
    return (x == dbm_LS_INFINITY || y == dbm_LS_INFINITY) ? dbm_LS_INFINITY : (x + y) - ((x | y) & 1);
}

/** Constraint addition:
 * @param x,y: encoded constraints to add.
 * @return encoded constraint x+y.
 * @pre y finite.
 */
static inline raw_t dbm_addRawFinite(raw_t x, raw_t y)
{
    assert(x <= dbm_LS_INFINITY);
    assert(y < dbm_LS_INFINITY);
    assert(dbm_isValidRaw(x));
    assert(dbm_isValidRaw(y));
    return x == dbm_LS_INFINITY ? dbm_LS_INFINITY : (x + y) - ((x | y) & 1);
}

static inline raw_t dbm_addFiniteRaw(raw_t x, raw_t y) { return dbm_addRawFinite(y, x); }

/** Constraint addition.
 * @param x,y: encoded constraints to add.
 * @return encoded constraint x+y.
 * @pre x and y finite.
 */
static inline raw_t dbm_addFiniteFinite(raw_t x, raw_t y)
{
    assert(x < dbm_LS_INFINITY);
    assert(y < dbm_LS_INFINITY);
    assert(dbm_isValidRaw(x));
    assert(dbm_isValidRaw(y));
    return (x + y) - ((x | y) & 1);
}

/** Specialized constraint addition.
 * @param x,y: finite encoded constraints to add.
 * @pre x & y finite, x or y weak.
 * @return encoded constraint x+y
 */
static inline raw_t dbm_addFiniteWeak(raw_t x, raw_t y)
{
    assert(x < dbm_LS_INFINITY);
    assert(y < dbm_LS_INFINITY);
    assert(dbm_isValidRaw(x));
    assert(dbm_isValidRaw(y));
    assert((x | y) & 1);
    return x + y - 1;
}

/** Raw constraint increment:
 * @return constraint + increment with test infinity
 * @param c: constraint
 * @param i: increment
 */
static inline raw_t dbm_rawInc(raw_t c, raw_t i) { return c < dbm_LS_INFINITY ? c + i : c; }

/** Raw constraint decrement:
 * @return constraint + decremen with test infinity
 * @param c: constraint
 * @param d: decrement
 */
static inline raw_t dbm_rawDec(raw_t c, raw_t d) { return c < dbm_LS_INFINITY ? c - d : c; }

/** Convenience function to build a constraint.
 * @param i,j: indices.
 * @param bound: the bound.
 * @param strictness: strictness of the constraint.
 */
static inline constraint_t dbm_constraint(cindex_t i, cindex_t j, int32_t bound, strictness_t strictness)
{
    constraint_t c = {.i = i, .j = j, .value = dbm_bound2raw(bound, strictness)};
    return c;
}

/** 2nd convenience function to build a constraint.
 * @param i,j: indices.
 * @param bound: the bound.
 * @param isStrict: true if constraint is strict
 */
static inline constraint_t dbm_constraint2(cindex_t i, cindex_t j, int32_t bound, bool isStrict)
{
    constraint_t c = {.i = i, .j = j, .value = dbm_boundbool2raw(bound, isStrict)};
    return c;
}

/** Negation of a constraint.
 * Swap indices i,j, negate value, and toggle the strictness.
 * @param c: constraint to negate.
 * @return negated constraint.
 */
static inline constraint_t dbm_negConstraint(constraint_t c)
{
    cindex_t tmp = c.i;
    c.i = c.j;
    c.j = tmp;
    c.value = dbm_negRaw(c.value);
    return c;
}

/** Equality of constraints.
 * @param c1, c2: constraints.
 * @return true if c1 == c2.
 */
static inline bool dbm_areConstraintsEqual(constraint_t c1, constraint_t c2)
{
    return (c1.i == c2.i && c1.j == c2.j && c1.value == c2.value);
}

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DBM_CONSTRAINTS_H */

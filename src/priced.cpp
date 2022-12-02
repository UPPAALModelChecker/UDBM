/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2005, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: priced.cpp,v 1.8 2005/10/26 13:57:14 behrmann Exp $
 *
 **********************************************************************/

#include "dbm/priced.h"

#include "infimum.h"

#include "dbm/dbm_raw.hpp"
#include "dbm/print.h"
#include "debug/macros.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <cstdlib>

struct PDBM_s
{
    uint32_t count;
    uint32_t cost;
    uint32_t infimum;
    int32_t data[];
};

/** Returns the vectors of coefficients. */
static inline int32_t* pdbm_rates(const PDBM pdbm) { return pdbm->data; }

/** Returns the DBM matrix part of the priced DBM. */
static inline dbm::writer pdbm_matrix(const PDBM pdbm, cindex_t dim) { return {pdbm->data + dim, dim}; }

/** Returns the cache infimum. */
static inline auto& pdbm_cache(const PDBM pdbm) { return pdbm->infimum; }

/** Constant to mark the cached infimum as void. */
#define INVALID INT_MAX

#ifndef NDEBUG
/**
 * Returns true if two clocks form a zero cycle in a priced DBM.
 *
 * @param  pdbm is a closed priced DBM of dimension \a dim.
 * @param  dim  is the dimension of \a pdbm.
 * @param  i    is the index of a clock
 * @param  j    is the index of a clock
 * @return True if and only if \a i and \a j form a zero cycle in \a pdbm.
 */
static bool pdbm_areOnZeroCycle(const PDBM pdbm, cindex_t dim, cindex_t i, cindex_t j)
{
    assert(pdbm && dim && i < dim && j < dim && i != j);

    auto dbm = pdbm_matrix(pdbm, dim);
    return (dbm.bound(i, j) == -dbm.bound(j, i));
}
#endif

/**
 * Find the zero cycles of a DBM. The zero cycles are represented by
 * an array which for each clock contains the next element on the zero
 * cycle, or * 0 if the clock is the last element. I.e. next[i] is the
 * index of the next clock on a zero cycle with i.
 *
 * @post forall i.next[i] == 0 || next[i] > i
 */
static void dbm_findZeroCycles(dbm::reader dbm, cindex_t* next)
{
    const auto dim = dbm.get_dim();
    for (cindex_t i = 0; i < dim; ++i) {
        next[i] = 0;
        for (auto j = i + 1; j < dim; ++j) {
            if (dbm.bound(i, j) == -dbm.bound(j, i)) {
                next[i] = j;
                break;
            }
        }
    }
}

/**
 * Prepares a priced DBM for modification. If it is shared, a copy
 * will be created.
 */
static void pdbm_prepare(PDBM& pdbm, cindex_t dim)
{
    assert(pdbm && dim);

    if (pdbm->count > 1) {
        pdbm_decRef(pdbm);
        pdbm = pdbm_copy(nullptr, pdbm, dim);
        pdbm_incRef(pdbm);
    }
}

/**
 * Blanks or clears the priced DBM. When control returns from this
 * function, the DBM will be unshared and allocated. The contents of
 * the priced DBM cannot be relied on.
 *
 * This is similar to pdbm_prepare(), however
 *
 *  - it also works when the priced DBM is empty (which is represented
 *    as a NULL pointer).
 *
 *  - no copy is performed, even when the priced DBM was shared.
 */
static void pdbm_blank(PDBM& pdbm, cindex_t dim)
{
    if (pdbm == nullptr || pdbm->count > 1) {
        pdbm_decRef(pdbm);
        pdbm = pdbm_allocate(dim);
        pdbm_incRef(pdbm);
    }
}

size_t pdbm_size(cindex_t dim)
{
    assert(dim);
    return sizeof(struct PDBM_s) + (dim * dim + dim) * sizeof(int32_t);
}

PDBM pdbm_reserve(cindex_t dim, void* p)
{
    assert(dim && p);

    PDBM pdbm = (PDBM)p;
    pdbm->count = 0;
    pdbm_cache(pdbm) = INVALID;
    pdbm_rates(pdbm)[0] = 0;
    return pdbm;
}

PDBM pdbm_allocate(cindex_t dim)
{
    assert(dim);
    return pdbm_reserve(dim, malloc(pdbm_size(dim)));
}

void pdbm_deallocate(PDBM& pdbm)
{
    assert(pdbm == nullptr || pdbm->count == 0);

    free(pdbm);

    /* Setting the pointer to NULL protects against accidental use of
     * a deallocated priced DBM.
     */
    pdbm = nullptr;
}

PDBM pdbm_copy(PDBM dst, const PDBM src, cindex_t dim)
{
    assert(src && dim && (dst == nullptr || dst->count == 0));

    if (dst == nullptr) {
        dst = pdbm_allocate(dim);
    }

    memcpy(dst, src, pdbm_size(dim));
    dst->count = 0;

    return dst;
}

void pdbm_init(PDBM& pdbm, cindex_t dim)
{
    assert(dim);

    pdbm_blank(pdbm, dim);
    dbm_init(pdbm_matrix(pdbm, dim), dim);
    pdbm->cost = 0;
    pdbm_cache(pdbm) = 0;
    auto* r = pdbm_rates(pdbm);
    std::fill(r, r + dim, 0);

    assertx(pdbm_isValid(pdbm, dim));
}

void pdbm_zero(PDBM& pdbm, cindex_t dim)
{
    assert(dim);

    pdbm_blank(pdbm, dim);
    dbm_zero(pdbm_matrix(pdbm, dim), dim);
    pdbm->cost = 0;
    pdbm_cache(pdbm) = 0;
    auto* r = pdbm_rates(pdbm);
    std::fill(r, r + dim, 0);

    assertx(pdbm_isValid(pdbm, dim));
}

bool pdbm_constrain1(PDBM& pdbm, cindex_t dim, cindex_t i, cindex_t j, raw_t constraint)
{
    assert(pdbm && dim && i < dim && j < dim);

    auto dbm = pdbm_matrix(pdbm, dim);

    /* Check if entry is already tighter than constraint.
     */
    if (constraint >= dbm.at(i, j)) {
        return true;
    }

    /* If constraint will make DBM empty, then mark it as empty.
     */
    if (dbm_negRaw(constraint) >= dbm.at(j, i)) {
        if (pdbm->count > 1) {
            pdbm_decRef(pdbm);
            pdbm = nullptr;
        } else {
            dbm.at(i, j) = constraint;
            dbm.at(0, 0) = -1; /* consistent with isEmpty */
        }
        return false;
    }

    /* If DBM is shared, copy it first.
     */
    pdbm_prepare(pdbm, dim);
    dbm = pdbm_matrix(pdbm, dim);

    /* Compute the cost at the origin.
     */
    uint32_t cost = pdbm->cost;
    int32_t* rates = pdbm_rates(pdbm);
    for (uint32_t k = 1; k < dim; k++) {
        cost += rates[k] * dbm.bound(0, k);
    }

    /* Apply the constraint.
     */
    dbm.at(i, j) = constraint;
    dbm_closeij(dbm, dim, i, j);

    /* Compute the cost at the new offset point and invalidate the
     * cache.
     */
    for (uint32_t k = 1; k < dim; k++) {
        cost -= rates[k] * dbm.bound(0, k);
    }
    pdbm->cost = cost;
    pdbm_cache(pdbm) = INVALID;

    assertx(pdbm_isValid(pdbm, dim));
    return true;
}

bool pdbm_constrainN(PDBM& pdbm, cindex_t dim, const constraint_t* constraints, size_t n)
{
    assert(pdbm && dim);

    auto dbm = pdbm_matrix(pdbm, dim);

    for (; n != 0; n--, constraints++) {
        if (constraints->value < dbm.at(constraints->i, constraints->j)) {
            pdbm_prepare(pdbm, dim);

            dbm = pdbm_matrix(pdbm, dim);
            uint32_t cost = pdbm->cost;
            int32_t* rates = pdbm_rates(pdbm);

            for (uint32_t k = 1; k < dim; k++) {
                cost += rates[k] * dbm.bound(0, k);
            }

            if (!dbm_constrainN(dbm, dim, constraints, n)) {
                return false;
            }

            for (uint32_t k = 1; k < dim; k++) {
                cost -= rates[k] * dbm.bound(0, k);
            }

            pdbm->cost = cost;
            pdbm_cache(pdbm) = INVALID;
            break;
        }
    }
    return true;
}

/**
 * Returns the cost of the offset point of \a dbm2 in \a dbm1.
 *
 * @param dbm1   is the DBM for which to find the cost.
 * @param rates1 are the coefficients of the hyperplane of \a dbm1.
 * @param cost1  is the cost of the offset point of \a dbm1
 * @param dbm2   is the DBM providing the offset point for which
 *               we want to find the cost.
 * @param dim    is the dimension of \a dbm1 and \a dbm2.
 */
static int32_t costAtOtherOffset(dbm::reader dbm1, const int32_t* rates1, int32_t cost1, dbm::reader dbm2)
{
    assert(dbm1 && dbm2 && rates1 != nullptr);
    const auto dim = dbm1.get_dim();
    /* Notice that -dbm1[i] and -dbm2[i] are the lower bounds of clock i in dbm1 and dbm2, respectively. */
    for (uint32_t i = 1; i < dim; i++)
        cost1 += rates1[i] * (-dbm2.bound(0, i) - -dbm1.bound(0, i));
    return cost1;
}

/**
 * Given two planes, returns true if the slope of the first is less
 * than or equal to the slope of the other.
 *
 * @param first is a pointer to the first coefficient of the first plane.
 * @param last  is a pointer to the last coefficient of the first plane.
 * @param rate  is a pointer to the first coefficient of the second plane.
 */
inline static bool leq(const int32_t* first, const int32_t* last, const int32_t* rate)
{
    assert(first && last && rate);

    return std::equal(first, last, rate, std::less_equal<>{});
}

/**
 * Given a zone and two hyperplanes over that zone, returns the
 * infimum of the difference of the two planes.
 *
 * @param dbm   A DBM.
 * @param dim   The dimension of the DBM.
 * @param cost1 The cost at the offset for the first plane.
 * @param rate1 The rates of the first plane.
 * @param cost2 The cost at the offset for the second plane.
 * @param rate2 The rates of the second plane.
 */
static int32_t infOfDiff(const raw_t* dbm, uint32_t dim, int32_t cost1, const int32_t* rate1, int32_t cost2,
                         const int32_t* rate2)
{
    assert(dbm && dim && rate1 && rate2);

    int32_t cost = cost1 - cost2;
    auto rates = std::vector<int32_t>(dim, 0);
    std::transform(rate1, rate1 + dim, rate2, rates.begin(), std::minus<int32_t>{});

    return pdbm_infimum(dbm, dim, cost, rates.data());
}

relation_t pdbm_relation(const PDBM pdbm1, const PDBM pdbm2, cindex_t dim)
{
    assert(pdbm1 && pdbm2 && dim);

    auto dbm1 = pdbm_matrix(pdbm1, dim);
    auto dbm2 = pdbm_matrix(pdbm2, dim);
    int32_t* rates1 = pdbm_rates(pdbm1);
    int32_t* rates2 = pdbm_rates(pdbm2);
    uint32_t cost1 = pdbm1->cost;
    uint32_t cost2 = pdbm2->cost;

    int32_t c, d;

    switch (dbm_relation(dbm1, dbm2, dim)) {
    case base_SUPERSET:
        /* pdbm2 is smaller. Check whether it is also the most
         * expensive: This is the case when the difference between
         * pdbm2 and pdbm1 is always non-negative (the infimum is not
         * smaller than 0).
         */
        cost1 = costAtOtherOffset(dbm1, rates1, cost1, dbm2);
        if (cost1 <= cost2 &&
            (leq(rates1, rates1 + dim, rates2) || infOfDiff(dbm2, dim, cost2, rates2, cost1, rates1) >= 0)) {
            return base_SUPERSET;
        } else {
            return base_DIFFERENT;
        }

    case base_SUBSET:
        /* pdbm2 is bigger. Check whether it is also the cheapest:
         * This is the case when the difference between pdbm1 and
         * pdbm2 is always non-negative (the infimum is not smaller
         * than 0).
         */
        cost2 = costAtOtherOffset(dbm2, rates2, cost2, dbm1);
        if (cost2 <= cost1 &&
            (leq(rates2, rates2 + dim, rates1) || infOfDiff(dbm1, dim, cost1, rates1, cost2, rates2) >= 0)) {
            return base_SUBSET;
        } else {
            return base_DIFFERENT;
        }

    case base_EQUAL:
        /* Both have the same size. We need to compare the planes to
         * see which one is cheaper.
         */

        /* Do sound and cheap comparison first.
         */
        c = (cost1 <= cost2 && leq(rates1, rates1 + dim, rates2)) ? 1 : 0;
        d = (cost2 <= cost1 && leq(rates2, rates2 + dim, rates1)) ? 1 : 0;

        if ((c & d) != 0) {
            return base_EQUAL;
        } else if (c != 0) {
            return base_SUPERSET;
        } else if (d != 0) {
            return base_SUBSET;
        }

        /* The planes are incomparable, so we need to do the
         * subtraction. Notice that priced zones are not canonical,
         * so the two zones can in fact be equal even though the rates
         * are different.  Therefore we must also check for the
         * situation where both infima are zero.
         *
         * Notice that dbm1 equals dbm2, hence we do not need to
         * unpack dbm2.
         */
        c = infOfDiff(dbm1, dim, cost2, rates2, cost1, rates1);
        if (c > 0) {
            /* Early return to avoid unnecessary computation of the
             * second subtraction.
             */
            return base_SUPERSET;
        }

        d = infOfDiff(dbm1, dim, cost1, rates1, cost2, rates2);
        if (c == 0 && d == 0) {
            return base_EQUAL;
        }
        if (c >= 0) {
            return base_SUPERSET;
        }
        if (d >= 0) {
            return base_SUBSET;
        }
        return base_DIFFERENT;

    default: return base_DIFFERENT;
    }
}

relation_t pdbm_relationWithMinDBM(const PDBM pdbm1, cindex_t dim, const mingraph_t pdbm2, raw_t* dbm2)
{
    assert(pdbm1 && pdbm2 && dim && dbm2);

    auto dbm1 = pdbm_matrix(pdbm1, dim);
    uint32_t cost1 = pdbm1->cost;
    int32_t* rates1 = pdbm_rates(pdbm1);

    int32_t c, d;

    /* We know how the cost and the rates are encoded in a mingraph:
     * see writeToMinDBMWithOffset and readFromMinDBM.
     */
    uint32_t cost2 = pdbm2[0];
    const int32_t* rates2 = pdbm2 + 2;

    /* dbm_relationWithMinDBM will in some cases unpack pdbm2 into
     * dbm2. In order to detect whether this has happened, we set the
     * first entry of dbm2 to -1. If dbm_relationWithMinDBM did indeed
     * unpack pdbm2, this entry will have a different value
     * afterwards.
     */
    dbm2[0] = -1;

    switch (dbm_relationWithMinDBM(dbm1, dim, pdbm2 + dim + 2, dbm2)) {
    case base_SUPERSET:
        /* pdbm2 is smaller. Check whether it is also the most
         * expensive: This is the case when the difference between
         * pdbm2 and pdbm1 is always non-negative (the infimum is not
         * smaller than 0).
         */
        if (dbm2[0] == -1) {
            dbm_readFromMinDBM(dbm2, pdbm2 + dim + 2);
        }
        cost1 = costAtOtherOffset(dbm1, rates1, cost1, {dbm2, dim});
        if (cost1 <= cost2 &&
            (leq(rates1, rates1 + dim, rates2) || infOfDiff(dbm2, dim, cost2, rates2, cost1, rates1) >= 0)) {
            return base_SUPERSET;
        } else {
            return base_DIFFERENT;
        }

    case base_SUBSET:
        /* pdbm2 is bigger. Check whether it is also the cheapest:
         * This is the case when the difference between pdbm1 and
         * pdbm2 is always non-negative (the infimum is not smaller
         * than 0).
         */
        if (dbm2[0] == -1) {
            dbm_readFromMinDBM(dbm2, pdbm2 + dim + 2);
        }
        cost2 = costAtOtherOffset({dbm2, dim}, rates2, cost2, dbm1);
        if (cost2 <= cost1 &&
            (leq(rates2, rates2 + dim, rates1) || infOfDiff(dbm1, dim, cost1, rates1, cost2, rates2) >= 0)) {
            return base_SUBSET;
        } else {
            return base_DIFFERENT;
        }

    case base_EQUAL:
        /* Both have the same size. We need to compare the planes to
         * see which one is cheaper.
         */

        /* Do sound and cheap comparison first.
         */
        c = (cost1 <= cost2 && leq(rates1, rates1 + dim, rates2)) ? 1 : 0;
        d = (cost2 <= cost1 && leq(rates2, rates2 + dim, rates1)) ? 1 : 0;

        if ((c & d) != 0) {
            return base_EQUAL;
        } else if (c != 0) {
            return base_SUPERSET;
        } else if (d != 0) {
            return base_SUBSET;
        }

        /* The planes are incomparable, so we need to do the
         * subtraction. Notice that priced zones are not canonical,
         * so the two zones can in fact be equal even though the rates
         * are different.  Therefore we must also check for the
         * situation where both infima are zero.
         *
         * Notice that dbm1 equals dbm2, hence we do not need to
         * unpacked dbm2.
         */
        c = infOfDiff(dbm1, dim, cost2, rates2, cost1, rates1);
        if (c > 0) {
            /* Early return to avoid unnecessary computation of the
             * second subtraction.
             */
            return base_SUPERSET;
        }

        d = infOfDiff(dbm1, dim, cost1, rates1, cost2, rates2);
        if (c == 0 && d == 0) {
            return base_EQUAL;
        }
        if (c >= 0) {
            return base_SUPERSET;
        }
        if (d >= 0) {
            return base_SUBSET;
        }
        return base_DIFFERENT;

    default: return base_DIFFERENT;
    }
}

int32_t pdbm_getInfimum(const PDBM pdbm, cindex_t dim)
{
    assert(pdbm && dim);
    assert(dbm_isValid(pdbm_matrix(pdbm, dim), dim));
    uint32_t cache = pdbm_cache(pdbm);
    if (cache == INVALID) {
        pdbm_cache((PDBM)pdbm) = cache = pdbm_infimum(pdbm_matrix(pdbm, dim), dim, pdbm->cost, pdbm_rates(pdbm));
    }
    return cache;
}

int32_t pdbm_getInfimumValuation(const PDBM pdbm, cindex_t dim, int32_t* valuation, const bool* free)
{
    assert(pdbm && dim && valuation);
    assert(pdbm_isValid(pdbm, dim));

    std::vector<raw_t> copy(dim * dim);
    auto dbm = pdbm_matrix(pdbm, dim);
    int32_t* rates = pdbm_rates(pdbm);

    /* Compute the cost of the origin.
     */
    int32_t cost = pdbm->cost;
    for (uint32_t i = 1; i < dim; i++) {
        cost -= rates[i] * -dbm.bound(0, i);
    }

    /* If not all clocks are free, then restrict a copy of the DBM
     * according to the valuation given.
     */
    if (free != nullptr) {
        dbm_copy(copy.data(), dbm, dim);
        dbm = {copy.data(), dim};

        for (uint32_t i = 1; i < dim; i++) {
            if (!free[i]) {
                constraint_t con[2] = {dbm_constraint(i, 0, valuation[i], dbm_WEAK),
                                       dbm_constraint(0, i, -valuation[i], dbm_WEAK)};
                if (!dbm_constrainN(dbm, dim, con, 2)) {
                    throw std::out_of_range("No such evaluation exists");
                }
            }
        }
    }

    pdbm_infimum(dbm, dim, pdbm->cost, rates, valuation);
    for (uint32_t i = 0; i < dim; i++) {
        cost += rates[i] * valuation[i];
    }

    ASSERT(cost == pdbm_getCostOfValuation(pdbm, dim, valuation), pdbm_print(stderr, pdbm, dim));
    return cost;
}

bool pdbm_satisfies(const PDBM pdbm, cindex_t dim, cindex_t i, cindex_t j, raw_t constraint)
{
    assert(pdbm && dim && i < dim && i < dim);
    return dbm_satisfies(pdbm_matrix(pdbm, dim), dim, i, j, constraint);
}

bool pdbm_isEmpty(const PDBM pdbm, cindex_t dim)
{
    assert(pdbm && dim);
    return pdbm == nullptr || dbm_isEmpty(pdbm_matrix(pdbm, dim), dim);
}

bool pdbm_isUnbounded(const PDBM pdbm, cindex_t dim)
{
    assert(pdbm && dim);
    return dbm_isUnbounded(pdbm_matrix(pdbm, dim), dim);
}

uint32_t pdbm_hash(const PDBM pdbm, cindex_t dim, uint32_t seed)
{
    assert(pdbm && dim && !(pdbm_size(dim) & 3));
    return hash_computeI32((int32_t*)pdbm, pdbm_size(dim) >> 2, seed);
}

static bool isPointIncludedWeakly(const int32_t* pt, dbm::reader dbm, cindex_t dim)
{
    cindex_t i, j;
    assert(pt && dbm && dim);

    for (i = 0; i < dim; ++i) {
        for (j = 0; j < dim; ++j) {
            if (pt[i] - pt[j] > dbm.bound(i, j)) {
                return false;
            }
        }
    }

    return true;
}

bool pdbm_containsInt(const PDBM pdbm, cindex_t dim, const int32_t* pt)
{
    assert(pdbm && dim && pt);

    return dbm_isPointIncluded(pt, pdbm_matrix(pdbm, dim), dim);
}

bool pdbm_containsIntWeakly(const PDBM pdbm, cindex_t dim, const int32_t* pt)
{
    assert(pdbm && dim && pt);

    return isPointIncludedWeakly(pt, pdbm_matrix(pdbm, dim), dim);
}

bool pdbm_containsDouble(const PDBM pdbm, cindex_t dim, const double* pt)
{
    assert(pdbm && dim && pt);

    return dbm_isRealPointIncluded(pt, pdbm_matrix(pdbm, dim), dim);
}

void pdbm_up(PDBM& pdbm, cindex_t dim)
{
    assert(pdbm && dim);

    pdbm_prepare(pdbm, dim);
    dbm_up(pdbm_matrix(pdbm, dim), dim);

    assertx(pdbm_isValid(pdbm, dim));
}

void pdbm_upZero(PDBM& pdbm, cindex_t dim, int32_t rate, cindex_t zero)
{
    assert(pdbm && dim && zero > 0 && zero < dim);
    assert(pdbm_areOnZeroCycle(pdbm, dim, 0, zero));

    pdbm_prepare(pdbm, dim);

    raw_t* dbm = pdbm_matrix(pdbm, dim);
    int32_t* rates = pdbm_rates(pdbm);

    dbm_up(dbm, dim);
    rates[zero] = 0;
    rates[zero] = rate - pdbm_getSlopeOfDelayTrajectory(pdbm, dim);

    assertx(pdbm_isValid(pdbm, dim));
}

void pdbm_updateValue(PDBM& pdbm, cindex_t dim, cindex_t clock, uint32_t value)
{
    assert(pdbm && dim && clock < dim);
    assert(pdbm_getRate(pdbm, dim, clock) == 0);

    pdbm_prepare(pdbm, dim);

    dbm_updateValue(pdbm_matrix(pdbm, dim), dim, clock, value);

    assertx(pdbm_isValid(pdbm, dim));
}

void pdbm_updateValueZero(PDBM& pdbm, cindex_t dim, cindex_t clock, uint32_t value, cindex_t zero)
{
    assert(pdbm && dim && clock < dim && zero < dim);
    assert(pdbm_areOnZeroCycle(pdbm, dim, clock, zero));

    pdbm_prepare(pdbm, dim);

    int32_t* rates = pdbm_rates(pdbm);

    if (zero != 0) {
        rates[zero] += rates[clock];
    }
    rates[clock] = 0;
    dbm_updateValue(pdbm_matrix(pdbm, dim), dim, clock, value);

    assertx(pdbm_isValid(pdbm, dim));
}

/*


|           |
|           |
|     x-----|-----------
|           |
|           |
|           |
L           U


|           |
|           |
|           |-------x---
|           |
|           |
|           |
L           U


|           |
|           |
|------x    |
|           |
|           |
|           |
U           L

|           |
|           |
|-----------|-------x---
|           |
|           |
|           |
U           L

*/

/* extrapolateMaxBounds will change the lower and upper bounds, thus
 * it is only safe for clocks with a cost rate of zero. Thus as a
 * preprocessing step, we change the maximum constant to infinity for
 * all clocks with a non-xero cost rate.
 */
void pdbm_extrapolateMaxBounds(PDBM& pdbm, cindex_t dim, int32_t* max)
{
    assert(pdbm && dim);

    int32_t* rates = pdbm_rates(pdbm);

    for (uint32_t i = 1; i < dim; i++) {
        if (rates[i] != 0) {
            max[i] = dbm_INFINITY;
        }
    }

    pdbm_prepare(pdbm, dim);
    dbm_extrapolateMaxBounds(pdbm_matrix(pdbm, dim), dim, max);
}

/* extrapolateMaxBounds will change the lower and upper bounds, thus
 * it is only safe for clocks with a cost rate of zero.  Thus as a
 * preprocessing step, we change the maximum constant to infinity for
 * all clocks with a non-zero cost rate. For inactive clocks which are
 * on a zero cycle with another clock, the cost rate can be made zero
 * by transfering the rate to the other clock.
 */
void pdbm_diagonalExtrapolateMaxBounds(PDBM& pdbm, cindex_t dim, int32_t* max)
{
    assert(pdbm && dim);

    int32_t* rates = pdbm_rates(pdbm);

    /* If possible, transfer the cost rate of inactive clocks to other
     * clocks (clocks which are on a zero cycle with the inactive
     * clock).
     */
    for (cindex_t i = 1; i < dim; i++) {
        if (max[i] == -dbm_INFINITY && rates[i] != 0) {
            cindex_t first;
            if (pdbm_findZeroCycle(pdbm, dim, i, &first)) {
                /* Try to find an active clock on a zero cycle with i.
                 */
                cindex_t k = first;
                while (max[k] == -dbm_INFINITY) {
                    k++;
                    if (!pdbm_findNextZeroCycle(pdbm, dim, i, &k)) {
                        k = first;
                        break;
                    }
                }

                if (k != 0) {
                    rates[k] += rates[i];
                }
                rates[i] = 0;
            }
        }
    }

    /* It is only safe to extrapolate clocks with a zero cost rate. */
    for (cindex_t i = 1; i < dim; i++) {
        if (rates[i] != 0) {
            max[i] = dbm_INFINITY;
        }
    }

    pdbm_prepare(pdbm, dim);
    dbm_diagonalExtrapolateMaxBounds(pdbm_matrix(pdbm, dim), dim, max);
}

void pdbm_diagonalExtrapolateLUBounds(PDBM& pdbm, cindex_t dim, int32_t* lower, int32_t* upper)
{
    assert(pdbm && dim);

    int32_t* rates = pdbm_rates(pdbm);

    for (uint32_t i = 1; i < dim; i++) {
        //          if (rates[i] < 0)
        //          {
        //              lower[i] = dbm_INFINITY;
        //          }
        //          else if (rates[i] > 0)
        //          {
        //              upper[i] = dbm_INFINITY;
        //          }
        if (rates[i] != 0) {
            lower[i] = dbm_INFINITY;
            upper[i] = dbm_INFINITY;
        }
    }

    pdbm_prepare(pdbm, dim);
    dbm_diagonalExtrapolateLUBounds(pdbm_matrix(pdbm, dim), dim, lower, upper);
}

void pdbm_incrementCost(PDBM& pdbm, cindex_t dim, int32_t value)
{
    assert(pdbm && dim && value >= 0);

    pdbm_prepare(pdbm, dim);
    pdbm->cost += value;
    pdbm_cache(pdbm) += value;

    assertx(pdbm_isValid(pdbm, dim));
}

void pdbm_close(PDBM& pdbm, cindex_t dim)
{
    assert(dim);

    if (pdbm != nullptr) {
        pdbm_prepare(pdbm, dim);
        dbm_close(pdbm_matrix(pdbm, dim), dim);
    }

    assertx(pdbm_isValid(pdbm, dim));
}

size_t pdbm_analyzeForMinDBM(const PDBM pdbm, cindex_t dim, uint32_t* bitMatrix)
{
    assert(pdbm && dim);

    return dbm_analyzeForMinDBM(pdbm_matrix(pdbm, dim), dim, bitMatrix);
}

int32_t* pdbm_writeToMinDBMWithOffset(const PDBM pdbm, cindex_t dim, bool minimizeGraph, bool tryConstraints16,
                                      allocator_t allocator, uint32_t offset)
{
    assert(pdbm && dim);

    int32_t* graph = dbm_writeToMinDBMWithOffset(pdbm_matrix(pdbm, dim), dim, minimizeGraph, tryConstraints16,
                                                 allocator, offset + dim + 2);
    graph[offset] = pdbm->cost;
    graph[offset + 1] = pdbm_cache(pdbm);
    auto* r = pdbm_rates(pdbm);
    std::copy(r, r + dim, graph + offset + 2);
    return graph;
}

void pdbm_readFromMinDBM(PDBM& dst, cindex_t dim, mingraph_t src)
{
    assert(dst && dim);

    pdbm_blank(dst, dim);
    dst->cost = src[0];
    pdbm_cache(dst) = src[1];
    std::copy(src + 2, src + 2 + dim, pdbm_rates(dst));
    dbm_readFromMinDBM(pdbm_matrix(dst, dim), src + dim + 2);

    assertx(pdbm_isValid(dst, dim));
}

bool pdbm_findNextZeroCycle(const PDBM pdbm, cindex_t dim, cindex_t x, cindex_t* out)
{
    assert(pdbm && dim && x < dim && out);

    dbm::reader dbm = pdbm_matrix(pdbm, dim);
    cindex_t i = *out;

    while (i < dim) {
        if (i != x && dbm.bound(i, x) == -dbm.bound(x, i)) {
            *out = i;
            return true;
        }
        i++;
    }
    *out = i;

    return false;
}

bool pdbm_findZeroCycle(const PDBM pdbm, cindex_t dim, cindex_t x, cindex_t* out)
{
    *out = 0;
    return pdbm_findNextZeroCycle(pdbm, dim, x, out);
}

int32_t pdbm_getSlopeOfDelayTrajectory(const PDBM pdbm, cindex_t dim)
{
    assert(pdbm && dim);

    int32_t* rates = pdbm_rates(pdbm);
    int32_t sum = 0;
    for (uint32_t i = 1; i < dim; i++) {
        sum += rates[i];
    }
    return sum;
}

int32_t pdbm_getRate(const PDBM pdbm, cindex_t dim, cindex_t clock)
{
    assert(pdbm && dim && clock > 0 && clock < dim);

    return pdbm_rates(pdbm)[clock];
}

uint32_t pdbm_getCostAtOffset(const PDBM pdbm, cindex_t dim)
{
    assert(pdbm && dim);

    return pdbm->cost;
}

void pdbm_setCostAtOffset(PDBM& pdbm, cindex_t dim, uint32_t value)
{
    assert(pdbm && dim);

    pdbm_prepare(pdbm, dim);
    pdbm->cost = value;
    pdbm_cache(pdbm) = INVALID;
}

/**
 *
 */
static bool isRedundant(dbm::reader dbm, cindex_t i, cindex_t j, cindex_t* next)
{
    const auto dim = dbm.get_dim();
    assert(dbm && i < dim && j < dim && next);

    raw_t bij, bik, bkj;
    cindex_t k;

    bij = dbm.at(i, j);
    if (i != j && bij < dbm_LS_INFINITY) {
        for (k = 0; k < dim; k++) {
            if (i != k && j != k && next[k] == 0) {
                bik = dbm.at(i, k);
                bkj = dbm.at(k, j);
                if (bik < dbm_LS_INFINITY && bkj < dbm_LS_INFINITY && bij >= dbm_addFiniteFinite(bik, bkj)) {
                    return true;
                }
            }
        }
        return false;
    }
    return true;
}

uint32_t pdbm_getLowerRelativeFacets(PDBM& pdbm, cindex_t dim, cindex_t clock, cindex_t* facets)
{
    assert(pdbm && dim && clock < dim);

    std::vector<cindex_t> next(dim);

    pdbm_prepare(pdbm, dim);

    auto dbm = pdbm_matrix(pdbm, dim);

    /* Make all lower facets relative to clock weak.
     */
    dbm_relaxDownClock(dbm, dim, clock);

    /* Identify zero cycles.
     */
    dbm_findZeroCycles(dbm, next.data());

    /* Find non-redundant facets.
     */
    uint32_t cnt = 0;
    for (cindex_t i = 0; i < dim; ++i) {
        if (next[i] == 0 && !isRedundant(dbm, i, clock, next.data())) {
            *facets = i;
            facets++;
            cnt++;
        }
    }
    return cnt;
}

uint32_t pdbm_getUpperRelativeFacets(PDBM& pdbm, cindex_t dim, cindex_t clock, cindex_t* facets)
{
    assert(pdbm && dim && clock < dim && facets);

    std::vector<cindex_t> next(dim);
    cindex_t i;

    pdbm_prepare(pdbm, dim);

    auto dbm = pdbm_matrix(pdbm, dim);

    /* Make all upper facets relative to clock weak.
     */
    dbm_relaxUpClock(dbm, dim, clock);

    /* Identify zero cycles.
     */
    dbm_findZeroCycles(dbm, next.data());

    /* Find non-redundant facets.
     */
    uint32_t cnt = 0;
    for (i = 0; i < dim; i++) {
        if (next[i] == 0 && !isRedundant(dbm, clock, i, next.data())) {
            *facets = i;
            facets++;
            cnt++;
        }
    }
    return cnt;
}

uint32_t pdbm_getLowerFacets(PDBM& pdbm, cindex_t dim, cindex_t* facets)
{
    assert(pdbm && dim && facets);

    std::vector<cindex_t> next(dim);
    cindex_t i;

    pdbm_prepare(pdbm, dim);

    auto dbm = pdbm_matrix(pdbm, dim);

    /* Make all lower facets weak.
     */
    dbm_relaxDown(dbm, dim);

    /* Identify zero cycles.
     */
    dbm_findZeroCycles(dbm, next.data());

    /* Find non-redundant facets.
     */
    uint32_t cnt = 0;
    for (i = 0; i < dim; i++) {
        if (next[i] == 0 && !isRedundant(dbm, 0, i, next.data())) {
            *facets = i;
            facets++;
            cnt++;
        }
    }
    return cnt;
}

uint32_t pdbm_getUpperFacets(PDBM& pdbm, cindex_t dim, cindex_t* facets)
{
    assert(pdbm && dim && facets);

    std::vector<cindex_t> next(dim);
    cindex_t i;

    pdbm_prepare(pdbm, dim);

    auto dbm = pdbm_matrix(pdbm, dim);

    /* Make all upper facets weak.
     */
    dbm_relaxUp(dbm, dim);

    /* Identify zero cycles.
     */
    dbm_findZeroCycles(dbm, next.data());

    /* Find non-redundant facets.
     */
    uint32_t cnt = 0;
    for (i = 0; i < dim; i++) {
        if (next[i] == 0 && !isRedundant(dbm, i, 0, next.data())) {
            *facets = i;
            facets++;
            cnt++;
        }
    }
    return cnt;
}

int32_t pdbm_getCostOfValuation(const PDBM pdbm, cindex_t dim, const int32_t* valuation)
{
    assert(pdbm && dim && valuation);
    assert(pdbm_containsInt(pdbm, dim, valuation));

    auto dbm = pdbm_matrix(pdbm, dim);
    int32_t cost = pdbm->cost;
    for (uint32_t i = 1; i < dim; i++) {
        cost += (valuation[i] - -dbm.bound(0, i)) * pdbm_rates(pdbm)[i];
    }
    return cost;
}

void pdbm_relax(PDBM& pdbm, cindex_t dim)
{
    pdbm_prepare(pdbm, dim);

    raw_t* dbm = pdbm_matrix(pdbm, dim);
    for (raw_t *it = dbm + 1, *last = dbm + dim * dim - 1; it != last; ++it) {
        if (*it < dbm_LS_INFINITY) {
            *it = dbm_bound2raw(dbm_raw2bound(*it), dbm_WEAK);
        }
    }

    assertx(pdbm_isValid(pdbm, dim));
}

bool pdbm_isValid(const PDBM pdbm, cindex_t dim)
{
    assert(dim);

    if (pdbm == nullptr) {
        return true;
    }

    raw_t* dbm = pdbm_matrix(pdbm, dim);
    int32_t* rates = pdbm_rates(pdbm);
    uint32_t cost = pdbm->cost;
    int32_t cache = pdbm_cache(pdbm);
    int32_t inf = pdbm_infimum(dbm, dim, cost, rates);

    return (cache == INVALID || cache == inf) && dbm_isValid(dbm, dim) && rates[0] == 0 &&
           (!pdbm_isUnbounded(pdbm, dim) || pdbm_getSlopeOfDelayTrajectory(pdbm, dim) >= 0);
}

void pdbm_freeClock(PDBM& pdbm, cindex_t dim, cindex_t clock)
{
    assert(pdbm && dim && clock > 0 && clock < dim);
    assert(pdbm_rates(pdbm)[clock] == 0);

    dbm_freeClock(pdbm_matrix(pdbm, dim), dim, clock);

    assertx(pdbm_isValid(pdbm, dim));
}

void pdbm_getOffset(const PDBM pdbm, cindex_t dim, int32_t* valuation)
{
    assert(pdbm && dim && valuation);

    auto dbm = pdbm_matrix(pdbm, dim);
    for (uint32_t i = 1; i < dim; i++) {
        assert(dbm.bound(0, i) <= 0);
        valuation[i] = -dbm.bound(0, i);
    }
}

void pdbm_setRate(PDBM& pdbm, cindex_t dim, cindex_t clock, int32_t rate)
{
    assert(pdbm && dim && clock > 0 && clock < dim);

    pdbm_prepare(pdbm, dim);
    pdbm_rates(pdbm)[clock] = rate;
    pdbm_cache(pdbm) = INVALID;
}

raw_t* pdbm_getMutableMatrix(PDBM& pdbm, cindex_t dim)
{
    assert(dim);

    if (pdbm != nullptr) {
        pdbm_prepare(pdbm, dim);
    } else {
        pdbm = pdbm_allocate(dim);
        pdbm_incRef(pdbm);
    }
    pdbm_cache(pdbm) = INVALID;

    return pdbm_matrix(pdbm, dim);
}

const raw_t* pdbm_getMatrix(const PDBM pdbm, cindex_t dim)
{
    assert(pdbm && dim);
    return pdbm_matrix(pdbm, dim);
}

const int32_t* pdbm_getRates(const PDBM pdbm, cindex_t dim)
{
    assert(pdbm && dim);
    return pdbm_rates(pdbm);
}

bool pdbm_constrainToFacet(PDBM& pdbm, cindex_t dim, cindex_t i, cindex_t j)
{
    auto dbm = pdbm_matrix(pdbm, dim);
    auto bound = -dbm.bound(i, j);
    return pdbm_constrain1(pdbm, dim, j, i, dbm_bound2raw(bound, dbm_WEAK));
}

void pdbm_print(FILE* f, const PDBM pdbm, cindex_t dim)
{
    int32_t infimum = pdbm_getInfimum(pdbm, dim);
    dbm_print(f, pdbm_matrix(pdbm, dim), dim);
    fprintf(f, "Rates:");
    for (cindex_t i = 1; i < dim; i++) {
        fprintf(f, " %d", pdbm_getRate(pdbm, dim, i));
    }
    fprintf(f, "\n");
    fprintf(f, "Offset: %d Infimum: %d\n", pdbm->cost, infimum);
}

std::ostream& pdbm_print(std::ostream& os, const PDBM pdbm, cindex_t dim)
{
    int32_t infimum = pdbm_getInfimum(pdbm, dim);
    dbm_cppPrint(os, pdbm_matrix(pdbm, dim));
    os << "Rates:";
    for (cindex_t i = 1; i < dim; ++i) {
        os << ' ' << pdbm_getRate(pdbm, dim, i);
    }
    os << '\n' << "Offset: " << pdbm->cost << " Infimum: " << infimum << '\n';
    return os;
}

void pdbm_freeUp(PDBM& pdbm, cindex_t dim, cindex_t index)
{
    assert(pdbm_rates(pdbm)[index] >= 0);

    pdbm_prepare(pdbm, dim);
    dbm_freeUp(pdbm_matrix(pdbm, dim), dim, index);
}

void pdbm_freeDown(PDBM& pdbm, cindex_t dim, cindex_t index)
{
    assert(pdbm_rates(pdbm)[index] <= 0);

    pdbm_prepare(pdbm, dim);

    auto dbm = pdbm_matrix(pdbm, dim);

    /* Move offset point.
     */
    int32_t rate = pdbm_rates(pdbm)[index];
    int32_t bound = -dbm.bound(0, index);
    pdbm->cost -= bound * rate;

    /* Stretch down.
     */
    dbm_freeDown(dbm, dim, index);
}

void pdbm_normalise(PDBM pdbm, cindex_t dim)
{
    int32_t* rates = pdbm_rates(pdbm);
    std::vector<cindex_t> next(dim);
    cindex_t i;

    assert(dim > 0);
    dbm_findZeroCycles(pdbm_matrix(pdbm, dim), next.data());

    /* Everything in the equivalence class of 0 will have rate 0.
     */
    for (i = next[0]; i != 0; i = next[i]) {
        rates[i] = 0;
    }

    /* For all other equivalence classes, the rate is transfered to
     * the last clock.
     */
    for (i = 1; i < dim; i++) {
        if (next[i] != 0) {
            rates[next[i]] += rates[i];
            rates[i] = 0;
        }
    }
}

bool pdbm_hasNormalForm(PDBM pdbm, cindex_t dim)
{
    int32_t* rates = pdbm_rates(pdbm);
    std::vector<cindex_t> next(dim);
    cindex_t i;

    assert(dim > 0);
    dbm_findZeroCycles(pdbm_matrix(pdbm, dim), next.data());

    /* Everything in the equivalence class of 0 will have rate 0.
     */
    for (i = next[0]; i != 0; i = next[i]) {
        if (rates[i] != 0) {
            return false;
        }
    }

    /* For all other equivalence classes, the rate is transfered to
     * the last clock.
     */
    for (i = 1; i < dim; i++) {
        if (next[i] != 0) {
            if (rates[i] != 0) {
                return false;
            }
        }
    }

    return true;
}

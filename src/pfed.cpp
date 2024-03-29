// -*- Mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
///////////////////////////////////////////////////////////////////////////////
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 2006, Uppsala University and Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "dbm/pfed.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace dbm
{
    std::allocator<pfed_t::pfed_s> pfed_t::alloc;

    void pfed_t::decRef()
    {
        if (--(ptr->count) == 0) {
            alloc.destroy(ptr);
            alloc.deallocate(ptr, 1);
        }
    }

    void pfed_t::cow()
    {
        assert(ptr->count > 1);

        ptr->count--;
        pfed_s* old = ptr;
        ptr = alloc.allocate(1);
        alloc.construct(ptr, pfed_s());
        ptr->zones = old->zones;
        ptr->dim = old->dim;
        ptr->count = 1;
    }

    pfed_t::pfed_t()
    {
        ptr = alloc.allocate(1);
        alloc.construct(ptr, pfed_s());
        ptr->dim = 0;
        ptr->count = 1;
    }

    pfed_t::pfed_t(cindex_t dim)
    {
        ptr = alloc.allocate(1);
        alloc.construct(ptr, pfed_s());
        ptr->dim = dim;
        ptr->count = 1;
    }

    pfed_t::pfed_t(const PDBM pdbm, cindex_t dim)
    {
        ptr = alloc.allocate(1);
        alloc.construct(ptr, pfed_s());
        ptr->dim = dim;
        ptr->count = 1;
        add(pdbm, dim);
    }

    pfed_t::iterator pfed_t::erase(iterator i)
    {
        assert(ptr->count <= 1);
        return ptr->zones.erase(i);
    }

    bool pfed_t::constrain(cindex_t i, uint32_t value)
    {
        constraint_t c[2] = {constraint_t{i, 0, dbm_bound2raw(value, dbm_WEAK)},
                             constraint_t{0, i, dbm_bound2raw(-value, dbm_WEAK)}};
        return constrain(c, 2);
    }

    bool pfed_t::constrain(cindex_t i, cindex_t j, raw_t constraint)
    {
        erase_if_not(
            [dim = ptr->dim, i, j, constraint](PDBM& pdbm) { return pdbm_constrain1(pdbm, dim, i, j, constraint); });
        return !isEmpty();
    }

    bool pfed_t::constrain(const constraint_t* constraints, size_t n)
    {
        erase_if_not(
            [dim = ptr->dim, constraints, n](PDBM& pdbm) { return pdbm_constrainN(pdbm, dim, constraints, n); });
        return !isEmpty();
    }

    static int32_t min(int32_t a, int32_t b) { return a < b ? a : b; }

    int32_t pfed_t::getInfimum() const
    {
        return std::accumulate(cbegin(), cend(), INT_MAX, [dim = ptr->dim](const int32_t a, const PDBM pdbm) {
            return min(a, pdbm_getInfimum(pdbm, dim));
        });
    }

    int32_t pfed_t::getInfimumValuation(valuation_int& valuation, const bool* free) const
    {
        uint32_t dim = ptr->dim;
        int32_t infimum = INT_MAX;
        auto copy = std::vector<int32_t>(dim);

        for (const auto& zone : *this) {
            std::copy(valuation.begin(), valuation.end(), copy.begin());
            int32_t inf = pdbm_getInfimumValuation(zone, dim, copy.data(), free);
            if (inf < infimum) {
                infimum = inf;
                std::copy(copy.begin(), copy.end(), valuation.begin_mutable());
            }
        }
        return infimum;
    }

    bool pfed_t::satisfies(cindex_t i, cindex_t j, raw_t constraint) const
    {
        return std::find_if(begin(), end(), [dim = ptr->dim, i, j, constraint](const pdbm_t& pdbm) {
                   return pdbm_satisfies(pdbm, dim, i, j, constraint);
               }) != end();
    }

    bool pfed_t::isUnbounded() const
    {
        return std::find_if(begin(), end(),
                            [dim = ptr->dim](const pdbm_t& pdbm) { return pdbm_isUnbounded(pdbm, dim); }) != end();
    }

    uint32_t pfed_t::hash(uint32_t seed) const
    {
        return std::accumulate(begin(), end(), seed, [dim = ptr->dim](uint32_t seed, const pdbm_t& pdbm) {
            return pdbm_hash(pdbm, dim, seed);
        });
    }

    bool pfed_t::contains(const valuation_int& valuation) const
    {
        return std::find_if(begin(), end(), [dim = ptr->dim, &valuation](const pdbm_t& pdbm) {
                   return pdbm_containsInt(pdbm, dim, valuation);
               }) != end();
    }

    bool pfed_t::contains(const valuation_fp& valuation) const
    {
        return std::find_if(begin(), end(), [dim = ptr->dim, &valuation](const pdbm_t& pdbm) {
                   return pdbm_containsDouble(pdbm, dim, valuation);
               }) != end();
    }

    bool pfed_t::containsWeakly(const valuation_int& valuation) const
    {
        return std::find_if(begin(), end(), [dim = ptr->dim, &valuation](const pdbm_t& pdbm) {
                   return pdbm_containsIntWeakly(pdbm, dim, valuation);
               }) != end();
    }

    relation_t pfed_t::relation(const pfed_t& b) const
    {
        /* For symmetry, we call (*this) for a.
         */
        const pfed_t& a = *this;

        assert(a.getDimension() == b.getDimension());

        /* a_s[i] becomes true if zone i of a is contained in some
         * zone of b.

         * b_s[j] becomes true if zone j of b is contained in some
         * zone of a.
         */
        cindex_t dim = ptr->dim;
        size_t size_a = a.size();
        size_t size_b = b.size();
        bool* a_s = (bool*)calloc(size_a, sizeof(bool));
        bool* b_s = (bool*)calloc(size_b, sizeof(bool));
        bool *p_a, *p_b;
        const_iterator i, j;

        // Stack allocated obj are aligned so the override of
        // resetSmall is harmless.
        std::fill(a_s, a_s + size_a, 0);
        std::fill(b_s, b_s + size_b, 0);

        for (i = a.begin(), p_a = a_s; i != a.end(); ++i, ++p_a) {
            for (j = b.begin(), p_b = b_s; j != b.end(); ++j, ++p_b) {
                if (!*p_a || !*p_b) {
                    switch (pdbm_relation(*i, *j, dim)) {
                    case base_DIFFERENT: break;
                    case base_SUPERSET: *p_b = true; break;
                    case base_SUBSET: *p_a = true; break;
                    case base_EQUAL:
                        *p_b = true;
                        *p_a = true;
                        break;
                    }
                }
            }
        }

        /* a is a subset of b if all zones of a are contained in some
         * zone of b, and vice versa.
         */
        for (p_a = a_s; p_a < a_s + size_a && *p_a; ++p_a) {
        }

        for (p_b = b_s; p_b < b_s + size_b && *p_b; ++p_b) {
        }

        auto retVal = (relation_t)((p_a == a_s + size_a ? base_SUBSET : 0) | (p_b == b_s + size_b ? base_SUPERSET : 0));
        free(a_s);
        free(b_s);
        return retVal;
    }

    relation_t pfed_t::exactRelation(const pfed_t& b) const
    {
        assert(size() == 1 && b.size() == 1);
        return relation(b);
    }

    void pfed_t::up()
    {
        for (auto& zone : *this)
            pdbm_up(zone, ptr->dim);
    }

    void pfed_t::up(int32_t rate)
    {
        assert(rate >= 0);

        uint32_t dim = ptr->dim;

        prepare();

        for (auto& zone : *this) {
            cindex_t x;
            int32_t oldrate = pdbm_getSlopeOfDelayTrajectory(zone, dim);

            if (rate == oldrate) {
                /* This is a simple case which does not require splits.
                 */
                pdbm_up(zone, dim);
            } else if (pdbm_findZeroCycle(zone, dim, 0, &x)) {
                /* This is a simple case which does not require splits.
                 */
                pdbm_upZero(zone, dim, rate, x);
            } else if (rate < oldrate) {
                /* New rate is smaller than old rate, so we delay from the
                 * lower facets
                 */
                std::vector<uint32_t> facets(dim);
                uint32_t count = pdbm_getLowerFacets(zone, dim, facets.data());
                assert(count > 0 && count <= dim);
                for (uint32_t j = 0; j < count - 1; j++) {
                    ptr->zones.push_front(zone);
                    pdbm_constrainToFacet(ptr->zones.front(), dim, 0, facets[j]);
                    pdbm_upZero(ptr->zones.front(), dim, rate, facets[j]);
                }
                pdbm_constrainToFacet(zone, dim, 0, facets[count - 1]);
                pdbm_upZero(zone, dim, rate, facets[count - 1]);
            } else {
                assert(rate > oldrate);

                /* New rate is bigger than old rate, so we delay from the
                 * upper facets. We also need the original zone.
                 */
                std::vector<uint32_t> facets(dim);
                uint32_t count = pdbm_getUpperFacets(zone, dim, facets.data());
                assert(count > 0 && count <= dim);
                for (uint32_t j = 0; j < count; j++) {
                    ptr->zones.push_front(zone);
                    pdbm_constrainToFacet(ptr->zones.front(), dim, facets[j], 0);
                    pdbm_upZero(ptr->zones.front(), dim, rate, facets[j]);
                }
                pdbm_constrainToFacet(zone, dim, facets[count - 1], 0);
                pdbm_upZero(zone, dim, rate, facets[count - 1]);
            }
        }
    }

    void pfed_t::updateValueZero(cindex_t clock, int32_t value, cindex_t zero)
    {
        for (auto& zone : *this)
            pdbm_updateValueZero(zone, ptr->dim, clock, value, zero);
    }

    void pfed_t::updateValue(cindex_t clock, uint32_t value)
    {
        uint32_t dim = ptr->dim;

        prepare();

        for (auto& zone : *this) {
            cindex_t k;
            int32_t rate = pdbm_getRate(zone, dim, clock);

            if (rate == 0) {
                pdbm_updateValue(zone, dim, clock, value);
            } else if (pdbm_findZeroCycle(zone, dim, clock, &k)) {
                pdbm_updateValueZero(zone, dim, clock, value, k);
            } else if (rate > 0) {
                /* Find and reset lower facets when rate is positive.
                 */
                std::vector<uint32_t> facets(dim);
                int32_t count = pdbm_getLowerRelativeFacets(zone, dim, clock, facets.data());

                assert(count >= 1);
                for (int32_t j = 0; j < count - 1; j++) {
                    ptr->zones.push_front(zone);
                    pdbm_constrainToFacet(ptr->zones.front(), dim, facets[j], clock);
                    pdbm_updateValueZero(ptr->zones.front(), dim, clock, value, facets[j]);
                }
                pdbm_constrainToFacet(zone, dim, facets[count - 1], clock);
                pdbm_updateValueZero(zone, dim, clock, value, facets[count - 1]);
            } else {
                /* Find and reset upper facets when rate is negative.
                 */
                std::vector<uint32_t> facets(dim);
                int32_t count = pdbm_getUpperRelativeFacets(zone, dim, clock, facets.data());

                assert(count >= 1);
                for (int32_t j = 0; j < count - 1; j++) {
                    ptr->zones.push_front(zone);
                    pdbm_constrainToFacet(ptr->zones.front(), dim, clock, facets[j]);
                    pdbm_updateValueZero(ptr->zones.front(), dim, clock, value, facets[j]);
                }
                pdbm_constrainToFacet(zone, dim, clock, facets[count - 1]);
                pdbm_updateValueZero(zone, dim, clock, value, facets[count - 1]);
            }
        }
    }

    void pfed_t::extrapolateMaxBounds(int32_t* max)
    {
        for (auto& zone : *this)
            pdbm_extrapolateMaxBounds(zone, ptr->dim, max);
    }

    void pfed_t::diagonalExtrapolateMaxBounds(int32_t* max)
    {
        for (auto& zone : *this)
            pdbm_diagonalExtrapolateMaxBounds(zone, ptr->dim, max);
    }

    void pfed_t::diagonalExtrapolateLUBounds(int32_t* lower, int32_t* upper)
    {
        for (auto& zone : *this)
            pdbm_diagonalExtrapolateLUBounds(zone, ptr->dim, lower, upper);
    }

    void pfed_t::incrementCost(int32_t value)
    {
        for (auto& zone : *this)
            pdbm_incrementCost(zone, ptr->dim, value);
    }

    int32_t pfed_t::getCostOfValuation(const valuation_int& valuation) const
    {
        return std::accumulate(cbegin(), cend(), INT_MAX,
                               [dim = ptr->dim, &valuation](int32_t sum, const pdbm_t& pdbm) {
                                   return min(sum, pdbm_getCostOfValuation(pdbm, dim, valuation));
                               });
    }

    void pfed_t::relax()
    {
        for (auto& zone : *this)
            pdbm_relax(zone, ptr->dim);
    }

    void pfed_t::freeClock(cindex_t clock)
    {
        for (auto& zone : *this)
            pdbm_freeClock(zone, ptr->dim, clock);
    }

    void pfed_t::add(const PDBM pdbm, cindex_t dim)
    {
        assert(dim == ptr->dim);
        prepare();
        ptr->zones.push_front(pdbm_t(pdbm, dim));
    }

    void pfed_t::setZero()
    {
        PDBM pdbm = nullptr;
        pdbm_zero(pdbm, ptr->dim);
        *this = pfed_t(pdbm, ptr->dim);
    }

    void pfed_t::setInit()
    {
        PDBM pdbm = nullptr;
        pdbm_init(pdbm, ptr->dim);
        *this = pfed_t(pdbm, ptr->dim);
    }

    void pfed_t::setEmpty() { *this = pfed_t(ptr->dim); }

    void pfed_t::freeUp(cindex_t i)
    {
        for (auto& zone : *this)
            pdbm_freeUp(zone, ptr->dim, i);
    }

    void pfed_t::freeDown(cindex_t i)
    {
        for (auto& zone : *this)
            pdbm_freeDown(zone, ptr->dim, i);
    }

    pfed_t& pfed_t::operator=(const pfed_t& fed)
    {
        if (ptr != fed.ptr) {
            decRef();
            ptr = fed.ptr;
            incRef();
        }
        return *this;
    }

    pfed_t& pfed_t::operator|=(const pfed_t& fed)
    {
        // REVISIT: Eliminate included zones.
        assert(fed.ptr->dim == ptr->dim);
        prepare();
        ptr->zones.insert(ptr->zones.begin(), fed.begin(), fed.end());
        return *this;
    }

    pfed_t& pfed_t::operator|=(const PDBM pdbm)
    {
        // REVISIT: Eliminate included zones.
        add(pdbm, ptr->dim);
        return *this;
    }

    bool pfed_t::intersects(const pfed_t&) const { throw std::logic_error("pfed_t::intersects not implemented"); }

    pfed_t& pfed_t::operator&=(const pfed_t&) { throw std::logic_error("pfed_t::operator &= not implemented"); }

    pfed_t pfed_t::operator&(const pfed_t&) const { throw std::logic_error("pfed_t::operator & not implemented"); }

    pfed_t pfed_t::operator-(const pfed_t&) const { throw std::logic_error("pfed_t::operator - not implemented"); }

    pfed_t& pfed_t::operator-=(const pfed_t&) { throw std::logic_error("pfed_t::operator -= not implemented"); }

    pfed_t& pfed_t::operator+=(const pfed_t&) { throw std::logic_error("pfed_t::operator += not implemented"); }

    void pfed_t::down() { throw std::logic_error("pfed_t::down not implemented"); }

    int32_t pfed_t::getUpperMinimumCost(cindex_t) const
    {
        throw std::logic_error("pfed_t::getUpperMinimumCost not implemented");
    }

    void pfed_t::relaxUp() { throw std::logic_error("pfed_t::relaxUp not implemented"); }

    void pfed_t::getValuation(double*, size_t, bool* freeC) const
    {
        throw std::logic_error("pfed_t::getValuation not implemented");
    }

    void pfed_t::swapClocks(cindex_t, cindex_t) { throw std::logic_error("pfed_t::swapClocks not implemented"); }

    void pfed_t::splitExtrapolate(const constraint_t* begin, const constraint_t* end, const int32_t* max)
    {
        throw std::logic_error("pfed_t::splitExtrapolate not implemented");
    }

    void pfed_t::setDimension(cindex_t) { throw std::logic_error("pfed_t::setDimension not implemented"); }

    raw_t* pfed_t::getDBM() { throw std::logic_error("pfed_t::getDBM not implemented"); }

    const raw_t* pfed_t::const_dbm() const { throw std::logic_error("pfed_t::const_dbm not implemented"); }

    pfed_t& pfed_t::convexHull() { throw std::logic_error("pfed_t::convexHull not implemented"); }

    std::ostream& operator<<(std::ostream& os, const pfed_t& f)
    {
        for (const auto& zone : f)
            pdbm_print(os, zone, f.getDimension());
        return os;
    }
}  // namespace dbm

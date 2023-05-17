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
#include <functional>  // std::bind
#include <numeric>
#include <stdexcept>
#include <float.h>

using std::bind;
using std::for_each;
using std::accumulate;
using std::find_if;
using namespace std::placeholders;  // _1, _2, etc for std::bind

namespace dbm
{
    std::allocator<pfed_t::pfed_s> pfed_t::alloc;

    void pfed_t::decRef()
    {
        if (!--(ptr->count)) {
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

#define for_each__(Z)     for (iterator Z = beginMutable(), Z_ = endMutable(); Z != Z_; zone++)
#define for_each_(Z)      for (iterator Z = beginMutable(), Z_ = endMutable(); Z != Z_;)
#define for_each_const(Z) for (const_iterator Z = begin(), Z_ = end(); Z != Z_; zone++)

    bool pfed_t::constrain(cindex_t i, uint32_t value)
    {
        constraint_t c[2] = {constraint_t(i, 0, dbm_bound2raw(value, dbm_WEAK)),
                             constraint_t(0, i, dbm_bound2raw(-value, dbm_WEAK))};
        return constrain(c, 2);
    }

    bool pfed_t::constrain(cindex_t i, cindex_t j, raw_t constraint)
    {
//        // lambda x . pdbm_constrain1(x, ptr->dim, i, j, constraint)
//        erase_if_not(bind(pdbm_constrain1, _1, ptr->dim, i, j, constraint));

        auto it = beginMutable();
        auto end = endMutable();
        for (; it != end;) {
            int prev_offset[ptr->dim];
            pdbm_getOffset(*it, ptr->dim, prev_offset);

            if (!pdbm_constrain1(*it, ptr->dim, i, j, constraint)) {
                it = erase(it);
                continue;
            }

            int new_offset[ptr->dim];
            pdbm_getOffset(*it, ptr->dim, new_offset);
            for (int x = 1; x < ptr->dim; x++) {
                if (prev_offset[x] != new_offset[x]) {
                    it->cost_plane_operations.emplace_back(CostPlaneOperation::Type::ContinuousOffset, x, CostType(new_offset[x] - prev_offset[x]));
                }
            }
            it++;
        }
        return !isEmpty();
    }

    bool pfed_t::constrain(const constraint_t* constraints, size_t n)
    {
//        erase_if_not(bind(pdbm_constrainN, _1, ptr->dim, constraints, n));

        auto it = beginMutable();
        auto end = endMutable();
        for (; it != end;) {
            int prev_offset[ptr->dim];
            pdbm_getOffset(*it, ptr->dim, prev_offset);

            if (!pdbm_constrainN(*it, ptr->dim, constraints, n)) {
                it = erase(it);
                continue;
            }

            int new_offset[ptr->dim];
            pdbm_getOffset(*it, ptr->dim, new_offset);
            for (int x = 1; x < ptr->dim; x++) {
                if (prev_offset[x] != new_offset[x]) {
                    it->cost_plane_operations.emplace_back(CostPlaneOperation::Type::ContinuousOffset, x, CostType(new_offset[x] - prev_offset[x]));
                }
            }
            it++;
        }
        return !isEmpty();
    }

    static CostType min(CostType a, CostType b) { return a < b ? a : b; }
    static CostType max(CostType a, CostType b) { return a > b ? a : b; }

    CostType pfed_t::getInfimum() const
    {
        CostType inf = INFINITE_COST;
        for (const auto& zone : *this) {
            inf = min(inf, pdbm_getInfimum(zone, ptr->dim));
        }
        return inf;
    }

    CostType pfed_t::getSupremum() const
    {
        CostType sup = -INFINITE_COST;
        for (const auto& zone : *this) {
            sup = max(sup, pdbm_getSupremum(zone, ptr->dim));
        }
        return sup;
    }


    bool pfed_t::canDelay() const {
        for (const auto& zone : *this) {
            if (zone.canDelay()) {
                return true;
            }
        }
        return false;
    }

    void pfed_t::setUniformCost(CostType cost) {
        for (auto it = beginMutable(); it != endMutable(); ++it) {
            it->setUniformCost(cost);
        }
    }

    CostType pfed_t::getInfimumValuation(IntValuation& valuation, const bool* free) const
    {
        uint32_t dim = ptr->dim;
        CostType infimum = INFINITE_COST;
        int32_t copy[dim];

        for_each_const(zone)
        {
            std::copy(valuation.begin(), valuation.end(), copy);
            CostType inf = pdbm_getInfimumValuation(*zone, dim, copy, free);
            if (inf < infimum) {
                infimum = inf;
                std::copy(copy, copy + dim, valuation.begin());
            }
        }
        return infimum;
    }

    bool pfed_t::satisfies(cindex_t i, cindex_t j, raw_t constraint) const
    {
        return find_if(ptr->zones.begin(), ptr->zones.end(), bind(pdbm_satisfies, _1, ptr->dim, i, j, constraint)) != ptr->zones.end();
    }

    bool pfed_t::isUnbounded() const { return find_if(ptr->zones.begin(), ptr->zones.end(), bind(pdbm_isUnbounded, _1, ptr->dim)) != ptr->zones.end(); }

    uint32_t pfed_t::hash(uint32_t seed) const
    {
        /* The binary operator used in the following accumulation is
         * lambda seed zone . pdbm_hash(zone, ptr->dim, seed)
         */
        return accumulate(begin(), end(), seed, bind(pdbm_hash, _2, ptr->dim, _1));
    }

    bool pfed_t::contains(const IntValuation& valuation) const
    {
        return find_if(ptr->zones.begin(), ptr->zones.end(), bind(pdbm_containsInt, _1, ptr->dim, valuation())) != ptr->zones.end();
    }

    bool pfed_t::contains(const DoubleValuation& valuation) const
    {
        return find_if(ptr->zones.begin(), ptr->zones.end(), bind(pdbm_containsDouble, _1, ptr->dim, valuation())) != ptr->zones.end();
    }

    bool pfed_t::containsWeakly(const IntValuation& valuation) const
    {
        return find_if(ptr->zones.begin(), ptr->zones.end(), bind(pdbm_containsIntWeakly, _1, ptr->dim, valuation())) != ptr->zones.end();
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
        bool a_s[size_a];
        bool b_s[size_b];
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

        return (relation_t)((p_a == a_s + size_a ? base_SUBSET : 0) | (p_b == b_s + size_b ? base_SUPERSET : 0));
    }

    relation_t pfed_t::exactRelation(const pfed_t& b) const
    {
        assert(size() == 1 && b.size() == 1);
        return relation(b);
    }

    pfed_t& pfed_t::up() {
        for_each(beginMutable(), endMutable(), bind(pdbm_up, _1, ptr->dim));
        return *this;
    }

    pfed_t& pfed_t::up(CostType rate)
    {
        uint32_t dim = ptr->dim;

        prepare();

        for_each__(zone)
        {
            cindex_t x;
            CostType oldrate = pdbm_getSlopeOfDelayTrajectory(*zone, dim);
            int prev_offset[ptr->dim];
            pdbm_getOffset(*zone, ptr->dim, prev_offset);

//            pdbm_print(std::cout, zone->operator PDBM(), dim);

            auto constrain_and_up = [&](pdbm_t& z, cindex_t i, cindex_t j, cindex_t zero) {
                pdbm_constrainToFacet(z, dim, i, j);
                pdbm_upZero(z, dim, rate, zero);
                int new_offset[ptr->dim];
                pdbm_getOffset(z, ptr->dim, new_offset);
                for (int x = 1; x < ptr->dim; x++) {
                    if (prev_offset[x] != new_offset[x]) {
                        z.cost_plane_operations.emplace_back(CostPlaneOperation::Type::ContinuousOffset, x, CostType(new_offset[x] - prev_offset[x]));
                    }
                }
                z.cost_plane_operations.emplace_back(CostPlaneOperation::Delay, zero, rate);
            };

            if (rate == oldrate) {
                /* This is a simple case which does not require splits.
                 */
                pdbm_up(*zone, dim);
            } else if (pdbm_findZeroCycle(*zone, dim, 0, &x)) {
                /* This is a simple case which does not require splits.
                 */
                pdbm_upZero(*zone, dim, rate, x);
                zone->cost_plane_operations.emplace_back(CostPlaneOperation::Delay, x, rate);
            } else if (rate < oldrate) {
                /* New rate is smaller than old rate, so we delay from the
                 * lower facets
                 */
                uint32_t facets[dim];
                uint32_t count = pdbm_getLowerFacets(*zone, dim, facets);
                assert(count > 0 && count <= dim);
                for (uint32_t j = 0; j < count - 1; j++) {
                    ptr->zones.push_front(*zone);
                    constrain_and_up(ptr->zones.front(), 0, facets[j], facets[j]);
                }
                constrain_and_up(*zone, 0, facets[count - 1], facets[count - 1]);
            } else {
//                assert(rate > oldrate);

                /* New rate is bigger than old rate, so we delay from the
                 * upper facets. We also need the original zone.
                 */
                uint32_t facets[dim];
                uint32_t count = pdbm_getUpperFacets(*zone, dim, facets);
                assert(count > 0 && count <= dim);
                for (uint32_t j = 0; j < count; j++) {
                    ptr->zones.push_front(*zone);
                    constrain_and_up(ptr->zones.front(), facets[j], 0, facets[j]);
                }
                zone->cost_plane_operations.emplace_back(CostPlaneOperation::DelayKeep, 0, rate);
//                pdbm_constrainToFacet(*zone, dim, facets[count - 1], 0);
//                pdbm_upZero(*zone, dim, rate, facets[count - 1]);
            }
        }
        return *this;
    }

    pfed_t& pfed_t::updateValueZero(cindex_t clock, int32_t value, cindex_t zero)
    {
        for_each(beginMutable(), endMutable(), bind(pdbm_updateValueZero, _1, ptr->dim, clock, value, zero));
        return *this;
    }

    pfed_t& pfed_t::updateValue(cindex_t clock, uint32_t value)
    {
        uint32_t dim = ptr->dim;

        prepare();

        for_each__(zone)
        {
            cindex_t k;
            CostType rate = pdbm_getRate(*zone, dim, clock);

            uint32_t facets[dim];
            int prev_offset[ptr->dim];
            pdbm_getOffset(*zone, ptr->dim, prev_offset);

            // Constrain to the facet i - j < m where zero is the clock in the constraint that is not clock; reset clock
            auto constrain_and_reset = [&](pdbm_t& z, uint32_t i, uint32_t j, uint32_t zero) {
                pdbm_constrainToFacet(z, dim, i, j);
                pdbm_updateValueZero(z, dim, clock, value, zero);
                if (0 != zero) {
                    z.cost_plane_operations.emplace_back(CostPlaneOperation::Type::RelativeReset, clock, zero);
                }
                int new_offset[ptr->dim];
                pdbm_getOffset(z, ptr->dim, new_offset);
                for (int x = 1; x < ptr->dim; x++) {
                    if (prev_offset[x] != new_offset[x]) {
                        z.cost_plane_operations.emplace_back(CostPlaneOperation::Type::ContinuousOffset, x, CostType(new_offset[x] - prev_offset[x]));
                    }
                }
            };

            // Remove this because we cannot know which facet was reset
//            if (rate == 0) {
//                pdbm_updateValue(*zone, dim, clock, value);
//            } else
            if (pdbm_findZeroCycle(*zone, dim, clock, &k)) {
                pdbm_updateValueZero(*zone, dim, clock, value, k);
                if (0 != k) {
                    zone->cost_plane_operations.emplace_back(CostPlaneOperation::RelativeReset, clock, k);
                }
            } else if (rate > 0) {
                /* Find and reset lower facets when rate is positive.
                 */
                int32_t count = pdbm_getLowerRelativeFacets(*zone, dim, clock, facets);

                assert(count >= 1);
                for (int32_t j = 0; j < count - 1; j++) {
                    ptr->zones.push_front(*zone);
                    constrain_and_reset(ptr->zones.front(), facets[j], clock, facets[j]);
                }
                constrain_and_reset(*zone, facets[count - 1], clock, facets[count - 1]);
            } else {
                /* Find and reset upper facets when rate is negative.
                 */
                uint32_t facets[dim];
                int32_t count = pdbm_getUpperRelativeFacets(*zone, dim, clock, facets);

                assert(count >= 1);
                for (int32_t j = 0; j < count - 1; j++) {
                    ptr->zones.push_front(*zone);
                    constrain_and_reset(ptr->zones.front(), clock, facets[j], facets[j]);
                }
                constrain_and_reset(*zone, clock, facets[count - 1], facets[count - 1]);
            }
        }
        return *this;
    }

    void pfed_t::extrapolateMaxBounds(int32_t* max)
    {
        for_each(beginMutable(), endMutable(), bind(pdbm_extrapolateMaxBounds, _1, ptr->dim, max));
    }

    void pfed_t::diagonalExtrapolateMaxBounds(int32_t* max)
    {
        for_each(beginMutable(), endMutable(), bind(pdbm_diagonalExtrapolateMaxBounds, _1, ptr->dim, max));
    }

    void pfed_t::diagonalExtrapolateLUBounds(int32_t* lower, int32_t* upper)
    {
        for_each(beginMutable(), endMutable(), bind(pdbm_diagonalExtrapolateLUBounds, _1, ptr->dim, lower, upper));
    }

    void pfed_t::incrementCost(CostType value)
    {
        for (auto& pdbm : ptr->zones) {
            pdbm.cost_plane_operations.emplace_back(CostPlaneOperation::DiscreteOffset, 0, value);
        }
        for_each(beginMutable(), endMutable(), bind(pdbm_incrementCost, _1, ptr->dim, value));
    }

    CostType pfed_t::getCostOfValuation(const IntValuation& valuation) const
    {
        throw std::logic_error("pfed_t::getCostOfValuation not implemented");
    }

    void pfed_t::relax() { for_each(beginMutable(), endMutable(), bind(pdbm_relax, _1, ptr->dim)); }

    void pfed_t::freeClock(cindex_t clock)
    {
        for_each(beginMutable(), endMutable(), bind(pdbm_freeClock, _1, ptr->dim, clock));
    }

    void pfed_t::add(const PDBM pdbm, cindex_t dim)
    {
        assert(dim == ptr->dim);
        prepare();
        ptr->zones.push_front(pdbm_t(pdbm, dim));
    }

    void pfed_t::add(pdbm_t pdbm){
        assert(pdbm.getDimension() == ptr->dim);
        prepare();
        ptr->zones.push_front(pdbm);
    }

    void pfed_t::setZero()
    {
        PDBM pdbm = nullptr;
        pdbm_zero(pdbm, ptr->dim);
        *this = pfed_t(pdbm, ptr->dim);
        pdbm_decRef(pdbm); // We do not reference this, the pdbm_t does now.
    }

    void pfed_t::setInit()
    {
        PDBM pdbm = nullptr;
        pdbm_init(pdbm, ptr->dim);
        *this = pfed_t(pdbm, ptr->dim);
        pdbm_decRef(pdbm); // We do not reference this, the pdbm_t does now.
    }

    void pfed_t::nil()
    {
        setDimension(1);
    }

    void pfed_t::setEmpty() { *this = pfed_t(ptr->dim); }

    void pfed_t::freeUp(cindex_t i) { for_each(beginMutable(), endMutable(), bind(pdbm_freeUp, _1, ptr->dim, i)); }

    void pfed_t::freeDown(cindex_t i) { for_each(beginMutable(), endMutable(), bind(pdbm_freeDown, _1, ptr->dim, i)); }

    pfed_t& pfed_t::operator=(const pfed_t& fed)
    {
        if (ptr != fed.ptr) {
            decRef();
            ptr = fed.ptr;
            incRef();
        }
        return *this;
    }

    fed_t pfed_t::to_fed() const {
        fed_t fed(getDimension());
        for (const pdbm_t& pdbm : *this) {
            fed.add(pdbm.to_dbm());
        }
        return fed;
    }

    pfed_t& pfed_t::operator|=(const pfed_t& pfed)
    {
        // REVISIT: Eliminate included zones.
        assert(pfed.ptr->dim == ptr->dim);
        prepare();
        ptr->zones.insert(ptr->zones.begin(), pfed.ptr->zones.begin(), pfed.ptr->zones.end());
        return *this;
    }

    pfed_t& pfed_t::operator|=(const PDBM pdbm)
    {
        // REVISIT: Eliminate included zones.
        add(pdbm, ptr->dim);
        return *this;
    }

    pfed_t operator|(const pfed_t& a, const pfed_t& b) { return pfed_t(a) |= b; }

    bool pfed_t::intersects(const pfed_t&) const { throw std::logic_error("pfed_t::intersects not implemented"); }

    pfed_t& pfed_t::operator&=(const pfed_t&) { throw std::logic_error("pfed_t::operator &= not implemented"); }

    pfed_t& pfed_t::operator&=(const pdbm_t&) { throw std::logic_error("pfed_t::operator &= not implemented"); }

    pfed_t pfed_t::operator&(const pfed_t&) const { throw std::logic_error("pfed_t::operator & not implemented"); }

    pfed_t pfed_t::operator-(const pfed_t&) const { throw std::logic_error("pfed_t::operator - not implemented"); }

    pfed_t& pfed_t::operator-=(const pfed_t&) { throw std::logic_error("pfed_t::operator -= not implemented"); }

    pfed_t& pfed_t::operator+=(const pfed_t&) { throw std::logic_error("pfed_t::operator += not implemented"); }

    pfed_t&  pfed_t::down() { throw std::logic_error("pfed_t::down not implemented"); }

    void pfed_t::resize(const uint32_t* bitSrc, const uint32_t* bitDst, size_t bitSize, cindex_t* table) { throw std::logic_error("pfed_t::resize not implemented"); }

    bool pfed_t::operator==(const pfed_t& arg) const { return relation(arg) == base_EQUAL; }

    int32_t pfed_t::getUpperMinimumCost(cindex_t) const
    {
        throw std::logic_error("pfed_t::getUpperMinimumCost not implemented");
    }

    pfed_t& pfed_t::relaxUp() { throw std::logic_error("pfed_t::relaxUp not implemented"); }

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

    std::ostream& operator<<(std::ostream& o, const pfed_t& f)
    {
        pfed_t::const_iterator first = f.begin();
        pfed_t::const_iterator last = f.end();
        while (first != last) {
            pdbm_print(o, *first, f.getDimension());
            ++first;
        }
        return o;
    }

    raw_t pfed_t::getMaxUpper(cindex_t clock) const
    {
        throw std::logic_error("pfed_t::getMaxUpper not implemented");
    }

    raw_t pfed_t::getMaxLower(cindex_t clock) const
    {
        throw std::logic_error("pfed_t::getMaxLower not implemented");
    }

    std::string pfed_t::toString(const ClockAccessor& access, bool full) const{
        bool dbmprinted = false;
        bool brackets = size() > 1;
        uint32_t size = getDimension();
        std::stringstream out;
        for (auto z = begin(); z != end(); ++z) {
            bool oneprinted = false;
            if (dbmprinted) {
                out << " or ";
            }
            if (brackets) {
                out << "(";
            }
            for (uint32_t i = 0; i < size; i++) {
                for (uint32_t j = 0; j < size; j++) {
                    raw_t raw = z(i, j);
                    bool strict = dbm_rawIsStrict(raw);
                    int32_t bound = dbm_raw2bound(raw);
                    if (raw < dbm_LS_INFINITY) {
                        if (i != 0u) {
                            if (i != j) {
                                out << (oneprinted ? ", " : "") << access.getClockName(i);
                                if (j > 0) {
                                    out << "-" << access.getClockName(j);
                                }
                                out << (strict ? "<" : "<=") << bound;
                                oneprinted = true;
                            }
                        } else if ((j != 0u) && ((bound != 0) || strict)) {
                            // print for i==0,j!=0,bound!=0
                            out << (oneprinted ? ", " : "") << access.getClockName(j) << (strict ? ">" : ">=")
                                << -bound;
                            oneprinted = true;
                        }
                    }
                }
            }
            oneprinted = false;
            out << " :";
            for (uint32_t i = 1; i < size; ++i) {
                out << (oneprinted? ",": "") << " r(" << access.getClockName(i) << ") = " << pdbm_getRate(z->operator PDBM(), size, i);
                oneprinted = true;
            }
            if (brackets) {
                out << ")";
            }
            dbmprinted = true;
        }
        return out.str();
    }

    void pfed_t::updateClock(cindex_t x, cindex_t y){
        throw std::logic_error("pfed_t::updateClock not implemented");
    }

    pfed_t& pfed_t::upStop(const uint32_t* stopped){
        throw std::logic_error("pfed_t::upStop not implemented");
    }

    pfed_t& pfed_t::append(pfed_t& arg){
        throw std::logic_error("pfed_t::append not implemented");
    }

    pfed_t& pfed_t::steal(pfed_t& arg){
        throw std::logic_error("pfed_t::steal not implemented");
    }

    pfed_t& pfed_t::toLowerBounds() const {
        throw std::logic_error("pfed_t::toLowerBounds not implemented");
    }

    pfed_t& pfed_t::toUpperBounds() const {
        throw std::logic_error("pfed_t::toUpperBounds not implemented");
    }

    bool pfed_t::isConstrainedBy(cindex_t, cindex_t, raw_t) const {
        throw std::logic_error("pfed_t::isConstrainedBy not implemented");
    }
    bool pfed_t::getDelay(const double* point, cindex_t dim, double* min, double* max, double* minVal, bool* minStrict,
                          double* maxVal, bool* maxStrict, const uint32_t* stopped) const
    {
        throw std::logic_error("pfed_t::getDelay not implemented");
    }

    bool pdbm_t::constrain(cindex_t i, cindex_t j, raw_t c) {
        if (!isEmpty()) {
            return pdbm_constrain1(pdbm, dim, i, j, c);
        }
        return false;
    }

    void pdbm_t::setUniformCost(CostType cost) {
        if (!isEmpty()) {
            pdbm_setUniformCost(pdbm, dim, cost);
        }
    }

    bool pdbm_t::isEmpty() const {
        return pdbm_isEmpty(pdbm, dim);
    }

    bool pfed_t::le(const pfed_t& arg) const {
        throw std::logic_error("pfed_t::le not implemented");
    }

    bool pfed_t::lt(const pfed_t& arg) const {
        throw std::logic_error("pfed_t::lt not implemented");
    }

    bool pfed_t::gt(const pfed_t& arg) const {
        throw std::logic_error("pfed_t::gt not implemented");
    }

    bool pfed_t::ge(const pfed_t& arg) const {
        throw std::logic_error("pfed_t::ge not implemented");
    }

    bool pfed_t::eq(const pfed_t& arg) const {
        throw std::logic_error("pfed_t::eq not implemented");
    }

    bool pfed_t::unionWith(const pfed_t& other) {
        assert(ptr->dim == other.ptr->dim);
        bool changed = false;

        for (auto ot = other.begin(); ot != other.end(); ++ot) {
            bool maybe_insert = true;
            bool definitely_insert = false;
            for (auto it = beginMutable(); it != endMutable();) {
                switch (pdbm_relation(*ot, *it, ptr->dim)) {
                case base_SUPERSET:
                    it.remove();
                    definitely_insert = true;
                    break;
                case base_SUBSET:
                case base_EQUAL:
                    maybe_insert = false;
                case base_DIFFERENT:
                    ++it;
                    break;
                }
            }
            if (definitely_insert || maybe_insert) {
                add(*ot);
                changed = true;
            }
        }
        return changed;
    }


    int32_t pfed_t::maxOnZero(cindex_t x){
        throw std::logic_error("pfed_t::maxOnZero not implemented");
    }

    pfed_t& pfed_t::downStop(const uint32_t* stopped){
        throw std::logic_error("pfed_t::downStop not implemented");
    }

    bool pdbm_t::contains(const int32_t* point, cindex_t dim) const{
        throw std::logic_error("pfed_t::contains not implemented");
    }

    bool pdbm_t::contains(const double* point, cindex_t dim) const{
        throw std::logic_error("pfed_t::contains not implemented");
    }

    std::string pdbm_t::toString(const ClockAccessor&, bool full) const
    {
        std::stringstream ss;
        pdbm_print(ss, *this, dim);
        return ss.str();
    }

    bool pfed_t::hasZero() const{
        throw std::logic_error("pfed_t::hasZero not implemented");
    }

    void pfed_t::swap(pfed_t& arg){
        throw std::logic_error("pfed_t::swap not implemented");
    }

    bool pfed_t::contains(const int32_t* point, cindex_t dim) const{
        throw std::logic_error("pfed_t::contains not implemented");
    }

    bool pfed_t::contains(const double * point, cindex_t dim) const{
        throw std::logic_error("pfed_t::contains not implemented");
    }

    pfed_t& pfed_t::predt(const pfed_t& bad, const raw_t* restrict){
        throw std::logic_error("pfed_t::predt not implemented");
    }

    void pfed_t::intern(){
        throw std::logic_error("pfed_t::intern not implemented");
    }

    pfed_t& pfed_t::mergeReduce(size_t skip, int expensiveTry){
        throw std::logic_error("pfed_t::mergeReduce not implemented");
    }
    void pfed_t::simplify_rational_cost() {
        for (auto it = beginMutable(); it != endMutable(); ++it) {
            it->simplify_rational_cost();
        }
    }

    cindex_t pdbm_t::pdim() const { return dim; }


#define DBM_IJ(DBM, I, J) DBM[(I)*dim + (J)]
#define DBM(I, J)         DBM_IJ(dbm, I, J)

    bool pdbm_t::canDelay() const {
        if (isEmpty())
            return false;
        const raw_t* dbm = const_dbm();
        cindex_t dim = pdim();
        for (cindex_t i = 1; i < dim; ++i) {
            if (dbm_rawIsWeak(DBM(i, 0)) && dbm_rawIsWeak(DBM(0, i)) && DBM(i, 0) == dbm_weakNegRaw(DBM(0, i))) {
                return false;
            }
        }
        return true;
    }
    relation_t pdbm_t::relation(const pdbm_t& other) const {
        assert(dim == other.dim);
        return pdbm_relation(pdbm, other.pdbm, dim);
    }

    relation_t pdbm_t::strict_relation(const pdbm_t& other) const {
        assert(dim == other.dim);
        return pdbm_relation_strict(pdbm, other.pdbm, dim);
    }

    pdbm_t& pdbm_t::operator&=(const dbm::pdbm_t& arg) {
        assert(getDimension() == arg.getDimension());
        if (arg.isEmpty()) {
            pdbm_decRef(pdbm);
            pdbm = nullptr;
        } else if (!isEmpty()) {
            pdbm_intersect(pdbm, arg.pdbm, dim);
        }
        return *this;
    }
    void pdbm_t::simplify_rational_cost() {
        if (!isEmpty()) {
            pdbm_simplify_rational_cost(pdbm, dim);
        }
    }
}  // namespace dbm


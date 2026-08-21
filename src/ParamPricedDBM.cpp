/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "dbm/ParamPricedDBM.h"

#include <debug/macros.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <memory>
#include <tuple>

namespace PPDBM
{
    /**
     *
     * @param i
     * @param j
     */
    constexpr void DBMatrix::close(uint32_t i, uint32_t j)
    {
        const auto dim = getDim();
        const auto dij = _matrix(i, j);

        for (uint32_t a = 0; a < dim; ++a) {
            for (uint32_t b = 0; b < dim; ++b) {
                auto candidate = _matrix(a, i) + dij + _matrix(j, b);
                if (candidate.dominates(_matrix(a, b))) {
                    _matrix(a, b) = candidate;
                }
            }
        }
    }


    /**
     * resets a DBM to its unrestrained state.
     */
    constexpr void Ppdbm::reset()
    {
        _dbm.reset();
        std::fill(_offsetCost.begin(), _offsetCost.end(), 0);
        _rates.fill(0);
        _PC.constraints.clear();
        _emptyZone = false;
    }

    /**
     *
     */
    constexpr void Ppdbm::removeCostAtOffset()
    {
        for (uint32_t h = 1; h < _dim; h++) {
            for (uint32_t k = 0; k <= _param; k++) {
                _offsetCost[k] -= _rates(h,k) * (-_dbm(0, h).getValue());
            }
        }
    }

    /**
     *
     */
    constexpr void Ppdbm::addCostAtOffset()
    {
        for (uint32_t h = 1; h < _dim; h++) {
            for (uint32_t k = 0; k <= _param; k++) {
                _offsetCost[k] += _rates(h,k) * (-_dbm(0, h).getValue());
            }
        }
    }

    /**
     *
     * @param i
     * @return
     */
    constexpr bool Ppdbm::isDiagonalNegative(uint32_t i) const
    {
        const auto& d = _dbm(i, i);
        return d.getValue() < 0 || (d.getValue() == 0 && d.getStrict());
    }

    /**
     *
     * @param i
     * @param j
     * @param constraint
     * @return
     */
    constexpr bool Ppdbm::constrain_DBM(uint32_t i, uint32_t j, DbmBound constraint)
    {
        assert(i < _dim && j < _dim);

        // The constraint is useless?
        if (_dbm(i,j).dominates(constraint)) { return true; }

        // The constraint empties the zone?
        if (DbmBound::isIncompatible(constraint, _dbm(j,i))) {
            _dbm(i, j) = constraint;
            _emptyZone = true;
            return false;
        }

        // remove the cost at the previous offset
        removeCostAtOffset();

        // Add the constraint and close the DBM
        _dbm(i, j) = constraint;
        _dbm.close(i, j);
        if (isDiagonalNegative(i)) { _emptyZone = true; return false; }

        // Add the cost at the new offset
        addCostAtOffset();

        return true;
    }

    /**
     *
     * @param constraints
     * @return
     */
    constexpr bool Ppdbm::constrain_DBM_N(const std::vector<DbmConstraint>& constraints)
    {
        assert(!constraints.empty());

        // remove the cost at the previous offset
        removeCostAtOffset();

        bool empty = false;
        for (auto it = constraints.begin(); it != constraints.end() && !empty; ++it) {
            const auto& [i, j, bound] = *it;

            if (_dbm(i,j).dominates(bound)) { continue; } // useless constraint

            if (DbmBound::isIncompatible(bound, _dbm(j,i))) {
                _dbm(i, j) = bound;
                empty = true;
                break;
            } // constraint empties the zone

            _dbm(i, j) = bound;
            _dbm.close(i, j);
            empty = isDiagonalNegative(i);
        }

        // add the cost of the new offset
        addCostAtOffset();

        if (empty) { _emptyZone = true; return false; }
        return true;
    }

    /**
     * cost function of another DBM's offset calculated with actual rates.
     * @param otherDbm
     * @return
     */
    constexpr std::vector<uint32_t> Ppdbm::costAtOtherOffset(const DBMatrix& otherDbm) const
    {
        std::vector<uint32_t> cost = _offsetCost;

        for (uint32_t x = 1; x < _dim; ++x) {
            // difference in x coordinate of the offset
            const int32_t offsetDifference = -otherDbm(0, x).getValue() - (-_dbm(0, x).getValue());

            // multiply the difference by the associated rate and add it to the cost
            for (uint32_t k = 0; k <= _param; ++k) {
                cost[k] += _rates(x, k) * offsetDifference;
            }
        }

        return cost;
    }













    ///////////////////////////////////////////////////attempt at relation/////////////////////////////////////////////

    relation_t DBMatrix::relation(const DBMatrix& other) const
    {
        assert(getDim() == other.getDim());
        const auto dim = getDim();

        bool oneLeqTwo = true; // this <= other partout -> this subset of other
        bool twoLeqOne = true; // other <= this partout -> other subset of this

        for (uint32_t i = 0; i < dim; ++i) {
            for (uint32_t j = 0; j < dim; ++j) {
                if (!(*this)(i, j).dominates(other(i, j))) { oneLeqTwo = false; }
                if (!other(i, j).dominates((*this)(i, j))) { twoLeqOne = false; }
            }
        }

        if (oneLeqTwo && twoLeqOne) { return base_EQUAL; }
        if (oneLeqTwo) { return base_SUBSET; }
        if (twoLeqOne) { return base_SUPERSET; }
        return base_DIFFERENT;
    }

    CostVector Ppdbm::costAtVertex(const Vertex& v) const
    {
        assert(v.size() == _dim);

        CostVector cost = _offsetCost; // copie: coeffs a l'offset
        for (uint32_t h = 1; h < _dim; ++h) {
            const int32_t offsetH = -_dbm(0, h).getValue(); // valeur min de l'horloge h
            const int32_t delta = v[h] - offsetH;
            for (uint32_t k = 0; k <= _param; ++k) {
                cost[k] += _rates(h, k) * delta;
            }
        }
        return cost;
    }

    bool Ppdbm::dominatesVector(const CostVector& a, const CostVector& b)
    {
        assert(a.size() == b.size());
        for (size_t k = 0; k < a.size(); ++k) {
            if (a[k] > b[k]) { return false; }
        }
        return true;
    }

    bool Ppdbm::equalVector(const CostVector& a, const CostVector& b)
    {
        return a == b;
    }

    relation_t Ppdbm::compareCostLists(const std::vector<CostVector>& c1,
                                        const std::vector<CostVector>& c2) const
    {
        assert(c1.size() == c2.size());

        bool oneLeqTwo = true; // cout de *this* <= cout de other, sur tous les sommets
        bool twoLeqOne = true;

        for (size_t v = 0; v < c1.size(); ++v) {
            if (!dominatesVector(c1[v], c2[v])) { oneLeqTwo = false; }
            if (!dominatesVector(c2[v], c1[v])) { twoLeqOne = false; }
            if (!oneLeqTwo && !twoLeqOne) { break; } // court-circuit: plus rien a decider
        }

        if (oneLeqTwo && twoLeqOne) { return base_EQUAL; }
        if (oneLeqTwo) { return base_SUPERSET; } // *this* moins cher partout -> domine
        if (twoLeqOne) { return base_SUBSET; }
        return base_DIFFERENT; // pas de domination (cf. ta preuve, on ne coupe pas la branche)
    }

    relation_t Ppdbm::relation(const Ppdbm& other) const
    {
        assert(_dim == other._dim && _param == other._param);

        switch (_dbm.relation(other._dbm)) {
        case base_DIFFERENT:
            return base_DIFFERENT;

        case base_EQUAL: {
            auto vertices = sommetsDBM(_dbm); // meme zone des deux cotes
            std::vector<CostVector> c1, c2;
            c1.reserve(vertices.size());
            c2.reserve(vertices.size());
            for (const auto& v : vertices) {
                c1.push_back(costAtVertex(v));
                c2.push_back(other.costAtVertex(v));
            }
            return compareCostLists(c1, c2);
        }

        case base_SUPERSET: {
            // *this* est la zone la plus grande, other la plus petite.
            auto vertices = sommetsDBM(other._dbm);
            std::vector<CostVector> c1, c2;
            c1.reserve(vertices.size());
            c2.reserve(vertices.size());
            for (const auto& v : vertices) {
                c1.push_back(costAtVertex(v));
                c2.push_back(other.costAtVertex(v));
            }
            auto costRel = compareCostLists(c1, c2);
            return (costRel == base_SUPERSET) ? base_SUPERSET : base_DIFFERENT;
        }

        case base_SUBSET: {
            // other est la zone la plus grande, *this* la plus petite.
            auto vertices = sommetsDBM(_dbm);
            std::vector<CostVector> c1, c2;
            c1.reserve(vertices.size());
            c2.reserve(vertices.size());
            for (const auto& v : vertices) {
                c1.push_back(costAtVertex(v));
                c2.push_back(other.costAtVertex(v));
            }
            auto costRel = compareCostLists(c1, c2);
            return (costRel == base_SUBSET) ? base_SUBSET : base_DIFFERENT;
        }

        default:
            return base_DIFFERENT;
        }
    }
} // namespace PPBDM




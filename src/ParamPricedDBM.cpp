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


    /**
     * compares each constraints, and if every constraint of one dominates the other, then subset.
     * @param other
     * @return
     */
    relation_t DBMatrix::relation(const DBMatrix& other) const
    {
        assert(getDim() == other.getDim());
        const auto dim = getDim();

        bool thisLeqOther = true; // this <= other everywhere -> this subset of other
        bool otherLeqThis = true; // other <= this everywhere -> other subset of this

        for (uint32_t i = 0; i < dim; ++i) {
            for (uint32_t j = 0; j < dim; ++j) {
                if (!(*this)(i, j).dominates(other(i, j))) { thisLeqOther = false; }
                if (!other(i, j).dominates((*this)(i, j))) { otherLeqThis = false; }
            }
        }

        if (thisLeqOther && otherLeqThis) { return base_EQUAL; }
        if (thisLeqOther) { return base_SUBSET; }
        if (otherLeqThis) { return base_SUPERSET; }
        return base_DIFFERENT;
    }

    /**
     * computes the coefficients of the parameters in the cost function a the vertex v
     * @param v
     * @return
     */
    CostVector Ppdbm::costAtVertex(const Vertex& v) const
    {
        assert(v.size() == _dim);

        CostVector cost = _offsetCost; // copy: offset coeffs
        for (uint32_t x = 1; x < _dim; ++x) {
            const int32_t offsetX = -_dbm(0, x).getValue(); // min value of clock h
            const int32_t delta = v[x] - offsetX;
            for (uint32_t k = 0; k <= _param; ++k) {
                cost[k] += _rates(x, k) * delta;
            }
        }
        return cost;
    }

    /**
     * is every coefficient of a lesser or equal than every coefficient ob b?
     * @param a
     * @param b
     * @return
     */
    bool Ppdbm::dominatesVector(const CostVector& a, const CostVector& b)
    {
        assert(a.size() == b.size());
        for (uint32_t k = 0; k < a.size(); ++k) {
            if (a[k] > b[k]) { return false; }
        }
        return true;
    }

    /**
     * vector equality test
     * @param a
     * @param b
     * @return
     */
    bool Ppdbm::equalVector(const CostVector& a, const CostVector& b)
    {
        return a == b;
    }

    /**
     * Compares both versions of every cost function of a summit. If One DBM has lower coefficients everywhere,
     * then better cost functions and domination.
     * @param c1
     * @param c2
     * @return
     */
    relation_t Ppdbm::compareCostLists(const std::vector<CostVector>& c1, const std::vector<CostVector>& c2) const
    {
        assert(c1.size() == c2.size());

        bool thisLeqOther = true; // cost of *this* <= cost of other, on every summit
        bool otherLeqThis = true; // cost of other <= cost of *this*, on every summit

        for (uint32_t v = 0; v < c1.size(); ++v) {
            if (!dominatesVector(c1[v], c2[v])) { thisLeqOther = false; }
            if (!dominatesVector(c2[v], c1[v])) { otherLeqThis = false; }
            if (!thisLeqOther && !otherLeqThis) { break; } // shortcut
        }

        if (thisLeqOther && otherLeqThis) { return base_EQUAL; }
        if (thisLeqOther) { return base_SUPERSET; } // *this* cheaper everywhere -> dominates
        if (otherLeqThis) { return base_SUBSET; }
        return base_DIFFERENT; // no domination --> no branch cutting
    }

    /**
     * most important function, decides when the loops stop / when we cut branches of the exploration graph.
     * is one or the other DBM not interesting to consider? meaning is there a zone who is a subset of the other and
     * with less interesting cost functions ? (a zone already covered basically?)
     * @param other
     * @return
     */
    relation_t Ppdbm::relation(const Ppdbm& other) const
    {
        assert(_dim == other._dim && _param == other._param);

        switch (_dbm.relation(other._dbm)) {
        case base_DIFFERENT:
            return base_DIFFERENT;

        case base_EQUAL: {
            auto vertices = sommetsDBM(_dbm); // same zone on both sides
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
            // *this* is the bigger zone, other is the smaller
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
            // other is the bigger zone, *this* is the smaller:
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




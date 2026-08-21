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
    void DBMatrix::close(uint32_t i, uint32_t j)
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
    void Ppdbm::reset()
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
    void Ppdbm::removeCostAtOffset()
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
    void Ppdbm::addCostAtOffset()
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
    bool Ppdbm::isDiagonalNegative(uint32_t i) const
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
    bool Ppdbm::constrain_DBM(uint32_t i, uint32_t j, DbmBound constraint)
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
    bool Ppdbm::constrain_DBM_N(const std::vector<DbmConstraint>& constraints)
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
    std::vector<uint32_t> Ppdbm::costAtOtherOffset(const DBMatrix& otherDbm) const
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
} // namespace PPBDM




/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "dbm/ParamPricedDBM.h"

#include <debug/macros.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <memory>

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
    if (DbmBound::isIncompatible(constraint,_dbm(j,i))) {
        _dbm(i, j) = constraint;
        _emptyZone = true;
        return false;
    }

    // remove the cost at the previous offset
    for (uint32_t h = 1; h < _dim; h++) { // for every clock,
        for (uint32_t k = 0; k <= _param; k++) { // for every parameter,
            _offsetCost[k] -= _rates(h,k) * (-_dbm(0, h).getValue()); // x_0 - x_k <= value so x_k >= -value
        }
    }

    // Add the constraint and close the DBM
    _dbm(i, j) = constraint;
    //_dbm.close(i,j);

    // Add the cost at the new offset
    for (uint32_t h = 1; h < _dim; h++) { // for every clock,
        for (uint32_t k = 0; k <= _param; k++) { // for every parameter,
            _offsetCost[k] += _rates(h,k) * (-_dbm(0, h).getValue()); // x_0 - x_k <= value so x_k >= -value
        }
    }

    return true;
}




/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include "../include/dbm/ParamPricedDBM.h"

#include <debug/macros.h>

#include <algorithm>  // transform
#include <functional>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <memory>

/**
 * constructs an unrestrained matrix
 * @param dim
 * @param param
 */
Ppdbm::Ppdbm(int dim, int param)
    : dim(dim),
      param(param),
      DBM(dim),
      offsetCost(param + 1),
      rates(dim, std::vector<int>(param + 1))
{}

/**
 * resets a DBM to its unrestrained state.
 */
void Ppdbm::reset()
{
    std::fill(DBM.data.begin(), DBM.data.end(), INF_BOUND);
    for (int i = 0; i < dim; ++i) {
        DBM(i,i)= DIAG_BOUND;
    }

    std::fill(offsetCost.begin(), offsetCost.end(), 0);

    for (auto& rate : rates) {
        std::fill(rate.begin(), rate.end(), 0);
    }

    PC.constraints.clear();
}

/**
 *
 * @param i
 * @param j
 * @param constraint
 * @return
 */
bool Ppdbm::constrain_DBM(int i, int j, DbmBound constraint)
{
    assert(i >= 0 && i < dim);
    assert(j >= 0 && j < dim);

    // The constraint is useless?
    if (constraint.isLooserThan(DBM(i,j))) {
        return true;
    }

    // The constraint empties the zone?
    if (constraint.negate().isLooserThan(DBM(j, i))) {
        DBM(i, j) = constraint;
        emptyZone = true;
        return false;
    }

    // remove the cost at the previous offset
    for (int h = 1; h < dim; h++) { // for every clock,
        for (int k = 0; k <= param; k++) { // for every parameter,
            offsetCost[k] -= rates[h][k] * (-DBM(0, h).value); // x_0 - x_k <= value so x_k >= -value
        }
    }

    // Add the constraint and close the DBM
    DBM(i, j) = constraint;
    DBM.close(i,j);

    // Add the cost at the new offset
    for (int h = 1; h < dim; h++) { // for every clock,
        for (int k = 0; k <= param; k++) { // for every parameter,
            offsetCost[k] += rates[h][k] * (-DBM(0, h).value); // x_0 - x_k <= value so x_k >= -value
        }
    }

    return true;
}




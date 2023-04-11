//
// Created by ragusa on 3/17/23.
//

#ifndef UDBM_COST_TYPE_H
#define UDBM_COST_TYPE_H

#include <boost/rational.hpp>

/**
 * The type used to represent cost in the pdbm
 */
typedef boost::rational<int> CostType;
const CostType INFINITE_COST = (CostType)INT_MAX;

uint32_t hash_cost_type(const CostType& val);

#endif  // UDBM_COST_TYPE_H

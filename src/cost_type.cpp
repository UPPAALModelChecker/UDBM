//
// Created by ragusa on 4/5/23.
//

#include "dbm/cost_type.h"

uint32_t hash_cost_type(const CostType& val) {
    return ((uint32_t)val.numerator() << 16) ^ (uint32_t)val.denominator();
}

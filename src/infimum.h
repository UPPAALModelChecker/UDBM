/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2005, Uppsala University and Aalborg University.
 * All right reserved.
 *
 **********************************************************************/

#ifndef DBM_INFIMUM_H
#define DBM_INFIMUM_H

#include "dbm/constraints.h"

#include <cmath>
#include "dbm/cost_type.h"


CostType pdbm_infimum(const raw_t* dbm, uint32_t dim, CostType offsetCost, const CostType* rates);
void pdbm_infimum(const raw_t* dbm, uint32_t dim, CostType offsetCost, const CostType* rates, int32_t* valuation);

#endif

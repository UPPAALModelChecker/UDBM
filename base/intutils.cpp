/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : intutils.c (base)
 *
 * Utility functions for integers.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: intutils.c,v 1.8 2005/04/22 15:20:08 adavid Exp $
 *
 *********************************************************************/

#include "base/intutils.h"
#include "debug/macros.h"

#include <cassert>
#include <numeric>
#include <algorithm>

/* Optimistic implementation of equality: accumulate
 * a difference and test it at the end.
 */
uint32_t base_diff(const void* data, size_t n, uint32_t mask)
{
    const uint32_t* a = (uint32_t*)data;
    return std::accumulate(a, a + n, 0,
                           [mask](auto sum, auto value) { return sum | (value ^ mask); });
}

/* Semi-optimistic implementation of equality testing.
 */
bool base_areEqual(const void* data1, const void* data2, size_t n)
{
    if (data1 == data2) {
        return true;
    } else if (data1 == nullptr || data2 == nullptr) {
        return false;  // since different pointers
    } else {
        const uint32_t* a = (uint32_t*)data1;
        const uint32_t* b = (uint32_t*)data2;
        return std::equal(a, a + n, b);
    }
}

void base_fill(int32_t* from, int32_t* to, uint32_t intval) { std::fill(from, to, intval); }

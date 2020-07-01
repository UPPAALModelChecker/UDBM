/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; -*-
 *
 * This file is part of the UPPAAL DBM library.
 *
 * The UPPAAL DBM library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * The UPPAAL DBM library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the UPPAAL DBM library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA.
 */

// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
////////////////////////////////////////////////////////////////////
//
// Filename : teststreamhasher.cpp (hash/tests)
//
// Test compute.h
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// Not reviewed.
// $Id: teststreamhasher.cpp,v 1.2 2004/02/27 16:36:53 adavid Exp $
//
/////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "hash/StreamHasher.h"

using namespace hash;

static
uint32_t test_hasher(int32_t *data, uint32_t n, uint32_t initVal)
{
    StreamHasher hasher(initVal);
    uint32_t j;
    for(j = 0; j < n; ++j) hasher.addValue(data[j]);
    return hasher.hash();
}

#ifdef NDEBUG
#undef NDEBUG
#endif

static
void test(int i)
{
    /* no stack allocation to avoid potential stack overflow
     */
    int32_t *dat32 = (int32_t*)malloc(i*sizeof(int32_t));
    int j;
    uint32_t h32, hh32;

    for(j = 0 ; j < i ; ++j)
    {
        dat32[j] = rand();
    }

    h32 = 0;
    for(j = 1 ; j < i ; ++j)
    {
        hh32 = test_hasher(dat32, j, 0);
        assert(h32 != hh32);
        h32 = hh32;
    }

    free(dat32);
}

int main(int argc, char **argv)
{
    uint32_t seed = argc < 2 ? time(NULL) : atoi(argv[1]);

    printf("Testing with seed=%u\n", seed);
    srand(seed);

    for(uint32_t i = 0 ; i < 2048 ; ++i) test(i);

    printf("Passed\n");
    return 0;
}






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

/* -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*********************************************************************
 *
 * Filename : testdbm.c (dbm/tests)
 *
 * Test the DBM basic API.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: testdbm.c,v 1.35 2005/06/19 17:22:53 adavid Exp $
 *
 *********************************************************************/

/* Tests are always for debugging. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdlib.h>
#include "base/bitstring.h"
#include "base/Timer.h"
#include "dbm/priced.h"
#include "../infimum.h" // This should not be here.
#include "dbm/gen.h"
#include "dbm/print.h"
#include "debug/macros.h"

/* Inner repeat loop.
 */
#define LOOP 1000

/* print diff
 */
#define DIFF(D1,D2) dbm_printDiff(stderr, D1, D2, size)

/* Allocation of DBM, vector, bitstring
 */
#define ADBM(NAME) PDBM NAME = pdbm_allocate(size);

/* Random range
 * may change definition
 */
#define RANGE() ((rand()%10000)+10)

/* Show progress
 */
#define PROGRESS() debug_spin(stderr)

static uint32_t allDBMs = 0;
static uint32_t goodDBMs = 0;
static double time_infimum = 0;

static void generate(PDBM pdbm, uint32_t size)
{
    BOOL good = dbm_generate(pdbm_getMutableMatrix(pdbm, size), size, RANGE());
    allDBMs++;
    goodDBMs += good;
    for (uint32_t i = 1; i < size; i++) 
    {
        int32_t rate;
        if (pdbm_getMatrix(pdbm, size)[i * size] == dbm_LS_INFINITY)
        {
            rate = rand() % 100;
        }
        else
        {
            rate = rand() % 200;
            rate = (rate & 0x1 ? rate : -rate) / 2;
        }
        pdbm_setRate(pdbm, size, i, rate);
    }
    ASSERT(pdbm_isValid(pdbm, size), pdbm_print(stdout, pdbm, size));
}

static void benchmark_infimum(uint32_t size)
{
    ADBM(dbm1);
    uint32_t k;

    for (k = 0; k < LOOP; ++k)
    {
        PROGRESS();

        generate(dbm1, size);

        base::Timer timer;
        for (uint32_t l = 0; l < 100; l++)
        {
              pdbm_infimum(pdbm_getMatrix(dbm1, size), size, 0, pdbm_getRates(dbm1, size));
        }
        time_infimum += timer.getElapsed() / 100.0;
        assert(pdbm_isValid(dbm1, size));
    }

    free(dbm1);
}

int main(int argc, char *argv[])
{
    uint32_t i,start,end;
    uint32_t seed;

    if (argc < 3)
    {
        fprintf(stderr,"Usage: %s start_size end_size [seed]\n",argv[0]);
        return 1;
    }

    start = atoi(argv[1]);
    end = atoi(argv[2]);
    if (start < 1)
    {
        fprintf(stderr, "Minimum dimension 1\n");
        return 2;
    }
    seed = argc > 3 ? atoi(argv[3]) : time(NULL);
    printf("Testing with seed=%u\n", seed);
    srand(seed);

    for(i = start ; i <= end ; ++i) /* min dim = 1 */
    {
        fprintf(stderr, "Testing size %u\t", i);
        double start = time_infimum;
        benchmark_infimum(i);
        printf("Time: %.3fs\n", time_infimum - start);        
    }

    assert(allDBMs);
    printf("Total time: %.3fs\n", time_infimum);
    printf("Total generated DBMs: %u, non trivial ones: %u (%u%%)\n",
           allDBMs, goodDBMs, (100*goodDBMs)/allDBMs);

    printf("Passed\n");
    return 0;
}

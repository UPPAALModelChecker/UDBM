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
 * Filename : testmingraph.c (dbm/tests)
 *
 * Test API of mingraph.h
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: testmingraph.c,v 1.15 2005/05/11 19:08:14 adavid Exp $
 *
 *********************************************************************/

/* Tests are always for debugging. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "base/bitstring.h"
#include "dbm/mingraph.h"
#include "dbm/dbm.h"
#include "dbm/print.h"
#include "dbm/gen.h"
#include "debug/macros.h"

/* number of loops per size tested
 */
#define LOOPS 5000

/* print diff
 */
#define DIFF(D1,D2) dbm_printDiff(stdout, D1, D2, dim)

/* Cool assert
 */
#define DBM_EQUAL(D1,D2) \
ASSERT(dbm_areEqual(D1,D2,dim), DIFF(D1,D2))

/* Show progress
 */
#define PROGRESS() debug_spin(stderr)

/* easy endl
 */
#define ENDL printf(" \n")


/* Special allocator function for testing
 * purposes:
 * - allocates asked memory
 * - stores size of allocated memory for
 *   later check
 * - mark memory with magic numbers to check
 *   for overflow
 *
 * Idea:
 * allocate size + 3:
 * [size][pointer][free memory for use][pointer]
 * when deallocating, we check that memory before
 * and after is not touched.
 */
static
int32_t* test_alloc(uint32_t size, void *data)
{
    uint32_t *sizePtr = (uint32_t*) data;
    int32_t *mem = (int32_t*)
        malloc((size+3)*sizeof(int32_t)) ;
    assert(data);
    *sizePtr = size;

    /* magic numbers */
    mem[0] = (int32_t) size;
    mem[1] = (int32_t) (mem+2);
    mem[size+2] = (int32_t) (mem+2);

    return mem+2;
}

/* Free memory allocated by test_alloc
 * test for untouched memory area
 */
static
void test_free(int32_t *data)
{
    int32_t magic = (int32_t) data;
    uint32_t size = data[-2];

    assert(data[-1] == magic);
    assert(data[size] == magic);

    free(data-2);
}


/* allocate a DBM
 */
static
raw_t* allocDBM(uint32_t dim)
{
    return (raw_t*) malloc(dim*dim*sizeof(raw_t));
}


/* Pretty print of the stats
 */
static
void test_printStats(int32_t *stats, uint32_t *sizes, uint32_t dim)
{
    static const char *statNames[] =
    {
        "Trivial    ",
        "Copy32     ",
        "BitMatrix32",
        "Couplesij32",
        "Copy16     ",
        "BitMatrix16",
        "Couplesij16",
        "ERROR      "
    };

    uint32_t i, totalFull = 0, totalReduced = 0;
    for(i = 0; i < dbm_MINDBM_ERROR ; ++i)
    {
        int32_t n = stats[i];
        printf("%s: %d\tconsumed %u ints vs DBM(%u) DBM-diago(%u)\n",
               statNames[i],
               n,
               sizes[i],
               n*dim*dim+n, /* +n: to save dim */
               n*dim*(dim-1)+n);
        totalFull += n*dim*dim+n;
        totalReduced += sizes[i];
    }
    assert(stats[dbm_MINDBM_ERROR] == 0);
    printf("Total: consumed %u ints vs DBM(%u)\n",
           totalReduced, totalFull);
}

static
void test(int dim, BOOL tryBest)
{
    raw_t *dbm1 = allocDBM(dim);
    raw_t *dbm2 = allocDBM(dim);
    raw_t *dbm3 = allocDBM(dim);
    uint32_t k;
    int32_t stats[dbm_MINDBM_ERROR+1];
    uint32_t sizes[dbm_MINDBM_ERROR+1];
    size_t testSize = bits2intsize(dim*dim), nbCons1, nbCons2;
    uint32_t testMG1[testSize], testMG2[testSize];

    printf("** Testing size=%d **\n", dim);

    base_resetSmall(stats, dbm_MINDBM_ERROR+1);
    base_resetSmall(sizes, dbm_MINDBM_ERROR+1);

    for(k = 0; k < LOOPS; ++k)
    {
        /* test 16/32 bit saving,
         * all flag combinations,
         * different offsets including 0
         */
        /* Define with 0xfff.. because max range
         * computes the range with bitwise or
         */
        int32_t range = (k & 4) ? 0xfff : 0xfffffff;
        BOOL minGraph = tryBest || (k & 8) != 0;
        BOOL try16 = tryBest || (k & 16) != 0;
        int32_t *data;
        uint32_t offset = (k & 3); 
        uint32_t allocSize;
        mingraph_t ming;
        int32_t *ming2;
        uint32_t type, i;
        allocator_t c_alloc =
        {
            allocData:&allocSize,
            allocFunction:test_alloc
        };

        PROGRESS();

        /* generate */
        dbm_generate(dbm1, dim, range);
        
        /* save */
        data = dbm_writeToMinDBMWithOffset(dbm1, dim,
                                           minGraph,
                                           try16,
                                           c_alloc,
                                           offset);
        ming = &data[offset];
        
        /* check min graph */
        debug_randomize(testMG1, testSize);
        debug_randomize(testMG2, testSize);
        nbCons1 = dbm_analyzeForMinDBM(dbm1, dim, testMG1);
        assert(nbCons1 == base_countBitsN(testMG1, testSize));
        nbCons2 = dbm_getBitMatrixFromMinDBM(testMG2, ming, TRUE, dbm1);
        assert(nbCons2 == base_countBitsN(testMG2, testSize));
        ASSERT(nbCons1 == nbCons2, fprintf(stderr, "%d != %d\n", nbCons1, nbCons2));
        assert(base_areEqual(testMG1, testMG2, testSize));
        debug_randomize(testMG2, testSize);
        debug_randomize(dbm2, dim*dim);
        nbCons2 = dbm_getBitMatrixFromMinDBM(testMG2, ming, FALSE, dbm2);
        assert(nbCons2 == base_countBitsN(testMG2, testSize));
        ASSERT(nbCons1 == nbCons2, fprintf(stderr, "%d != %d\n", nbCons1, nbCons2));
        assert(base_areEqual(testMG1, testMG2, testSize));

        /* do some stats */
        type = dbm_getRepresentationType(ming);
        stats[type]++;
        sizes[type] += allocSize - offset;
        
        /* basic checks */
        assert(data);
        assert(allocSize >= offset);
        /* dbm generation not so reliable wrt range
         * assert(dbm_getMaxRange(dbm1, dim) < range);
         */
        assert(dim == dbm_getDimOfMinDBM(ming));
        assert(allocSize == offset +
               dbm_getSizeOfMinDBM(ming));
        assert(dbm_isEqualToMinDBM(dbm1, dim, ming));

        /* load */
        debug_randomize(dbm2, dim*dim);
        assert(dim ==
               dbm_readFromMinDBM(dbm2, ming));
        DBM_EQUAL(dbm1, dbm2);

        /* relation */
        assert(dbm_relationWithMinDBM(dbm1, dim, ming, NULL)
               == base_SUBSET);
        assert(dbm_relationWithMinDBM(dbm1, dim, ming, dbm2)
               == base_EQUAL);

        dbm_generateSuperset(dbm2, dbm1, dim);
        if (!dbm_areEqual(dbm1, dbm2, dim)) /* then superset strict */
        {
            assert(!dbm_isEqualToMinDBM(dbm2, dim, ming));

            assert(dbm_relationWithMinDBM(dbm2, dim, ming, NULL)
                   == base_DIFFERENT);
            assert(dbm_relationWithMinDBM(dbm2, dim, ming, dbm1)
                   == base_SUPERSET);
            /* dbm1 < dbm2
             * save dbm2 and retest dbm1
             */
            ming2 = dbm_writeToMinDBMWithOffset(dbm2, dim,
                                                minGraph,
                                                try16,
                                                c_alloc,
                                                0);
            assert(dbm_isEqualToMinDBM(dbm2, dim, ming2));
            assert(!dbm_isEqualToMinDBM(dbm1, dim, ming2));
            assert(allocSize == dbm_getSizeOfMinDBM(ming2));
            assert(dim ==
                   dbm_readFromMinDBM(dbm3, ming2));
            DBM_EQUAL(dbm2, dbm3);
            type = dbm_getRepresentationType(ming2);
            stats[type]++;
            sizes[type] += allocSize;
            assert(dbm_relationWithMinDBM(dbm1, dim, ming2, NULL)
                   == base_SUBSET);
            assert(dbm_relationWithMinDBM(dbm1, dim, ming2, dbm2)
                   == base_SUBSET);

            test_free(ming2);
        }

        /* be nasty */
        dbm_copy(dbm2, dbm1, dim);
        for(i = 1; i < dim; ++i) dbm2[i] -= 10;
        if (dbm_close(dbm2, dim))
        {
            assert(dbm_isEqualToMinDBM(dbm1, dim, ming));
            assert(dbm_areEqual(dbm2, dbm1, dim) ==
                   dbm_isEqualToMinDBM(dbm2, dim, ming));
        }


        /* convex union */
        dbm_generate(dbm2, dim, range);
        ming2 = dbm_writeToMinDBMWithOffset(dbm2, dim,
                                            minGraph,
                                            try16,
                                            c_alloc,
                                            0);
        /* load */
        debug_randomize(dbm3, dim*dim);
        assert(dim ==
               dbm_readFromMinDBM(dbm3, ming2));
        DBM_EQUAL(dbm2, dbm3);

        assert(dbm_isEqualToMinDBM(dbm2, dim, ming2));
        assert(allocSize == dbm_getSizeOfMinDBM(ming2));
        type = dbm_getRepresentationType(ming2);
        stats[type]++;
        sizes[type] += allocSize;

        /* if we have an inclusion then the union will
         * be either dbm1 or dbm2
         */
        if (dbm_relation(dbm1, dbm2, dim) == base_DIFFERENT)
        {
            dbm_copy(dbm3, dbm2, dim);
            dbm_convexUnion(dbm3, dbm1, dim);

            dbm_convexUnionWithMinDBM(dbm2, dim, ming, dbm1);
            DBM_EQUAL(dbm3, dbm2);

            assert(dbm_relationWithMinDBM(dbm2, dim, ming, NULL)
                   == base_DIFFERENT);
            assert(dbm_relationWithMinDBM(dbm2, dim, ming, dbm1)
                   == base_SUPERSET);

            assert(dbm_relationWithMinDBM(dbm2, dim, ming2, NULL)
                   == base_DIFFERENT);
            assert(dbm_relationWithMinDBM(dbm2, dim, ming2, dbm1)
                   == base_SUPERSET);
        }

        test_free(ming2);
        test_free(data);
    }

    test_printStats(stats, sizes, dim);

    free(dbm3);
    free(dbm2);
    free(dbm1);
}


int main(int argc, char *argv[])
{
    int i, start, end, seed;
    BOOL tryBest;

    if (argc < 3)
    {
        fprintf(stderr,"Usage: %s start_size end_size [tryBestFlag] [seed]\n",argv[0]);
        return 1;
    }

    start = atoi(argv[1]);
    end = atoi(argv[2]);
    tryBest = argc > 3 ? atoi(argv[3]) != 0 : FALSE;
    seed = argc > 4 ? atoi(argv[4]) : time(NULL);
    srand(seed);

    if (start < 1)
    {
        fprintf(stderr, "Minimum valid dimension is 1.\n");
        return 1;
    }

    /* Print the seed for the random generator
     * to be able to repeat a failed test
     */
    printf("Testing with seed=%d\n", seed);

    for(i = start ; i <= end ; ++i)
        test(i, tryBest);

    printf("\nPassed\n");
    return 0;
}

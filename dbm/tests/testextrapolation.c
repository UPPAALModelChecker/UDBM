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
 * Filename : testextrapolation.c
 *
 * Test of the extrapolation procedures of the DBM library.
 * If you try to reduce the closure then it is a good idea to
 * test with ./testextrapolation 50 1070275367
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: testextrapolation.c,v 1.17 2005/06/19 17:22:53 adavid Exp $
 *
 *********************************************************************/

/* Tests are always for debugging. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdlib.h>
#include <time.h>
#include "dbm/dbm.h"
#include "debug/macros.h"
#include "dbm/print.h"
#include "dbm/gen.h"

/************************************************
 * macros and useful functions, as in testdbm.c *
 ************************************************/

/* print diff
 */
#define DIFF(D1,D2) dbm_printDiff(stdout, D1, D2, size)

/* Cool assert
 */
#define DBM_EQUAL(D1,D2) \
ASSERT(dbm_areEqual(D1,D2,size), DIFF(D1,D2))

/* Allocation of DBM, vector, bitstring
 */
#define ADBM(NAME) raw_t *NAME = allocDBM(size)
#define AVECT(NAME) int32_t *NAME = allocPt(size)

/* Random range
 * may change definition
 */
#define MAXRANGE 10000
#define RANGE() ((rand()%MAXRANGE)+1)


/* Show progress
 */
#define PROGRESS() debug_spin(stderr)

/* For more readable code */
#define DBM(I,J) dbm[(I)*dim+(J)]

/* Alternative simple implementations */

void test_diagonalExtrapolateMaxBounds(raw_t *dbm, cindex_t dim, const int32_t *max)
{
    cindex_t i,j;
    assert(dbm && dim > 0 && max);

    /* other rows
     */
    for (i = 1; i < dim; ++i)
    {
        for (j = 0; j < dim; ++j)
        {
            if (i != j &&
                (dbm_raw2bound(DBM(0,i)) < -max[i] ||
                 dbm_raw2bound(DBM(0,j)) < -max[j] ||
                 dbm_raw2bound(DBM(i,j)) >  max[i]))
            {
                DBM(i,j) = dbm_LS_INFINITY;
            }
        }
    }

    /* 1st row */
    for (j = 1; j < dim; ++j)
    {
        assert(dbm_raw2bound(DBM(0,j)) <= max[0]);

        if (dbm_raw2bound(DBM(0,j)) < -max[j])
        {
            DBM(0,j) = (max[j] >= 0
                        ? dbm_bound2raw(-max[j], dbm_STRICT)
                        : dbm_LE_ZERO);
        }
    }
    dbm_close(dbm, dim);
}

void test_diagonalExtrapolateLUBounds(raw_t *dbm, cindex_t dim,
                                     const int32_t *lower, const int32_t *upper)
{
    cindex_t i,j;
    assert(dbm && dim > 0 && lower && upper);

    /* other rows */
    for (i = 1; i < dim; ++i)
    {
        for (j = 0; j < dim; ++j)
        {
            if (i != j &&
                (dbm_raw2bound(DBM(i,j)) >  lower[i] ||
                 dbm_raw2bound(DBM(0,i)) < -lower[i] ||
                 dbm_raw2bound(DBM(0,j)) < -upper[j]))
            {
                DBM(i,j) = dbm_LS_INFINITY;
            }
        }
    }

    /* 1st row */
    for (j = 1; j < dim; ++j)
    {
        assert(dbm_raw2bound(DBM(0,j))<= lower[0]);
        
        if (dbm_raw2bound(DBM(0,j)) < -upper[j])
        {
            DBM(0, j) = (upper[j] >= 0 
                         ? dbm_bound2raw(-upper[j], dbm_STRICT)
                         : dbm_LE_ZERO);
        }
    }
    dbm_close(dbm, dim);
}


/* easy endl
 */
/* #define ENDL putc('\n',stdout) */
#define ENDL printf(" \n")


/* allocate a DBM
 */
static
raw_t* allocDBM(uint32_t dim)
{
    return (raw_t*) malloc(dim*dim*sizeof(raw_t));
}

/* allocate a point
 */
static
int32_t *allocPt(uint32_t dim)
{
    return (int32_t*) malloc(dim*sizeof(int32_t));
}


/* Generation of DBMs: track non trivial ones
 * and count them all.
 */
#define DBM_GEN(D) \
do { \
  BOOL good = dbm_generate(D, size, RANGE());\
  allDBMs++; \
  goodDBMs += good; \
} while(0)

static uint32_t allDBMs = 0;
static uint32_t goodDBMs = 0;

/***********************************************/


/* test the 4 extrapolations
 */
static
void test(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm2);
    ADBM(dbm3);
    ADBM(dbm4);
    ADBM(dbm5);
    ADBM(dbm6);
    ADBM(dbm7);
    ADBM(dbm8);
    ADBM(dbm9);
    ADBM(dbm10);
    AVECT(lower);
    AVECT(upper);
    AVECT(max);
    uint32_t k, l;
    printf("*** Testing size=%u\n", size);


    for (k = 0; k < 2000; ++k)
    {
        PROGRESS();
        
        max[0] = lower[0] = upper[0] = 0;
        for (l = 1; l < size; l++) 
        {
            int32_t low = RANGE() - (MAXRANGE / 2);
            int32_t up  = RANGE() - (MAXRANGE / 2); /* low + RANGE()/2; */
            lower[l] = (low < 0 ? -dbm_INFINITY : low);
            upper[l] = (up < 0 ? -dbm_INFINITY : up);
            max[l] = (lower[l] < upper[l] ? upper[l] : lower[l]);
        }
        
        DBM_GEN(dbm1);
        dbm_copy(dbm10, dbm1, size);
        dbm_copy(dbm9, dbm1, size);
        dbm_copy(dbm8, dbm1, size); 
        dbm_copy(dbm7, dbm1, size); 
        dbm_copy(dbm6, dbm1, size); 
        dbm_copy(dbm5, dbm1, size); 
        dbm_copy(dbm4, dbm1, size); 
        dbm_copy(dbm3, dbm1, size); 
        dbm_copy(dbm2, dbm1, size); 
        assert(dbm_isClosed(dbm1, size));
        
        dbm_extrapolateMaxBounds(dbm2, size, max);
        dbm_extrapolateLUBounds(dbm3, size, max, max);
        dbm_extrapolateLUBounds(dbm4, size, lower, upper);
        dbm_diagonalExtrapolateMaxBounds(dbm5, size, max);
        test_diagonalExtrapolateMaxBounds(dbm8, size, max);
        dbm_diagonalExtrapolateLUBounds(dbm6, size, max, max);
        test_diagonalExtrapolateLUBounds(dbm9, size, max, max);
        dbm_diagonalExtrapolateLUBounds(dbm7, size, lower, upper);
        test_diagonalExtrapolateLUBounds(dbm10, size, lower, upper);
        
        assert(dbm_areEqual(dbm5, dbm8, size));
        assert(dbm_areEqual(dbm6, dbm9, size));
        assert(dbm_areEqual(dbm7, dbm10, size));

        /* 1 <= 2 == 3 <= 4 <= 7, 1 <= 2 == 3 <= 5 == 6 <= 7 */
        assert(dbm_relation(dbm1, dbm2, size) & base_SUBSET);
        assert(dbm_relation(dbm2, dbm3, size) == base_EQUAL);
        assert(dbm_relation(dbm3, dbm4, size) & base_SUBSET);
        assert(dbm_relation(dbm4, dbm7, size) & base_SUBSET);
        assert(dbm_relation(dbm3, dbm5, size) & base_SUBSET);
        assert(dbm_relation(dbm5, dbm6, size) == base_EQUAL);
        assert(dbm_relation(dbm6, dbm7, size) & base_SUBSET);
        
        /* further checks */
        assert(dbm_isValid(dbm2, size));
        assert(dbm_isValid(dbm3, size));
        assert(dbm_isValid(dbm4, size));
        assert(dbm_isValid(dbm5, size));
        assert(dbm_isValid(dbm6, size));
        assert(dbm_isValid(dbm7, size));
    }
    ENDL;

    free(lower);
    free(upper);
    free(max);
    free(dbm10);
    free(dbm9);
    free(dbm8);
    free(dbm7);
    free(dbm6);
    free(dbm5);
    free(dbm4);
    free(dbm3);
    free(dbm2);
    free(dbm1);
}


int main(int argc, char *argv[])
{
    int i,n,seed;

    if (argc < 2)
    {
        fprintf(stderr,"Usage: %s size [seed]\n",argv[0]);
        return 1;
    }

    n = atoi(argv[1]);
    seed = argc > 2 ? atoi(argv[2]) : time(NULL);
    srand(seed);
    printf("Testing with seed=%d\n", seed);

    for(i = 1 ; i <= n ; ++i)
        test(i);

    printf("Passed\n");
    return 0;
}

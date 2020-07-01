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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "base/bitstring.h"
#include "dbm/dbm.h"
#include "dbm/gen.h"
#include "dbm/print.h"
#include "debug/macros.h"

/* Define to have verbose messages
 */
/* #define VERBOSE */

#ifdef VERBOSE
#define PRINTF(S) printf(S)
#define ENDL printf(" \n")
#else
#define PRINTF(S) putc('.', stderr)
#define ENDL
#endif

/* Inner repeat loop.
 */
#define LOOP 1000

/* print diff
 */
#define DIFF(D1,D2) dbm_printDiff(stderr, D1, D2, size)

/* Cool assert
 */
#define DBM_EQUAL(D1,D2) \
ASSERT(dbm_areEqual(D1,D2,size), DIFF(D1,D2))

#define DBM_SUBSET(D1,D2) \
if (dbm_areEqual(D1,D2,size)) \
{\
    assert(dbm_relation(D1,D2,size) == base_EQUAL);\
}\
else\
{\
    assert(dbm_relation(D1,D2,size) == base_SUBSET);\
    assert(dbm_relation(D2,D1,size) == base_SUPERSET);\
}

/* Allocation of DBM, vector, bitstring
 */
#define ADBM(NAME) raw_t *NAME = allocDBM(size)
#define AVECT(NAME) int32_t NAME[size]
#define ABITS(NAME) uint32_t NAME[bits2intsize(size)]
#define DVECT(NAME) double NAME[size]

/* Random range
 * may change definition
 */
#define RANGE() ((rand()%10000)+10)

/* Show progress
 */
#define PROGRESS() debug_spin(stderr)

/* allocate a DBM
 */
static
raw_t* allocDBM(uint32_t dim)
{
    return (raw_t*) malloc(dim*dim*sizeof(raw_t));
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


/* test zero
 */
static
void test_zero(uint32_t size)
{
    ADBM(dbm);
    AVECT(pt);
    uint32_t k;
    PRINTF("zero");
    dbm_zero(dbm, size);
    assert(dbm_isClosed(dbm, size));

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        assert(dbm_generatePoint(pt, dbm, size));
        assert(dbm_isPointIncluded(pt, dbm, size));
        /* only possible point == 0 */
        assert(base_areIBitsReset(pt, size));
    }

    ENDL;
    free(dbm);
}


/* test relaxUp
 */
static
void test_relaxUp(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k,i;
    PRINTF("relaxUp");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        
        DBM_GEN(dbm1);
        dbm_copy(dbm3, dbm1, size);
        dbm_relaxUp(dbm1, size);
        DBM_SUBSET(dbm3, dbm1);
        // dummy non-optimized relaxUp:
        for(i = 0; i < size; ++i)
        {
            if (dbm3[i*size] != dbm_LS_INFINITY)
            {
                dbm3[i*size] |= dbm_WEAK;
            }
        }
        dbm_close(dbm3, size);
        DBM_EQUAL(dbm1, dbm3);

        /* further checks */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}

/* test relaxDown
 */
static
void test_relaxDown(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k,i;
    PRINTF("relaxDown");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        
        DBM_GEN(dbm1);
        dbm_copy(dbm3, dbm1, size);
        dbm_relaxDown(dbm1, size);
        DBM_SUBSET(dbm3, dbm1);
        // dummy non-optimized relaxDown:
        for(i = 0; i < size; ++i)
        {
            assert(dbm3[i] != dbm_LS_INFINITY); // lower bounds
            dbm3[i] |= dbm_WEAK;
        }
        dbm_close(dbm3, size);
        DBM_EQUAL(dbm1, dbm3);

        /* further checks */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}

/* test relaxDownClock
 */
static
void test_relaxDownClock(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k,i;
    PRINTF("relaxDownClock");

    for(k = 0; k < LOOP; ++k)
    {
        uint32_t kk = rand()%size;
        PROGRESS();
        
        DBM_GEN(dbm1);
        dbm_copy(dbm3, dbm1, size);
        dbm_relaxDownClock(dbm1, size, kk);
        DBM_SUBSET(dbm3, dbm1);
        // dummy non-optimized relaxDownClock:
        for(i = 0; i < size; ++i)
        {
            if (dbm3[i*size+kk] != dbm_LS_INFINITY)
            {
                dbm3[i*size+kk] |= dbm_WEAK;
            }
        }
        dbm_close(dbm3, size);
        DBM_EQUAL(dbm1, dbm3);

        /* further checks */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}

/* test relaxUpClock
 */
static
void test_relaxUpClock(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k,i;
    PRINTF("relaxUpClock");

    for(k = 0; k < LOOP; ++k)
    {
        uint32_t kk = rand()%size;
        PROGRESS();
        
        DBM_GEN(dbm1);
        dbm_copy(dbm3, dbm1, size);
        dbm_relaxUpClock(dbm1, size, kk);
        DBM_SUBSET(dbm3, dbm1);
        // dummy non-optimized relaxDownClock:
        for(i = 0; i < size; ++i)
        {
            if (dbm3[kk*size+i] != dbm_LS_INFINITY)
            {
                dbm3[kk*size+i] |= dbm_WEAK;
            }
        }
        dbm_close(dbm3, size);
        DBM_EQUAL(dbm1, dbm3);

        /* further checks */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}


/* test freeDown
 */
static
void test_freeDown(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k;
    PRINTF("freeDown");

    if (size > 1)
    {
        for(k = 0; k < LOOP; ++k)
        {
            uint32_t kk;
            PROGRESS();
            
            DBM_GEN(dbm1);
            
            for(kk = 0; kk < size; ++kk)
            {
                uint32_t index = rand()%(size-1)+1;
                dbm_copy(dbm3, dbm1, size);
                dbm_freeDown(dbm1, size, index);
                DBM_SUBSET(dbm3, dbm1);
                
                /* further checks */
                assert(dbm_isValid(dbm1, size));
                assert(!dbm_isEmpty(dbm1, size));
            }
        }
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}


/* test freeUp
 */
static
void test_freeUp(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k;
    PRINTF("freeUp");

    if (size > 1)
    {
        for(k = 0; k < LOOP; ++k)
        {
            uint32_t kk;
            PROGRESS();
            
            DBM_GEN(dbm1);
            
            for(kk = 0; kk < size; ++kk)
            {
                uint32_t index = rand()%(size-1)+1;
                dbm_copy(dbm3, dbm1, size);
                dbm_freeUp(dbm1, size, index);
                DBM_SUBSET(dbm3, dbm1);
                
                /* further checks */
                assert(dbm_isValid(dbm1, size));
                assert(!dbm_isEmpty(dbm1, size));
            }
        }
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}


/* test freeallup
 */
static
void test_freeAllUp(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k;
    PRINTF("freeAllUp");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        
        DBM_GEN(dbm1);
        dbm_copy(dbm3, dbm1, size);
        dbm_freeAllUp(dbm1, size);
        assert(dbm_isFreedAllUp(dbm1, size));
        assert(dbm_isUnbounded(dbm1, size));
        DBM_SUBSET(dbm3, dbm1);
        
        /* further checks */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}

/* test freealldown
 */
static
void test_freeAllDown(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k;
    PRINTF("freeAllDown");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        
        DBM_GEN(dbm1);
        dbm_copy(dbm3, dbm1, size);
        dbm_freeAllDown(dbm1, size);
        DBM_SUBSET(dbm3, dbm1);
        
        /* further checks */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}


/* test freeClock
 */
static
void test_freeClock(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm2);
    uint32_t k;
    PRINTF("freeClock");

    if (size > 1)
    {
        for(k = 0; k < LOOP; ++k)
        {
            uint32_t kk;
            PROGRESS();

            DBM_GEN(dbm1);
            dbm_copy(dbm2, dbm1, size);
            for(kk = 0; kk < 10; ++kk)
            {
                uint32_t i = rand()%(size-1)+1; /* any but ref clock */
                dbm_freeClock(dbm1, size, i);
                DBM_SUBSET(dbm2, dbm1);
                /* further checks */
                assert(dbm_isValid(dbm1, size));
                assert(!dbm_isEmpty(dbm1, size));
            }
        }
    }

    ENDL;
    free(dbm2);
    free(dbm1);
}


/* test up
 */
static
void test_down(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k;
    PRINTF("down");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        DBM_GEN(dbm1);
        dbm_copy(dbm3, dbm1, size);
        dbm_down(dbm1, size);

        DBM_SUBSET(dbm3, dbm1);
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));

        /* down is idempotent */
        dbm_copy(dbm3, dbm1, size);
        dbm_down(dbm1, size);
        DBM_EQUAL(dbm1, dbm3);
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}


/* test up
 */
static
void test_up(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm3);
    uint32_t k;
    PRINTF("up");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        DBM_GEN(dbm1);
        dbm_copy(dbm3, dbm1, size);
        dbm_up(dbm1, size);

        DBM_SUBSET(dbm3, dbm1);
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
        assert(dbm_isUnbounded(dbm1, size));

        /* up is idempotent */
        dbm_copy(dbm3, dbm1, size);
        dbm_up(dbm1, size);
        DBM_EQUAL(dbm1, dbm3);
    }

    ENDL;
    free(dbm3);
    free(dbm1);
}


/* test constrain & constrain1 & constrainN & satisfy & isEmpty
 * & closex & close & close1
 */
static
void test_constrain(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm2);
    ADBM(dbm3);
    ADBM(dbm4);
    ADBM(dbm5);
    AVECT(pt);
    ABITS(touched1);
    ABITS(touched2);
    ABITS(touched3);
    constraint_t *constraints = (constraint_t*) malloc(size*size*sizeof(constraint_t));
    uint32_t k;
    PRINTF("constrain+constrain1+constrainN+satisfy+isEmpty+close+closex+close1");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();

        base_resetBits(touched1, bits2intsize(size));
        base_resetBits(touched2, bits2intsize(size));
        base_resetBits(touched3, bits2intsize(size));
        DBM_GEN(dbm1);
        dbm_copy(dbm2, dbm1, size); /* dbm2 = dbm1 */
        dbm_copy(dbm3, dbm1, size); /* dbm2 = dbm1 */
        dbm_copy(dbm4, dbm1, size); /* dbm4 = dbm1 */
        dbm_copy(dbm5, dbm1, size); /* dbm5 = dbm1 */

        /* if the point belongs to the DBM,
         * constrain the DBM such that DBM = this point
         */
        if (dbm_generatePoint(pt, dbm1, size))
        {
            uint32_t i,j,nbConstraints = 0;
            int32_t constraint;
            BOOL stop = (BOOL) (rand() & 1); /* to stop constraining */
            
            for(i = 0; i < size; ++i)
            {
                for(j = 0; j < size; ++j)
                {
                    if (i != j)
                    {
                        constraint = dbm_bound2raw(pt[i] - pt[j], dbm_WEAK);

                        /* satisfy */
                        assert(dbm_satisfies(dbm1, size, i, j, constraint));

                        /* constrain */
                        assert(dbm_constrain(dbm1, size, i, j, constraint, touched1));
                        assert(dbm_constrain(dbm2, size, i, j, constraint, touched2));
                        assert(dbm_constrain(dbm3, size, i, j, constraint, touched3));
                        assert(dbm_constrain1(dbm4, size, i, j, constraint));

                        assert(dbm_close1(dbm2, size, i));
                        assert(dbm_close1(dbm2, size, j));
                        dbm_closeij(dbm3, size, i, j);

                        /* dbm1 not closed */
                        assert(dbm_isClosed(dbm2, size));
                        assert(dbm_isClosed(dbm3, size));
                        assert(dbm_isClosed(dbm4, size));

                        DBM_EQUAL(dbm2, dbm3);
                        DBM_EQUAL(dbm3, dbm4);

                        /* prepare for constrainN */
                        constraints[nbConstraints].i = i;
                        constraints[nbConstraints].j = j;
                        constraints[nbConstraints].value = constraint;
                        nbConstraints++;
                    }
                }
                if (stop && (rand() & 1)) break;
            }
            dbm_copy(dbm3, dbm1, size); /* dbm3 = dbm1 */

            assert(base_countBitsN(touched1, bits2intsize(size)) <= size);
            assert(dbm_closex(dbm1, size, touched1));
            assert(dbm_close(dbm3, size));
            assert(dbm_constrainN(dbm5, size, constraints, nbConstraints));

            DBM_EQUAL(dbm1, dbm2);
            DBM_EQUAL(dbm1, dbm3);
            DBM_EQUAL(dbm1, dbm5);

            assert(dbm_isValid(dbm1, size));
            assert(!dbm_isEmpty(dbm1, size));
            assert(dbm_isPointIncluded(pt, dbm1, size));
            
            /* now empty the DBM if we didn't stop constraining
             */
            if (!stop && size > 1) 
            {
                /* dbm = only 1 point
                 */
                assert(!dbm_isUnbounded(dbm1, size));

                j = rand()%(size-1)+1; /* any but 0 */
                i = rand()%size;
                if (i == j) i = 0;
                pt[j]++;
                constraint = dbm_bound2raw(pt[i] - pt[j], dbm_WEAK);

                assert(!dbm_satisfies(dbm1, size, i, j, constraint));
                assert(!dbm_constrain(dbm1, size, i, j, constraint, touched1));
                assert(!dbm_constrain1(dbm3, size, i, j, constraint));
                assert(dbm_isEmpty(dbm3, size));
                assert(!dbm_close1(dbm1, size, i) || !dbm_close1(dbm1, size, j));
            }
        }
    }

    ENDL;
    free(constraints);
    free(dbm5);
    free(dbm4);
    free(dbm3);
    free(dbm2);
    free(dbm1);
}

/* test generatePoint and isIncluded
 */
static
void test_point(uint32_t size)
{
    ADBM(dbm1);
    AVECT(pt);
    uint32_t i,k;
    PRINTF("discrete point");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();

        DBM_GEN(dbm1);
        for(i = 0; i < 10; ++i)
        {
            if (dbm_generatePoint(pt, dbm1, size))
            {
                assert(dbm_isPointIncluded(pt, dbm1, size));
            }
            /* else pt invalid -- don't test it */
        }
    }
    
    ENDL;
    free(dbm1);
}

/* test generatePoint and isIncluded
 */
static
void test_real_point(uint32_t size)
{
    ADBM(dbm1);
    DVECT(pt);
    uint32_t i,k;
    PRINTF("real point");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();

        DBM_GEN(dbm1);
        for(i = 0; i < 10; ++i)
        {
            if (dbm_generateRealPoint(pt, dbm1, size))
            {
                /*debug_printRealVector(stderr, pt, size);
                  putc('\n', stderr);*/
                assert(dbm_isRealPointIncluded(pt, dbm1, size));
            }
        }
    }
    
    ENDL;
    free(dbm1);
}


/* test dbm_intersection against dbm_debugIntersection
 */
static
void test_intersection(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm2);
    ADBM(dbm3);
    BOOL r0,r1;
    uint32_t k;
    uint32_t empty = 0, nonempty = 0;
    PRINTF("intersection");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();

        /* test operation */
        DBM_GEN(dbm1);               /* new dbm1    */
        if (k == 0)
        {
            DBM_GEN(dbm2);           /* new dbm2    */
        }
        else
        {
            dbm_copy(dbm2, dbm1, size);/* dbm2 = dbm1 */
            if (size > 1)
            {
                /* choose a clock */
                uint32_t i = rand()%(size - 1) + 1;
                /* translate xi = xi + 1 */ 
                dbm_updateIncrement(dbm2, size, i, 1);
                /* check */
                assert(dbm_isValid(dbm2, size));
            }
        }
        dbm_copy(dbm3, dbm1, size);  /* dbm4 = dbm1 */
        r0 = dbm_haveIntersection(dbm1, dbm2, size);
        r1 = dbm_intersection(dbm1, dbm2, size);     /* dbm1 &= dbm2 */
        /* no intersection => intersection empty */
        assert(r0 || !r1);

        if (r1) /* if non empty intersection */
        {
            nonempty++;
            assert(dbm_isValid(dbm1, size));
            assert(!dbm_isEmpty(dbm1, size));
            
            /* test consistency of operation */
            DBM_SUBSET(dbm1, dbm2);
        }
        else
        {
            empty++;
            assert(dbm_relation(dbm3, dbm2, size) == base_DIFFERENT);
        }
        
        /* no effect with superset */
        dbm_generateSuperset(dbm1, dbm2, size); /* dbm1 >= dbm2 */
        assert(dbm_isClosed(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
        dbm_copy(dbm3, dbm2, size);                   /* dbm3 = dbm2  */
        r1 = dbm_intersection(dbm2, dbm1, size);      /* dbm2 &= dbm1 */
        assert(dbm_isValid(dbm2, size));
        assert(!dbm_isEmpty(dbm2, size));
        DBM_EQUAL(dbm2, dbm3);

        r1 = dbm_intersection(dbm1, dbm2, size);      /* dbm1 &= dbm2 */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
        assert(r1);
    }

#ifdef VERBOSE
    printf(" nonempty=%u/%u\n", nonempty, empty+nonempty);
#endif
    free(dbm3);
    free(dbm2);
    free(dbm1);
}


/* test dbm_convexUnion against dbm_debugConvexUnion
 */
static
void test_convexUnion(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm2);
    ADBM(dbm3);
    uint32_t k;
    PRINTF("convexUnion");
    
    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();

        /* test operation */
        DBM_GEN(dbm1);         /* new dbm1     */
        DBM_GEN(dbm2);         /* new dbm2     */
        dbm_copy(dbm3, dbm1, size);                  /* dbm3 = dbm1  */

        dbm_convexUnion(dbm1, dbm2, size);     /* dbm1 |= dbm2 */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
        
        /* test consistency of operation */
        DBM_SUBSET(dbm2, dbm1);
        DBM_SUBSET(dbm3, dbm1);

        /* no effect with superset */
        dbm_generateSuperset(dbm1, dbm2, size); /* dbm1 >= dbm2 */
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
        dbm_convexUnion(dbm2, dbm1, size);      /* dbm2 |= dbm1 */

        DBM_EQUAL(dbm1, dbm2);
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm3);
    free(dbm2);
    free(dbm1);
}


/* test relation,superset
 */
static
void test_relation(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm2);
    uint32_t k;
    PRINTF("relation+superset");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();

        DBM_GEN(dbm1);
        dbm_generateSuperset(dbm2, dbm1, size);
        assert(dbm_isValid(dbm2, size));
        assert(!dbm_isEmpty(dbm2, size));

        DBM_SUBSET(dbm1, dbm2);

        /* different */
        if (size > 2 && !dbm_isUnbounded(dbm1, size))
        {
            uint32_t i = rand()%(size-1)+1;
            uint32_t j = rand()%(size-1)+1;
            if (i == j)
            {
                j = i + 1;
                if (j >= size) j = 1;
            }
            dbm_copy(dbm2, dbm1, size);
            dbm_updateIncrement(dbm1, size, i, 10);
            dbm_updateIncrement(dbm2, size, j, 10);
            ASSERT(dbm_relation(dbm1, dbm2, size) == base_DIFFERENT, DIFF(dbm1, dbm2));
            ASSERT(dbm_relation(dbm2, dbm1, size) == base_DIFFERENT, DIFF(dbm1, dbm2));
        }
    }

    ENDL;
    free(dbm2);
    free(dbm1);
}

/* test dbm_init against dbm_debugInit
 */
static
void test_init(uint32_t size)
{
    ADBM(dbm1);
    uint32_t k;
    PRINTF("init");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        dbm_init(dbm1, size);
        assert(dbm_isEqualToInit(dbm1, size));
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm1);
}


/* basic checks on DBM generation
 */
static
void test_generate(uint32_t size)
{
    ADBM(dbm1);
    uint32_t k;
    PRINTF("generate");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        DBM_GEN(dbm1);
        assert(dbm_isValid(dbm1, size));
        assert(!dbm_isEmpty(dbm1, size));
    }

    ENDL;
    free(dbm1);
}


/* test subset
 */
static
void test_subset(uint32_t size)
{
    ADBM(dbm1);
    ADBM(dbm2);
    uint32_t k, n = 0;
    PRINTF("subset");

    for(k = 0; k < LOOP; ++k)
    {
        PROGRESS();
        DBM_GEN(dbm1);
        if (dbm_generateSubset(dbm2, dbm1, size))
        {
            assert(dbm_relation(dbm2, dbm1, size) == base_SUBSET);
            n++;
        }
        else
        {
            ASSERT(dbm_areEqual(dbm1, dbm2, size), dbm_printDiff(stderr, dbm1, dbm2, size));
        }
        assert(dbm_isClosed(dbm2, size));
        assert(!dbm_isEmpty(dbm2, size));
    }

    assert((size == 1) == (n == 0));
    ENDL;
    free(dbm2);
    free(dbm1);
}

/* run all tests for a given size
 */
static
void test(uint32_t size)
{
    fprintf(stderr, "Testing size %u ", size);
    ENDL;
    test_generate(size);
    test_init(size);
    test_relation(size);
    test_convexUnion(size);
    test_intersection(size);
    test_point(size);
    test_real_point(size);
    test_constrain(size);
    test_up(size);
    test_down(size);
    //test_updateValue(size);
    //test_updateClock(size);
    //test_updateIncrement(size);
    //test_update(size);
    test_freeClock(size);
    test_freeUp(size);
    test_freeDown(size);
    test_freeAllUp(size);
    test_freeAllDown(size);
    test_relaxUp(size);
    test_relaxDown(size);
    test_relaxUpClock(size);
    test_relaxDownClock(size);
    //test_unbounded(size);
    test_zero(size);
    test_subset(size);

#ifndef VERBOSE
    printf(" \n");
#endif
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
        test(i);
    }

    assert(allDBMs);
    printf("Total generated DBMs: %u, non trivial ones: %u (%u%%)\n",
           allDBMs, goodDBMs, (100*goodDBMs)/allDBMs);

    printf("Passed\n");
    return 0;
}

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
 * Filename : testdbmfederation.c (dbm/tests)
 *
 * Test API from dbm/federation.h
 * Leaks are automatically detected.
 *
 * This file is a part of the UPPAAL toolkit.
 * Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
 * All right reserved.
 *
 * $Id: testdbmfederation.c,v 1.17 2005/08/03 15:31:00 adavid Exp $
 *
 **********************************************************************/
#define NTOOEXPENSIVE
/* Tests are always for debugging. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "dbm/dbmfederation.h"
#include "dbm/mingraph.h"
#include "dbm/dbm.h"
#include "dbm/print.h"
#include "dbm/gen.h"
#include "debug/c_allocator.h"
#include "debug/utils.h"
#include "debug/macros.h"

#ifdef VERBOSE
#define SHOW_TEST() printf("%s.", __FUNCTION__); fflush(stdout)
#define SKIP_TEST() printf("%s?", __FUNCTION__); fflush(stdout)
#define NO_TEST()   printf("%s*", __FUNCTION__); fflush(stdout)
#else
#define SHOW_TEST() fputc('.', stdout); fflush(stdout)
#define SKIP_TEST() fputc('?', stdout); fflush(stdout)
#define NO_TEST()   fputc('*', stdout); fflush(stdout)
#endif

#define PROGRESS() debug_spin(stderr)

#define NB_LOOPS (dim > 5 ? 1000 : 2000)


/** Straight-forward print.
 * @param out: where to print
 * @param dbmList: DBM list to print
 * @param dim: dimension of all the DBMs
 */
void dbmf_print(FILE* out, const DBMList_t *dbmList, cindex_t dim)
{
    size_t size = dbmf_getSize(dbmList);
    uint32_t i = 0;
    while(dbmList)
    {
        fprintf(out, "[%d/%d]:\n", ++i, size);
        dbm_print(out, dbmList->dbm, dim);
        dbmList = dbmList->next;
    }
}


/** Test that a discrete point is in a DBM list
 * @param pt: point
 * @param dbmList: list of DBMs
 * @param dim: dimension of the DBMs and the point
 * @return TRUE if pt included in dbmList, FALSE otherwise
 */
static
BOOL test_isPointIn(const int32_t *pt, const DBMList_t *dbmList, cindex_t dim)
{
    BOOL inside = FALSE;
    while(dbmList)
    {
        inside |= dbm_isPointIncluded(pt, dbmList->dbm, dim);
        dbmList = dbmList->next;
    }
    return inside;
}


/** Test that a real point is in a DBM list
 * @param pt: point
 * @param dbmList: list of DBMs
 * @param dim: dimension of the DBMs and the point
 * @return TRUE if pt included in dbmList, FALSE otherwise
 */
static
BOOL test_isRealPointIn(const double *pt, const DBMList_t *dbmList, cindex_t dim)
{
    BOOL inside = FALSE;
    while(dbmList)
    {
        inside |= dbm_isRealPointIncluded(pt, dbmList->dbm, dim);
        dbmList = dbmList->next;
    }
    return inside;
}


/** Allocate and generate n DBMs
 * @param factory: factory for allocation.
 * @param dim: dimension of DBMs to generate
 * @param n: number of DBMs to generate.
 * @pre dim <= factory->maxDim && n > 0
 * @return n allocated and generated DBMs.
 */
static
DBMList_t *test_allocGen(DBMAllocator_t *factory, cindex_t dim, size_t n)
{
    DBMList_t *result = NULL;
    assert(factory && factory->maxDim >= dim);

    while(n)
    {
        DBMList_t *newDBM = dbmf_allocate(factory);
        if (!newDBM) break;
        dbm_generate(newDBM->dbm, dim, 1000);
        newDBM->next = result;
        result = newDBM;
        n--;
    }
    
    return result;
}


/** Pick one DBM from a list of DBMs.
 * @param size: size of the list.
 * @param dbmList: list of DBMs.
 * @return one DBM of the list.
 * @pre size > 0, dbmList != NULL
 */
static
const DBMList_t *test_getDBM(size_t size, const DBMList_t *dbmList)
{
    assert(size);
    size = rand()%size; /* pick one */
    while(size)
    {
        assert(dbmList);
        dbmList = dbmList->next;
        size--;
    }
    assert(dbmList);
    return dbmList;
}


/** Generate a real point in a DBM list.
 * @param pt: point to generate
 * @param dbmList: list of DBMs
 * @param size: size of the list
 * @param dim: dimension of the DBMs and the point
 * @return TRUE if a point is generated successfully, FALSE otherwise
 */
static
BOOL test_generateRealPoint(double *pt, cindex_t dim,
                            const DBMList_t *dbmList, size_t size)
{
    return size && dbm_generateRealPoint(pt, test_getDBM(size, dbmList)->dbm, dim);
}


/** Generate a discrete point in a DBM list.
 * @param pt: point to generate
 * @param dbmList: list of DBMs
 * @param size: size of the list
 * @param dim: dimension of the DBMs and the point
 * @return TRUE if a point is generated successfully, FALSE otherwise
 */
static
BOOL test_generatePoint(int32_t *pt, cindex_t dim,
                        const DBMList_t *dbmList, size_t size)
{
    return size && dbm_generatePoint(pt, test_getDBM(size, dbmList)->dbm, dim);
}


/** Test if a dbm is <= in a list.
 * @param dbm,dim: DBM of dimension dim.
 * @param dbmList: list of DBMs.
 * @return TRUE if dbm <= dbmList
 */
static
BOOL test_isIncluded(const raw_t *dbm, cindex_t dim, const DBMList_t *dbmList)
{
    while(dbmList)
    {
        if (dbm_relation(dbm, dbmList->dbm, dim) & base_SUBSET)
        {
            return TRUE;
        }
        dbmList = dbmList->next;
    }
    return FALSE;
}


/** Test if a dbm is <= in a list, skipping
 * dbm itself.
 * @param dbm,dim: DBM of dimension dim.
 * @param dbmList: list of DBMs.
 * @return TRUE if dbm <= dbmList
 */
static
BOOL test_isIncludedInOther(const raw_t *dbm, cindex_t dim, const DBMList_t *dbmList)
{
    while(dbmList)
    {
        if (dbm != dbmList->dbm &&
            dbm_relation(dbm, dbmList->dbm, dim) & base_SUBSET)
        {
            return TRUE;
        }
        dbmList = dbmList->next;
    }
    return FALSE;
}


/** Add some superset/subset DBMs to a federation.
 * @param factory: compatible factory
 * @param fed: federation to add the supersets/subsets
 * @param nb: maximal number of supersets/subsets to add
 */
static
void test_addDBMs(DBMAllocator_t *factory, DBMFederation_t *fed, size_t nb)
{
    size_t listSize;
    assert(factory && fed && factory->maxDim >= fed->dim);
    listSize = dbmf_getSize(fed->dbmList);
    if (nb && listSize)
    {
        DBMList_t *origList = fed->dbmList;
        nb = rand()%nb + 1; /* 1..nb */
        /* pick a DBM, add a superset/subset of it to fed */
        do {
            const raw_t *dbm = test_getDBM(listSize, origList)->dbm;
            DBMList_t *newdbm = dbmf_allocate(factory);
            if (!newdbm) return;
            newdbm->next = fed->dbmList;
            fed->dbmList = newdbm;
            if (rand() & 1)
            {
                dbm_generateSuperset(newdbm->dbm, dbm, fed->dim);
            }
            else
            {
                dbm_generateSubset(newdbm->dbm, dbm, fed->dim);
            }
        } while(--nb);
    }
}


/** Allocate and generate DBMs arguments for an
 * operation with a DBM list.
 * Generation may include supersets, equal, subset, or
 * different DBMs compared with the ones in dbmList.
 * @param factory: compatible factory
 * @param dim: dimension of DBMs
 * @param dbmList: list of DBMs to base the generation on
 * @param n: upper bound for the number of generated
 * DBMs (min == 1)
 * @param superOrDiff: where to write the type of generated
 * arguments. If one superset or different DBM is generated
 * then *superOrDiff = TRUE, otherwise *superOrDiff = FALSE.
 * @return the generated DBMs.
 * @pre n > 0, dim > 0, factory->maxDim >= dim
 */
static
DBMList_t *test_allocGenArg(DBMAllocator_t *factory, cindex_t dim,
                            const DBMList_t *dbmList, size_t n, BOOL *superOrDiff)
{
    DBMList_t *result = NULL;
    size_t listSize = dbmf_getSize(dbmList);
    assert(factory && factory->maxDim >= dim && superOrDiff && n);
    
    n = rand()%(n+1);
    if (!dbmList) /* if nothing to compare to */
    {
        *superOrDiff = TRUE;
        return test_allocGen(factory, dim, n);
    }
    assert(listSize);
   
    *superOrDiff = FALSE;

    while(n--)
    {
        const DBMList_t *origDBM = test_getDBM(listSize, dbmList);
        DBMList_t *newDBM = dbmf_allocate(factory);
        if (!newDBM) return result;
        newDBM->next = result;
        result = newDBM;

        switch(rand()%4)
        {
        case 0: /* diff */
            dbm_generate(newDBM->dbm, dim, 1000);
            if (!(test_isIncluded(newDBM->dbm, dim, dbmList)))
            {
                *superOrDiff = TRUE;
            }
            break;
            
        case 1: /* equal */
            dbm_copy(newDBM->dbm, origDBM->dbm, dim);
            break;

        case 2: /* subset */
            dbm_generateSubset(newDBM->dbm, origDBM->dbm, dim);
            break;

        case 3: /* superset */
            dbm_generateSuperset(newDBM->dbm, origDBM->dbm, dim);
            if (!(test_isIncluded(newDBM->dbm, dim, dbmList)))
            {
                *superOrDiff = TRUE;
            }
            break;
        }
    }

    return result;
}


/** Generate constraints for a federation.
 * @param dim: dimension of DBMs
 * @param dbmList: list of DBMs
 * @param n: number of constraints to generate
 * @param constraints: where to generate
 * @post by constraining dbmList with constraints
 * the result is not empty (if dbmList is not empty)
 * @return number of generated constraints (may be < n)
 */
static
size_t test_genConstraints(cindex_t dim, const DBMList_t *dbmList,
                             size_t n, constraint_t *constraints)
{
    size_t listSize = dbmf_getSize(dbmList);
    size_t nb = 0;

    if (listSize && n && dim > 1)
    {
        /* pick a DBM */
        const raw_t *dbm = test_getDBM(listSize, dbmList)->dbm;
        uint32_t easy = rand()%(n+1); /* easy constraints 0..n */
        uint32_t i,j,k = 0;
        
        /* generate non tightening constraints */
        do {
            /* pick a constraint */
            do {
                i = rand()%dim;
                j = rand()%dim;
            } while(i == j);
            
            if (dbm[i*dim+j] != dbm_LS_INFINITY)
            {
                constraints[nb].i = i;
                constraints[nb].j = j;
                constraints[nb].value = dbm[i*dim+j] + rand()%4; /* loosen a bit */
                nb++;
            }

        } while(++k < easy); /* try easy times */

        /* generate tightening constraints */
        while(nb < n)
        {
            /* pick a constraint */
            do {
                i = rand()%dim;
                j = rand()%dim;
            } while(i == j);

            constraints[nb].i = i;
            constraints[nb].j = j;

            if (dbm[j*dim+i] == dbm_LS_INFINITY)
            {
                if (dbm[i*dim+j] == dbm_LS_INFINITY)
                {
                    /* anything will do */
                    constraints[nb].value = 1+rand()%1000;
                }
                else if (i != 0 || dbm[i*dim+j] > dbm_LE_ZERO)
                {
                    constraints[nb].value = dbm[i*dim+j] - rand()%10;
                    if (i == 0 && constraints[nb].value < dbm_LE_ZERO)
                    {
                        constraints[nb].value = dbm_LE_ZERO;
                    }
                }
                else
                {
                    constraints[nb].value = dbm_LE_ZERO;
                }
            }
            else /* has to take dbm[j,i] into account */
            {
                if (dbm[i*dim+j] == dbm_LS_INFINITY)
                {
                    constraints[nb].value = 2+rand()%500 - dbm[j*dim+i];
                }
                else
                {
                    raw_t rawRange = dbm[i*dim+j] + dbm[j*dim+i];
                    constraints[nb].value = RAND()%rawRange + 1 - dbm[j*dim+i];
                }
            }
            
            ++nb;
        }
    }
    return nb;
}


/** Mix a DBM list: misuse of quicksort.
 * We can swap indices randomly too, but
 * this is simpler (and quite cool).
 * @param dbmList: list of DBMs
 * @pre size <= 2^16
 */
static
void test_mix(DBMList_t **dbmList)
{
    assert(dbmList);
    if (*dbmList)
    {
        size_t size = dbmf_getSize(*dbmList);
        DBMList_t *dbm[size];
        uint32_t index[size];
        uint32_t i;
        DBMList_t *current;
        assert(size <= 0x10000);

        /* init the array */
        i = 0;
        current = *dbmList;
        do {
            dbm[i] = current;
            index[i] = i | (rand() << 16);
            i++;
            current = current->next;
        } while(current);
        assert(i == size);

        /* permutation */
        base_sort(index, size);

        /* reconstruct the list */
        current = dbm[index[0] & 0xffff];
        *dbmList = current;
        i = 1;
        while(i < size)
        {
            current->next = dbm[index[i] & 0xffff];
            current = current->next;
            ++i;
        }
        current->next = NULL;
    }
}


/** Test dbmf_addZero.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 */
static
void test_addZero(cindex_t dim, DBMAllocator_t *factory)
{
    DBMFederation_t fed;
    DBMList_t *tmp;

    SHOW_TEST();

    tmp = dbmf_allocate(factory);
    if (!tmp) return;

    dbm_zero(tmp->dbm, dim);
    dbmf_initFederation(&fed, dim);

    /* add to empty */
    assert(!dbmf_federationHasDBM(fed, tmp->dbm));
    if (!dbmf_addZero(&fed, factory))
    {
        assert(dbmf_federationHasDBM(fed, tmp->dbm));
 
        /* add to non empty */
        dbm_init(fed.dbmList->dbm, dim);
        /* if dim == 1, init == zero */
        assert(dim == 1 || !dbmf_federationHasDBM(fed, tmp->dbm));
        if (!dbmf_addZero(&fed, factory))
        {
            assert(dbmf_federationHasDBM(fed, tmp->dbm));
        }
        
        /* init still there */
        assert(dbm_isEqualToInit(fed.dbmList->dbm, dim) ||
               dbm_isEqualToInit(fed.dbmList->next->dbm, dim));

        dbmf_deallocateList(factory, fed.dbmList);
    }
    dbmf_deallocate(factory, tmp);
}


/** Test dbmf_addInit.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 */
static
void test_addInit(cindex_t dim, DBMAllocator_t *factory)
{
    DBMFederation_t fed;
    DBMList_t *tmp;

    SHOW_TEST();

    tmp = dbmf_allocate(factory);
    if (!tmp) return;

    dbm_zero(tmp->dbm, dim);
    dbmf_initFederation(&fed, dim);

    /* add to empty */
    if (!dbmf_addInit(&fed, factory))
    {
        assert(fed.dbmList &&
               dbm_isEqualToInit(fed.dbmList->dbm, dim));
        
        /* add to non empty */
        dbm_zero(fed.dbmList->dbm, dim);
        if (!dbmf_addInit(&fed, factory))
        {
            assert(fed.dbmList && fed.dbmList->next);
            
            /* one zero and one init */
            assert(dbm_areEqual(fed.dbmList->dbm, tmp->dbm, dim) ||
                   dbm_areEqual(fed.dbmList->next->dbm, tmp->dbm, dim));
            assert(dbm_isEqualToInit(fed.dbmList->dbm, dim) ||
                   dbm_isEqualToInit(fed.dbmList->next->dbm, dim));
        }

        dbmf_deallocateList(factory, fed.dbmList);
    }
    dbmf_deallocate(factory, tmp);
}


/** Test dbmf_copyDBM.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_copyDBM(cindex_t dim, DBMAllocator_t *factory)
{
    DBMFederation_t fed;
    DBMList_t *tmp;
    uint32_t k;

    SHOW_TEST();

    tmp = dbmf_allocate(factory);
    for (k = 0; k < NB_LOOPS; ++k)
    {
        dbm_generate(tmp->dbm, dim, 1000);
        fed = dbmf_copyDBM(tmp->dbm, dim, factory);
        assert(fed.dbmList &&
               dbm_areEqual(fed.dbmList->dbm, tmp->dbm, dim) &&
               !fed.dbmList->next);
        
        dbmf_deallocate(factory, fed.dbmList);
    }
    dbmf_deallocate(factory, tmp);
}


/** Test dbmf_copyDBM2List.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_copyDBM2List(cindex_t dim, DBMAllocator_t *factory)
{
    DBMList_t *fed;
    DBMList_t *tmp;
    uint32_t k;

    SHOW_TEST();

    tmp = dbmf_allocate(factory);
    for (k = 0; k < NB_LOOPS; ++k)
    {
        dbm_generate(tmp->dbm, dim, 1000);
        fed = dbmf_copyDBM2List(tmp->dbm, dim, factory);
        if (fed)
        {
            assert(dbm_areEqual(fed->dbm, tmp->dbm, dim) &&
                   !fed->next);
            
            dbmf_deallocate(factory, fed);
        }
    }
    dbmf_deallocate(factory, tmp);
}


/** Test dbmf_union.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_union(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *arg, *tmp, *current;
        BOOL superOrDiff, valid;
        PROGRESS();

        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        arg = test_allocGenArg(factory, dim, fed.dbmList, 10, &superOrDiff);
        tmp = dbmf_copyDBMList(arg, dim, &valid, factory);
        assert(superOrDiff == dbmf_union(&fed, arg, factory));

        if (valid && tmp && fed.dbmList)
        {
            /* check tmp <= fed */
            current = tmp;
            while(current)
            {
                assert(test_isIncluded(current->dbm, dim, fed.dbmList));
                current = current->next;
            }
            /* arg belongs to fed or deallocated */
            assert(tmp);
            dbmf_deallocateList(factory, tmp);
        }

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_convexUnion.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_convexUnion(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMList_t *tmp, *arg, *current;
        PROGRESS();
        arg = test_allocGen(factory, dim, fedSize);
        tmp = test_allocGen(factory, dim, 1);
        if (arg && tmp)
        {
            dbmf_convexUnion(tmp->dbm, dim, arg);
            
            /* check arg <= tmp */
            current = arg;
            while(current)
            {
                assert(dbm_relation(current->dbm, tmp->dbm, dim) & base_SUBSET);
                current = current->next;
            }

            dbmf_deallocateList(factory, arg);
        }
        if (tmp) dbmf_deallocate(factory, tmp);
    }
}


/** Test dbmf_convexUnionWithSelf.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_convexUnionWithSelf(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp, *current;
        BOOL valid;
        PROGRESS();

        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
        dbmf_convexUnionWithSelf(&fed, factory);

        if (valid && tmp)
        {
            /* check all tmp <= fed */
            current = tmp;
            while(current)
            {
                assert(test_isIncluded(current->dbm, dim, fed.dbmList));
                current = current->next;
            }
            assert(tmp);
        }
        /* only 1 DBM in fed */
        assert(fedSize == 0 || (fed.dbmList && fed.dbmList->next == NULL));

        if (fedSize)
        {
            if (tmp) dbmf_deallocateList(factory, tmp);
            dbmf_deallocateList(factory, fed.dbmList);
        }
    }
}


/** Test dbmf_intersection.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_intersection(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMList_t *tmp, *arg, *result;
        BOOL dummy, valid;
        PROGRESS();

        tmp = test_allocGen(factory, dim, 1);
        arg = test_allocGenArg(factory, dim, tmp, fedSize+1, &dummy);

        if (tmp)
        {
            result = dbmf_intersection(tmp->dbm, dim, arg, &valid, factory);
            
            /* printf("(%d:%d)", dbmf_getSize(arg), dbmf_getSize(result));
             * to check the relevance of the test */
            
            if (valid)
            {
                if (result)
                {
                    /* intersection != empty */
                    DBMList_t *current;
                    for(current = result; current != NULL; current = current->next)
                    {
                        assert(test_isIncluded(current->dbm, dim, arg));
                        assert(dbm_relation(current->dbm, tmp->dbm, dim) & base_SUBSET);
                    }
                }
                else
                {
                    /* intersection = empty */
                    uint32_t i;
                    double pt[dim];
                    for (i = 0; i < 100; ++i)
                    {
                        /* take a point in tmp, check it is not in arg */
                        if (dbm_generateRealPoint(pt, tmp->dbm, dim))
                        {
                            BOOL inside = FALSE;
                            DBMList_t *current;
                            for(current = arg; current != NULL; current = current->next)
                            {
                                inside |= dbm_isRealPointIncluded(pt, current->dbm, dim);
                            }
                            assert(!inside);
                        }
                    }
                }
            }

            dbmf_deallocate(factory, tmp);
            if (result) dbmf_deallocateList(factory, result);
        }

        if (arg) dbmf_deallocateList(factory, arg);
    }
}


/** Test dbmf_intersectsDBM.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_intersectsDBM(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp, *arg;
        BOOL dummy, valid;
        PROGRESS();
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        arg = test_allocGenArg(factory, dim, fed.dbmList, 1, &dummy);
        tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);

        if (fed.dbmList && arg && valid)
        {
            if (dbmf_intersectsDBM(&fed, arg->dbm, factory))
            {
                /* intersection != empty */
                DBMList_t *current = fed.dbmList;
                assert(current);
                /* result <= both arguments */
                do {
                    assert(test_isIncluded(current->dbm, dim, arg));
                    assert(test_isIncluded(current->dbm, dim, tmp));
                    current = current->next;
                } while(current);
            }
            else
            {
                /* intersection = empty */
                uint32_t i;
                double pt[dim];
                size_t size = dbmf_getSize(tmp);
                for (i = 0; i < 100; ++i)
                {
                    /* take a point in tmp, check it is not in arg */
                    if (test_generateRealPoint(pt, dim, tmp, size))
                    {
                        BOOL inside = FALSE;
                        DBMList_t *current = arg;
                        while(current)
                        {
                            inside |= dbm_isRealPointIncluded(pt, current->dbm, dim);
                            current = current->next;
                        }
                        assert(!inside);
                    }
                }
            }
        }

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
        if (arg) dbmf_deallocateList(factory, arg);
        if (tmp) dbmf_deallocateList(factory, tmp);
    }
}


/** Test dbmf_intersectsFederation.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_intersectsFederation(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp, *arg;
        BOOL dummy, valid;
        PROGRESS();
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        arg = test_allocGenArg(factory, dim, fed.dbmList, rand()%(fedSize+1)+1, &dummy);
        tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);

        if (fed.dbmList && arg && valid &&
            !dbmf_intersectsFederation(&fed, arg, factory))
        {
            if (fed.dbmList)
            {
                /* intersection != empty */
                DBMList_t *current = fed.dbmList;
                assert(current);
                /* result <= both arguments */
                do {
                    assert(test_isIncluded(current->dbm, dim, arg));
                    assert(test_isIncluded(current->dbm, dim, tmp));
                    current = current->next;
                } while(current);
            }
            else
            {
                /* intersection = empty */
                uint32_t i;
                double pt[dim];
                size_t size = dbmf_getSize(tmp);
                for (i = 0; i < 100; ++i)
                {
                    /* take a point in tmp, check it is not in arg */
                    if (test_generateRealPoint(pt, dim, tmp, size))
                    {
                        BOOL inside = FALSE;
                        DBMList_t *current = arg;
                        while(current)
                        {
                            inside |= dbm_isRealPointIncluded(pt, current->dbm, dim);
                            current = current->next;
                        }
                        assert(!inside);
                    }
                }
            }
        }

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
        if (arg) dbmf_deallocateList(factory, arg);
        if (tmp) dbmf_deallocateList(factory, tmp);
    }
}


/** Test dbmf_intersectionWithFederation.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_intersectionWithFederation(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMList_t *fed, *tmp, *arg;
        BOOL dummy, valid;
        PROGRESS();
        fed = test_allocGen(factory, dim, fedSize);
        arg = test_allocGenArg(factory, dim, fed, rand()%(fedSize+1)+1, &dummy);

        if (fed && arg && valid)
        {
            tmp = dbmf_intersectionWithFederation(fed, arg, dim, &valid, factory);
            if (valid)
            {
                if (tmp)
                {
                    /* intersection != empty */
                    DBMList_t *current = tmp;
                    assert(current);
                    /* result <= both arguments */
                    do {
                        assert(test_isIncluded(current->dbm, dim, arg));
                        assert(test_isIncluded(current->dbm, dim, fed));
                        current = current->next;
                    } while(current);
                }
                else
                {
                    /* intersection = empty */
                    uint32_t i;
                    double pt[dim];
                    size_t size = dbmf_getSize(fed);
                    for (i = 0; i < 100; ++i)
                    {
                        /* take a point in fed, check it is not in arg */
                        if (test_generateRealPoint(pt, dim, fed, size))
                        {
                            BOOL inside = FALSE;
                            DBMList_t *current = arg;
                            while(current)
                            {
                                inside |= dbm_isRealPointIncluded(pt, current->dbm, dim);
                                current = current->next;
                            }
                            assert(!inside);
                        }
                    }
                }
            }

            if (tmp) dbmf_deallocateList(factory, tmp);
        }

        if (arg) dbmf_deallocateList(factory, arg);
        if (fed) dbmf_deallocateList(factory, fed);
    }
}


/** Test dbmf_constrainN.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_constrainN(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        constraint_t constraints[dim+dim];
        size_t nbConstraints;
        DBMList_t *current;
        PROGRESS();

        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        nbConstraints = test_genConstraints(dim, fed.dbmList, dim+dim, constraints);
        dbmf_constrainN(&fed, constraints, nbConstraints, factory);

        current = fed.dbmList;
        while(current)
        {
            uint32_t i;
            assert(dbm_isClosed(current->dbm, dim));
            assert(!dbm_isEmpty(current->dbm, dim));
            for (i = 0; i < nbConstraints; ++i)
            {
                assert(dbm_satisfies(current->dbm, dim,
                                     constraints[i].i,
                                     constraints[i].j,
                                     constraints[i].value));
            }
            current = current->next;
        }

        /* can be NULL if fedSize == 0 */
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_constrain1.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_constrain1(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        constraint_t constraint;
        DBMList_t *current;
        PROGRESS();

        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        if (test_genConstraints(dim, fed.dbmList, 1, &constraint))
        {
            dbmf_constrain1(&fed,
                            constraint.i, constraint.j, constraint.value,
                            factory);

            current = fed.dbmList;
            while(current)
            {
                assert(dbm_isClosed(current->dbm, dim));
                assert(!dbm_isEmpty(current->dbm, dim));
                assert(dbm_satisfies(current->dbm, dim,
                                     constraint.i,
                                     constraint.j,
                                     constraint.value));
                current = current->next;
            }
        }

        /* can be NULL if fedSize == 0 */
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_up.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_up(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp, *current1, *current2;
        BOOL valid;
        PROGRESS();

        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);

        dbmf_up(fed);
        current1 = tmp;
        current2 = fed.dbmList;

        if (current2)
        {
            while(current1)
            {
                dbm_up(current1->dbm, dim);
                assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                current1 = current1->next;
                current2 = current2->next;
            }
        }

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
        if (tmp) dbmf_deallocateList(factory, tmp);
    }
}


/** Test dbmf_down.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_down(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp, *current1, *current2;
        BOOL valid;
        PROGRESS();

        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);

        dbmf_down(fed);
        current1 = tmp;
        current2 = fed.dbmList;
        if (current2)
        {
            while(current1)
            {
                dbm_down(current1->dbm, dim);
                assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                current1 = current1->next;
                current2 = current2->next;
            }
        }

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
        if (tmp) dbmf_deallocateList(factory, tmp);
    }
}


/** Test dbmf_reduce.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_reduce(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp;
        BOOL dummy;
        PROGRESS();

        /* - generate seed list
         * - generate argument list from the seed, merge
         * - generate some supersets from the seed, merge
         * - generate some subsets from the seed, merge
         * - reduce
         * - check that there is no inclusion between the DBMs
         */
        
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        tmp = test_allocGenArg(factory, dim, fed.dbmList, fedSize+1, &dummy);
        if (tmp)
        {
            dbmf_appendList(tmp, fed.dbmList);
            fed.dbmList = tmp;
            test_addDBMs(factory, &fed, fedSize);
            dbmf_reduce(&fed, factory);
            tmp = fed.dbmList;
            while(tmp)
            {
                assert(!test_isIncludedInOther(tmp->dbm, dim, fed.dbmList));
                tmp = tmp->next;
            }
        }
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}

/** See if the expensiveReduce test is too expensive
 * by computing the worst case for the number of splits.
 */
static
BOOL isTooExpensive(cindex_t dim, size_t size)
{
    /* never too expensive: return false */
#ifndef NTOOEXPENSIVE
    uint32_t tmp = 1;

    /* Simple cases */
    if (size < 3 || dim < 3) return FALSE;

    /* This greatly reduces recursive splits */
#ifndef NDISJOINT_DBM_SUBSTRACT
    if (dim*dim*dim*size < 4000) return FALSE;
#endif
    
    dim *= dim-1;
    while(size)
    {
        if (dim > 10000) return TRUE;
        if (size & 1) tmp *= dim;
        if (tmp > 200000) return TRUE;
        dim *= dim;
        size >>= 1;
    }
#endif
    return FALSE;
}


/** Test dbmf_expensiveReduce.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_expensiveReduce(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (isTooExpensive(dim, fedSize))
    {
        SKIP_TEST();
    }
    else
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            DBMList_t *tmp;
            BOOL dummy;
            PROGRESS();
            
            /* It is difficult to really test expensiveReduce
             * since it requires to use the substract functions,
             * and thus to re-implement the function.
             * We use the same test as the "cheap" reduce.
             * expensiveReduce must be at least at good as reduce.
             */
            
            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);
            tmp = test_allocGenArg(factory, dim, fed.dbmList, fedSize+1, &dummy);
            if (tmp && fed.dbmList)
            {
                dbmf_appendList(tmp, fed.dbmList);
                fed.dbmList = tmp;
                test_addDBMs(factory, &fed, fedSize);
                dbmf_expensiveReduce(&fed, factory);
                tmp = fed.dbmList;
                while(tmp)
                {
                    assert(!test_isIncludedInOther(tmp->dbm, dim, fed.dbmList));
                    tmp = tmp->next;
                }
            }
            if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
        }
    }
}


/** Test dbmf_freeClock.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_freeClock(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (dim > 1) /* otherwise can't free any clock */
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            DBMList_t *tmp, *current1, *current2;
            cindex_t clock = rand()%(dim-1) + 1; /* 1..dim-1 */
            BOOL valid;
            PROGRESS();
            
            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);
            tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
            
            dbmf_freeClock(fed, clock);
            current1 = tmp;
            current2 = fed.dbmList;
            if (current2)
            {
                while(current1)
                {
                    dbm_freeClock(current1->dbm, dim, clock);
                    assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                    current1 = current1->next;
                    current2 = current2->next;
                }
            }
            
            if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
            if (tmp) dbmf_deallocateList(factory, tmp);
        }
    }
    else
    {
        NO_TEST();
    }
}


/** Test dbmf_updateValue.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_updateValue(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (dim > 1) /* otherwise can't update any clock */
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            DBMList_t *tmp, *current1, *current2;
            cindex_t clock = rand()%(dim-1) + 1; /* 1..dim-1 */
            int32_t value = rand()%100;
            BOOL valid;
            PROGRESS();
            
            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);
            tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
            
            dbmf_updateValue(fed, clock, value);
            current1 = tmp;
            current2 = fed.dbmList;
            if (current2)
            {
                while(current1)
                {
                    dbm_updateValue(current1->dbm, dim, clock, value);
                    assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                    current1 = current1->next;
                    current2 = current2->next;
                }
            }
            
            if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
            if (tmp) dbmf_deallocateList(factory, tmp);
        }
    }
    else
    {
        NO_TEST();
    }
}


/** Test dbmf_updateClock.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_updateClock(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (dim > 2) /* otherwise can't copy any clock */
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            DBMList_t *tmp, *current1, *current2;
            uint32_t i,j;
            BOOL valid;
            PROGRESS();
            
            do {
                i = rand()%(dim-1)+1;
                j = rand()%(dim-1)+1;
            } while(i == j);

            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);
            tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
            
            dbmf_updateClock(fed, i, j);
            current1 = tmp;
            current2 = fed.dbmList;
            if (current2)
            {
                while(current1)
                {
                    dbm_updateClock(current1->dbm, dim, i, j);
                    assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                    current1 = current1->next;
                    current2 = current2->next;
                }
            }
            
            if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
            if (tmp) dbmf_deallocateList(factory, tmp);
        }
    }
    else
    {
        NO_TEST();
    }
}


/** Test dbmf_updateIncrement.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_updateIncrement(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (dim > 1) /* otherwise can't increment any clock */
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            DBMList_t *tmp, *current1, *current2;
            cindex_t clock = rand()%(dim-1) + 1; /* 1..dim-1 */
            int32_t value = rand()%100;
            BOOL valid;
            PROGRESS();
            
            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);
            tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
            
            dbmf_updateIncrement(fed, clock, value);
            current1 = tmp;
            current2 = fed.dbmList;
            if (current2)
            {
                while(current1)
                {
                    dbm_updateIncrement(current1->dbm, dim, clock, value);
                    assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                    current1 = current1->next;
                    current2 = current2->next;
                }
            }
            
            if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
            if (tmp) dbmf_deallocateList(factory, tmp);
        }
    }
    else
    {
        NO_TEST();
    }
}


/** Test dbmf_update.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_update(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (dim > 2) /* otherwise can't copy any clock */
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            DBMList_t *tmp, *current1, *current2;
            uint32_t i,j;
            int32_t value = rand()%1000;
            BOOL valid;
            PROGRESS();
            
            do {
                i = rand()%(dim-1)+1;
                j = rand()%(dim-1)+1;
            } while(i == j);

            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);
            tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
            
            dbmf_update(fed, i, j, value);
            current1 = tmp;
            current2 = fed.dbmList;
            if (current2)
            {
                while(current1)
                {
                    dbm_update(current1->dbm, dim, i, j, value);
                    assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                    current1 = current1->next;
                    current2 = current2->next;
                }
            }
            
            if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
            if (tmp) dbmf_deallocateList(factory, tmp);
        }
    }
    else
    {
        NO_TEST();
    }
}


/** Test dbmf_satisfy.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_satisfies(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (fedSize && dim > 1)
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            uint32_t n = rand()%fedSize+1;
            int32_t pt[dim];
            PROGRESS();
            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);

            if (fed.dbmList)
            {
                /* For some points of the federations:
                 * check that the federation satisfies all the
                 * constraints derived from these points.
                 */
                do {
                    if (test_generatePoint(pt, dim, fed.dbmList, fedSize))
                    {
                        uint32_t i, j;
                        for (i = 0; i < dim; ++i)
                        {
                            for (j = 0; j < dim; ++j)
                            {
                                if (i != j)
                                {
                                    assert(dbmf_satisfies(fed, i, j,
                                                          dbm_bound2raw(pt[i]-pt[j], dbm_WEAK)));
                                }
                            }
                        }
                    }
                } while(--n);
                
                dbmf_deallocateList(factory, fed.dbmList);
            }
        }
    }
    else
    {
        NO_TEST();
    }
}


/** Test dbmf_isEmpty.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_isEmpty(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (fedSize && dim > 1)
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            int32_t pt[dim];
            size_t n = 10;
            BOOL ok;
            PROGRESS();
            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);

            if (fed.dbmList)
            {
                /* Get a point in the federation */
                do {
                    ok = test_generatePoint(pt, dim, fed.dbmList, fedSize);
                } while(!ok && --n);
                
                /* Constrain the federation so that it contains
                 * only this point
                 */
                if (ok)
                {
                    uint32_t i,j;
                    assert(!dbmf_isEmpty(fed));
                    for (i = 0; i < dim; ++i)
                    {
                        for (j = 0; j < dim; ++j)
                        {
                            if (i != j)
                            {
                                dbmf_constrain1(&fed, i, j,
                                                dbm_bound2raw(pt[i]-pt[j], dbm_WEAK),
                                                factory);
                                assert(!dbmf_isEmpty(fed));
                            }
                        }
                    }
                    
                    /* Constrain slightly more: now it must become empty */
                    
                    /* will terminate since dim > 1 */
                    do {
                        i = rand()%dim;
                        j = rand()%dim;
                    } while(i == j);
                    dbmf_constrain1(&fed, i, j,
                                    dbm_bound2raw(pt[i]-pt[j], dbm_STRICT),
                                    factory);
                    assert(dbmf_isEmpty(fed));
                }
                
                if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
            }
        }
    }
    else
    {
        NO_TEST();
    }
}


/** Debug implementation of isUnbounded.
 */
static
BOOL test_dbmfIsUnbounded(const DBMFederation_t fed)
{
    APPLY_TO_FEDERATION(fed,
                        if (dbm_isUnbounded(_first->dbm, fed.dim))
                        return TRUE);
    return FALSE;
}


/** Test dbmf_isUnbounded.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_isUnbounded(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        PROGRESS();
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);

        assert(dbmf_isUnbounded(fed) == test_dbmfIsUnbounded(fed));

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_partialRelation.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_partialRelation(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp;
        PROGRESS();
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        tmp = dbmf_allocate(factory);

        if (dbmf_isEmpty(fed))
        {
            if (tmp)
            {
                dbm_init(tmp->dbm, dim);
                assert(dbmf_partialRelation(tmp->dbm, fed) == base_SUPERSET);
            }
        }
        else if (tmp)
        {
            DBMList_t *current;
            assert(fedSize);

            /* test subset */
            dbm_generateSubset(tmp->dbm,
                               test_getDBM(fedSize, fed.dbmList)->dbm,
                               dim);
            assert(dbmf_partialRelation(tmp->dbm, fed) & base_SUBSET);

            /* test superset */
            dbm_copy(tmp->dbm, fed.dbmList->dbm, dim);
            current = fed.dbmList->next;
            while(current)
            {
                dbm_convexUnion(tmp->dbm, current->dbm, dim);
                current = current->next;
            }
            assert(dbmf_partialRelation(tmp->dbm, fed) & base_SUPERSET);
        }

        if (tmp) dbmf_deallocate(factory, tmp);
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_relation.
 * Simple test: it is difficult to generate overlap cases.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_relation(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp;
        PROGRESS();
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        tmp = dbmf_allocate(factory);

        if (dbmf_isEmpty(fed))
        {
            if (tmp)
            {
                dbm_init(tmp->dbm, dim);
                assert(dbmf_partialRelation(tmp->dbm, fed) == base_SUPERSET);
            }
        }
        else if (tmp)
        {
            DBMList_t *current;
            BOOL valid;
            assert(fedSize);

            /* test subset */
            dbm_generateSubset(tmp->dbm,
                               test_getDBM(fedSize, fed.dbmList)->dbm,
                               dim);
            assert((dbmf_relation(tmp->dbm, fed, &valid, factory) & base_SUBSET) || !valid);

            /* test superset */
            dbm_copy(tmp->dbm, fed.dbmList->dbm, dim);
            current = fed.dbmList->next;
            while(current)
            {
                dbm_convexUnion(tmp->dbm, current->dbm, dim);
                current = current->next;
            }
            assert((dbmf_relation(tmp->dbm, fed, &valid, factory) & base_SUPERSET) || !valid);
        }

        if (tmp) dbmf_deallocate(factory, tmp);
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_stretchUp.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_stretchUp(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp, *current1, *current2;
        BOOL valid;
        PROGRESS();
        
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
        
        dbmf_stretchUp(fed);
        current1 = tmp;
        current2 = fed.dbmList;
        if (current2)
        {
            while(current1)
            {
                dbm_freeAllUp(current1->dbm, dim);
                assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                current1 = current1->next;
                current2 = current2->next;
            }
        }

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
        if (tmp) dbmf_deallocateList(factory, tmp);
    }
}


/** Test dbmf_stretchDown.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_stretchDown(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    if (dim > 1) /* otherwise can't free any clock */
    {
        uint32_t k;
        SHOW_TEST();
        for (k = 0; k < NB_LOOPS; ++k)
        {
            DBMFederation_t fed;
            DBMList_t *tmp, *current1, *current2;
            cindex_t clock = rand()%(dim-1) + 1; /* 1..dim-1 */
            BOOL valid;
            PROGRESS();
            
            fed.dim = dim;
            fed.dbmList = test_allocGen(factory, dim, fedSize);
            tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
            
            dbmf_stretchDown(fed, clock);
            current1 = tmp;
            current2 = fed.dbmList;
            if (current2)
            {
                while(current1)
                {
                    dbm_freeDown(current1->dbm, dim, clock);
                    assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                    current1 = current1->next;
                    current2 = current2->next;
                }
            }

            if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
            if (tmp) dbmf_deallocateList(factory, tmp);
        }
    }
    else
    {
        NO_TEST();
    }
}


/** Test dbmf_microDelay.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_microDelay(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *tmp, *current1, *current2;
        BOOL valid;
        PROGRESS();
        
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        tmp = dbmf_copyDBMList(fed.dbmList, dim, &valid, factory);
        
        dbmf_microDelay(fed);
        current1 = tmp;
        current2 = fed.dbmList;
        if (current2)
        {
            while(current1)
            {
                assert(current2);
                dbm_relaxUp(current1->dbm, dim);
                assert(dbm_areEqual(current1->dbm, current2->dbm, dim));
                current1 = current1->next;
                current2 = current2->next;
            }
        }
        
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
        if (tmp) dbmf_deallocateList(factory, tmp);
    }
}


/** Test dbmf_copy.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_copy(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t src, dst;
        DBMList_t *s, *d;
        PROGRESS();
        src.dim = dim;
        src.dbmList = test_allocGen(factory, dim, fedSize);
        dst.dbmList = NULL;
        if (rand() & 1)
        {
            dbmf_initFederation(&dst, dim);
        }
        else
        {
            assert(dim);
            dst.dim = rand()%dim+1; /* 1..dim */
            dst.dbmList = test_allocGen(factory, dst.dim, rand()%(fedSize+1));
        }

        if (!dbmf_copy(&dst, src, factory))
        {
            assert(dst.dim == src.dim);
            s = src.dbmList;
            d = dst.dbmList;
            while(s)
            {
                assert(d && dbm_areEqual(s->dbm, d->dbm, dim));
                s = s->next;
                d = d->next;
            }
            assert(!d);
        }
        if (src.dbmList) dbmf_deallocateList(factory, src.dbmList);
        if (dst.dbmList) dbmf_deallocateList(factory, dst.dbmList);
    }
}


/** Test dbmf_areEqual.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_areEqual(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed1, fed2;
        BOOL valid;
        PROGRESS();
        fed1.dim = dim;
        fed1.dbmList = test_allocGen(factory, dim, fedSize);
        dbmf_initFederation(&fed2, dim);

        if (!dbmf_copy(&fed2, fed1, factory))
        {
            assert(dbmf_areEqual(fed1, fed2, &valid, factory) || !valid);
            test_mix(&fed2.dbmList);
            assert(dbmf_areEqual(fed1, fed2, &valid, factory) || !valid);
            assert(dbmf_areEqual(fed2, fed1, &valid, factory) || !valid);
            dbmf_reduce(&fed1, factory);
            dbmf_reduce(&fed2, factory);
            assert(dbmf_areEqual(fed1, fed2, &valid, factory) || !valid);
            assert(dbmf_areEqual(fed2, fed1, &valid, factory) || !valid);
        
            if (dim > 1 && fed2.dbmList)
            {
                uint32_t p,i,j;
                int32_t pt[dim];
                BOOL constrained = FALSE;
                
                for(p = 0; p < 5 && fed2.dbmList; ++p)
                {
                    size_t fedSize2 = dbmf_getSize(fed2.dbmList);
                    size_t n = fedSize2;
                    BOOL ok = FALSE;
                    
                    /* Get a point in the federation */
                    while(n && !ok)
                    {
                        ok = test_generatePoint(pt, dim, fed2.dbmList, fedSize2);
                        --n;
                    }
                    if (ok)
                    {
                        do {
                            i = rand()%dim;
                            j = rand()%dim;
                        } while (i == j);
                        assert(test_isPointIn(pt, fed2.dbmList, dim));
                        assert(dbmf_isPointIncluded(pt, fed2));
                        /* constrain with this point */
                        dbmf_constrain1(&fed2, i, j,
                                        dbm_bound2raw(pt[i]-pt[j], dbm_STRICT),
                                        factory);
                        assert(!test_isPointIn(pt, fed2.dbmList, dim));
                        assert(!dbmf_isPointIncluded(pt, fed2));
                        constrained = TRUE;
                    }
                }
                
                if (constrained)
                {
                    if (dbmf_areEqual(fed1, fed2, &valid, factory))
                    {
                        dbmf_print(stdout, fed1.dbmList, dim);
                        dbmf_print(stdout, fed2.dbmList, dim);
                    }
                    assert(!dbmf_areEqual(fed1, fed2, &valid, factory) || !valid);
                    assert(!dbmf_areEqual(fed2, fed1, &valid, factory) || !valid);
                }
            }
            
            if (fed2.dbmList) dbmf_deallocateList(factory, fed2.dbmList);
        }
        if (fed1.dbmList) dbmf_deallocateList(factory, fed1.dbmList);
    }
}


/** Test dbmf_isPointIncluded.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_isPointIncluded(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        int32_t pt[dim];
        DBMList_t *current;
        PROGRESS();
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);

        current = fed.dbmList;
        while(current)
        {
            size_t n = 5;
            do {
                if (dbm_generatePoint(pt, current->dbm, dim))
                {
                    assert(dbmf_isPointIncluded(pt, fed));
                }
            } while(--n);
            current = current->next;
        }

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_substractFederationFromDBM.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_subFedFromDBM(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMFederation_t dbm;
        DBMList_t *result = NULL;
        size_t resultSize, n = fedSize;
        double pt[dim];
        BOOL valid;
        PROGRESS();

        dbm.dim = dim;
        dbm.dbmList = dbmf_allocate(factory);
        fed.dim = dim;
        fed.dbmList = NULL;
        if (dbm.dbmList)
        {
            dbm_generate(dbm.dbmList->dbm, dim, 1000);
            dbm.dbmList->next = NULL;
            while(n)
            {
                DBMList_t *tmp = dbmf_allocate(factory);
                tmp->next = fed.dbmList;
                fed.dbmList = tmp;
                dbm_generateArgDBM(tmp->dbm, dbm.dbmList->dbm, dim);
                --n;
            }
            
            result = dbmf_substractFederationFromDBM(dbm.dbmList->dbm, fed, &valid, factory);
            if (valid)
            {
                resultSize = dbmf_getSize(result);
                n = 3*dim*(fedSize+1);
                while(n)
                {
                    if (test_generateRealPoint(pt, dim, result, resultSize))
                    {
                        assert(test_isRealPointIn(pt, result, dim));
                        assert(test_isRealPointIn(pt, dbm.dbmList, dim));
                        assert(dbmf_isRealPointIncluded(pt, dbm));
                        assert(!test_isRealPointIn(pt, fed.dbmList, dim));
                        assert(!dbmf_isRealPointIncluded(pt, fed));
                    }
                    --n;
                }
            }

            dbmf_deallocate(factory, dbm.dbmList);
        }

        if (result) dbmf_deallocateList(factory, result);
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_substractDBMFromDBM.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_subDBMFromDBM(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMList_t *dbm1, *dbm2, *result = NULL;
        size_t n, resultSize;
        double pt[dim];
        BOOL valid;
        PROGRESS();

        dbm1 = dbmf_allocate(factory);
        dbm2 = dbmf_allocate(factory);
        if (dbm1 && dbm2)
        {
            dbm_generate(dbm1->dbm, dim, 1000);
            dbm_generateArgDBM(dbm2->dbm, dbm1->dbm, dim);
            
            result = dbmf_substractDBMFromDBM(dbm1->dbm, dbm2->dbm, dim, &valid, factory);
            if (valid)
            {
                resultSize = dbmf_getSize(result);
                n = 3*dim;
                while(n)
                {
                    if (test_generateRealPoint(pt, dim, result, resultSize))
                    {
                        assert(test_isRealPointIn(pt, result, dim));
                        assert(dbm_isRealPointIncluded(pt, dbm1->dbm, dim));
                        assert(!dbm_isRealPointIncluded(pt, dbm2->dbm, dim));
                    }
                    --n;
                }
            }
        }

        if (result) dbmf_deallocateList(factory, result);
        if (dbm1) dbmf_deallocate(factory, dbm1);
        if (dbm2) dbmf_deallocate(factory, dbm2);
    }
}


/** Test dbmf_substractDBMFromFederation.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_subDBMFromFed(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed, result;
        DBMList_t *arg;
        size_t n, resultSize;
        double pt[dim];
        BOOL valid;
        PROGRESS();
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        result.dim = dim;
        result.dbmList = NULL;
        arg = dbmf_allocate(factory);
        if (arg)
        {
            if (fedSize)
            {
                dbm_generateArgDBM(arg->dbm, test_getDBM(fedSize, fed.dbmList)->dbm, dim);
            }
            else
            {
                dbm_generate(arg->dbm, dim, 1000);
            }
        
            result = dbmf_substractDBMFromFederation(fed, arg->dbm, &valid, factory);
            if (valid)
            {
                n = 3*dim*(fedSize+1);
                resultSize = dbmf_getSize(result.dbmList);
                while(n)
                {
                    if (test_generateRealPoint(pt, dim, result.dbmList, resultSize))
                    {
                        assert(test_isRealPointIn(pt, result.dbmList, dim));
                        assert(test_isRealPointIn(pt, fed.dbmList, dim));
                        assert(!dbm_isRealPointIncluded(pt, arg->dbm, dim));
                    }
                    --n;
                }
            }
            dbmf_deallocate(factory, arg);
        }

        if (result.dbmList) dbmf_deallocateList(factory, result.dbmList);
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_substractFederationFromFederation.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_subFedFromFed(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed, arg, result;
        size_t n, resultSize, argSize = rand()%(fedSize+2);
        double pt[dim];
        BOOL valid;
        PROGRESS();

        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        dbmf_initFederation(&arg, dim);

        n = argSize;
        while(n)
        {
            DBMList_t *tmp = dbmf_allocate(factory);
            if (tmp)
            {
                tmp->next = arg.dbmList;
                arg.dbmList = tmp;
                if (fed.dbmList)
                {
                    dbm_generateArgDBM(tmp->dbm, test_getDBM(fedSize, fed.dbmList)->dbm, dim);
                }
                else
                {
                    dbm_generate(tmp->dbm, dim, 1000);
                }
            }
            --n;
        }

        result = dbmf_substractFederationFromFederation(fed, arg, &valid, factory);
        if (valid)
        {
            resultSize = dbmf_getSize(result.dbmList);
            n = 3*dim*(fedSize+1);
            while(n)
            {
                if (test_generateRealPoint(pt, dim, result.dbmList, resultSize))
                {
                    assert(test_isRealPointIn(pt, result.dbmList, dim));
                    assert(test_isRealPointIn(pt, fed.dbmList, dim));
                    assert(!test_isRealPointIn(pt, arg.dbmList, dim));
                }
                --n;
            }
        }

        if (result.dbmList) dbmf_deallocateList(factory, result.dbmList);
        if (arg.dbmList) dbmf_deallocateList(factory, arg.dbmList);
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_substractDBM.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_subDBM(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed;
        DBMList_t *arg;
        size_t n, resultSize;
        double pt[dim];
        PROGRESS();
        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        arg = dbmf_allocate(factory);
        if (arg)
        {
            if (fed.dbmList)
            {
                dbm_generateArgDBM(arg->dbm, test_getDBM(fedSize, fed.dbmList)->dbm, dim);
            }
            else
            {
                dbm_generate(arg->dbm, dim, 1000);
            }
        
            if (!dbmf_substractDBM(&fed, arg->dbm, factory))
            {
                resultSize = dbmf_getSize(fed.dbmList);
                n = 3*dim*(fedSize+1);
                while(n)
                {
                    if (test_generateRealPoint(pt, dim, fed.dbmList, resultSize))
                    {
                        assert(test_isRealPointIn(pt, fed.dbmList, dim));
                        assert(!dbm_isRealPointIncluded(pt, arg->dbm, dim));
                    }
                    --n;
                }
            }

            dbmf_deallocate(factory, arg);
        }

        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Test dbmf_substractFederation.
 * @param dim: dimension of DBMs to test.
 * @param factory: DBM factory.
 * @param fedSize: size of federation to test.
 */
static
void test_subFed(cindex_t dim, DBMAllocator_t *factory, size_t fedSize)
{
    uint32_t k;
    SHOW_TEST();
    for (k = 0; k < NB_LOOPS; ++k)
    {
        DBMFederation_t fed, arg;
        size_t n, resultSize, argSize = rand()%(fedSize+2);
        double pt[dim];
        PROGRESS();

        fed.dim = dim;
        fed.dbmList = test_allocGen(factory, dim, fedSize);
        dbmf_initFederation(&arg, dim);
        n = argSize;
        while(n)
        {
            DBMList_t *tmp = dbmf_allocate(factory);
            if (tmp)
            {
                tmp->next = arg.dbmList;
                arg.dbmList = tmp;
                if (fed.dbmList)
                {
                    dbm_generateArgDBM(tmp->dbm, test_getDBM(fedSize, fed.dbmList)->dbm, dim);
                }
                else
                {
                    dbm_generate(tmp->dbm, dim, 1000);
                }
            }
            --n;
        }

        if (!dbmf_substractFederation(&fed, arg.dbmList, factory))
        {
            resultSize = dbmf_getSize(fed.dbmList);
            n = 3*dim*(fedSize+1);
            while(n)
            {
                if (test_generateRealPoint(pt, dim, fed.dbmList, resultSize))
                {
                    assert(test_isRealPointIn(pt, fed.dbmList, dim));
                    assert(!test_isRealPointIn(pt, arg.dbmList, dim));
                }
                --n;
            }
        }

        if (arg.dbmList) dbmf_deallocateList(factory, arg.dbmList);
        if (fed.dbmList) dbmf_deallocateList(factory, fed.dbmList);
    }
}


/** Main test function: from dim to dimEnd and
 * call all the individual tests.
 * @param dim: starting dimension
 * @param dimEnd: ending dimension
 * @param fedSize: federation size to test
 */
static
void test(cindex_t dim, cindex_t dimEnd, size_t fedSize)
{
    DBMAllocator_t factory;
    dbmf_initFactory(&factory, dimEnd);

    while(dim <= dimEnd)
    {
        printf("Testing size %d ", dim);
        test_addZero(dim, &factory);
        test_addInit(dim, &factory);
        test_copyDBM(dim, &factory);
        test_copyDBM2List(dim, &factory);
        test_union(dim, &factory, fedSize);
        test_convexUnion(dim, &factory, fedSize);
        test_convexUnionWithSelf(dim, &factory, fedSize);
        test_intersection(dim, &factory, fedSize);
        test_intersectsDBM(dim, &factory, fedSize);
        test_intersectsFederation(dim, &factory, fedSize);
        test_intersectionWithFederation(dim, &factory, fedSize);
        test_constrainN(dim, &factory, fedSize);
        test_constrain1(dim, &factory, fedSize);
        test_up(dim, &factory, fedSize);
        test_down(dim, &factory, fedSize);
        test_freeClock(dim, &factory, fedSize);
        test_updateValue(dim, &factory, fedSize);
        test_updateClock(dim, &factory, fedSize);
        test_updateIncrement(dim, &factory, fedSize);
        test_update(dim, &factory, fedSize);
        test_satisfies(dim, &factory, fedSize);
        test_isEmpty(dim, &factory, fedSize);
        test_isUnbounded(dim, &factory, fedSize);
        test_stretchUp(dim, &factory, fedSize);
        test_stretchDown(dim, &factory, fedSize);
        test_microDelay(dim, &factory, fedSize);
        test_copy(dim, &factory, fedSize);
        test_areEqual(dim, &factory, fedSize);
        test_isPointIncluded(dim, &factory, fedSize);
        test_partialRelation(dim, &factory, fedSize);
        test_reduce(dim, &factory, fedSize);
        test_expensiveReduce(dim, &factory, fedSize);
        test_relation(dim, &factory, fedSize);
        test_subFedFromDBM(dim, &factory, fedSize);
        test_subDBMFromDBM(dim, &factory, fedSize);
        test_subDBMFromFed(dim, &factory, fedSize);
        test_subFedFromFed(dim, &factory, fedSize);
        test_subDBM(dim, &factory, fedSize);
        test_subFed(dim, &factory, fedSize);
        /* TODO: test_predt */
        printf(" \n");
        dim++;
    }

    dbmf_clearFactory(&factory);
}


int main(int argc, char *argv[])
{
    uint32_t start, end, fedSize, seed;

    if (argc < 4)
    {
        fprintf(stderr,
                "Usage: %s start_dim end_dim federation_size [seed]\n",
                argv[0]);
        return 1;
    }

    start = atoi(argv[1]);
    end = atoi(argv[2]);
    fedSize = atoi(argv[3]);
    seed = argc > 4 ? atoi(argv[4]) : time(NULL);
    srand(seed);

    if (start < 1)
    {
        fprintf(stderr, "start_dim must be >= 1\n");
        return 1;
    }

    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    printf("Test with seed=%d\n",seed);
    test(start, end, fedSize);

    printf("Passed\n");
    return 0;
}

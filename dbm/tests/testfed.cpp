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
// Filename : testfed.cpp
//
// Test of fed_t.
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testfed.cpp,v 1.21 2005/08/04 13:36:19 adavid Exp $
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <time.h>
#include <iostream>
#include "dbm/fed.h"
#include "dbm/gen.h"
#include "dbm/print.h"
#include "dbm/dbmfederation.h"
#include "debug/utils.h"
#include "debug/macros.h"

using namespace std;
using namespace dbm;

#ifdef VERBOSE
#define SHOW_TEST() (cout << __FUNCTION__ << '.').flush()
#define SKIP_TEST() (cout << __FUNCTION__ << '?').flush()
#define NO_TEST()   (cout << __FUNCTION__ << '*').flush()
#else
#define SHOW_TEST() (cout << '.').flush()
#define SKIP_TEST() (cout << '?').flush()
#define NO_TEST()   (cout << '*').flush()
#endif

#define PROGRESS() debug_spin(stderr)

#define NB_LOOPS 250


// Skip too expensive tests
#define CHEAP dim <= 100 && size <= 100


/** Test that a discrete point is in a DBM list
 * @param pt: point
 * @param fed: federation of DBMs.
 * @param dim: dimension of the DBMs and the point
 * @return true if pt included in dbmList, false otherwise
 */
static
bool test_isPointIn(const int32_t *pt, const fed_t& fed, cindex_t dim)
{
    bool inside = false;
    for(fed_t::const_iterator iter(fed); !iter.null(); ++iter)
    {
        inside = inside || dbm_isPointIncluded(pt, iter(), dim);
    }
    return inside;
}

/** Test that a real point is in a DBM list
 * @param pt: point
 * @param fed: federation of DBMs.
 * @param dim: dimension of the DBMs and the point
 * @return true if pt included in dbmList, false otherwise
 */
static
bool test_isRealPointIn(const double *pt, const fed_t& fed, cindex_t dim)
{
    bool inside = false;
    for(fed_t::const_iterator iter(fed); !iter.null(); ++iter)
    {
        inside = inside || dbm_isRealPointIncluded(pt, iter(), dim);
    }
    return inside;
}

/** Generate a federation of  n DBMs
 * @param dim: dimension of DBMs to generate
 * @param n: number of DBMs to generate.
 * @return n allocated and generated DBMs.
 */
static
fed_t test_gen(cindex_t dim, size_t n)
{
    fed_t result(dim);
    if (dim > 0)
    {
        if (dim == 1)
        {
            if (n) result.setInit();
        }
        else
        {
            for(; n ; --n)
            {
                dbm_t dbm(dim);
                dbm_generate(dbm.getDBM(), dim, 1000);
                result.add(dbm);
            }
        }
    }
    return result;
}

/** Pick one DBM from a federation of DBMs.
 * @param fed: the federation.
 * @pre fed.size() > 0, fed.getDimension() > 1, otherwise no DBM to return
 */
static
const dbm_t& test_getDBM(const fed_t& fed)
{
    fed_t::const_iterator iter(fed);
    for(size_t size = rand()%fed.size(); size != 0; --size, ++iter);
    return *iter;
}

/** Generate a real point in a federation.
 * @param fed: federation.
 * @param pt: where to write the point = double[fed.getDimension()]
 * @pre fed.size() > 0, fed.getDimension() > 1, otherwise no point to return
 */
static
bool test_generateRealPoint(double *pt, const fed_t& fed)
{
    return fed.getDimension() > 1 && !fed.isEmpty() &&
        dbm_generateRealPoint(pt, test_getDBM(fed)(), fed.getDimension());
}

/** Generate a discrete point in a federation.
 * @param fed: federation.
 * @param pt: where to write the point = int[fed.getDimension()]
 * @pre fed.size() >0, fed.getDimension() > 1, otherwise no point to return
 */
static
bool test_generatePoint(int32_t *pt, const fed_t& fed)
{
    return fed.getDimension() > 1 && !fed.isEmpty() &&
        dbm_generatePoint(pt, test_getDBM(fed)(), fed.getDimension());
}

///< Add some superset/subset DBMs to a federation.
///< @pre fed.getDimension() > 1
///< @param fed: federation to change.
///< @param nb: number of DBMs to add.
static
void test_addDBMs(fed_t& fed, size_t nb)
{
    if (fed.size())
    {
        for(cindex_t dim = fed.getDimension(); nb; --nb)
        {
            const raw_t *inList = test_getDBM(fed)();
            dbm_t dbm(dim);
            if (rand() & 1)
            {
                dbm_generateSuperset(dbm.getDBM(), inList, dim);
            }
            else
            {
                dbm_generateSubset(dbm.getDBM(), inList, dim);
            }
            fed.add(dbm);
        }
    }
}


/** Generate arguments for an operation with a federation.
 * @param n: max size of the generated federation.
 * @param fed: base federation.
 */
static
fed_t test_genArg(size_t n, const fed_t& fed)
{
    cindex_t dim = fed.getDimension();
    fed_t result(dim);
    n = rand()%(n+1);
    if (fed.isEmpty())
    {
        return test_gen(dim, n);
    }
    for( ; n; --n)
    {
        const dbm_t& inList = test_getDBM(fed);
        dbm_t dbm(dim);
        switch(rand()%4)
        {
        case 0: // diff
            dbm_generate(dbm.getDBM(), dim, 1000);
            break;
        case 1: // equal
            dbm = inList;
            break;
        case 2: // subset
            dbm_generateSubset(dbm.getDBM(), inList(), dim);
            break;
        case 3: // superset
            dbm_generateSuperset(dbm.getDBM(), inList(), dim);
            break;
        }
        result.add(dbm);
    }
    return result;
}

/** Generate constraints for a federation.
 * @param fed: federation.
 * @param n: number of constraints to generate
 * @param constraints: where to generate
 * @post by constraining fed with constraints
 * the result is not empty (if fed is not empty)
 * @return number of generated constraints (may be < n)
 */
static
size_t test_genConstraints(const fed_t& fed, size_t n, constraint_t* constraints)
{
    size_t listSize = fed.size();
    size_t nb = 0;
    cindex_t dim = fed.getDimension();

    if (listSize && n && dim > 1)
    {
        const raw_t *dbm = test_getDBM(fed)(); // pick a DBM
        uint32_t easy = rand()%(n+1); // nb of easy constraints 0..n
        uint32_t i,j,k = 0;
        
        do { // generate non tightening constraints
            do { // pick a constraint
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

        } while(++k < easy);

        // generate tightening constraints
        while(nb < n)
        {
            // pick a constraint
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
                    // anything will do
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
            else // has to take dbm[j,i] into account
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
void test_mix(fed_t& fed)
{
    size_t size = fed.size();
    if (size > 0)
    {
        fdbm_t *oldlist[size];
        fdbm_t *newlist[size];
        uint32_t index[size];
        size_t i;
        assert(fed.write(oldlist) == size);
        for(i = 0; i < size; ++i)
        {
            index[i] = i | (rand() << 16);
        }
        base_sort(index, size); // permutation
        for(i = 0; i < size; ++i)
        {
            newlist[i] = oldlist[index[i] & 0xffff];
        }
        fed.read(newlist, size);
    }
    assert(!fed.hasEmpty());
}


// Global allocator for old federations
DBMAllocator_t factory;

// Declare and init a DBMFederation_t
#define NEW_FED(NAME, DIM) DBMFederation_t NAME; dbmf_initFederation(&NAME, DIM)

// Deallocate a DBMFederation_t
#define DEL_FED(NAME) dbmf_deallocateFederation(&factory, &NAME)

// Convert a fed_t to a DBMFederation_t
static
DBMFederation_t test_new2old(const fed_t& fed)
{
    cindex_t dim = fed.getDimension();
    NEW_FED(result, dim);
    for(fed_t::const_iterator i = fed.begin(); !i.null(); ++i)
    {
        DBMList_t *current = dbmf_allocate(&factory);
        dbm_copy(current->dbm, i->const_dbm(), dim);
        dbmf_addDBM(current, &result);
    }
    return result;
}

// Convert a DBMFederation_t to a fed_t
static
fed_t test_old2new(const DBMFederation_t& fed)
{
    fed_t result(fed.dim);
    for(const DBMList_t *i = fed.dbmList; i != NULL; i = i->next)
    {
        result.add(i->dbm, fed.dim);
    }
    return result;
}

// Test setZero
static
void test_setZero(cindex_t dim)
{
    SHOW_TEST();
    fed_t fed(dim);
    fed.setZero();
    assert(dim > 0);
    assert(fed.size() == 1);
    assert(!fed.isEmpty());
    fed_t::const_iterator iter(fed);
    assert(!iter.null());
    assert((*iter).isZero());
    ++iter;
    assert(iter.null());
    fed.nil();
    assert(fed.isEmpty());
    fed.setDimension(dim);
    fed.setZero();
    iter = fed_t::const_iterator(fed);
    assert(!iter.null());
    assert((*iter).isZero());
    assert((*iter).getDimension() == fed.getDimension() && fed.getDimension() == dim);
    ++iter;
    assert(iter.null());

    NEW_FED(old, dim);
    if (!dbmf_addZero(&old, &factory))
    {
        assert(test_old2new(old) == fed);
    }
    DEL_FED(old);
}

// Test setInit
static
void test_setInit(cindex_t dim)
{
    SHOW_TEST();
    fed_t fed(dim);
    fed.setInit();
    assert(dim > 0);
    assert(fed.size() == 1);
    assert(!fed.isEmpty());
    fed_t::const_iterator iter(fed);
    assert(!iter.null());
    assert((*iter).isInit());
    ++iter;
    assert(iter.null());
    fed.nil();
    assert(fed.isEmpty());
    fed.setDimension(dim);
    fed.setInit();
    iter = fed_t::const_iterator(fed);
    assert(!iter.null());
    assert((*iter).isInit());
    assert((*iter).getDimension() == fed.getDimension() && fed.getDimension() == dim);
    ++iter;
    assert(iter.null());

    NEW_FED(old, dim);
    if (!dbmf_addInit(&old, &factory))
    {
        assert(test_old2new(old) == fed);
    }
    DEL_FED(old);
}

// Test copy -- no real copy, since copy on write!
static
void test_copy(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1; // copy constructor
        uint32_t h = fed1.hash();
        assert(fed1.getDimension() == dim);
        assert(fed1.getDimension() == fed2.getDimension());
        assert(fed1.size() == fed2.size());
        assert(fed2.hash() == h);
        for(fed_t::const_iterator iter(fed1); !iter.null(); ++iter)
        {
            assert(fed2.hasSame(*iter));
        }
        fed1.nil();
        fed1 = fed2; // copy
        assert(fed1.hash() == h);
        assert(fed1.getDimension() == dim);
        assert(fed1.getDimension() == fed2.getDimension());
        assert(fed1.size() == fed2.size());
        for(fed_t::const_iterator iter(fed1); !iter.null(); ++iter)
        {
            assert(fed2.hasSame(*iter));
        }
        // preview on relations
        assert(fed1 == fed2 && fed1 >= fed2 && fed1 <= fed2);
        assert(fed1.eq(fed2) && fed1.ge(fed1) && fed1.le(fed2));
        fed1.nil();
        fed2.nil();
        if (dim > 0)
        {
            dbm_t dbm(dim);
            dbm_generate(dbm.getDBM(), dim, 1000);
            fed1 = dbm; // copy DBM
            fed2.setDimension(dim); // need to know dim for matrix copy
            fed2 = dbm(); // copy matrix of raw_t
            assert(fed1 == fed2);
            assert(fed1 == dbm);
            assert(fed2.eq(fed1));
            assert(fed1.size() == 1);
            assert(dbm == *fed_t::const_iterator(fed1));
        }
    }
}

// Test |=
static
void test_union(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2(test_gen(dim, size));
        DBMFederation_t f1 = test_new2old(fed1);
        DBMFederation_t f2 = test_new2old(fed2);
        fed_t fed3 = fed1;
        uint32_t h = fed1.hash();
        fed1 |= fed2;
        dbmf_union(&f1, f2.dbmList, &factory);
        f2.dbmList = NULL;
        assert(test_old2new(f1) == fed1);
        DEL_FED(f1);
        size_t siz = fed1.size();
        fed1 |= fed2;
        assert(fed1.size() == siz); // no DBM added
        assert(fed2 <= fed1 && fed2.le(fed1) && fed1.ge(fed2) && fed1 >= fed2);
        assert(fed3.hash() == h); // no side effect with ref etc..
        for(fed_t::const_iterator iter(fed2); !iter.null(); ++iter)
        {
            assert(*iter <= fed1);
            fed3 |= *iter;
        }
        assert(fed1 == fed3 && fed1 >= fed3 && fed1 <= fed3 && fed3 == fed1);
        assert(fed1.eq(fed3) && fed1.ge(fed3) && fed1.le(fed3) && fed3.eq(fed1));
        fed1.setDimension(dim); // empty fed too
        fed1 |= fed2;
        assert(fed1 == fed2 && fed1 >= fed2 && fed1 <= fed2 && fed2 == fed1);
        assert(fed1.eq(fed2) && fed1.ge(fed2) && fed1.le(fed2) && fed2.eq(fed1));
        fed1.reduce();
        fed2 = test_genArg(size, fed1);
        fed1.removeIncludedIn(fed2);
        for(fed_t::const_iterator iter(fed1); !iter.null(); ++iter)
        {
            assert(!(*iter <= fed2));
        }
    }
}

// Test +=
static
void test_convexUnion(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2(test_gen(dim, size));
        dbm_t dbm(dim);
        uint32_t h1 = fed1.hash();
        uint32_t h2 = fed2.hash();
        dbm_generate(dbm.getDBM(), dim, 1000);
        uint32_t h3 = dbm.hash();
        fed_t fed3 = fed1 + fed2;
        assert(h1 == fed1.hash() && h2 == fed2.hash());
        assert(fed1 <= fed3 && fed2 <= fed3 && fed3 >= fed1 && fed3 >= fed2);
        assert(fed1.le(fed3) && fed2.le(fed3) && fed3.ge(fed1) && fed3.ge(fed2));
        assert(fed3.size() == 1);
        fed3 += dbm;
        assert(dbm.hash() == h3);
        assert(dbm <= fed3);
        fed3 = fed1;
        fed3.convexHull();
        assert(fed1.hash() == h1);
        assert(fed3.ge(fed1) && fed3 >= fed1 && fed3.size() == 1);
    }
}

// test &= intersection
static
void test_intersection(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        BOOL valid = TRUE;
        fed_t fed1(test_gen(dim, size));
        fed_t fed2(test_genArg(size, fed1));
        uint32_t h1 = fed1.hash();
        uint32_t h2 = fed2.hash();
        fed_t fed3 = fed1 & fed2;
        DBMFederation_t f1 = test_new2old(fed1);
        DBMFederation_t f2 = test_new2old(fed2);
        NEW_FED(f3, dim);
        f3.dbmList = dbmf_intersectionWithFederation(f1.dbmList, f2.dbmList, dim, &valid, &factory);
        if (valid)
        {
            assert(test_old2new(f3).eq(fed3));
        }
        DEL_FED(f1);
        DEL_FED(f2);
        DEL_FED(f3);
        assert(fed1.hash() == h1 && fed2.hash() == h2);
        dbm_t dbm2(dim);
        if (!fed2.isEmpty()) dbm2 = *fed_t::const_iterator(fed2); // 1st DBM thanks
        uint32_t hd = dbm2.hash();
        fed_t fed4 = dbm2 & fed1;
        assert(dbm2.hash() == hd);
        assert((fed1 & fed2).eq(fed2 & fed1));
        assert(fed3.le(fed1) && fed3.le(fed2));
        assert(fed4.le(fed1) && fed4.le(dbm2));
        for(fed_t::const_iterator iter(fed3); !iter.null(); ++iter)
        {
            assert(iter->le(fed1) && iter->le(fed2));
        }
        for(fed_t::const_iterator iter(fed4); !iter.null(); ++iter)
        {
            assert(iter->le(fed1) && iter->le(dbm2));
        }
        if (fed3.isEmpty())
        {
            double pt[dim];
            for(int i = 0; i < 500; ++i)
            {
                // pt in fed1 not in fed2
                if (test_generateRealPoint(pt, fed1))
                {
                    assert(!fed2.contains(pt, dim));
                    assert(!test_isRealPointIn(pt, fed2, dim));
                }
                // pt in fed2 not in fed1
                if (test_generateRealPoint(pt, fed2))
                {
                    assert(!fed1.contains(pt, dim));
                    assert(!test_isRealPointIn(pt, fed1, dim));
                }
            }
        }
        if (fed4.isEmpty())
        {
            double pt[dim];
            for(int i = 0; i < 500; ++i)
            {
                // pt in fed1 not in fed2
                if (test_generateRealPoint(pt, fed1))
                {
                    assert(!dbm2.contains(pt, dim));
                }
                // pt in fed2 not in fed1
                if (!dbm2.isEmpty() && dbm_generateRealPoint(pt, dbm2(), dim))
                {
                    assert(!fed1.contains(pt, dim));
                    assert(!test_isRealPointIn(pt, fed1, dim));
                }
            }
        }
    }
}

// test &= contrain
static
void test_constrain(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        DBMFederation_t f1 = test_new2old(fed1);
        constraint_t constraints[3*dim];
        constraint_t onec;
        size_t nb = test_genConstraints(fed1, 3*dim, constraints);
        size_t n1 = test_genConstraints(fed1, 1, &onec);
        if (n1) fed1 &= onec;
        bool notEmpty = fed1.constrain(constraints, nb);
        assert(notEmpty == !fed1.isEmpty());
        if (n1) dbmf_constrain1(&f1, onec.i, onec.j, onec.value, &factory);
        dbmf_constrainN(&f1, constraints, nb, &factory);
        assert(test_old2new(f1) == fed1);
        DEL_FED(f1);
        if (n1) assert(fed1.isEmpty() || (fed1 && onec));
        for(size_t i = 0; i < nb; ++i)
        {
            assert(fed1.isEmpty() || (fed1 && constraints[i]));
            for(fed_t::const_iterator iter(fed1); !iter.null(); ++iter)
            {
                assert(*iter && constraints[i]);
                if (n1) assert(*iter && onec);
            } 
        }
        assert(fed2.hash() == h);
        for(fed_t::iterator iter = fed2.beginMutable(); !iter.null(); ++iter)
        {
            iter->constrain(constraints, nb);
            if (n1) *iter &= onec;
        }
        fed2.removeEmpty(); // invariant of fed_t
        assert(fed1.hash() == fed2.hash());
        ASSERT(fed1 == fed2 && fed1 == fed2, cerr << fed1 << endl << fed2 << endl);
        fed1.reduce();
        test_mix(fed1);
        test_mix(fed2);
        assert(fed1 == fed2 && fed2 == fed1);
        assert(fed1.eq(fed2) && fed2.eq(fed1));
        assert(fed1.hash() == fed2.reduce().hash());
        if (CHEAP)
        {
            fed2.mergeReduce().expensiveReduce();
            assert(fed1.eq(fed2) && fed2.eq(fed1));
            assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
        }
    }
}

// test up
static
void test_up(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        DBMFederation_t f1 = test_new2old(fed1);
        uint32_t h = fed1.hash();
        assert(fed2 <= fed1.up());
        dbmf_up(f1);
        assert(test_old2new(f1) == fed1);
        DEL_FED(f1);
        assert(dim == 0 || fed1.isUnbounded());
        assert(fed2.hash() == h);
        for(fed_t::iterator iter2 = fed2.beginMutable(); !iter2.null(); ++iter2)
        {
            iter2->up();
        }
        assert(fed1 == fed2 && fed2 == fed1);
        fed1.reduce();
        assert(fed1 == fed2 && fed2 == fed1);
        assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
        assert(fed1.hash() == fed2.reduce().hash());
        if (CHEAP)
        {
            fed2.mergeReduce().expensiveReduce();
            assert(fed1.eq(fed2) && fed2.eq(fed1));
            assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
        }
    }
}

// test down
static
void test_down(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        DBMFederation_t f1 = test_new2old(fed1);
        uint32_t h = fed1.hash();
        assert(fed2 <= fed1.down());
        dbmf_down(f1);
        assert(test_old2new(f1) == fed1);
        DEL_FED(f1);
        assert(fed2.hash() == h);
        for(fed_t::iterator iter2 = fed2.beginMutable(); !iter2.null(); ++iter2)
        {
            iter2->down();
        }
        assert(fed1 == fed2 && fed2 == fed1);
        fed1.reduce();
        assert(fed1 == fed2 && fed2 == fed1);
        assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
        assert(fed1.hash() == fed2.reduce().hash());
        if (CHEAP)
        {
            fed2.mergeReduce().expensiveReduce();
            assert(fed1.eq(fed2) && fed2.eq(fed1));
            assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
        }
    }
}

// test freeClock
void test_freeClock(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            DBMFederation_t f1 = test_new2old(fed1);
            uint32_t h = fed1.hash();
            cindex_t c = rand()%(dim-1)+1; // not ref clock
            assert(fed2 <= fed1.freeClock(c));
            dbmf_freeClock(f1, c);
            assert(test_old2new(f1) == fed1);
            DEL_FED(f1);
            assert(fed2.hash() == h);
            for(fed_t::iterator iter2 = fed2.beginMutable(); !iter2.null(); ++iter2)
            {
                iter2->freeClock(c);
            }
            assert(fed1 == fed2 && fed2 == fed1);
            fed1.reduce();
            assert(fed1 == fed2 && fed2 == fed1);
            assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
            assert(fed1.hash() == fed2.reduce().hash());
            if (CHEAP)
            {
                fed2.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed2) && fed2.eq(fed1));
                assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
            }
        }
    }
}

// test updateValue
static
void test_updateValue(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            fed_t fed3 = fed1;
            DBMFederation_t f1 = test_new2old(fed1);
            uint32_t h = fed1.hash();
            cindex_t c = rand()%(dim-1)+1;
            int32_t v = rand()%100;
            fed1.updateValue(c, v);
            fed2(c) = v;
            dbmf_updateValue(f1, c, v);
            assert(test_old2new(f1) == fed1);
            DEL_FED(f1);
            assert(fed1.hash() == fed2.hash());
            assert(fed3.hash() == h);
            assert(fed1 == fed2 && fed2 == fed1);
            h = fed1.reduce().hash();
            assert(fed1 == fed2 && fed2 == fed1);
            assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
            assert(h == fed2.reduce().hash());
            if (CHEAP)
            {
                fed2.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed2) && fed2.eq(fed1));
                assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
            }
            for(fed_t::iterator iter = fed3.beginMutable(); !iter.null(); ++iter)
            {
                iter->updateValue(c, v);
            }
            assert(h == fed3.reduce().hash());
            assert(fed1 == fed3 && fed3 == fed1);
            assert(fed1.eq(fed3) && fed3.eq(fed1) && fed1.le(fed3) && fed1.ge(fed3));
            if (CHEAP)
            {
                fed3.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed3) && fed3.eq(fed1));
                assert((fed1 - fed3).isEmpty() && (fed3 - fed1).isEmpty());
            }
        }
    }
}

// test updateClock
static
void test_updateClock(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            fed_t fed3 = fed1;
            DBMFederation_t f1 = test_new2old(fed1);
            uint32_t h = fed1.hash();
            cindex_t c1 = rand()%(dim-1)+1;
            cindex_t c2 = rand()%(dim-1)+1;
            fed1.updateClock(c1, c2);
            fed2(c1) = fed2(c2);
            dbmf_updateClock(f1, c1, c2);
            assert(test_old2new(f1) == fed1);
            DEL_FED(f1);
            assert(fed3.hash() == h);
            assert(fed1 == fed2 && fed2 == fed1);
            assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
            h = fed1.reduce().hash();
            assert(fed1 == fed2 && fed2 == fed1);
            assert(h == fed2.reduce().hash());
            if (CHEAP)
            {
                fed2.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed2) && fed2.eq(fed1));
                assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
            }
            for(fed_t::iterator iter = fed3.beginMutable(); !iter.null(); ++iter)
            {
                iter->updateClock(c1, c2);
            }
            assert(fed1 == fed3 && fed3 == fed1);
            assert(fed1.eq(fed3) && fed3.eq(fed1) && fed1.le(fed3) && fed1.ge(fed3));
            assert(c1 == c2 || h == fed3.reduce().hash());
            if (CHEAP)
            {
                fed3.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed3) && fed3.eq(fed1));
                assert((fed1 - fed3).isEmpty() && (fed3 - fed1).isEmpty());
            }
        }
    }
}

// test updateIncrement
static
void test_updateIncrement(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            fed_t fed3 = fed1;
            DBMFederation_t f1 = test_new2old(fed1);
            uint32_t h = fed1.hash();
            cindex_t c = rand()%(dim-1)+1;
            cindex_t v = rand()%50;
            fed1.updateIncrement(c, v);
            fed2(c) += v;
            dbmf_updateIncrement(f1, c, v);
            assert(test_old2new(f1) == fed1);
            DEL_FED(f1);
            assert(fed3.hash() == h);
            assert(fed1 == fed2 && fed2 == fed1);
            assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
            h = fed1.reduce().hash();
            assert(fed1 == fed2 && fed2 == fed1);
            assert(h == fed2.reduce().hash());
            if (CHEAP)
            {
                fed2.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed2) && fed2.eq(fed1));
                assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
            }
            for(fed_t::iterator iter = fed3.beginMutable(); !iter.null(); ++iter)
            {
                iter->updateIncrement(c, v);
            }
            assert(fed1 == fed3 && fed3 == fed1);
            assert(fed1.eq(fed3) && fed3.eq(fed1) && fed1.le(fed3) && fed1.ge(fed3));
            assert(v == 0 || h == fed3.reduce().hash());
            if (CHEAP)
            {
                fed3.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed3) && fed3.eq(fed1));
                assert((fed1 - fed3).isEmpty() && (fed3 - fed1).isEmpty());
            }
        }
    }
}

// test update
static
void test_update(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else // dim > 1 pre-condition
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            fed_t fed3 = fed1;
            DBMFederation_t f1 = test_new2old(fed1);
            uint32_t h = fed1.hash();
            cindex_t c1 = rand()%(dim-1)+1;
            cindex_t c2 = rand()%(dim-1)+1;
            cindex_t v = rand()%50;
            fed1.update(c1, c2, v);
            fed2(c1) = fed2(c2) + v;
            dbmf_update(f1, c1, c2, v);
            assert(test_old2new(f1) == fed1);
            DEL_FED(f1);
            assert(fed3.hash() == h);
            assert(fed1 == fed2 && fed2 == fed1);
            assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
            h = fed1.reduce().hash();
            assert(fed1 == fed2 && fed2 == fed1);
            assert(h == fed2.reduce().hash());
            if (CHEAP)
            {
                fed2.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed2) && fed2.eq(fed1));
                assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
            }
            for(fed_t::iterator iter = fed3.beginMutable(); !iter.null(); ++iter)
            {
                iter->update(c1, c2, v);
            }
            assert(fed1 == fed3 && fed3 == fed1);
            assert(fed1.eq(fed3) && fed3.eq(fed1) && fed1.le(fed3) && fed1.ge(fed3));
            assert((c1 == c2 && v == 0) || h == fed3.reduce().hash());
            if (CHEAP)
            {
                fed3.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed3) && fed3.eq(fed1));
                assert((fed1 - fed3).isEmpty() && (fed3 - fed1).isEmpty());
            }
        }
    }
}

// test satisfies
static
void test_satisfies(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed(test_gen(dim, size));
            DBMFederation_t f = test_new2old(fed);
            int32_t pt[dim];
            for(int c = 0; c < 30; ++c)
            {
                if (test_generatePoint(pt, fed))
                {
                    // all the constraints derived from pt are satisfied
                    for(cindex_t i = 0; i < dim; ++i)
                    {
                        for(cindex_t j = 0; j < dim; ++j) if (i != j)
                        {
                            raw_t val = dbm_bound2raw(pt[i]-pt[j], dbm_WEAK);
                            assert(fed.satisfies(i, j, val));
                            assert(dbmf_satisfies(f, i, j, val)); 
                        }
                    }
                }
            }
            DEL_FED(f);
        }
    }
}

// test contrain and isEmpty
static
void test_constrainEmpty(cindex_t dim, size_t size)
{
    if (dim <= 1 || size == 0)
    {
        NO_TEST();
    }
    else
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed(test_gen(dim, size));
            int32_t pt[dim];
            cindex_t i, j;
            // dim > 1 & size > 0 -> possible to generate a non-empty fed
            while(fed.isEmpty()) fed = test_gen(dim, size);
            if (test_generatePoint(pt, fed))
            {
                assert(fed.contains(pt, dim));
                assert(test_isPointIn(pt, fed, dim));

                // constrain fed so that it contains only this point
                for(i = 0; i < dim; ++i)
                {
                    for(j = 0; j < dim; ++j) if (i != j)
                    {
                        assert(fed.constrain(i, j, pt[i]-pt[j], dbm_WEAK));
                        assert(!fed.isEmpty());
                    }
                }
                fed.reduce();
                ASSERT(fed.size() == 1, // after reduce when fed has only 1 point!
                       debug_cppPrintVector(cerr, pt, dim) << endl << fed << endl);
                // now empty the federation
                do {
                    i = rand()%dim;
                    j = rand()%dim;
                } while(i == j);
                ASSERT(!fed.constrain(i, j, pt[i]-pt[j], dbm_STRICT),
                       debug_cppPrintVector(cerr, pt, dim) << endl << fed << endl);
                assert(fed.isEmpty());
            }
        }
    }
}

// test isUnbounded
static
void test_isUnbounded(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed(test_gen(dim, size));
        DBMFederation_t f = test_new2old(fed);
        bool testu = false;
        for(fed_t::const_iterator iter(fed); !iter.null(); ++iter)
        {
            if (iter->isUnbounded())
            {
                testu = true;
                break;
            }
        }
        assert(testu == fed.isUnbounded());
        assert(testu == dbmf_isUnbounded(f));
        assert(dim == 0 || fed.up().isUnbounded());
        DEL_FED(f);
    }
}

// additional tests on relations (exact and approx.)
static
void test_relation(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        BOOL valid = TRUE;
        fed_t fed(test_gen(dim, size));
        DBMFederation_t f = test_new2old(fed);
        dbm_t dbm(dim);
        if (fed.size())
        {
            fed.reduce(); // otherwise the strict assertions are not valid
            dbmf_reduce(&f, &factory);
            assert(test_old2new(f) == fed);
            BOOL strict = dbm_generateSubset(dbm.getDBM(), test_getDBM(fed)(), dim);
            assert(dbm <= fed && dbm.le(fed));
            assert(dbmf_partialRelation(dbm(), f) & base_SUBSET);
            int rel = dbmf_relation(dbm(), f, &valid, &factory) & base_SUBSET;
            assert(!valid || rel);
            if (strict)
            {
                assert(dbm < fed && dbm.lt(fed));
                assert(dbmf_partialRelation(dbm(), f) == base_SUBSET);
                rel = dbmf_relation(dbm(), f, &valid, &factory);
                assert(!valid || rel == base_SUBSET);
            }
            dbm += fed; // convex union
            assert(dbm >= fed && dbm.ge(fed));
            assert(dbmf_partialRelation(dbm(), f) & base_SUPERSET);
            rel = dbmf_relation(dbm(), f, &valid, &factory);
            assert(!valid || (rel & base_SUPERSET));
            assert(dbm == fed || dbm > fed);
            rel = dbmf_partialRelation(dbm(), f);
            assert(rel == base_EQUAL || rel == base_SUPERSET);
            assert(dbm.eq(fed) || dbm.gt(fed));
        }
        DEL_FED(f);
    }
}

// test freeAllUp
static
void test_freeAllUp(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        assert(fed2 <= fed1.freeAllUp());
        assert(up(fed2) <= fed1);
        assert(dim == 0 || fed1.isUnbounded());
        assert(fed2.hash() == h);
        for(fed_t::iterator iter2 = fed2.beginMutable(); !iter2.null(); ++iter2)
        {
            iter2->freeAllUp();
        }
        assert(fed1 == fed2 && fed2 == fed1);
        fed1.reduce();
        assert(fed1 == fed2 && fed2 == fed1);
        assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
        assert(fed1.hash() == fed2.reduce().hash());
        if (CHEAP)
        {
            fed2.mergeReduce().expensiveReduce();
            assert(fed1.eq(fed2) && fed2.eq(fed1));
            assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
        }
    }
}

// test freeAllDown
static
void test_freeAllDown(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        assert(fed2 <= fed1.freeAllDown());
        assert(down(fed2) <= fed1);
        assert(fed2.hash() == h);
        for(fed_t::iterator iter2 = fed2.beginMutable(); !iter2.null(); ++iter2)
        {
            iter2->freeAllDown();
        }
        assert(fed1 == fed2 && fed2 == fed1);
        fed1.reduce();
        assert(fed1 == fed2 && fed2 == fed1);
        assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
        assert(fed1.hash() == fed2.reduce().hash());
        if (CHEAP)
        {
            fed2.mergeReduce().expensiveReduce();
            assert(fed1.eq(fed2) && fed2.eq(fed1));
            assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
        }
    }
}

// test freeDown
static
void test_freeDown(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c = rand()%(dim-1)+1; // not ref clock
            assert(fed2 <= fed1.freeDown(c));
            assert(fed2.hash() == h);
            for(fed_t::iterator iter2 = fed2.beginMutable(); !iter2.null(); ++iter2)
            {
                iter2->freeDown(c);
            }
            assert(fed1 == fed2 && fed2 == fed1);
            fed1.reduce();
            assert(fed1 == fed2 && fed2 == fed1);
            assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
            assert(fed1.hash() == fed2.reduce().hash());
            if (CHEAP)
            {
                fed2.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed2) && fed2.eq(fed1));
                assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
            }
        }
    }
}

// test freeUp
static
void test_freeUp(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            fed_t fed2 = fed1;
            uint32_t h = fed1.hash();
            cindex_t c = rand()%(dim-1)+1; // not ref clock
            assert(fed2 <= fed1.freeUp(c));
            assert(fed2.hash() == h);
            for(fed_t::iterator iter2 = fed2.beginMutable(); !iter2.null(); ++iter2)
            {
                iter2->freeUp(c);
            }
            assert(fed1 == fed2 && fed2 == fed1);
            fed1.reduce();
            assert(fed1 == fed2 && fed2 == fed1);
            assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
            assert(fed1.hash() == fed2.reduce().hash());
            if (CHEAP)
            {
                fed2.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed2) && fed2.eq(fed1));
                assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
            }
        }
    }
}

// test relaxUp
static
void test_relaxUp(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        fed_t fed1(test_gen(dim, size));
        fed_t fed2 = fed1;
        uint32_t h = fed1.hash();
        assert(fed2 <= fed1.relaxUp());
        assert(fed2.hash() == h);
        for(fed_t::iterator iter2 = fed2.beginMutable(); !iter2.null(); ++iter2)
        {
            iter2->relaxUp();
        }
        assert(fed1 == fed2 && fed2 == fed1);
        fed1.reduce();
        assert(fed1 == fed2 && fed2 == fed1);
        assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
        assert(fed1.hash() == fed2.reduce().hash());
        if (CHEAP)
        {
            fed2.mergeReduce().expensiveReduce();
            assert(fed1.eq(fed2) && fed2.eq(fed1));
            assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
        }
    }
}

// additional tests for ==
static
void test_equal(cindex_t dim, size_t size)
{
    if (dim <= 1)
    {
        NO_TEST();
    }
    else
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t fed1(test_gen(dim, size));
            int32_t pt[dim];
            if (test_generatePoint(pt, fed1))
            {
                assert(fed1.contains(pt, dim));
                assert(test_isPointIn(pt, fed1, dim));
                cindex_t i, j;
                do {
                    i = rand()%dim;
                    j = rand()%dim;
                } while(i == j); // will terminate if dim > 1
                fed_t fed2 = fed1;
                fed2.constrain(i, j, pt[i]-pt[j], dbm_STRICT);
                assert(!test_isPointIn(pt, fed2, dim));
                assert(!fed2.contains(pt, dim));
                assert(fed2 < fed1 && fed1 > fed2 && fed1 != fed2);
            }
            fed_t fed2 = fed1;
            test_addDBMs(fed2, size);
            assert(fed2 >= fed1);
            fed1 = fed2;
            assert(fed1 == fed2 && fed2 == fed1);
            test_mix(fed1);
            assert(fed1 == fed2 && fed2 == fed1);
            fed1.reduce();
            assert(fed1 == fed2 && fed2 == fed1);
            assert(fed1.eq(fed2) && fed2.eq(fed1) && fed1.le(fed2) && fed1.ge(fed2));
            assert(fed1.hash() == fed2.reduce().hash());
            if (CHEAP)
            {
                fed2.mergeReduce().expensiveReduce();
                assert(fed1.eq(fed2) && fed2.eq(fed1));
                assert((fed1 - fed2).isEmpty() && (fed2 - fed1).isEmpty());
            }
        }
    }
}

// test subtractions
static
void test_subtract(cindex_t dim, size_t size)
{
    SHOW_TEST();
    uint32_t k;
    for(k = 0; k < NB_LOOPS; ++k)
    {
        PROGRESS();
        BOOL valid = TRUE;
        fed_t fed1(test_gen(dim, size));
        fed_t fed2(test_genArg(size, fed1));
        fed_t fed3 = fed1;
        uint32_t h1 = fed1.hash();
        uint32_t h2 = fed2.hash();
        fed_t fed4 = fed1 & fed2;
        double pt[dim];
        DBMFederation_t f1 = test_new2old(fed1);
        DBMFederation_t f2 = test_new2old(fed2);
        NEW_FED(f3,dim);
        DBMFederation_t f4 = test_new2old(fed4);
        assert(fed4.le(fed1) && fed4.le(fed2));
        assert((fed4 - fed1).isEmpty());
        assert((fed4 - fed2).isEmpty());
        f3 = dbmf_substractFederationFromFederation(f4, f1, &valid, &factory);
        if (valid)
        {
            assert(test_old2new(f3).eq(fed4 - fed1));
        }
        DEL_FED(f3);
        f3 = dbmf_substractFederationFromFederation(f4, f2, &valid, &factory);
        if (valid)
        {
            assert(test_old2new(f3).eq(fed4 - fed2));
        }
        DEL_FED(f3);
        fed_t s12 = fed1 - fed2;
        fed_t s14 = fed1 - fed4;
        f3 = dbmf_substractFederationFromFederation(f1, f2, &valid, &factory);
        if (valid)
        {
            assert(test_old2new(f3).eq(s12));
        }
        DEL_FED(f3);
        f3 = dbmf_substractFederationFromFederation(f1, f4, &valid, &factory);
        if (valid)
        {
            fed_t f = test_old2new(f3);
            ASSERT(f.eq(s14), cerr<<(f-s14)<<endl<<(s14-f)<<endl);
        }
        DEL_FED(f3);
        ASSERT(s12.eq(s14), cerr<<s12.reduce()<<endl<<s14.reduce()<<endl);
        fed_t s21 = fed2 - fed1;
        fed_t s24 = fed2 - fed4;
        f3 = dbmf_substractFederationFromFederation(f2, f1, &valid, &factory);
        if (valid)
        {
            fed_t f = test_old2new(f3);
            ASSERT(f.eq(s21), cerr<<(f-s21)<<endl<<(s21-f)<<endl);
        }
        DEL_FED(f3);
        f3 = dbmf_substractFederationFromFederation(f2, f4, &valid, &factory);
        if (valid)
        {
            fed_t f = test_old2new(f3);
            ASSERT(f.eq(s24), cerr<<(f-s24)<<endl<<(s24-f)<<endl);
        }
        DEL_FED(f3);
        DEL_FED(f1);
        DEL_FED(f2);
        DEL_FED(f4);
        ASSERT(s21.eq(s24), cerr<<s21.reduce()<<endl<<s24.reduce()<<endl);
        fed_t u1 = fed1 | fed2;
        fed_t u2 = s12 | s21 | fed4;
        ASSERT(u1.eq(u2), cerr<<u1.reduce()<<endl<<u2.reduce()<<endl);
        s12.nil();
        s14.nil();
        s21.nil();
        s24.nil();
        u1.nil();
        u2.nil();
        assert(fed1.hash() == h1);
        assert(fed2.hash() == h2);
        fed1 -= fed2;
        assert(fed3.hash() == h1);
        for(fed_t::const_iterator iter(fed2); !iter.null(); ++iter)
        {
            fed3 -= *iter;
        }
        assert(fed3.eq(fed1));
        for(int count = 0; count < 500; ++count)
        {
            if (test_generateRealPoint(pt, fed1)) // points in fed1 not in fed2
            {
                assert(test_isRealPointIn(pt, fed1, dim));
                assert(fed1.contains(pt, dim));
                assert(!test_isRealPointIn(pt, fed2, dim));
                assert(!fed2.contains(pt, dim));
            }
            if (test_generateRealPoint(pt, fed2)) // points in fed2 not in fed1
            {
                assert(test_isRealPointIn(pt, fed2, dim));
                assert(fed2.contains(pt, dim));
                assert(!test_isRealPointIn(pt, fed1, dim));
                assert(!fed1.contains(pt, dim));
            }
            if (test_generateRealPoint(pt, fed4)) // points in fed4 not in fed1
            {
                assert(test_isRealPointIn(pt, fed4, dim));
                assert(fed4.contains(pt, dim));
                assert(!test_isRealPointIn(pt, fed1, dim));
                assert(!fed1.contains(pt, dim));
            }
        }
    }
}

// test predt
static
void test_predt(cindex_t dim, size_t size)
{
    if (dim < 1)
    {
        NO_TEST();
    }
    else
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t good(test_gen(dim, size));
            fed_t bad(test_genArg(size, good));
            uint32_t hg = good.hash();
            uint32_t hb = bad.hash();
            fed_t p = predt(good, bad);
            /* predt of DBM v1.x is known to be wrong
            DBMFederation_t fg = test_new2old(good);
            DBMFederation_t fb = test_new2old(bad);
            if (!dbmf_predt(&fg, &fb, &factory))
            {
                ASSERT(test_old2new(fg).eq(p), cerr<<test_old2new(fg).reduce()<<endl<<p.reduce()<<endl);
            }
            DEL_FED(fg);
            DEL_FED(fb);
            */
            assert(good.hash() == hg);
            assert(bad.hash() == hb);
            assert((p & bad).isEmpty());
            assert(good.le(bad) || !(p & good).isEmpty());
            assert(p.le(down(good)-bad));
            assert(!bad.isEmpty() || p.eq(down(good)));
        }
    }
}

// test different reduce methods.
static
void test_reduce(cindex_t dim, size_t size)
{
    if (dim < 1)
    {
        NO_TEST();
    }
    else
    {
        SHOW_TEST();
        uint32_t k;
        for(k = 0; k < NB_LOOPS; ++k)
        {
            PROGRESS();
            fed_t f1(test_gen(dim, size));
            fed_t f2(test_genArg(size, f1));
            f1.append(f2);

            (f2 = f1).setMutable();
            assert(f2.reduce().eq(f1));

            (f2 = f1).setMutable();
            assert(f2.expensiveReduce().eq(f1));
            
            (f2 = f1).setMutable();
            assert(f2.mergeReduce().eq(f1));

            (f2 = f1).setMutable();
            assert(f2.convexReduce().eq(f1));

            (f2 = f1).setMutable();
            assert(f2.expensiveConvexReduce().eq(f1));

            (f2 = f1).setMutable();
            assert(f2.partitionReduce().eq(f1));
        }
    }
}

void test(int dim, int size)
{
    test_setZero(dim);
    test_setInit(dim);
    test_copy(dim, size);
    test_union(dim, size);
    test_convexUnion(dim, size);
    test_intersection(dim, size);
    test_constrain(dim, size);
    test_up(dim, size);
    test_down(dim, size);
    test_freeClock(dim, size);
    test_updateValue(dim, size);
    test_updateClock(dim, size);
    test_updateIncrement(dim, size);
    test_update(dim, size);
    test_satisfies(dim, size);
    test_constrainEmpty(dim, size);
    test_isUnbounded(dim, size);
    test_relation(dim, size);
    test_freeUp(dim, size);
    test_freeAllUp(dim, size);
    test_freeDown(dim, size);
    test_freeAllDown(dim, size);
    test_relaxUp(dim, size);
    test_equal(dim, size);
    test_subtract(dim, size);
    test_predt(dim, size);
    test_reduce(dim, size);
}

int main(int argc, char *argv[])
{
    int start, end, size, seed;

    if (argc < 4)
    {
        cerr << "Usage: " << argv[0]
             << " start_dim end_dim fed_size [seed]\n";
        return 1;
    }

    start = atoi(argv[1]);
    end = atoi(argv[2]);
    size = atoi(argv[3]);
    seed = argc > 4 ? atoi(argv[4]) : time(NULL);
    srand(seed);
    if (size < 1)
    {
        cerr << "Minimum size=1 taken\n";
        size = 1;
    }
    if (start < 1)
    {
        cerr << "Minimum dimension=1 taken\n";
        start = 1;
    }
    
    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    cout << "Test with seed=" << seed << endl;

    for(int i = start ; i <= end ; ++i)
    {
        dbmf_initFactory(&factory, i);
        cout << i << ": ";
        test(i, size);
        (cout << " \n").flush();
        if (rand() & 1) cleanUp();
        dbmf_clearFactory(&factory);
    }

    cout << "Passed\n";
    return 0;
}

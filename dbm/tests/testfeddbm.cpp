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
// Filename : testfeddbm.cpp
//
// Test dbm_t (fed.h)
//
// This file is a part of the UPPAAL toolkit.
// Copyright (c) 1995 - 2003, Uppsala University and Aalborg University.
// All right reserved.
//
// $Id: testfeddbm.cpp,v 1.8 2005/05/31 19:35:14 adavid Exp $
//
///////////////////////////////////////////////////////////////////

// Tests are always for debugging.

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <time.h>
#include "dbm/fed.h"
#include "dbm/print.h"
#include "dbm/gen.h"
#include "debug/utils.h"
#include "debug/macros.h"
#include "debug/new.h"

using namespace std;
using namespace dbm;
using namespace base;

// Range for DBM generation
#define MAXRANGE 10000
#define RANGE() ((rand()%MAXRANGE)+10)

// Progress
#define PROGRESS() debug_spin(stderr)

// DBM generation
#define GEN(D) dbm_generate(D, dim, RANGE())

// New DBM
#define NEW(N) raw_t *N = new raw_t[dim*dim]

// Delete DBM
#define FREE(N) delete [] N

// Check pointer
#define POINTER(D) assert(D.isEmpty() == (D() == NULL))

// Begin test loop
#define BEGIN for(int kLoop = 0; kLoop < 23; ++kLoop) {

// End test loop
#define END }

// Invalidate matrix
#define INVALIDATE(D) debug_randomize(D, dim*dim)

// Matrix access
#define DBM(I,J) dbm[(I)*dim+(J)]

// Equality
#define EQ(A,B) (A == B && A.relation(B) == base_EQUAL)
#define EQD(A,B) (A == B && A.relation(B, dim) == base_EQUAL)

// Not equality
#define NEQ(A,B) (A != B && A.relation(B) != base_EQUAL)
#define NEQD(A,B) (A != B && A.relation(B, dim) != base_EQUAL)

void test(cindex_t dim)
{
    NEW(dbm);
    NEW(dbm2);
    constraint_t *cnstr = new constraint_t[dim*dim];
    int32_t lower[dim], upper[dim];
    IntValuation pt(dim);
    // coverage
    bool c1 = false, c2 = false, c4 = false;
    bool c5 = false, c6 = false, c7 = false;
    BEGIN;
    dbm_t a;          // default
    dbm_t b(dim);     // dim
    dbm_t c(a), d(b); // copy
    cindex_t i,j,k;
    bool ab_equal = false;
    const raw_t *ptr;
    PROGRESS();

    // constructor post-conditions
    assert(a.isEmpty() && a.getDimension() == 1);
    assert(EQ(a,c) && EQ(b,d));
    assert(b.getDimension() == dim && d.getDimension() == dim);
    assert(c.isEmpty() && c.getDimension() == 1);
    assert(b.isEmpty() && d.isEmpty());
    POINTER(a);
    POINTER(b);
    POINTER(c);
    POINTER(d);

    // check copy
    GEN(dbm);
    a = c = dbm;
    // copy -> only diagonal ok -> mark as non empty
    assert(!a.isEmpty() && a.getDimension() == 1);
    assert(!c.isEmpty() && c.getDimension() == 1);
    c.copyFrom(dbm, dim); // copy succeeds: change dim
    a = c;
    b = dbm;
    d.copyFrom(dbm, dim);
#if 0 // checked
    if (kLoop == 0 && dim < 10)
    {
        cout << "a=\n" << a
             << "b=\n" << b
             << "c=\n" << c
             << "d=\n" << d;
    }
#endif
    assert(EQ(a,b) && a() == c() && EQ(a,d) && EQD(a,dbm));
    assert(EQ(b,c) && EQ(b,d) && EQ(c,d) && EQD(b,dbm));
    a.nil();
    assert(a.isEmpty());
    a.setDimension(dim);
    a = c;
    assert(a() == c() && a.getDimension() == c.getDimension() && a.getDimension() == dim);
    assert(a() && b() && c() && d());
    POINTER(a);
    POINTER(b);
    POINTER(c);
    POINTER(d);
    assert(EQD(a,dbm));
    INVALIDATE(dbm);
    a.copyTo(dbm, dim);
    assert(*dbm == dbm_LE_ZERO); // always
    assert(EQD(a,dbm));
    // check copy with constraint access
    if (!a.isEmpty()) // pre-condition
        for(i = 0; i < dim; ++i)
            for(j = 0; j < dim; ++j)
                assert(a(i,j) == DBM(i,j));
    // check !=
    GEN(dbm);
    if (a == dbm)
    {
        // very rare branch, don't check
        assert(a.relation(dbm, dim) == base_EQUAL);
        if (dim > 1) assert(dbm_areEqual(dbm, a(), dim));
        assert(EQD(b,dbm) && EQD(c,dbm) && EQD(d,dbm));
    }
    else
    {
        c1 = true;
        assert(a.relation(dbm, dim) != base_EQUAL);
        assert(dim > 1 && !dbm_areEqual(dbm, a(), dim));
        assert(NEQD(a,dbm) && NEQD(b,dbm) && NEQD(c,dbm) && NEQD(d,dbm));
    }

    // check relation
    a.copyTo(dbm, dim);
    assert(*dbm == dbm_LE_ZERO); // always
    assert(EQD(a,dbm));
    if (rand()%10 > 0) dbm_generateSuperset(dbm, dbm, dim);
    b = dbm;
    assert(a <= dbm && c <= dbm);
    assert(a <= b && c <= b);
    assert(b >= a && b >= c);
    if (a != b)
    {
        ab_equal = false;
        c2 = true;
        assert(a < dbm && c <dbm);
        assert(a < b && c < b);
        assert(b > a && b > c);
    }

    // zero
    dbm_zero(dbm, dim);
    a.setZero();
    d = a;
    a.setZero();
    assert(a() == d());
    d.nil();
    assert(a == dbm && a.isZero());
    
    // init
    dbm_init(dbm, dim);
    a.setInit();
    assert(a == dbm && a.isInit());

    // convex union, recall: c==d <= b
    if (kLoop & 1)
    {
        a = c;
        assert(a <= b);
        a += b; // a not mutable => get b
        assert(a == b);
        d = a;
        a += b; // no effect
        assert(a() == d());
        d.nil();
    }
    else
    {
        assert(c <= b);
        a = c + b; // c not mutable => +b returns b
        assert(a == b);
    }
    if (ab_equal) assert(a() == b()); // stronger than a == b
    // else a += b returned &a and not &b
    c += b;
    d = c;
    c += b; // no effect
    assert(c() == d());
    d.nil();
    assert(dim <= 1 || c() != b());
    assert(EQ(a,b));
    assert(EQ(c,b));
    a.intern();
    b.intern();
    c.intern();
    assert(a() == b() && b() == c());
    GEN(dbm);
    b = dbm;
    // basic inclusion property of convex union
    c = a + b;
    assert(a <= c && c >= b);
    assert(a == c || a < c);
    assert(b == c || c > b);
    // duplicate check to test garbage collection
    assert(a <= a + b && a + b >= b);
    
    // intersection
    GEN(dbm);
    a = dbm;
    b.copyTo(dbm2, dim);
    bool i1 = dbm_haveIntersection(dbm2, dbm, dim);
    bool i2 = dbm_intersection(dbm2, dbm, dim);
    assert(i1 || !i2);
    assert(dbm_isEmpty(dbm2, dim) || (dbm_relation(dbm2, dbm, dim) & base_SUBSET));
    assert((a & b) <= a && (a & b) <= b); // ok with empty set
    assert((a & a) == a && (b & b) == b);
    assert((a & (a + b)) == a && (b & (a + b)) == b);
    k = rand()%dim;
    if (k)
    {
        c = a(k) + 1; // clock increment
        d = c & a;
        assert(d <= c && d <= a);
        c4 = c4 || !d.isEmpty();
    }

    // constrain
    assert(a == dbm);
    for(k = 7; k > 0 && !dbm_generatePoint(pt.begin(), dbm, dim); --k);
    if (k > 0)
    {
        bool stop = rand() & 1;
        c5 = true;
        b = a;
        assert(b.contains(pt));
        for(i = 0, k = 0; i < dim; ++i)
        {
            for(j = 0; j < dim; ++j)
            {
                if (i != j)
                {
                    cnstr[k] = constraint_t(i, j, dbm_bound2raw(pt[i]-pt[j], dbm_WEAK));
                    assert(a && cnstr[k]); // satisfies
                    a &= cnstr[k];
                    c = a;
                    a &= cnstr[k];
                    assert(a() == c());
                    c.nil();
                    assert(dim <= 1 || !a.isEmpty());
                    k++;
                }
            }
            if (stop && (rand() & 1)) break;
        }
        b &= pointer_t<constraint_t>(cnstr, k);
        assert(a == b);
        assert(dbm_constrainN(dbm, dim, cnstr, k));
        assert(a == dbm);
        c = b;
        b &= pointer_t<constraint_t>(cnstr, k);
        assert(b() == c());
        c.nil();
        assert(a.contains(pt));
        if (!stop && dim > 1 && k > 0)
        {
            c6 = true;
            assert(!a.isUnbounded());
            i = rand()%(dim-1)+1;
            j = rand()%dim;
            if (i == j) j = 0; // i != 0, ok
            if (rand() & 1)
            {
                k = i;
                i = j;
                j = k;
            }
            // constrain too much now
            constraint_t cij(i, j, dbm_bound2raw(pt[i]-pt[j], dbm_STRICT));
            a &= cij;
            assert(a.isEmpty() && !(b && cij));
        }
    }

    // up & down
    GEN(dbm);
    c.setDimension(dim); // because c.nil() before
    a = b = c = dbm;
    b.down();
    c.up();
    assert(a <= up(a) && a <= down(a));
    assert(b == down(a) && c == up(a) && a == dbm);
    assert(c.isUnbounded());
    dbm_up(dbm, dim);
    assert(c == dbm);
    a.copyTo(dbm, dim);
    dbm_down(dbm, dim);
    assert(b == dbm);
    
    // updates: only for dim > 1
    if (dim > 1)
    {
        GEN(a.getDBM());
        a.copyTo(dbm, dim);
        assert(dbm_isValid(dbm, dim));
        i = rand()%(dim-1)+1;
        // updateValue
        a(i) = 0;
        b = a;
        a(i) = 0;
        assert(a() == b());
        dbm_updateValue(dbm, dim, i, 0);
        assert(a == dbm);
        a(i) = 1;
        assert(a() != b()); // mutable copy used
        b = a;
        a(i) = 1;
        assert(a() == b());
        dbm_updateValue(dbm, dim, i, 1);
        assert(a == dbm);
        // other updates: need 2 clocks != 0 -> dim >= 3
        if (dim >= 3)
        {
            int32_t inc = rand()%100;
            b = a;
            a(i) = a(i); // no effect
            assert(a() == b());
            do { j = rand()%(dim-1)+1; } while(i == j);
            b.nil();
            // updateClock
            ptr = a();
            a(i) = a(j);
            assert(ptr == a());
            dbm_updateClock(dbm, dim, i, j);
            assert(a == dbm);
            b = a;
            a(i) = a(j) + 0;
            assert(a() == b());
            // updateIncrement
            ptr = a();
            a(i) += 0;
            assert(a() == ptr && a == b);
            a(i) += inc;
            dbm_updateIncrement(dbm, dim, i, inc);
            assert(a == dbm);
            // update
            ptr = a();
            a(i) = a(i) + 0;
            assert(a() == ptr);
            a(i) = a(j) + inc;
            dbm_update(dbm, dim, i, j, inc);
            assert(a == dbm);
        }
    }

    // freeUp, freeDown, relaxXX
    GEN(dbm);
    a = dbm;
    b = freeAllUp(a);
    ptr = b();
    b.freeAllUp();
    assert(ptr == b());
    dbm_freeAllUp(dbm, dim);
    assert(b == dbm);
    b = freeAllDown(a);
    ptr = b();
    b.freeAllDown();
    assert(ptr == b());
    a.copyTo(dbm, dim);
    dbm_freeAllDown(dbm, dim);
    assert(b == dbm);
    k = rand()%dim;
    if (k != 0)
    {
        c7 = true;
        c = freeDown(a, k);
        ptr = c();
        c.freeDown(k);
        assert(ptr == c());
        a.copyTo(dbm, dim);
        dbm_freeDown(dbm, dim, k);
        assert(c == dbm);
        c = freeUp(a, k);
        ptr = c();
        c.freeUp(k);
        assert(ptr == c());
        a.copyTo(dbm, dim);
        dbm_freeUp(dbm, dim, k);
        assert(c == dbm);
    }
    d = relaxUp(a);
    ptr = d();
    d.relaxUp();
    assert(ptr == d());
    a.copyTo(dbm, dim);
    dbm_relaxUp(dbm, dim);
    assert(d == dbm && a <= d);
    d = relaxDown(a);
    ptr = d();
    d.relaxDown();
    assert(ptr == d());
    a.copyTo(dbm, dim);
    dbm_relaxDown(dbm, dim);
    assert(d == dbm && a <= d);
    k = rand()%dim;
    // k == 0 ok
    d = relaxUpClock(a, k);
    ptr = d();
    d.relaxUpClock(k);
    assert(ptr == d());
    a.copyTo(dbm, dim);
    dbm_relaxUpClock(dbm, dim, k);
    assert(d == dbm && a <= d);
    d = relaxDownClock(a, k);
    ptr = d();
    d.relaxDownClock(k);
    assert(ptr == d());
    a.copyTo(dbm, dim);
    dbm_relaxDownClock(dbm, dim, k);
    assert(d == dbm && a <= d);

    // extrapolations
    // bounds 1st
    lower[0] = upper[0] = 0;
    for(k = 1; k < dim; ++k)
    {
        int32_t low = RANGE() - (MAXRANGE / 2);
        int32_t up  = RANGE() - (MAXRANGE / 2); /* low + RANGE()/2; */
        if (low < -2000)
        {
            lower[k] = upper[k] = -dbm_INFINITY;
        }
        else
        {
            lower[k] = low;
            upper[k] = up;
        }
    }
    for(k = 0; k < 4; ++k)
    {
        b = a;
        a.copyTo(dbm, dim);
        switch(k)
        {
        case 0:
            b.extrapolateMaxBounds(upper);
            dbm_extrapolateMaxBounds(dbm, dim, upper);
            break;
        case 1:
            b.diagonalExtrapolateMaxBounds(upper);
            dbm_diagonalExtrapolateMaxBounds(dbm, dim, upper);
            break;
        case 2:
            b.extrapolateLUBounds(lower, upper);
            dbm_extrapolateLUBounds(dbm, dim, lower, upper);
            break;
        case 3:
            b.diagonalExtrapolateLUBounds(lower, upper);
            dbm_diagonalExtrapolateLUBounds(dbm, dim, lower, upper);
            break;
        }
        assert(b == dbm && a <= b);
    }

    END;
    delete [] cnstr;
    FREE(dbm2);
    FREE(dbm);
    // meant to check branches
    //cout << c1 << c2 << c4 << c5 << c6 << c7 << ' ';
    PROGRESS();
}

int main(int argc, char *argv[])
{
    cindex_t start, end;
    int seed;

    if (argc < 3)
    {
        cerr << "Usage: " << argv[0]
             << " start end [seed]\n";
        return 1;
    }

    start = atoi(argv[1]);
    end = atoi(argv[2]);
    seed = argc > 3 ? atoi(argv[3]) : time(NULL);
    srand(seed);
    if (start < 1)
    {
        cerr << "Minimum dimension=1 taken\n";
        start = 1; // min dim == 1
    }

    /* Print the seed for the random generator
     * to be able to repeat a failed test.
     */
    cout << "Test with seed=" << seed << endl;

    for(cindex_t i = start ; i <= end ; ++i)
    {
        (cerr << '.').flush();
        for(int k = 0; k < 300; ++k)
        {
            test(i);
        }
    }
    cerr << endl;
    cout << "\nPassed\n";
    return 0;
}
